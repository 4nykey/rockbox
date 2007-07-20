/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gameboy emulator based on gnuboy
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "loader.h"
#include "rockmacros.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
struct plugin_api* rb;
int shut,cleanshut;
char *errormsg;
int gnuboy_main(char *rom);
void pcm_close(void);

#define optionname "options"

void die(char *message, ...)
{
    shut=1;
    errormsg=message;
}

struct options options;

void *audio_bufferbase;
void *audio_bufferpointer;
size_t audio_buffer_free;

void *my_malloc(size_t size)
{
    void *alloc;

    if (size + 4 > audio_buffer_free)
        return 0;
    alloc = audio_bufferpointer;
    audio_bufferpointer += size + 4;
    audio_buffer_free -= size + 4;
    return alloc;
}

/* Using #define isn't enough with GCC 4.0.1 */

void* memcpy(void* dst, const void* src, size_t size)
{
    return rb->memcpy(dst, src, size);
}

void setoptions (void)
{
   int fd;
   DIR* dir;
   char optionsave[sizeof(savedir)+sizeof(optionname)];

    dir=rb->opendir(savedir);
    if(!dir)
      rb->mkdir(savedir);
    else
      rb->closedir(dir);

   snprintf(optionsave, sizeof(optionsave), "%s/%s", savedir, optionname);

    fd = open(optionsave, O_RDONLY);
    if(fd < 0) /* no options to read, set defaults */
    {
        options.LEFT=BUTTON_LEFT;
        options.RIGHT=BUTTON_RIGHT;

#if (CONFIG_KEYPAD == IRIVER_H100_PAD)
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_ON;
        options.B=BUTTON_OFF;
        options.START=BUTTON_REC;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MODE;

#elif (CONFIG_KEYPAD == IRIVER_H300_PAD)
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_REC;
        options.B=BUTTON_MODE;
        options.START=BUTTON_ON;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_OFF;

#elif CONFIG_KEYPAD == RECORDER_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_F1;
        options.B=BUTTON_F2;
        options.START=BUTTON_F3;
        options.SELECT=BUTTON_PLAY;
        options.MENU=BUTTON_OFF;

#elif CONFIG_KEYPAD == IPOD_4G_PAD
        options.UP=BUTTON_MENU;
        options.DOWN=BUTTON_PLAY;

        options.A=BUTTON_NONE;
        options.B=BUTTON_NONE;
        options.START=BUTTON_SELECT;
        options.SELECT=BUTTON_NONE;
        options.MENU=(BUTTON_SELECT | BUTTON_REPEAT);

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_EQ;
        options.START=BUTTON_MODE;
        options.SELECT=(BUTTON_SELECT | BUTTON_REL);
        options.MENU=(BUTTON_SELECT | BUTTON_REPEAT);

#elif CONFIG_KEYPAD == GIGABEAT_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_VOL_UP;
        options.B=BUTTON_VOL_DOWN;
        options.START=BUTTON_A;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MENU;
      
#elif CONFIG_KEYPAD == SANSA_E200_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_SELECT;
        options.B=BUTTON_REC;
        options.START=BUTTON_SCROLL_UP;
        options.SELECT=BUTTON_SCROLL_DOWN;
        options.MENU=BUTTON_POWER;

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_REC;
        options.START=BUTTON_SELECT;
        options.SELECT=BUTTON_NONE;
        options.MENU=BUTTON_POWER;  
      
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
        options.UP=BUTTON_SCROLL_UP;
        options.DOWN=BUTTON_SCROLL_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_FF;
        options.START=BUTTON_REW;
        options.SELECT=BUTTON_NONE;
        options.MENU=BUTTON_POWER;
#endif

      options.maxskip=4;
      options.fps=0;
      options.showstats=0;
#if (LCD_WIDTH>=160) && (LCD_HEIGHT>=144)
      options.fullscreen=0;
#else
      options.fullscreen=1;
#endif
      options.sound=1;
      options.pal=0;
   }
   else
      read(fd,&options, sizeof(options));

    close(fd);
}

void savesettings(void)
{
    int fd;
    char optionsave[sizeof(savedir)+sizeof(optionname)];

    if(options.dirty)
    {
        options.dirty=0;
        snprintf(optionsave, sizeof(optionsave), "%s/%s", savedir, optionname);
        fd = open(optionsave, O_WRONLY|O_CREAT|O_TRUNC);
        write(fd,&options, sizeof(options));
        close(fd);
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    PLUGIN_IRAM_INIT(api)

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;

    rb->lcd_setfont(0);

    rb->lcd_clear_display();

    if (!parameter)
    {
        rb->splash(HZ*3, "Play gameboy ROM file! (.gb/.gbc)");
        return PLUGIN_OK;
    }
    if(rb->audio_status())
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_buffer(&audio_buffer_free);
        plugbuf=true;
    }
    else
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_audio_buffer(&audio_buffer_free);
        plugbuf=false;
    }
#if MEM <= 8 && !defined(SIMULATOR)
    /* loaded as an overlay plugin, protect from overwriting ourselves */
    if ((unsigned)(plugin_start_addr - (unsigned char *)audio_bufferbase)
        < audio_buffer_free)
        audio_buffer_free = plugin_start_addr - (unsigned char *)audio_bufferbase;
#endif
    setoptions();

    shut=0;
    cleanshut=0;

#ifdef HAVE_WHEEL_POSITION
    rb->wheel_send_events(false);
#endif

    gnuboy_main(parameter);

#ifdef HAVE_WHEEL_POSITION
    rb->wheel_send_events(true);
#endif

    if(shut&&!cleanshut)
    {
        rb->splash(HZ/2, errormsg);
        return PLUGIN_ERROR;
    }
    if(!rb->audio_status())
        pcm_close();
    rb->splash(HZ/2, "Closing Rockboy");

    savesettings();

    cleanup();

    return PLUGIN_OK;
}
