/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 J�rg Hohensohn
 *
 * This module collects the Talkbox and voice UI functions.
 * (Talkbox reads directory names from mp3 clips called thumbnails,
 *  the voice UI lets menus and screens "talk" from a voicefont in memory.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __TALK_H__
#define __TALK_H__

#include <stdbool.h>

enum {
    UNIT_INT = 1, /* plain number */
    UNIT_SIGNED,  /* number with mandatory sign (even if positive) */
    UNIT_MS,      /* milliseconds */
    UNIT_SEC,     /* seconds */
    UNIT_MIN,     /* minutes */
    UNIT_HOUR,    /* hours */
    UNIT_KHZ,     /* kHz */
    UNIT_DB,      /* dB, mandatory sign */
    UNIT_PERCENT, /* % */
    UNIT_MB,      /* megabyte */
    UNIT_GB,      /* gigabyte */
    UNIT_MAH,     /* milliAmp hours */
    UNIT_PIXEL,   /* pixels */
    UNIT_PER_SEC, /* per second */
    UNIT_HERTZ,   /* hertz */
    UNIT_LAST     /* END MARKER */
};

#define UNIT_SHIFT (32-4) /* this many bits left from UNIT_xx enum */

/* make a "talkable" ID from number + unit
   unit is upper 4 bits, number the remaining (in regular 2's complement) */
#define TALK_ID(n,u) ((u)<<UNIT_SHIFT | ((n) & ~(-1<<UNIT_SHIFT))) 

/* convenience macro to have both string and ID as arguments */
#define STR(id) str(id), id


void talk_init(void);
int talk_buffer_steal(void); /* claim the mp3 buffer e.g. for play/record */
int talk_id(int id, bool enqueue); /* play a voice ID from voicefont */
int talk_file(char* filename, bool enqueue); /* play a thumbnail from file */
int talk_number(int n, bool enqueue); /* say a number */
int talk_value(int n, int unit, bool enqueue); /* say a numeric value */

#endif /* __TALK_H__ */
