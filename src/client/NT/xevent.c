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
    IFWINDOWS( Trace("Lookup_key: got key ks=%04X ret=%d\n", ks, ret) );
  
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
	XGrabPointer(dpy, drawWindow, true, 0, GrabModeAsync,
		     GrabModeAsync, drawWindow, pointerControlCursor,
		     CurrentTime);
	XWarpPointer(dpy, None, drawWindow,
		     0, 0, 0, 0,
		     (int)draw_width/2, (int)draw_height/2);
	XDefineCursor(dpy, drawWindow, pointerControlCursor);
	XSelectInput(dpy, drawWindow,
		     PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
    } else {
	XUngrabPointer(dpy, CurrentTime);
	XDefineCursor(dpy, drawWindow, None);
	XSelectInput(dpy, drawWindow, ButtonPressMask | ButtonReleaseMask);
	XFlush(dpy);
    }
}

void Platform_specific_talk_set_state(bool on)
{
    char *wintalkstr;

    assert(clData.talking != on);

    /* kps - this seems not to care so much about the value of 'on' ? */
    wintalkstr = (char *)mfcDoTalkWindow();
    if (*wintalkstr)
	Net_talk(wintalkstr);

    scoresChanged = true;
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
    Handle_talk_key_repeat();
}

void xevent_pointer(void)
{
    POINT point;

    if (!clData.pointerControl || clData.talking)
	return;

    GetCursorPos(&point);
    mouseMovement = point.x - draw_width/2;
    XWarpPointer(dpy, None, drawWindow,
		 0, 0, 0, 0,
		 draw_width/2, draw_height/2);

    if (mouseMovement != 0) {
	Send_pointer_move(mouseMovement);
	delta.x = draw_width / 2 - mousePosition.x;
	delta.y = draw_height / 2 - mousePosition.y;
	if (ABS(delta.x) > 3 * draw_width / 8
	    || ABS(delta.y) > 1 * draw_height / 8)
	    XFlush(dpy);
    }
}

int win_xevent(XEvent event)
{
    int queued = 0;

#ifdef SOUND
    audioEvents();
#endif /* SOUND */

    mouseMovement = 0;

    switch (event.type) {
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

    xevent_keyboard(queued);
    xevent_pointer();
    return 0;
}
