/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "types.h"
#include "i2c.h"
#include "mas.h"
#include "sh7034.h"
#include "system.h"
#include "debug.h"
#include "kernel.h"
#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "file.h"
#include "dir.h"
#include "panic.h"

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

unsigned char fliptable[] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

extern unsigned int stack[];
/* Place the MP3 data right after the stack */

#define MP3BUF_LEN 0x100000 /* 1 Mbyte */

unsigned char *mp3buf = (unsigned char *)stack;

int mp3buf_write;
int mp3buf_read;
int last_dma_chunk_size;

bool dma_on;
static void mas_poll_start(unsigned int interval_in_ms);

void setup_sci0(void)
{
    /* PB15 is I/O, PB14 is IRQ6, PB12 is SCK0 */
    PBCR1 = (PBCR1 & 0x0cff) | 0x1200;
    
    /* Set PB12 to output */
    PBIOR |= 0x1000;

    /* Disable serial port */
    SCR0 = 0x00;

    /* Synchronous, no prescale */
    SMR0 = 0x80;

    /* Set baudrate 1Mbit/s */
    BRR0 = 0x03;

    /* use SCK as serial clock output */
    SCR0 = 0x01;

    /* Clear FER and PER */
    SSR0 &= 0xe7;

    /* Set interrupt ITU2 and SCI0 priority to 0 */
    IPRD &= 0x0ff0;

    /* set IRQ6 and IRQ7 to edge detect */
    ICR |= 0x03;

    /* set PB15 and PB14 to inputs */
    PBIOR &= 0x7fff;
    PBIOR &= 0xbfff;

    /* set IRQ6 prio 8 and IRQ7 prio 0 */
    IPRB = ( IPRB & 0xff00 ) | 0x0080;

    /* Enable End of DMA interrupt at prio 8 */
    IPRC = (IPRC & 0xf0ff) | 0x0800;
    
    /* Enable Tx (only!) */
//    SCR0 |= 0xa0;
}

int mas_tx_ready(void)
{
    return (SSR0 & SCI_TDRE);
}

void init_dma(void)
{
    SAR3 = (unsigned int) mp3buf + mp3buf_read;
    DAR3 = 0x5FFFEC3;
    CHCR3 &= ~0x0002; /* Clear interrupt */
    CHCR3 = 0x1504; /* Single address destination, TXI0, IE=1 */
    last_dma_chunk_size = MIN(65536, mp3buf_write - mp3buf_read);
    DTCR3 = last_dma_chunk_size & 0xffff;
    DMAOR = 0x0001; /* Enable DMA */
}

void start_dma(void)
{
    SCR0 |= 0x80;
    dma_on = TRUE;
}

void stop_dma(void)
{
    SCR0 &= 0x7f;
    dma_on = FALSE;
}

void dma_tick(void)
{
    if(!dma_on)
    {
        if(PBDR & 0x4000)
        {
            if(!(SCR0 & 0x80))
                start_dma();
        }
    }
}

void bitswap(unsigned char *data, int length)
{
    unsigned int i;
    for(i = 0;i < length;i++)
    {
        data[i] = fliptable[data[i]];
    }
}

struct event_queue disk_queue;
int filling;

int main(void)
{
    char buf[40];
    char str[32];
    int i=0;
    DIR *d;
    struct dirent *dent;
    int f;
    int free_space_left;
    int mp3_space_left;
    int amount_to_read;
    int play_song;
    struct event *ev;
    
    /* Clear it all! */
    SSR1 &= ~(SCI_RDRF | SCI_ORER | SCI_PER | SCI_FER);

    /* This enables the serial Rx interrupt, to be able to exit into the
       debugger when you hit CTRL-C */
    SCR1 |= 0x40;
    SCR1 &= ~0x80;

    IPRE |= 0xf000; /* Highest priority */

    i2c_init();

    dma_on = TRUE;
    
    kernel_init();

    set_irq_level(0);

    setup_sci0();

    i=mas_readmem(MAS_BANK_D1,0xff6,(unsigned long*)buf,2);
    if (i) {
        debugf("Error - mas_readmem() returned %d\n", i);
        while(1);
    }

    i = buf[0] | buf[1] << 8;
    debugf("MAS version: %x\n", i);
    i = buf[4] | buf[5] << 8;
    debugf("MAS revision: %x\n", i);

    i=mas_readmem(MAS_BANK_D1,0xff9,(unsigned long*)buf,7);
    if (i) {
        debugf("Error - mas_readmem() returned %d\n", i);
        while(1);
    }

    for(i = 0;i < 7;i++)
    {
        str[i*2+1] = buf[i*4];
        str[i*2] = buf[i*4+1];
    }
    str[i*2] = 0;
    debugf("Description: %s\n", str);

    i=mas_writereg(0x3b, 0x20);
    if (i < 0) {
        debugf("Error - mas_writereg() returned %d\n", i);
        while(1);
    }

    i = mas_run(1);
    if (i < 0) {
        debugf("Error - mas_run() returned %d\n", i);
        while(1);
    }

    i = ata_init();
    debugf("ata_init() returned %d\n", i);

    i = disk_init();
    debugf("disk_init() returned %d\n", i);

    debugf("part[0] starts at sector %d\n", part[0].start);
    
    i = fat_mount(part[0].start);
    debugf("fat_mount() returned %d\n", i);
    
    if((d = opendir("/")))
    {
        while((dent = readdir(d)))
        {
            debugf("%s\n", dent->d_name);
        }
        closedir(d);
    }

    f = open("/machinae_supremacy_-_arcade.mp3", O_RDONLY);
    if(f < 0)
    {
        debugf("Couldn't open file\n");
    }

    mp3buf_read = mp3buf_write = 0;
    
    /* First read in a few seconds worth of MP3 data */
    i = read(f, mp3buf, 0x20000);
    debugf("Read %d bytes\n", i);

    ata_spindown(-1);

    debugf("bit swapping...\n");
    bitswap(mp3buf + mp3buf_write, i);
    
    mp3buf_write = i;

    queue_init(&disk_queue);
    
    mas_poll_start(1);
    
    debugf("let's play...\n");
    init_dma();
    
    dma_on = TRUE;
    
    /* Enable Tx & TXIE */
    SCR0 |= 0xa0;
    
    CHCR3 |= 1;

#define MP3_LOW_WATER 0x30000
#define MP3_CHUNK_SIZE 0x20000

    play_song = 1;
    filling = 1;
    
    while(play_song)
    {
        if(filling)
        {
            free_space_left = mp3buf_read - mp3buf_write;
            if(free_space_left < 0)
                free_space_left = MP3BUF_LEN + free_space_left;

            if(free_space_left <= MP3_CHUNK_SIZE)
            {
                debugf("0\n");
                ata_spindown(-1);
                filling = 0;
                continue;
            }
            
            amount_to_read = MIN(MP3_CHUNK_SIZE, free_space_left);
            amount_to_read = MIN(MP3BUF_LEN - mp3buf_write, amount_to_read);
            
            /* Read in a few seconds worth of MP3 data. We don't want to
               read too large chunks because the bitswapping will take
               too much time. We must keep the DMA happy and also give
               the other threads a chance to run. */
            debugf("R\n", i);
            i = read(f, mp3buf+mp3buf_write, amount_to_read);
            if(i)
            {
                debugf("B\n");
                bitswap(mp3buf + mp3buf_write, i);
            
                mp3buf_write += i;
                if(mp3buf_write >= MP3BUF_LEN)
                {
                    mp3buf_write = 0;
                    debugf("W\n");
                }
            }
            else
            {
                play_song = 0;
                ata_spindown(-1);
                filling = 0;
            }
        }
        else
        {
            debugf("S\n");
            ev = queue_wait(&disk_queue);
            debugf("Q\n");
            debugf("1\n");
            filling = 1;
        }
    }
    debugf("Song is finished\n");
    while(1);
}

#pragma interrupt
void IRQ6(void)
{
    stop_dma();
}

#pragma interrupt
void DEI3(void)
{
    int unplayed_space_left;
    int space_until_end_of_buffer;
    mp3buf_read += last_dma_chunk_size;
    if(mp3buf_read >= MP3BUF_LEN)
        mp3buf_read = 0;
    
    unplayed_space_left = mp3buf_write - mp3buf_read;
    if(unplayed_space_left < 0)
       unplayed_space_left = MP3BUF_LEN + unplayed_space_left;

    space_until_end_of_buffer = MP3BUF_LEN - mp3buf_read;

    if(!filling && unplayed_space_left < MP3_LOW_WATER)
    {
        queue_post(&disk_queue, 1, 0);
    }
    
    if(unplayed_space_left)
    {
        last_dma_chunk_size = MIN(65536, unplayed_space_left);
        last_dma_chunk_size = MIN(last_dma_chunk_size, space_until_end_of_buffer);
        DTCR3 = last_dma_chunk_size & 0xffff;
        SAR3 = (unsigned int)mp3buf + mp3buf_read;
        CHCR3 &= ~0x0002;
    }
    else
    {
        debugf("No more MP3 data. Stopping.\n");
        CHCR3 = 0;
    }
}

static void mas_poll_start(unsigned int interval_in_ms)
{
    unsigned int count;

    count = FREQ / 1000 / 8 * interval_in_ms;

    if(count > 0xffff)
    {
        panicf("Error! The MAS poll interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }
    
    /* We are using timer 1 */
    
    TSTR &= ~0x02; /* Stop the timer */
    TSNC &= ~0x02; /* No synchronization */
    TMDR &= ~0x02; /* Operate normally */

    TCNT1 = 0;   /* Start counting at 0 */
    GRA1 = count;
    TCR1 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 2 */
    IPRC = (IPRC & ~0x000f) | 0x0002;
    
    TSR1 &= ~0x02;
    TIER1 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x02; /* Start timer 2 */
}

#pragma interrupt
void IMIA1(void)
{
    dma_tick();
    TSR1 &= ~0x01;
}

