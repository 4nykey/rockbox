/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <lcd.h>

/* 
 * Icons of size 6x8 pixels 
 */

#ifdef HAVE_LCD_BITMAP

enum icons_6x8 {
    Box_Filled, Box_Empty, Slider_Horizontal, File, 
    Folder,     Directory, Playlist,          Repeat,
    Selected,   Cursor,    Wps,               Mod_Ajz, 
    LastIcon
};

/* Symbolic names for icons */
enum icons_5x8 {
    Icon_Lock
};

enum icons_7x8 {
    Icon_Plug,
    Icon_Speaker,
    Icon_Mute,
    Icon_Play,
    Icon_Stop,
    Icon_Pause,
    Icon_FastForward,
    Icon_FastBackward,
    Icon_Record,
    Icon_RecPause,
    Icon_Normal,
    Icon_Repeat,
    Icon_Shuffle,
    Icon_Last
};

extern unsigned char bitmap_icons_5x8[1][5];
extern unsigned char bitmap_icons_6x8[LastIcon][6];
extern unsigned char bitmap_icons_7x8[Icon_Last][7];

extern unsigned char rockbox112x37[];

extern unsigned char slider_bar[];

#define STATUSBAR_X_POS       0
#define STATUSBAR_Y_POS       0 // MUST be a multiple of 8
#define STATUSBAR_HEIGHT      8
#define STATUSBAR_WIDTH       LCD_WIDTH
#define ICON_BATTERY_X_POS    0
#define ICON_BATTERY_WIDTH    18
#define ICON_PLUG_X_POS       STATUSBAR_X_POS+ICON_BATTERY_WIDTH+2
#define ICON_PLUG_WIDTH       7
#define ICON_VOLUME_X_POS     STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+2+2
#define ICON_VOLUME_WIDTH     16
#define ICON_PLAY_STATE_X_POS STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+2+2+2
#define ICON_PLAY_STATE_WIDTH 7
#define ICON_PLAY_MODE_X_POS  STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+2+2+2+2
#define ICON_PLAY_MODE_WIDTH  7
#define ICON_SHUFFLE_X_POS    STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+ICON_PLAY_MODE_WIDTH+2+2+2+2+2
#define ICON_SHUFFLE_WIDTH    7
#define LOCK_X_POS            STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+ICON_PLAY_MODE_WIDTH+ICON_SHUFFLE_WIDTH+2+2+2+2+2+2
#define LOCK_WIDTH            5
#define TIME_X_END            STATUSBAR_WIDTH-1

extern void statusbar_wipe(void);
extern void statusbar_icon_battery(int percent, bool charging);
extern void statusbar_icon_volume(int percent);
extern void statusbar_icon_play_state(int state);
extern void statusbar_icon_play_mode(int mode);
extern void statusbar_icon_shuffle(void);
extern void statusbar_icon_lock(void);
#ifdef HAVE_RTC
extern void statusbar_time(int hour, int minute);
#endif
#endif /* End HAVE_LCD_BITMAP */
