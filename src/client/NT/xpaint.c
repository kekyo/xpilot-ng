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
 * Copyright (C) 2003 Kristian Söderblom <kps@users.sourceforge.net>
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

/*
 * Globals.
 */
Display		*dpy;		/* Display of player (pointer) */
Display		*kdpy;		/* Keyboard display */
short		about_page;	/* Which page is the player on? */

GC		gameGC;		/* GC for the game area */
GC		messageGC;	/* GC for messages in the game area */
GC		radarGC;	/* GC for the radar */
GC		buttonGC;	/* GC for the buttons */
GC		scoreListGC;	/* GC for the player list */
GC		textGC;		/* GC for the info text */
GC		talkGC;		/* GC for the message window */
GC		motdGC;		/* GC for the motd text */
XGCValues	gcv;

Window	topWindow;		/* Top-level window (topshell) */
Window	drawWindow;		/* Main play window */
Window	keyboardWindow;		/* Keyboard window */
#ifdef _WINDOWS
/*
 * Windows needs some dummy windows (size 0,0)
 * so we can store the active fonts.
 */
				/* supports 1 active font per window */
Window	textWindow;		/* for the GC into the config window */
Window	msgWindow;		/* for messages into the playfield */
Window	buttonWindow;		/* to calculate size of buttons */
#endif

Pixmap	drawPixmap;		/* Saved pixmap for the drawing */
				/* area (monochromes use this) */
Window	playersWindow;		/* Player list window */
				/* monochromes) */
Window	aboutWindow;
Window	about_close_b;		/* About window's close button */
Window	about_next_b;		/* About window's next button */
Window	about_prev_b;		/* About window's previous button */
Window	keys_close_b;		/* Help window's close button */
Window	talkWindow;
bool	gotFocus;
bool	players_exposed;

static int clockColor;		/* Clock color index */
static int scoreColor;		/* Score list color indices */
static int scoreSelfColor;	/* Score list own score color index */
static int scoreInactiveColor;	/* Score list inactive player color index */
static int scoreInactiveSelfColor;
				/* Score list inactive self color index */
static int scoreOwnTeamColor;	/* Score list own team color index */
static int scoreEnemyTeamColor;	/* Score list enemy team color index */

static void Paint_clock(bool redraw);

int Paint_init(void)
{
    if (Init_wreckage() == -1)
	return -1;

    if (Init_asteroids() == -1)
	return -1;

    if (Bitmaps_init() == -1)
	return -1;
    
    return 0;
}

void Paint_cleanup(void)
{
    Bitmaps_cleanup();
}

void Paint_frame(void)
{
    static long		scroll_i = 0;
    static int		prev_damaged = 0;
    static int		prev_prev_damaged = 0;

#ifdef _WINDOWS
    /* give any outgoing data a head start to the server */
    /* send anything to the server before returning to Windows */
    Net_flush();
#endif

    Paint_frame_start();
    Paint_score_table();

    /*
     * Switch between two different window titles.
     */
    if (titleFlip && ((loopsSlow % TITLE_DELAY) == 0)) {
	scroll_i = !scroll_i;
	if (scroll_i)
	    XStoreName(dpy, topWindow, COPYRIGHT);
	else
	    XStoreName(dpy, topWindow, TITLE);
    }

    /* This seems to have a bug (in Windows) 'cause last frame we ended
       with an XSetForeground(white) confusing SET_FG */
    SET_FG(colors[BLACK].pixel);

#ifdef _WINDOWS
    drawPixmap = drawWindow;		/* let's try this */
    XSetForeground(dpy, gameGC, colors[BLACK].pixel);
    XFillRectangle(dpy, drawPixmap, gameGC, 0, 0, draw_width, draw_height);
#endif

    rd.newFrame();


    /*
     * Do we really need to draw all this if the player is damaged?
     */
    if (damaged <= 0) {
	if (prev_damaged || prev_prev_damaged) {
	    /* clean up ecm damage */
	    SET_FG(colors[BLACK].pixel);
	    XFillRectangle(dpy, drawWindow, gameGC,
			   0, 0, draw_width, draw_height);
	}

	Arc_start();

	Rectangle_start();
	Segment_start();

	Paint_world();

	Segment_end();
	Rectangle_end();

	Rectangle_start();
	Segment_start();

	if (oldServer) {
	    Paint_vfuel();
	    Paint_vdecor();
	    Paint_vcannon();
	    Paint_vbase();
	} else
	    Paint_objects();

	Paint_shots();

	Rectangle_end();
	Segment_end();

	Rectangle_start();
	Segment_start();

	Paint_ships();
	Paint_meters();
	Paint_HUD();
	Paint_recording();
	Paint_HUD_values();

	Rectangle_end();
	Segment_end();

	Arc_end();

	Paint_messages();
	Paint_radar();
	Paint_score_objects();
    }
    else {
	/* Damaged. */

	XSetFunction(dpy, gameGC, GXxor);
	SET_FG(colors[BLACK].pixel ^ colors[BLUE].pixel);
	XFillRectangle(dpy, drawWindow, gameGC, 0, 0, draw_width, draw_height);
	XSetFunction(dpy, gameGC, GXcopy);
	SET_FG(colors[BLACK].pixel);
    }
    prev_prev_damaged = prev_damaged;
    prev_damaged = damaged;

    rd.endFrame();

    if (radar_exposures == 1)
	Paint_world_radar();

    /*
     * Now switch planes and clear the screen.
     */
    if (radarPixmap != radarWindow && radar_exposures > 0) {
	if (!instruments.slidingRadar
	    || BIT(Setup->mode, WRAP_PLAY) == 0) {
#ifndef _WINDOWS
	    XCopyArea(dpy, radarPixmap, radarWindow, gameGC,
		      0, 0, 256, RadarHeight, 0, 0);
#else
	    WinXBltPixToWin(radarPixmap, radarWindow,
			    0, 0, 256, RadarHeight, 0, 0);
#endif
	} else {
	    int x, y;
	    double xp, yp, xo, yo;
	    unsigned w1, h1, w2, h2;

	    xp = (double) (selfPos.x * 256) / Setup->width;
	    yp = (double) (selfPos.y * RadarHeight) / Setup->height;
	    xo = (double) 256 / 2;
	    yo = (double) RadarHeight / 2;

	    assert(xp >= 0.0);
	    assert(xp < 256.0);
	    assert(yp >= 0.0);
	    assert(yp < RadarHeight);

	    /* kps - i don't see what adding 0.5 to these helps. */
	    x = xp - xo /*+ 0.5*/;
	    y = yp - yo /*+ 0.5*/;
	    if (x <= 0.0)
		x += 256;
	    if (y <= 0.0)
		y += RadarHeight;
	    w1 = (unsigned)x;
	    h1 = (unsigned)y;

	    assert(w1 > 0);
	    assert(w1 <= 256);
	    assert(h1 > 0);
	    assert(h1 <= RadarHeight);

	    h2 = RadarHeight - h1;
	    w2 = 256 - w1;

#ifndef _WINDOWS
	    XCopyArea(dpy, radarPixmap, radarWindow, gameGC,
		      0, (int)h2, w1, h1, (int)w2, 0);
	    if (w2 > 0)
		XCopyArea(dpy, radarPixmap, radarWindow, gameGC,
			  (int)w1, (int)h2, w2, h1, 0, 0);
	    if (h2 > 0)
		XCopyArea(dpy, radarPixmap, radarWindow, gameGC,
			  0, 0, w1, h2, (int)w2, (int)h1);
	    if (w2 > 0 && h2 > 0)
		XCopyArea(dpy, radarPixmap, radarWindow, gameGC,
			  (int)w1, 0, w2, h2, 0, (int)h1);
#else
	    Paint_world_radar();
#endif
	}
    }
    else if (radar_exposures > 2)
	Paint_world_radar();

#ifndef _WINDOWS
    if (dbuf_state->type == PIXMAP_COPY)
	XCopyArea(dpy, drawPixmap, drawWindow, gameGC,
		  0, 0, draw_width, draw_height, 0, 0);

    dbuff_switch(dbuf_state);

    if (dbuf_state->type == COLOR_SWITCH) {
	XSetPlaneMask(dpy, gameGC, dbuf_state->drawing_planes);
	XSetPlaneMask(dpy, messageGC, dbuf_state->drawing_planes);
    }
#endif

    if (!damaged) {
	/* Prepare invisible buffer for next frame by clearing. */

	/*
	 * DBE's XdbeBackground switch option is
	 * probably faster than XFillRectangle.
	 */
#ifndef _WINDOWS
	if (dbuf_state->multibuffer_type != MULTIBUFFER_DBE) {
	    SET_FG(colors[BLACK].pixel);
	    XFillRectangle(dpy, drawPixmap, gameGC,
			   0, 0, draw_width, draw_height);
	}
#endif
    }

#ifndef _WINDOWS
    if (clData.talking) {
	static bool toggle;
	static long last_toggled;

	if (loops >= last_toggled + FPS / 2 || loops < last_toggled) {
	    toggle = (toggle == false) ? true : false;
	    last_toggled = loops;
	}
	Talk_cursor(toggle);
    }
#endif

#ifdef _WINDOWS
    Paint_score_table();
    PaintWinClient();
#endif

    Paint_clock(false);

    XFlush(dpy);
}


#define SCORE_BORDER		6


static void Paint_score_background(void)
{
    if (fullColor &&
	Bitmap_get(playersWindow, BM_SCORE_BG, 0) != NULL &&
	Bitmap_get(playersWindow, BM_LOGO, 0) != NULL) {
	unsigned bgh, lh;

	XSetForeground(dpy, scoreListGC, colors[BLACK].pixel);

	bgh = pixmaps[BM_SCORE_BG].height;
	lh = pixmaps[BM_LOGO].height;

	IFWINDOWS( XFillRectangle(dpy, playersWindow, scoreListGC,
				  0, 0, players_width, players_height) );

	Bitmap_paint(playersWindow, BM_SCORE_BG, 0, 0, 0);
	if (players_height > bgh + lh)
	    XFillRectangle(dpy, playersWindow, scoreListGC,
			   0, (int)bgh,
			   players_width, players_height - (bgh + lh));
	Bitmap_paint(playersWindow, BM_LOGO, 0, (int)(players_height - lh), 0);
	XFlush(dpy);
    } else {
	XSetForeground(dpy, scoreListGC, colors[windowColor].pixel);
	XFillRectangle(dpy, playersWindow, scoreListGC,
		       0, 0, players_width, players_height);
	XFlush(dpy);
    }
}


void Paint_score_start(void)
{
    char	headingStr[MSG_LEN];
    static int thisLine;

    thisLine = SCORE_BORDER + scoreListFont->ascent;

    if (showUserName)
	strlcpy(headingStr, "NICK=USER@HOST", sizeof(headingStr));
    else if (BIT(Setup->mode, TEAM_PLAY))
	strlcpy(headingStr, "     SCORE   NAME     LIFE", sizeof(headingStr));
    else {
	strlcpy(headingStr, "  ", sizeof(headingStr));
	if (BIT(Setup->mode, TIMING))
	    strcat(headingStr, "LAP ");
	strlcpy(headingStr, " AL ", sizeof(headingStr));
	strcat(headingStr, "  SCORE  ");
	if (BIT(Setup->mode, LIMITED_LIVES))
	    strlcat(headingStr, "LIFE", sizeof(headingStr));
	strlcat(headingStr, " NAME", sizeof(headingStr));
    }
    Paint_score_background();

    ShadowDrawString(dpy, playersWindow, scoreListGC,
		     SCORE_BORDER, thisLine,
		     headingStr,
		     colors[scoreColor].pixel,
		     colors[BLACK].pixel);

    gcv.line_style = LineSolid;
    XChangeGC(dpy, scoreListGC, GCLineStyle, &gcv);
    XDrawLine(dpy, playersWindow, scoreListGC,
	      SCORE_BORDER, thisLine,
	      (int)players_width - SCORE_BORDER, thisLine);

    gcv.line_style = LineOnOffDash;
    XChangeGC(dpy, scoreListGC, GCLineStyle, &gcv);

    Paint_clock(true);
}


void Paint_score_entry(int entry_num, other_t* other, bool is_team)
{
    static char		raceStr[8], teamStr[4], lifeStr[8], label[MSG_LEN];
    static int		lineSpacing = -1, firstLine;
    int			thisLine, color;
    char		scoreStr[16];

    /*
     * First time we're here, set up miscellaneous strings for
     * efficiency and calculate some other constants.
     */
    if (lineSpacing == -1) {
	memset(raceStr, 0, sizeof raceStr);
	memset(teamStr, 0, sizeof teamStr);
	memset(lifeStr, 0, sizeof lifeStr);
	teamStr[1] = ' ';
	raceStr[2] = ' ';

	lineSpacing
	    = scoreListFont->ascent + scoreListFont->descent + 3;
	firstLine
	    = 2*SCORE_BORDER + scoreListFont->ascent + lineSpacing;
    }
    thisLine = firstLine + lineSpacing * entry_num;

    /*
     * Setup the status line
     */
    if (showUserName)
	sprintf(label, "%s=%s@%s",
		other->nick_name, other->user_name, other->host_name);
    else {
	if (BIT(Setup->mode, TIMING)) {
	    raceStr[0] = ' ';
	    raceStr[1] = ' ';
	    if ((other->mychar == ' ' || other->mychar == 'R')
		&& other->round + other->check > 0) {
		if (other->round > 99)
		    sprintf(raceStr, "%3d", other->round);
		else
		    sprintf(raceStr, "%d.%c",
			    other->round, other->check + 'a');
	    }
	}
	if (BIT(Setup->mode, TEAM_PLAY))
	    teamStr[0] = other->team + '0';
	else
	    sprintf(teamStr, "%c", other->alliance);

	if (BIT(Setup->mode, LIMITED_LIVES))
	    sprintf(lifeStr, " %3d", other->life);

	if (Using_score_decimals())
	    sprintf(scoreStr, "%*.*f",
		    9 - showScoreDecimals, showScoreDecimals,
		    other->score);
	else {
	    double score = other->score;
	    int sc = (int)(score >= 0.0 ? score + 0.5 : score - 0.5);
	    sprintf(scoreStr, "%6d", sc);
	}

	if (BIT(Setup->mode, TEAM_PLAY))
	    sprintf(label, "%c %s  %-18s%s",
		    other->mychar, scoreStr, other->nick_name, lifeStr);
	else
	    sprintf(label, "%c %s%s%s%s  %s",
		    other->mychar, raceStr, teamStr,
		    scoreStr, lifeStr,
		    other->nick_name);
    }

    /*
     * Draw the line
     * e94_msu eKthHacks
     */
    if (!is_team && strchr("DPW", other->mychar)) {
	if (self && other->id == self->id)
	    color = scoreInactiveSelfColor;
	else
	    color = scoreInactiveColor;

	XSetForeground(dpy, scoreListGC, colors[color].pixel);
	XDrawString(dpy, playersWindow, scoreListGC,
		    SCORE_BORDER, thisLine,
		    label, (int)strlen(label));
    } else {
	if (!is_team) {
	    if (self && other->id == self->id)
		color = scoreSelfColor;
	    else
		color = scoreColor;
	} else {
	    color = Team_color(other->team);
	    if (!color) {
		if (self && other->team == self->team)
		    color = scoreOwnTeamColor;
		else
		    color = scoreEnemyTeamColor;
	    }
	}

	ShadowDrawString(dpy, playersWindow, scoreListGC, SCORE_BORDER,
			 thisLine, label,
			 colors[color].pixel,
			 colors[BLACK].pixel);
    }

    /*
     * Underline the teams
     */
    if (is_team) {
	color = (windowColor != BLUE ? BLUE : BLACK);
	XSetForeground(dpy, scoreListGC, colors[color].pixel);
	gcv.line_style = LineSolid;
	XChangeGC(dpy, scoreListGC, GCLineStyle, &gcv);
	XDrawLine(dpy, playersWindow, scoreListGC,
		  SCORE_BORDER, thisLine,
		  (int)players_width - SCORE_BORDER, thisLine);
	gcv.line_style = LineOnOffDash;
	XChangeGC(dpy, scoreListGC, GCLineStyle, &gcv);
    }
}


struct team_score {
    double	score;
    int		life;
    int		playing;
};

static void Paint_clock(bool redraw)
{
    int			second,
			minute,
			hour,
			border = 3;
    struct tm		*m;
    char		buf[16];
    static unsigned	width;
    unsigned		height = scoreListFont->ascent + scoreListFont->descent
				+ 3;

    if (!clockColor) {
	if (width != 0) {
	    XSetForeground(dpy, scoreListGC, colors[windowColor].pixel);
	    /* kps - Vato does not want this to be done on windows */
	    IFNWINDOWS(XFillRectangle(dpy, playersWindow, scoreListGC,
				      256 - (int)(width + 2 * border), 0,
				      width + 2 * border, height));
	    width = 0;
	}
	return;
    }

    if (!redraw && !newSecond)
	return;

    m = localtime(&currentTime);
    second = m->tm_sec;
    minute = m->tm_min;
    hour = m->tm_hour;
    /*warn("drawing clock at %02d:%02d:%02d", hour, minute, second);*/

    if (!instruments.clockAMPM)
	sprintf(buf, "%02d:%02d" /*":%02d"*/, hour, minute /*, second*/);
    else {
	char tmpchar = 'A';
	/* strftime(buf, sizeof(buf), "%l:%M%p", m); */
	if (hour > 12){
	    tmpchar = 'P';
	    hour %= 12;
	}
	sprintf(buf, "%2d:%02d%cM", hour, minute, tmpchar);
    }
    width = XTextWidth(scoreListFont, buf, (int)strlen(buf));
    XSetForeground(dpy, scoreListGC, colors[windowColor].pixel);
    /* kps - Vato does not want this to be done on windows */ 
    IFNWINDOWS(XFillRectangle(dpy, playersWindow, scoreListGC,
			      256 - (int)(width + 2 * border), 0,
			      width + 2 * border, height));
    ShadowDrawString(dpy, playersWindow, scoreListGC,
		     256 - (int)(width + border),
		     scoreListFont->ascent + 4,
		     buf,
		     colors[clockColor].pixel,
		     colors[BLACK].pixel);
}


void ShadowDrawString(Display *display, Window w, GC gc,
		      int x, int y, const char* str,
		      unsigned long fg, unsigned long bg)
{
    XSetForeground(display, gc, bg);
    XDrawString(display, w, gc, x+1, y+1, str, (int)strlen(str));
    x--; y--;
    XSetForeground(display, gc, fg);
    XDrawString(display, w, gc, x, y, str, (int)strlen(str));
}

void Play_beep(void)
{
    XBell(dpy, 0);
    XFlush(dpy);
}


xp_option_t xpaint_options[] = {
    COLOR_INDEX_OPTION(
	"clockColor",
	1,
	&clockColor,
	"Which color number to use for drawing the clock.\n"
	"The clock is displayed in the top right of the score window.\n"),

    COLOR_INDEX_OPTION(
	"scoreColor",
	1,
	&scoreColor,
	"Which color number to use for drawing score list entries.\n"),

    COLOR_INDEX_OPTION(
	"scoreSelfColor",
	3,
	&scoreSelfColor,
	"Which color number to use for drawing your own score.\n"),

    COLOR_INDEX_OPTION(
	"scoreInactiveColor",
	12,
	&scoreInactiveColor,
	"Which color number to use for drawing inactive players's scores.\n"),

    COLOR_INDEX_OPTION(
	"scoreInactiveSelfColor",
	12,
	&scoreInactiveSelfColor,
	"Which color number to use for drawing your score when inactive.\n"),

    COLOR_INDEX_OPTION(
	"scoreOwnTeamColor",
	4,
	&scoreOwnTeamColor,
	"Which color number to use for drawing your own team score.\n"),

    COLOR_INDEX_OPTION(
	"scoreEnemyTeamColor",
	11,
	&scoreEnemyTeamColor,
	"Which color number to use for drawing enemy team score.\n"),
};

void Store_xpaint_options(void)
{
    STORE_OPTIONS(xpaint_options);
}
