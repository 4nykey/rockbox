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
 * Rockbox button functions
 */

#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "serial.h"
#include "power.h"
#include "powermgmt.h"
#include "button-target.h"

#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif

struct event_queue button_queue;

static long lastbtn;   /* Last valid button status */
static long last_read; /* Last button status, for debouncing/filtering */
#ifdef HAVE_LCD_BITMAP
static bool flipped;  /* buttons can be flipped to match the LCD flip */
#endif
#ifdef CONFIG_BACKLIGHT
static bool filter_first_keypress;
#ifdef HAVE_REMOTE_LCD
static bool remote_filter_first_keypress;
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_HEADPHONE_DETECTION
bool phones_present = false;
#endif

/* how long until repeat kicks in, in ticks */
#define REPEAT_START      30

/* the speed repeat starts at, in ticks */
#define REPEAT_INTERVAL_START   16

/* speed repeat finishes at, in ticks */
#define REPEAT_INTERVAL_FINISH  5

static int button_read(void);

static void button_tick(void)
{
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    static bool post = false;
#ifdef CONFIG_BACKLIGHT
    static bool skip_release = false;
#ifdef HAVE_REMOTE_LCD
    static bool skip_remote_release = false;
#endif
#endif
    int diff;
    int btn;

#ifdef HAS_SERIAL_REMOTE
    /* Post events for the remote control */
    btn = remote_control_rx();
    if(btn)
    {
        queue_post(&button_queue, btn, 0);
    }
#endif

#ifdef HAVE_HEADPHONE_DETECTION
    if ( headphones_inserted() )
    {
        if (! phones_present )
        {
            queue_post(&button_queue, SYS_PHONE_PLUGGED, 0);
            phones_present = true;
        }
    } else {
        if ( phones_present )
        {
            queue_post(&button_queue, SYS_PHONE_UNPLUGGED, 0);
            phones_present = false;
        }
    }
#endif

    btn = button_read();

    /* Find out if a key has been released */
    diff = btn ^ lastbtn;
    if(diff && (btn & diff) == 0)
    {
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
        if(diff & BUTTON_REMOTE)
            if(!skip_remote_release)
                queue_post(&button_queue, BUTTON_REL | diff, 0);
            else
                skip_remote_release = false;
        else
#endif
            if(!skip_release)
                queue_post(&button_queue, BUTTON_REL | diff, 0);
            else
                skip_release = false;
#else
        queue_post(&button_queue, BUTTON_REL | diff, 0);
#endif
    }
    else
    {
        if ( btn )
        {
            /* normal keypress */
            if ( btn != lastbtn )
            {
                post = true;
                repeat = false;
                repeat_speed = REPEAT_INTERVAL_START;
            }
            else /* repeat? */
            {
                if ( repeat )
                {
                    if (!post)
                        count--;
                    if (count == 0) {
                        post = true;
                        /* yes we have repeat */
                        if (repeat_speed > REPEAT_INTERVAL_FINISH)
                            repeat_speed--;
                        count = repeat_speed;

                        repeat_count++;

                        /* Send a SYS_POWEROFF event if we have a device
                           which doesn't shut down easily with the OFF
                           key */
#ifdef HAVE_SW_POWEROFF
                        if ((btn == POWEROFF_BUTTON
#ifdef RC_POWEROFF_BUTTON
                                    || btn == RC_POWEROFF_BUTTON
#endif
                                    ) &&
#if defined(CONFIG_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)
                                !charger_inserted() &&
#endif
                                repeat_count > POWEROFF_COUNT)
                        {
                            /* Tell the main thread that it's time to
                               power off */
                            sys_poweroff();

                            /* Safety net for players without hardware
                               poweroff */
                            if(repeat_count > POWEROFF_COUNT * 10)
                                power_off();
                        }
#endif
                    }
                }
                else
                {
                    if (count++ > REPEAT_START)
                    {
                        post = true;
                        repeat = true;
                        repeat_count = 0;
                        /* initial repeat */
                        count = REPEAT_INTERVAL_START;
                    }
                }
            }
            if ( post )
            {
                if (repeat)
                {
                    /* Only post repeat events if the queue is empty,
                     * to avoid afterscroll effects. */
                    if (queue_empty(&button_queue))
                    {
                        queue_post(&button_queue, BUTTON_REPEAT | btn, 0);
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                        skip_remote_release = false;
#endif
                        skip_release = false;
#endif
                        post = false;
                    }
                }
                else
                {
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                    if (btn & BUTTON_REMOTE) {
                        if (!remote_filter_first_keypress || is_remote_backlight_on()
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
                           || (remote_type()==REMOTETYPE_H300_NONLCD)
#endif
                            )
                            queue_post(&button_queue, btn, 0);
                        else
                            skip_remote_release = true;
                    }
                    else
#endif
                        if (!filter_first_keypress || is_backlight_on())
                            queue_post(&button_queue, btn, 0);
                        else
                            skip_release = true;
#else /* no backlight, nothing to skip */
                    queue_post(&button_queue, btn, 0);
#endif
                    post = false;
                }
#ifdef HAVE_REMOTE_LCD
                if(btn & BUTTON_REMOTE)
                    remote_backlight_on();
                else
#endif
                    backlight_on();

                reset_poweroff_timer();
            }
        }
        else
        {
            repeat = false;
            count = 0;
        }
    }
    lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
}

long button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) )
    {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

long button_get_w_tmo(int ticks)
{
    struct event ev;
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    return (ev.id != SYS_TIMEOUT)? ev.id: BUTTON_NONE;
}

void button_init(void)
{
    /* hardware inits */
    button_init_device();

    queue_init(&button_queue, true);
    button_read();
    lastbtn = button_read();
    tick_add_task(button_tick);
    reset_poweroff_timer();

#ifdef HAVE_LCD_BITMAP
    flipped = false;
#endif
#ifdef CONFIG_BACKLIGHT
    filter_first_keypress = false;
#ifdef HAVE_REMOTE_LCD
    remote_filter_first_keypress = false;
#endif    
#endif
}

#ifdef HAVE_LCD_BITMAP /* only bitmap displays can be flipped */
/*
 * helper function to swap LEFT/RIGHT, UP/DOWN (if present), and F1/F3 (Recorder)
 */
static int button_flip(int button)
{
    int newbutton;

    newbutton = button &
        ~(BUTTON_LEFT | BUTTON_RIGHT
#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
        | BUTTON_UP | BUTTON_DOWN
#endif
#if defined(BUTTON_SCROLL_UP) && defined(BUTTON_SCROLL_DOWN)
        | BUTTON_SCROLL_UP | BUTTON_SCROLL_DOWN
#endif
#if CONFIG_KEYPAD == RECORDER_PAD
        | BUTTON_F1 | BUTTON_F3
#endif
        );

    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
#if defined(BUTTON_UP) && defined(BUTTON_DOWN)
    if (button & BUTTON_UP)
        newbutton |= BUTTON_DOWN;
    if (button & BUTTON_DOWN)
        newbutton |= BUTTON_UP;
#endif
#if defined(BUTTON_SCROLL_UP) && defined(BUTTON_SCROLL_DOWN)
    if (button & BUTTON_SCROLL_UP)
        newbutton |= BUTTON_SCROLL_DOWN;
    if (button & BUTTON_SCROLL_DOWN)
        newbutton |= BUTTON_SCROLL_UP;
#endif
#if CONFIG_KEYPAD == RECORDER_PAD
    if (button & BUTTON_F1)
        newbutton |= BUTTON_F3;
    if (button & BUTTON_F3)
        newbutton |= BUTTON_F1;
#endif

    return newbutton;
}

/*
 * set the flip attribute
 * better only call this when the queue is empty
 */
void button_set_flip(bool flip)
{
    if (flip != flipped) /* not the current setting */
    {
        /* avoid race condition with the button_tick() */
        int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
        lastbtn = button_flip(lastbtn);
        flipped = flip;
        set_irq_level(oldlevel);
    }
}
#endif /* HAVE_LCD_BITMAP */

#ifdef CONFIG_BACKLIGHT
void set_backlight_filter_keypress(bool value)
{
    filter_first_keypress = value;
}
#ifdef HAVE_REMOTE_LCD
void set_remote_backlight_filter_keypress(bool value)
{
    remote_filter_first_keypress = value;
}
#endif
#endif

/*
 * Get button pressed from hardware
 */
static int button_read(void)
{
    int btn = button_read_device();
    int retval;

#ifdef HAVE_LCD_BITMAP
    if (btn && flipped)
        btn = button_flip(btn); /* swap upside down */
#endif

    /* Filter the button status. It is only accepted if we get the same
       status twice in a row. */
    if (btn != last_read)
        retval = lastbtn;
    else
        retval = btn;
    last_read = btn;

    return retval;
}


int button_status(void)
{
    return lastbtn;
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}
