/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Justin Heiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ICONS_H_
#define _ICONS_H_

#include <lcd.h>

/* 
 * Icons of size 5x7 pixels for the Player LCD
 */

#ifdef HAVE_LCD_CHARCELLS

enum {
    Unknown = 0x90,
    Plugin = 0x17,
    Folder, Mod_Ajz, Language, File, Wps, Playlist, Text, Config,
};

#endif

#endif /* _ICONS_H_ */
