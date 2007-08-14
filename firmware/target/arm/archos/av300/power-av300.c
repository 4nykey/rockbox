/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: power-x5.c 10967 2006-09-17 09:19:50Z jethead71 $
 *
 * Based on code from the ArchOpen project - http://www.archopen.org
 * Adapted for Rockbox in January 2007
 *
 * Original files: 
 *   lib/target/arch_AV3XX/ata.c
 *
 *   AvLo - linav project
 *   Copyright (c) 2005 by Christophe THOMAS (oxygen77 at free.fr)
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
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"

#ifndef SIMULATOR

void power_init(void)
{
    /* Charger detect */
}

bool charger_inserted(void)
{     
    return false;
}

void ide_power_enable(bool on)
{
    if(on)
        cpld_set_port_3(CPLD_HD_POWER); /* powering up HD */
    else
        cpld_clear_port_3(CPLD_HD_POWER);
}

bool ide_powered(void)
{
    return false;
}

void power_off(void)
{
}

#else

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */

bool tuner_power(bool status)
{
    (void)status;
    return true;
}
