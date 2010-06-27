/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "plugin.h"
#include "tv_preferences.h"


static struct tv_preferences prefs;
/* read-only preferences pointer, for access by other files */
const struct tv_preferences * const preferences = &prefs;

static int listner_count = 0;

#define TV_MAX_LISTNERS 5
static void (*listners[TV_MAX_LISTNERS])(const struct tv_preferences *oldp);

static void tv_notify_change_preferences(const struct tv_preferences *oldp)
{
    int i;

    /*
     * the following items do not check.
     *   - alignment
     *   - horizontal_scroll_mode
     *   - vertical_scroll_mode
     *   - page_mode
     *   - font
     *   - autoscroll_speed
     *   - narrow_mode
     */
    if ((oldp == NULL)                                                    ||
        (oldp->word_mode            != preferences->word_mode)            ||
        (oldp->line_mode            != preferences->line_mode)            ||
        (oldp->windows              != preferences->windows)              ||
        (oldp->horizontal_scrollbar != preferences->horizontal_scrollbar) ||
        (oldp->vertical_scrollbar   != preferences->vertical_scrollbar)   ||
        (oldp->encoding             != preferences->encoding)             ||
        (oldp->indent_spaces        != preferences->indent_spaces)        ||
#ifdef HAVE_LCD_BITMAP
        (oldp->header_mode          != preferences->header_mode)          ||
        (oldp->footer_mode          != preferences->footer_mode)          ||
        (oldp->statusbar            != preferences->statusbar)            ||
        (rb->strcmp(oldp->font_name, preferences->font_name))             ||
#endif
        (rb->strcmp(oldp->file_name, preferences->file_name)))
    {
        /* callback functions are called as FILO */
        for (i = listner_count - 1; i >= 0; i--)
            listners[i](oldp);
    }
}

void tv_set_preferences(const struct tv_preferences *new_prefs)
{
    static struct tv_preferences old_prefs;
    struct tv_preferences *oldp = NULL;
    static bool is_initialized = false;

    if (is_initialized)
        tv_copy_preferences((oldp = &old_prefs));
    is_initialized = true;

    rb->memcpy(&prefs, new_prefs, sizeof(struct tv_preferences));
    tv_notify_change_preferences(oldp);
}

void tv_copy_preferences(struct tv_preferences *copy_prefs)
{
    rb->memcpy(copy_prefs, preferences, sizeof(struct tv_preferences));
}

void tv_set_default_preferences(struct tv_preferences *p)
{
    p->word_mode = WM_WRAP;
    p->line_mode = LM_NORMAL;
    p->windows = 1;
    p->alignment = AL_LEFT;
    p->horizontal_scroll_mode = HS_SCREEN;
    p->vertical_scroll_mode = VS_PAGE;
    p->page_mode = PM_NO_OVERLAP;
    p->horizontal_scrollbar = SB_OFF;
    p->vertical_scrollbar = SB_OFF;
#ifdef HAVE_LCD_BITMAP
    p->header_mode = HD_PATH;
    p->footer_mode = FT_PAGE;
    p->statusbar   = true;
    rb->strlcpy(p->font_name, rb->global_settings->font_file, MAX_PATH);
    p->font = rb->font_get(FONT_UI);
#else
    p->header_mode = HD_NONE;
    p->footer_mode = FT_NONE;
    p->statusbar   = false;
#endif
    p->autoscroll_speed = 1;
    p->narrow_mode = NM_PAGE;
    p->indent_spaces = 2;
    /* Set codepage to system default */
    p->encoding = rb->global_settings->default_codepage;
    p->file_name[0] = '\0';
}

void tv_add_preferences_change_listner(void (*listner)(const struct tv_preferences *oldp))
{
    if (listner_count < TV_MAX_LISTNERS)
        listners[listner_count++] = listner;
}
