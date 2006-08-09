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
#include "config.h"

#include "ata.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "panic.h"
#include "menu.h"
#include "system.h"
#include "usb.h"
#include "powermgmt.h"
#include "adc.h"
#include "i2c.h"
#ifndef DEBUG
#include "serial.h"
#endif
#include "audio.h"
#include "mp3_playback.h"
#include "thread.h"
#include "settings.h"
#include "backlight.h"
#include "status.h"
#include "debug_menu.h"
#include "version.h"
#include "sprintf.h"
#include "font.h"
#include "language.h"
#include "gwps.h"
#include "playlist.h"
#include "buffer.h"
#include "rolo.h"
#include "screens.h"
#include "power.h"
#include "talk.h"
#include "plugin.h"
#include "misc.h"
#include "database.h"
#include "dircache.h"
#include "tagcache.h"
#include "tagtree.h"
#include "lang.h"
#include "string.h"
#include "splash.h"
#include "eeprom_settings.h"

#if (CONFIG_CODEC == SWCODEC)
#include "playback.h"
#include "pcmbuf.h"
#else
#define pcmbuf_init()
#endif
#if (CONFIG_CODEC == SWCODEC) && defined(HAVE_RECORDING) && !defined(SIMULATOR)
#include "pcm_record.h"
#define SETTINGS_RESET BUTTON_REC
#endif

#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

#if CONFIG_USBOTG == USBOTG_ISP1362
#include "isp1362.h"
#endif

#if CONFIG_USBOTG == USBOTG_M5636
#include "m5636.h"
#endif

/*#define AUTOROCK*/ /* define this to check for "autostart.rock" on boot */

const char appsversion[]=APPSVERSION;

void init(void);

void app_main(void)
{
    init();
    browse_root();
}

int init_dircache(void)
{
#ifdef HAVE_DIRCACHE
    int result = 0;
    
    dircache_init();
    if (global_settings.dircache)
    {
# ifdef HAVE_EEPROM
        if (firmware_settings.initialized && firmware_settings.disk_clean)
        {
            result = dircache_load(DIRCACHE_FILE);
            remove(DIRCACHE_FILE);
            if (result == 0)
                return 0;
        }
# endif
        
        result = dircache_build(global_settings.dircache_size);
    }
    
    return result;
#else
    return 0;
#endif
}

void init_tagcache(void)
{
    bool clear = false;

    tagcache_init();
    
    while (!tagcache_is_initialized())
    {
#ifdef HAVE_LCD_CHARCELLS
        char buf[32];
#endif
        int ret = tagcache_get_commit_step();

        if (ret > 0)
        {
#ifdef HAVE_LCD_BITMAP
            gui_syncsplash(0, true, "%s [%d/%d]",
                                    str(LANG_TAGCACHE_INIT), ret, TAG_COUNT);
#else
            lcd_double_height(false);
            snprintf(buf, sizeof(buf), " TC [%d/%d]", ret, TAG_COUNT);
            lcd_puts(0, 1, buf);
#endif
            clear = true;
        }
        sleep(HZ/4);
    }
    tagtree_init();

    if (clear)
    {
        backlight_on();
        show_logo();
    }
}

#ifdef SIMULATOR

void init(void)
{
    init_threads();
    buffer_init();
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();
    show_logo();
    button_init();
    backlight_init();
    lang_init();
    /* Must be done before any code uses the multi-screen APi */
    screen_access_init();
    gui_syncstatusbar_init(&statusbars);
    settings_reset();
    settings_calc_config_sector();
    settings_load(SETTINGS_ALL);
    gui_sync_wps_init();
    settings_apply();
    init_dircache();
    init_tagcache();
    sleep(HZ/2);
    tree_init();
    playlist_init();
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.stereo_width,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);

#if CONFIG_CODEC == SWCODEC
    audio_preinit();
#endif

    /* audio_init must to know the size of voice buffer so init voice first */
#if CONFIG_CODEC == SWCODEC
    talk_init();
#endif

    audio_init();
    button_clear_queue(); /* Empty the keyboard buffer */
}

#else

void init(void)
{
    int rc;
    bool mounted = false;
#if defined(CONFIG_CHARGING) && (CONFIG_CPU == SH7034)
    /* if nobody initialized ATA before, I consider this a cold start */
    bool coldstart = (PACR2 & 0x4000) != 0; /* starting from Flash */
#endif
    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
    cpu_boost(true);
#endif
    
    buffer_init();

    settings_reset();

    power_init();

    set_irq_level(0);
    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();
    show_logo();
    lang_init();

#ifdef DEBUG
    debug_init();
#else
#ifndef HAVE_MMC  /* FIXME: This is also necessary for debug builds
                   * (do debug builds on the Ondio make sense?) */
    serial_setup();
#endif
#endif

    i2c_init();

#ifdef CONFIG_RTC
    rtc_init();
#endif
#ifdef HAVE_RTC_RAM
    settings_load(SETTINGS_RTC); /* early load parts of global_settings */
#endif

    adc_init();
    
    usb_init();
#if CONFIG_USBOTG == USBOTG_ISP1362
    isp1362_init();
#elif CONFIG_USBOTG == USBOTG_M5636
    m5636_init();
#endif

    backlight_init();

    button_init();

    powermgmt_init();
    
#if CONFIG_CODEC == SWCODEC
    audio_preinit();
#endif

#ifdef CONFIG_TUNER
    radio_init();
#endif

    /* Must be done before any code uses the multi-screen APi */
    screen_access_init();
    gui_syncstatusbar_init(&statusbars);

#if defined(CONFIG_CHARGING) && (CONFIG_CPU == SH7034)
    if (coldstart && charger_inserted()
        && !global_settings.car_adapter_mode
#ifdef ATA_POWER_PLAYERSTYLE
        && !ide_powered() /* relies on probing result from bootloader */
#endif
        )
    {
        rc = charging_screen(); /* display a "charging" screen */
        if (rc == 1)            /* charger removed */
            power_off();
        /* "On" pressed or USB connected: proceed */
        show_logo();  /* again, to provide better visual feedback */
    }
#endif

    rc = ata_init();
    if(rc)
    {
#ifdef HAVE_LCD_BITMAP
        char str[32];
        lcd_clear_display();
        snprintf(str, 31, "ATA error: %d", rc);
        lcd_puts(0, 1, str);
        lcd_puts(0, 3, "Press ON to debug");
        lcd_update();
        while(!(button_get(true) & BUTTON_REL));
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }

    usb_start_monitoring();
    while (usb_detect())
    {   /* enter USB mode early, before trying to mount */
        if (button_get_w_tmo(HZ/10) == SYS_USB_CONNECTED)
#ifdef HAVE_MMC
            if (!mmc_touched() || (mmc_remove_request() == SYS_MMC_EXTRACTED))
#endif
            {
                usb_screen();
                mounted = true; /* mounting done @ end of USB mode */
            }
#ifdef HAVE_USB_POWER
        if (usb_powered())      /* avoid deadlock */
            break;
#endif
    }

    if (!mounted)
    {
        rc = disk_mount_all();
        if (rc<=0)
        {
            lcd_clear_display();
            lcd_puts(0, 0, "No partition");
            lcd_puts(0, 1, "found.");
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0, 2, "Insert USB cable");
            lcd_puts(0, 3, "and fix it.");
            lcd_update();
#endif
            while(button_get(true) != SYS_USB_CONNECTED) {};
            usb_screen();
            system_reboot();
        }
    }

#ifdef HAVE_EEPROM
    eeprom_settings_init();
#endif
    
    settings_calc_config_sector();
    
#if defined(SETTINGS_RESET) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#ifdef SETTINGS_RESET
    /* Reset settings if holding the rec button. */
    if ((button_status() & SETTINGS_RESET) == SETTINGS_RESET)
#else
    /* Reset settings if the hold button is turned on */
    if (button_hold())
#endif
    {
        gui_syncsplash(HZ*2, true, str(LANG_RESET_DONE_CLEAR));
        settings_reset();
    }
    else
#endif
        settings_load(SETTINGS_ALL);

    
    init_dircache();
    
    gui_sync_wps_init();
    settings_apply();
    //init_dircache();
    init_tagcache();

#ifdef HAVE_EEPROM
    if (firmware_settings.initialized)
    {
        /* In case we crash. */
        firmware_settings.disk_clean = false;
        eeprom_settings_store();
    }
#endif
    status_init();
    playlist_init();
    tree_init();
        
    /* No buffer allocation (see buffer.c) may take place after the call to
       audio_init() since the mpeg thread takes the rest of the buffer space */
    mp3_init( global_settings.volume,
              global_settings.bass,
              global_settings.treble,
              global_settings.balance,
              global_settings.loudness,
              global_settings.avc,
              global_settings.channel_config,
              global_settings.stereo_width,
              global_settings.mdb_strength,
              global_settings.mdb_harmonics,
              global_settings.mdb_center,
              global_settings.mdb_shape,
              global_settings.mdb_enable,
              global_settings.superbass);

     /* audio_init must to know the size of voice buffer so init voice first */
    talk_init();

    audio_init();
#if (CONFIG_CODEC == SWCODEC) && defined(HAVE_RECORDING) && !defined(SIMULATOR)
    pcm_rec_init();
#endif

    /* runtime database has to be initialized after audio_init() */
    cpu_boost(false);

#ifdef AUTOROCK
    {
        int fd;
        static const char filename[] = PLUGIN_DIR "/autostart.rock"; 

        fd = open(filename, O_RDONLY);
        if(fd >= 0) /* no complaint if it doesn't exist */
        {
            close(fd);
            plugin_load((char*)filename, NULL); /* start if it does */
        }
    }
#endif /* #ifdef AUTOROCK */

#ifdef CONFIG_CHARGING
    car_adapter_mode_init();
#endif
}

#ifdef CPU_PP
void cop_main(void)
{
/* This is the entry point for the coprocessor
   Anyone not running an upgraded bootloader will never reach this point,
   so it should not be assumed that the coprocessor be usable even on
   platforms which support it.

   At present all we do is send the COP to sleep if anything wakes it. */
    while(1) {
        COP_CTL = PROC_SLEEP;
    }
}
#endif

int main(void)
{
    app_main();

    while(1) {
#if CONFIG_LED == LED_REAL
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
#endif
    }
    return 0;
}
#endif

