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
#include "lcd.h"
#include "sh7034.h"
#include "kernel.h"
#include "debug.h"

#define PB13 0x2000
#define PB7  0x0080
#define PB5  0x0020

/* cute little functions */
#define SDA_LO  (PBDR &= ~PB7)
#define SDA_HI  (PBDR |= PB7)
#define SDA_INPUT (PBIOR &= ~PB7)
#define SDA_OUTPUT (PBIOR |= PB7)
#define SDA     (PBDR & PB7)

#define SCL_INPUT (PBIOR &= ~PB13)
#define SCL_OUTPUT (PBIOR |= PB13)
#define SCL_LO  (PBDR &= ~PB13)
#define SCL_HI  (PBDR |= PB13)
#define SCL     (PBDR & PB13)

/* arbitrary delay loop */
#define DELAY   do { int _x; for(_x=0;_x<20;_x++);} while (0)

void i2c_start(void)
{
    SDA_OUTPUT;
    SDA_HI;
    SCL_HI;
    SDA_LO;
    DELAY;
    SCL_LO;
}

void i2c_stop(void)
{
   SDA_LO;
   SCL_HI;
   DELAY;
   SDA_HI;
}

void i2c_init(void)
{
   int i;

   /* make PB5, PB7 & PB13 general I/O */
   PBCR1 &= ~0x0c00; /* PB13 */
   PBCR2 &= ~0xcc00; /* PB5 abd PB7 */

   /* PB5 is "MAS enable". make it output and high */
   PBIOR |= PB5;
   PBDR |= PB5;

   /* Set the clock line to an output */
   PBIOR |= PB13;
   
   SDA_OUTPUT;
   SDA_HI;
   SCL_LO;
   for (i=0;i<3;i++)
      i2c_stop();
}

void i2c_ack(int bit)
{
    /* Here's the deal. The MAS is slow, and sometimes needs to wait
       before it can receive the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we release the ack. */
    
    SCL_LO;      /* Set the clock low */
    if ( bit )
        SDA_HI;
    else
        SDA_LO;
    
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the MAS to release it */
        yield();

    DELAY;
    SCL_OUTPUT;
    SCL_LO;
}

int i2c_getack(void)
{
    unsigned short x;

    /* Here's the deal. The MAS is slow, and sometimes needs to wait
       before it can send the acknowledge. Therefore it forces the clock
       low until it is ready. We need to poll the clock line until it goes
       high before we read the ack. */
    
    SDA_LO;      /* First, discharge the data line */
    SDA_INPUT;   /* And set to input */
    SCL_LO;      /* Set the clock low */
    SCL_INPUT;   /* Set the clock to input */
    while(!SCL)  /* and wait for the MAS to release it */
        yield();
    
    x = SDA;
    if (x)
        /* ack failed */
        return 0;
    SCL_OUTPUT;
    SCL_LO;
    SDA_HI;
    SDA_OUTPUT;
    return 1;
}

void i2c_outb(unsigned char byte)
{
   int i;

   /* clock out each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
      if ( i & byte )
         SDA_HI;
      else
         SDA_LO;
      SCL_HI;
      SCL_LO;
   }

   SDA_HI;
}

unsigned char i2c_inb(int ack)
{
   int i;
   unsigned char byte = 0;

   /* clock in each bit, MSB first */
   for ( i=0x80; i; i>>=1 ) {
       /* Tricky business. Here we discharge the data line by driving it low
          and then set it to input to see if it stays low or goes high */
       SDA_LO;      /* First, discharge the data line */
       SDA_INPUT;   /* And set to input */
       SCL_HI;
       if ( SDA )
           byte |= i;
       SCL_LO;
       SDA_OUTPUT;
   }
   
   i2c_ack(ack);
   
   return byte;
}

int i2c_write(int address, unsigned char* buf, int count )
{
    int i,x=0;

    i2c_start();
    i2c_outb(address & 0xfe);
    if (i2c_getack())
    {
        for (i=0; i<count; i++)
        {
            i2c_outb(buf[i]);
            if (!i2c_getack())
            {
                x=-2;
                break;
            }
        }
    }
    else
    {
        debugf("i2c_write() - no ack\n");
        x=-1;
    }
    i2c_stop();
    return x;
}

int i2c_read(int address, unsigned char* buf, int count )
{
    int i,x=0;
    
    i2c_start();
    i2c_outb(address | 1);
    if (i2c_getack()) {
        for (i=0; i<count; i++) {
            buf[i] = i2c_inb(0);
        }
    }
    else
        x=-1;
    i2c_stop();
    return x;
}
