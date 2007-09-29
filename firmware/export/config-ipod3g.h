/*
 * This config file is for the Apple iPod 3g
 */
#define TARGET_TREE /* this target is using the target tree system */

#define IPOD_ARCH 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 7

/* define this if you have recording possibility */
/*#define HAVE_RECORDING*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if the LCD needs to be shutdown */
#define HAVE_LCD_SHUTDOWN

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  2   /* 4 colours - 2bpp */

#define LCD_PIXELFORMAT HORIZONTAL_PACKING

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* LCD contrast */
#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    40 /* Match boot contrast */

#define CONFIG_KEYPAD IPOD_3G_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50605

/* Define if the device can wake from an RTC alarm */
#define HAVE_RTC_ALARM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8731L audio codec */
#define HAVE_WM8731

/* WM8731 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define BATTERY_CAPACITY_DEFAULT 630 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 630  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 10   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* Hardware controlled charging? FIXME */
//#define CONFIG_CHARGING CHARGING_SIMPLE

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5002 */
#define CONFIG_CPU PP5002

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* Define this if you want to use the PP5002 i2c interface */
#define CONFIG_I2C I2C_PP5002

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

#define CONFIG_LCD LCD_IPOD2BPP

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IPODSTYLE
/* actually both firewire and USB, USB isn't handled yet */

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#endif /* SIMULATOR */
