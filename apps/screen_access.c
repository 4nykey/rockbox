/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <lcd.h>
#include <lcd-remote.h>
#include "backlight.h"
#include <font.h>
#include <button.h>
#include <sprintf.h>
#include <settings.h>
#include <kernel.h>
#include <icons.h>

#include "screen_access.h"
#include "textarea.h"

struct screen screens[NB_SCREENS];

void screen_init(struct screen * screen, enum screen_type screen_type)
{
    switch(screen_type)
    {
#ifdef HAVE_REMOTE_LCD
        case SCREEN_REMOTE:
            screen->is_color=false;/* No color remotes yet */
            screen->pixel_format=LCD_REMOTE_PIXELFORMAT;
            screen->depth=LCD_REMOTE_DEPTH;
            screen->has_disk_led=false;

            screen->width=LCD_REMOTE_WIDTH;
            screen->height=LCD_REMOTE_HEIGHT;
            screen->setmargins=&lcd_remote_setmargins;
            screen->getymargin=&lcd_remote_getymargin;
            screen->getxmargin=&lcd_remote_getxmargin;
            screen->getstringsize=&lcd_remote_getstringsize;
#if 1 /* all remote LCDs are bitmapped so far */
            screen->setfont=&lcd_remote_setfont;
            screen->setfont(FONT_UI);
            screen->mono_bitmap=&lcd_remote_mono_bitmap;
            screen->mono_bitmap_part=&lcd_remote_mono_bitmap_part;
            screen->bitmap=(screen_bitmap_func*)&lcd_remote_bitmap;
            screen->bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part;
            screen->set_drawmode=&lcd_remote_set_drawmode;
#if LCD_REMOTE_DEPTH <= 2
            /* No transparency yet for grayscale and mono lcd */
            screen->transparent_bitmap=(screen_bitmap_func*)&lcd_remote_bitmap;
            screen->transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_remote_bitmap_part;
            /* No colour remotes yet */
#endif
#if LCD_REMOTE_DEPTH > 1
#if defined(HAVE_LCD_COLOR)
            screen->color_to_native=&lcd_remote_color_to_native;
#endif
            screen->get_background=&lcd_remote_get_background;
            screen->get_foreground=&lcd_remote_get_foreground;
            screen->set_background=&lcd_remote_set_background;
            screen->set_foreground=&lcd_remote_set_foreground;
#endif /* LCD_REMOTE_DEPTH > 1 */
            screen->update_rect=&lcd_remote_update_rect;
            screen->fillrect=&lcd_remote_fillrect;
            screen->drawrect=&lcd_remote_drawrect;
            screen->drawpixel=&lcd_remote_drawpixel;
            screen->drawline=&lcd_remote_drawline;
            screen->vline=&lcd_remote_vline;
            screen->hline=&lcd_remote_hline;
            screen->scroll_step=&lcd_remote_scroll_step;
            screen->invertscroll=&lcd_remote_invertscroll;
            screen->puts_style_offset=&lcd_remote_puts_style_offset;
            screen->puts_scroll_style=&lcd_remote_puts_scroll_style;
            screen->puts_scroll_style_offset=&lcd_remote_puts_scroll_style_offset;
#endif /* 1 */

#if 0 /* no charcell remote LCDs so far */
            screen->double_height=&lcd_remote_double_height;
            screen->putc=&lcd_remote_putc;
            screen->get_locked_pattern=&lcd_remote_get_locked_pattern;
            screen->define_pattern=&lcd_remote_define_pattern;
            screen->icon=&lcd_remote_icon;
#endif /* 0 */

            screen->init=&lcd_remote_init;
            screen->putsxy=&lcd_remote_putsxy;
            screen->puts=&lcd_remote_puts;
            screen->puts_offset=&lcd_remote_puts_offset;
            screen->puts_scroll=&lcd_remote_puts_scroll;
            screen->puts_scroll_offset=&lcd_remote_puts_scroll_offset;
            screen->scroll_speed=&lcd_remote_scroll_speed;
            screen->scroll_delay=&lcd_remote_scroll_delay;
            screen->stop_scroll=&lcd_remote_stop_scroll;
            screen->clear_display=&lcd_remote_clear_display;
            screen->update=&lcd_remote_update;
            screen->backlight_on=&remote_backlight_on;
            screen->backlight_off=&remote_backlight_off;
            screen->is_backlight_on=&is_remote_backlight_on;
            screen->backlight_set_timeout=&remote_backlight_set_timeout;
            break;
#endif /* HAVE_REMOTE_LCD */

        case SCREEN_MAIN:
        default:
#if defined(HAVE_LCD_COLOR)
            screen->is_color=true;
#else
            screen->is_color=false;
#endif
#ifdef HAVE_LCD_BITMAP
            screen->pixel_format=LCD_PIXELFORMAT;
#endif
            screen->depth=LCD_DEPTH;
#if (CONFIG_LED == LED_VIRTUAL)
            screen->has_disk_led=false;
#elif defined(HAVE_REMOTE_LCD)
            screen->has_disk_led=true;
#endif
            screen->width=LCD_WIDTH;
            screen->height=LCD_HEIGHT;
            screen->setmargins=&lcd_setmargins;
            screen->getymargin=&lcd_getymargin;
            screen->getxmargin=&lcd_getxmargin;
            screen->getstringsize=&lcd_getstringsize;
#ifdef HAVE_LCD_BITMAP
            screen->setfont=&lcd_setfont;
            screen->setfont(FONT_UI);
            screen->mono_bitmap=&lcd_mono_bitmap;
            screen->mono_bitmap_part=&lcd_mono_bitmap_part;
            screen->set_drawmode=&lcd_set_drawmode;
            screen->bitmap=(screen_bitmap_func*)&lcd_bitmap;
            screen->bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_part;
#if LCD_DEPTH <= 2
            /* No transparency yet for grayscale and mono lcd */
            screen->transparent_bitmap=(screen_bitmap_func*)&lcd_bitmap;
            screen->transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_part;
#else
            screen->transparent_bitmap=(screen_bitmap_func*)&lcd_bitmap_transparent;
            screen->transparent_bitmap_part=(screen_bitmap_part_func*)&lcd_bitmap_transparent_part;
#endif
#if LCD_DEPTH > 1
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
            screen->color_to_native=&lcd_color_to_native;
#endif
            screen->get_background=&lcd_get_background;
            screen->get_foreground=&lcd_get_foreground;
            screen->set_background=&lcd_set_background;
            screen->set_foreground=&lcd_set_foreground;
#endif /* LCD_DEPTH > 1 */
            screen->update_rect=&lcd_update_rect;
            screen->fillrect=&lcd_fillrect;
            screen->drawrect=&lcd_drawrect;
            screen->drawpixel=&lcd_drawpixel;
            screen->drawline=&lcd_drawline;
            screen->vline=&lcd_vline;
            screen->hline=&lcd_hline;
            screen->scroll_step=&lcd_scroll_step;
            screen->invertscroll=&lcd_invertscroll;
            screen->puts_style_offset=&lcd_puts_style_offset;
            screen->puts_scroll_style=&lcd_puts_scroll_style;
            screen->puts_scroll_style_offset=&lcd_puts_scroll_style_offset;
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
            screen->double_height=&lcd_double_height;
            screen->putc=&lcd_putc;
            screen->get_locked_pattern=&lcd_get_locked_pattern;
            screen->define_pattern=&lcd_define_pattern;
            screen->unlock_pattern=&lcd_unlock_pattern;
            screen->icon=&lcd_icon;
#endif /* HAVE_LCD_CHARCELLS */

            screen->init=&lcd_init;
            screen->putsxy=&lcd_putsxy;
            screen->puts=&lcd_puts;
            screen->puts_offset=&lcd_puts_offset;
            screen->puts_scroll=&lcd_puts_scroll;
            screen->puts_scroll_offset=&lcd_puts_scroll_offset;
            screen->scroll_speed=&lcd_scroll_speed;
            screen->scroll_delay=&lcd_scroll_delay;
            screen->stop_scroll=&lcd_stop_scroll;
            screen->clear_display=&lcd_clear_display;
            screen->update=&lcd_update;
            screen->backlight_on=&backlight_on;
            screen->backlight_off=&backlight_off;
            screen->is_backlight_on=&is_backlight_on;
            screen->backlight_set_timeout=&backlight_set_timeout;
            break;
    }
    screen->screen_type=screen_type;
#ifdef HAS_BUTTONBAR
    screen->has_buttonbar=false;
#endif
    gui_textarea_update_nblines(screen);
}

#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif

void screen_access_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
        screen_init(&screens[i], i);
}
