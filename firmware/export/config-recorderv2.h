/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a Recorder style 10-key keyboard */
#define HAVE_RECORDER_KEYPAD 1

/* define this if you have a real-time clock */
#define HAVE_RTC 1

/* Define this if you have a MAS3587F */
#define HAVE_MAS3587F

/* Define this if you have a FM Recorder key system */
#define HAVE_FMADC 1

/* Define this if you have a LiIon battery */
#define HAVE_LIION

/* Define this if you need to power on ATA */
#define NEEDS_ATA_POWER_ON

/* Define this if battery voltage can only be measured with ATA powered */
#define NEED_ATA_POWER_BATT_MEASURE

/* Define this to the CPU frequency */
#define CPU_FREQ      11059200

/* Battery scale factor (guessed, seems to be 1,25 * value from recorder) */
#define BATTERY_SCALE_FACTOR 8081

/* Define this if you control power on PB5 (instead of the OFF button) */
#define HAVE_POWEROFF_ON_PB5

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* FM recorders can wake up from RTC alarm */
#define HAVE_ALARM_MOD 1

/* Define this if you have an FM Radio */
#define HAVE_FMRADIO 1

/* How to detect USB */
#define USB_FMRECORDERSTYLE 1

/* Define this if the platform has batteries */
#define HAVE_BATTERIES 1

/* The start address index for ROM builds */
#define ROM_START 0x12010

/* Define this for programmable LED available */
#define HAVE_LED

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

