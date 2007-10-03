/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
/*
 * This config file is for toshiba Gigabeat F
 */
#define TARGET_TREE /* this target is using the target tree system */

#define CONFIG_SDRAM_START 0x00900000

#define OLYMPUS_MROBE_500 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 22

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* LCD dimensions */
#define LCD_WIDTH  480
#define LCD_HEIGHT 640
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if your LCD can be enabled/disabled */
//#define HAVE_LCD_ENABLE

#define CONFIG_KEYPAD MROBE500_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
//#define CONFIG_RTC RTC_RX5X348AB

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          0   /* 0.5 mA */
#define MAX_DIM_BRIGHTNESS_SETTING      15  /* highest 'dimness' */
#define MAX_BRIGHTNESS_SETTING          63  /* 32 mA */
#define DEFAULT_BRIGHTNESS_SETTING      39  /* 20 mA */
#define DEFAULT_DIMNESS_SETTING          9  /* 5 mA */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
//#define HAVE_WM8751

/* Define this if you want to use the adaptive bass capibility of the 8751 */
/* #define USE_ADAPTIVE_BASS */

#define HW_SAMPR_CAPS (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | \
                       SAMPR_CAP_11)

#define BATTERY_CAPACITY_DEFAULT 1100 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU DM320

/* Define this if you have a Texas Instruments TSC2100 touch screen */
#define HAVE_TSC2100

/* Define this if you want to use coldfire's i2c interface */
//#define CONFIG_I2C I2C_S3C2440

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 16934400

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_MROBE500

/* define this if the backlight can be set to a brightness */
#define HAVE_BACKLIGHT_SET_FADING
#define __BACKLIGHT_INIT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */

#define BOOTFILE_EXT "mrobe500"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#endif
