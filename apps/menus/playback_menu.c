
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "splash.h"
#include "settings.h"
#include "menu.h"
#include "sound_menu.h"
#include "kernel.h"
#include "playlist.h"
#include "dsp.h"
#include "scrobbler.h"
#include "audio.h"

#if CONFIG_CODEC == SWCODEC
int setcrossfadeonexit_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            audio_set_crossfade(global_settings.crossfade);
            break;
    }
    return action;
}

#endif /* CONFIG_CODEC == SWCODEC */

/***********************************/
/*    PLAYBACK MENU                */
int playback_callback(int action,const struct menu_item_ex *this_item);
    
MENUITEM_SETTING(shuffle_item, &global_settings.playlist_shuffle, playback_callback);
MENUITEM_SETTING(repeat_mode, &global_settings.repeat_mode, playback_callback);
MENUITEM_SETTING(play_selected, &global_settings.play_selected, NULL);
MENUITEM_SETTING(resume, &global_settings.resume, NULL);

MENUITEM_SETTING(ff_rewind_accel, &global_settings.ff_rewind_accel, NULL);
MENUITEM_SETTING(ff_rewind_min_step, &global_settings.ff_rewind_min_step, NULL);
MAKE_MENU(ff_rewind_settings_menu, ID2P(LANG_WIND_MENU), 0,
          &ff_rewind_min_step, &ff_rewind_accel);
#if CONFIG_CODEC == SWCODEC
int buffermargin_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            audio_set_buffer_margin(global_settings.buffer_margin);
            break;
    }
    return action;
}
#else 
# define buffermargin_callback NULL
#endif
MENUITEM_SETTING(buffer_margin, &global_settings.buffer_margin,
                 buffermargin_callback);
MENUITEM_SETTING(fade_on_stop, &global_settings.fade_on_stop, NULL);
MENUITEM_SETTING(party_mode, &global_settings.party_mode, NULL);

#if CONFIG_CODEC == SWCODEC
/* crossfade submenu */
MENUITEM_SETTING(crossfade, &global_settings.crossfade, setcrossfadeonexit_callback);
MENUITEM_SETTING(crossfade_fade_in_delay,
    &global_settings.crossfade_fade_in_delay, setcrossfadeonexit_callback);
MENUITEM_SETTING(crossfade_fade_in_duration,
    &global_settings.crossfade_fade_in_duration, setcrossfadeonexit_callback);
MENUITEM_SETTING(crossfade_fade_out_delay,
    &global_settings.crossfade_fade_out_delay, setcrossfadeonexit_callback);
MENUITEM_SETTING(crossfade_fade_out_duration,
    &global_settings.crossfade_fade_out_duration, setcrossfadeonexit_callback);
MENUITEM_SETTING(crossfade_fade_out_mixmode,
    &global_settings.crossfade_fade_out_mixmode,NULL);
MAKE_MENU(crossfade_settings_menu,ID2P(LANG_CROSSFADE),0,
          &crossfade, &crossfade_fade_in_delay, &crossfade_fade_in_duration,
          &crossfade_fade_out_delay, &crossfade_fade_out_duration,
          &crossfade_fade_out_mixmode);

/* replay gain submenu */

int replaygain_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            dsp_set_replaygain(true);
            break;
    }
    return action;
}
MENUITEM_SETTING(replaygain, &global_settings.replaygain ,replaygain_callback);
MENUITEM_SETTING(replaygain_noclip, &global_settings.replaygain_noclip ,replaygain_callback);
MENUITEM_SETTING(replaygain_type, &global_settings.replaygain_type ,replaygain_callback);
MENUITEM_SETTING(replaygain_preamp, &global_settings.replaygain_preamp ,replaygain_callback);
MAKE_MENU(replaygain_settings_menu,ID2P(LANG_REPLAYGAIN),0,
          &replaygain,&replaygain_noclip,
          &replaygain_type,&replaygain_preamp);
          
MENUITEM_SETTING(beep, &global_settings.beep ,NULL);
#endif /* CONFIG_CODEC == SWCODEC */

#ifdef HAVE_SPDIF_POWER
MENUITEM_SETTING(spdif_enable, &global_settings.spdif_enable, NULL);
#endif
MENUITEM_SETTING(id3_v1_first, &global_settings.id3_v1_first, NULL);
MENUITEM_SETTING(next_folder, &global_settings.next_folder, NULL);
int audioscrobbler_callback(int action,const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM: /* on exit */
            if (!scrobbler_is_enabled() && global_settings.audioscrobbler)
                gui_syncsplash(HZ*2, true, str(LANG_PLEASE_REBOOT));
        
            if(scrobbler_is_enabled() && !global_settings.audioscrobbler)
                scrobbler_shutdown();
            break;
    }
    return action;
}
MENUITEM_SETTING(audioscrobbler, &global_settings.audioscrobbler, audioscrobbler_callback);

#ifdef HAVE_HEADPHONE_DETECTION
MENUITEM_SETTING(unplug_mode, &global_settings.unplug_mode, NULL);
MENUITEM_SETTING(unplug_rw, &global_settings.unplug_rw, NULL);
MENUITEM_SETTING(unplug_autoresume, &global_settings.unplug_autoresume, NULL);
MAKE_MENU(unplug_menu, ID2P(LANG_UNPLUG), 0,
          &unplug_mode, &unplug_rw, &unplug_autoresume);
#endif

MAKE_MENU(playback_menu_item,ID2P(LANG_PLAYBACK),0,
          &shuffle_item, &repeat_mode, &play_selected, &resume,
          &ff_rewind_settings_menu, 
          &buffer_margin, &fade_on_stop, &party_mode,
          
#if CONFIG_CODEC == SWCODEC
          &crossfade_settings_menu, &replaygain_settings_menu, &beep,
#endif

#ifdef HAVE_SPDIF_POWER
          &spdif_enable,
#endif
          &id3_v1_first, &next_folder, &audioscrobbler
#ifdef HAVE_HEADPHONE_DETECTION
         ,&unplug_menu
#endif
         );
int playback_callback(int action,const struct menu_item_ex *this_item)
{
    static bool old_shuffle = false;
    static int old_repeat_mode = 0;
    (void)this_item;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            if (this_item == &shuffle_item)
                old_shuffle = global_settings.playlist_shuffle;
            else if (this_item == &repeat_mode)
                old_repeat_mode = global_settings.repeat_mode;
            break;
        case ACTION_EXIT_MENUITEM: /* on exit */
            if ((this_item == &shuffle_item) &&
                (old_shuffle != global_settings.playlist_shuffle)
                && (audio_status() & AUDIO_STATUS_PLAY))
            {
#if CONFIG_CODEC == SWCODEC
                dsp_set_replaygain(true);
#endif
                if (global_settings.playlist_shuffle)
                {
                    playlist_randomise(NULL, current_tick, true);
                }
                else
                {
                    playlist_sort(NULL, true);
                }
            }
            break;
    }
    return action;
}
/*    PLAYBACK MENU                */
/***********************************/
