/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004-2006 dionoea (Antoine Cellerier)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************************************************************
Solitaire by dionoea
Graphics & Fix Bugs by Ben Basha

use arrows to move the cursor
use ON to select cards, move cards, reveal hidden cards, ...
use PLAY to move a card from the remains' stack to the top of the cursor
use F1 to put card under cursor on one of the 4 final stacks
use F2 to un-select card if a card was selected, else draw 3 new cards
    out of the remains' stack
use F3 to put card on top of the remains' stack on one of the 4 final stacks

*****************************************************************************/

#include "plugin.h"
#include "configfile.h"
#include "button.h"
#include "lcd.h"


#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
static struct plugin_api* rb;

#define min(a,b) (a<b?a:b)


/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SOL_QUIT BUTTON_OFF
#define SOL_UP BUTTON_UP
#define SOL_DOWN BUTTON_DOWN
#define SOL_LEFT BUTTON_LEFT
#define SOL_RIGHT BUTTON_RIGHT
#define SOL_MOVE BUTTON_ON
#define SOL_DRAW BUTTON_F2
#define SOL_REM2CUR BUTTON_PLAY
#define SOL_CUR2STACK BUTTON_F1
#define SOL_REM2STACK BUTTON_F3
#define SOL_MENU_RUN BUTTON_RIGHT
#define SOL_MENU_RUN2 BUTTON_PLAY
#define SOL_MENU_INFO BUTTON_F1
#define SOL_MENU_INFO2 BUTTON_F2
#define SOL_MENU_INFO3 BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SOL_QUIT BUTTON_OFF
#define SOL_UP_PRE BUTTON_UP
#define SOL_UP (BUTTON_UP | BUTTON_REL)
#define SOL_DOWN_PRE BUTTON_DOWN
#define SOL_DOWN (BUTTON_DOWN | BUTTON_REL)
#define SOL_LEFT_PRE BUTTON_LEFT
#define SOL_LEFT (BUTTON_LEFT | BUTTON_REL)
#define SOL_RIGHT_PRE BUTTON_RIGHT
#define SOL_RIGHT (BUTTON_RIGHT | BUTTON_REL)
#define SOL_MOVE_PRE BUTTON_MENU
#define SOL_MOVE (BUTTON_MENU | BUTTON_REL)
#define SOL_DRAW_PRE BUTTON_MENU
#define SOL_DRAW (BUTTON_MENU | BUTTON_REPEAT)
#define SOL_REM2CUR_PRE BUTTON_LEFT
#define SOL_REM2CUR (BUTTON_LEFT | BUTTON_REPEAT)
#define SOL_CUR2STACK_PRE BUTTON_RIGHT
#define SOL_CUR2STACK (BUTTON_RIGHT | BUTTON_REPEAT)
#define SOL_REM2STACK_PRE BUTTON_UP
#define SOL_REM2STACK (BUTTON_UP | BUTTON_REPEAT)
#define SOL_MENU_RUN BUTTON_RIGHT
#define SOL_MENU_INFO BUTTON_MENU

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SOL_QUIT BUTTON_OFF
#define SOL_UP BUTTON_UP
#define SOL_DOWN BUTTON_DOWN
#define SOL_LEFT BUTTON_LEFT
#define SOL_RIGHT BUTTON_RIGHT
#define SOL_MOVE_PRE BUTTON_SELECT
#define SOL_MOVE (BUTTON_SELECT | BUTTON_REL)
#define SOL_DRAW BUTTON_MODE
#define SOL_REM2CUR (BUTTON_LEFT | BUTTON_ON)
#define SOL_CUR2STACK (BUTTON_SELECT | BUTTON_REPEAT)
#define SOL_REM2STACK (BUTTON_RIGHT | BUTTON_ON)
#define SOL_MENU_RUN BUTTON_SELECT
#define SOL_MENU_RUN2 BUTTON_RIGHT
#define SOL_MENU_INFO BUTTON_MODE
#define SOL_OPT BUTTON_ON
#define SOL_REM BUTTON_REC

#define SOL_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define SOL_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define SOL_UP BUTTON_MENU
#define SOL_DOWN BUTTON_PLAY
#define SOL_LEFT BUTTON_LEFT
#define SOL_RIGHT BUTTON_RIGHT
#define SOL_MOVE BUTTON_SELECT
#define SOL_DRAW (BUTTON_SELECT | BUTTON_PLAY)
#define SOL_REM2CUR (BUTTON_SELECT | BUTTON_LEFT)
#define SOL_CUR2STACK (BUTTON_SELECT | BUTTON_RIGHT)
#define SOL_REM2STACK (BUTTON_LEFT | BUTTON_RIGHT)
#define SOL_MENU_RUN BUTTON_SELECT
#define SOL_MENU_INFO (BUTTON_PLAY | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define SOL_QUIT BUTTON_POWER
#define SOL_UP BUTTON_UP
#define SOL_DOWN BUTTON_DOWN
#define SOL_LEFT BUTTON_LEFT
#define SOL_RIGHT BUTTON_RIGHT
#define SOL_MOVE BUTTON_SELECT
#define SOL_DRAW BUTTON_PLAY
#define SOL_REM2CUR (BUTTON_REC | BUTTON_LEFT)
#define SOL_CUR2STACK (BUTTON_REC | BUTTON_UP)
#define SOL_REM2STACK (BUTTON_REC | BUTTON_DOWN)
#define SOL_MENU_RUN BUTTON_SELECT
#define SOL_MENU_INFO BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SOL_QUIT BUTTON_A
#define SOL_UP BUTTON_UP
#define SOL_DOWN BUTTON_DOWN
#define SOL_LEFT BUTTON_LEFT
#define SOL_RIGHT BUTTON_RIGHT
#define SOL_MOVE_PRE BUTTON_SELECT
#define SOL_MOVE (BUTTON_SELECT | BUTTON_REL)
#define SOL_DRAW BUTTON_MENU
#define SOL_REM2CUR (BUTTON_LEFT | BUTTON_POWER)
#define SOL_CUR2STACK (BUTTON_SELECT | BUTTON_REPEAT)
#define SOL_REM2STACK (BUTTON_RIGHT | BUTTON_POWER)
#define SOL_MENU_RUN BUTTON_SELECT
#define SOL_MENU_RUN2 BUTTON_RIGHT
#define SOL_MENU_INFO BUTTON_MENU
#endif

/* common help definitions */
#define HELP_SOL_UP "UP: Move the cursor up in the column."
#define HELP_SOL_DOWN "DOWN: Move the cursor down in the column."
#define HELP_SOL_LEFT "LEFT: Move the cursor to the previous column."
#define HELP_SOL_RIGHT "RIGHT: Move the cursor to the next column."

/* variable help definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define HELP_SOL_MOVE "ON: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "F2: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "PLAY: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "F1: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "F3: Put the card on top of the remains' stack on one of the 4 final stacks."

#elif CONFIG_KEYPAD == ONDIO_PAD
#define HELP_SOL_MOVE "MODE: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "MODE..: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "LEFT..: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "RIGHT..: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "UP..: Put the card on top of the remains' stack on one of the 4 final stacks."

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define HELP_SOL_MOVE "SELECT: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "REC: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "PLAY+LEFT: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "SELECT..: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "PLAY+RIGHT: Put the card on top of the remains' stack on one of the 4 final stacks."

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) ||(CONFIG_KEYPAD == IPOD_3G_PAD)
#define HELP_SOL_MOVE "SELECT: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "SELECT+PLAY: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "SELECT+LEFT: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "SELECT+RIGHT..: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "LEFT+RIGHT: Put the card on top of the remains' stack on one of the 4 final stacks."

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define HELP_SOL_MOVE "MENU: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "PLAY: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "REC+LEFT: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "REC+UP..: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "REC+DOWN: Put the card on top of the remains' stack on one of the 4 final stacks."

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define HELP_SOL_MOVE "SELECT: Select cards, Move cards, reveal hidden cards ..."
#define HELP_SOL_DRAW "MENU: Un-select a card if it was selected. Else, draw 3 new cards out of the remains' stack."
#define HELP_SOL_REM2CUR "POWER+LEFT: Put the card on top of the remains' stack on top of the cursor."
#define HELP_SOL_CUR2STACK "SELECT..: Put the card under the cursor on one of the 4 final stacks."
#define HELP_SOL_REM2STACK "POWER+RIGHT: Put the card on top of the remains' stack on one of the 4 final stacks."

#endif

#if LCD_DEPTH>1
static const unsigned colors[4] = {
#ifdef HAVE_LCD_COLOR
    LCD_BLACK, LCD_RGBPACK(255, 0, 0), LCD_BLACK, LCD_RGBPACK(255, 0, 0)
#else
    LCD_BLACK, LCD_BRIGHTNESS(127), LCD_BLACK, LCD_BRIGHTNESS(127)
#endif
};
#endif

#define BMPHEIGHT_c 10
#define BMPWIDTH_c 8

#ifdef HAVE_LCD_COLOR
#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#define BMPHEIGHT_CARDBACK 33
#define BMPWIDTH_CARDBACK 26
#else
#define BMPHEIGHT_CARDBACK 24
#define BMPWIDTH_CARDBACK 18
#endif

extern const fb_data solitaire_cardback[];
#endif

static const unsigned char suitsi[4][30] = {
     {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00, 0x00, 0x07, 0x0f, 0x1f, 0x1f, 0x0f, 0x7f, 0x7f, 0x7f, 0x0f, 0x1f, 0x1f, 0x0f, 0x07, 0x00},
     {0x00, 0xf0, 0xf8, 0xfc, 0xfe, 0xfc, 0xf8, 0xf0, 0xf8, 0xfc, 0xfe, 0xfc, 0xf8, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00},
     {0x00, 0xc0, 0xe0, 0xe0, 0xfc, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfc, 0xe0, 0xe0, 0xc0, 0x00, 0x00, 0x03, 0x07, 0x07, 0x0f, 0x0f, 0x7f, 0x7f, 0x7f, 0x0f, 0x0f, 0x07, 0x07, 0x03, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x80, 0xe0, 0xf8, 0xfe, 0xfe, 0xf8, 0xe0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1f, 0x7f, 0x7f, 0x1f, 0x07, 0x01, 0x00, 0x00, 0x00},
};
static const unsigned char suits[4][16] = {
/* Spades */
    {0x00, 0x78, 0x3c, 0xfe, 0xfe, 0x3c, 0x78, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00},/* ........ */
/* Hearts */
    {0x00, 0x3c, 0x7e, 0xfc, 0xf8, 0xfc, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00},/* ........ */
/* Clubs */
    {0x00, 0x70, 0x34, 0xfe, 0xfe, 0x34, 0x70, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00},/* ........ */
/* Diamonds */
    {0x00, 0x70, 0xfc, 0xfe, 0xfe, 0xfc, 0x70, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00} /* ........ */
};

static unsigned char numbers[13][16] = {
/* Ace */
    {0x00, 0xf0, 0xfc, 0x7e, 0x36, 0x7e, 0xfc, 0xf0, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01},
/* 2 */
    {0x00, 0x8c, 0xce, 0xe6, 0xf6, 0xbe, 0x9c, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00}, /* ........ */
/* 3 */
    {0x00, 0xcc, 0x86, 0xb6, 0xb6, 0xfe, 0xdc, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00}, /* ........ */
/* 4 */
    {0x00, 0x3e, 0x3e, 0x30, 0xfe, 0xfe, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00}, /* ........ */
/* 5 */
    {0x00, 0xbe, 0xbe, 0xb6, 0xb6, 0xf6, 0xe6, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00}, /* ........ */
/* 6 */
    {0x00, 0xfc, 0xfe, 0xb6, 0xb6, 0xf6, 0xe4, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00}, /* ........ */
/* 7 */
    {0x00, 0x86, 0xc6, 0x66, 0x36, 0x1e, 0x0e, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ........ */
/* 8 */
    {0x00, 0xdc, 0xfe, 0xb6, 0xb6, 0xfe, 0xdc, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00}, /* ........ */
/* 9 */
    {0x00, 0x9c, 0xbe, 0xb6, 0xb6, 0xfe, 0xfc, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00},  /* ........ */
/* 10 */
    {0x00, 0x18, 0x0c, 0xfe, 0x00, 0xfc, 0x86, 0xfc, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00}, /* ........ */
/* Jack */
    {0x00, 0xe6, 0xc6, 0x86, 0xfe, 0xfe, 0x06, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00}, /* ........ */
/* Queen */
    {0x00, 0x7c, 0xfe, 0xc6, 0xe6, 0xfe, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00}, /* ........ */
/* King */
    {0x00, 0xfe, 0xfe, 0x38, 0x7c, 0xee, 0xc6, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00} /* ........ */
};


#define FILENAME "sol.cfg"


#define NOT_A_CARD 255

/* number of cards per suit */
#define CARDS_PER_SUIT 13

/* number of suits */
#define SUITS 4

/* number of columns */
#define COL_NUM 7

/* pseudo column numbers to be used for cursor coordinates */
/* columns COL_NUM t COL_NUM + SUITS - 1 correspond to the final stacks */
#define STACKS_COL COL_NUM
/* column COL_NUM + SUITS corresponds to the remains' stack */
#define REM_COL (STACKS_COL + SUITS)

#define NOT_A_COL 255


/* size of a card on the screen */
#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#define CARD_WIDTH 26
#define CARD_HEIGHT 34
#else
#define CARD_WIDTH 18
#define CARD_HEIGHT 24
#endif

/* where the cards start */
#define CARD_START CARD_HEIGHT +4

#if (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
#define KNOWN_CARD 23
#define NOT_KNOWN_CARD 30
#else
#define KNOWN_CARD 13
#define NOT_KNOWN_CARD 20
#endif

/* background color */
#define background_color LCD_RGBPACK(0,157,0)

typedef struct card {
    unsigned char suit : 2;
    unsigned char num : 4;
    unsigned char known : 1;
    unsigned char used : 1;/* this is what is used when dealing cards */
    unsigned char next;
} card;

unsigned char next_random_card(card *deck){
    unsigned char i,r;

    r = rb->rand()%(SUITS * CARDS_PER_SUIT)+1;
    i = 0;

    while(r>0){
        i = (i + 1)%(SUITS * CARDS_PER_SUIT);
        if(!deck[i].used) r--;
    }

    deck[i].used = 1;

    return i;
}

#define HELP_QUIT 0
#define HELP_USB 1

/* help for the not so intuitive interface */
int solitaire_help(void){

    int button;
    int lastbutton = BUTTON_NONE;

    while(1){

        rb->lcd_clear_display();

#if CONFIG_KEYPAD == RECORDER_PAD
        rb->lcd_putsxy(0, 0, "Press a key to see");
        rb->lcd_putsxy(0, 7, "it's role.");
        rb->lcd_putsxy(0, 21, "Press OFF to");
        rb->lcd_putsxy(0, 28, "return to menu.");
        rb->lcd_putsxy(0, 42, "All actions can");
        rb->lcd_putsxy(0, 49, "be done using");
        rb->lcd_putsxy(0, 56, "arrows, ON and F2.");
#elif CONFIG_KEYPAD == ONDIO_PAD
        rb->lcd_putsxy(0, 0, "Press a key short");
        rb->lcd_putsxy(0, 7, "or long to see it's");
        rb->lcd_putsxy(0, 21, "role. Press OFF to");
        rb->lcd_putsxy(0, 28, "return to menu.");
        rb->lcd_putsxy(0, 42, "All actions can be");
        rb->lcd_putsxy(0, 49, "done using arrows,");
        rb->lcd_putsxy(0, 56, "short & long MODE.");
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
        rb->lcd_putsxy(20, 8, "Press a key or key");
        rb->lcd_putsxy(20, 16, "combo to see it's");
        rb->lcd_putsxy(20, 24, "role. Press STOP to");
        rb->lcd_putsxy(20, 32, "return to menu.");
        rb->lcd_putsxy(20, 48, "All actions can be");
        rb->lcd_putsxy(20, 56, "done using the");
        rb->lcd_putsxy(20, 64, "joystick and RECORD.");
#endif

        rb->lcd_update();

        button = rb->button_get(true);
        switch(button){
            case SOL_UP:
#ifdef SOL_UP_PRE
                if(lastbutton != SOL_UP_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_UP);
                break;

            case SOL_DOWN:
#ifdef SOL_DOWN_PRE
                if(lastbutton != SOL_DOWN_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_DOWN);
                break;

            case SOL_LEFT:
#ifdef SOL_LEFT_PRE
                if(lastbutton != SOL_LEFT_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_LEFT);
                break;

            case SOL_RIGHT:
#ifdef SOL_RIGHT_PRE
                if(lastbutton != SOL_RIGHT_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_RIGHT);
                break;

            case SOL_MOVE:
#ifdef SOL_MOVE_PRE
                if(lastbutton != SOL_MOVE_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_MOVE);
                break;

            case SOL_DRAW:
#ifdef SOL_DRAW_PRE
                if(lastbutton != SOL_DRAW_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_DRAW);
                break;

            case SOL_CUR2STACK:
#ifdef SOL_CUR2STACK_PRE
                if(lastbutton != SOL_CUR2STACK_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_CUR2STACK);
                break;

            case SOL_REM2STACK:
#ifdef SOL_REM2STACK_PRE
                if(lastbutton != SOL_REM2STACK_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_REM2STACK);
                break;

            case SOL_REM2CUR:
#ifdef SOL_REM2CUR_PRE
                if(lastbutton != SOL_REM2CUR_PRE)
                    break;
#endif
                rb->splash(HZ*2, true, HELP_SOL_REM2CUR);
                break;
#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
                return HELP_QUIT;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return HELP_USB;
                break;
        }
        if(button != BUTTON_NONE)
            lastbutton = button;
    }
}
#define CFGFILE_VERSION 0
int draw_type;


unsigned char change_draw(unsigned char draw){
         if (draw == 0)
            return 1;
         else
             return 0;
}
static struct configdata config[] = {
   { TYPE_INT, 0, 1, &draw_type, "draw_type", NULL, NULL }
};

/* menu return codes */
#define MENU_RESUME 0
#define MENU_RESTART 1
#define MENU_HELP 3
#define MENU_QUIT 4
#define MENU_USB 5
#define MENU_OPT 2

/* menu item number */
#define MENU_LENGTH 5

/* different menu behaviors */

#define MENU_BEFOREGAME 0
#define MENU_BEFOREGAMEOP 1
#define MENU_DURINGGAME 2
unsigned char when;
/* the menu */
/* text displayed changes depending on the 'when' parameter */
int solitaire_menu(unsigned char when_n)
{
    static char menu[3][MENU_LENGTH][17] =
        { { "Start Game",
            "",
            "Draw Three Cards",
            "Help",
            "Quit" },
          { "Start Game",
            "",
            "Draw One Card",
            "Help",
            "Quit" },
          { "Resume Game",
            "Restart Game",
            "",
            "Help",
            "Quit"},
        };


    int i;
    int cursor=0;
    int button;
    int fh;
    when=when_n;
    rb->lcd_getstringsize("A", NULL, &fh);
    fh++;

    if(when != MENU_BEFOREGAMEOP && when!=MENU_BEFOREGAME && when!=MENU_DURINGGAME)
        when = MENU_DURINGGAME;

    while(1){

        rb->lcd_clear_display();
        rb->lcd_putsxy(20, 1, "Solitaire");

        for(i = 0; i<MENU_LENGTH; i++){
            rb->lcd_putsxy(1, 17+fh*i, menu[when][i]);
            if(cursor == i) {
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(0,17+fh*i, LCD_WIDTH, fh);
                rb->lcd_set_drawmode(DRMODE_SOLID);
            }
        }

        rb->lcd_update();

        button = rb->button_get(true);
        switch(button){
            case SOL_UP:
                cursor = (cursor + MENU_LENGTH - 1)%MENU_LENGTH;
                break;

            case SOL_DOWN:
                cursor = (cursor + 1)%MENU_LENGTH;
                break;

            case SOL_LEFT:
                return MENU_RESUME;

            case SOL_MENU_RUN:
#ifdef SOL_MENU_RUN2
            case SOL_MENU_RUN2:
#endif
                switch(cursor){
                    case MENU_RESUME:
                    case MENU_RESTART:
                    case MENU_OPT:
                    case MENU_QUIT:
                        return cursor;

                    case MENU_HELP:
                        if(solitaire_help() == HELP_USB)
                            return MENU_USB;
                        break;
                }
                break;

            case SOL_MENU_INFO:
#if defined(SOL_MENU_INFO2) && defined(SOL_MENU_INFO3)
            case SOL_MENU_INFO2:
            case SOL_MENU_INFO3:
#endif
                rb->splash(HZ, true, "Solitaire for Rockbox by dionoea");
                break;

#ifdef SOL_OPT
            case SOL_OPT:
                return MENU_OPT;
#endif

#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
                return MENU_QUIT;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return MENU_USB;
                break;
        }
    }
}

/* player's cursor */
unsigned char cur_card;
/* player's cursor column num */
unsigned char cur_col;

/* selected card */
unsigned char sel_card;

/* the deck */
card deck[SUITS * CARDS_PER_SUIT];

/* the remaining cards */
unsigned char rem;
unsigned char cur_rem;
unsigned char coun_rem;

/* the 7 game columns */
unsigned char cols[COL_NUM];

int CARDS_PER_DRAW;
/* the 4 final stacks */
unsigned char stacks[SUITS];

/* initialize the game */
void solitaire_init(void){

    unsigned char c;
    int i,j;
#if LCD_DEPTH>1
                rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_background(background_color);
#endif
#endif
/* number of cards that are drawn on the remains' stack (by pressing F2) */
    if(draw_type == 0) {
      CARDS_PER_DRAW =3;
    } else {
      CARDS_PER_DRAW=1;
    }
    /* init deck */
    for(i=0;i<SUITS;i++){
        for(j=0;j<CARDS_PER_SUIT;j++){
            deck[i*CARDS_PER_SUIT+j].suit = i;
            deck[i*CARDS_PER_SUIT+j].num = j;
            deck[i*CARDS_PER_SUIT+j].known = 1;
            deck[i*CARDS_PER_SUIT+j].used = 0;
            deck[i*CARDS_PER_SUIT+j].next = NOT_A_CARD;
        }
    }

    /* deal the cards ... */
    /* ... in the columns */
    for(i=0; i<COL_NUM; i++){
        c = NOT_A_CARD;
        for(j=0; j<=i; j++){
            if(c == NOT_A_CARD){
                cols[i] = next_random_card(deck);
                c = cols[i];
            } else {
                deck[c].next = next_random_card(deck);
                c = deck[c].next;
            }
            if(j<i) deck[c].known = 0;
        }
    }

    /* ... shuffle what's left of the deck */
    rem = next_random_card(deck);
    c = rem;

    for(i=1; i<SUITS * CARDS_PER_SUIT - COL_NUM * (COL_NUM + 1)/2; i++){
        deck[c].next = next_random_card(deck);
        c = deck[c].next;
    }

    /* we now finished dealing the cards. The game can start ! (at last) */

    /* init the stack */
    for(i = 0; i<SUITS; i++){
        stacks[i] = NOT_A_CARD;
    }

    /* the cursor starts on upper left card */
    cur_card = cols[0];
    cur_col = 0;

    /* no card is selected */
    sel_card = NOT_A_CARD;

    /* init the remainder */
    cur_rem = NOT_A_CARD;

    coun_rem=0;
}

/* find the column number in which 'card' can be found */
unsigned char find_card_col(unsigned char card){
    int i;
    unsigned char c;

    if(card == NOT_A_CARD) return NOT_A_COL;

    for(i=0; i<COL_NUM; i++){
        c = cols[i];
        while(c!=NOT_A_CARD){
            if(c == card) return i;
            c = deck[c].next;
        }
    }

    for(i=0; i<SUITS; i++){
        c = stacks[i];
        while(c!=NOT_A_CARD){
            if(c == card) return STACKS_COL + i;
            c = deck[c].next;
        }
    }

    return REM_COL;
}

/* find the card preceding 'card' */
/* if it doesn't exist, return NOT_A_CARD */
unsigned char find_prev_card(unsigned char card){
    int i;

    for(i=0; i<SUITS*CARDS_PER_SUIT; i++){
        if(deck[i].next == card) return i;
    }

    return NOT_A_CARD;
}

/* find the last card of a given column */
unsigned char find_last_card(unsigned char col){
    unsigned char c;

    if(col < COL_NUM){
        c = cols[col];
    } else if(col < REM_COL){
        c = stacks[col - STACKS_COL];
    } else {
        c = rem;
    }

    if(c == NOT_A_CARD)
        return c;
    else {
        while(deck[c].next != NOT_A_CARD){
            c = deck[c].next;
        }
        return c;
    }
}

#define MOVE_OK 0
#define MOVE_NOT_OK 1
unsigned char move_card(unsigned char dest_col, unsigned char src_card){
    /* the column on which to take src_card */
    unsigned char src_col;

    /* the last card of dest_col */
    unsigned char dest_card;

    /* the card under src_card */
    unsigned char src_card_prev;

    /* you can't move no card (at least, it doesn't have any consequence) */
    if(src_card == NOT_A_CARD) return MOVE_NOT_OK;
    /* you can't put a card back on the remains' stack */
    if(dest_col == REM_COL) return MOVE_NOT_OK;

    src_col = find_card_col(src_card);
    dest_card = find_last_card(dest_col);
    src_card_prev = find_prev_card(src_card);

    /* you can't move more than one card at a time from the final stack */
    /* to the rest of the game */
    if(src_col >= COL_NUM && src_col < REM_COL
       && deck[src_card].next != NOT_A_CARD){
        return MOVE_NOT_OK;
    }

    /* if we (that means dest) are on one of the 7 columns ... */
    if(dest_col < COL_NUM){
        /* ... check is we are on an empty color and that the src is a king */
        if(dest_card == NOT_A_CARD
           && deck[src_card].num == CARDS_PER_SUIT - 1){
            /* this is a winning combination */
            cols[dest_col] = src_card;
        }
        /* ... or check if the cards follow one another and have same suit */
        else if((deck[dest_card].suit + deck[src_card].suit)%2==1
           && deck[dest_card].num == deck[src_card].num + 1){
            /* this is a winning combination */
            deck[dest_card].next = src_card;
        }
        /* ... or, humpf, well that's not good news */
        else {
            /* this is not a winning combination */
            return MOVE_NOT_OK;
        }
    }
    /* if we are on one of the 4 final stacks ... */
    else if(dest_col < REM_COL){
        /* ... check if we are on an empty stack, that the src is an
         * ace and that this is the good final stack */
        if(dest_card == NOT_A_CARD
           && deck[src_card].num == 0
           && deck[src_card].suit == dest_col - STACKS_COL){
            /* this is a winning combination */
            stacks[dest_col - STACKS_COL] = src_card;
        }
        /* ... or check if the cards follow one another, have the same
         * suit and {that src has no .next element or is from the remains'
         * stack} */
        else if(deck[dest_card].suit == deck[src_card].suit
           && deck[dest_card].num + 1 == deck[src_card].num
           && (deck[src_card].next == NOT_A_CARD || src_col == REM_COL) ){
            /* this is a winning combination */
            deck[dest_card].next = src_card;
        }
        /* ... or, well that's not good news */
        else {
            /* this is not a winnong combination */
            return MOVE_NOT_OK;
        }
    }
    /* if we are on the remains' stack */
    else {
        /* you can't move a card back to the remains' stack */
        return MOVE_NOT_OK;
    }

    /* if the src card is from the remains' stack, we don't want to take
     * the following cards */
    if(src_col == REM_COL){
        /* if src card is the first card from the stack */
        if(src_card_prev == NOT_A_CARD){
            rem = deck[src_card].next;
            coun_rem = coun_rem-1;
        }
        /* if src card is not the first card from the stack */
        else {
            deck[src_card_prev].next = deck[src_card].next;
        }
        deck[src_card].next = NOT_A_CARD;
        cur_rem = src_card_prev;
        coun_rem = coun_rem-1;
    }
    /* if the src card is from somewhere else, just take everything */
    else {
        if(src_card_prev == NOT_A_CARD){
            if(src_col < COL_NUM){
                cols[src_col] = NOT_A_CARD;
            } else {
                stacks[src_col - STACKS_COL] = NOT_A_CARD;
            }
        } else {
            deck[src_card_prev].next = NOT_A_CARD;
        }
    }
    sel_card = NOT_A_CARD;
    /* tada ! */
    return MOVE_OK;
}



#define SOLITAIRE_WIN 0
#define SOLITAIRE_QUIT 1
#define SOLITAIRE_USB 2

#if ( LCD_WIDTH > ( CARD_WIDTH * 8 ) )
#   define BIG_SCREEN 1
#else
#   define BIG_SCREEN 0
#endif
#define LCD_WIDTH2 (LCD_WIDTH - BIG_SCREEN)

/* the game */
int solitaire(void){

    int i,j,x;
    int button, lastbutton = 0;
    unsigned char c,h,prevcard;
    int biggest_col_length;

    configfile_init(rb);
    configfile_load(FILENAME, config, 1, 0);

    rb->srand( *rb->current_tick );
    switch(solitaire_menu(draw_type==0?MENU_BEFOREGAME:MENU_BEFOREGAMEOP)) {
       case MENU_QUIT:
            return SOLITAIRE_QUIT;

        case MENU_USB:
            return SOLITAIRE_USB;
       case MENU_OPT:
            draw_type=change_draw(draw_type);
            configfile_save(FILENAME, config, 1, 0);
            when=draw_type==0?MENU_BEFOREGAME:MENU_BEFOREGAMEOP;
            return 0;
    }
#if LCD_DEPTH>1
                rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_background(background_color);
#else
                rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
#endif
    solitaire_init();

    while(true){

        rb->lcd_clear_display();

        /* get the biggest column length so that display can be "optimized" */
        biggest_col_length = 0;

        for(i=0;i<COL_NUM;i++){
            j = 0;
            c = cols[i];
            while(c != NOT_A_CARD){
                j++;
                c = deck[c].next;
            }
            if(j>biggest_col_length) biggest_col_length = j;
        }

        /* check if there are cards remaining in the game. */
        /* if there aren't any, that means you won :) */
        if(biggest_col_length == 0 && rem == NOT_A_CARD){
            rb->splash(HZ*2, true, "You Won :)");
            return SOLITAIRE_WIN;
        }

        /* draw the columns */
        for(i=0;i<COL_NUM;i++){
            c = cols[i];
            j = CARD_START;
            while(true){
                if(c==NOT_A_CARD) {
                    /* draw the cursor on empty columns */
                    if(cur_col == i){
                        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                        rb->lcd_fillrect(1+i*(LCD_WIDTH-2)/COL_NUM+1, j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
                    }
                    break;
                }
                /* clear the card's spot */

                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_fillrect(1+i*(LCD_WIDTH - 2)/COL_NUM, j+1, CARD_WIDTH, CARD_HEIGHT-1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                /* known card */
                if(deck[c].known == 1){
#if LCD_DEPTH>1
   #ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
                rb->lcd_set_background(LCD_WHITE);
   #else
                rb->lcd_set_foreground(LCD_DEFAULT_BG);
                rb->lcd_set_background(LCD_DEFAULT_BG);
   #endif
#endif
                    rb->lcd_fillrect(1+i*(LCD_WIDTH - 2)/COL_NUM+1, j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
#if LCD_DEPTH>1
                    rb->lcd_set_foreground(colors[deck[c].suit]);
#endif
                    rb->lcd_mono_bitmap(numbers[deck[c].num], 1+i*(LCD_WIDTH - 2)/COL_NUM+1, j+1, BMPWIDTH_c, BMPHEIGHT_c);
                    rb->lcd_mono_bitmap(suits[deck[c].suit],  1+i*(LCD_WIDTH - 2)/COL_NUM+ BMPWIDTH_c +2, j+1, BMPWIDTH_c, BMPHEIGHT_c);
#if LCD_DEPTH>1
                    rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
                    rb->lcd_set_background(background_color);
#endif
#endif
                } else {
#ifdef HAVE_LCD_COLOR
                    rb->lcd_bitmap(solitaire_cardback,
                                   1+i*(LCD_WIDTH - 2)/COL_NUM+1, j+1,
                                   BMPWIDTH_CARDBACK, BMPHEIGHT_CARDBACK);
#endif
                }
                /* draw top line of the card */
                rb->lcd_drawline(1+i*(LCD_WIDTH - 2)/COL_NUM+1,j,1+i*(LCD_WIDTH - 2)/COL_NUM+CARD_WIDTH-1,j);
                /* selected card */
                if(c == sel_card && sel_card != NOT_A_CARD){
                     rb->lcd_drawrect(1+i*(LCD_WIDTH - 2)/COL_NUM+1, j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
                }
                /* cursor (or not) */
                if(c == cur_card){
                    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                    rb->lcd_fillrect(1+i*(LCD_WIDTH - 2)/COL_NUM+1, j+1, CARD_WIDTH-1, CARD_HEIGHT-1);
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    /* go to the next card */
                    c = deck[c].next;
                    if(c == NOT_A_CARD) break;
                    else {
                        if(deck[c].known == 0) {
                            j += CARD_HEIGHT - NOT_KNOWN_CARD;
                        } else {
                            j += CARD_HEIGHT - KNOWN_CARD;
                        }
                     }
                } else {
                    /* go to the next card */
                    h = c;
                    c = deck[c].next;
                    if(c == NOT_A_CARD) break;
                    if(c!=NOT_A_CARD) {
                        if(deck[h].known == 0) {
 /*changeeee*/             j += CARD_HEIGHT - NOT_KNOWN_CARD;
                        } else {
                            j += min(CARD_HEIGHT - KNOWN_CARD, (LCD_HEIGHT - CARD_START - CARD_HEIGHT)/biggest_col_length);
                        }
                    }
                }
            }
            if(cols[i]!=NOT_A_CARD){
                /* draw line to the left of the column */
                rb->lcd_drawline(1+i*(LCD_WIDTH - 2)/COL_NUM,CARD_START,1+i*(LCD_WIDTH - 2)/COL_NUM,j+CARD_HEIGHT-1);
                /* draw line to the right of the column */
                rb->lcd_drawline(1+i*(LCD_WIDTH - 2)/COL_NUM+CARD_WIDTH,CARD_START,1+i*(LCD_WIDTH - 2)/COL_NUM+CARD_WIDTH,j+CARD_HEIGHT-1);
                /* draw bottom of the last card */
                rb->lcd_drawline(1+i*(LCD_WIDTH - 2)/COL_NUM+1,j+CARD_HEIGHT,1+i*(LCD_WIDTH - 2)/COL_NUM+CARD_WIDTH-1,j+CARD_HEIGHT);
            }
        }
#if LCD_DEPTH>1
        rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_background(background_color);
#endif
#endif
        rb->lcd_set_drawmode(DRMODE_SOLID);
        /* draw the stacks */
        for(i=0; i<SUITS; i++){
            c = stacks[i];
            if(c!=NOT_A_CARD){
                while(deck[c].next != NOT_A_CARD){
                    c = deck[c].next;
                }
            }

            if(c != NOT_A_CARD) {
#if LCD_DEPTH>1
   #ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
                rb->lcd_set_background(LCD_WHITE);
   #else
                rb->lcd_set_foreground(LCD_DEFAULT_BG);
                rb->lcd_set_background(LCD_DEFAULT_BG);
   #endif
#endif
                    rb->lcd_fillrect(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 2, CARD_WIDTH-1, CARD_HEIGHT-1);
#if LCD_DEPTH>1
                    rb->lcd_set_foreground(colors[i]);
#endif
                rb->lcd_mono_bitmap(numbers[deck[c].num], LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 2, BMPWIDTH_c, BMPHEIGHT_c);
                rb->lcd_mono_bitmap(suits[deck[c].suit], LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+10, 2, BMPWIDTH_c, BMPHEIGHT_c);
            } else {
#if LCD_DEPTH>1
   #ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
                rb->lcd_set_background(LCD_WHITE);
   #else
                rb->lcd_set_foreground(LCD_DEFAULT_BG);
                rb->lcd_set_background(LCD_DEFAULT_BG);
   #endif
#endif
                    rb->lcd_fillrect(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 2, CARD_WIDTH-1, CARD_HEIGHT-1);
#if LCD_DEPTH>1
                    rb->lcd_set_foreground(colors[i]);
#endif
#ifdef HAVE_LCD_COLOR
                rb->lcd_mono_bitmap(suitsi[i], LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+(CARD_WIDTH/2-7), CARD_HEIGHT/2-7, 15, 16);
#endif
            }
#if LCD_DEPTH>1
                    rb->lcd_set_foreground(LCD_BLACK);
#ifdef HAVE_LCD_COLOR
                rb->lcd_set_background(background_color);
#endif
#endif
            /* draw a selected card */
            if(c != NOT_A_CARD) {
                if(sel_card == c){
                    rb->lcd_drawrect(LCD_WIDTH2 -(CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 2, CARD_WIDTH-1, CARD_HEIGHT-1);
                }
            }
            rb->lcd_drawline(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 1,LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+CARD_WIDTH,1);
            rb->lcd_drawline(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2 ,2,LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2,CARD_HEIGHT);
            rb->lcd_drawline(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1,CARD_HEIGHT+1,LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+CARD_WIDTH - 1,CARD_HEIGHT+1);
#if BIG_SCREEN
            rb->lcd_drawline(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+CARD_WIDTH,2,LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+CARD_WIDTH,CARD_HEIGHT);
#endif
            /* draw the cursor on one of the stacks */
            if(cur_col == STACKS_COL + i){
                rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
                rb->lcd_fillrect(LCD_WIDTH2 - (CARD_WIDTH*4+8)+CARD_WIDTH*i+i*2+1, 2, CARD_WIDTH-1, CARD_HEIGHT-1);
                rb->lcd_set_drawmode(DRMODE_SOLID);
            }
        }

        /* draw the remains */


           if(rem != NOT_A_CARD) {
              coun_rem = coun_rem>2?coun_rem=2:coun_rem;
              if(cur_rem != NOT_A_CARD && find_prev_card(cur_rem) != NOT_A_CARD && CARDS_PER_DRAW != 1) {
                  j = (coun_rem)*(BMPWIDTH_c+2);
                  for(i=0;i<=coun_rem;i++) {
                   if (i>0 && i<3){
#if LCD_DEPTH>1
   #ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
                rb->lcd_set_background(LCD_WHITE);
   #else
                rb->lcd_set_foreground(LCD_DEFAULT_BG);
                rb->lcd_set_background(LCD_DEFAULT_BG);
   #endif
#endif
                     rb->lcd_fillrect(CARD_WIDTH+4+j+1, 2, BMPWIDTH_c+1, CARD_HEIGHT-1);
        #if LCD_DEPTH>1
                     rb->lcd_set_foreground(LCD_BLACK);
        #endif
                     rb->lcd_drawline(CARD_WIDTH+4+j+1,1,CARD_WIDTH+4+j+BMPWIDTH_c+2,1); /* top line */
                     rb->lcd_drawline(CARD_WIDTH+4+j+1,CARD_HEIGHT+1,CARD_WIDTH+4+j+BMPWIDTH_c+2,CARD_HEIGHT+1); /* bottom line */
                     rb->lcd_drawline(CARD_WIDTH+4+j,2,CARD_WIDTH+4+j,CARD_HEIGHT); /* right line */


                     prevcard = cur_rem;
                     for(x=0;i>x;x++)
                        prevcard = find_prev_card(prevcard);
        #if LCD_DEPTH>1
                     rb->lcd_set_foreground(colors[deck[prevcard].suit]);
           #ifdef HAVE_LCD_COLOR
                     rb->lcd_set_background(LCD_WHITE);
           #endif
        #endif
                     rb->lcd_mono_bitmap(numbers[deck[prevcard].num], CARD_WIDTH+4+j+1, 3, BMPWIDTH_c, BMPHEIGHT_c);
                     rb->lcd_mono_bitmap(suits[deck[prevcard].suit],  CARD_WIDTH+4+j+1, 4+BMPHEIGHT_c, BMPWIDTH_c, BMPHEIGHT_c);


                   }
                  j -= BMPWIDTH_c+2;
                  }
              }
#if LCD_DEPTH>1
               rb->lcd_set_foreground(LCD_BLACK);
#endif
               if(CARDS_PER_DRAW==1 || cur_rem==NOT_A_CARD)
                  j=0;
               else
                  j=(coun_rem)*(BMPWIDTH_c+2);
               if(cur_rem != NOT_A_CARD){
               rb->lcd_drawline(CARD_WIDTH+4+j+1,1,CARD_WIDTH+4+j+CARD_WIDTH-1,1); /* top line */
               rb->lcd_drawline(CARD_WIDTH+4+j,2,CARD_WIDTH+4+j,CARD_HEIGHT); /* left line */
               rb->lcd_drawline(CARD_WIDTH+4+j+1,CARD_HEIGHT+1,CARD_WIDTH+4+j+CARD_WIDTH - 1,CARD_HEIGHT+1); /* bottom line */
               rb->lcd_drawline(CARD_WIDTH+4+j+CARD_WIDTH,2,CARD_WIDTH+4+j+CARD_WIDTH,CARD_HEIGHT); /* right line */
               }

               rb->lcd_drawline(2,1,CARD_WIDTH+1,1); /* top line */
               rb->lcd_drawline(1,2,1,CARD_HEIGHT); /* left line */
               rb->lcd_drawline(2,CARD_HEIGHT+1,CARD_WIDTH + 1,CARD_HEIGHT+1); /* bottom line */
               rb->lcd_drawline(CARD_WIDTH+2,2,CARD_WIDTH+2,CARD_HEIGHT); /* right line */

               if(cur_rem != NOT_A_CARD){
#if LCD_DEPTH>1
   #ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
                rb->lcd_set_background(LCD_WHITE);
   #else
                rb->lcd_set_foreground(LCD_DEFAULT_BG);
                rb->lcd_set_background(LCD_DEFAULT_BG);
   #endif
#endif
                    rb->lcd_fillrect(CARD_WIDTH+4+j+1, 2, CARD_WIDTH-1, CARD_HEIGHT-1);
    #if LCD_DEPTH>1
                    rb->lcd_set_foreground(colors[deck[cur_rem].suit]);
    #endif
                    rb->lcd_mono_bitmap(numbers[deck[cur_rem].num], CARD_WIDTH+4+j+1, 3, BMPWIDTH_c, BMPHEIGHT_c);
                    rb->lcd_mono_bitmap(suits[deck[cur_rem].suit],  CARD_WIDTH+4+j+10, 3, BMPWIDTH_c, BMPHEIGHT_c);
                    /* draw a selected card */
    #if LCD_DEPTH>1
                    rb->lcd_set_foreground(LCD_BLACK);
       #ifdef HAVE_LCD_COLOR
                    rb->lcd_set_background(background_color);
       #endif
    #endif
                    if(sel_card == cur_rem){
                        rb->lcd_drawrect(CARD_WIDTH+4+j+1, 2,CARD_WIDTH-1, CARD_HEIGHT-1);
                    }
                }
               if(rem != NOT_A_CARD){
#ifdef HAVE_LCD_COLOR
                     rb->lcd_bitmap(solitaire_cardback, 2, 2,
                                    BMPWIDTH_CARDBACK, BMPHEIGHT_CARDBACK);
#endif
                }
            }


        /* draw the cursor */
        if(cur_col == REM_COL && rem != NOT_A_CARD){
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(CARD_WIDTH+4+j+1, 2,CARD_WIDTH-1, CARD_HEIGHT-1);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        } else if(cur_col == REM_COL && rem == NOT_A_CARD) {
            rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
            rb->lcd_fillrect(CARD_WIDTH+4+1, 2,CARD_WIDTH-1, CARD_HEIGHT-1);
            rb->lcd_set_drawmode(DRMODE_SOLID);
        }


        rb->lcd_update();

        /* what to do when a key is pressed ... */
        button = rb->button_get(true);
        switch(button){

            /* move cursor to the last card of the previous column */
            /* or to the previous final stack */
            /* or to the remains stack */
            case SOL_RIGHT:
#ifdef SOL_RIGHT_PRE
                if(lastbutton != SOL_RIGHT_PRE)
                    break;
#endif
                if(cur_col >= COL_NUM){
                    cur_col = 0;
                } else if(cur_col == COL_NUM - 1){
                    cur_col = REM_COL;
                } else {
                    cur_col = (cur_col+1)%(REM_COL+1);
                }
                if(cur_col == REM_COL){
                    cur_card = cur_rem;
                    break;
                }
                cur_card  = find_last_card(cur_col);
                break;

            /* move cursor to the last card of the next column */
            /* or to the next final stack */
            /* or to the remains stack */
            case SOL_LEFT:
#ifdef SOL_LEFT_PRE
                if(lastbutton != SOL_LEFT_PRE)
                    break;
#endif
                if(cur_col == 0){
                    cur_col = REM_COL;
                } else if(cur_col >= COL_NUM) {
                    cur_col = COL_NUM - 1;
                } else {
                    cur_col = (cur_col + REM_COL)%(REM_COL+1);
                }
                if(cur_col == REM_COL){
                    cur_card = cur_rem;
                    break;
                }
                cur_card = find_last_card(cur_col);
                break;

            /* move cursor to card that's bellow */
            case SOL_DOWN:
#ifdef SOL_DOWN_PRE
                if(lastbutton != SOL_DOWN_PRE)
                    break;
#endif
                if(cur_col >= COL_NUM) {
                    cur_col = (cur_col - COL_NUM + 1)%(SUITS + 1) + COL_NUM;
                    if(cur_col == REM_COL){
                        cur_card = cur_rem;
                    }
                    else {
                        cur_card = find_last_card(cur_col);
                    }
                    break;
                }
                if(cur_card == NOT_A_CARD) break;
                if(deck[cur_card].next != NOT_A_CARD){
                    cur_card = deck[cur_card].next;
                } else {
                    cur_card = cols[cur_col];
                    while(deck[cur_card].known == 0
                          && deck[cur_card].next != NOT_A_CARD){
                        cur_card = deck[cur_card].next;
                    }
                }
                break;

            /* move cursor to card that's above */
            case SOL_UP:
#ifdef SOL_UP_PRE
                if(lastbutton != SOL_UP_PRE)
                    break;
#endif
                if(cur_col >= COL_NUM) {
                    cur_col = (cur_col - COL_NUM + SUITS)%(SUITS + 1) + COL_NUM;
                    if(cur_col == REM_COL){
                        cur_card = cur_rem;
                    } else {
                        cur_card = find_last_card(cur_col);
                    }
                    break;
                }
                if(cur_card == NOT_A_CARD) break;
                do{
                    cur_card = find_prev_card(cur_card);
                    if(cur_card == NOT_A_CARD){
                        cur_card = find_last_card(cur_col);
                    }
                } while (deck[cur_card].next != NOT_A_CARD
                         && deck[cur_card].known == 0);
                break;

            /* Try to put card under cursor on one of the stacks */
            case SOL_CUR2STACK:
#ifdef SOL_CUR2STACK_PRE
                if(lastbutton != SOL_CUR2STACK_PRE)
                    break;
#endif
                if(cur_card != NOT_A_CARD){
                    move_card(deck[cur_card].suit + STACKS_COL, cur_card);
                    sel_card = NOT_A_CARD;
                }
                break;

            /* Move cards arround, Uncover cards, ... */
            case SOL_MOVE:
#ifdef SOL_MOVE_PRE
                if(lastbutton != SOL_MOVE_PRE)
                    break;
#endif

                if(sel_card == NOT_A_CARD) {
                    if(cur_card != NOT_A_CARD) {
                        /* reveal a hidden card */
                        if(deck[cur_card].next == NOT_A_CARD && deck[cur_card].known==0){
                            deck[cur_card].known = 1;
                        } else if(cur_col == REM_COL && cur_rem == NOT_A_CARD) {
                               break;
                        /* select a card */
                        } else {
                            sel_card = cur_card;
                        }
                    }
                /* unselect card or try putting card on one of the 4 stacks */
                } else if(sel_card == cur_card) {
                    move_card(deck[sel_card].suit + COL_NUM, sel_card);
                    sel_card = NOT_A_CARD;
                    if( cur_col == REM_COL )
                    {
                        cur_card = cur_rem;
                    }
                /* try moving cards */
                } else {
                    if(move_card(cur_col, sel_card) == MOVE_OK){
                        sel_card = NOT_A_CARD;
                    }
                }
                break;

            /* If the card on the top of the remains can be put where */
            /* the cursor is, go ahead */
            case SOL_REM2CUR:
#ifdef SOL_REM2CUR_PRE
                if(lastbutton != SOL_REM2CUR_PRE)
                    break;
#endif
                coun_rem = coun_rem-1;
                move_card(cur_col, cur_rem);
                sel_card = NOT_A_CARD;
                break;

            /* If the card on top of the remains can be put on one */
            /* of the stacks, do so */
            case SOL_REM2STACK:
#ifdef SOL_REM2STACK_PRE
                if(lastbutton != SOL_REM2STACK_PRE)
                    break;
#endif
                if(cur_rem != NOT_A_CARD){
                    move_card(deck[cur_rem].suit + COL_NUM, cur_rem);
                    sel_card = NOT_A_CARD;
                    coun_rem = coun_rem-1;
                }
                break;


#ifdef SOL_REM
            case SOL_REM:
                if(sel_card != NOT_A_CARD){
                    /* unselect selected card */
                    sel_card = NOT_A_CARD;
                    break;
                }
                if(rem != NOT_A_CARD && cur_rem != NOT_A_CARD) {
                       sel_card=cur_rem;
                       break;
                }
                break;
#endif

            /* unselect selected card or ... */
            /* draw new cards from the remains of the deck */
            case SOL_DRAW:
#ifdef SOL_DRAW_PRE
                if(lastbutton != SOL_DRAW_PRE)
                    break;
#endif
                if(sel_card != NOT_A_CARD){
                    /* unselect selected card */
                    sel_card = NOT_A_CARD;
                    break;
                }
                if(rem != NOT_A_CARD) {
                    int cur_rem_old = cur_rem;
                    coun_rem = 0;
                    /* draw new cards form the remains of the deck */
                    if(cur_rem == NOT_A_CARD){ /*if the cursor card is null*/
                        cur_rem = rem;
                        i = CARDS_PER_DRAW - 1;
                    } else {
                        i = CARDS_PER_DRAW;
                    }

                    while(i>0 && deck[cur_rem].next != NOT_A_CARD){
                        cur_rem = deck[cur_rem].next;
                        i--;
                        coun_rem = coun_rem +1;
                    }
                    /* test if any cards are really left on */
                    /* the remains' stack */
                    if(i == CARDS_PER_DRAW){
                        cur_rem = NOT_A_CARD;
                        coun_rem = 0;
                    }
                    /* if cursor was on remains' stack when new cards were
                     * drawn, put cursor on top of remains' stack */
                    if(cur_col == REM_COL && cur_card == cur_rem_old) {
                        cur_card = cur_rem;
                        sel_card = NOT_A_CARD;
                    }
                }
                break;

            /* Show the menu */
#ifdef SOL_RC_QUIT
            case SOL_RC_QUIT:
#endif
            case SOL_QUIT:
#if LCD_DEPTH>1
                rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
                switch(solitaire_menu(MENU_DURINGGAME)){
                    case MENU_QUIT:
                        return SOLITAIRE_QUIT;

                    case MENU_USB:
                        return SOLITAIRE_USB;

                    case MENU_RESTART:
                        solitaire_init();
                        break;
                }
            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return SOLITAIRE_USB;
                break;
        }

        if(button != BUTTON_NONE)
            lastbutton = button;

        /* fix incoherences concerning cur_col and cur_card */
        c = find_card_col(cur_card);
        if(c != NOT_A_COL && c != cur_col)
            cur_card = find_last_card(cur_col);

        if(cur_card == NOT_A_CARD && find_last_card(cur_col) != NOT_A_CARD)
            cur_card = find_last_card(cur_col);
    }
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int result;

    /* plugin init */
    (void)parameter;
    rb = api;
    /* end of plugin init */
    /* Welcome to Solitaire ! */

    rb->splash(HZ, true, "Welcome to Solitaire !");

    /* play the game :) */
    /* Keep playing if a game was won (that means display the menu after */
    /* winning instead of quiting) */
    while((result = solitaire()) == SOLITAIRE_WIN);

    /* Exit the plugin */
    return (result == SOLITAIRE_USB) ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}

#endif
