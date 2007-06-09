/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "sprintf.h"
#include "string.h"
#include "settings.h"
#include "kernel.h"
#include "system.h"

#include "action.h"
#include "screen_access.h"
#include "list.h"
#include "scrollbar.h"
#include "statusbar.h"
#include "textarea.h"
#include "lang.h"
#include "sound.h"
#include "misc.h"

#ifdef HAVE_LCD_CHARCELLS
#define SCROLL_LIMIT 1
#else
#define SCROLL_LIMIT (nb_lines<3?1:2)
#endif

/* The minimum number of pending button events in queue before starting
 * to limit list drawing interval.
 */
#define FRAMEDROP_TRIGGER 6

#ifdef HAVE_LCD_BITMAP
static int offset_step = 16; /* pixels per screen scroll step */
/* should lines scroll out of the screen */
static bool offset_out_of_view = false;
#endif
static struct gui_list* last_list_displayed[NB_SCREENS];

#define SHOW_LIST_TITLE ((gui_list->title != NULL) && \
                         (gui_list->display->nb_lines > 2))

static void gui_list_select_at_offset(struct gui_list * gui_list, int offset);

/*
 * Initializes a scrolling list
 *  - gui_list : the list structure to initialize
 *  - callback_get_item_icon : pointer to a function that associates an icon
 *    to a given item number
 *  - callback_get_item_name : pointer to a function that associates a label
 *    to a given item number
 *  - data : extra data passed to the list callback
 */
static void gui_list_init(struct gui_list * gui_list,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size
    )
{
    gui_list->callback_get_item_icon = NULL;
    gui_list->callback_get_item_name = callback_get_item_name;
    gui_list->display = NULL;
    gui_list_set_nb_items(gui_list, 0);
    gui_list->selected_item = 0;
    gui_list->start_item = 0;
    gui_list->limit_scroll = false;
    gui_list->data=data;
    gui_list->cursor_flash_state=false;
#ifdef HAVE_LCD_BITMAP
    gui_list->offset_position = 0;
#endif
    gui_list->scroll_all=scroll_all;
    gui_list->selected_size=selected_size;
    gui_list->title = NULL;
    gui_list->title_width = 0;
    gui_list->title_icon = Icon_NOICON;

    gui_list->last_displayed_selected_item = -1 ;
    gui_list->last_displayed_start_item = -1 ;
    gui_list->show_selection_marker = true;
}

/* this toggles the selection bar or cursor */
void gui_synclist_hide_selection_marker(struct gui_synclist * lists, bool hide)
{
    int i;
    FOR_NB_SCREENS(i)
        lists->gui_list[i].show_selection_marker = !hide;
}

/*
 * Attach the scrolling list to a screen
 * (The previous screen attachement is lost)
 *  - gui_list : the list structure
 *  - display : the screen to attach
 */
static void gui_list_set_display(struct gui_list * gui_list, struct screen * display)
{
    if(gui_list->display != 0) /* we switched from a previous display */
        gui_list->display->stop_scroll();
    gui_list->display = display;
#ifdef HAVE_LCD_CHARCELLS
    display->double_height(false);
#endif
    gui_list_select_at_offset(gui_list, 0);
}

/*
 * One call on 2, the selected lune will either blink the cursor or
 * invert/display normal the selected line
 * - gui_list : the list structure
 */
static void gui_list_flash(struct gui_list * gui_list)
{
    struct screen * display=gui_list->display;
    gui_list->cursor_flash_state=!gui_list->cursor_flash_state;
    int selected_line=gui_list->selected_item-gui_list->start_item+SHOW_LIST_TITLE;
#ifdef HAVE_LCD_BITMAP
    int line_ypos=display->getymargin()+display->char_height*selected_line;
    if (global_settings.invert_cursor)
    {
        int line_xpos=display->getxmargin();
        display->set_drawmode(DRMODE_COMPLEMENT);
        display->fillrect(line_xpos, line_ypos, display->width,
                          display->char_height);
        display->set_drawmode(DRMODE_SOLID);
        display->invertscroll(0, selected_line);
    }
    else
    {
        int cursor_xpos=(global_settings.scrollbar &&
                         display->nb_lines < gui_list->nb_items)?1:0;
        screen_put_cursorxy(display, cursor_xpos, selected_line,
                            gui_list->cursor_flash_state);
    }
    display->update_rect(0, line_ypos,display->width,
                         display->char_height);
#else
    screen_put_cursorxy(display, 0, selected_line,
                        gui_list->cursor_flash_state);
    gui_textarea_update(display);
#endif
}


#ifdef HAVE_LCD_BITMAP
static int gui_list_get_item_offset(struct gui_list * gui_list, int item_width,
                             int text_pos)
{
    struct screen * display=gui_list->display;
    int item_offset;

    if (offset_out_of_view)
    {
        item_offset = gui_list->offset_position;
    }
    else
    {
        /* if text is smaller then view */
        if (item_width <= display->width - text_pos)
        {
            item_offset = 0;
        }
        else
        {
            /* if text got out of view  */
            if (gui_list->offset_position >
                    item_width - (display->width - text_pos))
                item_offset = item_width - (display->width - text_pos);
            else
                item_offset = gui_list->offset_position;
        }
    }

    return item_offset;
}
#endif

/*
 * Draws the list on the attached screen
 * - gui_list : the list structure
 */
static void gui_list_draw_smart(struct gui_list *gui_list)
{
    struct screen * display=gui_list->display;
    int text_pos;
    bool draw_icons = (gui_list->callback_get_item_icon != NULL && global_settings.show_icons);
    bool draw_cursor;
    int i;
    int lines;
#ifdef HAVE_LCD_BITMAP
    int item_offset;
    int old_margin = display->getxmargin();
#endif
    int start, end;
    bool partial_draw = false;

#ifdef HAVE_LCD_BITMAP
    display->setfont(FONT_UI);
#endif
    /* Speed up UI by drawing the changed contents only. */
    if (gui_list == last_list_displayed[gui_list->display->screen_type]
        && gui_list->last_displayed_start_item == gui_list->start_item
        && gui_list->selected_size == 1)
    {
        partial_draw = true;
    }

    if (SHOW_LIST_TITLE)
        lines = display->nb_lines - 1;
    else
        lines = display->nb_lines;

    if (partial_draw)
    {
        end = gui_list->last_displayed_selected_item - gui_list->start_item;
        i = gui_list->selected_item - gui_list->start_item;
        if (i < end )
        {
            start = i;
            end++;
        }
        else
        {
            start = end;
            end = i + 1;
        }
    }
    else
    {
        gui_textarea_clear(display);
        start = 0;
        end = display->nb_lines;
        gui_list->last_displayed_start_item = gui_list->start_item;
        last_list_displayed[gui_list->display->screen_type] = gui_list;
    }

    gui_list->last_displayed_selected_item = gui_list->selected_item;

    /* position and draw the list title & icon */
    if (SHOW_LIST_TITLE && !partial_draw)
    {
        if (gui_list->title_icon != NOICON && draw_icons)
        {
            screen_put_icon(display, 0, 0, gui_list->title_icon);
#ifdef HAVE_LCD_BITMAP
            text_pos = get_icon_width(display->screen_type)+2; /* pixels */
#else
            text_pos = 1; /* chars */
#endif
        }
        else
        {
            text_pos = 0;
        }

#ifdef HAVE_LCD_BITMAP
        screen_set_xmargin(display, text_pos); /* margin for title */
        item_offset = gui_list_get_item_offset(gui_list, gui_list->title_width,
                                               text_pos);
        if (item_offset > gui_list->title_width - (display->width - text_pos))
            display->puts_offset(0, 0, gui_list->title, item_offset);
        else
            display->puts_scroll_offset(0, 0, gui_list->title, item_offset);
#else
        display->puts_scroll(text_pos, 0, gui_list->title);
#endif
    }

    /* Adjust the position of icon, cursor, text for the list */
#ifdef HAVE_LCD_BITMAP
    gui_textarea_update_nblines(display);
    bool draw_scrollbar;

    draw_scrollbar = (global_settings.scrollbar &&
                lines < gui_list->nb_items);

    draw_cursor = !global_settings.invert_cursor && 
                    gui_list->show_selection_marker;
    text_pos = 0; /* here it's in pixels */
    if(draw_scrollbar || SHOW_LIST_TITLE) /* indent if there's
                                             a title */
    {
        text_pos += SCROLLBAR_WIDTH;
    }
    if(draw_cursor)
        text_pos += get_icon_width(display->screen_type) + 2;

    if(draw_icons)
        text_pos += get_icon_width(display->screen_type) + 2;
#else
    draw_cursor = true;
    if(draw_icons)
        text_pos = 2; /* here it's in chars */
    else
        text_pos = 1;
#endif

#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, text_pos); /* margin for list */
#endif

    if (SHOW_LIST_TITLE)
    {
        start++;
        if (end < display->nb_lines)
            end++;
    }

    for (i = start; i < end; i++)
    {
        unsigned char *s;
        char entry_buffer[MAX_PATH];
        unsigned char *entry_name;
        int current_item = gui_list->start_item +
                           (SHOW_LIST_TITLE ? i-1 : i);

        /* When there are less items to display than the
         * current available space on the screen, we stop*/
        if(current_item >= gui_list->nb_items)
            break;
        s = gui_list->callback_get_item_name(current_item,
                                             gui_list->data,
                                             entry_buffer);
        entry_name = P2STR(s);
        
#ifdef HAVE_LCD_BITMAP
        /* position the string at the correct offset place */
        int item_width,h;
        display->getstringsize(entry_name, &item_width, &h);
        item_offset = gui_list_get_item_offset(gui_list, item_width, text_pos);
#endif

        if(gui_list->show_selection_marker &&
           current_item >= gui_list->selected_item &&
           current_item <  gui_list->selected_item + gui_list->selected_size)
        {/* The selected item must be displayed scrolling */
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)/* Display inverted-line-style*/
            {
                /* if text got out of view */
                if (item_offset > item_width - (display->width - text_pos))
                {
                    /* don't scroll */
                    display->puts_style_offset(0, i, entry_name,
                                               STYLE_INVERT,item_offset);
                }
                else
                {
                    display->puts_scroll_style_offset(0, i, entry_name,
                                                      STYLE_INVERT,
                                                      item_offset);
                }
            }
            else  /*  if (!global_settings.invert_cursor) */
            {
                if (item_offset > item_width - (display->width - text_pos))
                    display->puts_offset(0, i, entry_name,item_offset);
                else
                    display->puts_scroll_offset(0, i, entry_name,item_offset);
                if (current_item % gui_list->selected_size != 0)
                    draw_cursor = false;
            }
#else
            display->puts_scroll(text_pos, i, entry_name);
#endif

            if (draw_cursor)
            {
                screen_put_icon_with_offset(display, 0, i,
                                           (draw_scrollbar || SHOW_LIST_TITLE)?
                                                   SCROLLBAR_WIDTH: 0,
                                           0, Icon_Cursor);
            }
        }
        else
        {/* normal item */
            if(gui_list->scroll_all)
            {
#ifdef HAVE_LCD_BITMAP
                display->puts_scroll_offset(0, i, entry_name,item_offset);
#else
                display->puts_scroll(text_pos, i, entry_name);
#endif
            }
            else
            {
#ifdef HAVE_LCD_BITMAP
                display->puts_offset(0, i, entry_name,item_offset);
#else
                display->puts(text_pos, i, entry_name);
#endif
            }
        }
        /* Icons display */
        if(draw_icons)
        {
            enum themable_icons icon;
            icon = gui_list->callback_get_item_icon(current_item, gui_list->data);
            if(icon > Icon_NOICON)
            {
#ifdef HAVE_LCD_BITMAP
                int x = draw_cursor?1:0;
                int x_off = (draw_scrollbar || SHOW_LIST_TITLE) ? SCROLLBAR_WIDTH: 0;
                screen_put_icon_with_offset(display, x, i,
                                           x_off, 0, icon);
#else
                screen_put_icon(display, 1, i, icon);
#endif
            }
        }
    }

#ifdef HAVE_LCD_BITMAP
    /* Draw the scrollbar if needed*/
    if(draw_scrollbar)
    {
        int y_start = gui_textarea_get_ystart(display);
        if (SHOW_LIST_TITLE)
            y_start += display->char_height;
        int scrollbar_y_end = display->char_height *
                              lines + y_start;
        gui_scrollbar_draw(display, 0, y_start, SCROLLBAR_WIDTH-1,
                           scrollbar_y_end - y_start, gui_list->nb_items,
                           gui_list->start_item,
                           gui_list->start_item + lines, VERTICAL);
    }

    screen_set_xmargin(display, old_margin);
#endif

    gui_textarea_update(display);
}

/*
 * Force a full screen update.
 */
static void gui_list_draw(struct gui_list *gui_list)
{
    last_list_displayed[gui_list->display->screen_type] = NULL;
    return gui_list_draw_smart(gui_list);
}

/*
 * Selects an item in the list
 *  - gui_list : the list structure
 *  - item_number : the number of the item which will be selected
 */
static void gui_list_select_item(struct gui_list * gui_list, int item_number)
{
    if( item_number > gui_list->nb_items-1 || item_number < 0 )
        return;
    gui_list->selected_item = item_number;
    gui_list_select_at_offset(gui_list, 0);
}

/* select an item above the current one */
static void gui_list_select_above(struct gui_list * gui_list, int items)
{
    int nb_lines = gui_list->display->nb_lines;
    if (SHOW_LIST_TITLE)
        nb_lines--;
    
    gui_list->selected_item -= items;
    
    /* in bottom "3rd" of the screen, so dont move the start item.
       by 3rd I mean above SCROLL_LIMIT lines above the end of the screen */
    if (items && gui_list->start_item + SCROLL_LIMIT < gui_list->selected_item)
    {
        if (gui_list->show_selection_marker == false)
        {
            gui_list->start_item -= items;
            if (gui_list->start_item < 0)
                gui_list->start_item = 0;
        }
        return;
    }
    if (gui_list->selected_item < 0)
    {
        if(gui_list->limit_scroll)
        {
            gui_list->selected_item = 0;
            gui_list->start_item = 0;
        }
        else
        {
            gui_list->selected_item += gui_list->nb_items;
            if (global_settings.scroll_paginated)
            {
                gui_list->start_item = gui_list->nb_items - nb_lines;
            }
        }
    }
    if (gui_list->nb_items > nb_lines)
    {
        if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item > gui_list->selected_item)
                gui_list->start_item = MAX(0, gui_list->start_item - nb_lines);
        }
        else
        {
            int top_of_screen = gui_list->selected_item - SCROLL_LIMIT;
            int temp = MIN(top_of_screen, gui_list->nb_items - nb_lines);
            gui_list->start_item = MAX(0, temp);
        }
    }
    else gui_list->start_item = 0;
    if (gui_list->selected_size > 1)
    {
        if (gui_list->start_item + nb_lines == gui_list->selected_item)
            gui_list->start_item++;
    }
}
/* select an item below the current one */
static void gui_list_select_below(struct gui_list * gui_list, int items)
{
    int nb_lines = gui_list->display->nb_lines;
    int bottom;
    if (SHOW_LIST_TITLE)
        nb_lines--;
    
    gui_list->selected_item += items;
    bottom = gui_list->nb_items - nb_lines;
    
    /* always move the screen if selection isnt "visible" */
    if (items && gui_list->show_selection_marker == false)
    {
        if (bottom < 0)
            bottom = 0;
        gui_list->start_item = MIN(bottom, gui_list->start_item +
                                           items);
        return;
    }
    /* in top "3rd" of the screen, so dont move the start item */
    if (items && 
        (gui_list->start_item + nb_lines - SCROLL_LIMIT > gui_list->selected_item)
        && (gui_list->selected_item < gui_list->nb_items))
    {
        if (gui_list->show_selection_marker == false)
        {
            if (bottom < 0)
                bottom = 0;
            gui_list->start_item = MIN(bottom, 
                                       gui_list->start_item + items);
        }
        return;
    }
    
    if (gui_list->selected_item >= gui_list->nb_items)
    {
        if(gui_list->limit_scroll)
        {
            gui_list->selected_item = gui_list->nb_items-gui_list->selected_size;
            gui_list->start_item = MAX(0,gui_list->nb_items - nb_lines);
        }
        else
        {
            gui_list->selected_item = 0;
            gui_list->start_item = 0;
        }
        return;
    }
    
    if (gui_list->nb_items > nb_lines)
    {
        if (global_settings.scroll_paginated)
        {
            if (gui_list->start_item + nb_lines <= gui_list->selected_item)
                gui_list->start_item = MIN(bottom, gui_list->selected_item);
        }
        else
        {
            int top_of_screen = gui_list->selected_item + SCROLL_LIMIT - nb_lines;
            int temp = MAX(0, top_of_screen);
            gui_list->start_item = MIN(bottom, temp);
        }
    }
    else gui_list->start_item = 0;
}

static void gui_list_select_at_offset(struct gui_list * gui_list, int offset)
{
    if (gui_list->selected_size > 1)
    {
        offset *= gui_list->selected_size;
        /* always select the first item of multi-line lists */
        offset -= offset%gui_list->selected_size;
    }
    if (offset == 0 && global_settings.scroll_paginated &&
        (gui_list->nb_items > gui_list->display->nb_lines - SHOW_LIST_TITLE))
    {
        gui_list->selected_item = gui_list->selected_item;
    }
    else if (offset < 0)
        gui_list_select_above(gui_list, -offset);
    else
        gui_list_select_below(gui_list, offset);
}

/*
 * Adds an item to the list (the callback will be asked for one more item)
 * - gui_list : the list structure
 */
static void gui_list_add_item(struct gui_list * gui_list)
{
    gui_list->nb_items++;
    /* if only one item in the list, select it */
    if(gui_list->nb_items == 1)
        gui_list->selected_item = 0;
}

/*
 * Removes an item to the list (the callback will be asked for one less item)
 * - gui_list : the list structure
 */
static void gui_list_del_item(struct gui_list * gui_list)
{
    if(gui_list->nb_items > 0)
    {
        gui_textarea_update_nblines(gui_list->display);
        int nb_lines = gui_list->display->nb_lines;

        int dist_selected_from_end = gui_list->nb_items
            - gui_list->selected_item - 1;
        int dist_start_from_end = gui_list->nb_items
            - gui_list->start_item - 1;
        if(dist_selected_from_end == 0)
        {
            /* Oops we are removing the selected item,
               select the previous one */
            gui_list->selected_item--;
        }
        gui_list->nb_items--;

        /* scroll the list if needed */
        if( (dist_start_from_end < nb_lines) && (gui_list->start_item != 0) )
            gui_list->start_item--;
    }
}

#ifdef HAVE_LCD_BITMAP

/*
 * Makes all the item in the list scroll by one step to the right.
 * Should stop increasing the value when reaching the widest item value 
 * in the list.
 */
static void gui_list_scroll_right(struct gui_list * gui_list)
{
    /* FIXME: This is a fake right boundry limiter. there should be some
     * callback function to find the longest item on the list in pixels,
     * to stop the list from scrolling past that point */
    gui_list->offset_position+=offset_step;
    if (gui_list->offset_position > 1000)
        gui_list->offset_position = 1000;
}

/*
 * Makes all the item in the list scroll by one step to the left.
 * stops at starting position.
 */
static void gui_list_scroll_left(struct gui_list * gui_list)
{
    gui_list->offset_position-=offset_step;
    if (gui_list->offset_position < 0)
        gui_list->offset_position = 0;
}
void gui_list_screen_scroll_step(int ofs)
{
     offset_step = ofs;
}

void gui_list_screen_scroll_out_of_view(bool enable)
{
    if (enable)
        offset_out_of_view = true;
    else
        offset_out_of_view = false;
}
#endif /* HAVE_LCD_BITMAP */

/* 
 * Set the title and title icon of the list. Setting title to NULL disables
 * both the title and icon. Use NOICON if there is no icon.
 */
static void gui_list_set_title(struct gui_list * gui_list, 
                               char * title, enum themable_icons icon)
{
    gui_list->title = title;
    gui_list->title_icon = icon;
    if (title) {
#ifdef HAVE_LCD_BITMAP
        gui_list->display->getstringsize(title, &gui_list->title_width, NULL);
#else
        gui_list->title_width = strlen(title);
#endif
    } else {
        gui_list->title_width = 0;
    }
}

/*
 * Synchronized lists stuffs
 */
void gui_synclist_init(
    struct gui_synclist * lists,
    list_get_name callback_get_item_name,
    void * data,
    bool scroll_all,
    int selected_size
    )
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_init(&(lists->gui_list[i]),
                      callback_get_item_name,
                      data, scroll_all, selected_size);
        gui_list_set_display(&(lists->gui_list[i]), &(screens[i]));
    }
}

void gui_synclist_set_nb_items(struct gui_synclist * lists, int nb_items)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_set_nb_items(&(lists->gui_list[i]), nb_items);
#ifdef HAVE_LCD_BITMAP
        lists->gui_list[i].offset_position = 0;
#endif
    }
}
int gui_synclist_get_nb_items(struct gui_synclist * lists)
{
    return gui_list_get_nb_items(&((lists)->gui_list[0]));
}
int  gui_synclist_get_sel_pos(struct gui_synclist * lists)
{
    return gui_list_get_sel_pos(&((lists)->gui_list[0]));
}
void gui_synclist_set_icon_callback(struct gui_synclist * lists,
                                    list_get_icon icon_callback)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        gui_list_set_icon_callback(&(lists->gui_list[i]), icon_callback);
    }
}

void gui_synclist_draw(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_draw(&(lists->gui_list[i]));
}

void gui_synclist_select_item(struct gui_synclist * lists, int item_number)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_item(&(lists->gui_list[i]), item_number);
}

static void gui_synclist_select_next_page(struct gui_synclist * lists,
                                   enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_at_offset(&(lists->gui_list[i]),
                                  screens[screen].nb_lines);
}

static void gui_synclist_select_previous_page(struct gui_synclist * lists,
                                       enum screen_type screen)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_select_at_offset(&(lists->gui_list[i]),
                                      -screens[screen].nb_lines);
}

void gui_synclist_add_item(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_add_item(&(lists->gui_list[i]));
}

void gui_synclist_del_item(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_del_item(&(lists->gui_list[i]));
}

void gui_synclist_limit_scroll(struct gui_synclist * lists, bool scroll)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_limit_scroll(&(lists->gui_list[i]), scroll);
}

void gui_synclist_set_title(struct gui_synclist * lists,
                            char * title, enum themable_icons icon)
{
    int i;
    FOR_NB_SCREENS(i)
            gui_list_set_title(&(lists->gui_list[i]), title, icon);
}

void gui_synclist_flash(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_flash(&(lists->gui_list[i]));
}

#ifdef HAVE_LCD_BITMAP
static void gui_synclist_scroll_right(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_right(&(lists->gui_list[i]));
}

static void gui_synclist_scroll_left(struct gui_synclist * lists)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_list_scroll_left(&(lists->gui_list[i]));
}
#endif /* HAVE_LCD_BITMAP */

unsigned gui_synclist_do_button(struct gui_synclist * lists,
                                unsigned button,enum list_wrap wrap)
{
#ifdef HAVE_LCD_BITMAP
    static bool scrolling_left = false;
#endif
    static int next_item_modifier = 1;
    static int last_accel_tick = 0;
    int i;

    if (global_settings.list_accel_start_delay)
    {
        int start_delay = global_settings.list_accel_start_delay * (HZ/2);
        int accel_wait = global_settings.list_accel_wait * HZ/2;

        if (get_action_statuscode(NULL)&ACTION_REPEAT)
        {
            if (!last_accel_tick)
                last_accel_tick = current_tick + start_delay;
            else if (current_tick >= 
                        last_accel_tick + accel_wait)
            {
                last_accel_tick = current_tick;
                next_item_modifier++;
            }
        }
        else if (last_accel_tick)
        {
            next_item_modifier = 1;
            last_accel_tick = 0;
        }
    }

    switch (wrap)
    {
        case LIST_WRAP_ON:
            gui_synclist_limit_scroll(lists, false);
        break;
        case LIST_WRAP_OFF:
            gui_synclist_limit_scroll(lists, true);
        break;
        case LIST_WRAP_UNLESS_HELD:
            if (button == ACTION_STD_PREVREPEAT ||
                button == ACTION_STD_NEXTREPEAT ||
                button == ACTION_LISTTREE_PGUP  ||
                button == ACTION_LISTTREE_PGDOWN)
                gui_synclist_limit_scroll(lists, true);
            else gui_synclist_limit_scroll(lists, false);
        break;
    };

    switch(button)
    {
#ifdef HAVE_VOLUME_IN_LIST
        case ACTION_LIST_VOLUP:
            global_settings.volume += 2; 
            /* up two because the falthrough brings it down one */
        case ACTION_LIST_VOLDOWN:
            global_settings.volume--;
            setvol();
            return button;
#endif
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            FOR_NB_SCREENS(i)
                gui_list_select_at_offset(&(lists->gui_list[i]), -next_item_modifier);
            if (queue_count(&button_queue) < FRAMEDROP_TRIGGER)
                gui_synclist_draw(lists);
            yield();
            return ACTION_STD_PREV;

        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            FOR_NB_SCREENS(i)
                gui_list_select_at_offset(&(lists->gui_list[i]), next_item_modifier);
            if (queue_count(&button_queue) < FRAMEDROP_TRIGGER)
                gui_synclist_draw(lists);
            yield();
            return ACTION_STD_NEXT;

#ifdef HAVE_LCD_BITMAP
        case ACTION_TREE_ROOT_INIT:
         /* After this button press ACTION_TREE_PGLEFT is allowed
            to skip to root. ACTION_TREE_ROOT_INIT must be defined in the
            keymaps as a repeated button press (the same as the repeated
            ACTION_TREE_PGLEFT) with the pre condition being the non-repeated
            button press */
            if (lists->gui_list[0].offset_position == 0)
            {
                scrolling_left = false;
                return ACTION_STD_CANCEL;
            }
        case ACTION_TREE_PGRIGHT:
            gui_synclist_scroll_right(lists);
            gui_synclist_draw(lists);
            return ACTION_TREE_PGRIGHT;
        case ACTION_TREE_PGLEFT:
            if(!scrolling_left && (lists->gui_list[0].offset_position == 0))
                return ACTION_STD_CANCEL;
            gui_synclist_scroll_left(lists);
            gui_synclist_draw(lists);
            scrolling_left = true; /* stop ACTION_TREE_PAGE_LEFT
                                      skipping to root */
            return ACTION_TREE_PGLEFT;
#endif

/* for pgup / pgdown, we are obliged to have a different behaviour depending
 * on the screen for which the user pressed the key since for example, remote
 * and main screen doesn't have the same number of lines */
        case ACTION_LISTTREE_PGUP:
        {
            int screen =
#ifdef HAVE_REMOTE_LCD
                get_action_statuscode(NULL)&ACTION_REMOTE ?
                         SCREEN_REMOTE : 
#endif
                                          SCREEN_MAIN;
            gui_synclist_select_previous_page(lists, screen);
            gui_synclist_draw(lists);
            yield();
        }
        return ACTION_STD_NEXT;

        case ACTION_LISTTREE_PGDOWN:
        {
            int screen =
#ifdef HAVE_REMOTE_LCD
                get_action_statuscode(NULL)&ACTION_REMOTE ?
                         SCREEN_REMOTE : 
#endif
                                          SCREEN_MAIN;
            gui_synclist_select_next_page(lists, screen);
            gui_synclist_draw(lists);
            yield();
        }
        return ACTION_STD_PREV;
    }
    return 0;
}
