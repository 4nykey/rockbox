/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#ifndef SIMULATOR
#include <stdio.h>
#include <stdbool.h>
#include "lcd.h"
#include "menu.h"
#include "debug_menu.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "adc.h"
#include "mas.h"
#include "power.h"
#include "rtc.h"
#include "debug.h"
#include "thread.h"
#include "powermgmt.h"
#include "system.h"

/*---------------------------------------------------*/
/*    SPECIAL DEBUG STUFF                            */
/*---------------------------------------------------*/
extern int ata_device;
extern int ata_io_address;
extern int num_threads;
extern char *thread_name[];

#ifdef HAVE_LCD_BITMAP
/* Test code!!! */
bool dbg_os(void)
{
    char buf[32];
    int button;
    int i;
    int usage;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        lcd_puts(0, 0, "Stack usage:");
        for(i = 0; i < num_threads;i++)
        {
            usage = thread_stack_usage(i);
            snprintf(buf, 32, "%s: %d%%", thread_name[i], usage);
            lcd_puts(0, 1+i, buf);
        }

        lcd_update();
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_OFF:
            case BUTTON_LEFT:
                return false;
        }
    }
    return false;
}
#else
bool dbg_os(void)
{
    char buf[32];
    int button;
    int usage;
    int currval = 0;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        lcd_puts(0, 0, "Stack usage");

        usage = thread_stack_usage(currval);
        snprintf(buf, 32, "%d: %d%%  ", currval, usage);
        lcd_puts(0, 1, buf);
	
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
        case BUTTON_STOP:
            return false;

	    case BUTTON_LEFT:
            currval--;
            if(currval < 0)
                currval = num_threads-1;
            break;

        case BUTTON_RIGHT:
            currval++;
            if(currval > num_threads-1)
                currval = 0;
            break;
        }
    }
    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
/* Test code!!! */
bool dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        snprintf(buf, 32, "PADR: %04x", porta);
        lcd_puts(0, 0, buf);
        snprintf(buf, 32, "PBDR: %04x", portb);
        lcd_puts(0, 1, buf);

        snprintf(buf, 32, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_puts(0, 2, buf);
        snprintf(buf, 32, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_puts(0, 3, buf);
        snprintf(buf, 32, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_puts(0, 4, buf);
        snprintf(buf, 32, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));
        lcd_puts(0, 5, buf);

        battery_voltage = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV %d%%  ", batt_int, batt_frac,
                 battery_level());
        lcd_puts(0, 6, buf);

        snprintf(buf, 32, "ATA: %s, 0x%x",
                 ata_device?"slave":"master", ata_io_address);
        lcd_puts(0, 7, buf);
	
        lcd_update();
        sleep(HZ/10);

        button = button_get(false);

        switch(button)
        {
            case BUTTON_OFF:
                return false;
        }
    }
    return false;
}
#else
bool dbg_ports(void)
{
    unsigned short porta;
    unsigned short portb;
    unsigned char portc;
    char buf[32];
    int button;
    int battery_voltage;
    int batt_int, batt_frac;
    int currval = 0;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();

    while(1)
    {
        porta = PADR;
        portb = PBDR;
        portc = PCDR;

        switch(currval)
        {
        case 0:
            snprintf(buf, 32, "PADR: %04x  ", porta);
            break;
        case 1:
            snprintf(buf, 32, "PBDR: %04x  ", portb);
            break;
        case 2:
            snprintf(buf, 32, "AN0: %03x  ", adc_read(0));
            break;
        case 3:
            snprintf(buf, 32, "AN1: %03x  ", adc_read(1));
            break;
        case 4:
            snprintf(buf, 32, "AN2: %03x  ", adc_read(2));
            break;
        case 5:
            snprintf(buf, 32, "AN3: %03x  ", adc_read(3));
            break;
        case 6:
            snprintf(buf, 32, "AN4: %03x  ", adc_read(4));
            break;
        case 7:
            snprintf(buf, 32, "AN5: %03x  ", adc_read(5));
            break;
        case 8:
            snprintf(buf, 32, "AN6: %03x  ", adc_read(6));
            break;
        case 9:
            snprintf(buf, 32, "AN7: %03x  ", adc_read(7));
            break;
        case 10:
            snprintf(buf, 32, "%s, 0x%x ",
                     ata_device?"slv":"mst", ata_io_address);
            break;
        }
        lcd_puts(0, 0, buf);
        
        battery_voltage = (adc_read(ADC_UNREG_POWER) * 
                           BATTERY_SCALE_FACTOR) / 10000;
        batt_int = battery_voltage / 100;
        batt_frac = battery_voltage % 100;
    
        snprintf(buf, 32, "Batt: %d.%02dV", batt_int, batt_frac);
        lcd_puts(0, 1, buf);
        
        sleep(HZ/5);

        button = button_get(false);

        switch(button)
        {
        case BUTTON_STOP:
            return false;

        case BUTTON_LEFT:
            currval--;
            if(currval < 0)
                currval = 10;
            break;

        case BUTTON_RIGHT:
            currval++;
            if(currval > 10)
                currval = 0;
            break;
        }
    }
    return false;
}
#endif

#ifdef HAVE_RTC
/* Read RTC RAM contents and display them */
bool dbg_rtc(void)
{
    char buf[32];
    unsigned char addr = 0, r, c;
    int i;
    int button;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "RTC read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            snprintf(buf, 10, "0x%02x: ", addr + r*4);
            for (c = 0; c <= 3; c++) {
                i = rtc_read(addr + r*4 + c);
                snprintf(buf + 6 + c*2, 3, "%02x", i);
            }
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();
        sleep(HZ/2);

        button = button_get(false);

        switch(button)
        {
        case BUTTON_DOWN:
            if (addr < 63-16) { addr += 16; }
            break;
        case BUTTON_UP:
            if (addr) { addr -= 16; }
            break;
        case BUTTON_F2:
            /* clear the user RAM space */
            for (c = 0; c <= 43; c++)
                rtc_write(0x14 + c, 0);
            break;
        case BUTTON_OFF:
        case BUTTON_LEFT:
            return false;
        }
    }
    return false;
}
#else
bool dbg_rtc(void)
{
    return false;
}
#endif

#ifdef HAVE_LCD_CHARCELLS
#define NUMROWS 1
#else
#define NUMROWS 4
#endif
/* Read MAS registers and display them */
bool dbg_mas(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "MAS register read:");

    while(1)
    {
        for (r = 0; r < NUMROWS; r++) {
            i = mas_readreg(addr + r);
            snprintf(buf, 30, "%02x %08x", addr + r, i);
            lcd_puts(0, r+1, buf);
        }

        lcd_update();
        sleep(HZ/16);

        switch(button_get(false))
        {
#ifdef HAVE_RECORDER_KEYPAD
        case BUTTON_DOWN:
#else
        case BUTTON_RIGHT:
#endif
            addr += NUMROWS;
            break;
#ifdef HAVE_RECORDER_KEYPAD
        case BUTTON_UP:
#else
        case BUTTON_LEFT:
#endif
            if(addr)
                addr -= NUMROWS;
            break;
#ifdef HAVE_RECORDER_KEYPAD
        case BUTTON_LEFT:
#else
        case BUTTON_DOWN:
#endif
            return false;
        }
    }
    return false;
}

#ifdef HAVE_MAS3587F
bool dbg_mas_codec(void)
{
    char buf[32];
    unsigned int addr = 0, r, i;

#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, "MAS codec reg read:");

    while(1)
    {
        for (r = 0; r < 4; r++) {
            i = mas_codec_readreg(addr + r);
            snprintf(buf, 30, "0x%02x: %08x", addr + r, i);
            lcd_puts(1, r+1, buf);
        }
        
        lcd_update();
        sleep(HZ/16);

        switch(button_get(false))
        {
        case BUTTON_DOWN:
            addr += 4;
            break;
        case BUTTON_UP:
            if (addr) { addr -= 4; }
            break;
        case BUTTON_LEFT:
            return false;
        }
    }
    return false;
}
#endif

#ifdef HAVE_LCD_BITMAP
/*
 * view_battery() shows a automatically scaled graph of the battery voltage
 * over time. Usable for estimating battery life / charging rate.
 * The power_history array is updated in power_thread of powermgmt.c.
 */

#define BAT_FIRST_VAL  MAX(POWER_HISTORY_LEN - LCD_WIDTH - 1, 0)
#define BAT_YSPACE    (LCD_HEIGHT - 20)

bool view_battery(void)
{
    int view = 0;
    int i, x, y;
    int maxv, minv;
    char buf[32];
    
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    while(1)
    {
        switch (view) {
            case 0: /* voltage history graph */
                /* Find maximum and minimum voltage for scaling */
                maxv = minv = 0;
                for (i = BAT_FIRST_VAL; i < POWER_HISTORY_LEN; i++) {
                    if (power_history[i] > maxv)
                        maxv = power_history[i];
                    if ((minv == 0) || ((power_history[i]) && 
                                        (power_history[i] < minv)) )
                    {
                        minv = power_history[i];
                    }
                }
                
                if (minv < 1)
                    minv = 1;
                if (maxv < 2)
                    maxv = 2;
                    
                lcd_clear_display();
                lcd_puts(0, 0, "Battery voltage:");
                snprintf(buf, 30, "scale %d.%02d-%d.%02d V", 
                         minv / 100, minv % 100, maxv / 100, maxv % 100);
                lcd_puts(0, 1, buf);
                
                x = 0;
                for (i = BAT_FIRST_VAL+1; i < POWER_HISTORY_LEN; i++) {
                    y = (power_history[i] - minv) * BAT_YSPACE / (maxv - minv);
                    lcd_clearline(x, LCD_HEIGHT-1, x, 20);
                    lcd_drawline(x, LCD_HEIGHT-1, x, 
                                 MIN(MAX(LCD_HEIGHT-1 - y, 20), LCD_HEIGHT-1));
                    x++;
                }

                break;
                
            case 1: /* status: */
                lcd_clear_display();
                lcd_puts(0, 0, "Power status:");
                
                y = (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) / 10000;
                snprintf(buf, 30, "Battery: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 1, buf);
                y = (adc_read(ADC_EXT_POWER) * EXT_SCALE_FACTOR) / 10000;
                snprintf(buf, 30, "External: %d.%02d V", y / 100, y % 100);
                lcd_puts(0, 2, buf);
                snprintf(buf, 30, "Charger: %s", 
                         charger_inserted() ? "present" : "absent");
                lcd_puts(0, 3, buf);
#ifdef HAVE_CHARGE_CTRL
                snprintf(buf, 30, "Charging: %s", 
                         charger_enabled ? "yes" : "no");
                lcd_puts(0, 4, buf);
#endif                
                y = ( power_history[POWER_HISTORY_LEN-1] * 100
                    + power_history[POWER_HISTORY_LEN-2] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD+1] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_NEGD] * 100 )
                    / CHARGE_END_NEGD / 2;
                
                snprintf(buf, 30, "short delta: %d", y);
                lcd_puts(0, 5, buf);
                
                y = ( power_history[POWER_HISTORY_LEN-1] * 100
                    + power_history[POWER_HISTORY_LEN-2] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD+1] * 100
                    - power_history[POWER_HISTORY_LEN-1-CHARGE_END_ZEROD] * 100 )
                    / CHARGE_END_ZEROD / 2;
                
                snprintf(buf, 30, "long delta: %d", y);
                lcd_puts(0, 6, buf);

#ifdef HAVE_CHARGE_CTRL
                lcd_puts(0, 7, power_message);
#endif
                break;
                
            case 2: /* voltage deltas: */
                lcd_clear_display();
                lcd_puts(0, 0, "Voltage deltas:");
                
                for (i = 0; i <= 6; i++) {
                    y = power_history[POWER_HISTORY_LEN-1-i] - 
                        power_history[POWER_HISTORY_LEN-1-i-1];
                    snprintf(buf, 30, "-%d min: %s%d.%02d V", i,
                             (y < 0) ? "-" : "", ((y < 0) ? y * -1 : y) / 100, 
                             ((y < 0) ? y * -1 : y ) % 100);
                    lcd_puts(0, i+1, buf);
                }
                break;
        }
        
        lcd_update();
        sleep(HZ/2);
        
        switch(button_get(false))
        {
            case BUTTON_UP:
                if (view)
                    view--;
                break;
                
            case BUTTON_DOWN:
                if (view < 2)
                    view++;
                break;
                
            case BUTTON_LEFT:
            case BUTTON_OFF:
                return false;
        }
    }
    return false;
}

#endif

#ifdef HAVE_MAS3507D
bool dbg_mas_info(void)
{
    int button;
    char buf[32];
    int currval = 0;
    unsigned long val;
    unsigned long pll48, pll44, config;
    int pll_toggle = 0;
    
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(0, 0);
#endif
    while(1)
    {
        switch(currval)
        {
        case 0:
            mas_readmem(MAS_BANK_D1, 0xff7, &val, 1);
            lcd_puts(0, 0, "Design Code");
            snprintf(buf, 32, "%05x      ", val);
            break;
        case 1:
            lcd_puts(0, 0, "DC/DC mode ");
            snprintf(buf, 32, "8e: %05x  ", mas_readreg(0x8e) & 0xfffff);
            break;
        case 2:
            lcd_puts(0, 0, "Mute/Bypass");
            snprintf(buf, 32, "aa: %05x  ", mas_readreg(0xaa) & 0xfffff);
            break;
        case 3:
            lcd_puts(0, 0, "PIOData    ");
            snprintf(buf, 32, "ed: %05x  ", mas_readreg(0xed) & 0xfffff);
            break;
        case 4:
            lcd_puts(0, 0, "Startup Cfg");
            snprintf(buf, 32, "e6: %05x  ", mas_readreg(0xe6) & 0xfffff);
            break;
        case 5:
            lcd_puts(0, 0, "KPrescale  ");
            snprintf(buf, 32, "e7: %05x  ", mas_readreg(0xe7) & 0xfffff);
            break;
        case 6:
            lcd_puts(0, 0, "KBass      ");
            snprintf(buf, 32, "6b: %05x   ", mas_readreg(0x6b) & 0xfffff);
            break;
        case 7:
            lcd_puts(0, 0, "KTreble    ");
            snprintf(buf, 32, "6f: %05x   ", mas_readreg(0x6f) & 0xfffff);
            break;
        case 8:
            mas_readmem(MAS_BANK_D0, 0x300, &val, 1);
            lcd_puts(0, 0, "Frame Count");
            snprintf(buf, 32, "0/300: %04x", val & 0xffff);
            break;
        case 9:
            mas_readmem(MAS_BANK_D0, 0x301, &val, 1);
            lcd_puts(0, 0, "Status1    ");
            snprintf(buf, 32, "0/301: %04x", val & 0xffff);
            break;
        case 10:
            mas_readmem(MAS_BANK_D0, 0x302, &val, 1);
            lcd_puts(0, 0, "Status2    ");
            snprintf(buf, 32, "0/302: %04x", val & 0xffff);
            break;
        case 11:
            mas_readmem(MAS_BANK_D0, 0x303, &val, 1);
            lcd_puts(0, 0, "CRC Count  ");
            snprintf(buf, 32, "0/303: %04x", val & 0xffff);
            break;
        case 12:
            mas_readmem(MAS_BANK_D0, 0x36d, &val, 1);
            lcd_puts(0, 0, "PLLOffset48");
            snprintf(buf, 32, "0/36d %05x", val & 0xfffff);
            break;
        case 13:
            mas_readmem(MAS_BANK_D0, 0x32d, &val, 1);
            lcd_puts(0, 0, "PLLOffset48");
            snprintf(buf, 32, "0/32d %05x", val & 0xfffff);
            break;
        case 14:
            mas_readmem(MAS_BANK_D0, 0x36e, &val, 1);
            lcd_puts(0, 0, "PLLOffset44");
            snprintf(buf, 32, "0/36e %05x", val & 0xfffff);
            break;
        case 15:
            mas_readmem(MAS_BANK_D0, 0x32e, &val, 1);
            lcd_puts(0, 0, "PLLOffset44");
            snprintf(buf, 32, "0/32e %05x", val & 0xfffff);
            break;
        case 16:
            mas_readmem(MAS_BANK_D0, 0x36f, &val, 1);
            lcd_puts(0, 0, "OutputConf ");
            snprintf(buf, 32, "0/36f %05x", val & 0xfffff);
            break;
        case 17:
            mas_readmem(MAS_BANK_D0, 0x32f, &val, 1);
            lcd_puts(0, 0, "OutputConf ");
            snprintf(buf, 32, "0/32f %05x", val & 0xfffff);
            break;
        case 18:
            mas_readmem(MAS_BANK_D1, 0x7f8, &val, 1);
            lcd_puts(0, 0, "LL Gain    ");
            snprintf(buf, 32, "1/7f8 %05x", val & 0xfffff);
            break;
        case 19:
            mas_readmem(MAS_BANK_D1, 0x7f9, &val, 1);
            lcd_puts(0, 0, "LR Gain    ");
            snprintf(buf, 32, "1/7f9 %05x", val & 0xfffff);
            break;
        case 20:
            mas_readmem(MAS_BANK_D1, 0x7fa, &val, 1);
            lcd_puts(0, 0, "RL Gain    ");
            snprintf(buf, 32, "1/7fa %05x", val & 0xfffff);
            break;
        case 21:
            mas_readmem(MAS_BANK_D1, 0x7fb, &val, 1);
            lcd_puts(0, 0, "RR Gain    ");
            snprintf(buf, 32, "1/7fb %05x", val & 0xfffff);
            break;
        case 22:
            lcd_puts(0, 0, "L Trailbits");
            snprintf(buf, 32, "c5: %05x   ", mas_readreg(0xc5) & 0xfffff);
            break;
        case 23:
            lcd_puts(0, 0, "R Trailbits");
            snprintf(buf, 32, "c6: %05x   ", mas_readreg(0xc6) & 0xfffff);
            break;
        }
        lcd_puts(0, 1, buf);
        
        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
        case BUTTON_STOP:
            return false;

        case BUTTON_LEFT:
            currval--;
            if(currval < 0)
                currval = 23;
            break;

        case BUTTON_RIGHT:
            currval++;
            if(currval > 23)
                currval = 0;
            break;
        case BUTTON_PLAY:
            pll_toggle = !pll_toggle;
            if(pll_toggle)
            {
                /* 14.31818 MHz crystal */
                pll48 = 0x5d9d0;
                pll44 = 0xfffceceb;
                config = 0;
            }
            else
            {
                /* 14.725 MHz crystal */
                pll48 = 0x2d0de;
                pll44 = 0xfffa2319;
                config = 0;
            }
            mas_writemem(MAS_BANK_D0, 0x32d, &pll48, 1);
            mas_writemem(MAS_BANK_D0, 0x32e, &pll44, 1);
            mas_writemem(MAS_BANK_D0, 0x32f, &config, 1);
            mas_run(0x475);
            break;
        }
    }
    return false;
}
#endif

bool debug_menu(void)
{
    int m;
    bool result;

    struct menu_items items[] = {
        { "View I/O ports", dbg_ports },
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_RTC
        { "View/clr RTC RAM", dbg_rtc },
#endif /* HAVE_RTC */
#endif /* HAVE_LCD_BITMAP */
        { "View OS stacks", dbg_os },
#ifdef HAVE_MAS3507D
        { "View MAS info", dbg_mas_info },
#endif
        { "View MAS regs", dbg_mas },
#ifdef HAVE_MAS3587F
        { "View MAS codec", dbg_mas_codec },
#endif
#ifdef HAVE_LCD_BITMAP
        { "View battery", view_battery },
#endif
    };

    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    result = menu_run(m);
    menu_exit(m);
    
    return result;
}

#endif /* SIMULATOR */

