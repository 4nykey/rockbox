/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin, Dan Everton, Matthias Mohr
 *               2010 Jonathan Gordon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file.h"
#include "misc.h"
#include "plugin.h"
#include "viewport.h"

#include "skin_buffer.h"
#include "skin_parser.h"
#include "tag_table.h"

#ifdef __PCTOOL__
#ifdef WPSEDITOR
#include "proxy.h"
#include "sysfont.h"
#else
#include "action.h"
#include "checkwps.h"
#include "audio.h"
#define lang_is_rtl() (false)
#define DEBUGF printf
#endif /*WPSEDITOR*/
#else
#include "debug.h"
#include "language.h"
#endif /*__PCTOOL__*/

#include <ctype.h>
#include <stdbool.h>
#include "font.h"

#include "wps_internals.h"
#include "skin_engine.h"
#include "settings.h"
#include "settings_list.h"
#if CONFIG_TUNER
#include "radio.h"
#include "tuner.h"
#endif
#include "skin_fonts.h"

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#endif

#ifdef HAVE_ALBUMART
#include "playback.h"
#endif

#include "backdrop.h"
#include "statusbar-skinned.h"

#define WPS_ERROR_INVALID_PARAM         -1


static bool isdefault(struct skin_tag_parameter *param)
{
    return param->type == DEFAULT;
}


/* which screen are we parsing for? */
static enum screen_type curr_screen;

/* the current viewport */
static struct skin_element *curr_viewport_element;
static struct skin_viewport *curr_vp;

static struct line *curr_line;

static int follow_lang_direction = 0;

typedef int (*parse_function)(struct skin_element *element,
                              struct wps_token *token,
                              struct wps_data *wps_data);

#ifdef HAVE_LCD_BITMAP
/* add a skin_token_list item to the list chain. ALWAYS appended because some of the
 * chains require the order to be kept.
 */
static void add_to_ll_chain(struct skin_token_list **list, struct skin_token_list *item)
{
    if (*list == NULL)
        *list = item;
    else
    {
        struct skin_token_list *t = *list;
        while (t->next)
            t = t->next;
        t->next = item;
    }
}

/* traverse the image linked-list for an image */
struct gui_img* find_image(char label, struct wps_data *data)
{
    struct skin_token_list *list = data->images;
    while (list)
    {
        struct gui_img *img = (struct gui_img *)list->token->value.data;
        if (img->label == label)
            return img;
        list = list->next;
    }
    return NULL;
}

#endif

/* traverse the viewport linked list for a viewport */
struct skin_viewport* find_viewport(char label, struct wps_data *data)
{
    struct skin_element *list = data->tree;
    while (list)
    {
        struct skin_viewport *vp = (struct skin_viewport *)list->data;
        if (vp->label == label)
            return vp;
        list = list->next;
    }
    return NULL;
}

#ifdef HAVE_LCD_BITMAP

/* create and init a new wpsll item.
 * passing NULL to token will alloc a new one.
 * You should only pass NULL for the token when the token type (table above)
 * is WPS_NO_TOKEN which means it is not stored automatically in the skins token array
 */
static struct skin_token_list *new_skin_token_list_item(struct wps_token *token,
                                                        void* token_data)
{
    struct skin_token_list *llitem = 
        (struct skin_token_list *)skin_buffer_alloc(sizeof(struct skin_token_list));
    if (!token)
        token = (struct wps_token*)skin_buffer_alloc(sizeof(struct wps_token));
    if (!llitem || !token)
        return NULL;
    llitem->next = NULL;
    llitem->token = token;
    if (token_data)
        llitem->token->value.data = token_data;
    return llitem;
}

static int parse_statusbar_tags(struct skin_element* element,
                                struct wps_token *token,
                                struct wps_data *wps_data)
{
    (void)element;
    if (token->type == SKIN_TOKEN_DRAW_INBUILTBAR)
    {
        token->value.data = (void*)&curr_vp->vp;
    }
    else
    {
        struct skin_element *def_vp = wps_data->tree;
        struct skin_viewport *default_vp = def_vp->data;
        if (def_vp->params_count == 0)
        {
            wps_data->wps_sb_tag = true;
            wps_data->show_sb_on_wps = (token->type == SKIN_TOKEN_ENABLE_THEME);
        }
        if (wps_data->show_sb_on_wps)
        {
            viewport_set_defaults(&default_vp->vp, curr_screen);
        }
        else
        {
            viewport_set_fullscreen(&default_vp->vp, curr_screen);
        }
    }
    return 0;
}
            
static int get_image_id(int c)
{
    if(c >= 'a' && c <= 'z')
        return c - 'a';
    else if(c >= 'A' && c <= 'Z')
        return c - 'A' + 26;
    else
        return -1;
}

char *get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size)
{
    snprintf(buf, buf_size, "%s/%s", bmpdir, start);
    
    return buf;
}

static int parse_image_display(struct skin_element *element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    char *text = element->params[0].data.text;
    char label = text[0];
    char sublabel = text[1];
    int subimage;
    struct gui_img *img;

    /* sanity check */
    img = find_image(label, wps_data);
    if (!img)
    {
        token->value.i = label; /* so debug works */
        return WPS_ERROR_INVALID_PARAM;
    }

    if ((subimage = get_image_id(sublabel)) != -1)
    {
        if (subimage >= img->num_subimages)
            return WPS_ERROR_INVALID_PARAM;

        /* Store sub-image number to display in high bits */
        token->value.i = label | (subimage << 8);
        return 4; /* We have consumed 2 bytes */
    } else {
        token->value.i = label;
        return 3; /* We have consumed 1 byte */
    }
}

static int parse_image_load(struct skin_element *element,
                            struct wps_token *token,
                            struct wps_data *wps_data)
{
    const char* filename;
    const char* id;
    int x,y;
    struct gui_img *img;

    /* format: %x(n,filename.bmp,x,y)
       or %xl(n,filename.bmp,x,y)
       or %xl(n,filename.bmp,x,y,num_subimages)
    */

    id = element->params[0].data.text;
    filename = element->params[1].data.text;
    x = element->params[2].data.number;
    y = element->params[3].data.number;

    /* check the image number and load state */
    if(find_image(*id, wps_data))
    {
        /* Invalid image ID */
        return WPS_ERROR_INVALID_PARAM;
    }
    img = (struct gui_img*)skin_buffer_alloc(sizeof(struct gui_img));
    if (!img)
        return WPS_ERROR_INVALID_PARAM;
    /* save a pointer to the filename */
    img->bm.data = (char*)filename;
    img->label = *id;
    img->x = x;
    img->y = y;
    img->num_subimages = 1;
    img->always_display = false;
 //   img->just_drawn = false;
    img->display = -1;

    /* save current viewport */
    img->vp = &curr_vp->vp;

    if (token->type == SKIN_TOKEN_IMAGE_DISPLAY)
    {
        img->always_display = true;
    }
    else if (element->params_count == 5)
    {
        img->num_subimages = element->params[4].data.number;
        if (img->num_subimages <= 0)
            return WPS_ERROR_INVALID_PARAM;
    }
    struct skin_token_list *item = 
            (struct skin_token_list *)new_skin_token_list_item(NULL, img);
    if (!item)
        return WPS_ERROR_INVALID_PARAM;
    add_to_ll_chain(&wps_data->images, item);

    return 0;
}
struct skin_font {
    int id; /* the id from font_load */
    char *name;  /* filename without path and extension */
};
static struct skin_font skinfonts[MAXUSERFONTS];
static int parse_font_load(struct skin_element *element,
                           struct wps_token *token,
                           struct wps_data *wps_data)
{
    (void)wps_data; (void)token;
    int id = element->params[0].data.number;
    char *filename = element->params[1].data.text;
    char *ptr;
    
#if defined(DEBUG) || defined(SIMULATOR)
    if (skinfonts[id-FONT_FIRSTUSERFONT].name != NULL)
    {
        DEBUGF("font id %d already being used\n", id);
    }
#endif
    /* make sure the filename contains .fnt, 
     * we dont actually use it, but require it anyway */
    ptr = strchr(filename, '.');
    if (!ptr || strncmp(ptr, ".fnt", 4))
        return WPS_ERROR_INVALID_PARAM;
    skinfonts[id-FONT_FIRSTUSERFONT].id = -1;
    skinfonts[id-FONT_FIRSTUSERFONT].name = filename;

    return 0;
}


#ifdef HAVE_LCD_BITMAP

static int parse_playlistview(struct skin_element *element,
                              struct wps_token *token,
                              struct wps_data *wps_data)
{
    (void)wps_data;
    struct playlistviewer *viewer = 
        (struct playlistviewer *)skin_buffer_alloc(sizeof(struct playlistviewer));
    if (!viewer)
        return WPS_ERROR_INVALID_PARAM;
    viewer->vp = &curr_vp->vp;
    viewer->show_icons = true;
    viewer->start_offset = element->params[0].data.number;
    viewer->lines[0] = element->params[1].data.code;
    viewer->lines[1] = element->params[2].data.code;
    
    token->value.data = (void*)viewer;
    
    return 0;
}
#endif

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))

static int parse_viewportcolour(struct skin_element *element,
                                struct wps_token *token,
                                struct wps_data *wps_data)
{
    (void)wps_data;
    struct skin_tag_parameter *param = element->params;
    struct viewport_colour *colour = 
        (struct viewport_colour *)skin_buffer_alloc(sizeof(struct viewport_colour));
    if (!colour)
        return -1;
    if (isdefault(param))
    {
        colour->colour = get_viewport_default_colour(curr_screen,
                                   token->type == SKIN_TOKEN_VIEWPORT_FGCOLOUR);
    }
    else
    {
        if (!parse_color(param->data.text, &colour->colour))
            return -1;
    }
    colour->vp = &curr_vp->vp;
    token->value.data = colour;
    if (element->line == curr_viewport_element->line)
    {
        if (token->type == SKIN_TOKEN_VIEWPORT_FGCOLOUR)
        {
            curr_vp->start_fgcolour = colour->colour;
            curr_vp->vp.fg_pattern = colour->colour;
        }
        else
        {
            curr_vp->start_bgcolour = colour->colour;
            curr_vp->vp.bg_pattern = colour->colour;
        }
    }
    return 0;
}

static int parse_image_special(struct skin_element *element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    (void)wps_data; /* kill warning */
    (void)token;
    bool error = false;

#if LCD_DEPTH > 1
    if (token->type == SKIN_TOKEN_IMAGE_BACKDROP)
    {
        char *filename = element->params[0].data.text;
        /* format: %X|filename.bmp| or %Xd */
        if (!strcmp(filename, "d"))
        {
            wps_data->backdrop = NULL;
            return 0;
        }
        else if (!error)
        {
            wps_data->backdrop = filename;
        }
    }
#endif
    /* Skip the rest of the line */
    return error ? WPS_ERROR_INVALID_PARAM : 0;
}
#endif

#endif /* HAVE_LCD_BITMAP */

static int parse_setting_and_lang(struct skin_element *element,
                                  struct wps_token *token,
                                  struct wps_data *wps_data)
{
    /* NOTE: both the string validations that happen in here will
     * automatically PASS on checkwps because its too hard to get
     * settings_list.c and english.lang built for it. 
     * If that ever changes remove the #ifndef __PCTOOL__'s here 
     */
    (void)wps_data;
    char *temp = element->params[0].data.text;
    int i;
    
    if (token->type == SKIN_TOKEN_TRANSLATEDSTRING)
    {
#ifndef __PCTOOL__
        i = lang_english_to_id(temp);
        if (i < 0)
            return WPS_ERROR_INVALID_PARAM;
#endif
    }
    else
    {
        /* Find the setting */
        for (i=0; i<nb_settings; i++)
            if (settings[i].cfg_name &&
                !strcmp(settings[i].cfg_name, temp))
                break;
#ifndef __PCTOOL__
        if (i == nb_settings)
            return WPS_ERROR_INVALID_PARAM;
#endif
    }
    /* Store the setting number */
    token->value.i = i;
    return 0;
}

static int parse_timeout_tag(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    (void)wps_data;
    int val = 0;
    if (element->params_count == 0)
    {
        switch (token->type)
        {
            case SKIN_TOKEN_SUBLINE_TIMEOUT:
                return -1;
            case SKIN_TOKEN_BUTTON_VOLUME:
            case SKIN_TOKEN_TRACK_STARTING:
            case SKIN_TOKEN_TRACK_ENDING:
            case SKIN_TOKEN_LASTTOUCH:
                val = 10;
                break;
            default:
                break;
        }
    }
    else
        val = element->params[0].data.number;
    token->value.i = val;
    if (token->type == SKIN_TOKEN_SUBLINE_TIMEOUT)
        curr_line->timeout = val * TIMEOUT_UNIT;
    return 0;
}

static int parse_progressbar_tag(struct skin_element* element,
                                 struct wps_token *token,
                                 struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP
    struct progressbar *pb;
    struct skin_token_list *item;
    struct viewport *vp = &curr_vp->vp;
    struct skin_tag_parameter *param = element->params;
    
    if (element->params_count == 0 && 
        element->tag->type != SKIN_TOKEN_PROGRESSBAR)
        return 0; /* nothing to do */
    pb = (struct progressbar*)skin_buffer_alloc(sizeof(struct progressbar));
    
    token->value.data = pb;
    
    if (!pb)
        return WPS_ERROR_INVALID_PARAM;
    pb->vp = vp;
    pb->have_bitmap_pb = false;
    pb->bm.data = NULL; /* no bitmap specified */
    pb->follow_lang_direction = follow_lang_direction > 0;
    
    if (element->params_count == 0)
    {
        pb->x = 0;
        pb->width = vp->width;
        pb->height = SYSFONT_HEIGHT-2;
        pb->y = -1; /* Will be computed during the rendering */
        pb->type = element->tag->type;
        return 0;
    }
    
    item = new_skin_token_list_item(token, pb);
    if (!item)
        return -1;
    add_to_ll_chain(&wps_data->progressbars, item);
    
    /* (x,y,width,height,filename) */
    if (!isdefault(param))
        pb->x = param->data.number;
    else
        pb->x = vp->x;
    param++;
    
    if (!isdefault(param))
        pb->y = param->data.number;
    else
        pb->y = -1; /* computed at rendering */
    param++;
    
    if (!isdefault(param))
        pb->width = param->data.number;
    else
        pb->width = vp->width - pb->x;
    param++;
    
    if (!isdefault(param))
    {
        /* A zero height makes no sense - reject it */
        if (param->data.number == 0)
            return WPS_ERROR_INVALID_PARAM;

        pb->height = param->data.number;
    }
    else
    {
        if (vp->font > FONT_UI)
            pb->height = -1; /* calculate at display time */
        else
        {
#ifndef __PCTOOL__
            pb->height = font_get(vp->font)->height;
#else
            pb->height = 8;
#endif
        }
    }
    param++;
    if (!isdefault(param))
        pb->bm.data = param->data.text;
        
        
    if (token->type == SKIN_TOKEN_VOLUME)
        token->type = SKIN_TOKEN_VOLUMEBAR;
    else if (token->type == SKIN_TOKEN_BATTERY_PERCENT)
        token->type = SKIN_TOKEN_BATTERY_PERCENTBAR;
    pb->type = token->type;
        
    return 0;
    
#else
    (void)element;
    if (token->type == SKIN_TOKEN_PROGRESSBAR ||
        token->type == SKIN_TOKEN_PLAYER_PROGRESSBAR)
    {
        wps_data->full_line_progressbar = 
                        token->type == SKIN_TOKEN_PLAYER_PROGRESSBAR;
    }
    return 0;

#endif
}

#ifdef HAVE_ALBUMART
static int parse_albumart_load(struct skin_element* element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    struct dim dimensions;
    int albumart_slot;
    bool swap_for_rtl = lang_is_rtl() && follow_lang_direction;
    struct skin_albumart *aa = 
        (struct skin_albumart *)skin_buffer_alloc(sizeof(struct skin_albumart));
    (void)token; /* silence warning */
    if (!aa)
        return -1;

    /* reset albumart info in wps */
    aa->width = -1;
    aa->height = -1;
    aa->xalign = WPS_ALBUMART_ALIGN_CENTER; /* default */
    aa->yalign = WPS_ALBUMART_ALIGN_CENTER; /* default */

    aa->x = element->params[0].data.number;
    aa->y = element->params[1].data.number;
    aa->width = element->params[2].data.number;
    aa->height = element->params[3].data.number;
    
    aa->vp = &curr_vp->vp;
    aa->draw_handle = -1;

    /* if we got here, we parsed everything ok .. ! */
    if (aa->width < 0)
        aa->width = 0;
    else if (aa->width > LCD_WIDTH)
        aa->width = LCD_WIDTH;

    if (aa->height < 0)
        aa->height = 0;
    else if (aa->height > LCD_HEIGHT)
        aa->height = LCD_HEIGHT;

    if (swap_for_rtl)
        aa->x = LCD_WIDTH - (aa->x + aa->width);

    aa->state = WPS_ALBUMART_LOAD;
    wps_data->albumart = aa;

    dimensions.width = aa->width;
    dimensions.height = aa->height;

    albumart_slot = playback_claim_aa_slot(&dimensions);

    if (0 <= albumart_slot)
        wps_data->playback_aa_slot = albumart_slot;
        
    if (element->params_count > 4 && !isdefault(&element->params[4]))
    {
        switch (*element->params[4].data.text)
        {
            case 'l':
            case 'L':
                if (swap_for_rtl)
                    aa->xalign = WPS_ALBUMART_ALIGN_RIGHT;
                else
                    aa->xalign = WPS_ALBUMART_ALIGN_LEFT;
                break;
            case 'c':
            case 'C':
                aa->xalign = WPS_ALBUMART_ALIGN_CENTER;
                break;
            case 'r':
            case 'R':
                if (swap_for_rtl)
                    aa->xalign = WPS_ALBUMART_ALIGN_LEFT;
                else
                    aa->xalign = WPS_ALBUMART_ALIGN_RIGHT;
                break;
        }
    }
    if (element->params_count > 5 && !isdefault(&element->params[5]))
    {
        switch (*element->params[5].data.text)
        {
            case 't':
            case 'T':
                aa->yalign = WPS_ALBUMART_ALIGN_TOP;
                break;
            case 'c':
            case 'C':
                aa->yalign = WPS_ALBUMART_ALIGN_CENTER;
                break;
            case 'b':
            case 'B':
                aa->yalign = WPS_ALBUMART_ALIGN_BOTTOM;
                break;
        }
    }
    return 0;
}

#endif /* HAVE_ALBUMART */

#ifdef HAVE_TOUCHSCREEN

struct touchaction {const char* s; int action;};
static const struct touchaction touchactions[] = {
    /* generic actions, convert to screen actions on use */
    {"prev", ACTION_STD_PREV },         {"next", ACTION_STD_NEXT },
    {"rwd", ACTION_STD_PREVREPEAT },    {"ffwd", ACTION_STD_NEXTREPEAT },
    {"hotkey", ACTION_STD_HOTKEY},      {"select", ACTION_STD_OK },
    {"menu", ACTION_STD_MENU },         {"cancel", ACTION_STD_CANCEL },
    {"contextmenu", ACTION_STD_CONTEXT},{"quickscreen", ACTION_STD_QUICKSCREEN },
    /* not really WPS specific, but no equivilant ACTION_STD_* */
    {"voldown", ACTION_WPS_VOLDOWN},    {"volup", ACTION_WPS_VOLUP},

    /* WPS specific actions */
    {"browse", ACTION_WPS_BROWSE },
    {"play", ACTION_WPS_PLAY },         {"stop", ACTION_WPS_STOP },
    {"shuffle", ACTION_TOUCH_SHUFFLE }, {"repmode", ACTION_TOUCH_REPMODE },
    {"pitch", ACTION_WPS_PITCHSCREEN},  {"playlist", ACTION_WPS_VIEW_PLAYLIST }, 

#if CONFIG_TUNER    
    /* FM screen actions */
    /* Also allow browse, play, stop from WPS codes */
    {"mode", ACTION_FM_MODE },          {"record", ACTION_FM_RECORD },
    {"presets", ACTION_FM_PRESET}, 
#endif
};

static int parse_touchregion(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    (void)token;
    unsigned i, imax;
    struct touchregion *region = NULL;
    const char *action;
    const char pb_string[] = "progressbar";
    const char vol_string[] = "volume";
    char temp[20];

    /* format: %T(x,y,width,height,action)
     * if action starts with & the area must be held to happen
     * action is one of:
     * play  -  play/pause playback
     * stop  -  stop playback, exit the wps
     * prev  -  prev track
     * next  -  next track
     * ffwd  -  seek forward
     * rwd   -  seek backwards
     * menu  -  go back to the main menu
     * browse - go back to the file/db browser
     * shuffle - toggle shuffle mode
     * repmode - cycle the repeat mode
     * quickscreen - go into the quickscreen
     * contextmenu - open the context menu
     * playlist - go into the playlist
     * pitch - go into the pitchscreen
     * volup - increase volume by one step
     * voldown - decrease volume by one step
    */

    
    region = (struct touchregion*)skin_buffer_alloc(sizeof(struct touchregion));
    if (!region)
        return WPS_ERROR_INVALID_PARAM;

    /* should probably do some bounds checking here with the viewport... but later */
    region->action = ACTION_NONE;
    region->x = element->params[0].data.number;
    region->y = element->params[1].data.number;
    region->width = element->params[2].data.number;
    region->height = element->params[3].data.number;
    region->wvp = curr_vp;
    region->armed = false;
    region->reverse_bar = false;
    action = element->params[4].data.text;

    strcpy(temp, action);
    action = temp;
    
    if (*action == '!')
    {
        region->reverse_bar = true;
        action++;
    }

    if(!strcmp(pb_string, action))
        region->type = WPS_TOUCHREGION_SCROLLBAR;
    else if(!strcmp(vol_string, action))
        region->type = WPS_TOUCHREGION_VOLUME;
    else
    {
        region->type = WPS_TOUCHREGION_ACTION;

        if (*action == '&')
        {
            action++;
            region->repeat = true;
        }
        else
            region->repeat = false;

        imax = ARRAYLEN(touchactions);
        for (i = 0; i < imax; i++)
        {
            /* try to match with one of our touchregion screens */
            if (!strcmp(touchactions[i].s, action))
            {
                region->action = touchactions[i].action;
                break;
            }
        }
        if (region->action == ACTION_NONE)
            return WPS_ERROR_INVALID_PARAM;
    }
    struct skin_token_list *item = new_skin_token_list_item(NULL, region);
    if (!item)
        return WPS_ERROR_INVALID_PARAM;
    add_to_ll_chain(&wps_data->touchregions, item);
    return 0;
}
#endif

static bool check_feature_tag(const int type)
{
    switch (type)
    {
        case SKIN_TOKEN_RTC_PRESENT:
#if CONFIG_RTC
            return true;
#else
            return false;
#endif
        case SKIN_TOKEN_HAVE_RECORDING:
#ifdef HAVE_RECORDING
            return true;
#else
            return false;
#endif
        case SKIN_TOKEN_HAVE_TUNER:
#if CONFIG_TUNER
            if (radio_hardware_present())
                return true;
#endif
            return false;

#if CONFIG_TUNER
        case SKIN_TOKEN_HAVE_RDS:
#ifdef HAVE_RDS_CAP
            return true;
#else
            return false;
#endif /* HAVE_RDS_CAP */
#endif /* CONFIG_TUNER */
        default: /* not a tag we care about, just don't skip */
            return true;
    }
}

/*
 * initial setup of wps_data; does reset everything
 * except fields which need to survive, i.e.
 * 
 **/
static void skin_data_reset(struct wps_data *wps_data)
{
    wps_data->tree = NULL;
#ifdef HAVE_LCD_BITMAP
    wps_data->images = NULL;
    wps_data->progressbars = NULL;
#endif
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    wps_data->backdrop = NULL;
#endif
#ifdef HAVE_TOUCHSCREEN
    wps_data->touchregions = NULL;
#endif
#ifdef HAVE_ALBUMART
    wps_data->albumart = NULL;
    if (wps_data->playback_aa_slot >= 0)
    {
        playback_release_aa_slot(wps_data->playback_aa_slot);
        wps_data->playback_aa_slot = -1;
    }
#endif

#ifdef HAVE_LCD_BITMAP
    wps_data->peak_meter_enabled = false;
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
#else /* HAVE_LCD_CHARCELLS */
    /* progress bars */
    int i;
    for (i = 0; i < 8; i++)
    {
        wps_data->wps_progress_pat[i] = 0;
    }
    wps_data->full_line_progressbar = false;
#endif
    wps_data->wps_loaded = false;
}

#ifdef HAVE_LCD_BITMAP
static bool load_skin_bmp(struct wps_data *wps_data, struct bitmap *bitmap, char* bmpdir)
{
    (void)wps_data; /* only needed for remote targets */
    char img_path[MAX_PATH];
    int fd;
    get_image_filename(bitmap->data, bmpdir,
                       img_path, sizeof(img_path));

    /* load the image */
    int format;
#ifdef HAVE_REMOTE_LCD
    if (curr_screen == SCREEN_REMOTE)
        format = FORMAT_ANY|FORMAT_REMOTE;
    else
#endif
        format = FORMAT_ANY|FORMAT_TRANSPARENT;

    fd = open(img_path, O_RDONLY);
    if (fd < 0)
        return false;
    size_t buf_size = read_bmp_file(img_path, bitmap, 0, 
                                    format|FORMAT_RETURN_SIZE, NULL);  
    char* imgbuf = (char*)skin_buffer_alloc(buf_size);
    if (!imgbuf)
    {
        close(fd);
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);
    bitmap->data = imgbuf;
    int ret = read_bmp_fd(fd, bitmap, buf_size, format, NULL);

    close(fd);
    if (ret > 0)
    {
        return true;
    }
    else
    {
        /* Abort if we can't load an image */
        DEBUGF("Couldn't load '%s'\n", img_path);
        return false;
    }
}

static bool load_skin_bitmaps(struct wps_data *wps_data, char *bmpdir)
{
    struct skin_token_list *list;
    bool retval = true; /* return false if a single image failed to load */
    /* do the progressbars */
    list = wps_data->progressbars;
    while (list)
    {
        struct progressbar *pb = (struct progressbar*)list->token->value.data;
        if (pb->bm.data)
        {
            pb->have_bitmap_pb = load_skin_bmp(wps_data, &pb->bm, bmpdir);
            if (!pb->have_bitmap_pb) /* no success */
                retval = false;
        }
        list = list->next;
    }
    /* regular images */
    list = wps_data->images;
    while (list)
    {
        struct gui_img *img = (struct gui_img*)list->token->value.data;
        if (img->bm.data)
        {
            img->loaded = load_skin_bmp(wps_data, &img->bm, bmpdir);
            if (img->loaded)
                img->subimage_height = img->bm.height / img->num_subimages;
            else
                retval = false;
        }
        list = list->next;
    }

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
    /* Backdrop load scheme:
     * 1) %X|filename|
     * 2) load the backdrop from settings
     */
    if (wps_data->backdrop)
    {
        bool needed = wps_data->backdrop[0] != '-';
        wps_data->backdrop = skin_backdrop_load(wps_data->backdrop,
                                                bmpdir, curr_screen);
        if (!wps_data->backdrop && needed)
            retval = false;
    }
#endif /* has backdrop support */

    return retval;
}

static bool skin_load_fonts(struct wps_data *data)
{
    /* don't spit out after the first failue to aid debugging */
    bool success = true;
    struct skin_element *vp_list;
    int font_id;
    /* walk though each viewport and assign its font */
    for(vp_list = data->tree; vp_list; vp_list = vp_list->next)
    {
        /* first, find the viewports that have a non-sys/ui-font font */
        struct skin_viewport *skin_vp =
                (struct skin_viewport*)vp_list->data;
        struct viewport *vp = &skin_vp->vp;


        if (vp->font <= FONT_UI)
        {   /* the usual case -> built-in fonts */
#ifdef HAVE_REMOTE_LCD
            if (vp->font == FONT_UI)
                vp->font += curr_screen;
#endif
            continue;
        }
        font_id = vp->font;

        /* now find the corresponding skin_font */
        struct skin_font *font = &skinfonts[font_id-FONT_FIRSTUSERFONT];
        if (!font->name)
        {
            if (success)
            {
                DEBUGF("font %d not specified\n", font_id);
            }
            success = false;
            continue;
        }

        /* load the font - will handle loading the same font again if
         * multiple viewports use the same */
        if (font->id < 0)
        {
            char *dot = strchr(font->name, '.');
            *dot = '\0';
            font->id = skin_font_load(font->name);
        }

        if (font->id < 0)
        {
            DEBUGF("Unable to load font %d: '%s.fnt'\n",
                    font_id, font->name);
            font->name = NULL; /* to stop trying to load it again if we fail */
            success = false;
            font->name = NULL;
            continue;
        }

        /* finally, assign the font_id to the viewport */
        vp->font = font->id;
    }
    return success;
}

#endif /* HAVE_LCD_BITMAP */
static int convert_viewport(struct wps_data *data, struct skin_element* element)
{
    struct skin_viewport *skin_vp = 
        (struct skin_viewport *)skin_buffer_alloc(sizeof(struct skin_viewport));
    struct screen *display = &screens[curr_screen];
    
    if (!skin_vp)
        return CALLBACK_ERROR;
        
    skin_vp->hidden_flags = 0;
    skin_vp->label = VP_NO_LABEL;
    element->data = skin_vp;
    curr_vp = skin_vp;
    curr_viewport_element = element;
    
    viewport_set_defaults(&skin_vp->vp, curr_screen);
    
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
    skin_vp->start_fgcolour = skin_vp->vp.fg_pattern;
    skin_vp->start_bgcolour = skin_vp->vp.bg_pattern;
#endif
    

    struct skin_tag_parameter *param = element->params;
    if (element->params_count == 0) /* default viewport */
    {
        if (!data->tree) /* first viewport in the skin */
            data->tree = element;
        skin_vp->label = VP_DEFAULT_LABEL;
        return CALLBACK_OK;
    }
    
    if (element->params_count == 6)
    {
        if (element->tag->type == SKIN_TOKEN_UIVIEWPORT_LOAD)
        {
            if (isdefault(param))
            {
                skin_vp->hidden_flags = VP_NEVER_VISIBLE;
                skin_vp->label = VP_INFO_LABEL|VP_DEFAULT_LABEL;
            }
            else
            {
                skin_vp->hidden_flags = VP_NEVER_VISIBLE;
                skin_vp->label = VP_INFO_LABEL|param->data.text[0];
            }
        }
        else
        {
                skin_vp->hidden_flags = VP_DRAW_HIDEABLE|VP_DRAW_HIDDEN;
                skin_vp->label = param->data.text[0];
        }
        param++;
    }
    /* x */
    if (!isdefault(param))
    {
        skin_vp->vp.x = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.x += display->lcdwidth;
    }
    param++;
    /* y */
    if (!isdefault(param))
    {
        skin_vp->vp.y = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.y += display->lcdheight;
    }
    param++;
    /* width */
    if (!isdefault(param))
    {
        skin_vp->vp.width = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.width = (skin_vp->vp.width + display->lcdwidth) - skin_vp->vp.x;
    }
    else
    {
        skin_vp->vp.width = display->lcdwidth - skin_vp->vp.x;
    }
    param++;
    /* height */
    if (!isdefault(param))
    {
        skin_vp->vp.height = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.height = (skin_vp->vp.height + display->lcdheight) - skin_vp->vp.y;
    }
    else
    {
        skin_vp->vp.height = display->lcdheight - skin_vp->vp.y;
    }
    param++;
#ifdef HAVE_LCD_BITMAP
    /* font */
    if (!isdefault(param))
    {
        skin_vp->vp.font = param->data.number;
    }
#endif    

    return CALLBACK_OK;
    
}

static int skin_element_callback(struct skin_element* element, void* data)
{
    struct wps_data *wps_data = (struct wps_data *)data;
    struct wps_token *token;
    parse_function function = NULL;
    
    switch (element->type)
    {
        /* IMPORTANT: element params are shared, so copy them if needed
         *            or use then NOW, dont presume they have a long lifespan
         */
        case TAG:
        {
            token = (struct wps_token*)skin_buffer_alloc(sizeof(struct wps_token));
            memset(token, 0, sizeof(*token));
            token->type = element->tag->type;
            
            if ((element->tag->flags&SKIN_REFRESH_ALL) == SKIN_RTC_REFRESH)
            {
#ifdef CONFIG_RTC
                curr_line->update_mode |= SKIN_REFRESH_DYNAMIC;
#else
                curr_line->update_mode |= SKIN_REFRESH_STATIC;
#endif
            }
            else
                curr_line->update_mode |= element->tag->flags&SKIN_REFRESH_ALL;
            
            element->data = token;
            
            /* Some tags need special handling for the tag, so add them here */
            switch (token->type)
            {
                case SKIN_TOKEN_ALIGN_LANGDIRECTION:
                    follow_lang_direction = 2;
                    break;
                case SKIN_TOKEN_PROGRESSBAR:
                case SKIN_TOKEN_VOLUME:
                case SKIN_TOKEN_BATTERY_PERCENT:
                case SKIN_TOKEN_PLAYER_PROGRESSBAR:
                    function = parse_progressbar_tag;
                    break;
                case SKIN_TOKEN_SUBLINE_TIMEOUT:
                case SKIN_TOKEN_BUTTON_VOLUME:
                case SKIN_TOKEN_TRACK_STARTING:
                case SKIN_TOKEN_TRACK_ENDING:
                case SKIN_TOKEN_LASTTOUCH:
                    function = parse_timeout_tag;
                    break;
#ifdef HAVE_LCD_BITMAP
                case SKIN_TOKEN_DISABLE_THEME:
                case SKIN_TOKEN_ENABLE_THEME:
                case SKIN_TOKEN_DRAW_INBUILTBAR:
                    function = parse_statusbar_tags;
                    break;
#endif
                case SKIN_TOKEN_FILE_DIRECTORY:
                    token->value.i = element->params[0].data.number;
                    break;
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
                case SKIN_TOKEN_VIEWPORT_FGCOLOUR:
                case SKIN_TOKEN_VIEWPORT_BGCOLOUR:
                    function = parse_viewportcolour;
                    break;
                case SKIN_TOKEN_IMAGE_BACKDROP:
                    function = parse_image_special;
                    break;
#endif
                case SKIN_TOKEN_TRANSLATEDSTRING:
                case SKIN_TOKEN_SETTING:
                    function = parse_setting_and_lang;
                    break;
#ifdef HAVE_LCD_BITMAP
                case SKIN_TOKEN_VIEWPORT_CUSTOMLIST:
                    function = parse_playlistview;
                    break;
                case SKIN_TOKEN_LOAD_FONT:
                    function = parse_font_load;
                    break;
                case SKIN_TOKEN_VIEWPORT_ENABLE:
                case SKIN_TOKEN_UIVIEWPORT_ENABLE:
                    token->value.i = element->params[0].data.text[0];
                    break;
                case SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY:
                    function = parse_image_display;
                    break;
                case SKIN_TOKEN_IMAGE_PRELOAD:
                case SKIN_TOKEN_IMAGE_DISPLAY:
                    function = parse_image_load;
                    break;
#endif
#ifdef HAVE_TOUCHSCREEN
                case SKIN_TOKEN_TOUCHREGION:
                    function = parse_touchregion;
                    break;
#endif
#ifdef HAVE_ALBUMART
                case SKIN_TOKEN_ALBUMART_DISPLAY:
                    if (wps_data->albumart)
                        wps_data->albumart->vp = &curr_vp->vp;
                    break;
                case SKIN_TOKEN_ALBUMART_LOAD:
                    function = parse_albumart_load;
                    break;
#endif
                default:
                    break;
            }
            if (function)
            {
                if (function(element, token, wps_data) < 0)
                    return CALLBACK_ERROR;
            }
            /* tags that start with 'F', 'I' or 'D' are for the next file */
            if ( *(element->tag->name) == 'I' || *(element->tag->name) == 'F' ||
                 *(element->tag->name) == 'D')
                token->next = true;
            if (follow_lang_direction > 0 )
                follow_lang_direction--;
            break;
        }
        case VIEWPORT:
            return convert_viewport(wps_data, element);
        case LINE:
        {
            struct line *line = 
                (struct line *)skin_buffer_alloc(sizeof(struct line));
            line->update_mode = SKIN_REFRESH_STATIC;
            line->timeout = DEFAULT_SUBLINE_TIME_MULTIPLIER * TIMEOUT_UNIT;
            curr_line = line;
            element->data = line;
        }
        break;
        case LINE_ALTERNATOR:
        {
            struct line_alternator *alternator = 
                (struct line_alternator *)skin_buffer_alloc(sizeof(struct line_alternator));
            alternator->current_line = 0;
#ifndef __PCTOOL__
            alternator->last_change_tick = current_tick;
#endif
            element->data = alternator;
        }
        break;
        case CONDITIONAL:
        {
            struct conditional *conditional = 
                (struct conditional *)skin_buffer_alloc(sizeof(struct conditional));
            conditional->last_value = -1;
            conditional->token = element->data;
            element->data = conditional;
            if (!check_feature_tag(element->tag->type))
            {
                return FEATURE_NOT_AVAILABLE;
            }
            return CALLBACK_OK;
        }
        case TEXT:
            curr_line->update_mode |= SKIN_REFRESH_STATIC;
            break;
        default:
            break;
    }
    return CALLBACK_OK;
}

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool skin_data_load(enum screen_type screen, struct wps_data *wps_data,
                    const char *buf, bool isfile)
{
    char *wps_buffer = NULL;
    if (!wps_data || !buf)
        return false;
#ifdef HAVE_ALBUMART
    int status;
    struct mp3entry *curtrack;
    long offset;
    struct skin_albumart old_aa = {.state = WPS_ALBUMART_NONE};
    if (wps_data->albumart)
    {
        old_aa.state = wps_data->albumart->state;
        old_aa.height = wps_data->albumart->height;
        old_aa.width = wps_data->albumart->width;
    }
#endif
#ifdef HAVE_LCD_BITMAP
    int i;
    for (i=0;i<MAXUSERFONTS;i++)
    {
        skinfonts[i].id = -1;
        skinfonts[i].name = NULL;
    }
#endif
#ifdef DEBUG_SKIN_ENGINE
    if (isfile && debug_wps)
    {
        DEBUGF("\n=====================\nLoading '%s'\n=====================\n", buf);
    }
#endif


    skin_data_reset(wps_data);
    wps_data->wps_loaded = false;
    curr_screen = screen;
    curr_line = NULL;
    curr_vp = NULL;
    curr_viewport_element = NULL;

    if (isfile)
    {
        int fd = open_utf8(buf, O_RDONLY);

        if (fd < 0)
            return false;

        /* get buffer space from the plugin buffer */
        size_t buffersize = 0;
        wps_buffer = (char *)plugin_get_buffer(&buffersize);

        if (!wps_buffer)
            return false;

        /* copy the file's content to the buffer for parsing,
           ensuring that every line ends with a newline char. */
        unsigned int start = 0;
        while(read_line(fd, wps_buffer + start, buffersize - start) > 0)
        {
            start += strlen(wps_buffer + start);
            if (start < buffersize - 1)
            {
                wps_buffer[start++] = '\n';
                wps_buffer[start] = 0;
            }
        }
        close(fd);
        if (start <= 0)
            return false;
    }
    else
    {
        wps_buffer = (char*)buf;
    }
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    wps_data->backdrop = "-";
#endif
    /* parse the skin source */
    skin_buffer_save_position();
    wps_data->tree = skin_parse(wps_buffer, skin_element_callback, wps_data);
    if (!wps_data->tree) {
        skin_data_reset(wps_data);
        skin_buffer_restore_position();
        return false;
    }

#ifdef HAVE_LCD_BITMAP
    char bmpdir[MAX_PATH];
    if (isfile)
    {
        /* get the bitmap dir */
        char *dot = strrchr(buf, '.');
        strlcpy(bmpdir, buf, dot - buf + 1);
    }
    else
    {
        snprintf(bmpdir, MAX_PATH, "%s", BACKDROP_DIR);
    }
    /* load the bitmaps that were found by the parsing */
    if (!load_skin_bitmaps(wps_data, bmpdir) ||
        !skin_load_fonts(wps_data)) 
    {
        skin_data_reset(wps_data);
        skin_buffer_restore_position();
        return false;
    }
#endif
#if defined(HAVE_ALBUMART) && !defined(__PCTOOL__)
    status = audio_status();
    if (status & AUDIO_STATUS_PLAY)
    {
        struct skin_albumart *aa = wps_data->albumart;
        if (aa && ((aa->state && !old_aa.state) ||
            (aa->state &&
            (((old_aa.height != aa->height) ||
            (old_aa.width != aa->width))))))
        {
            curtrack = audio_current_track();
            offset = curtrack->offset;
            audio_stop();
            if (!(status & AUDIO_STATUS_PAUSE))
                audio_play(offset);
        }
    }
#endif
    wps_data->wps_loaded = true;
#ifdef DEBUG_SKIN_ENGINE
 //   if (isfile && debug_wps)
 //       debug_skin_usage();
#endif
    return true;
}
