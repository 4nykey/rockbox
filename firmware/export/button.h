
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>
#include "config.h"

extern struct event_queue button_queue;
extern long last_keypress;

void button_init (void);
int button_get (bool block);
int button_get_w_tmo(int ticks);

#define	BUTTON_NONE		0x0000

#ifdef HAVE_NEO_KEYPAD

#define BUTTON_UP               0x0080
#define	BUTTON_DOWN		0x0010
#define	BUTTON_LEFT		0x0001
#define	BUTTON_RIGHT		0x0002

#define BUTTON_SELECT           0x0040

#define BUTTON_ON BUTTON_SELECT

#define BUTTON_PROGRAM          0x0020
#define	BUTTON_MENU		0x0004
#define	BUTTON_PLAY		0x0008
#define	BUTTON_STOP		0x0100

#define BUTTON_IR               0x2000
#define BUTTON_REPEAT           0x4000
#define BUTTON_REL              0x8000

#define BUTTON_FLAG_MASK	0xF000
#define BUTTON_MASK		0x0FFF
#define BUTTON_ALL		BUTTON_MASK
#define BUTTON_ALL_FLAGS	BUTTON_FLAG_MASK

#define NEO_IR_BUTTON_POWER     0x0001
#define NEO_IR_BUTTON_SETTING   0x0002
#define NEO_IR_BUTTON_REWIND    0x0004
#define NEO_IR_BUTTON_FFORWARD  0x0008
#define NEO_IR_BUTTON_PLAY      0x0010
#define NEO_IR_BUTTON_VOLUP     0x0020
#define NEO_IR_BUTTON_VOLDN     0x0040
#define NEO_IR_BUTTON_BROWSE    0x0080
#define NEO_IR_BUTTON_EQ        0x0100
#define NEO_IR_BUTTON_MUTE      0x0200
#define NEO_IR_BUTTON_PROGRAM   0x0400
#define NEO_IR_BUTTON_STOP      0x0800
#define NEO_IR_BUTTON_NONE      0x0000

#define NEO_IR_BUTTON_REPEAT    0x1000

#else

/* Shared button codes */
#define	BUTTON_ON		0x0001
#define	BUTTON_UP		0x0010
#define	BUTTON_DOWN		0x0020
#define	BUTTON_LEFT		0x0040
#define	BUTTON_RIGHT		0x0080

/* Button modifiers */
#define	BUTTON_REMOTE		0x2000
#define	BUTTON_REPEAT		0x4000
#define	BUTTON_REL		0x8000

/* remote control buttons */
#define BUTTON_RC_VOL_UP        (0x0008 | BUTTON_REMOTE)
#define BUTTON_RC_VOL_DOWN      (0x0800 | BUTTON_REMOTE)
#define BUTTON_RC_PLAY          (BUTTON_UP | BUTTON_REMOTE)
#define BUTTON_RC_STOP          (BUTTON_DOWN | BUTTON_REMOTE)
#define BUTTON_RC_LEFT          (BUTTON_LEFT | BUTTON_REMOTE)
#define BUTTON_RC_RIGHT         (BUTTON_RIGHT| BUTTON_REMOTE)

#ifdef HAVE_RECORDER_KEYPAD

/* Recorder specific button codes */
#define	BUTTON_OFF		0x0002
#define	BUTTON_PLAY		0x0004
#define	BUTTON_F1		0x0100
#define	BUTTON_F2		0x0200
#define	BUTTON_F3		0x0400

#elif HAVE_PLAYER_KEYPAD

/* Jukebox 6000 and Studio specific button codes */
#define	BUTTON_MENU		0x0002
#define	BUTTON_PLAY		BUTTON_UP
#define	BUTTON_STOP		BUTTON_DOWN

#endif /* HAVE_PLAYER_KEYPAD */

#endif /* HAVE_NEO_KEYPAD */

#endif /* _BUTTON_H_ */

