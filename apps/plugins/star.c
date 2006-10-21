/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Frederic Dang Ngoc
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

/* title of the game */
#define STAR_TITLE      "Star"

/* font used to display title */
#define STAR_TITLE_FONT  2

/* size of a level in file */
#define STAR_LEVEL_SIZE    ((STAR_WIDTH + 1) * STAR_HEIGHT + 1)

/* size of the game board */
#define STAR_WIDTH      16
#define STAR_HEIGHT      9

/* number of level */
#define STAR_LEVEL_COUNT 20

/* values of object in the board */
#define STAR_VOID       '.'
#define STAR_WALL       '*'
#define STAR_STAR       'o'
#define STAR_BALL       'X'
#define STAR_BLOCK      'x'

/* sleep time between two frames */
#if (LCD_HEIGHT * LCD_WIDTH >= 70000) /* iPod 5G LCD is *slow* */
#define STAR_SLEEP rb->yield();
#elif (LCD_HEIGHT * LCD_WIDTH >= 30000)
#define STAR_SLEEP rb->sleep(0);
#else
#define STAR_SLEEP rb->sleep(1);
#endif

/* value of ball and block control */
#define STAR_CONTROL_BALL  0
#define STAR_CONTROL_BLOCK 1

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define STAR_QUIT           BUTTON_OFF
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_ON
#define STAR_TOGGLE_CONTROL2 BUTTON_PLAY
#define STAR_LEVEL_UP       BUTTON_F3
#define STAR_LEVEL_DOWN     BUTTON_F1
#define STAR_LEVEL_REPEAT   BUTTON_F2
#define STAR_MENU_RUN       BUTTON_PLAY
#define STAR_MENU_RUN2      BUTTON_RIGHT
#define STAR_MENU_RUN3      BUTTON_ON

#elif CONFIG_KEYPAD == ONDIO_PAD
#define STAR_QUIT           BUTTON_OFF
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL_PRE BUTTON_MENU
#define STAR_TOGGLE_CONTROL (BUTTON_MENU | BUTTON_REL)
#define STAR_LEVEL_UP       (BUTTON_MENU | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_MENU | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_MENU | BUTTON_UP)
#define STAR_MENU_RUN       BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define STAR_QUIT           BUTTON_OFF
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_MODE
#define STAR_TOGGLE_CONTROL2 BUTTON_SELECT
#define STAR_LEVEL_UP       (BUTTON_ON | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_ON | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_ON | BUTTON_SELECT)
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#define STAR_RC_QUIT BUTTON_RC_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)

#define STAR_QUIT           (BUTTON_SELECT | BUTTON_MENU)
#define STAR_UP             BUTTON_MENU
#define STAR_DOWN           BUTTON_PLAY
#define STAR_TOGGLE_CONTROL_PRE BUTTON_SELECT
#define STAR_TOGGLE_CONTROL (BUTTON_SELECT | BUTTON_REL)
#define STAR_LEVEL_UP       (BUTTON_SELECT | BUTTON_RIGHT)
#define STAR_LEVEL_DOWN     (BUTTON_SELECT | BUTTON_LEFT)
#define STAR_LEVEL_REPEAT   (BUTTON_SELECT | BUTTON_PLAY)
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_SELECT
#define STAR_LEVEL_UP_PRE   BUTTON_REC
#define STAR_LEVEL_UP       (BUTTON_REC|BUTTON_REL)
#define STAR_LEVEL_DOWN_PRE BUTTON_REC
#define STAR_LEVEL_DOWN     (BUTTON_REC|BUTTON_REPEAT)
#define STAR_LEVEL_REPEAT   BUTTON_PLAY
#define STAR_MENU_RUN       BUTTON_RIGHT
#define STAR_MENU_RUN2      BUTTON_SELECT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define STAR_QUIT           BUTTON_A
#define STAR_UP             BUTTON_UP
#define STAR_DOWN           BUTTON_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_MENU
#define STAR_LEVEL_UP       (BUTTON_POWER | BUTTON_UP)
#define STAR_LEVEL_DOWN     (BUTTON_POWER | BUTTON_DOWN)
#define STAR_LEVEL_REPEAT   (BUTTON_POWER | BUTTON_RIGHT)
#define STAR_MENU_RUN       BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)

#define STAR_QUIT           BUTTON_POWER
#define STAR_UP             BUTTON_SCROLL_UP
#define STAR_DOWN           BUTTON_SCROLL_DOWN
#define STAR_TOGGLE_CONTROL BUTTON_REW
#define STAR_LEVEL_UP       (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define STAR_LEVEL_DOWN     (BUTTON_PLAY | BUTTON_SCROLL_DOWN)
#define STAR_LEVEL_REPEAT   (BUTTON_PLAY | BUTTON_RIGHT)
#define STAR_MENU_RUN       BUTTON_FF

#endif

/* function returns because of USB? */
static bool usb_detected = false;

/* position of the ball */
static int ball_x, ball_y;

/* position of the block */
static int block_x, block_y;

/* number of stars to get to finish the level */
static int star_count;

/* the object we control : ball or block */
static int control;

/* the current board */
static char board[STAR_HEIGHT][STAR_WIDTH];

#include "star_tiles.h"

#define TILE_WIDTH    BMPWIDTH_star_tiles
#define TILE_HEIGHT   (BMPHEIGHT_star_tiles/5)
#define STAR_OFFSET_X ((LCD_WIDTH - STAR_WIDTH * TILE_WIDTH) / 2)
#define STAR_OFFSET_Y ((LCD_HEIGHT - STAR_HEIGHT * TILE_HEIGHT - MAX(TILE_HEIGHT, 8)) / 2)

#define WALL  0
#define SPACE 1
#define BLOCK 2
#define STAR  3
#define BALL  4

/* bitmap of the arrow animation */
static unsigned char arrow_bmp[4][7] =
    {
        {0x7f, 0x7f, 0x3e, 0x3e, 0x1c, 0x1c, 0x08},
        {0x3e, 0x3e, 0x1c, 0x1c, 0x08, 0x08, 0x08},
        {0x1c, 0x1c, 0x1c, 0x1c, 0x08, 0x08, 0x08},
        {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}
    };

/* sequence of the bitmap arrow to follow to do one turn */
static unsigned char anim_arrow[8] = {0, 1, 2, 3, 2, 1, 0};

/* current_level */
static int current_level = 0;

/* char font size */
static int char_width = -1;
static int char_height = -1;

static struct plugin_api* rb;

/* this arrays contains a group of levels loaded into memory */
static unsigned char* levels =
"****************\n"
"*X**........o*x*\n"
"*..........o.***\n"
"*.......**o....*\n"
"*...**.o......**\n"
"**.o..o.....o..*\n"
"*.o......o**.o.*\n"
"*.....**o.....o*\n"
"****************\n"
"\n"
".*..*.*.*...*.**\n"
"*...o.........X*\n"
"...*o..*o...o...\n"
"*.*.o.....o..*.*\n"
"......*...o...*.\n"
"*....*x*..o....*\n"
"...*..*.*...*oo*\n"
"*.............*.\n"
".*..*........*..\n"
"\n"
"****************\n"
"*...........o*x*\n"
"*...**......**X*\n"
"*...*o.........*\n"
"*.o.....o**...o*\n"
"*.*o..o..o*..o**\n"
"*.**o.*o..o.o***\n"
"*o....**o......*\n"
"****************\n"
"\n"
"****************\n"
"*............*x*\n"
"*.....*........*\n"
"**o*o.o*o*o*o*o*\n"
"*.*.*o.o*.*.*.**\n"
"**o*o*o.o*o*o*o*\n"
"*.....*........*\n"
"*...*.......*X.*\n"
"****************\n"
"\n"
".**************.\n"
"*X..*...*..*...*\n"
"*..*o.*.o..o.*.*\n"
"**......*..*...*\n"
"*o.*o*........**\n"
"**.....*.o.*...*\n"
"*o*..*.*.*...*x*\n"
"*...*....o*..*o*\n"
".**************.\n"
"\n"
"....************\n"
"...*...o...*o.o*\n"
"..*....o....*.**\n"
".*.....o.......*\n"
"*X.....o.......*\n"
"**.....o..*...**\n"
"*......o....*..*\n"
"*x.*...o..**o..*\n"
"****************\n"
"\n"
"****************\n"
"*..............*\n"
".**.***..*o.**o*\n"
".*o..*o.*.*.*.*.\n"
"..*..*..***.**..\n"
".**..*..*o*.*o**\n"
"*..............*\n"
"*..X*o....x..*o*\n"
"****************\n"
"\n"
"***************.\n"
"*..o**.........*\n"
"*..*o..**.o....*\n"
"*..o**.*.*o....*\n"
"**.....**..*o*.*\n"
"**.*.......o*o.*\n"
"*oxo*...o..*X*.*\n"
"**.............*\n"
".***************\n"
"\n"
"..*.***********.\n"
".*o*o......*..X*\n"
"*o.o*....o....*.\n"
".*.*..o**..o*..*\n"
"*..*o.*oxo....o*\n"
"*.....o**.....*.\n"
"*o*o.........*..\n"
"*...........*...\n"
".***********....\n"
"\n"
"....***********.\n"
"*****.o........*\n"
"*...x.***o.o*.o*\n"
"*.o...*o.*o...*.\n"
"*.....*..o..*.o*\n"
"*o*o..*.o*..*X*.\n"
".*o...***..***..\n"
"*.........*.*.*.\n"
".*********..*..*\n"
"\n"
"****************\n"
"*......*......X*\n"
"*..*oo.....oo.**\n"
"**...o...**...o*\n"
"*o....*o*oo..***\n"
"**.**....**....*\n"
"*o..o*.o....x.o*\n"
"**o***....*...**\n"
"***************.\n"
"\n"
"**.....**..****.\n"
"*X*****o.***.o**\n"
"*....oo.....o..*\n"
"*.**..**o..*o*.*\n"
"*.*.o.*.*o.**..*\n"
"*.**..**...*x*.*\n"
"*.....o........*\n"
"*........o.....*\n"
"****************\n"
"\n"
".**************.\n"
"*.X*........o.**\n"
"*.*...*o...o**.*\n"
"*.......o....*.*\n"
"*.o..........*o*\n"
"*.*......o.....*\n"
"**......o.o..*o*\n"
"*x..*....o.*.*.*\n"
".**************.\n"
"\n"
"****************\n"
"*o*o........o*o*\n"
"*.o*X......**..*\n"
"*.x........o...*\n"
"*........o*....*\n"
"*......o.......*\n"
"*.o*........*..*\n"
"*o*o........o*o*\n"
"****************\n"
"\n"
".******.********\n"
"*.....o*.....o.*\n"
"*.*.o.*..*...o.*\n"
"*..X*...*oo.*o.*\n"
".*.*...*.o..x*.*\n"
"*o.......*..*o.*\n"
".*......o.....*.\n"
"*o............o*\n"
"****************\n"
"\n"
"****************\n"
"**.x*o.o......o*\n"
"*o.Xo*o.......**\n"
"**.***........**\n"
"**.....o*o*....*\n"
"*oo.......o*o..*\n"
"**.o....****o..*\n"
"**o*..*........*\n"
"****************\n"
"\n"
"****************\n"
"*.o*........*X.*\n"
"*.*..o*oo*o..*.*\n"
"*....*o**o*.o..*\n"
"*.o*.......o*..*\n"
"*..o*o....o*...*\n"
"*.*..*.**o*..*.*\n"
"*....o.*o...x..*\n"
"****************\n"
"\n"
"****************\n"
"*.o....o..x*...*\n"
"*..*o*o...*o...*\n"
"*...*o*....*o..*\n"
"*...o..*...o*o.*\n"
"*.*o*...*.o*...*\n"
"*.o*o.*.o.*....*\n"
"*o*X..*.....*..*\n"
"****************\n"
"\n"
"****************\n"
"*o...**.....**o*\n"
"*.*..*......*o.*\n"
"*.o*...o**..o..*\n"
"*.*....*o......*\n"
"*....*...o*....*\n"
"*.**.o*.**o..*x*\n"
"*.o*.*o.....**X*\n"
"****************\n"
"\n"
"****************\n"
"*...o*o........*\n"
"**o..o*.**o...**\n"
"*.*.*.o...*..*.*\n"
"*.x.*..**..*.Xo*\n"
"*.*..*...o.*.*.*\n"
"**...o**.*o..o**\n"
"*........o*o...*\n"
"****************";

/**
 * Display text.
 */
void star_display_text(char *str, bool waitkey)
{
    int chars_by_line;
    int lines_by_screen;
    int chars_for_line;
    int current_line = 0;
    int first_char_index = 0;
    char *ptr_char;
    char *ptr_line;
    int i;
    char line[255];
    int key;
    bool go_on;

    rb->lcd_clear_display();

    chars_by_line = LCD_WIDTH / char_width;
    lines_by_screen = LCD_HEIGHT / char_height;

    do
    {
        ptr_char = str + first_char_index;
        chars_for_line = 0;
        i = 0;
        ptr_line = line;
        while (i < chars_by_line)
        {
            switch (*ptr_char)
            {
                case '\t':
                case ' ':
                    *(ptr_line++) = ' ';
                case '\n':
                case '\0':
                    chars_for_line = i;
                    break;

                default:
                    *(ptr_line++) = *ptr_char;
            }
            if (*ptr_char == '\n' || *ptr_char == '\0')
                break;
            ptr_char++;
            i++;
        }

        if (chars_for_line == 0)
            chars_for_line = i;

        line[chars_for_line] = '\0';

        /* test if we have cut a word. If it is the case we don't have to */
        /* skip the space */
        if (i == chars_by_line && chars_for_line == chars_by_line)
            first_char_index += chars_for_line;
        else
            first_char_index += chars_for_line + 1;

        /* print the line on the screen */
        rb->lcd_putsxy(0, current_line * char_height, line);

        /* if the number of line showed on the screen is equals to the */
        /* maximum number of line we can show, we wait for a key pressed to */
        /* clear and show the remaining text. */
        current_line++;
        if (current_line == lines_by_screen || *ptr_char == '\0')
        {
            current_line = 0;
            rb->lcd_update();
            go_on = false;
            while (waitkey && !go_on)
            {
                key = rb->button_get(true);
                switch (key)
                {
                    case STAR_QUIT:
                    case BUTTON_LEFT:
                    case STAR_DOWN:
                        go_on = true;
                        break;

                    default:
                        if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                        {
                            usb_detected = true;
                            go_on = true;
                            break;
                        }
                }
            }
            rb->lcd_clear_display();
        }
    } while (*ptr_char != '\0');
}

/**
 * Do a pretty transition from one level to another.
 */
static void star_transition_update(void)
{
    int center_x = LCD_WIDTH / 2;
    int center_y = LCD_HEIGHT / 2;
    int x = 0;
    int y = 0;
#if LCD_WIDTH >= LCD_HEIGHT
    int lcd_demi_width = LCD_WIDTH / 2;
    int var_y = 0;

    for (; x < lcd_demi_width ; x++)
    {
        var_y += LCD_HEIGHT;
        if (var_y > LCD_WIDTH)
        {
            var_y -= LCD_WIDTH;
            y++;
        }
        rb->lcd_update_rect(center_x - x, center_y - y, x * 2, 1);
        rb->lcd_update_rect(center_x - x, center_y - y, 1, y * 2);
        rb->lcd_update_rect(center_x + x - 1, center_y - y, 1, y * 2);
        rb->lcd_update_rect(center_x - x, center_y + y - 1, x * 2, 1);
        STAR_SLEEP
    }
#else
    int lcd_demi_height = LCD_HEIGHT / 2;
    int var_x = 0;

    for (; y < lcd_demi_height ; y++)
    {
        var_x += LCD_WIDTH;
        if (var_x > LCD_HEIGHT)
        {
            var_x -= LCD_HEIGHT;
            x++;
        }
        rb->lcd_update_rect(center_x - x, center_y - y, x * 2, 1);
        rb->lcd_update_rect(center_x - x, center_y - y, 1, y * 2);
        rb->lcd_update_rect(center_x + x - 1, center_y - y, 1, y * 2);
        rb->lcd_update_rect(center_x - x, center_y + y - 1, x * 2, 1);
        STAR_SLEEP
    }
#endif
    rb->lcd_update();
}

/**
 * Display information board of the current level.
 */
static void star_display_board_info(void)
{
    int label_pos_y, tile_pos_y;
    char str_info[32];

    if (TILE_HEIGHT > char_height)
    {
        tile_pos_y = LCD_HEIGHT - TILE_HEIGHT;
        label_pos_y = tile_pos_y + (TILE_HEIGHT - char_height) / 2;
    }
    else
    {
        label_pos_y = LCD_HEIGHT - char_height;
        tile_pos_y = label_pos_y + (char_height - TILE_HEIGHT) / 2;
    }
    
    rb->snprintf(str_info, sizeof(str_info), "L:%02d", current_level + 1);
    rb->lcd_putsxy(STAR_OFFSET_X, label_pos_y, str_info);
    rb->snprintf(str_info, sizeof(str_info), "S:%02d", star_count);
    rb->lcd_putsxy(LCD_WIDTH/2 - 2 * char_width, label_pos_y, str_info);
    rb->lcd_putsxy(STAR_OFFSET_X + (STAR_WIDTH-1) * TILE_WIDTH - 2 * char_width,
                   label_pos_y, "C:");

    rb->lcd_bitmap_part(star_tiles, 0, control == STAR_CONTROL_BALL ?
                        BALL*TILE_HEIGHT : BLOCK*TILE_HEIGHT, TILE_WIDTH,
                        STAR_OFFSET_X + (STAR_WIDTH-1) * TILE_WIDTH,
                        tile_pos_y, TILE_WIDTH, TILE_HEIGHT);

    rb->lcd_update_rect(0, MIN(label_pos_y, tile_pos_y), LCD_WIDTH,
                        MAX(TILE_HEIGHT, char_height));
}


/**
 * Load a level into board array.
 */
static int star_load_level(int current_level)
{
    int x, y;
    char *ptr_tab;

    ptr_tab = levels + current_level * STAR_LEVEL_SIZE;
    control = STAR_CONTROL_BALL;
    star_count = 0;

    rb->lcd_clear_display();

    for (y = 0 ; y < STAR_HEIGHT ; y++)
    {
        for (x = 0 ; x < STAR_WIDTH ; x++)
        {
            board[y][x] = *ptr_tab;
            switch (*ptr_tab)
            {
#   define DRAW_TILE( a )                                                 \
                    rb->lcd_bitmap_part( star_tiles, 0,                   \
                                         a*TILE_HEIGHT, TILE_WIDTH,       \
                                         STAR_OFFSET_X + x * TILE_WIDTH,  \
                                         STAR_OFFSET_Y + y * TILE_HEIGHT, \
                                         TILE_WIDTH, TILE_HEIGHT);
                case STAR_VOID:
                    DRAW_TILE( SPACE );
                    break;

                case STAR_WALL:
                    DRAW_TILE( WALL );
                    break;

                case STAR_STAR:
                    DRAW_TILE( STAR );
                    star_count++;
                    break;

                case STAR_BALL:
                    ball_x = x;
                    ball_y = y;
                    DRAW_TILE( BALL );
                    break;


                case STAR_BLOCK:
                    block_x = x;
                    block_y = y;
                    DRAW_TILE( BLOCK );
                    break;
            }
            ptr_tab++;
        }
        ptr_tab++;
    }
    star_display_board_info();
    star_transition_update();
    return 1;
}

static void star_animate_tile(int tile_no, int start_x, int start_y,
                              int delta_x, int delta_y)
{
    int i;
    
    if (delta_x != 0) /* horizontal */
    {
        for (i = 1 ; i <= TILE_WIDTH ; i++)
        {
            STAR_SLEEP
            rb->lcd_bitmap_part(star_tiles, 0, SPACE * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_part(star_tiles, 0, tile_no * TILE_HEIGHT, TILE_WIDTH,
                        start_x + delta_x * i, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_update_rect(start_x + delta_x * i - (delta_x>0?1:0),
                                start_y, TILE_WIDTH + 1, TILE_HEIGHT);
        }
    }
    else /* vertical */
    {
        for (i = 1 ; i <= TILE_HEIGHT ; i++)
        {
            STAR_SLEEP
            rb->lcd_bitmap_part(star_tiles, 0, SPACE * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_bitmap_part(star_tiles, 0, tile_no * TILE_HEIGHT, TILE_WIDTH,
                        start_x, start_y + delta_y * i, TILE_WIDTH, TILE_HEIGHT);
            rb->lcd_update_rect(start_x, start_y + delta_y * i - (delta_y>0?1:0),
                                TILE_WIDTH, TILE_HEIGHT + 1);
        }
    }
}

/**
 * Run the game.
 */
static int star_run_game(void)
{
    int move_x = 0;
    int move_y = 0;
    int key;
    int lastkey = BUTTON_NONE;

    int label_offset_y;

    label_offset_y = LCD_HEIGHT - char_height;

    if (!star_load_level(current_level))
        return 0;

    while (true)
    {
        move_x = 0;
        move_y = 0;

        while ((move_x == 0) && (move_y == 0))
        {
            key = rb->button_get(true);
            switch (key)
            {
#ifdef STAR_RC_QUIT
                case STAR_RC_QUIT:
#endif
                case STAR_QUIT:
                    return -1;

                case BUTTON_LEFT:
                    move_x = -1;
                    break;

                case BUTTON_RIGHT:
                    move_x = 1;
                    break;

                case STAR_UP:
                    move_y = -1;
                    break;

                case STAR_DOWN:
                    move_y = 1;
                    break;

                case STAR_LEVEL_DOWN:
#ifdef STAR_LEVEL_DOWN_PRE
                    if (lastkey != STAR_LEVEL_DOWN_PRE)
                        break;
#endif
                    if (current_level > 0)
                    {
                        current_level--;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_LEVEL_REPEAT:
                    star_load_level(current_level);
                    break;

                case STAR_LEVEL_UP:
#ifdef STAR_LEVEL_UP_PRE
                    if (lastkey != STAR_LEVEL_UP_PRE)
                        break;
#endif
                    if (current_level < STAR_LEVEL_COUNT - 1)
                    {
                        current_level++;
                        star_load_level(current_level);
                    }
                    break;

                case STAR_TOGGLE_CONTROL:
#ifdef STAR_TOGGLE_CONTROL_PRE
                    if (lastkey != STAR_TOGGLE_CONTROL_PRE)
                        break;
#endif
#ifdef STAR_TOGGLE_CONTROL2
                case STAR_TOGGLE_CONTROL2:
#endif
                    if (control == STAR_CONTROL_BALL)
                        control = STAR_CONTROL_BLOCK;
                    else
                        control = STAR_CONTROL_BALL;
                    star_display_board_info();
                    break;

                default:
                    if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    {
                        usb_detected = true;
                        return 0;
                    }
                    break;
            }
            if (key != BUTTON_NONE)
                lastkey = key;
        }

        if (control == STAR_CONTROL_BALL)
        {
            board[ball_y][ball_x] = STAR_VOID;
            while ((board[ball_y + move_y][ball_x + move_x] == STAR_VOID
                    || board[ball_y + move_y][ball_x + move_x] == STAR_STAR))

            {
                star_animate_tile(BALL, STAR_OFFSET_X + ball_x * TILE_WIDTH,
                                  STAR_OFFSET_Y + ball_y * TILE_HEIGHT,
                                  move_x, move_y);
                ball_x += move_x;
                ball_y += move_y;

                if (board[ball_y][ball_x] == STAR_STAR)
                {
                    board[ball_y][ball_x] = STAR_VOID;
                    star_count--;

                    star_display_board_info();
                }
            }
            board[ball_y][ball_x] = STAR_BALL;
        }
        else
        {
            board[block_y][block_x] = STAR_VOID;
            while (board[block_y + move_y][block_x + move_x] == STAR_VOID)
            {
                star_animate_tile(BLOCK, STAR_OFFSET_X + block_x * TILE_WIDTH,
                                  STAR_OFFSET_Y + block_y * TILE_HEIGHT,
                                  move_x, move_y);
                block_x += move_x;
                block_y += move_y;
            }
            board[block_y][block_x] = STAR_BLOCK;
        }

        if (star_count == 0)
        {
            current_level++;
            if (current_level == STAR_LEVEL_COUNT)
            {
                rb->lcd_clear_display();
                star_display_text("Congratulations!", true);
                rb->lcd_update();
                
                /* There is no such level as STAR_LEVEL_COUNT so it can't be the
                 * current_level */
                current_level--;
                return 1;
            }
            star_load_level(current_level);
        }
    }
}

/**
 * Display the choose level screen.
 */
static int star_choose_level(void)
{
    int level = current_level;
    int key = BUTTON_NONE;
    char str_info[32];
    int lastkey = BUTTON_NONE;

    while (true)
    {
       rb->lcd_clear_display();
       /* levels are numbered 0 to (STAR_LEVEL_COUNT-1) internally, but 
        * displayed as 1 to STAR_LEVEL_COUNT because it looks nicer */
       rb->snprintf(str_info, sizeof(str_info), "Level:%02d / %02d",
                 level+1, STAR_LEVEL_COUNT );
       rb->lcd_putsxy(0, 0, str_info);
       rb->lcd_update();
         key = rb->button_get(true);
         switch (key)
         {
             case STAR_QUIT:
             case BUTTON_LEFT:
                 return -1;
                 break;

            case STAR_MENU_RUN:
#ifdef STAR_MENU_RUN2
            case STAR_MENU_RUN2:
#endif
#ifdef STAR_MENU_RUN3
            case STAR_MENU_RUN3:
#endif
                 current_level=level;
                 return star_run_game();
                 break;

             case STAR_UP:
             case BUTTON_REPEAT | STAR_UP:
                 if(level< STAR_LEVEL_COUNT - 1)
                    level++;
                 break;

             case STAR_DOWN:
             case BUTTON_REPEAT | STAR_DOWN:
                 if(level> 0)
                    level--;
                 break;

             default:
                 if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                 {
                     usb_detected = true;
                     return 0;
                 }
                 break;
         }
         
         if (key != BUTTON_NONE)
             lastkey = key;
     }
}

/**
 * Display the choice menu.
 */
static int star_menu(void)
{
    int move_y;
    int menu_y = 0;
    int i = 0;
    bool refresh = true;
    char anim_state = 0;
    unsigned char *menu[5] = {"Play", "Choose Level", "Information",
                                                      "Keys", "Exit"};
    int menu_count = sizeof(menu) / sizeof(unsigned char *);
    int menu_offset_y;
    int key;

    menu_offset_y = LCD_HEIGHT - char_height * menu_count;

    while (true)
    {
        if (refresh)
        {
            rb->lcd_clear_display();
            rb->lcd_putsxy((LCD_WIDTH - char_width *
                            rb->strlen(STAR_TITLE)) / 2,
                           0, STAR_TITLE);
            for (i = 0 ; i < menu_count ; i++)
            {
                rb->lcd_putsxy(15, menu_offset_y + char_height * i, menu[i]);
            }

            rb->lcd_update();
            refresh = false;
        }

        move_y = 0;
        rb->lcd_mono_bitmap(arrow_bmp[anim_arrow[(anim_state & 0x38) >> 3]],
                            2, menu_offset_y + menu_y * char_height, 7, 8);
        rb->lcd_update_rect (2, menu_offset_y + menu_y * 8, 8, 8);
        STAR_SLEEP
        anim_state++;

        key = rb->button_get(false);
        switch (key)
        {
#ifdef STAR_RC_QUIT
            case STAR_RC_QUIT:
#endif
            case STAR_QUIT:
                return PLUGIN_OK;
            case STAR_UP:
                if (menu_y > 0) {
                    move_y = -1;
#if LCD_DEPTH > 1
                    int oldforeground = rb->lcd_get_foreground();
                    rb->lcd_set_foreground(LCD_BLACK);
#endif
                    rb->lcd_fillrect(0,menu_offset_y + char_height * menu_y - 2,
                                                      15, char_height + 3);
#if LCD_DEPTH > 1
                    rb->lcd_set_foreground(oldforeground);
#endif
                }
                break;
            case STAR_DOWN:
                if (menu_y < menu_count-1) {
                    move_y = 1;
#if LCD_DEPTH > 1
                    int oldforeground = rb->lcd_get_foreground();
                    rb->lcd_set_foreground(LCD_BLACK);
#endif
                    rb->lcd_fillrect(0,menu_offset_y + char_height * menu_y - 1,
                                                       15, char_height + 2);
#if LCD_DEPTH > 1
                    rb->lcd_set_foreground(oldforeground);
#endif
                }
                break;

            case STAR_MENU_RUN:
#ifdef STAR_MENU_RUN2
            case STAR_MENU_RUN2:
#endif
#ifdef STAR_MENU_RUN3
            case STAR_MENU_RUN3:
#endif
                refresh = true;
                switch (menu_y)
                {
                    case 0:
                        star_run_game();
                        break;
                    case 1:
                        star_choose_level();
                        break;
                    case 2:
                        star_display_text(
                            "INFO\n\n"
                            "Take all the stars to go to the next level. "
                            "You can toggle control with the block to "
                            "use it as a mobile wall. The block cannot "
                            "take stars.", true);
                        break;
                    case 3:
#if CONFIG_KEYPAD == RECORDER_PAD
                        star_display_text("KEYS\n\n"
                                          "[ON] Toggle Ctl.\n"
                                          "[OFF] Exit\n"
                                          "[F1] Prev. level\n"
                                          "[F2] Reset level\n"
                                          "[F3] Next level", true);
#elif CONFIG_KEYPAD == ONDIO_PAD
                        star_display_text("KEYS\n\n"
                                          "[MODE] Toggle Ctl\n"
                                          "[OFF] Exit\n"
                                          "[M <] Prev. level\n"
                                          "[M ^] Reset level\n"
                                          "[M >] Next level", true);
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
                        star_display_text("KEYS\n\n"
                                          "[MODE/NAVI] Toggle Ctrl\n"
                                          "[OFF] Exit\n"
                                          "[ON + LEFT] Prev. level\n"
                                          "[ON + NAVI] Reset level\n"
                                          "[ON + RIGHT] Next level", true);
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
                        star_display_text("KEYS\n\n"
                                          "[SELECT] Toggle Ctl\n"
                                          "[S + MENU] Exit\n"
                                          "[S <] Prev. level\n"
                                          "[S + PLAY] Reset level\n"
                                          "[S >] Next level", true);
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
                        star_display_text("KEYS\n\n"
                                          "[SELECT] Toggle Ctl\n"
                                          "[POWER] Exit\n"
                                          "[REC..] Prev. level\n"
                                          "[PLAY] Reset level\n"
                                          "[REC] Next level", true);
#elif CONFIG_KEYPAD == GIGABEAT_PAD
                        star_display_text("KEYS\n\n"
                                          "[MENU] Toggle Ctl\n"
                                          "[A] Exit\n"
                                          "[PWR+DOWN] Prev. level\n"
                                          "[PWR+RIGHT] Reset level\n"
                                          "[PWR+UP] Next level", true);
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
                        star_display_text("KEYS\n\n"
                                          "[REW] Toggle Ctl\n"
                                          "[POWER] Exit\n"
                                          "[PLAY+DOWN] Prev. level\n"
                                          "[PLAY+RIGHT] Reset level\n"
                                          "[PLAY+UP] Next level", true);
#endif
                        break;
                    case 4:
                        return PLUGIN_OK;
                }
                if (usb_detected)
                    return PLUGIN_USB_CONNECTED;
                break;

            default:
                if (rb->default_event_handler(key) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }

        for (i = 0 ; i < char_height ; i++)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect (2, menu_offset_y, 8, menu_count * 8);
            rb->lcd_set_drawmode(DRMODE_FG);
            rb->lcd_mono_bitmap(arrow_bmp[anim_arrow[(anim_state & 0x38) >> 3]],
                                2, menu_offset_y + menu_y * 8 + move_y * i, 7, 8);
            rb->lcd_update_rect(2, menu_offset_y, 8, menu_count * 8);
            anim_state++;
            STAR_SLEEP
        }
            rb->lcd_set_drawmode(DRMODE_SOLID);
        menu_y += move_y;
    }
}

/**
 * Main entry point
 */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    rb = api;

    /* get the size of char */
    rb->lcd_setfont(FONT_SYSFIXED);
    if (char_width == -1)
        rb->lcd_getstringsize("a", &char_width, &char_height);

#if LCD_DEPTH > 1
    rb->lcd_set_background( LCD_BLACK );
    rb->lcd_set_foreground( LCD_WHITE );
#endif

    /* display choice menu */
    return star_menu();
}

#endif
