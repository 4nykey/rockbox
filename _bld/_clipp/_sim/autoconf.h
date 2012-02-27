/* This header was made by configure */
#ifndef __BUILD_AUTOCONF_H
#define __BUILD_AUTOCONF_H

/* Define endianess for the target or simulator platform */
#define ROCKBOX_LITTLE_ENDIAN 1

/* Define this if you build rockbox to support the logf logging and display */
#undef ROCKBOX_HAS_LOGF

/* Define this if you want logf to output to the serial port */
#undef LOGF_SERIAL

/* Define this to record a chart with timings for the stages of boot */
#undef DO_BOOTCHART

/* optional define for a backlight modded Ondio */


/* optional define for FM radio mod for iAudio M5 */


/* optional define for ATA poweroff on Player */


/* optional defines for RTC mod for h1x0 */



/* the threading backend we use */
#define HAVE_SIGALTSTACK_THREADS

/* lcd dimensions for application builds from configure */



/* root of Rockbox */
#define ROCKBOX_DIR "/.rockbox"
#define ROCKBOX_SHARE_PATH ""
#define ROCKBOX_BINARY_PATH ""
#define ROCKBOX_LIBRARY_PATH ""

#endif /* __BUILD_AUTOCONF_H */
