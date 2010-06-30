/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "ata.h"
#include "usb.h"
#include "usb_core.h"
#include "usb_drv.h"
#include "usb-target.h"
#include "mc13783.h"
#include "ccm-imx31.h"
#include "avic-imx31.h"
#include "power-gigabeat-s.h"

static int usb_status = USB_EXTRACTED;

static void enable_transceiver(bool enable)
{
    if (enable)
    {
        if (GPIO1_DR & (1 << 30))
        {
            bitclr32(&GPIO3_DR, (1 << 16)); /* Reset ISP1504 */
            sleep(HZ/100);
            bitset32(&GPIO3_DR, (1 << 16));
            sleep(HZ/10);
            bitclr32(&GPIO1_DR, (1 << 30)); /* Select ISP1504 */
        }
    }
    else
    {
        bitset32(&GPIO1_DR, (1 << 30)); /* Deselect ISP1504 */
    }
}

/* Read the immediate state of the cable from the PMIC */
bool usb_plugged(void)
{
    return mc13783_read(MC13783_INTERRUPT_SENSE0) & MC13783_USB4V4S;
}

void usb_connect_event(void)
{
    int status = usb_plugged() ? USB_INSERTED : USB_EXTRACTED;
    usb_status = status;
    /* Notify power that USB charging is potentially available */
    charger_usb_detect_event(status);
    usb_status_event((status == USB_INSERTED) ? USB_POWERED : USB_UNPOWERED);
}

int usb_detect(void)
{
    return usb_status;
}

void usb_init_device(void)
{
    /* Do one-time inits */
    usb_drv_startup();

    /* Initially poll */
    usb_connect_event();

    /* Enable PMIC event */
    mc13783_enable_event(MC13783_USB_EVENT);
}

void usb_enable(bool on)
{
    /* Module clock should be on since since this could be called with
     * OFF initially and writing module registers would hardlock otherwise. */
    ccm_module_clock_gating(CG_USBOTG, CGM_ON_RUN_WAIT);
    enable_transceiver(true);

    if (on)
    {
        usb_core_init();
    }
    else
    {
        usb_core_exit();
        enable_transceiver(false);
        ccm_module_clock_gating(CG_USBOTG, CGM_OFF);
    }
}

void usb_attach(void)
{
    usb_drv_attach();
}

static void __attribute__((interrupt("IRQ"))) USB_OTG_HANDLER(void)
{
    usb_drv_int(); /* Call driver handler */
}

void usb_drv_int_enable(bool enable)
{
    if (enable)
    {
        avic_enable_int(INT_USB_OTG, INT_TYPE_IRQ, INT_PRIO_DEFAULT,
                        USB_OTG_HANDLER);
    }
    else
    {
        avic_disable_int(INT_USB_OTG);
    }
}

/* Called during the bus reset interrupt when in detect mode */
void usb_drv_usb_detect_event(void)
{
    if (usb_drv_powered())
        usb_status_event(USB_INSERTED);
}
