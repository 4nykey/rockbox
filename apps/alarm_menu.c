/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Uwe Freese
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#ifdef HAVE_RTC_ALARM

#include <stdbool.h>

#include "options.h"

#include "lcd.h"
#include "action.h"
#include "kernel.h"
#include "sprintf.h"
#include <string.h>
#include "settings.h"
#include "power.h"
#include "icons.h"
#include "rtc.h"
#include "misc.h"
#include "screens.h"

#include "lang.h"
#include "power.h"
#include "alarm_menu.h"
#include "backlight.h"

#include "splash.h"
#include "statusbar.h"
#include "textarea.h"

bool alarm_screen(void)
{
    int h, m;
    bool done=false;
    char buf[32];
    struct tm *tm;
    int togo;
    int button;
    int i;
    bool update = true;
    

    rtc_get_alarm(&h, &m);

    /* After a battery change the RTC values are out of range */
    if (m > 60 || h > 24) {
        m = 0;
        h = 12;
    } else {
        m = m / 5 * 5; /* 5 min accuracy should be enough */
    }

    while(!done) {
        if(update)
        {
            FOR_NB_SCREENS(i)
            {
                screens[i].setmargins(0, 0);
                gui_textarea_clear(&screens[i]);
                screens[i].puts(0, 3, str(LANG_ALARM_MOD_KEYS));
            }
            update = false;
        }

        snprintf(buf, 32, str(LANG_ALARM_MOD_TIME), h, m);
        FOR_NB_SCREENS(i)
        {
            screens[i].puts(0, 1, buf);
            gui_textarea_update(&screens[i]);
        }
        button = get_action(CONTEXT_SETTINGS,HZ);

        switch(button) {
            case ACTION_STD_OK:
            /* prevent that an alarm occurs in the shutdown procedure */
            /* accept alarms only if they are in 2 minutes or more */
            tm = get_time();
            togo = (m + h * 60 - tm->tm_min - tm->tm_hour * 60 + 1440) % 1440;
            if (togo > 1) {
                rtc_init();
                rtc_set_alarm(h,m);
                rtc_enable_alarm(true);
                gui_syncsplash(HZ*2, true, str(LANG_ALARM_MOD_TIME_TO_GO),
                       togo / 60, togo % 60);
                done = true;
            } else {
                gui_syncsplash(HZ, true, str(LANG_ALARM_MOD_ERROR));
                update = true;
            }
            break;

         /* inc(m) */
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            m += 5;
            if (m == 60) {
                h += 1;
                m = 0;
            }
            if (h == 24)
                h = 0;
            break;

         /* dec(m) */
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
             m -= 5;
             if (m == -5) {
                 h -= 1;
                 m = 55;
             }
             if (h == -1)
                 h = 23;
             break;

         /* inc(h) */
         case ACTION_STD_NEXT:
         case ACTION_STD_NEXTREPEAT:
             h = (h+1) % 24;
             break;

         /* dec(h) */
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
             h = (h+23) % 24;
             break;

        case ACTION_STD_CANCEL:
            rtc_enable_alarm(false);
            gui_syncsplash(HZ*2, true, str(LANG_ALARM_MOD_DISABLE));
            done = true;
            break;

        case ACTION_NONE:
            gui_syncstatusbar_draw(&statusbars, false);
            break;

        default:
            if(default_event_handler(button) == SYS_USB_CONNECTED)
            {
                rtc_enable_alarm(false);
                return true;
            }
            break;
        }
    }
    action_signalscreenchange();
    return false;
}

#endif /* HAVE_RTC_ALARM */
