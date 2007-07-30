/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include "hwcompat.h"
#include "kernel.h"

static struct mutex adc_mutex NOCACHEBSS_ATTR;

unsigned short adc_scan(int channel)
{
    int i, j;
    unsigned short data = 0;
    unsigned pval;

    (void)channel; /* there is only one */
    spinlock_lock(&adc_mutex);

    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        pval = GPIOB_OUTPUT_VAL;
        GPIOB_OUTPUT_VAL = pval | 0x04;  /* B2 -> high */
        for (i = 32; i > 0; --i);

        GPIOB_OUTPUT_VAL = pval;         /* B2 -> low */
        for (i = 200; i > 0; --i);

        for (j = 0; j < 8; j++)
        {
            GPIOB_OUTPUT_VAL = pval | 0x02; /* B1 -> high */
            for (i = 8; i > 0; --i);

            data = (data << 1) | ((GPIOB_INPUT_VAL & 0x08) >> 3);

            GPIOB_OUTPUT_VAL = pval;     /* B1 -> low */
            for (i = 320; i > 0; --i);
        }
    }
    else if ((IPOD_HW_REVISION >> 16) == 2)
    {
        pval = GPIOB_OUTPUT_VAL;
        GPIOB_OUTPUT_VAL = pval | 0x0a;  /* B1, B3 -> high */
        while (!(GPIOB_INPUT_VAL & 0x04)); /* wait for B2 == 1 */

        GPIOB_OUTPUT_VAL = pval;         /* B1, B3 -> low */
        while (GPIOB_INPUT_VAL & 0x04);  /* wait for B2 == 0 */

        for (j = 0; j < 8; j++)
        {
            GPIOB_OUTPUT_VAL = pval | 0x02; /* B1 -> high */
            while (!(GPIOB_INPUT_VAL & 0x04)); /* wait for B2 == 1 */

            data = (data << 1) | ((GPIOB_INPUT_VAL & 0x10) >> 4);

            GPIOB_OUTPUT_VAL = pval;     /* B1 -> low */
            while (GPIOB_INPUT_VAL & 0x04); /* wait for B2 == 0 */
        }
    }
    spinlock_unlock(&adc_mutex);
    return data;
}

void adc_init(void)
{
    spinlock_init(&adc_mutex);
    
    GPIOB_ENABLE |= 0x1e;  /* enable B1..B4 */

    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        GPIOB_OUTPUT_EN  = (GPIOB_OUTPUT_EN & ~0x08) | 0x16;
                                         /* B1, B2, B4 -> output, B3 -> input */
        GPIOB_OUTPUT_VAL = (GPIOB_OUTPUT_VAL & ~0x06) | 0x10;
                                         /* B1, B2 -> low, B4 -> high */
    }
    else if ((IPOD_HW_REVISION >> 16) == 2)
    {
        GPIOB_OUTPUT_EN   = (GPIOB_OUTPUT_EN & ~0x14) | 0x0a;
                                         /* B1, B3 -> output, B2, B4 -> input */
        GPIOB_OUTPUT_VAL &= ~0x0a;       /* B1, B3 -> low */
        while (GPIOB_INPUT_VAL & 0x04);  /* wait for B2 == 0 */
    }
}
