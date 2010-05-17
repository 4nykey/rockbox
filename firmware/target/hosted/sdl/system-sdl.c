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

#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "system-sdl.h"
#include "thread-sdl.h"
#include "sim-ui-defines.h"
#include "lcd-sdl.h"
#ifdef HAVE_LCD_BITMAP
#include "lcd-bitmap.h"
#elif defined(HAVE_LCD_CHARCELLS)
#include "lcd-charcells.h"
#endif
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote-bitmap.h"
#endif
#include "panic.h"
#include "debug.h"

SDL_Surface    *gui_surface;

bool            background = true;          /* use backgrounds by default */
#ifdef HAVE_REMOTE_LCD
bool            showremote = true;          /* include remote by default */
#endif
bool            mapping = false;
bool            debug_buttons = false;

bool            lcd_display_redraw = true;  /* Used for player simulator */
char            having_new_lcd = true;      /* Used for player simulator */
bool            sim_alarm_wakeup = false;
const char     *sim_root_dir = NULL;
extern int      display_zoom;

#ifdef DEBUG
bool debug_audio = false;
#endif

bool debug_wps = false;
int wps_verbose_level = 3;


void sys_poweroff(void)
{
    /* Order here is relevent to prevent deadlocks and use of destroyed
       sync primitives by kernel threads */
    sim_thread_shutdown();
    sim_kernel_shutdown();
    SDL_Quit();
}

/*
 * Button read loop */
void gui_message_loop(void);

/*
 * This callback let's the main thread run again after SDL has been initialized
 **/
static uint32_t cond_signal(uint32_t interval, void *param)
{
    (void)interval;
    SDL_cond *c = (SDL_cond*)param;
    /* remove timer, CondSignal returns 0 on success */
    return SDL_CondSignal(c);
}

/*
 * This thread will read the buttons in an interrupt like fashion, and
 * also initializes SDL_INIT_VIDEO and the surfaces
 *
 * it must be done in the same thread (at least on windows) because events only
 * work in the thread which called SDL_Init(SubSystem) with SDL_INIT_VIDEO
 *
 * This is an SDL thread and relies on preemptive behavoir of the host
 **/
static int sdl_event_thread(void * param)
{
    SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_Surface *picture_surface;
    int width, height;

    /* Try and load the background image. If it fails go without */
    if (background) {
        picture_surface = SDL_LoadBMP("UI256.bmp");
        if (picture_surface == NULL) {
            background = false;
            DEBUGF("warn: %s\n", SDL_GetError());
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
        panicf("%s", SDL_GetError());
    }

    SDL_WM_SetCaption(UI_TITLE, NULL);

    sim_lcd_init();
#ifdef HAVE_REMOTE_LCD
    if (showremote)
        sim_lcd_remote_init();
#endif

    if (background && picture_surface != NULL)
        SDL_BlitSurface(picture_surface, NULL, gui_surface, NULL);

    /* calling SDL_CondSignal() right away here doesn't work reliably so
     * post-pone it a bit */
    SDL_AddTimer(100, cond_signal, param);
    /*
     * finally enter the button loop */
    while(1)
        gui_message_loop();

    return 0;
}


void system_init(void)
{
    SDL_cond *c;
    SDL_mutex *m;
    if (SDL_Init(SDL_INIT_TIMER))
        panicf("%s", SDL_GetError());
    atexit(SDL_Quit);

    c = SDL_CreateCond();
    m = SDL_CreateMutex();

    SDL_CreateThread(sdl_event_thread, c);

    /* Lock mutex and wait for sdl_event_thread to run so that it can
     * initialize the surfaces and video subsystem needed for SDL events */
    SDL_LockMutex(m);
    SDL_CondWait(c, m);
    SDL_UnlockMutex(m);

    /* cleanup */
    SDL_DestroyCond(c);
    SDL_DestroyMutex(m);
}

void system_exception_wait(void)
{
    sim_thread_exception_wait();
}

void system_reboot(void)
{
    sim_thread_exception_wait();
}


void sys_handle_argv(int argc, char *argv[])
{
    if (argc >= 1) 
    {
        int x;
        for (x = 1; x < argc; x++) 
        {
#ifdef DEBUG
            if (!strcmp("--debugaudio", argv[x])) 
            {
                debug_audio = true;
                printf("Writing debug audio file.\n");
            }
            else 
#endif
                if (!strcmp("--debugwps", argv[x]))
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
            else if (!strcmp("--mapping", argv[x]))
            {
                    mapping = true;
                    printf("Printing click coords with drag radii.\n");
            }
            else if (!strcmp("--debugbuttons", argv[x]))
            {
                    debug_buttons = true;
                    printf("Printing background button clicks.\n");
            }
            else 
            {
                printf("rockboxui\n");
                printf("Arguments:\n");
#ifdef DEBUG
                printf("  --debugaudio \t Write raw PCM data to audiodebug.raw\n");
#endif
                printf("  --debugwps \t Print advanced WPS debug info\n");
                printf("  --nobackground \t Disable the background image\n");
#ifdef HAVE_REMOTE_LCD
                printf("  --noremote \t Disable the remote image (will disable backgrounds)\n");
#endif
                printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
                printf("  --zoom [VAL]\t Window zoom (will disable backgrounds)\n");
                printf("  --alarm \t Simulate a wake-up on alarm\n");
                printf("  --root [DIR]\t Set root directory\n");
                printf("  --mapping \t Output coordinates and radius for mapping backgrounds\n");
                exit(0);
            }
        }
    }
    if (display_zoom > 1) {
        background = false;
    }
}
