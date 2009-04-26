/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-e200v2-fuze.c 19224 2008-11-26 10:21:03Z pondlife $
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "ascodec-target.h"
#include "as3514.h"

/* TODO: This file is copy & pasted from backlight-e200v2-fuze.c, as I think
 * it'll be the same for c200v2; prove it */
void _backlight_set_brightness(int brightness)
{
    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    ascodec_write(AS3514_DCDC15, backlight_brightness);
}

void _backlight_off(void)
{
    ascodec_write(AS3514_DCDC15, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

void _buttonlight_on(void)
{
    GPIOD_DIR |= (1<<7);
    GPIOD_PIN(7) = (1<<7);
}

void _buttonlight_off(void)
{
    GPIOD_DIR |= (1<<7);
    GPIOD_PIN(7) = 0;
}
