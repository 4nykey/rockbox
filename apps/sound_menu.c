/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Bj�rn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "mpeg.h"
#include "settings.h"

static char *fmt[] =
{
    "",                 /* no decimals */
    "%d.%d %s  ",       /* 1 decimal */
    "%d.%02d %s  "      /* 2 decimals */
};

void set_sound(char* string, 
               int* variable,
               int setting)
{
    bool done = false;
    int min, max;
    int val;
    int numdec;
    int integer;
    int dec;
    char* unit;
    char str[32];

    unit = mpeg_sound_unit(setting);
    numdec = mpeg_sound_numdecimals(setting);
    min = mpeg_sound_min(setting);
    max = mpeg_sound_max(setting);
    
    lcd_clear_display();
    lcd_puts_scroll(0,0,string);

    while (!done) {
        val = mpeg_val2phys(setting, *variable);
        if(numdec)
        {
            integer = val / (10 * numdec);
            dec = val % (10 * numdec);
            snprintf(str,sizeof str, fmt[numdec], integer, dec, unit);
        }
        else
        {
            snprintf(str,sizeof str,"%d %s  ", val, unit);
        }
        lcd_puts(0,1,str);
        lcd_update();

        switch( button_get(true) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#else
            case BUTTON_RIGHT:
#endif
                (*variable)++;
                if(*variable > max )
                    *variable = max;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#else
            case BUTTON_LEFT:
#endif
                (*variable)--;
                if(*variable < min )
                    *variable = min;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                done = true;
                break;
        }
        mpeg_sound_set(setting, *variable);
    }
    lcd_stop_scroll();
}

static void volume(void)
{
    set_sound("Volume", &global_settings.volume, SOUND_VOLUME);
}

static void bass(void)
{
    set_sound("Bass", &global_settings.bass, SOUND_BASS);
};

static void treble(void)
{
    set_sound("Treble", &global_settings.treble, SOUND_TREBLE);
}

void sound_menu(void)
{
    int m;
    struct menu_items items[] = {
        { "Volume", volume },
        { "Bass",   bass },
        { "Treble", treble }
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}
