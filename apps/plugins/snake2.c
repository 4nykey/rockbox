/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Mat Holton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
Snake2!

Board consists of a WIDTHxHEIGHT grid. If board element is 0 then nothing is
there otherwise it is part of the snake or a wall.

Head and Tail are stored

*/

#include "plugin.h"
#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

#define WIDTH  28
#define HEIGHT 16

#if (LCD_WIDTH >= 320) && (LCD_HEIGHT >= 240)
    #define MULTIPLIER 10 /*Modifier for porting on other screens*/
    #define MODIFIER_1 10
    #define MODIFIER_2 8
    #define CENTER_X 20
    #define CENTER_Y 55
    #define TOP_X1 34  /* x-coord of the upperleft item (game type) */
    #define TOP_X2 281 /* x-coord of the upperright item (maze type) */
    #define TOP_X3 42  /* x-coord of the lowerleft item (speed) */
    #define TOP_X4 274 /* x-coord of the lowerright item (hi-score) */
    #define TOP_Y1 4   /* y-coord of the top row of items */
    #define TOP_Y2 25  /* y-coord of the bottom row of items */
    #define BMPHEIGHT_snake2_header 38
    #define BMPWIDTH_snake2_header 320
    #define BMPHEIGHT_snake2_right 192
    #define BMPWIDTH_snake2_right 10
    #define BMPHEIGHT_snake2_left 192
    #define BMPWIDTH_snake2_left 10
    #define BMPHEIGHT_snake2_bottom 10
    #define BMPWIDTH_snake2_bottom 320
#elif (LCD_WIDTH >= 240) && (LCD_HEIGHT >= 168)
    #define MULTIPLIER 8
    #define MODIFIER_1 8
    #define MODIFIER_2 6
    #define CENTER_X 8
    #define CENTER_Y 34
    #define TOP_X1 34
    #define TOP_X2 201
    #define TOP_X3 42
    #define TOP_X4 194
    #define TOP_Y1 4
    #define TOP_Y2 25
    #define BMPHEIGHT_snake2_header 38
    #define BMPWIDTH_snake2_header 240
    #define BMPHEIGHT_snake2_right 120
    #define BMPWIDTH_snake2_right 10
    #define BMPHEIGHT_snake2_left 120
    #define BMPWIDTH_snake2_left 10
    #define BMPHEIGHT_snake2_bottom 10
    #define BMPWIDTH_snake2_bottom 240
#elif (LCD_WIDTH >= 220) && (LCD_HEIGHT >= 176)
    #define MULTIPLIER 7
    #define MODIFIER_1 7
    #define MODIFIER_2 5
    #define CENTER_X 12
    #define CENTER_Y 46
    #define TOP_X1 34
    #define TOP_X2 181
    #define TOP_X3 42
    #define TOP_X4 174
    #define TOP_Y1 4
    #define TOP_Y2 25
    #define BMPHEIGHT_snake2_header 38
    #define BMPWIDTH_snake2_header 220
    #define BMPHEIGHT_snake2_right 128
    #define BMPWIDTH_snake2_right 10
    #define BMPHEIGHT_snake2_left 128
    #define BMPWIDTH_snake2_left 10
    #define BMPHEIGHT_snake2_bottom 10
    #define BMPWIDTH_snake2_bottom 220
#elif (LCD_WIDTH >= 176) && (LCD_HEIGHT >= 132)
    #define MULTIPLIER 5
    #define MODIFIER_1 5
    #define MODIFIER_2 3
    #define CENTER_X 18
    #define CENTER_Y 40
    #define TOP_X1 34
    #define TOP_X2 137
    #define TOP_X3 42
    #define TOP_X4 130
    #define TOP_Y1 4
    #define TOP_Y2 25
    #define BMPHEIGHT_snake2_header 38
    #define BMPWIDTH_snake2_header 176
    #define BMPHEIGHT_snake2_right 84
    #define BMPWIDTH_snake2_right 10
    #define BMPHEIGHT_snake2_left 84
    #define BMPWIDTH_snake2_left 10
    #define BMPHEIGHT_snake2_bottom 10
    #define BMPWIDTH_snake2_bottom 176
#elif (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 128)
    #define MULTIPLIER 5
    #define MODIFIER_1 5
    #define MODIFIER_2 3
    #define CENTER_X 10
    #define CENTER_Y 38
    #define TOP_X1 34
    #define TOP_X2 121
    #define TOP_X3 42
    #define TOP_X4 114
    #define TOP_Y1 4
    #define TOP_Y2 25
    #define BMPHEIGHT_snake2_header 38
    #define BMPWIDTH_snake2_header 160
    #define BMPHEIGHT_snake2_right 80
    #define BMPWIDTH_snake2_right 10
    #define BMPHEIGHT_snake2_left 80
    #define BMPWIDTH_snake2_left 10
    #define BMPHEIGHT_snake2_bottom 10
    #define BMPWIDTH_snake2_bottom 160
#else
    #define MULTIPLIER 4
    #define MODIFIER_1 4
    #define MODIFIER_2 2
    #define CENTER_X 0
    #define CENTER_Y 0

#endif

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SNAKE2_UP   BUTTON_UP
#define SNAKE2_DOWN BUTTON_DOWN
#define SNAKE2_QUIT BUTTON_OFF
#define SNAKE2_LEVEL_UP BUTTON_UP
#define SNAKE2_LEVEL_DOWN BUTTON_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_F3
#define SNAKE2_PLAYPAUSE BUTTON_PLAY
#define SNAKE2_PLAYPAUSE_TEXT "Play"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SNAKE2_UP   BUTTON_UP
#define SNAKE2_DOWN BUTTON_DOWN
#define SNAKE2_QUIT BUTTON_OFF
#define SNAKE2_LEVEL_UP BUTTON_UP
#define SNAKE2_LEVEL_DOWN BUTTON_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_SELECT_TYPE BUTTON_LEFT
#define SNAKE2_PLAYPAUSE BUTTON_MENU
#define SNAKE2_PLAYPAUSE_TEXT "Menu"

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SNAKE2_UP   BUTTON_UP
#define SNAKE2_DOWN BUTTON_DOWN
#define SNAKE2_QUIT BUTTON_OFF
#define SNAKE2_LEVEL_UP BUTTON_UP
#define SNAKE2_LEVEL_DOWN BUTTON_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_MODE
#define SNAKE2_PLAYPAUSE BUTTON_ON
#define SNAKE2_PLAYPAUSE_TEXT "Play"

#define SNAKE2_RC_QUIT  BUTTON_RC_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
#define SNAKE2_UP   BUTTON_MENU
#define SNAKE2_DOWN BUTTON_PLAY
#define SNAKE2_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define SNAKE2_LEVEL_UP BUTTON_SCROLL_FWD
#define SNAKE2_LEVEL_DOWN BUTTON_SCROLL_BACK
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_PLAY
#define SNAKE2_PLAYPAUSE BUTTON_SELECT
#define SNAKE2_PLAYPAUSE_TEXT "Select"

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define SNAKE2_UP   BUTTON_UP
#define SNAKE2_DOWN BUTTON_DOWN
#define SNAKE2_QUIT BUTTON_POWER
#define SNAKE2_LEVEL_UP BUTTON_UP
#define SNAKE2_LEVEL_DOWN BUTTON_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_PLAY
#define SNAKE2_PLAYPAUSE BUTTON_SELECT
#define SNAKE2_PLAYPAUSE_TEXT "Select"

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SNAKE2_UP   BUTTON_UP
#define SNAKE2_DOWN BUTTON_DOWN
#define SNAKE2_QUIT BUTTON_A
#define SNAKE2_LEVEL_UP BUTTON_UP
#define SNAKE2_LEVEL_DOWN BUTTON_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_MENU
#define SNAKE2_PLAYPAUSE BUTTON_SELECT
#define SNAKE2_PLAYPAUSE_TEXT "Select"

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define SNAKE2_UP   BUTTON_SCROLL_UP
#define SNAKE2_DOWN BUTTON_SCROLL_DOWN
#define SNAKE2_QUIT BUTTON_POWER
#define SNAKE2_LEVEL_UP BUTTON_SCROLL_UP
#define SNAKE2_LEVEL_DOWN BUTTON_SCROLL_DOWN
#define SNAKE2_MAZE_NEXT BUTTON_RIGHT
#define SNAKE2_MAZE_LAST BUTTON_LEFT
#define SNAKE2_SELECT_TYPE BUTTON_PLAY
#define SNAKE2_PLAYPAUSE BUTTON_FF
#define SNAKE2_PLAYPAUSE_TEXT "FF"

#else
#error "lacks keymapping"
#endif

static int max_levels = 0;
static char (*level_cache)[HEIGHT][WIDTH];

/*Board itself - 2D int array*/
static int board[WIDTH][HEIGHT];
/*
  Buffer for sorting movement (in case user presses two movements during a
  single frame
*/
static int ardirectionbuffer[2];
static unsigned int score, hiscore = 0;
static int applex;
static int appley;
static int strwdt,strhgt; /*used for string width, height for orientation purposes*/
static int dir;
static int frames;
static int apple;
static int level = 4, speed = 5,dead = 0, quit = 0;
static int sillydir = 0, num_levels = 0;
static int level_from_file = 0;
static struct plugin_api* rb;
static int headx, heady, tailx, taily, applecountdown = 5;
static int game_type = 0;
static int num_apples_to_get=1;
static int num_apples_to_got=0;
static int game_b_level=0;
static int applecount=0;
static char phscore[30];

#if (LCD_WIDTH >= 160) && (LCD_HEIGHT >= 128)
#ifdef HAVE_LCD_COLOR
extern const unsigned short snake2_header1[];
extern const unsigned short snake2_header2[];
extern const unsigned short snake2_left[];
extern const unsigned short snake2_right[];
extern const unsigned short snake2_bottom[];
#else
extern const unsigned char snake2_header1[];
extern const unsigned char snake2_header2[];
extern const unsigned char snake2_left[];
extern const unsigned char snake2_right[];
extern const unsigned char snake2_bottom[];
#endif
#endif

#define NORTH       1
#define EAST        2
#define SOUTH       4
#define WEST        8
#define HEAD        16

#define EAST_NORTH  32
#define EAST_SOUTH  64
#define WEST_NORTH  128
#define WEST_SOUTH  256

#define NORTH_EAST  512
#define NORTH_WEST  1024
#define SOUTH_EAST  2048
#define SOUTH_WEST  4096

#define LEVELS_FILE  PLUGIN_DIR "/snake2.levels"
#define HISCORE_FILE PLUGIN_DIR "/snake2.hs"

int load_all_levels(void)
{
    int linecnt = 0;
    int fd;
    int size;
    char buf[64]; /* Larger than WIDTH, to allow for whitespace after the
                     lines */

    /* Init the level_cache pointer and
       calculate how many levels that will fit */
    level_cache = rb->plugin_get_buffer(&size);
    max_levels = size / (HEIGHT*WIDTH);

    num_levels = 0;

    /* open file */
    if ((fd = rb->open(LEVELS_FILE, O_RDONLY)) < 0)
    {
        return -1;
    }

    while(rb->read_line(fd, buf, 64))
    {
        if(rb->strlen(buf) == 0) /* Separator? */
        {
            num_levels++;
            if(num_levels > max_levels)
            {
                rb->splash(HZ, true, "Too many levels in file");
                break;
            }
            continue;
        }

        rb->memcpy(level_cache[num_levels][linecnt], buf, WIDTH);
        linecnt++;
        if(linecnt == HEIGHT)
        {
            linecnt = 0;
        }
    }

    rb->close(fd);
    return 0;
}

/*Hi-Score reading and writing to file "/.rockbox/snake2.levels" function */
void iohiscore(void)
{
    int fd;
    unsigned int compare;

    /* clear the buffer we're about to load the highscore data into */
    rb->memset(phscore, 0, sizeof(phscore));

    fd = rb->open(HISCORE_FILE,O_RDWR | O_CREAT);

    /* highscore used to %d, is now %d\n
       Deal with no file or bad file */
    rb->read(fd,phscore, sizeof(phscore));

    compare = rb->atoi(phscore);

    if(hiscore > compare){
        rb->lseek(fd,0,SEEK_SET);
        rb->fdprintf(fd, "%d\n", hiscore);
    }
    else
        hiscore = compare;

    rb->close(fd);

}

/*
** Completely clear the board of walls and/or snake     */

void clear_board( void)
{
    int x,y;

    for (x = 0; x < WIDTH; x++)
    {
        for (y = 0; y < HEIGHT; y++)
        {
            board[x][y] = 0;
        }
    }
}

int load_level( int level_number )
{
    int x,y;
    clear_board();
    for(y = 0;y < HEIGHT;y++)
    {
        for(x = 0;x < WIDTH;x++)
        {
            switch(level_cache[level_number][y][x])
            {
                case '|':
                    board[x][y] = NORTH;
                    break;

                case '-':
                    board[x][y] = EAST;
                    break;

                case '+':
                    board[x][y] = HEAD;
                    break;
            }
        }
    }
    return 1;
}

/*
** Gets the currently chosen direction from the first place
** in the direction buffer. If there is something in the
** next part of the buffer then that is moved to the first place
*/
void get_direction( void )
{
    /*if 1st place is empty*/
    if(ardirectionbuffer[0] != -1)
    {
        /*return this direction*/
        dir = ardirectionbuffer[0];
        ardirectionbuffer[0]=-1;
        /*now see if one needs moving:*/
        if(ardirectionbuffer[1] != -1)
        {
            /*there's a move waiting to be done
              so move it into the space:*/
            ardirectionbuffer[0] = ardirectionbuffer[1];
            ardirectionbuffer[1] = -1;
        }
    }
}

/*
** Sets the direction
*/
void set_direction(int newdir)
{
    if(ardirectionbuffer[0] != newdir)
    {
        /*if 1st place is empty*/
        if(ardirectionbuffer[0] == -1)
        {
            /*use 1st space:*/
            ardirectionbuffer[0] = newdir;
        }
        else
        {
            /*use 2nd space:*/
            if(ardirectionbuffer[0] != newdir) ardirectionbuffer[1] = newdir;
        }

        if(frames < 0) ardirectionbuffer[0] = newdir;
    }
}

void new_level(int level)
{
    load_level(level);

    ardirectionbuffer[0] = -1;
    ardirectionbuffer[1] = -1;
    dir   = EAST;
    headx = WIDTH/2;
    heady = HEIGHT/2;
    tailx = headx - 4;
    taily = heady;
    applecountdown = 0;
    /*Create a small snake to start off with*/
    board[headx][heady]   = dir;
    board[headx-1][heady] = dir;
    board[headx-2][heady] = dir;
    board[headx-3][heady] = dir;
    board[headx-4][heady] = dir;
    num_apples_to_got=0;
}

void init_snake(void)
{
    num_apples_to_get=1;
    if(game_type == 1)
       level_from_file = 1;
    game_b_level=1;
    new_level(level_from_file);
}

/*
** Draws the apple. If it doesn't exist then
** a new one get's created.
*/
void draw_apple( void )
{
    int x,y;

#if LCD_WIDTH >= 160 && LCD_HEIGHT >= 128
    char pscore[5], counter[4];

    rb->lcd_bitmap(snake2_header2,0,0,BMPWIDTH_snake2_header, BMPHEIGHT_snake2_header);
    rb->lcd_bitmap(snake2_left,0,BMPHEIGHT_snake2_header,BMPWIDTH_snake2_left, BMPHEIGHT_snake2_left);
    rb->lcd_bitmap(snake2_right,LCD_WIDTH-BMPWIDTH_snake2_right,BMPHEIGHT_snake2_header,BMPWIDTH_snake2_right, BMPHEIGHT_snake2_right);
    rb->lcd_bitmap(snake2_bottom,0,BMPHEIGHT_snake2_header+BMPHEIGHT_snake2_left,BMPWIDTH_snake2_bottom, BMPHEIGHT_snake2_bottom);

    rb->snprintf(counter,sizeof(counter),"%d",applecount);
    rb->lcd_getstringsize(counter,&strwdt,&strhgt);
    rb->lcd_putsxy(TOP_X3-strwdt/2,TOP_Y2,counter);

    rb->snprintf(pscore,sizeof(pscore),"%d",score);
    rb->lcd_getstringsize(pscore,&strwdt,&strhgt);
    rb->lcd_putsxy(TOP_X4-strwdt/2,TOP_Y2,pscore);
#endif

    if (!apple)
    {
          do
          {
            x = (rb->rand() % (WIDTH-1))+1;
            y = (rb->rand() % (HEIGHT-1))+1;
          } while (board[x][y]);
          apple=1;
          board[x][y]=-1;
          applex = x;appley = y;
    }
    rb->lcd_fillrect((CENTER_X+applex*MULTIPLIER)+1,CENTER_Y+appley*MULTIPLIER,MODIFIER_2,MODIFIER_1);
    rb->lcd_fillrect(CENTER_X+applex*MULTIPLIER,(CENTER_Y+appley*MULTIPLIER)+1,MODIFIER_1,MODIFIER_2);
}

/*
    * x x *
    * x x *
    * x x *
    * x x *
*/
void draw_vertical_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+1,CENTER_Y+y*MULTIPLIER,MODIFIER_2,MODIFIER_1);
}

/*
    * * * *
    X X X X
    X X X X
    * * * *
*/
void draw_horizontal_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,CENTER_Y+y*MULTIPLIER+1,MODIFIER_1,MODIFIER_2);
}

/*
    * * * *
    * * X X
    * X X X
    * X X *
*/
void draw_n_to_e_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+1,CENTER_Y+y*MULTIPLIER+2,MODIFIER_2,MODIFIER_2);
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+2,CENTER_Y+y*MULTIPLIER+1,MODIFIER_2,MODIFIER_2);
}

/*
   * * * *
   * * X X
   * X X X
   * X X *
*/
void draw_w_to_s_bit(int x, int y)
{
    draw_n_to_e_bit(x,y);
}

/*
   * * * *
   X X * *
   X X X *
   * X X *
*/
void draw_n_to_w_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,CENTER_Y+y*MULTIPLIER+1,MODIFIER_2,MODIFIER_2);
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+1,CENTER_Y+y*MULTIPLIER+2,MODIFIER_2,MODIFIER_2);
}

/*
   * * * *
   X X * *
   X X X *
   * X X *
*/
void draw_e_to_s_bit(int x, int y)
{
    draw_n_to_w_bit(x, y);
}

/*
   * X X *
   * X X X
   * * X X
   * * * *
*/
void draw_s_to_e_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+1,CENTER_Y+y*MULTIPLIER,MODIFIER_2,MODIFIER_2);
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+2,CENTER_Y+y*MULTIPLIER+1,MODIFIER_2,MODIFIER_2);
}

/*
   * X X *
   * X X X
   * * X X
   * * * *
*/
void draw_w_to_n_bit(int x, int y)
{
    draw_s_to_e_bit(x,y);
}

/*
   * X X *
   X X X *
   X X * *
   * * * *
*/
void draw_e_to_n_bit(int x, int y)
{
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER+1,CENTER_Y+y*MULTIPLIER,MODIFIER_2,MODIFIER_2);
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,CENTER_Y+y*MULTIPLIER+1,MODIFIER_2,MODIFIER_2);
}

/*
   * X X *
   X X X *
   X X * *
   * * * *
*/
void draw_s_to_w_bit(int x, int y)
{
    draw_e_to_n_bit(x, y);
}

/*
** Draws a wall/obsticals
*/
void draw_boundary ( void )
{
    int x, y;

    /*TODO: Load levels from file!*/

    /*top and bottom line*/
    for(x=0; x < WIDTH; x++)
    {
        board[x][0] = EAST;
        board[x][HEIGHT-1] = WEST;
    }

    /*left and right lines*/
    for(y=0; y < HEIGHT; y++)
    {
        board[0][y] = NORTH;
        board[WIDTH-1][y] = SOUTH;
    }

    /*corners:*/
    board[0][0]              = NORTH_EAST;
    board[WIDTH-1][0]        = EAST_SOUTH;
    board[0][HEIGHT-1]       = SOUTH_EAST;
    board[WIDTH-1][HEIGHT-1] = EAST_NORTH;
}

/*
** Redraw the entire board
*/
void redraw (void)
{
    int x,y;

    for (x = 0; x < WIDTH; x++)
    {
        for (y = 0; y < HEIGHT; y++)
        {
            switch (board[x][y])
            {
                case -1:
                    rb->lcd_fillrect((CENTER_X+x*MULTIPLIER)+1,CENTER_Y+y*MULTIPLIER,MODIFIER_2,MODIFIER_1);
                    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,(CENTER_Y+y*MULTIPLIER)+1,MODIFIER_1,MODIFIER_2);
                    break;
                case 0:
                    break;

                case NORTH:
                case SOUTH:
                    draw_vertical_bit(x,y);
                    break;

                case EAST:
                case WEST:
                    draw_horizontal_bit(x,y);
                    break;

                default:
                    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,CENTER_Y+y*MULTIPLIER,MODIFIER_1,MODIFIER_1);
                    break;
            }
        }
    }
}

/*
** Draws the snake bit described by nCurrentBit at position x/y
** deciding whether it's a corner bit by examing the nPrevious bit
*/
void draw_snake_bit(int currentbit, int previousbit, int x, int y)
{
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(CENTER_X+x*MULTIPLIER,CENTER_Y+y*MULTIPLIER,MODIFIER_1,MODIFIER_1);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    switch(currentbit)
    {
        case(NORTH):
            switch(previousbit)
            {
                case(SOUTH):
                case(NORTH):
                    draw_vertical_bit(x,y);
                break;

                case(EAST):
                    draw_e_to_n_bit(x,y);
                break;

                case(WEST):
                    draw_w_to_n_bit(x,y);
                break;
            }
        break;

        case(EAST):
            switch(previousbit)
            {
                case(WEST):
                case(EAST):
                    draw_horizontal_bit(x,y);
                break;

                case(NORTH):
                    draw_n_to_e_bit(x,y);
                break;

                case(SOUTH):
                    draw_s_to_e_bit(x,y);
                break;
            }
        break;

        case(SOUTH):
            switch(previousbit)
            {
                case(SOUTH):
                case(NORTH):
                    draw_vertical_bit(x,y);
                break;

                case(EAST):
                    draw_e_to_s_bit(x,y);
                break;

                case(WEST):
                    draw_w_to_s_bit(x,y);
                break;
            }
        break;

        case(WEST):
            switch(previousbit)
            {
                case(EAST):
                case(WEST):
                    draw_horizontal_bit(x,y);
                break;

                case(SOUTH):
                    draw_s_to_w_bit(x,y);
                break;

                case(NORTH):
                    draw_n_to_w_bit(x,y);
                break;
            }
        break;
    }
}

/*
** Death 'sequence' and end game stuff.
*/
void die (void)
{
    int button;
    bool done=false;
    char pscore[20];

    rb->splash(HZ*2, true, "Oops!");

    rb->lcd_clear_display();

    applecount=0;

    rb->lcd_getstringsize("You died!",&strwdt,&strhgt);
    rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt,"You died!");

    rb->snprintf(pscore,sizeof(pscore),"Your score: %d",score);
    rb->lcd_getstringsize(pscore,&strwdt,&strhgt);
    rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt * 2 + 2,pscore);

    if (score>hiscore)
    {
        hiscore=score;
        rb->lcd_getstringsize("New high score!",&strwdt,&strhgt);
        rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt * 4 + 2,"New high score!");
    }
    else
    {
        rb->snprintf(phscore,sizeof(phscore),"High score: %d",hiscore);
        rb->lcd_getstringsize(phscore,&strwdt,&strhgt);
        rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt * 5,phscore);
    }

    rb->snprintf(phscore,sizeof(phscore),"Press %s...",SNAKE2_PLAYPAUSE_TEXT);
    rb->lcd_getstringsize(phscore,&strwdt,&strhgt);
    rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt * 7,phscore);

    rb->lcd_update();

    while(!done)
    {
        button=rb->button_get(true);
        switch(button)
        {
            case SNAKE2_PLAYPAUSE:
                done = true;
                break;
        }
    }

    dead=1;
}

/*
** Check for collision. TODO: Currently this
** sets of the death sequence. What we want is it to only return a true/false
** depending on whether a collision occured.
*/
void collision ( int x, int y )
{
    int bdeath=0;


    switch (board[x][y])
    {
        case 0:

        break;
        case -1:
            score = score + (1 * level);
            apple=0;
            applecountdown=2;
            applecount++;

            if(game_type==1)
            {
                if(num_apples_to_get == num_apples_to_got)
                {
                    level_from_file++;
                    if(level_from_file >= num_levels)
                    {
                        level_from_file = 1;
                        /*and increase the number of apples to pick up
                        before level changes*/
                        num_apples_to_get+=2;
                        game_b_level++;
                    }
                    rb->splash(HZ, true, "Level Completed!");
                    rb->lcd_clear_display();
                    new_level(level_from_file);
                    rb->lcd_clear_display();
                    redraw();
                    rb->lcd_update();
                }
                else
                    num_apples_to_got++;
            }
        break;
        default:
            bdeath=1;
            break;
    }

    if(bdeath==1)
    {
        die();
        sillydir = dir;
        frames = -110;
    }
}

void move( void )
{
    int taildir;
    /*this actually sets the dir variable.*/
    get_direction();
    /*draw head*/
    switch (dir)
    {
        case (NORTH):
            board[headx][heady]=NORTH;
            heady--;
            break;
        case (EAST):
            board[headx][heady]=EAST;
            headx++;
            break;
        case (SOUTH):
            board[headx][heady]=SOUTH;
            heady++;
            break;
        case (WEST):
            board[headx][heady]=WEST;
            headx--;
            break;
    }

    if(headx == WIDTH)
        headx = 0;
    else if(headx < 0)
        headx = WIDTH-1;

    if(heady == HEIGHT)
        heady = 0;
    else if(heady < 0)
        heady = HEIGHT-1;

    rb->lcd_fillrect(CENTER_X+headx*MULTIPLIER,CENTER_Y+heady*MULTIPLIER,MODIFIER_1,MODIFIER_1);

    /*clear tail*/
    if(applecountdown <= 0)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(CENTER_X+tailx*MULTIPLIER,CENTER_Y+taily*MULTIPLIER,MODIFIER_1,MODIFIER_1);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        taildir = board[tailx][taily];
        board[tailx][taily] = 0;

        switch (taildir)
        {
            case(NORTH):
                taily--;
            break;

            case(EAST):
                tailx++;
            break;

            case(SOUTH):
                taily++;
            break;

            case(WEST):
                tailx--;
            break;
        }

        if(tailx == WIDTH)
            tailx = 0;
        else if(tailx < 0)
            tailx = WIDTH-1;

        if(taily == HEIGHT)
            taily = 0;
        else if(taily < 0)
            taily = HEIGHT-1;
    }
    else
        applecountdown--;
}

void frame (void)
{
    int olddir, noldx, noldy, temp;
    noldx  = headx;
    noldy  = heady;
    olddir = 0;
    switch(dir)
    {
        case(NORTH):
            if(heady == HEIGHT-1)
                temp = 0;
            else
                temp = heady + 1;

            olddir = board[headx][temp];
        break;

        case(EAST):
            if(headx == 0)
                temp = WIDTH-1;
            else
                temp = headx - 1;

            olddir = board[temp][heady];
        break;

        case(SOUTH):
            if(heady == 0)
                temp = HEIGHT-1;
            else
                temp = heady - 1;

            olddir = board[headx][temp];
        break;

        case(WEST):
            if(headx == WIDTH-1)
                temp = 0;
            else
                temp = headx + 1;

            olddir = board[temp][heady];
        break;
    }

    move();

    /*
      now redraw the bit that was
      the tail, to something snake-like:
     */
    draw_snake_bit(dir, olddir, noldx, noldy);

    collision(headx, heady);

    rb->lcd_update();
}

void game_pause (void)
{
    int button;

    rb->lcd_clear_display();
    rb->lcd_getstringsize("Paused",&strwdt,&strhgt);
    rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,LCD_HEIGHT/2,"Paused");

    rb->lcd_update();
    while (1)
    {
        button = rb->button_get(true);
        switch (button)
        {
            case SNAKE2_PLAYPAUSE:
                rb->lcd_clear_display();
                redraw();
                rb->lcd_update();
                rb->sleep(HZ/2);
                return;

            default:
                if (rb->default_event_handler(button)==SYS_USB_CONNECTED) {
                    dead = 1;
                    quit = 2;
                    return;
                }
                break;
        }
    }
}

void game (void)
{
    int button;

    rb->lcd_clear_display();
    redraw();
    rb->lcd_update();
    /*main loop:*/
    while (1)
    {
        if(frames==5)
        {
           frame();
           if(frames>0) frames=0;
        }
        frames++;

        if(frames == 0)
        {
            die();
        }
        else
        {
            if(frames < 0)
            {
                if(sillydir != dir)
                {
                    /*it has, great set frames to a positive value again:*/
                    frames = 1;
                }
            }
        }

        if (dead) return;

        draw_apple();

        rb->sleep(HZ/speed);

        button = rb->button_get(false);

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold())
        button = SNAKE2_PLAYPAUSE;
#endif

        switch (button)
        {
             case SNAKE2_UP:
             case SNAKE2_UP | BUTTON_REPEAT:
                 if (dir != SOUTH) set_direction(NORTH);
                 break;

             case BUTTON_RIGHT:
             case BUTTON_RIGHT | BUTTON_REPEAT:
                 if (dir != WEST) set_direction(EAST);
                 break;

             case SNAKE2_DOWN:
             case SNAKE2_DOWN | BUTTON_REPEAT:
                 if (dir != NORTH) set_direction(SOUTH);
                 break;

             case BUTTON_LEFT:
             case BUTTON_LEFT | BUTTON_REPEAT:
                 if (dir != EAST) set_direction(WEST);
                 break;

#ifdef SNAKE2_RC_QUIT
             case SNAKE2_RC_QUIT:
#endif
             case SNAKE2_QUIT:
                 dead=1;
                 return;

             case SNAKE2_PLAYPAUSE:
                 game_pause();
                 break;

             default:
                 if (rb->default_event_handler(button)==SYS_USB_CONNECTED) {
                     quit = 2;
                     return;
                 }
                 break;
        }
    }

}

void game_init(void)
{
    int button;
    char plevel[30];

    dead=0;
    apple=0;
    score=0;
    applecount=0;

    clear_board();
    load_level( level_from_file );
    rb->lcd_clear_display();
    redraw();
    rb->lcd_update();

    while (1)
    {
#if LCD_WIDTH >= 160 && LCD_HEIGHT >= 128

        rb->lcd_bitmap(snake2_header1,0,0,BMPWIDTH_snake2_header, BMPHEIGHT_snake2_header);
        rb->lcd_bitmap(snake2_left,0,BMPHEIGHT_snake2_header,BMPWIDTH_snake2_left, BMPHEIGHT_snake2_left);
        rb->lcd_bitmap(snake2_right,LCD_WIDTH-BMPWIDTH_snake2_right,BMPHEIGHT_snake2_header,BMPWIDTH_snake2_right, BMPHEIGHT_snake2_right);
        rb->lcd_bitmap(snake2_bottom,0,BMPHEIGHT_snake2_header+BMPHEIGHT_snake2_left,BMPWIDTH_snake2_bottom, BMPHEIGHT_snake2_bottom);

        rb->snprintf(plevel,sizeof(plevel),"%d",level);
        rb->lcd_getstringsize(plevel,&strwdt,&strhgt);
        rb->lcd_putsxy(TOP_X3-strwdt/2,TOP_Y2, plevel);

        rb->snprintf(plevel,sizeof(plevel),"%d",level_from_file);
        rb->lcd_getstringsize(plevel,&strwdt,&strhgt);
        rb->lcd_putsxy(TOP_X2-strwdt/2,TOP_Y1, plevel);

         if(game_type==0){
             rb->lcd_getstringsize("A",&strwdt,&strhgt);
             rb->lcd_putsxy(TOP_X1,TOP_Y1,"A");
         }
         else{
             rb->lcd_getstringsize("B",&strwdt,&strhgt);
             rb->lcd_putsxy(TOP_X1,TOP_Y1,"B");
        }

        rb->snprintf(phscore,sizeof(phscore),"%d",hiscore);
        rb->lcd_getstringsize(phscore,&strwdt,&strhgt);
        rb->lcd_putsxy(TOP_X4-strwdt/2,TOP_Y2, phscore);

#else
        rb->snprintf(plevel,sizeof(plevel),"Speed: %02d",level);
        rb->lcd_getstringsize("Speed: 00",&strwdt,&strhgt);
        rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt+4, plevel);

        rb->snprintf(plevel,sizeof(plevel),"Maze:  %d",level_from_file);
        rb->lcd_getstringsize(plevel,&strwdt,&strhgt);
        rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt*2+4, plevel);

        if(game_type==0){
            rb->lcd_getstringsize("Game Type: A ",&strwdt,&strhgt);
            rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt*3+4,"Game Type: A");
        }
        else{
            rb->lcd_getstringsize("Game Type: B ",&strwdt,&strhgt);
            rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt*3+4,"Game Type: B");
        }

        rb->snprintf(phscore,sizeof(phscore),"Hi Score: %d",hiscore);
        rb->lcd_getstringsize(phscore,&strwdt,&strhgt);
        rb->lcd_putsxy((LCD_WIDTH - strwdt)/2,strhgt*4+4, phscore);
#endif

        rb->lcd_update();

        button=rb->button_get(true);
        switch (button)
        {
            case SNAKE2_LEVEL_UP:
            case SNAKE2_LEVEL_UP|BUTTON_REPEAT:
                if (level<10)
                    level+=1;
                break;
            case SNAKE2_LEVEL_DOWN:
            case SNAKE2_LEVEL_DOWN|BUTTON_REPEAT:
                if (level>1)
                    level-=1;
                break;
            case SNAKE2_QUIT:
                quit=1;
                return;
                break;
            case SNAKE2_PLAYPAUSE:
                speed = level*20;
                return;
                break;
            case SNAKE2_SELECT_TYPE:
                if(game_type==0)game_type=1; else game_type=0;
                break;
            case SNAKE2_MAZE_NEXT:
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
                rb->lcd_fillrect(CENTER_X, CENTER_Y, CENTER_X+WIDTH*MULTIPLIER,
                                 CENTER_Y+HEIGHT*MULTIPLIER);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                if(level_from_file < num_levels)
                    level_from_file++;
                else
                    level_from_file = 0;
                load_level( level_from_file );
                redraw();
                break;
#ifdef SNAKE2_MAZE_LAST
            case SNAKE2_MAZE_LAST:
                rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
                rb->lcd_fillrect(CENTER_X, CENTER_Y, CENTER_X+WIDTH*MULTIPLIER,
                                 CENTER_Y+HEIGHT*MULTIPLIER);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                if(level_from_file > 0)
                    level_from_file--;
                else
                    level_from_file = num_levels;
                load_level( level_from_file );
                redraw();
                break;
#endif
            default:
                if (rb->default_event_handler(button)==SYS_USB_CONNECTED) {
                    quit = 2;
                    return;
                }
                break;
        }
    }

}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)(parameter);
    rb = api;

    /* Lets use the default font */
    rb->lcd_setfont(FONT_SYSFIXED);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif

    load_all_levels();

    if (num_levels == 0) {
        rb->splash(HZ*2, true, "Failed loading levels!");
        return PLUGIN_OK;
    }

    iohiscore();

    while(quit==0)
    {
      game_init();
      rb->lcd_clear_display();
      frames=1;

      if(quit==0)
      {
          init_snake();

          /*Start Game:*/
          game();
      }
    }

    iohiscore();
    return (quit==1) ? PLUGIN_OK : PLUGIN_USB_CONNECTED;
}

#endif
