/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "system.h"
#include "lcd.h"
#include "led.h"
#include "mpeg.h"
#include "audio.h"
#if CONFIG_CODEC == SWCODEC
#include "pcm_record.h"
#endif
#ifdef HAVE_UDA1380
#include "uda1380.h"
#endif

#include "mp3_playback.h"
#include "mas.h"
#include "button.h"
#include "kernel.h"
#include "settings.h"
#include "lang.h"
#include "font.h"
#include "icons.h"
#include "icon.h"
#include "screens.h"
#include "peakmeter.h"
#include "statusbar.h"
#include "menu.h"
#include "sound_menu.h"
#include "timefuncs.h"
#include "debug.h"
#include "misc.h"
#include "tree.h"
#include "string.h"
#include "dir.h"
#include "errno.h"
#include "talk.h"
#include "atoi.h"
#include "sound.h"
#include "ata.h"
#include "splash.h"
#include "screen_access.h"
#ifdef HAVE_RECORDING


#if CONFIG_KEYPAD == RECORDER_PAD
#define REC_SHUTDOWN (BUTTON_OFF | BUTTON_REPEAT)
#define REC_STOPEXIT BUTTON_OFF
#define REC_RECPAUSE BUTTON_PLAY
#define REC_INC BUTTON_RIGHT
#define REC_DEC BUTTON_LEFT
#define REC_NEXT BUTTON_DOWN
#define REC_PREV BUTTON_UP
#define REC_SETTINGS BUTTON_F1
#define REC_F2 BUTTON_F2
#define REC_F3 BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD /* only limited features */
#define REC_SHUTDOWN (BUTTON_OFF | BUTTON_REPEAT)
#define REC_STOPEXIT BUTTON_OFF
#define REC_RECPAUSE_PRE BUTTON_MENU
#define REC_RECPAUSE (BUTTON_MENU | BUTTON_REL)
#define REC_INC BUTTON_RIGHT
#define REC_DEC BUTTON_LEFT
#define REC_NEXT BUTTON_DOWN
#define REC_PREV BUTTON_UP
#define REC_SETTINGS (BUTTON_MENU | BUTTON_REPEAT) 

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define REC_SHUTDOWN (BUTTON_OFF | BUTTON_REPEAT)
#define REC_STOPEXIT BUTTON_OFF
#define REC_RECPAUSE BUTTON_ON
#define REC_NEWFILE BUTTON_REC
#define REC_INC BUTTON_RIGHT
#define REC_DEC BUTTON_LEFT
#define REC_NEXT BUTTON_DOWN
#define REC_PREV BUTTON_UP
#define REC_SETTINGS BUTTON_MODE

#define REC_RC_SHUTDOWN (BUTTON_RC_STOP | BUTTON_REPEAT)
#define REC_RC_STOPEXIT BUTTON_RC_STOP
#define REC_RC_RECPAUSE BUTTON_RC_ON
#define REC_RC_NEWFILE BUTTON_RC_REC
#define REC_RC_INC BUTTON_RC_BITRATE
#define REC_RC_DEC BUTTON_RC_SOURCE
#define REC_RC_NEXT BUTTON_RC_FF
#define REC_RC_PREV BUTTON_RC_REW
#define REC_RC_SETTINGS BUTTON_RC_MODE

#elif CONFIG_KEYPAD == GMINI100_PAD
#define REC_SHUTDOWN (BUTTON_OFF | BUTTON_REPEAT)
#define REC_STOPEXIT BUTTON_OFF
#define REC_RECPAUSE BUTTON_ON
#define REC_INC BUTTON_RIGHT
#define REC_DEC BUTTON_LEFT
#endif

#define PM_HEIGHT ((LCD_HEIGHT >= 72) ? 2 : 1)

bool f2_rec_screen(void);
bool f3_rec_screen(void);

#define SOURCE_MIC   0
#define SOURCE_LINE  1
#ifdef HAVE_SPDIF_IN
#define SOURCE_SPDIF 2
#define MAX_SOURCE   SOURCE_SPDIF
#else
#define MAX_SOURCE   SOURCE_LINE
#endif

#if CONFIG_CODEC == SWCODEC
#define REC_FILE_ENDING ".wav"
#else
#define REC_FILE_ENDING ".mp3"
#endif

#define MAX_FILE_SIZE 0x7F800000 /* 2 GB - 4 MB */

const char* const freq_str[6] =
{
    "44.1kHz",
    "48kHz",
    "32kHz",
    "22.05kHz",
    "24kHz",
    "16kHz"
};

static void set_gain(void)
{
    if(global_settings.rec_source == SOURCE_MIC)
    {
        audio_set_recording_gain(global_settings.rec_mic_gain,
                                 0, AUDIO_GAIN_MIC);
    }
    else
    {
        audio_set_recording_gain(global_settings.rec_left_gain,
                                 global_settings.rec_right_gain,
                                 AUDIO_GAIN_LINEIN);
    }
}

static const char* const fmtstr[] =
{
    "%c%d %s",            /* no decimals */
    "%c%d.%d %s ",        /* 1 decimal */
    "%c%d.%02d %s "       /* 2 decimals */
};

char *fmt_gain(int snd, int val, char *str, int len)
{
    int i, d, numdec;
    const char *unit;
    char sign = ' ';

    val = sound_val2phys(snd, val);
    if(val < 0)
    {
        sign = '-';
        val = -val;
    }
    numdec = sound_numdecimals(snd);
    unit = sound_unit(snd);
    
    if(numdec)
    {
        i = val / (10*numdec);
        d = val % (10*numdec);
        snprintf(str, len, fmtstr[numdec], sign, i, d, unit);
    }
    else
        snprintf(str, len, fmtstr[numdec], sign, val, unit);
    
    return str;
}

static int cursor;

void adjust_cursor(void)
{
    int max_cursor;
    
    if(cursor < 0)
        cursor = 0;

    switch(global_settings.rec_source)
    {
    case SOURCE_MIC:
        max_cursor = 1;
        break;
    case SOURCE_LINE:
        max_cursor = 3;
        break;
    default:
        max_cursor = 0;
        break;
    }

    if(cursor > max_cursor)
        cursor = max_cursor;
}

char *rec_create_filename(char *buffer)
{
    if(global_settings.rec_directory)
        getcwd(buffer, MAX_PATH);
    else
        strncpy(buffer, rec_base_directory, MAX_PATH);

#ifdef CONFIG_RTC 
    create_datetime_filename(buffer, buffer, "R", REC_FILE_ENDING);
#else
    create_numbered_filename(buffer, buffer, "rec_", REC_FILE_ENDING, 4);
#endif
    return buffer;
}

int rec_create_directory(void)
{
    int rc;
    
    /* Try to create the base directory if needed */
    if(global_settings.rec_directory == 0)
    {
        rc = mkdir(rec_base_directory, 0);
        if(rc < 0 && errno != EEXIST)
        {
            gui_syncsplash(HZ * 2, true,
                   "Can't create the %s directory. Error code %d.",
                   rec_base_directory, rc);
            return -1;
        }
        else
        {
            /* If we have created the directory, we want the dir browser to
               be refreshed even if we haven't recorded anything */
            if(errno != EEXIST)
                return 1;
        }
    }
    return 0;
}

static char path_buffer[MAX_PATH];

/* used in trigger_listerner and recording_screen */
static unsigned int last_seconds = 0;

/**
 * Callback function so that the peak meter code can send an event
 * to this application. This function can be passed to
 * peak_meter_set_trigger_listener in order to activate the trigger.
 */
static void trigger_listener(int trigger_status)
{
    switch (trigger_status)
    {
        case TRIG_GO:
            if((audio_status() & AUDIO_STATUS_RECORD) != AUDIO_STATUS_RECORD)
            {
                talk_buffer_steal(); /* we use the mp3 buffer */
                audio_record(rec_create_filename(path_buffer));

                /* give control to mpeg thread so that it can start recording*/
                yield(); yield(); yield();
            }

            /* if we're already recording this is a retrigger */
            else
            {
                audio_new_file(rec_create_filename(path_buffer));
                /* tell recording_screen to reset the time */
                last_seconds = 0;
            }
            break;

        /* A _change_ to TRIG_READY means the current recording has stopped */
        case TRIG_READY:
            if(audio_status() & AUDIO_STATUS_RECORD)
            {
                audio_stop();
                if (global_settings.rec_trigger_mode != TRIG_MODE_REARM)
                {
                    peak_meter_set_trigger_listener(NULL);
                    peak_meter_trigger(false);
                }
            }
            break;
    }
}

bool recording_screen(void)
{
    long button;
    long lastbutton = BUTTON_NONE;
    bool done = false;
    char buf[32];
    char buf2[32];
    int w, h;
    int update_countdown = 1;
    bool have_recorded = false;
    unsigned int seconds;
    int hours, minutes;
    char path_buffer[MAX_PATH];
    char filename[13];
    bool been_in_usb_mode = false;
    int last_audio_stat = -1;
    int audio_stat;
#if CONFIG_LED == LED_REAL
    bool led_state = false;
    int led_countdown = 2;
#endif
    int i;
    int filename_offset[NB_SCREENS];
    int pm_y[NB_SCREENS];

    const unsigned char *byte_units[] = {
        ID2P(LANG_BYTE),
        ID2P(LANG_KILOBYTE),
        ID2P(LANG_MEGABYTE),
        ID2P(LANG_GIGABYTE)
    };

    cursor = 0;
#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(false);
#endif

#ifndef SIMULATOR
    audio_init_recording(talk_get_bufsize());
#else
    audio_init_recording(0);
#endif

    sound_set_volume(global_settings.volume);

#if CONFIG_CODEC == SWCODEC
    audio_stop();
    /* Set peak meter to recording mode */
    peak_meter_playback(false);

#if defined(HAVE_SPDIF_IN) && !defined(SIMULATOR)
    if (global_settings.rec_source == SOURCE_SPDIF)
        cpu_boost(true);
#endif

#else
    /* Yes, we use the D/A for monitoring */
    peak_meter_playback(true);
#endif
    peak_meter_enabled = true;

#if CONFIG_CODEC != SWCODEC
    if (global_settings.rec_prerecord_time)
#endif
        talk_buffer_steal(); /* will use the mp3 buffer */

#if defined(HAVE_SPDIF_POWER) && !defined(SIMULATOR)
    /* Tell recording whether we want S/PDIF power enabled at all times */
    audio_set_spdif_power_setting(global_settings.spdif_enable);
#endif

    audio_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels,
                               global_settings.rec_editable,
                               global_settings.rec_prerecord_time);

    set_gain();

    settings_apply_trigger();

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("M", &w, &h);
        screens[i].setmargins(global_settings.invert_cursor ? 0 : w, 8);
        filename_offset[i] = ((screens[i].height >= 80) ? 1 : 0);
        pm_y[i] = 8 + h * (2 + filename_offset[i]);
    }
    
    if(rec_create_directory() > 0)
        have_recorded = true;

    while(!done)
    {
#if CONFIG_CODEC == SWCODEC        
        audio_stat = pcm_rec_status();
#else
        audio_stat = audio_status();
#endif
        
#if CONFIG_LED == LED_REAL

        /*
         * Flash the LED while waiting to record.  Turn it on while
         * recording.
         */
        if(audio_stat & AUDIO_STATUS_RECORD)
        {
            if (audio_stat & AUDIO_STATUS_PAUSE)
            {
                if (--led_countdown <= 0)
                {
                    led_state = !led_state;
                    led(led_state);
                    led_countdown = 2;
                }
            }
            else
            {
                /* trigger is on in status TRIG_READY (no check needed) */
                led(true);
            }
        }
        else
        {
            int trigStat = peak_meter_trigger_status();
            /*
             * other trigger stati than trig_off and trig_steady
             * already imply that we are recording.
             */
            if (trigStat == TRIG_STEADY)
            {
                if (--led_countdown <= 0)
                {
                    led_state = !led_state;
                    led(led_state);
                    led_countdown = 2;
                }
            }
            else
            {
                /* trigger is on in status TRIG_READY (no check needed) */
                led(false);
            }
        }
#endif /* CONFIG_LED */

        /* Wait for a button a while (HZ/10) drawing the peak meter */
        button = peak_meter_draw_get_btn(0, pm_y, h * PM_HEIGHT);

        if (last_audio_stat != audio_stat)
        {
            if (audio_stat == AUDIO_STATUS_RECORD)
            {
                have_recorded = true;
            }
            last_audio_stat = audio_stat;
        }

        switch(button)
        {
            case REC_STOPEXIT:
            case REC_SHUTDOWN:
#ifdef REC_RC_STOPEXIT
            case REC_RC_STOPEXIT:
#endif
#ifdef REC_RC_SHUTDOWN
            case REC_RC_SHUTDOWN:
#endif          
                /* turn off the trigger */
                peak_meter_trigger(false);
                peak_meter_set_trigger_listener(NULL);

                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    audio_stop_recording();
                }
                else
                {
                    peak_meter_playback(true);
#if CONFIG_CODEC != SWCODEC
                    peak_meter_enabled = false;
#endif                    
                    done = true;
                }
                update_countdown = 1; /* Update immediately */
                break;

            case REC_RECPAUSE:
#ifdef REC_RC_RECPAUSE
            case REC_RC_RECPAUSE:
#endif
#ifdef REC_NEWFILE            
            case REC_NEWFILE:
#endif
#ifdef REC_RC_NEWFILE
            case REC_RC_NEWFILE:
#endif
#ifdef REC_RECPAUSE_PRE
                if (lastbutton != REC_RECPAUSE_PRE)
                    break;
#endif
                /* Only act if the mpeg is stopped */
                if(!(audio_stat & AUDIO_STATUS_RECORD))
                {
                    /* is this manual or triggered recording? */
                    if ((global_settings.rec_trigger_mode == TRIG_MODE_OFF) ||
                        (peak_meter_trigger_status() != TRIG_OFF))
                    {
                        /* manual recording */
                        have_recorded = true;
                        talk_buffer_steal(); /* we use the mp3 buffer */
                        audio_record(rec_create_filename(path_buffer));
                        last_seconds = 0;
                        if (global_settings.talk_menu)
                        {   /* no voice possible here, but a beep */
                            audio_beep(HZ/2); /* longer beep on start */
                        }
                    }

                    /* this is triggered recording */
                    else
                    {
                        /* we don't start recording now, but enable the
                           trigger and let the callback function
                           trigger_listener control when the recording starts */
                        peak_meter_trigger(true);
                        peak_meter_set_trigger_listener(&trigger_listener);
                    }
                }
                else
                {
#ifdef REC_NEWFILE
                    /*if new file button pressed, start new file */
                    if ((button == REC_NEWFILE)
#ifdef REC_RC_NEWFILE
                          || (button == REC_RC_NEWFILE)
#endif
                                )
                    {
                        audio_new_file(rec_create_filename(path_buffer));
                        last_seconds = 0;
                    }
                    else
#endif
                    /* if pause button pressed, pause or resume */
                    {
                        if(audio_stat & AUDIO_STATUS_PAUSE)
                        {
                            audio_resume_recording();
                            if (global_settings.talk_menu)
                            {   /* no voice possible here, but a beep */
                                audio_beep(HZ/4); /* short beep on resume */
                            }
                        }
                        else
                        {
                            audio_pause_recording();
                        }
                    }
                }
                update_countdown = 1; /* Update immediately */
                break;

#ifdef REC_PREV
            case REC_PREV:
#ifdef REC_RC_PREV
            case REC_RC_PREV:
#endif
                cursor--;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;
#endif

#ifdef REC_NEXT
            case REC_NEXT:
#ifdef REC_RC_NEXT
            case REC_RC_NEXT:
#endif
                cursor++;
                adjust_cursor();
                update_countdown = 1; /* Update immediately */
                break;
#endif
       
            case REC_INC:
            case REC_INC | BUTTON_REPEAT:
#ifdef REC_RC_INC
            case REC_RC_INC:
            case REC_RC_INC | BUTTON_REPEAT:
#endif
                switch(cursor)
                {
                    case 0:
                        if(global_settings.volume <
                           sound_max(SOUND_VOLUME))
                            global_settings.volume++;
                        sound_set_volume(global_settings.volume);
                        break;
                    case 1:
                        if(global_settings.rec_source == SOURCE_MIC)
                        {
                            if(global_settings.rec_mic_gain <
                               sound_max(SOUND_MIC_GAIN))
                            global_settings.rec_mic_gain++;
                        }
                        else
                        {
                            if(global_settings.rec_left_gain <
                               sound_max(SOUND_LEFT_GAIN))
                                global_settings.rec_left_gain++;
                            if(global_settings.rec_right_gain <
                               sound_max(SOUND_RIGHT_GAIN))
                                global_settings.rec_right_gain++;
                        }
                        break;
                    case 2:
                        if(global_settings.rec_left_gain <
                           sound_max(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain++;
                        break;
                    case 3:
                        if(global_settings.rec_right_gain <
                           sound_max(SOUND_RIGHT_GAIN))
                            global_settings.rec_right_gain++;
                        break;
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
            case REC_DEC:
            case REC_DEC | BUTTON_REPEAT:
#ifdef REC_RC_DEC
            case REC_RC_DEC:
            case REC_RC_DEC | BUTTON_REPEAT:
#endif
                switch(cursor)
                {
                    case 0:
                        if(global_settings.volume >
                           sound_min(SOUND_VOLUME))
                            global_settings.volume--;
                        sound_set_volume(global_settings.volume);
                        break;
                    case 1:
                        if(global_settings.rec_source == SOURCE_MIC)
                        {
                            if(global_settings.rec_mic_gain >
                               sound_min(SOUND_MIC_GAIN))
                                global_settings.rec_mic_gain--;
                        }
                        else
                        {
                            if(global_settings.rec_left_gain >
                               sound_min(SOUND_LEFT_GAIN))
                                global_settings.rec_left_gain--;
                            if(global_settings.rec_right_gain >
                               sound_min(SOUND_RIGHT_GAIN))
                                global_settings.rec_right_gain--;
                        }
                        break;
                    case 2:
                        if(global_settings.rec_left_gain >
                           sound_min(SOUND_LEFT_GAIN))
                            global_settings.rec_left_gain--;
                        break;
                    case 3:
                        if(global_settings.rec_right_gain >
                           sound_min(SOUND_RIGHT_GAIN))
                            global_settings.rec_right_gain--;
                        break;
                }
                set_gain();
                update_countdown = 1; /* Update immediately */
                break;
                
#ifdef REC_SETTINGS
            case REC_SETTINGS:
#ifdef REC_RC_SETTINGS
            case REC_RC_SETTINGS:
#endif
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
#if CONFIG_LED == LED_REAL
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (recording_menu(false))
                    {
                        return SYS_USB_CONNECTED;
                    }
                    settings_save();

#if CONFIG_CODEC != SWCODEC
                    if (global_settings.rec_prerecord_time)
#endif
                        talk_buffer_steal(); /* will use the mp3 buffer */

                    audio_set_recording_options(global_settings.rec_frequency,
                                               global_settings.rec_quality,
                                               global_settings.rec_source,
                                               global_settings.rec_channels,
                                               global_settings.rec_editable,
                                               global_settings.rec_prerecord_time);

                    adjust_cursor();
                    set_gain();
                    update_countdown = 1; /* Update immediately */

                    FOR_NB_SCREENS(i)
                    {
                        screens[i].setfont(FONT_SYSFIXED);
                        screens[i].setmargins(global_settings.invert_cursor ? 0 : w, 8);
                    }
                }
                break;
#endif

#ifdef REC_F2
            case REC_F2:
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
#if CONFIG_LED == LED_REAL
                    /* led is restored at begin of loop / end of function */
                    led(false);
#endif
                    if (f2_rec_screen())
                    {
                        have_recorded = true;
                        done = true;
                    }
                    else
                        update_countdown = 1; /* Update immediately */
                }
                break;
#endif

#ifdef REC_F3
            case REC_F3:
                if(audio_stat & AUDIO_STATUS_RECORD)
                {
                    audio_new_file(rec_create_filename(path_buffer));
                    last_seconds = 0;
                }
                else
                {
                    if(audio_stat != AUDIO_STATUS_RECORD)
                    {
#if CONFIG_LED == LED_REAL
                        /* led is restored at begin of loop / end of function */
                        led(false);
#endif
                        if (f3_rec_screen())
                        {
                            have_recorded = true;
                            done = true;
                        }
                        else
                            update_countdown = 1; /* Update immediately */
                    }
                }
                break;
#endif

            case SYS_USB_CONNECTED:
                /* Only accept USB connection when not recording */
                if(audio_stat != AUDIO_STATUS_RECORD)
                {
                    default_event_handler(SYS_USB_CONNECTED);
                    done = true;
                    been_in_usb_mode = true;
                }
                break;
                
            default:
                default_event_handler(button);
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;

            FOR_NB_SCREENS(i)       
                screens[i].setfont(FONT_SYSFIXED);

        seconds = audio_recorded_time() / HZ;
        
        update_countdown--;
        if(update_countdown == 0 || seconds > last_seconds)
        {
            unsigned int dseconds, dhours, dminutes;
            unsigned long num_recorded_bytes;
            int pos = 0;
            char spdif_sfreq[8];

            update_countdown = 5;
            last_seconds = seconds;

            FOR_NB_SCREENS(i)
                screens[i].clear_display(); 

            hours = seconds / 3600;
            minutes = (seconds - (hours * 3600)) / 60;
            snprintf(buf, 32, "%s %02d:%02d:%02d",
                     str(LANG_RECORDING_TIME),
                     hours, minutes, seconds%60);
            FOR_NB_SCREENS(i)
                screens[i].puts(0, 0, buf); 

            dseconds = rec_timesplit_seconds();
            num_recorded_bytes = audio_num_recorded_bytes();

            if(audio_stat & AUDIO_STATUS_PRERECORD)
            {
                snprintf(buf, 32, "%s...", str(LANG_RECORD_PRERECORD));
            }
            else
            {
                /* Display the split interval if the record timesplit
                   is active */
                if (global_settings.rec_timesplit)
                {
                    /* Display the record timesplit interval rather
                       than the file size if the record timer is
                       active */
                    dhours = dseconds / 3600;
                    dminutes = (dseconds - (dhours * 3600)) / 60;
                    snprintf(buf, 32, "%s %02d:%02d",
                             str(LANG_RECORD_TIMESPLIT_REC),
                             dhours, dminutes);
                }
                else
                {
                    output_dyn_value(buf2, sizeof buf2,
                                     num_recorded_bytes,
                                     byte_units, true);
                    snprintf(buf, 32, "%s %s",
                             str(LANG_RECORDING_SIZE), buf2);
                }
            }
            FOR_NB_SCREENS(i)
                screens[i].puts(0, 1, buf);

            FOR_NB_SCREENS(i)
            {
                if (filename_offset[i] > 0)
                {
                    if (audio_stat & AUDIO_STATUS_RECORD)
                    {
                        strncpy(filename, path_buffer +
                                    strlen(path_buffer) - 12, 13);
                        filename[12]='\0';
                    }
                    else
                        strcpy(filename, "");

                    snprintf(buf, 32, "Filename: %s", filename);
                    screens[i].puts(0, 2, buf);         
                }
            }

            /* We will do file splitting regardless, either at the end of
               a split interval, or when the filesize approaches the 2GB
               FAT file size (compatibility) limit. */
            if (audio_stat && 
                ((global_settings.rec_timesplit && (seconds >= dseconds))
                 || (num_recorded_bytes >= MAX_FILE_SIZE)))
            {
                audio_new_file(rec_create_filename(path_buffer));
                update_countdown = 1;
                last_seconds = 0;
            }

            snprintf(buf, 32, "%s: %s", str(LANG_VOLUME),
                     fmt_gain(SOUND_VOLUME,
                              global_settings.volume,
                              buf2, sizeof(buf2)));
            
            if (global_settings.invert_cursor && (pos++ == cursor))
            {
                FOR_NB_SCREENS(i)
                    screens[i].puts_style_offset(0, filename_offset[i] +
                                           PM_HEIGHT + 2, buf, STYLE_INVERT,0);
            }
            else
            {
                FOR_NB_SCREENS(i)
                    screens[i].puts(0, filename_offset[i] + PM_HEIGHT + 2, buf);
            }                

            if(global_settings.rec_source == SOURCE_MIC)
            { 
                snprintf(buf, 32, "%s:%s", str(LANG_RECORDING_GAIN),
                         fmt_gain(SOUND_MIC_GAIN,
                                  global_settings.rec_mic_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(2==cursor)))
                {
                    FOR_NB_SCREENS(i)
                        screens[i].puts_style_offset(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf, STYLE_INVERT,0);
                }
                else
                {
                    FOR_NB_SCREENS(i)
                        screens[i].puts(0, filename_offset[i] +
                                            PM_HEIGHT + 3, buf);
                }
            }
            else if(global_settings.rec_source == SOURCE_LINE)
            {
                snprintf(buf, 32, "%s:%s",
                         str(LANG_RECORDING_LEFT),
                         fmt_gain(SOUND_LEFT_GAIN,
                                  global_settings.rec_left_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(2==cursor)))
                {
                    FOR_NB_SCREENS(i)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                           PM_HEIGHT + 3, buf, STYLE_INVERT,0);
                }
                else
                {
                     FOR_NB_SCREENS(i)
                         screens[i].puts(0, filename_offset[i] +
                                             PM_HEIGHT + 3, buf);
                }                

                snprintf(buf, 32, "%s:%s",
                         str(LANG_RECORDING_RIGHT),
                         fmt_gain(SOUND_RIGHT_GAIN,
                                  global_settings.rec_right_gain,
                                  buf2, sizeof(buf2)));
                if(global_settings.invert_cursor && ((1==cursor)||(3==cursor)))
                {
                    FOR_NB_SCREENS(i)
                        screens[i].puts_style_offset(0, filename_offset[i] + 
                                            PM_HEIGHT + 4, buf, STYLE_INVERT,0);
                }
                else
                {
                    FOR_NB_SCREENS(i)
                        screens[i].puts(0, filename_offset[i] + 
                                            PM_HEIGHT + 4, buf);
                }                

            }

            if(!global_settings.invert_cursor){
                switch(cursor)
                {
                    case 1:
                        FOR_NB_SCREENS(i)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] +
                                                    PM_HEIGHT + 3, true);

                        if(global_settings.rec_source != SOURCE_MIC)
                        {
                            FOR_NB_SCREENS(i)
                                screen_put_cursorxy(&screens[i], 0, 
                                                        filename_offset[i] +
                                                        PM_HEIGHT + 4, true);
                        }
                    break;
                    case 2:
                        FOR_NB_SCREENS(i)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 3, true);
                    break;
                    case 3:
                        FOR_NB_SCREENS(i)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 4, true);
                    break;
                    default:
                        FOR_NB_SCREENS(i)
                            screen_put_cursorxy(&screens[i], 0, 
                                                    filename_offset[i] + 
                                                    PM_HEIGHT + 2, true);
                }
            }
/* Can't measure S/PDIF sample rate on Archos yet */
#if (CONFIG_CODEC != MAS3587F) && defined(HAVE_SPDIF_IN) && !defined(SIMULATOR)
            if (global_settings.rec_source == SOURCE_SPDIF)
                snprintf(spdif_sfreq, 8, "%dHz", audio_get_spdif_sample_rate());
#else
            (void)spdif_sfreq;
#endif
            snprintf(buf, 32, "%s %s",
#if (CONFIG_CODEC != MAS3587F) && defined(HAVE_SPDIF_IN) && !defined(SIMULATOR)
                     global_settings.rec_source == SOURCE_SPDIF ?
                     spdif_sfreq :
#endif
                     freq_str[global_settings.rec_frequency],
                     global_settings.rec_channels ?
                     str(LANG_CHANNEL_MONO) : str(LANG_CHANNEL_STEREO));
            FOR_NB_SCREENS(i)
                screens[i].puts(0, filename_offset[i] + PM_HEIGHT + 5, buf);

            gui_syncstatusbar_draw(&statusbars, true);

            FOR_NB_SCREENS(i)
            {
                peak_meter_screen(&screens[i], 0, pm_y[i], h*PM_HEIGHT);
                screens[i].update();                
            }

            /* draw the trigger status */
            if (peak_meter_trigger_status() != TRIG_OFF)
            {
                peak_meter_draw_trig(LCD_WIDTH - TRIG_WIDTH, 4 * h);
                FOR_NB_SCREENS(i){
                    screens[i].update_rect(LCD_WIDTH - (TRIG_WIDTH + 2), 4 * h,
                                    TRIG_WIDTH + 2, TRIG_HEIGHT);
                }
            }
        }

        if(audio_stat & AUDIO_STATUS_ERROR)
        {
            done = true;
        }
    }

    
#if CONFIG_CODEC == SWCODEC        
    audio_stat = pcm_rec_status();
#else
    audio_stat = audio_status();
#endif
    if (audio_stat & AUDIO_STATUS_ERROR)
    {
        gui_syncsplash(0, true, str(LANG_DISK_FULL));
        gui_syncstatusbar_draw(&statusbars, true);
        
        FOR_NB_SCREENS(i)
            screens[i].update();
        
        audio_error_clear();

        while(1)
        {
            button = button_get(true);
            if(button == (REC_STOPEXIT | BUTTON_REL))
                break;
        }
    }
    
#if CONFIG_CODEC == SWCODEC        
    audio_stop_recording(); 
    audio_close_recording();

#if defined(HAVE_SPDIF_IN) && !defined(SIMULATOR)
    if (global_settings.rec_source == SOURCE_SPDIF)
        cpu_boost(false);
#endif

#else
    audio_init_playback();
#endif

    /* make sure the trigger is really turned off */
    peak_meter_trigger(false);
    peak_meter_set_trigger_listener(NULL);

    sound_settings_apply();

    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    if (have_recorded)
        reload_directory();

#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
    ata_set_led_enabled(true);
#endif
    return been_in_usb_mode;
}

#ifdef REC_F2
bool f2_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h, i;
    char buf[32];
    int button;

    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("A",&w,&h);
    }

    while (!exit) {
        const char* ptr=NULL;

        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();

            /* Recording quality */
            screens[i].putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_RECORDING_QUALITY));
        }
        
            snprintf(buf, 32, "%d", global_settings.rec_quality);
        FOR_NB_SCREENS(i)
        {
            screens[i].putsxy(0, LCD_HEIGHT/2-h, buf);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                        LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8);
        }

        /* Frequency */
        snprintf(buf, sizeof buf, "%s:", str(LANG_RECORDING_FREQUENCY));
        ptr = freq_str[global_settings.rec_frequency];
        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(buf,&w,&h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, buf);
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                            LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8);
        }

        /* Channel mode */
        switch ( global_settings.rec_channels ) {
            case 0:
                ptr = str(LANG_CHANNEL_STEREO);
                break;

            case 1:
                ptr = str(LANG_CHANNEL_MONO);
                break;
        }

        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(str(LANG_RECORDING_CHANNELS), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h*2,
                       str(LANG_RECORDING_CHANNELS));
            screens[i].getstringsize(str(LANG_F2_MODE), &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2 - h, str(LANG_F2_MODE));
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy(LCD_WIDTH - w, LCD_HEIGHT/2, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastForward], 
                        LCD_WIDTH/2 + 8, LCD_HEIGHT/2 - 4, 7, 8);

            screens[i].update();
        }

        button = button_get(true);
        switch (button) {
            case BUTTON_LEFT:
            case BUTTON_F2 | BUTTON_LEFT:
                global_settings.rec_quality++;
                if(global_settings.rec_quality > 7)
                    global_settings.rec_quality = 0;
                used = true;
                break;

            case BUTTON_DOWN:
            case BUTTON_F2 | BUTTON_DOWN:
                global_settings.rec_frequency++;
                if(global_settings.rec_frequency > 5)
                    global_settings.rec_frequency = 0;
                used = true;
                break;

            case BUTTON_RIGHT:
            case BUTTON_F2 | BUTTON_RIGHT:
                global_settings.rec_channels++;
                if(global_settings.rec_channels > 1)
                    global_settings.rec_channels = 0;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F2 | BUTTON_REPEAT:
                used = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    if (global_settings.rec_prerecord_time)
        talk_buffer_steal(); /* will use the mp3 buffer */

    audio_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels,
                               global_settings.rec_editable,
                               global_settings.rec_prerecord_time);

    set_gain();
    
    settings_save();
    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    return false;
}
#endif /* #ifdef REC_F2 */

#ifdef REC_F3
bool f3_rec_screen(void)
{
    bool exit = false;
    bool used = false;
    int w, h, i;
    int button;
    char *src_str[] =
    {
        str(LANG_RECORDING_SRC_MIC),
        str(LANG_RECORDING_SRC_LINE),
        str(LANG_RECORDING_SRC_DIGITAL)
    };
    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_SYSFIXED);
        screens[i].getstringsize("A",&w,&h);
    }
    
    while (!exit) {
        char* ptr=NULL;
        ptr = src_str[global_settings.rec_source];
        FOR_NB_SCREENS(i)
        {
            screens[i].clear_display();

            /* Recording source */
            screens[i].putsxy(0, LCD_HEIGHT/2 - h*2, str(LANG_RECORDING_SOURCE));
 
            screens[i].getstringsize(ptr, &w, &h);
            screens[i].putsxy(0, LCD_HEIGHT/2-h, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 
                            LCD_WIDTH/2 - 16, LCD_HEIGHT/2 - 4, 7, 8);
        }

        /* trigger setup */
        ptr = str(LANG_RECORD_TRIGGER);
        FOR_NB_SCREENS(i)
        {
            screens[i].getstringsize(ptr,&w,&h);
            screens[i].putsxy((LCD_WIDTH-w)/2, LCD_HEIGHT - h*2, ptr);
            screens[i].mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                        LCD_WIDTH/2 - 3, LCD_HEIGHT - h*3, 7, 8);

            screens[i].update();
        }

        button = button_get(true);
        switch (button) {
            case BUTTON_DOWN:
            case BUTTON_F3 | BUTTON_DOWN:
#ifndef SIMULATOR
                rectrigger();
                settings_apply_trigger();
#endif
                exit = true;
                break;

            case BUTTON_LEFT:
            case BUTTON_F3 | BUTTON_LEFT:
                global_settings.rec_source++;
                if(global_settings.rec_source > MAX_SOURCE)
                    global_settings.rec_source = 0;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REL:
                if ( used )
                    exit = true;
                used = true;
                break;

            case BUTTON_F3 | BUTTON_REPEAT:
                used = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }

    if (global_settings.rec_prerecord_time)
        talk_buffer_steal(); /* will use the mp3 buffer */

    audio_set_recording_options(global_settings.rec_frequency,
                               global_settings.rec_quality,
                               global_settings.rec_source,
                               global_settings.rec_channels,
                               global_settings.rec_editable,
                               global_settings.rec_prerecord_time);
                                                              

    set_gain();

    settings_save();
    FOR_NB_SCREENS(i)
        screens[i].setfont(FONT_UI);

    return false;
}
#endif /* #ifdef REC_F3 */

#if CONFIG_CODEC == SWCODEC
void audio_beep(int duration)
{
    /* dummy */
    (void)duration;
}

#ifdef SIMULATOR
/* stubs for recording sim */
void audio_init_recording(unsigned int buffer_offset)
{
    buffer_offset = buffer_offset;
}

void audio_close_recording(void)
{
}

unsigned long audio_recorded_time(void)
{
    return 123;
}

unsigned long audio_num_recorded_bytes(void)
{
    return 5 * 1024 * 1024;
}

void audio_set_recording_options(int frequency, int quality,
                                int source, int channel_mode,
                                bool editable, int prerecord_time)
{
    frequency = frequency;
    quality = quality;
    source = source;
    channel_mode = channel_mode;
    editable = editable;
    prerecord_time = prerecord_time;
}

void audio_set_recording_gain(int left, int right, int type)
{
    left = left;
    right = right;
    type = type;
}

void audio_stop_recording(void)
{
}

void audio_pause_recording(void)
{
}

void audio_resume_recording(void)
{
}

void pcm_rec_get_peaks(int *left, int *right)
{
    if (left)
        *left = 0;
    if (right)
        *right = 0;
}

void audio_record(const char *filename)
{
    filename = filename;
}

void audio_new_file(const char *filename)
{
    filename = filename;
}

unsigned long pcm_rec_status(void)
{
    return 0;
}

#endif /* #ifdef SIMULATOR */
#endif /* #ifdef CONFIG_CODEC == SWCODEC */


#endif /* HAVE_RECORDING */
