/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2006 by Daniel Everton <dan@iocaine.org>
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

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "autoconf.h"
#include "button.h"
#include "system-sdl.h"
#include "thread.h"
#include "kernel.h"
#include "uisdl.h"
#include "lcd-sdl.h"
#ifdef HAVE_LCD_BITMAP
#include "lcd-bitmap.h"
#elif defined(HAVE_LCD_CHARCELLS)
#include "lcd-charcells.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote-bitmap.h"
#endif
#include "thread-sdl.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"

/* extern functions */
extern void new_key(int key);

void button_event(int key, bool pressed);

SDL_Surface *gui_surface;
bool background = true;                   /* use backgrounds by default */
#ifdef HAVE_REMOTE_LCD
static bool showremote = true;            /* include remote by default */
#endif

bool lcd_display_redraw = true;         /* Used for player simulator */
char having_new_lcd = true;               /* Used for player simulator */
bool sim_alarm_wakeup = false;
const char *sim_root_dir = NULL;

bool debug_audio = false;

bool debug_wps = false;
int wps_verbose_level = 3;

void gui_message_loop(void)
{
    SDL_Event event;
    bool done = false;

    while(!done && SDL_WaitEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                sim_enter_irq_handler();
                button_event(event.key.keysym.sym, true);
                sim_exit_irq_handler();
                break;
            case SDL_KEYUP:
                sim_enter_irq_handler();
                button_event(event.key.keysym.sym, false);
                sim_exit_irq_handler();
                break;
#ifndef HAVE_TOUCHSCREEN
            case SDL_MOUSEBUTTONDOWN:
                if (debug_wps && event.button.button == 1)
                {
                    printf("Mouse at: (%d, %d)\n", event.button.x, event.button.y);
                }
                break;
#else 
            case SDL_MOUSEBUTTONUP:
                sim_enter_irq_handler();
                button_event(BUTTON_TOUCHSCREEN, false);
                sim_exit_irq_handler();
                break;
#endif
            case SDL_QUIT:
                done = true;
                break;
            default:
                /*printf("Unhandled event\n"); */
                break;
        }
    }
}

bool gui_startup(void)
{
    SDL_Surface *picture_surface;
    int width, height;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)) {
        fprintf(stderr, "fatal: %s\n", SDL_GetError());
        return false;
    }

    atexit(SDL_Quit);

    /* Try and load the background image. If it fails go without */
    if (background) {
        picture_surface = SDL_LoadBMP("UI256.bmp");
        if (picture_surface == NULL) {
            background = false;
            fprintf(stderr, "warn: %s\n", SDL_GetError());
        }
    }

    /* Set things up */
    if (background)
    {
        width = UI_WIDTH;
        height = UI_HEIGHT;
    } 
    else 
    {
#ifdef HAVE_REMOTE_LCD
        if (showremote)
        {
            width = SIM_LCD_WIDTH > SIM_REMOTE_WIDTH ? SIM_LCD_WIDTH : SIM_REMOTE_WIDTH;
            height = SIM_LCD_HEIGHT + SIM_REMOTE_HEIGHT;
        }
        else
#endif
        {
            width = SIM_LCD_WIDTH;
            height = SIM_LCD_HEIGHT;
        }
    }
   
    
    if ((gui_surface = SDL_SetVideoMode(width * display_zoom, height * display_zoom, 24, SDL_HWSURFACE|SDL_DOUBLEBUF)) == NULL) {
        fprintf(stderr, "fatal: %s\n", SDL_GetError());
        return false;
    }

    SDL_WM_SetCaption(UI_TITLE, NULL);

    sim_lcd_init();
#ifdef HAVE_REMOTE_LCD
    if (showremote)
        sim_lcd_remote_init();
#endif

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    if (background && picture_surface != NULL) {
        SDL_BlitSurface(picture_surface, NULL, gui_surface, NULL);
        SDL_UpdateRect(gui_surface, 0, 0, 0, 0);
    }
    
    return true;
}

bool gui_shutdown(void)
{
    /* Order here is relevent to prevent deadlocks and use of destroyed
       sync primitives by kernel threads */
    thread_sdl_shutdown();
    sim_kernel_shutdown();
    return true;
}

#if defined(WIN32) && defined(main)
/* Don't use SDL_main on windows -> no more stdio redirection */
#undef main
#endif

int main(int argc, char *argv[])
{
    if (argc >= 1) 
    {
        int x;
        for (x = 1; x < argc; x++) 
        {
            if (!strcmp("--debugaudio", argv[x])) 
            {
                debug_audio = true;
                printf("Writing debug audio file.\n");
            } 
            else if (!strcmp("--debugwps", argv[x]))
            {
                debug_wps = true;
                printf("WPS debug mode enabled.\n");
            } 
            else if (!strcmp("--nobackground", argv[x]))
            {
                background = false;
                printf("Disabling background image.\n");
            } 
#ifdef HAVE_REMOTE_LCD
            else if (!strcmp("--noremote", argv[x]))
            {
                showremote = false;
                background = false;
                printf("Disabling remote image.\n");
            }
#endif
            else if (!strcmp("--old_lcd", argv[x]))
            {
                having_new_lcd = false;
                printf("Using old LCD layout.\n");
            }
            else if (!strcmp("--zoom", argv[x]))
            {
                x++;
                if(x < argc)
                    display_zoom=atoi(argv[x]);
                else
                    display_zoom = 2;
                printf("Window zoom is %d\n", display_zoom);
            }
            else if (!strcmp("--alarm", argv[x]))
            {
                sim_alarm_wakeup = true;
                printf("Simulating alarm wakeup.\n");
            }
            else if (!strcmp("--root", argv[x]))
            {
                x++;
                if (x < argc)
                {
                    sim_root_dir = argv[x];
                    printf("Root directory: %s\n", sim_root_dir);
                }
            }
            else 
            {
                printf("rockboxui\n");
                printf("Arguments:\n");
                printf("  --debugaudio \t Write raw PCM data to audiodebug.raw\n");
                printf("  --debugwps \t Print advanced WPS debug info\n");
                printf("  --nobackground \t Disable the background image\n");
#ifdef HAVE_REMOTE_LCD
                printf("  --noremote \t Disable the remote image (will disable backgrounds)\n");
#endif
                printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
                printf("  --zoom [VAL]\t Window zoom (will disable backgrounds)\n");
                printf("  --alarm \t Simulate a wake-up on alarm\n");
                printf("  --root [DIR]\t Set root directory\n");
                exit(0);
            }
        }
    }

    if (display_zoom > 1) {
        background = false;
    }

    if (!sim_kernel_init()) {
        fprintf(stderr, "sim_kernel_init failed\n");
        return -1;
    }

    if (!gui_startup()) {
        fprintf(stderr, "gui_startup failed\n");
        return -1;
    }

    /* app_main will be called by the new main thread */
    if (!thread_sdl_init(NULL)) {
        fprintf(stderr, "thread_sdl_init failed\n");
        return -1;
    }

    gui_message_loop();

    return gui_shutdown();
}

