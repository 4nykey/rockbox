/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdbool.h>
#include "config.h"

#if defined(CPU_PP)
 /* Portalplayer chips use a microsecond timer. */
 #define TIMER_FREQ 1000000
#elif CONFIG_CPU == S3C2440 || CONFIG_CPU == DM320 || CONFIG_CPU == TCC7801 \
      || defined(CPU_TCC77X) || CONFIG_CPU == AS3525 || CONFIG_CPU == IMX31L \
      || CONFIG_CPU == JZ4732 || CONFIG_CPU == PNX0101 \
      || defined(CPU_COLDFIRE)
 #include "timer-target.h"
#elif defined(SIMULATOR)
 #define TIMER_FREQ 1000000
#else
 #define TIMER_FREQ CPU_FREQ
#endif
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, int int_prio, void (*timer_callback)(void)
                    IF_COP(,int core));
bool timer_set_period(long cycles);
#ifdef CPU_COLDFIRE
void timers_adjust_prescale(int multiplier, bool enable_irq);
#endif
void timer_unregister(void);

/* For target-specific interface use */
extern void (*pfn_timer)(void);
extern void (*pfn_unregister)(void);

#endif /* __TIMER_H__ */
