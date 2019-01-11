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
 * Copyright (C) 2003-2004 Kristian Söderblom <kps@users.sourceforge.net>
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

bool	titleFlip;		/* Do special title bar flipping? */

bool pre_exists = False;
int pre_acc_num, new_acc_num = 0;   /* pre are the Saved mouse settings */
int pre_acc_denom, new_acc_denom = 1; 
int pre_threshold, new_threshold = 0; 
bool mouseAccelInClient = false;

char	gameFontName[FONT_LEN];	/* The fonts used in the game */
char	messageFontName[FONT_LEN];
char	scoreListFontName[FONT_LEN];
char	buttonFontName[FONT_LEN];
char	textFontName[FONT_LEN];
char	talkFontName[FONT_LEN];
char	motdFontName[FONT_LEN];

#ifdef DEVELOPMENT
static bool testxsync = false;
static bool testxdebug = false;
static bool testxafter = false;
static bool testxcolors = false;
#endif /* DEVELOPMENT */

#define DISPLAY_ENV	"DISPLAY"
#define DISPLAY_DEF	":0.0"
#define KEYBOARD_ENV	"KEYBOARD"

/*
 * Default fonts
 */
#define GAME_FONT	"-*-fixed-bold-*-*--13-*-*-*-c-*-iso8859-1"
/*"-*-times-*-*-*--18-*-*-*-*-*-iso8859-1"*/
#define MESSAGE_FONT	"-*-times-*-*-*--14-*-*-*-*-*-iso8859-1"
/* "-*-fixed-medium-r-*--13-*-*-*-c-*-iso8859-1" */
#define SCORE_LIST_FONT	"-*-fixed-bold-*-*--13-*-*-*-c-*-iso8859-1"
#define BUTTON_FONT	"-*-fixed-bold-*-*--13-*-*-*-c-*-iso8859-1"
/*"-*-*-bold-o-*--14-*-*-*-*-*-iso8859-1"*/
#define TEXT_FONT	"-*-fixed-bold-*-*--13-*-*-*-c-*-iso8859-1"
/*"-*-*-bold-i-*--14-*-*-*-p-*-iso8859-1"*/
#define TALK_FONT	"-*-fixed-bold-*-*--15-*-*-*-c-*-iso8859-1"
#define MOTD_FONT	"-*-fixed-bold-*-*--13-*-*-*-c-*-iso8859-1"
/*"-*-courier-bold-r-*--14-*-*-*-*-*-iso8859-1"*/

static char displayName[MAX_DISP_LEN];
static char keyboardName[MAX_DISP_LEN];

/* kps - this is quite useless currently */
static bool Set_geometry(xp_option_t *opt, const char *value)
{
    UNUSED_PARAM(opt);
    XFREE(geometry);
    geometry = xp_safe_strdup(value);
    return true;
}

static const char *Get_geometry(xp_option_t *opt)
{
    static char buf[20]; /* should be enough */
    UNUSED_PARAM(opt);
    snprintf(buf, sizeof(buf), "%dx%d", top_width, top_height);
    return buf;
}

static bool Set_texturedObjects(xp_option_t *opt, bool val);

static bool Set_fullColor(xp_option_t *opt, bool val)
{
    UNUSED_PARAM(opt);

    if (val == fullColor)
	return true;

    if (val) {
	/* see if we can use fullColor at all. */
	fullColor = true;
	if (Colors_init_bitmaps() == -1) {
	    /* no we can't have fullColor. */
	    warn("Unable to enable fullColor.");
	    fullColor = false;
	}
    } else {
	Colors_free_bitmaps();
	fullColor = false;
	Set_texturedObjects(NULL, false);
    }
    /* Make sure texture on score list is redrawn. */
    scoresChanged = true;
    return true;
}

static bool Set_texturedObjects(xp_option_t *opt, bool val)
{
    UNUSED_PARAM(opt);

    if (val == texturedObjects)
	return true;

    if (val) {
	/* Can't use texturedObjects without fullColor */
	texturedObjects = true;
	if (!fullColor) {
	    /* no we can't have texturedObjects. */
	    warn("Unable to enable texturedObjects without fullColor on.");
	    texturedObjects = false;
	}
    }
    else
	texturedObjects = false;

    return true;
}

static bool Set_mouseAccelNum(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    if (value < 0) {
	new_acc_num = 0;
	return false;
    } else {
	new_acc_num = value;
	if (dpy) {
#ifndef _WINDOWS
	    if (mouseAccelInClient) {
		XChangePointerControl(dpy, True, True, value,
				      new_acc_denom, new_threshold);
	    }
#endif
	}
	return true;
    }
  
}

static bool Set_mouseAccelDenom(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    if (value < 0) {
	new_acc_denom = 1;
	return false;
    } else {
	new_acc_denom = value;
	if (dpy) {
#ifndef _WINDOWS
	    if (mouseAccelInClient) {
		XChangePointerControl(dpy, True, True, new_acc_num,
				      value, new_threshold);
	    }
#endif
	}
	return true;
    }  
}     

static bool Set_mouseAccelThresh(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    if (value < 0) {
	new_threshold = 0;
	return false;
    } else {    
	new_threshold = value;
	if (dpy) {
#ifndef _WINDOWS
	    if (mouseAccelInClient) {
		XChangePointerControl(dpy, True, True, new_acc_num,
				      new_acc_denom, value);
	    }
#endif
	}
	return true;
    }
}

static bool Set_fontName(xp_option_t *opt, const char *val)
{
    char *buf = Option_get_private_data(opt);
    char *tmpval, *fontname;

    assert(val != NULL);

    /* remove whitespace from font specification */
    tmpval = xp_safe_strdup(val);
    
    fontname = strtok(tmpval, " \t\r\n");
    if (!fontname) {
	XFREE(tmpval);
	return false;
    }

    strlcpy(buf, fontname, FONT_LEN);
    XFREE(tmpval);

    return true;
}

xp_option_t xdefault_options[] = {
    XP_BOOL_OPTION(
	"fullColor",
	true,
	&fullColor,
	Set_fullColor,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Whether to use a colors as close as possible to the specified ones\n"
	"or use a few standard colors for everything. May require more\n"
	"resources from your system.\n"),

    XP_BOOL_OPTION(
	"texturedObjects",
	true,
	&texturedObjects,
	Set_texturedObjects,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Whether to draw certain game objects with textures.\n"
	"Be warned that this requires more graphics speed.\n"
	"fullColor must be on for this to work.\n"
	"You may also need to enable multibuffering or double-buffering.\n"),

    XP_STRING_OPTION(
	"geometry",
	"1024x768",
	NULL, 0,
	Set_geometry, NULL, Get_geometry,
	XP_OPTFLAG_DEFAULT,
	"Set the window size and position in standard X geometry format.\n"
	"The maximum allowed window size is 1922x1440.\n"),

    XP_STRING_OPTION(
	"display",
	"",
	displayName,
	sizeof displayName,
	NULL, NULL, NULL,
	XP_OPTFLAG_KEEP,
	"Set the X display.\n"),

    XP_STRING_OPTION(
	"keyboard",
	"",
	keyboardName,
	sizeof keyboardName,
	NULL, NULL, NULL,
	XP_OPTFLAG_KEEP,
	"Set the X keyboard input if you want keyboard input from\n"
	"another display.  The default is to use the keyboard input from\n"
	"the X display.\n"),

    XP_STRING_OPTION(
	"visual",
	"",
	visualName,
	sizeof visualName,
	NULL, NULL, NULL,
	XP_OPTFLAG_KEEP,
	"Specify which visual to use for allocating colors.\n"
	"To get a listing of all possible visuals on your dislay\n"
	"set the argument for this option to list.\n"),

    XP_BOOL_OPTION(
	"colorSwitch",
	true,
	&colorSwitch,
	NULL,
	XP_OPTFLAG_KEEP,
	"Use color buffering or not.\n"
	"Usually color buffering is faster, especially on 8-bit\n"
	"PseudoColor displays.\n"),

    XP_BOOL_OPTION(
	"multibuffer",
	false,
	&multibuffer,
	NULL,
	XP_OPTFLAG_DEFAULT,
	"Use the X windows multibuffer extension if present.\n"),

    XP_BOOL_OPTION(
	"ignoreWindowManager",
	false,
	&ignoreWindowManager,
	NULL,
	XP_OPTFLAG_DEFAULT,
	"Ignore the window manager when opening the top level player window.\n"
	"This can be handy if you want to have your XPilot window on a\n"
	"preferred position without window manager borders.\n"
	"Also sometimes window managers may interfere when switching\n"
	"colormaps. This option may prevent that.\n"),

    XP_BOOL_OPTION(
	"titleFlip",
	true,
	&titleFlip,
	NULL,
	XP_OPTFLAG_DEFAULT,
	"Should the title bar change or not.\n"
	"Some window managers like twm may have problems with\n"
	"flipping title bars.  Hence this option to turn it off.\n"),

    XP_STRING_OPTION(
	"gameFont",
	GAME_FONT,
	gameFontName,
	sizeof gameFontName,
	Set_fontName, gameFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used on the HUD and for most other text.\n"),

    XP_STRING_OPTION(
	"scoreListFont",
	SCORE_LIST_FONT,
	scoreListFontName,
	sizeof scoreListFontName,
	Set_fontName, scoreListFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used on the score list.\n"
	"This must be a non-proportional font.\n"),

    XP_STRING_OPTION(
	"buttonFont",
	BUTTON_FONT,
	buttonFontName,
	sizeof buttonFontName,
	Set_fontName, buttonFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used on all buttons.\n"),

    XP_STRING_OPTION(
	"textFont",
	TEXT_FONT,
	textFontName,
	sizeof textFontName,
	Set_fontName, textFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used in the help and about windows.\n"),

    XP_STRING_OPTION(
	"talkFont",
	TALK_FONT,
	talkFontName,
	sizeof talkFontName,
	Set_fontName, talkFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used in the talk window.\n"),

    XP_STRING_OPTION(
	"motdFont",
	MOTD_FONT,
	motdFontName,
	sizeof motdFontName,
	Set_fontName, motdFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used in the MOTD window and key list window.\n"
	"This must be a non-proportional font.\n"),

    XP_STRING_OPTION(
	"messageFont",
	MESSAGE_FONT,
	messageFontName,
	sizeof messageFontName,
	Set_fontName, messageFontName, NULL,
	XP_OPTFLAG_DEFAULT,
	"The font used for drawing messages.\n"),

    XP_BOOL_OPTION(
	"showNastyShots",
	false,
	&instruments.showNastyShots,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Use the Nasty Looking Shots instead of the round shots.\n"
	"You will probably want to increase your shotSize if you use this.\n"),

    XP_BOOL_OPTION(
        "mouseAccelInClient",
	true,
	&mouseAccelInClient,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set to true if you want to set the mouse turn rates to be linear\n"),

    XP_INT_OPTION(
        "mouseAccelNum",
	0,
	0,
	10,
	&new_acc_num,
	Set_mouseAccelNum,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Fine tune the mouse acceleration\n"),

    XP_INT_OPTION(
        "mouseAccelDenom",
	1,
	1,
	10,
	&new_acc_denom,
	Set_mouseAccelDenom,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set the mouse acceleration denominator\n"),

    XP_INT_OPTION(
        "mouseAccelThresh",
	0,
	0,
	10,
	&new_threshold,
	Set_mouseAccelThresh,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set the mouse acceleration threshold\n"),


};

void Store_X_options(void)
{
    STORE_OPTIONS(xdefault_options);
}


#ifdef DEVELOPMENT
static int X_error_handler(Display *display, XErrorEvent *xev)
{
    char		buf[1024];

    fflush(stdout);
    fprintf(stderr, "X error\n");
    XGetErrorText(display, xev->error_code, buf, sizeof buf);
    buf[sizeof(buf) - 1] = '\0';
    fprintf(stderr, "%s\n", buf);
    fflush(stderr);
    *(double *) -3 = 2.10;	/*core dump*/
    exit(1);
    return 0;
}

static void X_after(Display *display)
{
    static int		n;

    UNUSED_PARAM(display);
    if (n < 1000)
	printf("_X_ %4d\n", n++);
}
#endif /* DEVELOPMENT */

#ifndef _WINDOWS
void Handle_X_options(void)
{
    char *ptr;

    /* handle display */
    assert(displayName);
    if (strlen(displayName) == 0) {
	if ((ptr = getenv(DISPLAY_ENV)) != NULL)
	    Set_option("display", ptr, xp_option_origin_env);
	else
	    Set_option("display", DISPLAY_DEF, xp_option_origin_default);
    }

    if ((dpy = XOpenDisplay(displayName)) == NULL)
	fatal("Can't open display '%s'.", displayName);

    /* handle keyboard */
    assert(keyboardName);
    if (strlen(keyboardName) == 0) {
	if ((ptr = getenv(KEYBOARD_ENV)) != NULL)
	    Set_option("keyboard", ptr, xp_option_origin_env);
    }

    if (strlen(keyboardName) == 0)
	kdpy = NULL;
    else if ((kdpy = XOpenDisplay(keyboardName)) == NULL)
	fatal("Can't open keyboard '%s'.", keyboardName);

    /* handle visual */
    assert(visualName);
    if (strncasecmp(visualName, "list", 4) == 0) {
	List_visuals();
	exit(0);
    }
    
    /* handle mouse */

    /* We know that we have the display open here and this is as early */
    /* in the code as we can back up the existing mouse acceleration   */
    /* we are going to overide these with xpilot settings, and want to */
    /* restore these settings on exiting xpilot                        */
    pre_exists = True;
    XGetPointerControl(dpy, &pre_acc_num, &pre_acc_denom, &pre_threshold);


#ifdef DEVELOPMENT
    if (testxsync) {
	XSynchronize(dpy, True);
	XSetErrorHandler(X_error_handler);
    }

    if (testxdebug)
	XSetErrorHandler(X_error_handler);

    if (testxafter) {
	XSetAfterFunction(dpy, (int (*)(
#if NeedNestedPrototypes
	    Display *
#endif
	    )) X_after);
    }

    if (testxcolors)
	Colors_debug();
#endif

}
#endif /* _WINDOWS */


bool Set_scaleFactor(xp_option_t *opt, double val)
{
    UNUSED_PARAM(opt);
    clData.scaleFactor = val;
    clData.scale = 1.0 / val;
    clData.fscale = (float)clData.scale;
    /* Resize removed because it is not needed here */
    Scale_dashes();
    Config_redraw();
    Bitmap_update_scale();
    return true;
}

bool Set_altScaleFactor(xp_option_t *opt, double val)
{
    UNUSED_PARAM(opt);
    clData.altScaleFactor = val;
    return true;
}

xp_keysym_t String_to_xp_keysym(/*const*/ char *str)
{
    KeySym ks;
    xp_keysym_t xpks;

    assert(str);
    if ((ks = XStringToKeysym(str)) == NoSymbol)
	return XP_KS_UNKNOWN;
    else {
	xpks = (xp_keysym_t)ks;
	assert(xpks != XP_KS_UNKNOWN);
	return xpks;
    }
}
