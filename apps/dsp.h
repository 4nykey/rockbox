/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _DSP_H
#define _DSP_H

#include <stdlib.h>
#include <stdbool.h>

#define NATIVE_FREQUENCY       44100
enum
{
    STEREO_INTERLEAVED = 0,
    STEREO_NONINTERLEAVED,
    STEREO_MONO,
    STEREO_NUM_MODES,
};

enum {
    CODEC_SET_FILEBUF_WATERMARK = 1,
    CODEC_SET_FILEBUF_CHUNKSIZE,
    CODEC_SET_FILEBUF_PRESEEK,
    DSP_SET_FREQUENCY,
    DSP_SWITCH_FREQUENCY,
    DSP_SET_CLIP_MIN,
    DSP_SET_CLIP_MAX,
    DSP_SET_SAMPLE_DEPTH,
    DSP_SET_STEREO_MODE,
    DSP_RESET,
    DSP_FLUSH,
    DSP_SET_TRACK_GAIN,
    DSP_SET_ALBUM_GAIN,
    DSP_SET_TRACK_PEAK,
    DSP_SET_ALBUM_PEAK,
    DSP_CROSSFEED
};

/* A bunch of fixed point assembler helper macros */
#if defined(CPU_COLDFIRE) && !defined(SIMULATOR)
/* These macros use the Coldfire EMAC extension and need the MACSR flags set
 * to fractional mode with no rounding.
 */

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm ("mac.l    %[a], %[b], %%acc0\n\t" \
         "movclr.l %%acc0, %[t]\n\t" \
         : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})

/* Multiply two S.31 fractional integers, and return the 32 most significant
 * bits after a shift left by the constant z. NOTE: Only works for shifts of
 * up to 8 on Coldfire!
 */
#define FRACMUL_SHL(x, y, z) \
({ \
    long t, t2; \
    asm ("mac.l    %[a], %[b], %%acc0\n\t" \
         "moveq.l  %[d], %[t]\n\t" \
         "move.l   %%accext01, %[t2]\n\t" \
         "and.l    %[mask], %[t2]\n\t" \
         "lsr.l    %[t], %[t2]\n\t" \
         "movclr.l %%acc0, %[t]\n\t" \
         "asl.l    %[c], %[t]\n\t" \
         "or.l     %[t2], %[t]\n\t" \
         : [t] "=&d" (t), [t2] "=&d" (t2) \
         : [a] "r" (x), [b] "r" (y), [mask] "d" (0xff), \
           [c] "i" ((z)), [d] "i" (8 - (z))); \
    t; \
})

/* Multiply one S.31-bit and one S8.23 fractional integer and return the
 * sign bit and the 31 most significant bits of the result. Load next value
 * to multiply with into x from s (and increase s); x must contain the
 * initial value.
 */
#define FRACMUL_8_LOOP(x, y, s, d) \
{ \
    long t, t2; \
    asm volatile ("mac.l    %[a], %[b], (%[src])+, %[a], %%acc0\n\t" \
                  "move.l   %%accext01, %[t2]\n\t" \
                  "movclr.l %%acc0, %[t]\n\t" \
                  "asl.l    #8, %[t]\n\t" \
                  "move.b   %[t2], %[t]\n\t" \
                  "move.l   %[t], (%[dst])+\n\t" \
                  : [a] "+r" (x), [src] "+a" (s), [dst] "+a" (d), \
                    [t] "=r" (t), [t2] "=r" (t2) \
                  : [b] "r" (y)); \
}

#define ACC(acc, x, y) \
    (void)acc; \
    asm ("mac.l %[a], %[b], %%acc0" \
         : : [a] "i,r" (x), [b] "i,r" (y));

#define GET_ACC(acc) \
({ \
    long t; \
    (void)acc; \
    asm ("movclr.l %%acc0, %[t]" \
         : [t] "=r" (t)); \
    t; \
})

#define ACC_INIT(acc, x, y) ACC(acc, x, y)

#elif defined(CPU_ARM) && !defined(SIMULATOR)

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t, t2; \
    asm ("smull    %[t], %[t2], %[a], %[b]\n\t" \
         "mov      %[t2], %[t2], asl #1\n\t" \
         "orr      %[t], %[t2], %[t], lsr #31\n\t" \
         : [t] "=&r" (t), [t2] "=&r" (t2) \
         : [a] "r" (x), [b] "r" (y)); \
    t; \
})

/* Multiply two S.31 fractional integers, and return the 32 most significant
 * bits after a shift left by the constant z.
 */
#define FRACMUL_SHL(x, y, z) \
({ \
    long t, t2; \
    asm ("smull    %[t], %[t2], %[a], %[b]\n\t" \
         "mov      %[t2], %[t2], asl %[c]\n\t" \
         "orr      %[t], %[t2], %[t], lsr %[d]\n\t" \
         : [t] "=&r" (t), [t2] "=&r" (t2) \
         : [a] "r" (x), [b] "r" (y), \
           [c] "M" ((z) + 1), [d] "M" (31 - (z))); \
    t; \
})

#define ACC_INIT(acc, x, y) acc = FRACMUL(x, y)
#define ACC(acc, x, y) acc += FRACMUL(x, y)
#define GET_ACC(acc) acc

/* Multiply one S.31-bit and one S8.23 fractional integer and store the
 * sign bit and the 31 most significant bits of the result to d (and
 * increase d). Load next value to multiply with into x from s (and
 * increase s); x must contain the initial value.
 */
#define FRACMUL_8_LOOP(x, y, s, d) \
({ \
    long t, t2; \
    asm volatile ("smull    %[t], %[t2], %[a], %[b]\n\t" \
                  "mov      %[t2], %[t2], asl #9\n\t" \
                  "orr      %[d], %[t2], %[t], lsr #23\n\t" \
                  : [d] "=&r" (*(d)++), [t] "=&r" (t), [t2] "=&r" (t2) \
                  : [a] "r" (x), [b] "r" (y)); \
    x = *(s)++; \
})

#else

#define ACC_INIT(acc, x, y) acc = FRACMUL(x, y)
#define ACC(acc, x, y) acc += FRACMUL(x, y)
#define GET_ACC(acc) acc
#define FRACMUL(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 31))
#define FRACMUL_SHL(x, y, z) \
((long)(((((long long) (x)) * ((long long) (y))) >> (31 - (z)))))
#define FRACMUL_8_LOOP(x, y, s, d) \
({ \
    long t = x; \
    x = *(s)++; \
    *(d)++ = (long) (((((long long) (t)) * ((long long) (y))) >> 23)); \
})

#endif

#define DIV64(x, y, z) (long)(((long long)(x) << (z))/(y))

int dsp_process(char *dest, const char *src[], int count);
int dsp_input_count(int count);
int dsp_output_count(int count);
int dsp_stereo_mode(void);
bool dsp_configure(int setting, intptr_t value);
void dsp_set_replaygain(bool always);
void dsp_set_crossfeed(bool enable);
void dsp_set_crossfeed_direct_gain(int gain);
void dsp_set_crossfeed_cross_params(long lf_gain, long hf_gain, long cutoff);
void dsp_set_eq(bool enable);
void dsp_set_eq_precut(int precut);
void dsp_set_eq_coefs(int band);
void sound_set_pitch(int r);
int sound_get_pitch(void);
void channels_set(int value);
void stereo_width_set(int value);
void dsp_dither_enable(bool enable);

#endif
