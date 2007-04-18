/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Greg White
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "mmu-meg-fx.h"
#include "system-arm.h"

#define CPUFREQ_DEFAULT 98784000
#define CPUFREQ_NORMAL  98784000
#define CPUFREQ_MAX    296352000

#define HAVE_INVALIDATE_ICACHE
static inline void invalidate_icache(void)
{
    clean_dcache();
    asm volatile(
        "mov r0, #0 \n"
        "mcr p15, 0, r0, c7, c5, 0 \n"
        : : : "r0"
    );
}

#endif /* SYSTEM_TARGET_H */
