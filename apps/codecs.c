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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <atoi.h>
#include <timefuncs.h>
#include <ctype.h>
#include "debug.h"
#include "button.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "sprintf.h"
#include "logf.h"
#include "screens.h"
#include "misc.h"
#include "mas.h"
#include "codecs.h"
#include "lang.h"
#include "keyboard.h"
#include "mpeg.h"
#include "buffer.h"
#include "mp3_playback.h"
#include "playback.h"
#include "backlight.h"
#include "ata.h"
#include "talk.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "system.h"
#include "sound.h"
#include "splash.h"
#include "general.h"

#ifdef SIMULATOR
#if CONFIG_CODEC == SWCODEC
unsigned char codecbuf[CODEC_SIZE];
#endif
void *sim_codec_load_ram(char* codecptr, int size,
        void* ptr2, int bufwrap, void **pd);
void sim_codec_close(void *pd);
#else
#define sim_codec_close(x)
extern unsigned char codecbuf[];
#endif

extern void* plugin_get_audio_buffer(int *buffer_size);

struct codec_api ci_voice;

struct codec_api ci = {

    0, /* filesize */
    0, /* curpos */
    NULL, /* id3 */
    NULL, /* taginfo_ready */
    false, /* stop_codec */
    0, /* new_track */
    0, /* seek_time */
    NULL, /* get_codec_memory */
    NULL, /* pcmbuf_insert */
    NULL, /* set_elapsed */
    NULL, /* read_filebuf */
    NULL, /* request_buffer */
    NULL, /* advance_buffer */
    NULL, /* advance_buffer_loc */
    NULL, /* seek_buffer */
    NULL, /* seek_complete */
    NULL, /* mp3_get_filepos */
    NULL, /* request_next_track */
    NULL, /* discard_codec */
    NULL, /* set_offset */
    NULL, /* configure */
    
    gui_syncsplash,

    /* file */
    (open_func)PREFIX(open),
    close,
    (read_func)read,
    PREFIX(lseek),
    (creat_func)PREFIX(creat),
    (write_func)write,
    PREFIX(remove),
    PREFIX(rename),
    PREFIX(ftruncate),
    PREFIX(fsync),
    fdprintf,
    read_line,
    settings_parseline,
#ifndef SIMULATOR
    ata_sleep,
#endif

    /* dir */
    PREFIX(opendir),
    PREFIX(closedir),
    PREFIX(readdir),
    PREFIX(mkdir),

    /* kernel/ system */
    PREFIX(sleep),
    yield,
    &current_tick,
    default_event_handler,
    default_event_handler_ex,
    create_thread,
    remove_thread,
    reset_poweroff_timer,
#ifndef SIMULATOR
    system_memory_guard,
    &cpu_frequency,
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef CPU_BOOST_LOGGING
    cpu_boost_,
#else
    cpu_boost,
#endif
#endif
#endif

    /* strings and memory */
    snprintf,
    strcpy,
    strncpy,
    strlen,
    strrchr,
    strcmp,
    strcasecmp,
    strncasecmp,
    memset,
    memcpy,
    memmove,
    _ctype_,
    atoi,
    strchr,
    strcat,
    memcmp,
    strcasestr,
    memchr,

    /* sound */
    sound_set,
#ifndef SIMULATOR
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
#endif

    /* playback control */
    PREFIX(audio_play),
    audio_stop,
    audio_pause,
    audio_resume,
    audio_next,
    audio_prev,
    audio_ff_rewind,
    audio_next_track,
    playlist_amount,
    audio_status,
    audio_has_changed_track,
    audio_current_track,
    audio_flush_and_reload_tracks,
    audio_get_file_pos,

    /* misc */
    srand,
    rand,
    (qsort_func)qsort,
    kbd_input,
    get_time,
    set_time,
    plugin_get_audio_buffer,
    round_value_to_list32,

#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    logf,
#endif
    &global_settings,
    mp3info,
    count_mp3_frames,
    create_xing_header,
    find_next_frame,
    battery_level,
    battery_level_safe,

#ifdef RB_PROFILE
    profile_thread,
    profstop,
    profile_func_enter,
    profile_func_exit,
#endif

#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
    false,  /* stop_encoder */
    0,      /* enc_codec_loaded */
    enc_get_inputs,
    enc_set_parameters,
    enc_get_chunk,
    enc_finish_chunk,
    enc_pcm_buf_near_empty,
    enc_get_pcm_data,
    enc_unget_pcm_data,
#endif

    /* new stuff at the end, sort into place next time
       the API gets incompatible */

};

void codec_get_full_path(char *path, const char *codec_root_fn)
{
    snprintf(path, MAX_PATH-1, CODECS_DIR "/%s." CODEC_EXTENSION,
             codec_root_fn);
}

int codec_load_ram(char* codecptr, int size, void* ptr2, int bufwrap,
                   struct codec_api *api)
{
    struct codec_header *hdr;
    int status;
#ifndef SIMULATOR
    int copy_n;
    
    if ((char *)&codecbuf[0] != codecptr) {
        size = MIN(size, CODEC_SIZE);
        copy_n = MIN(size, bufwrap);
        memcpy(codecbuf, codecptr, copy_n);         
        if (size - copy_n > 0) {
            memcpy(&codecbuf[copy_n], ptr2, size - copy_n);
        }
        api->discard_codec();
    }
    hdr = (struct codec_header *)codecbuf;
        
    if (size <= (signed)sizeof(struct codec_header)
        || (hdr->magic != CODEC_MAGIC
#ifdef HAVE_RECORDING
             && hdr->magic != CODEC_ENC_MAGIC
#endif
            )
        || hdr->target_id != TARGET_ID
        || hdr->load_addr != codecbuf
        || hdr->end_addr > codecbuf + CODEC_SIZE)
    {
        logf("codec header error");
        return CODEC_ERROR;
    }
#else /* SIMULATOR */
    void *pd;
    
    hdr = sim_codec_load_ram(codecptr, size, ptr2, bufwrap, &pd);
    api->discard_codec();

    if (pd == NULL)
        return CODEC_ERROR;

    if (hdr == NULL
        || hdr->magic != CODEC_MAGIC
        || hdr->target_id != TARGET_ID) {
        sim_codec_close(pd);
        return CODEC_ERROR;
    }
#endif /* SIMULATOR */
    if (hdr->api_version > CODEC_API_VERSION
        || hdr->api_version < CODEC_MIN_API_VERSION) {
        sim_codec_close(pd);
        return CODEC_ERROR;
    }

    invalidate_icache();
    status = hdr->entry_point(api);

    sim_codec_close(pd);

    return status;
}

int codec_load_file(const char *plugin, struct codec_api *api)
{
    char msgbuf[80];
    char path[MAX_PATH];
    int fd;
    int rc;

    codec_get_full_path(path, plugin);
    
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        snprintf(msgbuf, sizeof(msgbuf)-1, "Couldn't load codec: %s", path);
        logf("Codec load error:%d", fd);
        gui_syncsplash(HZ*2, true, msgbuf);
        return fd;
    }
    
    rc = read(fd, &codecbuf[0], CODEC_SIZE);
    close(fd);
    if (rc <= 0) {
        logf("Codec read error");
        return CODEC_ERROR;
    }

    return codec_load_ram(codecbuf, (size_t)rc, NULL, 0, api);
}
