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
#include "icon.h"

/*
 * Item structures.
 *
 * If you add an item here then please make sure you also add
 * the item in the proper place in ../replay/xp-replay.c.
 */
#ifdef _WINDOWS
#pragma warning(disable : 4305)
#endif
#include "../items/itemRocketPack.xbm"
#include "../items/itemCloakingDevice.xbm"
#include "../items/itemEnergyPack.xbm"
#include "../items/itemWideangleShot.xbm"
#include "../items/itemRearShot.xbm"
#include "../items/itemMinePack.xbm"
#include "../items/itemSensorPack.xbm"
#include "../items/itemTank.xbm"
#include "../items/itemEcm.xbm"
#include "../items/itemArmor.xbm"
#include "../items/itemAfterburner.xbm"
#include "../items/itemTransporter.xbm"
#include "../items/itemDeflector.xbm"
#include "../items/itemHyperJump.xbm"
#include "../items/itemPhasingDevice.xbm"
#include "../items/itemMirror.xbm"
#include "../items/itemLaser.xbm"
#include "../items/itemEmergencyThrust.xbm"
#include "../items/itemTractorBeam.xbm"
#include "../items/itemAutopilot.xbm"
#include "../items/itemEmergencyShield.xbm"

/* How far away objects should be placed from each other etc... */
#define BORDER			10
#define BTN_BORDER		4

/* Information window dimensions */
#define ABOUT_WINDOW_WIDTH	600
#define ABOUT_WINDOW_HEIGHT	700

/*
 * Globals.
 */
int			ButtonHeight;
Atom			ProtocolAtom, KillAtom;
bool			quitting = false;
unsigned		top_width, top_height;
unsigned		players_width, players_height;
bool			radar_score_mapped;
Cursor			pointerControlCursor;
bool			ignoreWindowManager;

XFontStruct* gameFont;		/* The fonts used in the game */
XFontStruct* messageFont;
XFontStruct* scoreListFont;
XFontStruct* buttonFont;
XFontStruct* textFont;
XFontStruct* talkFont;
XFontStruct* motdFont;

/*static char myName[] = "xpilot";*/
static char myClass[] = "XPilot";

/*
 * NB!  Is dependent on the order of the items in item.h!
 */
static struct {
    unsigned char*	data;
    const char*		keysText;
} itemBitmapData[NUM_ITEMS] = {
    {
	itemEnergyPack_bits,
	"Extra energy/fuel"
    },
    {
	itemWideangleShot_bits,
	"Extra front cannons"
    },
    {
	itemRearShot_bits,
	"Extra rear cannon"
    },
    {
	itemAfterburner_bits,
	"Afterburner; makes your engines more powerful"
    },
    {
	itemCloakingDevice_bits,
	"Cloaking device; "
	"makes you almost invisible, both on radar and on screen"
    },
    {
	itemSensorPack_bits,
	"Sensor; "
	"enables you to see cloaked opponents more easily"
    },
    {
	itemTransporter_bits,
	"Transporter; enables you to steal equipment from "
	"other players"
    },
    {
	itemTank_bits,
	"Tank; "
	"makes refueling quicker, increases maximum fuel "
	"capacity and can be jettisoned to confuse enemies"
    },
    {
	itemMinePack_bits,
	"Mine; "
	"can be dropped as a bomb or as a stationary mine"
    },
    {
	itemRocketPack_bits,
	"Rocket; can be utilized as smart missile, "
	"heatseeking missile, nuclear missile or just a "
	"plain unguided missile (torpedo)"
    },
    {
	itemEcm_bits,
	"ECM (Electronic Counter Measures); "
	"can be used to disturb electronic equipment, for instance "
	"can it be used to confuse smart missiles and reprogram "
	"robots to seek certain players"
    },
    {
	itemLaser_bits,
	"Laser; "
	"limited range laser beam, costs a lot of fuel, "
	"having more laser items increases the range of the laser, "
	"they can be irrepairably damaged by ECMs"
    },
    {
	itemEmergencyThrust_bits,
	"Emergency Thrust; "
	"gives emergency thrust capabilities for a limited period"
    },
    {   itemTractorBeam_bits,
	"Tractor Beam; "
	"gives mutual attractive force to currently locked on ship, "
	"this means the heavier your ship, the less likely you will move "
	"when being tractored or using a tractor"
    },
    {
	itemAutopilot_bits,
	"Autopilot; "
	"when on, the ship will turn and thrust against the "
	"direction of travel"
    },
    {
	itemEmergencyShield_bits,
	"EmergencyShield; "
	"gives emergency shield capabilities for a limited period"
    },
    {
	itemDeflector_bits,
	"Deflector; "
	"pushes hostile objects away from your ship"
    },
    {
	itemHyperJump_bits,
	"Hyperjump; "
	"enables you to teleport to a random map location"
    },
    {
	itemPhasingDevice_bits,
	"Phasing Device; "
	"lets you fly through anything for a limited period"
    },
    {
	itemMirror_bits,
	"Mirror; "
	"reflects laser beams"
    },
    {
	itemArmor_bits,
	"Armor; "
	"absorbs shots in the absence of shields"
    },
};
#ifdef _WINDOWS
Pixmap	itemBitmaps[NUM_ITEMS][2];	/* Bitmaps for the items in 2 colors */
#else
Pixmap	itemBitmaps[NUM_ITEMS];		/* Bitmaps for the items */
#endif

char dashes[NUM_DASHES];
char cdashes[NUM_CDASHES];

static int Quit_callback(int, void *, const char **);
static int Config_callback(int, void *, const char **);
static int Colors_callback(int, void *, const char **);
static int Score_callback(int, void *, const char **);
static int Player_callback(int, void *, const char **);

int button_form;
static int menu_button;

const char *Item_get_text(int i)
{
    return itemBitmapData[i].keysText;
}

/*
 * Set specified font for that GC.
 * Return font that is used for this GC, even if setting a new
 * font failed (return default font in that case).
 */
static XFontStruct* Set_font(Display* display, GC gc,
			     const char* fontName,
			     const char *resName)
{
    XFontStruct*	font;

#ifndef _WINDOWS
    if ((font = XLoadQueryFont(display, fontName)) == NULL) {
	error("Couldn't find font '%s' for %s, using default font",
	      fontName, resName);
	font = XQueryFont(display, XGContextFromGC(gc));
    } else
	XSetFont(display, gc, font->fid);
#else
    font = WinXLoadFont(fontName);
    XSetFont(display, gc, font->fid);
#endif

    return font;
}

/*
 * Initialize miscellaneous window hints and properties.
 */
#ifndef _WINDOWS
extern char		**Argv;
extern int		Argc;

static void Init_disp_prop(Display *d, Window win,
			   unsigned w, unsigned h, int x, int y,
			   int flags)
{
    XClassHint		xclh;
    XWMHints		xwmh;
    XSizeHints		xsh;
    char		msg[256];

    xwmh.flags	   = InputHint|StateHint|IconPixmapHint;
    xwmh.input	   = True;
    xwmh.initial_state = NormalState;
    xwmh.icon_pixmap   = XCreateBitmapFromData(d, win,
					       (char *)icon_bits,
					       icon_width, icon_height);

    xsh.flags = (flags|PMinSize|PMaxSize|PBaseSize|PResizeInc);
    xsh.width = w;
    xsh.base_width =
    xsh.min_width = MIN_TOP_WIDTH;
    xsh.max_width = MAX_TOP_WIDTH;
    xsh.width_inc = 1;
    xsh.height = h;
    xsh.base_height =
    xsh.min_height = MIN_TOP_HEIGHT;
    xsh.max_height = MAX_TOP_HEIGHT;
    xsh.height_inc = 1;
    xsh.x = x;
    xsh.y = y;

    xclh.res_name = NULL;	/* NULL: Automatically uses Argv[0], */
    xclh.res_class = myClass; /* stripped of directory prefixes. */

    /*
     * Set the above properties.
     */
    XSetWMProperties(d, win, NULL, NULL, Argv, Argc,
		     &xsh, &xwmh, &xclh);

    /*
     * Now initialize icon and window title name.
     */
    if (titleFlip)
	sprintf(msg, "Successful connection to server at \"%s\".",
		servername);
    else
	sprintf(msg, "%s -- Server at \"%s\".", TITLE, servername);
    XStoreName(d, win, msg);

    sprintf(msg, "%s:%s", connectParam.nick_name, servername);
    XSetIconName(d, win, msg);

    if (d != dpy)
	return;

    /*
     * Specify IO error handler and the WM_DELETE_WINDOW atom in
     * an attempt to catch 'nasty' quits.
     */
    ProtocolAtom = XInternAtom(d, "WM_PROTOCOLS", False);
    KillAtom = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, win, &KillAtom, 1);
    XSetIOErrorHandler(FatalError);
}
#endif


/*
 * The following function initializes a toplevel window.
 * It returns 0 if the initialization was successful,
 * or -1 if it couldn't initialize the double buffering routine.
 */
int Init_top(void)
{
    int					top_x, top_y;
#ifndef _WINDOWS
    int					i;
    int					x, y;
    unsigned				w, h;
    unsigned long			values;
    int					top_flags;
    XGCValues				xgc;
    XSetWindowAttributes		sattr;
    unsigned long			mask;

    if (topWindow)
	fatal("Init_top called twice");

    if (Colors_init() == -1)
	return -1;
#endif

    radarDrawRectanglePtr = XFillRectangle;

    /*
     * Get toplevel geometry.
     */
#ifndef _WINDOWS
    top_flags = 0;
    if (geometry != NULL && geometry[0] != '\0')
	mask = XParseGeometry(geometry, &x, &y, &w, &h);
    else
	mask = 0;

    if ((mask & WidthValue) != 0) {
	top_width = w;
	top_flags |= USSize;
    } else {
	top_width = DEF_TOP_WIDTH;
	top_flags |= PSize;
    }
    LIMIT(top_width, MIN_TOP_WIDTH, MAX_TOP_WIDTH);
    if ((mask & HeightValue) != 0) {
	top_height = h;
	top_flags |= USSize;
    } else {
	top_height = DEF_TOP_HEIGHT;
	top_flags |= PSize;
    }
    LIMIT(top_height, MIN_TOP_HEIGHT, MAX_TOP_HEIGHT);
    if ((mask & XValue) != 0) {
	if ((mask & XNegative) != 0)
	    top_x = DisplayWidth(dpy, DefaultScreen(dpy)) - top_width + x;
	else
	    top_x = x;
	top_flags |= USPosition;
    } else {
	top_x = (DisplayWidth(dpy, DefaultScreen(dpy)) - top_width) /2;
	top_flags |= PPosition;
    }
    if ((mask & YValue) != 0) {
	if ((mask & YNegative) != 0)
	    top_y = DisplayHeight(dpy, DefaultScreen(dpy)) - top_height + y;
	else
	    top_y = y;
	top_flags |= USPosition;
    } else {
	top_y = (DisplayHeight(dpy, DefaultScreen(dpy)) - top_height) /2;
	top_flags |= PPosition;
    }
    if (geometry != NULL) {
	free(geometry);
	geometry = NULL;
    }

    /*
     * Create toplevel window (we need this first so that we can create GCs)
     */
    mask = 0;
    /*old debug: sattr.background_pixel = colors[WHITE].pixel;*/
    sattr.background_pixel = colors[BLACK].pixel;
    mask |= CWBackPixel;
    sattr.border_pixel = colors[WHITE].pixel;
    mask |= CWBorderPixel;
    if (colormap != 0) {
	sattr.colormap = colormap;
	mask |= CWColormap;
    }
    if (ignoreWindowManager) {
	sattr.override_redirect = True;
	mask |= CWOverrideRedirect;
    }
    topWindow = XCreateWindow(dpy,
			      DefaultRootWindow(dpy),
			      top_x, top_y,
			      top_width, top_height,
			      0, (int)dispDepth,
			      InputOutput, visual,
			      mask, &sattr);
    XSelectInput(dpy, topWindow,
		 KeyPressMask | KeyReleaseMask
		 | FocusChangeMask | StructureNotifyMask);
    Init_disp_prop(dpy, topWindow, top_width, top_height,
		   top_x, top_y, top_flags);
    if (kdpy) {
	int scr = DefaultScreen(kdpy);
	keyboardWindow = XCreateSimpleWindow(kdpy,
					     DefaultRootWindow(kdpy),
					     top_x, top_y,
					     top_width, top_height,
					     0, 0, BlackPixel(dpy, scr));
	XSelectInput(kdpy, keyboardWindow,
		     KeyPressMask | KeyReleaseMask | FocusChangeMask);
	Init_disp_prop(kdpy, keyboardWindow, top_width, top_height,
		       top_x, top_y, top_flags);
    }
#else	/* _WINDOWS */
	/* Bucko seems to use 0 as the topWindow index */
	topWindow = 0;
	top_x = top_y = 0;
	WinXParseGeometry(geometry, &top_width, &top_height);
#endif	/* _WINDOWS */

    return 0;
}


/*
 * Creates the playing windows.
 * Returns 0 on success, -1 on error.
 */
int Init_playing_windows(void)
{
#ifndef _WINDOWS
    unsigned			w, h;
    Pixmap			pix;
    GC				cursorGC;
#else
    int				i;
#endif

    if (!topWindow) {
	if (Init_top())
	    return -1;
    }

    Scale_dashes();

    draw_width = top_width - (256 + 2);
    draw_height = top_height;
#ifdef  _WINDOWS
    /*
     * What follows is poor code.  WinX needs to know beforehand if its
     * dealing with draw because it might want to create 2 bitmaps for it.
     * Since i know draw is the first window created (after top),
     * i can cheat it.
     */
    drawWindow = 1;
#endif
    drawWindow = XCreateSimpleWindow(dpy, topWindow, 258, 0,
				     draw_width, draw_height,
				     0, 0, colors[BLACK].pixel);
    IFWINDOWS( if (drawWindow != 1) error("draw != 1") );
    radarWindow = XCreateSimpleWindow(dpy, topWindow, 0, 0,
				      256, RadarHeight, 0, 0,
				      colors[BLACK].pixel);
    radar_score_mapped = true;

#ifdef _WINDOWS
    WinXSetEventMask(drawWindow, NoEventMask);
    radar_exposures = 1;
    radarGC = WinXCreateWinDC(radarWindow);
    gameGC = WinXCreateWinDC(drawWindow);

    textWindow = XCreateSimpleWindow(dpy, topWindow, 0, 0,
				     0, 0, 0, 0,
				     colors[BLACK].pixel);
    textGC = WinXCreateWinDC(textWindow);

    msgWindow = XCreateSimpleWindow(dpy, topWindow, 0, 0,
				    0, 0, 0, 0,
				    colors[BLACK].pixel);
    messageGC = WinXCreateWinDC(msgWindow);
    motdGC = WinXCreateWinDC(topWindow);

    for (i = 0; i < MAX_COLORS; i++)
	colors[i].pixel = i;

    players_exposed = 1;
    /* radarPixmap = XCreatePixmap(dpy, radar, 256, RadarHeight, dispDepth); */
    radarPixmap2 = XCreatePixmap(dpy, radarWindow, 256, RadarHeight, dispDepth);
    /*
     * Create item bitmaps AFTER the windows
     */
    WinXCreateItemBitmaps();
    /* create the fonts AFTER the windows */
    gameFont
	= Set_font(dpy, gameGC, gameFontName, "gameFont");
    messageFont
	= Set_font(dpy, messageGC, messageFontName, "messageFont");
    textFont
	= Set_font(dpy, textGC, textFontName, "textFont");
    motdFont
	= Set_font(dpy, motdGC, motdFontName, "motdFont");

    buttonWindow = XCreateSimpleWindow(dpy, topWindow, 0, 0,
				       0, 0, 0, 0,
				       colors[BLACK].pixel);
    buttonGC = WinXCreateWinDC(buttonWindow);
    buttonFont
	= Set_font(dpy, buttonGC, buttonFontName, "buttonFont");
#endif

    /* Create buttons */
#define BUTTON_WIDTH	84
    ButtonHeight = buttonFont->ascent + buttonFont->descent + 2*BTN_BORDER;

    button_form
	= Widget_create_form(0, topWindow,
			     0, (int)RadarHeight,
			     256, ButtonHeight + 2,
			     0);
    Widget_create_activate(button_form,
			   0 + 0*BUTTON_WIDTH, 0,
			   BUTTON_WIDTH, ButtonHeight,
			   1, "QUIT",
			   Quit_callback, NULL);
    Widget_create_activate(button_form,
			   1 + 1*BUTTON_WIDTH, 0,
			   BUTTON_WIDTH, ButtonHeight,
			   1, "ABOUT",
			   About_callback, NULL);
    menu_button
	= Widget_create_menu(button_form,
			     2 + 2*BUTTON_WIDTH, 0,
			     BUTTON_WIDTH, ButtonHeight,
			     1, "MENU");
    Widget_add_pulldown_entry(menu_button,
			      "KEYS", Keys_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
			      "CONFIG", Config_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
			      "COLORS", Colors_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
			      "SCORE", Score_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
			      "PLAYER", Player_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
			      "MOTD", Motd_callback, NULL);
#ifdef _WINDOWS
    Widget_add_pulldown_entry(menu_button,
			      "CREDITS", Credits_callback, NULL);
#endif
    Widget_map_sub(button_form);

    /* Create score list window */
    players_width = RadarWidth;
    players_height = top_height - (RadarHeight + ButtonHeight + 2);
    playersWindow
	= XCreateSimpleWindow(dpy, topWindow,
			      0, (int)RadarHeight + ButtonHeight + 2,
			      players_width, players_height,
			      0, 0,
			      colors[windowColor].pixel);
#ifdef _WINDOWS
    scoreListGC = WinXCreateWinDC(playersWindow);
    scoreListFont
	= Set_font(dpy, scoreListGC, scoreListFontName, "scoreListFont");
#endif

    /*
     * Selecting the events we can handle.
     */
    XSelectInput(dpy, radarWindow, ExposureMask);
    XSelectInput(dpy, playersWindow, ExposureMask);

    /* WinXSetEvent(players, WM_PAINT, WinXPaintPlayers); */
    pointerControlCursor = !None;

    Init_spark_colors();

    return 0;
}

#ifdef _WINDOWS
void WinXCreateItemBitmaps(void)
{
    int			i;
	extern int hudColor;

    for (i = 0; i < NUM_ITEMS; i++) {
	itemBitmaps[i][ITEM_HUD]
	    = WinXCreateBitmapFromData(dpy, drawWindow,
				       (char *)itemBitmapData[i].data,
				       ITEM_SIZE, ITEM_SIZE,
				       colors[hudColor].pixel);
	itemBitmaps[i][ITEM_PLAYFIELD]
	    = WinXCreateBitmapFromData(dpy, drawWindow,
				       (char *)itemBitmapData[i].data,
				       ITEM_SIZE, ITEM_SIZE,
				       colors[RED].pixel);
    }
    Colors_init_bitmaps();
    
}
#endif

static int Config_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    Config(true, CONFIG_DEFAULT);
    return 0;
}

static int Colors_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    Config(true, CONFIG_COLORS);
    return 0;
}

static int Score_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    Config(false, CONFIG_NONE);
    if (showUserName) {
	showUserName = false;
	scoresChanged = true;
    }
    return 0;
}

static int Player_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    Config(false, CONFIG_NONE);
    if (!showUserName) {
	showUserName = true;
	scoresChanged = true;
    }
    return 0;
}

static int Quit_callback(int widget_desc, void *data, const char **str)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(str);
    quitting = true;
    return 0;
}

void Raise_window(void)
{
    XMapRaised(dpy, topWindow);
}

void Resize(Window w, unsigned width, unsigned height)
{
    if (w != topWindow)
	return;

    /* ignore illegal resizes */
    LIMIT(width, MIN_TOP_WIDTH, MAX_TOP_WIDTH);
    LIMIT(height, MIN_TOP_HEIGHT, MAX_TOP_HEIGHT);
    top_width = width;
    top_height = height;
    if (!drawWindow)
	return;

    if (radar_score_mapped)
	draw_width = top_width - 258;
    else
	draw_width = top_width;
    draw_height = top_height;

    Check_view_dimensions();
    Net_flush();
    XResizeWindow(dpy, drawWindow, draw_width, draw_height);
#ifndef _WINDOWS
    if (dbuf_state->type == PIXMAP_COPY) {
	XFreePixmap(dpy, drawPixmap);
	drawPixmap = XCreatePixmap(dpy, drawWindow, draw_width, draw_height,
				   dispDepth);
    }
#endif
    players_height = top_height - (RadarHeight + ButtonHeight + 2);
    XResizeWindow(dpy, playersWindow,
		  players_width, players_height);
#ifdef _WINDOWS
    WinXResize();
#endif
    Talk_resize();
    Config_resize();
}


/*
 * Cleanup player structure, close the display etc.
 */
void Platform_specific_cleanup(void)
{
    if (button_form) {
	Widget_destroy(button_form);
	button_form = 0;
    }
    Widget_cleanup();
}


int FatalError(Display *display)
{
    UNUSED_PARAM(display);
    Net_cleanup();
    /*
     * It's already a fatal I/O error, nothing to cleanup.
     */
    exit(0);
    return(0);
}

void Scale_dashes(void)
{
    if (dpy == NULL)
	return;

    dashes[0] = WINSCALE(8);
    if (dashes[0] < 1)
	dashes[0] = 1;
    dashes[1] = WINSCALE(4);
    if (dashes[1] < 1)
	dashes[1] = 1;
 
    cdashes[0] = WINSCALE(3);
    if (cdashes[0] < 1)
	cdashes[0] = 1;
    cdashes[1] = WINSCALE(9);
    if (cdashes[1] < 1)
	cdashes[1] = 1;

    XSetDashes(dpy, gameGC, 0, dashes, NUM_DASHES);
}
