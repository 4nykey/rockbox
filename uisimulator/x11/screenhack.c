/* xscreensaver, Copyright (c) 1992, 1995, 1997, 1998
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * And remember: X Windows is to graphics hacking as roman numerals are to
 * the square root of pi.
 */

/* This file contains simple code to open a window or draw on the root.
   The idea being that, when writing a graphics hack, you can just link
   with this .o to get all of the uninteresting junk out of the way.

   -  create a procedure `screenhack(dpy, window)'

   -  create a variable `char *progclass' which names this program's
      resource class.

   -  create a variable `char defaults []' for the default resources, and
      null-terminate it.

   -  create a variable `XrmOptionDescRec options[]' for the command-line,
      and null-terminate it.

   And that's it...
 */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef __sgi
# include <X11/SGIScheme.h>	/* for SgiUseSchemes() */
#endif /* __sgi */

#ifdef HAVE_XMU
# ifndef VMS
#  include <X11/Xmu/Error.h>
# else /* VMS */
#  include <Xmu/Error.h>
# endif
#else
# include "xmu.h"
#endif
#include "screenhack.h"
#include "version.h"
#include "vroot.h"

#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif

char having_new_lcd=True;

char *progname;
XrmDatabase db;
XtAppContext app;
Bool mono_p;

static XrmOptionDescRec default_options [] = {
  { "-root",	".root",		XrmoptionNoArg, "True" },
  { "-window",	".root",		XrmoptionNoArg, "False" },
  { "-mono",	".mono",		XrmoptionNoArg, "True" },
  { "-install",	".installColormap",	XrmoptionNoArg, "True" },
  { "-noinstall",".installColormap",	XrmoptionNoArg, "False" },
  { "-visual",	".visualID",		XrmoptionSepArg, 0 },
  { "-window-id", ".windowID",		XrmoptionSepArg, 0 },
  { 0, 0, 0, 0 }
};

static char *default_defaults[] = {
  ".root:		false",
  "*geometry:		"
#ifdef HAVE_LCD_BITMAP
  "120x68"
#else
  "280x132" /* A bit larger that necessary */
#endif
  , /* this should be .geometry, but nooooo... */
  "*mono:		false",
  "*installColormap:	false",
  "*visualID:		default",
  "*windowID:		",
  0
};

extern Display* dpy;

static XrmOptionDescRec *merged_options;
static int merged_options_size;
static char **merged_defaults;

static void merge_options (void)
{
    int def_opts_size, opts_size;
    int def_defaults_size, defaults_size;

    for (def_opts_size = 0; default_options[def_opts_size].option;
         def_opts_size++)
        ;
    for (opts_size = 0; options[opts_size].option; opts_size++)
        ;

    merged_options_size = def_opts_size + opts_size;
    merged_options = (XrmOptionDescRec *)
        malloc ((merged_options_size + 1) * sizeof(*default_options));
    memcpy (merged_options, default_options,
            (def_opts_size * sizeof(*default_options)));
    memcpy (merged_options + def_opts_size, options,
            ((opts_size + 1) * sizeof(*default_options)));

    for (def_defaults_size = 0; default_defaults[def_defaults_size];
         def_defaults_size++)
        ;
    for (defaults_size = 0; defaults[defaults_size]; defaults_size++)
        ;

    merged_defaults = (char **)
        malloc ((def_defaults_size + defaults_size + 1) * sizeof (*defaults));

    memcpy (merged_defaults, default_defaults,
            def_defaults_size * sizeof(*defaults));
    memcpy (merged_defaults + def_defaults_size, defaults,
            (defaults_size + 1) * sizeof(*defaults));

    /* This totally sucks.  Xt should behave like this by default.
       If the string in `defaults' looks like ".foo", change that
       to "Progclass.foo".
    */
    {
        char **s;
        for (s = merged_defaults; *s; s++)
            if (**s == '.')
            {
                const char *oldr = *s;
                char *newr = (char *) malloc(strlen(oldr) 
                                             + strlen(progclass) + 3);
                strcpy (newr, progclass);
                strcat (newr, oldr);
                *s = newr;
            }
    }
}


/* Make the X errors print out the name of this program, so we have some
   clue which one has a bug when they die under the screensaver.
 */

static int screenhack_ehandler (Display *dpy, XErrorEvent *error)
{
    fprintf (stderr, "\nX error in %s:\n", progname);
    if (XmuPrintDefaultErrorMessage (dpy, error, stderr))
        exit (-1);
    else
        fprintf (stderr, " (nonfatal.)\n");
    return 0;
}

static Bool MapNotify_event_p (Display *dpy, XEvent *event, XPointer window)
{
    (void)dpy;
    return (event->xany.type == MapNotify &&
            event->xvisibility.window == (Window) window);
}

static Atom XA_WM_PROTOCOLS, XA_WM_DELETE_WINDOW;

/* Dead-trivial event handling: exits if "q" or "ESC" are typed.
   Exit if the WM_PROTOCOLS WM_DELETE_WINDOW ClientMessage is received.
 */
int screenhack_handle_event(Display *dpy, XEvent *event, bool *release)
{
    int key=0;

    *release = FALSE;

    switch (event->xany.type) {
    case KeyPress:
      {
          KeySym keysym;
          unsigned char c = 0;
          XLookupString (&event->xkey, &c, 1, &keysym, 0);
          key = keysym;
          /*	    fprintf(stderr, "KEY PRESSED: %c (%02x)\n", c, c); */
      }
      break;
    case KeyRelease:
      {
          KeySym keysym;
          unsigned char c = 0;
          XLookupString (&event->xkey, &c, 1, &keysym, 0);
          key = keysym;
          /* fprintf(stderr, "KEY RELEASED: %c (%02x) %x\n", c, c,
             event->xkey.keycode); */

          *release = TRUE;
      }
      break;
    case Expose:
      {
	/*
	int x=event->xexpose.width+event->xexpose.x;
	int y=event->xexpose.height+event->xexpose.y;
	screen_resized(x, y);
	fprintf(stderr, "WINDOW RESIZED to width %d height %d\n",
		x, y);
	*/
	screen_redraw();
      }
      break;
    default:
      break;
    case ButtonPress:
      fprintf(stderr, "BUTTON PRESSED: x: %d y:%d\n",event->xbutton.x,event->xbutton.y);
      break;
    case ClientMessage:
      {
          if (event->xclient.message_type != XA_WM_PROTOCOLS) {
              char *s = XGetAtomName(dpy, event->xclient.message_type);
              if (!s) s = "(null)";
              fprintf (stderr, "%s: unknown ClientMessage %s received!\n",
                       progname, s);
          }
          else if (event->xclient.data.l[0] != (int)XA_WM_DELETE_WINDOW) {
              char *s1 = XGetAtomName(dpy, event->xclient.message_type);
              char *s2 = XGetAtomName(dpy, event->xclient.data.l[0]);
              if (!s1) s1 = "(null)";
              if (!s2) s2 = "(null)";
              fprintf (stderr, "%s: unknown ClientMessage %s[%s] received!\n",
                       progname, s1, s2);
          }
          else {
              exit (0);
          }
      }
      break;
    }
    return key;
}


int screenhack_handle_events(bool *release)
{
    int key=0;
    while (XPending(dpy))
    {
        XEvent event;
        XNextEvent(dpy, &event);
        key=screenhack_handle_event(dpy, &event, release);
    }
    return key;
}


static Visual *
pick_visual (Screen *screen)
{
#ifdef USE_GL
    /* If we're linking against GL (that is, this is the version of 
       screenhack.o that the GL hacks will use, which is different from the 
       one that the non-GL hacks will use) then try to pick the "best" visual 
       by interrogating the GL library instead of by asking Xlib.  GL knows 
       better.
    */
    Visual *v = 0;
    char *string = get_string_resource ("visualID", "VisualID");
    char *s;

    if (string)
        for (s = string; *s; s++)
            if (isupper (*s)) *s = _tolower (*s);

    if (!string || !*string ||
        !strcmp (string, "gl") ||
        !strcmp (string, "best") ||
        !strcmp (string, "color") ||
        !strcmp (string, "default"))
        v = get_gl_visual (screen);		/* from ../utils/visual-gl.c */

    if (string)
        free (string);
    if (v)
        return v;
#endif /* USE_GL */

    return get_visual_resource (screen, "visualID", "VisualID", False);
}

int main (int argc, char **argv)
{
    Widget toplevel;
    Display *dpy;
    Window window;
    Screen *screen;
    Visual *visual;
    Colormap cmap;
    XEvent event;
    char version[255];

    sprintf(version,"rockboxui %s",ROCKBOXUI_VERSION);
    merge_options ();

#ifdef __sgi
    /* We have to do this on SGI to prevent the background color from being
       overridden by the current desktop color scheme (we'd like our 
       backgrounds to be black, thanks.)  This should be the same as setting 
       the "*useSchemes: none" resource, but it's not -- if that resource is
       present in the `default_defaults' above, it doesn't work, though it
       does work when passed as an -xrm arg on the command line.  So screw it,
       turn them off from C instead.
    */
    SgiUseSchemes ("none"); 
#endif /* __sgi */

    toplevel = XtAppInitialize (&app, progclass, merged_options,
                                merged_options_size, &argc, argv,
                                merged_defaults, 0, 0);
    dpy = XtDisplay (toplevel);
    screen = XtScreen (toplevel);
    db = XtDatabase (dpy);

    XtGetApplicationNameAndClass (dpy, &progname, &progclass);

    /* half-assed way of avoiding buffer-overrun attacks. */
    if (strlen (progname) >= 100) progname[100] = 0;

    XSetErrorHandler (screenhack_ehandler);

    XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
    XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);


    if (argc > 1)
    {
      int x;
      for (x=1; x<argc; x++) {
	if (!strcmp("--old_lcd", argv[x])) {
	  having_new_lcd=FALSE;
	  printf("Using old LCD layout.\n");
	} else {
	  printf("rockboxui\n");
	  printf("Arguments:\n");
	  printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
	  exit(0);
	}
      }
    }

    if (CellsOfScreen (DefaultScreenOfDisplay (dpy)) <= 2)
        mono_p = True;

    {
        Boolean def_visual_p;
        visual = pick_visual (screen);

        if (toplevel->core.width <= 0)
            toplevel->core.width = 600;
        if (toplevel->core.height <= 0)
            toplevel->core.height = 480;

        def_visual_p = (visual == DefaultVisualOfScreen (screen));

        if (!def_visual_p)
        {
            unsigned int bg, bd;
            Widget new;

            cmap = XCreateColormap (dpy, RootWindowOfScreen(screen),
                                    visual, AllocNone);
            bg = get_pixel_resource ("background", "Background", dpy, cmap);
            bd = get_pixel_resource ("borderColor", "Foreground", dpy, cmap);

            new = XtVaAppCreateShell (progname, progclass,
                                      topLevelShellWidgetClass, dpy,
                                      XtNmappedWhenManaged, False,
                                      XtNvisual, visual,
                                      XtNdepth, visual_depth (screen, visual),
                                      XtNwidth, toplevel->core.width,
                                      XtNheight, toplevel->core.height,
                                      XtNcolormap, cmap,
                                      XtNbackground, (Pixel) bg,
                                      XtNborderColor, (Pixel) bd,
                                      XtNinput, True,  /* for WM_HINTS */
                                      0);
            XtDestroyWidget (toplevel);
            toplevel = new;
            XtRealizeWidget (toplevel);
            window = XtWindow (toplevel);
        }
        else
        {
            XtVaSetValues (toplevel,
                           XtNmappedWhenManaged, False,
                           XtNinput, True,  /* for WM_HINTS */
                           0);
            XtRealizeWidget (toplevel);
            window = XtWindow (toplevel);

            if (get_boolean_resource ("installColormap", "InstallColormap"))
            {
                cmap = XCreateColormap (dpy, window,
                                        DefaultVisualOfScreen (XtScreen 
                                                               (toplevel)),
                                        AllocNone);
                XSetWindowColormap (dpy, window, cmap);
            }
            else
            {
                cmap = DefaultColormap (dpy, DefaultScreen (dpy));
            }
        }

        XtPopup (toplevel, XtGrabNone);

        XtVaSetValues(toplevel, XtNtitle, version, 0);

        /* For screenhack_handle_events(): select KeyPress, and
           announce that we accept WM_DELETE_WINDOW. */
        {
            XWindowAttributes xgwa;
            XGetWindowAttributes (dpy, window, &xgwa);
            XSelectInput (dpy, window, 
                          xgwa.your_event_mask | KeyPressMask | KeyRelease |
                          ButtonPressMask | ExposureMask);
            XChangeProperty (dpy, window, XA_WM_PROTOCOLS, XA_ATOM, 32,
                             PropModeReplace,
                             (unsigned char *) &XA_WM_DELETE_WINDOW, 1);
        }
    }

    XSetWindowBackground (dpy, window,
                          get_pixel_resource ("background", "Background",
                                              dpy, cmap));
    XClearWindow (dpy, window);

    /* wait for it to be mapped */
    XIfEvent (dpy, &event, MapNotify_event_p, (XPointer) window);

    XSync (dpy, False);

    screenhack (dpy, window); /* doesn't return */
    return 0;
}
