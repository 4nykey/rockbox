/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* The DSP portion (awe!) */

enum { voice_count = 8 };
enum { register_count = 128 };

struct raw_voice_t
{
    int8_t  volume [2];
    uint8_t rate [2];
    uint8_t waveform;
    uint8_t adsr [2];   /* envelope rates for attack, decay, and sustain */
    uint8_t gain;       /* envelope gain (if not using ADSR) */
    int8_t  envx;       /* current envelope level */
    int8_t  outx;       /* current sample */
    int8_t  unused [6];
};
    
struct globals_t
{
    int8_t  unused1 [12];
    int8_t  volume_0;         /* 0C   Main Volume Left (-.7) */
    int8_t  echo_feedback;    /* 0D   Echo Feedback (-.7) */
    int8_t  unused2 [14];
    int8_t  volume_1;         /* 1C   Main Volume Right (-.7) */
    int8_t  unused3 [15];
    int8_t  echo_volume_0;    /* 2C   Echo Volume Left (-.7) */
    uint8_t pitch_mods;       /* 2D   Pitch Modulation on/off for each voice */
    int8_t  unused4 [14];
    int8_t  echo_volume_1;    /* 3C   Echo Volume Right (-.7) */
    uint8_t noise_enables;    /* 3D   Noise output on/off for each voice */
    int8_t  unused5 [14];
    uint8_t key_ons;          /* 4C   Key On for each voice */
    uint8_t echo_ons;         /* 4D   Echo on/off for each voice */
    int8_t  unused6 [14];
    uint8_t key_offs;         /* 5C   key off for each voice
                                      (instantiates release mode) */
    uint8_t wave_page;        /* 5D   source directory (wave table offsets) */
    int8_t  unused7 [14];
    uint8_t flags;            /* 6C   flags and noise freq */
    uint8_t echo_page;        /* 6D */
    int8_t  unused8 [14];
    uint8_t wave_ended;       /* 7C */
    uint8_t echo_delay;       /* 7D   ms >> 4 */
    char    unused9 [2];
};

enum state_t {  /* -1, 0, +1 allows more efficient if statements */
    state_decay   = -1,
    state_sustain = 0,
    state_attack  = +1,
    state_release = 2
};

struct cache_entry_t
{
    int16_t const* samples;
    unsigned end; /* past-the-end position */
    unsigned loop; /* number of samples in loop */
    unsigned start_addr;
};

enum { brr_block_size = 16 };

struct voice_t
{
#if SPC_BRRCACHE
    int16_t const* samples;
    long wave_end;
    int wave_loop;
#else
    int16_t samples [3 + brr_block_size + 1];
    int block_header; /* header byte from current block */
#endif
    uint8_t const* addr;
    short volume [2];
    long position;/* position in samples buffer, with 12-bit fraction */
    short envx;
    short env_mode;
    short env_timer;
    short key_on_delay;
};

#if SPC_BRRCACHE
/* a little extra for samples that go past end */
static int16_t BRRcache [0x20000 + 32];
#endif

enum { fir_buf_half = 8 };

#ifdef CPU_COLDFIRE
/* global because of the large aligment requirement for hardware masking -
 * L-R interleaved 16-bit samples for easy loading and mac.w use.
 */
enum
{
    fir_buf_size = fir_buf_half * sizeof ( int32_t ),
    fir_buf_mask = ~fir_buf_size
};
int32_t fir_buf[fir_buf_half]
    __attribute__ ((aligned (fir_buf_size*2))) IBSS_ATTR;
#endif /* CPU_COLDFIRE */

struct Spc_Dsp
{
    union
    {
        struct raw_voice_t voice [voice_count];
        uint8_t reg [register_count];
        struct globals_t g;
        int16_t align;
    } r;
    
    unsigned echo_pos;
    int keys_down;
    int noise_count;
    uint16_t noise; /* also read as int16_t */
    
#ifdef CPU_COLDFIRE
    /* circularly hardware masked address */
    int32_t *fir_ptr;
    /* wrapped address just behind current position -
       allows mac.w to increment and mask fir_ptr */
    int32_t *last_fir_ptr;
    /* copy of echo FIR constants as int16_t for use with mac.w */
    int16_t fir_coeff[voice_count];
#else
    /* fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code */
    int fir_pos; /* (0 to 7) */
    int fir_buf [fir_buf_half * 2] [2];
    /* copy of echo FIR constants as int, for faster access */
    int fir_coeff [voice_count]; 
#endif
    
    struct voice_t voice_state [voice_count];
    
#if SPC_BRRCACHE
    uint8_t oldsize;
    struct cache_entry_t wave_entry     [256];
    struct cache_entry_t wave_entry_old [256];
#endif
};

struct src_dir
{
    char start [2];
    char loop  [2];
};

static void DSP_reset( struct Spc_Dsp* this )
{
    this->keys_down   = 0;
    this->echo_pos    = 0;
    this->noise_count = 0;
    this->noise       = 2;
    
    this->r.g.flags   = 0xE0; /* reset, mute, echo off */
    this->r.g.key_ons = 0;
    
    memset( this->voice_state, 0, sizeof this->voice_state );
    
    int i;
    for ( i = voice_count; --i >= 0; )
    {
        struct voice_t* v = this->voice_state + i;
        v->env_mode = state_release;
        v->addr     = ram.ram;
    }
    
    #if SPC_BRRCACHE
        this->oldsize = 0;
        for ( i = 0; i < 256; i++ )
            this->wave_entry [i].start_addr = -1;
    #endif

#ifdef CPU_COLDFIRE
    this->fir_ptr      = fir_buf;
    this->last_fir_ptr = &fir_buf [7];
    memset( fir_buf, 0, sizeof fir_buf );
#else
    this->fir_pos = 0;
    memset( this->fir_buf, 0, sizeof this->fir_buf );
#endif

    assert( offsetof (struct globals_t,unused9 [2]) == register_count );
    assert( sizeof (this->r.voice) == register_count );
}

static void DSP_write( struct Spc_Dsp* this, int i, int data ) ICODE_ATTR;
static void DSP_write( struct Spc_Dsp* this, int i, int data )
{
    assert( (unsigned) i < register_count );
    
    this->r.reg [i] = data;
    int high = i >> 4;
    int low  = i & 0x0F;
    if ( low < 2 ) /* voice volumes */
    {
        int left  = *(int8_t const*) &this->r.reg [i & ~1];
        int right = *(int8_t const*) &this->r.reg [i |  1];
        struct voice_t* v = this->voice_state + high;
        v->volume [0] = left;
        v->volume [1] = right;
    }
    else if ( low == 0x0F ) /* fir coefficients */
    {
        this->fir_coeff [7 - high] = (int8_t) data; /* sign-extend */
    }
}

static inline int DSP_read( struct Spc_Dsp* this, int i )
{
    assert( (unsigned) i < register_count );
    return this->r.reg [i];
}
    
/* if ( n < -32768 ) out = -32768; */
/* if ( n >  32767 ) out =  32767; */
#define CLAMP16( n, out )\
{\
    if ( (int16_t) n != n )\
        out = 0x7FFF ^ (n >> 31);\
}

#if SPC_BRRCACHE
static void decode_brr( struct Spc_Dsp* this, unsigned start_addr,
                        struct voice_t* voice,
                        struct raw_voice_t const* const raw_voice ) ICODE_ATTR;
static void decode_brr( struct Spc_Dsp* this, unsigned start_addr,
                        struct voice_t* voice,
                        struct raw_voice_t const* const raw_voice )
{
    /* setup same variables as where decode_brr() is called from */
    #undef RAM
    #define RAM ram.ram
    struct src_dir const* const sd =
        (struct src_dir*) &RAM [this->r.g.wave_page * 0x100];
    struct cache_entry_t* const wave_entry =
        &this->wave_entry [raw_voice->waveform];
    
    /* the following block can be put in place of the call to
       decode_brr() below
    */
    {
        DEBUGF( "decode at %08x (wave #%d)\n",
                start_addr, raw_voice->waveform );
        
        /* see if in cache */
        int i;
        for ( i = 0; i < this->oldsize; i++ )
        {
            struct cache_entry_t* e = &this->wave_entry_old [i];
            if ( e->start_addr == start_addr )
            {
                DEBUGF( "found in wave_entry_old (oldsize=%d)\n",
                    this->oldsize );
                *wave_entry = *e;
                goto wave_in_cache;
            }
        }
        
        wave_entry->start_addr = start_addr;
        
        uint8_t const* const loop_ptr =
            RAM + GET_LE16A( sd [raw_voice->waveform].loop );
        short* loop_start = 0;
        
        short* out = BRRcache + start_addr * 2;
        wave_entry->samples = out;
        *out++ = 0;
        int smp1 = 0;
        int smp2 = 0;
        
        uint8_t const* addr = RAM + start_addr;
        int block_header;
        do
        {
            if ( addr == loop_ptr )
            {
                loop_start = out;
                DEBUGF( "loop at %08x (wave #%d)\n", addr - RAM, raw_voice->waveform );
            }
            
            /* header */
            block_header = *addr;
            addr += 9;
            voice->addr = addr;
            int const filter = (block_header & 0x0C) - 0x08;
            
            /* scaling
               (invalid scaling gives -4096 for neg nybble, 0 for pos) */
            static unsigned char const right_shifts [16] = {
                5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  4,  4, 29, 29, 29,
            };
            static unsigned char const left_shifts  [16] = {
                0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11, 11, 11
            };
            int const scale = block_header >> 4;
            int const right_shift = right_shifts [scale];
            int const left_shift  = left_shifts  [scale];
            
            /* output position */
            out += brr_block_size;
            int offset = -brr_block_size << 2;
            
            do /* decode and filter 16 samples */
            {
                /* Get nybble, sign-extend, then scale
                   get byte, select which nybble, sign-extend, then shift based
                   on scaling. also handles invalid scaling values. */
                int delta = (int) (int8_t) (addr [offset >> 3] << (offset & 4))
                        >> right_shift << left_shift;
                
                out [offset >> 2] = smp2;
                
                if ( filter == 0 ) /* mode 0x08 (30-90% of the time) */
                {
                    delta -= smp2 >> 1;
                    delta += smp2 >> 5;
                    smp2 = smp1;
                    delta += smp1;
                    delta += (-smp1 - (smp1 >> 1)) >> 5;
                }
                else
                {
                    if ( filter == -4 ) /* mode 0x04 */
                    {
                        delta += smp1 >> 1;
                        delta += (-smp1) >> 5;
                    }
                    else if ( filter > -4 ) /* mode 0x0C */
                    {
                        delta -= smp2 >> 1;
                        delta += (smp2 + (smp2 >> 1)) >> 4;
                        delta += smp1;
                        delta += (-smp1 * 13) >> 7;
                    }
                    smp2 = smp1;
                }
                
                CLAMP16( delta, delta );
                smp1 = (int16_t) (delta * 2); /* sign-extend */
            }
            while ( (offset += 4) != 0 );
            
            /* if next block has end flag set, this block ends early */
            /* (verified) */
            if ( (block_header & 3) != 3 && (*addr & 3) == 1 )
            {
                /* skip last 9 samples */
                out -= 9;
                goto early_end;
            }
        }
        while ( !(block_header & 1) && addr < RAM + 0x10000 );
        
        out [0] = smp2;
        out [1] = smp1;
        
    early_end:
        wave_entry->end = (out - 1 - wave_entry->samples) << 12;
        
        wave_entry->loop = 0;
        if ( (block_header & 2) )
        {
            if ( loop_start )
            {
                int loop = out - loop_start;
                wave_entry->loop = loop;
                wave_entry->end += 0x3000;
                out [2] = loop_start [2];
                out [3] = loop_start [3];
                out [4] = loop_start [4];
            }
            else
            {
                DEBUGF( "loop point outside initial wave\n" );
            }
        }
        
        DEBUGF( "end at %08x (wave #%d)\n", addr - RAM, raw_voice->waveform );
        
        /* add to cache */
        this->wave_entry_old [this->oldsize++] = *wave_entry;
wave_in_cache:;
    }
}
#endif

static void key_on(struct Spc_Dsp* const this, struct voice_t* const voice,
                   struct src_dir const* const sd,
                   struct raw_voice_t const* const raw_voice,
                   const int key_on_delay, const int vbit) ICODE_ATTR;
static void key_on(struct Spc_Dsp* const this, struct voice_t* const voice,
                   struct src_dir const* const sd,
                   struct raw_voice_t const* const raw_voice,
                   const int key_on_delay, const int vbit) {
    #undef RAM
    #define RAM ram.ram
    int const env_rate_init = 0x7800;
    voice->key_on_delay = key_on_delay;
    if ( key_on_delay == 0 )
    {
        this->keys_down |= vbit;
        voice->envx         = 0;
        voice->env_mode     = state_attack;
        voice->env_timer    = env_rate_init; /* TODO: inaccurate? */
        unsigned start_addr = GET_LE16A(sd [raw_voice->waveform].start);
        #if !SPC_BRRCACHE
        {
            voice->addr = RAM + start_addr;
            /* BRR filter uses previous samples */
            voice->samples [brr_block_size + 1] = 0;
            voice->samples [brr_block_size + 2] = 0;
            /* decode three samples immediately */
            voice->position     = (brr_block_size + 3) * 0x1000 - 1;
            voice->block_header = 0; /* "previous" BRR header */
        }
        #else
        {
            voice->position = 3 * 0x1000 - 1;
            struct cache_entry_t* const wave_entry =
                &this->wave_entry [raw_voice->waveform];
            
            /* predecode BRR if not already */
            if ( wave_entry->start_addr != start_addr )
            {
                /* the following line can be replaced by the indicated block
                   in decode_brr() */
                decode_brr( this, start_addr, voice, raw_voice );
            }
            
            voice->samples   = wave_entry->samples;
            voice->wave_end  = wave_entry->end;
                    voice->wave_loop = wave_entry->loop;
        }
        #endif
    }
}

static void DSP_run_( struct Spc_Dsp* this, long count, int32_t* out_buf )
    ICODE_ATTR;
static void DSP_run_( struct Spc_Dsp* this, long count, int32_t* out_buf )
{
    #undef RAM
#ifdef CPU_ARM
    uint8_t* const ram_ = ram.ram;
    #define RAM ram_
#else
    #define RAM ram.ram
#endif
#if 0
    EXIT_TIMER(cpu);
    ENTER_TIMER(dsp);
#endif

    /* Here we check for keys on/off.  Docs say that successive writes
       to KON/KOF must be separated by at least 2 Ts periods or risk
       being neglected.  Therefore DSP only looks at these during an
       update, and not at the time of the write.  Only need to do this
       once however, since the regs haven't changed over the whole
       period we need to catch up with. */
    
    {
        int key_ons  = this->r.g.key_ons;
        int key_offs = this->r.g.key_offs;
        /* keying on a voice resets that bit in ENDX */
        this->r.g.wave_ended &= ~key_ons;
        /* key_off bits prevent key_on from being acknowledged */
        this->r.g.key_ons = key_ons & key_offs;
        
        /* process key events outside loop, since they won't re-occur */
        struct voice_t* voice = this->voice_state + 8;
        int vbit = 0x80;
        do
        {
            --voice;
            if ( key_offs & vbit )
            {
                voice->env_mode     = state_release;
                voice->key_on_delay = 0;
            }
            else if ( key_ons & vbit )
            {
                voice->key_on_delay = 8;
            }
        }
        while ( (vbit >>= 1) != 0 );
    }
    
    struct src_dir const* const sd =
        (struct src_dir*) &RAM [this->r.g.wave_page * 0x100];

    #ifdef ROCKBOX_BIG_ENDIAN
        /* Convert endiannesses before entering loops - these
           get used alot */
        const uint32_t rates[voice_count] =
        {
            GET_LE16A( this->r.voice[0].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[1].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[2].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[3].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[4].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[5].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[6].rate ) & 0x3FFF,
            GET_LE16A( this->r.voice[7].rate ) & 0x3FFF,
        };
        #define VOICE_RATE(x) *(x)
        #define IF_RBE(...) __VA_ARGS__
    #ifdef CPU_COLDFIRE
        /* Initialize mask register with the buffer address mask */
        asm volatile ("move.l %[m], %%mask" : : [m]"i"(fir_buf_mask));
        const int echo_wrap  = (this->r.g.echo_delay & 15) * 0x800;
        const int echo_start = this->r.g.echo_page * 0x100;
    #endif /* CPU_COLDFIRE */
    #else
        #define VOICE_RATE(x) (INT16A(raw_voice->rate) & 0x3FFF)
        #define IF_RBE(...)
    #endif /* ROCKBOX_BIG_ENDIAN */
    
#if !SPC_NOINTERP
    int const slow_gaussian = (this->r.g.pitch_mods >> 1) |
        this->r.g.noise_enables;
#endif
    /* (g.flags & 0x40) ? 30 : 14 */
    int const global_muting = ((this->r.g.flags & 0x40) >> 2) + 14 - 8; 
    int const global_vol_0  = this->r.g.volume_0;
    int const global_vol_1  = this->r.g.volume_1;
    
    /* each rate divides exactly into 0x7800 without remainder */
    int const env_rate_init = 0x7800;
    static unsigned short const env_rates [0x20] ICONST_ATTR =
    {
        0x0000, 0x000F, 0x0014, 0x0018, 0x001E, 0x0028, 0x0030, 0x003C,
        0x0050, 0x0060, 0x0078, 0x00A0, 0x00C0, 0x00F0, 0x0140, 0x0180,
        0x01E0, 0x0280, 0x0300, 0x03C0, 0x0500, 0x0600, 0x0780, 0x0A00,
        0x0C00, 0x0F00, 0x1400, 0x1800, 0x1E00, 0x2800, 0x3C00, 0x7800
    };
    
    do /* one pair of output samples per iteration */
    {
        /* Noise */
        if ( this->r.g.noise_enables )
        {
            if ( (this->noise_count -=
                 env_rates [this->r.g.flags & 0x1F]) <= 0 )
            {
                this->noise_count = env_rate_init;
                int feedback = (this->noise << 13) ^ (this->noise << 14);
                this->noise = (feedback & 0x8000) ^ (this->noise >> 1 & ~1);
            }
        }
        
#if !SPC_NOECHO
        int echo_0 = 0;
        int echo_1 = 0;
#endif
        long prev_outx = 0; /* TODO: correct value for first channel? */
        int chans_0 = 0;
        int chans_1 = 0;
        /* TODO: put raw_voice pointer in voice_t? */
        struct raw_voice_t * raw_voice = this->r.voice;
        struct voice_t* voice = this->voice_state;
        int vbit = 1;
        IF_RBE( const uint32_t* vr = rates; )
        for ( ; vbit < 0x100; vbit <<= 1, ++voice, ++raw_voice IF_RBE( , ++vr ) )
        {
            /* pregen involves checking keyon, etc */
#if 0
            ENTER_TIMER(dsp_pregen);
#endif
            
            /* Key on events are delayed */
            int key_on_delay = voice->key_on_delay;

            if ( --key_on_delay >= 0 ) /* <1% of the time */
            {
                key_on(this,voice,sd,raw_voice,key_on_delay,vbit);
            }
            
            if ( !(this->keys_down & vbit) ) /* Silent channel */
            {
        silent_chan:
                raw_voice->envx = 0;
                raw_voice->outx = 0;
                prev_outx = 0;
                continue;
            }
            
            /* Envelope */
            {
                int const env_range = 0x800;
                int env_mode = voice->env_mode;
                int adsr0 = raw_voice->adsr [0];
                int env_timer;
                if ( env_mode != state_release ) /* 99% of the time */
                {
                    env_timer = voice->env_timer;
                    if ( adsr0 & 0x80 ) /* 79% of the time */
                    {
                        int adsr1 = raw_voice->adsr [1];
                        if ( env_mode == state_sustain ) /* 74% of the time */
                        {
                            if ( (env_timer -= env_rates [adsr1 & 0x1F]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            envx--; /* envx *= 255 / 256 */
                            envx -= envx >> 8;
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                        else if ( env_mode < 0 ) /* 25% state_decay */
                        {
                            int envx = voice->envx;
                            if ( (env_timer -=
                                env_rates [(adsr0 >> 3 & 0x0E) + 0x10]) <= 0 )
                            {
                                envx--; /* envx *= 255 / 256 */
                                envx -= envx >> 8;
                                voice->envx = envx;
                                /* TODO: should this be 8? */
                                raw_voice->envx = envx >> 4;
                                env_timer = env_rate_init;
                            }
                            
                            int sustain_level = adsr1 >> 5;
                            if ( envx <= (sustain_level + 1) * 0x100 )
                                voice->env_mode = state_sustain;
                            
                            goto write_env_timer;
                        }
                        else /* state_attack */
                        {
                            int t = adsr0 & 0x0F;
                            if ( (env_timer -= env_rates [t * 2 + 1]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            
                            int const step = env_range / 64;
                            envx += step;
                            if ( t == 15 )
                                envx += env_range / 2 - step;
                            
                            if ( envx >= env_range )
                            {
                                envx = env_range - 1;
                                voice->env_mode = state_decay;
                            }
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                    }
                    else /* gain mode */
                    {
                        int t = raw_voice->gain;
                        if ( t < 0x80 )
                        {
                            raw_voice->envx = t;
                            voice->envx = t << 4;
                            goto env_end;
                        }
                        else
                        {
                            if ( (env_timer -= env_rates [t & 0x1F]) > 0 )
                                goto write_env_timer;
                            
                            int envx = voice->envx;
                            int mode = t >> 5;
                            if ( mode <= 5 ) /* decay */
                            {
                                int step = env_range / 64;
                                if ( mode == 5 ) /* exponential */
                                {
                                    envx--; /* envx *= 255 / 256 */
                                    step = envx >> 8;
                                }
                                if ( (envx -= step) < 0 )
                                {
                                    envx = 0;
                                    if ( voice->env_mode == state_attack )
                                        voice->env_mode = state_decay;
                                }
                            }
                            else /* attack */
                            {
                                int const step = env_range / 64;
                                envx += step;
                                if ( mode == 7 &&
                                     envx >= env_range * 3 / 4 + step )
                                    envx += env_range / 256 - step;
                                
                                if ( envx >= env_range )
                                    envx = env_range - 1;
                            }
                            voice->envx = envx;
                            /* TODO: should this be 8? */
                            raw_voice->envx = envx >> 4;
                            goto init_env_timer;
                        }
                    }
                }
                else /* state_release */
                {
                    int envx = voice->envx;
                    if ( (envx -= env_range / 256) > 0 )
                    {
                        voice->envx = envx;
                        raw_voice->envx = envx >> 8;
                        goto env_end;
                    }
                    else
                    {
                        /* bit was set, so this clears it */
                        this->keys_down ^= vbit;
                        voice->envx = 0;
                        goto silent_chan;
                    }
                }
            init_env_timer:
                env_timer = env_rate_init;
            write_env_timer:
                voice->env_timer = env_timer;
            env_end:;
            }
#if 0            
            EXIT_TIMER(dsp_pregen);
            
            ENTER_TIMER(dsp_gen);
#endif
            #if !SPC_BRRCACHE
            /* Decode BRR block */
            if ( voice->position >= brr_block_size * 0x1000 )
            {
                voice->position -= brr_block_size * 0x1000;
                
                uint8_t const* addr = voice->addr;
                if ( addr >= RAM + 0x10000 )
                    addr -= 0x10000;
                
                /* action based on previous block's header */
                if ( voice->block_header & 1 )
                {
                    addr = RAM + GET_LE16A( sd [raw_voice->waveform].loop );
                    this->r.g.wave_ended |= vbit;
                    if ( !(voice->block_header & 2) ) /* 1% of the time */
                    {
                        /* first block was end block;
                           don't play anything (verified) */
                        /* bit was set, so this clears it */
                        this->keys_down ^= vbit; 
                        
                        /* since voice->envx is 0,
                           samples and position don't matter */
                        raw_voice->envx = 0;
                        voice->envx = 0;
                        goto skip_decode;
                    }
                }
                
                /* header */
                int const block_header = *addr;
                addr += 9;
                voice->addr = addr;
                voice->block_header = block_header;
                int const filter = (block_header & 0x0C) - 0x08;
                
                /* scaling (invalid scaling gives -4096 for neg nybble,
                   0 for pos) */
                static unsigned char const right_shifts [16] = {
                    5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  4,  4, 29, 29, 29,
                };
                static unsigned char const left_shifts  [16] = {
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11, 11, 11
                };
                int const scale = block_header >> 4;
                int const right_shift = right_shifts [scale];
                int const left_shift  = left_shifts  [scale];
                
                /* previous samples */
                int smp2 = voice->samples [brr_block_size + 1];
                int smp1 = voice->samples [brr_block_size + 2];
                voice->samples [0] = voice->samples [brr_block_size];
                
                /* output position */
                short* out = voice->samples + (1 + brr_block_size);
                int offset = -brr_block_size << 2;
                
                /* if next block has end flag set,
                   this block ends early (verified) */
                if ( (block_header & 3) != 3 && (*addr & 3) == 1 )
                {
                    /* arrange for last 9 samples to be skipped */
                    int const skip = 9;
                    out += (skip & 1);
                    voice->samples [skip] = voice->samples [brr_block_size];
                    voice->position += skip * 0x1000;
                    offset = (-brr_block_size + (skip & ~1)) << 2;
                    addr -= skip / 2;
                    /* force sample to end on next decode */
                    voice->block_header = 1;
                }
                
                do /* decode and filter 16 samples */
                {
                    /* Get nybble, sign-extend, then scale
                       get byte, select which nybble, sign-extend, then shift
                       based on scaling. also handles invalid scaling values.*/
                    int delta = (int) (int8_t) (addr [offset >> 3] <<
                            (offset & 4)) >> right_shift << left_shift;
                    
                    out [offset >> 2] = smp2;
                    
                    if ( filter == 0 ) /* mode 0x08 (30-90% of the time) */
                    {
                        delta -= smp2 >> 1;
                        delta += smp2 >> 5;
                        smp2 = smp1;
                        delta += smp1;
                        delta += (-smp1 - (smp1 >> 1)) >> 5;
                    }
                    else
                    {
                        if ( filter == -4 ) /* mode 0x04 */
                        {
                            delta += smp1 >> 1;
                            delta += (-smp1) >> 5;
                        }
                        else if ( filter > -4 ) /* mode 0x0C */
                        {
                            delta -= smp2 >> 1;
                            delta += (smp2 + (smp2 >> 1)) >> 4;
                            delta += smp1;
                            delta += (-smp1 * 13) >> 7;
                        }
                        smp2 = smp1;
                    }
                    
                    CLAMP16( delta, delta );
                    smp1 = (int16_t) (delta * 2); /* sign-extend */
                }
                while ( (offset += 4) != 0 );
                
                out [0] = smp2;
                out [1] = smp1;
                
            skip_decode:;
            }
            #endif

            /* Get rate (with possible modulation) */
            int rate = VOICE_RATE(vr);
            if ( this->r.g.pitch_mods & vbit )
                rate = (rate * (prev_outx + 32768)) >> 15;

        #if !SPC_NOINTERP
            /* Interleved gauss table (to improve cache coherency). */
            /* gauss [i * 2 + j] = normal_gauss [(1 - j) * 256 + i] */
            static short const gauss [512] =
            {
370,1305, 366,1305, 362,1304, 358,1304, 354,1304, 351,1304, 347,1304, 343,1303,
339,1303, 336,1303, 332,1302, 328,1302, 325,1301, 321,1300, 318,1300, 314,1299,
311,1298, 307,1297, 304,1297, 300,1296, 297,1295, 293,1294, 290,1293, 286,1292,
283,1291, 280,1290, 276,1288, 273,1287, 270,1286, 267,1284, 263,1283, 260,1282,
257,1280, 254,1279, 251,1277, 248,1275, 245,1274, 242,1272, 239,1270, 236,1269,
233,1267, 230,1265, 227,1263, 224,1261, 221,1259, 218,1257, 215,1255, 212,1253,
210,1251, 207,1248, 204,1246, 201,1244, 199,1241, 196,1239, 193,1237, 191,1234,
188,1232, 186,1229, 183,1227, 180,1224, 178,1221, 175,1219, 173,1216, 171,1213,
168,1210, 166,1207, 163,1205, 161,1202, 159,1199, 156,1196, 154,1193, 152,1190,
150,1186, 147,1183, 145,1180, 143,1177, 141,1174, 139,1170, 137,1167, 134,1164,
132,1160, 130,1157, 128,1153, 126,1150, 124,1146, 122,1143, 120,1139, 118,1136,
117,1132, 115,1128, 113,1125, 111,1121, 109,1117, 107,1113, 106,1109, 104,1106,
102,1102, 100,1098,  99,1094,  97,1090,  95,1086,  94,1082,  92,1078,  90,1074,
 89,1070,  87,1066,  86,1061,  84,1057,  83,1053,  81,1049,  80,1045,  78,1040,
 77,1036,  76,1032,  74,1027,  73,1023,  71,1019,  70,1014,  69,1010,  67,1005,
 66,1001,  65, 997,  64, 992,  62, 988,  61, 983,  60, 978,  59, 974,  58, 969,
 56, 965,  55, 960,  54, 955,  53, 951,  52, 946,  51, 941,  50, 937,  49, 932,
 48, 927,  47, 923,  46, 918,  45, 913,  44, 908,  43, 904,  42, 899,  41, 894,
 40, 889,  39, 884,  38, 880,  37, 875,  36, 870,  36, 865,  35, 860,  34, 855,
 33, 851,  32, 846,  32, 841,  31, 836,  30, 831,  29, 826,  29, 821,  28, 816,
 27, 811,  27, 806,  26, 802,  25, 797,  24, 792,  24, 787,  23, 782,  23, 777,
 22, 772,  21, 767,  21, 762,  20, 757,  20, 752,  19, 747,  19, 742,  18, 737,
 17, 732,  17, 728,  16, 723,  16, 718,  15, 713,  15, 708,  15, 703,  14, 698,
 14, 693,  13, 688,  13, 683,  12, 678,  12, 674,  11, 669,  11, 664,  11, 659,
 10, 654,  10, 649,  10, 644,   9, 640,   9, 635,   9, 630,   8, 625,   8, 620,
  8, 615,   7, 611,   7, 606,   7, 601,   6, 596,   6, 592,   6, 587,   6, 582,
  5, 577,   5, 573,   5, 568,   5, 563,   4, 559,   4, 554,   4, 550,   4, 545,
  4, 540,   3, 536,   3, 531,   3, 527,   3, 522,   3, 517,   2, 513,   2, 508,
  2, 504,   2, 499,   2, 495,   2, 491,   2, 486,   1, 482,   1, 477,   1, 473,
  1, 469,   1, 464,   1, 460,   1, 456,   1, 451,   1, 447,   1, 443,   1, 439,
  0, 434,   0, 430,   0, 426,   0, 422,   0, 418,   0, 414,   0, 410,   0, 405,
  0, 401,   0, 397,   0, 393,   0, 389,   0, 385,   0, 381,   0, 378,   0, 374,
            };
            /* Gaussian interpolation using most recent 4 samples */
            long position = voice->position;
            voice->position += rate;
            short const* interp = voice->samples + (position >> 12);
            int offset = position >> 4 & 0xFF;
            
            /* Only left half of gaussian kernel is in table, so we must mirror
               for right half */
            short const* fwd = gauss       + offset * 2;
            short const* rev = gauss + 510 - offset * 2;
            
            /* Use faster gaussian interpolation when exact result isn't needed
               by pitch modulator of next channel */
            int amp_0, amp_1;
            if ( !(slow_gaussian & vbit) ) /* 99% of the time */
            {
                /* Main optimization is lack of clamping. Not a problem since
                   output never goes more than +/- 16 outside 16-bit range and
                   things are clamped later anyway. Other optimization is to
                   preserve fractional accuracy, eliminating several masks. */
                int output = (((fwd [0] * interp [0] +
                         fwd [1] * interp [1] +
                         rev [1] * interp [2] +
                         rev [0] * interp [3]    ) >> 11) * voice->envx) >> 11;
                
                /* duplicated here to give compiler more to run in parallel */
                amp_0 = voice->volume [0] * output;
                amp_1 = voice->volume [1] * output;
                raw_voice->outx = output >> 8;
            }
            else
            {
                int output = *(int16_t*) &this->noise;
                if ( !(this->r.g.noise_enables & vbit) )
                {
                    output = (fwd [0] * interp [0]) & ~0xFFF;
                    output = (output + fwd [1] * interp [1]) & ~0xFFF;
                    output = (output + rev [1] * interp [2]) >> 12;
                    output = (int16_t) (output * 2);
                    output += ((rev [0] * interp [3]) >> 12) * 2;
                    CLAMP16( output, output );
                }
                output = (output * voice->envx) >> 11 & ~1;
                
                /* duplicated here to give compiler more to run in parallel */
                amp_0 = voice->volume [0] * output;
                amp_1 = voice->volume [1] * output;
                prev_outx = output;
                raw_voice->outx = (int8_t) (output >> 8);
            }
        #else
        /* two-point linear interpolation */
        #ifdef CPU_COLDFIRE
            int amp_0 = (int16_t)this->noise;
            int amp_1;

            if ( (this->r.g.noise_enables & vbit) == 0 )
            {
                uint32_t f = voice->position;
                int32_t y0;

                /**
                 * Formula (fastest found so far of MANY):
                 * output = y0 + f*y1 - f*y0
                 */
                asm volatile (
                /* separate fractional and whole parts   */
                "move.l     %[f], %[y1]               \r\n"
                "and.l      #0xfff, %[f]              \r\n"
                "lsr.l      %[sh], %[y1]              \r\n"
                /* load samples y0 (upper) & y1 (lower)  */
                "move.l     2(%[s], %[y1].l*2), %[y1] \r\n"
                /* %acc0 = f*y1                          */
                "mac.w      %[f]l, %[y1]l, %%acc0     \r\n"
                /* msac.w is 2% boostier so add negative */
                "neg.l      %[f]                      \r\n"
                /* %acc0 -= f*y0                         */
                "mac.w      %[f]l, %[y1]u, %%acc0     \r\n"
                /* separate out y0 and sign extend       */
                "swap       %[y1]                     \r\n"
                "movea.w    %[y1], %[y0]              \r\n"
                /* fetch result, scale down and add y0   */
                "movclr.l   %%acc0, %[y1]             \r\n"
                /* output = y0 + (result >> 12)          */
                "asr.l      %[sh], %[y1]              \r\n"
                "add.l      %[y0], %[y1]              \r\n"
                : [f]"+&d"(f), [y0]"=&a"(y0), [y1]"=&d"(amp_0)
                : [s]"a"(voice->samples), [sh]"d"(12)
                    );
            }

            /* apply voice envelope to output */
            asm volatile (
            "mac.w %[output]l, %[envx]l, %%acc0 \r\n"
            :
            : [output]"r"(amp_0), [envx]"r"(voice->envx)
            );

            /* advance voice position */
            voice->position += rate;

            /* fetch output, scale and apply left and right
               voice volume */
            asm volatile (
            "movclr.l %%acc0,    %[output]         \r\n"
            "asr.l    %[sh],     %[output]         \r\n"
            "mac.l    %[vvol_0], %[output], %%acc0 \r\n"
            "mac.l    %[vvol_1], %[output], %%acc1 \r\n"
            : [output]"=&r"(amp_0)
            : [vvol_0]"r"((int)voice->volume[0]),
              [vvol_1]"r"((int)voice->volume[1]),
              [sh]"d"(11)
            );

            /* save this output into previous, scale and save in
               output register */
            prev_outx = amp_0;
            raw_voice->outx = amp_0 >> 8;

            /* fetch final voice output */
            asm volatile (
            "movclr.l %%acc0, %[amp_0] \r\n"
            "movclr.l %%acc1, %[amp_1] \r\n"
            : [amp_0]"=r"(amp_0), [amp_1]"=r"(amp_1)
            );
        #else

            /* Try this one out on ARM and see - similar to above but the asm
               on coldfire removes a redundant register load worth 1 or 2%;
               switching to loading two samples at once may help too. That's
               done above and while 6 to 7% faster on cf over two 16 bit loads
               it makes it endian dependant.
               
               measured small improvement (~1.5%) - hcs
            */

            int output;
            
            if ( (this->r.g.noise_enables & vbit) == 0 )
            {
                int const fraction = voice->position & 0xfff;
                short const* const pos = (voice->samples + (voice->position >> 12)) + 1;
                output = pos[0] + ((fraction * (pos[1] - pos[0])) >> 12);
            } else {
                output = *(int16_t *)&this->noise;
            }

            voice->position += rate;
            
            /* old version */
#if 0
            int fraction = voice->position & 0xFFF;
            short const* const pos = voice->samples + (voice->position >> 12);
            voice->position += rate;
            int output =
                (pos [2] * fraction + pos [1] * (0x1000 - fraction)) >> 12;
            /* no interpolation (hardly faster, and crappy sounding) */
            /*int output = pos [0];*/
            if ( this->r.g.noise_enables & vbit )
                output = *(int16_t*) &this->noise;
#endif
            output = (output * voice->envx) >> 11;

            /* duplicated here to give compiler more to run in parallel */
            int amp_0 = voice->volume [0] * output;
            int amp_1 = voice->volume [1] * output;

            prev_outx = output;
            raw_voice->outx = (int8_t) (output >> 8);
        #endif /* CPU_COLDFIRE */
        #endif
        
        #if SPC_BRRCACHE
            if ( voice->position >= voice->wave_end )
            {
                long loop_len = voice->wave_loop << 12;
                voice->position -= loop_len;
                this->r.g.wave_ended |= vbit;
                if ( !loop_len )
                {
                    this->keys_down ^= vbit;
                    raw_voice->envx = 0;
                    voice->envx = 0;
                }
            }
        #endif
#if 0
            EXIT_TIMER(dsp_gen);
            
            ENTER_TIMER(dsp_mix);
#endif            
            chans_0 += amp_0;
            chans_1 += amp_1;
            #if !SPC_NOECHO
                if ( this->r.g.echo_ons & vbit )
                {
                    echo_0 += amp_0;
                    echo_1 += amp_1;
                }
            #endif
#if 0            
            EXIT_TIMER(dsp_mix);
#endif
        }
        /* end of voice loop */
        
    #if !SPC_NOECHO
    #ifdef CPU_COLDFIRE
        /* Read feedback from echo buffer */
        int echo_pos = this->echo_pos;
        uint8_t* const echo_ptr = RAM + ((echo_start + echo_pos) & 0xFFFF);
        echo_pos += 4;
        if ( echo_pos >= echo_wrap )
            echo_pos = 0;
        this->echo_pos = echo_pos;
        int fb = swap_odd_even32(*(int32_t *)echo_ptr);
        int out_0, out_1;

        /* Keep last 8 samples */
        *this->last_fir_ptr = fb;
        this->last_fir_ptr  = this->fir_ptr;

        /* Apply echo FIR filter to output samples read from echo buffer -
           circular buffer is hardware incremented and masked; FIR
           coefficients and buffer history are loaded in parallel with
           multiply accumulate operations. Shift left by one here and once
           again when calculating feedback to have sample values justified
           to bit 31 in the output to ease endian swap, interleaving and
           clamping before placing result in the program's echo buffer. */
        int _0, _1, _2;
        asm volatile (
        "move.l                           (%[fir_c])  , %[_2]         \r\n"
        "mac.w      %[fb]u, %[_2]u, <<,   (%[fir_p])+&, %[_0], %%acc0 \r\n"
        "mac.w      %[fb]l, %[_2]u, <<,   (%[fir_p])& , %[_1], %%acc1 \r\n"
        "mac.w      %[_0]u, %[_2]l, <<                       , %%acc0 \r\n"
        "mac.w      %[_0]l, %[_2]l, <<,  4(%[fir_c])  , %[_2], %%acc1 \r\n"
        "mac.w      %[_1]u, %[_2]u, <<,  4(%[fir_p])& , %[_0], %%acc0 \r\n"
        "mac.w      %[_1]l, %[_2]u, <<,  8(%[fir_p])& , %[_1], %%acc1 \r\n"
        "mac.w      %[_0]u, %[_2]l, <<                       , %%acc0 \r\n"
        "mac.w      %[_0]l, %[_2]l, <<,  8(%[fir_c])  , %[_2], %%acc1 \r\n"
        "mac.w      %[_1]u, %[_2]u, <<, 12(%[fir_p])& , %[_0], %%acc0 \r\n"
        "mac.w      %[_1]l, %[_2]u, <<, 16(%[fir_p])& , %[_1], %%acc1 \r\n"
        "mac.w      %[_0]u, %[_2]l, <<                       , %%acc0 \r\n"
        "mac.w      %[_0]l, %[_2]l, <<, 12(%[fir_c])  , %[_2], %%acc1 \r\n"
        "mac.w      %[_1]u, %[_2]u, <<, 20(%[fir_p])& , %[_0], %%acc0 \r\n"
        "mac.w      %[_1]l, %[_2]u, <<                       , %%acc1 \r\n"
        "mac.w      %[_0]u, %[_2]l, <<                       , %%acc0 \r\n"
        "mac.w      %[_0]l, %[_2]l, <<                       , %%acc1 \r\n"
        : [_0]"=&r"(_0), [_1]"=&r"(_1), [_2]"=&r"(_2),
          [fir_p]"+a"(this->fir_ptr)
        : [fir_c]"a"(this->fir_coeff), [fb]"r"(fb)
        );

        /* Generate output */
        asm volatile (
        /* fetch filter results _after_ gcc loads asm
           block parameters to eliminate emac stalls   */
        "movclr.l   %%acc0, %[out_0]                \r\n"
        "movclr.l   %%acc1, %[out_1]                \r\n"
        /* apply global volume                         */
        "mac.l      %[chans_0], %[gv_0]    , %%acc2 \r\n"
        "mac.l      %[chans_1], %[gv_1]    , %%acc3 \r\n"
        /* apply echo volume and add to final output   */
        "mac.l      %[ev_0],   %[out_0], >>, %%acc2 \r\n"
        "mac.l      %[ev_1],   %[out_1], >>, %%acc3 \r\n"
        : [out_0]"=&r"(out_0), [out_1]"=&r"(out_1)
        : [chans_0]"r"(chans_0), [gv_0]"r"(global_vol_0),
          [ev_0]"r"((int)this->r.g.echo_volume_0),
          [chans_1]"r"(chans_1), [gv_1]"r"(global_vol_1),
          [ev_1]"r"((int)this->r.g.echo_volume_1)
        );

        /* Feedback into echo buffer */
        if ( !(this->r.g.flags & 0x20) )
        {
            asm volatile (
            /* scale echo voices; saturate if overflow */
            "mac.l      %[sh], %[e1]       , %%acc1 \r\n"
            "mac.l      %[sh], %[e0]       , %%acc0 \r\n"
            /* add scaled output from FIR filter       */
            "mac.l      %[out_1], %[ef], <<, %%acc1 \r\n"
            "mac.l      %[out_0], %[ef], <<, %%acc0 \r\n"
            /* swap and fetch feedback results - simply
               swap_odd_even32 mixed in between macs and
               movclrs to mitigate stall issues        */
            "move.l     #0x00ff00ff, %[sh]          \r\n"
            "movclr.l   %%acc1, %[e1]               \r\n"
            "swap       %[e1]                       \r\n"
            "movclr.l   %%acc0, %[e0]               \r\n"
            "move.w     %[e1], %[e0]                \r\n"
            "and.l      %[e0], %[sh]                \r\n"
            "eor.l      %[sh], %[e0]                \r\n"
            "lsl.l      #8, %[sh]                   \r\n"
            "lsr.l      #8, %[e0]                   \r\n"
            "or.l       %[sh], %[e0]                \r\n"
            /* save final feedback into echo buffer    */
            "move.l     %[e0], (%[echo_ptr])        \r\n"
            : [e0]"+&d"(echo_0), [e1]"+&d"(echo_1)
            : [out_0]"r"(out_0), [out_1]"r"(out_1),
              [ef]"r"((int)this->r.g.echo_feedback),
              [echo_ptr]"a"((int32_t *)echo_ptr),
              [sh]"d"(1 << 9)
            );
        }

        /* Output final samples */
        asm volatile (
        /* fetch output saved in %acc2 and %acc3 */
        "movclr.l   %%acc2, %[out_0] \r\n"
        "movclr.l   %%acc3, %[out_1] \r\n"
        /* scale right by global_muting shift    */
        "asr.l      %[gm],  %[out_0] \r\n"
        "asr.l      %[gm],  %[out_1] \r\n"
        : [out_0]"=&d"(out_0), [out_1]"=&d"(out_1)
        : [gm]"d"(global_muting)
        );

        out_buf [             0] = out_0;
        out_buf [WAV_CHUNK_SIZE] = out_1;
        out_buf ++;
    #else /* !CPU_COLDFIRE */
        /* Read feedback from echo buffer */
        int echo_pos = this->echo_pos;
        uint8_t* const echo_ptr = RAM +
                ((this->r.g.echo_page * 0x100 + echo_pos) & 0xFFFF);
        echo_pos += 4;
        if ( echo_pos >= (this->r.g.echo_delay & 15) * 0x800 )
            echo_pos = 0;
        this->echo_pos = echo_pos;
        int fb_0 = GET_LE16SA( echo_ptr     );
        int fb_1 = GET_LE16SA( echo_ptr + 2 );
        
        /* Keep last 8 samples */
        int (* const fir_ptr) [2] = this->fir_buf + this->fir_pos;
        this->fir_pos = (this->fir_pos + 1) & (fir_buf_half - 1);
        fir_ptr [           0] [0] = fb_0;
        fir_ptr [           0] [1] = fb_1;
        /* duplicate at +8 eliminates wrap checking below */
        fir_ptr [fir_buf_half] [0] = fb_0;
        fir_ptr [fir_buf_half] [1] = fb_1;
        
        /* Apply FIR */
        fb_0 *= this->fir_coeff [0];
        fb_1 *= this->fir_coeff [0];

        #define DO_PT( i )\
            fb_0 += fir_ptr [i] [0] * this->fir_coeff [i];\
            fb_1 += fir_ptr [i] [1] * this->fir_coeff [i];
        
        DO_PT( 1 )
        DO_PT( 2 )
        DO_PT( 3 )
        DO_PT( 4 )
        DO_PT( 5 )
        DO_PT( 6 )
        DO_PT( 7 )
        
        /* Generate output */
        int amp_0 = (chans_0 * global_vol_0 + fb_0 * this->r.g.echo_volume_0)
                    >> global_muting;
        int amp_1 = (chans_1 * global_vol_1 + fb_1 * this->r.g.echo_volume_1)
                    >> global_muting;
        out_buf [             0] = amp_0;
        out_buf [WAV_CHUNK_SIZE] = amp_1;
        out_buf ++;
        
        /* Feedback into echo buffer */
        int e0 = (echo_0 >> 7) + ((fb_0 * this->r.g.echo_feedback) >> 14);
        int e1 = (echo_1 >> 7) + ((fb_1 * this->r.g.echo_feedback) >> 14);
        if ( !(this->r.g.flags & 0x20) )
        {
            CLAMP16( e0, e0 );
            SET_LE16A( echo_ptr    , e0 );
            CLAMP16( e1, e1 );
            SET_LE16A( echo_ptr + 2, e1 );
        }
    #endif /* CPU_COLDFIRE */
    #else
        /* Generate output  */
        int amp_0 = (chans_0 * global_vol_0) >> global_muting;
        int amp_1 = (chans_1 * global_vol_1) >> global_muting;
        out_buf [             0] = amp_0;
        out_buf [WAV_CHUNK_SIZE] = amp_1;
        out_buf ++;
    #endif
    }
    while ( --count );
#if 0
    EXIT_TIMER(dsp);
    ENTER_TIMER(cpu);
#endif
}

static inline void DSP_run( struct Spc_Dsp* this, long count, int32_t* out )
{
    /* Should we just fill the buffer with silence? Flags won't be cleared */
    /* during this run so it seems it should keep resetting every sample. */
    if ( this->r.g.flags & 0x80 )
        DSP_reset( this );
    
    DSP_run_( this, count, out );
}
