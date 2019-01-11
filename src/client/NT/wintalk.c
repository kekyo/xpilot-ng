/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "xpclient_x11.h"

/* Information window dimensions */
#define TALK_TEXT_HEIGHT	(textFont->ascent + textFont->descent)
#define TALK_OUTSIDE_BORDER	2
#define TALK_INSIDE_BORDER	3
#define TALK_WINDOW_HEIGHT	(TALK_TEXT_HEIGHT + 2 * TALK_INSIDE_BORDER)
#define TALK_WINDOW_X		(50 - TALK_OUTSIDE_BORDER)
#define TALK_WINDOW_Y		(draw_height*3/4 - TALK_WINDOW_HEIGHT/2)
#define TALK_WINDOW_WIDTH	(draw_width \
				    - 2*(TALK_WINDOW_X + TALK_OUTSIDE_BORDER))

#define CTRL(c)			((c) & 0x1F)

/*
 * Globals.
 */
static bool talk_created;
static char talk_str[MAX_CHARS];
static struct {
    bool visible;
    short offset;
    short point;
} talk_cursor;

/* position in history when browsing in the talk window */
static int history_pos = 0;

/* faster cursor movement in talk window after pressing a key for some time */
#define CRS_START_HOPPING	7
#define CRS_HOP			4

/* selections in draw and talk window */
selection_t selection;
bool save_talk_str = false;	/* see Get_msg_from_history */

/* if a cut doesn't suceed, leave the selection in appropriate state */
#define SET_SELECTION_STATE			\
if (selection.len > 0)				\
    selection.draw.state = SEL_SELECTED;	\
else						\
    selection.draw.state = SEL_NONE;

extern keys_t Lookup_key(XEvent * event, KeySym ks, bool reset);
extern void Add_pending_messages(void);

extern message_t *TalkMsg[MAX_MSGS], *GameMsg[MAX_MSGS];
extern char *HistoryMsg[MAX_HIST_MSGS];

static void Talk_create_window(void)
{
}


void Talk_cursor(bool visible)
{
}


void Talk_map_window(bool map)
{

}

/*
 * redraw a possible selection [un]emphasized.
 * to unemphasize a selection, 'selection.txt' is needed.
 * thus selection.talk.state == SEL_SELECTED indicates that it
 * should not be drawn emphasized
 */
void Talk_refresh()
{
}

/*
 * add a line to the history.
 */
void Add_msg_to_history(char *message)
{
    char *tmp;
    char **msg_set;
    int i;

    /*always */
    save_talk_str = false;

    if (strlen(message) == 0) {
	return;			/* unexpected. nothing to add */
    }

    msg_set = HistoryMsg;
    /* pipe the msgs through the buffer, the latest getting into [0] */
    tmp = msg_set[maxLinesInHistory - 1];
    for (i = maxLinesInHistory - 1; i > 0; i--) {
	msg_set[i] = msg_set[i - 1];
    }
    msg_set[0] = tmp;		/* memory recycling */

    strcpy(msg_set[0], message);
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
char *Get_msg_from_history(int *pos, char *message, keys_t direction)
{
    int i;
    char **msg_set;

    if (direction != KEY_TALK_CURSOR_UP
	    && direction != KEY_TALK_CURSOR_DOWN
	    && direction != KEY_DUMMY) {
	return NULL;
    }

    if (direction == KEY_DUMMY
	&& (*pos < 0 || *pos > maxLinesInHistory - 1)) {
	*pos = 0;
    }

    msg_set = HistoryMsg;

    if (save_talk_str) {
	if (strlen(message) > 0) {
	    Add_msg_to_history(message);
	}
	save_talk_str = false;
	return NULL;
    }

    /* search for the next message, return it */
    for (i = 0; i < maxLinesInHistory; i++) {
	if (direction == KEY_TALK_CURSOR_UP) {
	    (*pos)++;
	    if (*pos >= maxLinesInHistory) {
		*pos = 0;	/* wrap */
	    }
	} else if (direction == KEY_TALK_CURSOR_DOWN) {
	    (*pos)--;
	    if (*pos < 0) {
		*pos = maxLinesInHistory - 1;	/*wrap */
	    }
	}
	if (strlen(msg_set[*pos]) > 0) {
	    return (msg_set[*pos]);
	}
    }
    return NULL;		/* no history */
}

/*
 * Pressing a key while there is an emphasized text in the talk window
 * substitutes this text, means: delete the text before inserting the
 * new character and place the character at this 'gap'.
 */
void Talk_delete_emphasized_text()
{

    int oldlen, newlen;
    int onewidth = XTextWidth(talkFont, talk_str, 1);
    char new_str[MAX_CHARS];

    if (selection.talk.state != SEL_EMPHASIZED) {
	return;
    }

    Talk_cursor(false);

    strcpy(new_str, talk_str);
    oldlen = strlen(talk_str);
    newlen = oldlen;

    if (oldlen > 0) {
	strncpy(&new_str[selection.talk.x1], &new_str[selection.talk.x2],
		oldlen - selection.talk.x2);
	new_str[selection.talk.x1 + oldlen - selection.talk.x2] = '\0';
	talk_cursor.point = selection.talk.x1;
	newlen -= (selection.talk.x2 - selection.talk.x1);
	selection.talk.state = SEL_NONE;
	new_str[newlen] = '\0';
	if (talk_cursor.point > newlen) {
	    talk_cursor.point = newlen;
	}
    }
    new_str[newlen] = '\0';
    if (talk_cursor.point > newlen) {
	talk_cursor.point = newlen;
    }

    /*
     * Now reflect the text changes onto the screen.
     */
    if (newlen < oldlen) {
	XSetForeground(dpy, talkGC, colors[BLACK].pixel);
	XDrawString(dpy, talkWindow, talkGC,
		    talk_cursor.point * onewidth + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &talk_str[talk_cursor.point],
		    oldlen - talk_cursor.point);
	XSetForeground(dpy, talkGC, colors[WHITE].pixel);
    }
    if (talk_cursor.point < newlen) {
	XDrawString(dpy, talkWindow, talkGC,
		    talk_cursor.point * onewidth + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &new_str[talk_cursor.point],
		    newlen - talk_cursor.point);
    }
    Talk_cursor(true);

    strcpy(talk_str, new_str);
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

    int str_len;		/* current length */
    int max_len = MAX_CHARS - 2;	/* absolute max */
    int new_len;		/* after pasting */
    int char_width = XTextWidth(talkFont, talk_str, 1);
    int max_width = (TALK_WINDOW_WIDTH - 2 * TALK_INSIDE_BORDER - 5);

    int accept_len;		/* for still matching the window */
    char paste_buf[MAX_CHARS - 2];	/* gets the XBuffer */
    char tmp_str[MAX_CHARS - 2];
    char talk_backup[MAX_CHARS - 2];	/* no 'collision' with data */
    bool cursor_visible = false;
    int i;

    if (!data || data_len == 0 || strlen(data) == 0) {
	return 0;
    }

    if (overwrite) {
	/* implicitly, old text will be deleted now: */
	str_len = 0;
    } else {
	str_len = strlen(talk_str);
	strcpy(talk_backup, talk_str);
    }
    accept_len = (max_width / char_width) - str_len + 1;
    if (accept_len + str_len > max_len)
	accept_len = max_len - str_len;

    if (!accept_len) {
	/* no room to broom */
	XBell(dpy, 100);
	return 0;
    }
    if (data_len > accept_len) {
	/* not all accepted */
	XBell(dpy, 100);
    } else if (data_len < accept_len) {
	/* not the whole string required to paste */
	accept_len = data_len;
    }
    strncpy(paste_buf, data, accept_len);
    paste_buf[accept_len] = '\0';

    /*
     * substitute unprintables according to iso-latin-1.
     *  (according to 'data_len' one could ask for all but the
     *   final '\0' to be converted, but we have only text selections anyway)
     */
    /* don't convert a final newline to space */
    if (paste_buf[accept_len - 1] == '\n'
	|| paste_buf[accept_len - 1] == '\r') {
	paste_buf[accept_len - 1] = '\0';
	accept_len--;
    }
    for (i = 0; i < accept_len; i++) {
	if (((unsigned char) paste_buf[i] < 33
	     /* && (unsigned char)paste_buf[i] != '\0' */ )
	    || ((unsigned char) paste_buf[i] > 126
		&& (unsigned char) paste_buf[i] < 161)) {
	    paste_buf[i] = ' ';
	}
    }

    if (overwrite) {
	strncpy(tmp_str, paste_buf, accept_len);
	tmp_str[accept_len] = '\0';
	new_len = accept_len;
    } else {
	/* paste: insert/append */
	strcpy(tmp_str, talk_backup);
	strcpy(&tmp_str[talk_cursor.point], paste_buf);
	strcpy(&tmp_str[talk_cursor.point + accept_len],
	       &talk_backup[talk_cursor.point]);
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
		    tmp_str, accept_len);
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
			talk_cursor.point * char_width +
			TALK_INSIDE_BORDER,
			talkFont->ascent + TALK_INSIDE_BORDER,
			&talk_backup[talk_cursor.point],
			str_len - talk_cursor.point);
	    XSetForeground(dpy, talkGC, colors[WHITE].pixel);
	}

	/* the new part of the line */
	XDrawString(dpy, talkWindow, talkGC,
		    talk_cursor.point * char_width + TALK_INSIDE_BORDER,
		    talkFont->ascent + TALK_INSIDE_BORDER,
		    &tmp_str[talk_cursor.point],
		    new_len - talk_cursor.point);
    }
    strcpy(talk_str, tmp_str);

    cursor_visible = talk_cursor.visible;
    Talk_cursor(false);
    if (overwrite) {
	talk_cursor.point = new_len;
    } else {
	talk_cursor.point += accept_len;
    }
    Talk_cursor(cursor_visible);

    return accept_len;
}


/*
 * place the cursor in the talk window with help of the pointer button.
 * return the cursor position as index in talk_str.
 *
 * place cursor conveniently, if 'pending' is set and cutting
 * in the talk window finished outside of it (border is also outside).
 */

void Clear_talk_selection()
{
    selection.talk.x1 = selection.talk.x2 = 0;
    selection.talk.state = SEL_NONE;
    selection.talk.incl_nl = false;
}

void Clear_draw_selection()
{
    selection.draw.x1 = selection.draw.x2
	= selection.draw.y1 = selection.draw.y2 = 0;
    selection.draw.state = SEL_NONE;
}

/*
 * show that someone else owns the selection now
 */
void Clear_selection()
{
    if (clData.talking && selection.talk.state == SEL_EMPHASIZED) {
	/* trick to unemphasize */
	selection.talk.state = SEL_SELECTED;
	Talk_refresh();
    }
    Clear_draw_selection();
    Clear_talk_selection();
    if (selection.txt)
	free(selection.txt);
    selection.txt = NULL;
    selection.len = 0;

}

/*
 * cutting from talk window happens here. wanted: selection.txt and the
 * string indices of start and end (selection.draw.x1/2)
 * call Talk_place_cursor() to place the cursor.
 */
void Talk_window_cut(XButtonEvent * xbutton)
{
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
void Talk_cut_from_messages(XButtonEvent * xbutton)
{

}
