/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: irivertools.h
 *
 * Copyright (C) 2007 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef IRIVERTOOLS_H_INCLUDED
#define IRIVERTOOLS_H_INCLUDED

#include "rbutil.h"
#include "installlog.h"
#include "md5sum.h"

#define ESTF_SIZE 32

struct sumpairs {
    char *unpatched;
    char *patched;
};

/* precalculated checksums for H110/H115 */
static struct sumpairs h100pairs[] = {
#include "h100sums.h"
};

/* precalculated checksums for H120/H140 */
static struct sumpairs h120pairs[] = {
#include "h120sums.h"
};

/* precalculated checksums for H320/H340 */
static struct sumpairs h300pairs[] = {
#include "h300sums.h"
};


enum striptype
{
  STRIP_NONE,
  STRIP_HEADER_CHECKSUM,
  STRIP_HEADER_CHECKSUM_ESTF
};

/* protos for iriver.c */

int intable(char *md5, struct sumpairs *table, int len);

bool PatchFirmware(wxString firmware,wxString bootloader,int series, int table_entry);


#endif // IRIVERTOOLS_H_INCLUDED
