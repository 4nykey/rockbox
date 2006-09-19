/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _TAGTREE_H
#define _TAGTREE_H

#include "tagcache.h"
#include "tree.h"

enum table { root = 1, navibrowse, allsubentries, playtrack };

struct tagentry {
    char *name;
    int newtable;
    int extraseek;
};

bool tagtree_export(void);
bool tagtree_import(void);
void tagtree_init(void);
int tagtree_enter(struct tree_context* c);
void tagtree_exit(struct tree_context* c);
int tagtree_load(struct tree_context* c);
struct tagentry* tagtree_get_entry(struct tree_context *c, int id);
bool tagtree_insert_selection_playlist(int position, bool queue);
char *tagtree_get_title(struct tree_context* c);
int tagtree_get_attr(struct tree_context* c);
#ifdef HAVE_LCD_BITMAP
const unsigned char* tagtree_get_icon(struct tree_context* c);
#else
int   tagtree_get_icon(struct tree_context* c);
#endif
int tagtree_get_filename(struct tree_context* c, char *buf, int buflen);

#endif
