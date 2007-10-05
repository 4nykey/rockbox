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

/* DRAM starts at 0x10000000, but in Rockbox we remap it to 0x00000000 */
#define DRAM_START       0x10000000

/* Processor ID */
#define PROCESSOR_ID     (*(volatile unsigned long *)(0x60000000))

#define PROC_ID_CPU      0x55
#define PROC_ID_COP      0xaa

/* Mailboxes */
/* Each processor has two mailboxes it can write to and two which
   it can read from.  We define the first to be for sending messages
   and the second for replying to messages */
#define CPU_MESSAGE      (*(volatile unsigned long *)(0x60001000))
#define COP_MESSAGE      (*(volatile unsigned long *)(0x60001004))
#define CPU_REPLY        (*(volatile unsigned long *)(0x60001008))
#define COP_REPLY        (*(volatile unsigned long *)(0x6000100c))
#define MBOX_CONTROL     (*(volatile unsigned long *)(0x60001010))

/* Interrupts */
#define CPU_INT_STAT        (*(volatile unsigned long*)(0x60004000))
#define COP_INT_STAT        (*(volatile unsigned long*)(0x60004004))
#define CPU_FIQ_STAT        (*(volatile unsigned long*)(0x60004008))
#define COP_FIQ_STAT        (*(volatile unsigned long*)(0x6000400c))

#define INT_STAT            (*(volatile unsigned long*)(0x60004010))
#define INT_FORCED_STAT     (*(volatile unsigned long*)(0x60004014))
#define INT_FORCED_SET      (*(volatile unsigned long*)(0x60004018))
#define INT_FORCED_CLR      (*(volatile unsigned long*)(0x6000401c))

#define CPU_INT_EN_STAT     (*(volatile unsigned long*)(0x60004020))
#define CPU_INT_EN          (*(volatile unsigned long*)(0x60004024))
#define CPU_INT_CLR         (*(volatile unsigned long*)(0x60004028))
#define CPU_INT_PRIORITY    (*(volatile unsigned long*)(0x6000402c))

#define COP_INT_EN_STAT     (*(volatile unsigned long*)(0x60004030))
#define COP_INT_EN          (*(volatile unsigned long*)(0x60004034))
#define COP_INT_CLR         (*(volatile unsigned long*)(0x60004038))
#define COP_INT_PRIORITY    (*(volatile unsigned long*)(0x6000403c))

#define CPU_HI_INT_STAT     (*(volatile unsigned long*)(0x60004100))
#define COP_HI_INT_STAT     (*(volatile unsigned long*)(0x60004104))
#define CPU_HI_FIQ_STAT     (*(volatile unsigned long*)(0x60004108))
#define COP_HI_FIQ_STAT     (*(volatile unsigned long*)(0x6000410c))

#define HI_INT_STAT         (*(volatile unsigned long*)(0x60004110))
#define HI_INT_FORCED_STAT  (*(volatile unsigned long*)(0x60004114))
#define HI_INT_FORCED_SET   (*(volatile unsigned long*)(0x60004118))
#define HI_INT_FORCED_CLR   (*(volatile unsigned long*)(0x6000411c))

#define CPU_HI_INT_EN_STAT  (*(volatile unsigned long*)(0x60004120))
#define CPU_HI_INT_EN       (*(volatile unsigned long*)(0x60004124))
#define CPU_HI_INT_CLR      (*(volatile unsigned long*)(0x60004128))
#define CPU_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000412c))
 
#define COP_HI_INT_EN_STAT  (*(volatile unsigned long*)(0x60004130))
#define COP_HI_INT_EN       (*(volatile unsigned long*)(0x60004134))
#define COP_HI_INT_CLR      (*(volatile unsigned long*)(0x60004138))
#define COP_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000413c))

#define TIMER1_IRQ   0
#define TIMER2_IRQ   1
#define MAILBOX_IRQ  4
#define I2S_IRQ      10
#define IDE_IRQ      23
#define USB_IRQ      24
#define FIREWIRE_IRQ 25
#define HI_IRQ       30
#define GPIO_IRQ     (32+0)
#define SER0_IRQ     (32+4)
#define SER1_IRQ     (32+5)
#define I2C_IRQ      (32+8)

#define TIMER1_MASK   (1 << TIMER1_IRQ)
#define TIMER2_MASK   (1 << TIMER2_IRQ)
#define MAILBOX_MASK  (1 << MAILBOX_IRQ)
#define I2S_MASK      (1 << I2S_IRQ)
#define IDE_MASK      (1 << IDE_IRQ)
#define USB_MASK      (1 << USB_IRQ)
#define FIREWIRE_MASK (1 << FIREWIRE_IRQ)
#define HI_MASK       (1 << HI_IRQ)
#define GPIO_MASK     (1 << (GPIO_IRQ-32))
#define SER0_MASK     (1 << (SER0_IRQ-32))
#define SER1_MASK     (1 << (SER1_IRQ-32))
#define I2C_MASK      (1 << (I2C_IRQ-32))

/* Timers */
#define TIMER1_CFG   (*(volatile unsigned long *)(0x60005000))
#define TIMER1_VAL   (*(volatile unsigned long *)(0x60005004))
#define TIMER2_CFG   (*(volatile unsigned long *)(0x60005008))
#define TIMER2_VAL   (*(volatile unsigned long *)(0x6000500c))
#define USEC_TIMER   (*(volatile unsigned long *)(0x60005010))
#define RTC          (*(volatile unsigned long *)(0x60005014))

/* Device Controller */
#define DEV_RS       (*(volatile unsigned long *)(0x60006004))
#define DEV_OFF_MASK (*(volatile unsigned long *)(0x60006008))
#define DEV_EN       (*(volatile unsigned long *)(0x6000600c))

#define DEV_SYSTEM      0x00000004
#define DEV_SER0        0x00000040
#define DEV_SER1        0x00000080
#define DEV_I2S         0x00000800
#define DEV_I2C         0x00001000
#define DEV_ATA         0x00004000
#define DEV_OPTO        0x00010000
#define DEV_PIEZO       0x00010000
#define DEV_USB         0x00400000
#define DEV_FIREWIRE    0x00800000
#define DEV_IDE0        0x02000000
#define DEV_LCD         0x04000000

/* clock control */
#define CLOCK_SOURCE    (*(volatile unsigned long *)(0x60006020))
#define PLL_CONTROL     (*(volatile unsigned long *)(0x60006034))
#define PLL_STATUS      (*(volatile unsigned long *)(0x6000603c))
#define CLCD_CLOCK_SRC  (*(volatile unsigned long *)(0x600060a0))

/* Processors Control */
#define CPU_CTL          (*(volatile unsigned long *)(0x60007000))
#define COP_CTL          (*(volatile unsigned long *)(0x60007004))

#define PROC_SLEEP     0x80000000
#define PROC_WAIT      0x40000000
#define PROC_WAIT_CLR  0x20000000
#define PROC_CNT_START 0x08000000
#define PROC_WAKE      0x00000000
/**
 * This is based on some quick but sound experiments on PP5022C.
 * CPU/COP_CTL bitmap:
 *   [31] - sleep until an interrupt occurs
 *   [30] - wait for cycle countdown to 0
 *   [29] - wait for cycle countdown to 0
 *          behaves identically to bit 30 unless bit 30 is set as well
 *          in which case this bit is cleared at the end of the count
 *   [28] - unknown - no execution effect observed yet
 *   [27] - begin cycle countdown
 * [26:8] - semaphore flags for core communication ?
 *          no execution effect observed yet
 *          [11:8] seem to often be set to the core's own ID
 *                 nybble when sleeping - 0x5 or 0xa.
 *  [7:0] - W: number of cycles to skip on next instruction
 *          R: cycles remaining
 * Executing on CPU
 *   CPU_CTL = 0x68000080
 *   nop
 * stalls the nop for 128 cycles
 * Reading CPU_CTL after the nop will return 0x48000000
 */

/* Cache Control */
#define CACHE_PRIORITY   (*(volatile unsigned long *)(0x60006044))
#define CACHE_CTL        (*(volatile unsigned long *)(0x6000c000))
#define CACHE_MASK       (*(volatile unsigned long *)(0xf000f040))
#define CACHE_OPERATION  (*(volatile unsigned long *)(0xf000f044))
#define CACHE_FLUSH_MASK (*(volatile unsigned long *)(0xf000f048))

/* CACHE_CTL bits */
#define CACHE_CTL_DISABLE    0x0000
#define CACHE_CTL_ENABLE     0x0001
#define CACHE_CTL_RUN        0x0002
#define CACHE_CTL_INIT       0x0004
#define CACHE_CTL_VECT_REMAP 0x0010
#define CACHE_CTL_READY      0x4000
#define CACHE_CTL_BUSY       0x8000
/* CACHE_OPERATION bits */
#define CACHE_OP_FLUSH       0x0002
#define CACHE_OP_INVALIDATE  0x0004

/* GPIO Ports */
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

/* Device initialization */
#define PP_VER1          (*(volatile unsigned long *)(0x70000000))
#define PP_VER2          (*(volatile unsigned long *)(0x70000004))
#define STRAP_OPT_A      (*(volatile unsigned long *)(0x70000008))
#define STRAP_OPT_B      (*(volatile unsigned long *)(0x7000000c))
#define BUS_WIDTH_MASK   0x00000010
#define RAM_TYPE_MASK    0x000000c0
#define ROM_TYPE_MASK    0x00000008

#define DEV_INIT         (*(volatile unsigned long *)(0x70000020))
/* some timing that needs to be handled during clock setup */
#define DEV_TIMING1      (*(volatile unsigned long *)(0x70000034))
#define XMB_NOR_CFG      (*(volatile unsigned long *)(0x70000038))
#define XMB_RAM_CFG      (*(volatile unsigned long *)(0x7000003c))

#define INIT_USB         0x80000000

/* I2S */
#define IISCONFIG           (*(volatile unsigned long*)(0x70002800))
#define IISFIFO_CFG         (*(volatile unsigned long*)(0x7000280c))
#define IISFIFO_WR          (*(volatile unsigned long*)(0x70002840))
#define IISFIFO_RD          (*(volatile unsigned long*)(0x70002880))

/* Serial Controller */
#define SERIAL0             (*(volatile unsigned long*)(0x70006000))
#define SERIAL1             (*(volatile unsigned long*)(0x70006040))

/* I2C */
#define I2C_BASE            0x7000c000

/* EIDE Controller */
#define IDE_BASE            0xc3000000

#define IDE0_PRI_TIMING0    (*(volatile unsigned long*)(0xc3000000))
#define IDE0_PRI_TIMING1    (*(volatile unsigned long*)(0xc3000004))
#define IDE0_SEC_TIMING0    (*(volatile unsigned long*)(0xc3000008))
#define IDE0_SEC_TIMING1    (*(volatile unsigned long*)(0xc300000c))

#define IDE1_PRI_TIMING0    (*(volatile unsigned long*)(0xc3000010))
#define IDE1_PRI_TIMING1    (*(volatile unsigned long*)(0xc3000014))
#define IDE1_SEC_TIMING0    (*(volatile unsigned long*)(0xc3000018))
#define IDE1_SEC_TIMING1    (*(volatile unsigned long*)(0xc300001c))

#define IDE0_CFG            (*(volatile unsigned long*)(0xc3000028))
#define IDE1_CFG            (*(volatile unsigned long*)(0xc300002c))

#define IDE0_CNTRLR_STAT    (*(volatile unsigned long*)(0xc30001e0))

/* USB controller */
#define USB_BASE            0xc5000000

/* Firewire Controller */
#define FIREWIRE_BASE       0xc6000000

/* Memory controller */
#define CACHE_BASE              (*(volatile unsigned long*)(0xf0000000))
/* 0xf0000000-0xf0001fff */
#define CACHE_DATA_BASE         (*(volatile unsigned long*)(0xf0000000))
/* 0xf0002000-0xf0003fff */
#define CACHE_DATA_MIRROR_BASE  (*(volatile unsigned long*)(0xf0002000))
/* 0xf0004000-0xf0007fff */
#define CACHE_STATUS_BASE       (*(volatile unsigned long*)(0xf0004000))
#define CACHE_FLUSH_BASE        (*(volatile unsigned long*)(0xf0008000))
#define CACHE_INVALID_BASE      (*(volatile unsigned long*)(0xf000c000))
#define MMAP_PHYS_READ_MASK     0x0100
#define MMAP_PHYS_WRITE_MASK    0x0200
#define MMAP_PHYS_DATA_MASK     0x0400
#define MMAP_PHYS_CODE_MASK     0x0800
#define MMAP_FIRST              (*(volatile unsigned long*)(0xf000f000))
#define MMAP_LAST               (*(volatile unsigned long*)(0xf000f03c))
#define MMAP0_LOGICAL           (*(volatile unsigned long*)(0xf000f000))
#define MMAP0_PHYSICAL          (*(volatile unsigned long*)(0xf000f004))
#define MMAP1_LOGICAL           (*(volatile unsigned long*)(0xf000f008))
#define MMAP1_PHYSICAL          (*(volatile unsigned long*)(0xf000f00c))
#define MMAP2_LOGICAL           (*(volatile unsigned long*)(0xf000f010))
#define MMAP2_PHYSICAL          (*(volatile unsigned long*)(0xf000f014))
#define MMAP3_LOGICAL           (*(volatile unsigned long*)(0xf000f018))
#define MMAP3_PHYSICAL          (*(volatile unsigned long*)(0xf000f01c))
#define MMAP4_LOGICAL           (*(volatile unsigned long*)(0xf000f020))
#define MMAP4_PHYSICAL          (*(volatile unsigned long*)(0xf000f024))
#define MMAP5_LOGICAL           (*(volatile unsigned long*)(0xf000f028))
#define MMAP5_PHYSICAL          (*(volatile unsigned long*)(0xf000f02c))
#define MMAP6_LOGICAL           (*(volatile unsigned long*)(0xf000f030))
#define MMAP6_PHYSICAL          (*(volatile unsigned long*)(0xf000f034))
#define MMAP7_LOGICAL           (*(volatile unsigned long*)(0xf000f038))
#define MMAP7_PHYSICAL          (*(volatile unsigned long*)(0xf000f03c))

#endif /* __PP5020_H__ */
