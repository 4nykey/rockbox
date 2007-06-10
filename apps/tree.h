/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _TREE_H_
#define _TREE_H_

#include <stdbool.h>
#include <applimits.h>
#include <file.h>

struct entry {
    short attr; /* FAT attributes + file type flags */
    unsigned long time_write; /* Last write time */
    char *name;
};


/* browser context for file or db */
struct tree_context {
    /* The directory we are browsing */
    char currdir[MAX_PATH];
    /* the number of directories we have crossed from / */
    int dirlevel;
    /* The currently selected file/id3dbitem index (old dircursor+dirfile) */
    int selected_item;
    /* The selected item in each directory crossed
     * (used when we want to return back to a previouws directory)*/
    int selected_item_history[MAX_DIR_LEVELS];

    int firstpos; /* which dir entry is on first
                     position in dir buffer */
    int pos_history[MAX_DIR_LEVELS];

    int *dirfilter; /* file use */
    int filesindir; /* The number of files in the dircache */
    int dirsindir; /* file use */
    int dirlength; /* total number of entries in dir, incl. those not loaded */
#ifdef HAVE_TAGCACHE
    int table_history[MAX_DIR_LEVELS]; /* db use */
    int extra_history[MAX_DIR_LEVELS]; /* db use */
    int currtable; /* db use */
    int currextra; /* db use */
#endif
    /* A big buffer with plenty of entry structs,
     * contains all files and dirs in the current
     * dir (with filters applied) */
    void* dircache;
    int dircache_size;
    char* name_buffer;
    int name_buffer_size;
    int dentry_size;
    bool dirfull;
};

void tree_mem_init(void);
void tree_gui_init(void);
void get_current_file(char* buffer, int buffer_len);
int rockbox_browse(const char *root, int dirfilter);
bool create_playlist(void);
void resume_directory(const char *dir);
char *getcwd(char *buf, int size);
void reload_directory(void);
bool check_rockboxdir(void);
struct tree_context* tree_get_context(void);
void tree_flush(void);
void tree_restore(void);


extern struct gui_synclist tree_lists;
extern struct gui_syncstatusbar statusbars;
#endif
