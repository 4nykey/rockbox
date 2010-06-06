/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Jonathan Gordon
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
#include <SDL.h>
#include "button.h"
#include "buttonmap.h"
#include "touchscreen.h"

int key_to_touch(int keyboard_button)
{
    int new_btn = BUTTON_NONE;
    switch (keyboard_button)
    {
        case BUTTON_TOUCHSCREEN:
            switch (touchscreen_get_mode())
            {
                case TOUCHSCREEN_POINT:
                    new_btn = BUTTON_TOUCHSCREEN;
                    break;
                case TOUCHSCREEN_BUTTON:
                {
                    static const int touchscreen_buttons[3][3] = {
                        {BUTTON_TOPLEFT, BUTTON_TOPMIDDLE, BUTTON_TOPRIGHT},
                        {BUTTON_MIDLEFT, BUTTON_CENTER, BUTTON_MIDRIGHT},
                        {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT},
                    };
                    int px_x = ((mouse_coords&0xffff0000)>>16);
                    int px_y = ((mouse_coords&0x0000ffff));
                    new_btn = touchscreen_buttons[px_y/(LCD_HEIGHT/3)][px_x/(LCD_WIDTH/3)];
                    break;
                }
            }
            break;
        case SDLK_KP7:
        case SDLK_7:
            new_btn = BUTTON_TOPLEFT;
            break;
        case SDLK_KP8:
        case SDLK_8:
        case SDLK_UP:
            new_btn = BUTTON_TOPMIDDLE;
            break;
        case SDLK_KP9:
        case SDLK_9:
            new_btn = BUTTON_TOPRIGHT;
            break;
        case SDLK_KP4:
        case SDLK_u:
        case SDLK_LEFT:
            new_btn = BUTTON_MIDLEFT;
            break;
        case SDLK_KP5:
        case SDLK_i:
            new_btn = BUTTON_CENTER;
            break;
        case SDLK_KP6:
        case SDLK_o:
        case SDLK_RIGHT:
            new_btn = BUTTON_MIDRIGHT;
            break;
        case SDLK_KP1:
        case SDLK_j:
            new_btn = BUTTON_BOTTOMLEFT;
            break;
        case SDLK_KP2:
        case SDLK_k:
        case SDLK_DOWN:
            new_btn = BUTTON_BOTTOMMIDDLE;
            break;
        case SDLK_KP3:
        case SDLK_l:
            new_btn = BUTTON_BOTTOMRIGHT;
            break;
        case SDLK_F4:
            if(pressed)
            {
                touchscreen_set_mode(touchscreen_get_mode() == TOUCHSCREEN_POINT ? TOUCHSCREEN_BUTTON : TOUCHSCREEN_POINT);
                printf("Touchscreen mode: %s\n", touchscreen_get_mode() == TOUCHSCREEN_POINT ? "TOUCHSCREEN_POINT" : "TOUCHSCREEN_BUTTON");
            }
            break;
    }
    return new_btn;
}
