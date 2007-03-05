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
#ifndef LCD_REMOTE_TARGET_H
#define LCD_REMOTE_TARGET_H

#define REMOTE_INIT_LCD   1
#define REMOTE_DEINIT_LCD 2

void lcd_remote_init_device(void);
void lcd_remote_write_command(int cmd);
void lcd_remote_write_command_ex(int cmd, int data);
void lcd_remote_write_data(const unsigned char* p_bytes, int count);
bool remote_detect(void);
void lcd_remote_powersave(bool on);
void lcd_remote_set_contrast(int val);
void lcd_remote_on(void);
void lcd_remote_off(void);
void lcd_remote_poweroff(void); /* for when remote is plugged during shutdown*/

extern bool remote_initialized;

#endif
