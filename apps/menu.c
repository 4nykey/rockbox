/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
2005 Kevin Ferrare :
 - Multi screen support
 - Rewrote/removed a lot of code now useless with the new gui API
*/
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"

#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "usb.h"
#include "panic.h"
#include "settings.h"
#include "settings_list.h"
#include "status.h"
#include "screens.h"
#include "talk.h"
#include "lang.h"
#include "misc.h"
#include "action.h"
#include "menus/exported_menus.h"
#include "string.h"
#include "root_menu.h"
#include "bookmark.h"
#include "gwps-common.h" /* for fade() */
#include "audio.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "buttonbar.h"

#define MAX_MENUS 8
/* used to allow for dynamic menus */
#define MAX_MENU_SUBITEMS 64
static struct menu_item_ex *current_submenus_menu;
static int current_subitems[MAX_MENU_SUBITEMS];
static int current_subitems_count = 0;

static void get_menu_callback(const struct menu_item_ex *m,
                        menu_callback_type *menu_callback) 
{
    if (m->flags&(MENU_HAS_DESC|MENU_DYNAMIC_DESC))
        *menu_callback= m->callback_and_desc->menu_callback;
    else 
        *menu_callback = m->menu_callback;
}

static int get_menu_selection(int selected_item, const struct menu_item_ex *menu)
{
    int type = (menu->flags&MENU_TYPE_MASK);
    if (type == MT_MENU && (selected_item<current_subitems_count))
        return current_subitems[selected_item];
    return selected_item;
}
static int find_menu_selection(int selected)
{
    int i;
    for (i=0; i< current_subitems_count; i++)
        if (current_subitems[i] == selected)
            return i;
    return 0;
}
static char * get_menu_item_name(int selected_item,void * data, char *buffer)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex *)data;
    int type = (menu->flags&MENU_TYPE_MASK);
    selected_item = get_menu_selection(selected_item, menu);
    
    (void)buffer;
    /* only MT_MENU or MT_RETURN_ID is allowed in here */
    if (type == MT_RETURN_ID)
    {
        if (menu->flags&MENU_DYNAMIC_DESC)
            return menu->menu_get_name_and_icon->list_get_name(selected_item,
                    menu->menu_get_name_and_icon->list_get_name_data, buffer);
        return (char*)menu->strings[selected_item];
    }
    
    menu = menu->submenus[selected_item];
    
    if ((menu->flags&MENU_DYNAMIC_DESC) && (type != MT_SETTING_W_TEXT))
        return menu->menu_get_name_and_icon->list_get_name(selected_item,
                    menu->menu_get_name_and_icon->list_get_name_data, buffer);
    
    type = (menu->flags&MENU_TYPE_MASK);
    if ((type == MT_SETTING) || (type == MT_SETTING_W_TEXT))
    {
        const struct settings_list *v
                = find_setting(menu->variable, NULL);
        if (v)
            return str(v->lang_id);
        else return "Not Done yet!";
    }
    return P2STR(menu->callback_and_desc->desc);
}
#ifdef HAVE_LCD_BITMAP
static int menu_get_icon(int selected_item, void * data)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex *)data;
    int menu_icon = Icon_NOICON;
    selected_item = get_menu_selection(selected_item, menu);
    
    if ((menu->flags&MENU_TYPE_MASK) == MT_RETURN_ID)
    {
        return Icon_Menu_functioncall;
    }
    menu = menu->submenus[selected_item];
    if (menu->flags&MENU_HAS_DESC)
        menu_icon = menu->callback_and_desc->icon_id;
    else if (menu->flags&MENU_DYNAMIC_DESC)
        menu_icon = menu->menu_get_name_and_icon->icon_id;
    
    if (menu_icon == Icon_NOICON)
    {
        switch (menu->flags&MENU_TYPE_MASK)
        {
            case MT_SETTING:
            case MT_SETTING_W_TEXT:
                menu_icon = Icon_Menu_setting;
                break;
            case MT_MENU:
                    menu_icon = Icon_Submenu;
                break;
            case MT_FUNCTION_CALL:
            case MT_RETURN_VALUE:
                menu_icon = Icon_Menu_functioncall;
                break;
        }
    }
    return menu_icon;
}
#endif

static void init_menu_lists(const struct menu_item_ex *menu,
                     struct gui_synclist *lists, int selected, bool callback)
{
    int i, count = MENU_GET_COUNT(menu->flags);
    int type = (menu->flags&MENU_TYPE_MASK);
    menu_callback_type menu_callback = NULL;
    int icon;
    current_subitems_count = 0;

    if (type == MT_RETURN_ID)
        get_menu_callback(menu, &menu_callback);

    for (i=0; i<count; i++)
    {
        if (type != MT_RETURN_ID)
            get_menu_callback(menu->submenus[i],&menu_callback);
        if (menu_callback)
        {
            if (menu_callback(ACTION_REQUEST_MENUITEM,
                    type==MT_RETURN_ID ? (void*)(intptr_t)i: menu->submenus[i])
                    != ACTION_EXIT_MENUITEM)
            {
                current_subitems[current_subitems_count] = i;
                current_subitems_count++;
            }
        }
        else 
        {
            current_subitems[current_subitems_count] = i;
            current_subitems_count++;
        }
    }
    current_submenus_menu = (struct menu_item_ex *)menu;

    gui_synclist_init(lists,get_menu_item_name,(void*)menu,false,1);
#ifdef HAVE_LCD_BITMAP
    if (menu->callback_and_desc->icon_id == Icon_NOICON)
        icon = Icon_Submenu_Entered;
    else
        icon = menu->callback_and_desc->icon_id;
    gui_synclist_set_title(lists, P2STR(menu->callback_and_desc->desc), icon);  
    gui_synclist_set_icon_callback(lists, menu_get_icon);
#else
    (void)icon;
    gui_synclist_set_icon_callback(lists, NULL);
#endif
    gui_synclist_set_nb_items(lists,current_subitems_count);
    gui_synclist_limit_scroll(lists,true);
    gui_synclist_select_item(lists, find_menu_selection(selected));
    
    get_menu_callback(menu,&menu_callback);
    if (callback && menu_callback)
        menu_callback(ACTION_ENTER_MENUITEM,menu);
    gui_synclist_draw(lists);
}

static void talk_menu_item(const struct menu_item_ex *menu,
                    struct gui_synclist *lists)
{
    int id = -1;
    int type;
    unsigned char *str;
    int sel;
    
    if (talk_menus_enabled())
    {
        sel = get_menu_selection(gui_synclist_get_sel_pos(lists),menu);
        if ((menu->flags&MENU_TYPE_MASK) == MT_MENU)
        {
            type = menu->submenus[sel]->flags&MENU_TYPE_MASK;
            if ((type == MT_SETTING) || (type == MT_SETTING_W_TEXT))
                talk_setting(menu->submenus[sel]->variable);
            else 
            {
                if (menu->submenus[sel]->flags&(MENU_DYNAMIC_DESC))
                {
                    char buffer[80];
                    str = menu->submenus[sel]->menu_get_name_and_icon->
                        list_get_name(sel, menu->submenus[sel]->
                                      menu_get_name_and_icon->
                                      list_get_name_data, buffer);
                    id = P2ID(str);
                }
                else
                    id = P2ID(menu->submenus[sel]->callback_and_desc->desc);
                if (id != -1)
                   talk_id(id,false);
            }
        }
    }
}
#define MAX_OPTIONS 32
/* returns true if the menu needs to be redrwan */
bool do_setting_from_menu(const struct menu_item_ex *temp)
{
    int setting_id;
    const struct settings_list *setting = find_setting(
                                               temp->variable,
                                               &setting_id);
    bool ret_val = false;
    unsigned char *title;
    if (setting)
    {
        if ((temp->flags&MENU_TYPE_MASK) == MT_SETTING_W_TEXT)
            title = temp->callback_and_desc->desc;
        else
            title = ID2P(setting->lang_id);

        if ((setting->flags&F_BOOL_SETTING) == F_BOOL_SETTING)
        {
            bool temp_var, *var;
            bool show_icons = global_settings.show_icons;
            if (setting->flags&F_TEMPVAR)
            {
                temp_var = *(bool*)setting->setting;
                var = &temp_var;
            }
            else
            {
                var = (bool*)setting->setting;
            }
            set_bool_options(P2STR(title), var,
                        STR(setting->bool_setting->lang_yes),
                        STR(setting->bool_setting->lang_no),
                        setting->bool_setting->option_callback);
            if (setting->flags&F_TEMPVAR)
                *(bool*)setting->setting = temp_var;
            if (show_icons != global_settings.show_icons)
                ret_val = true;
        }
        else if (setting->flags&F_T_SOUND)
        {
            set_sound(P2STR(title), setting->setting,
                        setting->sound_setting->setting);
        }
        else /* other setting, must be an INT type */
        {
            int temp_var, *var;
            if (setting->flags&F_TEMPVAR)
            {
                temp_var = *(int*)setting->setting;
                var = &temp_var;
            }
            else
            {
                var = (int*)setting->setting;
            }
            if (setting->flags&F_INT_SETTING)
            {
                int min, max, step;
                if (setting->flags&F_FLIPLIST)
                {
                    min = setting->int_setting->max;
                    max = setting->int_setting->min;
                    step = -setting->int_setting->step;
                }
                else
                {
                    max = setting->int_setting->max;
                    min = setting->int_setting->min;
                    step = setting->int_setting->step;
                }
                set_int_ex(P2STR(title), NULL,
                        setting->int_setting->unit,var,
                        setting->int_setting->option_callback,
                        step, min, max,
                        setting->int_setting->formatter,
                        setting->int_setting->get_talk_id);
            }
            else if (setting->flags&F_CHOICE_SETTING)
            {
                static struct opt_items options[MAX_OPTIONS];
                char buffer[256];
                char *buf_start = buffer;
                int buf_free = 256;
                int i,j, count = setting->choice_setting->count;
                for (i=0, j=0; i<count && i<MAX_OPTIONS; i++)
                {
                    if (setting->flags&F_CHOICETALKS)
                    {
                        if (cfg_int_to_string(setting_id, i,
                                            buf_start, buf_free))
                        {
                            int len = strlen(buf_start) +1;
                            options[j].string = buf_start;
                            buf_start += len;
                            buf_free -= len;
                            options[j].voice_id = 
                                setting->choice_setting->talks[i];
                            j++;
                        }
                    }
                    else
                    {
                        options[j].string =
                            P2STR(setting->
                                  choice_setting->desc[i]);
                        options[j].voice_id = 
                            P2ID(setting->
                                  choice_setting->desc[i]);
                        j++;
                    }
                }
                set_option(P2STR(title), var, INT,
                            options,j,
                            setting->
                               choice_setting->option_callback);
            }
            if (setting->flags&F_TEMPVAR)
                *(int*)setting->setting = temp_var;
        }
    }
    return ret_val;
}

int do_menu(const struct menu_item_ex *start_menu, int *start_selected)
{
    int selected = start_selected? *start_selected : 0;
    int action;
    struct gui_synclist lists;
    const struct menu_item_ex *temp, *menu;
    int ret = 0;
    bool redraw_lists;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif

    const struct menu_item_ex *menu_stack[MAX_MENUS];
    int menu_stack_selected_item[MAX_MENUS];
    int stack_top = 0;
    bool in_stringlist, done = false;
    menu_callback_type menu_callback = NULL;
    bool talk_item = false;
    if (start_menu == NULL)
        menu = &main_menu_;
    else menu = start_menu;
#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&buttonbar, "<<<", "", "");
    gui_buttonbar_draw(&buttonbar);
#endif
    init_menu_lists(menu,&lists,selected,true);
    in_stringlist = ((menu->flags&MENU_TYPE_MASK) == MT_RETURN_ID);
    
    talk_menu_item(menu, &lists);
    
    action_signalscreenchange();
    
    /* load the callback, and only reload it if menu changes */
    get_menu_callback(menu, &menu_callback);
    gui_synclist_draw(&lists);
    
    while (!done)
    {
        talk_item = false;
        redraw_lists = false;
        gui_syncstatusbar_draw(&statusbars, true);
        action = get_action(CONTEXT_MAINMENU,HZ); 
        /* HZ so the status bar redraws corectly */
        if (action == ACTION_NONE)
        {
            continue;
        }
        
        if (menu_callback)
        {
            int old_action = action;
            action = menu_callback(action, menu);
            if (action == ACTION_EXIT_AFTER_THIS_MENUITEM)
            {
                action = old_action;
                ret = MENU_SELECTED_EXIT; /* will exit after returning
                                             from selection */
            }
            else if (action == ACTION_REDRAW)
            {
                action = old_action;
                redraw_lists = true;
            }
        }

        if (gui_synclist_do_button(&lists,action,LIST_WRAP_UNLESS_HELD))
        {
            talk_menu_item(menu, &lists);
            continue;
        }
        
        if (action == ACTION_TREE_WPS)
        {
            ret = GO_TO_PREVIOUS_MUSIC;
            done = true;
        }
        else if (action == ACTION_TREE_STOP)
        {
            list_stop_handler();
        }
        else if (action == ACTION_STD_CONTEXT &&
                 menu == &root_menu_)
        {
            ret = GO_TO_ROOTITEM_CONTEXT;
            done = true;
        } 
        else if (action == ACTION_STD_MENU)
        {
            if (menu != &root_menu_)
                ret = GO_TO_ROOT;
            else
                ret = GO_TO_PREVIOUS;
            done = true;
        }
        else if (action == ACTION_STD_CANCEL)
        {
            in_stringlist = false;
            if (menu_callback)
                menu_callback(ACTION_EXIT_MENUITEM, menu);
            
            if (menu->flags&MENU_EXITAFTERTHISMENU)
                done = true;
            if (stack_top > 0)
            {
                stack_top--;
                menu = menu_stack[stack_top];
                if (menu->flags&MENU_EXITAFTERTHISMENU)
                    done = true;
                init_menu_lists(menu, &lists, 
                                 menu_stack_selected_item[stack_top], false);
                /* new menu, so reload the callback */
                get_menu_callback(menu, &menu_callback);
                talk_item = true;
            }
            else if (menu != &root_menu_)
            {
                ret = GO_TO_PREVIOUS;
                done = true;
            }
        }
        else if (action == ACTION_STD_OK)
        {
            int type;
#ifdef HAS_BUTTONBAR
            gui_buttonbar_unset(&buttonbar);
            gui_buttonbar_draw(&buttonbar);
#endif
            selected = get_menu_selection(gui_synclist_get_sel_pos(&lists), menu);
            temp = menu->submenus[selected];
            redraw_lists = true;
            if (in_stringlist)
                type = (menu->flags&MENU_TYPE_MASK);
            else 
            {
                type = (temp->flags&MENU_TYPE_MASK);
                get_menu_callback(temp, &menu_callback);
                if (menu_callback)
                {
                    action = menu_callback(ACTION_ENTER_MENUITEM,temp);
                    if (action == ACTION_EXIT_MENUITEM)
                        break;
                }
            }
            switch (type)
            {
                case MT_MENU:
                    if (stack_top < MAX_MENUS)
                    {
                        menu_stack[stack_top] = menu;
                        menu_stack_selected_item[stack_top] = selected;
                        stack_top++;
                        init_menu_lists(temp, &lists, 0, true);
                        redraw_lists = false; /* above does the redraw */
                        menu = temp;
                        talk_item = true;
                    }
                    break;
                case MT_FUNCTION_CALL:
                {
                    int return_value;
                    talk_item = true;
                    action_signalscreenchange();
                    if (temp->flags&MENU_FUNC_USEPARAM)
                        return_value = temp->function->function_w_param(
                                    temp->function->param);
                    else 
                        return_value = temp->function->function();
                    if (temp->flags&MENU_FUNC_CHECK_RETVAL)
                    {
                        if (return_value == 1)
                        {
                            done = true;
                            ret =  return_value;
                        }
                    }
                    break;
                }
                case MT_SETTING:
                case MT_SETTING_W_TEXT:
                {
                    if (do_setting_from_menu(temp))
                    {
                        init_menu_lists(menu, &lists, selected, true);
                        redraw_lists = false; /* above does the redraw */
                    }
                    talk_item = true;
                    break;
                }
                case MT_RETURN_ID:
                    if (in_stringlist)
                    {
                        action_signalscreenchange();
                        done = true;
                        ret =  selected;
                    }
                    else if (stack_top < MAX_MENUS)
                    {
                        menu_stack[stack_top] = menu;
                        menu_stack_selected_item[stack_top] = selected;
                        stack_top++;
                        menu = temp;
                        init_menu_lists(menu,&lists,0,false);
                        redraw_lists = false; /* above does the redraw */
                        talk_item = true;
                        in_stringlist = true;
                    }
                    break;
                case MT_RETURN_VALUE:
                    ret = temp->value;
                    done = true;
                    break;
            }
            if (type != MT_MENU)
            {
                if (menu_callback)
                    menu_callback(ACTION_EXIT_MENUITEM,temp);
            }
            if (current_submenus_menu != menu)
                init_menu_lists(menu,&lists,selected,true);
            /* callback was changed, so reload the menu's callback */
            get_menu_callback(menu, &menu_callback);
            if ((menu->flags&MENU_EXITAFTERTHISMENU) && 
                !(temp->flags&MENU_EXITAFTERTHISMENU))
            {
                done = true;
                break;
            }
#ifdef HAS_BUTTONBAR
            gui_buttonbar_set(&buttonbar, "<<<", "", "");
            gui_buttonbar_draw(&buttonbar);
#endif
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
        {
            ret = MENU_ATTACHED_USB;
            done = true;
        }
        if (talk_item && !done)
            talk_menu_item(menu, &lists);
        
        if (redraw_lists)
            gui_synclist_draw(&lists);
    }
    action_signalscreenchange();
    if (start_selected)
    {
        /* make sure the start_selected variable is set to
           the selected item from the menu do_menu() was called from */
        if (stack_top > 0)
        {
            menu = menu_stack[0];
            init_menu_lists(menu,&lists,menu_stack_selected_item[0],true);
        }
        *start_selected = get_menu_selection(
                            gui_synclist_get_sel_pos(&lists), menu);
    }
    return ret;
}

