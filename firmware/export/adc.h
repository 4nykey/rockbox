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

#include "config.h"

#ifdef TARGET_TREE
#include "adc-target.h"

#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
#define NUM_ADC_CHANNELS 4

#define ADC_BUTTONS 0
#define ADC_REMOTE  1
#define ADC_BATTERY 2
#define ADC_REMOTEDETECT  3
#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */

/* ADC values for different remote control types */
#ifdef IRIVER_H100_SERIES
#define ADCVAL_H300_LCD_REMOTE      0x5E
#define ADCVAL_H100_LCD_REMOTE      0x96
#define ADCVAL_H300_LCD_REMOTE_HOLD 0xCC
#define ADCVAL_H100_LCD_REMOTE_HOLD 0xEA
#else /* H300 series */
#define ADCVAL_H300_LCD_REMOTE      0x35
#define ADCVAL_H100_LCD_REMOTE      0x54
#define ADCVAL_H300_LCD_REMOTE_HOLD 0x72
#define ADCVAL_H100_LCD_REMOTE_HOLD 0x83
#endif

#elif defined(IRIVER_IFP7XX)

#define NUM_ADC_CHANNELS 5

#define ADC_BUTTONS     0
#define ADC_BATTERY     1
#define ADC_BUTTON_PLAY 2
#define ADC_UNREG_POWER ADC_BATTERY /* For compatibility */

#else

#define NUM_ADC_CHANNELS 8

#ifdef HAVE_ONDIO_ADC

#define ADC_MMC_SWITCH          0 /* low values if MMC inserted */
#define ADC_USB_POWER           1 /* USB, reads 0x000 when USB is inserted */
#define ADC_BUTTON_OPTION       2 /* the option button, low value if pressed */
#define ADC_BUTTON_ONOFF        3 /* the on/off button, high value if pressed */
#define ADC_BUTTON_ROW1         4 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_USB_ACTIVE          5 /* USB bridge activity */
#define ADC_UNREG_POWER         7 /* Battery voltage */

#else
/* normal JBR channel assignment */
#define ADC_BATTERY             0 /* Battery voltage always reads 0x3FF due to
                                     silly scaling */
#ifdef HAVE_FMADC
#define ADC_CHARGE_REGULATOR    0 /* Uh, we read the battery voltage? */
#define ADC_USB_POWER           1 /* USB, reads 0x000 when USB is inserted */
#define ADC_BUTTON_OFF          2 /* the off button, high value if pressed */
#define ADC_BUTTON_ON           3 /* the on button, low value if pressed */
#else
#define ADC_CHARGE_REGULATOR    1 /* Regulator reference voltage, should read
                                     about 0x1c0 when charging, else 0x3FF */
#define ADC_USB_POWER           2 /* USB, reads 0x3FF when USB is inserted */
#endif

#define ADC_BUTTON_ROW1         4 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_BUTTON_ROW2         5 /* Used for scanning the keys, different
                                     voltages for different keys */
#define ADC_UNREG_POWER         6 /* Battery voltage with a better scaling */
#define ADC_EXT_POWER           7 /* The external power voltage, 0v or 2.7v */

#endif

#define EXT_SCALE_FACTOR 14800
#endif

unsigned short adc_read(int channel);
void adc_init(void);

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
/* Force a scan now */
unsigned short adc_scan(int channel);
#endif

#endif
