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
 * Archos Jukebox Recorder button functions
 */

#include <stdlib.h>
#include "config.h"
#include "sh7034.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"

struct event_queue button_queue;

#define POLL_FREQUENCY    HZ/20
#define REPEAT_START      6
#define REPEAT_INTERVAL   4

static int repeat_mask = DEFAULT_REPEAT_MASK;
static int release_mask = DEFAULT_RELEASE_MASK;

static int button_read(void);

static void button_tick(void)
{
    static int tick=0;
    static int count=0;
    static int lastbtn=0;
    static bool repeat=false;
    int diff;

    /* only poll every X ticks */
    if ( ++tick >= POLL_FREQUENCY )
    {
        bool post = false;
        int btn = button_read();
        
        /* Find out if a key has been released */
        diff = btn ^ lastbtn;
        if((btn & diff) == 0)
        {
            if(diff & release_mask)
                queue_post(&button_queue, BUTTON_REL | diff, NULL);
        }
        
        if ( btn )
        {
            /* normal keypress */
            if ( btn != lastbtn )
            {
                post = true;
                repeat = false;
            }
            else /* repeat? */
            {
                if ( repeat )
                {
                    if ( ! --count ) {
                        post = true;
                        count = REPEAT_INTERVAL;
                    }
                }
                else
                {
                    if(btn & repeat_mask ||
#ifdef HAVE_RECORDER_KEYPAD
                     btn == BUTTON_OFF)
#else
                     btn == BUTTON_STOP)
#endif
                    {
                        if (count++ > REPEAT_START)
                        {
                            /* Only repeat if a repeatable key is pressed */
                            if(btn & repeat_mask)
                            {
                                post = true;
                                repeat = true;
                                count = REPEAT_INTERVAL;
                            }
                            /* If the OFF button is pressed long enough,
                               and we are still alive, then the unit must be
                               connected to a charger. Therefore we will
                               reboot and let the original firmware handle
                               the charging. */
#ifdef HAVE_RECORDER_KEYPAD
                            if(btn == BUTTON_OFF)
#elif HAVE_PLAYER_KEYPAD
                            if(btn == BUTTON_STOP)
#endif
                                system_reboot();
                        }
                    }
                    else
                    {
                        count = 0;
                    }
                }
            }
            if ( post )
            {
                if(repeat)
                    queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
                else
                    queue_post(&button_queue, btn, NULL);
                backlight_on();
            }
        }
        else
        {
            repeat = false;
            count = 0;
        }
            
        lastbtn = btn & ~(BUTTON_REL | BUTTON_REPEAT);
        tick = 0;
    }
        
    backlight_tick();
}

int button_get(bool block)
{
    struct event ev;

    if ( block || !queue_empty(&button_queue) ) {
        queue_wait(&button_queue, &ev);
        return ev.id;
    }
    return BUTTON_NONE;
}

int button_get_w_tmo(int ticks)
{
    struct event ev;
    unsigned int timeout = current_tick + ticks;

    while (TIME_BEFORE( current_tick, timeout ))
    {
        if(!queue_empty(&button_queue))
        {
            queue_wait(&button_queue, &ev);
            return ev.id;
        }
        yield();
    }

    return BUTTON_NONE;
}

int button_set_repeat(int newmask)
{
    int oldmask = repeat_mask;
    repeat_mask = newmask;
    return oldmask;
}

int button_set_release(int newmask)
{
    int oldmask = release_mask;
    release_mask = newmask;
    return oldmask;
}

#ifdef HAVE_RECORDER_KEYPAD

/* AJBR buttons are connected to the CPU as follows:
 *
 * ON and OFF are connected to separate port B input pins.
 *
 * F1, F2, F3, and UP are connected to the AN4 analog input, each through
 * a separate voltage divider.  The voltage on AN4 depends on which button
 * (or none, or a combination) is pressed.
 *
 * DOWN, PLAY, LEFT, and RIGHT are likewise connected to AN5. */
 
/* Button analog voltage levels */
#define	LEVEL1		250
#define	LEVEL2		450
#define	LEVEL3		650
#define	LEVEL4		850

/*
 *Initialize buttons
 */
void button_init()
{
#ifndef SIMULATOR
    /* Set PB4 and PB8 as input pins */
    PBCR1 &= 0xfffc; /* PB8MD = 00 */
    PBCR2 &= 0xfcff; /* PB4MD = 00 */
    PBIOR &= ~(PBDR_BTN_ON|PBDR_BTN_OFF); /* Inputs */
#endif
    queue_init(&button_queue);
    tick_add_task(button_tick);
}

/*
 * Get button pressed from hardware
 */
static int button_read(void)
{
    int btn = BUTTON_NONE;

    /* Check port B pins for ON and OFF */
    int data = PBDR;
    if ((data & PBDR_BTN_ON) == 0)
        btn |= BUTTON_ON;
    else if ((data & PBDR_BTN_OFF) == 0)
        btn |= BUTTON_OFF;

    /* Check F1-3 and UP */
    data = adc_read(ADC_BUTTON_ROW1);
    if (data >= LEVEL4)
        btn |= BUTTON_F3;
    else if (data >= LEVEL3)
        btn |= BUTTON_UP;
    else if (data >= LEVEL2)
        btn |= BUTTON_F2;
    else if (data >= LEVEL1)
        btn |= BUTTON_F1;

    /* Check DOWN, PLAY, LEFT, RIGHT */
    data = adc_read(ADC_BUTTON_ROW2);
    if (data >= LEVEL4)
        btn |= BUTTON_DOWN;
    else if (data >= LEVEL3)
        btn |= BUTTON_PLAY;
    else if (data >= LEVEL2)
        btn |= BUTTON_LEFT;
    else if (data >= LEVEL1)
        btn |= BUTTON_RIGHT;
  
    return btn;
}

#elif HAVE_PLAYER_KEYPAD

/* The player has two buttons on port pins:

   STOP:  PA11
   ON:    PA5

   The rest are on analog inputs:
   
   LEFT:  AN0
   MENU:  AN1
   RIGHT: AN2
   PLAY:  AN3
*/

void button_init(void)
{
    /* set port pins as input */
    PAIOR &= ~0x820;
    queue_init(&button_queue);
    tick_add_task(button_tick);
}

static int button_read(void)
{
    int porta = PADR;
    int btn = BUTTON_NONE;

    /* buttons are active low */
    if(adc_read(0) < 0x180)
        btn |= BUTTON_LEFT;
    if(adc_read(1) < 0x180)
        btn |= BUTTON_MENU;
    if(adc_read(2) < 0x180)
        btn |= BUTTON_RIGHT;
    if(adc_read(3) < 0x180)
        btn |= BUTTON_PLAY | BUTTON_UP;

    if ( !(porta & 0x20) )
        btn |= BUTTON_ON;
    if ( !(porta & 0x800) )
        btn |= BUTTON_STOP | BUTTON_DOWN;

    return btn;
}

#endif



/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
