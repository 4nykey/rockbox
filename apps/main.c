/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Bj�rn Stenberg
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
#include "ata_idle_notify.h"
#include "disk.h"
#include "fat.h"
#include "lcd.h"
#include "rtc.h"
#include "debug.h"
#include "led.h"
#include "kernel.h"
#include "button.h"
#include "tree.h"
#include "filetypes.h"
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
#include "dircache.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#include "tagtree.h"
#endif
#include "lang.h"
#include "string.h"
#include "splash.h"
#include "eeprom_settings.h"
#include "scrobbler.h"
#include "icon.h"

#if (CONFIG_CODEC == SWCODEC)
#include "playback.h"
#include "pcmbuf.h"
#else
#define pcmbuf_init()
#endif
#if (CONFIG_CODEC == SWCODEC) && defined(HAVE_RECORDING) && !defined(SIMULATOR)
#include "pcm_record.h"
#endif

#ifdef BUTTON_REC
#define SETTINGS_RESET BUTTON_REC
#endif

#if CONFIG_TUNER
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

#include "cuesheet.h"

/*#define AUTOROCK*/ /* define this to check for "autostart.rock" on boot */

const char appsversion[]=APPSVERSION;

static void init(void);

#ifdef SIMULATOR
void app_main(void)
#else
static void app_main(void)
#endif
{
    init();
    browse_root();
}

static int init_dircache(bool preinit)
{
#ifdef HAVE_DIRCACHE
    int result = 0;
    bool clear = false;
    
    if (preinit)
        dircache_init();
    
    if (!global_settings.dircache)
        return 0;
    
# ifdef HAVE_EEPROM_SETTINGS
    if (firmware_settings.initialized && firmware_settings.disk_clean 
        && preinit)
    {
        result = dircache_load();

        if (result < 0)
        {
            firmware_settings.disk_clean = false;
            if (global_status.dircache_size <= 0)
            {
                /* This will be in default language, settings are not
                   applied yet. Not really any easy way to fix that. */
                gui_syncsplash(0, str(LANG_DIRCACHE_BUILDING));
                clear = true;
            }
            
            dircache_build(global_status.dircache_size);
        }
    }
    else
# endif
    {
        if (preinit)
            return -1;
        
        if (!dircache_is_enabled()
            && !dircache_is_initializing())
        {
            if (global_status.dircache_size <= 0)
            {
                gui_syncsplash(0, str(LANG_DIRCACHE_BUILDING));
                clear = true;
            }
            result = dircache_build(global_status.dircache_size);
        }
        
        if (result < 0)
        {
            /* Initialization of dircache failed. Manual action is
             * necessary to enable dircache again.
             */
            gui_syncsplash(0, "Dircache failed, disabled. Result: %d", result);
            global_settings.dircache = false;
        }
            
    }
    
    if (clear)
    {
        backlight_on();
        show_logo();
        global_status.dircache_size = dircache_get_cache_size();
        status_save();
    }
    
    return result;
#else
    (void)preinit;
    return 0;
#endif
}

#ifdef HAVE_TAGCACHE
static void init_tagcache(void)
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
            gui_syncsplash(0, "%s [%d/%d]",
                str(LANG_TAGCACHE_INIT), ret, 
                tagcache_get_max_commit_step());
#else
            lcd_double_height(false);
            snprintf(buf, sizeof(buf), " DB [%d/%d]", ret, 
                tagcache_get_max_commit_step());
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
#endif

#ifdef SIMULATOR

static void init(void)
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
    settings_load(SETTINGS_ALL);
    gui_sync_wps_init();
    settings_apply();
    init_dircache(true);
    init_dircache(false);
#ifdef HAVE_TAGCACHE
    init_tagcache();
#endif
    sleep(HZ/2);
    tree_init();
    filetype_init();
    icons_init();
    playlist_init();

#if CONFIG_CODEC != SWCODEC
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
#endif /* CONFIG_CODEC != SWCODEC */

    scrobbler_init();
    cuesheet_init();

    audio_init();
    button_clear_queue(); /* Empty the keyboard buffer */
}

#else

static void init(void)
{
    int rc;
    bool mounted = false;
#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
    /* if nobody initialized ATA before, I consider this a cold start */
    bool coldstart = (PACR2 & 0x4000) != 0; /* starting from Flash */
#endif
#ifdef CPU_PP
    COP_CTL = PROC_WAKE;
#endif
    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    set_cpu_frequency(CPUFREQ_NORMAL);
#ifdef CPU_COLDFIRE
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
#endif
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
    
#if !defined(TOSHIBA_GIGABEAT_F) || defined(SIMULATOR)
    show_logo();
#else
    sleep(1);  /* Weird.  We crash w/o this tiny delay. */
#endif    
    lang_init();

#ifdef DEBUG
    debug_init();
#else
#if !defined(HAVE_FMADC) && !defined(HAVE_MMC)
    serial_setup();
#endif
#endif

    i2c_init();

#if CONFIG_RTC
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
    
#if CONFIG_TUNER
    radio_init();
#endif

    /* Must be done before any code uses the multi-screen APi */
    screen_access_init();
    gui_syncstatusbar_init(&statusbars);

#if CONFIG_CHARGING && (CONFIG_CPU == SH7034)
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

    ata_idle_notify_init();
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
        while(!(button_get(true) & BUTTON_REL)); /*DO NOT CHANGE TO ACTION SYSTEM */
        dbg_ports();
#endif
        panicf("ata: %d", rc);
    }

#ifdef HAVE_EEPROM_SETTINGS
    eeprom_settings_init();
#endif
    
    usb_start_monitoring();
    while (usb_detect())
    {   
#ifdef HAVE_EEPROM_SETTINGS
        firmware_settings.disk_clean = false;
#endif
        /* enter USB mode early, before trying to mount */
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
#endif
            lcd_update();

            while(button_get(true) != SYS_USB_CONNECTED) {};
            usb_screen();
            system_reboot();
        }
    }

#if defined(SETTINGS_RESET) || (CONFIG_KEYPAD == IPOD_4G_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H10_PAD) || (CONFIG_KEYPAD == GIGABEAT_PAD)
#ifdef SETTINGS_RESET
    /* Reset settings if holding the rec button. */
    if ((button_status() & SETTINGS_RESET) == SETTINGS_RESET)
#else
    /* Reset settings if the hold button is turned on */
    if (button_hold())
#endif
    {
        gui_syncsplash(HZ*2, str(LANG_RESET_DONE_CLEAR));
        settings_reset();
    }
    else
#endif
        settings_load(SETTINGS_ALL);

    if (init_dircache(true) < 0)
    {
#ifdef HAVE_TAGCACHE
        remove(TAGCACHE_STATEFILE);
#endif
    }
    
    gui_sync_wps_init();
    settings_apply();
    init_dircache(false);
#ifdef HAVE_TAGCACHE
    init_tagcache();
#endif

#ifdef HAVE_EEPROM_SETTINGS
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
    filetype_init();
    icons_init();
    scrobbler_init();
    cuesheet_init();

#if CONFIG_CODEC != SWCODEC
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
#endif /* CONFIG_CODEC != SWCODEC */

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

#if CONFIG_CHARGING
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

   A kernel thread runs on the coprocessor which waits for other threads to be
   added, and gracefully handles RoLo */

#if CONFIG_CPU == PP5002
/* 3G doesn't have Rolo or dual core support yet */
    while(1) {
        COP_CTL = PROC_SLEEP;
    }
#else
    extern volatile unsigned char cpu_message;

    system_init();
    kernel_init();

    while(cpu_message != COP_REBOOT) {
        sleep(HZ);
    }
    rolo_restart_cop();
#endif /* PP5002 */
}
#endif /* CPU_PP */

int main(void)
{
    app_main();

    while(1) {
#if (CONFIG_LED == LED_REAL)
        led(true); sleep(HZ/10);
        led(false); sleep(HZ/10);
#endif
    }
    return 0;
}
#endif

