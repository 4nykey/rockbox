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
#include <stdlib.h>
#include <ctype.h>
#include "lang.h"
#include "string.h"
#include "config.h"
#include "file.h"
#include "dir.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "sprintf.h"
#include "errno.h"
#include "system.h"
#include "timefuncs.h"
#include "screens.h"
#include "talk.h"
#include "mpeg.h"
#include "audio.h"
#include "mp3_playback.h"
#include "settings.h"
#include "ata.h"
#include "ata_idle_notify.h"
#include "kernel.h"
#include "power.h"
#include "powermgmt.h"
#include "backlight.h"
#include "atoi.h"
#include "version.h"
#include "font.h"
#include "splash.h"
#include "tagcache.h"
#include "scrobbler.h"
#ifdef HAVE_MMC
#include "ata_mmc.h"
#endif
#include "tree.h"
#include "eeprom_settings.h"

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#include "icons.h"
#endif /* End HAVE_LCD_BITMAP */
#include "gui/gwps-common.h"

#include "misc.h"

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf, int buf_size, int value,
                       const unsigned char **units, bool bin_scale)
{
    int scale = bin_scale ? 1024 : 1000;
    int fraction = 0;
    int unit_no = 0;
    int i;
    char tbuf[5];

    while (value >= scale)
    {
        fraction = value % scale;
        value /= scale;
        unit_no++;
    }
    if (bin_scale)
        fraction = fraction * 1000 / 1024;

    if (value >= 100 || !unit_no)
        tbuf[0] = '\0';
    else if (value >= 10)
        snprintf(tbuf, sizeof(tbuf), "%01d", fraction / 100);
    else
        snprintf(tbuf, sizeof(tbuf), "%02d", fraction / 10);
    
    if (buf)
    {
        if (strlen(tbuf))
            snprintf(buf, buf_size, "%d%s%s%s", value, str(LANG_POINT),
                     tbuf, P2STR(units[unit_no]));
        else
            snprintf(buf, buf_size, "%d%s", value, P2STR(units[unit_no]));
    }
    else
    {
        /* strip trailing zeros from the fraction */
        for (i = strlen(tbuf) - 1; (i >= 0) && (tbuf[i] == '0'); i--)
            tbuf[i] = '\0';

        talk_number(value, true);
        if (tbuf[0] != 0)
        {
            talk_id(LANG_POINT, true);
            talk_spell(tbuf, true);
        }
        talk_id(P2ID(units[unit_no]), true);
    }
    return buf;
}

/* Create a filename with a number part in a way that the number is 1
 * higher than the highest numbered file matching the same pattern.
 * It is allowed that buffer and path point to the same memory location,
 * saving a strcpy(). Path must always be given without trailing slash.
 * "num" can point to an int specifying the number to use or NULL or a value
 * less than zero to number automatically. The final number used will also
 * be returned in *num. If *num is >= 0 then *num will be incremented by
 * one. */
char *create_numbered_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               int numberlen IF_CNFN_NUM_(, int *num))
{
    DIR *dir;
    struct dirent *entry;
    int max_num;
    int pathlen;
    int prefixlen = strlen(prefix);
    char fmtstring[12];

    if (buffer != path)
        strncpy(buffer, path, MAX_PATH);

    pathlen = strlen(buffer);

#ifdef IF_CNFN_NUM
    if (num && *num >= 0)
    {
        /* number specified */
        max_num = *num;
    }
    else
#endif
    {
        /* automatic numbering */
        max_num = 0;

    dir = opendir(pathlen ? buffer : "/");
    if (!dir)
        return NULL;

    while ((entry = readdir(dir)))
    {
        int curr_num;

        if (strncasecmp((char *)entry->d_name, prefix, prefixlen)
            || strcasecmp((char *)entry->d_name + prefixlen + numberlen, suffix))
            continue;

        curr_num = atoi((char *)entry->d_name + prefixlen);
        if (curr_num > max_num)
            max_num = curr_num;
    }

    closedir(dir);
    }

    max_num++;

    snprintf(fmtstring, sizeof(fmtstring), "/%%s%%0%dd%%s", numberlen);
    snprintf(buffer + pathlen, MAX_PATH - pathlen, fmtstring, prefix,
             max_num, suffix);

#ifdef IF_CNFN_NUM
    if (num)
        *num = max_num;
#endif

    return buffer;
}

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t)
{
    if ( t < 3600000 ) 
    {
      snprintf(buf, buf_size, "%d:%02d",
               (int) (t / 60000), (int) (t % 60000 / 1000));
    } 
    else
    {
      snprintf(buf, buf_size, "%d:%02d:%02d",
               (int) (t / 3600000), (int) (t % 3600000 / 60000),
               (int) (t % 60000 / 1000));
    }
}

#if CONFIG_RTC
/* Create a filename with a date+time part.
   It is allowed that buffer and path point to the same memory location,
   saving a strcpy(). Path must always be given without trailing slash.
   unique_time as true makes the function wait until the current time has
   changed. */
char *create_datetime_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               bool unique_time)
{
    struct tm *tm = get_time();
    static struct tm last_tm;
    int pathlen;

    while (unique_time && !memcmp(get_time(), &last_tm, sizeof (struct tm)))
        sleep(HZ/10);

    last_tm = *tm;

    if (buffer != path)
        strncpy(buffer, path, MAX_PATH);

    pathlen = strlen(buffer);
    snprintf(buffer + pathlen, MAX_PATH - pathlen,
             "/%s%02d%02d%02d-%02d%02d%02d%s", prefix,
             tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec, suffix);

    return buffer;
}
#endif /* CONFIG_RTC */

/* Read (up to) a line of text from fd into buffer and return number of bytes
 * read (which may be larger than the number of bytes stored in buffer). If
 * an error occurs, -1 is returned (and buffer contains whatever could be
 * read). A line is terminated by a LF char. Neither LF nor CR chars are
 * stored in buffer.
 */
int read_line(int fd, char* buffer, int buffer_size)
{
    int count = 0;
    int num_read = 0;
    
    errno = 0;

    while (count < buffer_size)
    {
        unsigned char c;

        if (1 != read(fd, &c, 1))
            break;
        
        num_read++;
            
        if ( c == '\n' )
            break;

        if ( c == '\r' )
            continue;

        buffer[count++] = c;
    }

    buffer[MIN(count, buffer_size - 1)] = 0;

    return errno ? -1 : num_read;
}

/* Performance optimized version of the previous function. */
int fast_readline(int fd, char *buf, int buf_size, void *parameters,
                  int (*callback)(int n, const char *buf, void *parameters))
{
    char *p, *next;
    int rc, pos = 0;
    int count = 0;
    
    while ( 1 )
    {
        next = NULL;
        
        rc = read(fd, &buf[pos], buf_size - pos - 1);
        if (rc >= 0)
            buf[pos+rc] = '\0';
        
        if ( (p = strchr(buf, '\r')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        else
            p = buf;
        
        if ( (p = strchr(p, '\n')) != NULL)
        {
            *p = '\0';
            next = ++p;
        }
        
        rc = callback(count, buf, parameters);
        if (rc < 0)
            return rc;
        
        count++;
        if (next)
        {
            pos = buf_size - ((long)next - (long)buf) - 1;
            memmove(buf, next, pos);
        }
        else
            break ;
    }
    
    return 0;
}

#ifdef HAVE_LCD_BITMAP

#if LCD_DEPTH == 16 
#define BMP_COMPRESSION 3 /* BI_BITFIELDS */
#define BMP_NUMCOLORS 3
#else
#define BMP_COMPRESSION 0 /* BI_RGB */
#if LCD_DEPTH <= 8
#define BMP_NUMCOLORS (1 << LCD_DEPTH)
#else
#define BMP_NUMCOLORS 0
#endif
#endif

#if LCD_DEPTH == 1
#define BMP_BPP 1
#define BMP_LINESIZE ((LCD_WIDTH/8 + 3) & ~3)
#elif LCD_DEPTH <= 4
#define BMP_BPP 4
#define BMP_LINESIZE ((LCD_WIDTH/2 + 3) & ~3)
#elif LCD_DEPTH <= 8
#define BMP_BPP 8
#define BMP_LINESIZE ((LCD_WIDTH + 3) & ~3)
#elif LCD_DEPTH <= 16
#define BMP_BPP 16
#define BMP_LINESIZE ((LCD_WIDTH*2 + 3) & ~3)
#else
#define BMP_BPP 24
#define BMP_LINESIZE ((LCD_WIDTH*3 + 3) & ~3)
#endif

#define BMP_HEADERSIZE (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE   (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE  (BMP_HEADERSIZE + BMP_DATASIZE)

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff

static const unsigned char bmpheader[] =
{
    0x42, 0x4d,                 /* 'BM' */
    LE32_CONST(BMP_TOTALSIZE),  /* Total file size */
    0x00, 0x00, 0x00, 0x00,     /* Reserved */
    LE32_CONST(BMP_HEADERSIZE), /* Offset to start of pixel data */

    0x28, 0x00, 0x00, 0x00,     /* Size of (2nd) header */
    LE32_CONST(LCD_WIDTH),      /* Width in pixels */
    LE32_CONST(LCD_HEIGHT),     /* Height in pixels */
    0x01, 0x00,                 /* Number of planes (always 1) */
    LE16_CONST(BMP_BPP),        /* Bits per pixel 1/4/8/16/24 */
    LE32_CONST(BMP_COMPRESSION),/* Compression mode */
    LE32_CONST(BMP_DATASIZE),   /* Size of bitmap data */
    0xc4, 0x0e, 0x00, 0x00,     /* Horizontal resolution (pixels/meter) */
    0xc4, 0x0e, 0x00, 0x00,     /* Vertical resolution (pixels/meter) */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of used colours */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of important colours */

#if LCD_DEPTH == 1
    0x90, 0xee, 0x90, 0x00,     /* Colour #0 */
    0x00, 0x00, 0x00, 0x00      /* Colour #1 */
#elif LCD_DEPTH == 2
    0xe6, 0xd8, 0xad, 0x00,     /* Colour #0 */
    0x99, 0x90, 0x73, 0x00,     /* Colour #1 */
    0x4c, 0x48, 0x39, 0x00,     /* Colour #2 */
    0x00, 0x00, 0x00, 0x00      /* Colour #3 */
#elif LCD_DEPTH == 16
    0x00, 0xf8, 0x00, 0x00,     /* red bitfield mask */
    0xe0, 0x07, 0x00, 0x00,     /* green bitfield mask */
    0x1f, 0x00, 0x00, 0x00      /* blue bitfield mask */
#endif
};

static void (*screen_dump_hook)(int fh) = NULL;

void screen_dump(void)
{
    int fh;
    char filename[MAX_PATH];
    int bx, by;
#if LCD_DEPTH == 1
    static unsigned char line_block[8][BMP_LINESIZE];
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    static unsigned char line_block[BMP_LINESIZE];
#else
    static unsigned char line_block[4][BMP_LINESIZE];
#endif
#elif LCD_DEPTH == 16
    static unsigned short line_block[BMP_LINESIZE/2];
#endif

#if CONFIG_RTC
    create_datetime_filename(filename, "", "dump ", ".bmp", false);
#else
    create_numbered_filename(filename, "", "dump_", ".bmp", 4
                             IF_CNFN_NUM_(, NULL));
#endif

    fh = creat(filename);
    if (fh < 0)
        return;

    if (screen_dump_hook)
    {
        screen_dump_hook(fh);
    }
    else
    {
        write(fh, bmpheader, sizeof(bmpheader));

        /* BMP image goes bottom up */
#if LCD_DEPTH == 1
        for (by = LCD_FBHEIGHT - 1; by >= 0; by--)
        {
            unsigned char *src = &lcd_framebuffer[by][0];
            unsigned char *dst = &line_block[0][0];

            memset(line_block, 0, sizeof(line_block));
            for (bx = LCD_WIDTH/8; bx > 0; bx--)
            {
                unsigned dst_mask = 0x80;
                int ix;

                for (ix = 8; ix > 0; ix--)
                {
                    unsigned char *dst_blk = dst;
                    unsigned src_byte = *src++;
                    int iy;

                    for (iy = 8; iy > 0; iy--)
                    {
                        if (src_byte & 0x80)
                            *dst_blk |= dst_mask;
                        src_byte <<= 1;
                        dst_blk += BMP_LINESIZE;
                    }
                    dst_mask >>= 1;
                }
                dst++;
            }

            write(fh, line_block, sizeof(line_block));
        }
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
        for (by = LCD_FBHEIGHT - 1; by >= 0; by--)
        {
            unsigned char *src = &lcd_framebuffer[by][0];
            unsigned char *dst = line_block;

            memset(line_block, 0, sizeof(line_block));
            for (bx = LCD_FBWIDTH; bx > 0; bx--)
            {
                unsigned src_byte = *src++;

                *dst++ = ((src_byte >> 2) & 0x30) | ((src_byte >> 4) & 0x03);
                *dst++ = ((src_byte << 2) & 0x30) | (src_byte & 0x03);
            }

            write(fh, line_block, sizeof(line_block));
        }
#else /* VERTICAL_PACKING */
        for (by = LCD_FBHEIGHT - 1; by >= 0; by--)
        {
            unsigned char *src = &lcd_framebuffer[by][0];
            unsigned char *dst = &line_block[3][0];

            memset(line_block, 0, sizeof(line_block));
            for (bx = LCD_WIDTH/2; bx > 0; bx--)
            {
                unsigned char *dst_blk = dst++;
                unsigned src_byte0 = *src++;
                unsigned src_byte1 = *src++;
                int iy;

                for (iy = 4; iy > 0; iy--)
                {
                    *dst_blk = ((src_byte0 & 3) << 4) | (src_byte1 & 3);
                    src_byte0 >>= 2;
                    src_byte1 >>= 2;
                    dst_blk -= BMP_LINESIZE;
                }
            }

            write(fh, line_block, sizeof(line_block));
        }
#endif
#elif LCD_DEPTH == 16
        for (by = LCD_HEIGHT - 1; by >= 0; by--)
        {
            unsigned short *src = &lcd_framebuffer[by][0];
            unsigned short *dst = line_block;

            memset(line_block, 0, sizeof(line_block));
            for (bx = LCD_WIDTH; bx > 0; bx--)
            {
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
                /* iPod LCD data is big endian although the CPU is not */
                *dst++ = htobe16(*src++);
#else
                *dst++ = htole16(*src++);
#endif
            }

            write(fh, line_block, sizeof(line_block));
        }
#endif /* LCD_DEPTH */
    }

    close(fh);
}

void screen_dump_set_hook(void (*hook)(int fh))
{
    screen_dump_hook = hook;
}

#endif /* HAVE_LCD_BITMAP */

/* parse a line from a configuration file. the line format is:

   name: value

   Any whitespace before setting name or value (after ':') is ignored.
   A # as first non-whitespace character discards the whole line.
   Function sets pointers to null-terminated setting name and value.
   Returns false if no valid config entry was found.
*/

bool settings_parseline(char* line, char** name, char** value)
{
    char* ptr;

    while ( isspace(*line) )
        line++;

    if ( *line == '#' )
        return false;

    ptr = strchr(line, ':');
    if ( !ptr )
        return false;

    *name = line;
    *ptr = 0;
    ptr++;
    while (isspace(*ptr))
        ptr++;
    *value = ptr;
    return true;
}

static void system_flush(void)
{
    tree_flush();
    call_ata_idle_notifys(true); /*doesnt work on usb and shutdown from ata thread */
}

static void system_restore(void)
{
    tree_restore();
}

static bool clean_shutdown(void (*callback)(void *), void *parameter)
{
#ifdef SIMULATOR
    (void)callback;
    (void)parameter;
    call_ata_idle_notifys(true);
    exit(0);
#else
    int i;

#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING)
    if(!charger_inserted())
#endif
    {
        bool batt_crit = battery_level_critical();
        int audio_stat = audio_status();

        FOR_NB_SCREENS(i)
            screens[i].clear_display();
#ifdef X5_BACKLIGHT_SHUTDOWN
        x5_backlight_shutdown();
#endif
        if (!battery_level_safe())
            gui_syncsplash(3*HZ, "%s %s",
                           str(LANG_WARNING_BATTERY_EMPTY),
                           str(LANG_SHUTTINGDOWN));
        else if (battery_level_critical())
            gui_syncsplash(3*HZ, "%s %s",
                           str(LANG_WARNING_BATTERY_LOW),
                           str(LANG_SHUTTINGDOWN));
        else {
#ifdef HAVE_TAGCACHE
            if (!tagcache_prepare_shutdown())
            {
                cancel_shutdown();
                gui_syncsplash(HZ, str(LANG_TAGCACHE_BUSY));
                return false;
            }
#endif
            gui_syncsplash(0, str(LANG_SHUTTINGDOWN));
        }
        
        if (global_settings.fade_on_stop 
            && (audio_stat & AUDIO_STATUS_PLAY))
        {
            fade(0);
        }

#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
        if (!batt_crit && (audio_stat & AUDIO_STATUS_RECORD))
        {
            audio_stop_recording();
            while(audio_status() & AUDIO_STATUS_RECORD)
                sleep(1);
        }
            
        audio_close_recording();
#endif
        /* audio_stop_recording == audio_stop for HWCODEC */

        audio_stop();
        while (audio_status())
            sleep(1);
        
        if (callback != NULL)
            callback(parameter);

        if (!batt_crit) /* do not save on critical battery */
            system_flush();
#ifdef HAVE_EEPROM_SETTINGS
        if (firmware_settings.initialized)
        {
            firmware_settings.disk_clean = true;
            firmware_settings.bl_version = 0;
            eeprom_settings_store();
        }
#endif
        shutdown_hw();
    }
#endif
    return false;
}

#if CONFIG_CHARGING
static bool waiting_to_resume_play = false;
static long play_resume_tick;

static void car_adapter_mode_processing(bool inserted)
{    
    if (global_settings.car_adapter_mode)
    {
        if(inserted)
        { 
            /*
             * Just got plugged in, delay & resume if we were playing
             */
            if (audio_status() & AUDIO_STATUS_PAUSE)
            {
                /* delay resume a bit while the engine is cranking */
                play_resume_tick = current_tick + HZ*5;
                waiting_to_resume_play = true;
            }
        }
        else
        {
            /*
             * Just got unplugged, pause if playing
             */
            if ((audio_status() & AUDIO_STATUS_PLAY) &&
                !(audio_status() & AUDIO_STATUS_PAUSE))
            {
                if (global_settings.fade_on_stop)
                    fade(0);
                else
                    audio_pause();
            }
        }
    }
}

static void car_adapter_tick(void)
{
    if (waiting_to_resume_play)
    {
        if (TIME_AFTER(current_tick, play_resume_tick))
        {
            if (audio_status() & AUDIO_STATUS_PAUSE)
            {
                audio_resume(); 
            }
            waiting_to_resume_play = false;
        }
    }
}

void car_adapter_mode_init(void)
{
    tick_add_task(car_adapter_tick);
}
#endif

#ifdef HAVE_HEADPHONE_DETECTION
static void unplug_change(bool inserted)
{
    if (global_settings.unplug_mode)
    {
        if (inserted)
        {
            if ( global_settings.unplug_mode > 1 )
                audio_resume();
            backlight_on();
        } else {
            audio_pause();

            if (global_settings.unplug_rw)
            {
                if ( audio_current_track()->elapsed >
                (unsigned long)(global_settings.unplug_rw*1000))
                    audio_ff_rewind(audio_current_track()->elapsed -
                    (global_settings.unplug_rw*1000));
                else
                    audio_ff_rewind(0);
            }
        }
    }
}
#endif

long default_event_handler_ex(long event, void (*callback)(void *), void *parameter)
{
    switch(event)
    {
        case SYS_USB_CONNECTED:
            if (callback != NULL)
                callback(parameter);
#ifdef HAVE_MMC
            if (!mmc_touched() || (mmc_remove_request() == SYS_MMC_EXTRACTED))
#endif
            {
                scrobbler_flush_cache();
                system_flush();
                usb_screen();
                system_restore();
            }
            return SYS_USB_CONNECTED;
        case SYS_POWEROFF:
            if (!clean_shutdown(callback, parameter))
                return SYS_POWEROFF;
            break;
#if CONFIG_CHARGING
        case SYS_CHARGER_CONNECTED:
            car_adapter_mode_processing(true);
            return SYS_CHARGER_CONNECTED;
            
        case SYS_CHARGER_DISCONNECTED:
            car_adapter_mode_processing(false);
            return SYS_CHARGER_DISCONNECTED;
#endif
#ifdef HAVE_HEADPHONE_DETECTION
        case SYS_PHONE_PLUGGED:
            unplug_change(true);
            return SYS_PHONE_PLUGGED;

        case SYS_PHONE_UNPLUGGED:
            unplug_change(false);
            return SYS_PHONE_UNPLUGGED;
#endif
    }
    return 0;
}

long default_event_handler(long event)
{
    return default_event_handler_ex(event, NULL, NULL);
}

int show_logo( void )
{
#ifdef HAVE_LCD_BITMAP
    char version[32];
    int font_h, font_w;

    lcd_clear_display();
    lcd_bitmap(rockboxlogo, 0, 10, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);

#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    lcd_remote_bitmap(remote_rockboxlogo, 0, 10, BMPWIDTH_remote_rockboxlogo,
                      BMPHEIGHT_remote_rockboxlogo);
#endif

    snprintf(version, sizeof(version), "Ver. %s", appsversion);
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(version)*font_w)/2),
               LCD_HEIGHT-font_h, (unsigned char *)version);
    lcd_update();

#ifdef HAVE_REMOTE_LCD
    lcd_remote_setfont(FONT_SYSFIXED);
    lcd_remote_getstringsize((unsigned char *)"A", &font_w, &font_h);
    lcd_remote_putsxy((LCD_REMOTE_WIDTH/2) - ((strlen(version)*font_w)/2),
               LCD_REMOTE_HEIGHT-font_h, (unsigned char *)version);
    lcd_remote_update();
#endif

#else
    char *rockbox = "  ROCKbox!";
    lcd_clear_display();
    lcd_double_height(true);
    lcd_puts(0, 0, rockbox);
    lcd_puts_scroll(0, 1, appsversion);
#endif

    return 0;
}

#if CONFIG_CODEC == SWCODEC
int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    int type;

    bool track = ((global_settings.replaygain_type == REPLAYGAIN_TRACK)
        || ((global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
            && global_settings.playlist_shuffle));

    type = (!track && have_album_gain) ? REPLAYGAIN_ALBUM 
        : have_track_gain ? REPLAYGAIN_TRACK : -1;
    
    return type;
}
#endif
