/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Bj�rn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include "ata.h"
#include "debug.h"
#include "fat.h"
#ifdef HAVE_HOTSWAP
#include "hotswap.h"
#endif
#include "file.h" /* for release_dirs() */
#include "dir.h" /* for release_files() */
#include "disk.h"

/* Partition table entry layout:
   -----------------------
   0: 0x80 - active
   1: starting head
   2: starting sector
   3: starting cylinder
   4: partition type
   5: end head
   6: end sector
   7: end cylinder
   8-11: starting sector (LBA)
   12-15: nr of sectors in partition
*/

#define BYTES2INT32(array,pos)					\
    ((long)array[pos] | ((long)array[pos+1] << 8 ) |		\
     ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

static struct partinfo part[8]; /* space for 4 partitions on 2 drives */
static int vol_drive[NUM_VOLUMES]; /* mounted to which drive (-1 if none) */

struct partinfo* disk_init(IF_MV_NONVOID(int drive))
{
    int i;
    unsigned char sector[512];
#ifdef HAVE_MULTIVOLUME
    /* For each drive, start at a different position, in order not to destroy 
       the first entry of drive 0. 
       That one is needed to calculate config sector position. */
    struct partinfo* pinfo = &part[drive*4];
    if ((size_t)drive >= sizeof(part)/sizeof(*part)/4)
        return NULL; /* out of space in table */
#else
    struct partinfo* pinfo = part;
#endif

    ata_read_sectors(IF_MV2(drive,) 0,1,&sector);

    /* check that the boot sector is initialized */
    if ( (sector[510] != 0x55) ||
         (sector[511] != 0xaa)) {
        DEBUGF("Bad boot sector signature\n");
        return NULL;
    }

    /* parse partitions */
    for ( i=0; i<4; i++ ) {
        unsigned char* ptr = sector + 0x1be + 16*i;
        pinfo[i].type  = ptr[4];
        pinfo[i].start = BYTES2INT32(ptr, 8);
        pinfo[i].size  = BYTES2INT32(ptr, 12);

        DEBUGF("Part%d: Type %02x, start: %08lx size: %08lx\n",
               i,pinfo[i].type,pinfo[i].start,pinfo[i].size);

        /* extended? */
        if ( pinfo[i].type == 5 ) {
            /* not handled yet */
        }
    }

    return pinfo;
}

struct partinfo* disk_partinfo(int partition)
{
    return &part[partition];
}

int disk_mount_all(void)
{
    int mounted;
    int i;
    
#ifdef HAVE_MMC
    card_enable_monitoring(false);
#endif

    fat_init(); /* reset all mounted partitions */
    for (i=0; i<NUM_VOLUMES; i++)
        vol_drive[i] = -1; /* mark all as unassigned */

    mounted = disk_mount(0);
#ifdef HAVE_HOTSWAP
    if (card_detect())
    {
        mounted += disk_mount(1); /* try 2nd "drive", too */
    }
#ifdef HAVE_MMC
    card_enable_monitoring(true);
#endif
#endif

    return mounted;
}

static int get_free_volume(void)
{
    int i;
    for (i=0; i<NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == -1) /* unassigned? */
            return i;
    }

    return -1; /* none found */
}

int disk_mount(int drive)
{
    int i;
    int mounted = 0; /* reset partition-on-drive flag */
    int volume = get_free_volume();
    struct partinfo* pinfo = disk_init(IF_MV(drive));

    if (pinfo == NULL)
    {
        return 0;
    }
#ifndef ELIO_TPJ1022
    /* The Elio's hard drive has no partition table and probing for partitions causes
       Rockbox to crash - so we temporarily disable the probing until we fix the
       real problem. */
    for (i=0; volume != -1 && i<4; i++)
    {
#ifdef MAX_LOG_SECTOR_SIZE
        int j;

        for (j = 1; j <= (MAX_LOG_SECTOR_SIZE/SECTOR_SIZE); j <<= 1)
        {
            if (!fat_mount(IF_MV2(volume,) IF_MV2(drive,) pinfo[i].start * j))
            {
                pinfo[i].start *= j;
                pinfo[i].size *= j;
                mounted++;
                vol_drive[volume] = drive; /* remember the drive for this volume */
                volume = get_free_volume(); /* prepare next entry */
                break;
            }
        }
#else
        if (!fat_mount(IF_MV2(volume,) IF_MV2(drive,) pinfo[i].start))
        {
            mounted++;
            vol_drive[volume] = drive; /* remember the drive for this volume */
            volume = get_free_volume(); /* prepare next entry */
        }
#endif
    }
#endif

    if (mounted == 0 && volume != -1) /* none of the 4 entries worked? */
    {   /* try "superfloppy" mode */
        DEBUGF("No partition found, trying to mount sector 0.\n");
        if (!fat_mount(IF_MV2(volume,) IF_MV2(drive,) 0))
        {
            mounted = 1;
            vol_drive[volume] = drive; /* remember the drive for this volume */
        }
    }
    return mounted;
}

#ifdef HAVE_HOTSWAP
int disk_unmount(int drive)
{
    int unmounted = 0;
    int i;
    for (i=0; i<NUM_VOLUMES; i++)
    {
        if (vol_drive[i] == drive)
        {   /* force releasing resources */
            vol_drive[i] = -1; /* mark unused */
            unmounted++;
            release_files(i);
            release_dirs(i);
            fat_unmount(i, false);
        }
    }

    return unmounted;
}
#endif /* #ifdef HAVE_HOTSWAP */
