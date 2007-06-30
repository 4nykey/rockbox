#ifndef __PP5024_H__
#define __PP5024_H__
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* We believe is this quite similar to the 5020 and for how we just use that
   completely and redifine any minor differences */
#include "pp5020.h"

#undef GPIO_IRQ
/* Ports A, B, ?? */
#define GPIO0_IRQ   (32+0)
/* Ports F, H, ?? */
#define GPIO1_IRQ   (32+1)

#undef GPIO_MASK
#define GPIO0_MASK  (1 << (GPIO0_IRQ-32))
#define GPIO1_MASK  (1 << (GPIO1_IRQ-32))

#endif
