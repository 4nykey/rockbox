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
#ifndef _ADC_H_
#define _ADC_H_

#define NUM_ADC_CHANNELS 8

#define ADC_BATTERY             0 /* Battery voltage always reads 0x3FF due to
                                     silly scaling */
#define ADC_CHARGE_REGULATOR    1 /* Regulator reference voltage, should read
                                     about 0x1c0 when charging, else 0x3FF */
#define ADC_USB_POWER           2 /* USB, reads 0x3FF when USB is inserted */

#define ADC_BUTTON_ROW1         4 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_BUTTON_ROW2         5 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_UNREG_POWER         6 /* Battery voltage with a better scaling */
#define ADC_EXT_POWER           7 /* The external power voltage, V=X*0.0148 */

#ifdef ARCHOS_RECORDER
#define BATTERY_SCALE_FACTOR 6465
#else
#define BATTERY_SCALE_FACTOR 6546
#endif

#define EXT_SCALE_FACTOR 14800

unsigned short adc_read(int channel);
void adc_init(void);

#endif
