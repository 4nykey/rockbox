/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include "config.h"

bool is_backlight_on(void);
void backlight_on(void);
void backlight_off(void);
void backlight_set_timeout(int index);

#ifdef HAVE_BACKLIGHT
void backlight_init(void);

int  backlight_get_current_timeout(void);

#ifdef HAVE_BACKLIGHT_PWM_FADING
void backlight_set_fade_in(int index);
void backlight_set_fade_out(int index);
#endif

void backlight_set_timeout_plugged(int index);
extern const signed char backlight_timeout_value[];

#ifdef HAS_BUTTON_HOLD
void backlight_hold_changed(bool hold_button);
void backlight_set_on_button_hold(int index);
#endif

#ifdef HAVE_LCD_SLEEP
void lcd_set_sleep_after_backlight_off(int index);
extern const signed char lcd_sleep_timeout_value[];
#endif

#else /* !HAVE_BACKLIGHT */
#define backlight_init()
#endif /* !HAVE_BACKLIGHT */

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void);
void remote_backlight_off(void);
void remote_backlight_set_timeout(int index);
void remote_backlight_set_timeout_plugged(int index);
bool is_remote_backlight_on(void);

#ifdef HAS_REMOTE_BUTTON_HOLD
void remote_backlight_hold_changed(bool rc_hold_button);
void remote_backlight_set_on_button_hold(int index);
#endif
#endif /* HAVE_REMOTE_LCD */

#ifdef SIMULATOR
void sim_backlight(int value);
void sim_remote_backlight(int value);
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_set_brightness(int val);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val);
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTON_LIGHT
void button_backlight_on(void);
void button_backlight_off(void);
void button_backlight_set_timeout(int index);
#endif

#endif /* BACKLIGHT_H */
