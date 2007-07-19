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
 * Copyright (C) 2007 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "config.h"
#include "option_select.h"
#include "sprintf.h"
#include "kernel.h"
#include "lang.h"
#include "talk.h"
#include "settings_list.h"
#include "sound.h"
#include "list.h"
#include "action.h"
#include "statusbar.h"
#include "misc.h"
#include "splash.h"

static const char *unit_strings[] = 
{   
    [UNIT_INT] = "",    [UNIT_MS]  = "ms",
    [UNIT_SEC] = "s",   [UNIT_MIN] = "min", 
    [UNIT_HOUR]= "hr",  [UNIT_KHZ] = "KHz", 
    [UNIT_DB]  = "dB",  [UNIT_PERCENT] = "%",
    [UNIT_MAH] = "mAh", [UNIT_PIXEL] = "px",
    [UNIT_PER_SEC] = "per sec",
    [UNIT_HERTZ] = "Hz",
    [UNIT_MB]  = "MB",  [UNIT_KBIT]  = "kb/s",
};

char *option_get_valuestring(struct settings_list *setting, 
                             char *buffer, int buf_len,
                             intptr_t temp_var)
{
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        bool val = (bool)temp_var;
        snprintf(buffer, buf_len, "%s", 
            str(val? setting->bool_setting->lang_yes :
                     setting->bool_setting->lang_no));
    }
#if 0 /* probably dont need this one */
    else if ((setting->flags & F_FILENAME) == F_FILENAME)
    {
        struct filename_setting *info = setting->filename_setting;
        snprintf(buffer, buf_len, "%s%s%s", info->prefix,
                 (char*)temp_var, info->suffix);
    }
#endif
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = setting->int_setting;
        if (info->formatter)
            info->formatter(buffer, buf_len, (int)temp_var,
                            unit_strings[info->unit]);
        else
            snprintf(buffer, buf_len, "%d %s", (int)temp_var,
                     unit_strings[info->unit]?
                             unit_strings[info->unit]:"");
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        char sign = ' ', *unit;
        unit = (char*)sound_unit(setting->sound_setting->setting);
        if (sound_numdecimals(setting->sound_setting->setting))
        {
            int integer, dec;
            int val = sound_val2phys(setting->sound_setting->setting,
                                 (int)temp_var);
            if(val < 0)
            {
                sign = '-';
                val = abs(val);
            }
            integer = val / 10; dec = val % 10;
            snprintf(buffer, buf_len, "%c%d.%d %s", sign, integer, dec, unit);
        }
        else
            snprintf(buffer, buf_len, "%d %s", (int)temp_var, unit);
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        if (setting->flags & F_CHOICETALKS)
        {
            int setting_id;
            find_setting(setting->setting, &setting_id);
            cfg_int_to_string(setting_id, (int)temp_var, buffer, buf_len);
        }
        else
        {
            int value= (int)temp_var;
            char *val = P2STR(setting->choice_setting->desc[value]);
            snprintf(buffer, buf_len, "%s", val);
        }
    }
    return buffer;
}

void option_talk(struct settings_list *setting, int temp_var)
{
    if (!talk_menus_enabled())
        return;
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        bool val = temp_var==1?true:false;
        talk_id(val? setting->bool_setting->lang_yes :
                setting->bool_setting->lang_no, false);
    }
#if 0 /* probably dont need this one */
    else if ((setting->flags & F_FILENAME) == F_FILENAME)
    {
    }
#endif
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = setting->int_setting;
        if (info->get_talk_id)
            talk_id(info->get_talk_id((int)temp_var), false);
        else 
            talk_value((int)temp_var, info->unit, false);
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int talkunit = UNIT_DB;
        const char *unit = sound_unit(setting->sound_setting->setting);
        /* crude reconstruction */
        if (*unit == '%')
            talkunit = UNIT_PERCENT;
        else if (*unit == 'H')
            talkunit = UNIT_HERTZ;
        talk_value((int)temp_var, talkunit, false);
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        int value = (int)temp_var;
        if (setting->flags & F_CHOICETALKS)
        {
            talk_id(setting->choice_setting->talks[value], false);
        }
        else
        {
            talk_id(P2ID(setting->choice_setting->desc[value]), false);
        }
    }
}
#if 0
int option_select_next_val(struct settings_list *setting,
                           intptr_t temp_var)
{
    int val = 0;
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        val = (bool)temp_var ? 0 : 1;
    }
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = setting->int_setting;
        val = (int)temp_var + info->step;
        if (val > info->max)
            val = info->min;
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int setting_id = setting->sound_setting->setting;
        int steps = sound_steps(setting_id);
        int min = sound_min(setting_id);
        int max = sound_max(setting_id);
        val = (int)temp_var + steps;
        if (val > max)
            val = min;
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        struct choice_setting *info = setting->choice_setting;
        val = (int)temp_var;
        if (val > info->count)
            val = 0;
    }
    return val;
}

int option_select_prev_val(struct settings_list *setting,
                           intptr_t temp_var)
{
    int val = 0;
    if ((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING)
    {
        val = (bool)temp_var ? 0 : 1;
    }
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = setting->int_setting;
        val = (int)temp_var - info->step;
        if (val < info->min)
            val = info->max;
    }
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int setting_id = setting->sound_setting->setting;
        int steps = sound_steps(setting_id);
        int min = sound_min(setting_id);
        int max = sound_max(setting_id);
        val = (int)temp_var -+ steps;
        if (val < min)
            val = max;
    }
    else if ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING)
    {
        struct choice_setting *info = setting->choice_setting;
        val = (int)temp_var;
        if (val < 0)
            val = info->count - 1;
    }
    return val;
}
#endif

static int selection_to_val(struct settings_list *setting, int selection)
{
    int min = 0, max = 0, step = 1;
    if (((setting->flags & F_BOOL_SETTING) == F_BOOL_SETTING) ||
          ((setting->flags & F_CHOICE_SETTING) == F_CHOICE_SETTING))
        return selection;
    else if ((setting->flags & F_T_SOUND) == F_T_SOUND)
    {
        int setting_id = setting->sound_setting->setting;
        step = sound_steps(setting_id);
        max = sound_max(setting_id);
        min = sound_min(setting_id);
    }
    else if ((setting->flags & F_INT_SETTING) == F_INT_SETTING)
    {
        struct int_setting *info = setting->int_setting;
        min = info->min;
        max = info->max;
        step = info->step;
    }
    if (setting->flags & F_FLIPLIST)
    {
        int a;
        a = min; min = max; max = a;
        step = -step;
    }
    return max- (selection * step);
}
static char * value_setting_get_name_cb(int selected_item, 
                                        void * data, char *buffer)
{
    selected_item = selection_to_val(data, selected_item);
    return option_get_valuestring(data, buffer, MAX_PATH, selected_item);
}

/* wrapper to convert from int param to bool param in option_screen */
static void (*boolfunction)(bool);
static void bool_funcwrapper(int value)
{
    if (value)
        boolfunction(true);
    else
        boolfunction(false);
}

bool option_screen(struct settings_list *setting, bool use_temp_var)
{
    int action;
    bool done = false;
    struct gui_synclist lists;
    int oldvalue, nb_items = 0, selected = 0, temp_var;
    int *variable;
    bool allow_wrap = ((int*)setting->setting != &global_settings.volume);
    int var_type = setting->flags&F_T_MASK;
    void (*function)(int) = NULL;
    
    if (var_type == F_T_INT || var_type == F_T_UINT)
    {
        variable = use_temp_var ? &temp_var: (int*)setting->setting;
        temp_var = oldvalue = *(int*)setting->setting;
    }
    else if (var_type == F_T_BOOL)
    {
        /* bools always use the temp variable...
        if use_temp_var is false it will be copied to setting->setting every change */
        variable = &temp_var;
        temp_var = oldvalue = *(bool*)setting->setting?1:0;
    }
    else return false; /* only int/bools can go here */
    gui_synclist_init(&lists, value_setting_get_name_cb, 
                      (void*)setting, false, 1);
    if (setting->lang_id == -1)
        gui_synclist_set_title(&lists, 
                                (char*)setting->cfg_vals, Icon_Questionmark);
    else
        gui_synclist_set_title(&lists, 
                                str(setting->lang_id), Icon_Questionmark);
    gui_synclist_set_icon_callback(&lists, NULL);
    
    /* set the number of items and current selection */
    if (var_type == F_T_INT || var_type == F_T_UINT)
    {
        if (setting->flags&F_CHOICE_SETTING)
        {
            nb_items = setting->choice_setting->count;
            selected = oldvalue;
            function = setting->choice_setting->option_callback;
        }
        else if (setting->flags&F_T_SOUND)
        {
            int setting_id = setting->sound_setting->setting;
            int steps = sound_steps(setting_id);
            int min = sound_min(setting_id);
            int max = sound_max(setting_id);
            nb_items = (max-min)/steps + 1;
            selected = (max-oldvalue)/steps;
            function = sound_get_fn(setting_id);
        }
        else
        {
            struct int_setting *info = setting->int_setting;
            int min, max, step;
            if (setting->flags&F_FLIPLIST)
            {
                min = info->max;
                max = info->min;
                step = -info->step;
            }
            else
            {
                max = info->max;
                min = info->min;
                step = info->step;
            }
            nb_items = (max-min)/step + 1;
            selected = (max - oldvalue)/step;
            function = info->option_callback;
        }
    }
    else if (var_type == F_T_BOOL)
    {
        selected = oldvalue;
        nb_items = 2;
        boolfunction = setting->bool_setting->option_callback;
        if (boolfunction)
            function = bool_funcwrapper;
    }
    
    gui_synclist_set_nb_items(&lists, nb_items);
    gui_synclist_select_item(&lists, selected);
    
    gui_synclist_limit_scroll(&lists, true);
    gui_synclist_draw(&lists);
    action_signalscreenchange();
    /* talk the item */
    option_talk(setting, *variable);
    while (!done)
    {
        action = get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if (action == ACTION_NONE)
            continue;
        if (gui_synclist_do_button(&lists,action,
            allow_wrap? LIST_WRAP_UNLESS_HELD: LIST_WRAP_OFF))
        {
            selected = gui_synclist_get_sel_pos(&lists);
            *variable = selection_to_val(setting, selected);
            if (var_type == F_T_BOOL)
            {
                if (!use_temp_var)
                    *(bool*)setting->setting = selected==1?true:false;
            }
            /* talk */
            option_talk(setting, *variable);
        }
        else if (action == ACTION_STD_CANCEL)
        {
            bool show_cancel = false;
            if (use_temp_var)
                show_cancel = true;
            else if (var_type == F_T_INT || var_type == F_T_UINT)
            {
                if (*variable != oldvalue)
                {
                    show_cancel = true;
                    *variable = oldvalue;
                }
            }
            else
            {
                if (*variable != oldvalue)
                {
                    show_cancel = true;
                    if (!use_temp_var)
                        *(bool*)setting->setting = oldvalue==1?true:false;
                    *variable = oldvalue;
                }
            }
            if (show_cancel)
                gui_syncsplash(HZ/2, str(LANG_MENU_SETTING_CANCEL));
            done = true;
        }
        else if (action == ACTION_STD_OK)
        {
            done = true;
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            return true;
        gui_syncstatusbar_draw(&statusbars, false);
        /* callback */
        if ( function )
            function(*variable);
    }
    
    if (oldvalue != *variable)
    {
        if (use_temp_var)
        {
            if (var_type == F_T_INT || var_type == F_T_UINT)
                *(int*)setting->setting = *variable;
            else 
                *(bool*)setting->setting = *variable?true:false;
        }
        settings_save();
    }
    
    action_signalscreenchange();
    return false;
}

/******************************************************
    Compatability functions 
*******************************************************/
#define MAX_OPTIONS 32
bool set_option(const char* string, void* variable, enum optiontype type,
                const struct opt_items* options, 
                int numoptions, void (*function)(int))
{
    int temp;
    char *strings[MAX_OPTIONS];
    struct choice_setting data;
    struct settings_list item;
    for (temp=0; temp<MAX_OPTIONS && temp<numoptions; temp++)
        strings[temp] = (char*)options[temp].string;
    if (type == BOOL)
    {
        temp = *(bool*)variable? 1: 0;
        item.setting = &temp;
    }
    else 
        item.setting = variable;
    item.flags = F_CHOICE_SETTING|F_T_INT;
    item.lang_id = -1;
    item.cfg_vals = (char*)string;
    data.count = numoptions<MAX_OPTIONS ? numoptions: MAX_OPTIONS;
    data.desc = (void*)strings; /* shutup gcc... */
    data.option_callback = function;
    item.choice_setting = &data;
    option_screen(&item, false);
    if (type == BOOL)
    {
        *(bool*)variable = (temp == 1? true: false);
    }
    return false;
}

bool set_int_ex(const unsigned char* string,
                const char* unit,
                int voice_unit,
                int* variable,
                void (*function)(int),
                int step,
                int min,
                int max,
                void (*formatter)(char*, int, int, const char*),
                long (*get_talk_id)(int))
{
    (void)unit;
    struct settings_list item;
    struct int_setting data = {
        function, voice_unit, min, max, step, 
        formatter, get_talk_id
    };
    item.int_setting = &data;
    item.flags = F_INT_SETTING|F_T_INT;
    item.lang_id = -1;
    item.cfg_vals = (char*)string;
    item.setting = variable;
    return option_screen(&item, false);
}

/* to be replaced */
void option_select_init_items(struct option_select * opt,
                              const char * title,
                              int selected,
                              const struct opt_items * items,
                              int nb_items)
{
    opt->title=title;
    opt->min_value=0;
    opt->max_value=nb_items;
    opt->option=selected;
    opt->items=items;
}

void option_select_next(struct option_select * opt)
{
    if(opt->option + 1 >= opt->max_value)
    {
            if(opt->option==opt->max_value-1)
                opt->option=opt->min_value;
            else
                opt->option=opt->max_value-1;
    }
    else
        opt->option+=1;
}

void option_select_prev(struct option_select * opt)
{
    if(opt->option - 1 < opt->min_value)
    {
        /* the dissimilarity to option_select_next() arises from the 
         * sleep timer problem (bug #5000 and #5001): 
         * there we have min=0, step = 5 but the value itself might
         * not be a multiple of 5 -- as time elapsed;
         * We need to be able to set timer to 0 (= Off) nevertheless. */
        if(opt->option!=opt->min_value)
            opt->option=opt->min_value;
        else
            opt->option=opt->max_value-1;
    }
    else
        opt->option-=1;
}

const char * option_select_get_text(struct option_select * opt)
{
        return(P2STR(opt->items[opt->option].string));
}
