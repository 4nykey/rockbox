/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#ifdef TARGET_TREE
#include "hotswap-target.h"
#else
#include "ata_mmc.h"
#endif

/* helper function to extract n (<=32) bits from an arbitrary position.
   counting from MSB to LSB */
unsigned long card_extract_bits(
    const unsigned long *p, /* the start of the bitfield array */
    unsigned int start,     /* bit no. to start reading  */
    unsigned int size)      /* how many bits to read */
{
    unsigned int long_index = start / 32;
    unsigned int bit_index = start % 32;
    unsigned long result;
    
    result = p[long_index] << bit_index;

    if (bit_index + size > 32)    /* crossing longword boundary */
        result |= p[long_index+1] >> (32 - bit_index);
        
    result >>= 32 - size;

    return result;
}

#ifdef TARGET_TREE
bool card_detect(void)
{
    return card_detect_target();
}

tCardInfo *card_get_info(int card_no)
{
    return card_get_info_target(card_no);
}
#endif
