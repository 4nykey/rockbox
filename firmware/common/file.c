/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Bj�rn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "file.h"
#include "fat.h"
#include "dir.h"
#include "debug.h"

/*
  These functions provide a roughly POSIX-compatible file IO API.

  Since the fat32 driver only manages sectors, we maintain a one-sector
  cache for each open file. This way we can provide byte access without
  having to re-read the sector each time. 
  The penalty is the RAM used for the cache and slightly more complex code.
*/

#define MAX_OPEN_FILES 4

struct filedesc {
    unsigned char cache[SECTOR_SIZE];
    int cacheoffset;
    int fileoffset;
    int size;
    int attr;
    struct fat_file fatfile;
    bool busy;
    bool write;
    bool dirty;
    bool trunc;
};

static struct filedesc openfiles[MAX_OPEN_FILES];

static int flush_cache(int fd);

int creat(const char *pathname, int mode)
{
    (void)mode;
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC);
}

int open(const char* pathname, int flags)
{
    DIR* dir;
    struct dirent* entry;
    int fd;
    char* name;
    struct filedesc* file = NULL;
    int rc;

    LDEBUGF("open(\"%s\",%d)\n",pathname,flags);

    if ( pathname[0] != '/' ) {
        DEBUGF("'%s' is not an absolute path.\n",pathname);
        DEBUGF("Only absolute pathnames supported at the moment\n");
        errno = EINVAL;
        return -1;
    }

    /* find a free file descriptor */
    for ( fd=0; fd<MAX_OPEN_FILES; fd++ )
        if ( !openfiles[fd].busy )
            break;

    if ( fd == MAX_OPEN_FILES ) {
        DEBUGF("Too many files open\n");
        errno = EMFILE;
        return -2;
    }

    file = &openfiles[fd];
    memset(file, 0, sizeof(struct filedesc));

    if (flags & (O_RDWR | O_WRONLY)) {
        file->write = true;
        
        if (flags & O_TRUNC)
            file->trunc = true;
    }
    file->busy = true;

    /* locate filename */
    name=strrchr(pathname+1,'/');
    if ( name ) {
        *name = 0;
        dir = opendir((char*)pathname);
        *name = '/';
        name++;
    }
    else {
        dir = opendir("/");
        name = (char*)pathname+1;
    }
    if (!dir) {
        DEBUGF("Failed opening dir\n");
        errno = EIO;
        file->busy = false;
        return -4;
    }

    /* scan dir for name */
    while ((entry = readdir(dir))) {
        if ( !strcasecmp(name, entry->d_name) ) {
            fat_open(entry->startcluster,
                     &(file->fatfile),
                     &(dir->fatdir));
            file->size = entry->size;
            file->attr = entry->attribute;
            break;
        }
    }

    if ( !entry ) {
        LDEBUGF("Didn't find file %s\n",name);
        if ( file->write && (flags & O_CREAT) ) {
            rc = fat_create_file(name,
                                 &(file->fatfile),
                                 &(dir->fatdir));
            if (rc < 0) {
                DEBUGF("Couldn't create %s in %s\n",name,pathname);
                errno = EIO;
                file->busy = false;
                closedir(dir);
                return rc * 10 - 5;
            }
            file->size = 0;
            file->attr = 0;
        }
        else {
            DEBUGF("Couldn't find %s in %s\n",name,pathname);
            errno = ENOENT;
            file->busy = false;
            closedir(dir);
            return -6;
        }
    }
    closedir(dir);

    file->cacheoffset = -1;
    file->fileoffset = 0;

    if (file->write && (flags & O_APPEND)) {
        rc = lseek(fd,0,SEEK_END);
        if (rc < 0 )
            return rc * 10 - 7;
    }

    return fd;
}

int close(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    LDEBUGF("close(%d)\n", fd);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        rc = flush(fd);
        if (rc < 0)
            return rc * 10 - 3;
    }

    file->busy = false;
    return 0;
}

int flush(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    LDEBUGF("flush(%d)\n", fd);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        /* flush sector cache */
        if ( file->dirty ) {
            rc = flush_cache(fd);
            if (rc < 0)
                return rc * 10 - 3;
        }

        /* truncate? */
        if (file->trunc) {
            rc = ftruncate(fd, file->fileoffset);
            if (rc < 0)
                return rc * 10 - 4;
        }

        /* tie up all loose ends */
        rc = fat_closewrite(&(file->fatfile), file->size, file->attr);
        if (rc < 0)
            return rc * 10 - 5;
    }
    return 0;
}

int remove(const char* name)
{
    int rc;
    struct filedesc* file;
    int fd = open(name, O_WRONLY);
    if ( fd < 0 )
        return fd * 10 - 1;

    file = &openfiles[fd];
    rc = fat_truncate(&(file->fatfile));
    if ( rc < 0 ) {
        DEBUGF("Failed truncating file: %d\n", rc);
        errno = EIO;
        return rc * 10 - 2;
    }

    rc = fat_remove(&(file->fatfile));
    if ( rc < 0 ) {
        DEBUGF("Failed removing file: %d\n", rc);
        errno = EIO;
        return rc * 10 - 3;
    }

    file->size = 0;

    rc = close(fd);
    if (rc<0)
        return rc * 10 - 4;

    return 0;
}

int rename(const char* path, const char* newpath)
{
    int rc, fd;
    char* nameptr;
    struct filedesc* file;

    /* verify new path does not already exist */
    fd = open(newpath, O_RDONLY);
    if ( fd >= 0 ) {
        close(fd);
        errno = EBUSY;
        return -1;
    }
    close(fd);

    fd = open(path, O_RDONLY);
    if ( fd < 0 ) {
        errno = EIO;
        return fd * 10 - 2;
    }

    /* strip path */
    nameptr = strrchr(newpath,'/');
    if (nameptr)
        nameptr++;
    else
        nameptr = (char*)newpath;

    file = &openfiles[fd];
    rc = fat_rename(&file->fatfile, nameptr, file->size, file->attr);
    if ( rc < 0 ) {
        DEBUGF("Failed renaming file: %d\n", rc);
        errno = EIO;
        return rc * 10 - 3;
    }

    rc = close(fd);
    if (rc<0) {
        errno = EIO;
        return rc * 10 - 4;
    }

    return 0;
}

int ftruncate(int fd, unsigned int size)
{
    int rc, sector;
    struct filedesc* file = &openfiles[fd];

    sector = size / SECTOR_SIZE;
    if (size % SECTOR_SIZE)
        sector++;

    rc = fat_seek(&(file->fatfile), sector);
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 1;
    }

    rc = fat_truncate(&(file->fatfile));
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 2;
    }

    file->size = size;

    return 0;
}

static int flush_cache(int fd)
{
    int rc;
    struct filedesc* file = &openfiles[fd];
    
    DEBUGF("Flushing dirty sector cache\n");
        
    rc = fat_readwrite(&(file->fatfile), 1,
                       file->cache, true );
    if ( rc < 0 )
        return rc * 10 - 2;

    file->dirty = false;

    return 0;
}

static int readwrite(int fd, void* buf, int count, bool write)
{
    int sectors;
    int nread=0;
    struct filedesc* file = &openfiles[fd];
    int rc;

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    LDEBUGF( "readwrite(%d,%x,%d,%s)\n",
             fd,buf,count,write?"write":"read");

    /* attempt to read past EOF? */
    if (!write && count > file->size - file->fileoffset)
        count = file->size - file->fileoffset;

    /* any head bytes? */
    if ( file->cacheoffset != -1 ) {
        int headbytes;
        int offs = file->cacheoffset;
        if ( count <= SECTOR_SIZE - file->cacheoffset ) {
            headbytes = count;
            file->cacheoffset += count;
            if ( file->cacheoffset >= SECTOR_SIZE )
                file->cacheoffset = -1;
        }
        else {
            headbytes = SECTOR_SIZE - file->cacheoffset;
            file->cacheoffset = -1;
        }

        if (write) {
            memcpy( file->cache + offs, buf, headbytes );
            if (offs+headbytes == SECTOR_SIZE) {
                int rc = flush_cache(fd);
                if ( rc < 0 ) {
                    errno = EIO;
                    return rc * 10 - 2;
                }
                file->cacheoffset = -1;
            }
            else
                file->dirty = true;
        }
        else {
            memcpy( buf, file->cache + offs, headbytes );
        }

        nread = headbytes;
        count -= headbytes;
    }

    /* if buffer has been modified, write it back to disk */
    if (count && file->dirty) {
        rc = flush_cache(fd);
        if (rc < 0)
            return nread ? nread : rc * 10 - 3;
    }

    /* read whole sectors right into the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        int rc = fat_readwrite(&(file->fatfile), sectors, 
                               buf+nread, write );
        if ( rc < 0 ) {
            DEBUGF("Failed read/writing %d sectors\n",sectors);
            errno = EIO;
            return nread ? nread : rc * 10 - 4;
        }
        else {
            if ( rc > 0 ) {
                nread += rc * SECTOR_SIZE;
                count -= sectors * SECTOR_SIZE;

                /* if eof, skip tail bytes */
                if ( rc < sectors )
                    count = 0;
            }
            else {
                /* eof */
                count=0;
            }

            file->cacheoffset = -1;
        }
    }

    /* any tail bytes? */
    if ( count ) {
        if (write) {
            if ( file->fileoffset + nread < file->size ) {
                /* sector is only partially filled. copy-back from disk */
                int rc;
                LDEBUGF("Copy-back tail cache\n");
                rc = fat_readwrite(&(file->fatfile), 1,
                                   file->cache, false );
                if ( rc < 0 ) {
                    DEBUGF("Failed writing\n");
                    errno = EIO;
                    return nread ? nread : rc * 10 - 5;
                }
                /* seek back one sector to put file position right */
                rc = fat_seek(&(file->fatfile), 
                              (file->fileoffset + nread) / 
                              SECTOR_SIZE);
                if ( rc < 0 ) {
                    DEBUGF("fat_seek() failed\n");
                    errno = EIO;
                    return nread ? nread : rc * 10 - 6;
                }
            }
            memcpy( file->cache, buf + nread, count );
            file->dirty = true;
        }
        else {
            rc = fat_readwrite(&(file->fatfile), 1, &(file->cache),false);
            if (rc < 1 ) {
                DEBUGF("Failed caching sector\n");
                errno = EIO;
                return nread ? nread : rc * 10 - 7;
            }
            memcpy( buf + nread, file->cache, count );
        }
            
        nread += count;
        file->cacheoffset = count;
    }

    file->fileoffset += nread;
    LDEBUGF("fileoffset: %d\n", file->fileoffset);

    /* adjust file size to length written */
    if ( write && file->fileoffset > file->size )
        file->size = file->fileoffset;

    return nread;
}

int write(int fd, void* buf, int count)
{
    if (!openfiles[fd].write) {
        errno = EACCES;
        return -1;
    }
    return readwrite(fd, buf, count, true);
}

int read(int fd, void* buf, int count)
{
    return readwrite(fd, buf, count, false);
}


int lseek(int fd, int offset, int whence)
{
    int pos;
    int newsector;
    int oldsector;
    int sectoroffset;
    int rc;
    struct filedesc* file = &openfiles[fd];

    LDEBUGF("lseek(%d,%d,%d)\n",fd,offset,whence);

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    switch ( whence ) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = file->fileoffset + offset;
            break;

        case SEEK_END:
            pos = file->size + offset;
            break;

        default:
            errno = EINVAL;
            return -2;
    }
    if ((pos < 0) || (pos > file->size)) {
        errno = EINVAL;
        return -3;
    }

    /* new sector? */
    newsector = pos / SECTOR_SIZE;
    oldsector = file->fileoffset / SECTOR_SIZE;
    sectoroffset = pos % SECTOR_SIZE;

    if ( (newsector != oldsector) ||
         ((file->cacheoffset==-1) && sectoroffset) ) {

        if ( newsector != oldsector ) {
            if (file->dirty) {
                rc = flush_cache(fd);
                if (rc < 0)
                    return rc * 10 - 5;
            }
            
            rc = fat_seek(&(file->fatfile), newsector);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 4;
            }
        }
        if ( sectoroffset ) {
            rc = fat_readwrite(&(file->fatfile), 1,
                               &(file->cache),false);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 6;
            }
            file->cacheoffset = sectoroffset;
        }
        else
            file->cacheoffset = -1;
    }
    else
        if ( file->cacheoffset != -1 )
            file->cacheoffset = sectoroffset;

    file->fileoffset = pos;

    return pos;
}
