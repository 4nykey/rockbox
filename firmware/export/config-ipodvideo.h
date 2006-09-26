/*
 * This config file is for the Apple iPod Video
 */
#define IPOD_ARCH 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 5

/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define CONFIG_KEYPAD IPOD_4G_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_PCF50605
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8758 audio codec */
#define HAVE_WM8758

#define AB_REPEAT_ENABLE 1

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IPODNANO /* port controlled */

/* We can fade the backlight by using PWM */
#define HAVE_BACKLIGHT_PWM_FADING

#define BATTERY_CAPACITY_DEFAULT 1300 /* default battery capacity */

#ifndef SIMULATOR

/* The Nano actually has a PP5021 - but it's register compatible with
   the 5020 so Rockbox doesn't care. */
/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 1300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */
#define BATTERY_SCALE_FACTOR 5865

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

#define CONFIG_LCD LCD_IPODVIDEO

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IPODSTYLE

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#endif
