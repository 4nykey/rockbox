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
#include "serial.h"
#include "power.h"
#include "system.h"

struct event_queue button_queue;

long last_keypress;
static int lastbtn;
#ifdef HAVE_RECORDER_KEYPAD
static bool flipped; /* bottons can be flipped to match the LCD flip */
#endif

/* how often we check to see if a button is pressed */
#define POLL_FREQUENCY    HZ/20

/* how long until repeat kicks in */
#define REPEAT_START      6

/* the speed repeat starts at */
#define REPEAT_INTERVAL_START   4

/* speed repeat finishes at */
#define REPEAT_INTERVAL_FINISH  2

/* Number of repeated keys before shutting off */
#define POWEROFF_COUNT 8

static int button_read(void);

static void button_tick(void)
{
    static int tick = 0;
    static int count = 0;
    static int repeat_speed = REPEAT_INTERVAL_START;
    static int repeat_count = 0;
    static bool repeat = false;
    int diff;
    int btn;

    /* Post events for the remote control */
    btn = remote_control_rx();
    if(btn)
    {
        queue_post(&button_queue, btn, NULL);
        backlight_on();
    }   
    
    /* only poll every X ticks */
    if ( ++tick >= POLL_FREQUENCY )
    {
        bool post = false;
        btn = button_read();
        
        /* Find out if a key has been released */
        diff = btn ^ lastbtn;
        if(diff && (btn & diff) == 0)
        {
            queue_post(&button_queue, BUTTON_REL | diff, NULL);
        }
        
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
                    count--;
                    if (count == 0) {
                        post = true;
                        /* yes we have repeat */
                        repeat_speed--;
                        if (repeat_speed < REPEAT_INTERVAL_FINISH)
                           repeat_speed = REPEAT_INTERVAL_FINISH;
                        count = repeat_speed;

                        repeat_count++;
                        
                        /* Shutdown if we have a device which doesn't shut
                           down easily with the OFF key */
#ifdef HAVE_POWEROFF_ON_PB5
                        if(btn == BUTTON_OFF && !charger_inserted() &&
                            repeat_count > POWEROFF_COUNT)
                            power_off();
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
                if(repeat)
                    queue_post(&button_queue, BUTTON_REPEAT | btn, NULL);
                else
                    queue_post(&button_queue, btn, NULL);
                backlight_on();

                last_keypress = current_tick;
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
    queue_wait_w_tmo(&button_queue, &ev, ticks);
    return (ev.id != SYS_TIMEOUT)? ev.id: BUTTON_NONE;
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
#ifdef HAVE_FMADC
/* FM Recorder super-special levels */
#define	LEVEL1		150
#define	LEVEL2		385
#define	LEVEL3		545
#define	LEVEL4		700
#else
/* plain bog standard Recorder levels */
#define	LEVEL1		250
#define	LEVEL2		500
#define	LEVEL3		700
#define	LEVEL4		900
#endif

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
    lastbtn = 0;
    tick_add_task(button_tick);
    last_keypress = current_tick;
    flipped = false;
}


/*
 * helper function to swap UP/DOWN, LEFT/RIGHT, F1/F3
 */
static int button_flip(int button)
{
    int newbutton;

    newbutton = button & 
        ~(BUTTON_UP | BUTTON_DOWN
        | BUTTON_LEFT | BUTTON_RIGHT 
        | BUTTON_F1 | BUTTON_F3);

    if (button & BUTTON_UP)
        newbutton |= BUTTON_DOWN;
    if (button & BUTTON_DOWN)
        newbutton |= BUTTON_UP;
    if (button & BUTTON_LEFT)
        newbutton |= BUTTON_RIGHT;
    if (button & BUTTON_RIGHT)
        newbutton |= BUTTON_LEFT;
    if (button & BUTTON_F1)
        newbutton |= BUTTON_F3;
    if (button & BUTTON_F3)
        newbutton |= BUTTON_F1;

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


/*
 * Get button pressed from hardware
 */
static int button_read(void)
{
    int btn = BUTTON_NONE;

    /* Check port B pins for ON and OFF */
    int data;

#ifdef HAVE_FMADC
    /* TODO: use proper defines here, and not the numerics in the
       function argument */
    if ( adc_read(3) < 512 )
        btn |= BUTTON_ON;
    if ( adc_read(2) > 512 )
        btn |= BUTTON_OFF;
#else
    data = PBDR;
    if ((data & PBDR_BTN_ON) == 0)
        btn |= BUTTON_ON;
    else if ((data & PBDR_BTN_OFF) == 0)
        btn |= BUTTON_OFF;
#endif

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

    /* Some units have mushy keypads, so pressing UP also activates
       the Left/Right buttons. Let's combat that by skipping the AN5
       checks when UP is pressed. */
    if(!(btn & BUTTON_UP))
    {
        /* Check DOWN, PLAY, LEFT, RIGHT */
        data = adc_read(ADC_BUTTON_ROW2);
        if (data >= LEVEL4)
            btn |= BUTTON_DOWN;
        else if (data >= LEVEL3) {
#ifdef HAVE_FMADC
            btn |= BUTTON_RIGHT;
#else
            btn |= BUTTON_PLAY;
#endif
        }
        else if (data >= LEVEL2)
            btn |= BUTTON_LEFT;
        else if (data >= LEVEL1) {
#ifdef HAVE_FMADC
            btn |= BUTTON_PLAY;
#else
            btn |= BUTTON_RIGHT;
#endif
        }
    }

    if (btn && flipped)
        return button_flip(btn); /* swap upside down */
  
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
    lastbtn = 0;
    tick_add_task(button_tick);

    last_keypress = current_tick;
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

#elif HAVE_NEO_KEYPAD
static bool mStation = false;
void button_init(void)
{
    /* set port pins as input */
    PAIOR &= ~0x4000;  //PA14 for stop button

    queue_init(&button_queue);
    lastbtn = 0;
    tick_add_task(button_tick);

    last_keypress = current_tick;
}
int button_read(void)
{
    int btn=BUTTON_NONE;
    
    btn|=((~PCDR)&0xFF);

    /* mStation does not have a stop button and this floods the button queue
       with stops if used on a mStation */
    if (!mStation)
        btn|=((~(PADR>>6))&0x100);

    return btn;
}

/* This function adds a button press event to the button queue, and this
   really isn't anything Neo-specific but might be subject for adding to
   the generic button driver */
int button_add(unsigned int button)
{
    queue_post(&button_queue,button,NULL);
    return 1;
}
#endif
