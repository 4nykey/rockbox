/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "uisdl.h"
#include "lcd-charcell.h"
#include "lcd-remote.h"
#include "config.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "misc.h"

#include "debug.h"
/* how long until repeat kicks in */
#define REPEAT_START      6

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

#if defined(IRIVER_H100_SERIES) || defined (IRIVER_H300_SERIES)
int _remote_type=REMOTETYPE_H100_LCD;

int remote_type(void)
{
    return _remote_type;
}
#endif

struct event_queue button_queue;

static int btn = 0;    /* Hopefully keeps track of currently pressed keys... */

#ifdef CONFIG_BACKLIGHT
static bool filter_first_keypress;

void set_backlight_filter_keypress(bool value)
{
    filter_first_keypress = value;
}
#ifdef HAVE_REMOTE_LCD
static bool remote_filter_first_keypress;

void set_remote_backlight_filter_keypress(bool value)
{
    remote_filter_first_keypress = value;
}
#endif
#endif

void button_event(int key, bool pressed)
{
    int new_btn = 0;
    int diff = 0;
    static int count = 0;
    static int lastbtn;
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

    switch (key)
    {

#if CONFIG_KEYPAD == GIGABEAT_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_A;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_BACK;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_FWD;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_ESCAPE:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REW;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_FF;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    case SDLK_t:
        if(pressed)
            switch(_remote_type)
            {
                case REMOTETYPE_UNPLUGGED: 
                    _remote_type=REMOTETYPE_H100_LCD;
                    DEBUGF("Changed remote type to H100\n");
                    break;
                case REMOTETYPE_H100_LCD:
                    _remote_type=REMOTETYPE_H300_LCD;
                    DEBUGF("Changed remote type to H300\n");
                    break;
                case REMOTETYPE_H300_LCD:
                    _remote_type=REMOTETYPE_H300_NONLCD;
                    DEBUGF("Changed remote type to H300 NON-LCD\n");
                    break;
                case REMOTETYPE_H300_NONLCD:
                    _remote_type=REMOTETYPE_UNPLUGGED;
                    DEBUGF("Changed remote type to none\n");
                    break;
            }
        break;
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MODE;
        break;

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
       new_btn = BUTTON_EQ;
       break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MODE;
        break;

#elif CONFIG_KEYPAD == ONDIO_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == PLAYER_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_PLAY;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_STOP;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_PERIOD:
    case SDLK_INSERT:
        new_btn = BUTTON_MENU;
        break;

#elif CONFIG_KEYPAD == RECORDER_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_F1;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_F2;
        break;
    case SDLK_KP_MINUS:
    case SDLK_F3:
        new_btn = BUTTON_F3;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_PLAY;
        break;

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP_PLUS:
    case SDLK_F8:
        new_btn = BUTTON_ON;
        break;
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_a:
        new_btn = BUTTON_OFF;
        break;
    case SDLK_KP_DIVIDE:
    case SDLK_F1:
        new_btn = BUTTON_F1;
        break;
    case SDLK_KP_MULTIPLY:
    case SDLK_F2:
        new_btn = BUTTON_F2;
        break;
    case SDLK_KP_MINUS:
    case SDLK_F3:
        new_btn = BUTTON_F3;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#elif CONFIG_KEYPAD == SANSA_E200_PAD
    case SDLK_KP4:
    case SDLK_LEFT:
        new_btn = BUTTON_LEFT;
        break;
    case SDLK_KP6:
    case SDLK_RIGHT:
        new_btn = BUTTON_RIGHT;
        break;
    case SDLK_KP8:
    case SDLK_UP:
        new_btn = BUTTON_SCROLL_UP;
        break;
    case SDLK_KP2:
    case SDLK_DOWN:
        new_btn = BUTTON_SCROLL_DOWN;
        break;
    case SDLK_KP9:
        new_btn = BUTTON_UP;
        break;
    case SDLK_KP3:
        new_btn = BUTTON_DOWN;
        break;
    case SDLK_KP1:
        new_btn = BUTTON_POWER;
        break;
    case SDLK_KP7:
        new_btn = BUTTON_REC;
        break;
    case SDLK_KP5:
    case SDLK_SPACE:
        new_btn = BUTTON_SELECT;
        break;

#endif /* CONFIG_KEYPAD */
    case SDLK_KP0:
    case SDLK_F5:
        if(pressed)
        {
            screen_dump();
            return;
        }
        break;
    }
    
    if (pressed)
        btn |= new_btn;
    else
        btn &= ~new_btn;

    /* Lots of stuff copied from real button.c. Not good, I think... */

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
                    if (count == 0)
                    {
                        post = true;
                        /* yes we have repeat */
                        repeat_speed--;
                        if (repeat_speed < REPEAT_INTERVAL_FINISH)
                            repeat_speed = REPEAT_INTERVAL_FINISH;
                        count = repeat_speed;

                        repeat_count++;
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
                if(repeat)
                {
                    if (queue_empty(&button_queue))
                    {
                        queue_post(&button_queue, BUTTON_REPEAT | btn, 0);
#ifdef CONFIG_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
                            if(btn & BUTTON_REMOTE)
                            {
                                if(skip_remote_release)
                                    skip_remote_release = false;
                            }
                            else
#endif                                    
                                if(skip_release)
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
                            if (!remote_filter_first_keypress || is_remote_backlight_on())
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

/* Again copied from real button.c... */

long button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) ) {
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
}

int button_status(void)
{
    return btn;
}

void button_clear_queue(void)
{
    queue_clear(&button_queue);
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void) {
    /* temp fix for hold button on irivers */
    return false;
}
#endif

#ifdef HAS_REMOTE_BUTTON_HOLD
bool remote_button_hold(void) {
    /* temp fix for hold button on irivers */
    return false;
}
#endif

