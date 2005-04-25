/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Bj�rn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "file.h"
#include "screens.h"
#include "kernel.h"
#include "tree.h"
#include "lcd.h"
#include "font.h"
#include "settings.h"
#include "icons.h"
#include "status.h"
#include "debug.h"
#include "button.h"
#include "menu.h"
#include "main_menu.h"
#include "mpeg.h"
#include "misc.h"
#include "ata.h"
#include "wps.h"
#include "filetypes.h"
#include "applimits.h"
#include "dbtree.h"
#include "icons.h"
#include "lang.h"
#include "keyboard.h"

/* workaround for cygwin not defining endian macros */
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN) && defined(_X86_)
#define LITTLE_ENDIAN
#endif

#ifdef LITTLE_ENDIAN
#define BE32(_x_) (((_x_ & 0xff000000) >> 24) | \
                   ((_x_ & 0x00ff0000) >> 8) | \
                   ((_x_ & 0x0000ff00) << 8) | \
                   ((_x_ & 0x000000ff) << 24))
#else
#define BE32(_x_) _x_
#endif

#define SONGENTRY_SIZE    (songlen+12+genrelen+4)
#define FILEENTRY_SIZE    (filelen+12)
#define ALBUMENTRY_SIZE   (albumlen+4+songarraylen*4)
#define ARTISTENTRY_SIZE  (artistlen+albumarraylen*4)

#define ID3DB_VERSION 2

static int fd;

static int
    songstart, albumstart, artiststart, filestart,
    songcount, albumcount, artistcount, filecount,
    songlen, songarraylen, genrelen, filelen,
    albumlen, albumarraylen,
    artistlen, rundbdirty,initialized = 0;

static int db_play_folder(struct tree_context* c);
static int db_search(struct tree_context* c, char* string);

static char searchstring[32];

int db_init(void)
{
    unsigned int version;
    unsigned int buf[17];
    unsigned char* ptr = (char*)buf;
    
    fd = open(ROCKBOX_DIR "/rockbox.id3db", O_RDONLY);
    if (fd < 0) {
        DEBUGF("Failed opening database\n");
        return -1;
    }
    read(fd, buf, 68);

    if (ptr[0] != 'R' ||
        ptr[1] != 'D' ||
        ptr[2] != 'B')
    {
        splash(HZ,true,"Not a rockbox ID3 database!");
        return -1;
    }
    
    version = BE32(buf[0])&0xFF;
    if (version != ID3DB_VERSION)
    {
        splash(HZ,true,"Unsupported database version %d!", version);
        return -1;
    }

    artiststart = BE32(buf[1]);
    albumstart = BE32(buf[2]);
    songstart = BE32(buf[3]);
    filestart = BE32(buf[4]);
    
    artistcount = BE32(buf[5]);
    albumcount = BE32(buf[6]);
    songcount = BE32(buf[7]);
    filecount = BE32(buf[8]);
    
    artistlen = BE32(buf[9]);
    albumlen   = BE32(buf[10]);
    songlen = BE32(buf[11]);
    genrelen = BE32(buf[12]);
    filelen = BE32(buf[13]);
    songarraylen = BE32(buf[14]);
    albumarraylen = BE32(buf[15]);
    rundbdirty = BE32(buf[16]);

    if (songstart > filestart ||
        albumstart > songstart ||
        artiststart > albumstart)
    {
        splash(HZ,true,"Corrupt ID3 database!");
        return -1;
    }

    initialized = 1;
    return 0;
}

int db_load(struct tree_context* c)
{
    int i, offset, rc;
    int dcachesize = global_settings.max_files_in_dir * sizeof(struct entry);
    int itemcount, stringlen, hits=0;
    unsigned long* nptr = (void*) c->name_buffer;
    unsigned long* dptr = c->dircache;
    unsigned long* safeplace = NULL;
    int safeplacelen = 0;

    int table = c->currtable;
    int extra = c->currextra;

    char* end_of_nbuf = c->name_buffer + c->name_buffer_size;

    if (!initialized) {
        DEBUGF("ID3 database is not initialized.\n");
        c->filesindir = 0;
        return 0;
    }
    
    c->dentry_size = 2;
    c->dirfull = false;

    DEBUGF("db_load() table: %d extra: 0x%x  firstpos: %d\n", table, extra, c->firstpos);

    if (!table) {
        table = root;
        c->currtable = table;
    }
    
    switch (table) {
        case root: {
            static const int tables[] = {allartists, allalbums, allsongs,
                                         search };
            char* nbuf = (char*)nptr;
            char* labels[] = { str(LANG_ID3DB_ARTISTS),
                               str(LANG_ID3DB_ALBUMS),
                               str(LANG_ID3DB_SONGS),
                               str(LANG_ID3DB_SEARCH)};
            DEBUGF("dbload table root\n");
            for (i=0; i < 4; i++) {
                strcpy(nbuf, labels[i]);
                dptr[0] = (unsigned long)nbuf;
                dptr[1] = tables[i];
                nbuf += strlen(nbuf) + 1;
                dptr += 2;
            }
            c->dirlength = c->filesindir = i;
            return i;
        }

        case search: {
            static const int tables[] = {searchartists,
                                         searchalbums,
                                         searchsongs};
            char* nbuf = (char*)nptr;
            char* labels[] = { str(LANG_ID3DB_SEARCH_ARTISTS),
                               str(LANG_ID3DB_SEARCH_ALBUMS),
                               str(LANG_ID3DB_SEARCH_SONGS)};
            DEBUGF("dbload table search\n");
            for (i=0; i < 3; i++) {
                strcpy(nbuf, labels[i]);
                dptr[0] = (unsigned long)nbuf;
                dptr[1] = tables[i];
                nbuf += strlen(nbuf) + 1;
                dptr += 2;
            }
            c->dirlength = c->filesindir = i;
            return i;
        }

        case searchartists:
        case searchalbums:
        case searchsongs:
            DEBUGF("dbload table searchsongs/searchartists/searchalbums\n");
            i = db_search(c, searchstring);
            c->dirlength = c->filesindir = i;
            if (c->dirfull) {
                splash(HZ, true, "%s %s",
                       str(LANG_SHOWDIR_ERROR_BUFFER),
                       str(LANG_SHOWDIR_ERROR_FULL));
                c->dirfull = false;
            }
            else
                splash(HZ, true, str(LANG_ID3DB_MATCHES), i);
            return i;

        case allsongs:
            DEBUGF("dbload table allsongs\n");		
            offset = songstart + c->firstpos * SONGENTRY_SIZE;
            itemcount = songcount;
            stringlen = songlen;
            break;

        case allalbums:
            DEBUGF("dbload table allalbums\n");		
            offset = albumstart + c->firstpos * ALBUMENTRY_SIZE;
            itemcount = albumcount;
            stringlen = albumlen;
            break;

        case allartists:
            DEBUGF("dbload table allartists\n");		
            offset = artiststart + c->firstpos * ARTISTENTRY_SIZE;
            itemcount = artistcount;
            stringlen = artistlen;
            break;

        case albums4artist:
            DEBUGF("dbload table albums4artist\n");		
            /* 'extra' is offset to the artist */
            safeplacelen = albumarraylen * 4;
            safeplace = (void*)(end_of_nbuf - safeplacelen);
            lseek(fd, extra + artistlen, SEEK_SET);
            rc = read(fd, safeplace, safeplacelen);
            if (rc < safeplacelen)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<albumarraylen; i++)
                safeplace[i] = BE32(safeplace[i]);
#endif
            offset = safeplace[0];
            itemcount = albumarraylen;
            stringlen = albumlen;
            break;

        case songs4album:
            DEBUGF("dbload table songs4album\n");
            /* 'extra' is offset to the album */
            safeplacelen = songarraylen * 4;
            safeplace = (void*)(end_of_nbuf - safeplacelen);
            lseek(fd, extra + albumlen + 4, SEEK_SET);
            rc = read(fd, safeplace, safeplacelen);
            if (rc < safeplacelen)
                return -1;

#ifdef LITTLE_ENDIAN
            for (i=0; i<songarraylen; i++) {
                safeplace[i] = BE32(safeplace[i]);
                DEBUGF("db_load songs4album song %d: 0x%x\n",i,safeplace[i]);
            }
#endif
            offset = safeplace[0];
            itemcount = songarraylen;
            stringlen = songlen;
            break;

        case songs4artist:
            DEBUGF("dbload table songs4artist\n");		
            /* 'extra' is offset to the artist, used as filter */
            offset = songstart + c->firstpos * SONGENTRY_SIZE;
            itemcount = songcount;
            stringlen = songlen;
            break;
            
        default:
            DEBUGF("Unsupported table %d\n", table);
            return -1;
    }
    end_of_nbuf -= safeplacelen;

    c->dirlength = itemcount;
    itemcount -= c->firstpos;

    if (!safeplace)
        lseek(fd, offset, SEEK_SET);

    /* name_buffer (nptr) contains only names, null terminated.
       the first word of dcache (dptr) is a pointer to the name,
       the rest is table specific. see below. */

    for ( i=0; i < itemcount; i++ ) {
        int rc, skip=0;
        int intbuf[4];

        if (safeplace) {
            if (!safeplace[i]) {
                c->dirlength = i;
                break;
            }
            lseek(fd, safeplace[i], SEEK_SET);
            offset = safeplace[i];
        }

        /* read name */
        rc = read(fd, nptr, stringlen);
        if (rc < stringlen)
        {
            DEBUGF("%d read(%d) returned %d\n", i, stringlen, rc);
            return -1;
        }

        switch (table) {
            case allsongs:
            case songs4album:
            case songs4artist:
                rc = read(fd, intbuf, 12);
		skip = SONGENTRY_SIZE-stringlen-12; /* skip the rest of the song info */
                if (rc < 12) {
                    DEBUGF("%d read(%d) returned %d\n", i, 12, rc);
                    return -1;
                }
                /* continue to next song if wrong artist */
                if (table == songs4artist && (int)BE32(intbuf[0]) != extra) {
 		    lseek(fd, skip, SEEK_CUR);
                    continue;
		}

                /* save offset of filename */
                dptr[1] = BE32(intbuf[2]);
                break;

            case allalbums:
            case albums4artist:
                /* save offset of this album */
                skip = songarraylen * 4 + 4;
                dptr[1] = offset;
                break;

            case allartists:
                /* save offset of this artist */
                skip = albumarraylen * 4;
                dptr[1] = offset;
                break;
        }

        /* store name pointer in dir cache */
        dptr[0] = (unsigned long)nptr;

        if (skip)
            lseek(fd, skip, SEEK_CUR);

        hits++;
        
        /* next name is stored immediately after this */
        nptr = (void*)nptr + strlen((char*)nptr) + 1;
        if ((void*)nptr + stringlen > (void*)end_of_nbuf) {
            c->dirfull = true;
            break;
        }

        /* limit dir buffer */
        dptr = (void*)dptr + c->dentry_size * sizeof(int);
        if ((void*)(dptr + c->dentry_size) >
            (void*)(c->dircache + dcachesize))
        {
            c->dirfull = true;
            break;
        }

        if (!safeplace)
            offset += stringlen + skip;
    }

    if (c->currtable == albums4artist && !c->dirfull) {
        strcpy((char*)nptr, str(LANG_ID3DB_ALL_SONGS));
        dptr[0] = (unsigned long)nptr;
        dptr[1] = extra; /* offset to artist */
        hits++;
    }
    
    c->filesindir = hits;

    return hits;
}

static int db_search(struct tree_context* c, char* string)
{
    int i, count, size, hits=0;
    long start;

    char* nptr = c->name_buffer;
    const char* end_of_nbuf = nptr + c->name_buffer_size;

    unsigned long* dptr = c->dircache;
    const long dcachesize = global_settings.max_files_in_dir *
        sizeof(struct entry);

    switch (c->currtable) {
        case searchartists:
            start = artiststart;
            count = artistcount;
            size = ARTISTENTRY_SIZE;
            break;

        case searchalbums:
            start = albumstart;
            count = albumcount;
            size = ALBUMENTRY_SIZE;
            break;

        case searchsongs:
            start = songstart;
            count = songcount;
            size = SONGENTRY_SIZE;
            break;
            
        default:
            DEBUGF("Invalid table %d\n", c->currtable);
            return 0;
    }

    lseek(fd, start, SEEK_SET);

    for (i=0; i<count; i++) {
        if (read(fd, nptr, size) < size) {
            DEBUGF("Short read(%d) in db_search()\n",size);
            break;
        }
        if (strcasestr(nptr, string)) {
            hits++;

            dptr[0] = (unsigned long)nptr;
            if (c->currtable == searchsongs) {
                /* store offset of filename */
                dptr[1] = BE32(*((long*)(nptr + songlen + 8)));
            }
            else
                /* store offset of database record */
                dptr[1] = start + i * size;

            dptr += 2;

            /* limit dir buffer */
            if ((void*)(dptr + c->dentry_size) >
                (void*)(c->dircache + dcachesize))
            {
                c->dirfull = true;
                break;
            }
    
            nptr += strlen(nptr) + 1;
            while ((unsigned long)nptr & 3)
                nptr++;

            /* limit name buffer */
            if ((void*)nptr + size > (void*)end_of_nbuf) {
                c->dirfull = true;
                break;
            }
        }
    }

    return hits;
}

int db_enter(struct tree_context* c)
{
    int rc = 0;
    int offset = (c->dircursor + c->dirstart) * c->dentry_size + 1;
    int newextra = ((int*)c->dircache)[offset];

    if (c->dirlevel >= MAX_DIR_LEVELS)
        return 0;
    
    c->dirpos[c->dirlevel] = c->dirstart;
    c->cursorpos[c->dirlevel] = c->dircursor;
    c->table_history[c->dirlevel] = c->currtable;
    c->extra_history[c->dirlevel] = c->currextra;
    c->pos_history[c->dirlevel] = c->firstpos;
    c->dirlevel++;
    
    switch (c->currtable) {
        case root:
            c->currtable = newextra;
            c->currextra = newextra;
            break;
            
        case allartists:
        case searchartists:
            c->currtable = albums4artist;
            c->currextra = newextra;
            break;

        case allalbums:
        case albums4artist:
        case searchalbums:
            /* virtual <all albums> entry points to the artist,
               all normal entries point to the album */
            if (newextra < albumstart)
                c->currtable = songs4artist;
            else
                c->currtable = songs4album;

            c->currextra = newextra;
            break;

        case allsongs:
        case songs4album:
        case songs4artist:
        case searchsongs:
            c->dirlevel--;
            if (db_play_folder(c) >= 0)
                rc = 3;
            break;

        case search:
            rc = kbd_input(searchstring, sizeof(searchstring));
            if (rc == -1 || !searchstring[0])
                c->dirlevel--;
            else
                c->currtable = newextra;
            break;
            
        default:
            c->dirlevel--;
            break;
    }

    c->dirstart = c->dircursor = c->firstpos = 0;

    return rc;
}

void db_exit(struct tree_context* c)
{
    c->dirlevel--;
    c->dirstart = c->dirpos[c->dirlevel];
    c->dircursor = c->cursorpos[c->dirlevel];
    c->currtable = c->table_history[c->dirlevel];
    c->currextra = c->extra_history[c->dirlevel];
    c->firstpos  = c->pos_history[c->dirlevel];
}

static int db_play_folder(struct tree_context* c)
{
    char buf[MAX_PATH];
    int rc, i;
    int filenum = c->dircursor + c->dirstart;

    if (playlist_create(NULL, NULL) < 0) {
        DEBUGF("Failed creating playlist\n");
        return -1;
    }

    /* TODO: add support for very long tables */
    
    for (i=0; i < c->filesindir; i++) {
        int pathoffset = ((int*)c->dircache)[i * c->dentry_size + 1];
        lseek(fd, pathoffset, SEEK_SET);
        rc = read(fd, buf, sizeof(buf));
        if (rc < songlen) {
            DEBUGF("short path read(%ld) = %d\n", sizeof(buf), rc);
            return -2;
        }

        playlist_insert_track(NULL, buf, PLAYLIST_INSERT, false);
    }

    if (global_settings.playlist_shuffle)
        filenum = playlist_shuffle(current_tick, filenum);
    if (!global_settings.play_selected)
        filenum = 0;

    playlist_start(filenum,0);

    return 0;
}

#ifdef HAVE_LCD_BITMAP
const char* db_get_icon(struct tree_context* c)
#else
int   db_get_icon(struct tree_context* c)
#endif
{
    int icon;

    switch (c->currtable)
    {
        case allsongs:
        case songs4album:
        case songs4artist:
        case searchsongs:
            icon = File;
            break;

        default:
            icon = Folder;
            break;
    }

#ifdef HAVE_LCD_BITMAP
    return bitmap_icons_6x8[icon];
#else
    return icon;
#endif
}
