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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <atoi.h>
#include <timefuncs.h>
#include "button.h"
#include "lcd.h"
#include "dir.h"
#include "file.h"
#include "kernel.h"
#include "sprintf.h"
#include "screens.h"
#include "misc.h"
#include "mas.h"
#include "plugin.h"
#include "lang.h"
#include "keyboard.h"
#include "mpeg.h"
#include "buffer.h"
#include "mp3_playback.h"
#include "backlight.h"
#include "ata.h"
#include "talk.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

#ifdef SIMULATOR
  #include <debug.h>
  #ifdef WIN32
    #include "plugin-win32.h"
    #define PREFIX(_x_) _x_
  #else
    #include <dlfcn.h>
    #define PREFIX(_x_) x11_ ## _x_
  #endif
#else
#define PREFIX(_x_) _x_
#endif

#define PLUGIN_BUFFER_SIZE 0x8000

#ifdef SIMULATOR
static unsigned char pluginbuf[PLUGIN_BUFFER_SIZE];
#else
extern unsigned char pluginbuf[];
extern void bitswap(unsigned char *data, int length);
#endif

static bool plugin_loaded = false;
static int  plugin_size = 0;
#ifndef SIMULATOR
static void (*pfn_timer)(void) = NULL; /* user timer handler */
#endif
static void (*pfn_tsr_exit)(void) = NULL; /* TSR exit callback */

static int plugin_test(int api_version, int model, int memsize);

static struct plugin_api rockbox_api = {
    PLUGIN_API_VERSION,

    plugin_test,
    
    /* lcd */
    lcd_clear_display,
    lcd_puts,
    lcd_puts_scroll,
    lcd_stop_scroll,
#ifdef HAVE_LCD_CHARCELLS
    lcd_define_pattern,
    lcd_get_locked_pattern,
    lcd_unlock_pattern,
    lcd_putc,
#else
    lcd_putsxy,
    lcd_bitmap,
    lcd_drawline,
    lcd_clearline,
    lcd_drawpixel,
    lcd_clearpixel,
    lcd_setfont,
    lcd_clearrect,
    lcd_fillrect,
    lcd_drawrect,
    lcd_invertrect,
    lcd_getstringsize,
    lcd_update,
    lcd_update_rect,
    progressbar,
    slidebar,
    scrollbar,
#ifndef SIMULATOR
    lcd_roll,
#endif
#endif

    /* button */
    button_get,
    button_get_w_tmo,

    /* file */
    (open_func)PREFIX(open),
    PREFIX(close),
    (read_func)read,
    lseek,
    (creat_func)PREFIX(creat),
    (write_func)write,
    remove,
    rename,
    ftruncate,
    PREFIX(filesize),
    fprintf,
    read_line,

    /* dir */
    PREFIX(opendir),
    PREFIX(closedir),
    PREFIX(readdir),

    /* kernel */
    PREFIX(sleep),
    usb_screen,
    &current_tick,

    /* strings and memory */
    snprintf,
    strcpy,
    strlen,
    memset,
    memcpy,

    /* sound */
#ifndef SIMULATOR
#ifdef HAVE_MAS3587F
    mas_codec_readreg,
#endif
#endif
    
    /* misc */
    srand,
    rand,
    splash,
    (qsort_func)qsort,
    kbd_input,
    mpeg_current_track,
    atoi,
    get_time,
    plugin_get_buffer,

    /* new stuff at the end, sort into place next time the API gets incompatible */

#ifndef HAVE_LCD_CHARCELLS
    &lcd_framebuffer[0][0],
    lcd_blit,
#endif
    yield,

    plugin_get_mp3_buffer,
    mpeg_sound_set,
#ifndef SIMULATOR
    mp3_play_init,
    mp3_play_data,
    mp3_play_pause,
    mp3_play_stop,
    mp3_is_playing,
    bitswap,
#endif
    &global_settings,
    backlight_set_timeout,
#ifndef SIMULATOR
    ata_sleep,
#endif
#ifdef HAVE_LCD_BITMAP
    checkbox,
#endif
#ifndef SIMULATOR
    plugin_register_timer,
    plugin_unregister_timer,
#endif
    plugin_tsr,
    create_thread,
    remove_thread,
    lcd_set_contrast,
    mpeg_play,
    mpeg_stop,
    mpeg_pause,
    mpeg_resume,
    mpeg_next,
    mpeg_prev,
    mpeg_ff_rewind,
    mpeg_next_track,
    mpeg_has_changed_track,
    mpeg_status,
#ifdef HAVE_LCD_BITMAP
    font_get,
#endif
};

int plugin_load(char* plugin, void* parameter)
{
    enum plugin_status (*plugin_start)(struct plugin_api* api, void* param);
    int rc;
    char buf[64];
#ifdef SIMULATOR
    void* pd;
    char path[256];
#else
    int fd;
#endif
#ifdef HAVE_LCD_BITMAP
    int xm,ym;
#endif

    if (pfn_tsr_exit != NULL) /* if we have a resident old plugin: */
    {
        pfn_tsr_exit(); /* force it to exit now */
        pfn_tsr_exit = NULL;
    }

#ifdef HAVE_LCD_BITMAP
    lcd_clear_display();
    xm = lcd_getxmargin();
    ym = lcd_getymargin();
    lcd_setmargins(0,0);
    lcd_update();
#else
    lcd_clear_display();
#endif
#ifdef SIMULATOR
#ifdef WIN32
    snprintf(path, sizeof path, "%s", plugin);
#else
    snprintf(path, sizeof path, "archos%s", plugin);
#endif
    pd = dlopen(path, RTLD_NOW);
    if (!pd) {
        snprintf(buf, sizeof buf, "Can't open %s", plugin);
        splash(HZ*2, true, buf);
        DEBUGF("dlopen(%s): %s\n",path,dlerror());
        dlclose(pd);
        return -1;
    }

    plugin_start = dlsym(pd, "plugin_start");
    if (!plugin_start) {
        plugin_start = dlsym(pd, "_plugin_start");
        if (!plugin_start) {
            splash(HZ*2, true, "Can't find entry point");
            dlclose(pd);
            return -1;
        }
    }
#else
    fd = open(plugin, O_RDONLY);
    if (fd < 0) {
        snprintf(buf, sizeof buf, str(LANG_PLUGIN_CANT_OPEN), plugin);
        splash(HZ*2, true, buf);
        return fd;
    }

    plugin_start = (void*)&pluginbuf;
    plugin_size = read(fd, plugin_start, PLUGIN_BUFFER_SIZE);
    close(fd);
    if (plugin_size < 0) {
        /* read error */
        snprintf(buf, sizeof buf, str(LANG_READ_FAILED), plugin);
        splash(HZ*2, true, buf);
        return -1;
    }
    if (plugin_size == 0) {
        /* loaded a 0-byte plugin, implying it's not for this model */
        splash(HZ*2, true, str(LANG_PLUGIN_WRONG_MODEL));
        return -1;
    }
#endif

    plugin_loaded = true;
    rc = plugin_start(&rockbox_api, parameter);
    plugin_loaded = false;

    switch (rc) {
        case PLUGIN_OK:
            break;

        case PLUGIN_USB_CONNECTED:
            return PLUGIN_USB_CONNECTED;

        case PLUGIN_WRONG_API_VERSION:
            splash(HZ*2, true, str(LANG_PLUGIN_WRONG_VERSION));
            break;

        case PLUGIN_WRONG_MODEL:
            splash(HZ*2, true, str(LANG_PLUGIN_WRONG_MODEL));
            break;

        default:
            splash(HZ*2, true, str(LANG_PLUGIN_ERROR));
            break;
    }

#ifdef SIMULATOR
    dlclose(pd);
#endif

#ifdef HAVE_LCD_BITMAP
    /* restore margins */
    lcd_setmargins(xm,ym);
#endif

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
   Playback gets stopped, to avoid conflicts. */
void* plugin_get_mp3_buffer(int* buffer_size)
{
#ifdef SIMULATOR
    static unsigned char buf[1700*1024];
    *buffer_size = sizeof(buf);
    return buf;
#else
    mpeg_stop();
    talk_buffer_steal(); /* we use the mp3 buffer, need to tell */
    *buffer_size = mp3end - mp3buf;
    return mp3buf;
#endif 
}

#ifndef SIMULATOR
/* Register a periodic time callback, called every "cycles" CPU clocks. 
   Note that this function will be called in interrupt context! */ 
int plugin_register_timer(int cycles, int prio, void (*timer_callback)(void))
{
    int phi = 0; /* bits for the prescaler */
    int prescale = 1;

    while (cycles > 0x10000)
    {   /* work out the smallest prescaler that makes it fit */
        phi++;
        prescale *= 2;
        cycles /= 2;
    }

    if (prescale > 8 || cycles == 0 || prio < 1 || prio > 15)
        return 0; /* error, we can't do such period, bad argument */

    and_b(~0x10, &TSTR); /* Stop the timer 4 */
    and_b(~0x10, &TSNC); /* No synchronization */
    and_b(~0x10, &TMDR); /* Operate normally */

    pfn_timer = timer_callback; /* install 2nd level ISR */

    and_b(~0x01, &TSR4);
    TIER4 = 0xF9; /* Enable GRA match interrupt */

    GRA4 = (unsigned short)(cycles - 1);
    TCR4 = 0x20 | phi; /* clear at GRA match, set prescaler */
    IPRD = (IPRD & 0xFF0F) | prio << 4; /* interrupt priority */
    or_b(0x10, &TSTR); /* start timer 4 */

    return cycles * prescale; /* return the actual period, in CPU clocks */
}

/* disable the user timer */
void plugin_unregister_timer(void)
{
    and_b(~0x10, &TSTR); /* stop the timer 4 */
    IPRD = (IPRD & 0xFF0F); /* disable interrupt */
    pfn_timer = NULL;
}

/* interrupt handler for user timer */
#pragma interrupt
void IMIA4(void)
{
    if (pfn_timer != NULL)
        pfn_timer(); /* call the user timer function */
    and_b(~0x01, &TSR4); /* clear the interrupt */
}
#endif /* #ifndef SIMULATOR */

/* The plugin wants to stay resident after leaving its main function, e.g.
   runs from timer or own thread. The callback is registered to later 
   instruct it to free its resources before a new plugin gets loaded. */
void plugin_tsr(void (*exit_callback)(void))
{
    pfn_tsr_exit = exit_callback; /* remember the callback for later */
}


static int plugin_test(int api_version, int model, int memsize)
{
    if (api_version < PLUGIN_MIN_API_VERSION ||
        api_version > PLUGIN_API_VERSION)
        return PLUGIN_WRONG_API_VERSION;

    if (model != MODEL)
        return PLUGIN_WRONG_MODEL;

    if (memsize != MEM)
        return PLUGIN_WRONG_MODEL;
    
    return PLUGIN_OK;
}
