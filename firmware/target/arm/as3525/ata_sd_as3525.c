/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Daniel Ankers
 * Copyright © 2008-2009 Rafaël Carré
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Driver for the ARM PL180 SD/MMC controller inside AS3525 SoC */

/* TODO: Find the real capacity of >2GB models (will be useful for USB) */

#include "config.h" /* for HAVE_MULTIVOLUME & AMS_OF_SIZE */
#include "fat.h"
#include "thread.h"
#include "hotswap.h"
#include "system.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "as3525.h"
#include "pl180.h"  /* SD controller */
#include "pl081.h"  /* DMA controller */
#include "dma-target.h" /* DMA request lines */
#include "clock-target.h"
#ifdef HAVE_BUTTON_LIGHT
#include "backlight-target.h"
#endif
#include "stdbool.h"
#include "ata_idle_notify.h"
#include "sd.h"

#ifdef HAVE_HOTSWAP
#include "disk.h"
#endif

/* command flags */
#define MCI_NO_FLAGS    (0<<0)
#define MCI_RESP        (1<<0)
#define MCI_LONG_RESP   (1<<1)
#define MCI_ARG         (1<<2)

/* ARM PL180 registers */
#define MCI_POWER(i)       (*(volatile unsigned char *) (pl180_base[i]+0x00))
#define MCI_CLOCK(i)       (*(volatile unsigned long *) (pl180_base[i]+0x04))
#define MCI_ARGUMENT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x08))
#define MCI_COMMAND(i)     (*(volatile unsigned long *) (pl180_base[i]+0x0C))
#define MCI_RESPCMD(i)     (*(volatile unsigned long *) (pl180_base[i]+0x10))
#define MCI_RESP0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x14))
#define MCI_RESP1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x18))
#define MCI_RESP2(i)       (*(volatile unsigned long *) (pl180_base[i]+0x1C))
#define MCI_RESP3(i)       (*(volatile unsigned long *) (pl180_base[i]+0x20))
#define MCI_DATA_TIMER(i)  (*(volatile unsigned long *) (pl180_base[i]+0x24))
#define MCI_DATA_LENGTH(i) (*(volatile unsigned short*) (pl180_base[i]+0x28))
#define MCI_DATA_CTRL(i)   (*(volatile unsigned char *) (pl180_base[i]+0x2C))
#define MCI_DATA_CNT(i)    (*(volatile unsigned short*) (pl180_base[i]+0x30))
#define MCI_STATUS(i)      (*(volatile unsigned long *) (pl180_base[i]+0x34))
#define MCI_CLEAR(i)       (*(volatile unsigned long *) (pl180_base[i]+0x38))
#define MCI_MASK0(i)       (*(volatile unsigned long *) (pl180_base[i]+0x3C))
#define MCI_MASK1(i)       (*(volatile unsigned long *) (pl180_base[i]+0x40))
#define MCI_SELECT(i)      (*(volatile unsigned long *) (pl180_base[i]+0x44))
#define MCI_FIFO_CNT(i)    (*(volatile unsigned long *) (pl180_base[i]+0x48))

#define MCI_ERROR \
    (MCI_DATA_CRC_FAIL | MCI_DATA_TIMEOUT | MCI_RX_OVERRUN | MCI_TX_UNDERRUN)

#define MCI_FIFO(i)        ((unsigned long *) (pl180_base[i]+0x80))
/* volumes */
#define     INTERNAL_AS3525 0   /* embedded SD card */
#define     SD_SLOT_AS3525   1   /* SD slot if present */

static const int pl180_base[NUM_VOLUMES] = {
            NAND_FLASH_BASE
#ifdef HAVE_MULTIVOLUME
            , SD_MCI_BASE
#endif
};

static int sd_select_bank(signed char bank);
static int sd_init_card(const int drive);
static void init_pl180_controller(const int drive);
/* TODO : BLOCK_SIZE != SECTOR_SIZE ? */
#define BLOCK_SIZE      512
#define SECTOR_SIZE     512
#define BLOCKS_PER_BANK 0x7a7800

static tSDCardInfo card_info[NUM_VOLUMES];

/* maximum timeouts recommanded in the SD Specification v2.00 */
#define SD_MAX_READ_TIMEOUT     ((AS3525_PCLK_FREQ) / 1000 * 100) /* 100 ms */
#define SD_MAX_WRITE_TIMEOUT    ((AS3525_PCLK_FREQ) / 1000 * 250) /* 250 ms */

/* for compatibility */
static long last_disk_activity = -1;

#define MIN_YIELD_PERIOD 5  /* ticks */
static long next_yield = 0;

static long sd_stack [(DEFAULT_STACK_SIZE*2 + 0x200)/sizeof(long)];
static const char         sd_thread_name[] = "ata/sd";
static struct mutex       sd_mtx;
static struct event_queue sd_queue;
#ifndef BOOTLOADER
static bool sd_enabled = false;
#endif

static struct wakeup transfer_completion_signal;
static volatile bool retry;

static inline void mci_delay(void) { int i = 0xffff; while(i--) ; }

#ifdef HAVE_HOTSWAP
#if defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2)
static int sd1_oneshot_callback(struct timeout *tmo)
{
    (void)tmo;

    /* This is called only if the state was stable for 300ms - check state
     * and post appropriate event. */
    if (card_detect_target())
    {
        queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
    }
    else
        queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);

    return 0;
}

void INT_GPIOA(void)
{
    static struct timeout sd1_oneshot;
    /* reset irq */
    GPIOA_IC = (1<<2);
    timeout_register(&sd1_oneshot, sd1_oneshot_callback, (3*HZ/10), 0);
}
#endif  /* defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2) */
#endif  /* HAVE_HOTSWAP */

void INT_NAND(void)
{
    const int status = MCI_STATUS(INTERNAL_AS3525);

    if(status & MCI_ERROR)
        retry = true;

    wakeup_signal(&transfer_completion_signal);
    MCI_CLEAR(INTERNAL_AS3525) = status;
}

#ifdef HAVE_MULTIVOLUME
void INT_MCI0(void)
{
    const int status = MCI_STATUS(SD_SLOT_AS3525);

    if(status & MCI_ERROR)
        retry = true;

    wakeup_signal(&transfer_completion_signal);
    MCI_CLEAR(SD_SLOT_AS3525) = status;
}
#endif

static bool send_cmd(const int drive, const int cmd, const int arg,
                     const int flags, int *response)
{
    int val, status;

    while(MCI_STATUS(drive) & MCI_CMD_ACTIVE);

    if(MCI_COMMAND(drive) & MCI_COMMAND_ENABLE) /* clears existing command */
    {
        MCI_COMMAND(drive) = 0;
        mci_delay();
    }

    val = cmd | MCI_COMMAND_ENABLE;
    if(flags & MCI_RESP)
    {
        val |= MCI_COMMAND_RESPONSE;
        if(flags & MCI_LONG_RESP)
            val |= MCI_COMMAND_LONG_RESPONSE;
    }

    MCI_CLEAR(drive) = 0x7ff;

    MCI_ARGUMENT(drive) = (flags & MCI_ARG) ? arg : 0;
    MCI_COMMAND(drive) = val;

    while(MCI_STATUS(drive) & MCI_CMD_ACTIVE);  /* wait for cmd completion */

    MCI_COMMAND(drive) = 0;
    MCI_ARGUMENT(drive) = ~0;

    status = MCI_STATUS(drive);
    MCI_CLEAR(drive) = 0x7ff;

    if(flags & MCI_RESP)
    {
        if(status & MCI_CMD_TIMEOUT)
            return false;
        else if(status & (MCI_CMD_CRC_FAIL /* FIXME? */ | MCI_CMD_RESP_END))
        {   /* resp received */
            if(flags & MCI_LONG_RESP)
            {
                /* store the response in reverse words order */
                response[0] = MCI_RESP3(drive);
                response[1] = MCI_RESP2(drive);
                response[2] = MCI_RESP1(drive);
                response[3] = MCI_RESP0(drive);
            }
            else
                response[0] = MCI_RESP0(drive);
            return true;
        }
    }
    else if(status & MCI_CMD_SENT)
        return true;

    return false;
}

static int sd_init_card(const int drive)
{
    unsigned int  c_size;
    unsigned long c_mult;
    int response;
    int max_tries = 100; /* max acmd41 attemps */
    bool sdhc;

    if(!send_cmd(drive, SD_GO_IDLE_STATE, 0, MCI_NO_FLAGS, NULL))
        return -1;

    mci_delay();

    sdhc = false;
    if(send_cmd(drive, SD_SEND_IF_COND, 0x1AA, MCI_RESP|MCI_ARG, &response))
        if((response & 0xFFF) == 0x1AA)
            sdhc = true;

    do {
        /* some MicroSD cards seems to need more delays, so play safe */
        mci_delay();
        mci_delay();
        mci_delay();
        mci_delay();

        /* app_cmd */
        if( !send_cmd(drive, SD_APP_CMD, 0, MCI_RESP|MCI_ARG, &response) ||
            !(response & (1<<5)) )
        {
            return -2;
        }

        /* acmd41 */
        if(!send_cmd(drive, SD_APP_OP_COND, (sdhc ? 0x40FF8000 : (1<<23)),
                        MCI_RESP|MCI_ARG, &card_info[drive].ocr))
            return -3;

    } while(!(card_info[drive].ocr & (1<<31)) && max_tries--);

    if(max_tries < 0)
        return -4;

    /* send CID */
    if(!send_cmd(drive, SD_ALL_SEND_CID, 0, MCI_RESP|MCI_LONG_RESP|MCI_ARG,
                            card_info[drive].cid))
        return -5;

    /* send RCA */
    if(!send_cmd(drive, SD_SEND_RELATIVE_ADDR, 0, MCI_RESP|MCI_ARG,
                &card_info[drive].rca))
        return -6;

    /* send CSD */
    if(!send_cmd(drive, SD_SEND_CSD, card_info[drive].rca,
                 MCI_RESP|MCI_LONG_RESP|MCI_ARG, card_info[drive].csd))
        return -7;

    /* These calculations come from the Sandisk SD card product manual */
    if( (card_info[drive].csd[3]>>30) == 0)
    {
        /* CSD version 1.0 */
        c_size = ((card_info[drive].csd[2] & 0x3ff) << 2) + (card_info[drive].csd[1]>>30) + 1;
        c_mult = 4 << ((card_info[drive].csd[1] >> 15) & 7);
        card_info[drive].max_read_bl_len = 1 << ((card_info[drive].csd[2] >> 16) & 15);
        card_info[drive].block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        card_info[drive].numblocks = c_size * c_mult * (card_info[drive].max_read_bl_len/512);
        card_info[drive].capacity = card_info[drive].numblocks * card_info[drive].block_size;
    }
#ifdef HAVE_MULTIVOLUME
    else if( (card_info[drive].csd[3]>>30) == 1)
    {
        /* CSD version 2.0 */
        c_size = ((card_info[drive].csd[2] & 0x3f) << 16) + (card_info[drive].csd[1]>>16) + 1;
        card_info[drive].max_read_bl_len = 1 << ((card_info[drive].csd[2] >> 16) & 0xf);
        card_info[drive].block_size = BLOCK_SIZE;     /* Always use 512 byte blocks */
        card_info[drive].numblocks = c_size << 10;
        card_info[drive].capacity = card_info[drive].numblocks * card_info[drive].block_size;
    }
#endif

    if(!send_cmd(drive, SD_SELECT_CARD, card_info[drive].rca, MCI_ARG, NULL))
        return -9;

    if(!send_cmd(drive, SD_APP_CMD, card_info[drive].rca, MCI_ARG, NULL))
        return -10;

    if(!send_cmd(drive, SD_SET_BUS_WIDTH, card_info[drive].rca | 2, MCI_ARG, NULL))
        return -11;

    if(!send_cmd(drive, SD_SET_BLOCKLEN, card_info[drive].block_size, MCI_ARG,
                 NULL))
        return -12;

    card_info[drive].initialized = 1;

    MCI_CLOCK(drive) |= MCI_CLOCK_BYPASS; /* full speed for controller clock */
    mci_delay();

    /*
     * enable bank switching 
     * without issuing this command, we only have access to 1/4 of the blocks
     * of the first bank (0x1E9E00 blocks, which is the size reported in the
     * CSD register)
     */
    if(drive == INTERNAL_AS3525)
    {
        const int ret = sd_select_bank(-1);
        if(ret < 0)
            return ret - 13;
    }

    return 0;
}

static void sd_thread(void) __attribute__((noreturn));
static void sd_thread(void)
{
    struct queue_event ev;
    bool idle_notified = false;

    while (1)
    {
        queue_wait_w_tmo(&sd_queue, &ev, HZ);

        switch ( ev.id )
        {
#ifdef HAVE_HOTSWAP
        case SYS_HOTSWAP_INSERTED:
        case SYS_HOTSWAP_EXTRACTED:
            fat_lock();          /* lock-out FAT activity first -
                                    prevent deadlocking via disk_mount that
                                    would cause a reverse-order attempt with
                                    another thread */
            mutex_lock(&sd_mtx); /* lock-out card activity - direct calls
                                    into driver that bypass the fat cache */

            /* We now have exclusive control of fat cache and ata */

            disk_unmount(SD_SLOT_AS3525);     /* release "by force", ensure file
                                    descriptors aren't leaked and any busy
                                    ones are invalid if mounting */

            /* Force card init for new card, re-init for re-inserted one or
             * clear if the last attempt to init failed with an error. */
            card_info[SD_SLOT_AS3525].initialized = 0;

            if (ev.id == SYS_HOTSWAP_INSERTED)
            {
                sd_enable(true);
                init_pl180_controller(SD_SLOT_AS3525);
                sd_init_card(SD_SLOT_AS3525);
                disk_mount(SD_SLOT_AS3525);
            }

            queue_broadcast(SYS_FS_CHANGED, 0);

            /* Access is now safe */
            mutex_unlock(&sd_mtx);
            fat_unlock();
            sd_enable(false);
            break;
#endif
        case SYS_TIMEOUT:
            if (TIME_BEFORE(current_tick, last_disk_activity+(3*HZ)))
            {
                idle_notified = false;
            }
            else
            {
                /* never let a timer wrap confuse us */
                next_yield = current_tick;

                if (!idle_notified)
                {
                    call_storage_idle_notifys(false);
                    idle_notified = true;
                }
            }
            break;
#if 0
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            /* Wait until the USB cable is extracted again */
            usb_wait_for_disconnect(&sd_queue);

            break;
        case SYS_USB_DISCONNECTED:
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
            break;
#endif
        }
    }
}

static void init_pl180_controller(const int drive)
{
    MCI_COMMAND(drive) = MCI_DATA_CTRL(drive) = 0;
    MCI_CLEAR(drive) = 0x7ff;

    MCI_MASK0(drive) = MCI_MASK1(drive) = MCI_ERROR | MCI_DATA_END;

#ifdef HAVE_MULTIVOLUME
    VIC_INT_ENABLE |=
        (drive == INTERNAL_AS3525) ? INTERRUPT_NAND : INTERRUPT_MCI0;

#if defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2)
    /* setup isr for microsd monitoring */
    VIC_INT_ENABLE |= (INTERRUPT_GPIOA);
    /* clear previous irq */
    GPIOA_IC = (1<<2);
    /* enable edge detecting */
    GPIOA_IS &= ~(1<<2);
    /* detect both raising and falling edges */
    GPIOA_IBE |= (1<<2);

#endif

#else
    VIC_INT_ENABLE |= INTERRUPT_NAND;
#endif

    MCI_POWER(drive) = MCI_POWER_UP|(10 /*voltage*/ << 2); /* use OF voltage */
    mci_delay();

    MCI_POWER(drive) |= MCI_POWER_ON;
    mci_delay();

    MCI_SELECT(drive) = 0;

    MCI_CLOCK(drive) = MCI_CLOCK_ENABLE | AS3525_SD_IDENT_DIV;
    mci_delay();
}

int sd_init(void)
{
    int ret;
    CGU_IDE =   (1<<7)  /* AHB interface enable */  |
                (1<<6)  /* interface enable */      |
                (AS3525_IDE_DIV << 2)               |
                AS3525_CLK_PLLA;  /* clock source = PLLA */


    CGU_PERI |= CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
    CGU_PERI |= CGU_MCI_CLOCK_ENABLE;
    CCU_IO &= ~(1<<3);           /* bits 3:2 = 01, xpd is SD interface */
    CCU_IO |= (1<<2);
#endif

    wakeup_init(&transfer_completion_signal);

    init_pl180_controller(INTERNAL_AS3525);
    ret = sd_init_card(INTERNAL_AS3525);
    if(ret < 0)
        return ret;
#ifdef HAVE_MULTIVOLUME
    init_pl180_controller(SD_SLOT_AS3525);
#endif

    /* init mutex */
    mutex_init(&sd_mtx);

    queue_init(&sd_queue, true);
    create_thread(sd_thread, sd_stack, sizeof(sd_stack), 0,
            sd_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));

#ifndef BOOTLOADER
    sd_enabled = true;
    sd_enable(false);
#endif
    return 0;
}

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MV2(int drive,) struct storage_info *info)
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    info->sector_size=card_info[drive].block_size;
    info->num_sectors=card_info[drive].numblocks;
    info->vendor="Rockbox";
    info->product = (drive == 0) ?  "Internal Storage" : "SD Card Slot";
    info->revision="0.00";
}
#endif

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MV_NONVOID(int drive))
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    return (drive==1);
}

bool sd_present(IF_MV_NONVOID(int drive))
{
#ifndef HAVE_MULTIVOLUME
    const int drive=0;
#endif
    return (card_info[drive].initialized && card_info[drive].numblocks > 0);
}
#endif

static int sd_wait_for_state(const int drive, unsigned int state)
{
    unsigned int response = 0;
    unsigned int timeout = 100; /* ticks */
    long t = current_tick;

    while (1)
    {
        long tick;

        if(!send_cmd(drive, SD_SEND_STATUS, card_info[drive].rca,
                    MCI_RESP|MCI_ARG, &response))
            return -1;

        if (((response >> 9) & 0xf) == state)
            return 0;

        if(TIME_AFTER(current_tick, t + timeout))
            return -2;

        if (TIME_AFTER((tick = current_tick), next_yield))
        {
            yield();
            timeout += current_tick - tick;
            next_yield = tick + MIN_YIELD_PERIOD;
        }
    }
}

static int sd_select_bank(signed char bank)
{
    /* allocate card data buffer on the stack */
    unsigned char card_data[512 + 31];
    /* align it on cache line size */
    unsigned char *aligned_card_data = (void*)(((int)&card_data[0] + 31) & ~31);
    unsigned char *uncached_card_data = UNCACHED_ADDR(aligned_card_data);
    int ret;

    do {
        /* The ISR will set this to true if an error occurred */
        retry = false;

        ret = sd_wait_for_state(INTERNAL_AS3525, SD_TRAN);
        if (ret < 0)
            return ret - 2;

        if(!send_cmd(INTERNAL_AS3525, SD_SWITCH_FUNC, 0x80ffffef, MCI_ARG, NULL))
            return -1;

        mci_delay();

        if(!send_cmd(INTERNAL_AS3525, 35, 0, MCI_NO_FLAGS, NULL))
            return -2;

        mci_delay();

        memset(uncached_card_data, 0, 512);
        if(bank == -1)
        {   /* enable bank switching */
            uncached_card_data[0] = 16;
            uncached_card_data[1] = 1;
            uncached_card_data[2] = 10;
        }
        else
            uncached_card_data[0] = bank;

        dma_retain();
        /* we don't use the uncached card data for DMA, because we need the
         * physical memory address for DMA transfers */
        dma_enable_channel(0, aligned_card_data, MCI_FIFO(INTERNAL_AS3525),
            DMA_PERI_SD, DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8,
            NULL);

        MCI_DATA_TIMER(INTERNAL_AS3525) = SD_MAX_WRITE_TIMEOUT;
        MCI_DATA_LENGTH(INTERNAL_AS3525) = 512;
        MCI_DATA_CTRL(INTERNAL_AS3525) =  (1<<0) /* enable */   |
                                (0<<1) /* transfer direction */ |
                                (1<<3) /* DMA */                |
                                (9<<4) /* 2^9 = 512 */ ;

        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);

        dma_release();

        mci_delay();

        ret = sd_wait_for_state(INTERNAL_AS3525, SD_TRAN);
        if (ret < 0)
            return ret - 4;
    } while(retry);

    card_info[INTERNAL_AS3525].current_bank = (bank == -1) ? 0 : bank;

    return 0;
}

#define UNALIGNED_NUM_SECTORS 10
static unsigned char aligned_buffer[UNALIGNED_NUM_SECTORS* SECTOR_SIZE] __attribute__((aligned(32)));   /* align on cache line size */
static unsigned char *uncached_buffer = UNCACHED_ADDR(&aligned_buffer[0]);

static int sd_transfer_sectors(IF_MV2(int drive,) unsigned long start,
                               int count, void* buf, const bool write)
{
#ifndef HAVE_MULTIVOLUME
    const int drive = 0;
#endif
    int ret = 0;

    /* skip SanDisk OF */
    if (drive == INTERNAL_AS3525)
        start += AMS_OF_SIZE;

    mutex_lock(&sd_mtx);
#ifndef BOOTLOADER
    sd_enable(true);
#endif

    if (card_info[drive].initialized <= 0)
    {
        ret = sd_init_card(drive);
        if (!(card_info[drive].initialized))
            goto sd_transfer_error;
    }

    last_disk_activity = current_tick;

    ret = sd_wait_for_state(drive, SD_TRAN);
    if (ret < 0)
    {
        ret -= 20;
        goto sd_transfer_error;
    }

    dma_retain();

    while(count)
    {
        /* 128 * 512 = 2^16, and doesn't fit in the 16 bits of DATA_LENGTH
         * register, so we have to transfer maximum 127 sectors at a time. */
        unsigned int transfer = (count >= 128) ? 127 : count; /* sectors */
        void *dma_buf;
        const int cmd =
            write ? SD_WRITE_MULTIPLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
        unsigned long bank_start = start;

        /* The ISR will set this to true if an error occurred */
        retry = false;

        /* Only switch banks for internal storage */
        if(drive == INTERNAL_AS3525)
        {
            int bank = start / BLOCKS_PER_BANK; /* Current bank */

            /* Switch bank if needed */
            if(card_info[INTERNAL_AS3525].current_bank != bank)
            {
                ret = sd_select_bank(bank);
                if (ret < 0)
                {
                    ret -= 2*20;
                    goto sd_transfer_error;
                }
            }

            /* Adjust start block in current bank */
            bank_start -= bank * BLOCKS_PER_BANK;

            /* Do not cross a bank boundary in a single transfer loop */
            if((transfer + bank_start) >= BLOCKS_PER_BANK)
                transfer = BLOCKS_PER_BANK - bank_start;
        }

        dma_buf = aligned_buffer;
        if(transfer > UNALIGNED_NUM_SECTORS)
            transfer = UNALIGNED_NUM_SECTORS;
        if(write)
            memcpy(uncached_buffer, buf, transfer * SECTOR_SIZE);

        /* Set bank_start to the correct unit (blocks or bytes) */
        if(!(card_info[drive].ocr & (1<<30)))   /* not SDHC */
            bank_start *= BLOCK_SIZE;

        if(!send_cmd(drive, cmd, bank_start, MCI_ARG, NULL))
        {
            ret -= 3*20;
            goto sd_transfer_error;
        }

        if(write)
            dma_enable_channel(0, dma_buf, MCI_FIFO(drive),
                (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                DMAC_FLOWCTRL_PERI_MEM_TO_PERI, true, false, 0, DMA_S8, NULL);
        else
            dma_enable_channel(0, MCI_FIFO(drive), dma_buf,
                (drive == INTERNAL_AS3525) ? DMA_PERI_SD : DMA_PERI_SD_SLOT,
                DMAC_FLOWCTRL_PERI_PERI_TO_MEM, false, true, 0, DMA_S8, NULL);

        /* FIXME : we should check if the timeouts calculated from the card's
         * CSD are lower, and use them if it is the case
         * Note : the OF doesn't seem to use them anyway */
        MCI_DATA_TIMER(drive) = write ?
                SD_MAX_WRITE_TIMEOUT : SD_MAX_READ_TIMEOUT;
        MCI_DATA_LENGTH(drive) = transfer * card_info[drive].block_size;
        MCI_DATA_CTRL(drive) =  (1<<0) /* enable */                     |
                                (!write<<1) /* transfer direction */    |
                                (1<<3) /* DMA */                        |
                                (9<<4) /* 2^9 = 512 */ ;


        wakeup_wait(&transfer_completion_signal, TIMEOUT_BLOCK);
        if(!retry)
        {
            if(!write)
                memcpy(buf, uncached_buffer, transfer * SECTOR_SIZE);
            buf += transfer * SECTOR_SIZE;
            start += transfer;
            count -= transfer;
        }

        last_disk_activity = current_tick;

        if(!send_cmd(drive, SD_STOP_TRANSMISSION, 0, MCI_NO_FLAGS, NULL))
        {
            ret = -4*20;
            goto sd_transfer_error;
        }

        ret = sd_wait_for_state(drive, SD_TRAN);
        if (ret < 0)
        {
            ret -= 5*20;
            goto sd_transfer_error;
        }
    }

    dma_release();

#ifndef BOOTLOADER
    sd_enable(false);
#endif
    mutex_unlock(&sd_mtx);
    return 0;

sd_transfer_error:
    card_info[drive].initialized = 0;
    return ret;
}

int sd_read_sectors(IF_MV2(int drive,) unsigned long start, int count,
                     void* buf)
{
    return sd_transfer_sectors(IF_MV2(drive,) start, count, buf, false);
}

int sd_write_sectors(IF_MV2(int drive,) unsigned long start, int count,
                     const void* buf)
{

#ifdef BOOTLOADER /* we don't need write support in bootloader */
#ifdef HAVE_MULTIVOLUME
    (void) drive;
#endif
    (void) start;
    (void) count;
    (void) buf;
    return -1;
#else
    return sd_transfer_sectors(IF_MV2(drive,) start, count, (void*)buf, true);
#endif
}

#ifndef BOOTLOADER
void sd_sleep(void)
{
}

void sd_spin(void)
{
}

void sd_spindown(int seconds)
{
    (void)seconds;
}

long sd_last_disk_activity(void)
{
    return last_disk_activity;
}

void sd_enable(bool on)
{
    /* buttonlight AMSes need a bit of special handling for the buttonlight here,
     * due to the dual mapping of GPIOD and XPD */
#if defined(HAVE_BUTTON_LIGHT) && defined(HAVE_MULTIVOLUME)
    extern int buttonlight_is_on;
#endif
    if (sd_enabled == on)
        return; /* nothing to do */
    if(on)
    {
        CGU_PERI |= CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
        CGU_PERI |= CGU_MCI_CLOCK_ENABLE;
#ifdef HAVE_BUTTON_LIGHT
        CCU_IO |= (1<<2);
        if (buttonlight_is_on)
            GPIOD_DIR &= ~(1<<7);
        else
            _buttonlight_off();
#endif
#endif
        CGU_IDE |= (1<<7)  /* AHB interface enable */  |
                   (1<<6)  /* interface enable */;
        sd_enabled = true;
    }
    else
    {
        CGU_PERI &= ~CGU_NAF_CLOCK_ENABLE;
#ifdef HAVE_MULTIVOLUME
#ifdef HAVE_BUTTON_LIGHT
        CCU_IO &= ~(1<<2);
        if (buttonlight_is_on)
            _buttonlight_on();
#endif
        CGU_PERI &= ~CGU_MCI_CLOCK_ENABLE;
#endif
        CGU_IDE &= ~((1<<7)|(1<<6));
        sd_enabled = false;
    }
}

/* move the sd-card info to mmc struct */
tCardInfo *card_get_info_target(int card_no)
{
    int i, temp;
    static tCardInfo card;
    static const char mantissa[] = {  /* *10 */
        0,  10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
    static const int exponent[] = {  /* use varies */
      1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };

    card.initialized  = card_info[card_no].initialized;
    card.ocr          = card_info[card_no].ocr;
    for(i=0; i<4; i++)  card.csd[i] = card_info[card_no].csd[i];
    for(i=0; i<4; i++)  card.cid[i] = card_info[card_no].cid[i];
    card.numblocks    = card_info[card_no].numblocks;
    card.blocksize    = card_info[card_no].block_size;
    temp              = card_extract_bits(card.csd, 29, 3);
    card.speed        = mantissa[card_extract_bits(card.csd, 25, 4)]
                      * exponent[temp > 2 ? 7 : temp + 4];
    card.nsac         = 100 * card_extract_bits(card.csd, 16, 8);
    temp              = card_extract_bits(card.csd, 13, 3);
    card.tsac         = mantissa[card_extract_bits(card.csd, 9, 4)]
                      * exponent[temp] / 10;
    card.cid[0]       = htobe32(card.cid[0]); /* ascii chars here */
    card.cid[1]       = htobe32(card.cid[1]); /* ascii chars here */
    temp = *((char*)card.cid+13); /* adjust year<=>month, 1997 <=> 2000 */
    *((char*)card.cid+13) = (unsigned char)((temp >> 4) | (temp << 4)) + 3;

    return &card;
}

bool card_detect_target(void)
{
#if defined(HAVE_HOTSWAP) && \
    (defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2))
    return !(GPIOA_PIN(2));
#else
    return false;
#endif
}

#ifdef HAVE_HOTSWAP
void card_enable_monitoring_target(bool on)
{
    if (on)
    {
    /* add e200v2/c200v2 here */
#if defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2)
        /* enable isr*/
        GPIOA_IE |= (1<<2);
#endif
    }
    else
    {
#if defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_C200V2)
        /* edisable isr*/
        GPIOA_IE &= ~(1<<2);
#endif
    }
}
#endif

#endif /* BOOTLOADER */
