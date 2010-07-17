/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "config.h"
#include <stdbool.h>
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"

void usb_init_device(void)
{
    /* GPIO42 is USB detect input
     * but it also serves as MCLK2 for DAC
     */
    and_l(~(1<<4), &GPIO1_OUT);           /* GPIO36 low */
    or_l((1<<4), &GPIO1_ENABLE);          /* GPIO36 */
    or_l((1<<4)|(1<<5), &GPIO1_FUNCTION); /* GPIO36 GPIO37 */

     /* GPIO22 GPIO30 high */
    or_l((1<<22)|(1<<30), &GPIO_OUT);
    or_l((1<<22)|(1<<30), &GPIO_ENABLE);
    or_l((1<<22)|(1<<30), &GPIO_FUNCTION);
}

int usb_detect(void)
{
    /* GPIO42 active low*/
    return (GPIO1_READ & (1<<10)) ? USB_EXTRACTED : USB_INSERTED;
}

void usb_enable(bool on)
{
    /* one second timeout */
    unsigned char timeout = 10;
   
    if(on)
    {
        and_l(~(1<<30),&GPIO_OUT);  /* GPIO30 low */
        and_l(~(1<<22),&GPIO_OUT);  /* GPIO22 low */

        or_l((1<<4),&GPIO1_OUT);    /* GPIO36 high */

    }
    else
    {
        or_l((1<<22),&GPIO_OUT);  /* GPIO22 high */
        or_l((1<<30),&GPIO_OUT);  /* GPIO30 high */

        and_l(~(1<<4),&GPIO1_OUT); /* GPIO36 low */

        while ( !(GPIO1_READ & (1<<5)) && timeout--)
        {
            sleep(HZ/10);
        }
        sleep(HZ);
    }
}
