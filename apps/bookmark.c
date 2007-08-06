/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C)2003 by Benjamin Metzler
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "applimits.h"
#include "lcd.h"
#include "action.h"
#include "usb.h"
#include "audio.h"
#include "playlist.h"
#include "settings.h"
#include "tree.h"
#include "bookmark.h"
#include "dir.h"
#include "status.h"
#include "system.h"
#include "errno.h"
#include "icons.h"
#include "atoi.h"
#include "string.h"
#include "menu.h"
#include "lang.h"
#include "screens.h"
#include "status.h"
#include "debug.h"
#include "kernel.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "abrepeat.h"
#include "splash.h"
#include "yesno.h"
#include "list.h"
#include "plugin.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
#include "backdrop.h"
#endif

#define MAX_BOOKMARKS 10
#define MAX_BOOKMARK_SIZE  350
#define RECENT_BOOKMARK_FILE ROCKBOX_DIR "/most-recent.bmark"

/* Used to buffer bookmarks while displaying the bookmark list. */
struct bookmark_list
{
    const char* filename;
    size_t buffer_size;
    int start;
    int count;
    int total_count;
    bool show_dont_resume;
    bool reload;
    char* items[];
};

static bool  add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                          bool most_recent);
static bool  check_bookmark(const char* bookmark);
static char* create_bookmark(void);
static bool  delete_bookmark(const char* bookmark_file_name, int bookmark_id);
static void  say_bookmark(const char* bookmark,
                          int bookmark_id);
static bool play_bookmark(const char* bookmark);
static bool  generate_bookmark_file_name(const char *in);
static const char* skip_token(const char* s);
static const char* int_token(const char* s, int* dest);
static const char* long_token(const char* s, long* dest);
static const char* bool_token(const char* s, bool* dest);
static bool  parse_bookmark(const char *bookmark,
                            int *resume_index,
                            int *resume_offset,
                            int *resume_seed,
                            int *resume_first_index,
                            char* resume_file,
                            unsigned int resume_file_size,
                            long* ms,
                            int * repeat_mode,
                            bool *shuffle,
                            char* file_name);
static int buffer_bookmarks(struct bookmark_list* bookmarks, int first_line);
static char* get_bookmark_info(int list_index, void* data, char *buffer);
static char* select_bookmark(const char* bookmark_file_name, bool show_dont_resume);
static bool  system_check(void);
static bool  write_bookmark(bool create_bookmark_file);
static int   get_bookmark_count(const char* bookmark_file_name);

static char global_temp_buffer[MAX_PATH+1];
/* File name created by generate_bookmark_file_name */
static char global_bookmark_file_name[MAX_PATH];
static char global_read_buffer[MAX_BOOKMARK_SIZE];
/* Bookmark created by create_bookmark*/
static char global_bookmark[MAX_BOOKMARK_SIZE];
/* Filename from parsed bookmark (can be made local where needed) */
static char global_filename[MAX_PATH];

/* ----------------------------------------------------------------------- */
/* This is the interface function from the main menu.                      */
/* ----------------------------------------------------------------------- */
bool bookmark_create_menu(void)
{
    write_bookmark(true);
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function acts as the load interface from the main menu             */
/* This function determines the bookmark file name and then loads that file*/
/* for the user.  The user can then select a bookmark to load.             */
/* If no file/directory is currently playing, the menu item does not work. */
/* ----------------------------------------------------------------------- */
bool bookmark_load_menu(void)
{
    if (system_check())
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            char* bookmark = select_bookmark(global_bookmark_file_name, false);
            
            if (bookmark != NULL)
            {
                return play_bookmark(bookmark);
            }
        }
    }

    return false;
}

/* ----------------------------------------------------------------------- */
/* Gives the user a list of the Most Recent Bookmarks.  This is an         */
/* interface function                                                      */
/* ----------------------------------------------------------------------- */
bool bookmark_mrb_load()
{
    char* bookmark = select_bookmark(RECENT_BOOKMARK_FILE, false);

    if (bookmark != NULL)
    {
        return play_bookmark(bookmark);
    }

    return false;
}

/* ----------------------------------------------------------------------- */
/* This function handles an autobookmark creation.  This is an interface   */
/* function.                                                               */
/* ----------------------------------------------------------------------- */
bool bookmark_autobookmark(void)
{
    if (!system_check())
        return false;

    audio_pause();    /* first pause playback */
    switch (global_settings.autocreatebookmark)
    {
        case BOOKMARK_YES:
            return write_bookmark(true);

        case BOOKMARK_NO:
            return false;

        case BOOKMARK_RECENT_ONLY_YES:
            return write_bookmark(false);
    }
#ifdef HAVE_LCD_BITMAP
    unsigned char *lines[]={ID2P(LANG_AUTO_BOOKMARK_QUERY)};
    struct text_message message={(char **)lines, 1};
#else
    unsigned char *lines[]={ID2P(LANG_AUTO_BOOKMARK_QUERY),
                            str(LANG_CONFIRM_WITH_BUTTON)};
    struct text_message message={(char **)lines, 2};
#endif
#if LCD_DEPTH > 1
    show_main_backdrop(); /* switch to main backdrop as we may come from wps */
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    show_remote_main_backdrop();
#endif
    gui_syncstatusbar_draw(&statusbars, false);
    if(gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
    {
        if (global_settings.autocreatebookmark == BOOKMARK_RECENT_ONLY_ASK)
            return write_bookmark(false);
        else
            return write_bookmark(true);
    }
    return false;
}

/* ----------------------------------------------------------------------- */
/* This function takes the current current resume information and writes   */
/* that to the beginning of the bookmark file.                             */
/* This file will contain N number of bookmarks in the following format:   */
/* resume_index*resume_offset*resume_seed*resume_first_index*              */
/* resume_file*milliseconds*MP3 Title*                                     */
/* ------------------------------------------------------------------------*/
static bool write_bookmark(bool create_bookmark_file)
{
    bool   success=false;
    char*  bookmark;

    if (!system_check())
       return false; /* something didn't happen correctly, do nothing */

    bookmark = create_bookmark();
    if (!bookmark)
       return false; /* something didn't happen correctly, do nothing */

    if (global_settings.usemrb)
        success = add_bookmark(RECENT_BOOKMARK_FILE, bookmark, true);


    /* writing the bookmark */
    if (create_bookmark_file)
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            success = add_bookmark(global_bookmark_file_name, bookmark, false);
        }
    }

    gui_syncsplash(HZ, success ? ID2P(LANG_BOOKMARK_CREATE_SUCCESS)
        : ID2P(LANG_BOOKMARK_CREATE_FAILURE));

    return true;
}

/* ----------------------------------------------------------------------- */
/* This function adds a bookmark to a file.                                */
/* ------------------------------------------------------------------------*/
static bool add_bookmark(const char* bookmark_file_name, const char* bookmark, 
                         bool most_recent)
{
    int    temp_bookmark_file = 0;
    int    bookmark_file = 0;
    int    bookmark_count = 0;
    char*  playlist = NULL;
    char*  cp;
    char*  tmp;
    int    len = 0;
    bool   unique = false;

    /* Opening up a temp bookmark file */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer),
             "%s.tmp", bookmark_file_name);
    temp_bookmark_file = open(global_temp_buffer,
                              O_WRONLY | O_CREAT | O_TRUNC);
    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    if (most_recent && (global_settings.usemrb == BOOKMARK_UNIQUE_ONLY))
    {
        playlist = strchr(bookmark,'/');
        cp = strrchr(bookmark,';');
        len = cp - playlist;
        unique = true;
    }

    /* Writing the new bookmark to the begining of the temp file */
    write(temp_bookmark_file, bookmark, strlen(bookmark));
    write(temp_bookmark_file, "\n", 1);
    bookmark_count++;

    /* Reading in the previous bookmarks and writing them to the temp file */
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_read_buffer,
                         sizeof(global_read_buffer)) > 0)
        {
            /* The MRB has a max of MAX_BOOKMARKS in it */
            /* This keeps it from getting too large */
            if (most_recent && (bookmark_count >= MAX_BOOKMARKS))
                break;
                        
            cp  = strchr(global_read_buffer,'/');
            tmp = strrchr(global_read_buffer,';');
            if (check_bookmark(global_read_buffer) &&
               (!unique || len != tmp -cp || strncmp(playlist,cp,len)))
            {
                bookmark_count++;
                write(temp_bookmark_file, global_read_buffer,
                      strlen(global_read_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    remove(bookmark_file_name);
    rename(global_temp_buffer, bookmark_file_name);

    return true;
}


/* ----------------------------------------------------------------------- */
/* This function takes the system resume data and formats it into a valid  */
/* bookmark.                                                               */
/* ----------------------------------------------------------------------- */
static char* create_bookmark()
{
    int resume_index = 0;
    char *file;

    /* grab the currently playing track */
    struct mp3entry *id3 = audio_current_track();
    if(!id3)
        return NULL;

    /* Get some basic resume information */
    /* queue_resume and queue_resume_index are not used and can be ignored.*/
    playlist_get_resume_info(&resume_index);

    /* Get the currently playing file minus the path */
    /* This is used when displaying the available bookmarks */
    file = strrchr(id3->path,'/');
    if(NULL == file)
        return NULL;

    /* create the bookmark */
    snprintf(global_bookmark, sizeof(global_bookmark),
             "%d;%ld;%d;%d;%ld;%d;%d;%s;%s",
             resume_index,
             id3->offset,
             playlist_get_seed(NULL),
             0,
             id3->elapsed,
             global_settings.repeat_mode,
             global_settings.playlist_shuffle,
             playlist_get_name(NULL, global_temp_buffer,
                sizeof(global_temp_buffer)),
             file+1);

    /* checking to see if the bookmark is valid */
    if (check_bookmark(global_bookmark))
        return global_bookmark;
    else
        return NULL;
}

static bool check_bookmark(const char* bookmark)
{
    return parse_bookmark(bookmark,
                          NULL,NULL,NULL, NULL,
                          NULL,0,NULL,NULL,
                          NULL, NULL);
}

/* ----------------------------------------------------------------------- */
/* This function will determine if an autoload is necessary.  This is an   */
/* interface function.                                                     */
/* ------------------------------------------------------------------------*/
bool bookmark_autoload(const char* file)
{
    int  fd;

    if(global_settings.autoloadbookmark == BOOKMARK_NO)
        return false;

    /*Checking to see if a bookmark file exists.*/
    if(!generate_bookmark_file_name(file))
    {
        return false;
    }
    fd = open(global_bookmark_file_name, O_RDONLY);
    if(fd<0)
        return false;
    close(fd);
    if(global_settings.autoloadbookmark == BOOKMARK_YES)
    {
        return bookmark_load(global_bookmark_file_name, true);
    }
    else
    {
        char* bookmark = select_bookmark(global_bookmark_file_name, true);
        
        if (bookmark)
        {
            return bookmark_load(global_bookmark_file_name, true);
        }

        return false;
    }
}

/* ----------------------------------------------------------------------- */
/* This function loads the bookmark information into the resume memory.    */
/* This is an interface function.                                          */
/* ------------------------------------------------------------------------*/
bool bookmark_load(const char* file, bool autoload)
{
    int  fd;
    char* bookmark = NULL;;

    if(autoload)
    {
        fd = open(file, O_RDONLY);
        if(fd >= 0)
        {
            if(read_line(fd, global_read_buffer, sizeof(global_read_buffer)) > 0)
                bookmark=global_read_buffer;
            close(fd);
        }
    }
    else
    {
        /* This is not an auto-load, so list the bookmarks */
        bookmark = select_bookmark(file, false);
    }

    if (bookmark != NULL)
    {
        return play_bookmark(bookmark);
    }

    return true;
}


static int get_bookmark_count(const char* bookmark_file_name)
{
    int read_count = 0;
    int file = open(bookmark_file_name, O_RDONLY);

    if(file < 0)
        return -1;

    /* Get the requested bookmark */
    while(read_line(file, global_read_buffer, sizeof(global_read_buffer)) > 0)
    {
        read_count++;
    }
    
    close(file);
    return read_count;
}

static int buffer_bookmarks(struct bookmark_list* bookmarks, int first_line)
{
    char* dest = ((char*) bookmarks) + bookmarks->buffer_size - 1;
    int read_count = 0;
    int file = open(bookmarks->filename, O_RDONLY);

    if (file < 0)
    {
        return -1;
    }

    if ((first_line != 0) && ((size_t) filesize(file) < bookmarks->buffer_size
            - sizeof(*bookmarks) - (sizeof(char*) * bookmarks->total_count)))
    {
        /* Entire file fits in buffer */
        first_line = 0;
    }
    
    bookmarks->start = first_line;
    bookmarks->count = 0;
    bookmarks->reload = false;
    
    while(read_line(file, global_read_buffer, sizeof(global_read_buffer)) > 0)
    {
        read_count++;
        
        if (read_count >= first_line)
        {
            dest -= strlen(global_read_buffer) + 1;
            
            if (dest < ((char*) bookmarks) + sizeof(*bookmarks)
                + (sizeof(char*) * (bookmarks->count + 1)))
            {
                break;
            }
            
            strcpy(dest, global_read_buffer);
            bookmarks->items[bookmarks->count] = dest;
            bookmarks->count++;
        }
    }

    close(file);
    return bookmarks->start + bookmarks->count;
}

static char* get_bookmark_info(int list_index, void* data, char *buffer)
{
    struct bookmark_list* bookmarks = (struct bookmark_list*) data;
    int     index = list_index / 2;
    int     resume_index = 0;
    long    resume_time = 0;
    bool    shuffle = false;

    if (bookmarks->show_dont_resume)
    {
        if (index == 0)
        {
            return list_index % 2 == 0 
                ? (char*) str(LANG_BOOKMARK_DONT_RESUME) : " ";
        }
        
        index--;
    }

    if (bookmarks->reload || (index >= bookmarks->start + bookmarks->count) 
        || (index < bookmarks->start))
    {
        int read_index = index;
        
        /* Using count as a guide on how far to move could possibly fail
         * sometimes. Use byte count if that is a problem?
         */
        
        if (read_index != 0)
        {
            /* Move count * 3 / 4 items in the direction the user is moving,
             * but don't go too close to the end.
             */
            int offset = bookmarks->count;
            int max = bookmarks->total_count - (bookmarks->count / 2);
            
            if (read_index < bookmarks->start)
            {
                offset *= 3;
            }
         
            read_index = index - offset / 4;

            if (read_index > max)
            {
                read_index = max;
            }
            
            if (read_index < 0)
            {
                read_index = 0;
            }
        }
        
        if (buffer_bookmarks(bookmarks, read_index) <= index)
        {
            return "";
        }
    }
    
    if (!parse_bookmark(bookmarks->items[index - bookmarks->start],
        &resume_index, NULL, NULL, NULL, NULL, 0, &resume_time, NULL, 
        &shuffle, global_filename))
    {
        return list_index % 2 == 0 ? (char*) str(LANG_BOOKMARK_INVALID) : " ";
    }

    if (list_index % 2 == 0)
    {
        char* dot = strrchr(global_filename, '.');

        if (dot)
        {
            *dot = '\0';
        }

        return global_filename;
    }
    else
    {
        char time_buf[32];

        format_time(time_buf, sizeof(time_buf), resume_time);
        snprintf(buffer, MAX_PATH, "%s, %d%s", time_buf, resume_index + 1,
            shuffle ? (char*) str(LANG_BOOKMARK_SHUFFLE) : "");
        return buffer;
    }
}

/* ----------------------------------------------------------------------- */
/* This displays a the bookmarks in a file and allows the user to          */
/* select one to play.                                                     */
/* ------------------------------------------------------------------------*/
static char* select_bookmark(const char* bookmark_file_name, bool show_dont_resume)
{
    struct bookmark_list* bookmarks;
    struct gui_synclist list;
    int last_item = -2;
    int item = 0;
    int action;
    size_t size;
    bool exit = false;
    bool refresh = true;

    bookmarks = plugin_get_buffer(&size);
    bookmarks->buffer_size = size;
    bookmarks->show_dont_resume = show_dont_resume;
    bookmarks->filename = bookmark_file_name;
    bookmarks->start = 0;
    gui_synclist_init(&list, &get_bookmark_info, (void*) bookmarks, false, 2);
    gui_synclist_set_title(&list, str(LANG_BOOKMARK_SELECT_BOOKMARK), 
        Icon_Bookmark);
    gui_syncstatusbar_draw(&statusbars, true);

    while (!exit)
    {
        gui_syncstatusbar_draw(&statusbars, false);
        
        if (refresh)
        {
            int count = get_bookmark_count(bookmark_file_name);
            bookmarks->total_count = count;

            if (bookmarks->total_count < 1)
            {
                /* No more bookmarks, delete file and exit */
                gui_syncsplash(HZ, ID2P(LANG_BOOKMARK_LOAD_EMPTY));
                remove(bookmark_file_name);
                return NULL;
            }

            if (bookmarks->show_dont_resume)
            {
                count++;
                item++;
            }

            gui_synclist_set_nb_items(&list, count * 2);

            if (item >= count)
            {
                /* Selected item has been deleted */
                item = count - 1;
                gui_synclist_select_item(&list, item * 2);
            }

            buffer_bookmarks(bookmarks, bookmarks->start);
            gui_synclist_draw(&list);
            refresh = false;
        }

        action = get_action(CONTEXT_BOOKMARKSCREEN, HZ / 2);
        gui_synclist_do_button(&list, action, LIST_WRAP_UNLESS_HELD);
        item = gui_synclist_get_sel_pos(&list) / 2;

        if (bookmarks->show_dont_resume)
        {
            item--;
        }
        
        if (item != last_item && talk_menus_enabled())
        {
            last_item = item;
            
            if (item == -1)
            {
                talk_id(LANG_BOOKMARK_DONT_RESUME, true);
            }
            else
            {
                say_bookmark(bookmarks->items[item - bookmarks->start], item);
            }
        }
        
        if (action == ACTION_STD_CONTEXT)
        {
            MENUITEM_STRINGLIST(menu_items, ID2P(LANG_BOOKMARK_CONTEXT_MENU),
                NULL, ID2P(LANG_BOOKMARK_CONTEXT_RESUME), 
                ID2P(LANG_BOOKMARK_CONTEXT_DELETE));
            static const int menu_actions[] = 
            {
                ACTION_STD_OK, ACTION_BMS_DELETE
            };
            int selection = do_menu(&menu_items, NULL);
            
            refresh = true;

            if (selection >= 0 && selection <= 
                (int) (sizeof(menu_actions) / sizeof(menu_actions[0])))
            {
                action = menu_actions[selection];
            }
        }

        switch (action)
        {
        case ACTION_STD_OK:
            if (item >= 0)
            {
                return bookmarks->items[item - bookmarks->start];
            }
            
            /* Else fall through */

        case ACTION_TREE_WPS:
        case ACTION_STD_CANCEL:
            exit = true;
            break;

        case ACTION_BMS_DELETE:
            if (item >= 0)
            {
                delete_bookmark(bookmark_file_name, item);
                bookmarks->reload = true;
                refresh = true;
                last_item = -2;
            }
            break;

        default:
            if (default_event_handler(action) == SYS_USB_CONNECTED)
            {
                exit = true;
            }

            break;
        }
    }

    return NULL;
}

/* ----------------------------------------------------------------------- */
/* This function takes a location in a bookmark file and deletes that      */
/* bookmark.                                                               */
/* ------------------------------------------------------------------------*/
static bool delete_bookmark(const char* bookmark_file_name, int bookmark_id)
{
    int temp_bookmark_file = 0;
    int bookmark_file = 0;
    int bookmark_count = 0;

    /* Opening up a temp bookmark file */
    snprintf(global_temp_buffer, sizeof(global_temp_buffer),
             "%s.tmp", bookmark_file_name);
    temp_bookmark_file = open(global_temp_buffer,
                              O_WRONLY | O_CREAT | O_TRUNC);

    if (temp_bookmark_file < 0)
        return false; /* can't open the temp file */

    /* Reading in the previous bookmarks and writing them to the temp file */
    bookmark_file = open(bookmark_file_name, O_RDONLY);
    if (bookmark_file >= 0)
    {
        while (read_line(bookmark_file, global_read_buffer,
                         sizeof(global_read_buffer)) > 0)
        {
            if (bookmark_id != bookmark_count)
            {
                write(temp_bookmark_file, global_read_buffer,
                      strlen(global_read_buffer));
                write(temp_bookmark_file, "\n", 1);
            }
            bookmark_count++;
        }
        close(bookmark_file);
    }
    close(temp_bookmark_file);

    remove(bookmark_file_name);
    rename(global_temp_buffer, bookmark_file_name);

    return true;
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark, says the voice UI part of it.          */
/* ------------------------------------------------------------------------*/
static void say_bookmark(const char* bookmark,
                         int bookmark_id)
{
    int resume_index;
    long ms;
    char dir[MAX_PATH];
    bool enqueue = false; /* only the first voice is not queued */

    if (!parse_bookmark(bookmark, &resume_index, NULL, NULL, NULL, 
        dir, sizeof(dir), &ms, NULL, NULL, NULL))
    {
        talk_id(LANG_BOOKMARK_INVALID, true);
        return;
    }

/* disabled, because transition between talkbox and voice UI clip is not nice */
#if 0 
    if (global_settings.talk_dir >= 3)
    {   /* "talkbox" enabled */
        char* last = strrchr(dir, '/');
        if (last)
        {   /* compose filename for talkbox */
            strncpy(last + 1, dir_thumbnail_name, sizeof(dir)-(last-dir)-1);
            talk_file(dir, enqueue);
            enqueue = true;
        }
    }
#endif
    talk_id(VOICE_EXT_BMARK, enqueue);
    talk_number(bookmark_id + 1, true);
    talk_id(VOICE_BOOKMARK_SELECT_INDEX_TEXT, true);
    talk_number(resume_index + 1, true);
    talk_id(LANG_TIME, true);
    if (ms / 60000)
        talk_value(ms / 60000, UNIT_MIN, true);
    talk_value((ms % 60000) / 1000, UNIT_SEC, true);
}

/* ----------------------------------------------------------------------- */
/* This function parses a bookmark and then plays it.                      */
/* ------------------------------------------------------------------------*/
static bool play_bookmark(const char* bookmark)
{
    int index;
    int offset;
    int seed;

    if (parse_bookmark(bookmark,
                       &index,
                       &offset,
                       &seed,
                       NULL,
                       global_temp_buffer,
                       sizeof(global_temp_buffer),
                       NULL,
                       &global_settings.repeat_mode,
                       &global_settings.playlist_shuffle,
                       global_filename))
    {
        bookmark_play(global_temp_buffer, index, offset, seed,
            global_filename);
        return true;
    }
    
    return false;
}

static const char* skip_token(const char* s)
{
    while (*s && *s != ';')
    {
        s++;
    }
    
    if (*s)
    {
        s++;
    }
    
    return s;
}

static const char* int_token(const char* s, int* dest)
{
    if (dest != NULL)
    {
        *dest = atoi(s);
    }
    
    return skip_token(s);
}

static const char* long_token(const char* s, long* dest)
{
    if (dest != NULL)
    {
        *dest = atoi(s);    /* Should be atol, but we don't have it. */
    }

    return skip_token(s);
}

static const char* bool_token(const char* s, bool* dest)
{
    if (dest != NULL)
    {
        *dest = atoi(s) != 0;
    }

    return skip_token(s);
}

/* ----------------------------------------------------------------------- */
/* This function takes a bookmark and parses it.  This function also       */
/* validates the bookmark.  Passing in NULL for an output variable         */
/* indicates that value is not requested.                                  */
/* ----------------------------------------------------------------------- */
static bool parse_bookmark(const char *bookmark,
                           int *resume_index,
                           int *resume_offset,
                           int *resume_seed,
                           int *resume_first_index,
                           char* resume_file,
                           unsigned int resume_file_size,
                           long* ms,
                           int * repeat_mode, bool *shuffle,
                           char* file_name)
{
    const char* s = bookmark;
    const char* end;
    
    s = int_token(s,  resume_index);
    s = int_token(s,  resume_offset);
    s = int_token(s,  resume_seed);
    s = int_token(s,  resume_first_index);
    s = long_token(s, ms);
    s = int_token(s,  repeat_mode);
    s = bool_token(s, shuffle);

    if (*s == 0)
    {
        return false;
    }
    
    end = strchr(s, ';');

    if (resume_file != NULL)
    {
        size_t len = (end == NULL) ? strlen(s) : (size_t) (end - s);

        len = MIN(resume_file_size - 1, len);
        strncpy(resume_file, s, len);
        resume_file[len] = 0;
    }

    if (end != NULL && file_name != NULL)
    {
        end++;
        strncpy(file_name, end, MAX_PATH - 1);
        file_name[MAX_PATH - 1] = 0;
    }
    
    return true;
}

/* ----------------------------------------------------------------------- */
/* This function is used by multiple functions and is used to generate a   */
/* bookmark named based off of the input.                                  */
/* Changing this function could result in how the bookmarks are stored.    */
/* it would be here that the centralized/decentralized bookmark code       */
/* could be placed.                                                        */
/* ----------------------------------------------------------------------- */
static bool generate_bookmark_file_name(const char *in)
{
    int len = strlen(in);

    /* if this is a root dir MP3, rename the bookmark file root_dir.bmark */
    /* otherwise, name it based on the in variable */
    if (!strcmp("/", in))
        strcpy(global_bookmark_file_name, "/root_dir.bmark");
    else
    {
        strcpy(global_bookmark_file_name, in);
        if(global_bookmark_file_name[len-1] == '/')
            len--;
        strcpy(&global_bookmark_file_name[len], ".bmark");
    }

    return true;
}

/* ----------------------------------------------------------------------- */
/* Returns true if a bookmark file exists for the current playlist         */
/* ----------------------------------------------------------------------- */
bool bookmark_exist(void)
{
    bool exist=false;

    if(system_check())
    {
        char* name = playlist_get_name(NULL, global_temp_buffer,
                                       sizeof(global_temp_buffer));
        if (generate_bookmark_file_name(name))
        {
            int fd=open(global_bookmark_file_name, O_RDONLY);
            if (fd >=0)
            {
                close(fd);
                exist=true;
            }
        }
    }

    return exist;
}

/* ----------------------------------------------------------------------- */
/* Checks the current state of the system and returns if it is in a        */
/* bookmarkable state.                                                     */
/* ----------------------------------------------------------------------- */
/* Inputs:                                                                 */
/* ----------------------------------------------------------------------- */
/* Outputs:                                                                */
/* return bool:  Indicates if the system was in a bookmarkable state       */
/* ----------------------------------------------------------------------- */
static bool system_check(void)
{
    int resume_index = 0;
    struct mp3entry *id3 = audio_current_track();

    if (!id3)
    {
        /* no track playing */
        return false; 
    }

    /* Checking to see if playing a queued track */
    if (playlist_get_resume_info(&resume_index) == -1)
    {
        /* something bad happened while getting the queue information */
        return false;
    }
    else if (playlist_modified(NULL))
    {
        /* can't bookmark while in the queue */
        return false;
    }

    return true;
}

