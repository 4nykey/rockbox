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
#include "hwcompat.h"

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"

#include "font-player.h"
#include "lcd-playersim.h"

/*** definitions ***/

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define ICON_HEIGHT 24
#define CHAR_PIXEL 4
#define BORDER_MARGIN 1

unsigned char lcd_buffer[2][11];

static int double_height=1;
extern bool lcd_display_redraw;
extern unsigned const char *lcd_ascii;


void lcd_print_icon(int x, int icon_line, bool enable, char **icon)
{
  int xpos = x;
  int ypos = icon_line*(ICON_HEIGHT+CHAR_HEIGHT*2*4);
  int row=0, col;

  int p=0, cp=0;
  struct coordinate points[LCD_WIDTH * LCD_HEIGHT];
  struct coordinate clearpoints[LCD_WIDTH * LCD_HEIGHT];
  
  while (icon[row]) {
    col=0;
    while (icon[row][col]) {
      switch(icon[row][col]) {
      case '*':
        if (enable) {
          /* set a dot */
          points[p].x = xpos + col +BORDER_MARGIN;
          points[p].y = ypos+row +BORDER_MARGIN;
          p++; /* increase the point counter */
        } else {
          /* clear a dot */
          clearpoints[cp].x = xpos + col +BORDER_MARGIN;
          clearpoints[cp].y = ypos+row +BORDER_MARGIN;
          cp++; /* increase the point counter */
        }
        break;
      case ' ': /* Clear bit */
        /* clear a dot */
        clearpoints[cp].x = xpos + col+BORDER_MARGIN;
        clearpoints[cp].y = ypos+row+BORDER_MARGIN;
        cp++; /* increase the point counter */
        break;
      }
      col++;
    }
    row++;
  }
  drawdots(0, &clearpoints[0], cp);
  drawdots(1, &points[0], p);
}

void lcd_print_char(int x, int y)
{
  int xpos = x * CHAR_WIDTH * CHAR_PIXEL;
  int ypos = y * CHAR_HEIGHT * CHAR_PIXEL + ICON_HEIGHT;
  int col, row;
  int p=0, cp=0;
  struct rectangle points[CHAR_HEIGHT*CHAR_WIDTH];
  struct rectangle clearpoints[CHAR_HEIGHT*CHAR_WIDTH];
  unsigned char ch=lcd_buffer[y][x];

  if (double_height == 2 && y == 1)
    return; /* Second row can't be printed in double height. ??*/

  /* Clear all char
  clearpoints[cp].x = xpos +BORDER_MARGIN;
  clearpoints[cp].y = ypos +BORDER_MARGIN;
  clearpoints[cp].width=CHAR_WIDTH*CHAR_PIXEL;
  clearpoints[cp].height=7*CHAR_PIXEL*double_height;
  cp++;
  */
  for (col=0; col<5; col++) {
    for (row=0; row<7; row++) {
      char fontbit=(*font_player)[ch][col]&(1<<row);
      int height=CHAR_PIXEL*double_height;

      y=CHAR_PIXEL*(double_height*row)+ypos;
      if (double_height==2) {
	if (row == 3) /* Adjust for blank row in the middle */
	  height=CHAR_PIXEL;
      }

      if (fontbit) {
        /* set a dot */
        points[p].x = xpos + col*CHAR_PIXEL +BORDER_MARGIN;
        points[p].y = y +BORDER_MARGIN;
        points[p].width=CHAR_PIXEL;
        points[p].height=height;
        p++; /* increase the point counter */
      } else {
        clearpoints[cp].x = xpos + col*CHAR_PIXEL +BORDER_MARGIN;
        clearpoints[cp].y = y +BORDER_MARGIN;
        clearpoints[cp].width=CHAR_PIXEL;
        clearpoints[cp].height=height;
        cp++;
      }
    }
  }
  drawrectangles(0, &clearpoints[0], cp);
  drawrectangles(1, &points[0], p);
}


/*
 * Draw a rectangle with upper left corner at (x, y)
 * and size (nx, ny)
 */
void lcd_drawrect (int x, int y, int nx, int ny)
{
    (void)x;
    (void)y;
    (void)nx;
    (void)ny;
}

/* Invert a rectangular area at (x, y), size (nx, ny) */
void lcd_invertrect (int x, int y, int nx, int ny)
{
    (void)x;
    (void)y;
    (void)nx;
    (void)ny;
}

void lcd_drawline( int x1, int y1, int x2, int y2 )
{
    (void)x1;
    (void)x2;
    (void)y1;
    (void)y2;
}

void lcd_clearline( int x1, int y1, int x2, int y2 )
{
    (void)x1;
    (void)x2;
    (void)y1;
    (void)y2;
}

/*
 * Set a single pixel
 */
void lcd_drawpixel(int x, int y)
{
    (void)x;
    (void)y;
}

/*
 * Clear a single pixel
 */
void lcd_clearpixel(int x, int y)
{
    (void)x;
    (void)y;
}

/*
 * Invert a single pixel
 */
void lcd_invertpixel(int x, int y)
{
    (void)x;
    (void)y;
}

void lcd_clear_display(void)
{
    int x, y;
    for (y=0; y<2; y++) {
      for (x=0; x<11; x++) {
        lcd_buffer[y][x]=lcd_ascii[' '];
      }
    }
    lcd_update();
}

void lcd_puts(int x, int y, unsigned char *str)
{
    int i;
    DEBUGF("lcd_puts(%d, %d, \"", x, y);
    for (i=0; *str && x<11; i++) {
#ifdef DEBUGF
        if (*str>=32 && *str<128)
            {DEBUGF("%c", *str);}
        else
            {DEBUGF("(0x%02x)", *str);}
#endif
      lcd_buffer[y][x++]=lcd_ascii[*str++];
    }
    DEBUGF("\")\n");
    for (; x<11; x++)
        lcd_buffer[y][x]=lcd_ascii[' '];
    lcd_update();
}

void lcd_double_height(bool on)
{
    double_height = 1;
    if (on)
        double_height = 2;
    lcd_display_redraw=true;
    lcd_update();
}

void lcd_define_pattern(int which, char *pattern, int length)
{
    int i, j;
    int pat = which / 8;
    char icon[8];
    memset(icon, 0, sizeof icon);

    DEBUGF("Defining pattern %d:", pat);
    for (j = 0; j <= 5; j++) {
        for (i = 0; i < length; i++) {
            if ((pattern[i])&(1<<(j)))
                icon[5-j] |= (1<<(i));
        }
    }
    for (i = 1; i <= 5; i++)
    {
        DEBUGF(" 0x%02x", icon[i]);
        (*font_player)[pat][i-1] = icon[i];
    }
    DEBUGF("\n");
    lcd_display_redraw=true;
    lcd_update();
}

extern void lcd_puts(int x, int y, unsigned char *str);

void lcd_putc(int x, int y, unsigned char ch)
{
    DEBUGF("lcd_putc(%d, %d, %d '0x%02x')\n", x, y, ch, ch);
    lcd_buffer[y][x]=lcd_ascii[ch];
    lcd_update();
}
