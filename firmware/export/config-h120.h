#define TARGET_TREE /* this target is using the target tree system */
/*
 * This config file is for iriver H120 and H140
 */
#define IRIVER_H100_SERIES 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 0

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  2

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* remote LCD */
#define LCD_REMOTE_WIDTH  128
#define LCD_REMOTE_HEIGHT 64
#define LCD_REMOTE_DEPTH  1

#define LCD_REMOTE_PIXELFORMAT VERTICAL_PACKING

#define CONFIG_KEYPAD IRIVER_H100_PAD

#define CONFIG_REMOTE_KEYPAD H100_REMOTE

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have an remote lcd */
#define HAVE_REMOTE_LCD

#define CONFIG_LCD LCD_S1D15E06

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER_H100 /* port controlled */

/* We can fade the backlight by using PWM */
#define HAVE_BACKLIGHT_PWM_FADING

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

#define CONFIG_TUNER TEA5767
#define CONFIG_TUNER_XTAL 32768

#define HAVE_UDA1380

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define hardware samples rate caps mask */
#define HW_SAMPR_CAPS   (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

#define HAVE_AGC

#define BATTERY_CAPACITY_DEFAULT 1300 /* default battery capacity */

#ifndef SIMULATOR

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU MCF5249

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_COLDFIRE

#define CONFIG_BATTERY BATT_LIPOL1300
#define BATTERY_CAPACITY_MIN 1300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */
#define BATTERY_SCALE_FACTOR 16665 /* FIX: this value is picked at random */

/* Define this if you can run rockbox from flash memory */
#define HAVE_FLASHED_ROCKBOX

/* Define if we have a hardware defect that causes ticking on the audio line */
#define HAVE_REMOTE_LCD_TICKING

/* Hardware controlled charging */
//#define CONFIG_CHARGING CHARGING_SIMPLE
#define CONFIG_CHARGING CHARGING_MONITOR /* FIXME: remove that once monitoring is fixed properly */

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x200000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IRIVERSTYLE

#define HAVE_ATA_LED_CTRL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#define BOOTLOADER_ENTRYPOINT  0x001F0000
#define FLASH_RAMIMAGE_ENTRY   0x00001000
#define FLASH_ROMIMAGE_ENTRY   0x00100000
#define FLASH_MAGIC            0xfbfbfbf2

/* Define this if there is an EEPROM chip */
#define HAVE_EEPROM

/* Define this if the EEPROM chip is used */
#define HAVE_EEPROM_SETTINGS

#endif /* !SIMULATOR */

/* Define this for S/PDIF input available */
#define HAVE_SPDIF_IN

/* Define this for S/PDIF output available */
#define HAVE_SPDIF_OUT

/* Define this if you can control the S/PDIF power */
#define HAVE_SPDIF_POWER

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN

/* Define this if you have a serial port */
/*#define HAVE_SERIAL*/

/** Port-specific settings **/

/* Main LCD backlight brightness range and defaults */
#define MIN_CONTRAST_SETTING        14 /* White screen a bit higher than this */
#define MAX_CONTRAST_SETTING        63 /* Black screen a bit lower than this */
#define DEFAULT_CONTRAST_SETTING    27

/* Remote LCD contrast range and defaults */
#define MIN_REMOTE_CONTRAST_SETTING     5
#define MAX_REMOTE_CONTRAST_SETTING     63
#define DEFAULT_REMOTE_CONTRAST_SETTING 42
