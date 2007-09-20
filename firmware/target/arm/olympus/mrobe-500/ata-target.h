/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
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
 
#ifndef ATA_TARGET_H
#define ATA_TARGET_H

/* Plain C read & write loops */
#define PREFER_C_READING
#define PREFER_C_WRITING
#if !defined(BOOTLOADER)
//#define ATA_OPTIMIZED_READING
//void copy_read_sectors(unsigned char* buf, int wordcount);
#endif

#define ATA_IOBASE      0x50000000
#define ATA_DATA        (*((volatile unsigned short*)(ATA_IOBASE+0xa0)))
#define ATA_ERROR       (*((volatile unsigned char*)(ATA_IOBASE + 0xa2)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(ATA_IOBASE + 0x24)))
#define ATA_SECTOR      (*((volatile unsigned char*)(ATA_IOBASE + 0x26)))
#define ATA_LCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x28)))
#define ATA_HCYL        (*((volatile unsigned char*)(ATA_IOBASE + 0x2A)))
#define ATA_SELECT      (*((volatile unsigned char*)(ATA_IOBASE + 0x2C)))
#define ATA_COMMAND     (*((volatile unsigned char*)(ATA_IOBASE + 0x2E)))
#define ATA_CONTROL     (*((volatile unsigned char*)(ATA_IOBASE + 0x9C)))

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define ERROR_ABRT      0x04

#define WRITE_PATTERN1 0xa5
#define WRITE_PATTERN2 0x5a
#define WRITE_PATTERN3 0xaa
#define WRITE_PATTERN4 0x55

#define READ_PATTERN1 0xa5
#define READ_PATTERN2 0x5a
#define READ_PATTERN3 0xaa
#define READ_PATTERN4 0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

#define SET_REG(reg,val) reg = (val)
#define SET_16BITREG(reg,val) reg = (val)

void ata_reset(void);
void ata_device_init(void);
bool ata_is_coldstart(void);

#endif
