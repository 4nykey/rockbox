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

#ifndef _FILE_H_
#define _FILE_H_

#undef MAX_PATH /* this avoids problems when building simulator */
#define MAX_PATH 260

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  4
#define O_APPEND 8
#define O_TRUNC  0x10
#endif

#if !defined(__ssize_t_defined) && !defined(_SSIZE_T_) && !defined(ssize_t)
#define __ssize_t_defined
#define _SSIZE_T_
#define ssize_t ssize_t
typedef signed long ssize_t;
#endif

#if !defined(__off_t_defined) && !defined(_OFF_T_) && !defined(off_t)
#define __off_t_defined
#define _OFF_T_
#define off_t off_t
typedef signed long off_t;
#endif

#if !defined(__mode_t_defined) && !defined(_MODE_T_) && !defined(mode_t)
#define __mode_t_defined
#define _MODE_T_
#define mode_t mode_t
typedef unsigned int mode_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

typedef int (*open_func)(const char* pathname, int flags);
typedef ssize_t (*read_func)(int fd, void *buf, size_t count);
typedef int (*creat_func)(const char *pathname, mode_t mode);
typedef ssize_t (*write_func)(int fd, const void *buf, size_t count);
typedef void (*qsort_func)(void *base, size_t nmemb,  size_t size,
                           int(*_compar)(const void *, const void *));

#ifndef SIMULATOR
extern int open(const char* pathname, int flags);
extern int close(int fd);
extern int fsync(int fd);
extern ssize_t read(int fd, void *buf, size_t count);
extern off_t lseek(int fildes, off_t offset, int whence);
extern int creat(const char *pathname, mode_t mode);
extern ssize_t write(int fd, const void *buf, size_t count);
extern int remove(const char* pathname);
extern int rename(const char* path, const char* newname);
extern int ftruncate(int fd, off_t length);
extern int filesize(int fd);
#endif /* SIMULATOR */

#endif
