/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

/* Use these to set the buttonlight mode */
enum buttonlight_mode
{
    /* ON follows the setting */
    BUTTONLIGHT_ON,

    /* buttonlights always off */
    BUTTONLIGHT_OFF,

    /* buttonlights follow backlight */
    BUTTONLIGHT_FOLLOW
};

/* Call this to flicker or signal the button lights. Only is effective for
 * modes that take a trigger input.
 */
void __buttonlight_trigger(void);

/* map the mode from the command into the state machine entries */
/* See enum buttonlight_mode for available functions */
void __buttonlight_mode(enum buttonlight_mode mode);

bool __backlight_init(void);
void __backlight_on(void);
void __backlight_off(void);
void __backlight_set_brightness(int brightness);

void __buttonlight_set_brightness(int brightness);

void __button_backlight_on(void);
void __button_backlight_off(void);

/* true: backlight fades off - false: backlight fades on */
void __backlight_dim(bool dim);
void __button_backlight_dim(bool dim_now);

#endif
