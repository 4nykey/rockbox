/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include "uisw32.h"
#include "kernel.h"
#include "thread-win32.h"
#include "thread.h"

/* (Daniel 2002-10-31) Mingw32 requires this errno variable to be present.
   I'm not quite sure why and I don't know if this breaks the MSVC compile.
   If it does, we should put this within #ifdef __MINGW32__ */
int errno;

int set_irq_level (int level)
{
    static int _lv = 0;
    return (_lv = level);
}

void sleep(int ticks)
{
    Sleep (1000 / HZ * ticks);
}


void yield (void)
{
    Sleep (1); /* prevent busy loop */
    PostThreadMessage (GetWindowThreadProcessId (hGUIWnd,NULL), TM_YIELD, 0, 0);
}

void queue_init(struct event_queue *q)
{
    q->read = 0;
    q->write = 0;
}

void queue_wait(struct event_queue *q, struct event *ev)
{
    while(q->read == q->write)
    {
        switch_thread();
    }

    *ev = q->events[(q->read++) & QUEUE_LENGTH_MASK];
}

void queue_wait_w_tmo(struct event_queue *q, struct event *ev, int ticks)
{
    unsigned int timeout = current_tick + ticks;

    while(q->read == q->write && TIME_BEFORE( current_tick, timeout ))
    {
        sleep(1);
    }

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

void switch_thread (void)
{
    yield ();
}

/* TODO: Implement mutexes for win32 */
void mutex_init(struct mutex *m)
{
    (void)m;
}

void mutex_lock(struct mutex *m)
{
    (void)m;
}

void mutex_unlock(struct mutex *m)
{
    (void)m;
}
