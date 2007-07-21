/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by K�vin Ferrare
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

#include "config.h"

#ifdef HAVE_DIRCACHE
# include "dircache.h"
# define DIR DIR_CACHED
# define dirent dircache_entry
# define opendir opendir_cached
# define closedir closedir_cached
# define readdir readdir_cached
# define closedir closedir_cached
# define mkdir mkdir_cached
# define rmdir rmdir_cached
#else
#include "dir_uncached.h"
#warning no dircache
# define DIR DIR_UNCACHED
# define dirent dirent_uncached
//# define opendir opendir_uncached
# define closedir closedir_uncached
# define readdir readdir_uncached
# define closedir closedir_uncached
# define mkdir mkdir_uncached
# define rmdir rmdir_uncached
#endif

#endif
