/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "backlight.h"
#include "backlight-target.h"

void power_init(void)
{
    /* Initialize IDE power pin */
    /* set GIO17 (ATA power) on and output */
    ide_power_enable(true);
    IO_GIO_DIR1&=~(1<<1);
    /* Charger detect */
}

unsigned int power_input_status(void)
{
	if(IO_GIO_BITSET1&(1<<9))
	{
		return POWER_INPUT_MAIN_CHARGER;
	}
    return POWER_INPUT_NONE;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) 
{
    return false;
}

void ide_power_enable(bool on)
{
    if (on)
        IO_GIO_BITCLR1=(1<<1);
    else
        IO_GIO_BITSET1=(1<<1);
}

bool ide_powered(void)
{
    return !(IO_GIO_BITSET1&(1<<1));
}

void power_off(void)
{
    /* turn off backlight and wait for 1 second */
    _backlight_off();
    sleep(HZ);
    /* Hard shutdown */
    IO_GIO_DIR1&=~(1<<10);
    IO_GIO_BITSET1|=1<<10;
}
