/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Stuart Martin
 * RTC config saving code (C) 2002 by hessu@hes.iki.fi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include "inttypes.h"
#include "config.h"
#include "action.h"
#include "crc32.h"
#include "settings.h"
#include "wps.h"
#include "file.h"
#include "buffer.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#include "skin_engine/skin_engine.h"
#include "skin_buffer.h"
#include "statusbar-skinned.h"
#include "bootchart.h"

static char *skin_buffer = NULL;
void theme_init_buffer(void)
{
    skin_buffer = buffer_alloc(SKIN_BUFFER_SIZE);
}


/* call this after loading a .wps/.rwps or other skin files, so that the
 * skin buffer is reset properly
 */
struct skin_load_setting {
    char* setting;
    char* suffix;
    void (*loadfunc)(enum screen_type screen, const char *buf, bool isfile);
};

static const struct skin_load_setting skins[] = {
    /* This determins the load order. *sbs must be loaded before any other
     * skin on that screen */
#ifdef HAVE_LCD_BITMAP
    { global_settings.sbs_file, "sbs", sb_skin_data_load},
#endif    
    { global_settings.wps_file, "wps", wps_data_load},
#if CONFIG_TUNER
    { global_settings.fms_file, "fms", fms_data_load},
#endif
#if defined(HAVE_REMOTE_LCD) && NB_SCREENS > 1
    { global_settings.rsbs_file, "rsbs", sb_skin_data_load},
    { global_settings.rwps_file, "rwps", wps_data_load},
#if CONFIG_TUNER
    { global_settings.rfms_file, "rfms", fms_data_load},
#endif
#endif
};

void settings_apply_skins(void)
{
    char buf[MAX_PATH];
    /* re-initialize the skin buffer before we start reloading skins */
    enum screen_type screen = SCREEN_MAIN;
    unsigned int i;
    
    skin_buffer_init(skin_buffer, SKIN_BUFFER_SIZE);
#ifdef HAVE_LCD_BITMAP
    skin_backdrop_init();
    skin_font_init();
#endif
#if CONFIG_TUNER
    fms_skin_init();
#endif
    for (i=0; i<ARRAYLEN(skins); i++)
    {
#ifdef HAVE_REMOTE_LCD
        screen = skins[i].suffix[0] == 'r' ? SCREEN_REMOTE : SCREEN_MAIN;
#endif
        CHART2(">skin load ", skins[i].suffix);
        if (skins[i].setting[0] && skins[i].setting[0] != '-')
        {
            char path[MAX_PATH];
            snprintf(buf, sizeof buf, "%s/%s.%s",
                     get_user_file_path(WPS_DIR, false, path, sizeof(path)),
                     skins[i].setting, skins[i].suffix);
            skins[i].loadfunc(screen, buf, true);
        }
        else
        {
            skins[i].loadfunc(screen, NULL, true);
        }
        CHART2("<skin load ", skins[i].suffix);
    }
    viewportmanager_theme_changed(THEME_STATUSBAR);
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    FOR_NB_SCREENS(i)
        screens[i].backdrop_show(sb_get_backdrop(i));
#endif
}
