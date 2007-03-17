/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Adam Gashlin (hcs)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include "inttypes.h"

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define WAV_CHUNK_SIZE (1024*2)

/* Volume for ADX decoder */
#define BASE_VOL 0x2000

/* Number of times to loop looped tracks when repeat is disabled */
#define LOOP_TIMES 2

/* Length of fade-out for looped tracks (milliseconds) */
#define FADE_LENGTH 10000L

static int16_t samples[WAV_CHUNK_SIZE] IBSS_ATTR;

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    int channels;
    int sampleswritten, i;
    uint8_t *buf;
    int32_t ch1_1, ch1_2, ch2_1, ch2_2; /* ADPCM history */
    size_t n, bufsize;
    int endofstream; /* end of stream flag */
    uint32_t avgbytespersec;
    int looping; /* looping flag */
    int loop_count; /* number of loops done so far */
    int fade_count; /*  countdown for fadeout */
    int fade_frames; /* length of fade in frames */
    off_t start_adr, end_adr; /* loop points */
    off_t chanstart, bufoff;

    /* Generic codec initialisation */
    /* we only render 16 bits */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
    /*ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, 1024*256);*/
  
next_track:
    DEBUGF("ADX: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }
    DEBUGF("ADX: after init\n");
    
    /* init history */
    ch1_1=ch1_2=ch2_1=ch2_2=0;

    /* wait for track info to load */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
        
    /* Read the entire file (or as much as possible) */
    DEBUGF("ADX: request initial buffer\n");
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, ci->filesize);
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < 0x38) {
        return CODEC_ERROR;
    }
    bufsize = n;
    bufoff = 0;
    DEBUGF("ADX: read size = %x\n",bufsize);

    /* Get file header for starting offset, channel count */
    
    chanstart = ((buf[2] << 8) | buf[3]) + 4;
    channels = buf[7];
    
    /* useful for seeking and reporting current playback position */
    avgbytespersec = ci->id3->frequency * 18 * channels / 32;
    DEBUGF("avgbytespersec=%ld\n",avgbytespersec);

    /* Get loop data */
    
    looping = 0; start_adr = 0; end_adr = 0;
    if (!memcmp(buf+0x10,"\x01\xF4\x03\x00",4)) {
        /* Soul Calibur 2 style (type 03) */
        DEBUGF("ADX: type 03 found\n");
        /* check if header is too small for loop data */
		if (chanstart-6 < 0x2c) looping=0;
	    else {
		    looping = (buf[0x18]) ||
		              (buf[0x19]) ||
		              (buf[0x1a]) ||
		              (buf[0x1b]);
		    end_adr = (buf[0x28]<<24) |
		              (buf[0x29]<<16) |
		              (buf[0x2a]<<8) |
		              (buf[0x2b]);

		    start_adr = (
		      (buf[0x1c]<<24) |
		      (buf[0x1d]<<16) |
		      (buf[0x1e]<<8) |
		      (buf[0x1f])
		      )/32*channels*18+chanstart;
		}
    } else if (!memcmp(buf+0x10,"\x01\xF4\x04\x00",4)) {
        /* Standard (type 04) */
        DEBUGF("ADX: type 04 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x38) looping=0;
		else {
			looping = (buf[0x24]) ||
			          (buf[0x25]) ||
			          (buf[0x26]) ||
			          (buf[0x27]);
		    end_adr = (buf[0x34]<<24) |
		              (buf[0x35]<<16) |
		              (buf[0x36]<<8) |
		              buf[0x37];
			start_adr = (
			  (buf[0x28]<<24) |
			  (buf[0x29]<<16) |
			  (buf[0x2a]<<8) |
			  (buf[0x2b])
			  )/32*channels*18+chanstart;
		}
    } else {
        DEBUGF("ADX: error, couldn't determine ADX type\n");
        return CODEC_ERROR;
    }

    if (looping) {
        DEBUGF("ADX: looped, start: %lx end: %lx\n",start_adr,end_adr);
    } else {
        DEBUGF("ADX: not looped\n");
    }
    
    /* advance to first frame */
    /*ci->seek_buffer(chanstart);*/
    DEBUGF("ADX: first frame at %lx\n",chanstart);
    bufoff = chanstart;

    /* setup pcm buffer format */
    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    if (channels == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (channels == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("ADX CODEC_ERROR: more than 2 channels\n");
        return CODEC_ERROR;
    }    

    endofstream = 0;
    loop_count = 0;
    fade_count = -1; /* disable fade */
    fade_frames = 1;

    /* The main decoder loop */
        
    while (!endofstream) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }
        
        /* do we need to loop? */
        if (bufoff >= end_adr-18*channels && looping) {
            DEBUGF("ADX: loop!\n");
            /* check for endless looping */
            if (ci->global_settings->repeat_mode==REPEAT_ONE) {
                loop_count=0;
                fade_count = -1; /* disable fade */
            } else {
                /* otherwise start fade after LOOP_TIMES loops */
                loop_count++;
                if (loop_count >= LOOP_TIMES && fade_count < 0) {
                    /* frames to fade over */
                    fade_frames = FADE_LENGTH*ci->id3->frequency/32/1000;
                    /* volume relative to fade_frames */
                    fade_count = fade_frames;
                    DEBUGF("ADX: fade_frames = %d\n",fade_frames);
                }
            }
            bufoff = start_adr;
        }

        /* do we need to seek? */
        if (ci->seek_time) {
            uint32_t newpos;
            
            DEBUGF("ADX: seek to %ldms\n",ci->seek_time);

            endofstream = 0;
            loop_count = 0;
            fade_count = -1; /* disable fade */
            fade_frames = 1;

            newpos = (((uint64_t)avgbytespersec*(ci->seek_time - 1))
                      / (1000LL*18*channels))*(18*channels);
            bufoff = chanstart + newpos;
            while (bufoff > end_adr-18*channels) {
                bufoff-=end_adr-start_adr;
                loop_count++;
            }
            ci->seek_complete();
        }

        if (bufoff>ci->filesize-channels*18) break; /* End of stream */
        
        /* dance with the devil in the pale moonlight */
        if ((bufoff > ci->curpos + (off_t)bufsize - channels*18) ||
            bufoff < ci->curpos) {
            DEBUGF("ADX: requesting another buffer at %lx size %lx\n",
                bufoff,ci->filesize-bufoff);
            ci->seek_buffer(bufoff);
            buf = ci->request_buffer(&n, ci->filesize-bufoff);
            bufsize = n;
            DEBUGF("ADX: read size = %x\n",bufsize);
            if ((off_t)bufsize < channels*18) {
                /* if we can't get a full frame, just request a single
                   frame (should be able to fit it in the guard buffer) */
                DEBUGF("ADX: requesting single frame at %lx\n",bufoff);
                buf = ci->request_buffer(&n, channels*18);
                bufsize=n;
                DEBUGF("ADX: read size = %x\n",bufsize);
            }
            if (!buf) {
                DEBUGF("ADX: couldn't get buffer at %lx size %lx\n",
                    bufoff,ci->filesize-bufoff);
                return CODEC_ERROR;
            }
            buf-=bufoff;
        }

        if (bufsize == 0) break; /* End of stream */
            
        sampleswritten=0;
          
        while (
        /* Is there data in the file buffer? */
        ((size_t)bufoff <= ci->curpos+bufsize-(18*channels)) &&
        /* Is there space in the output buffer? */
        (sampleswritten <= WAV_CHUNK_SIZE-(32*channels)) &&
        /* Should we be looping? */
        ((!looping) || bufoff < end_adr-18*channels)) {
            /* decode 18 bytes to 32 samples (from bero) */
            int32_t scale = ((buf[bufoff] << 8) | (buf[bufoff+1])) * BASE_VOL;
  
            int32_t ch1_0, d;

            for (i = 2; i < 18; i++)
            {
                d = (buf[bufoff+i] >> 4) & 15;
                if (d & 8) d-= 16;
                ch1_0 = (d*scale + 0x7298L*ch1_1 - 0x3350L*ch1_2) >> 14;
	            if (ch1_0 > 32767) ch1_0 = 32767;
                else if (ch1_0 < -32768) ch1_0 = -32768;
	            samples[sampleswritten] = ch1_0;
	            sampleswritten+=channels;
                ch1_2 = ch1_1; ch1_1 = ch1_0;
                d = buf[bufoff+i] & 15;
                if (d & 8) d -= 16;
                ch1_0 = (d*scale + 0x7298L*ch1_1 - 0x3350L*ch1_2) >> 14;
                if (ch1_0 > 32767) ch1_0 = 32767;
                else if (ch1_0 < -32768) ch1_0 = -32768; 
  	            samples[sampleswritten] = ch1_0;
  	            sampleswritten+=channels;
	            ch1_2 = ch1_1; ch1_1 = ch1_0;
            }
            bufoff+=18;
            
            if (channels == 2) {
                int32_t scale = ((buf[bufoff] << 8)|(buf[bufoff+1]))*BASE_VOL;
  
                int32_t ch2_0, d;
                sampleswritten-=63;

                for (i = 2; i < 18; i++)
                {
                    d = (buf[bufoff+i] >> 4) & 15;
                    if (d & 8) d-= 16;
                    ch2_0 = (d*scale + 0x7298L*ch2_1 - 0x3350L*ch2_2) >> 14;
	                if (ch2_0 > 32767) ch2_0 = 32767;
                    else if (ch2_0 < -32768) ch2_0 = -32768;
	                samples[sampleswritten] = ch2_0;
	                sampleswritten+=2;
                    ch2_2 = ch2_1; ch2_1 = ch2_0;
                    d = buf[bufoff+i] & 15;
                    if (d & 8) d -= 16;
                    ch2_0 = (d*scale + 0x7298L*ch2_1 - 0x3350L*ch2_2) >> 14;
                    if (ch2_0 > 32767) ch2_0 = 32767;
                    else if (ch2_0 < -32768) ch2_0 = -32768; 
  	                samples[sampleswritten] = ch2_0;
  	                sampleswritten+=2;
  	                ch2_2 = ch2_1; ch2_1 = ch2_0;
                }
                bufoff+=18;
                sampleswritten--; /* go back to first channel's next sample */
            }
            if (fade_count>0) {
                fade_count--;
                for (i=0;i<(channels==1?32:64);i++) samples[sampleswritten-i-1]=
                  ((int32_t)samples[sampleswritten-i-1])*fade_count/fade_frames;
                if (fade_count==0) {endofstream=1; break;}
            }
        }

        if (channels == 2)
            sampleswritten >>= 1; /* make samples/channel */

        ci->pcmbuf_insert(samples, NULL, sampleswritten);
            
        ci->set_elapsed(
           ((end_adr-start_adr)*loop_count + bufoff-chanstart)*
           1000LL/avgbytespersec);
    }

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}
