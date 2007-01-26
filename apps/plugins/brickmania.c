/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005, 2006 Ben Basha (Paprica)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "configfile.h" /* Part of libplugin */

PLUGIN_HEADER


#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#define RC_QUIT BUTTON_RC_STOP

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif CONFIG_KEYPAD == ONDIO_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_MENU
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif CONFIG_KEYPAD == RECORDER_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD

#define QUIT BUTTON_OFF
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)

#define QUIT BUTTON_MENU
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_SCROLL_BACK
#define DOWN BUTTON_SCROLL_FWD

#define SCROLL_FWD(x) ((x) & BUTTON_SCROLL_FWD)
#define SCROLL_BACK(x) ((x) & BUTTON_SCROLL_BACK)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define QUIT BUTTON_A
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD /* grayscale at the moment */

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_SELECT
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD /* grayscale at the moment */

#define QUIT BUTTON_POWER
#define LEFT BUTTON_LEFT
#define RIGHT BUTTON_RIGHT
#define SELECT BUTTON_PLAY
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN

/* Only iPod have scroll events */
#define SCROLL_FWD(x) (0)
#define SCROLL_BACK(x) (0)

#else
#error Unsupported keypad
#endif

static struct plugin_api* rb;

enum menu_items {
    BM_START,
    BM_SEL_START,
    BM_RESUME,
    BM_SEL_RESUME,
    BM_NO_RESUME,
    BM_HELP,
    BM_SEL_HELP,
    BM_QUIT,
    BM_SEL_QUIT,
};

/* External bitmaps */
#if (LCD_WIDTH != 112) && (LCD_HEIGHT != 64)
extern const fb_data brickmania_menu_bg[];
extern const fb_data brickmania_gameover[];
#endif
extern const fb_data brickmania_menu_items[];
extern const fb_data brickmania_ball[];
#ifdef HAVE_LCD_COLOR
extern const fb_data brickmania_break[];
#endif

/* normal, glue, fire */
extern const fb_data brickmania_pads[];

/* +life, -life, glue, fire, normal */
extern const fb_data brickmania_powerups[];

/* purple, red, blue, pink, green, yellow orange */
extern const fb_data brickmania_bricks[];

#if (LCD_WIDTH == 320) && (LCD_HEIGHT == 240)

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 30

#define PAD_WIDTH 40
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 7
#define BRICK_HEIGHT 12
#define BRICK_WIDTH 32
#define BALL 5
#define HALFBALL 3
#define LEFTMARGIN 5
#define TOPMARGIN 30

#define MENU_BMPHEIGHT 20
#define MENU_BMPWIDTH 112

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#define BMPHEIGHT_menu LCD_HEIGHT
#define BMPWIDTH_menu LCD_WIDTH

#define BMPXOFS_start 105
#define BMPYOFS_start 110
#define BMPXOFS_resume 112
#define BMPYOFS_resume 132
#define BMPXOFS_help 142
#define BMPYOFS_help 150
#define BMPXOFS_quit 143
#define BMPYOFS_quit 170
#define HIGHSCORE_XPOS 57
#define HIGHSCORE_YPOS 88

#define STRINGPOS_finsh 140
#define STRINGPOS_congrats 157
#define STRINGPOS_navi 150
#define STRINGPOS_flipsides 150

#elif (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)

/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 30

/* Offsets for LCDS > 220x176 */
#define XOFS ((LCD_WIDTH-220)/2)
#define YOFS ((LCD_HEIGHT-176)/2)

#define PAD_WIDTH 40
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 7
#define BRICK_HEIGHT 8
#define BRICK_WIDTH 21
#define BALL 5
#define HALFBALL 3
#define LEFTMARGIN 5
#define TOPMARGIN 30

#define MENU_BMPHEIGHT 20
#define MENU_BMPWIDTH 112

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#define BMPHEIGHT_menu LCD_HEIGHT
#define BMPWIDTH_menu LCD_WIDTH

#define BMPXOFS_start (55+XOFS)
#define BMPYOFS_start (78+YOFS)
#define BMPXOFS_resume (62+XOFS)
#define BMPYOFS_resume (100+YOFS)
#define BMPXOFS_help (92+XOFS)
#define BMPYOFS_help (118+YOFS)
#define BMPXOFS_quit (93+XOFS)
#define BMPYOFS_quit (138+YOFS)
#define HIGHSCORE_XPOS (7+XOFS)
#define HIGHSCORE_YPOS (56+YOFS)

#define STRINGPOS_finsh 140
#define STRINGPOS_congrats 157
#define STRINGPOS_navi 150
#define STRINGPOS_flipsides 150

#elif (LCD_WIDTH == 160) && (LCD_HEIGHT == 128)
/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 50

#define PAD_WIDTH 30
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 7
#define BRICK_HEIGHT 7
#define BRICK_WIDTH 15
#define BALL 5
#define HALFBALL 3
#define LEFTMARGIN 5
#define TOPMARGIN 21

#define MENU_BMPHEIGHT 17
#define MENU_BMPWIDTH 88

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#if LCD_DEPTH == 16
#define BMPHEIGHT_menu 128
#else
#define BMPHEIGHT_menu 45
#endif
#define BMPWIDTH_menu 160

#if LCD_DEPTH == 16
#define BMPXOFS_start 38
#define BMPYOFS_start 56
#define BMPXOFS_resume 42
#define BMPYOFS_resume 72
#define BMPXOFS_help 65
#define BMPYOFS_help 85
#define BMPXOFS_quit 66
#define BMPYOFS_quit 99
#else
#define BMPXOFS_start 34
#define BMPYOFS_start 64
#define BMPXOFS_resume 38
#define BMPYOFS_resume 77
#define BMPXOFS_help 64
#define BMPYOFS_help 90
#define BMPXOFS_quit 64
#define BMPYOFS_quit 103
#endif
#define HIGHSCORE_XPOS 7
#define HIGHSCORE_YPOS 36

#define STRINGPOS_finsh 110
#define STRINGPOS_congrats 100
#define STRINGPOS_navi 100
#define STRINGPOS_flipsides 100

/* iPod Mini */
#elif (LCD_WIDTH == 138) && (LCD_HEIGHT == 110)
/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 50

#define PAD_WIDTH 30
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 5
#define BRICK_HEIGHT 7
#define BRICK_WIDTH 13
#define BALL 5
#define HALFBALL 3
#define LEFTMARGIN 4
#define TOPMARGIN 10

#define MENU_BMPHEIGHT 17
#define MENU_BMPWIDTH 88

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#define BMPHEIGHT_menu 45
#define BMPWIDTH_menu 160

#define BMPXOFS_start 28
#define BMPYOFS_start 41
#define BMPXOFS_resume 33
#define BMPYOFS_resume 58
#define BMPXOFS_help 56
#define BMPYOFS_help 74
#define BMPXOFS_quit 56
#define BMPYOFS_quit 90
#define HIGHSCORE_XPOS 76
#define HIGHSCORE_YPOS 22

#define STRINGPOS_finsh 54
#define STRINGPOS_congrats 44
#define STRINGPOS_navi 44
#define STRINGPOS_flipsides 44


#elif (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 75

#define PAD_WIDTH 30
#define PAD_HEIGHT 3
#define PAD_POS_Y LCD_HEIGHT - 5
#define BRICK_HEIGHT 4
#define BRICK_WIDTH 11
#define BALL 3
#define HALFBALL 2
#define LEFTMARGIN 1
#define TOPMARGIN 10

#define MENU_BMPHEIGHT 9
#define MENU_BMPWIDTH 80

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 7

#define BMPXOFS_start 12
#define BMPYOFS_start 20
#define BMPXOFS_resume 18
#define BMPYOFS_resume 31
#define BMPXOFS_help 39
#define BMPYOFS_help 42
#define BMPXOFS_quit 40
#define BMPYOFS_quit 53
#define HIGHSCORE_XPOS 0
#define HIGHSCORE_YPOS 0

#define STRINGPOS_finsh 54
#define STRINGPOS_congrats 44
#define STRINGPOS_navi 44
#define STRINGPOS_flipsides 44

#elif (LCD_WIDTH == 176) && (LCD_HEIGHT == 132) && (LCD_DEPTH==16)
/* The time (in ms) for one iteration through the game loop - decrease this
   to speed up the game - note that current_tick is (currently) only accurate
   to 10ms.
*/
#define CYCLETIME 30

#define PAD_WIDTH 40
#define PAD_HEIGHT 5
#define PAD_POS_Y LCD_HEIGHT - 7
#define BRICK_HEIGHT 7
#define BRICK_WIDTH 17
#define BALL 5
#define HALFBALL 3
#define LEFTMARGIN 3
#define TOPMARGIN 21

#define MENU_BMPHEIGHT 16
#define MENU_BMPWIDTH 89

#define BMPHEIGHT_powerup 6
#define BMPWIDTH_powerup 10

#define BMPHEIGHT_menu 132
#define BMPWIDTH_menu 176

#define BMPXOFS_start 44
#define BMPYOFS_start 58
#define BMPXOFS_resume 50
#define BMPYOFS_resume 75
#define BMPXOFS_help 74
#define BMPYOFS_help 89
#define BMPXOFS_quit 75
#define BMPYOFS_quit 104
#define HIGHSCORE_XPOS 7
#define HIGHSCORE_YPOS 36

#define STRINGPOS_finsh 110
#define STRINGPOS_congrats 110
#define STRINGPOS_navi 100
#define STRINGPOS_flipsides 100

#else
#error Unsupported LCD Size
#endif

int levels_num = 29;

static unsigned char levels[29][8][10] = {
    { /* level1 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x2,0x1,0x0,0x0,0x0,0x0,0x1,0x2,0x0},
        {0x0,0x0,0x2,0x1,0x0,0x0,0x1,0x2,0x0,0x0},
        {0x0,0x0,0x0,0x2,0x1,0x1,0x2,0x0,0x0,0x0},
        {0x7,0x0,0x0,0x7,0x2,0x2,0x7,0x0,0x0,0x7},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level2 */
        {0x0,0x0,0x7,0x7,0x1,0x1,0x7,0x7,0x0,0x0},
        {0x0,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x2,0x1,0x0,0x0,0x1,0x2,0x1,0x1},
        {0x1,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x1},
        {0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x2,0x1,0x0},
        {0x0,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x0}
    },
    { /* level3 */
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x3,0x23,0x23,0x3,0x0,0x0,0x2,0x22,0x22,0x2},
        {0x3,0x3,0x3,0x3,0x0,0x0,0x2,0x2,0x2,0x2},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x37,0x37,0x0,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6},
        {0x5,0x25,0x25,0x5,0x0,0x0,0x6,0x26,0x26,0x6},
        {0x5,0x5,0x5,0x5,0x0,0x0,0x6,0x6,0x6,0x6}
    },
    { /* level4 */
        {0x0,0x0,0x0,0x27,0x27,0x27,0x27,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x27,0x7,0x7,0x27,0x0,0x0,0x0},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x22,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x22},
        {0x26,0x6,0x0,0x2,0x2,0x2,0x2,0x0,0x6,0x26},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1}
    },
    { /* level5 */
        {0x1,0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4},
        {0x0,0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0},
        {0x2,0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5},
        {0x2,0x0,0x3,0x3,0x0,0x4,0x4,0x0,0x5,0x5},
        {0x0,0x33,0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0},
        {0x3,0x33,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x36},
        {0x3,0x0,0x4,0x4,0x0,0x5,0x5,0x0,0x6,0x36},
        {0x0,0x24,0x24,0x0,0x25,0x25,0x0,0x26,0x26,0x0}
    },
    { /* level6 */
        {0x0,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x0},
        {0x3,0x1,0x3,0x7,0x0,0x0,0x7,0x3,0x1,0x3},
        {0x3,0x1,0x3,0x7,0x7,0x7,0x7,0x3,0x1,0x3},
        {0x0,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5},
        {0x0,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level7 */
        {0x0,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x0,0x0,0x2,0x2,0x2,0x2,0x0,0x0,0x6},
        {0x6,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x6},
        {0x0,0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x0,0x0},
        {0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    },
    { /* level8 */
        {0x0,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x0,0x4,0x0,0x0,0x4,0x0,0x0,0x0},
        {0x6,0x6,0x0,0x2,0x32,0x32,0x2,0x0,0x6,0x6},
        {0x0,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x0},
        {0x0,0x6,0x6,0x0,0x0,0x0,0x0,0x6,0x6,0x0},
        {0x0,0x0,0x0,0x5,0x25,0x25,0x5,0x0,0x0,0x0},
        {0x0,0x5,0x5,0x25,0x5,0x5,0x25,0x5,0x5,0x0},
        {0x5,0x5,0x25,0x5,0x5,0x5,0x5,0x25,0x5,0x5}
    },
    { /* level9 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x3,0x0,0x0,0x0,0x0,0x0,0x0,0x3,0x2},
        {0x2,0x0,0x3,0x0,0x1,0x1,0x0,0x3,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x0,0x1,0x0,0x3,0x3,0x0,0x1,0x0,0x2},
        {0x2,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x2},
        {0x2,0x2,0x0,0x0,0x1,0x1,0x0,0x0,0x2,0x2},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    { /* level10 */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x4,0x1,0x1,0x4,0x0,0x0,0x0},
        {0x0,0x0,0x3,0x4,0x1,0x1,0x4,0x3,0x0,0x0},
        {0x0,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x0},
        {0x1,0x2,0x3,0x4,0x1,0x1,0x4,0x3,0x2,0x1}
    },
    { /* level11 */
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3},
        {0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x7,0x7,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x0,0x7,0x7,0x7,0x7,0x0,0x0,0x2},
        {0x0,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x0},
        {0x0,0x2,0x0,0x1,0x0,0x0,0x1,0x0,0x2,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5}
    },
    { /* level 12 */
        {0x2,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x2},
        {0x1,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1},
        {0x1,0x1,0x1,0x0,0x1,0x1,0x0,0x1,0x1,0x1},
        {0x0,0x1,0x0,0x1,0x6,0x6,0x1,0x0,0x1,0x0},
        {0x0,0x0,0x1,0x1,0x6,0x6,0x1,0x1,0x0,0x0},
        {0x1,0x1,0x1,0x7,0x0,0x0,0x7,0x1,0x1,0x1},
        {0x1,0x1,0x7,0x1,0x0,0x0,0x1,0x7,0x1,0x1},
        {0x2,0x2,0x0,0x2,0x2,0x2,0x2,0x0,0x2,0x2}
    },
    {/* levell13 */
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x0,0x2},
        {0x2,0x0,0x2,0x3,0x3,0x3,0x3,0x3,0x0,0x2},
        {0x2,0x0,0x2,0x4,0x4,0x4,0x4,0x4,0x0,0x2},
        {0x2,0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level14 */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x4,0x4,0x4,0x4,0x2,0x2,0x4,0x4,0x4,0x4},
        {0x4,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x4},
        {0x4,0x0,0x0,0x2,0x3,0x3,0x2,0x0,0x0,0x4},
        {0x4,0x0,0x2,0x23,0x3,0x3,0x23,0x2,0x0,0x4},
        {0x4,0x0,0x2,0x22,0x2,0x2,0x22,0x2,0x0,0x4},
        {0x4,0x0,0x6,0x21,0x5,0x5,0x21,0x6,0x0,0x4},
        {0x4,0x6,0x1,0x1,0x5,0x5,0x1,0x1,0x6,0x4}
    },
    {/* level 15 */
        {0x4,0x4,0x4,0x4,0x4,0x3,0x3,0x3,0x3,0x3},
        {0x2,0x2,0x1,0x1,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x2,0x1,0x1,0x1,0x0,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x5,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x1,0x2,0x2,0x2,0x1,0x3,0x5,0x3,0x0},
        {0x2,0x1,0x1,0x2,0x1,0x1,0x1,0x6,0x0,0x0},
        {0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2}
    },
    {/* level 16 (Rockbox) by ts-x */
        {0x2,0x2,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x2,0x0,0x3,0x0,0x3,0x4,0x0,0x5,0x5,0x0},
        {0x2,0x0,0x3,0x3,0x3,0x4,0x4,0x5,0x0,0x5},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x6,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0},
        {0x7,0x0,0x7,0x1,0x0,0x1,0x0,0x2,0x0,0x0},
        {0x7,0x7,0x7,0x1,0x1,0x1,0x2,0x0,0x2,0x0}
    },
    {/* level 17 (Alien) by ts-x */
        {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2},
        {0x1,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x0,0x1},
        {0x2,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x2},
        {0x1,0x0,0x1,0x2,0x2,0x2,0x2,0x1,0x0,0x1},
        {0x2,0x0,0x0,0x1,0x2,0x2,0x1,0x0,0x0,0x2},
        {0x2,0x1,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x2},
        {0x2,0x2,0x1,0x0,0x1,0x1,0x0,0x1,0x2,0x2}
    },
    {/* level 18 (Tetris) by ts-x */
        {0x0,0x2,0x0,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x0,0x2,0x7,0x0,0x3,0x4,0x0,0x2,0x2,0x0},
        {0x2,0x2,0x7,0x0,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x2,0x7,0x7,0x3,0x4,0x0,0x6,0x2,0x2},
        {0x2,0x1,0x7,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x2,0x1,0x0,0x7,0x3,0x4,0x4,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x7,0x3,0x0,0x6,0x6,0x5,0x5},
        {0x1,0x1,0x1,0x0,0x3,0x0,0x6,0x6,0x5,0x5}
    },
    { /* level 19 (Stalactites) by ts-x */
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x7,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x3,0x4,0x7,0x5,0x0,0x1,0x2},
        {0x5,0x2,0x6,0x3,0x4,0x0,0x5,0x3,0x1,0x2},
        {0x5,0x0,0x6,0x0,0x4,0x7,0x5,0x0,0x1,0x0},
        {0x5,0x0,0x0,0x3,0x4,0x0,0x0,0x0,0x1,0x2},
        {0x0,0x0,0x6,0x0,0x0,0x0,0x5,0x0,0x0,0x0},
        {0x5,0x0,0x0,0x0,0x4,0x0,0x0,0x0,0x1,0x0}
    },
    { /* level 20 (Maze) by ts-x */
        {0x1,0x1,0x21,0x1,0x1,0x1,0x1,0x1,0x1,0x21},
        {0x1,0x0,0x0,0x3,0x0,0x0,0x3,0x1,0x31,0x1},
        {0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x31,0x0,0x1},
        {0x21,0x0,0x21,0x3,0x0,0x3,0x0,0x3,0x0,0x2},
        {0x1,0x0,0x1,0x21,0x0,0x12,0x0,0x0,0x0,0x0},
        {0x31,0x0,0x1,0x0,0x0,0x1,0x0,0x0,0x3,0x0},
        {0x1,0x0,0x1,0x0,0x1,0x1,0x31,0x1,0x1,0x2},
        {0x22,0x0,0x2,0x1,0x1,0x1,0x1,0x1,0x1,0x21}
    },
    { /* level 21 (Dentist) by ts-x */
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0},
        {0x2,0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x2,0x2},
        {0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x6,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x0,0x0,0x0,0x2},
        {0x2,0x0,0x6,0x0,0x6,0x0,0x6,0x0,0x6,0x2},
        {0x2,0x2,0x6,0x0,0x6,0x0,0x6,0x0,0x2,0x2},
        {0x0,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x0}
    },
    { /* level 22 (Spider) by ts-x */
        {0x31,0x3,0x1,0x1,0x0,0x0,0x1,0x1,0x3,0x31},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x33,0x1,0x1,0x36,0x1,0x1,0x36,0x1,0x1,0x33},
        {0x0,0x0,0x1,0x1,0x1,0x1,0x1,0x1,0x0,0x0},
        {0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x1,0x0,0x0},
        {0x21,0x3,0x1,0x21,0x2,0x2,0x21,0x1,0x3,0x21},
        {0x0,0x0,0x0,0x1,0x21,0x1,0x1,0x0,0x0,0x0},
        {0x3,0x1,0x3,0x1,0x0,0x0,0x1,0x3,0x1,0x3}
    },
    { /* level 23 (Pool) by ts-x */
        {0x0,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x7,0x0},
        {0x0,0x0,0x5,0x0,0x2,0x0,0x0,0x0,0x2,0x0},
        {0x7,0x3,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x0,0x0,0x0,0x0,0x5,0x0,0x7},
        {0x7,0x0,0x4,0x0,0x0,0x3,0x0,0x0,0x0,0x7},
        {0x7,0x0,0x0,0x6,0x0,0x0,0x0,0x0,0x4,0x7},
        {0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x0,0x7,0x7,0x7,0x7,0x0,0x7,0x7,0x7,0x0}
    },
    { /* level 24 (Vorbis Fish) by ts-x */
        {0x0,0x0,0x4,0x4,0x5,0x5,0x5,0x0,0x0,0x5},
        {0x0,0x4,0x6,0x4,0x4,0x5,0x5,0x5,0x0,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x5,0x5,0x5},
        {0x5,0x6,0x0,0x6,0x4,0x4,0x4,0x4,0x5,0x5},
        {0x0,0x5,0x6,0x4,0x4,0x5,0x5,0x4,0x5,0x0},
        {0x5,0x5,0x4,0x4,0x5,0x5,0x5,0x4,0x5,0x5},
        {0x5,0x4,0x4,0x4,0x5,0x5,0x4,0x4,0x5,0x5},
        {0x0,0x0,0x4,0x4,0x4,0x4,0x4,0x5,0x0,0x5}
    },
    {/* level 25 (Rainbow) by ts-x */
        {0x0,0x4,0x1,0x0,0x0,0x0,0x0,0x1,0x4,0x0},
        {0x24,0x1,0x3,0x1,0x0,0x0,0x21,0x3,0x1,0x24},
        {0x1,0x23,0x5,0x3,0x1,0x21,0x3,0x5,0x3,0x21},
        {0x3,0x5,0x6,0x5,0x3,0x3,0x5,0x6,0x5,0x3},
        {0x5,0x6,0x7,0x6,0x5,0x5,0x6,0x7,0x6,0x5},
        {0x6,0x7,0x2,0x27,0x6,0x6,0x27,0x2,0x7,0x6},
        {0x7,0x2,0x0,0x2,0x27,0x27,0x2,0x0,0x2,0x7},
        {0x32,0x0,0x0,0x0,0x2,0x2,0x0,0x0,0x0,0x32}
    },
    { /* level 26 (Bowtie) by ts-x */
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x1,0x0,0x6,0x6,0x0,0x0,0x6,0x6,0x0,0x1},
        {0x5,0x0,0x6,0x0,0x1,0x1,0x0,0x6,0x0,0x5},
        {0x1,0x0,0x0,0x1,0x5,0x5,0x1,0x0,0x0,0x1},
        {0x5,0x1,0x5,0x1,0x0,0x0,0x1,0x5,0x1,0x5}
    },
    { /* level 27 (Frog) by ts-x */
        {0x0,0x5,0x25,0x0,0x0,0x0,0x0,0x25,0x5,0x0},
        {0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5,0x5},
        {0x25,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x25},
        {0x5,0x0,0x3,0x0,0x6,0x6,0x0,0x3,0x0,0x5},
        {0x5,0x0,0x31,0x0,0x6,0x6,0x0,0x31,0x0,0x5},
        {0x5,0x0,0x0,0x5,0x6,0x6,0x5,0x0,0x0,0x5},
        {0x5,0x5,0x5,0x35,0x0,0x0,0x35,0x5,0x5,0x5},
        {0x0,0x25,0x5,0x0,0x4,0x4,0x0,0x5,0x25,0x0}
    },
    { /* level 28 (DigDug) by ts-x */
        {0x35,0x5,0x5,0x25,0x0,0x25,0x25,0x5,0x5,0x35},
        {0x6,0x0,0x0,0x6,0x0,0x6,0x6,0x0,0x0,0x6},
        {0x7,0x0,0x37,0x37,0x0,0x37,0x37,0x7,0x0,0x7},
        {0x7,0x0,0x7,0x0,0x0,0x0,0x7,0x7,0x7,0x7},
        {0x4,0x4,0x4,0x24,0x0,0x24,0x4,0x0,0x0,0x4},
        {0x4,0x4,0x0,0x0,0x0,0x4,0x4,0x0,0x4,0x4},
        {0x24,0x24,0x4,0x4,0x4,0x4,0x0,0x0,0x24,0x4},
        {0x1,0x1,0x1,0x1,0x1,0x1,0x21,0x21,0x1,0x1}
    },
    { /* TheEnd */
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x22,0x0,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x26,0x0,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x0,0x26,0x0,0x26,0x26,0x24,0x0,0x24,0x0},
        {0x22,0x22,0x26,0x0,0x0,0x26,0x24,0x24,0x0,0x0},
        {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}
    }
};

#define MAX_BALLS 10
int pad_pos_x;
int x[MAX_BALLS],y[MAX_BALLS];
int life;
int start_game,con_game;
int pad_type;
int score=0,vscore=0;
bool flip_sides=false;
int cur_level=0;
int brick_on_board=0;
int used_balls=1;

typedef struct cube {
    int powertop;
    int power;
    char poweruse;
    char used;
    int color;
    int hits;
    int hiteffect;
} cube;
cube brick[80];

typedef struct balls {
    int pos_x;
    int pos_y;
    int y;
    int tempy;
    int x;
    int tempx;
    bool glue;
} balls;

balls ball[MAX_BALLS];

typedef struct sfire {
    int top;
    int left;
} sfire;
sfire fire[30];


int highscore;
#define MAX_POINTS 200000 /* i dont think it needs to be more */
static struct configdata config[] =
{
   {TYPE_INT, 0, MAX_POINTS, &highscore, "highscore", NULL, NULL}
};

void int_game(int new_game)
{
    int i,j;

    pad_pos_x=LCD_WIDTH/2-PAD_WIDTH/2;

    for(i=0;i<MAX_BALLS;i++) {
        ball[i].x=0;
        ball[i].y=0;
        ball[i].tempy=0;
        ball[i].tempx=0;
        ball[i].pos_y=PAD_POS_Y-BALL;
        ball[i].pos_x=pad_pos_x+(PAD_WIDTH/2)-2;
        ball[i].glue=false;
    }

    used_balls=1;
    start_game =1;
    con_game =0;
    pad_type=0;

    flip_sides=false;

    if (new_game==1)
        brick_on_board=0;

    for(i=0;i<=7;i++) {
        for(j=0;j<=9;j++) {
            brick[i*10+j].poweruse=(levels[cur_level][i][j]==0?0:1);
            if (i*10+j<=30)
                fire[i*10+j].top=-8;
            if (new_game==1) {
                brick[i*10+j].power=rb->rand()%25;
                /* +8 make the game with less powerups */

                brick[i*10+j].hits=levels[cur_level][i][j]>=10?
                    levels[cur_level][i][j]/16-1:0;
                brick[i*10+j].hiteffect=0;
                brick[i*10+j].powertop=TOPMARGIN+i*BRICK_HEIGHT+BRICK_HEIGHT;
                brick[i*10+j].used=(levels[cur_level][i][j]==0?0:1);
                brick[i*10+j].color=(levels[cur_level][i][j]>=10?
                                     levels[cur_level][i][j]%16:
                                     levels[cur_level][i][j])-1;
                if (levels[cur_level][i][j]!=0)
                    brick_on_board++;
            }
        }
    }
}

int sw,i,w;

/* sleep timer counting the score */
void sleep (int secs)
{
    bool done=false;
    char s[20];
    int count=0;

    while (!done) {

        if (vscore<score) {
            vscore++;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, &w);
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);
#else
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 2, s);
#endif
            rb->lcd_update_rect(0,0,LCD_WIDTH,w+2);
        } else {
            if (count==0)
                count=*rb->current_tick+HZ*secs;
            if (*rb->current_tick>=count)
                done=true;
        }
        rb->yield();
    }

}

#define HIGH_SCORE "brickmania.score"
#define MENU_LENGTH 4
int game_menu(int when)
{
    int button,cur=0;
    char str[10];
    rb->lcd_clear_display();
#if (LCD_WIDTH != 112) && (LCD_HEIGHT != 64)
    rb->lcd_bitmap(brickmania_menu_bg, 0, 0, BMPWIDTH_menu, BMPHEIGHT_menu);
#endif
    while (true) {
        for(i=0;i<MENU_LENGTH;i++) {
#ifdef HAVE_LCD_COLOR
            if (cur==0)
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_START, MENU_BMPWIDTH,
                               BMPXOFS_start, BMPYOFS_start, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_START, MENU_BMPWIDTH,
                               BMPXOFS_start, BMPYOFS_start, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            if (when==1) {
                if (cur==1)
                    rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
                else
                    rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            } else {
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_NO_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            }


            if (cur==2)
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_HELP, MENU_BMPWIDTH,
                               BMPXOFS_help, BMPYOFS_help, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_HELP, MENU_BMPWIDTH,
                               BMPXOFS_help, BMPYOFS_help, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            if (cur==3)
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_QUIT, MENU_BMPWIDTH,
                               BMPXOFS_quit, BMPYOFS_quit, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_transparent_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_QUIT, MENU_BMPWIDTH,
                               BMPXOFS_quit, BMPYOFS_quit, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
#else
            if (cur==0)
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_START, MENU_BMPWIDTH,
                               BMPXOFS_start, BMPYOFS_start, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_START, MENU_BMPWIDTH,
                               BMPXOFS_start, BMPYOFS_start, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            if (when==1) {
                if (cur==1)
                    rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
                else
                    rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            } else {
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_NO_RESUME, MENU_BMPWIDTH,
                               BMPXOFS_resume, BMPYOFS_resume, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            }


            if (cur==2)
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_HELP, MENU_BMPWIDTH,
                               BMPXOFS_help, BMPYOFS_help, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_HELP, MENU_BMPWIDTH,
                               BMPXOFS_help, BMPYOFS_help, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);

            if (cur==3)
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_SEL_QUIT, MENU_BMPWIDTH,
                               BMPXOFS_quit, BMPYOFS_quit, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
            else
                rb->lcd_bitmap_part(brickmania_menu_items, 0,
                               MENU_BMPHEIGHT * BM_QUIT, MENU_BMPWIDTH,
                               BMPXOFS_quit, BMPYOFS_quit, MENU_BMPWIDTH,
                               MENU_BMPHEIGHT);
#endif
        }
        rb->lcd_set_drawmode(DRMODE_FG);
        /* high score */
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_background(LCD_RGBPACK(0,0,140));
        rb->lcd_set_foreground(LCD_WHITE);
#endif
        rb->lcd_putsxy(HIGHSCORE_XPOS, HIGHSCORE_YPOS, "High Score");
        rb->snprintf(str, sizeof(str), "%d", highscore);
        rb->lcd_getstringsize("High Score", &sw, NULL);
        rb->lcd_getstringsize(str, &w, NULL);
        rb->lcd_putsxy(HIGHSCORE_XPOS+sw/2-w/2, HIGHSCORE_YPOS+9, str);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        rb->lcd_update();

        button = rb->button_get(true);
        switch(button) {
            case UP:
                if (cur==0)
                    cur = MENU_LENGTH-1;
                else
                    cur--;
                if (when==0 && cur==1) {
                    cur = 0;
                }
                break;

            case DOWN:
                if (cur==MENU_LENGTH-1)
                    cur = 0;
                else
                    cur++;
                if (when==0 && cur==1) {
                    cur=2;
                }
                break;

            case RIGHT:
            case SELECT:
                if (cur==0) {
                    score=0;
                    vscore=0;
                    return 0;
                } else if (cur==1 && when==1) {
                    return 1;
                } else if (cur==2) {
                    return 2;
                } else if (cur==3) {
                    return 3;
                }
                break;
#ifdef RC_QUIT
            case RC_QUIT:
#endif
            case QUIT:
                return 3;
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return 3;
                break;
        }
    }
}

int help(int when)
{
    int w,h;
    int button;
    int xoffset=0;
    int yoffset=0;
    /* set the maximum x and y in the helpscreen
       dont forget to update, if you change text */
    int maxY=180;
    int maxX=215;

    while(true) {
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_clear_display();
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_set_foreground(LCD_WHITE);
#else
        rb->lcd_clear_display();
#endif

        rb->lcd_getstringsize("BrickMania", &w, &h);
        rb->lcd_putsxy(LCD_WIDTH/2-w/2+xoffset, 1+yoffset, "BrickMania");

#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1+xoffset, 1*(h+2)+yoffset,"Aim");
        rb->lcd_set_foreground(LCD_WHITE);
#else
        rb->lcd_putsxy(1+xoffset, 1*(h+2)+yoffset,"Aim");
#endif
        rb->lcd_putsxy(1+xoffset, 2*(h+2)+yoffset,
                       "destroy all the bricks by bouncing");
        rb->lcd_putsxy(1+xoffset, 3*(h+2)+yoffset,
                       "the ball of them using the paddle.");
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1+xoffset, 5*(h+2)+yoffset,"Controls");
        rb->lcd_set_foreground(LCD_WHITE);
#else
        rb->lcd_putsxy(1+xoffset, 5*(h+2)+yoffset,"Controls");
#endif
        rb->lcd_putsxy(1+xoffset, 6*(h+2)+yoffset,"< & > Move the paddle");
#if CONFIG_KEYPAD == ONDIO_PAD
        rb->lcd_putsxy(1+xoffset, 7*(h+2)+yoffset,
                       "MENU  Releases the ball/Fire!");
#elif CONFIG_KEYPAD == RECORDER_PAD
        rb->lcd_putsxy(1+xoffset, 7*(h+2)+yoffset,
                       "PLAY  Releases the ball/Fire!");
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
        rb->lcd_putsxy(1+xoffset, 7*(h+2)+yoffset,
                       "NAVI  Releases the ball/Fire!");
#else
        rb->lcd_putsxy(1+xoffset, 7*(h+2)+yoffset,
                       "SELECT  Releases the ball/Fire!");
#endif
        rb->lcd_putsxy(1+xoffset, 8*(h+2)+yoffset, "STOP  Opens menu/Quit");
#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        rb->lcd_putsxy(1+xoffset, 10*(h+2)+yoffset, "Specials");
        rb->lcd_set_foreground(LCD_WHITE);
#else
        rb->lcd_putsxy(1+xoffset, 10*(h+2)+yoffset, "Specials");
#endif
        rb->lcd_putsxy(1+xoffset, 11*(h+2)+yoffset,
                       "N  Normal:returns paddle to normal");
        rb->lcd_putsxy(1+xoffset, 12*(h+2)+yoffset, "D  DIE!:loses a life");
        rb->lcd_putsxy(1+xoffset, 13*(h+2)+yoffset,
                       "L  Life:gains a life/power up");
        rb->lcd_putsxy(1+xoffset, 14*(h+2)+yoffset,
                       "F  Fire:allows you to shoot bricks");
        rb->lcd_putsxy(1+xoffset, 15*(h+2)+yoffset,
                       "G  Glue:ball sticks to paddle");
        rb->lcd_putsxy(1+xoffset, 16*(h+2)+yoffset,
                       "B  Ball:generates another ball");
        rb->lcd_putsxy(1+xoffset, 17*(h+2)+yoffset,
                       "FL Flip:flips left / right movement");
        rb->lcd_update();

        button=rb->button_get(true);
        switch (button) {
#ifdef RC_QUIT
            case RC_QUIT:
#endif
            case QUIT:
                switch (game_menu(when)) {
                    case 0:
                        cur_level=0;
                        life=2;
                        int_game(1);
                        break;
                    case 1:
                        con_game=1;
                        break;
                    case 2:
                        if (help(when)==1)
                            return 1;
                        break;
                    case 3:
                        return 1;
                        break;
                }
                return 0;
                break;
            case LEFT:
            case LEFT | BUTTON_REPEAT:
                if( xoffset<0)
                    xoffset+=2;
                break;
            case RIGHT:
            case RIGHT | BUTTON_REPEAT:
                if(xoffset+maxX > LCD_WIDTH)
                    xoffset-=2;
                break;
            case UP:
            case UP | BUTTON_REPEAT:
                if(yoffset <0)
                    yoffset+=2;
                break;
            case DOWN:
            case DOWN | BUTTON_REPEAT:
                if(yoffset+maxY > LCD_HEIGHT)
                    yoffset-=2;
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
    }
    return 0;
}

int pad_check(int ballxc, int mode, int pon ,int ballnum)
{
    /* pon: positive(1) or negative(0) */

    if (mode==0) {
        if (pon == 0)
            return -ballxc;
        else
            return ballxc;
    } else {
        if (ball[ballnum].x > 0)
            return ballxc;
        else
           return ballxc*-1;
    }
}

int fire_space(void)
{
    int t;
    for(t=0;t<=30;t++)
        if (fire[t].top+7 < 0)
            return t;

    return 0;
}

int game_loop(void)
{
    int j,i,k,bricky,brickx;
    char s[30];
    int sec_count=0,num_count=10;
    int end;

    rb->srand( *rb->current_tick );

    configfile_init(rb);
    configfile_load(HIGH_SCORE,config,1,0);

    switch(game_menu(0)) {
        case 0:
            cur_level = 0;
            life = 2;
            int_game(1);
            break;
        case 1:
            con_game = 1;
            break;
        case 2:
            if (help(0) == 1) return 1;
            break;
        case 3:
            return 1;
            break;
    }

    while(true) {
        /* Convert CYCLETIME (in ms) to HZ */
        end = *rb->current_tick + (CYCLETIME * HZ) / 1000;

        if (life >= 0) {
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_background(LCD_BLACK);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_clear_display();
            rb->lcd_set_background(LCD_BLACK);
            rb->lcd_set_foreground(LCD_WHITE);
#else
            rb->lcd_clear_display();
#endif

            if (flip_sides) {
                if (*rb->current_tick>=sec_count) {
                    sec_count=*rb->current_tick+HZ;
                    if (num_count!=0)
                        num_count--;
                    else
                        flip_sides=false;
                }
                rb->snprintf(s, sizeof(s), "%d", num_count);
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-2, STRINGPOS_flipsides, s);
            }

            /* write life num */
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->snprintf(s, sizeof(s), "L:%d", life);
            rb->lcd_putsxy(0, 0, s);
#else
            rb->snprintf(s, sizeof(s), "Life: %d", life);
            rb->lcd_putsxy(2, 2, s);
#endif

#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->snprintf(s, sizeof(s), "L%d", cur_level+1);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw, 0, s);
#else
            rb->snprintf(s, sizeof(s), "Level %d", cur_level+1);
            rb->lcd_getstringsize(s, &sw, NULL);
            rb->lcd_putsxy(LCD_WIDTH-sw-2, 2, s);
#endif

            if (vscore<score) vscore++;
            rb->snprintf(s, sizeof(s), "%d", vscore);
            rb->lcd_getstringsize(s, &sw, NULL);
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 0, s);
#else
            rb->lcd_putsxy(LCD_WIDTH/2-sw/2, 2, s);
#endif

            /* continue game */
            if (con_game== 1 && start_game!=1) {
#if CONFIG_KEYPAD == ONDIO_PAD
                rb->snprintf(s, sizeof(s), "MENU To Continue");
#elif CONFIG_KEYPAD == IRIVER_H300_PAD
                rb->snprintf(s, sizeof(s), "Press NAVI To Continue");
#else
                rb->snprintf(s, sizeof(s), "Press SELECT To Continue");
#endif
                rb->lcd_getstringsize(s, &sw, NULL);
                rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_navi, s);

                sec_count=*rb->current_tick+HZ;
            }

            /* draw the ball */
            for(i=0;i<used_balls;i++)
                rb->lcd_bitmap(brickmania_ball,ball[i].pos_x, ball[i].pos_y,
                               BALL, BALL);

            if (brick_on_board==0)
                brick_on_board--;

            /* if the pad is fire */
            for(i=0;i<=30;i++) {
                if (fire[i].top+7>0) {
                    if (con_game!=1)
                        fire[i].top-=4;
                    rb->lcd_vline(fire[i].left, fire[i].top, fire[i].top+7);
                }
            }

            /* the bricks */
            for (i=0;i<=7;i++) {
                for (j=0;j<=9;j++) {
                    if (brick[i*10+j].power<7) {
                        if (brick[i*10+j].poweruse==2) {
                            if (con_game!=1)
                                brick[i*10+j].powertop+=2;
                            rb->lcd_bitmap_part(brickmania_powerups,0,
                                                BMPHEIGHT_powerup*brick[i*10+j
                                                    ].power,
                                                BMPWIDTH_powerup,
                                                LEFTMARGIN+j*BRICK_WIDTH+
                                                (BRICK_WIDTH/2-
                                                 BMPWIDTH_powerup/2),
                                                brick[i*10+j].powertop,
                                                BMPWIDTH_powerup,
                                                BMPHEIGHT_powerup);
                        }
                    }

                    if ((pad_pos_x<LEFTMARGIN+j*BRICK_WIDTH+5 &&
                         pad_pos_x+PAD_WIDTH>LEFTMARGIN+j*BRICK_WIDTH+5) &&
                        brick[i*10+j].powertop+6>=PAD_POS_Y &&
                        brick[i*10+j].poweruse==2) {
                        switch(brick[i*10+j].power) {
                            case 0:
                                life++;
                                score+=50;
                                break;
                            case 1:
                                life--;
                                if (life>=0) {
                                    int_game(0);
                                    sleep(2);
                                }
                                break;
                            case 2:
                                score+=34;
                                pad_type=1;
                                break;
                            case 3:
                                score+=47;
                                pad_type=2;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                break;
                            case 4:
                                score+=23;
                                pad_type=0;
                                for(k=0;k<used_balls;k++)
                                    ball[k].glue=false;
                                flip_sides=false;
                                break;
                            case 5:
                                score+=23;
                                sec_count=*rb->current_tick+HZ;
                                num_count=10;
                                flip_sides=!flip_sides;
                                break;
                            case 6:
                                score+=23;
                                used_balls++;
                                ball[used_balls-1].x= rb->rand()%1 == 0 ?
                                    -1 : 1;
                                ball[used_balls-1].y= -4;
                                break;
                        }
                        brick[i*10+j].poweruse=1;
                    }

                    if (brick[i*10+j].powertop>PAD_POS_Y)
                        brick[i*10+j].poweruse=1;

                    brickx=LEFTMARGIN+j*BRICK_WIDTH;
                    bricky=TOPMARGIN+i*BRICK_HEIGHT;
                    if (pad_type==2) {
                        for (k=0;k<=30;k++) {
                            if (fire[k].top+7>0) {
                                if (brick[i*10+j].used==1 &&
                                    (fire[k].left+1 >= brickx &&
                                     fire[k].left+1 <= brickx+BRICK_WIDTH) &&
                                    (bricky+BRICK_HEIGHT>fire[k].top)) {
                                    score+=13;
                                    fire[k].top=-16;
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=3;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                        brick_on_board--;
                                    }
                                }
                            }
                        }
                    }

                    if (brick[i*10+j].used==1) {
                        rb->lcd_bitmap_part(brickmania_bricks,0,
                                            BRICK_HEIGHT*brick[i*10+j].color,
                                            BRICK_WIDTH,
                                            LEFTMARGIN+j*BRICK_WIDTH,
                                            TOPMARGIN+i*BRICK_HEIGHT,
                                            BRICK_WIDTH, BRICK_HEIGHT);
#ifdef HAVE_LCD_COLOR  /* No transparent effect for greyscale lcds for now */
                        if (brick[i*10+j].hiteffect>0)
                            rb->lcd_bitmap_transparent_part(brickmania_break,
                                                            0,
                                                            BRICK_HEIGHT*brick[i*10+j].hiteffect,
                                                            BRICK_WIDTH,
                                                            LEFTMARGIN+j*BRICK_WIDTH,
                                                            TOPMARGIN+i*BRICK_HEIGHT,
                                                            BRICK_WIDTH,
                                                            BRICK_HEIGHT);
#endif
                    }

                    for(k=0;k<used_balls;k++) {
                        if (ball[k].pos_y <160) {
                            if (brick[i*10+j].used==1) {
                                if ((ball[k].pos_x+ball[k].x+HALFBALL >=
                                     brickx &&
                                     ball[k].pos_x+ball[k].x+HALFBALL <=
                                     brickx+BRICK_WIDTH) &&
                                    ((bricky-4<ball[k].pos_y+BALL &&
                                      bricky>ball[k].pos_y+BALL) ||
                                     (bricky+4>ball[k].pos_y+BALL+BALL &&
                                      bricky<ball[k].pos_y+BALL+BALL)) &&
                                    (ball[k].y >0)) {
                                    ball[k].tempy=bricky-ball[k].pos_y-BALL;
                                }
                                else if ((ball[k].pos_x+ball[k].x+HALFBALL >=
                                          brickx &&
                                          ball[k].pos_x+ball[k].x+HALFBALL <=
                                          brickx+BRICK_WIDTH) &&
                                         ((bricky+BRICK_HEIGHT+4>ball[k].pos_y &&
                                           bricky+BRICK_HEIGHT<ball[k].pos_y) ||
                                          (bricky+BRICK_HEIGHT-4<ball[k].pos_y-BALL &&
                                           bricky+BRICK_HEIGHT>ball[k].pos_y-BALL)) &&
                                         (ball[k].y <0)) {
                                    ball[k].tempy=
                                        -(ball[k].pos_y-(bricky+BRICK_HEIGHT));
                                }

                                if ((ball[k].pos_y+HALFBALL >=
                                     bricky &&
                                     ball[k].pos_y+HALFBALL <=
                                     bricky+BRICK_HEIGHT) &&
                                    ((brickx-4<ball[k].pos_x+BALL &&
                                      brickx>ball[k].pos_x+BALL) ||
                                     (brickx+4>ball[k].pos_x+BALL+BALL &&
                                      brickx<ball[k].pos_x+BALL+BALL)) &&
                                    (ball[k].x >0)) {
                                    ball[k].tempx=brickx-ball[k].pos_x-BALL;
                                }
                                else if ((ball[k].pos_y+ball[k].y+HALFBALL >=
                                          bricky &&
                                          ball[k].pos_y+ball[k].y+HALFBALL <=
                                          bricky+BRICK_HEIGHT) &&
                                         ((brickx+BRICK_WIDTH+4>ball[k].pos_x &&
                                           brickx+BRICK_WIDTH<ball[k].pos_x) ||
                                          (brickx+BRICK_WIDTH-4<ball[k].pos_x-
                                           BALL &&
                                           brickx+BRICK_WIDTH>ball[k].pos_x-
                                           BALL)) && (ball[k].x <0)) {
                                    ball[k].tempx=
                                        -(ball[k].pos_x-(brickx+BRICK_WIDTH));
                                }

                                if ((ball[k].pos_x+HALFBALL >= brickx &&
                                     ball[k].pos_x+HALFBALL <=
                                     brickx+BRICK_WIDTH) &&
                                    ((bricky+BRICK_HEIGHT==ball[k].pos_y) ||
                                     (bricky+BRICK_HEIGHT-6<=ball[k].pos_y &&
                                      bricky+BRICK_HEIGHT>ball[k].pos_y)) &&
                                    (ball[k].y <0)) { /* bottom line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].y = ball[k].y*-1;
                                }
                                else if ((ball[k].pos_x+HALFBALL >= brickx &&
                                          ball[k].pos_x+HALFBALL <=
                                          brickx+BRICK_WIDTH) &&
                                         ((bricky==ball[k].pos_y+BALL) ||
                                          (bricky+6>=ball[k].pos_y+BALL &&
                                           bricky<ball[k].pos_y+BALL)) &&
                                         (ball[k].y >0)) { /* top line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].y = ball[k].y*-1;
                                }

                                if ((ball[k].pos_y+HALFBALL >= bricky &&
                                     ball[k].pos_y+HALFBALL <=
                                     bricky+BRICK_HEIGHT) &&
                                    ((brickx==ball[k].pos_x+BALL) ||
                                     (brickx+6>=ball[k].pos_x+BALL &&
                                      brickx<ball[k].pos_x+BALL)) &&
                                    (ball[k].x > 0)) { /* left line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }
                                    ball[k].x = ball[k].x*-1;

                                }
                                else if ((ball[k].pos_y+HALFBALL >= bricky &&
                                          ball[k].pos_y+HALFBALL <=
                                          bricky+BRICK_HEIGHT) &&
                                         ((brickx+BRICK_WIDTH==
                                           ball[k].pos_x) ||
                                          (brickx+BRICK_WIDTH-6<=
                                           ball[k].pos_x &&
                                           brickx+BRICK_WIDTH>
                                           ball[k].pos_x)) &&
                                         (ball[k].x < 0)) { /* Right line */
                                    if (brick[i*10+j].hits > 0) {
                                        brick[i*10+j].hits--;
                                        brick[i*10+j].hiteffect++;
                                        score+=2;
                                    }
                                    else {
                                        brick[i*10+j].used=0;
                                        if (brick[i*10+j].power!=10)
                                            brick[i*10+j].poweruse=2;
                                    }

                                    ball[k].x = ball[k].x*-1;
                                }

                                if (brick[i*10+j].used==0) {
                                    brick_on_board--;
                                    score+=8;
                                }
                            }
                        }
                    } /* for k */
                } /* for j */
            } /* for i */

            /* draw the pad */
            rb->lcd_bitmap_part(brickmania_pads,0,pad_type*PAD_HEIGHT,
                                PAD_WIDTH,pad_pos_x, PAD_POS_Y, PAD_WIDTH,
                                PAD_HEIGHT);

            for(k=0;k<used_balls;k++) {

                if ((ball[k].pos_x >= pad_pos_x &&
                     ball[k].pos_x <= pad_pos_x+PAD_WIDTH) &&
                    (PAD_POS_Y-4<ball[k].pos_y+BALL &&
                     PAD_POS_Y>ball[k].pos_y+BALL) && (ball[k].y >0))
                    ball[k].tempy=PAD_POS_Y-ball[k].pos_y-BALL;
                else if ((4>ball[k].pos_y && 0<ball[k].pos_y) &&
                         (ball[k].y <0))
                    ball[k].tempy=-ball[k].pos_y;
                if ((LCD_WIDTH-4<ball[k].pos_x+BALL &&
                     LCD_WIDTH>ball[k].pos_x+BALL) && (ball[k].x >0))
                    ball[k].tempx=LCD_WIDTH-ball[k].pos_x-BALL;
                else if ((4>ball[k].pos_x && 0<ball[k].pos_x) &&
                         (ball[k].x <0))
                    ball[k].tempx=-ball[k].pos_x;

                /* top line */
                if (ball[k].pos_y<= 0)
                    ball[k].y = ball[k].y*-1;
                /* bottom line */
                else if (ball[k].pos_y+BALL >= LCD_HEIGHT) {
                    if (used_balls>1) {
                        used_balls--;
                        ball[k].pos_x = ball[used_balls].pos_x;
                        ball[k].pos_y = ball[used_balls].pos_y;
                        ball[k].y = ball[used_balls].y;
                        ball[k].tempy = ball[used_balls].tempy;
                        ball[k].x = ball[used_balls].x;
                        ball[k].tempx = ball[used_balls].tempx;
                        ball[k].glue = ball[used_balls].glue;

                        ball[used_balls].x=0;
                        ball[used_balls].y=0;
                        ball[used_balls].tempy=0;
                        ball[used_balls].tempx=0;
                        ball[used_balls].pos_y=PAD_POS_Y-BALL;
                        ball[used_balls].pos_x=pad_pos_x+(PAD_WIDTH/2)-2;

                        k--;
                        continue;
                    } else {
                        life--;
                        if (life>=0) {
                            int_game(0);
                            sleep(2);
                        }
                    }
                }

                /* left line ,right line */
                if ((ball[k].pos_x <= 0) ||
                    (ball[k].pos_x+BALL >= LCD_WIDTH)) {
                    ball[k].x = ball[k].x*-1;
                    ball[k].pos_x = ball[k].pos_x <= 0 ? 0 : LCD_WIDTH-BALL;
                }

                if ((ball[k].pos_y+BALL >= PAD_POS_Y &&
                     (ball[k].pos_x >= pad_pos_x &&
                      ball[k].pos_x <= pad_pos_x+PAD_WIDTH)) &&
                    start_game != 1 && !ball[k].glue) {

                    if ((ball[k].pos_x+HALFBALL >= pad_pos_x &&
                         ball[k].pos_x+HALFBALL <=
                         pad_pos_x+(PAD_WIDTH/2/4)) ||
                        (ball[k].pos_x +HALFBALL>=
                         pad_pos_x+(PAD_WIDTH-(PAD_WIDTH/2/4)) &&
                         ball[k].pos_x+HALFBALL <= pad_pos_x+PAD_WIDTH)) {

                        ball[k].y = -2;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = pad_check(6,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+2*(PAD_WIDTH/2/4)) ||
                             (ball[k].pos_x+HALFBALL >=
                              pad_pos_x+(PAD_WIDTH-2*(PAD_WIDTH/2/4)) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+(PAD_WIDTH-(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -3;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = pad_check(4,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+2*(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+3*(PAD_WIDTH/2/4)) ||
                             (ball[k].pos_x+2 >=
                              pad_pos_x+(PAD_WIDTH-3*(PAD_WIDTH/2/4)) &&
                              ball[k].pos_x+2 <=
                              pad_pos_x+ ((PAD_WIDTH/2)-2*(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -4;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = pad_check(3,0,ball[k].pos_x+2<=
                                                  pad_pos_x+(PAD_WIDTH/2)?
                                                  0:1,k);

                    }
                    else if ((ball[k].pos_x+HALFBALL >=
                              pad_pos_x+3*(PAD_WIDTH/2/4) &&
                              ball[k].pos_x+HALFBALL <=
                              pad_pos_x+4*(PAD_WIDTH/2/4)-2) ||
                             (ball[k].pos_x+2 >= pad_pos_x+(PAD_WIDTH/2+2) &&
                              ball[k].pos_x+2 <=
                              pad_pos_x+(PAD_WIDTH-3*(PAD_WIDTH/2/4)) )) {

                        ball[k].y = -4;
                        if (ball[k].pos_x != 0 &&
                            ball[k].pos_x+BALL!=LCD_WIDTH)
                            ball[k].x = pad_check(2,1,0,k);

                    }
                    else {
                        ball[k].y = -4;
                    }
                }

                if (!ball[k].glue) {
                    ball[k].pos_x+=ball[k].tempx!=0?ball[k].tempx:ball[k].x;
                    ball[k].pos_y+=ball[k].tempy!=0?ball[k].tempy:ball[k].y;

                    ball[k].tempy=0;
                    ball[k].tempx=0;
                }

                if (ball[k].pos_y+5 >= PAD_POS_Y &&
                    (pad_type==1 && !ball[k].glue) &&
                    (ball[k].pos_x >= pad_pos_x &&
                     ball[k].pos_x <= pad_pos_x+PAD_WIDTH)) {
                    ball[k].y=0;
                    ball[k].pos_y=PAD_POS_Y-BALL;
                    ball[k].glue=true;
                }
            } /* for k */

            rb->lcd_update();

            if (brick_on_board < 0) {
                if (cur_level+1<levels_num) {
                    cur_level++;
                    score+=100;
                    int_game(1);
                    sleep(2);
                }
                else {
                    rb->lcd_getstringsize("Congratulations!", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_congrats,
                                   "Congratulations!");
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
                    rb->lcd_getstringsize("No more levels", &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_finsh,
                                   "No more levels");
#else
                    rb->lcd_getstringsize("You have finished the game!",
                                          &sw, NULL);
                    rb->lcd_putsxy(LCD_WIDTH/2-sw/2, STRINGPOS_finsh,
                                   "You have finished the game!");
#endif
                    vscore=score;
                    rb->lcd_update();
                    if (score>highscore) {
                        sleep(2);
                        highscore=score;
                        rb->splash(HZ*2,true,"New High Score");
                    }
                    else {
                        sleep(3);
                    }

                    switch(game_menu(0)) {
                        case 0:
                            life=2;
                            cur_level=0;
                            int_game(1);
                            break;
                        case 1:
                            con_game=1;
                            break;
                        case 2:
                            if (help(0)==1) return 1;
                            break;
                        case 3:
                            return 1;
                            break;
                    }
                }
            }

            int move_button,button;
            int button_right,button_left;
            button=rb->button_get(false);

#ifdef HAS_BUTTON_HOLD
            if (rb->button_hold())
            button = QUIT;
#endif

            move_button=rb->button_status();

            button_right=((move_button & RIGHT) || (SCROLL_FWD(button)));
            button_left=((move_button & LEFT) || (SCROLL_BACK(button)));
            if ((con_game== 1 && start_game!=1) && (button_right || button_left))
                continue;
            if ((button_right && flip_sides==false) ||
                (button_left && flip_sides==true)) {
                if (pad_pos_x+8+PAD_WIDTH > LCD_WIDTH) {
                    for(k=0;k<used_balls;k++)
                        if (start_game==1 || ball[k].glue)
                            ball[k].pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                    pad_pos_x+=LCD_WIDTH-pad_pos_x-PAD_WIDTH;
                }
                else {
                    for(k=0;k<used_balls;k++)
                        if ((start_game==1 || ball[k].glue))
                            ball[k].pos_x+=8;
                    pad_pos_x+=8;
                }
            }
            else if ((button_left && flip_sides==false) ||
                     (button_right && flip_sides==true)) {
                if (pad_pos_x-8 < 0) {
                    for(k=0;k<used_balls;k++)
                        if (start_game==1 || ball[k].glue)
                            ball[k].pos_x-=pad_pos_x;
                    pad_pos_x-=pad_pos_x;
                }
                else {
                    for(k=0;k<used_balls;k++)
                        if (start_game==1 || ball[k].glue)
                            ball[k].pos_x-=8;
                    pad_pos_x-=8;
                }
            }


            switch(button) {
                case UP:
                case SELECT:
                    if (start_game==1 && con_game!=1 && pad_type!=1) {
                        for(k=0;k<used_balls;k++) {
                            ball[k].y=-4;
                            ball[k].x=pad_pos_x+(PAD_WIDTH/2)-2>=
                                LCD_WIDTH/2?2:-2;
                        }
                        start_game =0;
                    }
                    else if (pad_type==1) {
                        for(k=0;k<used_balls;k++) {
                            if (ball[k].glue)
                                ball[k].glue=false;
                            else if (start_game==1) {
                                ball[k].x = x[k];
                                ball[k].y = y[k];
                            }
                        }

                        if (start_game!=1 && con_game==1) {
                            start_game =0;
                            con_game=0;
                        }
                    } else if (pad_type==2 && con_game!=1) {
                        int tfire;
                        tfire=fire_space();
                        fire[tfire].top=PAD_POS_Y-7;
                        fire[tfire].left=pad_pos_x+1;
                        tfire=fire_space();
                        fire[tfire].top=PAD_POS_Y-7;
                        fire[tfire].left=pad_pos_x+PAD_WIDTH-1;
                    } else if (con_game==1 && start_game!=1) {
                        for(k=0;k<used_balls;k++) {
                            ball[k].x=x[k];
                            ball[k].y=y[k];
                        }
                        con_game=0;
                    }
                    break;
#ifdef RC_QUIT
                case RC_QUIT:
#endif
                case QUIT:
                    switch(game_menu(1)) {
                        case 0:
                            life=2;
                            cur_level=0;
                            int_game(1);
                            break;
                        case 1:
                            for(k=0;k<used_balls;k++)
                                if (ball[k].x!=0 && ball[k].y !=0)
                                    con_game=1;
                            break;
                        case 2:
                            if (help(1)==1)
                                return 1;
                            break;
                        case 3:
                            return 1;
                            break;
                    }

                    for(k=0;k<used_balls;k++) {
                        if (ball[k].x!=0)
                            x[k]=ball[k].x;
                        ball[k].x=0;
                        if (ball[k].y!=0)
                            y[k]=ball[k].y;
                        ball[k].y=0;
                    }

                    break;

                default:
                    if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                        return 1;
                    break;
            }
        }
        else {
#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
            rb->splash(HZ*2,true,"Game Over");
#else
            rb->lcd_bitmap(brickmania_gameover,LCD_WIDTH/2-55,LCD_HEIGHT-87,
                           110,52);
            rb->lcd_update();
#endif
            if (score>highscore) {
                sleep(2);
                highscore=score;
                rb->splash(HZ*2,true,"New High Score");
            } else {
                sleep(3);
            }

            for(k=0;k<used_balls;k++) {
                ball[k].x=0;
                ball[k].y=0;
            }

            switch(game_menu(0)) {
                case 0:
                    cur_level=0;
                    life=2;
                    int_game(1);
                    break;
                case 1:
                    con_game=1;
                    break;
                case 2:
                    if (help(0)==1)
                        return 1;
                    break;
                case 3:
                    return 1;
                    break;
            }
        }
        if (end > *rb->current_tick)
            rb->sleep(end-*rb->current_tick);
        else
            rb->yield();
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Permanently enable the backlight (unless the user has turned it off) */
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);

    /* now go ahead and have fun! */
    while (game_loop()!=1);

    configfile_save(HIGH_SCORE,config,1,0);

    /* Restore user's original backlight setting */
    rb->lcd_setfont(FONT_UI);
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);

    return PLUGIN_OK;
}
