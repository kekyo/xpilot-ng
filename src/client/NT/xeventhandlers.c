/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xpclient_x11.h"

#ifdef DEVELOPMENT
time_t	back_in_play_since;
#endif

#ifndef _WINDOWS
/*
 * code for the following three functions and the selectionEvents
 * happily and with benediction taken from the terminal emulator
 * 'rxvt-2.6Pre2' (GNU) maintained by Geoff Wing <gcw@pobox.com>.
 * (modified)
*/
static void Selection_paste(Window win, unsigned prop, int Delete)
{
    long            nread;
    unsigned long   bytes_after, nitems;
    unsigned char  *data;
    Atom            actual_type;
    int             actual_fmt;

    if (prop == None)
        return;

    for (nread = 0, bytes_after = 1; bytes_after > 0; nread += nitems) {
        if ((XGetWindowProperty(dpy, win, prop, (nread / 4), MAX_CHARS / 4,
                                Delete, AnyPropertyType, &actual_type,
                                &actual_fmt, &nitems, &bytes_after,
                                &data) != Success)) {
            XFree(data);
            return;
        }
        if (Talk_paste((char*)data, nitems, False) == 0)
	   /* talk window doesn't accept text anymore */
	    return;
	else
	    save_talk_str = true;
        XFree(data);
    }
}

/*
 * we want a selection
 */
static void Selection_request(void)
{
    Atom prop;

    if (selection.txt != NULL) {
	/* internal selection */
	if (Talk_paste(selection.txt, selection.len, False) > 0)
	    save_talk_str = true;
    }
    else if (XGetSelectionOwner(dpy, XA_PRIMARY) == None)
	Selection_paste(DefaultRootWindow(dpy), XA_CUT_BUFFER0, False);
    else {
	prop = XInternAtom(dpy, "VT_SELECTION", False);
	XConvertSelection(dpy, XA_PRIMARY, XA_STRING, prop, talkWindow,
			    CurrentTime);
	/* the selectionNotify event 'will do the rest' */
    }
}

/*
 * our selection is requested
 */
static void Selection_send(const XSelectionRequestEvent *rq)
{
    XEvent          ev;
    Atom32          target_list[2];
    static Atom     xa_targets = None;

    if (xa_targets == None)
        xa_targets = XInternAtom(dpy, "TARGETS", False);

    ev.xselection.type = SelectionNotify;
    ev.xselection.property = None;
    ev.xselection.display = rq->display;
    ev.xselection.requestor = rq->requestor;
    ev.xselection.selection = rq->selection;
    ev.xselection.target = rq->target;
    ev.xselection.time = rq->time;

    if (rq->target == xa_targets) {
        target_list[0] = (Atom32) xa_targets;
        target_list[1] = (Atom32) XA_STRING;
        XChangeProperty(dpy, rq->requestor, rq->property, rq->target,
                        (8 * sizeof(target_list[0])), PropModeReplace,
                        (unsigned char *)target_list,
                        (sizeof(target_list) / sizeof(target_list[0])));
        ev.xselection.property = rq->property;
    }
    else if (rq->target == XA_STRING) {
	XChangeProperty(dpy, rq->requestor, rq->property,
			rq->target, 8, PropModeReplace,
			(unsigned char *) selection.txt, (int)selection.len);
	ev.xselection.property = rq->property;
    }
    XSendEvent(dpy, rq->requestor, False, 0, &ev);
}

void SelectionNotify_event(XEvent *event)
{
    Selection_paste(event->xselection.requestor,
		    event->xselection.property, True);
}

void SelectionRequest_event(XEvent *event)
{
    Selection_send(&(event->xselectionrequest));
}

void MapNotify_event(XEvent *event)
{
    UNUSED_PARAM(event);
    if (ignoreWindowManager == 1) {
        XSetInputFocus(dpy, topWindow, RevertToParent, CurrentTime);
        ignoreWindowManager = 2;
    }
}


/*
 * On HP-UX 10.20 with CDE strange things happen
 * sometimes when closing xpilot via the window
 * manager.  Keypresses may result in funny characters
 * after the client exits.  The remedy to this seems
 * to be to explicitly destroy the top window with
 * XDestroyWindow when the window manager asks the
 * client to quit and then wait for the resulting
 * DestroyNotify event before closing the connection
 * with the X server.
 */
int ClientMessage_event(XEvent *event)
{
    XClientMessageEvent	*cmev;
    cmev = (XClientMessageEvent *)event;
    if (cmev->message_type == ProtocolAtom
	&& cmev->format == 32
	&& (unsigned)cmev->data.l[0] == KillAtom) {
        XDestroyWindow(dpy, topWindow);
	XSync(dpy, True);
	printf("Quit\n");
	return -1;
    }
    return 0;
}

void FocusIn_event(XEvent *event)
{
    UNUSED_PARAM(event);
#ifdef DEVELOPMENT
    if (!gotFocus)
        time(&back_in_play_since);
#endif
#if 0  /* kps - this is probably not useful any more */
    if (clData.restorePointerControl && !clData.talking) {
	Pointer_control_set_state(true);
	clData.restorePointerControl = false;
    }
#endif
    gotFocus = true;
    XAutoRepeatOff(dpy);
}

void UnmapNotify_event(XEvent *event)
{
    UNUSED_PARAM(event);
#if 0  /* kps - this is probably not useful any more */
    if (clData.pointerControl) {
        clData.restorePointerControl = true;
        Pointer_control_set_state(false);
    }
#endif
    gotFocus = false;
    XAutoRepeatOn(dpy);
}

void ConfigureNotify_event(XEvent *event)
{
    XConfigureEvent	*conf;
    static unsigned int conf_width = 0;
    static unsigned int conf_height = 0;
   
    /* Changed to check if this is a window move or a 
       window resize event , sadly ConfigureNotify and
       Expose are used to resize windows, if the window
       size has not changed then we do not need to destroy
       and resize widgets */
    
    conf = &(event->xconfigure);
    
    if (((unsigned) conf->width != conf_width) || 
	((unsigned) conf->height != conf_height))
      {
	Resize(conf->window, (unsigned)conf->width, (unsigned)conf->height);  
	
	conf_height = (unsigned)conf->height;
	conf_width = (unsigned)conf->width;	
      }
    else
      {
	Widget_event(event); 
      }
}
#endif

void KeyChanged_event(XEvent *event)
{
#ifdef DEVELOPMENT
    if (back_in_play_since) {
        time_t now = time(NULL);
	if (now - back_in_play_since > 0)
	    back_in_play_since = 0;
	else
	    /* after popup ignore key events for 1 seconds. */
	    return;
    }
#endif
    if (event->xkey.window == topWindow)
        Key_event(event);
    else if (event->xkey.window == talkWindow) {
        if (event->type == KeyPress) {
	    talk_key_repeating = 1;
	    gettimeofday(&talk_key_repeat_time, NULL);
	    talk_key_repeat_event = *event;
	}
	else if (talk_key_repeating && event->xkey.keycode ==
		 talk_key_repeat_event.xkey.keycode)
	    talk_key_repeating = 0;

	Talk_event(event);
	if (!clData.talking)
	    talk_key_repeating = 0;
    }
	/* else : here we can add widget.c key uses. */
}

void ButtonPress_event(XEvent *event)
{
    if (event->xbutton.window == drawWindow
	|| event->xbutton.window == talkWindow) {
        if (clData.pointerControl
	    && !clData.talking
	    && event->xbutton.button <= MAX_POINTER_BUTTONS)
	    Pointer_button_pressed((int)event->xbutton.button);

	return;
    }
    if (Widget_event(event) != 0)
        return;
    Expose_button_window(BLACK, event->xbutton.window);
}

void MotionNotify_event(XEvent *event)
{
    if (event->xmotion.window == drawWindow) {
        if (clData.pointerControl) {
	    if (!clData.talking) {
	        if (!event->xmotion.send_event)
		    mouseMovement += event->xmotion.x - mousePosition.x;
	    }
	    mousePosition.x = event->xmotion.x;
	    mousePosition.y = event->xmotion.y;
	}
    } else
        Widget_event(event);
}

int ButtonRelease_event(XEvent *event)
{
    if (event->xbutton.window == drawWindow
	|| event->xbutton.window == talkWindow) {

        if (clData.pointerControl
	    && !clData.talking
	    && event->xbutton.button <= MAX_POINTER_BUTTONS)
	    Pointer_button_released((int)event->xbutton.button);

	return 0;
    }
    if (Widget_event(event) != 0) {
	if (quitting) {
	    quitting = false;
	    printf("Quit\n");
	    return -1;
	}
	return 0;
    }
    Expose_button_window(buttonColor ? buttonColor: RED,
			 event->xbutton.window);
    if (event->xbutton.window == about_close_b)
	About(about_close_b);
    else if (event->xbutton.window == about_next_b)
	About(about_next_b);
    else if (event->xbutton.window == about_prev_b)
	About(about_prev_b);
    return 0;
}

void Expose_event(XEvent *event)
{
    if (event->xexpose.window == playersWindow) {
        if (event->xexpose.count == 0) {
	    players_exposed = true;
	    scoresChanged++;
	}
    }
    else if (event->xexpose.window == aboutWindow) {
	if (event->xexpose.count == 0)
	    Expose_about_window();
    }
    else if (event->xexpose.window == radarWindow) {
	if (event->xexpose.count <= 1)
	    radar_exposures = 1;
	else
	    radar_exposures++;
    }
    else if (event->xexpose.window == talkWindow) {
	if (event->xexpose.count == 0) {
	    Talk_event(event);
	    if (!clData.talking)
		talk_key_repeating = 0;
	}
    }
    else if (Widget_event(event) == 0) {
	if (event->xexpose.count == 0)
	    Expose_button_window(buttonColor ? buttonColor : RED,
				 event->xexpose.window);
    }
}
