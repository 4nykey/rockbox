/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Sebastian Henriksen, Hardeep Sidhu
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "action.h"
#include "dir.h"
#include "file.h"
#include "filetree.h"
#include "kernel.h"
#include "keyboard.h"
#include "lang.h"
#include "list.h"
#include "misc.h"
#include "onplay.h"
#include "playlist.h"
#include "settings.h"
#include "splash.h"
#include "sprintf.h"
#include "tree.h"
#include "yesno.h"

#define PLAYLIST_CATALOG_CFG ROCKBOX_DIR "/playlist_catalog.config"
#define PLAYLIST_CATALOG_DEFAULT_DIR "/Playlists"
#define MAX_PLAYLISTS 400
#define PLAYLIST_DISPLAY_COUNT 10

/* Use for recursive directory search */
struct add_track_context {
    int fd;
    int count;
};

/* keep track of most recently used playlist */
static char most_recent_playlist[MAX_PATH];

/* directory where our playlists our stored (configured in
   PLAYLIST_CATALOG_CFG) */
static char playlist_dir[MAX_PATH];
static int  playlist_dir_length;
static bool playlist_dir_exists = false;

/* Retrieve playlist directory from config file and verify it exists */
static int initialize_catalog(void)
{
    static bool initialized = false;

    if (!initialized)
    {
        int f;
        DIR* dir;
        bool default_dir = true;

        f = open(PLAYLIST_CATALOG_CFG, O_RDONLY);
        if (f >= 0)
        {
            char buf[MAX_PATH+5];

            while (read_line(f, buf, sizeof(buf)))
            {
                char* name;
                char* value;

                /* directory config is of the format: "dir: /path/to/dir" */
                if (settings_parseline(buf, &name, &value) &&
                    !strncasecmp(name, "dir:", strlen(name)) &&
                    strlen(value) > 0)
                {
                    strncpy(playlist_dir, value, strlen(value));
                    default_dir = false;
                }
            }

            close(f);
        }

        /* fall back to default directory if no or invalid config */
        if (default_dir)
            strncpy(playlist_dir, PLAYLIST_CATALOG_DEFAULT_DIR,
                sizeof(playlist_dir));

        playlist_dir_length = strlen(playlist_dir);

        dir = opendir(playlist_dir);
        if (dir)
        {
            playlist_dir_exists = true;
            closedir(dir);
            memset(most_recent_playlist, 0, sizeof(most_recent_playlist));
            initialized = true;
        }
    }

    if (!playlist_dir_exists)
    {
        gui_syncsplash(HZ*2, true, str(LANG_CATALOG_NO_DIRECTORY),
            playlist_dir);
        return -1;
    }

    return 0;
}

/* Use the filetree functions to retrieve the list of playlists in the
   directory */
static int create_playlist_list(char** playlists, int num_items,
                                int* num_playlists)
{
    int result = -1;
    int num_files = 0;
    int index = 0;
    int i;
    bool most_recent = false;
    struct entry *files;
    struct tree_context* tc = tree_get_context();
    int dirfilter = *(tc->dirfilter);

    *num_playlists = 0;

    /* use the tree browser dircache to load only playlists */
    *(tc->dirfilter) = SHOW_PLAYLIST;
    
    if (ft_load(tc, playlist_dir) < 0)
    {
        gui_syncsplash(HZ*2, true, str(LANG_CATALOG_NO_DIRECTORY),
            playlist_dir);
        goto exit;
    }
    
    files = (struct entry*) tc->dircache;
    num_files = tc->filesindir;
    
    /* we've overwritten the dircache so tree browser will need to be
       reloaded */
    reload_directory();
    
    /* if it exists, most recent playlist will always be index 0 */
    if (most_recent_playlist[0] != '\0')
    {
        index = 1;
        most_recent = true;
    }

    for (i=0; i<num_files && index<num_items; i++)
    {
        if (files[i].attr & TREE_ATTR_M3U)
        {
            if (most_recent && !strncmp(files[i].name, most_recent_playlist,
                sizeof(most_recent_playlist)))
            {
                playlists[0] = files[i].name;
                most_recent = false;
            }
            else
            {
                playlists[index] = files[i].name;
                index++;
            }
        }
    }

    *num_playlists = index;

    /* we couldn't find the most recent playlist, shift all playlists up */
    if (most_recent)
    {
        for (i=0; i<index-1; i++)
            playlists[i] = playlists[i+1];

        (*num_playlists)--;

        most_recent_playlist[0] = '\0';
    }

    result = 0;

exit:
    *(tc->dirfilter) = dirfilter;
    return result;
}

/* Callback for gui_synclist */
static char* playlist_callback_name(int selected_item, void* data,
                                    char* buffer)
{
    char** playlists = (char**) data;

    strncpy(buffer, playlists[selected_item], MAX_PATH);

    return buffer;
}

/* Display all playlists in catalog.  Selected "playlist" is returned.
   If "view" mode is set then we're not adding anything into playlist. */
static int display_playlists(char* playlist, bool view)
{
    int result = -1;
    int num_playlists = 0;
    bool exit = false;
    char temp_buf[MAX_PATH];
    char* playlists[MAX_PLAYLISTS];
    struct gui_synclist playlist_lists;

    if (create_playlist_list(playlists, sizeof(playlists),
            &num_playlists) != 0)
        return -1;

    if (num_playlists <= 0)
    {
        gui_syncsplash(HZ*2, true, str(LANG_CATALOG_NO_PLAYLISTS));
        return -1;
    }

    if (!playlist)
        playlist = temp_buf;

    gui_synclist_init(&playlist_lists, playlist_callback_name, playlists,
        false, 1);
    gui_synclist_set_nb_items(&playlist_lists, num_playlists);
    gui_synclist_draw(&playlist_lists);

    while (!exit)
    {
        int button = get_action(CONTEXT_LIST,HZ/2);
        char* sel_file;

        gui_synclist_do_button(&playlist_lists, button);

        sel_file = playlists[gui_synclist_get_sel_pos(&playlist_lists)];

        switch (button)
        {
            case ACTION_STD_CANCEL:
                exit = true;
                break;

            case ACTION_STD_OK:
                if (view)
                {
                    /* In view mode, selecting a playlist starts playback */
                    if (playlist_create(playlist_dir, sel_file) != -1)
                    {
                        if (global_settings.playlist_shuffle)
                            playlist_shuffle(current_tick, -1);
                        playlist_start(0, 0);
                    }
                }
                else
                {
                    /* we found the playlist we want to add to */
                    snprintf(playlist, MAX_PATH, "%s/%s", playlist_dir,
                        sel_file);
                }

                result = 0;
                exit = true;
                break;

            case ACTION_STD_CONTEXT:
                /* context menu only available in view mode */
                if (view)
                {
                    snprintf(playlist, MAX_PATH, "%s/%s", playlist_dir,
                        sel_file);

                    if (onplay(playlist, TREE_ATTR_M3U,
                            CONTEXT_TREE) != ONPLAY_OK)
                    {
                        result = 0;
                        exit = true;
                    }
                    else
                        gui_synclist_draw(&playlist_lists);
                }
                break;

            case ACTION_NONE:
                gui_syncstatusbar_draw(&statusbars, false);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    result = -1;
                    exit = true;
                }
                break;
        }
    }
    action_signalscreenchange();
    return result;
}

/* display number of tracks inserted into playlists.  Used for directory
   insert */
static void display_insert_count(int count)
{
    gui_syncsplash(0, true, str(LANG_PLAYLIST_INSERT_COUNT), count,
#if CONFIG_KEYPAD == PLAYER_PAD
        str(LANG_STOP_ABORT)
#else
        str(LANG_OFF_ABORT)
#endif
        );
}

/* Add specified track into playlist.  Callback from directory insert */
static int add_track_to_playlist(char* filename, void* context)
{
    struct add_track_context* c = (struct add_track_context*) context;

    if (fdprintf(c->fd, "%s\n", filename) <= 0)
        return -1;

    (c->count)++;

    if (((c->count)%PLAYLIST_DISPLAY_COUNT) == 0)
        display_insert_count(c->count);

    return 0;
}

/* Add "sel" file into specified "playlist".  How to insert depends on type
   of file */
static int add_to_playlist(const char* playlist, char* sel, int sel_attr)
{
    int fd;
    int result = -1;
    
    fd = open(playlist, O_CREAT|O_WRONLY|O_APPEND);
    if(fd < 0)
        return result;

    /* In case we're in the playlist directory */
    reload_directory();

    if ((sel_attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
    {
        /* append the selected file */
        if (fdprintf(fd, "%s\n", sel) > 0)
            result = 0;
    }
    else if ((sel_attr & TREE_ATTR_MASK) == TREE_ATTR_M3U)
    {
        /* append playlist */
        int f, fs, i;
        char buf[1024];
        
        if(strcasecmp(playlist, sel) == 0)
            goto exit;
        
        f = open(sel, O_RDONLY);
        if (f < 0)
            goto exit;
        
        fs = filesize(f);
        
        for (i=0; i<fs;)
        {
            int n;
            
            n = read(f, buf, sizeof(buf));
            if (n < 0)
                break;
            
            if (write(fd, buf, n) < 0)
                break;
            
            i += n;
        }
        
        if (i >= fs)
            result = 0;
        
        close(f);
    }
    else if (sel_attr & ATTR_DIRECTORY)
    {
        /* search directory for tracks and append to playlist */
        bool recurse = false;
        char *lines[] = {
            (char *)str(LANG_RECURSE_DIRECTORY_QUESTION),
                sel
        };
        struct text_message message={lines, 2};
        struct add_track_context context;

        if (global_settings.recursive_dir_insert != RECURSE_ASK)
            recurse = (bool)global_settings.recursive_dir_insert;
        else
        {
            /* Ask if user wants to recurse directory */
            recurse = (gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES);
        }
        
        context.fd = fd;
        context.count = 0;

        display_insert_count(0);

        result = playlist_directory_tracksearch(sel, recurse,
            add_track_to_playlist, &context);

        display_insert_count(context.count);
    }

exit:
    close(fd);
    return result;
}

bool catalog_view_playlists(void)
{
    if (initialize_catalog() == -1)
        return false;

    if (display_playlists(NULL, true) == -1)
        return false;

    return true;
}

bool catalog_add_to_a_playlist(char* sel, int sel_attr, bool new_playlist)
{
    char playlist[MAX_PATH];
    
    if (initialize_catalog() == -1)
        return false;

    if (new_playlist)
    {
        snprintf(playlist, MAX_PATH, "%s/", playlist_dir);
        if (kbd_input(playlist, MAX_PATH))
            return false;
        
        if(strlen(playlist) <= 4 ||
            strcasecmp(&playlist[strlen(playlist)-4], ".m3u"))
            strcat(playlist, ".m3u");
    }
    else
    {
        if (display_playlists(playlist, false) == -1)
            return false;
    }

    if (add_to_playlist(playlist, sel, sel_attr) == 0)
    {
        strncpy(most_recent_playlist, playlist+playlist_dir_length+1,
            sizeof(most_recent_playlist));
        return true;
    }
    else
        return false;
}
