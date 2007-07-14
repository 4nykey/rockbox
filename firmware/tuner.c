/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * General tuner functions
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "config.h"
#include "kernel.h"
#include "tuner.h"
#include "fmradio.h"

/* General region information */
const struct fm_region_data fm_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = { 87500000, 108000000,  50000 },
    [REGION_US_CANADA] = { 87900000, 107900000, 200000 },
    [REGION_JAPAN]     = { 76000000,  90000000, 100000 },
    [REGION_KOREA]     = { 87500000, 108000000, 100000 }
};

#ifndef SIMULATOR

/* Tuner-specific region information */

#if (CONFIG_TUNER & LV24020LP)
/* deemphasis setting for region */
const unsigned char lv24020lp_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = 0, /* 50uS */
    [REGION_US_CANADA] = 1, /* 75uS */
    [REGION_JAPAN]     = 0, /* 50uS */
    [REGION_KOREA]     = 0, /* 50uS */
};
#endif /* (CONFIG_TUNER & LV24020LP) */

#if (CONFIG_TUNER & TEA5767)
const struct tea5767_region_data tea5767_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = { 0, 0 }, /* 50uS, US/Europe band */
    [REGION_US_CANADA] = { 1, 0 }, /* 75uS, US/Europe band */
    [REGION_JAPAN]     = { 0, 1 }, /* 50uS, Japanese band  */
    [REGION_KOREA]     = { 0, 0 }, /* 50uS, US/Europe band */ 
};
#endif /* (CONFIG_TUNER & TEA5767) */

#ifdef CONFIG_TUNER_MULTI
int (*tuner_set)(int setting, int value);
int (*tuner_get)(int setting);
#define TUNER_TYPE_CASE(type, set, get, region_data) \
    case type:                                       \
        tuner_set = set;                             \
        tuner_get = get;                             \
        break;
#else
#define TUNER_TYPE_CASE(type, set, get, region_data)
#endif /* CONFIG_TUNER_MULTI */

void tuner_init(void)
{
#ifdef CONFIG_TUNER_MULTI
    switch (tuner_detect_type())
#endif
    {
    #if (CONFIG_TUNER & LV24020LP)
        TUNER_TYPE_CASE(LV24020LP,
                        lv24020lp_set,
                        lv24020lp_get,
                        lv24020lp_region_data)
    #endif
    #if (CONFIG_TUNER & TEA5767)
        TUNER_TYPE_CASE(TEA5767,
                        tea5767_set,
                        tea5767_get,
                        tea5767_region_data)
    #endif
    #if (CONFIG_TUNER & S1A0903X01)
        TUNER_TYPE_CASE(S1A0903X01,
                        s1a0903x01_set,
                        s1a0903x01_get,
                        NULL)
    #endif
    }
}

#endif /* SIMULATOR */
