/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "button-target.h"
#include "tuner.h"
#include "backlight-target.h"
#include "powermgmt.h"

void power_init(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);
    
    /* Set outputs as per OF - further investigation required. */
    pcf50606_write(PCF5060X_DCDEC1,  0xe4);
    pcf50606_write(PCF5060X_IOREGC,  0xf5);
    pcf50606_write(PCF5060X_D1REGC1, 0xf5);
    pcf50606_write(PCF5060X_D2REGC1, 0xe9);
    pcf50606_write(PCF5060X_D3REGC1, 0xf8); /* WM8985 3.3v */
    pcf50606_write(PCF5060X_DCUDC1,  0xe7);
    pcf50606_write(PCF5060X_LPREGC1, 0x0);
    pcf50606_write(PCF5060X_LPREGC2, 0x2);

#ifndef BOOTLOADER
    IEN |= EXT3_IRQ_MASK;   /* Unmask EXT3 */
#endif
}

void power_off(void)
{
    /* Turn the backlight off first to avoid a bright stripe on power-off */
    _backlight_off();
    sleep(HZ/10);
    
    /* Power off the player using the same mechanism as the OF */
    GPIOA_CLEAR = (1<<7);
    while(true);
}

#ifndef BOOTLOADER
void EXT3(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);

    if (data[0] & 0x04)
    {
        /* ONKEY1S */
        if (!charger_inserted())
            sys_poweroff();
        else
            pcf50606_reset_timeout();
    }

    if (data[2] & 0x08)
    {
        /* Touchscreen event, do something about it */
        button_read_touch();
    }
}
#endif

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    return ((GPIOC & (1<<26)) == 0) ?
        POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}
#endif

#if CONFIG_TUNER

/** Tuner **/
static bool powered = false;

bool tuner_power(bool status)
{
    bool old_status;
    lv24020lp_lock();

    old_status = powered;

    if (status != old_status)
    {
        if (status)
        {
            /* When power up, host should initialize the 3-wire bus
               in host read mode: */

            /* 1. Set direction of the DATA-line to input-mode. */
            GPIOC_DIR &= ~(1 << 30); 

            /* 2. Drive NR_W low */
            GPIOC_CLEAR = (1 << 31); 
            GPIOC_DIR |= (1 << 31); 

            /* 3. Drive CLOCK high */
            GPIOC_SET = (1 << 29); 
            GPIOC_DIR |= (1 << 29); 

            lv24020lp_power(true);
        }
        else
        {
            lv24020lp_power(false);

            /* set all as inputs */
            GPIOC_DIR &= ~((1 << 29) | (1 << 30) | (1 << 31));
        }

        powered = status;
    }

    lv24020lp_unlock();
    return old_status;
}

#endif /* CONFIG_TUNER */
