/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rafaël Carré
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
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"
#include "panic.h"

#include "clock-target.h" /* CPUFREQ_* are defined here */

/* We can use a interrupt-based mechanism on the fuzev2 */
#define INCREASED_SCROLLWHEEL_POLLING \
    (defined(HAVE_SCROLLWHEEL) && (CONFIG_CPU == AS3525))

#if INCREASED_SCROLLWHEEL_POLLING
/* let the timer interrupt twice as often for the scrollwheel polling */
#define KERNEL_TIMER_FREQ (TIMER_FREQ/2)
#else
#define KERNEL_TIMER_FREQ TIMER_FREQ
#endif

#define AS3525_UNCACHED_ADDR(a) ((typeof(a)) ((uintptr_t)(a) + 0x10000000))

#ifdef SANSA_C200V2
/* 0: Backlight on A5, 1: Backlight on A7 */
extern int c200v2_variant;
/* c200v2 changes the timer interval often due to software pwm */
#define TIMER_PERIOD TIMER2_BGLOAD
#else
#define TIMER_PERIOD (KERNEL_TIMER_FREQ/HZ)
#endif

/*
 * This function is not overly accurate, so rather call it with an usec more
 * than less (see below comment)
 * 
 * if inlined it expands to a really small and fast function if it's called
 * with compile time constants */
static inline void udelay(unsigned usecs) __attribute__((always_inline));
static inline void udelay(unsigned usecs)
{
    unsigned now;
    int end;
    
    /**
     * we're limited to 0.666us multiplies due to the odd timer frequency (1.5MHz),
     * to avoid calculating which is safer (need to round up for small values)
     * and saves spending time in the divider we have a lut for
     * small us values, it should be roughly us*3/2
     **/
    static const unsigned char udelay_lut[] =
    {
         0,  2,  3,  5,  6,  8,  9, 11, 12, 14,
        15, 17, 18, 20, 21, 23, 24, 26, 27, 29,
    };


    now = TIMER2_VALUE;
    /* we don't want to handle multiple overflows, so limit the numbers
     * (if you want to wait more than a tick just poll current_tick, or
     * call sleep()) */
    if (UNLIKELY(usecs >= (TIMER_PERIOD*2/3)))
        panicf("%s(): %d too high!", __func__, usecs);
    if (UNLIKELY(usecs <= 0))
        return;
    if (usecs < ARRAYLEN(udelay_lut))
    {   /* the timer decrements */
        end = now - udelay_lut[usecs];
    }
    else
    {   /* to usecs */
        int delay = usecs * 3 / 2; /* us * 0.666 = us*timer_period */
        end = now - delay;
    }

    unsigned old;

    /* underrun ? */
    if (end < 0)
    {
        do {
            old = now;
            now = TIMER2_VALUE;
        } while(now <= old);  /* if the new value is higher then we wrapped */

        end += TIMER_PERIOD;
    }

    do {
        /* if timer wraps then we missed our end value */
        old = now;
        now = TIMER2_VALUE;
    } while(now > (unsigned)end && now <= old);
}
#endif /* SYSTEM_TARGET_H */
