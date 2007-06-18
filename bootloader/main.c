/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "usb.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "uda1380.h"
#include "eeprom_settings.h"
#include "rbunicode.h"
#include "common.h"

#include <stdarg.h>

/* Maximum allowed firmware image size. 10MB is more than enough */
#define MAX_LOADSIZE (10*1024*1024)

#define DRAM_START 0x31000000

#ifdef HAVE_EEPROM_SETTINGS
static bool recovery_mode = false;
#endif

char version[] = APPSVERSION;

/* Reset the cookie for the crt0 crash check */
inline void __reset_cookie(void)
{
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
}

void start_iriver_fw(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" lea.l 8,%a0");
    asm(" jmp (%a0)");
}

void start_firmware(void)
{
    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(*(int *)DRAM_START));
    asm(" move.l %0,%%a0" :: "m"(*(int *)(DRAM_START+4)));
    asm(" jmp (%a0)");
}

#ifdef IRIVER_H100_SERIES
void start_flashed_romimage(void)
{
    uint8_t *src = (uint8_t *)FLASH_ROMIMAGE_ENTRY;
    int *reset_vector;
    
    if (!detect_flashed_romimage())
        return ;
        
    reset_vector = (int *)(&src[sizeof(struct flash_header)+4]);

    asm(" move.w #0x2700,%sr");
    __reset_cookie();
    
    asm(" move.l %0,%%d0" :: "i"(DRAM_START));
    asm(" movec.l %d0,%vbr");
    asm(" move.l %0,%%sp" :: "m"(reset_vector[0]));
    asm(" move.l %0,%%a0" :: "m"(reset_vector[1]));
    asm(" jmp (%a0)");
    
    /* Failure */
    power_off();
}

void start_flashed_ramimage(void)
{
    struct flash_header hdr;
    unsigned char *buf = (unsigned char *)DRAM_START;
    uint8_t *src = (uint8_t *)FLASH_RAMIMAGE_ENTRY;

    if (!detect_flashed_ramimage())
         return;
    
    /* Load firmware from flash */
    cpu_boost(true);
    memcpy(&hdr, src, sizeof(struct flash_header));
    src += sizeof(struct flash_header);
    memcpy(buf, src, hdr.length);
    cpu_boost(false);
    
    start_firmware();

    /* Failure */
    power_off();
}
#endif /* IRIVER_H100_SERIES */

void shutdown(void)
{
    printf("Shutting down...");
#ifdef HAVE_EEPROM_SETTINGS
    /* Reset the rockbox crash check. */
    firmware_settings.bl_version = 0;
    eeprom_settings_store();
#endif
    
    /* We need to gracefully spin down the disk to prevent clicks. */
    if (ide_powered())
    {
        /* Make sure ATA has been initialized. */
        ata_init();
        
        /* And put the disk into sleep immediately. */
        ata_sleepnow();
    }

    sleep(HZ*2);
    
    /* Backlight OFF */
    __backlight_off();
#ifdef HAVE_REMOTE_LCD
    __remote_backlight_off();
#endif
    
    __reset_cookie();
    power_off();
}

/* Print the battery voltage (and a warning message). */
void check_battery(void)
{
    int adc_battery, battery_voltage, batt_int, batt_frac;
    
    adc_battery = adc_read(ADC_BATTERY);

    battery_voltage = (adc_battery * BATTERY_SCALE_FACTOR) / 10000;
    batt_int = battery_voltage / 100;
    batt_frac = battery_voltage % 100;

    printf("Batt: %d.%02dV", batt_int, batt_frac);

    if (battery_voltage <= 310) 
    {
        printf("WARNING! BATTERY LOW!!");
        sleep(HZ*2);
    }
}

#ifdef HAVE_EEPROM_SETTINGS
void initialize_eeprom(void)
{
    if (detect_original_firmware())
        return ;
    
    if (!eeprom_settings_init())
    {
        recovery_mode = true;
        return ;
    }
    
    /* If bootloader version has not been reset, disk might
     * not be intact. */
    if (firmware_settings.bl_version || !firmware_settings.disk_clean)
    {
        firmware_settings.disk_clean = false;
        recovery_mode = true;
    }
    
    firmware_settings.bl_version = EEPROM_SETTINGS_BL_MINVER;
    eeprom_settings_store();
}

void try_flashboot(void)
{
    if (!firmware_settings.initialized)
        return ;
    
   switch (firmware_settings.bootmethod)
    {
        case BOOT_DISK:
            return;
        
        case BOOT_ROM:
            start_flashed_romimage();
            recovery_mode = true;
            break;
        
        case BOOT_RAM:
            start_flashed_ramimage();
            recovery_mode = true;
            break;
        
        default:
            recovery_mode = true;
            return;
    }
}

static const char *options[] = { 
    "Boot from disk",
    "Boot RAM image",
    "Boot ROM image",
    "Shutdown" 
};

#define FAILSAFE_OPTIONS 4
void failsafe_menu(void)
{
    int timeout = 15;
    int option = 3;
    int button;
    int defopt = -1;
    char buf[32];
    int i;
    extern int line;
    
    reset_screen();
    printf("Bootloader %s", version);
    check_battery();
    printf("=========================");
    line += FAILSAFE_OPTIONS;
    printf("");
    printf("  [NAVI] to confirm.");
    printf("  [REC] to set as default.");
    printf("");

    if (firmware_settings.initialized)
    {
        defopt = firmware_settings.bootmethod;
        if (defopt < 0 || defopt >= FAILSAFE_OPTIONS)
            defopt = option;
    }
    
    while (timeout > 0)
    {
        /* Draw the menu. */
        line = 3;
        for (i = 0; i < FAILSAFE_OPTIONS; i++)
        {
            char *def = "[DEF]";
            char *arrow = "->";
            
            if (i != defopt)
                def = "";
            if (i != option)
                arrow = "  ";
            
            printf("%s %s %s", arrow, options[i], def);
        }
        
        snprintf(buf, sizeof(buf), "Time left: %ds", timeout);
        lcd_puts(0, 10, buf);
        lcd_update();
        button = button_get_w_tmo(HZ);

        if (button == BUTTON_NONE)
        {
            timeout--;
            continue ;
        }

        timeout = 15;
        /* Ignore the ON/PLAY -button because it can cause trouble 
           with the RTC alarm mod. */
        switch (button & ~(BUTTON_ON))
        {
            case BUTTON_UP:
            case BUTTON_RC_REW:
                if (option > 0)
                    option--;
                break ;
                
            case BUTTON_DOWN:
            case BUTTON_RC_FF:
                if (option < FAILSAFE_OPTIONS-1)
                    option++;
                break ;

            case BUTTON_SELECT:
            case BUTTON_RC_ON:
                timeout = 0;
                break ;
            
            case BUTTON_REC:
            case BUTTON_RC_REC:
                if (firmware_settings.initialized)
                {
                    firmware_settings.bootmethod = option;
                    eeprom_settings_store();
                    defopt = option;
                }
                break ;
        }
    }
    
    lcd_puts(0, 10, "Executing command...");
    lcd_update();
    sleep(HZ);
    reset_screen();

    switch (option)
    {
        case BOOT_DISK:
            return ;
        
        case BOOT_RAM:
            start_flashed_ramimage();
            printf("Image not found");
            break;
        
        case BOOT_ROM:
            start_flashed_romimage();
            printf("Image not found");
            break;
    }
    
    shutdown();
}
#endif

void main(void)
{
    int i;
    int rc;
    bool rc_on_button = false;
    bool on_button = false;
    bool rec_button = false;
    bool hold_status = false;
    int data;
    extern int line;        /* From common.c */
    extern int remote_line; /* From common.c */
    
    /* We want to read the buttons as early as possible, before the user
       releases the ON button */

    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs
       (The ON and Hold buttons on the main unit and the remote) */
    or_l(0x00100062, &GPIO1_FUNCTION);
    and_l(~0x00100062, &GPIO1_ENABLE);

    data = GPIO1_READ;
    if ((data & 0x20) == 0)
        on_button = true;

    if ((data & 0x40) == 0)
        rc_on_button = true;

    /* Set the default state of the hard drive power to OFF */
    ide_power_enable(false);

    power_init();

    /* Turn off if neither ON button is pressed */
    if(!(on_button || rc_on_button || usb_detect()))
    {
        __reset_cookie();
        power_off();
    }

    /* Start with the main backlight OFF. */
    __backlight_init();
    __backlight_off();
    
    /* Remote backlight ON */
#ifdef HAVE_REMOTE_LCD
    __remote_backlight_on();
#endif

    system_init();
    kernel_init();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    /* Set up waitstates for the peripherals */
    set_cpu_frequency(0); /* PLL off */
#ifdef CPU_COLDFIRE
    coldfire_set_pllcr_audio_bits(DEFAULT_PLLCR_AUDIO_BITS);
#endif
#endif
    set_irq_level(0);

#ifdef HAVE_EEPROM_SETTINGS
    initialize_eeprom();
#endif
    
    adc_init();
    button_init();
    
    if ((on_button && button_hold()) ||
         (rc_on_button && remote_button_hold()))
    {
        hold_status = true;
    }
    
    /* Power on the hard drive early, to speed up the loading. */
    if (!hold_status
# ifdef HAVE_EEPROM_SETTINGS
        && !recovery_mode
# endif
        )
    {
        ide_power_enable(true);
    }
    
# ifdef EEPROM_SETTINGS
    if (!hold_status && !usb_detect() && !recovery_mode)
        try_flashboot();
# endif

    backlight_init();
#ifdef HAVE_UDA1380
    audiohw_reset();
#endif

    lcd_init();
#ifdef HAVE_REMOTE_LCD
    lcd_remote_init();
#endif
    font_init();

    lcd_setfont(FONT_SYSFIXED);

    printf("Rockbox boot loader");
    printf("Version %s", version);

    sleep(HZ/50); /* Allow the button driver to check the buttons */
    rec_button = ((button_status() & BUTTON_REC) == BUTTON_REC)
        || ((button_status() & BUTTON_RC_REC) == BUTTON_RC_REC);

    check_battery();

    /* Don't start if the Hold button is active on the device you
       are starting with */
    if (!usb_detect() && (hold_status
#ifdef HAVE_EEPROM_SETTINGS
                          || recovery_mode
#endif
                          ))
    {
        if (detect_original_firmware())
        {
            printf("Hold switch on");
            shutdown();
        }
        
#ifdef HAVE_EEPROM_SETTINGS
        failsafe_menu();
#endif
    }

    /* Holding REC while starting runs the original firmware */
    if (detect_original_firmware() && rec_button)
    {
        printf("Starting original firmware...");
        start_iriver_fw();
    }

    usb_init();

    /* A hack to enter USB mode without using the USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        int w, h;
        font_getstringsize(msg, &w, &h, FONT_SYSFIXED);
        reset_screen();
        lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, msg);
        lcd_update();

#ifdef HAVE_REMOTE_LCD
        lcd_remote_puts(0, 3, msg);
        lcd_remote_update();
#endif

#ifdef HAVE_EEPROM_SETTINGS
        if (firmware_settings.initialized)
        {
            firmware_settings.disk_clean = false;
            eeprom_settings_store();
        }
#endif
        ide_power_enable(true);
        ata_enable(false);
        sleep(HZ/20);
        usb_enable(true);
        cpu_idle_mode(true);
        while (usb_detect())
        {
            /* Print the battery status. */
            line = 0;
            remote_line = 0;
            check_battery();
            
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);

            /* Backlight OFF */
            __backlight_off();
        }

        cpu_idle_mode(false);
        usb_enable(false);

        reset_screen();
        lcd_update();
    }

    rc = ata_init();
    if(rc)
    {
        reset_screen();
        printf("ATA error: %d", rc);
        printf("Insert USB cable and press");
        printf("a button");
        while(!(button_get(true) & BUTTON_REL));
    }


    disk_init();

    rc = disk_mount_all();
    if (rc<=0)
    {
        reset_screen();
        printf("No partition found");
        while(button_get(true) != SYS_USB_CONNECTED) {};
    }

    printf("Loading firmware");
    i = load_firmware((unsigned char *)DRAM_START, BOOTFILE, MAX_LOADSIZE);
    if(i < 0)
        printf("Error: %s", strerror(i));

    if (i == EOK)
        start_firmware();

    if (!detect_original_firmware())
    {
        printf("No firmware found on disk");
        sleep(HZ*2);
        shutdown();
    }
    else {
        sleep(HZ*2);
        start_iriver_fw();
    }
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void screen_dump(void)
{
}

int usb_screen(void)
{
   return 0;
}

unsigned short *bidi_l2v(const unsigned char *str, int orientation)
{
    static unsigned short utf16_buf[SCROLL_LINE_SIZE];
    unsigned short *target;
    (void)orientation;
    
    target = utf16_buf;
    
    while (*str)
        str = utf8decode(str, target++);
    *target = 0;
    return utf16_buf;
}
