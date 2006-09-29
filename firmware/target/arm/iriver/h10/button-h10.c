/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Custom written for the H10 based on analysis of the GPIO data */


#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"


void button_init_device(void)
{
    /* Enable REW, FF, Play, Left, Right, Hold buttons */
    GPIOA_ENABLE |= 0xfc;
    
    /* Enable POWER button */
    GPIOB_ENABLE |= 0x1;
    
    /* We need to output to pin 6 of GPIOD when reading the scroll pad value */
    GPIOD_ENABLE |= 0x40;
    GPIOD_OUTPUT_EN |= 0x40;
    GPIOD_OUTPUT_VAL |= 0x40;
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x4)?false:true;
}

bool remote_button_hold(void)
{
    return adc_scan(ADC_REMOTE) < 0x17;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;
    unsigned char state;
    static bool hold_button = false;
    static bool remote_hold_button = false;
    bool hold_button_old;
    bool remote_hold_button_old;

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);
    }
#endif

    /* device buttons */
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOA_INPUT_VAL & 0xf8;
        if ((state & 0x8) == 0) btn |= BUTTON_FF;
        if ((state & 0x10) == 0) btn |= BUTTON_PLAY;
        if ((state & 0x20) == 0) btn |= BUTTON_REW;
        if ((state & 0x40) == 0) btn |= BUTTON_RIGHT;
        if ((state & 0x80) == 0) btn |= BUTTON_LEFT;
        
        /* Read power button */
        if (GPIOB_INPUT_VAL & 0x1) btn |= BUTTON_POWER;
        
        /* Read scroller */
        if ( GPIOD_INPUT_VAL & 0x20 )
        {
            GPIOD_OUTPUT_VAL &=~ 0x40;
            udelay(50);
            data = adc_scan(ADC_SCROLLPAD);
            GPIOD_OUTPUT_VAL |= 0x40;
            
            if(data < 0x210)
            {
                btn |= BUTTON_SCROLL_DOWN;
            } else {
                btn |= BUTTON_SCROLL_UP;
            }
        }
    }
    
    /* remote buttons */
    remote_hold_button_old = remote_hold_button;

    data = adc_scan(ADC_REMOTE);
    remote_hold_button = data < 0x17;

#ifndef BOOTLOADER
    if (remote_hold_button != remote_hold_button_old)
        backlight_hold_changed(remote_hold_button);
#endif

    if(!remote_hold_button)
    {
        if (data < 0x3FF)
        {
            if(data < 0x1F0)
                if(data < 0x141)
                    btn |= BUTTON_RC_FF;
                else
                    btn |= BUTTON_RC_REW;
            else
                if(data < 0x2BC)
                   btn |= BUTTON_RC_VOL_DOWN;
                else
                    btn |= BUTTON_RC_VOL_UP;
        }
    }

    /* remote play button should be dead if hold */
    if (!remote_hold_button && !(GPIOA_INPUT_VAL & 0x1))
        btn |= BUTTON_RC_PLAY;
    
    return btn;
}
