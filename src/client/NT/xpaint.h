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

#ifndef XPAINT_H
#define XPAINT_H

/* how to draw a selection */
#define DRAW_EMPHASIZED		BLUE

/* The fonts used in the game */
extern XFontStruct* gameFont;
extern XFontStruct* messageFont;
extern XFontStruct* scoreListFont;
extern XFontStruct* buttonFont;
extern XFontStruct* textFont;
extern XFontStruct* talkFont;
extern XFontStruct* motdFont;

/* The name of the fonts used in the game */
#define FONT_LEN	256
extern char gameFontName[FONT_LEN];
extern char messageFontName[FONT_LEN];
extern char scoreListFontName[FONT_LEN];
extern char buttonFontName[FONT_LEN];
extern char textFontName[FONT_LEN];
extern char talkFontName[FONT_LEN];
extern char motdFontName[FONT_LEN];

#define NUM_DASHES		2
#define NUM_CDASHES		2
#define DASHES_LENGTH		12

extern char	dashes[NUM_DASHES];
extern char	cdashes[NUM_CDASHES];

extern Display	*dpy;			/* Display of player (pointer) */
extern Display	*kdpy;			/* Keyboard display */
extern short	about_page;		/* Which page is the player on? */
extern int	radar_exposures;	/* Is radar window exposed? */
extern bool     radar_score_mapped;     /* Is the radar and score window mapped */
					/* windows has 2 sets of item bitmaps */
#define	ITEM_HUD	0		/* one color for the HUD */
#define	ITEM_PLAYFIELD	1		/* and one color for the playfield */
#ifdef _WINDOWS
extern Pixmap	itemBitmaps[][2];
#else
extern Pixmap	itemBitmaps[];
#endif

extern GC	gameGC, messageGC, radarGC, buttonGC;
extern GC	scoreListGC, textGC, talkGC, motdGC;
extern XGCValues gcv;
extern Window	topWindow, drawWindow, keyboardWindow;
extern Window	radarWindow, playersWindow;
#ifdef _WINDOWS				/* see paint.c for details */
extern Window	textWindow, msgWindow, buttonWindow;
#endif
extern Pixmap	drawPixmap;		/* Drawing area pixmap */
extern Pixmap	radarPixmap;		/* Radar drawing pixmap */
extern Pixmap	radarPixmap2;		/* Second radar drawing pixmap */
extern long	dpl_1[2];		/* Used by radar hack */
extern long	dpl_2[2];		/* Used by radar hack */
extern Window	aboutWindow;		/* The About window */
extern Window	about_close_b;		/* About close button */
extern Window	about_next_b;		/* About next page button */
extern Window	about_prev_b;		/* About prev page button */
extern Window	talkWindow;		/* Talk window */
extern XColor	colors[MAX_COLORS];	/* Colors */
extern Colormap	colormap;		/* Private colormap */
extern int	maxColors;		/* Max. number of colors to use */
extern bool	titleFlip;		/* Do special titlebar flipping? */

extern int	(*radarDrawRectanglePtr)/* Function to draw player on radar */
		(Display *disp, Drawable d, GC gc,
		 int x, int y, unsigned width, unsigned height);

extern unsigned long	current_foreground;

static inline void SET_FG(unsigned long fg)
{
    if (fg != current_foreground)
	XSetForeground(dpy, gameGC, current_foreground = fg);
}

/*
 * MS compiler might not grok this, in that case define inline to empty.
 */
static inline void Check_name_string(other_t *other)
{
    if (other && other->max_chars_in_names != maxCharsInNames) {
	int len;

	strlcpy(other->id_string, other->nick_name, sizeof(other->id_string));
	len = strlen(other->id_string);
	if (maxCharsInNames >= 0 && maxCharsInNames < len)
	    other->id_string[maxCharsInNames] = '\0';
	other->name_len = strlen(other->id_string);                 
	other->name_width
	    = 2 + XTextWidth(gameFont, other->id_string, other->name_len);
	other->max_chars_in_names = maxCharsInNames;
    }
}

extern void Paint_item_symbol(int type, Drawable d, GC mygc,
			      int x, int y, int color);
extern void Paint_item(int type, Drawable d, GC mygc, int x, int y);
extern void Gui_paint_item_symbol(int type, Drawable d, GC mygc,
				  int x, int y, int c);
extern void Gui_paint_item(int type, Drawable d, GC mygc, int x, int y);

extern void Store_xpaint_options(void);

/*
 * colors.c
 */
extern void List_visuals(void);
extern int Colors_init(void);
extern int Colors_init_bitmaps(void);
extern void Colors_free_bitmaps(void);
extern void Colors_cleanup(void);
extern void Colors_debug(void);
extern void Init_spark_colors(void);
extern void Store_color_options(void);

#endif
