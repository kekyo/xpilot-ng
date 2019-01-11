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

/* Information window dimensions */
#define TALK_TEXT_HEIGHT	(talkFont->ascent + talkFont->descent)
#define TALK_OUTSIDE_BORDER	2
#define TALK_INSIDE_BORDER	3
#define TALK_WINDOW_HEIGHT	(unsigned)\
				(TALK_TEXT_HEIGHT + 2 * TALK_INSIDE_BORDER)
#define TALK_WINDOW_X		(50 - TALK_OUTSIDE_BORDER)
#define TALK_WINDOW_Y		((int)(draw_height*3/4 - TALK_WINDOW_HEIGHT/2))
#define TALK_WINDOW_WIDTH	(draw_width \
				    - 2*(TALK_WINDOW_X + TALK_OUTSIDE_BORDER))

#undef CTRL
#define CTRL(c)			((c) & 0x1F)

/*
 * Globals.
 */

static bool		talk_created;
static char		talk_str[MAX_CHARS];
static struct {
    bool		visible;
    int			offset;
    size_t		point;
} talk_cursor;

/* position in history when browsing in the talk window */
static int		history_pos = 0;

/* faster cursor movement in talk window after pressing a key for some time */
#define CRS_START_HOPPING	7
#define CRS_HOP			4

/* selections in draw and talk window */
selection_t		selection;
bool			save_talk_str = false; /* see Get_msg_from_history */

static void Talk_create_window(void)
{
    /*
     * Create talk window.
     */
    talkWindow
	= XCreateSimpleWindow(dpy, drawWindow,
			      TALK_WINDOW_X, TALK_WINDOW_Y,
			      TALK_WINDOW_WIDTH, TALK_WINDOW_HEIGHT,
			      TALK_OUTSIDE_BORDER, colors[WHITE].pixel,
			      colors[BLACK].pixel);

    XSelectInput(dpy, talkWindow, ButtonPressMask | ButtonReleaseMask |
		 KeyPressMask | KeyReleaseMask | ExposureMask);
}


void Talk_cursor(bool visible)
{
    if (clData.talking == false || visible == talk_cursor.visible)
	return;

    if (visible == false) {
	XSetForeground(dpy, talkGC, colors[BLACK].pixel);
	XDrawString(dpy, talkWindow, talkGC,
		    talk_cursor.offset + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    "_", 1);
	XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	if (talk_cursor.point < strlen(talk_str)) {
	    /* cursor _in message */
	    if (selection.talk.state == SEL_EMPHASIZED
		&& talk_cursor.point >= selection.talk.x1
		&& talk_cursor.point < selection.talk.x2)
		/* cursor in a selection? redraw the character emphasized */
		XSetForeground(dpy, talkGC, colors[DRAW_EMPHASIZED].pixel);
	    XDrawString(dpy, talkWindow, talkGC,
			talk_cursor.offset + TALK_INSIDE_BORDER,
			talkFont->ascent + TALK_INSIDE_BORDER,
			&talk_str[talk_cursor.point], 1);
	    XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	}
	talk_cursor.visible = false;
    } else {
	/* visible */
	talk_cursor.offset = XTextWidth(talkFont, talk_str,
					(int)talk_cursor.point);
	/*
	 * goodie: 'inverse' cursor (an underscore) if there is already an
	 * unemphasized underscore
	 */
	if (talk_cursor.point < strlen(talk_str)
	    && talk_str[talk_cursor.point] == '_'
	    && ( selection.talk.state != SEL_EMPHASIZED
		|| talk_cursor.point < selection.talk.x1
		|| talk_cursor.point >= selection.talk.x2) )
	    XSetForeground(dpy, talkGC, colors[DRAW_EMPHASIZED].pixel);

	XDrawString(dpy, talkWindow, talkGC,
		    talk_cursor.offset + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    "_", 1);
	talk_cursor.visible = true;
    }
}


void Talk_map_window(bool map)
{
    static Window	root;
    static int		root_x, root_y;

    if (map) {
	Window child;
	int win_x, win_y;
	unsigned int keys_buttons;

	if (talk_created == false) {
	    Talk_create_window();
	    talk_created = true;
	}
	XMapWindow(dpy, talkWindow);
	clData.talking = true;

	XQueryPointer(dpy, DefaultRootWindow(dpy),
		      &root, &child, &root_x, &root_y, &win_x, &win_y,
		      &keys_buttons);
	XWarpPointer(dpy, None, talkWindow,
		     0, 0, 0, 0,
		     (int)(TALK_WINDOW_WIDTH - (TALK_WINDOW_HEIGHT / 2)),
		     (int)TALK_WINDOW_HEIGHT / 2);
	XFlush(dpy);	/* warp pointer ASAP. */
    }
    else if (talk_created) {
	XUnmapWindow(dpy, talkWindow);
	XWarpPointer(dpy, None, root, 0, 0, 0, 0, root_x, root_y);
	XFlush(dpy);	/* warp pointer ASAP. */
	clData.talking = false;
	/* reset browsing */
	history_pos = -1;
    }
    talk_cursor.visible = false;
}

/*
 * redraw a possible selection [un]emphasized.
 * to unemphasize a selection, 'selection.txt' is needed.
 * thus selection.talk.state == SEL_SELECTED indicates that it
 * should not be drawn emphasized
 */
static void Talk_refresh(void)
{
    size_t len;

    if (!clData.talking)
	return;

    len = strlen(talk_str);
    if (selection.talk.x1 > len || selection.talk.x2 > len)
	/* don't redraw. it's not there anymore */
	return;
    else if (len == 0) {
	XClearWindow(dpy, talkWindow);
	return;
    }

    if (selection.talk.state == SEL_EMPHASIZED)
	XSetForeground(dpy, talkGC, colors[DRAW_EMPHASIZED].pixel);
    else
	XSetForeground(dpy, talkGC, colors[WHITE].pixel);

    assert(selection.talk.x2 >= selection.talk.x1);
    XDrawString(dpy, talkWindow, talkGC,
		XTextWidth(talkFont, talk_str, (int)selection.talk.x1)
		 + TALK_INSIDE_BORDER,
		talkFont->ascent + TALK_INSIDE_BORDER,
		&talk_str[selection.talk.x1],
		(int)(selection.talk.x2 - selection.talk.x1));
    XSetForeground(dpy, talkGC, colors[WHITE].pixel);
}

/*
 * add a line to the history.
 */
static void Add_msg_to_history(char *message)
{
    char *tmp;
    char **msg_set;
    int i;

    /*always*/
    save_talk_str = false;

    if (strlen(message) == 0)
	return; /* unexpected. nothing to add */

    msg_set = HistoryMsg;
    /* pipe the msgs through the buffer, the latest getting into [0] */
    tmp = msg_set[maxLinesInHistory - 1];
    for (i = maxLinesInHistory - 1; i > 0; i--)
	msg_set[i] = msg_set[i - 1];
    msg_set[0] = tmp; /* memory recycling */

    strlcpy(msg_set[0], message, MAX_CHARS);
    history_pos = 0;

    return;
}

/*
 * Fetch a message from the 'history' by returning a pointer to it.
 * Choice depends on current poition (*pos, call by ref for modification here)
 * and 'direction' of browsing
 *
 * if we are here _and the gobal 'save_talk_str' is set, then the submitted
 * message is unfinished - call Add_msg_to_history(), but don't return
 * the next line.
 * Purpose: gives ability to abort writing a message and resume later.
 * The global 'save_talk' can be set anywhere else in the code whenever
 * a line from the history gets modified
 * (thus save_talk not as parameter here)
 *
 */
static char *Get_msg_from_history(int* pos, char *message, keys_t direction)
{
    int i;
    char **msg_set;

    if (direction != KEY_TALK_CURSOR_UP
	&& direction != KEY_TALK_CURSOR_DOWN
	&& direction != KEY_DUMMY) {
	return NULL;
    }

    if (direction == KEY_DUMMY && (*pos < 0 || *pos > maxLinesInHistory-1))
	*pos = 0;

    msg_set = HistoryMsg;

    if (save_talk_str) {
	if (strlen(message) > 0)
	    Add_msg_to_history(message);
	save_talk_str = false;
	return NULL;
    }

    /* search for the next message, return it */
    for (i=0; i < maxLinesInHistory; i++) {
	if (direction == KEY_TALK_CURSOR_UP) {
            (*pos)++;
            if (*pos >= maxLinesInHistory)
                *pos = 0; /* wrap */
	} else if (direction == KEY_TALK_CURSOR_DOWN) {
	    (*pos)--;
	    if (*pos < 0)
		*pos = maxLinesInHistory - 1; /*wrap*/
	}
	if (strlen(msg_set[*pos]) > 0)
	    return (msg_set[*pos]);
    }
    return NULL; /* no history */
}

/*
 * Pressing a key while there is an emphasized text in the talk window
 * substitutes this text, means: delete the text before inserting the
 * new character and place the character at this 'gap'.
 */
static void Talk_delete_emphasized_text(void)
{

    size_t	oldlen, newlen;
    int		width;
    char	new_str[MAX_CHARS];

    if (selection.talk.state != SEL_EMPHASIZED)
	return;

    Talk_cursor(false);

    strlcpy(new_str, talk_str, MAX_CHARS);
    oldlen = strlen(talk_str);
    newlen = oldlen;

    if (oldlen > 0 && oldlen >= selection.talk.x2) {
	strncpy(&new_str[selection.talk.x1], &talk_str[selection.talk.x2],
		oldlen - selection.talk.x2);
	talk_cursor.point = selection.talk.x1;
	newlen -= (selection.talk.x2 - selection.talk.x1);
	selection.talk.state = SEL_NONE;
	new_str[newlen] = '\0';
	if (talk_cursor.point > newlen)
	    talk_cursor.point = newlen;
    }
    new_str[newlen] = '\0';
    assert(talk_cursor.point <= newlen);

    width = XTextWidth(talkFont, talk_str, (int)selection.talk.x1);
    /*
     * Now reflect the text changes onto the screen.
     */
    if (newlen < oldlen) {
	XSetForeground(dpy, talkGC, colors[BLACK].pixel);
	assert(oldlen >= talk_cursor.point);
	XDrawString(dpy, talkWindow, talkGC,
		    width + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &talk_str[talk_cursor.point],
		    (int)(oldlen - talk_cursor.point));
	XSetForeground(dpy, talkGC, colors[WHITE].pixel);
    }
    if (talk_cursor.point < newlen)
	XDrawString(dpy, talkWindow, talkGC,
		    width + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &new_str[talk_cursor.point],
		    (int)(newlen - talk_cursor.point));
    Talk_cursor(true);

    strlcpy(talk_str, new_str, MAX_CHARS);
}

int Talk_do_event(XEvent *event)
{
    char		ch;
    bool		cursor_visible = talk_cursor.visible;
    size_t		oldlen, newlen, oldwidth;
    KeySym		keysym;
    char		new_str[MAX_CHARS];
    bool		result = true;
    /* int			str_len; of a history message */
    static int		talk_crs_repeat_count;

    /* according to Ralf Berger <gotan@physik.rwth-aachen.de>
     * compose should be static:
     * the value of 'compose' has to be preserved across calls to
     * 'XLookupString' (the man page also mentioned that a portable program
     * should pass NULL but i don't know if that's specific to dec-alpha ?).
     * To fix this declare 'compose' static (can't hurt anyway).
     */
    static XComposeStatus	compose;

    switch (event->type) {

    case Expose:
	XClearWindow(dpy, talkWindow);
	XDrawString(dpy, talkWindow, talkGC,
		    TALK_INSIDE_BORDER, talkFont->ascent + TALK_INSIDE_BORDER,
		    talk_str, (int)strlen(talk_str));
	if (selection.talk.state == SEL_EMPHASIZED)
	    Talk_refresh();
	if (cursor_visible) {
	    talk_cursor.visible = false;
	    Talk_cursor(cursor_visible);
	}
	break;

    case KeyRelease:
	/*
	 * stop faster cursor movement in talk window
	 */
	talk_crs_repeat_count = 0;

	/*
	 * Nothing to do.
	 * We may want to make some kind of key repeat ourselves.
	 * Some day...
	 */
	break;

    case KeyPress:
	/* 'strange' keys exist */
	if ((keysym = XLookupKeysym(&event->xkey, 0)) == NoSymbol)
	    break;

	/* 'unprintables'? */
	if (XLookupString(&event->xkey, &ch, 1, &keysym, &compose)
		== NoSymbol) {

	    keys_t key;		/* what key is it */
	    char *tmp;		/* for receiving a line from the history */

	    /* search the 'key' */
	    for (key = Lookup_key(event, keysym, true);
		 key != KEY_DUMMY;
		 key = Lookup_key(event, keysym, false)) {
		switch (key) {
		case KEY_TALK_CURSOR_RIGHT:
		    Talk_cursor(false);
		    /*
		     * faster cursor movement after some time.
		     * 'talk_crs_repeat_count' is reset at 'KeyRelease'
		     */
		    if (talk_crs_repeat_count > CRS_START_HOPPING) {
			if (talk_cursor.point < strlen(talk_str) - CRS_HOP)
			    talk_cursor.point += CRS_HOP;
			else
			    talk_cursor.point = strlen(talk_str);
		    } else {
			if (talk_cursor.point < strlen(talk_str)) {
			    talk_cursor.point++;
			    talk_crs_repeat_count++;
			}
		    }
		    Talk_cursor(true);
		    break; /* out of switch(key) */

		case KEY_TALK_CURSOR_LEFT:
                    Talk_cursor(false);
                    /* faster cursor movement after some time */
                    if (talk_crs_repeat_count > CRS_START_HOPPING) {
                        if (talk_cursor.point > CRS_HOP)
                            talk_cursor.point -= CRS_HOP;
                        else
                            talk_cursor.point = 0;
                    } else {
                        if (talk_cursor.point > 0) {
                            talk_cursor.point--;
			    talk_crs_repeat_count++;
                        }
                    }
                    Talk_cursor(true);
		    break;

		case KEY_TALK_CURSOR_UP:
		case KEY_TALK_CURSOR_DOWN:
		    /*
		     * browsing in history.
		     * Get_msg_from_history() will notice if you start
		     * browsing while having an unfinished message:
		     *   you want to store it and will type a new one.
		     *   thus Get_msg_from_history() won't return the next
		     *   message _this time. clear the talk window instead.
		     */
		    tmp = Get_msg_from_history(&history_pos, talk_str, key);
		    if (tmp && strlen(tmp) > 0) {
			/* we got smthng from the history */
			/*strlcpy(talk_str, tmp, MAX_CHARS);
			str_len = strlen(talk_str); */
			Talk_paste(tmp, strlen(tmp), true);
		    } else {
			talk_str[0] = '\0';
			Talk_cursor(false);
			talk_cursor.point = 0;
			Talk_cursor(true);
			XClearWindow(dpy, talkWindow);
		    }
		    break;

		default:
		    break;

		} /* switch */
	    } /* for */
	    break; /* out of 'KeyPress' */
	} /* XLookupString() == NoSymbol */

	/*
	 * printable or a <ctrl-char>
	 */

	{
	    /*
	     * unemphasize?
	     * The key might change the talk string (important when browsing).
	     */
	    switch (ch) {
	    /* special handling for */
	    case CTRL('A'): /* only cursor movement */
	    case CTRL('E'):
	    case CTRL('B'):
	    case CTRL('F'):
	    case CTRL('\r'): /* perhaps nothing was changed */
	    case CTRL('\n'):
	    case '\033':     /* canceled */
		break;
	    default:
		if (isprint(ch)) {
		    /*
		     * the string is modified.
		     * next Get_msg_from_history() will store it in the history
		     */
		    save_talk_str = true;
		    if (selection.talk.state == SEL_EMPHASIZED)
			/* convenient deleting of text */
			Talk_delete_emphasized_text();
		}
		break;
	    }
	}

	switch (ch) {
	case '\0':
	    /*
	     * ?  Ignore.
	     */
	    break;

	case '\r':
	case '\n':
	    /*
	     * Return.  Send the talk message to the server if there is text.
	     */
	    if (talk_str[0] != '\0') {
		if (selection.talk.state == SEL_EMPHASIZED) {
		    /*
		     * send a message. it will appear as talk message.
		     * keep a possible emphasizing and set selection.draw.*
		     * to proper values.
		     */
		    selection.talk.state = SEL_SELECTED;
		    selection.keep_emphasizing = true;
		    selection.draw.x1 = selection.talk.x1;
		    selection.draw.x2 = selection.talk.x2;
		}
		/* add to history if the message was not gotten by browsing */
		if (save_talk_str)
		    Add_msg_to_history(talk_str);
		Net_talk(talk_str);
		talk_cursor.point = 0;
		talk_str[0] = '\0';
	    }
	    else {
		/* talk_str is empty */
		save_talk_str = false;
		history_pos = 0;		/* reset history position */
	    }
	    result = false;
	    break;

	case '\033':
	    /*
	     * Escape.  Cancel talking.
	     */
	    talk_str[0] = '\0';
	    talk_cursor.point = 0;
	    save_talk_str = false;
	    selection.talk.state = SEL_NONE;
	    result = false;
	    break;

	case CTRL('A'):
	    /*
	     * Put cursor at start of line.
	     */
	    Talk_cursor(false);
	    talk_cursor.point = 0;
	    Talk_cursor(true);
	    break;

	case CTRL('E'):
	    /*
	     * Put cursor at end of line.
	     */
	    Talk_cursor(false);
	    talk_cursor.point = strlen(talk_str);
	    Talk_cursor(true);
	    break;

	case CTRL('B'):
	    /*
	     * Put cursor one character back.
	     */
	    if (talk_cursor.point > 0) {
		Talk_cursor(false);
		talk_cursor.point--;
		Talk_cursor(true);
	    }
	    break;

	case CTRL('F'):
	    /*
	     * Put cursor one character forward.
	     */
	    if (talk_cursor.point < strlen(talk_str)) {
		Talk_cursor(false);
		talk_cursor.point++;
		Talk_cursor(true);
	    }
	    break;

	case '\b':
	case '\177':
	case CTRL('D'):
	case CTRL('W'):
	case CTRL('U'):
	case CTRL('K'):
	    /*
	     * Erase characters.
	     */
	    Talk_cursor(false);

	    strlcpy(new_str, talk_str, MAX_CHARS);
	    oldlen = strlen(talk_str);
	    newlen = oldlen;

	    /*
	     * Calculate text changes first without drawing.
	     */
	    if (ch == CTRL('W')) {
		/*
		 * Word erase.
		 * Erase whitespace first and then one word.
		 */
		while (newlen > 0 && talk_str[newlen - 1] == ' ')
		    newlen--;
		while (newlen > 0 && talk_str[newlen - 1] != ' ')
		    newlen--;
	    }
	    else if (ch == CTRL('U'))
		/*
		 * Erase everything.
		 */
		newlen = 0;
	    else if (ch == CTRL('K'))
		/*
		 * Clear rest of the line.
		 */
		newlen = talk_cursor.point;
	    else if (oldlen > 0) {
		if (selection.talk.state == SEL_EMPHASIZED) {
		    /*
		     * Erase the emphasized text
		     */
		    strncpy(&new_str[selection.talk.x1],
			    &new_str[selection.talk.x2],
			    oldlen - selection.talk.x2);
		    new_str[selection.talk.x1 + oldlen - selection.talk.x2]
			= '\0';
		    talk_cursor.point = selection.talk.x1;
		    assert(selection.talk.x2 >= selection.talk.x1);
		    assert(newlen >= (selection.talk.x2 - selection.talk.x1));
		    newlen -= (selection.talk.x2 - selection.talk.x1);
		    selection.talk.state = SEL_NONE;
		} else {
		    /*
		     * Erase possibly several characters.
		     */
		    if (talk_crs_repeat_count > CRS_START_HOPPING) {
			if (talk_cursor.point > CRS_HOP) {
			    assert(newlen >= CRS_HOP);
			    newlen -= CRS_HOP;
			    if (ch != CTRL('D')
				|| talk_cursor.point >= newlen)
				talk_cursor.point -= CRS_HOP;
			    strlcpy(&new_str[talk_cursor.point],
				    &talk_str[talk_cursor.point + CRS_HOP],
				    MAX_CHARS - talk_cursor.point);
			} else {
			    int old_talk_cursor_point = talk_cursor.point;
			    assert(newlen >= talk_cursor.point);
			    newlen -= talk_cursor.point;
			    if (ch != CTRL('D')
				|| talk_cursor.point >= newlen)
				talk_cursor.point = 0;
			    strlcpy(&new_str[0],
				    &talk_str[old_talk_cursor_point],
				    MAX_CHARS);
			}
		    } else {
			/*
			 * Erase one character.
			 */
			if (talk_cursor.point > 0) {
			    assert(newlen > 0);
			    newlen--;
			    if (ch != CTRL('D')
				|| talk_cursor.point >= newlen)
				talk_cursor.point--;
			    talk_crs_repeat_count++;
			    strlcpy(&new_str[talk_cursor.point],
				    &talk_str[talk_cursor.point + 1],
				    MAX_CHARS - talk_cursor.point);
			}
		    }
		}
	    }

	    new_str[newlen] = '\0';
	    if (talk_cursor.point > newlen)
		talk_cursor.point = newlen;

	    /*
	     * Now reflect the text changes onto the screen.
	     */
	    if (newlen < oldlen) {
		XSetForeground(dpy, talkGC, colors[BLACK].pixel);
		assert(oldlen >= talk_cursor.point);
		XDrawString(dpy, talkWindow, talkGC,
			    XTextWidth(talkFont, talk_str,
				       (int)talk_cursor.point)
			    + TALK_INSIDE_BORDER,
			    talkFont->ascent + TALK_INSIDE_BORDER,
			    &talk_str[talk_cursor.point],
			    (int)(oldlen - talk_cursor.point));
		XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	    }
	    if (talk_cursor.point < newlen)
		XDrawString(dpy, talkWindow, talkGC,
			    XTextWidth(talkFont, talk_str,
				       (int)talk_cursor.point)
			    + TALK_INSIDE_BORDER,
			    talkFont->ascent + TALK_INSIDE_BORDER,
			    &new_str[talk_cursor.point],
			    (int)(newlen - talk_cursor.point));
	    Talk_cursor(cursor_visible);

	    strlcpy(talk_str, new_str, MAX_CHARS);

	    break;

	default:
	    if ((ch & 0x7F) == ch && !isprint(ch))
		/*
		 * Unknown special character.
		 */
		break;

	    oldlen = strlen(talk_str);
	    oldwidth = XTextWidth(talkFont, talk_str, (int)oldlen);
	    if (oldlen >= MAX_CHARS - 2
		|| oldwidth >= TALK_WINDOW_WIDTH - 2*TALK_INSIDE_BORDER - 5) {
		/*
		 * No more space for new text.
		 */
		XBell(dpy, 100);
		break;
	    }

	    /*
	     * Enter new text.
	     */
	    /* Width of prefix before cursor */
	    oldwidth = XTextWidth(talkFont, talk_str, (int)talk_cursor.point);
	    strlcpy(new_str, talk_str, MAX_CHARS);
	    strlcpy(&new_str[talk_cursor.point + 1],
		    &talk_str[talk_cursor.point],
		    MAX_CHARS - (talk_cursor.point + 1));
	    new_str[talk_cursor.point] = ch;
	    newlen = oldlen + 1;

	    /*
	     * Reflect text changes onto screen.
	     */
	    Talk_cursor(false);
	    if (talk_cursor.point < oldlen) {
		/*
		 * Erase old text from cursor to end of line.
		 */
		XSetForeground(dpy, talkGC, colors[BLACK].pixel);
		XDrawString(dpy, talkWindow, talkGC,
			    (int)oldwidth + TALK_INSIDE_BORDER,
			    talkFont->ascent + TALK_INSIDE_BORDER,
			    &talk_str[talk_cursor.point],
			    (int)(oldlen - talk_cursor.point));
		XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	    }
	    assert(newlen >= talk_cursor.point);
	    XDrawString(dpy, talkWindow, talkGC,
			(int)oldwidth + TALK_INSIDE_BORDER,
			talkFont->ascent + TALK_INSIDE_BORDER,
			&new_str[talk_cursor.point],
			(int)(newlen - talk_cursor.point));
	    talk_cursor.point++;
	    strlcpy(talk_str, new_str, MAX_CHARS);
	    Talk_cursor(cursor_visible);
	    break;
	}
	XFlush(dpy);	/* needed in case we don't get frames to draw soon. */

	/*
	 * End of KeyPress.
	 */
	break;

    default:
	break;
    }

    return result;	/* keep on talking if true, no more talking if false */
}

/*
 * Try to paste 'data_len' amount of 'data' at the cursor position into
 * the talk window.  Cut off overflow ('accept_len').
 *
 * The global 'talk_str' will contain the new content.
 * (safe if *data references *talk_str).
 *
 * if 'overwrite' then don't insert/append but substitute the text
 *
 * Return the number of pasted characters.
 */
int Talk_paste(char *data, size_t data_len, bool overwrite)
{

    size_t str_len;			/* current length */
    size_t max_len    = MAX_CHARS - 2;	/* absolute max */
    size_t new_len;			/* after pasting */
    size_t char_width = XTextWidth(talkFont, talk_str, 1);
    size_t max_width  = (TALK_WINDOW_WIDTH - 2*TALK_INSIDE_BORDER - 5);

    size_t accept_len;			/* for still matching the window */
    char paste_buf[MAX_CHARS];		/* gets the XBuffer */
    char tmp_str[MAX_CHARS];
    char talk_backup[MAX_CHARS];	/* no 'collision' with data */
    bool cursor_visible = false;
    int i;

    if (!data || data_len == 0 || strlen(data) == 0)
	return 0;

    if (overwrite)
	/* implicitly, old text will be deleted now: */
	str_len = 0;
    else {
	str_len = strlen(talk_str);
	strlcpy(talk_backup, talk_str, sizeof(talk_backup));
    }
    /* XTextWidth may return zero, avoid division by zero */
    if (char_width == 0)
	char_width = 1;
    /* !@# XXX This is wrong with proportional fonts */
    accept_len = (max_width / char_width) - str_len + 1;
    if (accept_len + str_len > max_len)
	accept_len = max_len - str_len;

    if (!accept_len) {
	/* no room to broom */
	XBell(dpy, 100);
	return 0;
    }
    if (data_len > accept_len)
	/* not all accepted */
	XBell(dpy, 100);
    else if (data_len < accept_len)
	/* not the whole string required to paste */
	accept_len = data_len;

    strlcpy(paste_buf, data , accept_len + 1);

    /*
     * substitute unprintables according to iso-latin-1.
     *  (according to 'data_len' one could ask for all but the
     *   final '\0' to be converted, but we have only text selections anyway)
     */
    /* don't convert a final newline to space */
    if (paste_buf[accept_len-1] == '\n' || paste_buf[accept_len-1] == '\r') {
	paste_buf[accept_len-1] = '\0';
	accept_len--;
    }
    for (i = 0; i < (int)accept_len; i++) {
        if (	  ((unsigned char)paste_buf[i] <   33
		/* && (unsigned char)paste_buf[i] != '\0' */)
	    ||    ((unsigned char)paste_buf[i] >  126
		&& (unsigned char)paste_buf[i] <  161) )
            paste_buf[i] = ' ';
    }

    if (overwrite) {
	strlcpy(tmp_str, paste_buf, accept_len + 1);
	new_len = accept_len;
    } else {
	/* paste: insert/append */
	strlcpy(tmp_str, talk_backup, sizeof(tmp_str));
	strlcpy(&tmp_str[talk_cursor.point], paste_buf,
		sizeof(tmp_str) - talk_cursor.point);
	strlcpy(&tmp_str[talk_cursor.point + accept_len],
	        &talk_backup[talk_cursor.point],
	        sizeof(tmp_str) - (talk_cursor.point + accept_len));
	new_len = str_len + accept_len;
    }


    /*
     * graphics
     */
    if (overwrite) {
	XClearWindow(dpy, talkWindow);
	XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	XDrawString(dpy, talkWindow, talkGC,
		    TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    tmp_str, (int)accept_len);
    } else {
	if (selection.talk.state == SEL_EMPHASIZED) {
	    /* redraw unemphasized */
	    selection.talk.state = SEL_SELECTED;
	    Talk_refresh();
	}
	if (talk_cursor.point < str_len) {
	    /*
	     * erase from the point of insertion on
	     */
	    XSetForeground(dpy, talkGC, colors[BLACK].pixel);
	    XDrawString(dpy, talkWindow, talkGC,
			XTextWidth(talkFont, talk_backup,
				   (int)talk_cursor.point)
			+ TALK_INSIDE_BORDER,
			talkFont->ascent + TALK_INSIDE_BORDER,
			&talk_backup[talk_cursor.point],
			(int)(str_len - talk_cursor.point));
	    XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	}

	/* the new part of the line */
	assert(new_len >= talk_cursor.point);
	XDrawString(dpy, talkWindow, talkGC,
		    XTextWidth(talkFont, talk_backup,
			       (int)talk_cursor.point)
		    + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &tmp_str[talk_cursor.point],
		    (int)(new_len - talk_cursor.point));
    }
    strlcpy(talk_str, tmp_str, sizeof(talk_str));

    cursor_visible = talk_cursor.visible;
    Talk_cursor(false);
    if (overwrite)
	talk_cursor.point = new_len;
    else
	talk_cursor.point += accept_len;
    Talk_cursor(cursor_visible);

    return accept_len;
}


void Talk_resize(void)
{
    if (talk_created)
	XMoveResizeWindow(dpy, talkWindow,
			  TALK_WINDOW_X, TALK_WINDOW_Y,
			  TALK_WINDOW_WIDTH, TALK_WINDOW_HEIGHT);
}

/* Return length of first prefix with length at most len wider that 'width',
 * or len + 1 if such doesn't exist. */
static int Text_width_to_pos(XFontStruct *font, char *text, size_t len,
			     int width)
{
    /* dummies for 'XTextExtents', the faster version of XTextWidth */
    int	font_ascent_return, font_descent_return, direction_return;
    /* wanted: overall_return.width */
    XCharStruct overall_return;
    int i;

    for (i = 0; i <= (int)len; i++) {
	XTextExtents(font, text, i,
		     &direction_return,
		     &font_ascent_return, &font_descent_return,
		     &overall_return);
	if (overall_return.width >= width)
	    break;
    }
    return i;
}

/*
 * place the cursor in the talk window with help of the pointer button.
 * return the cursor position as index in talk_str.
 *
 * place cursor conveniently, if 'pending' is set and cutting
 * in the talk window finished outside of it (border is also outside).
 */
int Talk_place_cursor(XButtonEvent* xbutton, bool pending)
{
    int x, y;		/* pixelpositions */
    int cursor_pos;	/* string index */
    int Button = xbutton->button;

    x = xbutton->x;
    y = xbutton->y;

    x -= TALK_INSIDE_BORDER + 1; /* tuned */

    cursor_pos = Text_width_to_pos(talkFont, talk_str, strlen(talk_str), x) -1;

    /*
     * if it happened outside the window
     */
    if (cursor_pos < 0 || x > (int)TALK_WINDOW_WIDTH
	|| y < 0 || y >= (int)TALK_WINDOW_HEIGHT) {
	if (Button == 1 && pending) {
	    /* convenient finish of cutting */
	    if (cursor_pos < (int)selection.talk.x1)
		cursor_pos = 0; /* left end */
	    else {
		cursor_pos = strlen(talk_str); /* right end */
		selection.talk.incl_nl = true;
	    }
	} else
	    cursor_pos = 0;
    }

    /* no implicit lengthening of talk_str */
    if (cursor_pos > (int)strlen(talk_str)) {
        if (Button == 1)
            selection.talk.incl_nl = true;
        cursor_pos = strlen(talk_str);
    }

    /* place cursor with pointer */
    assert(cursor_pos >= 0);
    Talk_cursor(false);
    talk_cursor.point = cursor_pos;
    Talk_cursor(true);

    return cursor_pos;
}

static void Clear_talk_selection(void)
{
    selection.talk.x1 = selection.talk.x2 = 0;
    selection.talk.state = SEL_NONE;
    selection.talk.incl_nl = false;
}

static void Clear_draw_selection(void)
{
    selection.draw.x1 = selection.draw.x2
	= selection.draw.y1 = selection.draw.y2 = 0;
    selection.draw.state = SEL_NONE;
}

/*
 * If a cut doesn't suceed, leave the selection in appropriate state.
 */
static void Selection_set_state(void)
{
    if (selection.len > 0)
	selection.draw.state = SEL_SELECTED;
    else
	selection.draw.state = SEL_NONE;
}

/*
 * show that someone else owns the selection now
 */
void Clear_selection(void)
{
    if (clData.talking && selection.talk.state == SEL_EMPHASIZED) {
	/* trick to unemphasize */
	selection.talk.state = SEL_SELECTED;
	Talk_refresh();
    }
    Clear_draw_selection();
    Clear_talk_selection();
    XFREE(selection.txt);
    selection.txt_size = 0;
    selection.len = 0;

}

/*
 * cutting from talk window happens here. wanted: selection.txt and the
 * string indices of start and end (selection.draw.x1/2)
 * call Talk_place_cursor() to place the cursor.
 */
void Talk_window_cut(XButtonEvent* xbutton)
{
    int cursor_pos;	/* where to place the text cursor */
    int ButtonState = xbutton->type;
    int Button      = xbutton->button;
    char tmp[MAX_CHARS];	/* preparing a string for the cut buffer */
    bool was_pending = false;

    /* convenient cursor placement when finishing a cut */
    if (selection.talk.state == SEL_PENDING
	  && ButtonState == ButtonRelease && Button == Button1)
	was_pending = true;

    cursor_pos = Talk_place_cursor(xbutton, was_pending);

    if (ButtonState == ButtonPress) {
	/*
	 * a cut began. redraw unemphasized
	 */
	selection.talk.state = SEL_PENDING;
	Talk_refresh();
	assert(cursor_pos >= 0);
	selection.talk.x1 = cursor_pos;
	selection.talk.incl_nl = false;

    } else if (ButtonState == ButtonRelease) {
	if (selection.talk.state != SEL_PENDING)
	    /*
	     * cut didn't start properly
	     */
	    return;

	assert(cursor_pos >= 0);
	selection.talk.x2 = cursor_pos;
	if (selection.talk.x1 == selection.talk.x2) {
	    /* no cut */
	    selection.talk.state = SEL_NONE;
	    return;
	}

	/*
	 * A real cut has been made
	 */
	Clear_draw_selection();
	selection.txt_size = MAX_MSGS * MSG_LEN;
	selection.txt = XMALLOC(char, selection.txt_size);
	if (selection.txt == NULL) {
	    error("No memory for Selection");
	    return;
        }

	/* swap order, if necessary */
	if (selection.talk.x1 > selection.talk.x2) {
	    int tmp2 = selection.talk.x2;
	    selection.talk.x2 = selection.talk.x1;
	    selection.talk.x1 = tmp2;
	}

	/*
	 * making the cut available; see end of Talk_cut_from_messages()
	 */
	XSetSelectionOwner(dpy, XA_PRIMARY, talkWindow, CurrentTime);
	/* 'cut buffer' is binary stuff - append '\0'  */
	assert(selection.talk.x2 >= selection.talk.x1);
	strlcpy(tmp, &talk_str[selection.talk.x1],
		selection.talk.x2 - selection.talk.x1 + 1);
	if (selection.talk.incl_nl) {
	    strlcat(tmp, "\n", selection.talk.x2 - selection.talk.x1 + 1);
	    selection.talk.incl_nl = false;
	}
	strlcpy(selection.txt, tmp, selection.txt_size);
	selection.len = strlen(tmp);
	XStoreBytes(dpy, selection.txt, (int)selection.len);

	/*
	 * emphasize the selection
	 */
	selection.talk.state = SEL_EMPHASIZED;
	selection.draw.state = SEL_NONE;
	Talk_refresh();

    } /* ButtonRelease */
}

bool Talk_cut_area_hit(XButtonEvent *xbutton)
{
    const int BORDER = 10;
    const int SPACING = messageFont->ascent+messageFont->descent+1;
    int y;	/* of initial ButtonEvent */

    y = xbutton->y - BORDER;

    if (y < 0)
	y = -1;
    else
	y /= SPACING;

    if (y < maxMessages)
	return true;

    return false;
}

/*
 * wanted:
 * indices of the characters and the rows of start and end of
 * the selected text.
 *
 * for proper cutting: Notice if cutting heppens left from the most left
 * or right from the most right character (c1/2.x_off).
 *
 * while the cut is pending, the state of Talk+GameMsg[] is 'freezed'
 * in Paint_message(). thus call Add_pending_messages() here after this.
 */
void Talk_cut_from_messages(XButtonEvent* xbutton)
{

    const int BORDER = 10;
    const int SPACING = messageFont->ascent+messageFont->descent+1;
    int		x, y;	/* of initial ButtonEvent */

    typedef struct {
	int x;		/* pixel positions */
	int y;
	int pixel_len;  /* of whole string */
	int str_index;  /* wanted: index in the message */
	/* at the end of a string? */
	int x_off;	/* -1: left end,  +1: right end */
    } cut_position;

    static cut_position c1 = {0, 0, 0, 0, 0,};

    static int	last_msg_index;	/* index of last message */
    int		i;

    /* quick check if there are messages at all */
    if (TalkMsg[0]->len == 0) {
	Selection_set_state();
	return;
    }

    x = xbutton->x - BORDER;
    y = xbutton->y - BORDER;

    if (y < 0)
	y = -1;
    else
	y /= SPACING;

   /*
    * ..............Button Press...............
    * store the important things
    */
    if (xbutton->type == ButtonPress) {
	/* how many messages? */
	last_msg_index = 0;
	while (last_msg_index < maxMessages
		&& TalkMsg[last_msg_index]->len != 0) {
	    last_msg_index++;
	}
	last_msg_index--; /* make it an index */

	c1.x = x;
	c1.y = y;
	c1.x_off = c1.str_index = c1.pixel_len = 0;

	/*
	 * first adjustments
	 */
	if (c1.y < 0) {
	    /* upper-left end */
	    c1.x	= -1;
	    c1.y	= 0;
	} else if (c1.y > last_msg_index) {
	    /* lower-right end */
	    c1.x_off	= 1;
	    c1.y	= last_msg_index;
	    c1.x	= 0; /* stay safe */
	} else if (c1.x < 0) {
	    /* left end */
	    c1.x	 = -1;
	}
	selection.draw.state = SEL_PENDING;
	return;

   /*
    * ..............Button Release...............
    */
    } else if (xbutton->type == ButtonRelease) {
	message_t *ptr;

	cut_position c2 = {0, 0, 0, 0, 0,};

	/*
	 * selection.txt will contain the whole selection
	 */
	char	cut_str[MSG_LEN]; /* for fetching the messages line by line */
	int	cut_str_len;
	int 	current_line;	/* when going through a multi line selection */


	if (selection.draw.state != SEL_PENDING) {
	    /* no proper start of cut */
	    Selection_set_state();
	    return;
	}

	/*
	 * The cut has been made
	 */

	c2.x = x;
	c2.y = y;
	c2.x_off = c2.str_index = c2.pixel_len = 0;

        if (c2.y < 0) {
	    /* upper-left end */
	    c2.x	= -1;
	    c2.y	= 0;
        } else if (c2.y > last_msg_index) {
	    /* lower-right end */
	    c2.x_off	= 1;
	    c2.y	= last_msg_index;
	    c2.x	= 0;
        } else if (c2.x < 0) {
	    /* left end */
	    c2.x	 = -1;
	}

	/* swap order? */
	if (c2.y < c1.y
	    || (c2.y == c1.y
		 && (c2.x_off - c1.x_off < 0
	             || (c2.x < c1.x && (c2.x_off - c1.x_off == 0))))) {
	    cut_position backup = c1;
	    c1 = c2;
	    c2 = backup;
	}

	/*
	 *further decisions, as start and end are in order now
	 */

	/* cut finished at next line - without a character there, though */
	if (c2.x == -1  &&  c1.y < c2.y) {
	    c2.y	-= 1;
	    c2.x_off	= 1 ;
	    c2.x	= 0;
	}
	/* cut started at end of line; jump to next if possible */
	ptr = TalkMsg[c1.y];
	if ((c1.x > XTextWidth(messageFont, ptr->txt, (int)ptr->len)
	    || c1.x_off == 1)  &&  c1.y < c2.y) {
	    c1.x	= 0;
	    c1.y	+= 1;
	}
	if (c1.x == -1)
	    c1.x = 0;

	/*
	 * find the indices in the talk string
	 */
	c1.str_index = 0;
	if (c1.x_off == 1)
	    c1.str_index = ptr->len - 1;
	else
	    c1.str_index
		= Text_width_to_pos(messageFont, ptr->txt, ptr->len, c1.x);

	ptr = TalkMsg[c2.y];
	c2.str_index = 0;
	if (c2.x_off == 1)
	    c2.str_index = ptr->len - 1;
	else
	    c2.str_index
		= Text_width_to_pos(messageFont, ptr->txt, ptr->len, c2.x);

	/*
	 * 'c1' ~ 'c2':
	 * the cut doesn't really include a character:
	 * - cutting from the end of a line to the beginning of the next
	 * - or different pixels but the same character
	 */
	if (c1.y == c2.y
	    && (c1.str_index > c2.str_index || (c1.str_index == c2.str_index
						&& c1.x_off == c2.x_off))) {
	    Add_pending_messages();
	    Selection_set_state();
	    return;
	}

	/*
	 * 'plug-in':
	 * don't include the last character (if explicitly clicked on)
	 */
	if (c2.str_index == 0) {
	    if (c1.y == c2.y) {
		/* it's possible */
		Add_pending_messages();
		Selection_set_state();
		return;
	    } else {
		/*
		 * the last character pointed at is the first on a line,
		 * don't include
		 */
		c2.y--;
		c2.x_off = 1;
		c2.str_index = TalkMsg[c2.y]->len - 1;
		if (c1.y == c2.y && c1.str_index >= c2.str_index
		    && c1.x_off == 1) {
		    Add_pending_messages();
		    Selection_set_state();
		    return;
		}
	    }
	} else if (c2.str_index == (int)TalkMsg[c2.y]->len) {
		c2.str_index = TalkMsg[c2.y]->len - 1;
		c2.x_off = 0;
	} else if (c2.str_index > 0 && c2.x_off == 0)
	    /* c2 is not the first on the line and a nl isn't included */
	    c2.str_index--;

	if (c1.str_index == (int)TalkMsg[c1.y]->len)
	    c1.str_index = TalkMsg[c1.y]->len - 1;

	/*
	 * set the globals
	 */
	selection.txt_size = MAX_MSGS * MSG_LEN;
	selection.txt = XMALLOC(char, selection.txt_size);
	if (selection.txt == NULL) {
	    error("No memory for Selection");
	    return;
        }
	selection.draw.x1 = c1.str_index;
	selection.draw.x2 = c2.str_index;
	selection.draw.y1 = c1.y;
	selection.draw.y2 = c2.y;

	current_line = selection.draw.y1;

	/* fetch the first line */
	strlcpy(cut_str, TalkMsg[current_line]->txt, sizeof(cut_str));
	cut_str_len = TalkMsg[current_line]->len;
	current_line++;

	if (selection.draw.y1 == selection.draw.y2) {
	/* ...it's the only line */
	    assert((selection.draw.x2 + 1) >= selection.draw.x1);
	    strlcpy(selection.txt, &cut_str[selection.draw.x1],
		    (size_t)(selection.draw.x2 + 1 + 1) - selection.draw.x1);
	    cut_str[0] = '\0';
	    if (c2.x_off == 1)
		strlcat(selection.txt, "\n", selection.txt_size);
	} else {
	    /* ...several lines */
	    assert(cut_str_len >= selection.draw.x1);
	    strlcpy(selection.txt, &cut_str[selection.draw.x1],
		    (size_t)(cut_str_len - selection.draw.x1 + 1));
	    strlcat(selection.txt, "\n", selection.txt_size);

	    /* whole lines themselves only if there are >= 3 lines */
	    for (i = selection.draw.y1 + 1; i < selection.draw.y2; i++) {
		strlcpy(cut_str, TalkMsg[current_line]->txt, sizeof(cut_str));
		cut_str_len = TalkMsg[current_line]->len;
		current_line++;
		strlcat(selection.txt, cut_str, selection.txt_size);
		strlcat(selection.txt, "\n", selection.txt_size);
	    }

	    /* the last line */
	    strlcpy(cut_str, TalkMsg[current_line]->txt, sizeof(cut_str));
	    cut_str_len = TalkMsg[current_line]->len;
	    current_line++;
	    strncat(selection.txt, cut_str, (size_t)selection.draw.x2 + 1);
	    if (c2.x_off == 1)
		strlcat(selection.txt, "\n", selection.txt_size);
	} /* more than one line */

	selection.len = strlen(selection.txt);

	/*
	 * store in 'cut buffer',
	 * usually a selection request is served by the event in xevent.c.
	 * We get that event as we own the 'primary' from now on.
	 * draw the selection emphasized from now on
	 */
	XSetSelectionOwner(dpy, XA_PRIMARY, drawWindow, CurrentTime);
	XStoreBytes(dpy, selection.txt, (int)selection.len);
	selection.draw.state = SEL_EMPHASIZED;
	selection.talk.state = SEL_SELECTED;
	Talk_refresh();
	Clear_talk_selection();
	Add_pending_messages();
	return;

    /* ButtonRelease */
    } else {
	return; /* neither ButtonPress nor ButtonRelease ? */
    }
}
