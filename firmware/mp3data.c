/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Parts of this code has been stolen from the Ample project and was written
 * by David H�rdeman. It has since been extended and enhanced pretty much by
 * all sorts of friendly Rockbox people.
 *
 * A nice reference for MPEG header info:
 * http://rockbox.haxx.se/docs/mpeghdr.html
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "debug.h"
#include "mp3data.h"
#include "file.h"

#define DEBUG_VERBOSE

#define BYTES2INT(b1,b2,b3,b4) (((b1 & 0xFF) << (3*8)) | \
                                ((b2 & 0xFF) << (2*8)) | \
                                ((b3 & 0xFF) << (1*8)) | \
                                ((b4 & 0xFF) << (0*8)))

#define SYNC_MASK (0x7ff << 21)
#define VERSION_MASK (3 << 19)
#define LAYER_MASK (3 << 17)
#define PROTECTION_MASK (1 << 16)
#define BITRATE_MASK (0xf << 12)
#define SAMPLERATE_MASK (3 << 10)
#define PADDING_MASK (1 << 9)
#define PRIVATE_MASK (1 << 8)
#define CHANNELMODE_MASK (3 << 6)
#define MODE_EXT_MASK (3 << 4)
#define COPYRIGHT_MASK (1 << 3)
#define ORIGINAL_MASK (1 << 2)
#define EMPHASIS_MASK 3

/* Table of bitrates for MP3 files, all values in kilo.
 * Indexed by version, layer and value of bit 15-12 in header.
 */
const int bitrate_table[2][3][16] =
{
    {
        {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
        {0,32,48,56, 64,80, 96, 112,128,160,192,224,256,320,384,0},
        {0,32,40,48, 56,64, 80, 96, 112,128,160,192,224,256,320,0}
    },
    {
        {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0}
    }
};

/* Table of samples per frame for MP3 files.
 * Indexed by layer. Multiplied with 1000.
 */
const int bs[3] = {384000, 1152000, 1152000};

/* Table of sample frequency for MP3 files.
 * Indexed by version and layer.
 */

const int freqtab[][4] =
{
    {11025, 12000, 8000, 0},  /* MPEG version 2.5 */
    {44100, 48000, 32000, 0}, /* MPEG Version 1 */
    {22050, 24000, 16000, 0}, /* MPEG version 2 */
};

/* check if 'head' is a valid mp3 frame header */
static bool is_mp3frameheader(unsigned long head)
{
    if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
        return false;
    if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
        return false;
    if (!(head & LAYER_MASK)) /* no layer? */
        return false;
    if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
        return false;
    if (!(head & BITRATE_MASK)) /* no bitrate? */
        return false;
    if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
        return false;
    if (((head >> 19) & 1) == 1 &&
        ((head >> 17) & 3) == 3 &&
        ((head >> 16) & 1) == 1)
        return false;
    if ((head & 0xffff0000) == 0xfffe0000)
        return false;
    
    return true;
}

static bool mp3headerinfo(struct mp3info *info, unsigned long header)
{
    int bittable = 0;
    int bitindex;
    int freqindex;
    
    /* MPEG Audio Version */
    switch(header & VERSION_MASK) {
    case 0:
        /* MPEG version 2.5 is not an official standard */
        info->version = MPEG_VERSION2_5;
        bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
        break;
        
    case (1 << 19):
        return false;
        
    case (2 << 19):
        /* MPEG version 2 (ISO/IEC 13818-3) */
        info->version = MPEG_VERSION2;
        bittable = MPEG_VERSION2 - 1;
        break;
        
    case (3 << 19):
        /* MPEG version 1 (ISO/IEC 11172-3) */
        info->version = MPEG_VERSION1;
        bittable = MPEG_VERSION1 - 1;
        break;
    }

    switch(header & LAYER_MASK) {
    case 0:
        return false;
    case (1 << 17):
        info->layer = 2;
        break;
    case (2 << 17):
        info->layer = 1;
        break;
    case (3 << 17):
        info->layer = 0;
        break;
    }

    info->protection = (header & PROTECTION_MASK)?true:false;
    
    /* Bitrate */
    bitindex = (header & 0xf000) >> 12;
    info->bitrate = bitrate_table[bittable][info->layer][bitindex];
    if(info->bitrate == 0)
        return false;
    
    /* Sampling frequency */
    freqindex = (header & 0x0C00) >> 10;
    info->frequency = freqtab[info->version][freqindex];
    if(info->frequency == 0)
        return false;

    info->padding = (header & 0x0200)?1:0;
    
    /* Calculate number of bytes, calculation depends on layer */
    switch(info->layer) {
    case 0:
        info->frame_size = info->bitrate * 48000;
        info->frame_size /=
            freqtab[info->version][freqindex] << bittable;
        break;
    case 1:
    case 2:
        info->frame_size = info->bitrate * 144000;
        info->frame_size /=
            freqtab[info->version][freqindex] << bittable;
        break;
    default:
        info->frame_size = 1;
    }
    
    info->frame_size += info->padding;

    /* Calculate time per frame */
    info->frame_time = bs[info->layer] /
        (freqtab[info->version][freqindex] << bittable);

    info->channel_mode = (header & 0xc0) >> 6;
    info->mode_extension = (header & 0x30) >> 4;
    info->emphasis = header & 3;

#ifdef DEBUG_VERBOSE
    DEBUGF( "Header: %08x, Ver %d, lay %d, bitr %d, freq %d, "
            "chmode %d, mode_ext %d, emph %d, bytes: %d time: %d\n",
            header, info->version, info->layer, info->bitrate, info->frequency,
            info->channel_mode, info->mode_extension,
            info->emphasis, info->frame_size, info->frame_time);
#endif
    return true;
}

unsigned long find_next_frame(int fd, int *offset, int max_offset, unsigned long last_header)
{
    unsigned long header=0;
    unsigned char tmp;
    int i;

    int pos = 0;

    /* We remember the last header we found, to use as a template to see if
       the header we find has the same frequency, layer etc */
    last_header &= 0xffff0c00;
    
    /* Fill up header with first 24 bits */
    for(i = 0; i < 3; i++) {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return 0;
        header |= tmp;
        pos++;
    }

    do {
        header <<= 8;
        if(!read(fd, &tmp, 1))
            return 0;
        header |= tmp;
        pos++;
        if(max_offset > 0 && pos > max_offset)
            return 0;
    } while(!is_mp3frameheader(header) ||
            (last_header?((header & 0xffff0c00) != last_header):false));

    *offset = pos - 4;

#ifdef DEBUG
    if(*offset)
        DEBUGF("Warning: skipping %d bytes of garbage\n", *offset);
#endif
    
    return header;
}

#ifdef SIMULATOR
unsigned char mp3buf[0x100000];
unsigned char mp3end[1];
#else
extern unsigned char mp3buf[];
extern unsigned char mp3end[];
#endif
static int fnf_read_index;
static int fnf_buf_len;

static int buf_getbyte(int fd, unsigned char *c)
{
    if(fnf_read_index < fnf_buf_len)
    {
        *c = mp3buf[fnf_read_index++];
        return 1;
    }
    else
    {
        fnf_buf_len = read(fd, mp3buf, mp3end - mp3buf);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = 0;

        if(fnf_buf_len > 0)
        {
            *c = mp3buf[fnf_read_index++];
            return 1;
        }
        else
            return 0;
    }
    return 0;
}

static int buf_seek(int fd, int len)
{
    fnf_read_index += len;
    if(fnf_read_index > fnf_buf_len)
    {
        len = fnf_read_index - fnf_buf_len;
        
        fnf_buf_len = read(fd, mp3buf, mp3end - mp3buf);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = 0;
        fnf_read_index += len;
    }
    
    if(fnf_read_index > fnf_buf_len)
    {
        return -1;
    }
    else
        return 0;
}

static void buf_init(void)
{
    fnf_buf_len = 0;
    fnf_read_index = 0;
}

unsigned long buf_find_next_frame(int fd, int *offset, int max_offset,
                                  unsigned long last_header)
{
    unsigned long header=0;
    unsigned char tmp;
    int i;

    int pos = 0;

    /* We remember the last header we found, to use as a template to see if
       the header we find has the same frequency, layer etc */
    last_header &= 0xffff0c00;

    /* Fill up header with first 24 bits */
    for(i = 0; i < 3; i++) {
        header <<= 8;
        if(!buf_getbyte(fd, &tmp))
            return 0;
        header |= tmp;
        pos++;
    }

    do {
        header <<= 8;
        if(!buf_getbyte(fd, &tmp))
            return 0;
        header |= tmp;
        pos++;
        if(max_offset > 0 && pos > max_offset)
            return 0;
    } while(!is_mp3frameheader(header) ||
            (last_header?((header & 0xffff0c00) != last_header):false));

    *offset = pos - 4;

#ifdef DEBUG
    if(*offset)
        DEBUGF("Warning: skipping %d bytes of garbage\n", *offset);
#endif
    
    return header;
}

int get_mp3file_info(int fd, struct mp3info *info)
{
    unsigned char frame[1050];
    unsigned char *vbrheader;
    unsigned long header;
    int bytecount;
    int num_offsets;
    int frames_per_entry;
    int i;
    int offset;
    int j;
    int tmp;

    header = find_next_frame(fd, &bytecount, 0x20000, 0);
    /* Quit if we haven't found a valid header within 128K */
    if(header == 0)
        return -1;

    memset(info, 0, sizeof(struct mp3info));
    if(!mp3headerinfo(info, header))
        return -2;

    /* OK, we have found a frame. Let's see if it has a Xing header */
    if(read(fd, frame, info->frame_size-4) < 0)
        return -3;

    /* calculate position of VBR header */
    if ( info->version == MPEG_VERSION1 ) {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 17;
        else
            vbrheader = frame + 32;
    }
    else {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 9;
        else
            vbrheader = frame + 17;
    }

    if (vbrheader[0] == 'X' &&
        vbrheader[1] == 'i' &&
        vbrheader[2] == 'n' &&
        vbrheader[3] == 'g')
    {
        int i = 8; /* Where to start parsing info */

        DEBUGF("Xing header\n");

        /* Remember where in the file the Xing header is */
        info->vbr_header_pos = lseek(fd, 0, SEEK_CUR) - info->frame_size;
        
        /* We want to skip the Xing frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to find out the real info about
           the mp3 stream */
        header = find_next_frame(fd, &tmp, 0x20000, 0);
        if(header == 0)
            return -4;

        if(!mp3headerinfo(info, header))
            return -5;

        /* Yes, it is a VBR file */
        info->is_vbr = true;
        info->is_xing_vbr = true;
        
        if(vbrheader[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
            info->frame_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                          vbrheader[i+2], vbrheader[i+3]);
            info->file_time = info->frame_count * info->frame_time;
            i += 4;
        }

        if(vbrheader[7] & VBR_BYTES_FLAG) /* Is byte count there? */
        {
            info->byte_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                         vbrheader[i+2], vbrheader[i+3]);
            i += 4;
        }

        if(info->file_time && info->byte_count)
            info->bitrate = info->byte_count * 8 / info->file_time;
        else
            info->bitrate = 0;
        
        if(vbrheader[7] & VBR_TOC_FLAG) /* Is table-of-contents there? */
        {
            memcpy( info->toc, vbrheader+i, 100 );
        }
    }

    if (vbrheader[0] == 'V' &&
        vbrheader[1] == 'B' &&
        vbrheader[2] == 'R' &&
        vbrheader[3] == 'I')
    {
        DEBUGF("VBRI header\n");

        /* We want to skip the VBRI frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to find out the real info about
           the mp3 stream */
        header = find_next_frame(fd, &bytecount, 0x20000, 0);
        if(header == 0)
            return -6;
        
        if(!mp3headerinfo(info, header))
            return -7;

        DEBUGF("%04x: %04x %04x ", 0, header >> 16, header & 0xffff);
        for(i = 4;i < (int)sizeof(frame)-4;i+=2) {
            if(i % 16 == 0) {
                DEBUGF("\n%04x: ", i-4);
            }
            DEBUGF("%04x ", (frame[i-4] << 8) | frame[i-4+1]);
        }

        DEBUGF("\n");
        
        /* Yes, it is a FhG VBR file */
        info->is_vbr = true;
        info->is_vbri_vbr = true;
        info->has_toc = false; /* We don't parse the TOC (yet) */

        info->byte_count = BYTES2INT(vbrheader[10], vbrheader[11],
                                     vbrheader[12], vbrheader[13]);
        info->frame_count = BYTES2INT(vbrheader[14], vbrheader[15],
                                      vbrheader[16], vbrheader[17]);
 
        info->file_time = info->frame_count * info->frame_time;
        info->bitrate = info->byte_count * 8 / info->file_time;

        /* We don't parse the TOC, since we don't yet know how to (FIXME) */
        num_offsets = BYTES2INT(0, 0, vbrheader[18], vbrheader[19]);
        frames_per_entry = BYTES2INT(0, 0, vbrheader[24], vbrheader[25]);
        DEBUGF("Frame size (%dkpbs): %d bytes (0x%x)\n",
               info->bitrate, info->frame_size, info->frame_size);
        DEBUGF("Frame count: %x\n", info->frame_count);
        DEBUGF("Byte count: %x\n", info->byte_count);
        DEBUGF("Offsets: %d\n", num_offsets);
        DEBUGF("Frames/entry: %d\n", frames_per_entry);

        offset = 0;

        for(i = 0;i < num_offsets;i++)
        {
           j = BYTES2INT(0, 0, vbrheader[26+i*2], vbrheader[27+i*2]);
           offset += j;
           DEBUGF("%03d: %x (%x)\n", i, offset - bytecount, j);
        }
    }

    /* Is it a LAME Info frame? */
    if (vbrheader[0] == 'I' &&
        vbrheader[1] == 'n' &&
        vbrheader[2] == 'f' &&
        vbrheader[3] == 'o')
    {
        /* Make sure we skip this frame in playback */
        bytecount += info->frame_size;
    }

    return bytecount;
}

/* This is an MP3 header, 128kbit/s, 44.1kHz, with silence */
static const unsigned char xing_frame_header[] = {
    0xff, 0xfa, 0x90, 0x64, 0x86, 0x1f
};

static const char cooltext[] = "Rockbox rocks";

static void int2bytes(unsigned char *buf, int val)
{
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

int count_mp3_frames(int fd, int startpos, int filesize,
                     void (*progressfunc)(int))
{
    unsigned long header = 0;
    struct mp3info info;
    int num_frames;
    int bytes;
    int cnt;
    int progress_chunk = filesize / 50; /* Max is 50%, in 1% increments */
    int progress_cnt = 0;
    bool is_vbr = false;
    int last_bitrate = 0;

    if(lseek(fd, startpos, SEEK_SET) < 0)
        return -1;

    buf_init();

    /* Find out the total number of frames */
    num_frames = 0;
    cnt = 0;
    
    while((header = buf_find_next_frame(fd, &bytes, -1, header))) {
        mp3headerinfo(&info, header);

        /* See if this really is a VBR file */
        if(last_bitrate && info.bitrate != last_bitrate)
        {
            is_vbr = true;
        }
        last_bitrate = info.bitrate;
        
        buf_seek(fd, info.frame_size-4);
        num_frames++;
        if(progressfunc)
        {
            cnt += bytes + info.frame_size;
            if(cnt > progress_chunk)
            {
                progress_cnt++;
                progressfunc(progress_cnt);
                cnt = 0;
            }
        }
    }
    DEBUGF("Total number of frames: %d\n", num_frames);

    if(is_vbr)
        return num_frames;
    else
    {
        DEBUGF("Not a VBR file\n");
        return 0;
    }
}

int create_xing_header(int fd, int startpos, int filesize,
                       unsigned char *buf, int num_frames,
                       void (*progressfunc)(int), bool generate_toc)
{
    unsigned long header = 0;
    struct mp3info info;
    int pos, last_pos;
    int i, j;
    int bytes;
    int filepos;
    int tocentry;
    int x;
    int index;

    DEBUGF("create_xing_header()\n");
    
    /* Create the frame header */
    memset(buf, 0, 417);
    memcpy(buf, xing_frame_header, 6);
           
    lseek(fd, startpos, SEEK_SET);
    buf_init();
    
    buf[36] = 'X';
    buf[36+1] = 'i';
    buf[36+2] = 'n';
    buf[36+3] = 'g';
    int2bytes(&buf[36+4], (num_frames?VBR_FRAMES_FLAG:0 |
                           filesize?VBR_BYTES_FLAG:0 |
                           generate_toc?VBR_TOC_FLAG:0));
    index = 36+8;
    if(num_frames)
    {
        int2bytes(&buf[index], num_frames);
        index += 4;
    }

    if(filesize)
    {
        int2bytes(&buf[index], filesize - startpos);
        index += 4;
    }

    if(generate_toc)
    {
        /* Generate filepos table */
        last_pos = 0;
        filepos = 0;
        header = 0;
        x = 0;
        for(i = 0;i < 100;i++) {
            /* Calculate the absolute frame number for this seek point */
            pos = i * num_frames / 100;
            
            /* Advance from the last seek point to this one */
            for(j = 0;j < pos - last_pos;j++)
            {
                DEBUGF("fpos: %x frame no: %x ", filepos, x++);
                header = buf_find_next_frame(fd, &bytes, -1, header);
                mp3headerinfo(&info, header);
                buf_seek(fd, info.frame_size-4);
                filepos += info.frame_size;
            }
            
            if(progressfunc)
            {
                progressfunc(50 + i/2);
            }
            
            tocentry = filepos * 256 / filesize;
            
            DEBUGF("Pos %d: %d  relpos: %d  filepos: %x tocentry: %x\n",
                   i, pos, pos-last_pos, filepos, tocentry);
            
            /* Fill in the TOC entry */
            buf[index + i] = tocentry;
            
            last_pos = pos;
        }
    }

    memcpy(buf + index + 100, cooltext, sizeof(cooltext));

#ifdef DEBUG
    for(i = 0;i < 417;i++)
    {
        if(i && !(i % 16))
            DEBUGF("\n");

        DEBUGF("%02x ", buf[i]);
    }
#endif
    
    return 0;
}
