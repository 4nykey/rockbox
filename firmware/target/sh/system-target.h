/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#define or_b(mask, address) \
  asm                                       \
    ("or.b %0,@(r0,gbr)"                    \
     :                                      \
     : /* %0 */ I_CONSTRAINT((char)(mask)), \
       /* %1 */ "z"(address-GBR))

#define and_b(mask, address) \
  asm                                       \
    ("and.b %0,@(r0,gbr)"                   \
     :                                      \
     : /* %0 */ I_CONSTRAINT((char)(mask)), \
       /* %1 */ "z"(address-GBR))

#define xor_b(mask, address) \
  asm                                        \
    ("xor.b %0,@(r0,gbr)"                    \
     :                                       \
     : /* %0 */ I_CONSTRAINT((char)(mask)),  \
       /* %1 */ "z"(address-GBR))


/****************************************************************************
 * Interrupt level setting
 * The level is left shifted 4 bits
 ****************************************************************************/
#define HIGHEST_IRQ_LEVEL (15<<4)

static inline int set_irq_level(int level)
{
    int i;
    /* Read the old level and set the new one */
    asm volatile ("stc sr, %0" : "=r" (i));
    asm volatile ("ldc %0, sr" : : "r" (level));
    return i;
}

static inline uint16_t swap16(uint16_t value)
  /*
    result[15..8] = value[ 7..0];
    result[ 7..0] = value[15..8];
  */
{
    uint16_t result;
    asm volatile ("swap.b\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline uint32_t SWAW32(uint32_t value)
  /*
    result[31..16] = value[15.. 0];
    result[15.. 0] = value[31..16];
  */
{
    uint32_t result;
    asm volatile ("swap.w\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline uint32_t swap32(uint32_t value)
  /*
    result[31..24] = value[ 7.. 0];
    result[23..16] = value[15.. 8];
    result[15.. 8] = value[23..16];
    result[ 7.. 0] = value[31..24];
  */
{
    asm volatile ("swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n"
                  "swap.b\t%0,%0\n" : "+r"(value));
    return value;
}

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    asm volatile ("swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n"
                  "swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n" : "+r"(value));
    return value;
}

#define invalidate_icache()

#endif /* SYSTEM_TARGET_H */
