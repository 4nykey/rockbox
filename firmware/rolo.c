/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Randy D. Wood
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "file.h"
#include "mpeg.h"
#include "system.h"
#include "i2c.h"
#include "string.h"
#include "buffer.h"

#define IRQ0_EDGE_TRIGGER 0x80

static void rolo_error(char *text)
{
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO error:");
    lcd_puts_scroll(0, 1, text);
    lcd_update();
    button_get(true);
    button_get(true);
    button_get(true);
    lcd_stop_scroll();
}

/* these are in assembler file "descramble.S" */
extern unsigned short descramble(unsigned char* source, unsigned char* dest, int length);
extern void rolo_restart(unsigned char* source, unsigned char* dest, int length);

/***************************************************************************
 *
 * Name: rolo_load_app(char *filename,int scrambled)
 * Filename must be a fully defined filename including the path and extension
 *
 ***************************************************************************/
int rolo_load(char* filename)
{
    int fd;
    unsigned long length;
    unsigned long file_length;
    unsigned short checksum,file_checksum;
    unsigned char* ramstart = (void*)0x09000000;

    lcd_clear_display();
    lcd_puts(0, 0, "ROLO...");
    lcd_puts(0, 1, "Loading");
    lcd_update();

    mpeg_stop();

    fd = open(filename, O_RDONLY);
    if(-1 == fd) {
        rolo_error("File not found");
        return -1;
    }

    /* Read file length from header and compare to real file length */
    length=lseek(fd,0,SEEK_END)-FIRMWARE_OFFSET_FILE_DATA;
    lseek(fd, FIRMWARE_OFFSET_FILE_LENGTH, SEEK_SET);
    if(read(fd, &file_length, 4) != 4) {
        rolo_error("Error Reading File Length");
        return -1;
    }
    if (length != file_length) {
        rolo_error("File length mismatch");
        return -1;
    }

    /* Read and save checksum */
    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    if (read(fd, &file_checksum, 2) != 2) {
        rolo_error("Error Reading checksum");
        return -1;
    }
    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    /* verify that file can be read and descrambled */
    if ((mp3buf + (2*length)+4) >= mp3end) {
        rolo_error("Not enough room to load file");
        return -1;
    }

    if (read(fd, &mp3buf[length], length) != (int)length) {
        rolo_error("Error Reading File");
        return -1;
    }

    lcd_puts(0, 1, "Descramble");
    lcd_update();

    checksum = descramble(mp3buf + length, mp3buf, length);

    /* Verify checksum against file header */

    if (checksum != file_checksum) {
        rolo_error("Checksum Error");
        return -1;
    }

    lcd_puts(0, 1, "Executing     ");
    lcd_update();

    /* Disable interrupts */
    asm("mov #15<<4,r6\n"
        "ldc r6,sr");

    /* Calling these 2 initialization routines was necessary to get the
       the origional Archos version of the firmware to load and execute. */
    system_init();           /* Initialize system for restart */
    i2c_init();              /* Init i2c bus - it seems like a good idea */
    ICR = IRQ0_EDGE_TRIGGER; /* Make IRQ0 edge triggered */
#ifndef ARCHOS_PLAYER        /* player is to be checked later */
	PAIOR = 0x0FA0;          /* needed when flashed, probably model-specific */
#endif

    rolo_restart(mp3buf, ramstart, length);

    return 0; /* this is never reached */
}
