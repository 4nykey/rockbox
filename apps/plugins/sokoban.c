/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Eric Linenberg
 * February 2003: Robert Hak performs a cleanup/rewrite/feature addition.
 *                Eric smiles.  Bjorn cries.  Linus say 'huh?'.
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

#define SOKOBAN_TITLE       "Sokoban"
#define SOKOBAN_TITLE_FONT  2

#define LEVELS_FILE         "/.rockbox/sokoban/levels.txt"

#define ROWS                16
#define COLS                20
#define MAX_UNDOS           5

#define SOKOBAN_LEVEL_SIZE (ROWS*COLS)

static void init_undo(void);
static void undo(void);
static void add_undo(int button);

static int get_level(char *level, int level_size);
static int get_level_count(void);
static int load_level(void);
static void draw_level(void);

static void init_boards(void);
static void update_screen(void);
static bool sokoban_loop(void);

/* The Location, Undo and LevelInfo structs are OO-flavored.  
 * (oooh!-flavored as Schnueff puts it.)  It makes more you have to know, 
 * but the overall data layout becomes more manageable. */

/* We use the same three values in 2 structs.  Makeing them a struct
 * hopefully ensures that if you change things in one, the other changes
 * as well. */
struct LevelInfo {
    short level;
    short moves;
    short boxes_to_go;
};

/* What a given location on the board looks like at a given time */
struct Location {
    char spot;
    short row;
    short col;
};

/* A single level of undo.  Each undo move can affect upto,
 * but not more then, 3 spots on the board */
struct Undo {
    struct LevelInfo level;
    struct Location location[3];
};

/* Our full undo history */
static struct UndoInfo {
    short count;    /* How many undos are there in history */
    short current;  /* Which history is the current undo */
    struct Undo history[MAX_UNDOS];
} undo_info;

/* Our playing board */
static struct BoardInfo {
    char board[ROWS][COLS];
    struct LevelInfo level;
    struct Location player;
    int max_level;               /* How many levels do we have? */
    int level_offset;            /* Where in the level file is this level */
    int loaded_level;            /* Which level is in memory */
} current_info;

static struct plugin_api* rb;

static void init_undo(void)
{
    undo_info.count = 0;
    undo_info.current = 0;
}

static void undo(void)
{
    struct Undo *undo;
    int i = 0;
    short row, col;

    if (undo_info.count == 0)
        return;

    /* Update board info */
    undo = &undo_info.history[undo_info.current];
    
    rb->memcpy(&current_info.level, &undo->level, sizeof(undo->level));
    rb->memcpy(&current_info.player, &undo->location[0], sizeof(undo->location[0]));

    row = undo->location[0].row;
    col = undo->location[0].col;
    current_info.board[row][col] = '@';

    /* Update the two other possible spots */
    for (i = 1; i < 3; i++) {
        if (undo->location[i].spot != '\0') {
            row = undo->location[i].row;
            col = undo->location[i].col;
            current_info.board[row][col] = undo->location[i].spot;
            undo->location[i].spot = '\0';
        }
    }

    /* Remove this undo from the list */
    if (undo_info.current == 0) {
        if (undo_info.count > 1)
            undo_info.current = MAX_UNDOS - 1;
    } else {
        undo_info.current--;
    }
    
    undo_info.count--;

    return;
}

static void add_undo(int button)
{
    struct Undo *undo;
    int row, col, i;
    bool storable;

    if ((button != BUTTON_LEFT) && (button != BUTTON_RIGHT) &&
        (button != BUTTON_UP) && (button != BUTTON_DOWN))
        return;

    if (undo_info.count != 0) {
        if (undo_info.current < (MAX_UNDOS - 1)) 
            undo_info.current++;
        else
            undo_info.current = 0;
    }

    /* Make what follows more readable */
    undo = &undo_info.history[undo_info.current];

    /* Store our level info */
    rb->memcpy(&undo->level, &current_info.level, sizeof(undo->level));

    /* Store our player info */
    rb->memcpy(&undo->location[0], &current_info.player, sizeof(undo->location[0]));

    /* Now we need to store upto 2 blocks that may be affected.  
     * If player.spot is NULL, then there is no info stored 
     * for that block */

    row = current_info.player.row;
    col = current_info.player.col;

    /* This must stay as _1_ because the first block (0) is the player */
    for (i = 1; i < 3; i++) {
        storable = true;

        switch (button) {
        case BUTTON_LEFT:
            col--;
            if (col < 0) 
                storable = false;
            break;

        case BUTTON_RIGHT:
            col++;
            if (col >= COLS) 
                storable = false;
            break;

        case BUTTON_UP:
            row--;
            if (row < 0)
                storable = false;
            break;
            
        case BUTTON_DOWN:
            row++;
            if (row >= ROWS)
                storable = false;
            break;

        default:
            return;
        }

        if (storable) {
            undo->location[i].col = col;
            undo->location[i].row = row;
            undo->location[i].spot = current_info.board[row][col];
        } else {
            undo->location[i].spot = '\0';
        }
    }

    if (undo_info.count < MAX_UNDOS) 
        undo_info.count++;
}

static void init_boards(void)
{
    current_info.level.level = 0;
    current_info.level.moves = 0;
    current_info.level.boxes_to_go = 0;
    current_info.player.row = 0;
    current_info.player.col = 0;
    current_info.player.spot = ' ';
    current_info.max_level = 0;
    current_info.level_offset = 0;
    current_info.loaded_level = 0;

    init_undo();
}

static int get_level_count(void) 
{
    int fd = 0;
    int lastlen = 0;
    char buffer[COLS + 3]; /* COLS plus CR/LF and \0 */

    if ((fd = rb->open(LEVELS_FILE, O_RDONLY)) < 0) {
        rb->splash(0, true, "Unable to open %s", LEVELS_FILE);
        return -1;
    }

    while(1) {
        int len = rb->read_line(fd, buffer, sizeof(buffer));
        if(len <= 0)
            break;

        /* Two short lines in a row means new level */
        if(len < 3 && lastlen < 3)
            current_info.max_level++;

        lastlen = len;
    }

    rb->close(fd);
    return 0;
}

static int get_level(char *level, int level_size) 
{
    int fd = 0, i = 0;
    int nread = 0;
    int count = 0;
    int lastlen = 0;
    int level_ct = 1;
    unsigned char buffer[SOKOBAN_LEVEL_SIZE * 2];
    bool level_found = false;

    /* open file */
    if ((fd = rb->open(LEVELS_FILE, O_RDONLY)) < 0)
        return -1;

    /* Lets not reparse the full file if we can avoid it */
    if (current_info.loaded_level < current_info.level.level) {
        rb->lseek(fd, current_info.level_offset, SEEK_SET);
        level_ct = current_info.loaded_level;
    }

    if(current_info.level.level > 1) {
        while(!level_found) {
            int len = rb->read_line(fd, buffer, SOKOBAN_LEVEL_SIZE);
            if(len <= 0) {
                rb->close(fd);
                return -1;
            }
            
            /* Two short lines in a row means new level */
            if(len < 3 && lastlen < 3) {
                level_ct++;
                if(level_ct == current_info.level.level)
                    level_found = true;
            }
            lastlen = len;
        }
    }

    /* Remember the current offset */
    current_info.level_offset = rb->lseek(fd, 0, SEEK_CUR);
    
    /* read a full buffer chunk from here */
    nread = rb->read(fd, buffer, sizeof(buffer)-1);
    if (nread < 0)
        return -1;
    buffer[nread] = 0;
    
    rb->close(fd);
    
    /* If we read less then a level, error */
    if (nread < level_size)
        return -1;
    
    /* Load our new level */
    for(i=0, count=0; (count < nread) && (i<level_size);) {
        if (buffer[count] != '\n' && buffer[count] != '\r')
            level[i++] = buffer[count];
        count++;
    }
    level[i] = 0;

    current_info.loaded_level = current_info.level.level;
    return 0;
}

/* return non-zero on error */
static int load_level(void)
{
    short c = 0;
    short r = 0;
    short i = 0;
    char level[ROWS*COLS+1];
    int x = 0;

    current_info.player.spot=' ';
    current_info.level.boxes_to_go = 0;
    current_info.level.moves = 0;

    if (get_level(level, sizeof(level)) != 0)
        return -1;

    i = 0;
    for (r = 0; r < ROWS; r++) {
        x++;
        for (c = 0; c < COLS; c++, i++) {
            current_info.board[r][c] = level[i];
            
            if (current_info.board[r][c] == '.')
                current_info.level.boxes_to_go++;

            else if (current_info.board[r][c] == '@') {
                current_info.player.row = r;
                current_info.player.col = c;
            }
        }
    }

    return 0;
}

static void update_screen(void) 
{
    short b = 0, c = 0;
    short rows = 0, cols = 0;
    char s[25];
    
    short magnify = 4;

    /* load the board to the screen */
    for (rows=0 ; rows < ROWS ; rows++) {
        for (cols = 0 ; cols < COLS ; cols++) {
            c = cols * magnify;
            b = rows * magnify;

            switch(current_info.board[rows][cols]) {
            case 'X': /* black space */
                rb->lcd_drawrect(c, b, magnify, magnify);
                rb->lcd_drawrect(c+1, b+1, 2, 2);
                break;
                
            case '#': /* this is a wall */
                rb->lcd_drawpixel(c, b);
                rb->lcd_drawpixel(c+2, b);
                rb->lcd_drawpixel(c+1, b+1);
                rb->lcd_drawpixel(c+3, b+1);
                rb->lcd_drawpixel(c, b+2);
                rb->lcd_drawpixel(c+2, b+2);
                rb->lcd_drawpixel(c+1, b+3);
                rb->lcd_drawpixel(c+3, b+3);
                break;

            case '.': /* this is a home location */
                rb->lcd_drawrect(c+1, b+1, 2, 2);
                break;

            case '$': /* this is a box */
                rb->lcd_drawrect(c, b, magnify, magnify);
                break;

            case '@': /* this is you */
                rb->lcd_drawline(c+1, b, c+2, b);
                rb->lcd_drawline(c, b+1, c+3, b+1);
                rb->lcd_drawline(c+1, b+2, c+2, b+2);

                rb->lcd_drawpixel(c, b+3);
                rb->lcd_drawpixel(c+3, b+3);
                break;

            case '%': /* this is a box on a home spot */ 
                rb->lcd_drawrect(c, b, magnify, magnify);
                rb->lcd_drawrect(c+1, b+1, 2, 2);
                break;
            }
        }
    }
    

    rb->snprintf(s, sizeof(s), "%d", current_info.level.level);
    rb->lcd_putsxy(86, 22, s);
    rb->snprintf(s, sizeof(s), "%d", current_info.level.moves);
    rb->lcd_putsxy(86, 54, s);

    rb->lcd_drawrect(80,0,32,32);
    rb->lcd_drawrect(80,32,32,64);
    rb->lcd_putsxy(81, 10, "Level");
    rb->lcd_putsxy(81, 42, "Moves");

    /* print out the screen */
    rb->lcd_update();
}

static void draw_level(void)
{
    load_level();
    rb->lcd_clear_display();
    update_screen();
}

static bool sokoban_loop(void)
{
    char new_spot;
    bool moved = true;
    int i = 0, button = 0;
    short r = 0, c = 0;

    current_info.level.level = 1;

    load_level();
    update_screen(); 

    while (1) {
        moved = true;

        r = current_info.player.row;
        c = current_info.player.col;

        button = rb->button_get(true);

        add_undo(button);

        switch(button) 
        {
            case BUTTON_OFF:
                /* get out of here */
                return PLUGIN_OK; 

            case BUTTON_ON:
            case BUTTON_ON | BUTTON_REPEAT:
                /* this is UNDO */
                undo();
                rb->lcd_clear_display();
                update_screen(); 
                moved = false;
                break;

            case BUTTON_F3:
            case BUTTON_F3 | BUTTON_REPEAT:
                /* increase level */
                init_undo();
                current_info.level.boxes_to_go=0;
                moved = true;
                break;

            case BUTTON_F1:
            case BUTTON_F1 | BUTTON_REPEAT:
                /* previous level */
                init_undo();
                if (current_info.level.level > 1)
                    current_info.level.level--;

                draw_level();
                moved = false;
                break;

            case BUTTON_F2:
            case BUTTON_F2 | BUTTON_REPEAT:
                /* same level */
                init_undo();
                draw_level();
                moved = false;
                break;

            case BUTTON_LEFT:
                switch(current_info.board[r][c-1]) 
                {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r][c-1];
                        current_info.board[r][c-1] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r][c-2])
                        {
                            case ' ': /* going from blank to blank */
                                current_info.board[r][c-2] = current_info.board[r][c-1];
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r][c-2] = '%';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r][c-2]) {
                            case ' ': /* we are going from a home to a blank */
                                current_info.board[r][c-2] = '$';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                current_info.board[r][c-2] = '%';
                                current_info.board[r][c-1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.col--;
                break;

            case BUTTON_RIGHT: /* if it is a blank spot */
                switch(current_info.board[r][c+1]) {
                    case ' ':
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r][c+1];
                        current_info.board[r][c+1] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$': 
                        switch(current_info.board[r][c+2]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r][c+2] = current_info.board[r][c+1];
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r][c+2] = '%';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r][c+2]) {
                            case ' ': /* going from a home to a blank */
                                current_info.board[r][c+2] = '$';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.':
                                current_info.board[r][c+2] = '%';
                                current_info.board[r][c+1] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.col++;
                break;

            case BUTTON_UP:
                switch(current_info.board[r-1][c]) {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r-1][c];
                        current_info.board[r-1][c] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r-2][c]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r-2][c] = current_info.board[r-1][c];
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r-2][c] = '%';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r-2][c]) {
                            case ' ': /* we are going from a home to a blank */
                                current_info.board[r-2][c] = '$';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* if we are going from a home to home */
                                current_info.board[r-2][c] = '%';
                                current_info.board[r-1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.row--;
                break;

            case BUTTON_DOWN:
                switch(current_info.board[r+1][c]) {
                    case ' ': /* if it is a blank spot */
                    case '.': /* if it is a home spot */
                        new_spot = current_info.board[r+1][c];
                        current_info.board[r+1][c] = '@';
                        current_info.board[r][c] = current_info.player.spot;
                        current_info.player.spot = new_spot;
                        break;

                    case '$':
                        switch(current_info.board[r+2][c]) {
                            case ' ': /* going from blank to blank */
                                current_info.board[r+2][c] = current_info.board[r+1][c];
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = ' ';
                                break;

                            case '.': /* going from a blank to home */
                                current_info.board[r+2][c] = '%';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = ' ';
                                current_info.level.boxes_to_go--;
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    case '%':
                        switch(current_info.board[r+2][c]) {
                            case ' ': /* going from a home to a blank */
                                current_info.board[r+2][c] = '$';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot;
                                current_info.player.spot = '.';
                                current_info.level.boxes_to_go++;
                                break;

                            case '.': /* going from a home to home */
                                current_info.board[r+2][c] = '%';
                                current_info.board[r+1][c] = current_info.board[r][c];
                                current_info.board[r][c] = current_info.player.spot; 
                                current_info.player.spot = '.';
                                break;

                            default:
                                moved = false;
                                break;
                        }
                        break;

                    default:
                        moved = false;
                        break;
                }

                if (moved)
                    current_info.player.row++;
                break;

            case SYS_USB_CONNECTED:
                rb->usb_screen();
                return PLUGIN_USB_CONNECTED;

            default:
                moved = false;
                break;
        }
        
        if (moved) {
            current_info.level.moves++;
            rb->lcd_clear_display();
            update_screen(); 
        }

        /* We have completed this level */
        if (current_info.level.boxes_to_go == 0) {
            current_info.level.level++;

            /* clear undo stats */
            init_undo();

            rb->lcd_clear_display();
			
            if (current_info.level.level > current_info.max_level) {
                rb->lcd_putsxy(10, 20, "You WIN!!");

                for (i = 0; i < 30000 ; i++) {
                    rb->lcd_invertrect(0, 0, 111, 63);
                    rb->lcd_update();

                    button = rb->button_get(false);
                    if (button && ((button & BUTTON_REL) != BUTTON_REL))
                        break;
                }

                return PLUGIN_OK;
            }

            load_level();
            update_screen();
        }

    } /* end while */

    return PLUGIN_OK;
}


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int w, h;
    int len;

    TEST_PLUGIN_API(api);
    (void)(parameter);
    rb = api;
    
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_getstringsize(SOKOBAN_TITLE, &w, &h);

    /* Get horizontel centering for text */
    len = w;
    if (len%2 != 0)
        len =((len+1)/2)+(w/2);
    else
        len /= 2;

    if (h%2 != 0)
        h = (h/2)+1;
    else
        h /= 2;

    rb->lcd_clear_display();
    rb->lcd_putsxy(LCD_WIDTH/2-len,(LCD_HEIGHT/2)-h, SOKOBAN_TITLE);
    rb->lcd_update();
    rb->sleep(HZ*2);

    rb->lcd_clear_display();

    rb->lcd_putsxy(3,  6, "[OFF] To Stop");
    rb->lcd_putsxy(3, 16, "[ON] To Undo");
    rb->lcd_putsxy(3, 26, "[F1] - Level");
    rb->lcd_putsxy(3, 36, "[F2] Same Level");
    rb->lcd_putsxy(3, 46, "[F3] + Level");

    rb->lcd_update();
    rb->sleep(HZ*2);
    rb->lcd_clear_display();

    init_boards();

    if (get_level_count() != 0) {
        rb->splash(HZ*2, true, "Failed loading levels!");
        return PLUGIN_OK;
    }

    return sokoban_loop();
}

#endif
