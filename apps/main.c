/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Bj�rn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "panic.h"
#include "menu.h"
#include "system.h"
#ifndef SIMULATOR
#include "dmalloc.h"
#include "bmalloc.h"
#endif
#include "mpeg.h"
#include "main_menu.h"

#include "version.h"

char appsversion[]=APPSVERSION;

void app_main(void)
{
    browse_root();
}

#ifndef SIMULATOR

/* defined in linker script */
extern int poolstart[];
extern int poolend[];

int init(void)
{
    int rc;

    system_init();
    
#ifdef HAVE_LCD_BITMAP
    lcd_init();
#endif
    show_splash();
    dmalloc_initialize();
    bmalloc_add_pool(poolstart, poolend-poolstart);

#ifdef DEBUG
    debug_init();
#endif
    kernel_init();
    set_irq_level(0);

    rc = ata_init();
    if(rc)
        panicf("ata: %d",rc);

    rc = disk_init();
    if (rc)
        panicf("disk: %d",rc);

    rc = fat_mount(part[0].start);
    if(rc)
        panicf("mount: %d",rc);

    button_init();
    mpeg_init();

    return 0;
}

int main(void)
{
    init();
    app_main();

    while(1) {
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
    }
    return 0;
}
#endif
