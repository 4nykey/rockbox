/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Thom Johansen 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PP5020_H__
#define __PP5020_H__

/* All info gleaned and/or copied from the iPodLinux project. */
#define CPU_CTL          (*(volatile unsigned long *)(0x60007000))
#define COP_CTL          (*(volatile unsigned long *)(0x60007004))

#define GPIOA_ENABLE     (*(volatile unsigned long *)(0x6000d000))
#define GPIOB_ENABLE     (*(volatile unsigned long *)(0x6000d004))
#define GPIOC_ENABLE     (*(volatile unsigned long *)(0x6000d008))
#define GPIOD_ENABLE     (*(volatile unsigned long *)(0x6000d00c))
#define GPIOA_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d010))
#define GPIOB_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d014))
#define GPIOC_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d018))
#define GPIOD_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d01c))
#define GPIOA_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d020))
#define GPIOB_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d024))
#define GPIOC_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d028))
#define GPIOD_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d02c))
#define GPIOA_INPUT_VAL  (*(volatile unsigned long *)(0x6000d030))
#define GPIOB_INPUT_VAL  (*(volatile unsigned long *)(0x6000d034))
#define GPIOC_INPUT_VAL  (*(volatile unsigned long *)(0x6000d038))
#define GPIOD_INPUT_VAL  (*(volatile unsigned long *)(0x6000d03c))
#define GPIOA_INT_STAT   (*(volatile unsigned long *)(0x6000d040))
#define GPIOB_INT_STAT   (*(volatile unsigned long *)(0x6000d044))
#define GPIOC_INT_STAT   (*(volatile unsigned long *)(0x6000d048))
#define GPIOD_INT_STAT   (*(volatile unsigned long *)(0x6000d04c))
#define GPIOA_INT_EN     (*(volatile unsigned long *)(0x6000d050))
#define GPIOB_INT_EN     (*(volatile unsigned long *)(0x6000d054))
#define GPIOC_INT_EN     (*(volatile unsigned long *)(0x6000d058))
#define GPIOD_INT_EN     (*(volatile unsigned long *)(0x6000d05c))
#define GPIOA_INT_LEV    (*(volatile unsigned long *)(0x6000d060))
#define GPIOB_INT_LEV    (*(volatile unsigned long *)(0x6000d064))
#define GPIOC_INT_LEV    (*(volatile unsigned long *)(0x6000d068))
#define GPIOD_INT_LEV    (*(volatile unsigned long *)(0x6000d06c))
#define GPIOA_INT_CLR    (*(volatile unsigned long *)(0x6000d070))
#define GPIOB_INT_CLR    (*(volatile unsigned long *)(0x6000d074))
#define GPIOC_INT_CLR    (*(volatile unsigned long *)(0x6000d078))
#define GPIOD_INT_CLR    (*(volatile unsigned long *)(0x6000d07c))

#define GPIOE_ENABLE     (*(volatile unsigned long *)(0x6000d080))
#define GPIOF_ENABLE     (*(volatile unsigned long *)(0x6000d084))
#define GPIOG_ENABLE     (*(volatile unsigned long *)(0x6000d088))
#define GPIOH_ENABLE     (*(volatile unsigned long *)(0x6000d08c))
#define GPIOE_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d090))
#define GPIOF_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d094))
#define GPIOG_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d098))
#define GPIOH_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d09c))
#define GPIOE_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a0))
#define GPIOF_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a4))
#define GPIOG_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a8))
#define GPIOH_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0ac))
#define GPIOE_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b0))
#define GPIOF_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b4))
#define GPIOG_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b8))
#define GPIOH_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0bc))
#define GPIOE_INT_STAT   (*(volatile unsigned long *)(0x6000d0c0))
#define GPIOF_INT_STAT   (*(volatile unsigned long *)(0x6000d0c4))
#define GPIOG_INT_STAT   (*(volatile unsigned long *)(0x6000d0c8))
#define GPIOH_INT_STAT   (*(volatile unsigned long *)(0x6000d0cc))
#define GPIOE_INT_EN     (*(volatile unsigned long *)(0x6000d0d0))
#define GPIOF_INT_EN     (*(volatile unsigned long *)(0x6000d0d4))
#define GPIOG_INT_EN     (*(volatile unsigned long *)(0x6000d0d8))
#define GPIOH_INT_EN     (*(volatile unsigned long *)(0x6000d0dc))
#define GPIOE_INT_LEV    (*(volatile unsigned long *)(0x6000d0e0))
#define GPIOF_INT_LEV    (*(volatile unsigned long *)(0x6000d0e4))
#define GPIOG_INT_LEV    (*(volatile unsigned long *)(0x6000d0e8))
#define GPIOH_INT_LEV    (*(volatile unsigned long *)(0x6000d0ec))
#define GPIOE_INT_CLR    (*(volatile unsigned long *)(0x6000d0f0))
#define GPIOF_INT_CLR    (*(volatile unsigned long *)(0x6000d0f4))
#define GPIOG_INT_CLR    (*(volatile unsigned long *)(0x6000d0f8))
#define GPIOH_INT_CLR    (*(volatile unsigned long *)(0x6000d0fc))

#define GPIOI_ENABLE     (*(volatile unsigned long *)(0x6000d100))
#define GPIOJ_ENABLE     (*(volatile unsigned long *)(0x6000d104))
#define GPIOK_ENABLE     (*(volatile unsigned long *)(0x6000d108))
#define GPIOL_ENABLE     (*(volatile unsigned long *)(0x6000d10c))
#define GPIOI_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d110))
#define GPIOJ_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d114))
#define GPIOK_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d118))
#define GPIOL_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d11c))
#define GPIOI_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d120))
#define GPIOJ_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d124))
#define GPIOK_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d128))
#define GPIOL_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d12c))
#define GPIOI_INPUT_VAL  (*(volatile unsigned long *)(0x6000d130))
#define GPIOJ_INPUT_VAL  (*(volatile unsigned long *)(0x6000d134))
#define GPIOK_INPUT_VAL  (*(volatile unsigned long *)(0x6000d138))
#define GPIOL_INPUT_VAL  (*(volatile unsigned long *)(0x6000d13c))
#define GPIOI_INT_STAT   (*(volatile unsigned long *)(0x6000d140))
#define GPIOJ_INT_STAT   (*(volatile unsigned long *)(0x6000d144))
#define GPIOK_INT_STAT   (*(volatile unsigned long *)(0x6000d148))
#define GPIOL_INT_STAT   (*(volatile unsigned long *)(0x6000d14c))
#define GPIOI_INT_EN     (*(volatile unsigned long *)(0x6000d150))
#define GPIOJ_INT_EN     (*(volatile unsigned long *)(0x6000d154))
#define GPIOK_INT_EN     (*(volatile unsigned long *)(0x6000d158))
#define GPIOL_INT_EN     (*(volatile unsigned long *)(0x6000d15c))
#define GPIOI_INT_LEV    (*(volatile unsigned long *)(0x6000d160))
#define GPIOJ_INT_LEV    (*(volatile unsigned long *)(0x6000d164))
#define GPIOK_INT_LEV    (*(volatile unsigned long *)(0x6000d168))
#define GPIOL_INT_LEV    (*(volatile unsigned long *)(0x6000d16c))
#define GPIOI_INT_CLR    (*(volatile unsigned long *)(0x6000d170))
#define GPIOJ_INT_CLR    (*(volatile unsigned long *)(0x6000d174))
#define GPIOK_INT_CLR    (*(volatile unsigned long *)(0x6000d178))
#define GPIOL_INT_CLR    (*(volatile unsigned long *)(0x6000d17c))

#define DEV_RS (*(volatile unsigned long *)(0x60006004))
#define DEV_EN (*(volatile unsigned long *)(0x6000600c))

#define DEV_SYSTEM  0x4
#define DEV_USB     0x400000

#define DEV_INIT    (*(volatile unsigned long *)(0x70000020))

#define INIT_USB    0x80000000

#define TIMER1_CFG   (*(volatile unsigned long *)(0x60005000))
#define TIMER1_VAL   (*(volatile unsigned long *)(0x60005004))
#define TIMER2_CFG   (*(volatile unsigned long *)(0x60005008))
#define TIMER2_VAL   (*(volatile unsigned long *)(0x6000500c))
#define USEC_TIMER   (*(volatile unsigned long *)(0x60005010))

#define CPU_INT_STAT     (*(volatile unsigned long*)(0x64004000))
#define CPU_HI_INT_STAT  (*(volatile unsigned long*)(0x64004100))
#define CPU_INT_EN       (*(volatile unsigned long*)(0x60004024))
#define CPU_HI_INT_EN    (*(volatile unsigned long*)(0x60004124))
#define CPU_INT_CLR      (*(volatile unsigned long*)(0x60004028))
#define CPU_HI_INT_CLR   (*(volatile unsigned long*)(0x60004128))
       
#define TIMER1_IRQ   0
#define TIMER2_IRQ   1
#define I2S_IRQ      10
#define IDE_IRQ      23
#define GPIO_IRQ     (32+0)
#define SER0_IRQ     (32+4)
#define SER1_IRQ     (32+5)
#define I2C_IRQ      (32+8)

#define TIMER1_MASK  (1 << TIMER1_IRQ)
#define TIMER2_MASK  (1 << TIMER2_IRQ)
#define I2S_MASK     (1 << I2S_IRQ)
#define IDE_MASK     (1 << IDE_IRQ)
#define GPIO_MASK    (1 << (GPIO_IRQ-32))
#define SER0_MASK    (1 << (SER0_IRQ-32))
#define SER1_MASK    (1 << (SER1_IRQ-32))
#define I2C_MASK     (1 << (I2C_IRQ-32))

#define IISCONFIG           (*(volatile unsigned long*)(0x70002800))

#define IISFIFO_CFG         (*(volatile unsigned long*)(0x7000280c))
#define IISFIFO_WR          (*(volatile unsigned long*)(0x70002840))
#define IISFIFO_RD          (*(volatile unsigned long*)(0x70002880))

#define PROC_SLEEP   0x80000000
#define PROC_WAKE    0x0

#endif
