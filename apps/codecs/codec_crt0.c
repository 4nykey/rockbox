/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Tomasz Malesinski
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "codeclib.h"

struct codec_api *ci;

extern unsigned char iramcopy[];
extern unsigned char iramstart[];
extern unsigned char iramend[];
extern unsigned char iedata[];
extern unsigned char iend[];
extern unsigned char plugin_bss_start[];
extern unsigned char plugin_end_addr[];

extern enum codec_status codec_main(void);

CACHE_FUNCTION_WRAPPERS(ci);

enum codec_status codec_start(struct codec_api *api)
{
#ifndef SIMULATOR
#ifdef USE_IRAM
    api->memcpy(iramstart, iramcopy, iramend - iramstart);
    api->memset(iedata, 0, iend - iedata);
#endif
    api->memset(plugin_bss_start, 0, plugin_end_addr - plugin_bss_start);
#endif
    ci = api;
#if NUM_CORES > 1
    /* writeback cleared iedata and bss areas */
    flush_icache();
#endif
    return codec_main();
}
