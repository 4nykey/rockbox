/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Greyscale framework
* Parameter handling
*
* This is a generic framework to display up to 33 shades of grey
* on low-depth bitmap LCDs (Archos b&w, Iriver 4-grey) within plugins.
*
* Copyright (C) 2004-2006 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "gray.h"

/* Set position of the top left corner of the greyscale overlay
   Note that by is in pixel-block units (8 pixels) */
void gray_set_position(int x, int by)
{
    _gray_info.x = x;
    _gray_info.by = by;
    
    if (_gray_info.flags & _GRAY_RUNNING)
    {
#ifdef SIMULATOR
        gray_deferred_lcd_update();
        gray_update();
#else
        _gray_info.flags |= _GRAY_DEFERRED_UPDATE;
#endif
    }
}

/* Set the draw mode for subsequent drawing operations */
void gray_set_drawmode(int mode)
{
    _gray_info.drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

/* Return the current draw mode */
int  gray_get_drawmode(void)
{
    return _gray_info.drawmode;
}

/* Set the foreground shade for subsequent drawing operations */
void gray_set_foreground(unsigned brightness)
{
    _gray_info.fg_brightness = brightness;
    _gray_info.fg_index = _gray_info.idxtable[brightness]; 
}

/* Return the current foreground shade */
unsigned gray_get_foreground(void)
{
    return _gray_info.fg_brightness;
}

/* Set the background shade for subsequent drawing operations */
void gray_set_background(unsigned brightness)
{
    _gray_info.bg_brightness = brightness;
    _gray_info.bg_index = _gray_info.idxtable[brightness];
}

/* Return the current background shade */
unsigned gray_get_background(void)
{
    return _gray_info.bg_brightness;
}

/* Set draw mode, foreground and background shades at once */
void gray_set_drawinfo(int mode, unsigned fg_brightness, unsigned bg_brightness)
{
    gray_set_drawmode(mode);
    gray_set_foreground(fg_brightness);
    gray_set_background(bg_brightness);
}

/* Set font for the text output routines */
void gray_setfont(int newfont)
{
    _gray_info.curfont = newfont;
}

/* Get width and height of a text when printed with the current font */
int  gray_getstringsize(const unsigned char *str, int *w, int *h)
{
    return _gray_rb->font_getstringsize(str, w, h, _gray_info.curfont);
}

#endif /* HAVE_LCD_BITMAP */
