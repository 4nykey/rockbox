/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PP5002 I2C driver
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in January 2006
 *
 * Original file: linux/arch/armnommu/mach-ipod/hardware.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "cpu.h"
#include "kernel.h"
#include "logf.h"
#include "system.h"
#include "i2c-pp5002.h"

/* Local functions definitions */

#define IPOD_I2C_BASE   0xc0008000
#define IPOD_I2C_CTRL   (IPOD_I2C_BASE+0x00)
#define IPOD_I2C_ADDR   (IPOD_I2C_BASE+0x04)
#define IPOD_I2C_DATA0  (IPOD_I2C_BASE+0x0c)
#define IPOD_I2C_DATA1  (IPOD_I2C_BASE+0x10)
#define IPOD_I2C_DATA2  (IPOD_I2C_BASE+0x14)
#define IPOD_I2C_DATA3  (IPOD_I2C_BASE+0x18)
#define IPOD_I2C_STATUS (IPOD_I2C_BASE+0x1c)

/* IPOD_I2C_CTRL bit definitions */
#define IPOD_I2C_SEND   0x80

/* IPOD_I2C_STATUS bit definitions */
#define IPOD_I2C_BUSY   (1<<6)

#define POLL_TIMEOUT (HZ)

static int pp_i2c_wait_not_busy(void)
{
    unsigned long timeout;
    timeout = current_tick + POLL_TIMEOUT;
    while (TIME_BEFORE(current_tick, timeout)) {
         if (!(inb(IPOD_I2C_STATUS) & IPOD_I2C_BUSY)) {
            return 0;
         }
         yield();
    }

    return -1;
}


/* Public functions */

int pp_i2c_read_byte(unsigned int addr, unsigned int *data)
{
    if (pp_i2c_wait_not_busy() < 0)
    {
        return -1;
    }

    /* clear top 15 bits, left shift 1, or in 0x1 for a read */
    outb(((addr << 17) >> 16) | 0x1, IPOD_I2C_ADDR);

    outb(inb(IPOD_I2C_CTRL) | 0x20, IPOD_I2C_CTRL);

    outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

    if (pp_i2c_wait_not_busy() < 0)
    {
        return -1;
    }

    if (data)
    {
        *data = inb(IPOD_I2C_DATA0);
    }

    return 0;
}

int pp_i2c_send_bytes(unsigned int addr, unsigned int len, unsigned char *data)
{
    int data_addr;
    unsigned int i;

    if (len < 1 || len > 4)
    {
        return -1;
    }

    if (pp_i2c_wait_not_busy() < 0)
    {
        return -2;
    }

    /* clear top 15 bits, left shift 1 */
    outb((addr << 17) >> 16, IPOD_I2C_ADDR);

    outb(inb(IPOD_I2C_CTRL) & ~0x20, IPOD_I2C_CTRL);

    data_addr = IPOD_I2C_DATA0;
    for ( i = 0; i < len; i++ )
    {
        outb(*data++, data_addr);
        data_addr += 4;
    }

    outb((inb(IPOD_I2C_CTRL) & ~0x26) | ((len-1) << 1), IPOD_I2C_CTRL);

    outb(inb(IPOD_I2C_CTRL) | IPOD_I2C_SEND, IPOD_I2C_CTRL);

    return 0x0;
}

int pp_i2c_send_byte(unsigned int addr, int data0)
{
    unsigned char data[1];

    data[0] = data0;

    return pp_i2c_send_bytes(addr, 1, data);
}

int i2c_readbytes(unsigned int dev_addr, int addr, int len, unsigned char *data) {
    unsigned int temp;
    int i;
    pp_i2c_send_byte(dev_addr, addr);
    for (i = 0; i < len; i++) {
        pp_i2c_read_byte(dev_addr, &temp);
        data[i] = temp;
    }
    return i;
}

int i2c_readbyte(unsigned int dev_addr, int addr)
{
    int data;

    pp_i2c_send_byte(dev_addr, addr);
    pp_i2c_read_byte(dev_addr, &data);

    return data;
}

int pp_i2c_send(unsigned int addr, int data0, int data1)
{
    unsigned char data[2];

    data[0] = data0;
    data[1] = data1;

    return pp_i2c_send_bytes(addr, 2, data);
}

void i2c_init(void)
{
   /* From ipodlinux */

   outl(inl(0xcf005000) | 0x2, 0xcf005000);
	
   outl(inl(0xcf005030) | (1<<8), 0xcf005030);
   outl(inl(0xcf005030) & ~(1<<8), 0xcf005030);
}
