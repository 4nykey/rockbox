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
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "thread.h"
#include "sh7034.h"
#include "system.h"
#include "panic.h"

long current_tick = 0;

static void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

static void tick_start(unsigned int interval_in_ms);

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct event_queue *all_queues[32];
static int num_queues;

void sleep(int ticks) __attribute__ ((section(".icode")));
void queue_wait(struct event_queue *q, struct event *ev) __attribute__ ((section(".icode")));

/****************************************************************************
 * Standard kernel stuff
 ****************************************************************************/
void kernel_init(void)
{
    /* Init the threading API */
    init_threads();
    
    memset(tick_funcs, 0, sizeof(tick_funcs));

    num_queues = 0;
    memset(all_queues, 0, sizeof(all_queues));
    
    tick_start(1000/HZ);
}

void sleep(int ticks)
{
    /* Always sleep at least 1 tick */
    int timeout = current_tick + ticks + 1;

    while (TIME_BEFORE( current_tick, timeout )) {
        sleep_thread();
    }
    wake_up_thread();
}

void yield(void)
{
    switch_thread();
    wake_up_thread();
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/
void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;

    /* Add it to the all_queues array */
    all_queues[num_queues++] = q;
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    while(q->read == q->write)
    {
        sleep_thread();
    }
    wake_up_thread();

    *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks)
{
    unsigned int timeout = current_tick + ticks;

    while(q->read == q->write && TIME_BEFORE( current_tick, timeout ))
    {
        sleep_thread();
    }
    wake_up_thread();

    if(q->read != q->write)
    {
        *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }
}

void queue_post(struct event_queue *q, int id, void *data)
{
    int wr;
    int oldlevel;

    oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);
    wr = (q->write++) & QUEUE_LENGTH_MASK;

    q->events[wr].id = id;
    q->events[wr].data = data;
    set_irq_level(oldlevel);
}

bool queue_empty(struct event_queue* q)
{
    return ( q->read == q->write );
}

int queue_broadcast(int id, void *data)
{
   int i;

   for(i = 0;i < num_queues;i++)
   {
      queue_post(all_queues[i], id, data);
   }
   
   return num_queues;
}

/****************************************************************************
 * Timer tick
 ****************************************************************************/
static void tick_start(unsigned int interval_in_ms)
{
    unsigned int count;

    count = FREQ * interval_in_ms / 1000 / 8;

    if(count > 0xffff)
    {
        panicf("Error! The tick interval is too long (%d ms)\n",
               interval_in_ms);
        return;
    }
    
    /* We are using timer 0 */
    
    TSTR &= ~0x01; /* Stop the timer */
    TSNC &= ~0x01; /* No synchronization */
    TMDR &= ~0x01; /* Operate normally */

    TCNT0 = 0;   /* Start counting at 0 */
    GRA0 = count;
    TCR0 = 0x23; /* Clear at GRA match, sysclock/8 */

    /* Enable interrupt on level 1 */
    IPRC = (IPRC & ~0x00f0) | 0x0010;
    
    TSR0 &= ~0x01;
    TIER0 = 0xf9; /* Enable GRA match interrupt */

    TSTR |= 0x01; /* Start timer 1 */
}

#pragma interrupt
void IMIA0(void)
{
    int i;

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
        {
            tick_funcs[i]();
        }
    }

    current_tick++;
    wake_up_thread();

    TSR0 &= ~0x01;
}

int tick_add_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Add a task if there is room */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == NULL)
        {
            tick_funcs[i] = f;
            set_irq_level(oldlevel);
            return 0;
        }
    }
    set_irq_level(oldlevel);
    panicf("Error! tick_add_task(): out of tasks");
    return -1;
}

int tick_remove_task(void (*f)(void))
{
    int i;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Remove a task if it is there */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i] == f)
        {
            tick_funcs[i] = NULL;
            set_irq_level(oldlevel);
            return 0;
        }
    }
    
    set_irq_level(oldlevel);
    return -1;
}

/****************************************************************************
 * Simple mutex functions
 ****************************************************************************/
void mutex_init(struct mutex *m)
{
    m->locked = false;
}

void mutex_lock(struct mutex *m)
{
    /* Wait until the lock is open... */
    while(m->locked)
        sleep_thread();
    wake_up_thread();

    /* ...and lock it */
    m->locked = true;
}

void mutex_unlock(struct mutex *m)
{
    m->locked = false;
}
