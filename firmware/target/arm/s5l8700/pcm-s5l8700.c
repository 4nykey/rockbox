/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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
#include <string.h>

#include "config.h"
#include "system.h"
#include "audio.h"
#include "s5l8700.h"
#include "panic.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm_sampr.h"
#include "dma-target.h"
#include "mmu-target.h"

/*  Driver for the IIS/PCM part of the s5l8700 using DMA

    Notes:
    - not all possible PCM sample rates are enabled (no support in codec driver)
    - pcm_play_dma_pause is untested, not sure if implemented the right way
    - pcm_play_dma_stop is untested, not sure if implemented the right way
    - pcm_play_dma_get_peak_buffer is not implemented
    - recording is not implemented
*/

static void             dma_callback(void);
static volatile int     locked = 0;

/* table of recommended PLL/MCLK dividers for mode 256Fs from the datasheet */
static const struct div_entry {
    int             pdiv, mdiv, sdiv, cdiv;
} div_table[HW_NUM_FREQ] = {
#ifdef IPOD_NANO2G
    [HW_FREQ_11] = {   2,   41,    5,    4},
    [HW_FREQ_22] = {   2,   41,    4,    4},
    [HW_FREQ_44] = {   2,   41,    3,    4},
    [HW_FREQ_88] = {   2,   41,    2,    4},
#if 0   /* disabled because the codec driver does not support it (yet) */
    [HW_FREQ_8 ] = {   2,   12,    3,    9}
    [HW_FREQ_16] = {   2,   12,    2,    9},
    [HW_FREQ_32] = {   2,   12,    1,    9},
    
    [HW_FREQ_12] = {   2,   12,    4,    3},
    [HW_FREQ_24] = {   2,   12,    3,    3},
    [HW_FREQ_48] = {   2,   12,    2,    3},
    [HW_FREQ_96] = {   2,   12,    1,    3},
#endif
#else
    [HW_FREQ_11] = {  26,  189,    3,    8},
    [HW_FREQ_22] = {  50,   98,    2,    8},
    [HW_FREQ_44] = {  37,  151,    1,    9},
    [HW_FREQ_88] = {  50,   98,    1,    4},
#if 0   /* disabled because the codec driver does not support it (yet) */
    [HW_FREQ_8 ] = {  28,  192,    3,   12}
    [HW_FREQ_16] = {  28,  192,    3,    6},
    [HW_FREQ_32] = {  28,  192,    2,    6},
    
    [HW_FREQ_12] = {  28,  192,    3,    8},
    [HW_FREQ_24] = {  28,  192,    2,    8},
    [HW_FREQ_48] = {  28,  192,    2,    4},
    [HW_FREQ_96] = {  28,  192,    1,    4},
#endif
#endif
};

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (locked++ == 0) {
        INTMSK &= ~(1 << 10);
    }
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--locked == 0) {
        INTMSK |= (1 << 10);
    }
}

static void play_next(void *addr, size_t size)
{
    /* setup DMA */
    dma_setup_channel(DMA_IISOUT_CHANNEL, DMA_IISOUT_SELECT, DMA_MEM_TO_IO,
                      DMA_IISOUT_DSIZE, DMA_IISOUT_BLEN, addr, size / 2,
                      dma_callback);
    
    /* DMA channel on */
    clean_dcache();
    dma_enable_channel(DMA_IISOUT_CHANNEL);
}

static void dma_callback(void)
{
    unsigned char *dma_start_addr;
    size_t dma_size;
    
    register pcm_more_callback_type get_more = pcm_callback_for_more;
    if (get_more) {
        get_more(&dma_start_addr, &dma_size);
        
        if (dma_size == 0) {
            pcm_play_dma_stop();
            pcm_play_dma_stopped_callback();
        }
        else {
            play_next(dma_start_addr, dma_size);
        }
    }
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    /* S1: DMA channel 0 set */
    dma_setup_channel(DMA_IISOUT_CHANNEL, DMA_IISOUT_SELECT, DMA_MEM_TO_IO,
                      DMA_IISOUT_DSIZE, DMA_IISOUT_BLEN, (void *)addr, size / 2,
                      dma_callback);

#ifdef IPOD_NANO2G
    PCON5 = (PCON5 & ~(0xFFFF0000)) | 0x77720000;
    PCON6 = (PCON6 & ~(0x0F000000)) | 0x02000000;

    I2STXCON = (0x10 << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (5 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (2 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#else
    /* S2: IIS Tx mode set */
    I2STXCON = (DMA_IISOUT_BLEN << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (3 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (0 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#endif
    
    /* S3: DMA channel 0 on */
    clean_dcache();
    dma_enable_channel(DMA_IISOUT_CHANNEL);

    /* S4: IIS Tx clock on */
    I2SCLKCON = (1 << 0);   /* 1 = power on */
    
    /* S5: IIS Tx on */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (1 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

void pcm_play_dma_stop(void)
{
    /* DMA channel off */
    dma_disable_channel(DMA_IISOUT_CHANNEL);
    
    /* TODO Some time wait */
    /* LRCK half cycle wait */

    /* IIS Tx off */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (0 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

/* pause playback by disabling further DMA requests */
void pcm_play_dma_pause(bool pause)
{
    if (pause) {
        I2STXCOM &= ~(1 << 1);  /* DMA request enable */
    }
    else {
        I2STXCOM |= (1 << 1);   /* DMA request enable */
    }
}

void pcm_play_dma_init(void)
{
    /* configure IIS pins */
#ifdef IPOD_NANO2G
    PCON5 = (PCON5 & ~(0xFFFF0000)) | 0x22220000;
    PCON6 = (PCON6 & ~(0x0F000000)) | 0x02000000;
#else
    PCON7 = (PCON7 & ~(0x0FFFFF00)) | 0x02222200;
#endif

    /* enable clock to the IIS module */
    PWRCON &= ~(1 << 6);
    
    audiohw_preinit();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

/* set the configured PCM frequency */
void pcm_dma_apply_settings(void)
{
  //    audiohw_set_frequency(pcm_sampr);
    
    struct div_entry div = div_table[pcm_fsel];

    PLLCON &= ~4;
    PLLCON &= ~0x10;
    PLLCON &= 0x3f;
    PLLCON |= 4;

    /* configure PLL1 and MCLK for the desired sample rate */
    PLL1PMS = (div.pdiv << 16) |
              (div.mdiv << 8) |
              (div.sdiv << 0);
    PLL1LCNT = 7500;    /* no idea what to put here */

    /* enable PLL1 and wait for lock */
    PLLCON |= (1 << 1);
    while ((PLLLOCK & (1 << 1)) == 0);

    /* configure MCLK */
    CLKCON = (CLKCON & ~(0xFF)) | 
             (0 << 7) |         /* MCLK_MASK */
             (2 << 5) |         /* MCLK_SEL = PLL2 */
             (1 << 4) |         /* MCLK_DIV_ON */
             (div.cdiv - 1);    /* MCLK_DIV_VAL */
}

size_t pcm_get_bytes_waiting(void)
{
    return DMACTCNT0 * 2;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    *count = DMACTCNT0 >> 1;
    return (void *)((DMACADDR0 + 2) & ~3);
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = UNCACHED_ADDR(addr);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
}


const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    (void)count;
}

#endif /* HAVE_RECORDING */
