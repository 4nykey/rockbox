/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "ata.h"
#include "kernel.h"
#include "thread.h"
#include "led.h"
#include "cpu.h"
#include "system.h"
#include "debug.h"
#include "panic.h"
#include "usb.h"
#include "power.h"
#include "string.h"
#include "hwcompat.h"

#ifdef TARGET_TREE
#include "ata-target.h"
#endif

#define SECTOR_SIZE     (512)

#if (CONFIG_CPU == MCF5249) || (CONFIG_CPU == MCF5250)

/* asm optimised read & write loops */

#define NOINLINE_ATTR __attribute__((noinline)) /* don't inline the loops */

#define ATA_IOBASE      0x20000000
#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE + 0x20)))
#define ATA_CONTROL     (*((volatile unsigned short*)(ATA_IOBASE + 0x1c)))

#define ATA_ERROR       (*((volatile unsigned short*)(ATA_IOBASE + 0x22)))
#define ATA_NSECTOR     (*((volatile unsigned short*)(ATA_IOBASE + 0x24)))
#define ATA_SECTOR      (*((volatile unsigned short*)(ATA_IOBASE + 0x26)))
#define ATA_LCYL        (*((volatile unsigned short*)(ATA_IOBASE + 0x28)))
#define ATA_HCYL        (*((volatile unsigned short*)(ATA_IOBASE + 0x2a)))
#define ATA_SELECT      (*((volatile unsigned short*)(ATA_IOBASE + 0x2c)))
#define ATA_COMMAND     (*((volatile unsigned short*)(ATA_IOBASE + 0x2e)))

#define STATUS_BSY      0x8000
#define STATUS_RDY      0x4000
#define STATUS_DF       0x2000
#define STATUS_DRQ      0x0800
#define STATUS_ERR      0x0100

#define ERROR_ABRT      0x0400

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa500
#define READ_PATTERN2 0x5a00
#define READ_PATTERN3 0xaa00
#define READ_PATTERN4 0x5500

#define READ_PATTERN1_MASK 0xff00
#define READ_PATTERN2_MASK 0xff00
#define READ_PATTERN3_MASK 0xff00
#define READ_PATTERN4_MASK 0xff00

#define SET_REG(reg,val) reg = ((val) << 8)
#define SET_16BITREG(reg,val) reg = (val)

#elif (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)

/* Plain C read & write loops */
#define PREFER_C_READING
#define PREFER_C_WRITING

#if (CONFIG_CPU == PP5002)
#define ATA_IOBASE      0xc00031e0
#define ATA_CONTROL     (*((volatile unsigned char*)(0xc00033f8)))
#elif (CONFIG_CPU == PP5020)
#define ATA_IOBASE      0xc30001e0
#define ATA_CONTROL     (*((volatile unsigned char*)(0xc30003f8)))
#endif

#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE)))
#define ATA_ERROR       (*((volatile unsigned char*)(ATA_IOBASE + 0x04)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(ATA_IOBASE + 0x08)))
#define ATA_SECTOR      (*((volatile unsigned char*)(ATA_IOBASE + 0x0c)))
#define ATA_LCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x10)))
#define ATA_HCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x14)))
#define ATA_SELECT      (*((volatile unsigned char*)(ATA_IOBASE + 0x18)))
#define ATA_COMMAND     (*((volatile unsigned char*)(ATA_IOBASE + 0x1c)))

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define ERROR_ABRT      0x04

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa5
#define READ_PATTERN2 0x5a
#define READ_PATTERN3 0xaa
#define READ_PATTERN4 0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

#define SET_REG(reg,val) reg = (val)
#define SET_16BITREG(reg,val) reg = (val)

#elif CONFIG_CPU == SH7034

/* asm optimised read & write loops */

#define NOINLINE_ATTR __attribute__((noinline)) /* don't inline the loops */

#define SWAP_WORDS

#define ATA_IOBASE      0x06100100
#define ATA_DATA        (*((volatile unsigned short*)0x06104100))
#define ATA_CONTROL1    ((volatile unsigned char*)0x06200206)
#define ATA_CONTROL2    ((volatile unsigned char*)0x06200306)
#define ATA_CONTROL     (*ata_control)

#define ATA_ERROR       (*((volatile unsigned char*)ATA_IOBASE + 1))
#define ATA_NSECTOR     (*((volatile unsigned char*)ATA_IOBASE + 2))
#define ATA_SECTOR      (*((volatile unsigned char*)ATA_IOBASE + 3))
#define ATA_LCYL        (*((volatile unsigned char*)ATA_IOBASE + 4))
#define ATA_HCYL        (*((volatile unsigned char*)ATA_IOBASE + 5))
#define ATA_SELECT      (*((volatile unsigned char*)ATA_IOBASE + 6))
#define ATA_COMMAND     (*((volatile unsigned char*)ATA_IOBASE + 7))

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

#define ERROR_ABRT      0x04

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa5
#define READ_PATTERN2 0x5a
#define READ_PATTERN3 0xaa
#define READ_PATTERN4 0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

#define SET_REG(reg,val) reg = (val)
#define SET_16BITREG(reg,val) reg = (val)

#elif CONFIG_CPU == TCC730

/* Plain C read & write loops */
#define PREFER_C_READING
#define PREFER_C_WRITING

#define SWAP_WORDS

#define ATA_DATA_IDX        (0xD0)
#define ATA_ERROR_IDX       (0xD2) 
#define ATA_NSECTOR_IDX     (0xD4)
#define ATA_SECTOR_IDX      (0xD6)
#define ATA_LCYL_IDX        (0xD8)
#define ATA_HCYL_IDX        (0xDA)
#define ATA_SELECT_IDX      (0xDC)
#define ATA_COMMAND_IDX     (0xDE)
#define ATA_CONTROL_IDX     (0xEC)

#define ATA_FEATURE_IDX     ATA_ERROR_IDX
#define ATA_STATUS_IDX      ATA_COMMAND_IDX
#define ATA_ALT_STATUS_IDX  ATA_CONTROL_IDX

#define SET_REG(reg, value) (ide_write_register(reg ## _IDX, value))
#define SET_16BITREG(reg, value) (ide_write_register(reg ## _IDX, value))
#define GET_REG(reg) (ide_read_register(reg))

#define ATA_DATA        (GET_REG(ATA_DATA_IDX))
#define ATA_ERROR       (GET_REG(ATA_ERROR_IDX))
#define ATA_NSECTOR     (GET_REG(ATA_NSECTOR_IDX))
#define ATA_SECTOR      (GET_REG(ATA_SECTOR_IDX))
#define ATA_LCYL        (GET_REG(ATA_LCYL_IDX))
#define ATA_HCYL        (GET_REG(ATA_HCYL_IDX))
#define ATA_SELECT      (GET_REG(ATA_SELECT_IDX))
#define ATA_COMMAND     (GET_REG(ATA_COMMAND_IDX))

#define ATA_CONTROL     (GET_REG(ATA_CONTROL_IDX))

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01

#define ERROR_ABRT      0x04

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa5
#define READ_PATTERN2 0x5a
#define READ_PATTERN3 0xaa
#define READ_PATTERN4 0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

static unsigned char ide_sector_data[SECTOR_SIZE] __attribute__ ((section(".idata")));
static unsigned ide_reg_temp __attribute__ ((section(".idata")));

void ide_write_register(int reg, int value) {
    /* Archos firmware code does (sometimes!) this:
       set the RAM speed to 8 cycles. 
       MIUSCFG |= 0x7;
    */

    ide_reg_temp = value;

    long extAddr = (long)reg << 16;
    ddma_transfer(1, 1, &ide_reg_temp, extAddr, 2);  

    /* set the RAM speed to 6 cycles.
    unsigned char miuscfg = MIUSCFG;
    miuscfg = (miuscfg & ~7) | 5;
    */
}

int ide_read_register(int reg) {
    /* set the RAM speed to 6 cycles. 
       unsigned char miuscfg = MIUSCFG;
       miuscfg = (miuscfg & ~7) | 5;
       MIUSCFG = miuscfg; */
  
    long extAddr = (long)reg << 16;
    ddma_transfer(0, 1, &ide_reg_temp, extAddr, 2);
  
    /* This is done like this in the archos firmware... 
       miuscfg = MIUSCFG;
       miuscfg = (miuscfg & ~7) | 5;
       MIUSCFG = miuscfg;
       Though I'd expect MIUSCFG &= ~0x7; (1 cycle) */
  
    return ide_reg_temp;
}

#endif

#ifndef NOINLINE_ATTR
#define NOINLINE_ATTR
#endif

#define ATA_FEATURE     ATA_ERROR

#define ATA_STATUS      ATA_COMMAND
#define ATA_ALT_STATUS  ATA_CONTROL

#define SELECT_DEVICE1  0x10
#define SELECT_LBA      0x40

#define CONTROL_nIEN    0x02
#define CONTROL_SRST    0x04

#define CMD_READ_SECTORS           0x20
#define CMD_WRITE_SECTORS          0x30
#define CMD_READ_MULTIPLE          0xC4
#define CMD_WRITE_MULTIPLE         0xC5
#define CMD_SET_MULTIPLE_MODE      0xC6
#define CMD_STANDBY_IMMEDIATE      0xE0
#define CMD_STANDBY                0xE2
#define CMD_IDENTIFY               0xEC
#define CMD_SLEEP                  0xE6
#define CMD_SET_FEATURES           0xEF
#define CMD_SECURITY_FREEZE_LOCK   0xF5

#define Q_SLEEP 0

#define READ_TIMEOUT 5*HZ

static struct mutex ata_mtx;
char ata_device; /* device 0 (master) or 1 (slave) */
int ata_io_address; /* 0x300 or 0x200, only valid on recorder */
#if CONFIG_CPU == SH7034
static volatile unsigned char* ata_control;
#endif

bool old_recorder = false;
int ata_spinup_time = 0;
#if CONFIG_LED == LED_REAL
static bool ata_led_enabled = true;
static bool ata_led_on = false;
#endif
static bool spinup = false;
static bool sleeping = true;
static long sleep_timeout = 5*HZ;
static bool poweroff = false;
#ifdef HAVE_ATA_POWER_OFF
static int poweroff_timeout = 2*HZ;
#endif
static long ata_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char ata_thread_name[] = "ata";
static struct event_queue ata_queue;
static bool initialized = false;
static bool delayed_write = false;
static unsigned char delayed_sector[SECTOR_SIZE];
static int delayed_sector_num;

static long last_user_activity = -1;
long last_disk_activity = -1;

static int multisectors; /* number of supported multisectors */
static unsigned short identify_info[SECTOR_SIZE];

static int ata_power_on(void);
static int perform_soft_reset(void);
static int set_multiple_mode(int sectors);
static int set_features(void);

static int wait_for_bsy(void) ICODE_ATTR;
static int wait_for_bsy(void)
{
    long timeout = current_tick + HZ*30;
    while (TIME_BEFORE(current_tick, timeout) && (ATA_STATUS & STATUS_BSY)) {
        last_disk_activity = current_tick;
        yield();
    }

    if (TIME_BEFORE(current_tick, timeout))
        return 1;
    else
        return 0; /* timeout */
}

static int wait_for_rdy(void) ICODE_ATTR;
static int wait_for_rdy(void)
{
    long timeout;

    if (!wait_for_bsy())
        return 0;

    timeout = current_tick + HZ*10;

    while (TIME_BEFORE(current_tick, timeout) &&
           !(ATA_ALT_STATUS & STATUS_RDY)) {
        last_disk_activity = current_tick;
        yield();
    }

    if (TIME_BEFORE(current_tick, timeout))
        return STATUS_RDY;
    else
        return 0; /* timeout */
}

static int wait_for_start_of_transfer(void) ICODE_ATTR;
static int wait_for_start_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;

    return (ATA_ALT_STATUS & (STATUS_BSY|STATUS_DRQ)) == STATUS_DRQ;
}

static int wait_for_end_of_transfer(void) ICODE_ATTR;
static int wait_for_end_of_transfer(void)
{
    if (!wait_for_bsy())
        return 0;
    return (ATA_ALT_STATUS & (STATUS_RDY|STATUS_DRQ)) == STATUS_RDY;
}    

/* Optimization: don't do 256 calls to ddma_transfer; fuse with it
 * as in the Archos firmware.
 * It actually possible to do a single dma transfer to copy a whole sector between ATA
 * controller & cpu internal memory.
 */
/* the tight loop of ata_read_sectors(), to avoid the whole in IRAM */
static void copy_read_sectors(unsigned char* buf, int wordcount)
                              ICODE_ATTR NOINLINE_ATTR;
static void copy_read_sectors(unsigned char* buf, int wordcount)
{
#ifdef PREFER_C_READING
    unsigned short tmp = 0;

    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned char* bufend = buf + wordcount*2;
        do
        {   /* loop compiles to 9 assembler instructions */
            /* takes 14 clock cycles (2 pipeline stalls, 1 wait) */
            tmp = ATA_DATA;
#if defined(SWAP_WORDS) || defined(ROCKBOX_LITTLE_ENDIAN)
            *buf++ = tmp & 0xff; /* I assume big endian */
            *buf++ = tmp >> 8;   /*  and don't use the SWAB16 macro */
#else
            *buf++ = tmp >> 8;
            *buf++ = tmp & 0xff;
#endif
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {   /* loop compiles to 7 assembler instructions */
            /* takes 12 clock cycles (2 pipeline stalls, 1 wait) */
#ifdef SWAP_WORDS
            *wbuf = swap16(ATA_DATA);
#else
            *wbuf = ATA_DATA;
#endif
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
#else /* !PREFER_C_READING */
#if CONFIG_CPU == TCC730
    int sectorcount = wordcount / 0x100;
    do {
        /* Slurp an entire sector with a single dma transfer */
        ddma_transfer(0, 1, ide_sector_data, ATA_DATA_IDX << 16, SECTOR_SIZE);
        memcpy(buf, ide_sector_data, SECTOR_SIZE);
        buf += SECTOR_SIZE;
        sectorcount--;
    } while (sectorcount > 0);
#elif defined(CPU_COLDFIRE)
    unsigned char* bufend = buf + 2 * wordcount;
    /* coldfire asm reading, utilising line bursts */
    /* this assumes there is at least one full line to copy */
    asm volatile (
        "move.l  %[buf],%%d0     \n"
        "btst.l  #0,%%d0         \n" /* 16-bit aligned? */
        "jeq     .aligned        \n" /* yes, do word copy */

        /* not 16-bit aligned */
        "subq.l  #1,%[end]       \n" /* last byte is done unconditionally */
        "moveq.l #24,%%d1        \n" /* preload shift count */

        "move.w  (%[ata]),%%d2   \n" /* load initial word */
        "move.l  %%d2,%%d3       \n"
        "lsr.l   #8,%%d3         \n"
        "move.b  %%d3,(%[buf])+  \n" /* write high byte of it, aligns dest addr */
        
        "btst.l  #1,%%d0         \n" /* longword aligned? */
        "beq.b   .end_u_w1       \n" /* yes, skip leading word handling */
        
        "swap    %%d2            \n" /* move initial word up */
        "move.w  (%[ata]),%%d2   \n" /* combine with second word */
        "move.l  %%d2,%%d3       \n"
        "lsr.l   #8,%%d3         \n"
        "move.w  %%d3,(%[buf])+  \n" /* write bytes 2 and 3 as word */

    ".end_u_w1:                  \n"
        "moveq.l #12,%%d0        \n"
        "add.l   %[buf],%%d0     \n"
        "and.l   #0xFFFFFFF0,%%d0\n" /* d0 == first line bound */
        "cmp.l   %[buf],%%d0     \n" /* any leading longwords? */
        "bls.b   .end_u_l1       \n" /* no: skip loop */
        
    ".loop_u_l1:                 \n"
        "move.w  (%[ata]),%%d3   \n" /* load first word */
        "swap    %%d3            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d3   \n" /* load second word */
        "move.l  %%d3,%%d4       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d3         \n"
        "or.l    %%d3,%%d2       \n" /* combine old low byte with new top 3 bytes */
        "move.l  %%d2,(%[buf])+  \n" /* store as long */
        "move.l  %%d4,%%d2       \n"
        "cmp.l   %[buf],%%d0     \n" /* run up to first line bound */
        "bhi.b   .loop_u_l1      \n"

    ".end_u_l1:                  \n"
        "lea.l   (-14,%[end]),%[end] \n" /* adjust end addr. to 16 bytes/pass */

    ".loop_u_line:               \n"
        "move.w  (%[ata]),%%d3   \n" /* load 1st word */
        "swap    %%d3            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d3   \n" /* load 2nd word */
        "move.l  %%d3,%%d0       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d2       \n" /* combine old low byte with new top 3 bytes */
        "move.w  (%[ata]),%%d4   \n" /* load 3rd word */
        "swap    %%d4            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d4   \n" /* load 4th word */
        "move.l  %%d4,%%d0       \n"
        "lsl.l   %%d1,%%d3       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d3       \n" /* combine old low byte with new top 3 bytes */
        "move.w  (%[ata]),%%d5   \n" /* load 5th word */
        "swap    %%d5            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d5   \n" /* load 6th word */
        "move.l  %%d5,%%d0       \n"
        "lsl.l   %%d1,%%d4       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d4       \n" /* combine old low byte with new top 3 bytes */
        "move.w  (%[ata]),%%d6   \n" /* load 7th word */
        "swap    %%d6            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d6   \n" /* load 8th word */
        "move.l  %%d6,%%d0       \n"
        "lsl.l   %%d1,%%d5       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d5       \n" /* combine old low byte with new top 3 bytes */
        "movem.l %%d2-%%d5,(%[buf])  \n"  /* store line */
        "lea.l   (16,%[buf]),%[buf]  \n"
        "move.l  %%d6,%%d2       \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to last line bound */
        "bhi.b   .loop_u_line    \n"

        "lea.l   (12,%[end]),%[end]  \n"  /* readjust for longword loop */
        "cmp.l   %[buf],%[end]   \n" /* any trailing longwords? */
        "bls.b   .end_u_l2       \n" /* no: skip loop */

    ".loop_u_l2:                 \n"
        "move.w  (%[ata]),%%d3   \n" /* load first word */
        "swap    %%d3            \n" /* move to upper 16 bit */
        "move.w  (%[ata]),%%d3   \n" /* load second word */
        "move.l  %%d3,%%d4       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d3         \n"
        "or.l    %%d3,%%d2       \n" /* combine old low byte with new top 3 bytes */
        "move.l  %%d2,(%[buf])+  \n" /* store as long */
        "move.l  %%d4,%%d2       \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to last long bound */
        "bhi.b   .loop_u_l2      \n"
        
    ".end_u_l2:                  \n"
        "addq.l  #2,%[end]       \n" /* back to final end address */
        "cmp.l   %[buf],%[end]   \n" /* one word left? */
        "bls.b   .end_u_w2       \n"

        "swap    %%d2            \n" /* move old word to upper 16 bits */
        "move.w  (%[ata]),%%d2   \n" /* load final word */
        "move.l  %%d2,%%d3       \n"
        "lsr.l   #8,%%d3         \n"
        "move.w  %%d3,(%[buf])+  \n" /* write bytes 2 and 3 as word */

    ".end_u_w2:                  \n"
        "move.b  %%d2,(%[buf])+  \n" /* store final byte */
        "bra.b   .exit           \n"

        /* 16-bit aligned */
    ".aligned:                   \n"
        "btst.l  #1,%%d0         \n" /* longword aligned? */
        "beq.b   .end_a_w1       \n" /* yes, skip leading word handling */

        "move.w  (%[ata]),(%[buf])+  \n"  /* copy initial word */

    ".end_a_w1:                  \n"
        "moveq.l #12,%%d0        \n"
        "add.l   %[buf],%%d0     \n"
        "and.l   #0xFFFFFFF0,%%d0\n" /* d0 == first line bound */
        "cmp.l   %[buf],%%d0     \n" /* any leading longwords? */
        "bls.b   .end_a_l1       \n" /* no: skip loop */

    ".loop_a_l1:                 \n"
        "move.w  (%[ata]),%%d1   \n" /* load first word */
        "swap    %%d1            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d1   \n" /* load second word */
        "move.l  %%d1,(%[buf])+  \n" /* store as long */
        "cmp.l   %[buf],%%d0     \n" /* run up to first line bound */
        "bhi.b   .loop_a_l1      \n"

    ".end_a_l1:                  \n"
        "lea.l   (-14,%[end]),%[end] \n" /* adjust end addr. to 16 bytes/pass */

    ".loop_a_line:               \n"
        "move.w  (%[ata]),%%d0   \n" /* load 1st word */
        "swap    %%d0            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d0   \n" /* load 2nd word */
        "move.w  (%[ata]),%%d1   \n" /* load 3rd word */
        "swap    %%d1            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d1   \n" /* load 4th word */
        "move.w  (%[ata]),%%d2   \n" /* load 5th word */
        "swap    %%d2            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d2   \n" /* load 6th word */
        "move.w  (%[ata]),%%d3   \n" /* load 7th word */
        "swap    %%d3            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d3   \n" /* load 8th word */
        "movem.l %%d0-%%d3,(%[buf])  \n"  /* store line */
        "lea.l   (16,%[buf]),%[buf]  \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to last line bound */
        "bhi.b   .loop_a_line    \n"
        
        "lea.l   (12,%[end]),%[end]  \n"  /* readjust for longword loop */
        "cmp.l   %[buf],%[end]   \n" /* any trailing longwords? */
        "bls.b   .end_a_l2       \n" /* no: skip loop */

    ".loop_a_l2:                 \n"
        "move.w  (%[ata]),%%d1   \n" /* read first word */
        "swap    %%d1            \n" /* move it to upper 16 bits */
        "move.w  (%[ata]),%%d1   \n" /* read second word */
        "move.l  %%d1,(%[buf])+  \n" /* store as long */
        "cmp.l   %[buf],%[end]   \n" /* run up to last long bound */
        "bhi.b   .loop_a_l2      \n"
        
    ".end_a_l2:                  \n"
        "addq.l  #2,%[end]       \n" /* back to final end address */
        "cmp.l   %[buf],%[end]   \n" /* one word left? */
        "bls.b   .end_a_w2       \n"

        "move.w  (%[ata]),(%[buf])+  \n"  /* copy final word */

    ".end_a_w2:                  \n"

    ".exit:                      \n"
        : /* outputs */
        [buf]"+a"(buf),
        [end]"+a"(bufend)
        : /* inputs */
        [ata]"a"(&ATA_DATA)
        : /*trashed */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6"
    );
#else
    /* SH1 turbo-charged assembler reading */
    /* this assumes wordcount to be a multiple of 4 */
    asm volatile (
        "mov     %[buf],r0       \n"
        "tst     #1,r0           \n"  /* 16-bit aligned ? */
        "bt      .aligned        \n"  /* yes, do word copy */

        /* not 16-bit aligned */
        "mov     #-1,r3          \n"  /* prepare a bit mask for high byte */
        "shll8   r3              \n"  /* r3 = 0xFFFFFF00 */

        "mov.w   @%[ata],r2      \n"  /* read first word (1st round) */
        "mov.b   r2,@%[buf]      \n"  /* store low byte of first word */
        "bra     .start4_b       \n"  /* jump into loop after next instr. */
        "add     #-5,%[buf]      \n"  /* adjust for dest. offsets; now even */

        ".align  2               \n"
    ".loop4_b:                   \n"  /* main loop: copy 4 words in a row */
        "mov.w   @%[ata],r2      \n"  /* read first word (2+ round) */
        "and     r3,r1           \n"  /* get high byte of fourth word (2+ round) */
        "extu.b  r2,r0           \n"  /* get low byte of first word (2+ round) */
        "or      r1,r0           \n"  /* combine with high byte of fourth word */
        "mov.w   r0,@(4,%[buf])  \n"  /* store at buf[4] */
        "nop                     \n"  /* maintain alignment */
    ".start4_b:                  \n"
        "mov.w   @%[ata],r1      \n"  /* read second word */
        "and     r3,r2           \n"  /* get high byte of first word */
        "extu.b  r1,r0           \n"  /* get low byte of second word */
        "or      r2,r0           \n"  /* combine with high byte of first word */
        "mov.w   r0,@(6,%[buf])  \n"  /* store at buf[6] */
        "add     #8,%[buf]       \n"  /* buf += 8 */
        "mov.w   @%[ata],r2      \n"  /* read third word */
        "and     r3,r1           \n"  /* get high byte of second word */
        "extu.b  r2,r0           \n"  /* get low byte of third word */
        "or      r1,r0           \n"  /* combine with high byte of second word */
        "mov.w   r0,@%[buf]      \n"  /* store at buf[0] */
        "cmp/hi  %[buf],%[end]   \n"  /* check for end */
        "mov.w   @%[ata],r1      \n"  /* read fourth word */
        "and     r3,r2           \n"  /* get high byte of third word */
        "extu.b  r1,r0           \n"  /* get low byte of fourth word */
        "or      r2,r0           \n"  /* combine with high byte of third word */
        "mov.w   r0,@(2,%[buf])  \n"  /* store at buf[2] */
        "bt      .loop4_b        \n"
        /* 24 instructions for 4 copies, takes 30 clock cycles (4 wait) */
        /* avg. 7.5 cycles per word - 86% faster */

        "swap.b  r1,r0           \n"  /* get high byte of last word */
        "bra     .exit           \n"
        "mov.b   r0,@(4,%[buf])  \n"  /* and store it */

        /* 16-bit aligned, loop(read and store word) */
    ".aligned:                   \n"
        "mov.w   @%[ata],r2      \n"  /* read first word (1st round) */
        "bra     .start4_w       \n"  /* jump into loop after next instr. */
        "add     #-6,%[buf]      \n"  /* adjust for destination offsets */

        ".align  2               \n"
    ".loop4_w:                   \n"  /* main loop: copy 4 words in a row */
        "mov.w   @%[ata],r2      \n"  /* read first word (2+ round) */
        "swap.b  r1,r0           \n"  /* swap fourth word (2+ round) */
        "mov.w   r0,@(4,%[buf])  \n"  /* store fourth word (2+ round) */
        "nop                     \n"  /* maintain alignment */
    ".start4_w:                  \n"
        "mov.w   @%[ata],r1      \n"  /* read second word */
        "swap.b  r2,r0           \n"  /* swap first word */
        "mov.w   r0,@(6,%[buf])  \n"  /* store first word in buf[6] */
        "add     #8,%[buf]       \n"  /* buf += 8 */
        "mov.w   @%[ata],r2      \n"  /* read third word */
        "swap.b  r1,r0           \n"  /* swap second word */
        "mov.w   r0,@%[buf]      \n"  /* store second word in buf[0] */
        "cmp/hi  %[buf],%[end]   \n"  /* check for end */
        "mov.w   @%[ata],r1      \n"  /* read fourth word */
        "swap.b  r2,r0           \n"  /* swap third word */
        "mov.w   r0,@(2,%[buf])  \n"  /* store third word */
        "bt      .loop4_w        \n"
        /* 16 instructions for 4 copies, takes 22 clock cycles (4 wait) */
        /* avg. 5.5 cycles per word - 118% faster */

        "swap.b  r1,r0           \n"  /* swap fourth word (last round) */
        "mov.w   r0,@(4,%[buf])  \n"  /* and store it */

    ".exit:                      \n"
        : /* outputs */
        [buf]"+r"(buf)
        : /* inputs */
        [end]"r"(buf + 2 * wordcount - 12), /* adjusted for offsets */
        [ata]"r"(&ATA_DATA)
        : /*trashed */
        "r0","r1","r2","r3"
    );
#endif /* CPU */
#endif /* !PREFER_C_READING */
}

#if CONFIG_LED == LED_REAL
/* Conditionally block LED access for the ATA driver, so the LED can be
 * (mis)used for other purposes */
static void ata_led(bool on) {
    ata_led_on = on;
    if (ata_led_enabled) {
        led(ata_led_on);
    }
}
#else
#define ata_led(on) led(on)
#endif

int ata_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int incount,
                     void* inbuf)
{
    int ret = 0;
    long timeout;
    int count;
    void* buf;
    long spinup_start;

#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    mutex_lock(&ata_mtx);

    last_disk_activity = current_tick;
    spinup_start = current_tick;

    ata_led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                ata_led(false);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                ata_led(false);
                return -1;
            }
        }
    }

    timeout = current_tick + READ_TIMEOUT;

    SET_REG(ATA_SELECT, ata_device);
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        ata_led(false);
        return -2;
    }

 retry:
    buf = inbuf;
    count = incount;
    while (TIME_BEFORE(current_tick, timeout)) {
        ret = 0;
        last_disk_activity = current_tick;

        if ( count == 256 )
            SET_REG(ATA_NSECTOR, 0); /* 0 means 256 sectors */
        else
            SET_REG(ATA_NSECTOR, (unsigned char)count);

        SET_REG(ATA_SECTOR, start & 0xff);
        SET_REG(ATA_LCYL, (start >> 8) & 0xff);
        SET_REG(ATA_HCYL, (start >> 16) & 0xff);
        SET_REG(ATA_SELECT, ((start >> 24) & 0xf) | SELECT_LBA | ata_device);
        SET_REG(ATA_COMMAND, CMD_READ_MULTIPLE);

        /* wait at least 400ns between writing command and reading status */
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");

        while (count) {
            int sectors;
            int wordcount;
            int status;

            if (!wait_for_start_of_transfer()) {
                /* We have timed out waiting for RDY and/or DRQ, possibly
                   because the hard drive is shaking and has problems reading
                   the data. We have two options:
                   1) Wait some more
                   2) Perform a soft reset and try again.

                   We choose alternative 2.
                */
                perform_soft_reset();
                ret = -4;
                goto retry;
            }

            if (spinup) {
                ata_spinup_time = current_tick - spinup_start;
                spinup = false;
                sleeping = false;
                poweroff = false;
            }

            /* read the status register exactly once per loop */
            status = ATA_STATUS;

            /* if destination address is odd, use byte copying,
               otherwise use word copying */

            if (count >= multisectors )
                sectors = multisectors;
            else
                sectors = count;

            wordcount = sectors * SECTOR_SIZE / 2;

            copy_read_sectors(buf, wordcount);

            /*
              "Device errors encountered during READ MULTIPLE commands are
              posted at the beginning of the block or partial block transfer,
              but the DRQ bit is still set to one and the data transfer shall
              take place, including transfer of corrupted data, if any."
                -- ATA specification
            */
            if ( status & (STATUS_BSY | STATUS_ERR | STATUS_DF) ) {
                perform_soft_reset();
                ret = -5;
                goto retry;
            }

            buf += sectors * SECTOR_SIZE; /* Advance one chunk of sectors */
            count -= sectors;

            last_disk_activity = current_tick;
        }

        if(!ret && !wait_for_end_of_transfer()) {
            perform_soft_reset();
            ret = -3;
            goto retry;
        }
        break;
    }
    ata_led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if reading went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

/* the tight loop of ata_write_sectors(), to avoid the whole in IRAM */
static void copy_write_sectors(const unsigned char* buf, int wordcount) 
                               ICODE_ATTR NOINLINE_ATTR;
static void copy_write_sectors(const unsigned char* buf, int wordcount)
{
#ifdef PREFER_C_WRITING

    if ( (unsigned long)buf & 1)
    {   /* not 16-bit aligned, copy byte by byte */
        unsigned short tmp = 0;
        const unsigned char* bufend = buf + wordcount*2;
        do
        {
#if defined(SWAP_WORDS) || defined(ROCKBOX_LITTLE_ENDIAN)
            /* SH1: loop compiles to 9 assembler instructions */
            /* takes 13 clock cycles (2 pipeline stalls) */
            tmp = (unsigned short) *buf++;
            tmp |= (unsigned short) *buf++ << 8; /* I assume big endian */
            SET_16BITREG(ATA_DATA, tmp);  /* and don't use the SWAB16 macro */
#else
            tmp = (unsigned short) *buf++ << 8;
            tmp |= (unsigned short) *buf++;
            SET_16BITREG(ATA_DATA, tmp);
#endif
        } while (buf < bufend); /* tail loop is faster */
    }
    else
    {   /* 16-bit aligned, can do faster copy */
        unsigned short* wbuf = (unsigned short*)buf;
        unsigned short* wbufend = wbuf + wordcount;
        do
        {
#ifdef SWAP_WORDS
            /* loop compiles to 6 assembler instructions */
            /* takes 10 clock cycles (2 pipeline stalls) */
            SET_16BITREG(ATA_DATA, swap16(*wbuf));
#else
            SET_16BITREG(ATA_DATA, *wbuf);
#endif
        } while (++wbuf < wbufend); /* tail loop is faster */
    }
#else /* !PREFER_C_WRITING */
#ifdef CPU_COLDFIRE
    const unsigned char* bufend = buf + 2 * wordcount;
    /* coldfire asm writing, utilising line bursts */
    asm volatile (
        "move.l  %[buf],%%d0     \n"
        "btst.l  #0,%%d0         \n" /* 16-bit aligned? */
        "jeq     .w_aligned      \n" /* yes, do word copy */

        /* not 16-bit aligned */
        "subq.l  #1,%[end]       \n" /* last byte is done unconditionally */
        "moveq.l #24,%%d1        \n" /* preload shift count */

        "move.b  (%[buf])+,%%d2  \n"

        "btst.l  #1,%%d0         \n" /* longword aligned? */
        "beq.b   .w_end_u_w1     \n" /* yes, skip leading word handling */

        "swap    %%d2            \n"
        "move.w  (%[buf])+,%%d2  \n"
        "move.l  %%d2,%%d3       \n"
        "lsr.l   #8,%%d3         \n"
        "move.w  %%d3,(%[ata])   \n"
        
    ".w_end_u_w1:                \n"
        "moveq.l #12,%%d0        \n"
        "add.l   %[buf],%%d0     \n"
        "and.l   #0xFFFFFFF0,%%d0\n" /* d0 == first line bound */
        "cmp.l   %[buf],%%d0     \n" /* any leading longwords? */
        "bls.b   .w_end_u_l1     \n" /* no: skip loop */
        
    ".w_loop_u_l1:               \n"
        "move.l  (%[buf])+,%%d3  \n"
        "move.l  %%d3,%%d4       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d3         \n"
        "or.l    %%d3,%%d2       \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "move.l  %%d4,%%d2       \n"
        "cmp.l   %[buf],%%d0     \n" /* run up to first line bound */
        "bhi.b   .w_loop_u_l1    \n"

    ".w_end_u_l1:                \n"
        "lea.l   (-14,%[end]),%[end] \n" /* adjust end addr. to 16 bytes/pass */

    ".w_loop_u_line:             \n"
        "movem.l (%[buf]),%%d3-%%d6  \n"
        "lea.l   (16,%[buf]),%[buf]  \n"
        "move.l  %%d3,%%d0       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d2       \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "move.l  %%d4,%%d0       \n"
        "lsl.l   %%d1,%%d3       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d3       \n"
        "swap    %%d3            \n"
        "move.w  %%d3,(%[ata])   \n"
        "swap    %%d3            \n"
        "move.w  %%d3,(%[ata])   \n"
        "move.l  %%d5,%%d0       \n"
        "lsl.l   %%d1,%%d4       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d4       \n"
        "swap    %%d4            \n"
        "move.w  %%d4,(%[ata])   \n"
        "swap    %%d4            \n"
        "move.w  %%d4,(%[ata])   \n"
        "move.l  %%d6,%%d0       \n"
        "lsl.l   %%d1,%%d5       \n"
        "lsr.l   #8,%%d0         \n"
        "or.l    %%d0,%%d5       \n"
        "swap    %%d5            \n"
        "move.w  %%d5,(%[ata])   \n"
        "swap    %%d5            \n"
        "move.w  %%d5,(%[ata])   \n"
        "move.l  %%d6,%%d2       \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to last line bound */
        "bhi.b   .w_loop_u_line  \n"

        "lea.l   (12,%[end]),%[end]  \n"  /* readjust for longword loop */
        "cmp.l   %[buf],%[end]   \n" /* any trailing longwords? */
        "bls.b   .w_end_u_l2     \n" /* no: skip loop */

    ".w_loop_u_l2:               \n"
        "move.l  (%[buf])+,%%d3  \n"
        "move.l  %%d3,%%d4       \n"
        "lsl.l   %%d1,%%d2       \n"
        "lsr.l   #8,%%d3         \n"
        "or.l    %%d3,%%d2       \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "move.l  %%d4,%%d2       \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to first line bound */
        "bhi.b   .w_loop_u_l2    \n"

    ".w_end_u_l2:                \n"
        "addq.l  #2,%[end]       \n" /* back to final end address */
        "cmp.l   %[buf],%[end]   \n" /* one word left? */
        "bls.b   .w_end_u_w2     \n"

        "swap    %%d2            \n"
        "move.w  (%[buf])+,%%d2  \n"
        "move.l  %%d2,%%d3       \n"
        "lsr.l   #8,%%d3         \n"
        "move.w  %%d3,(%[ata])   \n"

    ".w_end_u_w2:                \n"
        "lsl.l   #8,%%d2         \n"
        "move.b  (%[buf])+,%%d2  \n"
        "move.w  %%d2,(%[ata])   \n"
        "bra.b    .w_exit        \n"

        /* 16-bit aligned */
    ".w_aligned:                 \n"
        "btst.l  #1,%%d0         \n"
        "beq.b   .w_end_a_w1     \n"

        "move.w  (%[buf])+,(%[ata])  \n"  /* copy initial word */

    ".w_end_a_w1:                \n"
        "moveq.l #12,%%d0        \n"
        "add.l   %[buf],%%d0     \n"
        "and.l   #0xFFFFFFF0,%%d0\n" /* d0 == first line bound */
        "cmp.l   %[buf],%%d0     \n" /* any leading longwords? */
        "bls.b   .w_end_a_l1     \n" /* no: skip loop */

    ".w_loop_a_l1:               \n"
        "move.l  (%[buf])+,%%d1  \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "cmp.l   %[buf],%%d0     \n" /* run up to first line bound */
        "bhi.b   .w_loop_a_l1    \n"

    ".w_end_a_l1:                \n"
        "lea.l   (-14,%[end]),%[end] \n" /* adjust end addr. to 16 bytes/pass */

    ".w_loop_a_line:             \n"
        "movem.l (%[buf]),%%d0-%%d3  \n"
        "lea.l   (16,%[buf]),%[buf]  \n"
        "swap    %%d0            \n"
        "move.w  %%d0,(%[ata])   \n"
        "swap    %%d0            \n"
        "move.w  %%d0,(%[ata])   \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "swap    %%d2            \n"
        "move.w  %%d2,(%[ata])   \n"
        "swap    %%d3            \n"
        "move.w  %%d3,(%[ata])   \n"
        "swap    %%d3            \n"
        "move.w  %%d3,(%[ata])   \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to last line bound */
        "bhi.b   .w_loop_a_line  \n"
        
        "lea.l   (12,%[end]),%[end]  \n"  /* readjust for longword loop */
        "cmp.l   %[buf],%[end]   \n" /* any trailing longwords? */
        "bls.b   .w_end_a_l2     \n" /* no: skip loop */

    ".w_loop_a_l2:               \n"
        "move.l  (%[buf])+,%%d1  \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "swap    %%d1            \n"
        "move.w  %%d1,(%[ata])   \n"
        "cmp.l   %[buf],%[end]   \n" /* run up to first line bound */
        "bhi.b   .w_loop_a_l2    \n"

    ".w_end_a_l2:                \n"
        "addq.l  #2,%[end]       \n" /* back to final end address */
        "cmp.l   %[buf],%[end]   \n" /* one word left? */
        "bls.b   .w_end_a_w2     \n"

        "move.w  (%[buf])+,(%[ata])  \n"  /* copy final word */

    ".w_end_a_w2:                \n"

    ".w_exit:                    \n"
        : /* outputs */
        [buf]"+a"(buf),
        [end]"+a"(bufend)
        : /* inputs */
        [ata]"a"(&ATA_DATA)
        : /*trashed */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6"
    );
#else
    /* SH1 optimized assembler version */
    /* this assumes wordcount to be a multiple of 2 */

/* writing is not unrolled as much as reading, for several reasons:
 * - a similar instruction sequence is faster for writing than for reading
 *   because the auto-incrementing load inctructions can be used
 * - writing profits from warp mode
 * Both of these add up to have writing faster than the more unrolled reading.
 */
    asm volatile (
        "mov     %[buf],r0       \n"
        "tst     #1,r0           \n"  /* 16-bit aligned ? */
        "bt      .w_aligned      \n"  /* yes, do word copy */

        /* not 16-bit aligned */
        "mov     #-1,r6          \n"  /* prepare a bit mask for high byte */
        "shll8   r6              \n"  /* r6 = 0xFFFFFF00 */

        "mov.b   @%[buf]+,r2     \n"  /* load (initial old second) first byte */
        "mov.w   @%[buf]+,r3     \n"  /* load (initial) first word */
        "bra     .w_start2_b     \n"
        "extu.b  r2,r0           \n"  /* extend unsigned */

        ".align  2               \n"
    ".w_loop2_b:                 \n"  /* main loop: copy 2 words in a row */
        "mov.w   @%[buf]+,r3     \n"  /* load first word (2+ round) */
        "extu.b  r2,r0           \n"  /* put away low byte of second word (2+ round) */
        "and     r6,r2           \n"  /* get high byte of second word (2+ round) */
        "or      r1,r2           \n"  /* combine with low byte of old first word */
        "mov.w   r2,@%[ata]      \n"  /* write that */
    ".w_start2_b:                \n"
        "cmp/hi  %[buf],%[end]   \n"  /* check for end */
        "mov.w   @%[buf]+,r2     \n"  /* load second word */
        "extu.b  r3,r1           \n"  /* put away low byte of first word */
        "and     r6,r3           \n"  /* get high byte of first word */
        "or      r0,r3           \n"  /* combine with high byte of old second word */
        "mov.w   r3,@%[ata]      \n"  /* write that */
        "bt      .w_loop2_b      \n"
        /* 12 instructions for 2 copies, takes 14 clock cycles */
        /* avg. 7 cycles per word - 85% faster */

        /* the loop "overreads" 1 byte past the buffer end, however, the last */
        /* byte is not written to disk */
        "and     r6,r2           \n"  /* get high byte of last word */
        "or      r1,r2           \n"  /* combine with low byte of old first word */
        "bra     .w_exit         \n"
        "mov.w   r2,@%[ata]      \n"  /* write last word */

        /* 16-bit aligned, loop(load and write word) */
    ".w_aligned:                 \n"
        "bra     .w_start2_w     \n"  /* jump into loop after next instr. */
        "mov.w   @%[buf]+,r2     \n"  /* load first word (1st round) */

        ".align  2               \n"
    ".w_loop2_w:                 \n"  /* main loop: copy 2 words in a row */
        "mov.w   @%[buf]+,r2     \n"  /* load first word (2+ round) */
        "swap.b  r1,r0           \n"  /* swap second word (2+ round) */
        "mov.w   r0,@%[ata]      \n"  /* write second word (2+ round) */
    ".w_start2_w:                \n"
        "cmp/hi  %[buf],%[end]   \n"  /* check for end */
        "mov.w   @%[buf]+,r1     \n"  /* load second word */
        "swap.b  r2,r0           \n"  /* swap first word */
        "mov.w   r0,@%[ata]      \n"  /* write first word */
        "bt      .w_loop2_w      \n"
        /* 8 instructions for 2 copies, takes 10 clock cycles */
        /* avg. 5 cycles per word - 100% faster */

        "swap.b  r1,r0           \n"  /* swap second word (last round) */
        "mov.w   r0,@%[ata]      \n"  /* and write it */

    ".w_exit:                    \n"
        : /* outputs */
        [buf]"+r"(buf)
        : /* inputs */
        [end]"r"(buf + 2 * wordcount - 4), /* adjusted for earl check */
        [ata]"r"(&ATA_DATA)
        : /*trashed */
        "r0","r1","r2","r3","r6"
    );
#endif /* CPU */
#endif /* !PREFER_C_WRITING */
}

int ata_write_sectors(IF_MV2(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    int i;
    int ret = 0;
    long spinup_start;

#ifdef HAVE_MULTIVOLUME
    (void)drive; /* unused for now */
#endif
    if (start == 0)
        panicf("Writing on sector 0\n");

    mutex_lock(&ata_mtx);
    
    last_disk_activity = current_tick;
    spinup_start = current_tick;

    ata_led(true);

    if ( sleeping ) {
        spinup = true;
        if (poweroff) {
            if (ata_power_on()) {
                mutex_unlock(&ata_mtx);
                ata_led(false);
                return -1;
            }
        }
        else {
            if (perform_soft_reset()) {
                mutex_unlock(&ata_mtx);
                ata_led(false);
                return -1;
            }
        }
    }
    
    SET_REG(ATA_SELECT, ata_device);
    if (!wait_for_rdy())
    {
        mutex_unlock(&ata_mtx);
        ata_led(false);
        return -2;
    }

    if ( count == 256 )
        SET_REG(ATA_NSECTOR, 0); /* 0 means 256 sectors */
    else
        SET_REG(ATA_NSECTOR, (unsigned char)count);
    SET_REG(ATA_SECTOR, start & 0xff);
    SET_REG(ATA_LCYL, (start >> 8) & 0xff);
    SET_REG(ATA_HCYL, (start >> 16) & 0xff);
    SET_REG(ATA_SELECT, ((start >> 24) & 0xf) | SELECT_LBA | ata_device);
    SET_REG(ATA_COMMAND, CMD_WRITE_SECTORS);

    for (i=0; i<count; i++) {

        if (!wait_for_start_of_transfer()) {
            ret = -3;
            break;
        }

        if (spinup) {
            ata_spinup_time = current_tick - spinup_start;
            spinup = false;
            sleeping = false;
            poweroff = false;
        }

        copy_write_sectors(buf, SECTOR_SIZE/2);

#ifdef USE_INTERRUPT
        /* reading the status register clears the interrupt */
        j = ATA_STATUS;
#endif
        buf += SECTOR_SIZE;

        last_disk_activity = current_tick;
    }

    if(!ret && !wait_for_end_of_transfer()) {
        DEBUGF("End on transfer failed. -- jyp");
        ret = -4;
    }

    ata_led(false);

    mutex_unlock(&ata_mtx);

    /* only flush if writing went ok */
    if ( (ret == 0) && delayed_write )
        ata_flush();

    return ret;
}

/* schedule a single sector write, executed with the the next spinup 
   (volume 0 only, used for config sector) */
extern void ata_delayed_write(unsigned long sector, const void* buf)
{
    memcpy(delayed_sector, buf, SECTOR_SIZE);
    delayed_sector_num = sector;
    delayed_write = true;
}

/* write the delayed sector to volume 0 */
extern void ata_flush(void)
{
    if ( delayed_write ) {
        DEBUGF("ata_flush()\n");
        delayed_write = false;
        ata_write_sectors(IF_MV2(0,) delayed_sector_num, 1, delayed_sector);
    }
}



static int check_registers(void)
{
#if (CONFIG_CPU == PP5002)
    /* This fails on the PP5002, but the ATA driver still works.  This
       needs more investigation. */
    return 0;
#else
    int i;
    if ( ATA_STATUS & STATUS_BSY )
            return -1;

    for (i = 0; i<64; i++) {
        SET_REG(ATA_NSECTOR, WRITE_PATTERN1);
        SET_REG(ATA_SECTOR,  WRITE_PATTERN2);
        SET_REG(ATA_LCYL,    WRITE_PATTERN3);
        SET_REG(ATA_HCYL,    WRITE_PATTERN4);

        if (((ATA_NSECTOR & READ_PATTERN1_MASK) == READ_PATTERN1) &&
            ((ATA_SECTOR & READ_PATTERN2_MASK) == READ_PATTERN2) &&
            ((ATA_LCYL & READ_PATTERN3_MASK) == READ_PATTERN3) &&
            ((ATA_HCYL & READ_PATTERN4_MASK) == READ_PATTERN4))
            return 0;
    }
    return -2;
#endif
}

static int freeze_lock(void)
{
    /* does the disk support Security Mode feature set? */
    if (identify_info[82] & 2)
    {
        SET_REG(ATA_SELECT, ata_device);

        if (!wait_for_rdy())
            return -1;

        SET_REG(ATA_COMMAND, CMD_SECURITY_FREEZE_LOCK);

        if (!wait_for_rdy())
            return -2;
    }

    return 0;
}

void ata_spindown(int seconds)
{
    sleep_timeout = seconds * HZ;
}

#ifdef HAVE_ATA_POWER_OFF
void ata_poweroff(bool enable)
{
    if (enable)
        poweroff_timeout = 2*HZ;
    else
        poweroff_timeout = 0;
}
#endif

bool ata_disk_is_active(void)
{
    return !sleeping;
}

static int ata_perform_sleep(void)
{
    int ret = 0;

    mutex_lock(&ata_mtx);

    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("ata_perform_sleep() - not RDY\n");
        mutex_unlock(&ata_mtx);
        return -1;
    }

    SET_REG(ATA_COMMAND, CMD_SLEEP);

    if (!wait_for_rdy())
    {
        DEBUGF("ata_perform_sleep() - CMD failed\n");
        ret = -2;
    }

    sleeping = true;
    mutex_unlock(&ata_mtx);
    return ret;
}

void ata_sleep(void)
{
    queue_post(&ata_queue, Q_SLEEP, NULL);
}

void ata_spin(void)
{
    last_user_activity = current_tick;
}

static void ata_thread(void)
{
    static long last_sleep = 0;
    struct event ev;
    
    while (1) {
        while ( queue_empty( &ata_queue ) ) {
            if ( !spinup && sleep_timeout && !sleeping &&
                 TIME_AFTER( current_tick, 
                             last_user_activity + sleep_timeout ) &&
                 TIME_AFTER( current_tick, 
                             last_disk_activity + sleep_timeout ) )
            {
                ata_perform_sleep();
                last_sleep = current_tick;
            }

#ifdef HAVE_ATA_POWER_OFF
            if ( !spinup && sleeping && poweroff_timeout && !poweroff &&
                 TIME_AFTER( current_tick, last_sleep + poweroff_timeout ))
            {
                mutex_lock(&ata_mtx);
                ide_power_enable(false);
                mutex_unlock(&ata_mtx);
                poweroff = true;
            }
#endif

            sleep(HZ/4);
        }
        queue_wait(&ata_queue, &ev);
        switch ( ev.id ) {
#ifndef USB_NONE
            case SYS_USB_CONNECTED:
                if (poweroff) {
                    mutex_lock(&ata_mtx);
                    ata_led(true);
                    ata_power_on();
                    ata_led(false);
                    mutex_unlock(&ata_mtx);
                }

                /* Tell the USB thread that we are safe */
                DEBUGF("ata_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);

                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&ata_queue);
                break;
#endif
            case Q_SLEEP:
                last_disk_activity = current_tick - sleep_timeout + (HZ/2);
                break;
        }
    }
}

/* Hardware reset protocol as specified in chapter 9.1, ATA spec draft v5 */
int ata_hard_reset(void)
{
    int ret;

#ifdef TARGET_TREE
    ata_reset();
#elif CONFIG_CPU == SH7034
    /* state HRR0 */
    and_b(~0x02, &PADRH); /* assert _RESET */
    sleep(1); /* > 25us */

    /* state HRR1 */
    or_b(0x02, &PADRH); /* negate _RESET */
    sleep(1); /* > 2ms */
#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
    and_l(~0x00080000, &GPIO_OUT);
    sleep(1); /* > 25us */

    or_l(0x00080000, &GPIO_OUT);
    sleep(1); /* > 25us */
#elif CONFIG_CPU == TCC730

    P6 &= ~0x40;
    ddma_transfer(0, 1, ide_sector_data, 0xF00000, SECTOR_SIZE);
    P6 |= 0x40;
    
    /*
      What can the following do?
    P1 |= 0x04;
    P10CON &= ~0x56;
    sleep(1);

    P10CON |= 0x56;
    P10 &= ~0x56;
    P1 &= ~0x04;
    sleep(1);
    */
#endif

    /* state HRR2 */
    SET_REG(ATA_SELECT, ata_device); /* select the right device */
    ret = wait_for_bsy();

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    return ret;
}

static int perform_soft_reset(void)
{
/* If this code is allowed to run on a Nano, the next reads from the flash will
 * time out, so we disable it. It shouldn't be necessary anyway, since the
 * ATA -> Flash interface automatically sleeps almost immediately after the
 * last command.
 */
#ifndef IPOD_NANO
    int ret;
    int retry_count;
    
    SET_REG(ATA_SELECT, SELECT_LBA | ata_device );
    SET_REG(ATA_CONTROL, CONTROL_nIEN|CONTROL_SRST );
    sleep(1); /* >= 5us */

    SET_REG(ATA_CONTROL, CONTROL_nIEN);
    sleep(1); /* >2ms */

    /* This little sucker can take up to 30 seconds */
    retry_count = 8;
    do
    {
        ret = wait_for_rdy();
    } while(!ret && retry_count--);

    /* Massage the return code so it is 0 on success and -1 on failure */
    ret = ret?0:-1;

    return ret;
#else
    return 0; /* Always report success */
#endif
}

int ata_soft_reset(void)
{
    int ret;
    
    mutex_lock(&ata_mtx);

    ret = perform_soft_reset();

    mutex_unlock(&ata_mtx);
    return ret;
}

static int ata_power_on(void)
{
    int rc;
    
    ide_power_enable(true);
    if( ata_hard_reset() )
        return -1;

    rc = set_features();
    if (rc)
        return rc * 10 - 2;

    if (set_multiple_mode(multisectors))
        return -3;

    if (freeze_lock())
        return -4;

    return 0;
}

static int master_slave_detect(void)
{
    /* master? */
    SET_REG(ATA_SELECT, 0);
    if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
        ata_device = 0;
        DEBUGF("Found master harddisk\n");
    }
    else {
        /* slave? */
        SET_REG(ATA_SELECT, SELECT_DEVICE1);
        if ( ATA_STATUS & (STATUS_RDY|STATUS_BSY) ) {
            ata_device = SELECT_DEVICE1;
            DEBUGF("Found slave harddisk\n");
        }
        else
            return -1;
    }
    return 0;
}

#if CONFIG_CPU == SH7034 /* special archos quirk */
static void io_address_detect(void)
{   /* now, use the HW mask instead of probing */
    if (read_hw_mask() & ATA_ADDRESS_200)
    {
        ata_io_address = 0x200; /* For debug purposes only */
        old_recorder = false;
        ata_control = ATA_CONTROL1;
    }
    else
    {
        ata_io_address = 0x300; /* For debug purposes only */
        old_recorder = true;
        ata_control = ATA_CONTROL2;
    }
}
#endif

#ifndef TARGET_TREE
void ata_enable(bool on)
{
#if CONFIG_CPU == SH7034
    if(on)
        and_b(~0x80, &PADRL); /* enable ATA */
    else
        or_b(0x80, &PADRL); /* disable ATA */

    or_b(0x80, &PAIORL);
#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
    if(on)
        and_l(~0x0040000, &GPIO_OUT);
    else
        or_l(0x0040000, &GPIO_OUT);
    
    or_l(0x00040000, &GPIO_ENABLE);
    or_l(0x00040000, &GPIO_FUNCTION);
#elif CONFIG_CPU == TCC730

#elif (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    /* TODO: Implement ata_enable() */
    (void)on;
#endif
}
#endif

static int identify(void)
{
    int i;

    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("identify() - not RDY\n");
        return -1;
    }
    SET_REG(ATA_COMMAND, CMD_IDENTIFY);

    if (!wait_for_start_of_transfer())
    {
        DEBUGF("identify() - CMD failed\n");
        return -2;
    }

    for (i=0; i<SECTOR_SIZE/2; i++) {
        /* the IDENTIFY words are already swapped, so we need to treat
           this info differently that normal sector data */
#if defined(ROCKBOX_BIG_ENDIAN) && !defined(SWAP_WORDS)
        identify_info[i] = swap16(ATA_DATA);
#else
        identify_info[i] = ATA_DATA;
#endif
    }
    
    return 0;
}

static int set_multiple_mode(int sectors)
{
    SET_REG(ATA_SELECT, ata_device);

    if(!wait_for_rdy()) {
        DEBUGF("set_multiple_mode() - not RDY\n");
        return -1;
    }

    SET_REG(ATA_NSECTOR, sectors);
    SET_REG(ATA_COMMAND, CMD_SET_MULTIPLE_MODE);

    if (!wait_for_rdy())
    {
        DEBUGF("set_multiple_mode() - CMD failed\n");
        return -2;
    }

    return 0;
}

static int set_features(void)
{
    struct {
        unsigned char id_word;
        unsigned char id_bit;
        unsigned char subcommand;
        unsigned char parameter;
    } features[] = {
        { 83, 3, 0x05, 0x80 }, /* power management: lowest power without standby */
        { 83, 9, 0x42, 0x80 }, /* acoustic management: lowest noise */
        { 82, 6, 0xaa, 0 },    /* enable read look-ahead */
        { 83, 14, 0x03, 0 },   /* force PIO mode */
        { 0, 0, 0, 0 }         /* <end of list> */
    };
    int i;
    int pio_mode = 2;

    /* Find out the highest supported PIO mode */
    if(identify_info[64] & 2)
        pio_mode = 4;
    else
        if(identify_info[64] & 1)
            pio_mode = 3;

    /* Update the table */
    features[3].parameter = 8 + pio_mode;
    
    SET_REG(ATA_SELECT, ata_device);

    if (!wait_for_rdy()) {
        DEBUGF("set_features() - not RDY\n");
        return -1;
    }

    for (i=0; features[i].id_word; i++) {
        if (identify_info[features[i].id_word] & (1 << features[i].id_bit)) {
            SET_REG(ATA_FEATURE, features[i].subcommand);
            SET_REG(ATA_NSECTOR, features[i].parameter);
            SET_REG(ATA_COMMAND, CMD_SET_FEATURES);

            if (!wait_for_rdy()) {
                DEBUGF("set_features() - CMD failed\n");
                return -10 - i;
            }

            if(ATA_ALT_STATUS & STATUS_ERR) {
                if(ATA_ERROR & ERROR_ABRT) {
                    return -20 - i;
                }
            }
        }
    }

    return 0;
}

unsigned short* ata_get_identify(void)
{
    return identify_info;
}

static int init_and_check(bool hard_reset)
{
    int rc;

    if (hard_reset)
    {
        /* This should reset both master and slave, we don't yet know what's in */
        ata_device = 0;
        if (ata_hard_reset())
            return -1;
    }

    rc = master_slave_detect();
    if (rc)
        return -10 + rc;

    /* symptom fix: else check_registers() below may fail */
    if (hard_reset && !wait_for_bsy())
        return -20;

    rc = check_registers();
    if (rc)
        return -30 + rc;
    
    return 0;
}

int ata_init(void)
{
    int rc;
#ifdef TARGET_TREE
    bool coldstart = ata_is_coldstart();
#elif CONFIG_CPU == TCC730
    bool coldstart = (P1 & 0x80) == 0;
#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
    bool coldstart = (GPIO_FUNCTION & 0x00080000) == 0;
#elif (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020)
    bool coldstart = false;
    /* TODO: Implement coldstart variable */
#elif defined(TOSHIBA_GIGABEAT_F)
    /* TODO */
    bool coldstart = true;
#else
    bool coldstart = (PACR2 & 0x4000) != 0;
#endif

    mutex_init(&ata_mtx);

    ata_led(false);

#ifdef TARGET_TREE
    ata_device_init();
#elif CONFIG_CPU == SH7034
    /* Port A setup */
    or_b(0x02, &PAIORH); /* output for ATA reset */
    or_b(0x02, &PADRH); /* release ATA reset */
    PACR2 &= 0xBFFF; /* GPIO function for PA7 (IDE enable) */
#elif CONFIG_CPU == MCF5249
#ifdef HAVE_ATA_LED_CTRL
    /* Enable disk LED & ISD chip power control */
    and_l(~0x0000240, &GPIO_OUT);
    or_l(0x00000240, &GPIO_ENABLE);
    or_l(0x00000200, &GPIO_FUNCTION);
#endif
    
    /* ATA reset */
    or_l(0x00080000, &GPIO_OUT);
    or_l(0x00080000, &GPIO_ENABLE);
    or_l(0x00080000, &GPIO_FUNCTION);

    /* FYI: The IDECONFIGx registers are set by set_cpu_frequency() */
#elif CONFIG_CPU == PP5002
    /* From ipod-ide.c:ipod_ide_register() */
    outl(inl(0xc0003024) | (1 << 7), 0xc0003024);
    outl(inl(0xc0003024) & ~(1<<2), 0xc0003024);

    outl(0x10, 0xc0003000);
    outl(0x80002150, 0xc0003004);
#elif CONFIG_CPU == PP5020
    /* From ipod-ide.c:ipod_ide_register() */
    outl(inl(0xc3000028) | (1 << 5), 0xc3000028);
    outl(inl(0xc3000028) & ~0x10000000, 0xc3000028);

    outl(0x10, 0xc3000000);
    outl(0x80002150, 0xc3000004);
#endif

    sleeping = false;
    ata_enable(true);

    if ( !initialized ) {
        if (!ide_powered()) /* somebody has switched it off */
        {
            ide_power_enable(true);
            sleep(HZ); /* allow voltage to build up */
        }

#if CONFIG_CPU == SH7034
        io_address_detect();
#endif  
        /* first try, hard reset at cold start only */
        rc = init_and_check(coldstart);  

        if (rc) 
        {   /* failed? -> second try, always with hard reset */
            DEBUGF("ata: init failed, retrying...\n");
            rc  = init_and_check(true);
            if (rc)
                return rc;
        }

        rc = identify();

        if (rc)
            return -40 + rc;

        multisectors = identify_info[47] & 0xff;
        DEBUGF("ata: %d sectors per ata request\n",multisectors);

        rc = freeze_lock();

        if (rc)
            return -50 + rc;

        rc = set_features();
        if (rc)
            return -60 + rc;

        queue_init(&ata_queue, true);

        last_disk_activity = current_tick;
        create_thread(ata_thread, ata_stack,
                      sizeof(ata_stack), ata_thread_name
                      IF_PRIO(, PRIORITY_SYSTEM));
        initialized = true;

    }
    rc = set_multiple_mode(multisectors);
    if (rc)
        return -70 + rc;

    return 0;
}

#if CONFIG_LED == LED_REAL
void ata_set_led_enabled(bool enabled) {
    ata_led_enabled = enabled;
    if (ata_led_enabled) {
        led(ata_led_on);
    } else {
        led(false);
    }
}
#endif
