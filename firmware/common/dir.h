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
#ifndef _DIR_H_
#define _DIR_H_

#ifndef DIRENT_DEFINED

#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20

struct dirent {
    unsigned char d_name[256];
    int attribute;
    int size;
};
#endif


#ifndef SIMULATOR

#include "fat.h"

typedef struct {
    int startcluster;
    struct fat_dir fatdir;
} DIR;

#else // SIMULATOR

#ifdef WIN32
#include <io.h>
typedef struct DIRtag
{
    struct dirent   fd;
    intptr_t        handle;
} DIR;
#endif //   WIN32
#endif // SIMULATOR

#ifndef DIRFUNCTIONS_DEFINED

extern DIR* opendir(char* name);
extern int closedir(DIR* dir);

extern struct dirent* readdir(DIR* dir);

#endif /* DIRFUNCTIONS_DEFINED */

#endif
