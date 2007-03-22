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
#include "plugin.h"
#include <ctype.h>
#include <string.h>
#include <sprintf.h>
#include <atoi.h>
#include "debug.h"
#include "i2c.h"
#include "lang.h"
#include "keyboard.h"
#include "buffer.h"
#include "backlight.h"
#include "sound_menu.h"
#include "mp3data.h"
#include "powermgmt.h"
#include "splash.h"
#include "logf.h"

#if CONFIG_CHARGING
#include "power.h"
#endif

#ifdef HAVE_LCD_BITMAP
#include "scrollbar.h"
#include "peakmeter.h" 
#include "bmp.h"
#include "bidi.h"
#endif

#ifdef SIMULATOR
static unsigned char pluginbuf[PLUGIN_BUFFER_SIZE];
void *sim_plugin_load(char *plugin, void **pd);
void sim_plugin_close(void *pd);
void sim_lcd_ex_init(int shades, unsigned long (*getpixel)(int, int));
void sim_lcd_ex_update_rect(int x, int y, int width, int height);
#else
#define sim_plugin_close(x)
extern unsigned char pluginbuf[];
#include "bitswap.h"
#endif

/* for actual plugins only, not for codecs */
static bool plugin_loaded = false;
static int  plugin_size = 0;
static bool (*pfn_tsr_exit)(bool reenter) = NULL; /* TSR exit callback */
static char current_plugin[MAX_PATH];

static const struct plugin_api rockbox_api = {

    /* lcd */
    lcd_set_contrast,
    lcd_clear_display,
    lcd_puts,
    lcd_puts_scroll,
    lcd_stop_scroll,
#ifdef HAVE_LCD_CHARCELLS
    lcd_define_pattern,
    lcd_get_locked_pattern,
    lcd_unlock_pattern,
    lcd_putc,
    lcd_put_cursor,
    lcd_remove_cursor,
    PREFIX(lcd_icon),
    lcd_double_height,
#else
    lcd_setmargins,
    lcd_set_drawmode,
    lcd_get_drawmode,
    lcd_setfont,
    lcd_getstringsize,
    lcd_drawpixel,
    lcd_drawline,
    lcd_hline,
    lcd_vline,
    lcd_drawrect,
    lcd_fillrect,
    lcd_mono_bitmap_part,
    lcd_mono_bitmap,
#if LCD_DEPTH > 1
    lcd_set_foreground,
    lcd_get_foreground,
    lcd_set_background,
    lcd_get_background,
    lcd_bitmap_part,
    lcd_bitmap,
    lcd_set_backdrop,
#endif
#if LCD_DEPTH == 16
    lcd_bitmap_transparent_part,
    lcd_bitmap_transparent,
#endif
    bidi_l2v,
    font_get_bits,
    font_load,
    lcd_putsxy,
    lcd_puts_style,
    lcd_puts_scroll_style,
    &lcd_framebuffer[0][0],
    lcd_blit,
    lcd_update,
    lcd_update_rect,
    gui_scrollbar_draw,
    font_get,
    font_getstringsize,
    font_get_width,
#endif
    backlight_on,
    backlight_off,
    backlight_set_timeout,
    gui_syncsplash,
#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    lcd_remote_set_contrast,
    lcd_remote_clear_display,
    lcd_remote_puts,
    lcd_remote_puts_scroll,
    lcd_remote_stop_scroll,
    lcd_remote_set_drawmode,
    lcd_remote_get_drawmode,
    lcd_remote_setfont,
    lcd_remote_getstringsize,
    lcd_remote_drawpixel,
    lcd_remote_drawline,
    lcd_remote_hline,
    lcd_remote_vline,
    lcd_remote_drawrect,
    lcd_remote_fillrect,
    lcd_remote_mono_bitmap_part,
    lcd_remote_mono_bitmap,
    lcd_remote_putsxy,
    lcd_remote_puts_style,
    lcd_remote_puts_scroll_style,
    &lcd_remote_framebuffer[0][0],
    lcd_remote_update,
    lcd_remote_update_rect,

    remote_backlight_on,
    remote_backlight_off,
#endif
#if NB_SCREENS == 2
    {&screens[SCREEN_MAIN], &screens[SCREEN_REMOTE]},
#else
    {&screens[SCREEN_MAIN]},
#endif
#if defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    lcd_remote_set_foreground,
    lcd_remote_get_foreground,
    lcd_remote_set_background,
    lcd_remote_get_background,
    lcd_remote_bitmap_part,
    lcd_remote_bitmap,
#endif

#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR)
    lcd_yuv_blit,
#endif
    /* list */
    gui_synclist_init,
    gui_synclist_set_nb_items,
    gui_synclist_set_icon_callback,
    gui_synclist_get_nb_items,
    gui_synclist_get_sel_pos,
    gui_synclist_draw,
    gui_synclist_select_item,
    gui_synclist_add_item,
    gui_synclist_del_item,
    gui_synclist_limit_scroll,
    gui_synclist_flash,
    gui_synclist_do_button,
    gui_synclist_set_title,

    /* button */
    button_get,
    button_get_w_tmo,
    button_status,
    button_clear_queue,
#ifdef HAS_BUTTON_HOLD
    button_hold,
#endif

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
    PREFIX(filesize),
    fdprintf,
    read_line,
    settings_parseline,
#ifndef SIMULATOR
    ata_sleep,
    ata_disk_is_active,
#endif
    ata_spindown,
    reload_directory,

    /* dir */
    PREFIX(opendir),
    PREFIX(closedir),
    PREFIX(readdir),
    PREFIX(mkdir),
    PREFIX(rmdir),
    
    /* dir, cached */
#ifdef HAVE_DIRCACHE
    opendir_cached,
    readdir_cached,
    closedir_cached,
#endif

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
    timer_register,
    timer_unregister,
    timer_set_period,
#endif
    queue_init,
    queue_delete,
    queue_post,
    queue_wait_w_tmo,
    usb_acknowledge,
#ifdef RB_PROFILE
    profile_thread,
    profstop,
    profile_func_enter,
    profile_func_exit,
#endif

#ifdef SIMULATOR
    /* special simulator hooks */
#if defined(HAVE_LCD_BITMAP) && LCD_DEPTH < 8
    sim_lcd_ex_init,
    sim_lcd_ex_update_rect,
#endif
#endif

    /* strings and memory */
    snprintf,
    vsnprintf,
    strcpy,
    strncpy,
    strlen,
    strrchr,
    strcmp,
    strncmp,
    strcasecmp,
    strncasecmp,
    memset,
    memcpy,
    memmove,
    _ctype_,
    atoi,
    strchr,
    strcat,
    memchr,
    memcmp,
    strcasestr,
    strtok_r,
    /* unicode stuff */
    utf8decode,
    iso_decode,
    utf16LEdecode,
    utf16BEdecode,
    utf8encode,
    utf8length,
    utf8seek,

    /* sound */
    sound_set,
    set_sound,

    sound_min,
    sound_max,
#ifndef SIMULATOR
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
#if CONFIG_CODEC != SWCODEC
    bitswap,
#endif
#endif
#if CONFIG_CODEC == SWCODEC
    pcm_play_data,
    pcm_play_stop,
    pcm_set_frequency,
    pcm_is_playing,
    pcm_play_pause,
#endif
#if CONFIG_CODEC == SWCODEC
    pcm_calculate_peaks,
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
#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    mpeg_get_last_header,
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) || \
    (CONFIG_CODEC == SWCODEC)
    sound_set_pitch,
#endif

#if !defined(SIMULATOR) && (CONFIG_CODEC != SWCODEC)
    /* MAS communication */
    mas_readmem,
    mas_writemem,
    mas_readreg,
    mas_writereg,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    mas_codec_writereg,
    mas_codec_readreg,
    i2c_begin,
    i2c_end,
    i2c_write,
#endif
#endif /* !SIMULATOR && CONFIG_CODEC != SWCODEC */

    /* menu */
    do_menu,
    /* OLD API - dont use unless you have to */
    menu_init,
    menu_exit,
    menu_show,
    menu_run,
    menu_count,
    set_option,
    set_int,
    set_bool,

    /* action handling */
    get_custom_action,
    get_action,
    action_signalscreenchange,
    action_userabort,

    /* power */
    battery_level,
    battery_level_safe,
    battery_time,
#ifndef SIMULATOR
    battery_voltage,
#endif
#if CONFIG_CHARGING
    charger_inserted,
# if CONFIG_CHARGING == CHARGING_MONITOR
    charging_state,
# endif
#endif
#ifdef HAVE_USB_POWER
        usb_powered,
#endif

    /* misc */
    srand,
    rand,
    (qsort_func)qsort,
    kbd_input,
    get_time,
    set_time,
    plugin_get_buffer,
    plugin_get_audio_buffer,
    plugin_tsr,
#if defined(DEBUG) || defined(SIMULATOR)
    debugf,
#endif
#ifdef ROCKBOX_HAS_LOGF
    _logf,
#endif
    &global_settings,
    mp3info,
    count_mp3_frames,
    create_xing_header,
    find_next_frame,
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    peak_meter_scale_value,
    peak_meter_set_use_dbfs,
    peak_meter_get_use_dbfs,
#endif
#ifdef HAVE_LCD_BITMAP
    read_bmp_file,
    screen_dump_set_hook,
#endif
    show_logo,
    tree_get_context,

#ifdef HAVE_WHEEL_POSITION
    wheel_status,
    wheel_send_events,
#endif
#if LCD_DEPTH > 1
    lcd_get_backdrop,
#endif
    /* new stuff at the end, sort into place next time
       the API gets incompatible */

/* Keep these at the bottom till fully proven */
#if CONFIG_CODEC == SWCODEC
    &audio_master_sampr_list[0],
    &hw_freq_sampr[0],
#ifndef SIMULATOR
    pcm_apply_settings,
#endif
#ifdef HAVE_RECORDING
    &rec_freq_sampr[0],
#ifndef SIMULATOR
    pcm_init_recording,
    pcm_close_recording,
    pcm_record_data,
    pcm_stop_recording,
    pcm_calculate_rec_peaks,
    audio_set_recording_gain,
    audio_set_output_source,
    rec_set_source,
#endif
#endif /* HAVE_RECORDING */
#endif /* CONFIG_CODEC == SWCODEC */

#ifdef IRAM_STEAL
    plugin_iram_init,
#endif

#if CONFIG_CODEC == SWCODEC && defined(HAVE_RECORDING) && !defined(SIMULATOR)
    sound_default,
    pcm_record_more,
#endif

#ifdef IRIVER_H100_SERIES
    /* Routines for the iriver_flash -plugin. */
    detect_original_firmware,
    detect_flashed_ramimage,
    detect_flashed_romimage,
#endif
    playlist_resume,
    playlist_start,
    &global_status,

    pcm_get_bytes_waiting,
};

int plugin_load(const char* plugin, void* parameter)
{
    int rc;
    struct plugin_header *hdr;
    const char *p = strrchr(plugin,'/');
#ifdef SIMULATOR
    void *pd;
#else
    int fd;
    ssize_t readsize;
#endif
#ifdef HAVE_LCD_BITMAP
    int xm, ym;
#endif
#ifdef HAVE_REMOTE_LCD
    int rxm, rym;
#endif

#if LCD_DEPTH > 1
    fb_data* old_backdrop;
#endif

    if (!p)
        p = plugin;
    action_signalscreenchange();
    
    if (pfn_tsr_exit != NULL) /* if we have a resident old plugin: */
    {
        if (pfn_tsr_exit(!strcmp(current_plugin,p)) == false )
        {
            /* not allowing another plugin to load */
            return PLUGIN_OK;
        }
        pfn_tsr_exit = NULL;
        plugin_loaded = false;
    }
    
    gui_syncsplash(0, str(LANG_WAIT));
    strcpy(current_plugin,p);

#ifdef SIMULATOR
    hdr = sim_plugin_load((char *)plugin, &pd);
    if (pd == NULL) {
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_CANT_OPEN), plugin);
        return -1;
    }
    if (hdr == NULL
        || hdr->magic != PLUGIN_MAGIC
        || hdr->target_id != TARGET_ID) {
        sim_plugin_close(pd);
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION) {
        sim_plugin_close(pd);
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
#else
    fd = open(plugin, O_RDONLY);
    if (fd < 0) {
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_CANT_OPEN), plugin);
        return fd;
    }

    readsize = read(fd, pluginbuf, PLUGIN_BUFFER_SIZE);
    close(fd);

    if (readsize < 0) {
        gui_syncsplash(HZ*2, str(LANG_READ_FAILED), plugin);
        return -1;
    }
    hdr = (struct plugin_header *)pluginbuf;

    if ((unsigned)readsize <= sizeof(struct plugin_header)
        || hdr->magic != PLUGIN_MAGIC
        || hdr->target_id != TARGET_ID
        || hdr->load_addr != pluginbuf
        || hdr->end_addr > pluginbuf + PLUGIN_BUFFER_SIZE) {
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION) {
        gui_syncsplash(HZ*2, str(LANG_PLUGIN_WRONG_VERSION));
        return -1;
    }
    plugin_size = hdr->end_addr - pluginbuf;

    /* zero out bss area only, above guards end of pluginbuf */
    memset(pluginbuf + readsize, 0, plugin_size - readsize);
#endif

    plugin_loaded = true;

#ifdef HAVE_LCD_BITMAP
    xm = lcd_getxmargin();
    ym = lcd_getymargin();
    lcd_setmargins(0,0);

#if LCD_DEPTH > 1
    old_backdrop = lcd_get_backdrop();
#endif

    lcd_clear_display();
    lcd_update();
#else /* !HAVE_LCD_BITMAP */
    lcd_clear_display();
#endif

#ifdef HAVE_REMOTE_LCD
    rxm = lcd_remote_getxmargin();
    rym = lcd_remote_getymargin();
    lcd_remote_setmargins(0, 0);
    lcd_remote_clear_display();
    lcd_remote_update();
#endif

    invalidate_icache();

    rc = hdr->entry_point((struct plugin_api*) &rockbox_api, parameter);
    /* explicitly casting the pointer here to avoid touching every plugin. */

    button_clear_queue();

#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
    lcd_set_backdrop(old_backdrop);
#ifdef HAVE_LCD_COLOR
    lcd_set_drawinfo(DRMODE_SOLID, global_settings.fg_color,
                                   global_settings.bg_color);
#else
    lcd_set_drawinfo(DRMODE_SOLID, LCD_DEFAULT_FG, LCD_DEFAULT_BG);
#endif
#else /* LCD_DEPTH == 1 */
    lcd_set_drawmode(DRMODE_SOLID);
#endif /* LCD_DEPTH */
    /* restore margins */
    lcd_setmargins(xm,ym);
    lcd_clear_display();
    lcd_update();
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH > 1
    lcd_remote_set_drawinfo(DRMODE_SOLID, LCD_REMOTE_DEFAULT_FG,
                            LCD_REMOTE_DEFAULT_BG);
#else
    lcd_remote_set_drawmode(DRMODE_SOLID);
#endif
    lcd_remote_setmargins(rxm, rym);
    lcd_remote_clear_display();
    lcd_remote_update();
#endif
    action_signalscreenchange();

    if (pfn_tsr_exit == NULL)
        plugin_loaded = false;

    sim_plugin_close(pd);

    switch (rc) {
        case PLUGIN_OK:
            break;

        case PLUGIN_USB_CONNECTED:
            return PLUGIN_USB_CONNECTED;

        default:
            gui_syncsplash(HZ*2, str(LANG_PLUGIN_ERROR));
            break;
    }
    return PLUGIN_OK;
}

/* Returns a pointer to the portion of the plugin buffer that is not already
   being used.  If no plugin is loaded, returns the entire plugin buffer */
void* plugin_get_buffer(int* buffer_size)
{
    int buffer_pos;

    if (plugin_loaded)
    {
        if (plugin_size >= PLUGIN_BUFFER_SIZE)
            return NULL;

        *buffer_size = PLUGIN_BUFFER_SIZE-plugin_size;
        buffer_pos = plugin_size;
    }
    else
    {
        *buffer_size = PLUGIN_BUFFER_SIZE;
        buffer_pos = 0;
    }

    return &pluginbuf[buffer_pos];
}

/* Returns a pointer to the mp3 buffer.
   Playback gets stopped, to avoid conflicts.
   Talk buffer is stolen as well.
 */
void* plugin_get_audio_buffer(int* buffer_size)
{
#if CONFIG_CODEC == SWCODEC
    return audio_get_buffer(true, (size_t *)buffer_size);
#else
    audio_stop();
    talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
    *buffer_size = audiobufend - audiobuf;
    return audiobuf;
#endif
}

#ifdef IRAM_STEAL
/* Initializes plugin IRAM */
void plugin_iram_init(char *iramstart, char *iramcopy, size_t iram_size,
                      char *iedata, size_t iedata_size)
{
    audio_iram_steal();
    memcpy(iramstart, iramcopy, iram_size);
    memset(iedata, 0, iedata_size);
    memset(iramcopy, 0, iram_size);
}
#endif /* IRAM_STEAL */

/* The plugin wants to stay resident after leaving its main function, e.g.
   runs from timer or own thread. The callback is registered to later
   instruct it to free its resources before a new plugin gets loaded. */
void plugin_tsr(bool (*exit_callback)(bool))
{
    pfn_tsr_exit = exit_callback; /* remember the callback for later */
}
