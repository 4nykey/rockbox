/* Note: this is just a basic early version that needs attention and
   corrections! */

/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_M41ST84W

/* define this if you have RTC RAM available for settings */
#define HAVE_RTC_RAM 1

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define CONFIG_KEYPAD GMINI100_PAD

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs. Although in this case
   the codec won't be loadable... */
#define CODEC_SIZE 0x40000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define BATTERY_CAPACITY_DEFAULT 2200 /* default battery capacity */

#ifndef SIMULATOR

/* Define this if you have a TCC730 (CalmRISC16) */
#define CONFIG_CPU TCC730

/* Define this if you have a gmini 100 style LCD */
#define CONFIG_LCD LCD_GMINI100

#define CONFIG_I2C I2C_GMINI

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Type of mobile power, FIXME: probably different, make new type */
#define CONFIG_BATTERY BATT_LIION2200
#define BATTERY_CAPACITY_MIN 2200 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */
#define BATTERY_SCALE_FACTOR 6465
/* chosen values at random -- jyp */

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

#define CPU_FREQ 30000000
/* approximate value (and false in general since freq is variable) */

/* Always enable debug till we stabilize */
#define EMULATOR

#define USB_GMINISTYLE

#define CONFIG_I2C I2C_GMINI

#define CONFIG_BACKLIGHT BL_GMINI

#define GMINI_ARCH

/* Software controlled LED */
#define CONFIG_LED LED_REAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#endif
