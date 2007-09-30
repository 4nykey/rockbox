/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis
 *
 * LCD scrolling driver and scheduler
 *
 * Much collected and combined from the various Rockbox LCD drivers.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __SCROLL_ENGINE_H__
#define __SCROLL_ENGINE_H__

void scroll_init(void);
void lcd_scroll_fn(void);
void lcd_remote_scroll_fn(void);

/* internal usage, but in multiple drivers */
#define SCROLL_SPACING   3
#ifdef HAVE_LCD_BITMAP
#define SCROLL_LINE_SIZE (MAX_PATH + SCROLL_SPACING + 3*LCD_WIDTH/2 + 2)
#else
#define SCROLL_LINE_SIZE (MAX_PATH + SCROLL_SPACING + 3*LCD_WIDTH + 2)
#endif

struct scrollinfo
{
    char line[SCROLL_LINE_SIZE];
    int len;    /* length of line in chars */
    int offset;
    int startx;
#ifdef HAVE_LCD_BITMAP
    int width;  /* length of line in pixels */
    int style; /* line style */
#endif/* HAVE_LCD_BITMAP */
    bool backward; /* scroll presently forward or backward? */
    bool bidir;
    long start_tick;
};

struct scroll_screen_info
{
    struct scrollinfo * const scroll;
    const int num_scroll; /* number of scrollable lines (also number of scroll structs) */
    int lines;  /* Bitpattern of which lines are scrolling */
    long ticks; /* # of ticks between updates*/
    long delay; /* ticks delay before start */
    int bidir_limit;  /* percent */
#ifdef HAVE_LCD_CHARCELLS
    long jump_scroll_delay; /* delay between jump scroll jumps */
    int jump_scroll; /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
#endif
#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD)
    int step;  /* pixels per scroll step */
#endif
#if defined(HAVE_REMOTE_LCD)
    long last_scroll;
#endif
};

/** main lcd **/
#ifdef HAVE_LCD_BITMAP
#define LCD_SCROLLABLE_LINES ((LCD_HEIGHT+4)/5 < 32 ? (LCD_HEIGHT+4)/5 : 32)
#else
#define LCD_SCROLLABLE_LINES LCD_HEIGHT
#endif

extern struct scroll_screen_info lcd_scroll_info;

/** remote lcd **/
#ifdef HAVE_REMOTE_LCD
#define LCD_REMOTE_SCROLLABLE_LINES \
    (((LCD_REMOTE_HEIGHT+4)/5 < 32) ? (LCD_REMOTE_HEIGHT+4)/5 : 32)
extern struct scroll_screen_info lcd_remote_scroll_info;
#endif

#endif /* __SCROLL_ENGINE_H__ */
