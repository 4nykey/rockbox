/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "kernel.h"
#include "thread.h"
#include "cpu.h"
#include "system.h"
#include "panic.h"
#include "debug.h"
#include "general.h"

/* Make this nonzero to enable more elaborate checks on objects */
#if defined(DEBUG) || defined(SIMULATOR)
#define KERNEL_OBJECT_CHECKS 1 /* Always 1 for DEBUG and sim*/
#else
#define KERNEL_OBJECT_CHECKS 0
#endif

#if KERNEL_OBJECT_CHECKS
#ifdef SIMULATOR
#define KERNEL_ASSERT(exp, msg...) \
    ({ if (!({ exp; })) { DEBUGF(msg); exit(-1); } })
#else
#define KERNEL_ASSERT(exp, msg...) \
    ({ if (!({ exp; })) panicf(msg); })
#endif
#else
#define KERNEL_ASSERT(exp, msg...) ({})
#endif

#if !defined(CPU_PP) || !defined(BOOTLOADER) 
volatile long current_tick SHAREDDATA_ATTR = 0;
#endif

/* List of tick tasks - final element always NULL for termination */
void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);

/* This array holds all queues that are initiated. It is used for broadcast. */
static struct
{
    struct event_queue *queues[MAX_NUM_QUEUES+1];
    IF_COP( struct corelock cl; )
} all_queues SHAREDBSS_ATTR;

/****************************************************************************
 * Standard kernel stuff
 ****************************************************************************/
void kernel_init(void)
{
    /* Init the threading API */
    init_threads();

    /* Other processors will not reach this point in a multicore build.
     * In a single-core build with multiple cores they fall-through and
     * sleep in cop_main without returning. */
    if (CURRENT_CORE == CPU)
    {
        memset(tick_funcs, 0, sizeof(tick_funcs));
        memset(&all_queues, 0, sizeof(all_queues));
        corelock_init(&all_queues.cl);
        tick_start(1000/HZ);
#ifdef KDEV_INIT
        kernel_device_init();
#endif
    }
}

/****************************************************************************
 * Timer tick - Timer initialization and interrupt handler is defined at
 * the target level.
 ****************************************************************************/
int tick_add_task(void (*f)(void))
{
    int oldlevel = disable_irq_save();
    void **arr = (void **)tick_funcs;
    void **p = find_array_ptr(arr, f);

    /* Add a task if there is room */
    if(p - arr < MAX_NUM_TICK_TASKS)
    {
        *p = f; /* If already in list, no problem. */
    }
    else
    {
        panicf("Error! tick_add_task(): out of tasks");
    }

    restore_irq(oldlevel);
    return 0;
}

int tick_remove_task(void (*f)(void))
{
    int oldlevel = disable_irq_save();
    int rc = remove_array_ptr((void **)tick_funcs, f);
    restore_irq(oldlevel);
    return rc;
}

/****************************************************************************
 * Tick-based interval timers/one-shots - be mindful this is not really
 * intended for continuous timers but for events that need to run for a short
 * time and be cancelled without further software intervention.
 ****************************************************************************/
#ifdef INCLUDE_TIMEOUT_API
/* list of active timeout events */
static struct timeout *tmo_list[MAX_NUM_TIMEOUTS+1];

/* timeout tick task - calls event handlers when they expire
 * Event handlers may alter expiration, callback and data during operation.
 */
static void timeout_tick(void)
{
    unsigned long tick = current_tick;
    struct timeout **p = tmo_list;
    struct timeout *curr;

    for(curr = *p; curr != NULL; curr = *(++p))
    {
        int ticks;

        if(TIME_BEFORE(tick, curr->expires))
            continue;

        /* this event has expired - call callback */
        ticks = curr->callback(curr);
        if(ticks > 0)
        {
            curr->expires = tick + ticks; /* reload */
        }
        else
        {
            timeout_cancel(curr); /* cancel */
        }
    }
}

/* Cancels a timeout callback - can be called from the ISR */
void timeout_cancel(struct timeout *tmo)
{
    int oldlevel = disable_irq_save();
    int rc = remove_array_ptr((void **)tmo_list, tmo);

    if(rc >= 0 && *tmo_list == NULL)
    {
        tick_remove_task(timeout_tick); /* Last one - remove task */
    }

    restore_irq(oldlevel);
}

/* Adds a timeout callback - calling with an active timeout resets the
   interval - can be called from the ISR */
void timeout_register(struct timeout *tmo, timeout_cb_type callback,
                      int ticks, intptr_t data)
{
    int oldlevel;
    void **arr, **p;

    if(tmo == NULL)
        return;

    oldlevel = disable_irq_save();

    /* See if this one is already registered */
    arr = (void **)tmo_list;
    p = find_array_ptr(arr, tmo);

    if(p - arr < MAX_NUM_TIMEOUTS)
    {
        /* Vacancy */
        if(*p == NULL)
        {
            /* Not present */
            if(*tmo_list == NULL)
            {
                tick_add_task(timeout_tick); /* First one - add task */
            }

            *p = tmo;
        }

        tmo->callback = callback;
        tmo->data = data;
        tmo->expires = current_tick + ticks;
    }

    restore_irq(oldlevel);
}

#endif /* INCLUDE_TIMEOUT_API */

/****************************************************************************
 * Thread stuff
 ****************************************************************************/
unsigned sleep(unsigned ticks)
{
#if defined(CPU_PP) && defined(BOOTLOADER)
    unsigned stop = USEC_TIMER + ticks * (1000000/HZ);
    while (TIME_BEFORE(USEC_TIMER, stop))
        switch_thread();
#elif defined(CREATIVE_ZVx) && defined(BOOTLOADER)
    /* hacky.. */
    long sleep_ticks = current_tick + ticks + 1;
    while (TIME_BEFORE(current_tick, sleep_ticks))
        switch_thread();
#else
    disable_irq();
    sleep_thread(ticks);
    switch_thread();
#endif
    return 0;
}

void yield(void)
{
#if ((defined(TATUNG_TPJ1022)) && defined(BOOTLOADER))
    /* Some targets don't like yielding in the bootloader */
#else
    switch_thread();
#endif
}

/****************************************************************************
 * Queue handling stuff
 ****************************************************************************/

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/****************************************************************************
 * Sender thread queue structure that aids implementation of priority
 * inheritance on queues because the send list structure is the same as
 * for all other kernel objects:
 *
 * Example state:
 * E0 added with queue_send and removed by thread via queue_wait(_w_tmo)
 * E3 was posted with queue_post
 * 4 events remain enqueued (E1-E4)
 *
 *                                 rd                          wr
 * q->events[]:          |  XX  |  E1  |  E2  |  E3  |  E4  |  XX  |
 * q->send->senders[]:   | NULL |  T1  |  T2  | NULL |  T3  | NULL |
 *                                 \/     \/            \/
 * q->send->list:       >->|T0|<->|T1|<->|T2|<-------->|T3|<-<
 * q->send->curr_sender:    /\
 *
 * Thread has E0 in its own struct queue_event.
 *
 ****************************************************************************/

/* Puts the specified return value in the waiting thread's return value
 * and wakes the thread.
 *
 * A sender should be confirmed to exist before calling which makes it
 * more efficent to reject the majority of cases that don't need this
 * called.
 */
static void queue_release_sender(struct thread_entry * volatile * sender,
                                 intptr_t retval)
{
    struct thread_entry *thread = *sender;

    *sender = NULL;               /* Clear slot. */
#ifdef HAVE_WAKEUP_EXT_CB
    thread->wakeup_ext_cb = NULL; /* Clear callback. */
#endif
    thread->retval = retval;      /* Assign thread-local return value. */
    *thread->bqp = thread;        /* Move blocking queue head to thread since
                                     wakeup_thread wakes the first thread in
                                     the list. */
    wakeup_thread(thread->bqp);
}

/* Releases any waiting threads that are queued with queue_send -
 * reply with 0.
 */
static void queue_release_all_senders(struct event_queue *q)
{
    if(q->send)
    {
        unsigned int i;
        for(i = q->read; i != q->write; i++)
        {
            struct thread_entry **spp =
                &q->send->senders[i & QUEUE_LENGTH_MASK];

            if(*spp)
            {
                queue_release_sender(spp, 0);
            }
        }
    }
}

/* Callback to do extra forced removal steps from sender list in addition
 * to the normal blocking queue removal and priority dis-inherit */
static void queue_remove_sender_thread_cb(struct thread_entry *thread)
{
    *((struct thread_entry **)thread->retval) = NULL;
#ifdef HAVE_WAKEUP_EXT_CB
    thread->wakeup_ext_cb = NULL;
#endif
    thread->retval = 0;
}

/* Enables queue_send on the specified queue - caller allocates the extra
 * data structure. Only queues which are taken to be owned by a thread should
 * enable this however an official owner is not compulsory but must be
 * specified for priority inheritance to operate.
 *
 * Use of queue_wait(_w_tmo) by multiple threads on a queue using synchronous
 * messages results in an undefined order of message replies or possible default
 * replies if two or more waits happen before a reply is done.
 */
void queue_enable_queue_send(struct event_queue *q,
                             struct queue_sender_list *send,
                             unsigned int owner_id)
{
    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    if(send != NULL && q->send == NULL)
    {
        memset(send, 0, sizeof(*send));
#ifdef HAVE_PRIORITY_SCHEDULING
        send->blocker.wakeup_protocol = wakeup_priority_protocol_release;
        send->blocker.priority = PRIORITY_IDLE;
        if(owner_id != 0)
        {
            send->blocker.thread = thread_id_entry(owner_id);
            q->blocker_p = &send->blocker;
        }
#endif
        q->send = send;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    (void)owner_id;
}

/* Unblock a blocked thread at a given event index */
static inline void queue_do_unblock_sender(struct queue_sender_list *send,
                                           unsigned int i)
{
    if(send)
    {
        struct thread_entry **spp = &send->senders[i];

        if(UNLIKELY(*spp))
        {
            queue_release_sender(spp, 0);
        }
    }
}

/* Perform the auto-reply sequence */
static inline void queue_do_auto_reply(struct queue_sender_list *send)
{
    if(send && send->curr_sender)
    {
        /* auto-reply */
        queue_release_sender(&send->curr_sender, 0);
    }
}

/* Moves waiting thread's refrence from the senders array to the
 * current_sender which represents the thread waiting for a reponse to the
 * last message removed from the queue. This also protects the thread from
 * being bumped due to overflow which would not be a valid action since its
 * message _is_ being processed at this point. */
static inline void queue_do_fetch_sender(struct queue_sender_list *send,
                                         unsigned int rd)
{
    if(send)
    {
        struct thread_entry **spp = &send->senders[rd];

        if(*spp)
        {
            /* Move thread reference from array to the next thread
               that queue_reply will release */
            send->curr_sender = *spp;
            (*spp)->retval = (intptr_t)spp;
            *spp = NULL;
        }
        /* else message was posted asynchronously with queue_post */
    }
}
#else
/* Empty macros for when synchoronous sending is not made */
#define queue_release_all_senders(q)
#define queue_do_unblock_sender(send, i)
#define queue_do_auto_reply(send)
#define queue_do_fetch_sender(send, rd)
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

/* Queue must not be available for use during this call */
void queue_init(struct event_queue *q, bool register_queue)
{
    int oldlevel = disable_irq_save();

    if(register_queue)
    {
        corelock_lock(&all_queues.cl);
    }

    corelock_init(&q->cl);
    q->queue = NULL;
    /* What garbage is in write is irrelevant because of the masking design-
     * any other functions the empty the queue do this as well so that
     * queue_count and queue_empty return sane values in the case of a
     * concurrent change without locking inside them. */
    q->read = q->write;
#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    q->send = NULL; /* No message sending by default */
    IF_PRIO( q->blocker_p = NULL; )
#endif

    if(register_queue)
    {
        void **queues = (void **)all_queues.queues;
        void **p = find_array_ptr(queues, q);

        if(p - queues >= MAX_NUM_QUEUES)
        {
            panicf("queue_init->out of queues");
        }

        if(*p == NULL)
        {
            /* Add it to the all_queues array */
            *p = q;
            corelock_unlock(&all_queues.cl);
        }
    }

    restore_irq(oldlevel);
}

/* Queue must not be available for use during this call */
void queue_delete(struct event_queue *q)
{
    int oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
    corelock_lock(&q->cl);

    /* Remove the queue if registered */
    remove_array_ptr((void **)all_queues.queues, q);

    corelock_unlock(&all_queues.cl);

    /* Release thread(s) waiting on queue head */
    thread_queue_wake(&q->queue);

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    if(q->send)
    {
        /* Release threads waiting for replies */
        queue_release_all_senders(q);

        /* Reply to any dequeued message waiting for one */
        queue_do_auto_reply(q->send);

        q->send = NULL;
        IF_PRIO( q->blocker_p = NULL; )
    }
#endif

    q->read = q->write;

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

/* NOTE: multiple threads waiting on a queue head cannot have a well-
   defined release order if timeouts are used. If multiple threads must
   access the queue head, use a dispatcher or queue_wait only. */
void queue_wait(struct event_queue *q, struct queue_event *ev)
{
    int oldlevel;
    unsigned int rd;

#ifdef HAVE_PRIORITY_SCHEDULING
    KERNEL_ASSERT(QUEUE_GET_THREAD(q) == NULL ||
                  QUEUE_GET_THREAD(q) == thread_id_entry(THREAD_ID_CURRENT),
                  "queue_wait->wrong thread\n");
#endif

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* auto-reply */
    queue_do_auto_reply(q->send);

    while(1)
    {
        struct thread_entry *current;

        rd = q->read;
        if (rd != q->write) /* A waking message could disappear */
            break;

        current = thread_id_entry(THREAD_ID_CURRENT);

        IF_COP( current->obj_cl = &q->cl; )
        current->bqp = &q->queue;

        block_thread(current);

        corelock_unlock(&q->cl);
        switch_thread();

        oldlevel = disable_irq_save();
        corelock_lock(&q->cl);
    } 

    q->read = rd + 1;
    rd &= QUEUE_LENGTH_MASK;
    *ev = q->events[rd];

    /* Get data for a waiting thread if one */
    queue_do_fetch_sender(q->send, rd);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_wait_w_tmo(struct event_queue *q, struct queue_event *ev, int ticks)
{
    int oldlevel;
    unsigned int rd, wr;

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
    KERNEL_ASSERT(QUEUE_GET_THREAD(q) == NULL ||
                  QUEUE_GET_THREAD(q) == thread_id_entry(THREAD_ID_CURRENT),
                  "queue_wait_w_tmo->wrong thread\n");
#endif

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Auto-reply */
    queue_do_auto_reply(q->send);

    rd = q->read;
    wr = q->write;
    if (rd == wr && ticks > 0)
    {
        struct thread_entry *current = thread_id_entry(THREAD_ID_CURRENT);

        IF_COP( current->obj_cl = &q->cl; )
        current->bqp = &q->queue;

        block_thread_w_tmo(current, ticks);
        corelock_unlock(&q->cl);    

        switch_thread();

        oldlevel = disable_irq_save();
        corelock_lock(&q->cl);

        rd = q->read;
        wr = q->write;
    }

    /* no worry about a removed message here - status is checked inside
       locks - perhaps verify if timeout or false alarm */
    if (rd != wr)
    {
        q->read = rd + 1;
        rd &= QUEUE_LENGTH_MASK;
        *ev = q->events[rd];
        /* Get data for a waiting thread if one */
        queue_do_fetch_sender(q->send, rd);
    }
    else
    {
        ev->id = SYS_TIMEOUT;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_post(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;

    /* overflow protect - unblock any thread waiting at this index */
    queue_do_unblock_sender(q->send, wr);

    /* Wakeup a waiting thread if any */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

#ifdef HAVE_EXTENDED_MESSAGING_AND_NAME
/* IRQ handlers are not allowed use of this function - we only aim to
   protect the queue integrity by turning them off. */
intptr_t queue_send(struct event_queue *q, long id, intptr_t data)
{
    int oldlevel;
    unsigned int wr;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    wr = q->write++ & QUEUE_LENGTH_MASK;

    q->events[wr].id   = id;
    q->events[wr].data = data;
    
    if(LIKELY(q->send))
    {
        struct queue_sender_list *send = q->send;
        struct thread_entry **spp = &send->senders[wr];
        struct thread_entry *current = thread_id_entry(THREAD_ID_CURRENT);

        if(UNLIKELY(*spp))
        {
            /* overflow protect - unblock any thread waiting at this index */
            queue_release_sender(spp, 0);
        }

        /* Wakeup a waiting thread if any */
        wakeup_thread(&q->queue);

        /* Save thread in slot, add to list and wait for reply */
        *spp = current;
        IF_COP( current->obj_cl = &q->cl; )
        IF_PRIO( current->blocker = q->blocker_p; )
#ifdef HAVE_WAKEUP_EXT_CB
        current->wakeup_ext_cb = queue_remove_sender_thread_cb;
#endif
        current->retval = (intptr_t)spp;
        current->bqp = &send->list;

        block_thread(current);

        corelock_unlock(&q->cl);
        switch_thread();

        return current->retval;
    }

    /* Function as queue_post if sending is not enabled */
    wakeup_thread(&q->queue);

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
    
    return 0;
}

#if 0 /* not used now but probably will be later */
/* Query if the last message dequeued was added by queue_send or not */
bool queue_in_queue_send(struct event_queue *q)
{
    bool in_send;

#if NUM_CORES > 1
    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);
#endif

    in_send = q->send && q->send->curr_sender;

#if NUM_CORES > 1
    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
#endif

    return in_send;
}
#endif

/* Replies with retval to the last dequeued message sent with queue_send */
void queue_reply(struct event_queue *q, intptr_t retval)
{
    if(q->send && q->send->curr_sender)
    {
        struct queue_sender_list *sender;

        int oldlevel = disable_irq_save();
        corelock_lock(&q->cl);

        sender = q->send;

        /* Double-check locking */
        if(LIKELY(sender && sender->curr_sender))
            queue_release_sender(&sender->curr_sender, retval);

        corelock_unlock(&q->cl);
        restore_irq(oldlevel);
    }
}
#endif /* HAVE_EXTENDED_MESSAGING_AND_NAME */

bool queue_peek(struct event_queue *q, struct queue_event *ev)
{
    unsigned int rd;

    if(q->read == q->write)
         return false;

    bool have_msg = false;

    int oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    rd = q->read;
    if(rd != q->write)
    {
        *ev = q->events[rd & QUEUE_LENGTH_MASK];
        have_msg = true;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);

    return have_msg;
}

/* Poll queue to see if a message exists - careful in using the result if
 * queue_remove_from_head is called when messages are posted - possibly use
 * queue_wait_w_tmo(&q, 0) in that case or else a removed message that
 * unsignals the queue may cause an unwanted block */
bool queue_empty(const struct event_queue* q)
{
    return ( q->read == q->write );
}

void queue_clear(struct event_queue* q)
{
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    /* Release all threads waiting in the queue for a reply -
       dequeued sent message will be handled by owning thread */
    queue_release_all_senders(q);

    q->read = q->write;

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

void queue_remove_from_head(struct event_queue *q, long id)
{
    int oldlevel;

    oldlevel = disable_irq_save();
    corelock_lock(&q->cl);

    while(q->read != q->write)
    {
        unsigned int rd = q->read & QUEUE_LENGTH_MASK;

        if(q->events[rd].id != id)
        {
            break;
        }

        /* Release any thread waiting on this message */
        queue_do_unblock_sender(q->send, rd);

        q->read++;
    }

    corelock_unlock(&q->cl);
    restore_irq(oldlevel);
}

/**
 * The number of events waiting in the queue.
 * 
 * @param struct of event_queue
 * @return number of events in the queue
 */
int queue_count(const struct event_queue *q)
{
    return q->write - q->read;
}

int queue_broadcast(long id, intptr_t data)
{
    struct event_queue **p = all_queues.queues;
    struct event_queue *q;

#if NUM_CORES > 1
    int oldlevel = disable_irq_save();
    corelock_lock(&all_queues.cl);
#endif

    for(q = *p; q != NULL; q = *(++p))
    {
        queue_post(q, id, data);
    }

#if NUM_CORES > 1
    corelock_unlock(&all_queues.cl);
    restore_irq(oldlevel);
#endif
   
    return p - all_queues.queues;
}

/****************************************************************************
 * Simple mutex functions ;)
 ****************************************************************************/

static inline void __attribute__((always_inline))
mutex_set_thread(struct mutex *mtx, struct thread_entry *td)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    mtx->blocker.thread = td;
#else
    mtx->thread = td;
#endif
}

static inline struct thread_entry * __attribute__((always_inline))
mutex_get_thread(volatile struct mutex *mtx)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    return mtx->blocker.thread;
#else
    return mtx->thread;
#endif
}

/* Initialize a mutex object - call before any use and do not call again once
 * the object is available to other threads */
void mutex_init(struct mutex *m)
{
    corelock_init(&m->cl);
    m->queue = NULL;
    m->recursion = 0;
    mutex_set_thread(m, NULL);
#ifdef HAVE_PRIORITY_SCHEDULING
    m->blocker.priority = PRIORITY_IDLE;
    m->blocker.wakeup_protocol = wakeup_priority_protocol_transfer;
    m->no_preempt = false;
#endif
}

/* Gain ownership of a mutex object or block until it becomes free */
void mutex_lock(struct mutex *m)
{
    struct thread_entry *current = thread_id_entry(THREAD_ID_CURRENT);

    if(current == mutex_get_thread(m))
    {
        /* current thread already owns this mutex */
        m->recursion++;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* must read thread again inside cs (a multiprocessor concern really) */
    if(LIKELY(mutex_get_thread(m) == NULL))
    {
        /* lock is open */
        mutex_set_thread(m, current);
        corelock_unlock(&m->cl);
        return;
    }

    /* block until the lock is open... */
    IF_COP( current->obj_cl = &m->cl; )
    IF_PRIO( current->blocker = &m->blocker; )
    current->bqp = &m->queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&m->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

/* Release ownership of a mutex object - only owning thread must call this */
void mutex_unlock(struct mutex *m)
{
    /* unlocker not being the owner is an unlocking violation */
    KERNEL_ASSERT(mutex_get_thread(m) == thread_id_entry(THREAD_ID_CURRENT),
                  "mutex_unlock->wrong thread (%s != %s)\n",
                  mutex_get_thread(m)->name,
                  thread_id_entry(THREAD_ID_CURRENT)->name);

    if(m->recursion > 0)
    {
        /* this thread still owns lock */
        m->recursion--;
        return;
    }

    /* lock out other cores */
    corelock_lock(&m->cl);

    /* transfer to next queued thread if any */
    if(LIKELY(m->queue == NULL))
    {
        /* no threads waiting - open the lock */
        mutex_set_thread(m, NULL);
        corelock_unlock(&m->cl);
        return;
    }
    else
    {
        const int oldlevel = disable_irq_save();
        /* Tranfer of owning thread is handled in the wakeup protocol
         * if priorities are enabled otherwise just set it from the
         * queue head. */
        IFN_PRIO( mutex_set_thread(m, m->queue); )
        IF_PRIO( unsigned int result = ) wakeup_thread(&m->queue);
        restore_irq(oldlevel);

        corelock_unlock(&m->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
        if((result & THREAD_SWITCH) && !m->no_preempt)
            switch_thread();
#endif
    }
}

/****************************************************************************
 * Simple semaphore functions ;)
 ****************************************************************************/
#ifdef HAVE_SEMAPHORE_OBJECTS
void semaphore_init(struct semaphore *s, int max, int start)
{
    KERNEL_ASSERT(max > 0 && start >= 0 && start <= max,
                  "semaphore_init->inv arg\n");
    s->queue = NULL;
    s->max = max;
    s->count = start;
    corelock_init(&s->cl);
}

void semaphore_wait(struct semaphore *s)
{
    struct thread_entry *current;

    corelock_lock(&s->cl);

    if(LIKELY(--s->count >= 0))
    {
        /* wait satisfied */
        corelock_unlock(&s->cl);
        return;
    }

    /* too many waits - block until dequeued... */
    current = thread_id_entry(THREAD_ID_CURRENT);

    IF_COP( current->obj_cl = &s->cl; )
    current->bqp = &s->queue;

    disable_irq();
    block_thread(current);

    corelock_unlock(&s->cl);

    /* ...and turn control over to next thread */
    switch_thread();
}

void semaphore_release(struct semaphore *s)
{
    IF_PRIO( unsigned int result = THREAD_NONE; )

    corelock_lock(&s->cl);

    if(s->count < s->max && ++s->count <= 0)
    {
        /* there should be threads in this queue */
        KERNEL_ASSERT(s->queue != NULL, "semaphore->wakeup\n");
        /* a thread was queued - wake it up */
        int oldlevel = disable_irq_save();
        IF_PRIO( result = ) wakeup_thread(&s->queue);
        restore_irq(oldlevel);
    }

    corelock_unlock(&s->cl);

#ifdef HAVE_PRIORITY_SCHEDULING
    if(result & THREAD_SWITCH)
        switch_thread();
#endif
}
#endif /* HAVE_SEMAPHORE_OBJECTS */

#ifdef HAVE_WAKEUP_OBJECTS
/****************************************************************************
 * Lightweight IRQ-compatible wakeup object
 */

/* Initialize the wakeup object */
void wakeup_init(struct wakeup *w)
{
    w->queue = NULL;
    w->signalled = false;
    IF_COP( corelock_init(&w->cl); )
}

/* Wait for a signal blocking indefinitely or for a specified period */
int wakeup_wait(struct wakeup *w, int timeout)
{
    int ret = OBJ_WAIT_SUCCEEDED; /* Presume success */
    int oldlevel = disable_irq_save();

    corelock_lock(&w->cl);

    if(LIKELY(!w->signalled && timeout != TIMEOUT_NOBLOCK))
    {
        struct thread_entry * current = thread_id_entry(THREAD_ID_CURRENT);

        IF_COP( current->obj_cl = &w->cl; )
        current->bqp = &w->queue;
        
        if (timeout != TIMEOUT_BLOCK)
            block_thread_w_tmo(current, timeout);
        else
            block_thread(current);

        corelock_unlock(&w->cl);
        switch_thread();

        oldlevel = disable_irq_save();
        corelock_lock(&w->cl);
    }

    if(UNLIKELY(!w->signalled))
    {
        /* Timed-out or failed */
        ret = (timeout != TIMEOUT_BLOCK) ?
            OBJ_WAIT_TIMEDOUT : OBJ_WAIT_FAILED;
    }

    w->signalled = false;  /* Reset */

    corelock_unlock(&w->cl);
    restore_irq(oldlevel);

    return ret;
}

/* Signal the thread waiting or leave the signal if the thread hasn't 
 * waited yet.
 *
 * returns THREAD_NONE or THREAD_OK
 */
int wakeup_signal(struct wakeup *w)
{
    int oldlevel = disable_irq_save();
    int ret;

    corelock_lock(&w->cl);

    w->signalled = true;
    ret = wakeup_thread(&w->queue);

    corelock_unlock(&w->cl);
    restore_irq(oldlevel);

    return ret;
}
#endif /* HAVE_WAKEUP_OBJECTS */
