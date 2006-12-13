/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 * 
 * H300 Port by Karl Kurbjun
 * IPod port by Dave Chapman and Paul Louden
 * Additional code contributed by Thom Johansen
 * Based off work by:   Digita Doom, IDoom, Prboom, lSDLDoom, LxDoom,
 *                      MBF, Boom, DosDoom,
 *                      and of course Original Doom by ID Software
 * See: http://prboom.sourceforge.net/about.html for the history
 *
 *
 ****************************************************************************/

#include "d_main.h"
#include "doomdef.h"
#include "settings.h"
#include "m_fixed.h"
#include "m_argv.h"
#include "m_misc.h"
#include "g_game.h"
#include "rockmacros.h"
#include "doomstat.h"
#include "i_system.h"
#include "hu_stuff.h"
#include "st_stuff.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

extern boolean timingdemo, singledemo, demoplayback, fastdemo; // killough

int filearray[9];
int fpoint=1; // save 0 for closing

int fileexists(const char * fname)
{
   int fd;
   fd = open(fname,O_RDONLY);

   if (fd>=0)
   {
      close(fd);
      return 0;
   }
   return -1;
}

#ifndef SIMULATOR
int my_open(const char *file, int flags)
{
   if(fpoint==8)
      return -1;
#undef open
   filearray[fpoint]=rb->open(file, flags);

   if(filearray[fpoint]<0)
      return filearray[fpoint];

   fpoint++;
   return filearray[fpoint-1];
}

int my_close(int id)
{
   int i=0;
   if(id<0)
      return id;
   while(filearray[i]!=id && i<8)
      i++;

   if(i==8)
   {
      printf("A requested FID did not exist!!!!");
      return -9;
   }
#undef close
   rb->close(id);

   for(; i<fpoint-1; i++)
      filearray[i]=filearray[i+1];

   fpoint--;
   return 0;
}
#endif
struct plugin_api* rb;
#define MAXARGVS  100

bool noprintf=0;  // Variable disables printf lcd updates to protect grayscale lib/direct lcd updates

// Here is a hacked up printf command to get the output from the game.
int printf(const char *fmt, ...)
{
   static int p_xtpt;
   char p_buf[50];
   bool ok;
   va_list ap;

   va_start(ap, fmt);
   ok = vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
   va_end(ap);

   rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
   if (!noprintf)
      rb->lcd_update();

   p_xtpt+=8;
   if(p_xtpt>LCD_HEIGHT-8)
   {
      p_xtpt=0;
      if (!noprintf)
         rb->lcd_clear_display();
   }
   return 1;
}

char *my_strtok( char * s, const char * delim )
{
    register char *spanp;
    register int c, sc;
    char *tok;
   static char *lasts;


    if (s == NULL && (s = lasts) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {        /* no non-delimiter characters */
        lasts = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

inline void* memcpy(void* dst, const void* src, size_t size)
{
   return rb->memcpy(dst, src, size);
}

struct argvlist
{
   int timedemo;        // 1 says there's a timedemo
   int demonum;
   int addonnum;
} argvlist;

const unsigned char versions_builtin[7][20] =
{
   "Doom Shareware",
   "Doom Registered",
   "Ultimate Doom",
   "Doom 2",
   "Doom 2 French",
   "Plutonia",
   "TNT"
};

const unsigned char wads_builtin[7][30] =
{
   GAMEBASE"doom1.wad",
   GAMEBASE"doom.wad",
   GAMEBASE"doomu.wad",
   GAMEBASE"doom2.wad",
   GAMEBASE"doom2f.wad",
   GAMEBASE"plutonia.wad",
   GAMEBASE"tnt.wad"
};

int namemap[7];
static struct menu_item *addons;
static struct menu_item *demolmp;
char addon[200];
// This sets up the base game and builds up myargv/c
bool Dhandle_ver (int dver)
{
   switch (dver) {
      case 0: /* Doom Shareware */
         gamemode = shareware;
         gamemission = doom;
         D_AddFile(wads_builtin[0],source_iwad);
         break;
      case 1: /* Doom registered */
         gamemode = registered;
         gamemission = doom;
         D_AddFile(wads_builtin[1],source_iwad);
         break;
      case 2: /* Ultimate Doom */
         gamemode = retail;
         gamemission = doom;
         D_AddFile(wads_builtin[2],source_iwad);
         break;
      case 3: /* Doom2 */
         gamemode = commercial;
         gamemission = doom2;
         D_AddFile(wads_builtin[3],source_iwad);
         break;
      case 4: /* Doom2f */
         gamemode = commercial;
         gamemission = doom2;
         D_AddFile(wads_builtin[4],source_iwad);
         break;
      case 5: /* Plutonia */
         gamemode = commercial;
         gamemission = pack_plut;
         D_AddFile(wads_builtin[5],source_iwad);
         break;
      case 6: /* TNT */
         gamemode = commercial;
         gamemission = pack_tnt;
         D_AddFile(wads_builtin[6],source_iwad);
         break;
      default:
         gamemission = none;
         return 0;
   }
   // Start adding to myargv
   if(argvlist.timedemo && (gamemode == shareware))
   {
         singletics = true;
         timingdemo = true;            // show stats after quit
         G_DeferedPlayDemo("demo3");
         singledemo = true;            // quit after one demo
   }

   if(argvlist.addonnum)
   {
      snprintf(addon,sizeof(addon),"%s%s", GAMEBASE"addons/", addons[argvlist.addonnum].desc);
      D_AddFile(addon,source_pwad);
      modifiedgame = true; 
   }

   if(argvlist.demonum)
   {
      snprintf(addon, sizeof(addon),"%s%s", GAMEBASE"demos/", demolmp[argvlist.demonum].desc);
      D_AddFile(addon, source_lmp);
      G_DeferedPlayDemo(addon);
      singledemo = true;          // quit after one demo
   }
   return 1;
}

// This function builds up the basegame list for use in the options selection
// it also sets the defaults for the argvlist
// Now checking for rcokdoom.wad based on prboom.wad
int Dbuild_base (struct opt_items *names)
{
   if ( fileexists(GAMEBASE"rockdoom.wad") )
      return 0;

   D_AddFile (GAMEBASE"rockdoom.wad", source_pwad);

   int i=0;
   /* Doom Shareware */
   if ( !fileexists (wads_builtin[0]) )
   {
      names[i].string=versions_builtin[0];
      names[i].voice_id=0;
      namemap[i]=0;
      i++;
   }

   /* Doom registered */
   if ( !fileexists (wads_builtin[1]) )
   {
      names[i].string=versions_builtin[1];
      names[i].voice_id=0;
      namemap[i]=1;
      i++;
   }

   /* Ultimate Doom */
   if ( !fileexists (wads_builtin[2]) )
   {
      names[i].string=versions_builtin[2];
      names[i].voice_id=0;
      namemap[i]=2;
      i++;
   }

   /* Doom2 */
   if ( !fileexists (wads_builtin[3]) )
   {
      names[i].string=versions_builtin[3];
      names[i].voice_id=0;
      namemap[i]=3;
      i++;
   }

   /* Doom2f */
   if ( !fileexists (wads_builtin[4]) )
   {
      names[i].string=versions_builtin[4];
      names[i].voice_id=0;
      namemap[i]=4;
      i++;
   }

   /* Plutonia */
   if ( !fileexists (wads_builtin[5]) )
   {
      names[i].string=versions_builtin[5];
      names[i].voice_id=0;
      namemap[i]=5;
      i++;
   }

   /* TNT */
   if ( !fileexists (wads_builtin[6]) )
   {
      names[i].string=versions_builtin[6];
      names[i].voice_id=0;
      namemap[i]=6;
      i++;
   }
   // Set argvlist defaults
   argvlist.timedemo=0;

   return i;
}

// This is a general function that takes in an menu_item structure and makes a list
// of files within it based on matching the string stringmatch to the files.
int Dbuild_filelistm(struct menu_item **names, char *firstentry, char *directory, char *stringmatch)
{
   int            i=0;
   DIR            *filedir;
   struct dirent  *dptr;
   char           *startpt;
   struct menu_item *temp;

   filedir=opendir(directory);

   if(filedir==NULL)
   {
      temp=malloc(sizeof(struct opt_items));
      temp[0].desc=firstentry;
      temp[0].function=0;
      *names=temp;
      return 1;
   }

   // Get the total number of entries
   while((dptr=rb->readdir(filedir)))
      i++;

   // Reset the directory
   closedir(filedir);
   filedir=opendir(directory);

   i++;
   temp=malloc(i*sizeof(struct opt_items));
   temp[0].desc=firstentry;
   temp[0].function=0;
   i=1;

   while((dptr=rb->readdir(filedir)))
   {
      if(rb->strcasestr(dptr->d_name, stringmatch))
      {
         startpt=malloc(strlen(dptr->d_name)*sizeof(char));
         strcpy(startpt,dptr->d_name);
         temp[i].desc=startpt;
         temp[i].function=0;
         i++;
      }
   }
   closedir(filedir);
   *names=temp;
   return i;
}

// This key configuration code is not the cleanest or the most efficient, but it works
int translatekey(int key)
{
   if (key<31)
   {
      switch(key)
      {
         case 0:
            return 0;
         case 1:
            return KEY_RIGHTARROW;
         case 2:
            return KEY_LEFTARROW;
         case 3:
            return KEY_UPARROW;
         case 4:
            return KEY_DOWNARROW;
         case 5:
            return KEY_RCTRL;
         case 6:
            return ' ';
         case 7:
            return KEY_ESCAPE;
         case 8:
            return 'w';
         case 9:
            return KEY_ENTER;
         case 10:
         default:
            return 0;
      }
   }
   else
   {
      switch(key)
      {
         case 0:
            return 0;
         case KEY_RIGHTARROW:
            return 1;
         case KEY_LEFTARROW:
            return 2;
         case KEY_UPARROW:
            return 3;
         case KEY_DOWNARROW:
            return 4;
         case KEY_RCTRL:
            return 5;
         case ' ':
            return 6;
         case KEY_ESCAPE:
            return 7;
         case 'w':
            return 8;
         case KEY_ENTER:
            return 9;
         case KEY_F9:
         default:
            return 0;
      }
   }
}

// I havn't added configurable keys for enter or escape because this requires some modification to
// m_menu.c which hasn't been done yet.

int Oset_keys()
{
   int m, result;
   int menuquit=0;

   static const struct opt_items doomkeys[] = {
      { "Unmapped", NULL },
      { "Key Right", NULL },
      { "Key Left", NULL },
      { "Key Up", NULL },
      { "Key Down", NULL },
      { "Key Record", NULL },
      { "Key Mode", NULL },
      { "Key Off", NULL },
      { "Key On", NULL },
      { "Key Select", NULL },
   };
   int numdoomkeys=sizeof(doomkeys) / sizeof(*doomkeys);

   static const struct menu_item items[] = {
      { "Game Right", NULL },
      { "Game Left", NULL },
      { "Game Up", NULL },
      { "Game Down", NULL },
      { "Game Shoot", NULL },
      { "Game Open", NULL },
      { "Game Strafe", NULL },
      { "Game Weapon", NULL },
      { "Game Automap", NULL },
   };

   m = rb->menu_init(items, sizeof(items) / sizeof(*items),
                NULL, NULL, NULL, NULL);

   while(!menuquit)
   {
      result=rb->menu_show(m);
      switch (result)
      {
         case 0:
            key_right=translatekey(key_right);
            rb->set_option(items[0].desc, &key_right, INT, doomkeys, numdoomkeys, NULL );
            key_right=translatekey(key_right);
            break;

         case 1:
            key_left=translatekey(key_left);
            rb->set_option(items[1].desc, &key_left, INT, doomkeys, numdoomkeys, NULL );
            key_left=translatekey(key_left);
            break;

         case 2:
            key_up=translatekey(key_up);
            rb->set_option(items[2].desc, &key_up, INT, doomkeys, numdoomkeys, NULL );
            key_up=translatekey(key_up);
            break;

         case 3:
            key_down=translatekey(key_down);
            rb->set_option(items[3].desc, &key_down, INT, doomkeys, numdoomkeys, NULL );
            key_down=translatekey(key_down);
            break;

         case 4:
            key_fire=translatekey(key_fire);
            rb->set_option(items[4].desc, &key_fire, INT, doomkeys, numdoomkeys, NULL );
            key_fire=translatekey(key_fire);
            break;

         case 5:
            key_use=translatekey(key_use);
            rb->set_option(items[5].desc, &key_use, INT, doomkeys, numdoomkeys, NULL );
            key_use=translatekey(key_use);
            break;

         case 6:
            key_strafe=translatekey(key_strafe);
            rb->set_option(items[6].desc, &key_strafe, INT, doomkeys, numdoomkeys, NULL );
            key_strafe=translatekey(key_strafe);
            break;

         case 7:
            key_weapon=translatekey(key_weapon);
            rb->set_option(items[7].desc, &key_weapon, INT, doomkeys, numdoomkeys, NULL );
            key_weapon=translatekey(key_weapon);
            break;

         case 8:
            key_map=translatekey(key_map);
            rb->set_option(items[8].desc, &key_map, INT, doomkeys, numdoomkeys, NULL );
            key_map=translatekey(key_map);
            break;

         default:
            menuquit=1;
            break;
      }
   }

   rb->menu_exit(m);

   return (1);
}

extern int fake_contrast;

static bool Doptions()
{
   static const struct opt_items onoff[2] = {
      { "Off", NULL },
      { "On", NULL },
   };

   int m, result;
   int menuquit=0;

   static const struct menu_item items[] = {
      { "Sound", NULL },
      { "Set Keys", NULL },
      { "Timedemo", NULL },
      { "Player Bobbing", NULL },
      { "Weapon Recoil", NULL },
      { "Translucency", NULL },
      { "Fake Contrast", NULL },
      { "Always Run", NULL },
      { "Headsup Display", NULL },
      { "Statusbar Always Red", NULL },
   };

   m = rb->menu_init(items, sizeof(items) / sizeof(*items),
                NULL, NULL, NULL, NULL);

   while(!menuquit)
   {
      result=rb->menu_show(m);
      switch (result)
      {
         case 0: /* Sound */
            nosfxparm=!nosfxparm; // Have to invert it before setting
            rb->set_option(items[0].desc, &nosfxparm, INT, onoff, 2, NULL );
            nosfxparm=!nosfxparm;
            break;

         case 1: /* Keys */
            Oset_keys();
            break;

         case 2: /* Timedemo */
            rb->set_option(items[2].desc, &argvlist.timedemo, INT, onoff, 2, NULL );
            break;

         case 3: /* Player Bobbing */
            rb->set_option(items[3].desc, &default_player_bobbing, INT, onoff, 2, NULL );
            break;

         case 4: /* Weapon Recoil */
            rb->set_option(items[4].desc, &default_weapon_recoil, INT, onoff, 2, NULL );
            break;

         case 5: /* Translucency */
            rb->set_option(items[5].desc, &default_translucency, INT, onoff, 2, NULL );
            break;

         case 6: /* Fake Contrast */
            rb->set_option(items[6].desc, &fake_contrast, INT, onoff, 2, NULL );
            break;

         case 7: /* Always Run */
            rb->set_option(items[7].desc, &autorun, INT, onoff, 2, NULL );
            break;

         case 8: /* Headsup Display */
            rb->set_option(items[8].desc, &hud_displayed, INT, onoff, 2, NULL );
            break;

         case 9: /* Statusbar always red */
            rb->set_option(items[9].desc, &sts_always_red, INT, onoff, 2, NULL );
            break;

         default:
            menuquit=1;
            break;
      }
   }

   rb->menu_exit(m);

   return (1);
}

int menuchoice(struct menu_item *menu, int items)
{
   int m, result;
   
   m = rb->menu_init(menu, items,NULL, NULL, NULL, NULL);

   result= rb->menu_show(m);
   rb->menu_exit(m);
   if(result<items && result>=0)
      return result;
   return 0;
}

//
// Doom Menu
//
int doom_menu()
{
   int m;
   int result;
   int status;
   int gamever;
   bool menuquit=0;

   static struct opt_items names[7];

   static const struct menu_item items[] = {
      { "Game", NULL },
      { "Addons", NULL },
      { "Demos", NULL },
      { "Options", NULL },
      { "Play Game", NULL },
      { "Quit", NULL },
   };

   if( (status=Dbuild_base(names)) == 0 ) // Build up the base wad files (select last added file)
   {
      rb->splash(HZ*2, true, "Missing Base WAD!");
      return -2;
   }

   int numadd=Dbuild_filelistm(&addons, "No Addon", GAMEBASE"addons/", ".WAD" );

   int numdemos=Dbuild_filelistm(&demolmp, "No Demo", GAMEBASE"demos/", ".LMP" );

   argvlist.demonum=0;
   argvlist.addonnum=0;

   gamever=status-1;

    /* Clean out the button Queue */
    while (rb->button_get(false) != BUTTON_NONE) 
        rb->yield();

   m = rb->menu_init(items, sizeof(items) / sizeof(*items),
                NULL, NULL, NULL, NULL);

   while(!menuquit)
   {
      result=rb->menu_show(m);
      switch (result) {
         case 0: /* Game picker */
            rb->set_option("Game WAD", &gamever, INT, names, status, NULL );
            break;

         case 1: /* Addon picker */
            argvlist.addonnum=menuchoice(addons,numadd);
            break;

         case 2: /* Demos */
            argvlist.demonum=menuchoice(demolmp,numdemos);
            break;

         case 3: /* Options */
            Doptions();
            break;

         case 4: /* Play Game */
            menuquit=1;
            break;

         case 5: /* Quit */
            menuquit=1;
            gamever=-1;
            break;

         default:
            break;
      }
   }

   rb->menu_exit(m);

   return (gamever);
}

extern int systemvol;
/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
   PLUGIN_IRAM_INIT(api)

   rb = api;
   (void)parameter;

   doomexit=0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
   rb->cpu_boost(true);
#endif

   rb->lcd_setfont(0);

   // We're using doom's memory management since it implements a proper free (and re-uses the memory)
   // and now with prboom's code: realloc and calloc
   printf ("Z_Init: Init zone memory allocation daemon.\n");
   Z_Init ();

   printf ("M_LoadDefaults: Load system defaults.\n");
   M_LoadDefaults ();              // load before initing other systems

   rb->splash(HZ*2, true, "Welcome to RockDoom");

   myargv =0;
   myargc=0;

   rb->lcd_clear_display();

   int result = doom_menu();
   if (result < 0)
   {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
       if( result == -1 ) return PLUGIN_OK; // Quit was selected
       else if( result == -2 ) return PLUGIN_ERROR; // Missing base wads
   }

   Dhandle_ver( namemap[ result ] );

   rb->lcd_setfont(0);

   rb->lcd_clear_display();

   systemvol= rb->global_settings->volume-rb->global_settings->volume%((rb->sound_max(SOUND_VOLUME)-rb->sound_min(SOUND_VOLUME))/15);
   general_translucency = default_translucency;                    // phares

#ifdef RB_PROFILE
   rb->profile_thread();
#endif

   rb->lcd_set_backdrop(NULL);

   D_DoomMain ();

#ifdef RB_PROFILE
   rb->profstop();
#endif

   M_SaveDefaults ();

   I_Quit(); // Make SURE everything was closed out right

   printf("There were still: %d files open\n", fpoint);
   while(fpoint>0)
   {
      rb->close(filearray[fpoint]);
      fpoint--;
   }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
   rb->cpu_boost(false);
#endif

   return PLUGIN_OK;
}
