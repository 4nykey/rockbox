/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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

/* Button Code Definitions for the Philips SA9200 target */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

/*
 * The format of the list is as follows
 * { Action Code,   Button code,    Prereq button code }
 * if there's no need to check the previous button's value, use BUTTON_NONE
 * Insert LAST_ITEM_IN_LIST at the end of each mapping
 */

/* CONTEXT_CUSTOM's used in this file...

CONTEXT_CUSTOM|CONTEXT_TREE = the standard list/tree defines (without directions)
CONTEXT_CUSTOM|CONTEXT_SETTINGS = the direction keys for the eq/col picker screens
                                  i.e where up/down is inc/dec
               CONTEXT_SETTINGS = up/down is prev/next, l/r is inc/dec

*/


static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,          BUTTON_UP,                      BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_CANCEL,        BUTTON_REW,                     BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_POWER,                   BUTTON_NONE },

    { ACTION_STD_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },

    { ACTION_STD_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,      BUTTON_MENU },
    { ACTION_STD_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_MENU },

    { ACTION_STD_OK,            BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },
    { ACTION_STD_OK,            BUTTON_FFWD,                    BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_standard */


static const struct button_mapping button_context_wps[]  = {
    { ACTION_WPS_PLAY,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_WPS_STOP,          BUTTON_POWER|BUTTON_REL,        BUTTON_POWER },

    { ACTION_WPS_SKIPNEXT,      BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD },
    { ACTION_WPS_SKIPPREV,      BUTTON_REW|BUTTON_REL,          BUTTON_REW },

    { ACTION_WPS_SEEKBACK,      BUTTON_REW|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_SEEKFWD,       BUTTON_FFWD|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_REW|BUTTON_REL,          BUTTON_REW|BUTTON_REPEAT },
    { ACTION_WPS_STOPSEEK,      BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD|BUTTON_REPEAT },

    { ACTION_WPS_ABSETB_NEXTDIR,    BUTTON_RIGHT|BUTTON_FFWD,   BUTTON_NONE },
    { ACTION_WPS_ABSETA_PREVDIR,    BUTTON_RIGHT|BUTTON_REW,    BUTTON_NONE },
    { ACTION_WPS_ABRESET,       BUTTON_RIGHT|BUTTON_SELECT,     BUTTON_NONE },

    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_DOWN,                    BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_UP,                      BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_VOL_UP,                  BUTTON_NONE },

    { ACTION_WPS_PITCHSCREEN,   BUTTON_RIGHT|BUTTON_UP,         BUTTON_RIGHT },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_RIGHT|BUTTON_DOWN,       BUTTON_RIGHT },

    { ACTION_WPS_QUICKSCREEN,   BUTTON_MENU|BUTTON_REPEAT,      BUTTON_MENU },
    { ACTION_WPS_MENU,          BUTTON_MENU|BUTTON_REL,         BUTTON_MENU },
    { ACTION_WPS_CONTEXT,       BUTTON_SELECT|BUTTON_REPEAT,    BUTTON_SELECT },

    { ACTION_WPS_ID3SCREEN,     BUTTON_RIGHT|BUTTON_MENU,       BUTTON_NONE },
    { ACTION_WPS_BROWSE,        BUTTON_SELECT|BUTTON_REL,       BUTTON_SELECT },

    LAST_ITEM_IN_LIST
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
    { ACTION_LISTTREE_PGUP,     BUTTON_RIGHT|BUTTON_UP,                 BUTTON_RIGHT },
    { ACTION_LISTTREE_PGUP,     BUTTON_UP|BUTTON_REL,                   BUTTON_RIGHT|BUTTON_UP },
    { ACTION_LISTTREE_PGUP,     BUTTON_RIGHT|BUTTON_UP|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RIGHT|BUTTON_DOWN,               BUTTON_RIGHT },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_DOWN|BUTTON_REL,                 BUTTON_RIGHT|BUTTON_DOWN },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_RIGHT|BUTTON_DOWN|BUTTON_REPEAT, BUTTON_NONE },
#ifdef HAVE_VOLUME_IN_LIST
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_LIST_VOLUP,        BUTTON_VOL_UP,                  BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN,                BUTTON_NONE },
    { ACTION_LIST_VOLDOWN,      BUTTON_VOL_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
#endif
    
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,          BUTTON_RIGHT|BUTTON_REL,        BUTTON_RIGHT },
    { ACTION_TREE_STOP,         BUTTON_POWER,                   BUTTON_NONE },
    { ACTION_TREE_STOP,         BUTTON_POWER|BUTTON_REL,        BUTTON_POWER },
    { ACTION_TREE_STOP,         BUTTON_POWER|BUTTON_REPEAT,     BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    { ACTION_NONE,              BUTTON_RIGHT,                           BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_RIGHT|BUTTON_REW,                BUTTON_RIGHT },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REL,                  BUTTON_RIGHT|BUTTON_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_RIGHT|BUTTON_REW,                BUTTON_REW|BUTTON_REL },
    { ACTION_TREE_ROOT_INIT,    BUTTON_RIGHT|BUTTON_REW|BUTTON_REPEAT,  BUTTON_RIGHT|BUTTON_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_RIGHT|BUTTON_REW|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_FFWD,               BUTTON_RIGHT },
    { ACTION_TREE_PGRIGHT,      BUTTON_FFWD|BUTTON_REL,                 BUTTON_RIGHT|BUTTON_FFWD },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_FFWD,               BUTTON_FFWD|BUTTON_REL },
    { ACTION_TREE_PGRIGHT,      BUTTON_RIGHT|BUTTON_FFWD|BUTTON_REPEAT, BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    { ACTION_NONE,              BUTTON_REW,                     BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_REW|BUTTON_REL,          BUTTON_REW },
    { ACTION_TREE_ROOT_INIT,    BUTTON_REW|BUTTON_REPEAT,       BUTTON_REW },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_TREE_PGLEFT,       BUTTON_REW|BUTTON_REL,          BUTTON_REW|BUTTON_REPEAT },
    { ACTION_NONE,              BUTTON_FFWD,                    BUTTON_NONE },
    { ACTION_STD_OK,            BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD },
    { ACTION_TREE_PGRIGHT,      BUTTON_FFWD|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_TREE_PGRIGHT,      BUTTON_FFWD|BUTTON_REL,         BUTTON_FFWD|BUTTON_REPEAT },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_REW,                 BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_FFWD,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_FFWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_RIGHT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_FFWD,                BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_FFWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_REW,                 BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_STD_PREV,              BUTTON_UP,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,        BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,              BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,        BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_RIGHT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,          BUTTON_SELECT,              BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    { ACTION_STD_OK,                BUTTON_RIGHT|BUTTON_REL,    BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    { ACTION_STD_OK,                BUTTON_SELECT|BUTTON_REL,   BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM|CONTEXT_SETTINGS),
}; /* button_context_eq */

/** Bookmark Screen **/
static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,            BUTTON_RIGHT,               BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_LIST),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_STD_CANCEL,            BUTTON_POWER,               BUTTON_NONE },
    { ACTION_STD_OK,                BUTTON_RIGHT,               BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_SETTINGS),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {
    { ACTION_QS_DOWNINV,            BUTTON_UP,                  BUTTON_NONE },
    { ACTION_QS_DOWNINV,            BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_DOWN,               BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_QS_DOWN,               BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_QS_LEFT,               BUTTON_REW,                 BUTTON_NONE },
    { ACTION_QS_LEFT,               BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_QS_RIGHT,              BUTTON_FFWD,                BUTTON_NONE },
    { ACTION_QS_RIGHT,              BUTTON_FFWD|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_CANCEL,            BUTTON_MENU,                BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {
    { ACTION_PS_INC_SMALL,          BUTTON_UP,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,            BUTTON_UP|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,          BUTTON_DOWN,                BUTTON_NONE },
    { ACTION_PS_DEC_BIG,            BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,         BUTTON_REW,                 BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,      BUTTON_REW|BUTTON_REL,      BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,        BUTTON_FFWD,                BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF,     BUTTON_FFWD|BUTTON_REL,     BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,        BUTTON_MENU,                BUTTON_NONE },
    { ACTION_PS_RESET,              BUTTON_RIGHT,               BUTTON_NONE },
    { ACTION_PS_EXIT,               BUTTON_POWER,               BUTTON_NONE },
    { ACTION_PS_SLOWER,             BUTTON_REW|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_PS_FASTER,             BUTTON_FFWD|BUTTON_REPEAT,  BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {
    { ACTION_KBD_LEFT,         BUTTON_REW,                             BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_REW|BUTTON_REPEAT,               BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_FFWD,                            BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_FFWD|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RIGHT|BUTTON_REW,                BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_RIGHT|BUTTON_REW|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RIGHT|BUTTON_FFWD,               BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_RIGHT|BUTTON_FFWD|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_SELECT,                          BUTTON_NONE },
    { ACTION_KBD_PAGE_FLIP,    BUTTON_RIGHT|BUTTON_MENU,               BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_RIGHT|BUTTON_REL,                BUTTON_RIGHT },
    { ACTION_KBD_ABORT,        BUTTON_POWER|BUTTON_REL,                BUTTON_POWER },
    { ACTION_KBD_BACKSPACE,    BUTTON_MENU,                            BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_MENU|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP,                              BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_UP|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN,                            BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_DOWN|BUTTON_REPEAT,              BUTTON_NONE },
    { ACTION_KBD_MORSE_INPUT,  BUTTON_RIGHT|BUTTON_POWER,              BUTTON_NONE },
    { ACTION_KBD_MORSE_SELECT, BUTTON_SELECT|BUTTON_REL,               BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_keyboard */

#ifdef HAVE_USBSTACK
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_PLAY,     BUTTON_RIGHT,                     BUTTON_NONE },
    { ACTION_USB_HID_STOP,     BUTTON_POWER,                     BUTTON_NONE },
    { ACTION_USB_HID_SKIPPREV, BUTTON_REW,                       BUTTON_NONE },
    { ACTION_USB_HID_SKIPNEXT, BUTTON_FFWD,                      BUTTON_NONE },
    { ACTION_USB_HID_VOLUP,    BUTTON_UP,                        BUTTON_NONE },
    { ACTION_USB_HID_VOLUP,    BUTTON_UP|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_USB_HID_VOLUP,    BUTTON_VOL_UP,                    BUTTON_NONE },
    { ACTION_USB_HID_VOLUP,    BUTTON_VOL_UP|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_USB_HID_VOLDOWN,  BUTTON_DOWN,                      BUTTON_NONE },
    { ACTION_USB_HID_VOLDOWN,  BUTTON_DOWN|BUTTON_REPEAT,        BUTTON_NONE },
    { ACTION_USB_HID_VOLDOWN,  BUTTON_VOL_DOWN,                  BUTTON_NONE },
    { ACTION_USB_HID_VOLDOWN,  BUTTON_VOL_DOWN|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MUTE,     BUTTON_SELECT,                    BUTTON_NONE },
    { ACTION_USB_HID_MENU,     BUTTON_MENU,                      BUTTON_NONE },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */
#endif

const struct button_mapping* get_context_mapping(int context)
{
    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_MAINMENU:
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
            return button_context_settings_right_is_inc;

        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
#ifdef HAVE_USBSTACK
        case CONTEXT_USB_HID:
            return button_context_usb_hid;
#endif
    }
    return button_context_standard;
}
