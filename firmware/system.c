/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include <lcd.h>
#include <led.h>

#define default_interrupt(name,number) \
  extern __attribute__((weak,alias("UIE" #number))) void name (void); void UIE##number (void)
#define reserve_interrupt(number) \
  void UIE##number (void)

extern void reset_pc (void);
extern void reset_sp (void);

reserve_interrupt (          0);
reserve_interrupt (          1);
reserve_interrupt (          2);
reserve_interrupt (          3);
default_interrupt (GII,      4);
reserve_interrupt (          5);
default_interrupt (ISI,      6);
reserve_interrupt (          7);
reserve_interrupt (          8);
default_interrupt (CPUAE,    9);
default_interrupt (DMAAE,   10);
default_interrupt (NMI,     11);
default_interrupt (UB,      12);
reserve_interrupt (         13);
reserve_interrupt (         14);
reserve_interrupt (         15);
reserve_interrupt (         16); // TCB #0
reserve_interrupt (         17); // TCB #1
reserve_interrupt (         18); // TCB #2
reserve_interrupt (         19); // TCB #3
reserve_interrupt (         20); // TCB #4
reserve_interrupt (         21); // TCB #5
reserve_interrupt (         22); // TCB #6
reserve_interrupt (         23); // TCB #7
reserve_interrupt (         24); // TCB #8
reserve_interrupt (         25); // TCB #9
reserve_interrupt (         26); // TCB #10
reserve_interrupt (         27); // TCB #11
reserve_interrupt (         28); // TCB #12
reserve_interrupt (         29); // TCB #13
reserve_interrupt (         30); // TCB #14
reserve_interrupt (         31); // TCB #15
default_interrupt (TRAPA32, 32);
default_interrupt (TRAPA33, 33);
default_interrupt (TRAPA34, 34);
default_interrupt (TRAPA35, 35);
default_interrupt (TRAPA36, 36);
default_interrupt (TRAPA37, 37);
default_interrupt (TRAPA38, 38);
default_interrupt (TRAPA39, 39);
default_interrupt (TRAPA40, 40);
default_interrupt (TRAPA41, 41);
default_interrupt (TRAPA42, 42);
default_interrupt (TRAPA43, 43);
default_interrupt (TRAPA44, 44);
default_interrupt (TRAPA45, 45);
default_interrupt (TRAPA46, 46);
default_interrupt (TRAPA47, 47);
default_interrupt (TRAPA48, 48);
default_interrupt (TRAPA49, 49);
default_interrupt (TRAPA50, 50);
default_interrupt (TRAPA51, 51);
default_interrupt (TRAPA52, 52);
default_interrupt (TRAPA53, 53);
default_interrupt (TRAPA54, 54);
default_interrupt (TRAPA55, 55);
default_interrupt (TRAPA56, 56);
default_interrupt (TRAPA57, 57);
default_interrupt (TRAPA58, 58);
default_interrupt (TRAPA59, 59);
default_interrupt (TRAPA60, 60);
default_interrupt (TRAPA61, 61);
default_interrupt (TRAPA62, 62);
default_interrupt (TRAPA63, 63);
default_interrupt (IRQ0,    64);
default_interrupt (IRQ1,    65);
default_interrupt (IRQ2,    66);
default_interrupt (IRQ3,    67);
default_interrupt (IRQ4,    68);
default_interrupt (IRQ5,    69);
default_interrupt (IRQ6,    70);
default_interrupt (IRQ7,    71);
default_interrupt (DEI0,    72);
reserve_interrupt (         73);
default_interrupt (DEI1,    74);
reserve_interrupt (         75);
default_interrupt (DEI2,    76);
reserve_interrupt (         77);
default_interrupt (DEI3,    78);
reserve_interrupt (         79);
default_interrupt (IMIA0,   80);
default_interrupt (IMIB0,   81);
default_interrupt (OVI0,    82);
reserve_interrupt (         83);
default_interrupt (IMIA1,   84);
default_interrupt (IMIB1,   85);
default_interrupt (OVI1,    86);
reserve_interrupt (         87);
default_interrupt (IMIA2,   88);
default_interrupt (IMIB2,   89);
default_interrupt (OVI2,    90);
reserve_interrupt (         91);
default_interrupt (IMIA3,   92);
default_interrupt (IMIB3,   93);
default_interrupt (OVI3,    94);
reserve_interrupt (         95);
default_interrupt (IMIA4,   96);
default_interrupt (IMIB4,   97);
default_interrupt (OVI4,    98);
reserve_interrupt (         99);
default_interrupt (REI0,   100);
default_interrupt (RXI0,   101);
default_interrupt (TXI0,   102);
default_interrupt (TEI0,   103);
default_interrupt (REI1,   104);
default_interrupt (RXI1,   105);
default_interrupt (TXI1,   106);
default_interrupt (TEI1,   107);
reserve_interrupt (        108);
default_interrupt (ADITI,  109);

void (*vbr[]) (void) __attribute__ ((section (".sram.vbr"))) = 
{
    /*** 0-1 Power-on Reset ***/
		
    reset_pc,reset_sp,

    /*** 2-3 Manual Reset ***/
		
    reset_pc,reset_sp,

    /*** 4 General Illegal Instruction ***/

    GII,

    /*** 5 Reserved ***/

    UIE5,

    /*** 6 Illegal Slot Instruction ***/

    ISI,

    /*** 7-8 Reserved ***/

    UIE7,UIE8,

    /*** 9 CPU Address Error ***/

    CPUAE,

    /*** 10 DMA Address Error ***/

    DMAAE,

    /*** 11 NMI ***/

    NMI,

    /*** 12 User Break ***/

    UB,

    /*** 13-31 Reserved ***/

    UIE13,UIE14,UIE15,UIE16,UIE17,UIE18,UIE19,UIE20,UIE21,UIE22,UIE23,UIE24,UIE25,UIE26,UIE27,UIE28,UIE29,UIE30,UIE31,
  
    /*** 32-63 TRAPA #20...#3F ***/

    TRAPA32,TRAPA33,TRAPA34,TRAPA35,TRAPA36,TRAPA37,TRAPA38,TRAPA39,TRAPA40,TRAPA41,TRAPA42,TRAPA43,TRAPA44,TRAPA45,TRAPA46,TRAPA47,TRAPA48,TRAPA49,TRAPA50,TRAPA51,TRAPA52,TRAPA53,TRAPA54,TRAPA55,TRAPA56,TRAPA57,TRAPA58,TRAPA59,TRAPA60,TRAPA61,TRAPA62,TRAPA63,
  
    /*** 64-71 IRQ0-7 ***/ 

    IRQ0,IRQ1,IRQ2,IRQ3,IRQ4,IRQ5,IRQ6,IRQ7,
  
    /*** 72 DMAC0 ***/ 
  
    DEI0,
    
    /*** 73 Reserved ***/

    UIE73,

    /*** 74 DMAC1 ***/ 

    DEI1,
    
    /*** 75 Reserved ***/

    UIE75,

    /*** 76 DMAC2 ***/ 

    DEI2,
    
    /*** 77 Reserved ***/

    UIE77,

    /*** 78 DMAC3 ***/ 

    DEI3,
    
    /*** 79 Reserved ***/

    UIE79, 
  
    /*** 80-82 ITU0 ***/
  
    IMIA0,IMIB0,OVI0,
    
    /*** 83 Reserved ***/

    UIE83, 

    /*** 84-86 ITU1 ***/
  
    IMIA1,IMIB1,OVI1,
    
    /*** 87 Reserved ***/

    UIE87, 
  
    /*** 88-90 ITU2 ***/
  
    IMIA2,IMIB2,OVI2,
    
    /*** 91 Reserved ***/

    UIE91, 
  
    /*** 92-94 ITU3 ***/
  
    IMIA3,IMIB3,OVI3,
    
    /*** 95 Reserved ***/

    UIE95, 
  
    /*** 96-98 ITU4 ***/
  
    IMIA4,IMIB4,OVI4,
    
    /*** 99 Reserved ***/

    UIE99,
    
    /*** 100-103 SCI0 ***/ 
  
    REI0,RXI0,TXI0,TEI0,
  
    /*** 104-107 SCI1 ***/ 
  
    REI1,RXI1,TXI1,TEI1,
  
    /*** 108 Parity Control Unit ***/
  
    UIE108,       
  
    /*** 109 AD Converter ***/
  
    ADITI
    
};


void system_reboot (void)
{
    cli ();

    asm volatile ("ldc\t%0,vbr" : : "r"(0));

    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    IPRD = 0;
    IPRE = 0;
    ICR = 0;

    asm volatile ("jmp @%0; mov.l @%1,r15" : :
		  "r"(*(char*)0),"r"(4));
}

void UIE (unsigned int pc) /* Unexpected Interrupt or Exception */
{
    unsigned int i,n;
    lcd_stop ();
    asm volatile ("sts\tpr,%0" : "=r"(n));
    n = (n - (unsigned)UIE0 - 4)>>2; // get exception or interrupt number
    lcd_start ();
    lcd_goto (0,0); lcd_puts ("** UIE00 **");
    lcd_goto (0,1); lcd_puts ("AT 00000000");
    lcd_goto (6,0); lcd_puthex (n,2);
    lcd_goto (3,1); lcd_puthex (pc,8); /* or pc - 4 !? */
    lcd_stop ();

    while (1)
    {
        led_toggle ();

        for (i = 0; i < 240000; ++i);
    }
}

asm (
    "_UIE0:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE1:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE2:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE3:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE4:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE5:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE6:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE7:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE8:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE9:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE10:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE11:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE12:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE13:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE14:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE15:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE16:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE17:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE18:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE19:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE20:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE21:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE22:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE23:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE24:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE25:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE26:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE27:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE28:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE29:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE30:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE31:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE32:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE33:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE34:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE35:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE36:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE37:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE38:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE39:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE40:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE41:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE42:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE43:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE44:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE45:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE46:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE47:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE48:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE49:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE50:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE51:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE52:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE53:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE54:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE55:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE56:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE57:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE58:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE59:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE60:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE61:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE62:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE63:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE64:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE65:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE66:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE67:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE68:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE69:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE70:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE71:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE72:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE73:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE74:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE75:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE76:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE77:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE78:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE79:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE80:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE81:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE82:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE83:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE84:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE85:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE86:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE87:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE88:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE89:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE90:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE91:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE92:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE93:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE94:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE95:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE96:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE97:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE98:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE99:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE100:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE101:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE102:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE103:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE104:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE105:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE106:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE107:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE108:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE109:\tbsr\t_UIE\n\tmov.l\t@r15+,r4");
