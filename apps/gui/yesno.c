/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id $
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

#include "yesno.h"
#include "system.h"
#include "kernel.h"
#include "misc.h"
#include "lang.h"
#include "action.h"

/*
 * Initializes the yesno asker
 *  - yn : the yesno structure
 *  - main_message : the question the user has to answer
 *  - yes_message : message displayed if answer is 'yes'
 *  - no_message : message displayed if answer is 'no'
 */
static void gui_yesno_init(struct gui_yesno * yn,
                    struct text_message * main_message,
                    struct text_message * yes_message,
                    struct text_message * no_message)
{
    yn->main_message=main_message;
    yn->result_message[YESNO_YES]=yes_message;
    yn->result_message[YESNO_NO]=no_message;
    yn->display=0;
}

/*
 * Attach the yesno to a screen
 *  - yn : the yesno structure
 *  - display : the screen to attach
 */
static void gui_yesno_set_display(struct gui_yesno * yn,
                           struct screen * display)
{
    yn->display=display;
}

/*
 * Draws the yesno
 *  - yn : the yesno structure
 */
static void gui_yesno_draw(struct gui_yesno * yn)
{
    struct screen * display=yn->display;
    int nb_lines, line_shift=0;

    gui_textarea_clear(display);
    nb_lines=yn->main_message->nb_lines;

    if(nb_lines+3<display->nb_lines)
        line_shift=1;
    nb_lines=gui_textarea_put_message(display, yn->main_message, line_shift);

    /* Space remaining for yes / no text ? */
    if(nb_lines+line_shift+2<=display->nb_lines)
    {
        if(nb_lines+line_shift+3<=display->nb_lines)
            nb_lines++;
        display->puts(0, nb_lines+line_shift, str(LANG_CONFIRM_WITH_PLAY_RECORDER));
        display->puts(0, nb_lines+line_shift+1, str(LANG_CANCEL_WITH_ANY_RECORDER));
    }
    gui_textarea_update(display);
}

/*
 * Draws the yesno result
 *  - yn : the yesno structure
 *  - result : the result tha must be displayed :
 *    YESNO_NO if no
 *    YESNO_YES if yes
 */
static bool gui_yesno_draw_result(struct gui_yesno * yn, enum yesno_res result)
{
    struct text_message * message=yn->result_message[result];
    if(message==NULL)
        return false;
    gui_textarea_put_message(yn->display, message, 0);
    return(true);
}
#include "debug.h"
enum yesno_res gui_syncyesno_run(struct text_message * main_message,
                                 struct text_message * yes_message,
                                 struct text_message * no_message)
{
    int i;
    unsigned button;
    int result=-1;
    bool result_displayed;
    struct gui_yesno yn[NB_SCREENS];
    FOR_NB_SCREENS(i)
    {
        gui_yesno_init(&(yn[i]), main_message, yes_message, no_message);
        gui_yesno_set_display(&(yn[i]), &(screens[i]));
        gui_yesno_draw(&(yn[i]));
    }
    action_signalscreenchange();
    while (result==-1)
    {
        button = get_action(CONTEXT_YESNOSCREEN,TIMEOUT_BLOCK);
        switch (button)
        {
            case ACTION_YESNO_ACCEPT:
                result=YESNO_YES;
                break;
            case ACTION_NONE:
                continue;
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return(YESNO_USB);
                result = YESNO_NO;
        }
    }
    action_signalscreenchange();
    FOR_NB_SCREENS(i)
        result_displayed=gui_yesno_draw_result(&(yn[i]), result);
    if(result_displayed)
        sleep(HZ);
    return(result);
}
