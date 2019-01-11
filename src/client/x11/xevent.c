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
#include "../xhacks.h"

int	talk_key_repeating;
XEvent	talk_key_repeat_event;
struct timeval talk_key_repeat_time;
static struct timeval time_now;

static ipos_t	delta;
ipos_t	mousePosition;	/* position of mouse pointer. */
int	mouseMovement;	/* horizontal mouse movement. */


keys_t Lookup_key(XEvent *event, KeySym ks, bool reset)
{
    keys_t ret = Generic_lookup_key((xp_keysym_t)ks, reset);

    UNUSED_PARAM(event);
  
#ifdef DEVELOPMENT
    if (reset && ret == KEY_DUMMY) {
	static XComposeStatus compose;
	char str[4];
	int count;

	memset(str, 0, sizeof str);
	count = XLookupString(&event->xkey, str, 1, &ks, &compose);
	if (count == NoSymbol)
	    warn("Unknown keysym: 0x%03lx.", ks);
	else {
	    warn("No action bound to keysym 0x%03lx.", ks);
	    if (*str)
		warn("(which is key \"%s\")", str);
	}
    }
#endif

    return ret;
}

void Platform_specific_pointer_control_set_state(bool on)
{
    assert(clData.pointerControl != on);

    if (on) {
	if (mouseAccelInClient)
	    XChangePointerControl(dpy, True, True,
				  new_acc_num, new_acc_denom, new_threshold);
	XGrabPointer(dpy, drawWindow, True, 0, GrabModeAsync,
		     GrabModeAsync, drawWindow, pointerControlCursor,
		     CurrentTime);
	XWarpPointer(dpy, None, drawWindow,
		     0, 0, 0, 0,
		     (int)draw_width/2, (int)draw_height/2);
	XDefineCursor(dpy, drawWindow, pointerControlCursor);
	XSelectInput(dpy, drawWindow,
		     PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
    } else {
	if (mouseAccelInClient && pre_exists)
	    XChangePointerControl(dpy, True, True, 
				  pre_acc_num, pre_acc_denom, pre_threshold);
	XUngrabPointer(dpy, CurrentTime);
	XDefineCursor(dpy, drawWindow, None);
	XSelectInput(dpy, drawWindow, ButtonPressMask | ButtonReleaseMask);
	XFlush(dpy);
    }
    Disable_emulate3buttons(on, dpy);
}

void Platform_specific_talk_set_state(bool on)
{
    assert(clData.talking != on);

    if (on) {
	XSelectInput(dpy, drawWindow,
		     PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
	Talk_map_window(true);
    }
    else
	Talk_map_window(false);
}

void Toggle_radar_and_scorelist(void)
{
    if (radar_score_mapped) {

	/* change the draw area to be the size of the window */
	draw_width = top_width;
	draw_height = top_height;

	/*
	 * We need to unmap the score and radar windows
	 * if config is mapped, leave it there its useful
	 * to have it popped up whilst in full screen
	 * the user can close it with "close"
	 */

	XUnmapWindow(dpy, radarWindow);
	XUnmapWindow(dpy, playersWindow);
	Widget_unmap(button_form);

	/* Move the draw area */
	XMoveWindow(dpy, drawWindow, 0, 0);

	/* Set the global variable to show that */
	/* the radar and score are now unmapped */
	radar_score_mapped = false;

	/* Generate resize event */
	Resize(topWindow, top_width, top_height);

    } else {

	/*
	 * We need to map the score and radar windows
	 * move the window back, note how 258 is a hard coded
	 * value in xinit.c, if they cant be bothered to declare
	 * a constant, neither can I - kps fix
	 */
	draw_width = top_width - (258);
	draw_height = top_height;

	XMoveWindow(dpy, drawWindow, 258, 0);
	Widget_map(button_form);
	XMapWindow(dpy, radarWindow);
	XMapWindow(dpy, playersWindow);

	/* reflect that we are remapped to the client */

	radar_score_mapped = true;
    }
}

void Toggle_fullscreen(void)
{
    return;
}

void Key_event(XEvent *event)
{
    KeySym ks;

    if ((ks = XLookupKeysym(&event->xkey, 0)) == NoSymbol)
	return;

    switch(event->type) {
    case KeyPress:
	Keyboard_button_pressed((xp_keysym_t)ks);
	break;
    case KeyRelease:
	Keyboard_button_released((xp_keysym_t)ks);
	break;
    default:
	return;
    }
}

void Talk_event(XEvent *event)
{
    if (!Talk_do_event(event))
	Talk_set_state(false);
}

static void Handle_talk_key_repeat(void)
{
    int i;

    if (talk_key_repeating) {
	gettimeofday(&time_now, NULL);
	i = 1000000 * (time_now.tv_sec - talk_key_repeat_time.tv_sec) +
	    time_now.tv_usec - talk_key_repeat_time.tv_usec;
	if ((talk_key_repeating > 1 && i > 50000) || i > 500000) {
	    Talk_event(&talk_key_repeat_event);
	    talk_key_repeating = 2;
	    talk_key_repeat_time = time_now;
	    if (!clData.talking)
		talk_key_repeating = 0;
	}
    }
}

void xevent_keyboard(int queued)
{
    int i, n;
    XEvent event;

    Handle_talk_key_repeat();

    if (kdpy) {
	n = XEventsQueued(kdpy, queued);
	for (i = 0; i < n; i++) {
	    XNextEvent(kdpy, &event);
	    switch (event.type) {
	    case KeyPress:
	    case KeyRelease:
		Key_event(&event);
		break;

		/* Back in play */
	    case FocusIn:
		gotFocus = true;
		XAutoRepeatOff(kdpy);
		break;

		/* Probably not playing now */
	    case FocusOut:
	    case UnmapNotify:
		gotFocus = false;
		XAutoRepeatOn(kdpy);
		break;

	    case MappingNotify:
		XRefreshKeyboardMapping(&event.xmapping);
		break;

	    default:
		warn("Unknown event type (%d) in xevent_keyboard",
		     event.type);
		break;
	    }
	}
    }
}

void xevent_pointer(void)
{
    XEvent event;

    if (!clData.pointerControl || clData.talking)
	return;

    if (mouseMovement != 0) {
	Client_pointer_move(mouseMovement);
	delta.x = draw_width / 2 - mousePosition.x;
	delta.y = draw_height / 2 - mousePosition.y;
	if (ABS(delta.x) > 3 * draw_width / 8
	    || ABS(delta.y) > 1 * draw_height / 8) {

	    memset(&event, 0, sizeof(event));
	    event.type = MotionNotify;
	    event.xmotion.display = dpy;
	    event.xmotion.window = drawWindow;
	    event.xmotion.x = draw_width/2;
	    event.xmotion.y = draw_height/2;
	    XSendEvent(dpy, drawWindow, False,
		       PointerMotionMask, &event);
	    XWarpPointer(dpy, None, drawWindow,
			 0, 0, 0, 0,
			 (int)draw_width/2, (int)draw_height/2);
	    XFlush(dpy);
	}
    }
}

int x_event(int new_input)
{
    int queued = 0, i, n;
    XEvent event;

#ifdef SOUND
    audioEvents();
#endif /* SOUND */

    mouseMovement = 0;

    switch (new_input) {
    case 0: queued = QueuedAlready; break;
    case 1: queued = QueuedAfterReading; break;
    case 2: queued = QueuedAfterFlush; break;
    default:
	warn("Bad input queue type (%d)", new_input);
	return -1;
    }
    n = XEventsQueued(dpy, queued);
    for (i = 0; i < n; i++) {
	XNextEvent(dpy, &event);

	switch (event.type) {
	    /*
	     * after requesting a selection we are notified that we
	     * can access it.
	     */
	case SelectionNotify:
	    SelectionNotify_event(&event);
	    break;
	    /*
	     * we are requested to provide a selection.
	     */
	case SelectionRequest:
	    SelectionRequest_event(&event);
	    break;

	case SelectionClear:
	    Clear_selection();
	    break;

	case MapNotify:
	    MapNotify_event(&event);
	    break;

	case ClientMessage:
	    if (ClientMessage_event(&event) == -1)
		return -1;
	    break;

	    /* Back in play */
	case FocusIn:
	    FocusIn_event(&event);
	    break;

	    /* Probably not playing now */
	case FocusOut:
	case UnmapNotify:
	    UnmapNotify_event(&event);
	    break;

	case MappingNotify:
	    XRefreshKeyboardMapping(&event.xmapping);
	    break;


	case ConfigureNotify:
	    ConfigureNotify_event(&event);
	    break;


	case KeyPress:
	    talk_key_repeating = 0;
	    /* FALLTHROUGH */
	case KeyRelease:
	    KeyChanged_event(&event);
	    break;

	case ButtonPress:
	    ButtonPress_event(&event);
	    break;

	case MotionNotify:
	    MotionNotify_event(&event);
	    break;

	case ButtonRelease:
	    if (ButtonRelease_event(&event) == -1)
	        return -1;
	    break;

	case Expose:
	    Expose_event(&event);
	    break;

	case EnterNotify:
	case LeaveNotify:
	    Widget_event(&event);
	    break;

	default:
	    break;
	}
    }

    xevent_keyboard(queued);
    xevent_pointer();
    return 0;
}
