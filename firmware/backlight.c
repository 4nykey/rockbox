/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"
#include "power.h"
#include "system.h"
#include "button.h"
#include "timer.h"
#include "backlight.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
#include "pcf50606.h" /* iRiver, iAudio X5 brightness */
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
#include "lcd.h" /* for lcd_enable() and lcd_sleep() */
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#if defined(TARGET_TREE) && !defined(SIMULATOR)
#include "backlight-target.h"
#endif

#ifdef SIMULATOR
static inline void __backlight_on(void)
{
    sim_backlight(100);
}

static inline void __backlight_off(void)
{
    sim_backlight(0);
}
#else
/* Basic low-level code that simply switches backlight on or off. Probably
 * a nice candidate for inclusion in the target/ dir. */
#ifndef TARGET_TREE
static inline void __backlight_on(void)
{
#if CONFIG_BACKLIGHT == BL_IRIVER_H100
    and_l(~0x00020000, &GPIO1_OUT);    
#elif CONFIG_BACKLIGHT == BL_IRIVER_H300
    lcd_enable(true);
    sleep(HZ/100); /* lcd needs time - avoid flashing for dark screens */
    or_l(0x00020000, &GPIO1_OUT);
#elif CONFIG_BACKLIGHT == BL_RTC
    /* Enable square wave */
    rtc_write(0x0a, rtc_read(0x0a) | 0x40);
#elif CONFIG_BACKLIGHT == BL_PA14_LO /* Player */
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH);
#elif CONFIG_BACKLIGHT == BL_PA14_HI /* Ondio */
    or_b(0x40, &PADRH); /* drive it high */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1 |= 0x10;
#elif CONFIG_BACKLIGHT == BL_IPOD4G
    /* brightness full */
    outl(0x80000000 | (0xff << 16), 0x7000a010);

    /* set port b bit 3 on */
    outl(((0x100 | 1) << 3), 0x6000d824);
#elif CONFIG_BACKLIGHT==BL_IPODMINI
    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);
#elif CONFIG_BACKLIGHT==BL_IPODNANO
    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

    /* set port L07 on */
    outl(((0x100 | 1) << 7), 0x6000d12c);
#elif CONFIG_BACKLIGHT==BL_IPOD3G
    outl(inl(0xc0001000) | 0x02, 0xc0001000);
#elif CONFIG_BACKLIGHT==BL_IRIVER_IFP7XX
    GPIO3_SET = 1;
#endif
}

static inline void __backlight_off(void)
{
#if CONFIG_BACKLIGHT == BL_IRIVER_H100
    or_l(0x00020000, &GPIO1_OUT);
#elif CONFIG_BACKLIGHT == BL_IRIVER_H300
    and_l(~0x00020000, &GPIO1_OUT);
    lcd_enable(false);
#elif CONFIG_BACKLIGHT == BL_RTC
    /* Disable square wave */
    rtc_write(0x0a, rtc_read(0x0a) & ~0x40);
#elif CONFIG_BACKLIGHT == BL_PA14_LO /* Player */
    and_b(~0x40, &PAIORH); /* let it float (up) */
#elif CONFIG_BACKLIGHT == BL_PA14_HI /* Ondio */
    and_b(~0x40, &PADRH); /* drive it low */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1 &= ~0x10;
#elif CONFIG_BACKLIGHT == BL_IPOD4G
   /* fades backlight off on 4g */
   outl(inl(0x70000084) & ~0x2000000, 0x70000084);
   outl(0x80000000, 0x7000a010);
#elif CONFIG_BACKLIGHT==BL_IPODNANO
    /* set port B03 off */
    outl(((0x100 | 0) << 3), 0x6000d824);

    /* set port L07 off */
    outl(((0x100 | 0) << 7), 0x6000d12c);
#elif CONFIG_BACKLIGHT==BL_IRIVER_IFP7XX
    GPIO3_CLR = 1;
#elif CONFIG_BACKLIGHT==BL_IPOD3G
    outl(inl(0xc0001000) & ~0x02, 0xc0001000);
#elif CONFIG_BACKLIGHT==BL_IPODMINI
    /* set port B03 off */
    outl(((0x100 | 0) << 3), 0x6000d824);
#endif
}
#endif
#endif /* SIMULATOR */

#if defined(CONFIG_BACKLIGHT) && !defined(BOOTLOADER)

const signed char backlight_timeout_value[19] =
{
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60, 90
};

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 2
#define REMOTE_BACKLIGHT_ON 3
#define REMOTE_BACKLIGHT_OFF 4
#define BACKLIGHT_UNBOOST_CPU 5
#ifdef HAVE_LCD_SLEEP
#define LCD_SLEEP 6
#endif

static void backlight_thread(void);
static long backlight_stack[DEFAULT_STACK_SIZE/sizeof(long)];
#ifdef X5_BACKLIGHT_SHUTDOWN
#define BACKLIGHT_QUIT 256
/* Need to save this for x5 shutdown */
static int backlight_thread_id;
#endif
static const char backlight_thread_name[] = "backlight";
static struct event_queue backlight_queue;

static int backlight_timer;
static int backlight_timeout = 5*HZ;
#ifdef CONFIG_CHARGING
static int backlight_timeout_plugged = 5*HZ;
#endif
#ifdef HAS_BUTTON_HOLD
static int backlight_on_button_hold = 0;
#endif

#ifdef HAVE_REMOTE_LCD
static int remote_backlight_timer;
static int remote_backlight_timeout = 5*HZ;
#ifdef CONFIG_CHARGING
static int remote_backlight_timeout_plugged = 5*HZ;
#endif
#endif

#ifdef HAVE_LCD_SLEEP
const signed char lcd_sleep_timeout_value[10] =
{
    -1, 0, 5, 10, 15, 20, 30, 45, 60, 90
};
static int lcd_sleep_timer;
static int lcd_sleep_timeout = 10*HZ;
#endif

#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
/* backlight fading */
#define BL_PWM_INTERVAL 5000  /* Cycle interval in �s */
#define BL_PWM_COUNT    100
static const char backlight_fade_value[8] = { 0, 1, 2, 4, 6, 8, 10, 20 };
static int fade_in_count = 1;
static int fade_out_count = 4;

static bool bl_timer_active = false;
static int bl_dim_current = BL_PWM_COUNT;
static int bl_dim_target  = BL_PWM_COUNT;
static int bl_pwm_counter = 0;
static volatile int bl_cycle_counter = 0;
static enum {DIM_STATE_START, DIM_STATE_MAIN} bl_dim_state = DIM_STATE_START;

static void backlight_isr(void)
{
    int timer_period;
    bool idle = false;
    
    timer_period = TIMER_FREQ / 1000 * BL_PWM_INTERVAL / 1000;
    switch (bl_dim_state) 
    {
      /* New cycle */
      case DIM_STATE_START:
        bl_pwm_counter = 0;
        bl_cycle_counter++;
        
        if (bl_dim_current > 0 && bl_dim_current < BL_PWM_COUNT)
        {
            __backlight_on();
            bl_pwm_counter = bl_dim_current;
            timer_period = timer_period * bl_pwm_counter / BL_PWM_COUNT;
            bl_dim_state = DIM_STATE_MAIN;
        } 
        else
        {
            if (bl_dim_current)
                __backlight_on();
            else
                __backlight_off();
            if (bl_dim_current == bl_dim_target)
                idle = true;
        }
        
        break ;
        
      /* Dim main screen */
      case DIM_STATE_MAIN:
        __backlight_off();
        bl_dim_state = DIM_STATE_START;
        timer_period = timer_period * (BL_PWM_COUNT - bl_pwm_counter) / BL_PWM_COUNT;
        break ;
    }

    if ((bl_dim_target > bl_dim_current) && (bl_cycle_counter >= fade_in_count))
    {
        bl_dim_current++;
        bl_cycle_counter = 0;
    }

    if ((bl_dim_target < bl_dim_current) && (bl_cycle_counter >= fade_out_count))
    {
        bl_dim_current--;
        bl_cycle_counter = 0;
    }

    if (idle) 
    {
#ifdef CPU_COLDFIRE
        queue_post(&backlight_queue, BACKLIGHT_UNBOOST_CPU, NULL);
#endif
        timer_unregister();
        bl_timer_active = false;
    }
    else
        timer_set_period(timer_period);
}

static void backlight_switch(void)
{
    if (bl_dim_target > (BL_PWM_COUNT/2))
    {
        __backlight_on();
        bl_dim_current = BL_PWM_COUNT;
    }
    else
    {
        __backlight_off();
        bl_dim_current = 0;
    }
}

static void backlight_release_timer(void)
{
#ifdef CPU_COLDFIRE
    cpu_boost(false);
#endif
    timer_unregister();
    bl_timer_active = false;
    backlight_switch();
}

static void backlight_dim(int value)
{
    /* protect from extraneous calls with the same target value */
    if (value == bl_dim_target)
        return;

    bl_dim_target = value;

    if (bl_timer_active)
        return ;

    if (timer_register(0, backlight_release_timer, 2, 0, backlight_isr))
    {
#ifdef CPU_COLDFIRE
        /* Prevent cpu frequency changes while dimming. */
        cpu_boost(true);
#endif
        bl_timer_active = true;
    }
    else
        backlight_switch();
}

void backlight_set_fade_in(int index)
{
    fade_in_count = backlight_fade_value[index];
}

void backlight_set_fade_out(int index)
{
    fade_out_count = backlight_fade_value[index];
}
#endif /* defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR) */

static void _backlight_on(void)
{
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    if (fade_in_count > 0)
        backlight_dim(BL_PWM_COUNT);
    else
    {
        bl_dim_target = bl_dim_current = BL_PWM_COUNT;
        __backlight_on();
    }
#else
    __backlight_on();
#endif
#ifdef HAVE_LCD_SLEEP
    lcd_sleep_timer = 0; /* LCD should be awake already */
#endif
}

static void _backlight_off(void)
{
#if defined(HAVE_BACKLIGHT_PWM_FADING) && !defined(SIMULATOR)
    if (fade_out_count > 0)
        backlight_dim(0);
    else
    {
        bl_dim_target = bl_dim_current = 0;
        __backlight_off();
    }
#else
    __backlight_off();
#endif

#ifdef HAVE_LCD_SLEEP
    /* Start LCD sleep countdown */
    if (lcd_sleep_timeout < 0)
    {
        lcd_sleep_timer = 0; /* Setting == Always */
        lcd_sleep();
    }
    else
        lcd_sleep_timer = lcd_sleep_timeout;
#endif
}

#ifdef HAVE_REMOTE_LCD
#ifdef SIMULATOR
static void __remote_backlight_on(void)
{
    sim_remote_backlight(100);
}

static void __remote_backlight_off(void)
{
    sim_remote_backlight(0);
}
#else
#ifndef TARGET_TREE
static void __remote_backlight_on(void)
{
#if defined(IRIVER_H300_SERIES)
    and_l(~0x00000002, &GPIO1_OUT);
#else
    and_l(~0x00000800, &GPIO_OUT);
#endif
}

static void __remote_backlight_off(void)
{
#if defined(IRIVER_H300_SERIES)
    or_l(0x00000002, &GPIO1_OUT);
#else
    or_l(0x00000800, &GPIO_OUT);
#endif
}
#endif /* TARGET_TREE */
#endif /* SIMULATOR */
#endif /* HAVE_REMOTE_LCD */

/* Update state of backlight according to timeout setting */
static void backlight_update_state(void)
{
#ifdef CONFIG_CHARGING
    if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
            )
        backlight_timer = backlight_timeout_plugged;
    else
#endif
        backlight_timer = backlight_timeout;

    /* Backlight == OFF in the setting? */
    if (backlight_timer < 0)
    {
        backlight_timer = 0; /* Disable the timeout */
#ifdef HAS_BUTTON_HOLD
        if (backlight_on_button_hold == 2 && button_hold())
            return; /* Keep on if "On" */
#endif
        _backlight_off();
    }
    else
    {
#ifdef HAS_BUTTON_HOLD
        if (backlight_on_button_hold == 1 && button_hold())
        {
            /* Keep off if "Off". */
            backlight_timer = 0; /* Disable the timeout */
            return;
        }
#endif
        _backlight_on();
    }
}

#ifdef HAVE_REMOTE_LCD
/* Update state of remote backlight according to timeout setting */
static void remote_backlight_update_state(void)
{
#ifdef CONFIG_CHARGING
    if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
            )
        remote_backlight_timer = remote_backlight_timeout_plugged;
    else
#endif
        remote_backlight_timer = remote_backlight_timeout;

    /* Backlight == OFF in the setting? */
    if (remote_backlight_timer < 0)
    {
        remote_backlight_timer = 0; /* Disable the timeout */
        __remote_backlight_off();
    }
    else
    {
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        if (remote_type() == REMOTETYPE_H300_NONLCD)
        {
            backlight_update_state();
        }
        else
#endif 
            __remote_backlight_on();
    }
}
#endif /* HAVE_REMOTE_LCD */

void backlight_thread(void)
{
    struct event ev;

    while(1)
    {
        queue_wait(&backlight_queue, &ev);
        switch(ev.id)
        {
#ifdef HAVE_REMOTE_LCD
            case REMOTE_BACKLIGHT_ON:
                remote_backlight_update_state();
                break;

            case REMOTE_BACKLIGHT_OFF:
                remote_backlight_timer = 0; /* Disable the timeout */
                __remote_backlight_off();
                break;
#endif
            case BACKLIGHT_ON:
                backlight_update_state();
                break;

            case BACKLIGHT_OFF:
                backlight_timer = 0; /* Disable the timeout */
#ifdef HAS_BUTTON_HOLD
                if (backlight_on_button_hold == 2 && button_hold())
                    break; /* Keep on if "On" */
#endif
                _backlight_off();
                break;

#ifdef HAVE_LCD_SLEEP
            case LCD_SLEEP:
                lcd_sleep();
                break;
#endif

#ifdef X5_BACKLIGHT_SHUTDOWN
            case BACKLIGHT_QUIT:
                remove_thread(backlight_thread_id);
                break;
#endif

#if defined(HAVE_BACKLIGHT_PWM_FADING) && defined(CPU_COLDFIRE) \
    && !defined(SIMULATOR)
            case BACKLIGHT_UNBOOST_CPU:
                cpu_boost(false);
                break;
#endif

            case SYS_USB_CONNECTED:
                /* Tell the USB thread that we are safe */
                DEBUGF("backlight_thread got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;

            case SYS_USB_DISCONNECTED:
                usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
                break;
        } /* end switch */
    } /* end while */
}

static void backlight_tick(void)
{
#ifdef CONFIG_CHARGING
    static bool charger_was_inserted = false;
    bool charger_is_inserted = charger_inserted()
#ifdef HAVE_USB_POWER
        || usb_powered()
#endif
        ;

    if( charger_was_inserted != charger_is_inserted )
    {
        backlight_on();
#ifdef HAVE_REMOTE_LCD
        remote_backlight_on();
#endif
    }
    charger_was_inserted = charger_is_inserted;
#endif /* CONFIG_CHARGING */

    if(backlight_timer)
    {
        backlight_timer--;
        if(backlight_timer == 0)
        {
            backlight_off();
        }
    }
#ifdef HAVE_LCD_SLEEP
    else if(lcd_sleep_timer)
    {
        lcd_sleep_timer--;
        if(lcd_sleep_timer == 0)
        {
            /* Queue on bl thread or freeze! */
            queue_post(&backlight_queue, LCD_SLEEP, NULL);
        }
    }
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_REMOTE_LCD
    if(remote_backlight_timer)
    {
        remote_backlight_timer--;
        if(remote_backlight_timer == 0)
        {
            remote_backlight_off();
        }
    }
#endif /* HAVE_REMOVE_LCD */
}

void backlight_init(void)
{
    queue_init(&backlight_queue);
#ifdef X5_BACKLIGHT_SHUTDOWN
    backlight_thread_id =
#endif
    create_thread(backlight_thread, backlight_stack,
                  sizeof(backlight_stack), backlight_thread_name);
    tick_add_task(backlight_tick);
#ifdef SIMULATOR
    /* do nothing */
#elif CONFIG_BACKLIGHT == BL_IRIVER_H100
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    and_l(~0x00020000, &GPIO1_OUT);  /* Start with the backlight ON */
#elif CONFIG_BACKLIGHT == BL_IRIVER_H300
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    or_l(0x00020000, &GPIO1_OUT);  /* Start with the backlight ON */
#elif CONFIG_BACKLIGHT == BL_PA14_LO || CONFIG_BACKLIGHT == BL_PA14_HI
    PACR1 &= ~0x3000;    /* Set PA14 (backlight control) to GPIO */
    or_b(0x40, &PAIORH); /* ..and output */
#elif CONFIG_BACKLIGHT == BL_GMINI
    P1CON |= 0x10; /* P1.4 C-MOS output mode */
#endif
    backlight_on();
#ifdef HAVE_REMOTE_LCD
    remote_backlight_on();
#endif
}

#ifdef X5_BACKLIGHT_SHUTDOWN
void x5_backlight_shutdown(void)
{
    /* Turn on the screen and don't let anyone else mess with it. Called
       from clean_shutdown in misc.c. */
    queue_empty(&backlight_queue);
    tick_remove_task(backlight_tick);
    /* Next time the thread runs, if at all, it will just remove itself. */
    queue_post(&backlight_queue, BACKLIGHT_QUIT, NULL);
    __backlight_on();
}
#endif /* X5_BACKLIGHT_SHUTDOWN */

void backlight_on(void)
{
    queue_post(&backlight_queue, BACKLIGHT_ON, NULL);
}

void backlight_off(void)
{
    queue_post(&backlight_queue, BACKLIGHT_OFF, NULL);
}

/* returns true when the backlight is on OR when it's set to always off */
bool is_backlight_on(void)
{
    if (backlight_timer || backlight_get_current_timeout() <= 0)
        return true;
    else
        return false;
}

/* return value in ticks; 0 means always on, <0 means always off */
int backlight_get_current_timeout(void)
{
#ifdef CONFIG_CHARGING
    if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
        )
        return backlight_timeout_plugged;
    else
        return backlight_timeout;
#else
    return backlight_timeout;
#endif
}

void backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index = 6;
    backlight_timeout = HZ * backlight_timeout_value[index];
    backlight_update_state();
}

#ifdef CONFIG_CHARGING
void backlight_set_timeout_plugged(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index = 6;
    backlight_timeout_plugged = HZ * backlight_timeout_value[index];
    backlight_update_state();
}
#endif /* CONFIG_CHARGING */

#ifdef HAS_BUTTON_HOLD
/* Hold button change event handler. */
void backlight_hold_changed(bool hold_button)
{
    /* Hold switch overrides all backlight behavior except when
       set to "Normal" */
    /* Queue or freeze */
    if (hold_button && backlight_on_button_hold == 1)
    {
        backlight_off(); /* setting == Off */
    }
    else  /* setting == On, Normal, no hold button, or anything else */
        backlight_on();
}

void backlight_set_on_button_hold(int index)
{
    if ((unsigned)index >= 3)
        /* if given a weird value, use default */
        index = 0;

    if (index == backlight_on_button_hold)
        return;

    backlight_on_button_hold = index;
    backlight_hold_changed(button_hold());
}
#endif /* HAS_BUTTON_HOLD */

#ifdef HAVE_LCD_SLEEP
void lcd_set_sleep_after_backlight_off(int index)
{
    if ((unsigned)index >= sizeof(lcd_sleep_timeout_value))
        /* if given a weird value, use default */
        index = 3;

    lcd_sleep_timeout = HZ * lcd_sleep_timeout_value[index];

    if (backlight_timer > 0 || backlight_get_current_timeout() == 0)
        /* Timer will be set when bl turns off or bl set to on. */
        return;

    /* Backlight is Off */
    if (lcd_sleep_timeout < 0)
        lcd_sleep_timer = 1; /* Always - sleep next tick */
    else
        lcd_sleep_timer = lcd_sleep_timeout; /* Never, other */
}
#endif /* HAVE_LCD_SLEEP */

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_ON, NULL);
}

void remote_backlight_off(void)
{
    queue_post(&backlight_queue, REMOTE_BACKLIGHT_OFF, NULL);
}

void remote_backlight_set_timeout(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index=6;
    remote_backlight_timeout = HZ * backlight_timeout_value[index];
    remote_backlight_update_state();
}

#ifdef CONFIG_CHARGING
void remote_backlight_set_timeout_plugged(int index)
{
    if((unsigned)index >= sizeof(backlight_timeout_value))
        /* if given a weird value, use default */
        index=6;
    remote_backlight_timeout_plugged = HZ * backlight_timeout_value[index];
    remote_backlight_update_state();
}
#endif /* CONFIG_CHARGING */

/* return value in ticks; 0 means always on, <0 means always off */
int remote_backlight_get_current_timeout(void)
{
#ifdef CONFIG_CHARGING
    if (charger_inserted()
#ifdef HAVE_USB_POWER
            || usb_powered()
#endif
        )
        return remote_backlight_timeout_plugged;
    else
        return remote_backlight_timeout;
#else
    return remote_backlight_timeout;
#endif
}

/* returns true when the backlight is on OR when it's set to always off */
bool is_remote_backlight_on(void)
{
    if (remote_backlight_timer != 0 ||
                    remote_backlight_get_current_timeout() <= 0)
        return true;
    else
        return false;
}

#endif /* HAVE_REMOTE_LCD */

#else /* !defined(CONFIG_BACKLIGHT) || defined(BOOTLOADER)
    -- no backlight, empty dummy functions */

#if defined(BOOTLOADER) && defined(CONFIG_BACKLIGHT)
void backlight_init(void)
{
#ifdef IRIVER_H300_SERIES
    or_l(0x00020000, &GPIO1_OUT);
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
#endif
}
#endif

void backlight_on(void) {}
void backlight_off(void) {}
void backlight_set_timeout(int index) {(void)index;}
bool is_backlight_on(void) {return true;}
#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void) {}
void remote_backlight_off(void) {}
void remote_backlight_set_timeout(int index) {(void)index;}
bool is_remote_backlight_on(void) {return true;}
#endif
#endif /* defined(CONFIG_BACKLIGHT) && !defined(BOOTLOADER) */

/* TODO: Move low level code to target/ tree. Create
   __backlight_set_brightness and call from here. */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
#if defined(IRIVER_H300_SERIES) || defined(IAUDIO_X5)
void backlight_set_brightness(int val)
{
#ifndef SIMULATOR
    /* set brightness by changing the PWM
     * accepts 0..15 but note that 0 and 1 give a black display on H300!
     * 0 is black on the X5.
     */

    /* disable IRQs while bitbanging */
    int old_irq_level = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Clamp setting to range */
    if(val<MIN_BRIGHTNESS_SETTING)
        val=MIN_BRIGHTNESS_SETTING;
    else if(val>MAX_BRIGHTNESS_SETTING)
        val=MAX_BRIGHTNESS_SETTING;

    pcf50606_write(0x35, (val << 1) | 0x01); /* 512Hz, Enable PWM */

    /* enable IRQs again */
    set_irq_level(old_irq_level);
#else
    val=0;
#endif
}
#endif
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

