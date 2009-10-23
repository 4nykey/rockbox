/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "kernel.h"
#include "system.h"
#include "panic.h"
#ifdef IPOD_NANO2G
#include "storage.h"
#endif

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));

default_interrupt(EXT0);
default_interrupt(EXT1);
default_interrupt(EXT2);
default_interrupt(EINT_VBUS);
default_interrupt(EINTG);
default_interrupt(INT_TIMERA);
default_interrupt(INT_WDT);
default_interrupt(INT_TIMERB);
default_interrupt(INT_TIMERC);
default_interrupt(INT_TIMERD);
default_interrupt(INT_DMA);
default_interrupt(INT_ALARM_RTC);
default_interrupt(INT_PRI_RTC);
default_interrupt(RESERVED1);
default_interrupt(INT_UART);
default_interrupt(INT_USB_HOST);
default_interrupt(INT_USB_FUNC);
default_interrupt(INT_LCDC_0);
default_interrupt(INT_LCDC_1);
default_interrupt(INT_ECC);
default_interrupt(INT_CALM);
default_interrupt(INT_ATA);
default_interrupt(INT_UART0);
default_interrupt(INT_SPDIF_OUT);
default_interrupt(INT_SDCI);
default_interrupt(INT_LCD);
default_interrupt(INT_SPI);
default_interrupt(INT_IIC);
default_interrupt(RESERVED2);
default_interrupt(INT_MSTICK);
default_interrupt(INT_ADC_WAKEUP);
default_interrupt(INT_ADC);
default_interrupt(INT_UNK1);
default_interrupt(INT_UNK2);
default_interrupt(INT_UNK3);


void INT_TIMER(void)
{
    if (TACON & 0x00038000) INT_TIMERA();
    if (TBCON & 0x00038000) INT_TIMERB();
    if (TCCON & 0x00038000) INT_TIMERC();
    if (TDCON & 0x00038000) INT_TIMERD();
}


#if CONFIG_CPU==S5L8701
static void (* const irqvector[])(void) =
{ /* still 90% unverified and probably incorrect */
    EXT0,EXT1,EXT2,EINT_VBUS,EINTG,INT_TIMER,INT_WDT,INT_UNK1,
    INT_UNK2,INT_UNK3,INT_DMA,INT_ALARM_RTC,INT_PRI_RTC,RESERVED1,INT_UART,INT_USB_HOST,
    INT_USB_FUNC,INT_LCDC_0,INT_LCDC_1,INT_CALM,INT_ATA,INT_UART0,INT_SPDIF_OUT,INT_ECC,
    INT_SDCI,INT_LCD,INT_SPI,INT_IIC,RESERVED2,INT_MSTICK,INT_ADC_WAKEUP,INT_ADC
};
#else
static void (* const irqvector[])(void) =
{
    EXT0,EXT1,EXT2,EINT_VBUS,EINTG,INT_TIMERA,INT_WDT,INT_TIMERB,
    INT_TIMERC,INT_TIMERD,INT_DMA,INT_ALARM_RTC,INT_PRI_RTC,RESERVED1,INT_UART,INT_USB_HOST,
    INT_USB_FUNC,INT_LCDC_0,INT_LCDC_1,INT_ECC,INT_CALM,INT_ATA,INT_UART0,INT_SPDIF_OUT,
    INT_SDCI,INT_LCD,INT_SPI,INT_IIC,RESERVED2,INT_MSTICK,INT_ADC_WAKEUP,INT_ADC
};
#endif

#if CONFIG_CPU==S5L8701
static const char * const irqname[] =
{ /* still 90% unverified and probably incorrect */
    "EXT0","EXT1","EXT2","EINT_VBUS","EINTG","INT_TIMER","INT_WDT","INT_UNK1",
    "INT_UNK2","INT_UNK3","INT_DMA","INT_ALARM_RTC","INT_PRI_RTC","Reserved","INT_UART","INT_USB_HOST",
    "INT_USB_FUNC","INT_LCDC_0","INT_LCDC_1","INT_CALM","INT_ATA","INT_UART0","INT_SPDIF_OUT","INT_ECC",
    "INT_SDCI","INT_LCD","INT_SPI","INT_IIC","Reserved","INT_MSTICK","INT_ADC_WAKEUP","INT_ADC"
};
#else
static const char * const irqname[] =
{
    "EXT0","EXT1","EXT2","EINT_VBUS","EINTG","INT_TIMERA","INT_WDT","INT_TIMERB",
    "INT_TIMERC","INT_TIMERD","INT_DMA","INT_ALARM_RTC","INT_PRI_RTC","Reserved","INT_UART","INT_USB_HOST",
    "INT_USB_FUNC","INT_LCDC_0","INT_LCDC_1","INT_ECC","INT_CALM","INT_ATA","INT_UART0","INT_SPDIF_OUT",
    "INT_SDCI","INT_LCD","INT_SPI","INT_IIC","Reserved","INT_MSTICK","INT_ADC_WAKEUP","INT_ADC"
};
#endif

static void UIRQ(void)
{
    unsigned int offset = INTOFFSET;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */

    int irq_no = INTOFFSET;
    
    irqvector[irq_no]();

    /* clear interrupt */
    SRCPND = (1 << irq_no);
    INTPND = INTPND;
    
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

void system_init(void)
{
}

void system_reboot(void)
{
#ifdef IPOD_NANO2G
#ifdef HAVE_STORAGE_FLUSH
    storage_flush();
#endif

    /* Reset the SoC */
    asm volatile("msr CPSR_c, #0xd3    \n"
                 "mov r5, #0x110000    \n"
                 "add r5, r5, #0xff    \n"
                 "add r6, r5, #0xa00   \n"
                 "mov r10, #0x3c800000 \n"
                 "str r6, [r10]        \n"
                 "mov r6, #0xff0       \n"
                 "str r6, [r10,#4]     \n"
                 "str r5, [r10]        \n");

    /* Wait for reboot to kick in */
    while(1);
#endif
}

void system_exception_wait(void)
{
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    if (cpu_frequency == frequency)
        return;
    
    /* CPU/COP frequencies can be scaled between Fbus (min) and Fsys (max).
       Fbus should not be set below ~32Mhz with LCD enabled or the display
       will be garbled. */
    if (frequency == CPUFREQ_MAX)
    {
    }
    else if (frequency == CPUFREQ_NORMAL)
    {
    }
    else
    {
    }

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
        "nop      \n\t"
    );

    cpu_frequency = frequency;
}

#endif
