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

#include <sys/stat.h>
#include <dirent.h>

#define DIRFUNCTIONS_DEFINED /* prevent those prototypes */
#define dirent x11_dirent
#include "../../firmware/common/dir.h"
#undef dirent

#define SIMULATOR_ARCHOS_ROOT "archos"

struct mydir {
  DIR *dir;
  char *name;
};

typedef struct mydir MYDIR;

MYDIR *x11_opendir(char *name)
{
  char buffer[256]; /* sufficiently big */
  DIR *dir;

  if(name[0] == '/') {
    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);    
    dir=(DIR *)opendir(buffer);
  }
  else
    dir=(DIR *)opendir(name);

  if(dir) {
    MYDIR *my = (MYDIR *)malloc(sizeof(MYDIR));
    my->dir = dir;
    my->name = (char *)strdup(name);

    return my;
  }
  /* failed open, return NULL */
  return (MYDIR *)0;
}

struct x11_dirent *x11_readdir(MYDIR *dir)
{
  char buffer[512]; /* sufficiently big */
  static struct x11_dirent secret;
  struct stat s;
  struct dirent *x11 = (readdir)(dir->dir);

  if(!x11)
    return (struct x11_dirent *)0;

  strcpy(secret.d_name, x11->d_name);

  /* build file name */
  sprintf(buffer, SIMULATOR_ARCHOS_ROOT "%s/%s",
          dir->name, x11->d_name);
  stat(buffer, &s); /* get info */

  secret.attribute = S_ISDIR(s.st_mode)?ATTR_DIRECTORY:0;
  secret.size = s.st_size;

  return &secret;
}

void x11_closedir(MYDIR *dir)
{
  free(dir->name);
  (closedir)(dir->dir);

  free(dir);
}


int x11_open(char *name, int opts)
{
  char buffer[256]; /* sufficiently big */

  if(name[0] == '/') {
    sprintf(buffer, "%s%s", SIMULATOR_ARCHOS_ROOT, name);

    debugf("We open the real file '%s'\n", buffer);
    return open(buffer, opts);
  }
  return open(name, opts);
}
