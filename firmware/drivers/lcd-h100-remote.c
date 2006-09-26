/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charit� III
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"

/*** definitions ***/

#define LCD_REMOTE_CNTL_ADC_NORMAL          0xa0
#define LCD_REMOTE_CNTL_ADC_REVERSE         0xa1
#define LCD_REMOTE_CNTL_SHL_NORMAL          0xc0
#define LCD_REMOTE_CNTL_SHL_REVERSE         0xc8
#define LCD_REMOTE_CNTL_DISPLAY_ON_OFF      0xae
#define LCD_REMOTE_CNTL_ENTIRE_ON_OFF       0xa4
#define LCD_REMOTE_CNTL_REVERSE_ON_OFF      0xa6
#define LCD_REMOTE_CNTL_NOP                 0xe3
#define LCD_REMOTE_CNTL_POWER_CONTROL       0x2b
#define LCD_REMOTE_CNTL_SELECT_REGULATOR    0x20
#define LCD_REMOTE_CNTL_SELECT_BIAS         0xa2
#define LCD_REMOTE_CNTL_SELECT_VOLTAGE      0x81
#define LCD_REMOTE_CNTL_INIT_LINE           0x40
#define LCD_REMOTE_CNTL_SET_PAGE_ADDRESS    0xB0

#define LCD_REMOTE_CNTL_HIGHCOL             0x10    /* Upper column address */
#define LCD_REMOTE_CNTL_LOWCOL              0x00    /* Lower column address */

#define CS_LO      and_l(~0x00000004, &GPIO1_OUT)
#define CS_HI      or_l(0x00000004, &GPIO1_OUT)
#define CLK_LO     and_l(~0x10000000, &GPIO_OUT)
#define CLK_HI     or_l(0x10000000, &GPIO_OUT)
#define DATA_LO    and_l(~0x00040000, &GPIO1_OUT)
#define DATA_HI    or_l(0x00040000, &GPIO1_OUT)
#define RS_LO      and_l(~0x00010000, &GPIO_OUT)
#define RS_HI      or_l(0x00010000, &GPIO_OUT)

#define SCROLLABLE_LINES 13

/*** globals ***/

fb_remote_data lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH]
                                      IBSS_ATTR;

static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;

#ifndef SIMULATOR
static int xoffset; /* needed for flip */

/* timeout counter for deasserting /CS after access, <0 means not counting */
static int cs_countdown IDATA_ATTR = 0;
#define CS_TIMEOUT (HZ/10)

#ifdef HAVE_REMOTE_LCD_TICKING
/* If set to true, will prevent "ticking" to headphones. */
static bool emireduce = false;
static int byte_delay = 0;
#endif

/* remote hotplug */
static struct event_queue remote_scroll_queue;
#define REMOTE_INIT_LCD   1
#define REMOTE_DEINIT_LCD 2

static bool remote_initialized = false;
static int _remote_type = REMOTETYPE_UNPLUGGED;

/* cached settings values */
static bool cached_invert = false;
static bool cached_flip = false;
static int cached_contrast = DEFAULT_REMOTE_CONTRAST_SETTING;
#endif

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
static void scroll_thread(void);
static long scroll_stack[DEFAULT_STACK_SIZE/sizeof(long)];
static const char scroll_name[] = "remote_scroll";
static int scroll_ticks = 12; /* # of ticks between updates*/
static int scroll_delay = HZ/2; /* ticks delay before start */
static int scroll_step = 6;  /* pixels per scroll step */
static int bidir_limit = 50;  /* percent */
static struct scrollinfo scroll[SCROLLABLE_LINES];

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

/*** driver routines ***/

#ifndef SIMULATOR

#ifdef HAVE_REMOTE_LCD_TICKING
static inline void _byte_delay(int delay)
{
    asm (
        "move.l  %[dly], %%d0    \n"
        "ble.s   2f              \n"
    "1:                          \n"
        "subq.l  #1, %%d0        \n"
        "bne.s   1b              \n"
    "2:                          \n"
        : /* outputs */
        : /* inputs */
        [dly]"d"(delay)
        : /* clobbers */
        "d0"
    );
}
#endif /* HAVE_REMOTE_LCD_TICKING */

/* Standard low-level byte writer. Requires CLK low on entry */
static inline void _write_byte(unsigned data)
{
    asm volatile (
        "move.l  (%[gpo1]), %%d0     \n"  /* Get current state of data line */
        "and.l   %[dbit], %%d0       \n"
        "beq.s   1f                  \n"  /*   and set it as previous-state bit */
        "bset    #8, %[data]         \n"  
    "1:                              \n"
        "move.l  %[data], %%d0       \n"  /* Compute the 'bit derivative', i.e. a value */
        "lsr.l   #1, %%d0            \n"  /*   with 1's where the data changes from the */
        "eor.l   %%d0, %[data]       \n"  /*   previous state, and 0's where it doesn't */
        "swap    %[data]             \n"  /* Shift data to upper byte */
        "lsl.l   #8, %[data]         \n"

        "lsl.l   #1,%[data]          \n"  /* Shift out MSB */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"  /* 1: flip DATA */
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"  /* Flip CLK */
        "eor.l   %[cbit], (%[gpo0])  \n"  /* Flip CLK */

        "lsl.l   #1,%[data]          \n"  /* ..unrolled.. */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], (%[gpo1])  \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        : /* outputs */
        [data]"+d"(data)
        : /* inputs */
        [gpo0]"a"(&GPIO_OUT),
        [cbit]"d"(0x10000000),
        [gpo1]"a"(&GPIO1_OUT),
        [dbit]"d"(0x00040000)
        : /* clobbers */
        "d0"
    );
}

/* Fast low-level byte writer. Don't use with high CPU clock.
 * Requires CLK low on entry */
static inline void _write_fast(unsigned data)
{
    asm volatile (
        "move.w  %%sr,%%d3           \n"  /* Get current interrupt level */
        "move.w  #0x2700,%%sr        \n"  /* Disable interrupts */

        "move.l  (%[gpo1]), %%d0     \n"  /* Get current state of data port */
        "move.l  %%d0, %%d1          \n"
        "and.l   %[dbit], %%d1       \n"  /* Check current state of data line */
        "beq.s   1f                  \n"  /*   and set it as previous-state bit */
        "bset    #8, %[data]         \n"
    "1:                              \n"
        "move.l  %[data], %%d1       \n"  /* Compute the 'bit derivative', i.e. a value */
        "lsr.l   #1, %%d1            \n"  /*   with 1's where the data changes from the */
        "eor.l   %%d1, %[data]       \n"  /*   previous state, and 0's where it doesn't */
        "swap    %[data]             \n"  /* Shift data to upper byte */
        "lsl.l   #8, %[data]         \n"
        
        "move.l  (%[gpo0]), %%d1     \n"  /* Get current state of clock port */
        "move.l  %[cbit], %%d2       \n"  /* Precalculate opposite state of clock line */
        "eor.l   %%d1, %%d2          \n"  

        "lsl.l   #1,%[data]          \n"  /* Shift out MSB */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"  /* 1: flip data bit */
        "move.l  %%d0, (%[gpo1])     \n"  /*   and output new DATA state */
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"  /* Set CLK */
        "move.l  %%d1, (%[gpo0])     \n"  /* Reset CLK */

        "lsl.l   #1,%[data]          \n"  /* ..unrolled.. */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "move.l  %%d0, (%[gpo1])     \n"
    "1:                              \n"
        "move.l  %%d2, (%[gpo0])     \n"
        "move.l  %%d1, (%[gpo0])     \n"

        "move.w  %%d3, %%sr          \n" /* Restore interrupt level */
        : /* outputs */
        [data]"+d"(data)
        : /* inputs */
        [gpo0]"a"(&GPIO_OUT),
        [cbit]"i"(0x10000000),
        [gpo1]"a"(&GPIO1_OUT),
        [dbit]"d"(0x00040000)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
    );
}

void lcd_remote_write_command(int cmd)
{
    cs_countdown = 0;
    RS_LO;
    CS_LO;

    _write_byte(cmd);
#ifdef HAVE_REMOTE_LCD_TICKING
     _byte_delay(byte_delay - 148);
#endif

    cs_countdown = CS_TIMEOUT;
}

void lcd_remote_write_command_ex(int cmd, int data)
{
    cs_countdown = 0;
    RS_LO;
    CS_LO;

    _write_byte(cmd);
#ifdef HAVE_REMOTE_LCD_TICKING
    _byte_delay(byte_delay - 148);
#endif
    _write_byte(data);
#ifdef HAVE_REMOTE_LCD_TICKING
    _byte_delay(byte_delay - 148);
#endif

    cs_countdown = CS_TIMEOUT;
}

void lcd_remote_write_data(const unsigned char* p_bytes, int count) ICODE_ATTR;
void lcd_remote_write_data(const unsigned char* p_bytes, int count)
{
    const unsigned char *p_end = p_bytes + count;

    cs_countdown = 0;
    RS_HI;
    CS_LO;

    /* This is safe as long as lcd_remote_write_data() isn't called from within
     * an ISR. */
    if (cpu_frequency < 50000000)
    {
        while (p_bytes < p_end)
        {
            _write_fast(*p_bytes++);
#ifdef HAVE_REMOTE_LCD_TICKING
            _byte_delay(byte_delay - 87);
#endif
        }
    }
    else
    {
        while (p_bytes < p_end)
        {
            _write_byte(*p_bytes++);
#ifdef HAVE_REMOTE_LCD_TICKING
            _byte_delay(byte_delay - 148);
#endif
        }
    }

    cs_countdown = CS_TIMEOUT;
}
#endif /* !SIMULATOR */

/*** hardware configuration ***/

int lcd_remote_default_contrast(void)
{
    return DEFAULT_REMOTE_CONTRAST_SETTING;
}

#ifndef SIMULATOR

#ifdef HAVE_REMOTE_LCD_TICKING
void lcd_remote_emireduce(bool state)
{
    emireduce = state;
}
#endif

void lcd_remote_powersave(bool on)
{
    if (remote_initialized)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | (on ? 0 : 1));
        lcd_remote_write_command(LCD_REMOTE_CNTL_ENTIRE_ON_OFF | (on ? 1 : 0));
    }
}

void lcd_remote_set_contrast(int val)
{
    cached_contrast = val;
    if (remote_initialized)
        lcd_remote_write_command_ex(LCD_REMOTE_CNTL_SELECT_VOLTAGE, val);
}

void lcd_remote_set_invert_display(bool yesno)
{
    cached_invert = yesno;
    if (remote_initialized)
        lcd_remote_write_command(LCD_REMOTE_CNTL_REVERSE_ON_OFF | (yesno?1:0));
}

/* turn the display upside down (call lcd_remote_update() afterwards) */
void lcd_remote_set_flip(bool yesno)
{
    cached_flip = yesno;
    if (yesno)
    {
        xoffset = 0;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_NORMAL);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_NORMAL);
        }
    }
    else
    {
        xoffset = 132 - LCD_REMOTE_WIDTH;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_REVERSE);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_REVERSE);
        }
    }
}

/* The actual LCD init */
static void remote_lcd_init(void)
{
    CS_HI;
    CLK_LO;
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_BIAS | 0x0);

    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x5);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x6);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x7);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_REGULATOR | 0x4); // 0x4 Select regulator @ 5.0 (default);
    
    sleep(1);
    
    lcd_remote_write_command(LCD_REMOTE_CNTL_INIT_LINE | 0x0); // init line
    lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | 0x0); // page address
    lcd_remote_write_command_ex(0x10, 0x00); // Column MSB + LSB

    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | 1);
    
    remote_initialized = true;

    lcd_remote_set_flip(cached_flip);
    lcd_remote_set_contrast(cached_contrast);
    lcd_remote_set_invert_display(cached_invert);
}

int remote_type(void)
{
    return _remote_type;
}

/* Monitor remote hotswap */
static void remote_tick(void)
{
    static bool last_status = false;
    static int countdown = 0;
    static int init_delay = 0;
    bool current_status;
    int val;
    int level;

    current_status = ((GPIO_READ & 0x40000000) == 0);
    /* Only report when the status has changed */
    if (current_status != last_status)
    {
        last_status = current_status;
        countdown = current_status ? 20*HZ : 1;
    }
    else
    {
        /* Count down until it gets negative */
        if (countdown >= 0)
            countdown--;
            
        if (current_status)
        {
            if (!(countdown % 8))
            {
                /* Determine which type of remote it is */
                level = set_irq_level(HIGHEST_IRQ_LEVEL);
                val = adc_scan(ADC_REMOTEDETECT);
                set_irq_level(level);
                
                if (val < ADCVAL_H100_LCD_REMOTE_HOLD)
                {
                    if (val < ADCVAL_H100_LCD_REMOTE)
                        if (val < ADCVAL_H300_LCD_REMOTE)
                            _remote_type = REMOTETYPE_H300_LCD;  /* hold off */
                        else
                            _remote_type = REMOTETYPE_H100_LCD;  /* hold off */
                    else
                        if (val < ADCVAL_H300_LCD_REMOTE_HOLD)
                            _remote_type = REMOTETYPE_H300_LCD;  /* hold on */
                        else
                            _remote_type = REMOTETYPE_H100_LCD;  /* hold on */

                    if (--init_delay <= 0)
                    {
                        queue_post(&remote_scroll_queue, REMOTE_INIT_LCD, 0);
                        init_delay = 6;
                    }
                }
                else
                {
                    _remote_type = REMOTETYPE_H300_NONLCD; /* hold on or off */
                }
            }
        }
        else
        {
            if (countdown == 0)
            {
                _remote_type = REMOTETYPE_UNPLUGGED;

                queue_post(&remote_scroll_queue, REMOTE_DEINIT_LCD, 0);
            }
        }
    }

    /* handle chip select timeout */
    if (cs_countdown >= 0)
        cs_countdown--;
    if (cs_countdown == 0)
        CS_HI;
}
#endif /* !SIMULATOR */

/* LCD init */
#ifdef SIMULATOR

void lcd_remote_init(void)
{
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name IF_PRIO(, PRIORITY_USER_INTERFACE));
}
#else /* !SIMULATOR */

/* Initialise ports and kick off monitor */
void lcd_remote_init(void)
{
#ifdef IRIVER_H300_SERIES
    or_l(0x10010000, &GPIO_FUNCTION); /* GPIO16: RS
                                         GPIO28: CLK */
    
    or_l(0x00040006, &GPIO1_FUNCTION); /* GPO33:  Backlight
                                          GPIO34: CS
                                          GPIO50: Data */
    or_l(0x10010000, &GPIO_ENABLE);
    or_l(0x00040006, &GPIO1_ENABLE);
#else
    or_l(0x10010800, &GPIO_FUNCTION); /* GPIO11: Backlight
                                         GPIO16: RS
                                         GPIO28: CLK */
    
    or_l(0x00040004, &GPIO1_FUNCTION); /* GPIO34: CS
                                          GPIO50: Data */
    or_l(0x10010800, &GPIO_ENABLE);
    or_l(0x00040004, &GPIO1_ENABLE);
#endif
    lcd_remote_clear_display();

    /* private queue */
    queue_init(&remote_scroll_queue, false);
    tick_add_task(remote_tick);
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name IF_PRIO(, PRIORITY_USER_INTERFACE));
}

/*** update functions ***/

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void) ICODE_ATTR;
void lcd_remote_update(void)
{
    int y;
    
    if (!remote_initialized)
        return;
      
#ifdef HAVE_REMOTE_LCD_TICKING
    /* Adjust byte delay for emi reduction. */
    byte_delay = emireduce ? cpu_frequency / 197600 + 28: 0;
#endif

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_REMOTE_HEIGHT/8; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL | ((xoffset >> 4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL | (xoffset & 0xf));
        lcd_remote_write_data(lcd_remote_framebuffer[y], LCD_REMOTE_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_remote_update_rect(int x, int y, int width, int height)
{
    int ymax;

    if (!remote_initialized)
        return;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 3;
    y >>= 3;

    if(x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_REMOTE_HEIGHT/8)
        ymax = LCD_REMOTE_HEIGHT/8-1;

#ifdef HAVE_REMOTE_LCD_TICKING
    /* Adjust byte delay for emi reduction */
    byte_delay = emireduce ? cpu_frequency / 197600 + 28: 0;
#endif

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL | (((x+xoffset) >> 4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL | ((x+xoffset) & 0xf));
        lcd_remote_write_data(&lcd_remote_framebuffer[y][x], width);
    }
}
#endif /* !SIMULATOR */

/*** parameter handling ***/

void lcd_remote_set_drawmode(int mode)
{
    drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_remote_get_drawmode(void)
{
    return drawmode;
}

void lcd_remote_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_remote_getxmargin(void)
{
    return xmargin;
}

int lcd_remote_getymargin(void)
{
    return ymargin;
}


void lcd_remote_setfont(int newfont)
{
    curfont = newfont;
}

int lcd_remote_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, curfont);
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] |= 1 << (y & 7);
}

static void clearpixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] &= ~(1 << (y & 7));
}

static void flippixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] ^= 1 << (y & 7);
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};
                               
static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
                      ICODE_ATTR;
static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= bits & mask;
}

static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address &= bits | ~mask;
}

static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address |= bits & mask;
}

static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    
    bits    ^= data;
    *address = data ^ (bits & mask);
}

static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                         ICODE_ATTR;
static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= ~bits & mask;
}

static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address &= ~(bits & mask);
}

static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address |= ~bits & mask;
}

static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;
    
    bits     = ~bits ^ data;
    *address = data ^ (bits & mask);
}

lcd_remote_blockfunc_type* const lcd_remote_blockfuncs[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

/*** drawing functions ***/

/* Clear the whole display */
void lcd_remote_clear_display(void)
{
    unsigned bits = (drawmode & DRMODE_INVERSEVID) ? 0xFFu : 0;

    memset(lcd_remote_framebuffer, bits, sizeof lcd_remote_framebuffer);
    scrolling_lines = 0;
}

/* Set a single pixel */
void lcd_remote_drawpixel(int x, int y)
{
    if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
        lcd_remote_pixelfuncs[drawmode](x, y);
}

/* Draw a line */
void lcd_remote_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_remote_pixelfunc_type *pfunc = lcd_remote_pixelfuncs[drawmode];

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i = 0; i < numpixels; i++)
    {
        if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
            pfunc(x, y);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a horizontal line (optimised) */
void lcd_remote_hline(int x1, int x2, int y)
{
    int x;
    fb_remote_data *dst, *dst_end;
    unsigned mask;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    
    /* nothing to draw? */
    if (((unsigned)y >= LCD_REMOTE_HEIGHT) || (x1 >= LCD_REMOTE_WIDTH) 
        || (x2 < 0))
        return;  
    
    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCD_REMOTE_WIDTH)
        x2 = LCD_REMOTE_WIDTH-1;
        
    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x1];
    mask  = 1 << (y & 7);

    dst_end = dst + x2 - x1;
    do
        bfunc(dst++, mask, 0xFFu);
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_remote_vline(int x, int y1, int y2)
{
    int ny;
    fb_remote_data *dst;
    unsigned mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        ny = y1;
        y1 = y2;
        y2 = ny;
    }

    /* nothing to draw? */
    if (((unsigned)x >= LCD_REMOTE_WIDTH) || (y1 >= LCD_REMOTE_HEIGHT) 
        || (y2 < 0))
        return;  
    
    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCD_REMOTE_HEIGHT)
        y2 = LCD_REMOTE_HEIGHT-1;
        
    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y1>>3][x];
    ny    = y2 - (y1 & ~7);
    mask  = 0xFFu << (y1 & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
}

/* Draw a rectangular box */
void lcd_remote_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_remote_vline(x, y, y2);
    lcd_remote_vline(x2, y, y2);
    lcd_remote_hline(x, x2, y);
    lcd_remote_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_remote_fillrect(int x, int y, int width, int height)
{
    int ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    lcd_remote_blockfunc_type *bfunc;
    bool fillopt = false;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH) 
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;
    
    if (drawmode & DRMODE_INVERSEVID)
    {
        if (drawmode & DRMODE_BG)
        {
            fillopt = true;
        }
    }
    else
    {
        if (drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = 0xFFu;
        }
    }
    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x];
    ny    = height - 1 + (y & 7);
    mask  = 0xFFu << (y & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        if (fillopt && (mask == 0xFFu))
            memset(dst, bits, width);
        else
        {
            fb_remote_data *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFu);
            while (dst_row < dst_end);
        }

        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFu))
        memset(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFu);
        while (dst < dst_end);
    }
}

/* About Rockbox' internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial bitmap */
void lcd_remote_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
                            ICODE_ATTR;
void lcd_remote_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
{
    int shift, ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH) 
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;
        
    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    src   += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    y     -= src_y;
    dst    = &lcd_remote_framebuffer[y>>3][x];
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = lcd_remote_blockfuncs[drawmode];
    mask   = 0xFFu << (shift + src_y);
    mask_bottom = 0xFFu >> (~ny & 7);

    if (shift == 0)
    {
        bool copyopt = (drawmode == DRMODE_SOLID);

        for (; ny >= 8; ny -= 8)
        {
            if (copyopt && (mask == 0xFFu))
                memcpy(dst, src, width);
            else
            {
                const unsigned char *src_row = src;
                fb_remote_data *dst_row = dst;

                dst_end = dst_row + width;
                do
                    bfunc(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }

            src += stride;
            dst += LCD_REMOTE_WIDTH;
            mask = 0xFFu;
        }
        mask &= mask_bottom;

        if (copyopt && (mask == 0xFFu))
            memcpy(dst, src, width);
        else
        {
            dst_end = dst + width;
            do
                bfunc(dst++, mask, *src++);
            while (dst < dst_end);
        }
    }
    else
    {
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            fb_remote_data *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;
            
            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    bfunc(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            bfunc(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
}

/* Draw a full bitmap */
void lcd_remote_bitmap(const unsigned char *src, int x, int y, int width,
                       int height)
{
    lcd_remote_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
static void lcd_remote_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    struct font* pf = font_get(curfont);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < LCD_REMOTE_WIDTH)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = font_get_width(pf, ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, ch);

        lcd_remote_bitmap_part(bits, ofs, 0, width, x, y, width - ofs,
                               pf->height);
        
        x += width - ofs;
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_remote_putsxy(int x, int y, const unsigned char *str)
{
    lcd_remote_putsxyofs(x, y, 0, str);
}

/*** line oriented text output ***/

/* put a string at a given char position */
void lcd_remote_puts(int x, int y, const unsigned char *str)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, 0);
}

void lcd_remote_puts_style(int x, int y, const unsigned char *str, int style)
{
    lcd_remote_puts_style_offset(x, y, str, style, 0);
}

void lcd_remote_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, offset);
}

/* put a string at a given char position, style, and pixel position,
 * skipping first offset pixel columns */
void lcd_remote_puts_style_offset(int x, int y, const unsigned char *str,
                                  int style, int offset)
{
    int xpos,ypos,w,h,xrect;
    int lastmode = drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_remote_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / utf8length((char *)str);
    ypos = ymargin + y*h;
    drawmode = (style & STYLE_INVERT) ?
               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    lcd_remote_putsxyofs(xpos, ypos, offset, str);
    drawmode ^= DRMODE_INVERSEVID;
    xrect = xpos + MAX(w - offset, 0);
    lcd_remote_fillrect(xrect, ypos, LCD_REMOTE_WIDTH - xrect, h);
    drawmode = lastmode;
}

/*** scrolling ***/

/* Reverse the invert setting of the scrolling line (if any) at given char
   position.  Setting will go into affect next time line scrolls. */
void lcd_remote_invertscroll(int x, int y)
{
    struct scrollinfo* s;

    (void)x;

    s = &scroll[y];
    s->invert = !s->invert;
}

void lcd_remote_stop_scroll(void)
{
    scrolling_lines=0;
}

void lcd_remote_scroll_speed(int speed)
{
    scroll_ticks = scroll_tick_table[speed];
}

void lcd_remote_scroll_step(int step)
{
    scroll_step = step;
}

void lcd_remote_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
}

void lcd_remote_bidir_scroll(int percent)
{
    bidir_limit = percent;
}

void lcd_remote_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_remote_puts_scroll_style(x, y, string, STYLE_DEFAULT);
}

void lcd_remote_puts_scroll_style(int x, int y, const unsigned char *string, int style)
{
     lcd_remote_puts_scroll_style_offset(x, y, string, style, 0);
}

void lcd_remote_puts_scroll_offset(int x, int y, const unsigned char *string, int offset)
{
     lcd_remote_puts_scroll_style_offset(x, y, string, STYLE_DEFAULT, offset);
}          
   
void lcd_remote_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                         int style, int offset)
{
    struct scrollinfo* s;
    int w, h;

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_remote_puts_style_offset(x,y,string,STYLE_INVERT,offset);
    }
    else
        lcd_remote_puts_offset(x,y,string,offset);

    lcd_remote_getstringsize(string, &w, &h);

    if (LCD_REMOTE_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, (char *)string);

        /* get width */
        s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( bidir_limit ) {
            s->bidir = s->width < (LCD_REMOTE_WIDTH - xmargin) *
                (100 + bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, (char *)string, LCD_REMOTE_WIDTH/2);

        s->len = utf8length((char *)string);
        s->offset = offset;
        s->startx = xmargin + x * s->width / s->len;;
        s->backward = false;
        scrolling_lines |= (1<<y);
    }
    else
        /* force a bit switch-off since it doesn't scroll */
        scrolling_lines &= ~(1<<y);
}

static void scroll_thread(void)
{
    struct font* pf;
    struct scrollinfo* s;
    long next_tick = current_tick;
    long delay = 0;
    int index;
    int xpos, ypos;
    int lastmode;
#ifndef SIMULATOR
    struct event ev;
#endif

    /* initialize scroll struct array */
    scrolling_lines = 0;

    while ( 1 ) {

#ifdef SIMULATOR
        sleep(delay);
#else
        if (remote_initialized)
            queue_wait_w_tmo(&remote_scroll_queue, &ev, delay);
        else
            queue_wait(&remote_scroll_queue, &ev);

        switch (ev.id)
        {
            case REMOTE_INIT_LCD:
                remote_lcd_init();
                lcd_remote_update();
                break;
                
            case REMOTE_DEINIT_LCD:
                CLK_LO;
                CS_HI;
                remote_initialized = false;
                break;
        }

        delay = next_tick - current_tick - 1;
        if (delay >= 0)
            continue;
#endif
        for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
            /* really scroll? */
            if ( !(scrolling_lines&(1<<index)) )
                continue;

            s = &scroll[index];

            /* check pause */
            if (TIME_BEFORE(current_tick, s->start_tick))
                continue;

            if (s->backward)
                s->offset -= scroll_step;
            else
                s->offset += scroll_step;

            pf = font_get(curfont);
            xpos = s->startx;
            ypos = ymargin + index * pf->height;

            if (s->bidir) { /* scroll bidirectional */
                if (s->offset <= 0) {
                    /* at beginning of line */
                    s->offset = 0;
                    s->backward = false;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
                if (s->offset >= s->width - (LCD_REMOTE_WIDTH - xpos)) {
                    /* at end of line */
                    s->offset = s->width - (LCD_REMOTE_WIDTH - xpos);
                    s->backward = true;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
            }
            else {
                /* scroll forward the whole time */
                if (s->offset >= s->width)
                    s->offset %= s->width;
            }

            lastmode = drawmode;
            drawmode = s->invert ?
                       (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
            lcd_remote_putsxyofs(xpos, ypos, s->offset, s->line);
            drawmode = lastmode;
            lcd_remote_update_rect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
        }

        next_tick += scroll_ticks;
        delay = next_tick - current_tick - 1;
        if (delay < 0)
        {
            next_tick = current_tick + 1;
            delay = 0;
        }
    }
}

