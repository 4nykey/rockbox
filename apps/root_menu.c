/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config.h"
#include "menu.h"
#include "root_menu.h"
#include "lang.h"
#include "settings.h"
#include "screens.h"
#include "kernel.h"
#include "debug.h"
#include "misc.h"
#include "rolo.h"
#include "powermgmt.h"
#include "power.h"

#if LCD_DEPTH > 1
#include "backdrop.h"
#endif
#include "talk.h"
#include "audio.h"

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "splash.h"
#include "buttonbar.h"
#include "textarea.h"
#include "action.h"
#include "yesno.h"

#include "main_menu.h"
#include "tree.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_RECORDING
#include "recording.h"
#endif
#include "gwps-common.h"
#include "bookmark.h"
#include "tagtree.h"
#include "menus/exported_menus.h"
#ifdef HAVE_RTC_ALARM
#include "rtc.h"
#endif
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif

struct root_items {
    int (*function)(void* param);
    void* param;
    const struct menu_item_ex *context_menu;
};
static int last_screen = GO_TO_ROOT; /* unfortunatly needed so we can resume
                                        or goto current track based on previous
                                        screen */
static int browser(void* param)
{
    int ret_val;
#ifdef HAVE_TAGCACHE
    struct tree_context* tc = tree_get_context();
#endif
    int filter = SHOW_SUPPORTED;
    char folder[MAX_PATH] = "/";
    /* stuff needed to remember position in file browser */
    static char last_folder[MAX_PATH] = "/";
    /* and stuff for the database browser */
#ifdef HAVE_TAGCACHE
    static int last_db_dirlevel = 0, last_db_selection = 0;
#endif
    
    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            filter = global_settings.dirfilter;
            if (global_settings.browse_current && 
                    last_screen == GO_TO_WPS && audio_status() &&
                    wps_state.current_track_path[0] != '\0')
            {
                strcpy(folder, wps_state.current_track_path);
            }
            else
                strcpy(folder, last_folder);
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            if (!tagcache_is_usable())
            {
                bool reinit_attempted = false;

                /* Now display progress until it's ready or the user exits */
                while(!tagcache_is_usable())
                {
                    gui_syncstatusbar_draw(&statusbars, false);
                    struct tagcache_stat *stat = tagcache_get_stat();                
    
                    /* Allow user to exit */
                    if (action_userabort(HZ/2))
                        break;

                    /* Maybe just needs to reboot due to delayed commit */
                    if (stat->commit_delayed)
                    {
                        gui_syncsplash(HZ*2, str(LANG_PLEASE_REBOOT));
                        break;
                    }

                    /* Check if ready status is known */
                    if (!stat->readyvalid)
                    {
                        gui_syncsplash(0, str(LANG_TAGCACHE_BUSY));
                        continue;
                    }
               
                    /* Re-init if required */
                    if (!reinit_attempted && !stat->ready && 
                        stat->processed_entries == 0 && stat->commit_step == 0)
                    {
                        /* Prompt the user */
                        reinit_attempted = true;
                        char *lines[]={str(LANG_TAGCACHE_BUSY), str(LANG_TAGCACHE_FORCE_UPDATE)};
                        struct text_message message={lines, 2};
                        if(gui_syncyesno_run(&message, NULL, NULL) == YESNO_NO)
                            break;
                        int i;
                        FOR_NB_SCREENS(i)
                            screens[i].clear_display();

                        /* Start initialisation */
                        tagcache_rebuild();
                    }

                    /* Display building progress */
                    if (stat->commit_step > 0)
                    {
                        gui_syncsplash(0, "%s [%d/%d]",
                            str(LANG_TAGCACHE_INIT), stat->commit_step, 
                            tagcache_get_max_commit_step());
                    }
                    else
                    {
                        gui_syncsplash(0, str(LANG_BUILDING_DATABASE),
                            stat->processed_entries);
                    }
                }
            }
            if (!tagcache_is_usable())
                return GO_TO_PREVIOUS;
            filter = SHOW_ID3DB;
            tc->dirlevel = last_db_dirlevel;
            tc->selected_item = last_db_selection;
        break;
#endif
        case GO_TO_BROWSEPLUGINS:
            filter = SHOW_PLUGINS;
            snprintf(folder, MAX_PATH, "%s/", PLUGIN_DIR);
        break;
    }
    ret_val = rockbox_browse(folder, filter);
    switch ((intptr_t)param)
    {
        case GO_TO_FILEBROWSER:
            get_current_file(last_folder, MAX_PATH);
        break;
#ifdef HAVE_TAGCACHE
        case GO_TO_DBBROWSER:
            last_db_dirlevel = tc->dirlevel;
            last_db_selection = tc->selected_item;
        break;
#endif
    }
    return ret_val;
}  

static int menu(void* param)
{
    (void)param;
    return do_menu(NULL, 0);
    
}
#ifdef HAVE_RECORDING
static int recscrn(void* param)
{
    (void)param;
    recording_screen(false);
    return GO_TO_ROOT;
}
#endif
static int wpsscrn(void* param)
{
    int ret_val = GO_TO_PREVIOUS;
    (void)param;
    if (audio_status())
    {
        ret_val = gui_wps_show();
    }
    else if ( global_status.resume_index != -1 )
    {
        DEBUGF("Resume index %X offset %lX\n",
               global_status.resume_index,
               (unsigned long)global_status.resume_offset);
        if (playlist_resume() != -1)
        {
            playlist_start(global_status.resume_index,
                global_status.resume_offset);
            ret_val = gui_wps_show();
        }
    }
    else
    {
        gui_syncsplash(HZ*2, str(LANG_NOTHING_TO_RESUME));
    }
#if LCD_DEPTH > 1
    show_main_backdrop();
#endif
    return ret_val;
}
#if CONFIG_TUNER
static int radio(void* param)
{
    (void)param;
    radio_screen();
    return GO_TO_ROOT;
}
#endif

static int load_bmarks(void* param)
{
    (void)param;
    bookmark_mrb_load();
    return GO_TO_PREVIOUS;
}
/* These are all static const'd from apps/menus/ *.c
   so little hack so we can use them */
extern struct menu_item_ex 
        file_menu, 
        tagcache_menu,
        manage_settings,
        recording_setting_menu,
        bookmark_settings_menu,
        system_menu;
static const struct root_items items[] = {
    [GO_TO_FILEBROWSER] =   { browser, (void*)GO_TO_FILEBROWSER, &file_menu},
    [GO_TO_DBBROWSER] =     { browser, (void*)GO_TO_DBBROWSER, &tagcache_menu },
    [GO_TO_WPS] =           { wpsscrn, NULL, &playback_menu_item },
    [GO_TO_MAINMENU] =      { menu, NULL, &manage_settings },
    
#ifdef HAVE_RECORDING
    [GO_TO_RECSCREEN] =     {  recscrn, NULL, &recording_setting_menu },
#endif
    
#if CONFIG_TUNER
    [GO_TO_FM] =            { radio, NULL, NULL },
#endif
    
    [GO_TO_RECENTBMARKS] =  { load_bmarks, NULL, &bookmark_settings_menu }, 
    [GO_TO_BROWSEPLUGINS] = { browser, (void*)GO_TO_BROWSEPLUGINS, NULL }, 
    
};
static const int nb_items = sizeof(items)/sizeof(*items);

int item_callback(int action, const struct menu_item_ex *this_item) ;

MENUITEM_RETURNVALUE(file_browser, ID2P(LANG_DIR_BROWSER), GO_TO_FILEBROWSER,
                        NULL, Icon_file_view_menu);
#ifdef HAVE_TAGCACHE
MENUITEM_RETURNVALUE(db_browser, ID2P(LANG_TAGCACHE), GO_TO_DBBROWSER, 
                        NULL, Icon_Audio);
#endif
MENUITEM_RETURNVALUE(rocks_browser, ID2P(LANG_PLUGINS), GO_TO_BROWSEPLUGINS, 
                        NULL, Icon_Plugin);
char *get_wps_item_name(int selected_item, void * data, char *buffer)
{
    (void)selected_item; (void)data; (void)buffer;
    if (audio_status())
        return ID2P(LANG_NOW_PLAYING);
    return ID2P(LANG_RESUME_PLAYBACK);
}
MENUITEM_RETURNVALUE_DYNTEXT(wps_item, GO_TO_WPS, NULL, get_wps_item_name, 
                                NULL, Icon_Playback_menu);
#ifdef HAVE_RECORDING
MENUITEM_RETURNVALUE(rec, ID2P(LANG_RECORDING_MENU), GO_TO_RECSCREEN,  
                        NULL, Icon_Recording);
#endif
#if CONFIG_TUNER
MENUITEM_RETURNVALUE(fm, ID2P(LANG_FM_RADIO), GO_TO_FM,  
                        item_callback, Icon_Radio_screen);
#endif
MENUITEM_RETURNVALUE(menu_, ID2P(LANG_SETTINGS_MENU), GO_TO_MAINMENU,  
                        NULL, Icon_Submenu_Entered);
MENUITEM_RETURNVALUE(bookmarks, ID2P(LANG_BOOKMARK_MENU_RECENT_BOOKMARKS),
                        GO_TO_RECENTBMARKS,  item_callback, 
                        Icon_Bookmark);
#ifdef HAVE_LCD_CHARCELLS
static int do_shutdown(void)
{
#if CONFIG_CHARGING
    if (charger_inserted())
        charging_splash();
    else
#endif
        sys_poweroff();
    return 0;
}
MENUITEM_FUNCTION(do_shutdown_item, 0, ID2P(LANG_SHUTDOWN),
                  do_shutdown, NULL, NULL, Icon_NOICON);
#endif
MAKE_MENU(root_menu_, ID2P(LANG_ROCKBOX_TITLE),
            NULL, Icon_Rockbox,
            &bookmarks, &file_browser, 
#ifdef HAVE_TAGCACHE
            &db_browser,
#endif
            &wps_item, &menu_, 
#ifdef HAVE_RECORDING
            &rec, 
#endif
#if CONFIG_TUNER
            &fm,
#endif
            &playlist_options, &rocks_browser,  &info_menu

#ifdef HAVE_LCD_CHARCELLS
            ,&do_shutdown_item
#endif
        );

int item_callback(int action, const struct menu_item_ex *this_item) 
{
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
#if CONFIG_TUNER
            if (this_item == &fm)
            {
                if (radio_hardware_present() == 0)
                    return ACTION_EXIT_MENUITEM;
            }
            else 
#endif
                if (this_item == &bookmarks)
            {
                if (global_settings.usemrb == 0)
                    return ACTION_EXIT_MENUITEM;
            }
        break;
    }
    return action;
}
static int get_selection(int last_screen)
{
    unsigned int i;
    for(i=0; i< sizeof(root_menu__)/sizeof(*root_menu__); i++)
    {
        if ((root_menu__[i]->flags&MT_RETURN_VALUE) && 
            (root_menu__[i]->value == last_screen))
        {
            return i;
        }
    }
    return 0;
}

static inline int load_screen(int screen)
{
    /* set the global_status.last_screen before entering,
        if we dont we will always return to the wrong screen on boot */
    int old_previous = last_screen;
    int ret_val;
    if (screen <= GO_TO_ROOT)
        return screen;
    if (screen == old_previous)
        old_previous = GO_TO_ROOT;
    global_status.last_screen = (char)screen;
    status_save();
    action_signalscreenchange();
    ret_val = items[screen].function(items[screen].param);
    last_screen = screen;
    if (ret_val == GO_TO_PREVIOUS)
        last_screen = old_previous;
    return ret_val;
}
static int load_context_screen(int selection)
{
    const struct menu_item_ex *context_menu = NULL;
    if (root_menu__[selection]->flags&MT_RETURN_VALUE)
    {
        int item = root_menu__[selection]->value;
        context_menu = items[item].context_menu;
    }
    /* special cases */
    else if (root_menu__[selection] == &info_menu)
    {
        context_menu = &system_menu;
    }
    
    if (context_menu)
        return do_menu(context_menu, NULL);
    else
        return GO_TO_PREVIOUS;
}
void root_menu(void)
{
    int previous_browser = GO_TO_FILEBROWSER;
    int previous_music = GO_TO_WPS;
    int next_screen = GO_TO_ROOT;
    int selected = 0;

    if (global_settings.start_in_screen == 0)
        next_screen = (int)global_status.last_screen;
    else next_screen = global_settings.start_in_screen - 2;
    
#ifdef HAVE_RTC_ALARM
    if ( rtc_check_alarm_started(true) ) 
    {
        rtc_enable_alarm(false);
        next_screen = GO_TO_WPS;
#if CONFIG_TUNER
        if (global_settings.alarm_wake_up_screen == ALARM_START_FM)
            next_screen = GO_TO_FM;
#endif
#ifdef HAVE_RECORDING
        if (global_settings.alarm_wake_up_screen == ALARM_START_REC)
        {
            recording_start_automatic = true;
            next_screen = GO_TO_RECSCREEN;
        }
#endif
    }
#endif /* HAVE_RTC_ALARM */

#ifdef HAVE_HEADPHONE_DETECTION
    if (next_screen == GO_TO_WPS && 
        (global_settings.unplug_autoresume && !headphones_inserted() ))
            next_screen = GO_TO_ROOT;
#endif

    while (true)
    {
        switch (next_screen)
        {
            case MENU_ATTACHED_USB:
            case MENU_SELECTED_EXIT:
                /* fall through */
            case GO_TO_ROOT:
                if (last_screen != GO_TO_ROOT)
                    selected = get_selection(last_screen);
                next_screen = do_menu(&root_menu_, &selected);
                if (next_screen != GO_TO_PREVIOUS)
                    last_screen = GO_TO_ROOT;
                break;

            case GO_TO_PREVIOUS:
                next_screen = last_screen;
                break;

            case GO_TO_PREVIOUS_BROWSER:
                next_screen = previous_browser;
                break;

            case GO_TO_PREVIOUS_MUSIC:
                next_screen = previous_music;
                break;
            case GO_TO_ROOTITEM_CONTEXT:
                next_screen = load_context_screen(selected);
                break;
            default:
                if (next_screen == GO_TO_FILEBROWSER 
#ifdef HAVE_TAGCACHE
                    || next_screen == GO_TO_DBBROWSER
#endif
                   )
                    previous_browser = next_screen;
                if (next_screen == GO_TO_WPS 
#if CONFIG_TUNER
                    || next_screen == GO_TO_FM
#endif
                   )
                    previous_music = next_screen;
                next_screen = load_screen(next_screen);
                break;
        } /* switch() */
    }
    return;
}
