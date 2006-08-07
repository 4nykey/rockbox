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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "backlight.h"
#include "button.h"
#include "lcd.h"
#include "lang.h"
#include "icons.h"
#include "font.h"
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "settings.h"
#include "status.h"
#include "playlist.h"
#include "sprintf.h"
#include "kernel.h"
#include "power.h"
#include "system.h"
#include "powermgmt.h"
#include "adc.h"
#include "action.h"
#include "talk.h"
#include "misc.h"
#include "id3.h"
#include "screens.h"
#include "debug.h"
#include "led.h"
#include "sound.h"
#include "gwps-common.h"
#include "splash.h"
#include "statusbar.h"
#include "screen_access.h"
#include "quickscreen.h"
#include "logo.h"
#include "pcmbuf.h"
#include "list.h"

#if defined(HAVE_LCD_BITMAP)
#include "widgets.h"
#endif
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif
#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#endif

#ifdef HAVE_LCD_COLOR
#include "backdrop.h"
#endif

#ifdef HAVE_LCD_BITMAP
#define SCROLLBAR_WIDTH  6
#endif

void usb_screen(void)
{
#ifdef USB_NONE
    /* nothing here! */
#else
    int i;
#ifdef HAVE_LCD_COLOR
    show_main_backdrop();
#endif
    FOR_NB_SCREENS(i) {
        screens[i].backlight_on();
        gui_logo_draw(&usb_logos[i], &screens[i]);
    }
#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
    status_set_audio(false);
    status_set_usb(true);
#endif /* HAVE_LCD_BITMAP */
    gui_syncstatusbar_draw(&statusbars, true);
#ifdef SIMULATOR
    while (button_get(true) & BUTTON_REL);
#else
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
    while(usb_wait_for_disconnect_w_tmo(&button_queue, HZ)) {
        if(usb_inserted()) {
#ifdef HAVE_MMC /* USB-MMC bridge can report activity */
            led(mmc_usb_active(HZ));
#endif /* HAVE_MMC */
            gui_syncstatusbar_draw(&statusbars, false);
        }
    }
#endif /* SIMULATOR */
#ifdef HAVE_LCD_CHARCELLS
    status_set_usb(false);
#endif /* HAVE_LCD_CHARCELLS */
    FOR_NB_SCREENS(i)
        screens[i].backlight_on();
#endif /* USB_NONE */
}

#ifdef HAVE_MMC
int mmc_remove_request(void)
{
    struct event ev;
    int i;
    FOR_NB_SCREENS(i)
        screens[i].clear_display();
    gui_syncsplash(1, true, str(LANG_REMOVE_MMC));
    if (global_settings.talk_menu)
        talk_id(LANG_REMOVE_MMC, false);

    while (1)
    {
        queue_wait_w_tmo(&button_queue, &ev, HZ/2);
        switch (ev.id)
        {
            case SYS_MMC_EXTRACTED:
                return SYS_MMC_EXTRACTED;

            case SYS_USB_DISCONNECTED:
                return SYS_USB_DISCONNECTED;
        }
    }
}
#endif


/* some simulator dummies */
#ifdef SIMULATOR
#define BATTERY_SCALE_FACTOR 7000
unsigned short adc_read(int channel)
{
   (void)channel;
   return 100;
}
#endif

#if defined(CONFIG_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)

#ifdef HAVE_LCD_BITMAP
void charging_display_info(bool animate)
{
    unsigned char charging_logo[36];
    const int pox_x = (LCD_WIDTH - sizeof(charging_logo)) / 2;
    const int pox_y = 32;
    static unsigned phase = 3;
    unsigned i;
    char buf[32];
    (void)buf;

#ifdef NEED_ATA_POWER_BATT_MEASURE
    if (ide_powered()) /* FM and V2 can only measure when ATA power is on */
#endif
    {
        int battery_voltage;
        int batt_int, batt_frac;

        battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;

        snprintf(buf, 32, "  Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, 7, buf);
    }

#if CONFIG_CHARGING == CHARGING_CONTROL

    snprintf(buf, 32, "Charge mode:");
    lcd_puts(0, 2, buf);

    if (charge_state == CHARGING)
        snprintf(buf, 32, str(LANG_BATTERY_CHARGE));
    else if (charge_state == TOPOFF)
        snprintf(buf, 32, str(LANG_BATTERY_TOPOFF_CHARGE));
    else if (charge_state == TRICKLE)
        snprintf(buf, 32, str(LANG_BATTERY_TRICKLE_CHARGE));
    else
        snprintf(buf, 32, "not charging");

    lcd_puts(0, 3, buf);
    if (!charger_enabled)
        animate = false;
#endif /* CONFIG_CHARGING == CHARGING_CONTROL */


    /* middle part */
    memset(charging_logo+3, 0x00, 32);
    charging_logo[0] = 0x3C;
    charging_logo[1] = 0x24;
    charging_logo[2] = charging_logo[35] = 0xFF;

    if (!animate)
    {   /* draw the outline */
        /* middle part */
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_FG);
        /* upper line */
        charging_logo[0] = charging_logo[1] = 0x00;
        memset(charging_logo+2, 0x80, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y, sizeof(charging_logo), 8);
        /* lower line */
        memset(charging_logo+2, 0x01, 34);
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 16, sizeof(charging_logo), 8);
        lcd_set_drawmode(DRMODE_SOLID);
    }
    else
    {   /* animate the middle part */
        for (i = 3; i<MIN(sizeof(charging_logo)-1, phase); i++)
        {
            if ((i-phase) % 8 == 0)
            {   /* draw a "bubble" here */
                unsigned bitpos;
                bitpos = (phase + i/8) % 15; /* "bounce" effect */
                if (bitpos > 7)
                    bitpos = 14 - bitpos;
                charging_logo[i] = 0x01 << bitpos;
            }
        }
        lcd_mono_bitmap(charging_logo, pox_x, pox_y + 8, sizeof(charging_logo), 8);
        phase++;
    }
    lcd_update();
}
#else /* not HAVE_LCD_BITMAP */

static unsigned char logo_chars[5];
static const unsigned char logo_pattern[] = {
    0x07, 0x04, 0x1c, 0x14, 0x1c, 0x04, 0x07, /* char 1 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, /* char 2 */
    0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, /* char 3 */
    0x1f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f, /* char 4 */
};

static void logo_lock_patterns(bool on)
{
    int i;

    if (on)
    {
        for (i = 0; i < 4; i++)
            logo_chars[i] = lcd_get_locked_pattern();
        logo_chars[4] = '\0';
    }
    else
    {
        for (i = 0; i < 4; i++)
            lcd_unlock_pattern(logo_chars[i]);
    }
}

void charging_display_info(bool animate)
{
    int battery_voltage;
    unsigned i, ypos;
    static unsigned phase = 3;
    char buf[28];

    battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR)
                      / 10000;
    snprintf(buf, sizeof(buf), "%s %d.%02dV", logo_chars,
             battery_voltage / 100, battery_voltage % 100);
    lcd_puts(0, 1, buf);

    memcpy(buf, logo_pattern, 28); /* copy logo patterns */

    if (!animate) /* build the screen */
    {
        lcd_double_height(false);
        lcd_puts(0, 0, "[Charging]");
    }
    else          /* animate the logo */
    {
        for (i = 3; i < MIN(19, phase); i++)
        {
            if ((i - phase) % 5 == 0)
            {    /* draw a "bubble" here */
                ypos = (phase + i/5) % 9; /* "bounce" effect */
                if (ypos > 4)
                    ypos = 8 - ypos;
                buf[5 - ypos + 7 * (i/5)] |= 0x10 >> (i%5);
            }
        }
        phase++;
    }

    for (i = 0; i < 4; i++)
        lcd_define_pattern(logo_chars[i], buf + 7 * i);
}
#endif /* (not) HAVE_LCD_BITMAP */

/* blocks while charging, returns on event:
   1 if charger cable was removed
   2 if Off/Stop key was pressed
   3 if On key was pressed
   4 if USB was connected */
#if (CONFIG_KEYPAD==IPOD_3G_PAD) || (CONFIG_KEYPAD==IPOD_4G_PAD)
# define CHARGE_SCREEN_RESUME BUTTON_SELECT
#else
# define CHARGE_SCREEN_RESUME BUTTON_ON
#endif
int charging_screen(void)
{
    unsigned int button;
    int rc = 0;

    ide_power_enable(false); /* power down the disk, else would be spinning */

    lcd_clear_display();
    backlight_set_timeout(global_settings.backlight_timeout);
#ifdef HAVE_REMOTE_LCD
    remote_backlight_set_timeout(global_settings.remote_backlight_timeout);
#endif
    backlight_set_timeout_plugged(global_settings.backlight_timeout_plugged);
    gui_syncstatusbar_draw(&statusbars, true);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(true);
#endif
    charging_display_info(false);

    do
    {
        gui_syncstatusbar_draw(&statusbars, false);
        charging_display_info(true);
        button = button_get_w_tmo(HZ/3);
        if (button == CHARGE_SCREEN_RESUME)
            rc = 2;
        else if (usb_detect())
            rc = 3;
        else if (!charger_inserted())
            rc = 1;
    } while (!rc);

#ifdef HAVE_LCD_CHARCELLS
    logo_lock_patterns(false);
#endif
    return rc;
}
#endif /* CONFIG_CHARGING && !HAVE_POWEROFF_WHILE_CHARGING */

#if (CONFIG_KEYPAD != PLAYER_PAD)
/* returns:
   0 if no key was pressed
   1 if USB was connected */

void pitch_screen_draw(int pitch)
{
    unsigned char* ptr;
    unsigned char buf[32];
    int w, h;

    lcd_clear_display();

    /* UP: Pitch Up */
    ptr = str(LANG_SYSFONT_PITCH_UP);
    lcd_getstringsize(ptr,&w,&h);
    lcd_putsxy((LCD_WIDTH-w)/2, 0, ptr);
    lcd_mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                    LCD_WIDTH/2 - 3, h, 7, 8);

    /* DOWN: Pitch Down */
    ptr = str(LANG_SYSFONT_PITCH_DOWN);
    lcd_getstringsize(ptr,&w,&h);
    lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
    lcd_mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                    LCD_WIDTH/2 - 3, LCD_HEIGHT - h*2, 7, 8);

    /* RIGHT: +2% */
    ptr = "+2%";
    lcd_getstringsize(ptr,&w,&h);
    lcd_putsxy(LCD_WIDTH-w, (LCD_HEIGHT-h)/2, ptr);
    lcd_mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                    LCD_WIDTH-w-8, (LCD_HEIGHT-h)/2, 7, 8);

    /* LEFT: -2% */
    ptr = "-2%";
    lcd_getstringsize(ptr,&w,&h);
    lcd_putsxy(0, (LCD_HEIGHT-h)/2, ptr);
    lcd_mono_bitmap(bitmap_icons_7x8[Icon_FastBackward],
                    w+1, (LCD_HEIGHT-h)/2, 7, 8);

    /* "Pitch" */
    snprintf((char *)buf, sizeof(buf), str(LANG_SYSFONT_PITCH));
    lcd_getstringsize(buf,&w,&h);
    lcd_putsxy((LCD_WIDTH-w)/2, (LCD_HEIGHT/2)-h, buf);
    /* "XX.X%" */
    snprintf((char *)buf, sizeof(buf), "%d.%d%%",
             pitch / 10, pitch % 10 );
    lcd_getstringsize(buf,&w,&h);
    lcd_putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT/2, buf);

    lcd_update();
}

bool pitch_screen(void)
{
    int button;
    int pitch = sound_get_pitch();
    bool exit = false;

    lcd_setfont(FONT_SYSFIXED);
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(true);
#endif

    while (!exit)
    {
        pitch_screen_draw(pitch);

        button = button_get(true);
        switch (button) {
            case PITCH_UP:
                if ( pitch < 2000 )
                    pitch++;
                sound_set_pitch(pitch);
                break;

            case PITCH_UP | BUTTON_REPEAT:
                if ( pitch < 1990 )
                    pitch += 10;
                else
                    pitch = 2000;
                sound_set_pitch(pitch);
                break;

            case PITCH_DOWN:
                if ( pitch > 500 )
                    pitch--;
                sound_set_pitch(pitch);
                break;

            case PITCH_DOWN | BUTTON_REPEAT:
                if ( pitch > 510 )
                    pitch -= 10;
                else
                    pitch = 500;
                sound_set_pitch(pitch);
                break;

            case PITCH_RIGHT:
                if ( pitch < 1980 )
                {
                    pitch += 20;
                    sound_set_pitch(pitch);

                    pitch_screen_draw(pitch);

                    while(button != (PITCH_RIGHT|BUTTON_REL))
                        button = button_get(true);

                    pitch -= 20;
                    sound_set_pitch(pitch);
                }
                break;

            case PITCH_LEFT:
                if ( pitch > 520 )
                {
                    pitch -= 20;
                    sound_set_pitch(pitch);

                    pitch_screen_draw(pitch);

                    while(button != (PITCH_LEFT|BUTTON_REL))
                        button = button_get(true);;

                    pitch += 20;
                    sound_set_pitch(pitch);
                }
                break;

            case PITCH_RESET:
                pitch = 1000;
                sound_set_pitch( pitch );
                break;

            case PITCH_EXIT:
                exit = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
    }
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(false);
#endif
    lcd_setfont(FONT_UI);
    return 0;
}
#endif

#ifdef HAVE_QUICKSCREEN

#define bool_to_int(b)\
    b?1:0
#define int_to_bool(i)\
    i==0?false:true

void quick_screen_quick_apply(struct gui_quickscreen *qs)
{
    global_settings.playlist_shuffle=int_to_bool(qs->left_option->option);
    global_settings.dirfilter=qs->bottom_option->option;
    global_settings.repeat_mode=qs->right_option->option;
}

bool quick_screen_quick(int button_enter)
{
    bool res, oldshuffle;
    struct option_select left_option;
    struct option_select bottom_option;
    struct option_select right_option;
    int oldrepeat, old_x_margin, old_y_margin;

    static const struct opt_items left_items[] = {
        [0]={ STR(LANG_SYSFONT_OFF) },
        [1]={ STR(LANG_SYSFONT_ON) }
    };
    static const struct opt_items bottom_items[] = {
        [SHOW_ALL]={ STR(LANG_SYSFONT_FILTER_ALL) },
        [SHOW_SUPPORTED]={ STR(LANG_SYSFONT_FILTER_SUPPORTED) },
        [SHOW_MUSIC]={ STR(LANG_SYSFONT_FILTER_MUSIC) },
        [SHOW_PLAYLIST]={ STR(LANG_SYSFONT_FILTER_PLAYLIST) },
        [SHOW_ID3DB]={ STR(LANG_SYSFONT_FILTER_ID3DB) }
    };
    static const struct opt_items right_items[] = {
        [REPEAT_OFF]={ STR(LANG_SYSFONT_OFF) },
        [REPEAT_ALL]={ STR(LANG_SYSFONT_REPEAT_ALL) },
        [REPEAT_ONE]={ STR(LANG_SYSFONT_REPEAT_ONE) },
        [REPEAT_SHUFFLE]={ STR(LANG_SYSFONT_SHUFFLE) },
#if (AB_REPEAT_ENABLE == 1)
        [REPEAT_AB]={ STR(LANG_SYSFONT_REPEAT_AB) }
#endif
    };
    struct gui_quickscreen qs;

    old_x_margin = lcd_getxmargin();
    old_y_margin = lcd_getymargin();
    lcd_setmargins(0, 0);

    option_select_init_items(&left_option,
                             (char *)str(LANG_SYSFONT_SHUFFLE),
                             bool_to_int(global_settings.playlist_shuffle),
                             left_items,
                             2);
    option_select_init_items(&bottom_option,
                             (char *)str(LANG_SYSFONT_FILTER),
                             global_settings.dirfilter,
                             bottom_items,
                             sizeof(bottom_items)/sizeof(struct opt_items));
    option_select_init_items(&right_option,
                             (char *)str(LANG_SYSFONT_REPEAT),
                             global_settings.repeat_mode,
                             right_items,
                             sizeof(right_items)/sizeof(struct opt_items));

    gui_quickscreen_init(&qs, &left_option, &bottom_option, &right_option,
                         &quick_screen_quick_apply);
    oldrepeat=global_settings.repeat_mode;
    oldshuffle=global_settings.playlist_shuffle;
    res=gui_syncquickscreen_run(&qs, button_enter);
    if(!res)
    {
        if ( oldrepeat != global_settings.repeat_mode &&
            (audio_status() & AUDIO_STATUS_PLAY) )
            audio_flush_and_reload_tracks();
        if(oldshuffle != global_settings.playlist_shuffle
           && audio_status() & AUDIO_STATUS_PLAY)
        {
#if CONFIG_CODEC == SWCODEC
            dsp_set_replaygain(true);
#endif
            if (global_settings.playlist_shuffle)
                playlist_randomise(NULL, current_tick, true);
            else
                playlist_sort(NULL, true);
        }
        settings_save();
    }
    lcd_setmargins(old_x_margin, old_y_margin);
    return(res);
}

#ifdef BUTTON_F3
void quick_screen_f3_apply(struct gui_quickscreen *qs)
{
    global_settings.scrollbar=int_to_bool(qs->left_option->option);

    global_settings.flip_display=int_to_bool(qs->bottom_option->option);
    button_set_flip(global_settings.flip_display);
    lcd_set_flip(global_settings.flip_display);

    global_settings.statusbar=int_to_bool(qs->right_option->option);
    gui_syncstatusbar_draw(&statusbars, true);
}

bool quick_screen_f3(int button_enter)
{
    bool res;
    struct option_select left_option;
    struct option_select bottom_option;
    struct option_select right_option;
    int old_x_margin, old_y_margin;

    static const struct opt_items onoff_items[] = {
        [0]={ STR(LANG_SYSFONT_OFF) },
        [1]={ STR(LANG_SYSFONT_ON) }
    };
    static const struct opt_items yesno_items[] = {
        [0]={ STR(LANG_SYSFONT_SET_BOOL_NO) },
        [1]={ STR(LANG_SYSFONT_SET_BOOL_YES) }
    };

    struct gui_quickscreen qs;

    old_x_margin = lcd_getxmargin();
    old_y_margin = lcd_getymargin();
    lcd_setmargins(0, 0);

    option_select_init_items(&left_option,
                             str(LANG_SYSFONT_SCROLL_BAR),
                             bool_to_int(global_settings.scrollbar),
                             onoff_items,
                             2);
    option_select_init_items(&bottom_option,
                             str(LANG_SYSFONT_FLIP_DISPLAY),
                             bool_to_int(global_settings.flip_display),
                             yesno_items,
                             2);
    option_select_init_items(&right_option,
                             str(LANG_SYSFONT_STATUS_BAR),
                             bool_to_int(global_settings.statusbar),
                             onoff_items,
                             2);
    gui_quickscreen_init(&qs, &left_option, &bottom_option, &right_option,
                         &quick_screen_f3_apply);
    res=gui_syncquickscreen_run(&qs, button_enter);
    if(!res)
        settings_save();
    lcd_setmargins(old_x_margin, old_y_margin);
    return(res);
}
#endif /* BUTTON_F3 */
#endif /* CONFIG_KEYPAD in (RECORDER_PAD |IRIVER_H100_PAD | IRIVER_H300_PAD) */

#if defined(CONFIG_CHARGING) || defined(SIMULATOR)
void charging_splash(void)
{
    gui_syncsplash(2*HZ, true, (unsigned char *)str(LANG_BATTERY_CHARGE));
    button_clear_queue();
}
#endif


#if defined(HAVE_LCD_BITMAP) && defined (CONFIG_RTC)

const int dayname[] = {
    LANG_WEEKDAY_SUNDAY,
    LANG_WEEKDAY_MONDAY,
    LANG_WEEKDAY_TUESDAY,
    LANG_WEEKDAY_WEDNESDAY,
    LANG_WEEKDAY_THURSDAY,
    LANG_WEEKDAY_FRIDAY,
    LANG_WEEKDAY_SATURDAY
};

const int monthname[] = {
    LANG_MONTH_JANUARY,
    LANG_MONTH_FEBRUARY,
    LANG_MONTH_MARCH,
    LANG_MONTH_APRIL,
    LANG_MONTH_MAY,
    LANG_MONTH_JUNE,
    LANG_MONTH_JULY,
    LANG_MONTH_AUGUST,
    LANG_MONTH_SEPTEMBER,
    LANG_MONTH_OCTOBER,
    LANG_MONTH_NOVEMBER,
    LANG_MONTH_DECEMBER
};

/* little helper function for voice output */
static void say_time(int cursorpos, const struct tm *tm)
{
    static const int unit[] = { UNIT_HOUR, UNIT_MIN, UNIT_SEC, 0, 0, 0 };
    int value = 0;

    if (!global_settings.talk_menu)
        return;

    switch(cursorpos)
    {
    case 0:
        value = tm->tm_hour;
        break;
    case 1:
        value = tm->tm_min;
        break;
    case 2:
        value = tm->tm_sec;
        break;
    case 3:
        value = tm->tm_year + 1900;
        break;
    case 5:
        value = tm->tm_mday;
        break;
    }

    if (cursorpos == 4) /* month */
        talk_id(LANG_MONTH_JANUARY + tm->tm_mon, false);
    else
        talk_value(value, unit[cursorpos], false);
}


#define INDEX_X 0
#define INDEX_Y 1
#define INDEX_WIDTH 2
bool set_time_screen(const char* string, struct tm *tm)
{
    bool done = false;
    int button;
    int min = 0, steps = 0;
    int cursorpos = 0;
    int lastcursorpos = !cursorpos;
    unsigned char buffer[19];
    int realyear;
    int julianday;
    int i;
    unsigned char reffub[5];
    unsigned int width, height;
    unsigned int separator_width, weekday_width;
    unsigned int line_height, prev_line_height;
    int lastmode = lcd_get_drawmode();

    char cursor[][3] = {{ 0,  8, 12}, {18,  8, 12}, {36,  8, 12},
                        {24, 16, 24}, {54, 16, 18}, {78, 16, 12}};
    char daysinmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    int monthname_len = 0, dayname_len = 0;

    int *valptr = NULL;

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts_scroll(0, 0, string);

    while ( !done ) {
        /* calculate the number of days in febuary */
        realyear = tm->tm_year + 1900;
        if((realyear % 4 == 0 && !(realyear % 100 == 0)) || realyear % 400 == 0)
            daysinmonth[1] = 29;
        else
            daysinmonth[1] = 28;

        /* fix day if month or year changed */
        if (tm->tm_mday > daysinmonth[tm->tm_mon])
            tm->tm_mday = daysinmonth[tm->tm_mon];

        /* calculate day of week */
        julianday = 0;
        for(i = 0; i < tm->tm_mon; i++) {
           julianday += daysinmonth[i];
        }
        julianday += tm->tm_mday;
        tm->tm_wday = (realyear + julianday + (realyear - 1) / 4 -
                       (realyear - 1) / 100 + (realyear - 1) / 400 + 7 - 1) % 7;

        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d ",
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
        lcd_puts(0, 1, buffer);

        /* recalculate the positions and offsets */
        lcd_getstringsize(string, &width, &prev_line_height);
        lcd_getstringsize(buffer, &width, &line_height);
        lcd_getstringsize(":", &separator_width, &height);

        /* hour */
        strncpy(reffub, buffer, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[0][INDEX_X] = 0;
        cursor[0][INDEX_Y] = prev_line_height;
        cursor[0][INDEX_WIDTH] = width;

        /* minute */
        strncpy(reffub, buffer + 3, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[1][INDEX_X] = cursor[0][INDEX_WIDTH] + separator_width;
        cursor[1][INDEX_Y] = prev_line_height;
        cursor[1][INDEX_WIDTH] = width;

        /* second */
        strncpy(reffub, buffer + 6, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[2][INDEX_X] = cursor[0][INDEX_WIDTH] + separator_width +
                             cursor[1][INDEX_WIDTH] + separator_width;
        cursor[2][INDEX_Y] = prev_line_height;
        cursor[2][INDEX_WIDTH] = width;

        lcd_getstringsize(buffer, &width, &prev_line_height);

        snprintf(buffer, sizeof(buffer), "%s %04d %s %02d ",
                 str(dayname[tm->tm_wday]), tm->tm_year+1900,
                 str(monthname[tm->tm_mon]), tm->tm_mday);
        lcd_puts(0, 2, buffer);

        /* recalculate the positions and offsets */
        lcd_getstringsize(buffer, &width, &line_height);

        /* store these 2 to prevent _repeated_ strlen calls */
        monthname_len = strlen(str(monthname[tm->tm_mon]));
        dayname_len = strlen(str(dayname[tm->tm_wday]));

        /* weekday */
        strncpy(reffub, buffer, dayname_len);
        reffub[dayname_len] = '\0';
        lcd_getstringsize(reffub, &weekday_width, &height);
        lcd_getstringsize(" ", &separator_width, &height);

        /* year */
        strncpy(reffub, buffer + dayname_len + 1, 4);
        reffub[4] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[3][INDEX_X] = weekday_width + separator_width;
        cursor[3][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[3][INDEX_WIDTH] = width;

        /* month */
        strncpy(reffub, buffer + dayname_len + 6, monthname_len);
        reffub[monthname_len] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[4][INDEX_X] = weekday_width + separator_width +
                             cursor[3][INDEX_WIDTH] + separator_width;
        cursor[4][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[4][INDEX_WIDTH] = width;

        /* day */
        strncpy(reffub, buffer + dayname_len + monthname_len + 7, 2);
        reffub[2] = '\0';
        lcd_getstringsize(reffub, &width, &height);
        cursor[5][INDEX_X] = weekday_width + separator_width +
                             cursor[3][INDEX_WIDTH] + separator_width +
                             cursor[4][INDEX_WIDTH] + separator_width;
        cursor[5][INDEX_Y] = cursor[0][INDEX_Y] + prev_line_height;
        cursor[5][INDEX_WIDTH] = width;

        lcd_set_drawmode(DRMODE_COMPLEMENT);
        lcd_fillrect(cursor[cursorpos][INDEX_X],
                     cursor[cursorpos][INDEX_Y] + lcd_getymargin(),
                     cursor[cursorpos][INDEX_WIDTH],
                     line_height);
        lcd_set_drawmode(DRMODE_SOLID);

        lcd_puts(0, 4, str(LANG_TIME_SET));
        lcd_puts(0, 5, str(LANG_TIME_REVERT));
#ifdef HAVE_LCD_BITMAP
        gui_syncstatusbar_draw(&statusbars, true);
#endif
        lcd_update();

        /* calculate the minimum and maximum for the number under cursor */
        if(cursorpos!=lastcursorpos) {
            lastcursorpos=cursorpos;
            switch(cursorpos) {
                case 0: /* hour */
                    min = 0;
                    steps = 24;
                    valptr = &tm->tm_hour;
                    break;
                case 1: /* minute */
                    min = 0;
                    steps = 60;
                    valptr = &tm->tm_min;
                    break;
                case 2: /* second */
                    min = 0;
                    steps = 60;
                    valptr = &tm->tm_sec;
                    break;
                case 3: /* year */
                    min = 1;
                    steps = 200;
                    valptr = &tm->tm_year;
                    break;
                case 4: /* month */
                    min = 0;
                    steps = 12;
                    valptr = &tm->tm_mon;
                    break;
                case 5: /* day */
                    min = 1;
                    steps = daysinmonth[tm->tm_mon];
                    valptr = &tm->tm_mday;
                    break;
            }
            say_time(cursorpos, tm);
        }

        button = button_get_w_tmo(HZ/2);
        switch ( button ) {
            case SETTINGS_PREV:
                cursorpos = (cursorpos + 6 - 1) % 6;
                break;
            case SETTINGS_NEXT:
                cursorpos = (cursorpos + 6 + 1) % 6;
                break;
            case SETTINGS_INC:
            case SETTINGS_INC | BUTTON_REPEAT:
                *valptr = (*valptr + steps - min + 1) %
                    steps + min;
                if(*valptr == 0)
                    *valptr = min;
                say_time(cursorpos, tm);
                break;
            case SETTINGS_DEC:
            case SETTINGS_DEC | BUTTON_REPEAT:
                *valptr = (*valptr + steps - min - 1) %
                    steps + min;
                if(*valptr == 0)
                    *valptr = min;
                say_time(cursorpos, tm);
                break;

            case SETTINGS_ACCEPT:
                done = true;
                break;

            case SETTINGS_CANCEL:
                done = true;
                tm->tm_year = -1;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    lcd_set_drawmode(lastmode);
    return false;
}
#endif /* defined(HAVE_LCD_BITMAP) && defined (CONFIG_RTC) */

#if (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
bool shutdown_screen(void)
{
    int button;
    bool done = false;

    lcd_stop_scroll();

    gui_syncsplash(0, true, str(LANG_CONFIRM_SHUTDOWN));

    while(!done)
    {
        button = button_get_w_tmo(HZ*2);
        switch(button)
        {
            case BUTTON_OFF:
                sys_poweroff();
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;

                /* Return if any other button was pushed, or if there
                   was a timeout. We ignore RELEASE events, since we may
                   have been called by a button down event, and the user might
                   not have released the button yet.
                   We also ignore REPEAT events, since we don't want to
                   remove the splash when the user holds OFF to shut down. */
                if(!(button & (BUTTON_REL | BUTTON_REPEAT)))
                   done = true;
                break;
        }
    }
    return false;
}
#endif

#if CONFIG_CODEC == SWCODEC
#define ID3_ITEMS   13
#else
#define ID3_ITEMS   11
#endif

char * id3_get_info(int selected_item, void* data, char *buffer)
{
    struct mp3entry* id3 =(struct mp3entry*)data;
    int info_no=selected_item/2;
    if(!(selected_item%2))
    {/* header */
        int headers[]=
        {
            LANG_ID3_TITLE,
            LANG_ID3_ARTIST,
            LANG_ID3_ALBUM,
            LANG_ID3_TRACKNUM,
            LANG_ID3_GENRE,
            LANG_ID3_YEAR,
            LANG_ID3_LENGTH,
            LANG_ID3_PLAYLIST,
            LANG_ID3_BITRATE,
            LANG_ID3_FRECUENCY,
#if CONFIG_CODEC == SWCODEC
            LANG_ID3_TRACK_GAIN,
            LANG_ID3_ALBUM_GAIN,
#endif
            LANG_ID3_PATH,
        };
        return( str(headers[info_no]));
    }
    else
    {/* data */

        char * info=NULL;
        switch(info_no)
        {
            case 0:/*LANG_ID3_TITLE*/
                info=id3->title;
                break;
            case 1:/*LANG_ID3_ARTIST*/
                info=id3->artist;
                break;
            case 2:/*LANG_ID3_ALBUM*/
                info=id3->album;
                break;
            case 3:/*LANG_ID3_TRACKNUM*/
                if (id3->track_string)
                    info = id3->track_string;
                else if (id3->tracknum)
                {
                    snprintf(buffer, MAX_PATH, "%d", id3->tracknum);
                    info = buffer;
                }
                break;
            case 4:/*LANG_ID3_GENRE*/
                info = id3_get_genre(id3);
                break;
            case 5:/*LANG_ID3_YEAR*/
                if (id3->year_string)
                    info = id3->year_string;
                else if (id3->year)
                {
                    snprintf(buffer, MAX_PATH, "%d", id3->year);
                    info = buffer;
                }
                break;
            case 6:/*LANG_ID3_LENGTH*/
                gui_wps_format_time(buffer, MAX_PATH, id3->length);
                info=buffer;
                break;
            case 7:/*LANG_ID3_PLAYLIST*/
                snprintf(buffer, MAX_PATH, "%d/%d", playlist_get_display_index(),
            playlist_amount());
                info=buffer;
                break;
            case 8:/*LANG_ID3_BITRATE*/
                snprintf(buffer, MAX_PATH, "%d kbps%s", id3->bitrate,
            id3->vbr ? str(LANG_ID3_VBR) : (const unsigned char*) "");
                info=buffer;
                break;
            case 9:/*LANG_ID3_FRECUENCY*/
                snprintf(buffer, MAX_PATH, "%ld Hz", id3->frequency);
                info=buffer;
                break;
#if CONFIG_CODEC == SWCODEC
            case 10:/*LANG_ID3_TRACK_GAIN*/
                info=id3->track_gain_string;
                break;
            case 11:/*LANG_ID3_ALBUM_GAIN*/
                info=id3->album_gain_string;
                break;
            case 12:/*LANG_ID3_PATH*/
#else
            case 10:/*LANG_ID3_PATH*/
#endif
                info=id3->path;
                break;
        }
        if(info==NULL)
            return(str(LANG_ID3_NO_INFO));
        return(info);
    }
}

bool browse_id3(void)
{
    struct gui_synclist id3_lists;
    struct mp3entry* id3 = audio_current_track();
    int key;

    gui_synclist_init(&id3_lists, &id3_get_info, id3, true, 2);
    gui_synclist_set_nb_items(&id3_lists, ID3_ITEMS*2);
    gui_synclist_draw(&id3_lists);
    while (true) {
        key = button_get_w_tmo(HZ/2);
        if(key!=BUTTON_NONE && !(key&BUTTON_REL) && !gui_synclist_do_button(&id3_lists, key))
            return(default_event_handler(key) == SYS_USB_CONNECTED);
        gui_syncstatusbar_draw(&statusbars, false);
    }
}


bool set_rating(void)
{
    struct mp3entry* id3 = audio_current_track();
    int button;
    bool exit = false;
    char rating_text[20];

    if (!(audio_status() & AUDIO_STATUS_PLAY)||id3==NULL)
        return false;
    while (!exit)
    {
        lcd_clear_display();
        lcd_puts(0, 0, str(LANG_RATING));
        snprintf(rating_text, sizeof(rating_text), "%d", id3->rating);
        lcd_puts(0, 1, (unsigned char *)rating_text);
        lcd_update();
        button = button_get(true);

        switch(button)
        {
            case SETTINGS_DEC:
                if (id3->rating > 0)
                    id3->rating--;
                else
                    id3->rating = 10;
                break;

            case SETTINGS_INC:
                if (id3->rating < 10)
                    id3->rating++;
                else
                    id3->rating = 0;
                break;
            case SETTINGS_CANCEL:
#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
                /* eat release event */
                button_get(true);
                exit = true;
                break;

            default:
                if(default_event_handler(button) ==  SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
    return false;
}
