/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
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

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
void button_use_hold_buttons(bool yesno);

/* Main unit's buttons */
#define BUTTON_POWER      0x00000001
#define BUTTON_PLUS       0x00000002
#define BUTTON_MINUS      0x00000004
#define BUTTON_MENU       0x00000008

/* Compatibility hacks for flipping. Needs a somewhat better fix. */
#define BUTTON_LEFT  BUTTON_MIDLEFT
#define BUTTON_RIGHT BUTTON_MIDRIGHT
#define BUTTON_UP    BUTTON_TOPMIDDLE
#define BUTTON_DOWN  BUTTON_BOTTOMMIDDLE

/* Touch Screen Area Buttons */
#define BUTTON_TOPLEFT      0x00000010
#define BUTTON_TOPMIDDLE    0x00000020
#define BUTTON_TOPRIGHT     0x00000040
#define BUTTON_MIDLEFT      0x00000080
#define BUTTON_CENTER       0x00000100
#define BUTTON_MIDRIGHT     0x00000200
#define BUTTON_BOTTOMLEFT   0x00000400
#define BUTTON_BOTTOMMIDDLE 0x00000800
#define BUTTON_BOTTOMRIGHT  0x00001000

/* Main unit's buttons whilst hold switch is on */
#ifdef HAVE_BUTTONS_IN_HOLD_MODE
#define BUTTON_HOLDPLUS   0x00002000
#define BUTTON_HOLDMINUS  0x00004000
#define BUTTON_HOLDMENU   0x00008000
#endif

#ifdef HAVE_BUTTONS_IN_HOLD_MODE
#define BUTTON_MAIN 0xFFFF
#else
#define BUTTON_MAIN 0x1FFF
#endif

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10
                
#endif /* _BUTTON_TARGET_H_ */
