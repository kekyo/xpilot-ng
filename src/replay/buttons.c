/* 
 * XP-Replay, playback an XPilot session.  Copyright (C) 1994-98 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Steven Singer        (S.Singer@ph.surrey.ac.uk)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERC_HANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "xp-replay.h"

#include "tools/grey.xbm"

struct button {
    struct button	*next;
    Display		*display;
    Window		window;
    unsigned long	fg;
    unsigned int	width;
    unsigned int	height;
    union button_image	image;
    unsigned int	imagewidth;
    unsigned int	imageheight;
    int			flags;
    int			group;
    void		(*callback)(void *);
    void		*data;
};

static unsigned long	background = 0,
			topshadow = 0,
			bottomshadow = 0,
			black = 0;
static Button		buttonhead = NULL, buttontail = NULL;
static XFontStruct	*buttonFont = NULL;

void SetGlobalButtonAttributes(unsigned long bg,
			       unsigned long ts,
			       unsigned long bs,
			       unsigned long bl)
{
    Button b;
    int flag = 0;

    if (background != bg) {
	for (b = buttonhead; b != NULL; b = b->next)
	    XSetWindowBackground(b->display, b->window, bg);
	background = bg;
	flag = 1;
    }
    background = bg;
    if (topshadow != ts || bottomshadow != bs || black != bl)
	flag = 1;
    topshadow = ts;
    bottomshadow = bs;
    black = bl;
    if (flag)
	for (b = buttonhead; b != NULL; b = b->next)
	    RedrawButton(b);
}

static void SetButtonFont(Display *display)
{
    if ((buttonFont =
	 XLoadQueryFont(display,
			"-*-helvetica-bold-r-*--14-*-*-*-*-*-*-*")) == NULL)
	buttonFont = XQueryFont(display, XGContextFromGC(DefaultGC(display,
	    DefaultScreen(display))));
}

Button CreateButton(Display *display, Window parent,
		    int x, int y,
		    unsigned int width, unsigned int height,
		    union button_image image,
		    unsigned iw, unsigned ih,
		    unsigned long foreground,
		    void (*callback)(void *),
		    void *data, int flags, int group)
{
    Window window;
    Button b;

    b = (Button) MyMalloc(sizeof(struct button), MEM_UI);

    if ((width == 0 || height == 0) && (flags & BUTTON_TEXT)) {
	if (buttonFont == NULL)
	    SetButtonFont(display);
	if (width == 0)
	    width = XTextWidth(buttonFont, image.string,
			       (int)strlen(image.string)) + 10;
	if (height == 0)
	    height = buttonFont->ascent + buttonFont->descent + 10;
    }
	
    window = XCreateSimpleWindow(display, parent,
				 x, y,
				 width, height,
				 0,
				 background, background);

    b->display = display;
    b->window = window;
    b->fg = foreground;
    b->width = width;
    b->height = height;
    b->image = image;
    b->imagewidth = iw;
    b->imageheight = ih;
    b->flags = flags;
    b->group = group;
    b->callback = callback;
    b->data = data;
    b->next = NULL;

    if (buttontail == NULL)
	buttonhead = buttontail = b;
    else {
	buttontail->next = b;
	buttontail = b;
    }

    XSelectInput(display, window,
		 ExposureMask | ButtonPressMask | ButtonReleaseMask);

    XMapWindow(display, window);

    return(b);
}

static void ReleaseButtons(Button b)
{
    Button c;

    if (b->group != 0) {
	for (c = buttonhead; c != NULL; c = c->next)
	    if (c->group == b->group && c != b
		&& (c->flags & BUTTON_PRESSED)) {
		c->flags &= ~BUTTON_PRESSED;
		RedrawButton(c);
	    }
    }
}

static void PressButton(Button b)
{
    /* Buttons which stay in have no affect if pressed when in */

    if ((b->flags & BUTTON_PRESSED && !(b->flags & BUTTON_RELEASE)) ||
	b->flags & BUTTON_DISABLED)
	return;

    ReleaseButtons(b);

    b->flags |= BUTTON_PRESSED;

    RedrawButton(b);

    if (!(b->flags & BUTTON_RELEASE) && b->callback != NULL)
	b->callback(b->data);
}

static void ReleaseButton(Button b, Bool inwindow)
{

    if (!(b->flags & BUTTON_PRESSED) || !(b->flags & BUTTON_RELEASE) ||
	b->flags & BUTTON_DISABLED)
	return;

    b->flags &= ~BUTTON_PRESSED;

    RedrawButton(b);

    if (inwindow && b->callback != NULL)
	b->callback(b->data);
}

int CheckButtonEvent(XEvent *event)
{
    Button b;

    for (b = buttonhead; b != NULL; b = b->next)
	if (event->xany.window == b->window)
	    break;

    if (b == NULL)
	return(0);

    switch(event->type) {
    case Expose:
	if (event->xexpose.count != 0)
	    return(1);
	RedrawButton(b);
	return(1);
    case ButtonPress:
	if (event->xbutton.button == 1)
	    PressButton(b);
	return(1);
    case ButtonRelease:
	if (event->xbutton.button == 1)
	    ReleaseButton(b, (event->xbutton.x >= 0 &&
			      event->xbutton.y >= 0 &&
			      event->xbutton.x < (int)b->width &&
			      event->xbutton.y < (int)b->height)
			  ? True : False);
	return(1);
    default:
	break;
    }

    return(0);
}

void RedrawButton(Button b)
{
    static GC gc = 0;
    static Pixmap grey = 0;
    int bh = b->height, bw = b->width;

    if (gc == 0) {
	gc = XCreateGC(b->display, b->window, 0, NULL);
	if (buttonFont == NULL)
	    SetButtonFont(b->display);
	XSetFont(b->display, gc, buttonFont->fid);
    }

    if (grey == 0) {
	grey = XCreateBitmapFromData(b->display, b->window, (char *) grey_bits,
				     grey_width, grey_height);
	XSetStipple(b->display, gc, grey);
    }

    XSetBackground(b->display, gc, background);

    XClearWindow(b->display, b->window);

    XSetForeground(b->display, gc, b->fg);

    if (b->flags & BUTTON_TEXT)
 	XDrawString(b->display, b->window, gc, 5, 5 + buttonFont->ascent,
		    b->image.string, (int)strlen(b->image.string));
    else if (b->image.icon != None) {
	int x, y;
	unsigned w, h;
	
	w = (b->imagewidth);
	if (w > b->width)
	    w = b->width;
	h = (b->imageheight);
	if (h > b->height)
	    h = b->height;
	x = (b->width - w) >> 1;
	y = (b->height - h) >> 1;
	XCopyPlane(b->display, b->image.icon, b->window, gc,
		   (int)((b->imagewidth-w) >> 1),
		   (int)((b->imageheight-h) >> 1),
		   w, h, x, y, 1);
    }

    if (b->flags & BUTTON_DISABLED) {
	XSetForeground(b->display, gc, background);
	    XSetFillStyle(b->display, gc, FillStippled);
	XFillRectangle(b->display, b->window, gc, 0, 0, b->width, b->height);
	XSetFillStyle(b->display, gc, FillSolid);
    }

    if (b->flags & BUTTON_PRESSED) {
	XSetForeground(b->display, gc, black);
	XDrawRectangle(b->display, b->window, gc,
		       0, 0, b->width-1, b->height-1);
	XDrawRectangle(b->display, b->window, gc,
		       1, 1, b->width-3, b->height-3);
    } else {
	XSetForeground(b->display, gc, bottomshadow);
	XDrawLine(b->display, b->window, gc, 0, bh-1, bw-1, bh-1);
	XDrawLine(b->display, b->window, gc, bw-1, bh-1, bw-1, 0);
	XSetForeground(b->display, gc, topshadow);
	XDrawLine(b->display, b->window, gc, 0, 0, bw-1, 0);
	XDrawLine(b->display, b->window, gc, 0, 0, 0, bh-1);
	XSetForeground(b->display, gc, bottomshadow);
	XDrawLine(b->display, b->window, gc, 1, bh-2, bw-2, bh-2);
	XDrawLine(b->display, b->window, gc, bw-2, bh-2, bw-2, 1);
	XSetForeground(b->display, gc, topshadow);
	XDrawLine(b->display, b->window, gc, 1, 1, bw-2, 1);
	XDrawLine(b->display, b->window, gc, 1, 1, 1, bh-2);
    }

    XSetForeground(b->display, gc, bottomshadow);
    XDrawLine(b->display, b->window, gc, 2, bh-3, bw-3, bh-3);
    XDrawLine(b->display, b->window, gc, bw-3, bh-3, bw-3, 2);
    XSetForeground(b->display, gc, topshadow);
    XDrawLine(b->display, b->window, gc, 2, 2, bw-3, 2);
    XDrawLine(b->display, b->window, gc, 2, 2, 2, bh-3);

}

void DisableButton(Button b)
{
    if (b->flags & BUTTON_DISABLED)
	return;

    b->flags |= BUTTON_DISABLED;

    RedrawButton(b);
}

void EnableButton(Button b)
{
    if (!(b->flags & BUTTON_DISABLED))
	return;

    b->flags &= ~BUTTON_DISABLED;

    RedrawButton(b);
}

void ReleaseableButton(Button b)
{
    int i;

    i = (b->flags & BUTTON_PRESSED);

    b->flags &= ~BUTTON_PRESSED;
    b->flags |= BUTTON_RELEASE;

    if (i)
	RedrawButton(b);
}

void NonreleaseableButton(Button b)
{
    b->flags &= ~BUTTON_RELEASE;
}

void ChangeButtonGroup(Button b, int group)
{
    if (group == b->group)
	return;

    b->group = group;

    if (b->flags & BUTTON_PRESSED)
	ReleaseButtons(b);
}

void MoveButton(Button b, int x, int y)
{
    XWindowChanges values;

    values.x = x;
    values.y = y;

    XConfigureWindow(b->display, b->window, CWX | CWY, &values);
}

void GetButtonSize(Button b, unsigned *width, unsigned *height)
{
    *width = b->width;
    *height = b->height;
}
