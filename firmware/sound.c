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
 * Copyright (C) 2007 by Christian Gmeiner
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
#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "sound.h"
#include "logf.h"
#include "system.h"
#include "i2c.h"
#include "mas.h"  
#ifndef SIMULATOR
#if CONFIG_CPU == PNX0101
#include "pnx0101.h"
#endif
#include "dac.h"
#if CONFIG_CODEC == SWCODEC
#include "pcm.h"
#endif
#endif

/* TODO
 * find a nice way to handle 1.5db steps -> see wm8751 ifdef in sound_set_bass/treble
*/

extern bool audio_is_initialized;

const char *sound_unit(int setting)
{
    return audiohw_settings[setting].unit;
}

int sound_numdecimals(int setting)
{
    return audiohw_settings[setting].numdecimals;
}

int sound_steps(int setting)
{
    return audiohw_settings[setting].steps;
}

int sound_min(int setting)
{
    return audiohw_settings[setting].minval;
}

int sound_max(int setting)
{
    return audiohw_settings[setting].maxval;
}

int sound_default(int setting)
{
    return audiohw_settings[setting].defaultval;
}

static sound_set_type * const sound_set_fns[] =
{
    [0 ... SOUND_LAST_SETTING-1] = NULL,
    [SOUND_VOLUME]        = sound_set_volume,
#if defined(AUDIOHW_HAVE_BASS)
    [SOUND_BASS]          = sound_set_bass,
#endif
#if defined(AUDIOHW_HAVE_TREBLE)
    [SOUND_TREBLE]        = sound_set_treble,
#endif
    [SOUND_BALANCE]       = sound_set_balance,
    [SOUND_CHANNELS]      = sound_set_channels,
    [SOUND_STEREO_WIDTH]  = sound_set_stereo_width,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_LOUDNESS]      = sound_set_loudness,
    [SOUND_AVC]           = sound_set_avc,
    [SOUND_MDB_STRENGTH]  = sound_set_mdb_strength,
    [SOUND_MDB_HARMONICS] = sound_set_mdb_harmonics,
    [SOUND_MDB_CENTER]    = sound_set_mdb_center,
    [SOUND_MDB_SHAPE]     = sound_set_mdb_shape,
    [SOUND_MDB_ENABLE]    = sound_set_mdb_enable,
    [SOUND_SUPERBASS]     = sound_set_superbass,
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
    [SOUND_BASS_CUTOFF]   = sound_set_bass_cutoff,
#endif
#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
    [SOUND_TREBLE_CUTOFF] = sound_set_treble_cutoff,
#endif
#if defined(AUDIOHW_HAVE_DEPTH_3D)
    [SOUND_DEPTH_3D]      = sound_set_depth_3d,
#endif
/* Hardware EQ tone controls */
#if defined(AUDIOHW_HAVE_EQ)
    [SOUND_EQ_BAND1_GAIN] = sound_set_hw_eq_band1_gain,
#if defined(AUDIOHW_HAVE_EQ_BAND1_FREQUENCY)
    [SOUND_EQ_BAND1_FREQUENCY] = sound_set_hw_eq_band1_frequency,
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2)
    [SOUND_EQ_BAND2_GAIN] = sound_set_hw_eq_band2_gain,
#if defined(AUDIOHW_HAVE_EQ_BAND2_FREQUENCY)
    [SOUND_EQ_BAND2_FREQUENCY] = sound_set_hw_eq_band2_frequency,
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND2_WIDTH)
    [SOUND_EQ_BAND2_WIDTH] = sound_set_hw_eq_band2_width,
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND2 */
#if defined(AUDIOHW_HAVE_EQ_BAND3)
    [SOUND_EQ_BAND3_GAIN] = sound_set_hw_eq_band3_gain,
#if defined(AUDIOHW_HAVE_EQ_BAND3_FREQUENCY)
    [SOUND_EQ_BAND3_FREQUENCY] = sound_set_hw_eq_band3_frequency,
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
    [SOUND_EQ_BAND3_WIDTH] = sound_set_hw_eq_band3_width,
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND3 */
#if defined(AUDIOHW_HAVE_EQ_BAND4)
    [SOUND_EQ_BAND4_GAIN] = sound_set_hw_eq_band4_gain,
#if defined(AUDIOHW_HAVE_EQ_BAND4_FREQUENCY)
    [SOUND_EQ_BAND4_FREQUENCY] = sound_set_hw_eq_band4_frequency,
#endif
#if defined(AUDIOHW_HAVE_EQ_BAND4_WIDTH)
    [SOUND_EQ_BAND4_WIDTH] = sound_set_hw_eq_band4_width,
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND4 */
#if defined(AUDIOHW_HAVE_EQ_BAND5)
    [SOUND_EQ_BAND5_GAIN] = sound_set_hw_eq_band5_gain,
#if defined(AUDIOHW_HAVE_EQ_BAND5_FREQUENCY)
    [SOUND_EQ_BAND5_FREQUENCY] = sound_set_hw_eq_band5_frequency,
#endif
#endif /* AUDIOHW_HAVE_EQ_BAND5 */
#endif /* AUDIOHW_HAVE_EQ */
};

sound_set_type* sound_get_fn(int setting)
{
    return ((unsigned)setting >= ARRAYLEN(sound_set_fns)?
                NULL : sound_set_fns[setting]);
}

#if CONFIG_CODEC == SWCODEC
static int (*dsp_callback)(int, intptr_t) = NULL;

void sound_set_dsp_callback(int (*func)(int, intptr_t))
{
    dsp_callback = func;
}
#endif

#if (CONFIG_CODEC == MAS3507D) && !defined(SIMULATOR)
/* convert tenth of dB volume (-780..+180) to dac3550 register value */
static int tenthdb2reg(int db)
{
    if (db < -540)                  /* 3 dB steps */
        return (db + 780) / 30;
    else                            /* 1.5 dB steps */
        return (db + 660) / 15;
}
#endif


#if !defined(AUDIOHW_HAVE_CLIPPING)
/*
 * The prescaler compensates for any kind of boosts, to prevent clipping.
 *
 * It's basically just a measure to make sure that audio does not clip during
 * tone controls processing, like if i want to boost bass 12 dB, i can decrease
 * the audio amplitude by -12 dB before processing, then increase master gain
 * by 12 dB after processing.
 */

/* all values in tenth of dB    MAS3507D    UDA1380  */
int current_volume = 0;    /* -780..+180  -840..   0 */
int current_balance = 0;   /* -960..+960  -840..+840 */
#ifdef AUDIOHW_HAVE_TREBLE
int current_treble = 0;    /* -150..+150     0.. +60 */
#endif
#ifdef AUDIOHW_HAVE_BASS
int current_bass = 0;      /* -150..+150     0..+240 */
#endif
#ifdef AUDIOHW_HAVE_EQ
int current_eq_band_gain[AUDIOHW_EQ_BAND_NUM];
#endif

static void set_prescaled_volume(void)
{
    int prescale = 0;
    int l, r;

/* The codecs listed use HW tone controls but don't have suitable prescaler
 * functionality, so we let the prescaler stay at 0 for these, unless
 * SW tone controls are in use. This is to avoid needing the SW DSP just for
 * the prescaling.
 */
#if defined(HAVE_SW_TONE_CONTROLS) || !(defined(HAVE_WM8975) \
    || defined(HAVE_WM8711) || defined(HAVE_WM8721) || defined(HAVE_WM8731) \
    || defined(HAVE_WM8758) || defined(HAVE_WM8985) || defined(HAVE_UDA1341))

#if defined(AUDIOHW_HAVE_BASS) && defined(AUDIOHW_HAVE_TREBLE)
    prescale = MAX(current_bass, current_treble);
#endif
#if defined(AUDIOHW_HAVE_EQ)
    int i;
    for (i = 0; i < AUDIOHW_EQ_BAND_NUM; i++)
        prescale = MAX(current_eq_band_gain[i], prescale);
#endif

    if (prescale < 0)
        prescale = 0;  /* no need to prescale if we don't boost
                          bass, treble or eq band */

    /* Gain up the analog volume to compensate the prescale gain reduction,
     * but if this would push the volume over the top, reduce prescaling
     * instead (might cause clipping). */
    if (current_volume + prescale > VOLUME_MAX)
        prescale = VOLUME_MAX - current_volume;
#endif

#if defined(AUDIOHW_HAVE_PRESCALER)
    audiohw_set_prescaler(prescale);
#else
    dsp_callback(DSP_CALLBACK_SET_PRESCALE, prescale);
#endif

    if (current_volume == VOLUME_MIN)
        prescale = 0;  /* Make sure the chip gets muted at VOLUME_MIN */

    l = r = current_volume + prescale;

    /* Balance the channels scaled by the current volume and min volume. */
    /* Subtract a dB from VOLUME_MIN to get it to a mute level */
    if (current_balance > 0)
    {
        l -= ((l - (VOLUME_MIN - ONE_DB)) * current_balance) / VOLUME_RANGE;
    }
    else if (current_balance < 0)
    {
        r += ((r - (VOLUME_MIN - ONE_DB)) * current_balance) / VOLUME_RANGE;
    }

#ifdef HAVE_SW_VOLUME_CONTROL
    dsp_callback(DSP_CALLBACK_SET_SW_VOLUME, 0);
#endif

#ifndef SIMULATOR
#if CONFIG_CODEC == MAS3507D
    dac_volume(tenthdb2reg(l), tenthdb2reg(r), false);
#elif defined(HAVE_UDA1380) || defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8711) || defined(HAVE_WM8721) || defined(HAVE_WM8731) \
   || defined(HAVE_WM8750) || defined(HAVE_WM8751) || defined(HAVE_AS3514) \
   || defined(HAVE_TSC2100) || defined(HAVE_AK4537) || defined(HAVE_UDA1341)
    audiohw_set_master_vol(tenthdb2master(l), tenthdb2master(r));
#if defined(HAVE_WM8975) || defined(HAVE_WM8758) \
    || defined(HAVE_WM8750) || (defined(HAVE_WM8751) && !defined(MROBE_100)) \
    || defined(HAVE_WM8985)
    audiohw_set_lineout_vol(tenthdb2master(0), tenthdb2master(0));
#endif

#elif defined(HAVE_TLV320) || defined(HAVE_WM8978) || defined(HAVE_WM8985)
    audiohw_set_headphone_vol(tenthdb2master(l), tenthdb2master(r));
#elif defined(HAVE_JZ4740_CODEC) || defined(HAVE_SDL_AUDIO)
    audiohw_set_volume(current_volume);
#endif
#else /* SIMULATOR */
    audiohw_set_volume(current_volume);
#endif /* !SIMULATOR */
}
#endif /* (CONFIG_CODEC == MAS3507D) || defined HAVE_UDA1380 */


#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
unsigned long mdb_shape_shadow = 0;
unsigned long loudness_shadow = 0;
#endif

void sound_set_volume(int value)
{
    if(!audio_is_initialized)
        return;

#if defined(AUDIOHW_HAVE_CLIPPING)
    audiohw_set_volume(value);
#elif CONFIG_CPU == PNX0101
    int tmp = (60 - value * 4) & 0xff;
    CODECVOL = tmp | (tmp << 8);
#else
    current_volume = value * 10;     /* tenth of dB */
    set_prescaled_volume();
#endif
}

void sound_set_balance(int value)
{
    if(!audio_is_initialized)
        return;

#ifdef AUDIOHW_HAVE_BALANCE
    audiohw_set_balance(value);
#else
    current_balance = value * VOLUME_RANGE / 100; /* tenth of dB */
    set_prescaled_volume();
#endif
}

#ifdef AUDIOHW_HAVE_BASS
void sound_set_bass(int value)
{
    if(!audio_is_initialized)
        return;

#if !defined(AUDIOHW_HAVE_CLIPPING)
#if defined(HAVE_WM8750) || defined(HAVE_WM8751)
    current_bass = value;
#else
    current_bass =  value * 10;
#endif
#endif

#if defined(HAVE_SW_TONE_CONTROLS)
    dsp_callback(DSP_CALLBACK_SET_BASS, current_bass);
#else
    audiohw_set_bass(value);
#endif

#if !defined(AUDIOHW_HAVE_CLIPPING)
    set_prescaled_volume();
#endif
}
#endif /* AUDIOHW_HAVE_BASS */

#ifdef AUDIOHW_HAVE_TREBLE
void sound_set_treble(int value)
{
    if(!audio_is_initialized)
        return;

#if !defined(AUDIOHW_HAVE_CLIPPING)
#if defined(HAVE_WM8750) || defined(HAVE_WM8751)
    current_treble = value;
#else
    current_treble = value * 10;
#endif
#endif

#if defined(HAVE_SW_TONE_CONTROLS)
    dsp_callback(DSP_CALLBACK_SET_TREBLE, current_treble);
#else
    audiohw_set_treble(value);
#endif

#if !defined(AUDIOHW_HAVE_CLIPPING)
    set_prescaled_volume();
#endif
}
#endif /* AUDIOHW_HAVE_TREBLE */

#if defined(AUDIOHW_HAVE_BASS_CUTOFF)
void sound_set_bass_cutoff(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_bass_cutoff(value);
}
#endif

#if defined(AUDIOHW_HAVE_TREBLE_CUTOFF)
void sound_set_treble_cutoff(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_treble_cutoff(value);
}
#endif

void sound_set_channels(int value)
{
    if(!audio_is_initialized)
        return;

#if CONFIG_CODEC == SWCODEC
    dsp_callback(DSP_CALLBACK_SET_CHANNEL_CONFIG, value);
#else
    audiohw_set_channel(value);
#endif
}

void sound_set_stereo_width(int value)
{
    if(!audio_is_initialized)
        return;

#if CONFIG_CODEC == SWCODEC
    dsp_callback(DSP_CALLBACK_SET_STEREO_WIDTH, value);
#else
    audiohw_set_stereo_width(value);
#endif
}

#if defined(AUDIOHW_HAVE_DEPTH_3D)
void sound_set_depth_3d(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_depth_3d(value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ)
int sound_enum_hw_eq_band_setting(unsigned int band,
                                  unsigned int band_setting)
{
    static const int8_t
    sound_hw_eq_band_setting[AUDIOHW_EQ_SETTING_NUM][AUDIOHW_EQ_BAND_NUM] =
    {
        [AUDIOHW_EQ_GAIN] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
            [AUDIOHW_EQ_BAND1] = SOUND_EQ_BAND1_GAIN,
        #ifdef AUDIOHW_HAVE_EQ_BAND2
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_GAIN,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND5
            [AUDIOHW_EQ_BAND5] = SOUND_EQ_BAND5_GAIN,
        #endif
        },
#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
        [AUDIOHW_EQ_FREQUENCY] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
        #ifdef AUDIOHW_HAVE_EQ_BAND1_FREQUENCY
            [AUDIOHW_EQ_BAND1] = SOUND_EQ_BAND1_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND2_FREQUENCY
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3_FREQUENCY
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4_FREQUENCY
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_FREQUENCY,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND5_FREQUENCY
            [AUDIOHW_EQ_BAND5] = SOUND_EQ_BAND5_FREQUENCY,
        #endif
        },
#endif /* AUDIOHW_HAVE_EQ_FREQUENCY */
#ifdef AUDIOHW_HAVE_EQ_WIDTH
        [AUDIOHW_EQ_WIDTH] =
        {
            [0 ... AUDIOHW_EQ_BAND_NUM-1] = -1,
        #ifdef AUDIOHW_HAVE_EQ_BAND2_WIDTH
            [AUDIOHW_EQ_BAND2] = SOUND_EQ_BAND2_WIDTH,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND3_WIDTH
            [AUDIOHW_EQ_BAND3] = SOUND_EQ_BAND3_WIDTH,
        #endif
        #ifdef AUDIOHW_HAVE_EQ_BAND4_WIDTH
            [AUDIOHW_EQ_BAND4] = SOUND_EQ_BAND4_WIDTH,
        #endif
        },
#endif /* AUDIOHW_HAVE_EQ_WIDTH */
    };

    if (band < AUDIOHW_EQ_BAND_NUM && band_setting < AUDIOHW_EQ_SETTING_NUM)
        return sound_hw_eq_band_setting[band_setting][band];

    return -1;
}

static void sound_set_hw_eq_band_gain(unsigned int band, int value)
{
    if(!audio_is_initialized)
        return;

    current_eq_band_gain[band] = value;
    audiohw_set_eq_band_gain(band, value);
    set_prescaled_volume();
}

void sound_set_hw_eq_band1_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND1, value);
}

#if defined(AUDIOHW_HAVE_EQ_BAND2)
void sound_set_hw_eq_band2_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3)
void sound_set_hw_eq_band3_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4)
void sound_set_hw_eq_band4_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND4, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND5)
void sound_set_hw_eq_band5_gain(int value)
{
    sound_set_hw_eq_band_gain(AUDIOHW_EQ_BAND5, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND1_FREQUENCY)
void sound_set_hw_eq_band1_frequency(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND1, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND2_FREQUENCY)
void sound_set_hw_eq_band2_frequency(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3_FREQUENCY)
void sound_set_hw_eq_band3_frequency(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4_FREQUENCY)
void sound_set_hw_eq_band4_frequency(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND4, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND5_FREQUENCY)
void sound_set_hw_eq_band5_frequency(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_frequency(AUDIOHW_EQ_BAND5, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND2_WIDTH)
void sound_set_hw_eq_band2_width(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND2, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND3_WIDTH)
void sound_set_hw_eq_band3_width(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND3, value);
}
#endif

#if defined(AUDIOHW_HAVE_EQ_BAND4_WIDTH)
void sound_set_hw_eq_band4_width(int value)
{
    if(!audio_is_initialized)
        return;

    audiohw_set_eq_band_width(AUDIOHW_EQ_BAND4, value);
}
#endif
#endif /* AUDIOHW_HAVE_EQ */

#if ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F))
void sound_set_loudness(int value)
{
    if(!audio_is_initialized)
        return;
    loudness_shadow = (loudness_shadow & 0x04) |
                       (MAX(MIN(value * 4, 0x44), 0) << 8);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}

void sound_set_avc(int value)
{
    if(!audio_is_initialized)
        return;
    int tmp;

    static const uint16_t avc_vals[] =
    {
        (0x1 << 8) | (0x8 << 12), /* 20ms */
        (0x2 << 8) | (0x8 << 12), /* 2s */
        (0x4 << 8) | (0x8 << 12), /* 4s */
        (0x8 << 8) | (0x8 << 12), /* 8s */
    };
    switch (value) {
        case 1:
        case 2:
        case 3:
        case 4:
            tmp = avc_vals[value -1];
            break;
        case -1: /* turn off and then turn on again to decay quickly */
            tmp = mas_codec_readreg(MAS_REG_KAVC);
            mas_codec_writereg(MAS_REG_KAVC, 0);
            break;
        default: /* off */
            tmp = 0;
            break;
    }
    mas_codec_writereg(MAS_REG_KAVC, tmp);     
}

void sound_set_mdb_strength(int value)
{
    if(!audio_is_initialized)
        return;
    mas_codec_writereg(MAS_REG_KMDB_STR, (value & 0x7f) << 8); 
}

void sound_set_mdb_harmonics(int value)
{
    if(!audio_is_initialized)
        return;
    int tmp = value * 127 / 100;
    mas_codec_writereg(MAS_REG_KMDB_HAR, (tmp & 0x7f) << 8);
}

void sound_set_mdb_center(int value)
{
    if(!audio_is_initialized)
        return;
    mas_codec_writereg(MAS_REG_KMDB_FC, (value/10) << 8);
}

void sound_set_mdb_shape(int value)
{
    if(!audio_is_initialized)
        return;
    mdb_shape_shadow = (mdb_shape_shadow & 0x02) | ((value/10) << 8);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void sound_set_mdb_enable(int value)
{
    if(!audio_is_initialized)
        return;
    mdb_shape_shadow = (mdb_shape_shadow & ~0x02) | (value?2:0);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void sound_set_superbass(int value)
{
    if(!audio_is_initialized)
        return;
    loudness_shadow = (loudness_shadow & ~0x04) | (value?4:0);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */

void sound_set(int setting, int value)
{
    sound_set_type* sound_set_val = sound_get_fn(setting);
    if (sound_set_val)
        sound_set_val(value);
}

#if (!defined(HAVE_AS3514) && !defined(HAVE_WM8975) \
  && !defined(HAVE_WM8758) && !defined(HAVE_TSC2100) \
  && !defined (HAVE_WM8711) && !defined (HAVE_WM8721) \
  && !defined (HAVE_WM8731) && !defined (HAVE_WM8978) \
  && !defined(HAVE_AK4537)) || defined(SIMULATOR)
int sound_val2phys(int setting, int value)
{
#if CONFIG_CODEC == MAS3587F
    int result = 0;

    switch(setting)
    {
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
            result = (value - 2) * 15;
            break;

        case SOUND_MIC_GAIN:
            result = value * 15 + 210;
            break;

       default:
            result = value;
            break;
    }
    return result;
#elif defined(HAVE_UDA1380)
    int result = 0;
    
    switch(setting)
    {
#ifdef HAVE_RECORDING
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
        case SOUND_MIC_GAIN:
            result = value * 5;         /* (1/2) * 10 */
            break;
#endif
        default:
            result = value;
            break;
    }
    return result;
#elif defined(HAVE_TLV320) || defined(HAVE_WM8711) \
      || defined(HAVE_WM8721) || defined(HAVE_WM8731)
    int result = 0;
    
    switch(setting)
    {
#ifdef HAVE_RECORDING
        case SOUND_LEFT_GAIN:
        case SOUND_RIGHT_GAIN:
            result = (value - 23) * 15;    /* (x - 23)/1.5 *10 */
            break;

        case SOUND_MIC_GAIN:
            result = value * 200;          /* 0 or 20 dB */
            break;
#endif
       default:
            result = value;
            break;
    }
    return result;
#elif defined(HAVE_AS3514)
    /* This is here for the sim only and the audio driver has its own */
    int result;

    switch(setting)
    {
#ifdef HAVE_RECORDING
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;
#endif
    default:
        result = value;
        break;
    }

    return result;
#elif defined(HAVE_WM8978)
    int result;

    switch (setting)
    {
#ifdef HAVE_RECORDING
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = value * 5;
        break;
#endif

    case SOUND_DEPTH_3D:
        result = (100 * value + 8) / 15;
        break;

    default:
        result = value;
    }

    return result;
#else
    (void)setting;
    return value;
#endif
}
#endif /* !defined(HAVE_AS3514) || defined(SIMULATOR) */

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
/* This function works by telling the decoder that we have another
   crystal frequency than we actually have. It will adjust its internal
   parameters and the result is that the audio is played at another pitch.

   The pitch value precision is based on PITCH_SPEED_PRECISION (in dsp.h)
*/

#ifdef SIMULATOR
static
#else
extern
#endif
        unsigned long shadow_io_control_main;
static int last_pitch = PITCH_SPEED_100;

void sound_set_pitch(int32_t pitch)
{
    unsigned long val;

    if (pitch != last_pitch)
    {
        /* Calculate the new (bogus) frequency */
        val = 18432 * PITCH_SPEED_100 / pitch;

        mas_writemem(MAS_BANK_D0, MAS_D0_OFREQ_CONTROL, &val, 1);

        /* We must tell the MAS that the frequency has changed.
         * This will unfortunately cause a short silence. */
        mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);

        last_pitch = pitch;
    }
}

int32_t sound_get_pitch(void)
{
    return last_pitch;
}
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
