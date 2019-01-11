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

/* How far away objects should be placed from each other etc... */
#define BORDER			10
#define BTN_BORDER		4

/* Information window dimensions */
#define ABOUT_WINDOW_WIDTH	600
#define ABOUT_WINDOW_HEIGHT	700

static bool		about_created = false;


#define NUM_ABOUT_PAGES		5

/*
 * This variable tells us what item comes last on page 0.  If -1 it hasn't
 * been initialized yet (page 0 needs exposing to do this).  If it is
 * NUM_ITEMS-1 then there is no need to split to page and the NEXT and PREV
 * keys will automatically skip that page.
 */
static int itemsplit = -1;

/*
 * General text formatting routine which does wrap around
 * if necessary at whitespaces.  The function returns the
 * vertical position it ended at.
 */
int DrawShadowText(Display *display, Window w, GC gc,
		   int x_border, int y_start, const char *str,
		   unsigned long fg, unsigned long bg)
{
    XFontStruct		*font = XQueryFont(display, XGContextFromGC(gc));
    int			y, x, tmp;
    int                 count = 1;
    XWindowAttributes	wattr;

    if (str == NULL || *str == '\0')
	return 0;

    /* Get width of window */
    XGetWindowAttributes(display, w, &wattr);

    /* Start position */
    x = x_border;
    y = y_start + font->ascent;

    do {
	char word[LINE_MAX];
	int wordLen, i;

	for (i = 0; *str && !isspace(*str) && i < LINE_MAX-1; str++, i++)
	    word[i] = *str;
	word[i] = '\0';

	/* Word length in pixels */
	wordLen = XTextWidth(font, word, i);

	/* We need a linebreak? */
	if (x + wordLen > wattr.width - BORDER) {
	    x = x_border;
	    y += font->ascent + font->descent + 1;
	}

	/* Draw word and move cursor to point to after this word */
	ShadowDrawString(display, w, gc, x, y, word, fg, bg);
	x += wordLen;

	/* Handle whitespace */
	for (; isspace(*str); str++)
	    switch (*str) {
		/* New paragraph */
	    case '\n':
		x = x_border;
		y += font->ascent + font->descent + 1;
		break;

		/* Just a space */
	    default:
		x += XTextWidth(font, " ", 1);
		break;
	    }
    } while (*str != '\0');

    tmp = font->descent+1;

    XFreeFontInfo(NULL, font, count);

    return y + tmp;
}


void Expose_about_window(void)
{
    int	i, y, old_y, box_start, box_end, first, last;

    XClearWindow(dpy, aboutWindow);

    switch (about_page) {

    case 0:
	DrawShadowText(dpy, aboutWindow, textGC,
	BORDER, BORDER,
	"ABOUT XPILOT\n"
	"\n"
	"The game was conceived in its original form at the "
	"University of Tromsø (Norway) by Ken Ronny Schouten and "
	"Bjørn Stabell during the fall of 1991, but much of the game today "
	"is the result of hard efforts by Bert Gijsbers of the "
	"molecular cytology lab at the University of Amsterdam "
	"(The Netherlands).  "
	"Bert joined the team in the spring of 1993.\n"
	"\n"
	"Dick Balaska (Connecticut, USA) ported XPilot to Windows 95 and NT "
	"in the summer of 1996.\n"
	"\n"
	"A large number of features have been contributed by XPilot fans from "
	"all around the world.  See the CREDITS file for details.\n"
	"\n"
	"For more information, "
	"read the XPilot FAQ (Frequently Asked Questions).\n"
	"\n\n"
	"Good luck as a future xpilot,\n"
	"Bjørn Stabell, Ken Ronny Schouten, Bert Gijsbers & Dick Balaska",
	colors[WHITE].pixel, colors[BLACK].pixel);
	break;

    case 1:
	DrawShadowText(dpy, aboutWindow, textGC,
	BORDER, BORDER,
	"ABOUT XPILOT NG\n"
	"\n"
	"XPilot NG is an improved version of XPilot.\n\n"
	"For more info visit http://xpilot.sourceforge.net/ or\n"
	"read the man pages xpilot-ng-x11(6) and xpilot-ng-server(6).\n\n"
	"You can report any bug you find to <" PACKAGE_BUGREPORT ">.",
	colors[WHITE].pixel, colors[BLACK].pixel);
	break;

    case 2:
	DrawShadowText(dpy, aboutWindow, textGC,
	BORDER, BORDER,
	"GAME OBJECTIVE\n"
	"\n"
	"XPilot is a multi-player 2D space game.  "
	"Some features are borrowed from classics like the Atari coin-ups "
	"Asteroids and Gravitar, and the home-computer games "
	"Thrust (Commdore 64) and Gravity Force, but XPilot has many "
	"new features as well.\n"
	"\n"
	"The primary goal of the game is to collect points and increase "
	"your rating by destroying enemy fighters and cannons.  "
	"You are equipped with a machine gun when you start the game, "
	"but after a while you should have managed to collect some other "
	"fancy equipment.\n"
	"\n"
	"Another important task is to refuel your ship.  This is "
	"vital because your engine, radar, weapons and shields all "
	"require fuel.  Some even work better the more fuel you "
	"have aboard (mainly the radar).\n"
	"\n"
	"Optional modes include variations on this game play: "
	"you can play together in teams, you can disable shields "
	"(and all other equipment if you like), "
	"you can race against time and fellow players, and much much more.",
	colors[WHITE].pixel, colors[BLACK].pixel);
	break;

    case 3:
    case 4:
	if (about_page == 3) {
	    y = DrawShadowText(dpy, aboutWindow, textGC,
			   BORDER, BORDER,
			   "BONUS ITEMS\n"
			   "\n"
			   "Scattered around the world you might find some "
			   "of these red triangle objects.  They are "
			   "well worth picking up since they either improve "
			   "on the equipment you have, or they give you "
			   "new equipment.  If a fighter explodes, some of "
			   "its equipment might be found among the debris.",
			   colors[WHITE].pixel, colors[BLACK].pixel);
	    first = 0;
	    last = (itemsplit == -1) ? (NUM_ITEMS-1) : itemsplit;
	} else {
	    y = DrawShadowText(dpy, aboutWindow, textGC,
			   BORDER, BORDER,
			   "BONUS ITEMS CONTINUED\n",
			   colors[WHITE].pixel, colors[BLACK].pixel);
	    first = itemsplit+1;
	    last = (NUM_ITEMS-1);
	}

	y += BORDER;
	box_start = y;
	y += BORDER / 2;
	for (i = first; i <= last; i++) {

	    y += BORDER / 2;

	    /* Draw description text */
	    old_y = y;
	    y = DrawShadowText(dpy, aboutWindow, textGC,
			       5*BORDER + 2*ITEM_SIZE, old_y,
			       Item_get_text(i),
			       colors[WHITE].pixel, colors[BLACK].pixel);
	    if (y - old_y < 2 * ITEM_TRIANGLE_SIZE)
		y = old_y + 2 * ITEM_TRIANGLE_SIZE;
	    box_end = y + BORDER / 2;
	    if (i == last)
		box_end += BORDER / 2;

	    /* Paint the item on the left side */
	    XSetForeground(dpy, textGC, colors[BLACK].pixel);
	    XFillRectangle(dpy, aboutWindow, textGC,
			   BORDER, box_start,
			   2*ITEM_SIZE+2*BORDER,
			   (unsigned)box_end - box_start);
	    XSetForeground(dpy, textGC, colors[RED].pixel);
	    Gui_paint_item((u_byte)i, aboutWindow, textGC, 2*BORDER + ITEM_SIZE,
		       old_y + ITEM_TRIANGLE_SIZE);
	    XSetForeground(dpy, textGC, colors[WHITE].pixel);

	    /*
	     * Check for items overlapping button window, if so then
	     * remove this item, set itemsplit to previous item and
	     * stop adding more items.
	     */
	    if (about_page == 3
		&& itemsplit == -1
		&& box_end >= (ABOUT_WINDOW_HEIGHT - BORDER * 2 - 4
			       - (2*BTN_BORDER + buttonFont->ascent
				  + buttonFont->descent))) {
		itemsplit = i-1;
		XSetForeground(dpy, textGC, colors[windowColor].pixel);
		XFillRectangle(dpy, aboutWindow, textGC,
			       BORDER, box_start,
			       ABOUT_WINDOW_WIDTH,
			       (unsigned)box_end - box_start);
		XSetForeground(dpy, textGC, colors[WHITE].pixel);
		break;
	    }

	    y = box_end;
	    box_start = box_end;

	}
	/*
	 * No page split, obviously font is small enough or not enough
	 * items.
	 */
	if (about_page == 3 && itemsplit == -1)
	    itemsplit = NUM_ITEMS-1;
	break;

    default:
	error("Unkown page number %d\n", about_page);
	break;
    }
}


static void About_create_window(void)
{
    const unsigned int		windowWidth = ABOUT_WINDOW_WIDTH,
				buttonWindowHeight = 2*BTN_BORDER
				    + buttonFont->ascent + buttonFont->descent,
				windowHeight = ABOUT_WINDOW_HEIGHT;
    unsigned			textWidth;
    XSetWindowAttributes	sattr;
    unsigned long		mask;


    /*
     * Create the window and initialize window name.
     */
    mask = 0;
    sattr.background_pixel = colors[windowColor].pixel;
    mask |= CWBackPixel;
    sattr.border_pixel = colors[borderColor].pixel;
    mask |= CWBorderPixel;
    if (colormap != 0) {
	sattr.colormap = colormap;
	mask |= CWColormap;
    }
    sattr.backing_store = Always;
    mask |= CWBackingStore;

    aboutWindow
	= XCreateWindow(dpy,
			DefaultRootWindow(dpy),
			0, 0,
			windowWidth, windowHeight,
			2, (int)dispDepth,
			InputOutput, visual,
			mask, &sattr);
    XStoreName(dpy, aboutWindow, "XPilot - information");
    XSetIconName(dpy, aboutWindow, "XPilot/info");
    XSetTransientForHint(dpy, aboutWindow, topWindow);

    textWidth = XTextWidth(buttonFont, "CLOSE", 5);
    about_close_b
	= XCreateSimpleWindow(dpy, aboutWindow,
			      BORDER,
			      (int)(windowHeight - BORDER
				    - buttonWindowHeight - 4),
			      2*BTN_BORDER + textWidth,
			      buttonWindowHeight,
			      0, 0,
			      colors[buttonColor].pixel);

    /*
     * Create 'buttons' in the window.
     */
    textWidth = XTextWidth(buttonFont, "PREV", 4);
    about_prev_b
	= XCreateSimpleWindow(dpy, aboutWindow,
			      (int)(windowWidth / 2
				    - BTN_BORDER - textWidth / 2),
			      (int)(windowHeight
				    - BORDER - buttonWindowHeight - 4),
			      2*BTN_BORDER + textWidth, buttonWindowHeight,
			      0, 0,
			      colors[buttonColor].pixel);

    textWidth = XTextWidth(buttonFont, "NEXT", 4);
    about_next_b
	= XCreateSimpleWindow(dpy, aboutWindow,
			      (int)(windowWidth - BORDER
				    - 2*BTN_BORDER - textWidth),
			      (int)(windowHeight - BORDER
				    - buttonWindowHeight - 4),
			      2*BTN_BORDER + textWidth, buttonWindowHeight,
			      0, 0,
			      colors[buttonColor].pixel);

    XSelectInput(dpy, about_close_b,
		 ExposureMask | ButtonPressMask | ButtonReleaseMask);
    XSelectInput(dpy, about_next_b,
		 ExposureMask | ButtonPressMask | ButtonReleaseMask);
    XSelectInput(dpy, about_prev_b,
		 ExposureMask | ButtonPressMask | ButtonReleaseMask);
    XSelectInput(dpy, aboutWindow, ExposureMask);

    Expose_about_window();

    XMapSubwindows(dpy, aboutWindow);
}


void Expose_button_window(int color, Window w)
{
    if (w != about_close_b
	&& w != about_next_b
	&& w != about_prev_b) {
	return;
    }

    {
	XWindowAttributes	wattr;		/* Get window height */
	XGetWindowAttributes(dpy, w, &wattr);	/* and width */

	XSetForeground(dpy, buttonGC, colors[color].pixel);
	XFillRectangle(dpy, w, buttonGC, 0, 0,
		       (unsigned)wattr.width, (unsigned)wattr.height);
	XSetForeground(dpy, buttonGC, colors[WHITE].pixel);
    }

    if (w == about_close_b)
	ShadowDrawString(dpy, w, buttonGC,
			 BTN_BORDER, buttonFont->ascent + BTN_BORDER,
			 "CLOSE",
			 colors[WHITE].pixel, colors[BLACK].pixel);
    if (w == about_prev_b)
	ShadowDrawString(dpy, w, buttonGC,
			 BTN_BORDER, buttonFont->ascent + BTN_BORDER,
			 "PREV",
			 colors[WHITE].pixel, colors[BLACK].pixel);
    if (w == about_next_b)
	ShadowDrawString(dpy, w, buttonGC,
			 BTN_BORDER, buttonFont->ascent + BTN_BORDER,
			 "NEXT",
			 colors[WHITE].pixel, colors[BLACK].pixel);
}


void About(Window w)
{
    if (about_created == false) {
	About_create_window();
	about_created = true;
    }
    if (w == about_close_b) {
	about_page = 0;
	XUnmapWindow(dpy, aboutWindow);
    } else if (w == about_next_b) {
	about_page++;
	if (about_page == 1 && itemsplit >= NUM_ITEMS-1)
	    about_page++;
	if (about_page >= NUM_ABOUT_PAGES)
	    about_page = 0;
	Expose_about_window();
    } else if (w == about_prev_b) {
	about_page--;
	if (about_page == 1 && itemsplit >= NUM_ITEMS-1)
	    about_page--;
	if (about_page <= -1)
	    about_page = NUM_ABOUT_PAGES-1;
	Expose_about_window();
    }
    WinXFlush(aboutWindow);
}


int About_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    if (about_created == false) {
	About_create_window();
	about_created = true;
    }
    XMapWindow(dpy, aboutWindow);
    return 0;
}

/*****************************************************************************/
int		keys_viewer = NO_WIDGET;

int Keys_callback(int widget_desc, void *data, const char **unused)
{
    unsigned	bufsize = (num_keydefs * 64);
    char	*buf = calloc(bufsize, 1), *end = buf, *str;
    const char	*help;
    int		i, len, maxkeylen = 0;

    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(unused);

    for (i = 0; i < num_keydefs; i++) {
	if ((str = XKeysymToString((KeySym)keydefs[i].keysym)) != NULL
	    && (len = strlen(str)) > maxkeylen) {
	    maxkeylen = len;
	}
    }
    for (i = 0; i < num_keydefs; i++) {
	if (!(str = XKeysymToString((KeySym)keydefs[i].keysym))
	    || !(help = Get_keyHelpString(keydefs[i].key)))
	    continue;

	if ((end - buf) + (maxkeylen + strlen(help) + 4) >= bufsize) {
	    bufsize += 4096;
	    xpprintf("realloc: %d\n", bufsize);
	    if (!(buf = realloc(buf, bufsize))) {
		error("No memory for key list");
		return 0;
	    }
	}
	sprintf(end, "%-*s  %s\n", maxkeylen, str, help);
	end += strlen(end);
    }
    keys_viewer =
	Widget_create_viewer(buf,
			     end - buf,
			     2*DisplayWidth(dpy, DefaultScreen(dpy))/3,
			     4*DisplayHeight(dpy, DefaultScreen(dpy))/5,
			     2,
			     "XPilot - key reference", "XPilot:keys",
			     motdFont);
    if (keys_viewer == NO_WIDGET) {
	warn("Can't create key viewer");
	return 0;
    }
#if 0
    else if (keys_viewer != NO_WIDGET)
	Widget_map(keys_viewer);
#endif
    return 0;
}

void Keys_destroy(void)
{
    Widget_destroy(keys_viewer);
    keys_viewer = NO_WIDGET;
    /*keys_created = false;*/
}


#define MAX_MOTD_SIZE	(30*1024)

static char		*motd_buf = NULL;
static size_t		motd_size;
       int		motd_viewer = NO_WIDGET;
static bool		motd_auto_popup;

int Motd_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);

    /* always refresh motd */
    motd_auto_popup = false;
    Net_ask_for_motd(0, MAX_MOTD_SIZE);
    Net_flush();

    if (motd_viewer != NO_WIDGET)
	Widget_map(motd_viewer);
    return 0;
}

void Motd_destroy(void)
{
    Widget_destroy(motd_viewer);
    motd_viewer = NO_WIDGET;
    XFREE(motd_buf);
}

int Handle_motd(long off, char *buf, int len, long filesize)
{
    int			i;
    static char		no_motd_msg[] = "\nThis server has no MOTD.\n\n";

    if (!motd_buf) {
	motd_size = MIN(filesize, MAX_MOTD_SIZE);
	i = MAX(motd_size, (long)(sizeof no_motd_msg)) + 1;
	if (!(motd_buf = malloc((size_t)i))) {
	    error("No memory for MOTD");
	    return -1;
	}
	memset(motd_buf, ' ', motd_size);
	for (i = 39; i < (int)motd_size; i += 40)
	    motd_buf[i] = '\n';
    }
    else if (filesize < (long)motd_size) {
	motd_size = filesize;
	motd_buf[motd_size] = '\0';
    }
    if (off < (long)motd_size && len > 0) {
	if (off + len > (long)motd_size)
	    len = motd_size - off;
	memcpy(motd_buf + off, buf, (size_t)len);
    }
    else if (len == 0 && off > 0)
	return 0;

    if (motd_size == 0) {
	if (motd_auto_popup) {
	    XFREE(motd_buf);
	    return 0;
	}
	strcpy(motd_buf, no_motd_msg);
	motd_size = strlen(motd_buf);
    }
    if (motd_viewer == NO_WIDGET) {
	char title[100];

	snprintf(title, sizeof(title), "XPilot motd from %s", servername);
	motd_viewer = Widget_create_viewer(
	    motd_buf,
	    (off || len) ? (off + len) : (int)strlen(motd_buf),
	    2*DisplayWidth(dpy, DefaultScreen(dpy))/3,
	    4*DisplayHeight(dpy, DefaultScreen(dpy))/8,
	    2,
	    title, "XPilot:motd",
	    motdFont);
	if (motd_viewer == NO_WIDGET)
	    warn("Can't create MOTD viewer");
    }
    else if (len > 0)
	Widget_update_viewer(motd_viewer, motd_buf, off + len);

    return 0;
}

void aboutCleanup(void)
{
    XFREE(motd_buf);
}

#ifdef _WINDOWS
int Credits_callback(int widget_desc, void *data, const char **unused)
{
    extern	void DoWinAboutBox();
    DoWinAboutBox();
    return(0);
}
#endif

