/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __UISDL_H__
#define __UISDL_H__

#include <stdbool.h>
#include "SDL.h"

/* colour definitions are R, G, B */

#if defined(ARCHOS_RECORDER)
#define UI_TITLE                    "Jukebox Recorder"
#define UI_WIDTH                    270 /* width of GUI window */
#define UI_HEIGHT                   406 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 80 /* x position of lcd */
#define UI_LCD_POSY                 104 /* y position of lcd (96 for real aspect) */
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 /* (80 for real aspect) */

#elif defined(ARCHOS_PLAYER)
#define UI_TITLE                    "Jukebox Player"
#define UI_WIDTH                    284 /* width of GUI window */
#define UI_HEIGHT                   420 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 75 /* x position of lcd */
#define UI_LCD_POSY                 116 /* y position of lcd */
#define UI_LCD_WIDTH                132
#define UI_LCD_HEIGHT               64

#elif defined(ARCHOS_FMRECORDER) || defined(ARCHOS_RECORDERV2)
#define UI_TITLE                    "Jukebox FM Recorder"
#define UI_WIDTH                    285 /* width of GUI window */
#define UI_HEIGHT                   414 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         126, 229, 126 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 87 /* x position of lcd */
#define UI_LCD_POSY                 77 /* y position of lcd (69 for real aspect) */
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 /* (80 for real aspect) */

#elif defined(ARCHOS_ONDIOSP) || defined(ARCHOS_ONDIOFM)
#define UI_TITLE                    "Ondio"
#define UI_WIDTH                    155 /* width of GUI window */
#define UI_HEIGHT                   334 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         130, 180, 250 /* bkgnd color of LCD (backlight mod) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 21 /* x position of lcd */
#define UI_LCD_POSY                 82 /* y position of lcd (74 for real aspect) */
#define UI_LCD_WIDTH                112
#define UI_LCD_HEIGHT               64 /* (80 for real aspect) */

#elif defined(IRIVER_H120) || defined(IRIVER_H100)
#define UI_TITLE                    "iriver H1x0"
#define UI_WIDTH                    379 /* width of GUI window */
#define UI_HEIGHT                   508 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         173, 216, 230 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 109 /* x position of lcd */
#define UI_LCD_POSY                 23 /* y position of lcd */
#define UI_LCD_WIDTH                160
#define UI_LCD_HEIGHT               128
#define UI_REMOTE_BGCOLOR           90, 145, 90 /* bkgnd of remote lcd (no bklight) */
#define UI_REMOTE_BGCOLORLIGHT      130, 180, 250 /* bkgnd of remote lcd (bklight) */
#define UI_REMOTE_POSX              50  /* x position of remote lcd */
#define UI_REMOTE_POSY              403 /* y position of remote lcd */
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            64

#elif defined(IRIVER_H300)
#define UI_TITLE                    "iriver H300"
#define UI_WIDTH                    288 /* width of GUI window */
#define UI_HEIGHT                   581 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 26 /* x position of lcd */
#define UI_LCD_POSY                 36 /* y position of lcd */
#define UI_LCD_WIDTH                220
#define UI_LCD_HEIGHT               176
#define UI_REMOTE_BGCOLOR           90, 145, 90 /* bkgnd of remote lcd (no bklight) */
#define UI_REMOTE_BGCOLORLIGHT      130, 180, 250 /* bkgnd of remote lcd (bklight) */
#define UI_REMOTE_POSX              12  /* x position of remote lcd */
#define UI_REMOTE_POSY              478 /* y position of remote lcd */
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            64

#elif defined(IPOD_3G)
#define UI_TITLE                    "iPod 3G"
#define UI_WIDTH                    218 /* width of GUI window */
#define UI_HEIGHT                   389 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         173, 216, 230 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 29 /* x position of lcd */
#define UI_LCD_POSY                 16 /* y position of lcd */
#define UI_LCD_WIDTH                160
#define UI_LCD_HEIGHT               128

#elif defined(IPOD_4G)
#define UI_TITLE                    "iPod 4G"
#define UI_WIDTH                    196 /* width of GUI window */
#define UI_HEIGHT                   370 /* height of GUI window */
#define UI_LCD_BGCOLOR              90, 145, 90 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         173, 216, 230 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 19 /* x position of lcd */
#define UI_LCD_POSY                 14 /* y position of lcd */
#define UI_LCD_WIDTH                160
#define UI_LCD_HEIGHT               128

#elif defined(IPOD_MINI) || defined(IPOD_MINI2G)
#define UI_TITLE                    "iPod mini"
#define UI_WIDTH                    191 /* width of GUI window */
#define UI_HEIGHT                   365 /* height of GUI window */
#define UI_LCD_BGCOLOR              100, 135, 100 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         223, 216, 255 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 24 /* x position of lcd */
#define UI_LCD_POSY                 17 /* y position of lcd */
#define UI_LCD_WIDTH                138
#define UI_LCD_HEIGHT               110

#elif defined(IPOD_COLOR)
#define UI_TITLE                    "iPod Color"
#define UI_WIDTH                    261 /* width of GUI window */
#define UI_HEIGHT                   493 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 21 /* x position of lcd */
#define UI_LCD_POSY                 16 /* y position of lcd */
#define UI_LCD_WIDTH                220
#define UI_LCD_HEIGHT               176

#elif defined(IPOD_NANO)
#define UI_TITLE                    "iPod Nano"
#define UI_WIDTH                    199 /* width of GUI window */
#define UI_HEIGHT                   421 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 13 /* x position of lcd */
#define UI_LCD_POSY                 14 /* y position of lcd */
#define UI_LCD_WIDTH                176
#define UI_LCD_HEIGHT               132

#elif defined(IPOD_VIDEO)
#define UI_TITLE                    "iPod Video"
#define UI_WIDTH                    350 /* width of GUI window */
#define UI_HEIGHT                   591 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 14 /* x position of lcd */
#define UI_LCD_POSY                 12 /* y position of lcd */
#define UI_LCD_WIDTH                320
#define UI_LCD_HEIGHT               240

#elif defined(IAUDIO_X5)
#define UI_TITLE                    "iAudio X5"
#define UI_WIDTH                    300 /* width of GUI window */
#define UI_HEIGHT                   558 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 55 /* x position of lcd */
#define UI_LCD_POSY                 61 /* y position of lcd (74 for real aspect) */
#define UI_LCD_WIDTH                LCD_WIDTH /* * 1.5 */
#define UI_LCD_HEIGHT               LCD_HEIGHT  /* * 1.5 */
#define UI_REMOTE_BGCOLOR           90, 145, 90 /* bkgnd of remote lcd (no bklight) */
#define UI_REMOTE_BGCOLORLIGHT      130, 180, 250 /* bkgnd of remote lcd (bklight) */
#define UI_REMOTE_POSX              12  /* x position of remote lcd */
#define UI_REMOTE_POSY              462 /* y position of remote lcd */
#define UI_REMOTE_WIDTH             128
#define UI_REMOTE_HEIGHT            96

#elif defined(GIGABEAT_F)
#define UI_TITLE                    "Toshiba Gigabeat"
#define UI_WIDTH                    401 /* width of GUI window */
#define UI_HEIGHT                   655 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 48 /* x position of lcd */
#define UI_LCD_POSY                 60 /* y position of lcd */
#define UI_LCD_WIDTH                240
#define UI_LCD_HEIGHT               320

#elif defined(IRIVER_H10)
#define UI_TITLE                    "iriver H10 20Gb"
#define UI_WIDTH                    392 /* width of GUI window */
#define UI_HEIGHT                   391 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 111 /* x position of lcd */
#define UI_LCD_POSY                 30 /* y position of lcd (74 for real aspect) */
#define UI_LCD_WIDTH                LCD_WIDTH /* * 1.5 */
#define UI_LCD_HEIGHT               LCD_HEIGHT  /* * 1.5 */

#elif defined(IRIVER_H10_5GB)
#define UI_TITLE                    "iriver H10 5/6Gb"
#define UI_WIDTH                    353 /* width of GUI window */
#define UI_HEIGHT                   460 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 112 /* x position of lcd */
#define UI_LCD_POSY                 45  /* y position of lcd (74 for real aspect) */
#define UI_LCD_WIDTH                LCD_WIDTH /* * 1.5 */
#define UI_LCD_HEIGHT               LCD_HEIGHT  /* * 1.5 */

#elif defined(SANSA_E200)
#define UI_TITLE                    "Sansa e200"
#define UI_WIDTH                    260 /* width of GUI window */
#define UI_HEIGHT                   502 /* height of GUI window */
/* high-colour */
#define UI_LCD_POSX                 42 /* x position of lcd */
#define UI_LCD_POSY                 37  /* y position of lcd (74 for real aspect) */
#define UI_LCD_WIDTH                LCD_WIDTH /* * 1.5 */
#define UI_LCD_HEIGHT               LCD_HEIGHT  /* * 1.5 */

#elif defined(IRIVER_IFP7XX)
#define UI_TITLE                    "iriver iFP7xx"
#define UI_WIDTH                    425 /* width of GUI window */
#define UI_HEIGHT                   183 /* height of GUI window */
#define UI_LCD_BGCOLOR              94, 104, 84 /* bkgnd color of LCD (no backlight) */
#define UI_LCD_BGCOLORLIGHT         60, 160, 230 /* bkgnd color of LCD (backlight) */
#define UI_LCD_BLACK                0, 0, 0 /* black */
#define UI_LCD_POSX                 115 /* x position of lcd */
#define UI_LCD_POSY                 54 /* y position of lcd */
#define UI_LCD_WIDTH                LCD_WIDTH /* * 1.5 */
#define UI_LCD_HEIGHT               LCD_HEIGHT  /* * 1.5 */
    
#endif
extern SDL_Surface *gui_surface;
extern bool background;  /* True if the background image is enabled */
extern int display_zoom; 

#endif /* #ifndef __UISDL_H__ */

