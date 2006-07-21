/*
 * This config file is for iAudio X5
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 10

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define IRAM_LCDFRAMEBUFFER IDATA_ATTR /* put the lcd frame buffer in IRAM */

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64
#define LCD_REMOTE_DEPTH  1

#define CONFIG_KEYPAD IAUDIO_X5_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have an remote lcd
#define HAVE_REMOTE_LCD
*/

#define CONFIG_LCD LCD_X5

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER_H100 /* port controlled */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* FM Tuner */
#define CONFIG_TUNER       TEA5767
#define CONFIG_TUNER_XTAL  32768


#define BATTERY_CAPACITY_DEFAULT 950 /* default battery capacity */

#define HAVE_TLV320

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5250 */
#define CONFIG_CPU MCF5250

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 950   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2250  /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50    /* capacity increment */
#define BATTERY_TYPES_COUNT  1     /* only one type */
#define BATTERY_SCALE_FACTOR 23437 /* FIX: this value is picked at random */

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50606

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_X5STYLE

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_M5636

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#endif
