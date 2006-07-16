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

/** 
 * Basic structure on this file was copied from dbtree.c and modified to
 * support the tag cache interface.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "splash.h"
#include "icons.h"
#include "tree.h"
#include "settings.h"
#include "tagcache.h"
#include "tagtree.h"
#include "lang.h"
#include "logf.h"
#include "playlist.h"
#include "keyboard.h"
#include "gui/list.h"
#include "buffer.h"
#include "atoi.h"
#include "playback.h"

#define FILE_SEARCH_INSTRUCTIONS ROCKBOX_DIR "/tagnavi.config"

static int tagtree_play_folder(struct tree_context* c);

static char searchstring[32];
#define MAX_TAGS 5

/*
 * "%3d. %s" autoscore title
 * 
 * valid = true
 * formatstr = "%-3d. %s"
 * tags[0] = tag_autoscore
 * tags[1] = tag_title
 * tag_count = 2
 */
struct display_format {
    bool valid;
    char formatstr[64];
    int tags[MAX_TAGS];
    int tag_count;
};

struct search_instruction {
    char name[64];
    int tagorder[MAX_TAGS];
    int tagorder_count;
    struct tagcache_search_clause clause[MAX_TAGS][TAGCACHE_MAX_CLAUSES];
    struct display_format format[MAX_TAGS];
    int clause_count[MAX_TAGS];
    int result_seek[MAX_TAGS];
};

static struct search_instruction *si, *csi;
static int si_count = 0;
static const char *strp;

static int current_offset;
static int current_entry_count;

static int get_token_str(char *buf, int size)
{
    /* Find the start. */
    while (*strp != '"' && *strp != '\0')
        strp++;

    if (*strp == '\0' || *(++strp) == '\0')
        return -1;
    
    /* Read the data. */
    while (*strp != '"' && *strp != '\0' && --size > 0)
        *(buf++) = *(strp++);
    
    *buf = '\0';
    if (*strp != '"')
        return -2;
    
    strp++;
    
    return 0;
}

#define MATCH(tag,str1,str2,settag) \
    if (!strcasecmp(str1, str2)) { \
        *tag = settag; \
        return 1; \
    }

static int get_tag(int *tag)
{
    char buf[32];
    int i;
    
    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;
    
    if (*strp == '\0')
        return 0;
    
    for (i = 0; i < (int)sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';
    
    MATCH(tag, buf, "album", tag_album);
    MATCH(tag, buf, "artist", tag_artist);
    MATCH(tag, buf, "bitrate", tag_bitrate);
    MATCH(tag, buf, "composer", tag_composer);
    MATCH(tag, buf, "genre", tag_genre);
    MATCH(tag, buf, "length", tag_length);
    MATCH(tag, buf, "title", tag_title);
    MATCH(tag, buf, "filename", tag_filename);
    MATCH(tag, buf, "tracknum", tag_tracknumber);
    MATCH(tag, buf, "year", tag_year);
    MATCH(tag, buf, "playcount", tag_playcount);
    MATCH(tag, buf, "autoscore", tag_virt_autoscore);
    
    logf("NO MATCH: %s\n", buf);
    if (buf[0] == '?')
        return 0;
    
    return -1;
}

static int get_clause(int *condition)
{
    char buf[4];
    int i;
    
    /* Find the start. */
    while (*strp == ' ' && *strp != '\0')
        strp++;
    
    if (*strp == '\0')
        return 0;
    
    for (i = 0; i < (int)sizeof(buf)-1; i++)
    {
        if (*strp == '\0' || *strp == ' ')
            break ;
        buf[i] = *strp;
        strp++;
    }
    buf[i] = '\0';
    
    MATCH(condition, buf, "=", clause_is);
    MATCH(condition, buf, "==", clause_is);
    MATCH(condition, buf, ">", clause_gt);
    MATCH(condition, buf, ">=", clause_gteq);
    MATCH(condition, buf, "<", clause_lt);
    MATCH(condition, buf, "<=", clause_lteq);
    MATCH(condition, buf, "~", clause_contains);
    MATCH(condition, buf, "^", clause_begins_with);
    MATCH(condition, buf, "$", clause_ends_with);
    
    return 0;
}

static bool add_clause(struct search_instruction *inst, 
                       int tag, int type, const char *str)
{
    int len = strlen(str);
    struct tagcache_search_clause *clause;
    
    if (inst->clause_count[inst->tagorder_count] >= TAGCACHE_MAX_CLAUSES)
    {
        logf("Too many clauses");
        return false;
    }
    
    clause = &inst->clause[inst->tagorder_count]
        [inst->clause_count[inst->tagorder_count]];
    if (len >= (int)sizeof(clause->str) - 1)
    {
        logf("Too long str in condition");
        return false;
    }
    
    clause->tag = tag;
    clause->type = type;
    if (len == 0)
        clause->input = true;
    else
        clause->input = false;
    
    if (tagcache_is_numeric_tag(tag))
    {
        clause->numeric = true;
        clause->numeric_data = atoi(str);
    }
    else
    {
        clause->numeric = false;
        strcpy(clause->str, str);
    }
    
    inst->clause_count[inst->tagorder_count]++;
    
    return true;
}

static int get_format_str(struct display_format *fmt)
{
    int ret;
    
    memset(fmt, 0, sizeof(struct display_format));
    
    if (get_token_str(fmt->formatstr, sizeof fmt->formatstr) < 0)
        return -10;
    
    while (fmt->tag_count < MAX_TAGS)
    {
        ret = get_tag(&fmt->tags[fmt->tag_count]);
        if (ret < 0)
            return -11;
        
        if (ret == 0)
            break;
        
        fmt->tag_count++;
    }
    
    fmt->valid = true;
    
    return 1;
}

static int get_condition(struct search_instruction *inst)
{
    struct display_format format;
    struct display_format *fmt = NULL;
    int tag;
    int condition;
    char buf[32];
        
    switch (*strp)
    {
        case '=':
            if (get_format_str(&format) < 0)
            {
                logf("get_format_str() parser failed!");
                return -4;
            }
            fmt = &format;
            break;
        
        case '?':
        case ' ':
        case '&':
            strp++;
            return 1;
        case ':':
            strp++;
        case '\0':
            return 0;
    }

    if (fmt)
    {
        memcpy(&inst->format[inst->tagorder_count], fmt, 
               sizeof(struct display_format));
    }
    else
        inst->format[inst->tagorder_count].valid = false;
    
    if (get_tag(&tag) <= 0)
        return -1;
    
    if (get_clause(&condition) <= 0)
        return -2;
    
    if (get_token_str(buf, sizeof buf) < 0)
        return -3;
    
    logf("got clause: %d/%d [%s]", tag, condition, buf);
    add_clause(inst, tag, condition, buf);
    
    return 1;
}

/* example search:
 * "Best" artist ? year >= "2000" & title !^ "crap" & genre = "good genre" \
 *      : album  ? year >= "2000" : songs
 * ^  begins with
 * *  contains
 * $  ends with
 */

static bool parse_search(struct search_instruction *inst, const char *str)
{
    int ret;
    
    memset(inst, 0, sizeof(struct search_instruction));
    strp = str;
    
    if (get_token_str(inst->name, sizeof inst->name) < 0)
    {
        logf("No name found.");
        return false;
    }
    
    while (inst->tagorder_count < MAX_TAGS)
    {
        ret = get_tag(&inst->tagorder[inst->tagorder_count]);
        if (ret < 0) 
        {
            logf("Parse error #1");
            return false;
        }
        
        if (ret == 0)
            break ;
        
        logf("tag: %d", inst->tagorder[inst->tagorder_count]);
        
        while (get_condition(inst) > 0) ;

        inst->tagorder_count++;
    }
    
    return true;
}


static struct tagcache_search tcs, tcs2;

static int compare(const void *p1, const void *p2)
{
    struct tagentry *e1 = (struct tagentry *)p1;
    struct tagentry *e2 = (struct tagentry *)p2;
    
    return strncasecmp(e1->name, e2->name, MAX_PATH);
}

static void tagtree_buffer_event(struct mp3entry *id3, bool last_track)
{
    (void)id3;
    (void)last_track;
    
    logf("be:%d%s", last_track, id3->path);
}

static void tagtree_unbuffer_event(struct mp3entry *id3, bool last_track)
{
    (void)last_track;
    long playcount;
    long playtime;
    long lastplayed;
    
    /* Do not gather data unless proper setting has been enabled. */
    if (!global_settings.runtimedb)
        return;
    
    /* Don't process unplayed tracks. */
    if (id3->elapsed == 0)
        return;
    
    if (!tagcache_find_index(&tcs, id3->path))
    {
        logf("tc stat: not found: %s", id3->path);
        return;
    }
    
    playcount  = tagcache_get_numeric(&tcs, tag_playcount);
    playtime   = tagcache_get_numeric(&tcs, tag_playtime);
    lastplayed = tagcache_get_numeric(&tcs, tag_lastplayed);
    
    playcount++;
    
    /* Ignore the last 15s (crossfade etc.) */
    playtime += MIN(id3->length, id3->elapsed + 15 * 1000);
    
    logf("ube:%s", id3->path);
    logf("-> %d/%d/%d", last_track, playcount, playtime);
    logf("-> %d/%d/%d", id3->elapsed, id3->length, MIN(id3->length, id3->elapsed + 15 * 1000));
    
    /* lastplayed not yet supported. */
    
    if (!tagcache_modify_numeric_entry(&tcs, tag_playcount, playcount)
        || !tagcache_modify_numeric_entry(&tcs, tag_playtime, playtime)
        || !tagcache_modify_numeric_entry(&tcs, tag_lastplayed, tag_lastplayed))
    {
        logf("tc stat: modify failed!");
        tagcache_search_finish(&tcs);
        return;
    }
    
    tagcache_search_finish(&tcs);
}

bool tagtree_export(void)
{
    gui_syncsplash(0, true, str(LANG_WAIT));
    if (!tagcache_create_changelog(&tcs))
    {
        gui_syncsplash(HZ*2, true, str(LANG_FAILED));
    }
    
    return false;
}

void tagtree_init(void)
{
    int fd;
    char buf[256];
    int pos = 0;
    
    si_count = 0;
    
    fd = open(FILE_SEARCH_INSTRUCTIONS, O_RDONLY);
    if (fd < 0)
    {
        logf("Search instruction file not found.");
        return ;
    }
    
    si = (struct search_instruction *)(((long)audiobuf & ~0x03) + 0x04);
    
    while ( 1 )
    {
        char *p;
        char *next = NULL;
        int rc;
        
        rc = read(fd, &buf[pos], sizeof(buf)-pos-1);
        if (rc >= 0)
            buf[pos+rc] = '\0';
        
        if ( (p = strchr(buf, '\r')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        else
            p = buf;
        
        if ( (p = strchr(p, '\n')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        
        if (!parse_search(si + si_count, buf))
            break;
        si_count++;
        
        if (next)
        {
            pos = sizeof(buf) - ((long)next - (long)buf) - 1;
            memmove(buf, next, pos);
        }
        else
            break ;
    }
    close(fd);
    
    audiobuf += sizeof(struct search_instruction) * si_count + 4;
    
    audio_set_track_buffer_event(tagtree_buffer_event);
    audio_set_track_unbuffer_event(tagtree_unbuffer_event);
}

bool show_search_progress(bool init, int count)
{
    static int last_tick = 0;
    
    if (init)
    {
        last_tick = current_tick;
        return true;
    }
    
    if (current_tick - last_tick > HZ/2)
    {
        gui_syncsplash(0, true, str(LANG_PLAYLIST_SEARCH_MSG), count,
#if CONFIG_KEYPAD == PLAYER_PAD
                       str(LANG_STOP_ABORT)
#else
                       str(LANG_OFF_ABORT)
#endif
                       );
        if (SETTINGS_CANCEL == button_get(false))
            return false;
        last_tick = current_tick;
    }
    
    return true;
}

int retrieve_entries(struct tree_context *c, struct tagcache_search *tcs, 
                     int offset, bool init)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int i;
    int namebufused = 0;
    int total_count = 0;
    int special_entry_count = 0;
    int extra = c->currextra;
    int tag;
    bool sort = false;
    
    if (init
#ifdef HAVE_TC_RAMCACHE
        && !tagcache_is_ramcache()
#endif
        )
    {
        show_search_progress(true, 0);
        gui_syncsplash(0, true, str(LANG_PLAYLIST_SEARCH_MSG),
                       0, csi->name);
    }
    
    if (c->currtable == allsubentries)
    {
        tag = tag_title;
        extra--;
    }
    else
        tag = csi->tagorder[extra];

    if (!tagcache_search(tcs, tag))
        return -1;
    
    for (i = 0; i < extra; i++)
    {
        tagcache_search_add_filter(tcs, csi->tagorder[i], csi->result_seek[i]);
        sort = true;
    }
    
    for (i = 0; i < csi->clause_count[extra]; i++)
    {
        tagcache_search_add_clause(tcs, &csi->clause[extra][i]);
        sort = true;
    }
    
    current_offset = offset;
    current_entry_count = 0;
    c->dirfull = false;
    
    if (tag != tag_title && tag != tag_filename)
    {
        if (offset == 0)
        {
            dptr->newtable = allsubentries;
            dptr->name = str(LANG_TAGNAVI_ALL_TRACKS);
            dptr++;
            current_entry_count++;
        }
        special_entry_count++;
    }
    
    total_count += special_entry_count;
    
    while (tagcache_get_next(tcs))
    {
        struct display_format *fmt = &csi->format[extra];

        if (total_count++ < offset)
            continue;
        
        dptr->newtable = navibrowse;
        dptr->extraseek = tcs->result_seek;
        if (tag == tag_title || tag == tag_filename)
            dptr->newtable = playtrack;
        
        if (!tcs->ramsearch || fmt->valid)
        {
            char buf[MAX_PATH];
            int buf_pos = 0;
            
            if (fmt->valid)
            {
                char fmtbuf[8];
                bool read_format = false;
                int fmtbuf_pos = 0;
                int parpos = 0;

                memset(buf, 0, sizeof buf);
                for (i = 0; fmt->formatstr[i] != '\0'; i++)
                {
                    if (fmt->formatstr[i] == '%')
                    {
                        read_format = true;
                        fmtbuf_pos = 0;
                        if (parpos >= fmt->tag_count)
                        {
                            logf("too many format tags");
                            return 0;
                        }
                    }
                    
                    if (read_format)
                    {
                        fmtbuf[fmtbuf_pos++] = fmt->formatstr[i];
                        if (fmtbuf_pos >= (long)sizeof(fmtbuf))
                        {
                            logf("format parse error");
                            return 0;
                        }
                        
                        if (fmt->formatstr[i] == 's')
                        {
                            fmtbuf[fmtbuf_pos] = '\0';
                            read_format = false;
                            snprintf(&buf[buf_pos], MAX_PATH - buf_pos, fmtbuf, tcs->result);
                            buf_pos += strlen(&buf[buf_pos]);
                            parpos++;
                        }
                        else if (fmt->formatstr[i] == 'd')
                        {
                            fmtbuf[fmtbuf_pos] = '\0';
                            read_format = false;
                            snprintf(&buf[buf_pos], MAX_PATH - buf_pos, fmtbuf,
                                     tagcache_get_numeric(tcs, fmt->tags[parpos]));
                            buf_pos += strlen(&buf[buf_pos]);
                            parpos++;
                        }
                        continue;
                    }
                    
                    buf[buf_pos++] = fmt->formatstr[i];
                    
                    if (buf_pos - 1 >= (long)sizeof(buf))
                    {
                        logf("buffer overflow");
                        return 0;
                    }
                }
                
                buf[buf_pos++] = '\0';
            }
            
            dptr->name = &c->name_buffer[namebufused];
            if (fmt->valid)
                namebufused += buf_pos;
            else
                namebufused += tcs->result_len;
            
            if (namebufused >= c->name_buffer_size)
            {
                logf("chunk mode #2: %d", current_entry_count);
                c->dirfull = true;
                sort = false;
                break ;
            }
            if (fmt->valid)
                strcpy(dptr->name, buf);
            else
                strcpy(dptr->name, tcs->result);
        }
        else
            dptr->name = tcs->result;
        
        dptr++;
        current_entry_count++;

        if (current_entry_count >= global_settings.max_files_in_dir)
        {
            logf("chunk mode #3: %d", current_entry_count);
            c->dirfull = true;
            sort = false;
            break ;
        }
        
        if (init && !tcs->ramsearch)
        {
            if (!show_search_progress(false, i))
            {
                tagcache_search_finish(tcs);
                return current_entry_count;
            }
        }
    }
    
    if (sort)
        qsort(c->dircache + special_entry_count * c->dentry_size,
              current_entry_count - special_entry_count,
              c->dentry_size, compare);
    
    if (!init)
    {
        tagcache_search_finish(tcs);
        return current_entry_count;
    }
    
    while (tagcache_get_next(tcs))
    {
        if (!tcs->ramsearch)
        {
            if (!show_search_progress(false, total_count))
                break;
        }
        total_count++;
    }
    
    tagcache_search_finish(tcs);
    return total_count;
}

static int load_root(struct tree_context *c)
{
    struct tagentry *dptr = (struct tagentry *)c->dircache;
    int i;
    
    c->currtable = root;
    for (i = 0; i < si_count; i++)
    {
        dptr->name = (si+i)->name;
        dptr->newtable = navibrowse;
        dptr->extraseek = i;
        dptr++;
    }
    
    current_offset = 0;
    current_entry_count = i;
    
    return i;
}

int tagtree_load(struct tree_context* c)
{
    int count;
    int table = c->currtable;
    
    c->dentry_size = sizeof(struct tagentry);

    if (!table)
    {
        c->dirfull = false;
        table = root;
        c->currtable = table;
    }

    switch (table) 
    {
        case root:
            count = load_root(c);
            c->dirlevel = 0;
            break;

        case allsubentries:
        case navibrowse:
            logf("navibrowse...");
            count = retrieve_entries(c, &tcs, 0, true);
            break;
        
        default:
            logf("Unsupported table %d\n", table);
            return -1;
    }
    
    if (count < 0)
    {
        c->dirlevel = 0;
        count = load_root(c);
        gui_syncsplash(HZ, true, str(LANG_TAGCACHE_BUSY));
    }

    /* The _total_ numer of entries available. */
    c->dirlength = c->filesindir = count;
    
    return count;
}

int tagtree_enter(struct tree_context* c)
{
    int rc = 0;
    struct tagentry *dptr;
    int newextra;
    int seek;

    dptr = tagtree_get_entry(c, c->selected_item);
    
    c->dirfull = false;
    newextra = dptr->newtable;
    seek = dptr->extraseek;

    if (c->dirlevel >= MAX_DIR_LEVELS)
        return 0;

    c->selected_item_history[c->dirlevel]=c->selected_item;
    c->table_history[c->dirlevel] = c->currtable;
    c->extra_history[c->dirlevel] = c->currextra;
    c->pos_history[c->dirlevel] = c->firstpos;
    c->dirlevel++;

    switch (c->currtable) {
        case root:
            c->currextra = newextra;
        
            if (newextra == navibrowse)
            {
                int i, j;
                
                csi = si+seek;
                c->currextra = 0;
                
                /* Read input as necessary. */
                for (i = 0; i < csi->tagorder_count; i++)
                {
                    for (j = 0; j < csi->clause_count[i]; j++)
                    {
                        if (!csi->clause[i][j].input)
                            continue;
                        
                        rc = kbd_input(searchstring, sizeof(searchstring));
                        if (rc == -1 || !searchstring[0])
                        {
                            c->dirlevel--;
                            return 0;
                        }
                        
                        if (csi->clause[i][j].numeric)
                            csi->clause[i][j].numeric_data = atoi(searchstring);
                        else
                            strncpy(csi->clause[i][j].str, searchstring,
                                    sizeof(csi->clause[i][j].str)-1);
                    }
                }
            }
            c->currtable = newextra;
        
            break;

        case navibrowse:
        case allsubentries:
            if (newextra == playtrack)
            {
                c->dirlevel--;
                if (tagtree_play_folder(c) >= 0)
                    rc = 2;
                break;
            }

            c->currtable = newextra;
            csi->result_seek[c->currextra] = seek;
            if (c->currextra < csi->tagorder_count-1)
                c->currextra++;
            else
                c->dirlevel--;
            break;
        
        default:
            c->dirlevel--;
            break;
    }
    
    c->selected_item=0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    return rc;
}

void tagtree_exit(struct tree_context* c)
{
    c->dirfull = false;
    c->dirlevel--;
    c->selected_item=c->selected_item_history[c->dirlevel];
    gui_synclist_select_item(&tree_lists, c->selected_item);
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
    c->firstpos  = c->pos_history[c->dirlevel];
}

int tagtree_get_filename(struct tree_context* c, char *buf, int buflen)
{
    struct tagentry *entry;
    
    entry = tagtree_get_entry(c, c->selected_item);

    if (!tagcache_search(&tcs, tag_filename))
        return -1;

    if (!tagcache_retrieve(&tcs, entry->extraseek, buf, buflen))
    {
        tagcache_search_finish(&tcs);
        return -2;
    }

    tagcache_search_finish(&tcs);
    
    return 0;
}
   
static int tagtree_play_folder(struct tree_context* c)
{
    int i;
    char buf[MAX_PATH];

    if (playlist_create(NULL, NULL) < 0)
    {
        logf("Failed creating playlist\n");
        return -1;
    }

    cpu_boost(true);
    if (!tagcache_search(&tcs, tag_filename))
    {
        gui_syncsplash(HZ, true, str(LANG_TAGCACHE_BUSY));
        return -1;
    }
    
    for (i=0; i < c->filesindir; i++)
    {
        if (!show_search_progress(false, i))
            break;
        
        if (!tagcache_retrieve(&tcs, tagtree_get_entry(c, i)->extraseek, 
                               buf, sizeof buf))
        {
            continue;
        }

        playlist_insert_track(NULL, buf, PLAYLIST_INSERT, false);
    }
    tagcache_search_finish(&tcs);
    cpu_boost(false);
    
    if (global_settings.playlist_shuffle)
        c->selected_item = playlist_shuffle(current_tick, c->selected_item);
    if (!global_settings.play_selected)
        c->selected_item = 0;
    gui_synclist_select_item(&tree_lists, c->selected_item);

    playlist_start(c->selected_item,0);

    return 0;
}

struct tagentry* tagtree_get_entry(struct tree_context *c, int id)
{
    struct tagentry *entry = (struct tagentry *)c->dircache;
    int realid = id - current_offset;
    
    /* Load the next chunk if necessary. */
    if (realid >= current_entry_count || realid < 0)
    {
        if (retrieve_entries(c, &tcs2, MAX(0, id - (current_entry_count / 2)),
                             false) < 0)
        {
            logf("retrieve failed");
            return NULL;
        }
        realid = id - current_offset;
    }
    
    return &entry[realid];
}

int tagtree_get_attr(struct tree_context* c)
{
    int attr = -1;
    switch (c->currtable)
    {
        case navibrowse:
            if (csi->tagorder[c->currextra] == tag_title)
                attr = TREE_ATTR_MPA;
            else
                attr = ATTR_DIRECTORY;
            break;

        case allsubentries:
            attr = TREE_ATTR_MPA;
            break;
        
        default:
            attr = ATTR_DIRECTORY;
            break;
    }

    return attr;
}

#ifdef HAVE_LCD_BITMAP
const unsigned char* tagtree_get_icon(struct tree_context* c)
#else
int   tagtree_get_icon(struct tree_context* c)
#endif
{
    int icon = Icon_Folder;

    if (tagtree_get_attr(c) == TREE_ATTR_MPA)
        icon = Icon_Audio;

#ifdef HAVE_LCD_BITMAP
    return bitmap_icons_6x8[icon];
#else
    return icon;
#endif
}
