/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

extern void debug_init(void);
extern void debugf(char* fmt,...);
#ifndef SIMULATOR
extern void dbg_ports(void);
#endif

#ifdef __GNUC__

/*  */
#if defined(DEBUG) || defined(SIMULATOR)
#define DEBUGF(...) debugf(__VA_ARGS__)
#else
#define DEBUGF(...)
#endif

#else

#define DEBUGF debugf

#endif /* GCC */


#endif
