/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __SPRINTF_H__
#define __SPRINTF_H__

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

int snprintf (char *buf, size_t size, const char *fmt, ...);
#ifndef NOCYGWIN
int vsnprintf (char *buf, int size, const char *fmt, va_list ap);
char *strtok_r (char *, const char *, char **);
#endif

int rockbox_fprintf (int fd, const char *fmt, ...);
#define fprintf rockbox_fprintf

#endif /* __SPRINTF_H__ */
