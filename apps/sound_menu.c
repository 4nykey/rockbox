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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "mp3_playback.h"
#include "settings.h"
#include "statusbar.h"
#include "screens.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "font.h"
#include "widgets.h"
#endif
#include "lang.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#ifdef HAVE_RECORDING
#include "audio.h"
#ifdef CONFIG_TUNER
#include "radio.h"
#endif
#endif
#if CONFIG_CODEC == MAS3587F
#include "peakmeter.h"
#include "mas.h"
#endif
#include "splash.h"
#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#include "eq_menu.h"
#include "pcmbuf.h"
#endif

int selected_setting; /* Used by the callback */
void dec_sound_formatter(char *buffer, int buffer_size, int val, const char * unit)
{
    val = sound_val2phys(selected_setting, val);
    char sign = ' ';
    if(val < 0)
    {
        sign = '-';
        val = abs(val);
    }
    int integer = val / 10;
    int dec = val % 10;
    snprintf(buffer, buffer_size, "%c%d.%d %s", sign, integer, dec, unit);
}

bool set_sound(const unsigned char * string,
               int* variable,
               int setting)
{
    int talkunit = UNIT_INT;
    const char* unit = sound_unit(setting);
    int numdec = sound_numdecimals(setting);
    int steps = sound_steps(setting);
    int min = sound_min(setting);
    int max = sound_max(setting);
    sound_set_type* sound_callback = sound_get_fn(setting);
    if (*unit == 'd') /* crude reconstruction */
        talkunit = UNIT_DB;
    else if (*unit == '%')
        talkunit = UNIT_PERCENT;
    else if (*unit == 'H')
        talkunit = UNIT_HERTZ;
    if(!numdec)
#if CONFIG_CODEC == SWCODEC
        /* We need to hijack this one and send it off to apps/dsp.c instead of
           firmware/sound.c */
        if (setting == SOUND_STEREO_WIDTH)
            return set_int(string, unit, talkunit,  variable, &stereo_width_set,
                           steps, min, max, NULL );
        else
#endif   
        return set_int(string, unit, talkunit,  variable, sound_callback,
                       steps, min, max, NULL );
    else
    {/* Decimal number */
        selected_setting=setting;
        return set_int(string, unit, talkunit,  variable, sound_callback,
                       steps, min, max, &dec_sound_formatter );
    }
}

static bool volume(void)
{
    return set_sound(str(LANG_VOLUME), &global_settings.volume, SOUND_VOLUME);
}

static bool balance(void)
{
    return set_sound(str(LANG_BALANCE), &global_settings.balance,
                     SOUND_BALANCE);
}

#ifndef HAVE_TLV320
static bool bass(void)
{
    return set_sound(str(LANG_BASS), &global_settings.bass, SOUND_BASS);
}

static bool treble(void)
{
    return set_sound(str(LANG_TREBLE), &global_settings.treble, SOUND_TREBLE);
}
#endif

#if CONFIG_CODEC == SWCODEC
static void crossfeed_format(char* buffer, int buffer_size, int value,
    const char* unit)
{
    snprintf(buffer, buffer_size, "%s%d.%d %s", value == 0 ? " " : "-",
        value / 10, value % 10, unit);
}

static bool crossfeed_enabled(void)
{
    bool result = set_bool_options(str(LANG_CROSSFEED),
                                   &global_settings.crossfeed,
                                   STR(LANG_ON),
                                   STR(LANG_OFF),
                                   NULL);

    dsp_set_crossfeed(global_settings.crossfeed);

    return result;
}

static bool crossfeed_direct_gain(void)
{
    return set_int(str(LANG_CROSSFEED_DIRECT_GAIN), str(LANG_UNIT_DB),
        UNIT_DB, &global_settings.crossfeed_direct_gain,
        &dsp_set_crossfeed_direct_gain, 5, 0, 60, crossfeed_format);
}

static void crossfeed_cross_gain_helper(int val)
{
    dsp_set_crossfeed_cross_params(val,
        val + global_settings.crossfeed_hf_attenuation,
        global_settings.crossfeed_hf_cutoff);
}

static bool crossfeed_cross_gain(void)
{
    return set_int(str(LANG_CROSSFEED_CROSS_GAIN), str(LANG_UNIT_DB),
        UNIT_DB, &global_settings.crossfeed_cross_gain,
        &crossfeed_cross_gain_helper, 5, 30, 120, crossfeed_format);
}

static void crossfeed_hf_att_helper(int val)
{
    dsp_set_crossfeed_cross_params(global_settings.crossfeed_cross_gain,
        global_settings.crossfeed_cross_gain + val,
        global_settings.crossfeed_hf_cutoff);
}

static bool crossfeed_hf_attenuation(void)
{
    return set_int(str(LANG_CROSSFEED_HF_ATTENUATION), str(LANG_UNIT_DB),
        UNIT_DB, &global_settings.crossfeed_hf_attenuation,
        &crossfeed_hf_att_helper, 5, 60, 240, crossfeed_format);
}

static void crossfeed_hf_cutoff_helper(int val)
{
    dsp_set_crossfeed_cross_params(global_settings.crossfeed_cross_gain,
        global_settings.crossfeed_cross_gain + global_settings.crossfeed_hf_attenuation, val);
}

static bool crossfeed_hf_cutoff(void)
{
    return set_int(str(LANG_CROSSFEED_HF_CUTOFF), str(LANG_UNIT_HERTZ),
        UNIT_HERTZ, &global_settings.crossfeed_hf_cutoff, &crossfeed_hf_cutoff_helper, 100, 500, 2000, 
        NULL);
}

static bool crossfeed_menu(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_CROSSFEED), crossfeed_enabled },
        { ID2P(LANG_CROSSFEED_DIRECT_GAIN), crossfeed_direct_gain },
        { ID2P(LANG_CROSSFEED_CROSS_GAIN), crossfeed_cross_gain },
        { ID2P(LANG_CROSSFEED_HF_ATTENUATION), crossfeed_hf_attenuation },
        { ID2P(LANG_CROSSFEED_HF_CUTOFF), crossfeed_hf_cutoff },
    };

    m=menu_init(items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
static bool loudness(void)
{
    return set_sound(str(LANG_LOUDNESS), &global_settings.loudness, 
                     SOUND_LOUDNESS);
}

static bool mdb_strength(void)
{
    return set_sound(str(LANG_MDB_STRENGTH), &global_settings.mdb_strength, 
                     SOUND_MDB_STRENGTH);
}

static bool mdb_harmonics(void)
{
    return set_sound(str(LANG_MDB_HARMONICS), &global_settings.mdb_harmonics, 
                     SOUND_MDB_HARMONICS);
}

static bool mdb_center(void)
{
    return set_sound(str(LANG_MDB_CENTER), &global_settings.mdb_center, 
                     SOUND_MDB_CENTER);
}

static bool mdb_shape(void)
{
    return set_sound(str(LANG_MDB_SHAPE), &global_settings.mdb_shape,
                     SOUND_MDB_SHAPE);
}

static void set_mdb_enable(bool value)
{
    sound_set_mdb_enable((int)value);
}

static bool mdb_enable(void)
{
    return set_bool_options(str(LANG_MDB_ENABLE),
                            &global_settings.mdb_enable, 
                            STR(LANG_SET_BOOL_YES), 
                            STR(LANG_SET_BOOL_NO), 
                            set_mdb_enable);
}

static void set_superbass(bool value)
{
    sound_set_superbass((int)value);
}

static bool superbass(void)
{
    return set_bool_options(str(LANG_SUPERBASS),
                            &global_settings.superbass, 
                            STR(LANG_SET_BOOL_YES), 
                            STR(LANG_SET_BOOL_NO), 
                            set_superbass);
}

static bool avc(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "20ms", TALK_ID(20, UNIT_MS) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) }
    };
    return set_option(str(LANG_DECAY), &global_settings.avc, INT,
                      names, 5, sound_set_avc);
}
#endif

#ifdef HAVE_RECORDING
static bool recsource(void)
{
    int n_opts = AUDIO_NUM_SOURCES;

    struct opt_items names[AUDIO_NUM_SOURCES] = {
        { STR(LANG_RECORDING_SRC_MIC) },
        { STR(LANG_RECORDING_SRC_LINE) },
#ifdef HAVE_SPDIF_IN
        { STR(LANG_RECORDING_SRC_DIGITAL) },
#endif
    };

    /* caveat: assumes it's the last item! */
#ifdef HAVE_FMRADIO_IN
    if (radio_hardware_present())
    {
        names[AUDIO_SRC_FMRADIO].string = ID2P(LANG_FM_RADIO);
        names[AUDIO_SRC_FMRADIO].voice_id = LANG_FM_RADIO;
    }
    else
        n_opts--;
#endif

    return set_option(str(LANG_RECORDING_SOURCE),
                      &global_settings.rec_source, INT, names,
                      n_opts, NULL );
}

/* To be removed when we add support for sample rates and channel settings */
#if CONFIG_CODEC == SWCODEC
static bool recquality(void)
{
    static const struct opt_items names[] = {
        { "MP3   64 kBit/s", TALK_ID(  64, UNIT_KBIT) },
        { "MP3   96 kBit/s", TALK_ID(  96, UNIT_KBIT) },
        { "MP3  128 kBit/s", TALK_ID( 128, UNIT_KBIT) },
        { "MP3  160 kBit/s", TALK_ID( 160, UNIT_KBIT) },
        { "MP3  192 kBit/s", TALK_ID( 192, UNIT_KBIT) },
        { "MP3  224 kBit/s", TALK_ID( 224, UNIT_KBIT) },
        { "MP3  320 kBit/s", TALK_ID( 320, UNIT_KBIT) },
        { "WV   900 kBit/s", TALK_ID( 900, UNIT_KBIT) },
        { "WAV 1411 kBit/s", TALK_ID(1411, UNIT_KBIT) }
    };

    return set_option(str(LANG_RECORDING_QUALITY),
                      &global_settings.rec_quality, INT,
                      names, sizeof (names)/sizeof(struct opt_items),
                      NULL );
}
#elif CONFIG_CODEC == MAS3587F
static bool recquality(void)
{
    return set_int(str(LANG_RECORDING_QUALITY), "", UNIT_INT,
                   &global_settings.rec_quality, 
                   NULL, 1, 0, 7, NULL );
}

static bool receditable(void)
{
    return set_bool(str(LANG_RECORDING_EDITABLE),
                    &global_settings.rec_editable);
}
#endif /* CONFIG_CODEC == MAS3587F */

static bool recfrequency(void)
{
    static const struct opt_items names[] = {
        { "44.1kHz", TALK_ID(44, UNIT_KHZ) },
#if CONFIG_CODEC != SWCODEC     /* This is temporary */
        { "48kHz", TALK_ID(48, UNIT_KHZ) },
        { "32kHz", TALK_ID(32, UNIT_KHZ) },
        { "22.05kHz", TALK_ID(22, UNIT_KHZ) },
        { "24kHz", TALK_ID(24, UNIT_KHZ) },
        { "16kHz", TALK_ID(16, UNIT_KHZ) }
#endif
    };
    return set_option(str(LANG_RECORDING_FREQUENCY),
                      &global_settings.rec_frequency, INT,
                      names, sizeof(names)/sizeof(*names), NULL );
}

static bool recchannels(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
        { STR(LANG_CHANNEL_MONO) }
    };
    return set_option(str(LANG_RECORDING_CHANNELS),
                      &global_settings.rec_channels, INT,
                      names, 2, NULL );
}

static bool rectimesplit(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "00:05" , TALK_ID(5, UNIT_MIN) },
        { "00:10" , TALK_ID(10, UNIT_MIN) },
        { "00:15" , TALK_ID(15, UNIT_MIN) },
        { "00:30" , TALK_ID(30, UNIT_MIN) },
        { "01:00" , TALK_ID(1, UNIT_HOUR) },
        { "01:14" , TALK_ID(74, UNIT_MIN) },
        { "01:20" , TALK_ID(80, UNIT_MIN) },
        { "02:00" , TALK_ID(2, UNIT_HOUR) },
        { "04:00" , TALK_ID(4, UNIT_HOUR) },
        { "06:00" , TALK_ID(6, UNIT_HOUR) },
        { "08:00" , TALK_ID(8, UNIT_HOUR) },
        { "10:00" , TALK_ID(10, UNIT_HOUR) },
        { "12:00" , TALK_ID(12, UNIT_HOUR) },
        { "18:00" , TALK_ID(18, UNIT_HOUR) },
        { "24:00" , TALK_ID(24, UNIT_HOUR) }
    };
    return set_option(str(LANG_SPLIT_TIME),
                      &global_settings.rec_timesplit, INT,
                      names, 16, NULL );
}

static bool recsizesplit(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "5MB" , TALK_ID(5, UNIT_MB) },
        { "10MB" , TALK_ID(10, UNIT_MB) },
        { "15MB" , TALK_ID(15, UNIT_MB) },
        { "32MB" , TALK_ID(32, UNIT_MB) },
        { "64MB" , TALK_ID(64, UNIT_MB) },
        { "75MB" , TALK_ID(75, UNIT_MB) },
        { "100MB" , TALK_ID(100, UNIT_MB) },
        { "128MB" , TALK_ID(128, UNIT_MB) },
        { "256MB" , TALK_ID(256, UNIT_MB) },
        { "512MB" , TALK_ID(512, UNIT_MB) },
        { "650MB" , TALK_ID(650, UNIT_MB) },
        { "700MB" , TALK_ID(700, UNIT_MB) },
        { "1GB" , TALK_ID(1024, UNIT_MB) },
        { "1.5GB" , TALK_ID(1536, UNIT_MB) },
        { "1.75GB" , TALK_ID(1792, UNIT_MB) }
    };
    return set_option(str(LANG_SPLIT_SIZE),
                      &global_settings.rec_sizesplit, INT,
                      names, 16, NULL );
}

static bool splitmethod(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_REC_TIME) },
        { STR(LANG_REC_SIZE) },
    };
    bool ret;
    ret=set_option( str(LANG_SPLIT_MEASURE),
                    &global_settings.rec_split_method, INT, names, 2, NULL);
    return ret;
}

static bool splittype(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_START_NEW_FILE) },
        { STR(LANG_STOP_RECORDING) },
    };
    bool ret;
    ret=set_option( str(LANG_SPLIT_TYPE),
                    &global_settings.rec_split_type, INT, names, 2, NULL);
    return ret;
}

static bool filesplitoptionsmenu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SPLIT_MEASURE), splitmethod },
        { ID2P(LANG_SPLIT_TYPE),  splittype  },
        { ID2P(LANG_SPLIT_TIME),  rectimesplit  },
        { ID2P(LANG_SPLIT_SIZE), recsizesplit   }
    };
     m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
static bool recprerecord(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "1s", TALK_ID(1, UNIT_SEC) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "3s", TALK_ID(3, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "5s", TALK_ID(5, UNIT_SEC) },
        { "6s", TALK_ID(6, UNIT_SEC) },
        { "7s", TALK_ID(7, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) },
        { "9s", TALK_ID(9, UNIT_SEC) },
        { "10s", TALK_ID(10, UNIT_SEC) },
        { "11s", TALK_ID(11, UNIT_SEC) },
        { "12s", TALK_ID(12, UNIT_SEC) },
        { "13s", TALK_ID(13, UNIT_SEC) },
        { "14s", TALK_ID(14, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "16s", TALK_ID(16, UNIT_SEC) },
        { "17s", TALK_ID(17, UNIT_SEC) },
        { "18s", TALK_ID(18, UNIT_SEC) },
        { "19s", TALK_ID(19, UNIT_SEC) },
        { "20s", TALK_ID(20, UNIT_SEC) },
        { "21s", TALK_ID(21, UNIT_SEC) },
        { "22s", TALK_ID(22, UNIT_SEC) },
        { "23s", TALK_ID(23, UNIT_SEC) },
        { "24s", TALK_ID(24, UNIT_SEC) },
        { "25s", TALK_ID(25, UNIT_SEC) },
        { "26s", TALK_ID(26, UNIT_SEC) },
        { "27s", TALK_ID(27, UNIT_SEC) },
        { "28s", TALK_ID(28, UNIT_SEC) },
        { "29s", TALK_ID(29, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORD_PRERECORD_TIME),
                      &global_settings.rec_prerecord_time, INT,
                      names, 31, NULL );
}

static bool recdirectory(void)
{
    static const struct opt_items names[] = {
        { rec_base_directory, -1 },
        { STR(LANG_RECORD_CURRENT_DIR) }
    };
    return set_option(str(LANG_RECORD_DIRECTORY),
                      &global_settings.rec_directory, INT,
                      names, 2, NULL );
}

static bool reconstartup(void)
{
    return set_bool(str(LANG_RECORD_STARTUP),
                    &global_settings.rec_startup);
}

#ifdef CONFIG_BACKLIGHT
static bool cliplight(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_MAIN_UNIT) }
#ifdef HAVE_REMOTE_LCD
      , { STR(LANG_REMOTE_MAIN) },
        { STR(LANG_REMOTE_UNIT) }
#endif
    };
      
    return set_option( str(LANG_CLIP_LIGHT),
                    &global_settings.cliplight, INT, names,
#ifdef HAVE_REMOTE_LCD
                     4, NULL );
#else
                     2, NULL );
#endif
}
#endif /*CONFIG_BACKLIGHT */

#ifdef HAVE_AGC
static bool agc_preset(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_AGC_SAFETY) },
        { STR(LANG_AGC_LIVE) },
        { STR(LANG_AGC_DJSET) },
        { STR(LANG_AGC_MEDIUM) },
        { STR(LANG_AGC_VOICE) },
    };
    if (global_settings.rec_source)
        return set_option(str(LANG_RECORD_AGC_PRESET),
                          &global_settings.rec_agc_preset_line,
                          INT, names, 6, NULL );
    else
        return set_option(str(LANG_RECORD_AGC_PRESET),
                          &global_settings.rec_agc_preset_mic,
                          INT, names, 6, NULL );
}

static bool agc_cliptime(void)
{
    static const struct opt_items names[] = {
        { "200ms", TALK_ID(200, UNIT_MS) },
        { "400ms", TALK_ID(400, UNIT_MS) },
        { "600ms", TALK_ID(600, UNIT_MS) },
        { "800ms", TALK_ID(800, UNIT_MS) },
        { "1s", TALK_ID(1, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORD_AGC_CLIPTIME),
                      &global_settings.rec_agc_cliptime,
                      INT, names, 5, NULL );
}
#endif /* HAVE_AGC */
#endif /* HAVE_RECORDING */

static bool chanconf(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_CHANNEL_STEREO) },
        { STR(LANG_CHANNEL_MONO) },
        { STR(LANG_CHANNEL_CUSTOM) },
        { STR(LANG_CHANNEL_LEFT) },
        { STR(LANG_CHANNEL_RIGHT) },
        { STR(LANG_CHANNEL_KARAOKE) }
    };
#if CONFIG_CODEC == SWCODEC
    return set_option(str(LANG_CHANNEL), &global_settings.channel_config, INT,
                      names, 6, channels_set);
#else
    return set_option(str(LANG_CHANNEL), &global_settings.channel_config, INT,
                      names, 6, sound_set_channels);
#endif
}

static bool stereo_width(void)
{
    return set_sound(str(LANG_STEREO_WIDTH), &global_settings.stereo_width,
                     SOUND_STEREO_WIDTH);
}

bool sound_menu(void)
{
    int m;
    bool result;
    static const struct menu_item items[] = {
        { ID2P(LANG_VOLUME), volume },
#ifndef HAVE_TLV320
        { ID2P(LANG_BASS), bass },
        { ID2P(LANG_TREBLE), treble },
#endif
        { ID2P(LANG_BALANCE), balance },
        { ID2P(LANG_CHANNEL_MENU), chanconf },
        { ID2P(LANG_STEREO_WIDTH), stereo_width },
#if CONFIG_CODEC == SWCODEC
        { ID2P(LANG_CROSSFEED), crossfeed_menu },
        { ID2P(LANG_EQUALIZER), eq_menu },
#endif
#ifdef HAVE_WM8758
        { ID2P(LANG_EQUALIZER_HARDWARE), eq_hw_menu },
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
        { ID2P(LANG_LOUDNESS), loudness },
        { ID2P(LANG_AUTOVOL), avc },
        { ID2P(LANG_SUPERBASS), superbass },
        { ID2P(LANG_MDB_ENABLE), mdb_enable },
        { ID2P(LANG_MDB_STRENGTH), mdb_strength },
        { ID2P(LANG_MDB_HARMONICS), mdb_harmonics },
        { ID2P(LANG_MDB_CENTER), mdb_center },
        { ID2P(LANG_MDB_SHAPE), mdb_shape },
#endif
    };
    
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(true);
#endif

    m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(false);
#endif

    return result;
}

#ifdef HAVE_RECORDING
enum trigger_menu_option
{
    TRIGGER_MODE,
    PRERECORD_TIME,
    START_THRESHOLD,
    START_DURATION,
    STOP_THRESHOLD,
    STOP_POSTREC,
    STOP_GAP,
    TRIG_OPTION_COUNT,
};

#if !defined(SIMULATOR) && CONFIG_CODEC == MAS3587F
static char* create_thres_str(int threshold)
{
    static char retval[6];
    if (threshold < 0) {
        if (threshold < -88) {
            snprintf (retval, sizeof retval, "%s", str(LANG_DB_INF));
        } else {
            snprintf (retval, sizeof retval, "%ddb", threshold + 1);
        }
    } else {
        snprintf (retval, sizeof retval, "%d%%", threshold);
    }
    return retval;
}

#define INF_DB (-89)
static void change_threshold(int *threshold, int change)
{
    if (global_settings.peak_meter_dbfs) {
        if (*threshold >= 0) {
            int db = (calc_db(*threshold * MAX_PEAK / 100) - 9000) / 100;
            *threshold = db;
        }
        *threshold += change;
        if (*threshold > -1) {
            *threshold = INF_DB;
        } else if (*threshold < INF_DB) {
            *threshold = -1;
        }
    } else {
        if (*threshold < 0) {
            *threshold = peak_meter_db2sample(*threshold * 100) * 100 / MAX_PEAK;
        }
        *threshold += change;
        if (*threshold > 100) {
            *threshold = 0;
        } else if (*threshold < 0) {
            *threshold = 100;
        }
    }
}

/* Variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define TRIG_CANCEL BUTTON_OFF
#define TRIG_ACCEPT BUTTON_PLAY
#define TRIG_RESET_SIM BUTTON_F2

#elif CONFIG_KEYPAD == ONDIO_PAD
#define TRIG_CANCEL BUTTON_OFF
#define TRIG_ACCEPT BUTTON_MENU
#endif

/**
 * Displays a menu for editing the trigger settings.
 */
bool rectrigger(void)
{
    int exit_request = false;
    enum trigger_menu_option selected = TRIGGER_MODE;
    bool retval = false;
    int old_x_margin, old_y_margin;

#define TRIGGER_MODE_COUNT 3
    static const unsigned char *trigger_modes[] = {
        ID2P(LANG_OFF),
        ID2P(LANG_RECORD_TRIG_NOREARM),
        ID2P(LANG_RECORD_TRIG_REARM)
    };

#define PRERECORD_TIMES_COUNT 31
    static const unsigned char *prerecord_times[] = {
        ID2P(LANG_OFF),"1s","2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s",
        "10s", "11s", "12s", "13s", "14s", "15s", "16s", "17s", "18s", "19s",
        "20s", "21s", "22s", "23s", "24s", "25s", "26s", "27s", "28s", "29s",
        "30s"
    };

    static const unsigned char *option_name[] = {
        [TRIGGER_MODE] =    ID2P(LANG_RECORD_TRIGGER_MODE),
        [PRERECORD_TIME] =  ID2P(LANG_RECORD_PRERECORD_TIME),
        [START_THRESHOLD] = ID2P(LANG_RECORD_START_THRESHOLD),
        [START_DURATION] =  ID2P(LANG_RECORD_MIN_DURATION),
        [STOP_THRESHOLD] =  ID2P(LANG_RECORD_STOP_THRESHOLD),
        [STOP_POSTREC] =    ID2P(LANG_RECORD_STOP_POSTREC),
        [STOP_GAP] =        ID2P(LANG_RECORD_STOP_GAP)
    };

    int old_start_thres = global_settings.rec_start_thres;
    int old_start_duration = global_settings.rec_start_duration;
    int old_prerecord_time = global_settings.rec_prerecord_time;
    int old_stop_thres = global_settings.rec_stop_thres;
    int old_stop_postrec = global_settings.rec_stop_postrec;
    int old_stop_gap = global_settings.rec_stop_gap;
    int old_trigger_mode = global_settings.rec_trigger_mode;

    int offset = 0;
    int option_lines;
    int w, h;
    /* array for y ordinate of peak_meter_draw_get_button
       function in peakmeter.c*/
    int pm_y[NB_SCREENS];

    /* restart trigger with new values */
    settings_apply_trigger();
    peak_meter_trigger (global_settings.rec_trigger_mode != TRIG_MODE_OFF);

    lcd_clear_display();

    old_x_margin = lcd_getxmargin();
    old_y_margin = lcd_getymargin();
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);

    lcd_getstringsize("M", &w, &h);

    // two lines are reserved for peak meter and trigger status
    option_lines = (LCD_HEIGHT/h) - (global_settings.statusbar ? 1:0) - 2;

    while (!exit_request) {
        int stat_height = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
        int button, i;
        const char *str;
        char option_value[TRIG_OPTION_COUNT][7];

        snprintf(
            option_value[TRIGGER_MODE],
            sizeof option_value[TRIGGER_MODE],
            "%s",
            P2STR(trigger_modes[global_settings.rec_trigger_mode]));

        snprintf (
            option_value[PRERECORD_TIME],
            sizeof option_value[PRERECORD_TIME],
            "%s",
            P2STR(prerecord_times[global_settings.rec_prerecord_time]));

        /* due to value range shift (peak_meter_define_trigger) -1 is 0db */
        if (global_settings.rec_start_thres == -1) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_start_thres);
        }
        snprintf(
            option_value[START_THRESHOLD],
            sizeof option_value[START_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[START_DURATION],
            sizeof option_value[START_DURATION],
            "%s",
            trig_durations[global_settings.rec_start_duration]);


        if (global_settings.rec_stop_thres <= INF_DB) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_stop_thres);
        }
        snprintf(
            option_value[STOP_THRESHOLD],
            sizeof option_value[STOP_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[STOP_POSTREC],
            sizeof option_value[STOP_POSTREC],
            "%s",
            trig_durations[global_settings.rec_stop_postrec]);

        snprintf(
            option_value[STOP_GAP],
            sizeof option_value[STOP_GAP],
            "%s",
            trig_durations[global_settings.rec_stop_gap]);

        lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        lcd_fillrect(0, stat_height, LCD_WIDTH, LCD_HEIGHT - stat_height);
        lcd_set_drawmode(DRMODE_SOLID);
        gui_syncstatusbar_draw(&statusbars, true);

        /* reselect FONT_SYSFONT as status_draw has changed the font */
        /*lcd_setfont(FONT_SYSFIXED);*/

        for (i = 0; i < option_lines; i++) {
            int x, y;

            str = P2STR(option_name[i + offset]);
            lcd_putsxy(5, stat_height + i * h, str);

            str = option_value[i + offset];
            lcd_getstringsize(str, &w, &h);
            y = stat_height + i * h;
            x = LCD_WIDTH - w;
            lcd_putsxy(x, y, str);
            if ((int)selected == (i + offset)) {
                lcd_set_drawmode(DRMODE_COMPLEMENT);
                lcd_fillrect(x, y, w, h);
                lcd_set_drawmode(DRMODE_SOLID);
            }
        }

        scrollbar(0, stat_height,
            4, LCD_HEIGHT - 16 - stat_height,
            TRIG_OPTION_COUNT, offset, offset + option_lines,
            VERTICAL);

        peak_meter_draw_trig(0, LCD_HEIGHT - 8 - TRIG_HEIGHT);

        FOR_NB_SCREENS(i)
            pm_y[i] = screens[i].height - 8;
        button = peak_meter_draw_get_btn(0, pm_y, 8, NB_SCREENS);

        lcd_update();

        switch (button) {
            case TRIG_CANCEL:
                gui_syncsplash(50, true, str(LANG_MENU_SETTING_CANCEL));
                global_settings.rec_start_thres = old_start_thres;
                global_settings.rec_start_duration = old_start_duration;
                global_settings.rec_prerecord_time = old_prerecord_time;
                global_settings.rec_stop_thres = old_stop_thres;
                global_settings.rec_stop_postrec = old_stop_postrec;
                global_settings.rec_stop_gap = old_stop_gap;
                global_settings.rec_trigger_mode = old_trigger_mode;
                exit_request = true;
                break;

            case TRIG_ACCEPT:
                exit_request = true;
                break;

            case BUTTON_UP:
                selected += TRIG_OPTION_COUNT - 1;
                selected %= TRIG_OPTION_COUNT;
                offset = MIN(offset, (int)selected);
                offset = MAX(offset, (int)selected - option_lines + 1);
                break;

            case BUTTON_DOWN:
                selected ++;
                selected %= TRIG_OPTION_COUNT;
                offset = MIN(offset, (int)selected);
                offset = MAX(offset, (int)selected - option_lines + 1);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode ++;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time ++;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, 1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration ++;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, 1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec ++;
                        global_settings.rec_stop_postrec %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap ++;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode+=TRIGGER_MODE_COUNT-1;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time += PRERECORD_TIMES_COUNT - 1;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, -1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration += TRIG_DURATION_COUNT-1;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, -1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_postrec %=
                            TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                break;

#ifdef TRIG_RESET_SIM
            case TRIG_RESET_SIM:
                peak_meter_trigger(true);
                break;
#endif

            case SYS_USB_CONNECTED:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    retval = true;
                    exit_request = true;
                }
                break;
        }
    }

    peak_meter_trigger(false);
    lcd_setfont(FONT_UI);
    lcd_setmargins(old_x_margin, old_y_margin);
    return retval;
}
#endif

bool recording_menu(bool no_source)
{
    int m;
    int i = 0;
    struct menu_item items[13];
    bool result;

#if CONFIG_CODEC == MAS3587F || CONFIG_CODEC == SWCODEC
    items[i].desc = ID2P(LANG_RECORDING_QUALITY);
    items[i++].function = recquality;
#endif
    items[i].desc = ID2P(LANG_RECORDING_FREQUENCY);
    items[i++].function = recfrequency;
    if(!no_source) {
        items[i].desc = ID2P(LANG_RECORDING_SOURCE);
        items[i++].function = recsource;
    }
    items[i].desc = ID2P(LANG_RECORDING_CHANNELS);
    items[i++].function = recchannels;
#if CONFIG_CODEC == MAS3587F
    items[i].desc = ID2P(LANG_RECORDING_EDITABLE);
    items[i++].function = receditable;
#endif
    items[i].desc = ID2P(LANG_RECORD_TIMESPLIT);
    items[i++].function = filesplitoptionsmenu;
    items[i].desc = ID2P(LANG_RECORD_PRERECORD_TIME);
    items[i++].function = recprerecord;
    items[i].desc = ID2P(LANG_RECORD_DIRECTORY);
    items[i++].function = recdirectory;
    items[i].desc = ID2P(LANG_RECORD_STARTUP);
    items[i++].function = reconstartup;
#ifdef CONFIG_BACKLIGHT
    items[i].desc = ID2P(LANG_CLIP_LIGHT);
    items[i++].function = cliplight;
#endif
#if !defined(SIMULATOR) && CONFIG_CODEC == MAS3587F
    items[i].desc = ID2P(LANG_RECORD_TRIGGER);
    items[i++].function = rectrigger;
#endif
#ifdef HAVE_AGC
    items[i].desc = ID2P(LANG_RECORD_AGC_PRESET);
    items[i++].function = agc_preset;
    items[i].desc = ID2P(LANG_RECORD_AGC_CLIPTIME);
    items[i++].function = agc_cliptime;
#endif

    m=menu_init( items, i, NULL, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
}
#endif
