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
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "version.h"
#include "debug_menu.h"
#include "sprintf.h"
#include <string.h>
#include "font.h"
#include "screens.h"
#include "status.h"
#include "talk.h"
#include "settings.h"

#define KEYBOARD_LINES 4
#define KEYBOARD_PAGES 3

static void kbd_setupkeys(char* line[KEYBOARD_LINES], int page)
{
    switch (page) {
    case 0:
        line[0] = "ABCDEFG !?\" @#$%+'";
        line[1] = "HIJKLMN 789 &_()-`";
        line[2] = "OPQRSTU 456 �|{}/<";
        line[3] = "VWXYZ.,0123 ~=[]*>";
        break;

    case 1:
        line[0] = "abcdefg ���������";
        line[1] = "hijklmn ���������";
        line[2] = "opqrstu ���������";
        line[3] = "vwxyz., ���      ";
        break;

    case 2:
        line[0] = "������� ���� ����";
        line[1] = "������� ���� ����";
        line[2] = "������ ����� ����";
        line[3] = "������ ����� ����";
        break;
    }
}

/* helper function to spell a char if voice UI is enabled */
void kbd_spellchar(char c)
{
    char spell_char[2]; /* store char to pass to talk_spell */

    if (global_settings.talk_menu) /* voice UI? */
    {
        spell_char[0] = c; 
        spell_char[1] = '\0'; /* mark end of char string */ 

        talk_spell(spell_char, false); 
    }
}

int kbd_input(char* text, int buflen)
{
    bool done = false;
    int page = 0;

    int font_w = 0, font_h = 0, i;
    int x = 0, y = 0;
    int main_x, main_y, max_chars, margin;
    int status_y1, status_y2, curpos;
    int len;
    int editpos;
    bool redraw = true;
    char* line[KEYBOARD_LINES];

    char outline[256];
    char c = 0;
    struct font* font = font_get(FONT_SYSFIXED);

    lcd_setfont(FONT_SYSFIXED);
    font_w = font->maxwidth;
    font_h = font->height;

    margin = 3;
    main_y = (KEYBOARD_LINES + 1) * font_h + margin*2;
    main_x = 0;
    status_y1 = LCD_HEIGHT - font_h;
    status_y2 = LCD_HEIGHT;

    editpos = strlen(text);

    max_chars = LCD_WIDTH / font_w;
    kbd_setupkeys(line, page);

    if (global_settings.talk_menu) /* voice UI? */
        talk_spell(text, true); /* spell initial text */ 

    while(!done)
    {
        len = strlen(text);
            
        if(redraw)
        {
            lcd_clear_display();
            
            lcd_setfont(FONT_SYSFIXED);
            
            /* draw page */
            for (i=0; i < KEYBOARD_LINES; i++)
                lcd_putsxy(0, 8+i * font_h, line[i]);
            
            /* separator */
            lcd_drawline(0, main_y - margin, LCD_WIDTH, main_y - margin);
            
            /* write out the text */
            if (editpos < max_chars - 3 )
            {
                strncpy(outline, text, max_chars - 2);
                if (len > max_chars - 2)
                    lcd_putsxy(LCD_WIDTH - font_w, main_y, ">");
                curpos = (1 + editpos) * font_w;
            }
            else
            {
                /* not room for all text, cut left, right or both */
                if (editpos == len )
                {
                    if ( max_chars - 3 == len)
                    {
                        strncpy(outline, text, max_chars - 2);
                        curpos  = (1 + editpos) * font_w;
                    }
                    else
                    {
                        strncpy(outline, text + editpos - max_chars + 2,
                                max_chars - 2);
                        if (len > max_chars - 2)
                            lcd_putsxy(0, main_y, "<");
                        curpos = ( max_chars - 1) * font_w;
                    }
                }
                else
                {
                    if (len - 1 == editpos)
                    {
                        strncpy(outline, text + editpos - max_chars + 3,
                                max_chars - 2);
                        curpos = ( max_chars - 2) * font_w;
                    }
                    else
                    {
                        strncpy(outline, text + editpos - max_chars + 4,
                                max_chars - 2);
                        curpos = ( max_chars - 3) * font_w;
                    }
                    lcd_putsxy(LCD_WIDTH - font_w, main_y, ">");
                    lcd_putsxy(0, main_y, "<");
                }
            }

            /* Zero terminate the string */
            outline[max_chars - 2] = '\0';
            
            lcd_putsxy(font_w,main_y,outline);
            
            /* cursor */
            lcd_drawline(curpos, main_y, curpos, main_y + font_h);
            
            /* draw the status bar */
            buttonbar_set("Shift", "OK", "Del");
            buttonbar_draw();
            
            /* highlight the key that has focus */
            lcd_invertrect(font_w * x, 8 + font_h * y, font_w, font_h);
            
            status_draw(true);
        
            lcd_update();
        }

        /* The default action is to redraw */
        redraw = true;
        
        switch ( button_get_w_tmo(HZ/2) ) {

            case BUTTON_OFF:
                /* abort */
                lcd_setfont(FONT_UI);
                return -1;
                break;

            case BUTTON_F1:
                /* Page */
                if (++page == KEYBOARD_PAGES)
                    page = 0;
                kbd_setupkeys(line, page);
                kbd_spellchar(line[y][x]);
                break;

            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (x < (int)strlen(line[y]) - 1)
                    x++;
                else
                    x = 0;
                kbd_spellchar(line[y][x]);
                break;

            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (x)
                    x--;
                else
                    x = strlen(line[y]) - 1;
                kbd_spellchar(line[y][x]);
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                if (y < KEYBOARD_LINES - 1)
                    y++;
                else
                    y=0;
                kbd_spellchar(line[y][x]);
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                if (y)
                    y--;
                else
                    y = KEYBOARD_LINES - 1;
                kbd_spellchar(line[y][x]);
                break;

            case BUTTON_F3:
            case BUTTON_F3 | BUTTON_REPEAT:
                /* backspace */
                if (editpos > 0)
                {
                    for (i = editpos; i <= (len - 1);i++)
                    {
                        text[i-1] = text[i];
                    }
                    text[i-1]='\0';
                    editpos--;
                    if (editpos < 0)
                        editpos=0;
                }
                break;

            case BUTTON_F2:
                /* F2 accepts what was entered and continues */
                done = true;
                break;

            case BUTTON_PLAY:
                /* PLAY inserts the selected char */
                if (len<buflen)
                {
                    c = line[y][x];
                    if ( editpos == len )
                    {
                        text[len] = c;
                        text[len+1] = 0;
                    }
                    else
                    {
                        for (i = len ; i + 1 > editpos; i--)
                            text[i+1] = text[i];
                        text[editpos] = c;
                    }
                    editpos++;
                }
                if (global_settings.talk_menu) /* voice UI? */
                    talk_spell(text, false); /* speak revised text */
                break;

            case BUTTON_ON | BUTTON_RIGHT:
            case BUTTON_ON | BUTTON_RIGHT | BUTTON_REPEAT:
                /* moved cursor right */
                editpos++;
                if (editpos > len)
                    editpos = len;
                else
                    kbd_spellchar(text[editpos]);
                break;

            case BUTTON_ON | BUTTON_LEFT:
            case BUTTON_ON | BUTTON_LEFT | BUTTON_REPEAT:
                /* moved cursor left */
                editpos--;
                if (editpos < 0)
                    editpos = 0;
                else
                    kbd_spellchar(text[editpos]);
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                lcd_setfont(FONT_SYSFIXED);
                break;

            case BUTTON_NONE:
                status_draw(false);
                redraw = false;
                break;
        }
    }
    lcd_setfont(FONT_UI);
    return 0;
}
