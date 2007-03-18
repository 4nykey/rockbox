/*
 * mpegplayer.c - based on :
 *        - mpeg2dec.c
 *        - m2psd.c (http://www.brouhaha.com/~eric/software/m2psd/)
 *
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * 
 * m2psd: MPEG 2 Program Stream Demultiplexer
 * Copyright (C) 2003 Eric Smith <eric@brouhaha.com>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*

NOTES:

mpegplayer is structured as follows:

1) Video thread (running on the COP for PortalPlayer targets).
2) Audio thread (running on the main CPU to maintain consistency with
   the audio FIQ hander on PP).
3) The main thread which takes care of buffering.

Using the main thread for buffering wastes the 8KB main stack which is
in IRAM.  However, 8KB is not enough for the audio thread to run (it
needs somewhere between 8KB and 9KB), so we create a new thread in
order to`give it a larger stack.

We use 4.5KB of the main stack for a libmad buffer (making use of
otherwise unused IRAM).  There is also the possiblity of stealing the
main Rockbox codec thread's 9KB of IRAM stack and using that for
mpegplayer's audio thread - but we should only implement that if we
can put the IRAM to good use.

The button loop (and hence pause/resume, main menu and, in the future,
seeking) is placed in the audio thread.  This keeps it on the main CPU
in PP targets and also allows buffering to continue in the background
whilst the main thread is filling the buffer.

A/V sync is not yet implemented but is planned to be achieved by
syncing the master clock with the audio, and then (as is currently
implemented), syncing video with the master clock.  This can happen in
the audio thread, along with resyncing after pause.

Seeking should probably happen in the main thread, as that's where the
buffering happens.

On PortalPlayer targets, the main CPU is not being fully utilised -
the bottleneck is the video decoding on the COP.  One way to improve
that might be to move the rendering of the frames (i.e. the
lcd_yuv_blit() call) from the COP back to the main CPU.  Ideas and
patches for that are welcome!

Notes about MPEG files:

MPEG System Clock is 27MHz - i.e. 27000000 ticks/second.

FPS is represented in terms of a frame period - this is always an
integer number of 27MHz ticks.

e.g. 29.97fps (30000/1001) NTSC video has an exact frame period of
900900 27MHz ticks.

In libmpeg2, info->sequence->frame_period contains the frame_period.

Working with Rockbox's 100Hz tick, the common frame rates would need
to be as follows:

FPS     | 27Mhz   | 100Hz
--------|----------------
10*     | 2700000 | 10
12*     | 2250000 |  8.3333
15*     | 1800000 |  6.6667
23.9760 | 1126125 |  4.170833333
24      | 1125000 |  4.166667
25      | 1080000 |  4
29.9700 |  900900 |  3.336667
30      |  900000 |  3.333333


*Unofficial framerates

*/


#include "mpeg2dec_config.h"

#include "plugin.h"

#include "mpeg2.h"
#include "mpeg_settings.h"
#include "video_out.h"
#include "../../codecs/libmad/mad.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_MENU       BUTTON_MODE
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define MPEG_MENU       BUTTON_MENU
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MPEG_MENU       (BUTTON_REC | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_A
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP
#define MPEG_VOLDOWN2   BUTTON_VOL_DOWN
#define MPEG_VOLUP2     BUTTON_VOL_UP

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_MENU       (BUTTON_REW | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_SCROLL_DOWN
#define MPEG_VOLUP      BUTTON_SCROLL_UP

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_SCROLL_UP
#define MPEG_VOLUP      BUTTON_SCROLL_DOWN

#else
#error MPEGPLAYER: Unsupported keypad
#endif

struct plugin_api* rb;

static mpeg2dec_t * mpeg2dec;
static int total_offset = 0;

/* Streams */
typedef struct
{
   uint8_t* curr_packet;        /* Current stream packet beginning */
   uint8_t* curr_packet_end;    /* Current stream packet end */

   uint8_t* next_packet;        /* Next stream packet beginning */    
 
   int id;
} Stream;

static Stream audio_str, video_str;

static uint8_t *disk_buf, *disk_buf_end;

/* Events */
static struct event_queue msg_queue IBSS_ATTR;

#define MSG_BUFFER_NEARLY_EMPTY 1
#define MSG_EXIT_REQUESTED      2

/* Threads */
static struct thread_entry* audiothread_id;
static struct thread_entry* videothread_id;

/* Status */
#define STREAM_PLAYING 0
#define STREAM_DONE 1
#define STREAM_PAUSING 2
#define PLEASE_STOP 3
#define PLEASE_PAUSE 4

int audiostatus IBSS_ATTR;
int videostatus IBSS_ATTR;

/* Various buffers */
/* TODO: Can we reduce the PCM buffer size? */
#define PCMBUFFER_SIZE (512*1024)
#define AUDIOBUFFER_SIZE (32*1024)
#define LIBMPEG2BUFFER_SIZE (2*1024*1024)

static int tick_enabled IBSS_ATTR = 0;

#define MPEG_CURRENT_TICK ((unsigned int)((*rb->current_tick - tick_offset)))

/* The value to subtract from current_tick to get the current mpeg tick */
static int tick_offset IBSS_ATTR;

/* The last tick - i.e. the time to reset the tick_offset to when unpausing */
static int last_tick IBSS_ATTR;

static void start_timer(void)
{
    last_tick = 0;
    tick_offset = *rb->current_tick;
}

static void unpause_timer(void)
{
    tick_offset = *rb->current_tick - last_tick;
}

static void pause_timer(void)
{
    /* Save the current MPEG tick */
    last_tick = *rb->current_tick - tick_offset;
}


static void button_loop(void)
{
    bool result;
    int vol, minvol, maxvol;
    int button = rb->button_get(false);

    switch (button)
    {
        case MPEG_VOLUP:
        case MPEG_VOLUP|BUTTON_REPEAT:
#ifdef MPEG_VOLUP2
        case MPEG_VOLUP2:
        case MPEG_VOLUP2|BUTTON_REPEAT:
#endif
            vol = rb->global_settings->volume;
            maxvol = rb->sound_max(SOUND_VOLUME);

            if (vol < maxvol) {
                vol++;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case MPEG_VOLDOWN:
        case MPEG_VOLDOWN|BUTTON_REPEAT:
#ifdef MPEG_VOLDOWN2
        case MPEG_VOLDOWN2:
        case MPEG_VOLDOWN2|BUTTON_REPEAT:
#endif
            vol = rb->global_settings->volume;
            minvol = rb->sound_min(SOUND_VOLUME);
            
            if (vol > minvol) {
                vol--;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;

        case MPEG_MENU:
            videostatus=PLEASE_PAUSE;
            rb->pcm_play_pause(false);
            pause_timer(); 

            /* Wait for video thread to stop */
            while (videostatus == PLEASE_PAUSE) { rb->sleep(HZ/25); }
            result = mpeg_menu();

            /* The menu can change the font, so restore */
            rb->lcd_setfont(FONT_SYSFIXED);

            unpause_timer();

            if (result) {
                audiostatus = PLEASE_STOP;
                videostatus = PLEASE_STOP;
            } else {
                videostatus = STREAM_PLAYING;
                rb->pcm_play_pause(true);
            }
            break;

        case MPEG_STOP:
            audiostatus = PLEASE_STOP;
            videostatus = PLEASE_STOP;
            break;

        case MPEG_PAUSE:
            videostatus=PLEASE_PAUSE;
            rb->pcm_play_pause(false);
            pause_timer(); /* Freeze time */

            button = BUTTON_NONE;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            do {
                button = rb->button_get(true);
                if (button == MPEG_STOP) {
                    audiostatus = PLEASE_STOP;
                    videostatus = PLEASE_STOP;
                    return;
                }
            } while (button != MPEG_PAUSE);

            videostatus = STREAM_PLAYING;
            rb->pcm_play_pause(true);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            unpause_timer(); /* Resume time */
            break;

        default:
            if(rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                audiostatus = PLEASE_STOP;
                videostatus = PLEASE_STOP;
            }
    }
}

/* libmad related functions/definitions */
#define INPUT_CHUNK_SIZE 8192

struct mad_stream stream IBSS_ATTR;
struct mad_frame  frame IBSS_ATTR;
struct mad_synth  synth IBSS_ATTR;

unsigned char mad_main_data[MAD_BUFFER_MDLEN];  /* 2567 bytes */

static void init_mad(void* mad_frame_overlap)
{
    rb->memset(&stream, 0, sizeof(struct mad_stream));
    rb->memset(&frame, 0, sizeof(struct mad_frame));
    rb->memset(&synth, 0, sizeof(struct mad_synth));

    mad_stream_init(&stream);
    mad_frame_init(&frame);

    /* We do this so libmad doesn't try to call codec_calloc() */
    frame.overlap = mad_frame_overlap;

    rb->memset(mad_main_data, 0, sizeof(mad_main_data));
    stream.main_data = &mad_main_data;
}

/* MPEG related headers */
uint8_t packet_start_code_prefix [3] = { 0x00, 0x00, 0x01 };
uint8_t end_code [4] =                 { 0x00, 0x00, 0x01, 0xb9 };
uint8_t pack_start_code [4] =          { 0x00, 0x00, 0x01, 0xba };
uint8_t system_header_start_code [4] = { 0x00, 0x00, 0x01, 0xbb };

/* This function demux the streams and give the next stream data pointer */
static void get_next_data( Stream* str )
{
    uint8_t *p;
    uint8_t *header;
    int stream;
    
    static int mpeg1_skip_table[16] = {
        0, 0, 4, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    if( str->curr_packet_end == NULL )
    {
        while( (p = disk_buf) == NULL )
        {
            rb->sleep(100);
        }
    }else{
        p = str->curr_packet_end;
    }
    
    for( ;; )
    {
        int length, bytes;
        
        /* Pack header, skip it */
        if( rb->memcmp (p, pack_start_code, sizeof (pack_start_code)) == 0 )
        {
            if ((p[4] & 0xc0) == 0x40) { /* mpeg-2 */
                p += 14 + (p[13] & 7);
            } else if ((p[4] & 0xf0) == 0x20) { /* mpeg-1 */
                p += 12;
            } else {
                rb->splash( 30, "Weird Pack header!" );
                p += 5;
            }
            /*rb->splash( 30, "Pack header" );*/
        }

        /* System header, parse and skip it */
        if( rb->memcmp (p, system_header_start_code, sizeof (system_header_start_code)) == 0 )
        {
            int header_length;

            p += 4;  /*skip start code*/
            header_length = (*(p++)) << 8;
            header_length += *(p++);

            p += header_length;
            /*rb->splash( 30, "System header" );*/
        }
        
        /* Packet header, parse it */
        if( rb->memcmp (p, packet_start_code_prefix, sizeof (packet_start_code_prefix)) != 0 )
        {
            /* Problem */
            //rb->splash( HZ*3, "missing packet start code prefix : %X%X at %X", *p, *(p+2), p-disk_buf );
            str->curr_packet_end = str->curr_packet = NULL;
            return;
            //++p;
            break;
        }        
        
        /* We retrieve basic infos */
        stream = *(p+3);
        length = (*(p+4)) << 8;
        length += *(p+5);
        
        /*rb->splash( 100, "Stream : %X", stream );*/
        if (stream != str->id)
        {
            /* End of stream ? */
            if( stream == 0xB9 )
            {
                str->curr_packet_end = str->curr_packet = NULL;
                return;
            }
            
            /* It's not the packet we're looking for, skip it */            
            p += length+6;
            continue;
        }
        
        /* Ok, it's our packet */
        str->curr_packet_end = p + length+6;    
        header = p;
        if ((header[6] & 0xc0) == 0x80) {    /* mpeg2 */
        
            length = 9 + header[8];
            /* header points to the mpeg2 pes header */
            if (header[7] & 0x80) {
                uint32_t pts, dts;

                pts = (((header[9] >> 1) << 30) |
                       (header[10] << 22) | ((header[11] >> 1) << 15) |
                       (header[12] << 7) | (header[13] >> 1));
                dts = (!(header[7] & 0x40) ? pts :
                       ((uint32_t)(((header[14] >> 1) << 30) |
                    (header[15] << 22) |
                    ((header[16] >> 1) << 15) |
                    (header[17] << 7) | (header[18] >> 1))));
                    
                if( stream >= 0xe0 )
                    mpeg2_tag_picture (mpeg2dec, pts, dts);
            }
        } else {    /* mpeg1 */
            int len_skip;
            uint8_t * ptsbuf;

            length = 7;
            while (header[length - 1] == 0xff) 
            {
                length++;
                if (length > 23) 
                {
                    rb->splash( 30, "Too much stuffing" );
                    break;
                }
            }
            if ((header[length - 1] & 0xc0) == 0x40) 
            {
                length += 2;
            }
            len_skip = length;
            length += mpeg1_skip_table[header[length - 1] >> 4];

            /* header points to the mpeg1 pes header */
            ptsbuf = header + len_skip;
            if ((ptsbuf[-1] & 0xe0) == 0x20)
            {
                uint32_t pts, dts;

                pts = (((ptsbuf[-1] >> 1) << 30) |
                       (ptsbuf[0] << 22) | ((ptsbuf[1] >> 1) << 15) |
                       (ptsbuf[2] << 7) | (ptsbuf[3] >> 1));
                dts = (((ptsbuf[-1] & 0xf0) != 0x30) ? pts :
                       ((uint32_t)(((ptsbuf[4] >> 1) << 30) |
                    (ptsbuf[5] << 22) | ((ptsbuf[6] >> 1) << 15) |
                    (ptsbuf[7] << 7) | (ptsbuf[18] >> 1))));
                if( stream >= 0xe0 )    
                    mpeg2_tag_picture (mpeg2dec, pts, dts);
            }
        }
        p += length;
        bytes = 6 + (header[4] << 8) + header[5] - length;
        if (bytes > 0) {
            /*str->curr_packet_end = p+bytes;*/
            str->curr_packet = p;
            return;
        }
        
        if( str->curr_packet_end > disk_buf_end )
        {
            /* We should ask for buffering here */
            str->curr_packet_end = str->curr_packet = NULL;
            return;
        }
                
        break;
    }    
}

uint8_t* mpa_buffer;
size_t mpa_buffer_size;

static volatile int madpcm_playing;
static volatile int16_t* pcm_buffer;
static volatile size_t pcm_buffer_size;

static volatile size_t pcmbuf_len;
static volatile int16_t* pcmbuf_end;
static volatile int16_t* pcmbuf_head;
static volatile int16_t* pcmbuf_tail;

static void init_pcmbuf(void)
{
    pcmbuf_head = pcm_buffer;
    pcmbuf_len = 0;
    pcmbuf_tail = pcm_buffer;
    pcmbuf_end = pcm_buffer + pcm_buffer_size / sizeof(int16_t);
    madpcm_playing = 0;
}

static void get_more(unsigned char** start, size_t* size)
{
   if (pcmbuf_len < 32*1024) {
      *start = NULL;
      *size = 0;
      madpcm_playing = 0;
   } else {
      *start = (unsigned char*)(pcmbuf_tail);
      *size = 32*1024;
      pcmbuf_tail += (32*1024)/sizeof(int16_t);
      pcmbuf_len -= 32*1024;
      if (pcmbuf_tail >= pcmbuf_end) { pcmbuf_tail = pcm_buffer; }
   }
}

int line;

static void mad_decode(void)
{
    int32_t* left;
    int32_t* right;
    int i;
    size_t n = 0;
    size_t len;
    int file_end = 0;  /* A count of the errors in each frame */
    int framelength;

    /* We need this here to init the EMAC for Coldfire targets */
    mad_synth_init(&synth);

    init_pcmbuf();

    /* This is the decoding loop. */
    for (;;) {
        button_loop();

        if (audiostatus == PLEASE_STOP) {
           goto done;
        }

        if (n < 1000) {  /* TODO: What is the maximum size of an MPEG audio frame? */
            get_next_data( &audio_str );
            if (audio_str.curr_packet == NULL) {
                goto done;
            }
            len = audio_str.curr_packet_end - audio_str.curr_packet;
            if (n + len > mpa_buffer_size) { 
                rb->splash( 30, "Audio buffer overflow" );
                audiostatus=STREAM_DONE;
                /* Wait to be killed */
                for (;;) { rb->sleep(HZ); }
            }
            rb->memcpy(mpa_buffer+n,audio_str.curr_packet,len);
            n += len;
        }
            
        /* Lock buffers */
        if (stream.error == 0) {
            mad_stream_buffer(&stream, mpa_buffer, n);
        }

        if (mad_frame_decode(&frame, &stream)) {
            if (stream.error == MAD_FLAG_INCOMPLETE
                || stream.error == MAD_ERROR_BUFLEN) {
                /* This makes the codec support partially corrupted files */
                if (file_end == 30)
                    break;

#if 0
                /* The mpa.c version: */
                if (stream.next_frame)
                    inputbuffer = stream.next_frame;
                else
                    inputbuffer++;
#endif

                stream.error = 0;
                file_end++;
                continue;
            } else if (MAD_RECOVERABLE(stream.error)) {
                continue;
            } else {
                /* Some other unrecoverable error */
                break;
            }
            break;
        }

        file_end = 0;

        mad_synth_frame(&synth, &frame);

        /* TODO: Don't memmove so much... */
        if (stream.next_frame) {
            len = stream.next_frame - mpa_buffer;
            rb->memmove(mpa_buffer,stream.next_frame,n-len);
            n -= len;
        } else {
            /* What to do here? */
            goto done;
        }
#if 0
        /* The mpa.c version: */
        if (stream.next_frame)
            inputbuffer = stream.next_frame;
        else
            inputbuffer = inputbuffer_end;
#endif

        framelength = synth.pcm.length; 

        if (framelength > 0) {
            /* Leave at least 32KB free (this will be the currently playing chunk) */
            while (pcmbuf_len + framelength*4 + 32*1024 > pcm_buffer_size) { rb->yield(); }

            if (MAD_NCHANNELS(&frame.header) == 2) {
                left = &synth.pcm.samples[0][0];
                right = &synth.pcm.samples[1][0];
                for (i = 0 ; i < framelength; i++) {
                    *(pcmbuf_head++) = *(left++) >> 13;
                    *(pcmbuf_head++) = *(right++) >> 13;
                    if (pcmbuf_head >= pcmbuf_end) { pcmbuf_head = pcm_buffer; }
                }
            } else { /* mono */
                int16_t sample;
                left = &synth.pcm.samples[0][0];
                for (i = 0 ; i < framelength; i++) {
                    sample = *(left++) >> 13;
                    *(pcmbuf_head++) = sample;
                    *(pcmbuf_head++) = sample;
                    if (pcmbuf_head >= pcmbuf_end) { pcmbuf_head = pcm_buffer; }
                }
            }

            /* TODO: Disable interrupts for Coldfire? */

            /* pcmbuf_len is also modified by the FIQ handler (in
            get_more), so we disable the FIQ TODO: Add sempahore so we
            don't change whilst get_more is running. */

#ifdef CPU_ARM
            disable_fiq();
#endif
            pcmbuf_len += framelength*4;
#ifdef CPU_ARM
            enable_fiq();
#endif
            if ((!madpcm_playing) && (pcmbuf_len > 64*1024)) {
                madpcm_playing = 1;
                rb->pcm_play_data(get_more,NULL,0);
            }
        }
        rb->yield();
    }

done:
    rb->pcm_play_stop();
    audiostatus=STREAM_DONE;

    for (;;) {  rb->sleep(HZ); }
}

/* End of libmad stuff */

static uint64_t eta IBSS_ATTR;

/* TODO: Running in the main thread, libmad needs 8.25KB of stack.
   The codec thread uses a 9KB stack.  So we can probable reduce this a
   little, but leave at 9KB for now to be safe. */
#define AUDIO_STACKSIZE (9*1024)
uint32_t audio_stack[AUDIO_STACKSIZE / sizeof(uint32_t)] IBSS_ATTR;

/* TODO: Check if 4KB is appropriate - it works for my test streams,
   so maybe we can reduce it. */
#define VIDEO_STACKSIZE (4*1024)
static uint32_t video_stack[VIDEO_STACKSIZE / sizeof(uint32_t)] IBSS_ATTR;

static void decode_mpeg2 (void)
{
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    char str[80];
    static int skipped = 0;
    int skipcount = 0;
    static int frame = 0;
    static int starttick = 0;
    static int lasttick;
    unsigned int eta2;
    unsigned int x;
    int fps;

    if (starttick == 0) {
        starttick=*rb->current_tick-1; /* Avoid divby0 */
        lasttick=starttick;
    }

    /* Request the first packet data */
    get_next_data( &video_str );
    mpeg2_buffer (mpeg2dec, video_str.curr_packet, video_str.curr_packet_end);
    total_offset += video_str.curr_packet_end - video_str.curr_packet;

    info = mpeg2_info (mpeg2dec);
    while (1) {
        if (videostatus == PLEASE_STOP) {
            goto done;
        } else if (videostatus == PLEASE_PAUSE) {
            videostatus = STREAM_PAUSING;
            while (videostatus == STREAM_PAUSING) { rb->sleep(HZ/10); }
        }
        state = mpeg2_parse (mpeg2dec);
        rb->yield();

        switch (state) {
        case STATE_BUFFER:
            /* Request next packet data */
            get_next_data( &video_str );
            mpeg2_buffer (mpeg2dec, video_str.curr_packet, video_str.curr_packet_end);
            total_offset += video_str.curr_packet_end - video_str.curr_packet;
            info = mpeg2_info (mpeg2dec);
            if (video_str.curr_packet == NULL) {
                /* No more data. */
                goto done;
            }
            continue;

        case STATE_SEQUENCE:
            vo_setup(info->sequence->width,
                     info->sequence->height,
                     info->sequence->chroma_width,
                     info->sequence->chroma_height);
            mpeg2_skip (mpeg2dec, false);

            break;
        case STATE_PICTURE:
            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            /* draw current picture */
            if (info->display_fbuf) {
                /* We start the timer when we draw the first frame */
                if (!tick_enabled) {
                    start_timer();
                    tick_enabled = 1 ;
                }

                eta += (info->sequence->frame_period);
                eta2 = eta / (27000000 / HZ);

                if (settings.limitfps) {
                    if (eta2 > MPEG_CURRENT_TICK) {
                        rb->sleep(eta2-MPEG_CURRENT_TICK);
                    }              
                }
                x = MPEG_CURRENT_TICK;

                /* If we are more than 1/20 second behind schedule (and 
                   more than 1/20 second into the decoding), skip frame */
                if (settings.skipframes && (x > HZ/20) && 
                   (eta2 < (x - (HZ/20))) && (skipcount < 10)) {
                    skipped++;
                    skipcount++;
                } else {
                    vo_draw_frame(info->display_fbuf->buf);
                    skipcount = 0;
                }

                /* Calculate fps */
                frame++;
                if (settings.showfps && (*rb->current_tick-lasttick>=HZ)) {
                    fps=(frame*(HZ*10))/x;
                    rb->snprintf(str,sizeof(str),"%d.%d %d %d %d",
                                 (fps/10),fps%10,skipped,x,eta2);
                    rb->lcd_putsxy(0,0,str);
                    rb->lcd_update_rect(0,0,LCD_WIDTH,8);
        
                    lasttick = *rb->current_tick;
                }
            }
            break;
        default:
            break;
        }

        rb->yield();
    }

done:
    videostatus = STREAM_DONE;
    for (;;) { rb->sleep(HZ); }
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    void* audiobuf;
    int audiosize;
    int in_file;
    uint8_t* buffer;
    size_t buffer_size;

    /* We define this here so it is on the main stack (in IRAM) */
    mad_fixed_t mad_frame_overlap[2][32][18];       /* 4608 bytes */

    /* This also stops audio playback - so we do it before using IRAM */
    audiobuf = api->plugin_get_audio_buffer(&audiosize);

    PLUGIN_IRAM_INIT(api)
    rb = api;

    /* Set disk pointers to NULL */
    disk_buf_end = disk_buf = NULL;
    
    /* Stream construction */
    /* We take the first stream of each (audio and video) */
    /* TODO : Search for these in the file first */
    audio_str.curr_packet_end = audio_str.curr_packet = audio_str.next_packet = NULL;
    video_str = audio_str;
    video_str.id = 0xe0;
    audio_str.id = 0xc0;

    /* Initialise our malloc buffer */
    mpeg2_alloc_init(audiobuf,audiosize);

    /* Grab most of the buffer for the compressed video - leave some for 
       PCM audio data and some for libmpeg2 malloc use. */
    buffer_size = audiosize - (PCMBUFFER_SIZE+AUDIOBUFFER_SIZE+LIBMPEG2BUFFER_SIZE);
    DEBUGF("audiosize=%d, buffer_size=%ld\n",audiosize,buffer_size);
    buffer = mpeg2_malloc(buffer_size,-1);

    if (buffer == NULL)
        return PLUGIN_ERROR;

    mpa_buffer_size = AUDIOBUFFER_SIZE;
    mpa_buffer = mpeg2_malloc(mpa_buffer_size,-2);

    if (mpa_buffer == NULL)
        return PLUGIN_ERROR;

    pcm_buffer_size = PCMBUFFER_SIZE;
    pcm_buffer = mpeg2_malloc(pcm_buffer_size,-2);

    if (pcm_buffer == NULL)
        return PLUGIN_ERROR;

    /* The remaining buffer is for use by libmpeg2 */

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

    if (parameter == NULL) {
        return PLUGIN_ERROR;
    }

    /* Open the video file */
    in_file = rb->open((char*)parameter,O_RDONLY);

    if (in_file < 0) {
        //fprintf(stderr,"Could not open %s\n",argv[1]);
        return PLUGIN_ERROR;
    }

    init_settings();

    mpeg2dec = mpeg2_init ();

    rb->queue_init( &msg_queue, false );    /* Msg queue init */

    if (mpeg2dec == NULL)
        return PLUGIN_ERROR;

    /* make sure the backlight is always on when viewing video
       (actually it should also set the timeout when plugged in,
       but the function backlight_set_timeout_plugged is not
       available in plugins) */
#if CONFIG_BACKLIGHT
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    /* Initialise libmad */
    rb->memset(mad_frame_overlap, 0, sizeof(mad_frame_overlap));
    init_mad(mad_frame_overlap);

    eta = 0;

    rb->splash(0, "Buffering...");

    disk_buf_end = buffer + rb->read (in_file, buffer, buffer_size);
    disk_buf = buffer;

    rb->lcd_setfont(FONT_SYSFIXED);

    audiostatus = STREAM_PLAYING;
    videostatus = STREAM_PLAYING;

    /* We put the video thread on the second processor for multi-core targets. */
    if ((videothread_id = rb->create_thread(decode_mpeg2,
        (uint8_t*)video_stack,VIDEO_STACKSIZE,"mpgvideo" IF_PRIO(,PRIORITY_PLAYBACK)
	IF_COP(, COP, true))) == NULL)
    {
        rb->splash(HZ, "Cannot create video thread!");
        return PLUGIN_ERROR;
    }

    if ((audiothread_id = rb->create_thread(mad_decode,
        (uint8_t*)audio_stack,AUDIO_STACKSIZE,"mpgaudio" IF_PRIO(,PRIORITY_PLAYBACK)
	IF_COP(, CPU, false))) == NULL)
    {
        rb->splash(HZ, "Cannot create audio thread!");
        rb->remove_thread(videothread_id);
        return PLUGIN_ERROR;
    }

    /* Wait until both threads have finished their work */
    while ((audiostatus != STREAM_DONE) || (videostatus != STREAM_DONE)) { 
        rb->sleep(HZ/10);
    }

    rb->remove_thread(audiothread_id);
    rb->remove_thread(videothread_id);
    rb->yield(); /* Is this needed? */

    rb->lcd_clear_display();
    rb->lcd_update();

    mpeg2_close (mpeg2dec);

    rb->queue_delete( &msg_queue );

    rb->close (in_file);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    save_settings();  /* Save settings (if they have changed) */

    rb->lcd_setfont(FONT_UI);

#if CONFIG_BACKLIGHT
    /* reset backlight settings */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#endif

    return PLUGIN_OK;
}
