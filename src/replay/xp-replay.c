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
/*
 * The scaling and gamma correction subroutines are adaptations from pbmplus:
 *
 * Copyright (C) 1989, 1991 by Jef Poskanzer.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */

#include "xp-replay.h"

#include "items/itemRocketPack.xbm"
#include "items/itemCloakingDevice.xbm"
#include "items/itemEnergyPack.xbm"
#include "items/itemWideangleShot.xbm"
#include "items/itemRearShot.xbm"
#include "items/itemMinePack.xbm"
#include "items/itemSensorPack.xbm"
#include "items/itemTank.xbm"
#include "items/itemEcm.xbm"
#include "items/itemAfterburner.xbm"
#include "items/itemTransporter.xbm"
#include "items/itemDeflector.xbm"
#include "items/itemHyperJump.xbm"
#include "items/itemPhasingDevice.xbm"
#include "items/itemLaser.xbm"
#include "items/itemMirror.xbm"
#include "items/itemArmor.xbm"
#include "items/itemEmergencyThrust.xbm"
#include "items/itemTractorBeam.xbm"
#include "items/itemAutopilot.xbm"
#include "items/itemEmergencyShield.xbm"

#include "tools/eject.xbm"
#include "tools/fastf.xbm"
#include "tools/pause.xbm"
#include "tools/play.xbm"
#include "tools/record.xbm"
#include "tools/revplay.xbm"
#include "tools/rewind.xbm"
#include "tools/stop.xbm"

struct rGC {
    struct rGC		*next;		/* linked list */
    unsigned long	mask;		/* XGCValues mask */
    unsigned long	foreground;
    unsigned long	background;
    unsigned char	line_width;
    unsigned char	line_style;
    unsigned char	dash_offset;
    unsigned char	function;
    unsigned char	fill_style;
    unsigned char	num_dashes;
    char		*dash_list;
    int			ts_x_origin;
    int			ts_y_origin;
    Pixmap		tile;
};

struct rArc {
    short		x;
    short		y;
    unsigned short	width;
    unsigned short	height;
    short		angle1;
    short		angle2;
};

struct rLines {
    XPoint		*points;
    unsigned short	npoints;
    short		mode;
};

struct rLine {
    short		x1;
    short		y1;
    short		x2;
    short		y2;
};

struct rString {
    short		x;
    short		y;
    unsigned char	font;
    size_t		length;
    char		*string;
};

struct rPolygon {
    XPoint		*points;
    unsigned short	npoints;
    unsigned char	shape;
    unsigned char	mode;
};

struct rSymbol {
    unsigned char	type;
    short		x;
    short		y;
};

struct rRectangle {
    short		x;
    short		y;
    unsigned short	width;
    unsigned short	height;
};

struct rRectangles {
    unsigned short	nrectangles;
    XRectangle		*rectangles;
};

struct rArcs {
    unsigned short	narcs;
    XArc		*arcs;
};

struct rSegments {
    unsigned short	nsegments;
    XSegment		*segments;
};

struct rDamage {
    unsigned char	damaged;
};

union shapep {
    struct rArc		arc;
    struct rLines	lines;
    struct rLine	line;
    struct rString	string;
    struct rPolygon	polygon;
    struct rSymbol	symbol;
    struct rRectangle	rectangle;
    struct rRectangles	rectangles;
    struct rArcs	arcs;
    struct rSegments	segments;
    struct rDamage	damage;
};

/*
 * Shape holds all info for one X windows drawing call.
 */
struct shape {
    struct shape	*next;		/* to next shape on frame list */
    char		type;		/* which drawing call */
    struct rGC		*gc;		/* Graphics Context */
    union shapep	shape;		/* actual shape data */
};

struct frame {
    struct frame	*next;		/* to next on frame list */
    struct frame	*prev;		/* to previous on frame list */
    struct frame	*older;		/* to next on LRU list */
    struct frame	*newer;		/* to previous on LRU list */
    long		filepos;	/* position in record file */
    unsigned		width;		/* width of view window */
    unsigned		height;		/* height of view window */
    struct shape	*shapes;	/* head of shape list */
    int			number;		/* frame sequence number */
};

typedef struct tile_list {
    struct tile_list	*next;
    Pixmap		tile;
    unsigned char	tile_id;
    int			flag;
} tile_list_t;

struct errorwin {
    Window		win;		/* Error display window */
    GC			gc;		/* GC for drawing therein */
    XFontStruct		*font;		/* Font to use */
    char		message[4096];	/* Current error message */
    int			len;		/* length thereof */
    Button		but;		/* OK button */
    Bool		grabbed;	/* Have we grabbed the pointer ? */
};

struct recordwin {
    Window		win;		/* Error display window */
    GC			gc;		/* GC for drawing therein */
    Button		mark_start_but;	/* Mark start button */
    Button		mark_end_but;	/* Mark end button */
    Button		recbut;		/* Record button */
    struct errorwin	*ewin;		/* Pointer to error window */
};

struct xprc {
    char		*filename;	/* name of input */
    FILE		*fp;		/* FILE pointer for input */
    int			seekable;	/* only seek if file is regular */
    int			eof;		/* if EOF encountered */
    int			majorversion;	/* major version of protocol */
    int			minorversion;	/* minor version of protocol */
    char		*nickname;	/* XPilot nick name of player */
    char		*realname;	/* login name of player */
    char		*hostname;	/* hostname of player */
    char		*servername;	/* hostname of server */
    int			fps;		/* frames per second of game */
    char		*recorddate;	/* date of game played */
    unsigned char	maxColors;	/* number of colors used */
    XColor		*colors;	/* pointer to color info */
    unsigned long	*pixels;	/* pointer to my pixel values */
    char		*gameFontName;	/* name of font used */
    char		*msgFontName;	/* name of font used */
    struct frame	*head;		/* to first frame */
    struct frame	*tail;		/* to last frame read sofar */
    struct frame	*cur;		/* current frame drawn */
    struct frame	*newest;	/* to first frame in LRU list */
    struct frame	*oldest;	/* to last frame in LRU list */
    struct frame	*save_first;	/* first frame to include in saving */
    struct frame	*save_last;	/* last frame to include in saving */
    XFontStruct		*gameFont;	/* X font for game situations */
    XFontStruct		*msgFont;	/* X font for messages */
    unsigned short	view_width;	/* initial width of viewing area */
    unsigned short	view_height;	/* initial height of viewing area */
    Window		topview;	/* Window to display frames in */
    GC			gc;		/* GC to use for frame display */
    double		scale;		/* scale reduction when saving */
    double		gamma;		/* gamma correction when saving */
    struct errorwin	*ewin;		/* Error display window */
    tile_list_t		*tlist;		/* list of pixmaps */
    int     		linewidth;	/* linewidth */
};

enum LabelDataTypes {
    LABEL_INTEGER,
    LABEL_STRING,
    LABEL_TIME
};

enum LabelTypes {
    LABEL_POSITION,
    LABEL_FPS,
    LABEL_SERVER,
    LABEL_PLAYER,
    LABEL_LOGIN,
    LABEL_CLIENT,
    NUM_LABELS
};

struct label {
    int			x, y;		/* position */
    int			w;		/* width (used to line up values) */
    Bool		j;		/* True for right justification */
    const char		*name;		/* Field name */
    enum LabelDataTypes	type;		/* data type */
    union {
	int		*i;
	char		*s;
	struct frame	**f;
    } data;				/* pointer to data */
};

enum ButtonTypes {
    BUTTON_RECORD,
    BUTTON_REWIND,
    BUTTON_REVERSE_PLAY,
    BUTTON_PLAY,
    BUTTON_FAST_FORWARD,
    BUTTON_PAUSE,
    BUTTON_STOP,
    BUTTON_EJECT,
    NUM_BUTTONS
};

#define BUTTON_BORDER 4
#define BUTTON_SPACING 2

static void quitCallback(void *);
static void pauseCallback(void *);
static void stopCallback(void *);
static void rewindCallback(void *);
static void fastfCallback(void *);
static void playCallback(void *);
static void revplayCallback(void *);
static void recordCallback(void *);
static void markSaveStart(void *);
static void markSaveEnd(void *);
static void saveStartToEndPPM(void *);
static void saveStartToEndXPR(void *);

static struct button_init {
    unsigned char *data;
    char color;
    unsigned width;
    unsigned height;
    void (*callback)(void *);
    int flags;
    int group;
} buttonInit[] = {
    {
	record_bits, 1, record_width, record_height,
	recordCallback,
	BUTTON_RELEASE, 0
    },
    {
	rewind_bits, 0, rewind_width, rewind_height,
	rewindCallback,
	0, 1
    },
    {
	revplay_bits, 0, revplay_width, revplay_height,
	revplayCallback,
	0, 1
    },
    {
	play_bits, 0, play_width, play_height,
	playCallback,
	0, 1
    },
    {
	fastf_bits, 0, fastf_width, fastf_height,
	fastfCallback,
	0, 1
    },
    {
	pause_bits, 0, pause_width, pause_height,
	pauseCallback,
	0, 1
    },
    {
	stop_bits, 0, stop_width, stop_height,
	stopCallback,
	BUTTON_PRESSED, 1
    },
    {
	eject_bits, 0, eject_width, eject_height,
	quitCallback,
	BUTTON_RELEASE, 0
    },
};

#define NUM_BUTTONS	(sizeof(buttonInit) / sizeof(struct button_init))

static enum playStates {
    STATE_PLAYING,
    STATE_PAUSED
} playState = STATE_PLAYING;

static int	currentSpeed = 0, frameStep = 0;

struct xui {
    Window		topmain;
    GC			gc;
    unsigned long	black;
    unsigned long	white;
    unsigned long	mainbg;
    unsigned long	red;
    XFontStruct		*smallFont;
    XFontStruct		*boldFont;
    Button		buttons[NUM_BUTTONS];
    struct label	*labels;
    struct errorwin	*ewin;
    struct recordwin	*rwin;
};

static char small_font[] = "-*-helvetica-medium-r-*--14-*-*-*-*-*-iso8859-1";
static char small_bold_font[] =
	"-*-helvetica-bold-r-*--14-*-*-*-*-*-iso8859-1";
static char alternative_font[] = "fixed";

static int		Argc;
static char		**Argv;

static int		debug = 0;	/* want debugging output */
static int		verbose = 0;	/* want extra info messages */
static int		compress = 0;	/* save files in compressed format */
static int		frame_count;	/* number of frame next read in */
static int		frames_in_core;	/* number of frame next read in */
#ifdef USE_GCLIST
static struct rGC	*gclist;	/* list of all GCs used */
#endif
static int		forceRedraw = False;
static int		quit = 0;
static struct xprc	*purge_argument;
static void 		Purge(struct xprc *rc);

static Display		*dpy;
static Colormap		colormap;
static int		screen_num;

static char	*mem_lowest;			/* lowest pointer seen */
static char	*mem_highest;			/* highest pointer seen */
static long	mem_program_used;		/* max. size of malloced mem */
static long	mem_all_types_used;		/* frame memory in use */
static long	mem_typed_used[NUM_MEMTYPES];	/* debugging & analysis */
static long	max_mem = 8 * 1024 * 1024;	/* memory limit (soft) */
static int	loopAtEnd;

static void openErrorWindow(struct errorwin *, const char *, ...);

/*
 * Print memory statistics when debugging.
 */
static void MemPrint(void)
{
    if (!debug)
	return;
    printf("after %d frames (%d in core):\n", frame_count, frames_in_core);
    printf("	string:          %10ld\n", mem_typed_used[MEM_STRING]);
    printf("	frame:           %10ld\n", mem_typed_used[MEM_FRAME]);
    printf("	shape:           %10ld\n", mem_typed_used[MEM_SHAPE]);
    printf("	point:           %10ld\n", mem_typed_used[MEM_POINT]);
    printf("	gc:              %10ld\n", mem_typed_used[MEM_GC]);
    printf("	misc:            %10ld\n", mem_typed_used[MEM_MISC]);
    printf("	user-interface:  %10ld\n", mem_typed_used[MEM_UI]);
    printf("	all types:       %10ld\n", mem_all_types_used);
    printf("	total program:   %10ld\n", mem_program_used);
}

/*
 * Maintain statistics on memory usage.
 * We want to know what part of the program uses what amount of memory.
 * This information is used to keep amount of memory used below a
 * certain maximum by limiting the number of frames which are kept
 * in memory.
 */
static void MemStats(char *p, size_t size, enum MemTypes mt)
{
    long		prev_mem_program_used = mem_program_used;

    if (!mem_lowest || p < mem_lowest)
	mem_lowest = p;
    if (!mem_highest || p + size > mem_highest)
	mem_highest = p + size;
    mem_typed_used[mt] += size;
    mem_all_types_used += size;
    mem_program_used = mem_highest - mem_lowest;
    if (mem_program_used >> 20 != prev_mem_program_used >> 20)
	MemPrint();
}

/*
 * Wrapper for free(3).
 * This one helps in maintaining statistics on memory usage.
 */
static void MyFree(void *p, size_t size, enum MemTypes mt)
{
    if (p) {
	free(p);
	mem_typed_used[mt] -= size;
	mem_all_types_used -= size;
    }
}

/*
 * Wrapper for malloc(3).
 * This one helps in maintaining statistics on memory usage.
 */
void *MyMalloc(size_t size, enum MemTypes mt)
{
    void *p;

    if (!(p = malloc(size))) {
	if (!size) {
	    /*
	     * caller shouldn't actually be using zero-sized memory anyway.
	     */
	    return NULL;
	}
	if (debug) {
	    printf("no memory\n");
	    MemPrint();
	}
	max_mem = mem_typed_used[MEM_FRAME] / 2;
	if (purge_argument != NULL)
	    Purge(purge_argument);

	if (!(p = malloc(size))) {
	    fprintf(stderr, "Not enough memory after allocating %ld bytes.\n",
		    mem_program_used);
	    exit(1);
	}
    }
    MemStats((char *)p, size, mt);
    return p;
}

/*
 * Read one 8-bit byte from the recorded input stream.
 */
static inline unsigned char RReadByte(FILE *fp)
{
    return (unsigned char) (getc(fp));
}

/*
 * Read one 16-bit unsigned word from the recorded input stream.
 */
static inline unsigned short RReadUShort(FILE *fp)
{
    unsigned short i;

    i = (getc(fp) & 0xFF);
    i |= (getc(fp) & 0xFF) << 8;

    return i;
}

/*
 * Read one 16-bit signed word from the recorded input stream.
 */
static inline short RReadShort(FILE *fp)
{
    short i;

    i = (short) RReadUShort(fp);

    if (i & 0x8000)
	i = -(-i & 0xffff);

    return i;
}

/*
 * Read one 32-bit unsigned longword from the recorded input stream.
 */
static inline unsigned long RReadULong(FILE *fp)
{
    unsigned long	i;

    i = (getc(fp) & 0xFF);
    i |= (getc(fp) & 0xFF) << 8;
    i |= (getc(fp) & 0xFF) << 16;
    i |= (getc(fp) & 0xFF) << 24;

    return i;
}

/*
 * Read one 32-bit signed longword from the recorded input stream.
 */
static inline long RReadLong(FILE *fp)
{
    long i;

    i = (long) RReadULong(fp);

    if (i & 0x80000000)
	i = -(-i & 0xffffffff);

    return i;
}

/*
 * Read a pascal-type string from the recorded input stream
 * and convert it to a nul-byte terminated C-string.
 */
static inline char *RReadString(FILE *fp)
{
    char		*s;
    int			i;
    size_t		len;

    len = RReadUShort(fp);
    s = (char *)MyMalloc(len + 1, MEM_STRING);
    s[len] = '\0';
    for (i = 0; i < (int)len; i++)
	s[i] = getc(fp);
    return s;
}

/*
 * Read the XPilot Record protocol header from the recorded input stream.
 */
static int RReadHeader(struct xprc *rc)
{
    char		magic[5];
    unsigned char	minor, major, fps;
    char		dot, nl;
    int			i;

    magic[0] = getc(rc->fp);
    magic[1] = getc(rc->fp);
    magic[2] = getc(rc->fp);
    magic[3] = getc(rc->fp);
    magic[4] = '\0';
    major = getc(rc->fp);
    dot = getc(rc->fp);
    minor = getc(rc->fp);
    nl = getc(rc->fp);
    if (strcmp(magic, "XPRC") || dot != '.' || nl != '\n') {
	fprintf(stderr, "Error: Not a valid XPilot Recording file.\n");
	return -1;
    }
    /*
     * We are incompatible with newer versions
     * and with older versions whose major version number
     * differs from ours.
     */
    if (major != RC_MAJORVERSION || minor > RC_MINORVERSION) {
	fprintf(stderr,
		"Error: Incompatible version. (file: %c.%c)(program: %c.%c)\n",
		major, minor, RC_MAJORVERSION, RC_MINORVERSION);
	return -1;
    }
    rc->majorversion = major;
    rc->minorversion = minor;
    rc->nickname = RReadString(rc->fp);
    rc->realname = RReadString(rc->fp);
    rc->hostname = RReadString(rc->fp);
    rc->servername = RReadString(rc->fp);
    fps = RReadByte(rc->fp);
    if (rc->fps == 0)
	rc->fps = fps;
    rc->recorddate = RReadString(rc->fp);
    rc->maxColors = (unsigned char) getc(rc->fp);
    rc->colors = (XColor *)MyMalloc(rc->maxColors * sizeof(XColor), MEM_MISC);
    for (i = 0; i < rc->maxColors; i++) {
	rc->colors[i].pixel = RReadULong(rc->fp);
	rc->colors[i].red = RReadUShort(rc->fp);
	rc->colors[i].green = RReadUShort(rc->fp);
	rc->colors[i].blue = RReadUShort(rc->fp);
	rc->colors[i].flags = DoRed | DoGreen | DoBlue;
    }
    rc->gameFontName = RReadString(rc->fp);
    rc->msgFontName = RReadString(rc->fp);
    rc->view_width = RReadUShort(rc->fp);
    rc->view_height = RReadUShort(rc->fp);

    if (verbose) {
	printf("Player is %s (in real life %s@%s).\n",
	       rc->nickname, rc->realname, rc->hostname);
	printf("Server was %s, running at %d frames per second.\n",
	       rc->servername, rc->fps);
	printf("Recorded on %s.\n", rc->recorddate);
    }

    return 0;
}

/*
 * Read an encoded Pixmap from the recorded input stream.
 */
static Pixmap RReadTile(struct xprc *rc)
{
    tile_list_t			*lptr;
    unsigned			width, height;
    int				x, y;
    unsigned			depth;
    XImage			*img;
    unsigned char		ch;
    Pixmap			tile;
    unsigned char		tile_id;

    ch = RReadByte(rc->fp);
    tile_id = RReadByte(rc->fp);
    if (ch == RC_TILE) {
	if (tile_id == 0)
	    return None;
	for (lptr = rc->tlist; lptr != NULL; lptr = lptr->next) {
	    if (lptr->tile_id == tile_id)
		return lptr->tile;
	}
	return None;
    }
    if (ch != RC_NEW_TILE) {
	fprintf(stderr, "Error: New tile expected, not found! (%d)\n", ch);
	exit(1);
    }
    width = RReadUShort(rc->fp);
    height = RReadUShort(rc->fp);
    depth = DefaultDepth(dpy, screen_num);
    img = XCreateImage(dpy, DefaultVisual(dpy, screen_num),
		       depth, ZPixmap,
		       0, NULL,
		       width, height,
		       (depth <= 8) ? 8 : (depth <= 16) ? 16 : 32,
		       0);
    if (!img) {
	fprintf(stderr, "Can't create XImage %ux%u", width, height);
	exit(1);
    }
    img->data = (char *)MyMalloc(img->bytes_per_line * height, MEM_GC);
    for (y = 0; y < img->height; y++) {
	for (x = 0; x < img->width; x++) {
	    ch = RReadByte(rc->fp);
	    XPutPixel(img, x, y, rc->pixels[ch]);
	}
    }
    tile = XCreatePixmap(dpy, RootWindow(dpy, screen_num),
			 width, height, depth);
    if (!tile) {
	fprintf(stderr, "Can't create Pixmap %ux%u", width, height);
	exit(1);
    }
    XPutImage(dpy, tile, rc->gc, img, 0, 0, 0, 0, width, height);
    XDestroyImage(img);

    if (!(lptr = XMALLOC(tile_list_t, 1))) {
	perror("memory");
	exit(1);
    }
    lptr->next = rc->tlist;
    lptr->tile = tile;
    lptr->tile_id = tile_id;
    rc->tlist = lptr;

    return tile;
}

/*
 * Read an encoded GC from the recorded input stream.
 */
static struct rGC *RReadGCValues(struct xprc *rc)
{
    int			c = getc(rc->fp);
    struct rGC		gc, *gcp;
    unsigned short	input_mask;

    memset(&gc, 0, sizeof(gc));

    if (c == RC_NOGC)
	gc.mask = 0;

    else if (c != RC_GC) {
	openErrorWindow(rc->ewin, "GC expected on position %ld, not %d",
			ftell(rc->fp), c);
	return NULL;
    }
    else {
	input_mask = RReadByte(rc->fp);
	if (input_mask & RC_GC_B2) {
	    input_mask |= (RReadByte(rc->fp) << 8);
	}
	gc.mask = 0;
	if (input_mask & RC_GC_FG) {
	    gc.mask |= GCForeground;
	    gc.foreground = rc->pixels[RReadByte(rc->fp)];
	}
	if (input_mask & RC_GC_BG) {
	    gc.mask |= GCBackground;
	    gc.background = rc->pixels[RReadByte(rc->fp)];
	}
	if (input_mask & RC_GC_LW) {
	    gc.mask |= GCLineWidth;
	    gc.line_width = RReadByte(rc->fp);
	    if(rc->linewidth)
	    	gc.line_width = rc->linewidth;
	}
	if (input_mask & RC_GC_LS) {
	    gc.mask |= GCLineStyle;
	    gc.line_style = RReadByte(rc->fp);
	}
	if (input_mask & RC_GC_DO) {
	    gc.mask |= GCDashOffset;
	    gc.dash_offset = RReadByte(rc->fp);
	}
	if (input_mask & RC_GC_FU) {
	    gc.mask |= GCFunction;
	    gc.function = RReadByte(rc->fp);
	}
	if (input_mask & RC_GC_DA) {
	    int i;
	    gc.num_dashes = RReadByte(rc->fp);
	    if (gc.num_dashes == 0) {
		gc.dash_list = NULL;
	    } else {
		gc.dash_list = (char *)MyMalloc(gc.num_dashes, MEM_GC);
		for (i = 0; i < gc.num_dashes; i++)
		    gc.dash_list[i] = RReadByte(rc->fp);
	    }
	}
	if (input_mask & RC_GC_B2) {
	    if (input_mask & RC_GC_FS) {
		gc.mask |= GCFillStyle;
		gc.fill_style = RReadByte(rc->fp);
	    }
	    if (input_mask & RC_GC_XO) {
		gc.mask |= GCTileStipXOrigin;
		gc.ts_x_origin = RReadLong(rc->fp);
	    }
	    if (input_mask & RC_GC_YO) {
		gc.mask |= GCTileStipYOrigin;
		gc.ts_y_origin = RReadLong(rc->fp);
	    }
	    if (input_mask & RC_GC_TI) {
		gc.mask |= GCTile;
		gc.tile = RReadTile(rc);
	    }
	}
    }

    /*
     * kps - This linked list of GCs is very inefficient for some
     * big recordings.
     */
#ifdef USE_GCLIST
    for (gcp = gclist; gcp != NULL; gcp = gcp->next) {
	if (gcp->mask != gc.mask)
	    continue;
	if ((gc.mask & GCForeground) && gc.foreground != gcp->foreground)
	    continue;
	if ((gc.mask & GCBackground) && gc.background != gcp->background)
	    continue;
	if ((gc.mask & GCLineWidth) && gc.line_width != gcp->line_width)
	    continue;
	if ((gc.mask & GCLineStyle) && gc.line_style != gcp->line_style)
	    continue;
	if ((gc.mask & GCDashOffset) && gc.dash_offset != gcp->dash_offset)
	    continue;
	if ((gc.mask & GCFunction) && gc.function != gcp->function)
	    continue;
	if ((gc.mask & GCFillStyle) && gc.fill_style != gcp->fill_style)
	    continue;
	if ((gc.mask & GCTileStipXOrigin)
	    && gc.ts_x_origin != gcp->ts_x_origin)
	    continue;
	if ((gc.mask & GCTileStipYOrigin)
	    && gc.ts_y_origin != gcp->ts_y_origin)
	    continue;
	if ((gc.mask & GCTile) && gc.tile != gcp->tile)
	    continue;
	if (gc.num_dashes != gcp->num_dashes)
	    continue;
	if (gc.num_dashes > 0) {
	    if (memcmp(gc.dash_list, gcp->dash_list, gc.num_dashes))
		continue;
	    MyFree(gc.dash_list, gc.num_dashes, MEM_GC);
	}
	return gcp;
    }
#endif
    gcp = (struct rGC *)MyMalloc(sizeof(*gcp), MEM_GC);
    memcpy(gcp, &gc, sizeof(*gcp));
#ifdef USE_GCLIST
    gcp->next = gclist;
    gclist = gcp;
#endif

    return gcp;
}

static void RemoveFrameFromLRU(struct xprc *rc, struct frame *f)
{
    if (f->older != NULL)
	f->older->newer = f->newer;

    if (rc->newest == f)
	rc->newest = f->older;

    if (f->newer != NULL)
	f->newer->older = f->older;

    if (rc->oldest == f)
	rc->oldest = f->newer;

    f->newer = NULL;
    f->older = NULL;
}

static void AddFrameToLRU(struct xprc *rc, struct frame *f)
{
    f->newer = NULL;
    f->older = rc->newest;

    if (rc->newest)
	rc->newest->newer = f;
    else
	rc->oldest = f;

    rc->newest = f;
}

static void TouchFrame(struct xprc *rc, struct frame *f)
{
    if (rc->newest != f) {
	RemoveFrameFromLRU(rc, f);
	AddFrameToLRU(rc, f);
    }
}

/*
 * Release all memory associated with a list of shape data.
 */
static void FreeShapes(struct shape *shp)
{
    struct shape	*nextshp;

    while (shp) {

	nextshp = shp->next;
	shp->next = NULL;

	switch (shp->type) {

	case RC_DRAWLINES:
	    MyFree(shp->shape.lines.points,
		   shp->shape.lines.npoints * sizeof(XPoint),
		   MEM_POINT);
	    break;

	case RC_DRAWSTRING:
	    MyFree(shp->shape.string.string,
		   shp->shape.string.length,
		   MEM_STRING);
	    break;

	case RC_FILLPOLYGON:
	    MyFree(shp->shape.polygon.points,
		   shp->shape.polygon.npoints * sizeof(XPoint),
		   MEM_POINT);
	    break;

	case RC_FILLRECTANGLES:
	    MyFree(shp->shape.rectangles.rectangles,
		   shp->shape.rectangles.nrectangles * sizeof(XRectangle),
		   MEM_POINT);
	    break;

	case RC_DRAWARCS:
	    MyFree(shp->shape.arcs.arcs,
		   shp->shape.arcs.narcs * sizeof(XArc),
		   MEM_POINT);
	    break;

	case RC_DRAWSEGMENTS:
	    MyFree(shp->shape.segments.segments,
		   shp->shape.segments.nsegments * sizeof(XSegment),
		   MEM_POINT);
	    break;

	default:
	    break;
	}

	shp->type = 0;
#ifndef USE_GCLIST
	MyFree(shp->gc, sizeof(struct rGC), MEM_GC);
#endif
	MyFree(shp, sizeof(struct shape), MEM_SHAPE);
	shp = nextshp;
    }
}

/*
 * Release all memory used for storing the data of a frame.
 */
static void FreeFrameData(struct frame *f)
{
    if (!f)
	return;

    FreeShapes(f->shapes);
    f->shapes = NULL;

    frames_in_core--;
}

/*
 * Release all memory associated with a frame, including the frame header.
 */
static void FreeFrame(struct xprc *rc, struct frame *f)
{
    if (!f)
	return;

    FreeFrameData(f);
    if (f->next)
	f->next->prev = f->prev;

    if (f->prev)
	f->prev->next = f->next;

    if (f == rc->head)
	rc->head = f->next;

    if (f == rc->tail)
	rc->tail = f->prev;

    if (f == rc->cur)
	rc->cur = (f->next != NULL) ? f->next : f->prev;

    f->next = NULL;
    f->prev = NULL;
    RemoveFrameFromLRU(rc, f);
    MyFree(f, sizeof(struct frame), MEM_FRAME);
}

/*
 * Free all frames, including their headers.
 */
static void FreeXPRCData(struct xprc *rc)
{
    while (rc->head)
	FreeFrame(rc, rc->head);
}

/*
 * Free up memory by removing the data of some frames from memory.
 * The frames last accessed are the ones chosen to be freed.
 */
static void Purge(struct xprc *rc)
{
    long		goal = (3 * max_mem) / 4;	/* free one quarter */
    int			max_purge = 3;		/* don't purge too much */
    struct frame	*keep_frame;
    struct frame	*f;

    /*
     * If there are a huge number of recorded frames
     * then the frame headers could take up all of the reserved memory.
     * If this happens permit ourselves to use more memory.
     */
    if (goal < mem_typed_used[MEM_FRAME]) {
	max_mem = mem_typed_used[MEM_FRAME] + (max_mem - goal);
	goal = (3 * max_mem) / 4;
    }
    if (!(keep_frame = rc->newest) || !(keep_frame = keep_frame->older))
	/* should at least keep two frames. */
	return;

    while (mem_all_types_used > goal
	&& rc->oldest != keep_frame
	&& max_purge > 0) {
	f = rc->oldest;
	RemoveFrameFromLRU(rc, f);
	FreeFrameData(f);
	max_purge--;
    }
}

/*
 * Read all of the data of one complete frame into memory.
 * The frame header has already been allocated.
 * If our input is a regular file then we may be reloading
 * the data of a previous purged frame.
 */
static int readFrameData(struct xprc *rc, struct frame *f)
{
    int			c = 0, prev_c;
    struct shape	*shp = NULL,
			*shphead = NULL,
			*newshp;
    XPoint		*xpp;
    XRectangle		*xrp;
    XArc		*xap;
    XSegment		*xsp;
    char		*cp;
    int			done = False;

    clearerr(rc->fp);
    if (rc->seekable && fseek(rc->fp, f->filepos, 0) != 0) {
	perror("Can't reposition file");
	exit(1);
    }

    while (!done) {

	prev_c = c;
	c = getc(rc->fp);

	switch (c) {

	case EOF:
	    openErrorWindow(rc->ewin,
			    "Premature End-Of-File encountered. Truncating.");
	    done = True;
	    rc->eof = True;
	    continue;

	case RC_ENDFRAME:
	    done = True;
	    continue;

	case RC_DRAWARC:
	case RC_DRAWLINES:
	case RC_DRAWLINE:
	case RC_DRAWRECTANGLE:
	case RC_DRAWSTRING:
	case RC_FILLARC:
	case RC_FILLPOLYGON:
	case RC_PAINTITEMSYMBOL:
	case RC_FILLRECTANGLE:
	case RC_FILLRECTANGLES:
	case RC_DRAWARCS:
	case RC_DRAWSEGMENTS:
	case RC_DAMAGED:
	    newshp = (struct shape *)MyMalloc(sizeof(struct shape), MEM_SHAPE);
	    newshp->next = NULL;
	    newshp->type = 0;
	    if ((newshp->gc = RReadGCValues(rc)) == NULL) {
		MyFree(newshp, sizeof(struct shape), MEM_SHAPE);
		done = True;
		continue;
	    }
	    if (shp == NULL) {
		shp = newshp;
		shphead = shp;
	    }
	    else {
		shp->next = newshp;
		shp = shp->next;
	    }
	    shp->type = c;

	    switch (c) {

	    case RC_DRAWARC:
	    case RC_FILLARC:
		shp->shape.arc.x = RReadShort(rc->fp);
		shp->shape.arc.y = RReadShort(rc->fp);
		shp->shape.arc.width = RReadByte(rc->fp);
		shp->shape.arc.height = RReadByte(rc->fp);
		shp->shape.arc.angle1 = RReadShort(rc->fp);
		shp->shape.arc.angle2 = RReadShort(rc->fp);
		break;

	    case RC_DRAWLINES:
		shp->shape.lines.npoints = c = RReadUShort(rc->fp);
		shp->shape.lines.points = xpp =
		    (XPoint *)MyMalloc(sizeof(XPoint) * c, MEM_POINT);
		while (c--) {
		    xpp->x = RReadShort(rc->fp);
		    xpp->y = RReadShort(rc->fp);
		    xpp++;
		}
		shp->shape.lines.mode = RReadByte(rc->fp);
		break;

	    case RC_DRAWLINE:
		shp->shape.line.x1 = RReadShort(rc->fp);
		shp->shape.line.y1 = RReadShort(rc->fp);
		shp->shape.line.x2 = RReadShort(rc->fp);
		shp->shape.line.y2 = RReadShort(rc->fp);
		break;

	    case RC_DRAWRECTANGLE:
	    case RC_FILLRECTANGLE:
		shp->shape.rectangle.x = RReadShort(rc->fp);
		shp->shape.rectangle.y = RReadShort(rc->fp);
		shp->shape.rectangle.width = RReadByte(rc->fp);
		shp->shape.rectangle.height = RReadByte(rc->fp);
		break;

	    case RC_DRAWSTRING:
		shp->shape.string.x = RReadShort(rc->fp);
		shp->shape.string.y = RReadShort(rc->fp);
		shp->shape.string.font = RReadByte(rc->fp);
		shp->shape.string.length = c = RReadUShort(rc->fp);
		shp->shape.string.string
		    = cp = (char *)MyMalloc((size_t)c, MEM_STRING);
		while (c--)
		    *cp++ = getc(rc->fp);
		break;

	    case RC_FILLPOLYGON:
		shp->shape.polygon.npoints = c = RReadUShort(rc->fp);
		shp->shape.polygon.points = xpp =
		    (XPoint *)MyMalloc(sizeof(XPoint) * c, MEM_POINT);
		while (c--) {
		    xpp->x = RReadShort(rc->fp);
		    xpp->y = RReadShort(rc->fp);
		    xpp++;
		}
		shp->shape.polygon.shape = RReadByte(rc->fp);
		shp->shape.polygon.mode = RReadByte(rc->fp);
		break;

	    case RC_PAINTITEMSYMBOL:
		shp->shape.symbol.type = RReadByte(rc->fp);
		shp->shape.symbol.x = RReadShort(rc->fp);
		shp->shape.symbol.y = RReadShort(rc->fp);
		break;

	    case RC_FILLRECTANGLES:
		shp->shape.rectangles.nrectangles = c = RReadUShort(rc->fp);
		shp->shape.rectangles.rectangles = xrp =
		    (XRectangle *)MyMalloc(sizeof(XRectangle) * c, MEM_POINT);
		while (c--) {
		    xrp->x = RReadShort(rc->fp);
		    xrp->y = RReadShort(rc->fp);
		    xrp->width = RReadByte(rc->fp);
		    xrp->height = RReadByte(rc->fp);
		    xrp++;
		}
		break;

	    case RC_DRAWARCS:
		shp->shape.arcs.narcs = c = RReadUShort(rc->fp);
		shp->shape.arcs.arcs = xap =
		    (XArc *)MyMalloc(sizeof(XArc) * c, MEM_POINT);
		while (c--) {
		    xap->x = RReadShort(rc->fp);
		    xap->y = RReadShort(rc->fp);
		    xap->width = RReadByte(rc->fp);
		    xap->height = RReadByte(rc->fp);
		    xap->angle1 = RReadShort(rc->fp);
		    xap->angle2 = RReadShort(rc->fp);
		    xap++;
		}
		break;

	    case RC_DRAWSEGMENTS:
		shp->shape.segments.nsegments = c = RReadUShort(rc->fp);
		shp->shape.segments.segments = xsp =
		    (XSegment *)MyMalloc(sizeof(XSegment) * c, MEM_POINT);
		while (c--) {
		    xsp->x1 = RReadShort(rc->fp);
		    xsp->y1 = RReadShort(rc->fp);
		    xsp->x2 = RReadShort(rc->fp);
		    xsp->y2 = RReadShort(rc->fp);
		    xsp++;
		}
		break;

	    case RC_DAMAGED:
		shp->shape.damage.damaged = RReadByte(rc->fp);
		break;

	    default:
		break;
	    }
	    break;

	default:
	    openErrorWindow(rc->ewin,
			    "Unknown shape type %d (previous = %d) when "
			    "reading frame %d,\nafter %d frames with %d in "
			    "core. Truncating...",
			    c, prev_c, f->number, frame_count, frames_in_core);
	    done = True;
	    continue;
	}

    }

    if (c != RC_ENDFRAME) {
	FreeShapes(shphead);
	return -1;
    }

    f->shapes = shphead;

    frames_in_core++;

    AddFrameToLRU(rc, f);

    if (mem_all_types_used >= max_mem)
	Purge(rc);

    return 0;
}

static int readNewFrame(struct xprc *rc)
{
    int			c;
    struct frame	*f = NULL;

    if (rc->eof)
	return -1;

    if ((c = getc(rc->fp)) == EOF) {
	rc->eof = True;
	MemPrint();
	return -1;
    }
    if (c != RC_NEWFRAME) {
	openErrorWindow(rc->ewin, "Corrupt record file, next frame expected, "
			"not %d.  Truncating.", c);
	rc->eof = True;
	return -1;
    }
    f = (struct frame *)MyMalloc(sizeof(struct frame), MEM_FRAME);
    f->width = RReadUShort(rc->fp);
    f->height = RReadUShort(rc->fp);
    f->shapes = NULL;
    f->next = NULL;
    f->prev = NULL;
    f->newer = NULL;
    f->older = NULL;
    f->number = frame_count;
    if (rc->seekable && (f->filepos = ftell(rc->fp)) == -1) {
	openErrorWindow(rc->ewin, "Can't get file position. Truncating.");
	rc->eof = True;
	MyFree(f, sizeof(struct frame), MEM_FRAME);
	return -1;
    }

    if (readFrameData(rc, f) == -1) {
	FreeFrame(rc, f);
	return -1;
    }

    if (rc->tail == NULL) {
	f->next = NULL;
	f->prev = NULL;
	rc->tail = rc->head = rc->cur = f;
    }
    else {
	f->prev = rc->tail;
	f->next = NULL;
	rc->tail->next = f;
	rc->tail = f;
    }

    frame_count++;

    return 0;
}

static Atom		ProtocolAtom;
static Atom		KillAtom;

static Pixmap		itemBitmaps[NUM_ITEMS];	 /* Bitmaps for the items */
static unsigned char	*itemData[NUM_ITEMS] = {
    itemEnergyPack_bits,
    itemWideangleShot_bits,
    itemRearShot_bits,
    itemAfterburner_bits,
    itemCloakingDevice_bits,
    itemSensorPack_bits,
    itemTransporter_bits,
    itemTank_bits,
    itemMinePack_bits,
    itemRocketPack_bits,
    itemEcm_bits,
    itemLaser_bits,
    itemEmergencyThrust_bits,
    itemTractorBeam_bits,
    itemAutopilot_bits,
    itemEmergencyShield_bits,
    itemDeflector_bits,
    itemHyperJump_bits,
    itemPhasingDevice_bits,
    itemMirror_bits,
    itemArmor_bits
};

static XFontStruct *loadQueryFont(const char *fontName, GC gc)
{
    XFontStruct		*font;

    if ((font = XLoadQueryFont(dpy, fontName)) == NULL) {
	fprintf(stderr, "Can't load font \"%s\", using \"%s\" instead.\n",
		fontName, alternative_font);
	if ((font = XLoadQueryFont(dpy, alternative_font)) == NULL) {
	    fprintf(stderr, "Can't load alternative font \"%s\".\n",
		    alternative_font);
	    font = XQueryFont(dpy, XGContextFromGC(gc));
	}
    }
    return font;
}

static void allocViewColors(struct xprc *rc)
{
    XColor		*cp, /**cp2,*/ myColor;
    int			i /*, j*/;

    rc->pixels = (unsigned long *)
	MyMalloc(2 * rc->maxColors * sizeof(*rc->pixels), MEM_MISC);

    for (i = 0; i < rc->maxColors; i++) {
	cp = &rc->colors[i];
	/*
	 * kps - don't try to do this "optimisation", it seems to break stuff
	 * for some recordings.
	 */
#if 0
	for (j = 0; j < i; j++) {
	    cp2 = &rc->colors[j];
	    if (cp->red == cp2->red &&
		cp->green == cp2->green &&
		cp->blue == cp2->blue) {
		break;
	    }
	}
	if (j < i) {
	    rc->pixels[j] = rc->pixels[i];
	    continue;
	}
#endif
	if (cp->red < 0x0100 &&
	    cp->green < 0x0100 &&
	    cp->blue < 0x0100 &&
	    colormap == DefaultColormap(dpy, screen_num)) {
	    rc->pixels[i] = BlackPixel(dpy, screen_num);
	    continue;
	}
	if (cp->red >= 0xFF00 &&
	    cp->green >= 0xFF00 &&
	    cp->blue >= 0xFF00 &&
	    colormap == DefaultColormap(dpy, screen_num)) {
	    rc->pixels[i] = WhitePixel(dpy, screen_num);
	    continue;
	}
	myColor = *cp;
	if (!XAllocColor(dpy, colormap, &myColor)) {
	    fprintf(stderr, "Hmm, not enough colors? (%d: %04x,%04x,%04x)\n",
		    i, cp->red, cp->green, cp->blue);
	    rc->pixels[i] = (i == 0)
			    ? BlackPixel(dpy, screen_num)
			    : WhitePixel(dpy, screen_num);
	}
	else if (i > 0
		 && myColor.pixel == BlackPixel(dpy, screen_num)
		 && colormap == DefaultColormap(dpy, screen_num)) {
	    rc->pixels[i] = WhitePixel(dpy, screen_num);
	} else
	    rc->pixels[i] = myColor.pixel;
    }
    for (i = 0; i < rc->maxColors; i++)
	rc->pixels[i + rc->maxColors] = rc->pixels[BLACK] ^ rc->pixels[i];
}

static unsigned long allocColor(const char *name)
{
    XColor		color;

    if (!XParseColor(dpy, colormap, name, &color)) {
	fprintf(stderr, "Can't parse color \"%s\"\n", name);
	return WhitePixel(dpy, screen_num);
    }
    if (!XAllocColor(dpy, colormap, &color)) {
	fprintf(stderr, "Can't allocate color \"%s\"\n", name);
	return WhitePixel(dpy, screen_num);
    }
    return color.pixel;
}

static void drawShapes(struct frame *f, XID drawable, struct xprc *rc)
{
    struct shape	*sp;
    XGCValues		values;

    for (sp = f->shapes; sp != NULL; sp = sp->next) {
	if (sp->gc != NULL) {
	    if (sp->gc->mask != 0) {
		values.foreground = sp->gc->foreground;
		values.background = sp->gc->background;
		values.line_width = sp->gc->line_width;
		values.line_style = sp->gc->line_style;
		values.dash_offset = sp->gc->dash_offset;
		values.function = sp->gc->function;
		values.fill_style = sp->gc->fill_style;
		values.ts_x_origin = sp->gc->ts_x_origin;
		values.ts_y_origin = sp->gc->ts_y_origin;
		values.tile = sp->gc->tile;
		XChangeGC(dpy, rc->gc, sp->gc->mask, &values);
	    }
	    if (sp->gc->num_dashes > 0) {
		XSetDashes(dpy, rc->gc,
			   sp->gc->dash_offset,
			   sp->gc->dash_list,
			   sp->gc->num_dashes);
	    }
	}

	switch(sp->type) {

	case RC_DRAWARC:
	    XDrawArc(dpy, drawable, rc->gc,
		     sp->shape.arc.x, sp->shape.arc.y,
		     sp->shape.arc.width, sp->shape.arc.height,
		     sp->shape.arc.angle1, sp->shape.arc.angle2);
	    break;

	case RC_DRAWLINES:
	    XDrawLines(dpy, drawable, rc->gc, sp->shape.lines.points,
		       sp->shape.lines.npoints, sp->shape.lines.mode);
	    break;

	case RC_DRAWLINE:
	    XDrawLine(dpy, drawable, rc->gc,
		      sp->shape.line.x1, sp->shape.line.y1,
		      sp->shape.line.x2, sp->shape.line.y2);
	    break;

	case RC_DRAWRECTANGLE:
	    XDrawRectangle(dpy, drawable, rc->gc,
			   sp->shape.rectangle.x, sp->shape.rectangle.y,
			   sp->shape.rectangle.width,
			   sp->shape.rectangle.height);
	    break;

	case RC_DRAWSTRING:
	    if (sp->shape.string.font == 0)
		XSetFont(dpy, rc->gc, rc->gameFont->fid);
	    else
		XSetFont(dpy, rc->gc, rc->msgFont->fid);
	    XDrawString(dpy, drawable, rc->gc,
			sp->shape.string.x, sp->shape.string.y,
			sp->shape.string.string,
			(int)sp->shape.string.length);
	    break;

	case RC_FILLARC:
	    XFillArc(dpy, drawable, rc->gc,
		     sp->shape.arc.x, sp->shape.arc.y,
		     sp->shape.arc.width, sp->shape.arc.height,
		     sp->shape.arc.angle1, sp->shape.arc.angle2);
	    break;

	case RC_FILLPOLYGON:
	    XFillPolygon(dpy, drawable, rc->gc,
			 sp->shape.polygon.points, sp->shape.polygon.npoints,
			 sp->shape.polygon.shape,
			 sp->shape.polygon.mode);
	    break;

	case RC_FILLRECTANGLE:
	    XFillRectangle(dpy, drawable, rc->gc,
			   sp->shape.rectangle.x, sp->shape.rectangle.y,
			   sp->shape.rectangle.width,
			   sp->shape.rectangle.height);
	    break;

	case RC_PAINTITEMSYMBOL:
	    values.stipple = itemBitmaps[sp->shape.symbol.type];
	    values.fill_style = FillStippled;
	    values.ts_x_origin = sp->shape.symbol.x;
	    values.ts_y_origin = sp->shape.symbol.y;
	    XChangeGC(dpy, rc->gc, GCStipple | GCFillStyle |
		      GCTileStipXOrigin | GCTileStipYOrigin, &values);
	    XFillRectangle(dpy, drawable, rc->gc,
			   sp->shape.symbol.x, sp->shape.symbol.y,
			   ITEM_SIZE, ITEM_SIZE);
	    XSetFillStyle(dpy, rc->gc, FillSolid);
	    break;

	case RC_FILLRECTANGLES:
	    XFillRectangles(dpy, drawable, rc->gc,
			    sp->shape.rectangles.rectangles,
			    sp->shape.rectangles.nrectangles);
	    break;

	case RC_DRAWARCS:
	    XDrawArcs(dpy, drawable, rc->gc,
		      sp->shape.arcs.arcs,
		      sp->shape.arcs.narcs);
	    break;

	case RC_DRAWSEGMENTS:
	    XDrawSegments(dpy, drawable, rc->gc,
			  sp->shape.segments.segments,
			  sp->shape.segments.nsegments);
	    break;

	case RC_DAMAGED:
	    if (sp->shape.damage.damaged)
		XFillRectangle(dpy, drawable, rc->gc,
			       0, 0, f->width, f->height);
	    break;

	default:
	    break;

	}
    }
}

static void OverWriteMsg(struct xprc *rc, const char *msg)
{
    XFontStruct		*font = rc->gameFont;
    int			len = strlen(msg);
    unsigned int	text_w = XTextWidth(font, msg, len);
    unsigned int	text_h = font->ascent + font->descent;
    int			text_x = (rc->cur->width - text_w) / 2;
    int			text_y = (rc->cur->height - text_h) / 2;
    unsigned int	area_w = (text_w < rc->cur->width / 2)
				 ? (rc->cur->width / 2) : text_w;
    unsigned int	area_h = rc->cur->height / 4;
    int			area_x = (rc->cur->width - area_w) / 2;
    int			area_y = (rc->cur->height - area_h) / 2;

    XSetForeground(dpy, rc->gc, rc->pixels[BLACK]);
    XFillRectangle(dpy, rc->topview, rc->gc,
		   area_x, area_y,
		   area_w, area_h);
    XSetForeground(dpy, rc->gc, rc->pixels[WHITE]);
    XSetLineAttributes(dpy, rc->gc, 0, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(dpy, rc->topview, rc->gc,
		   area_x, area_y,
		   area_w, area_h);
    XSetFont(dpy, rc->gc, rc->gameFont->fid);
    XDrawString(dpy, rc->topview, rc->gc,
		text_x, text_y + font->ascent,
		msg, len);
    XFlush(dpy);
}

static void redrawWindow(struct xprc *rc)
{
    XWindowAttributes	attrib;

    if (!rc->cur->shapes)
	readFrameData(rc, rc->cur);
    else if (rc->seekable)
	TouchFrame(rc, rc->cur);

    forceRedraw = False;

    XGetWindowAttributes(dpy, rc->topview, &attrib);

    if (attrib.width != (int)rc->cur->width ||
	attrib.height != (int)rc->cur->height) {
	XWindowChanges values;

	values.width = rc->cur->width;
	values.height = rc->cur->height;

	XReconfigureWMWindow(dpy, rc->topview, screen_num, CWWidth | CWHeight,
			     &values);
    }

    XClearWindow(dpy, rc->topview);

    drawShapes(rc->cur, rc->topview, rc);
}

/*
 * Initialize miscellaneous window hints and properties.
 */
static void Init_wm_prop(Window win,
			 int x, int y,
			 unsigned w, unsigned h,
			 unsigned min_w, unsigned min_h,
			 unsigned max_w, unsigned max_h,
			 int flags)
{
    XClassHint		xclh;
    XWMHints		xwmh;
    XSizeHints		xsh;
    char		msg[256];
    static char		myClass[] = "XP-replay";

    xwmh.flags	   = InputHint|StateHint;
    xwmh.input	   = True;
    xwmh.initial_state = NormalState;

    xsh.flags = (flags|PResizeInc);
    xsh.x = x;
    xsh.width = w;
    xsh.base_width =
    xsh.min_width = min_w;
    xsh.max_width = max_w;
    xsh.width_inc = 1;
    xsh.height = h;
    xsh.base_height =
    xsh.min_height = min_h;
    xsh.max_height = max_h;
    xsh.height_inc = 1;
    xsh.y = y;

    xclh.res_name = NULL;		/* NULL: Automatically uses Argv[0], */
    xclh.res_class = myClass;		/* stripped of directory prefixes. */

    /*
     * Set the above properties.
     */
    XSetWMProperties(dpy, win,
		     NULL, NULL,
		     Argv, Argc,
		     &xsh, &xwmh, &xclh);

    /*
     * Now initialize icon and window title name.
     */
    strcpy(msg, strchr(*Argv, '/') ? strchr(*Argv, '/') + 1 : *Argv);
    XStoreName(dpy, win, msg);

    XSetIconName(dpy, win, msg);

    /*
     * Tell the window manager that we know how to quit ourselves.
     */
    ProtocolAtom = XInternAtom(dpy, "WM_PROTOCOLS", False);
    KillAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &KillAtom, 1);
}

static struct recordwin *Init_recordwindow(unsigned long bg, void *data)
{
    struct recordwin	*rwin = (struct recordwin *)
	MyMalloc(sizeof(struct recordwin), MEM_UI);
    int			x, y;
    unsigned		w, h;
    XWindowChanges	values;
    union button_image	image;

    w = 100;
    h = 50;
    x = (DisplayWidth(dpy, screen_num) - w) / 2;
    y = (DisplayHeight(dpy, screen_num) - h) / 2;

    rwin->win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen_num),
				    x, y,
				    w, h,
				    1,
				    BlackPixel(dpy, screen_num),
				    bg);
    rwin->gc = XCreateGC(dpy, rwin->win, 0, NULL);
    XSetForeground(dpy, rwin->gc, BlackPixel(dpy, screen_num));
    XSelectInput(dpy, rwin->win,
		 KeyPressMask | KeyReleaseMask | ButtonPressMask);

    x = 0;
    y = 5;
    image.string = "Start save here";
    rwin->mark_start_but = CreateButton(dpy, rwin->win,
					5, y,
					0, 0,
					image, 0, 0,
					BlackPixel(dpy, screen_num),
					markSaveStart, data,
					BUTTON_TEXT | BUTTON_RELEASE,
					0);
    GetButtonSize(rwin->mark_start_but, &w, &h);
    if (x < (int)w)
	x = w;
    y += h + 5;

    image.string = "End save here";
    rwin->mark_end_but = CreateButton(dpy, rwin->win,
				      5, y,
				      0, 0,
				      image, 0, 0,
				      BlackPixel(dpy, screen_num),
				      markSaveEnd, data,
				      BUTTON_TEXT | BUTTON_RELEASE,
				      0);
    GetButtonSize(rwin->mark_end_but, &w, &h);
    if (x < (int)w)
	x = w;
    y += h + 5;

    image.string = "Save in PPM format";
    rwin->recbut = CreateButton(dpy, rwin->win,
				5, y,
				0, 0,
				image, 0, 0,
				BlackPixel(dpy, screen_num),
				saveStartToEndPPM, data,
				BUTTON_TEXT | BUTTON_RELEASE,
				0);
    GetButtonSize(rwin->recbut, &w, &h);
    if (x < (int)w)
	x = w;
    y += h + 5;

    image.string = "Save in XPR format";
    rwin->recbut = CreateButton(dpy, rwin->win,
				5, y,
				0, 0,
				image, 0, 0,
				BlackPixel(dpy, screen_num),
				saveStartToEndXPR, data,
				BUTTON_TEXT | BUTTON_RELEASE,
				0);
    GetButtonSize(rwin->recbut, &w, &h);
    if (x < (int)w)
	x = w;
    y += h + 5;

    values.width = x + 10;
    values.height = y;
    values.x = (DisplayWidth(dpy, screen_num) - values.width) / 2;
    values.y = (DisplayHeight(dpy, screen_num) - values.height) / 2;
    
    XReconfigureWMWindow(dpy, rwin->win, screen_num,
			 CWX | CWY | CWWidth | CWHeight, &values);

    Init_wm_prop(rwin->win,
		 x, y,
		 w, h,
		 0, 0,
		 0, 0,
		 PSize | PPosition);
    return(rwin);
}

static void openErrorWindow(struct errorwin *ewin, const char *fmt, ...)
{
    unsigned w, h;
    int i;
    va_list ap;
    XWindowChanges values;
    char *p = ewin->message, *q = ewin->message;

    va_start(ap, fmt);
    vsprintf(ewin->message, fmt, ap);
    va_end(ap);

    ewin->len = strlen(ewin->message);

    w = 0;
    h = 0;

    while(*p != '\0') {
	while(*p != '\0' && *p != '\n')
	    p++;
	if (p != q)
	    if ((i = XTextWidth(ewin->font, q, p-q)) > (int)w)
		w = i;
	if (*p != '\0')
	    p++;
	q = p;
	h++;
    }

    h *= ewin->font->ascent + ewin->font->descent + 2;
    MoveButton(ewin->but, 5, (int)(h + 5));
    values.width = w + 10;
    values.height = h + 15;
    GetButtonSize(ewin->but, &w, &h);
    values.height += h;
    if (values.width < (int)(w + 10))
	values.width = w + 10;
    values.x = (DisplayWidth(dpy, screen_num) - values.width)/2;
    values.y = (DisplayHeight(dpy, screen_num) - values.height)/2;

    XReconfigureWMWindow(dpy, ewin->win, screen_num,
			 CWX | CWY | CWWidth | CWHeight, &values);

    XMapWindow(dpy, ewin->win);

    /* In case it's already mapped, we'd better clear it */

    XClearArea(dpy, ewin->win, 0, 0, 0, 0, True);
    XBell(dpy, 0);

    /* We try to grab the pointer now, but our window may not have been
       mapped. If it hasn't we'll grab the pointer on out first expose
       event */

    ewin->grabbed = False;
    if (XGrabPointer(dpy, ewin->win, True, 0, GrabModeAsync, GrabModeSync,
		     ewin->win, None, CurrentTime) == GrabSuccess)
	ewin->grabbed = True;
}

static void closeErrorWindow(void *data)
{
    struct errorwin *ewin = (struct errorwin *) data;

    XUngrabPointer(dpy, CurrentTime);
    XUnmapWindow(dpy, ewin->win);
}

static struct errorwin *Init_errorwindow(unsigned long bg)
{
    struct errorwin *ewin = (struct errorwin *)
	MyMalloc(sizeof(struct errorwin), MEM_UI);
    int x, y;
    unsigned w, h;
    union button_image image;

    w = 100;
    h = 50;
    x = (DisplayWidth(dpy, screen_num) - w) / 2;
    y = (DisplayHeight(dpy, screen_num) - h) / 2;

    ewin->win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen_num),
				    x, y,
				    w, h,
				    1,
				    BlackPixel(dpy, screen_num),
				    bg);
    ewin->gc = XCreateGC(dpy, ewin->win, 0, NULL);
    ewin->font = loadQueryFont("-*-helvetica-medium-r-*-*-18-*-*-*-*-*-*-*",
			       ewin->gc);
    XSetForeground(dpy, ewin->gc, BlackPixel(dpy, screen_num));
    XSetFont(dpy, ewin->gc, ewin->font->fid);
    image.string = "OK";
    ewin->but = CreateButton(dpy, ewin->win, 5, 15,
			     0, 0, image, 0, 0,
			     BlackPixel(dpy, screen_num),
			     closeErrorWindow, (void *) ewin,
			     BUTTON_TEXT | BUTTON_RELEASE, 0);
    XSelectInput(dpy, ewin->win, ExposureMask);
    *ewin->message = '\0';
    ewin->len = 0;
    ewin->grabbed = True;
    Init_wm_prop(ewin->win,
		 x, y,
		 w, h,
		 0, 0,
		 0, 0,
		 PSize | PPosition);
    return(ewin);
}

static void Init_topview(struct xprc *rc)
{
    int			i;
    unsigned int	w = rc->view_width;
    unsigned int	h = rc->view_height;
    int			x = 0;
    int			y = (DisplayHeight(dpy, screen_num) - h) / 2;

    rc->topview = XCreateSimpleWindow(dpy, RootWindow(dpy, screen_num),
				      x, y,
				      w, h,
				      1,
				      BlackPixel(dpy, screen_num),
				      BlackPixel(dpy, screen_num));

    Init_wm_prop(rc->topview,
		 x, y,
		 w, h,
		 0, 0,
		 0, 0,
		 PSize | PPosition);

    rc->gc = XCreateGC(dpy, rc->topview, 0, NULL);

    XSelectInput(dpy, rc->topview,
		 ExposureMask |
		 KeyPressMask | KeyReleaseMask | ButtonPressMask);

    for (i = 0; i < NUM_ITEMS; i++)
        itemBitmaps[i] = XCreateBitmapFromData(dpy, rc->topview,
					       (char *)itemData[i],
					       ITEM_SIZE, ITEM_SIZE);

    rc->gameFont = loadQueryFont(rc->gameFontName, rc->gc);
    rc->msgFont = loadQueryFont(rc->msgFontName, rc->gc);

    allocViewColors(rc);

    rc->ewin = Init_errorwindow(BlackPixel(dpy, screen_num));
}

static void Init_topmain(struct xui *ui, struct xprc *rc)
{
    int			topx, topy, i, x, y, w, mw;
    unsigned		topw, toph;
    XWindowChanges	values;

    ui->black = BlackPixel(dpy, screen_num);
    ui->white = WhitePixel(dpy, screen_num);
    ui->mainbg = allocColor("gray80");
    ui->red = allocColor("red");

    topw = 26 - BUTTON_SPACING;
    toph = 0;

    for (i = 0; i < (int)NUM_BUTTONS; i++) {
	topw += buttonInit[i].width + BUTTON_BORDER + BUTTON_SPACING;
	if (toph < buttonInit[i].height)
	    toph = buttonInit[i].height;
    }


   ui->topmain = XCreateSimpleWindow(dpy, RootWindow(dpy, screen_num),
				     0, 0,
				     topw, toph+100,
				     1,
				     BlackPixel(dpy, screen_num),
				     ui->mainbg);

    ui->gc = XCreateGC(dpy, ui->topmain, 0, NULL);

    XSelectInput(dpy, ui->topmain,
		 ExposureMask |
		 KeyPressMask | KeyReleaseMask |
		 ButtonPressMask | ButtonReleaseMask);

    ui->smallFont = loadQueryFont(small_font, ui->gc);
    ui->boldFont = loadQueryFont(small_bold_font, ui->gc);

    SetGlobalButtonAttributes(ui->mainbg, ui->white, ui->black, ui->black);

    x = 5;
    y = (toph>>1) + 5;
    for (i = 0; i < (int)NUM_BUTTONS; i++) {
	union button_image p;

	p.icon = XCreateBitmapFromData(dpy, ui->topmain,
				       (char *) buttonInit[i].data,
				       buttonInit[i].width,
				       buttonInit[i].height);

	if (i == BUTTON_EJECT)
	    x += 16;

	ui->buttons[i] =
	    CreateButton(dpy, ui->topmain,
			 x, (int)(y - (buttonInit[i].height>>1)),
			 buttonInit[i].width+4, buttonInit[i].height+4, p,
			 buttonInit[i].width, buttonInit[i].height,
			 (buttonInit[i].color == 0) ? ui->black : ui->red,
			 buttonInit[i].callback, (void *) ui,
			 buttonInit[i].flags, buttonInit[i].group);
	x += buttonInit[i].width+BUTTON_BORDER+BUTTON_SPACING;
    }

    ui->labels = (struct label *)
	MyMalloc(NUM_LABELS * sizeof(struct label), MEM_UI);
    memset(ui->labels, 0, NUM_LABELS * sizeof(struct label));
    ui->labels[0].name = "Position";
    ui->labels[0].type = LABEL_TIME;
    ui->labels[0].data.f = &(rc->cur);
    ui->labels[0].j = False;
    ui->labels[1].name = "Server";
    ui->labels[1].type = LABEL_STRING;
    ui->labels[1].data.s = rc->servername;
    ui->labels[1].j = False;
    ui->labels[2].name = "Player";
    ui->labels[2].type = LABEL_STRING;
    ui->labels[2].data.s = rc->nickname;
    ui->labels[2].j = False;
    ui->labels[3].name = "Name";
    ui->labels[3].type = LABEL_STRING;
    ui->labels[3].data.s = rc->realname;
    ui->labels[3].j = False;
    ui->labels[4].name = "Client";
    ui->labels[4].type = LABEL_STRING;
    ui->labels[4].data.s = rc->hostname;
    ui->labels[4].j = False;
    ui->labels[5].name = "FPS";
    ui->labels[5].type = LABEL_INTEGER;
    ui->labels[5].data.i = &(rc->fps);
    ui->labels[5].j = True;

    mw = 0;
    ui->labels[0].x = 5;
    ui->labels[0].y = toph + 15;
    for (i = 1; i < 5; i++) {
	ui->labels[i].x = ui->labels[i-1].x;
	ui->labels[i].y = ui->labels[i-1].y
	    + ui->boldFont->ascent + ui->boldFont->descent + 2;
    }
    toph = ui->labels[4].y + ui->boldFont->ascent + ui->boldFont->descent + 5;
    for (i = 0; i < 5; i++) {
	w = XTextWidth(ui->boldFont, ui->labels[i].name,
		       (int)strlen(ui->labels[i].name));
	if (w > mw)
	    mw = w;
    }
    for (i = 0; i < 5; i++)
	ui->labels[i].w = mw;
    ui->labels[5].x = topw - ui->labels[0].x;
    ui->labels[5].y = ui->labels[0].y;
    ui->labels[5].w = 0;

    topx = DisplayWidth(dpy, screen_num) - topw;
    topy = (DisplayHeight(dpy, screen_num) - toph) / 3;

    values.x = topx;
    values.y = topy;
    values.width = topw;
    values.height = toph;

    XReconfigureWMWindow(dpy, ui->topmain, screen_num,
			 CWX | CWY | CWWidth | CWHeight, &values);

    Init_wm_prop(ui->topmain,
		 topx, topy,
		 topw, toph,
		 0, 0,
		 0, 0,
		 PSize | PPosition);

    ui->ewin = Init_errorwindow(ui->mainbg);
    ui->rwin = Init_recordwindow(ui->mainbg, (void *) rc);
}

static void redrawLabel(struct xui *ui, struct xprc *rc, struct label *lb)
{
    int			fn, hr, min, sec, fr;
    unsigned		lw, vw, cw;
    char		value_str[256];

    XSetForeground(dpy, ui->gc, ui->black);

    switch (lb->type) {

    case LABEL_TIME:
	fn = (*lb->data.f)->number;
	hr = fn / (60 * 60 * rc->fps);
	min = (fn - (hr * 60 * 60 * rc->fps)) / (60 * rc->fps);
	sec = (fn - ((hr * 60 + min) * 60 * rc->fps)) / (rc->fps);
	fr = (fn - (((hr * 60 + min) * 60 + sec) * rc->fps)) * 100 / rc->fps;
	sprintf(value_str, "%02d:%02d:%02d.%02d", hr, min, sec, fr);
	break;

    case LABEL_INTEGER:
	sprintf(value_str, "%d", *lb->data.i);
	break;

    case LABEL_STRING:
	strcpy(value_str, lb->data.s);
	break;

    default:
	break;
    }

    if (lb->w == 0)
	lw = XTextWidth(ui->boldFont, lb->name, (int)strlen(lb->name));
    else
	lw = lb->w;
    cw = XTextWidth(ui->boldFont, ": ", 2);
    vw = XTextWidth(ui->smallFont, value_str, (int)strlen(value_str));
#if 0
	XClearArea(dpy, ui->topmain,
		   lb->x, lb->y,
		   lw + 2,
		   ui->boldFont->ascent + ui->boldFont->descent,
		   False);
#endif
    XClearArea(dpy, ui->topmain,
	       (int)(lb->x + lw + cw + 1), lb->y,
	       vw + 2,
	       (unsigned)(ui->smallFont->ascent + ui->smallFont->descent),
	       False);
    if (lb->j) {
	lw = -lw-cw-vw;
	cw = -cw-vw;
	vw = -vw;
    } else {
	vw = lw+cw;
	cw = lw;
	lw = 0;
    }
    XSetFont(dpy, ui->gc, ui->boldFont->fid);
    XDrawString(dpy, ui->topmain, ui->gc,
		(int)(lb->x + 1 + lw), lb->y + ui->boldFont->ascent,
		lb->name, (int)strlen(lb->name));
    XDrawString(dpy, ui->topmain, ui->gc,
		(int)(lb->x + 1 + cw), lb->y + ui->boldFont->ascent,
		": ", 2);
    XSetFont(dpy, ui->gc, ui->smallFont->fid);
    XDrawString(dpy, ui->topmain, ui->gc,
		(int)(lb->x + 1 + vw), lb->y + ui->smallFont->ascent,
		value_str, (int)strlen(value_str));
}

static void redrawMain(struct xui *ui, struct xprc *rc)
{
    int			i;

    for (i = 0; i < NUM_LABELS; i++)
	redrawLabel(ui, rc, &(ui->labels[i]));
}

static void redrawError(struct errorwin *ewin)
{
    int y = 5 + ewin->font->ascent;
    char *p = ewin->message, *q = ewin->message;

    if (!ewin->grabbed)
	XGrabPointer(dpy, ewin->win, True, 0, GrabModeAsync, GrabModeSync,
		     ewin->win, None, CurrentTime);
    ewin->grabbed = True;	/* Only try once */

    while(*p != '\0') {
	while(*p != '\0' && *p != '\n')
	    p++;
	if (p != q)
	    XDrawString(dpy, ewin->win, ewin->gc, 5, y, q, p-q);
	if (*p != '\0')
	    p++;
	q = p;
	y += ewin->font->ascent + ewin->font->descent + 2;
    }
}

static void BuildGamma(unsigned char tbl[256], double gamma_val)
{
    int			i, v;
    double		one_over_gamma, ind;
    /* extern double	pow(double, double); */

    one_over_gamma = 1.0 / gamma_val;
    for (i = 0 ; i <= 255; i++) {
	ind = (double) i / 255.0;
	v = (int)((255.0 * pow(ind, one_over_gamma)) + 0.5);
	tbl[i] = (v > 255) ? 255 : v;
    }
    if (debug) {
	printf("gamma table for gamma correction factor %f:\n", gamma_val);
	for (i = 0 ; i <= 255; i++) {
	    printf("%02x  ", tbl[i]);
	    if (!((i + 1) & 0x0F))
		printf("\n");
	}
    }
}

static void GammaCorrect(unsigned char *data, int size, unsigned char tbl[256])
{
    while (size) {
	*data = tbl[*data];
	size--;
	data++;
    }
}

static void ScalePPM(unsigned char *rgbdata, unsigned cols, unsigned rows,
		     double scale, double gamma_val, FILE *fp)
{
#define SCALE		4096
#define HALFSCALE	2048

    unsigned char	*xelrow;
    unsigned char	*tempxelrow;
    unsigned char	*newxelrow;
    unsigned char	*xP;
    unsigned char	*nxP;
    size_t		rowsread, newrows, newcols;
    unsigned		row, col;
    int			needtoreadrow;
    double		xscale, yscale;
    long		sxscale, syscale;
    long		fracrowtofill, fracrowleft;
    long		*rs;
    long		*gs;
    long		*bs;
    long		r, g, b;
    long		fraccoltofill, fraccolleft;
    int			needcol;
    size_t		size_tempxelrow, size_newxelrow, size_rsgsbs;
    unsigned char	gammatbl[256];

    if (gamma_val > 0)
	BuildGamma(gammatbl, gamma_val);

    newcols = (size_t) (cols * scale + 0.999);
    newrows = (size_t) (rows * scale + 0.999);
    xscale = (double) newcols / (double) cols;
    yscale = (double) newrows / (double) rows;
    sxscale = (long) (xscale * SCALE);
    syscale = (long) (yscale * SCALE);
    size_newxelrow = 3 * newcols;
    size_tempxelrow = 3 * cols;
    size_rsgsbs = cols * sizeof(long);
    newxelrow = (unsigned char *)MyMalloc(size_newxelrow, MEM_MISC);
    tempxelrow = (unsigned char *)MyMalloc(size_tempxelrow, MEM_MISC);
    rs = (long *)MyMalloc(size_rsgsbs, MEM_MISC);
    gs = (long *)MyMalloc(size_rsgsbs, MEM_MISC);
    bs = (long *)MyMalloc(size_rsgsbs, MEM_MISC);
    fracrowtofill = SCALE;
    fracrowleft = syscale;
    for (col = 0; col < cols; col++)
	rs[col] = gs[col] = bs[col] = HALFSCALE;

    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", newcols, newrows);
    fprintf(fp, "%d\n", 255);

    xelrow = rgbdata;
    rowsread = 1;
    needtoreadrow = 0;
    for (row = 0; row < newrows; row++) {
	while (fracrowleft < fracrowtofill) {
	    if (needtoreadrow) {
		if (rowsread < rows) {
		    xelrow = &rgbdata[3 * rowsread * cols];
		    rowsread++;
		}
	    }
	    for (col = 0, xP = xelrow; col < cols; col++) {
		rs[col] += fracrowleft * *xP++;
		gs[col] += fracrowleft * *xP++;
		bs[col] += fracrowleft * *xP++;
	    }
	    fracrowtofill -= fracrowleft;
	    fracrowleft = syscale;
	    needtoreadrow = 1;
	}

	/* Now fracrowleft is >= fracrowtofill, so we can produce a row. */
	if (needtoreadrow) {
	    if (rowsread < rows) {
		xelrow = &rgbdata[3 * rowsread * cols];
		rowsread++;
		needtoreadrow = 0;
	    }
	}
	for (col = 0, xP = xelrow, nxP = tempxelrow; col < cols; col++) {
	    r = rs[col] + fracrowtofill * *xP++;
	    g = gs[col] + fracrowtofill * *xP++;
	    b = bs[col] + fracrowtofill * *xP++;
	    r /= SCALE;
	    if (r > 255) r = 255;
	    g /= SCALE;
	    if (g > 255) g = 255;
	    b /= SCALE;
	    if (b > 255) b = 255;
	    *nxP++ = r;
	    *nxP++ = g;
	    *nxP++ = b;
	    rs[col] = gs[col] = bs[col] = HALFSCALE;
	}
	fracrowleft -= fracrowtofill;
	if (fracrowleft == 0) {
	    fracrowleft = syscale;
	    needtoreadrow = 1;
	}
	fracrowtofill = SCALE;

	/* Now scale X from tempxelrow into newxelrow and write it out. */
	nxP = newxelrow;
	fraccoltofill = SCALE;
	r = g = b = HALFSCALE;
	needcol = 0;
	for (col = 0, xP = tempxelrow; col < cols; col++, xP += 3) {
	    fraccolleft = sxscale;
	    while (fraccolleft >= fraccoltofill) {
		if (needcol)
		    r = g = b = HALFSCALE;
		r += fraccoltofill * xP[0];
		g += fraccoltofill * xP[1];
		b += fraccoltofill * xP[2];
		r /= SCALE;
		if (r > 255) r = 255;
		g /= SCALE;
		if (g > 255) g = 255;
		b /= SCALE;
		if (b > 255) b = 255;
		*nxP++ = r;
		*nxP++ = g;
		*nxP++ = b;
		fraccolleft -= fraccoltofill;
		fraccoltofill = SCALE;
		needcol = 1;
	    }
	    if (fraccolleft > 0) {
		if (needcol) {
		    r = g = b = HALFSCALE;
		    needcol = 0;
		}
		r += fraccolleft * xP[0];
		g += fraccolleft * xP[1];
		b += fraccolleft * xP[2];
		fraccoltofill -= fraccolleft;
	    }
	}
	if (!needcol) {
	    if (fraccoltofill > 0) {
		xP -= 3;
		r += fraccoltofill * *xP++;
		g += fraccoltofill * *xP++;
		b += fraccoltofill * *xP++;
	    }
	    r /= SCALE;
	    if (r > 255) r = 255;
	    g /= SCALE;
	    if (g > 255) g = 255;
	    b /= SCALE;
	    if (b > 255) b = 255;
	    *nxP++ = r;
	    *nxP++ = g;
	    *nxP++ = b;
	}
	if (gamma_val > 0)
	    GammaCorrect(newxelrow, (int)(3 * newcols), gammatbl);
	fwrite(newxelrow, 1, 3 * newcols, fp);
    }

    fflush(fp);

    MyFree(newxelrow, size_newxelrow, MEM_MISC);
    MyFree(tempxelrow, size_tempxelrow, MEM_MISC);
    MyFree(rs, size_rsgsbs, MEM_MISC);
    MyFree(gs, size_rsgsbs, MEM_MISC);
    MyFree(bs, size_rsgsbs, MEM_MISC);
}

static void SaveFramesPPM(struct xprc *rc)
{
    struct frame	*begin = rc->save_first;
    struct frame	*end = rc->save_last;
    struct frame	*save;
    Pixmap		pixmap;
    XImage		*img;
    unsigned long	pixel;
    int			i, x, y;
    int			done = 0;
    FILE		*fp;
    unsigned char	*ptr, *line, *rgbdata;
    char		buf[256];

    if (!begin)
	openErrorWindow(rc->ewin, "The first frame to save hasn't been set.");
    else if (!end)
	openErrorWindow(rc->ewin, "The last frame to save hasn't been set.");

    if (!begin || !end)
	return;

    if (begin->number > end->number) {
	openErrorWindow(rc->ewin, "First save frame exceeds last save frame, "
			"not saving");
	return;
    }
    if (!rc->seekable) {
	if (!begin->shapes) {
	    openErrorWindow(rc->ewin, "Save failed for standard input");
	    return;
	}
    }

    pixmap = XCreatePixmap(dpy, RootWindow(dpy, screen_num),
			   rc->view_width, rc->view_height,
			   (unsigned)DefaultDepth(dpy, screen_num));
    if (pixmap == BadAlloc || pixmap == BadValue) {
	openErrorWindow(rc->ewin, "Can't allocate a pixmap. Unable to save");
	return;
    }
    if (rc->scale > 0) {
	rgbdata = (unsigned char *)
	    MyMalloc((size_t)(3 * rc->view_width * rc->view_height), MEM_MISC);
	line = NULL;
    } else {
	line = (unsigned char *)
	    MyMalloc((size_t)(3 * rc->view_width), MEM_MISC);
	rgbdata = NULL;
    }

    for (save = begin; !done; save = save->next) {
	sprintf(buf, "Saving frame %d (of %d) ...\n",
		save->number - begin->number + 1,
		end->number - begin->number + 1);
	OverWriteMsg(rc, buf);
	XSetForeground(dpy, rc->gc, rc->pixels[BLACK]);
	XFillRectangle(dpy, pixmap, rc->gc,
		       0, 0,
		       rc->view_width, rc->view_height);
	XFlush(dpy);
	if (!save->shapes)
	    readFrameData(rc, save);
	drawShapes(save, pixmap, rc);
	XFlush(dpy);

	if (!compress) {
	    sprintf(buf, "xp%05d.ppm", save->number);
	    if (!(fp = fopen(buf, "w"))) {
		perror(buf);
		break;
	    } else
		setvbuf(fp, NULL, _IOFBF, (size_t)(8 * 1024));
	} else {
	    sprintf(buf, "compress > xp%05d.ppm.Z", save->number);
	    if (!(fp = popen(buf, "w"))) {
		perror(buf);
		break;
	    }
	}
	if (!(rc->scale > 0)) {
	    fprintf(fp, "P6\n");
	    fprintf(fp, "%d %d\n", rc->view_width, rc->view_height);
	    fprintf(fp, "%d\n", 255);
	}

	img = XGetImage(dpy, pixmap,
			0, 0,
			rc->view_width, rc->view_height,
			AllPlanes, ZPixmap);
	for (y = 0; y < rc->view_height; y++) {
	    if (rc->scale > 0)
		ptr = line = rgbdata + (3 * y * rc->view_width);
	    else
		ptr = line;
	    for (x = 0; x < rc->view_width; x++) {
		pixel = XGetPixel(img, x, y);
		for (i = 0;;) {
		    if (pixel == rc->pixels[i])
			break;
		    if (++i >= rc->maxColors) {
			/* impossible? */
			i = 0;
			break;
		    }
		}
		*ptr++ = rc->colors[i].red >> 8;
		*ptr++ = rc->colors[i].green >> 8;
		*ptr++ = rc->colors[i].blue >> 8;
	    }
	    if (rc->scale > 0)
		line = ptr;
	    else
		fwrite(line, 1, (size_t)(3 * rc->view_width), fp);
	}
	XDestroyImage(img);

	if (rc->scale > 0) {
	    sprintf(buf, "Scaling frame %d (of %d) ...\n",
		    save->number - begin->number + 1,
		    end->number - begin->number + 1);
	    OverWriteMsg(rc, buf);
	    ScalePPM(rgbdata, rc->view_width, rc->view_height,
		     rc->scale, rc->gamma, fp);
	}

	if (!compress)
	    fclose(fp);
	else
	    pclose(fp);

	done = (save == end);
    }

    if (rc->scale > 0)
	MyFree(rgbdata, (size_t)(3 * rc->view_width * rc->view_height),
	       MEM_MISC);
    else
	MyFree(line, (size_t)(3 * rc->view_width), MEM_MISC);

    XFreePixmap(dpy, pixmap);

    if (done) {
	sprintf(buf, "Saved %d frames OK.\n", end->number - begin->number + 1);
	OverWriteMsg(rc, buf);
    } else {
	sprintf(buf, "Saving failed!\n");
	OverWriteMsg(rc, buf);
    }
}

static void RWriteByte(int i, FILE *fp)
{
    putc(i, fp);
}

static void RWriteShort(int i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

static void RWriteUShort(int i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

static void RWriteLong(int i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

static void RWriteULong(int i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

static void RWriteString(char *str, FILE *fp)
{
    int				len = strlen(str);
    int				i;

    RWriteUShort(len, fp);
    for (i = 0; i < len; i++)
	putc(str[i], fp);
}

static int pixel2index(struct xprc *rc, unsigned long pixel)
{
    int				i;

    for (i = 0; i < 2 * rc->maxColors; i++) {
	if (rc->pixels[i] == pixel)
	    return i;
    }
    fprintf(stderr, "Can't find matching pixel\n");
    return 0;
}

static XImage *pixmap2image(Pixmap pixmap)
{
    XImage		*img;
    Window		rootw;
    int			x, y;
    unsigned		width, height, border_width, depth;

    if (!XGetGeometry(dpy, pixmap, &rootw,
		      &x, &y,
		      &width, &height,
		      &border_width, &depth))
	return NULL;

    img = XGetImage(dpy, pixmap,
		    0, 0,
		    width, height,
		    AllPlanes, ZPixmap);
    if (!img)
	return NULL;

    return img;
}

static void RWriteTile(struct xprc *rc, struct rGC *gcp, FILE *fp)
{
    tile_list_t			*tptr;
    XImage			*img;
    int				i, x, y;

    for (tptr = rc->tlist; tptr; tptr = tptr->next) {
	if (tptr->tile == gcp->tile)
	    break;
    }
    if (!tptr || tptr->tile_id == 0 || tptr->tile == None) {
	RWriteByte(RC_TILE, fp);
	RWriteByte(0, fp);
	return;
    }
    if (tptr->flag) {
	RWriteByte(RC_TILE, fp);
	RWriteByte(tptr->tile_id, fp);
	return;
    }
    if (!(img = pixmap2image(tptr->tile))) {
	RWriteByte(RC_TILE, fp);
	RWriteByte(0, fp);
	return;
    }
    tptr->flag = 1;
    RWriteByte(RC_NEW_TILE, fp);
    RWriteByte(tptr->tile_id, fp);
    RWriteUShort(img->width, fp);
    RWriteUShort(img->height, fp);
    for (y = 0; y < img->height; y++) {
	for (x = 0; x < img->width; x++) {
	    unsigned long pixel = XGetPixel(img, x, y);
	    for (i = 0; i < rc->maxColors - 1; i++) {
		if (pixel == rc->pixels[i])
		    break;
	    }
	    RWriteByte(i, fp);
	}
    }

    XDestroyImage(img);
}

static void RWriteGC(struct xprc *rc, struct rGC *gcp, FILE *fp)
{
    int				mask = 0;
    int				i;

    if (gcp->mask == 0) {
	RWriteByte(RC_NOGC, fp);
	return;
    }

    RWriteByte(RC_GC, fp);

    if (gcp->mask & GCForeground)
	mask |= RC_GC_FG;
    if (gcp->mask & GCBackground)
	mask |= RC_GC_BG;
    if (gcp->mask & GCLineWidth)
	mask |= RC_GC_LW;
    if (gcp->mask & GCLineStyle)
	mask |= RC_GC_LS;
    if (gcp->mask & GCDashOffset)
	mask |= RC_GC_DO;
    if (gcp->mask & GCFunction)
	mask |= RC_GC_FU;
    if (gcp->num_dashes > 0 || (gcp->mask & GCDashOffset)) {
	mask |= RC_GC_DA;
	if (gcp->mask & GCDashOffset)
	    mask |= RC_GC_DO;
    }
    if (gcp->mask & GCFillStyle)
	mask |= RC_GC_FS;
    if (gcp->mask & GCTileStipXOrigin)
	mask |= RC_GC_XO;
    if (gcp->mask & GCTileStipYOrigin)
	mask |= RC_GC_YO;
    if (gcp->mask & GCTile)
	mask |= RC_GC_TI;
    if ((mask & 0xFF00) != 0)
	mask |= RC_GC_B2;

    RWriteByte(mask, fp);
    if (mask & RC_GC_B2)
	RWriteByte(mask >> 8, fp);
    if (mask & RC_GC_FG)
	RWriteByte(pixel2index(rc, gcp->foreground), fp);
    if (mask & RC_GC_BG)
	RWriteByte(pixel2index(rc, gcp->background), fp);
    if (mask & RC_GC_LW)
	RWriteByte(gcp->line_width, fp);
    if (mask & RC_GC_LS)
	RWriteByte(gcp->line_style, fp);
    if (mask & RC_GC_DO)
	RWriteByte(gcp->dash_offset, fp);
    if (mask & RC_GC_FU)
	RWriteByte(gcp->function, fp);
    if (mask & RC_GC_DA) {
	RWriteByte(gcp->num_dashes, fp);
	for (i = 0; i < gcp->num_dashes; i++)
	    RWriteByte(gcp->dash_list[i], fp);
    }
    if (mask & RC_GC_FS)
	RWriteByte(gcp->fill_style, fp);
    if (mask & RC_GC_XO)
	RWriteLong(gcp->ts_x_origin, fp);
    if (mask & RC_GC_YO)
	RWriteLong(gcp->ts_y_origin, fp);
    if (mask & RC_GC_TI)
	RWriteTile(rc, gcp, fp);
}

static void WriteHeader(struct xprc *rc, FILE *fp)
{
    int				i;

    rewind(fp);

    /* First write out magic 4 letter word */
    putc('X', fp);
    putc('P', fp);
    putc('R', fp);
    putc('C', fp);

    /* Write which version of the XPilot Record Protocol this is. */
    putc(RC_MAJORVERSION, fp);
    putc('.', fp);
    putc(RC_MINORVERSION, fp);
    putc('\n', fp);

    /* Write player's nick, login, host, server, FPS and the date. */
    RWriteString(rc->nickname, fp);
    RWriteString(rc->realname, fp);
    RWriteString(rc->hostname, fp);
    RWriteString(rc->servername, fp);
    RWriteByte(rc->fps, fp);
    RWriteString(rc->recorddate, fp);

    /* Write info about graphics setup. */
    putc(rc->maxColors, fp);
    for (i = 0; i < rc->maxColors; i++) {
	RWriteULong((int)rc->colors[i].pixel, fp);
	RWriteUShort(rc->colors[i].red, fp);
	RWriteUShort(rc->colors[i].green, fp);
	RWriteUShort(rc->colors[i].blue, fp);
    }
    RWriteString(rc->gameFontName, fp);
    RWriteString(rc->msgFontName, fp);

    RWriteUShort(rc->view_width, fp);
    RWriteUShort(rc->view_height, fp);
}

static void WriteFrame(struct xprc *rc, struct frame *f, FILE *fp)
{
    /* drawShapes(save, pixmap, rc); */
    struct shape	*sp;
    int			i;

    putc(RC_NEWFRAME, fp);
    RWriteUShort((int)f->width, fp);
    RWriteUShort((int)f->height, fp);

    for (sp = f->shapes; sp != NULL; sp = sp->next) {

	switch(sp->type) {

	case RC_DRAWARC:
	    putc(RC_DRAWARC, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.arc.x, fp);
	    RWriteShort(sp->shape.arc.y, fp);
	    RWriteByte(sp->shape.arc.width, fp);
	    RWriteByte(sp->shape.arc.height, fp);
	    RWriteShort(sp->shape.arc.angle1, fp);
	    RWriteShort(sp->shape.arc.angle2, fp);
	    break;

	case RC_DRAWLINES:
	    putc(RC_DRAWLINES, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteUShort(sp->shape.lines.npoints, fp);
	    for (i = 0; i < sp->shape.lines.npoints; i++) {
		RWriteShort(sp->shape.lines.points[i].x, fp);
		RWriteShort(sp->shape.lines.points[i].y, fp);
	    }
	    RWriteByte(sp->shape.lines.mode, fp);
	    break;

	case RC_DRAWLINE:
	    putc(RC_DRAWLINE, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.line.x1, fp);
	    RWriteShort(sp->shape.line.y1, fp);
	    RWriteShort(sp->shape.line.x2, fp);
	    RWriteShort(sp->shape.line.y2, fp);
	    break;

	case RC_DRAWRECTANGLE:
	    putc(RC_DRAWRECTANGLE, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.rectangle.x, fp);
	    RWriteShort(sp->shape.rectangle.y, fp);
	    RWriteByte(sp->shape.rectangle.width, fp);
	    RWriteByte(sp->shape.rectangle.height, fp);
	    break;

	case RC_DRAWSTRING:
	    putc(RC_DRAWSTRING, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.string.x, fp);
	    RWriteShort(sp->shape.string.y, fp);
	    RWriteByte(sp->shape.string.font, fp);
	    RWriteUShort((int)sp->shape.string.length, fp);
	    for (i = 0; i < (int)sp->shape.string.length; i++)
		putc(sp->shape.string.string[i], fp);
	    break;

	case RC_FILLARC:
	    putc(RC_FILLARC, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.arc.x, fp);
	    RWriteShort(sp->shape.arc.y, fp);
	    RWriteByte(sp->shape.arc.width, fp);
	    RWriteByte(sp->shape.arc.height, fp);
	    RWriteShort(sp->shape.arc.angle1, fp);
	    RWriteShort(sp->shape.arc.angle2, fp);
	    break;

	case RC_FILLPOLYGON:
	    putc(RC_FILLPOLYGON, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteUShort(sp->shape.polygon.npoints, fp);
	    for (i = 0; i < sp->shape.polygon.npoints; i++) {
		RWriteShort(sp->shape.polygon.points[i].x, fp);
		RWriteShort(sp->shape.polygon.points[i].y, fp);
	    }
	    RWriteByte(sp->shape.polygon.shape, fp);
	    RWriteByte(sp->shape.polygon.mode, fp);
	    break;

	case RC_FILLRECTANGLE:
	    putc(RC_FILLRECTANGLE, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteShort(sp->shape.rectangle.x, fp);
	    RWriteShort(sp->shape.rectangle.y, fp);
	    RWriteByte(sp->shape.rectangle.width, fp);
	    RWriteByte(sp->shape.rectangle.height, fp);
	    break;

	case RC_PAINTITEMSYMBOL:
	    putc(RC_PAINTITEMSYMBOL, fp);
	    RWriteGC(rc, sp->gc, fp);
	    putc(sp->shape.symbol.type, fp);
	    RWriteShort(sp->shape.symbol.x, fp);
	    RWriteShort(sp->shape.symbol.y, fp);
	    break;

	case RC_FILLRECTANGLES:
	    putc(RC_FILLRECTANGLES, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteUShort(sp->shape.rectangles.nrectangles, fp);
	    for (i = 0; i < sp->shape.rectangles.nrectangles; i++) {
		RWriteShort(sp->shape.rectangles.rectangles[i].x, fp);
		RWriteShort(sp->shape.rectangles.rectangles[i].y, fp);
		RWriteByte(sp->shape.rectangles.rectangles[i].width, fp);
		RWriteByte(sp->shape.rectangles.rectangles[i].height, fp);
	    }
	    break;

	case RC_DRAWARCS:
	    putc(RC_DRAWARCS, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteUShort(sp->shape.arcs.narcs, fp);
	    for (i = 0; i < sp->shape.arcs.narcs; i++) {
		RWriteShort(sp->shape.arcs.arcs[i].x, fp);
		RWriteShort(sp->shape.arcs.arcs[i].y, fp);
		RWriteByte(sp->shape.arcs.arcs[i].width, fp);
		RWriteByte(sp->shape.arcs.arcs[i].height, fp);
		RWriteShort(sp->shape.arcs.arcs[i].angle1, fp);
		RWriteShort(sp->shape.arcs.arcs[i].angle2, fp);
	    }
	    break;

	case RC_DRAWSEGMENTS:
	    putc(RC_DRAWSEGMENTS, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteUShort(sp->shape.segments.nsegments, fp);
	    for (i = 0; i < sp->shape.segments.nsegments; i++) {
		RWriteShort(sp->shape.segments.segments[i].x1, fp);
		RWriteShort(sp->shape.segments.segments[i].y1, fp);
		RWriteShort(sp->shape.segments.segments[i].x2, fp);
		RWriteShort(sp->shape.segments.segments[i].y2, fp);
	    }
	    break;

	case RC_DAMAGED:
	    putc(RC_DAMAGED, fp);
	    RWriteGC(rc, sp->gc, fp);
	    RWriteByte(sp->shape.damage.damaged, fp);
	    break;

	default:
	    break;
	}
    }

    putc(RC_ENDFRAME, fp);
}

static void SaveFramesXPR(struct xprc *rc)
{
    struct frame	*begin = rc->save_first;
    struct frame	*end = rc->save_last;
    struct frame	*save;
    int			done = 0;
    FILE		*fp;
    tile_list_t		*tptr;
    char		buf[256];

    if (!begin)
	openErrorWindow(rc->ewin, "The first frame to save hasn't been set.");
    else if (!end)
	openErrorWindow(rc->ewin, "The last frame to save hasn't been set.");

    if (!begin || !end)
	return;

    if (begin->number > end->number) {
	openErrorWindow(rc->ewin, "First save frame exceeds last save frame, "
			"not saving");
	return;
    }
    if (!rc->seekable) {
	if (!begin->shapes) {
	    openErrorWindow(rc->ewin, "Save failed for standard input");
	    return;
	}
    }
    for (tptr = rc->tlist; tptr; tptr = tptr->next)
	tptr->flag = 0;

    if (!compress) {
	sprintf(buf, "xp%d-%d.xpr", begin->number, end->number);
	if (!(fp = fopen(buf, "w"))) {
	    perror(buf);
	    return;
	} else
	    setvbuf(fp, NULL, _IOFBF, (size_t)(8 * 1024));
    } else {
	sprintf(buf, "compress > xp%d-%d.xpr.Z", begin->number, end->number);
	if (!(fp = popen(buf, "w"))) {
	    perror(buf);
	    return;
	}
    }

    WriteHeader(rc, fp);

    for (save = begin; !done; save = save->next) {
	sprintf(buf, "Saving frame %d (of %d) ...\n",
		save->number - begin->number + 1,
		end->number - begin->number + 1);
	OverWriteMsg(rc, buf);
	if (!save->shapes)
	    readFrameData(rc, save);
	WriteFrame(rc, save, fp);

	done = (save == end);
    }

    fclose(fp);

    if (done) {
	sprintf(buf, "Saved %d frames OK.\n", end->number - begin->number + 1);
	OverWriteMsg(rc, buf);
    } else {
	sprintf(buf, "Saving failed!\n");
	OverWriteMsg(rc, buf);
    }
}

static void dox(struct xui *ui, struct xprc *rc)
{
    XEvent		event;
    int			count;
    KeySym		keysym;
    XComposeStatus	compose;
    char		c;
    struct timeval	tv0, tv1;

    screen_num = DefaultScreen(dpy);
    colormap = DefaultColormap(dpy, screen_num);

    Init_topview(rc);

    readNewFrame(rc);
    if (rc->cur == NULL) {
	fprintf(stderr, "No frames, nothing to do.\n");
	return;
    }

    XMapWindow(dpy, rc->topview);
    XFlush(dpy);

    Init_topmain(ui, rc);

    XMapWindow(dpy, ui->topmain);
    XFlush(dpy);

    rc->ewin = ui->ewin;

    gettimeofday(&tv0, NULL);

    for (;;) {

	while (XEventsQueued(dpy, QueuedAfterFlush) > 0
	       || (!currentSpeed && !forceRedraw && !frameStep)) {

	    XNextEvent(dpy, &event);

	    if (CheckButtonEvent(&event)) {
		if (quit)
		    return;
		continue;
	    }

	    switch(event.type) {

	    case ClientMessage:
		if (event.xclient.message_type == ProtocolAtom
		    && (unsigned)event.xclient.data.l[0] == KillAtom)
		    return;
		break;

	    case Expose:
		if (event.xexpose.count != 0)
		    break;
		if (event.xany.window == rc->topview)
		    forceRedraw = True;
		if (event.xany.window == ui->topmain)
		    redrawMain(ui, rc);
		if (event.xany.window == ui->ewin->win)
		    redrawError(ui->ewin);
		break;

	    case ButtonPress:
		if (event.xany.window == rc->topview) {
		    if (event.xbutton.button == 1)
			frameStep--;
		    else if (event.xbutton.button == 2)
			frameStep++;
		}
		break;

	    case KeyRelease:
		break;

	    case KeyPress:
		count = XLookupString(&(event.xkey), &c, 1,
				      &keysym, &compose);
		if (count == NoSymbol)
		    break;

		switch(c) {

		case ' ':
		    switch(playState) {
		    case STATE_PLAYING:
			currentSpeed = 0;
			playState = STATE_PAUSED;
			forceRedraw = True;
			break;
		    case STATE_PAUSED:
			currentSpeed = 1;
			playState = STATE_PLAYING;
			break;
		    default:
			break;
		    }

		    break;

		case 'f':
		case 'F':
		    frameStep++;
		    break;

		case 'b':
		case 'B':
		case '\b':
		case '\177':
		    frameStep--;
		    break;

		case 'z':
		case 'Z':
		    frameStep = -rc->cur->number;
		    break;

		    /*
		     * kps - press a number key to fast forward that many
		     * minutes.
		     */
		case '1':
		    frameStep += rc->fps * 60;
		    break;
		case '2':
		    frameStep += rc->fps * 60 * 2;
		    break;
		case '3':
		    frameStep += rc->fps * 60 * 3;
		    break;
		case '4':
		    frameStep += rc->fps * 60 * 4;
		    break;
		case '5':
		    frameStep += rc->fps * 60 * 5;
		    break;
		case '6':
		    frameStep += rc->fps * 60 * 6;
		    break;
		case '7':
		    frameStep += rc->fps * 60 * 7;
		    break;
		case '8':
		    frameStep += rc->fps * 60 * 8;
		    break;
		case '9':
		    frameStep += rc->fps * 60 * 9;
		    break;
		case '0':
		    frameStep += rc->fps * 60 * 10;
		    break;


		case '[':
		    rc->save_first = rc->cur;
		    break;
		case ']':
		    rc->save_last = rc->cur;
		    break;
		case '*':
		    SaveFramesPPM(rc);
		    break;
		case '&':
		    SaveFramesXPR(rc);
		    break;

		case 'd':
		case 'D':
		    MemPrint();
		    break;

		case 'q':
		case 'Q':
		    XFreeGC(dpy, rc->gc);
		    return;
		default:
		    break;
		}

	    default:
		break;
	    }

	}

	/*
	 * for "-loop" option, act like z was pressed
	 */
	if (rc->eof == True && loopAtEnd) {
	    rc->eof = False;
	    frameStep = -rc->cur->number;
	}

	gettimeofday(&tv1, NULL);
	if (frameStep != 0)
	    tv0 = tv1;
	else if (!forceRedraw && currentSpeed != 0) {
	    long	delta_sec = tv1.tv_sec - tv0.tv_sec;
	    long	delta_usec = (long)tv1.tv_usec - (long)tv0.tv_usec;
	    long	delta_time = 1000000L * delta_sec + delta_usec;
	    long	frame_rate = 1000000L / rc->fps;

	    if (delta_time + 1000 < frame_rate) {
		fd_set	rset;
		int	rfd = ConnectionNumber(dpy);
		int	num;

		tv1.tv_sec = 0;
		tv1.tv_usec = frame_rate - delta_time;
		FD_ZERO(&rset);
		FD_SET(rfd, &rset);
		num = select(rfd + 1, &rset, NULL, NULL, &tv1);
		if (num == 1)
		    continue;
		tv0.tv_usec = tv0.tv_usec + frame_rate;
		if (tv0.tv_usec >= 1000000) {
		    tv0.tv_usec -= 1000000;
		    tv0.tv_sec++;
		}
	    }
	    else
		tv0 = tv1;
	    frameStep += currentSpeed;
	}

	while (frameStep > 0) {
	    if (rc->cur->next == NULL) {
		if (rc->eof == False)
		    readNewFrame(rc);
	    }
	    if (rc->cur->next != NULL) {
		rc->cur = rc->cur->next;
		forceRedraw = True;
		frameStep--;
	    }
	    else
		frameStep = 0;
	}
	while (frameStep < 0) {
	    if (rc->cur->prev != NULL) {
		if (!rc->seekable && rc->cur->prev->shapes == NULL) {
		    static int before;
		    if (!before++)
			openErrorWindow(rc->ewin,
					"Can't go backwards any further, "
					"because input is not a regular "
					"file.");
		    frameStep = 0;
		}
		else {
		    rc->cur = rc->cur->prev;
		    forceRedraw = True;
		    frameStep++;
		}
	    } else
		frameStep = 0;
	}

	if (forceRedraw == True) {
	    redrawWindow(rc);
	    redrawLabel(ui, rc, &(ui->labels[LABEL_POSITION]));
	}
    }
}

static void TestInput(struct xprc *rc)
{
    int			fd = fileno(rc->fp);
    struct stat		st;
    unsigned char	ch0, ch1, ch2;
    char		buf[1024];

    rc->seekable = False;
    if (fstat(fd, &st)) {
	perror("Can't stat input");
	return;
    }
    rc->seekable = S_ISREG(st.st_mode);
    if (rc->seekable) {
	ch0 = getc(rc->fp);
	ch1 = getc(rc->fp);
	ch2 = getc(rc->fp);
	rewind(rc->fp);
	if (ch0 == 0x1F && ch1 == 0x9D) {
	    if (verbose) {
		fprintf(stderr, "%s: \"%s\" is in compressed format, "
			"starting compress...\n", *Argv, rc->filename);
	    }
	    lseek(fd, 0L, SEEK_SET);
	    if (rc->fp == stdin)
		sprintf(buf, "compress -d");
	    else {
		fclose(rc->fp);
		sprintf(buf, "compress -d < %s", rc->filename);
	    }
	    if ((rc->fp = popen(buf, "r")) == NULL) {
		perror("Unable to start compress");
		exit(1);
	    }
	    rc->seekable = 0;
	}
	if (ch0 == 0x1F && ch1 == 0x8B) {
	    if (verbose)
		fprintf(stderr,
			"%s: \"%s\" is in gzip format, starting gzip...\n",
			*Argv, rc->filename);
	    lseek(fd, 0L, SEEK_SET);
	    if (rc->fp == stdin)
		sprintf(buf, "gzip -d");
	    else {
		fclose(rc->fp);
		sprintf(buf, "gzip -d < %s", rc->filename);
	    }
	    if ((rc->fp = popen(buf, "r")) == NULL) {
		perror("Unable to start gzip");
		exit(1);
	    }
	    rc->seekable = 0;
	}
	if (ch0 == 'B' && ch1 == 'Z' && ch2 == 'h') {
	    if (verbose) {
		fprintf(stderr,
			"%s: \"%s\" is in bzip2 format, starting bzip2...\n",
			*Argv, rc->filename);
	    }
	    lseek(fd, 0L, SEEK_SET);
	    if (rc->fp == stdin)
		sprintf(buf, "bzip2 -d");
	    else {
		fclose(rc->fp);
		sprintf(buf, "bzip2 -d < %s", rc->filename);
	    }
	    if ((rc->fp = popen(buf, "r")) == NULL) {
		perror("Unable to start bzip2");
		exit(1);
	    }
	    rc->seekable = 0;
	}
    }
    if (!rc->seekable) {
	if (verbose)
	    fprintf(stderr,
		    "Input is not a regular file, this may result\n"
		    "in limited reverse playback functionality.\n");
    } else {
	if (max_mem > 1 * 1024 * 1024)
	    max_mem = 1 * 1024 * 1024;
    }
}

static void usage(void)
{
    printf("Usage: %s [options] filename\n", *Argv);
    printf(
"    If filename is a dash - then standard input is used.\n"
"    Valid options are:\n"
"        -scale \"factor\"\n"
"               Set the scale reduction factor for saving operations.\n"
"               Valid scale factors are in the range [0.01 - 1.0].\n"
"        -linewidth \"width\"\n"
"               use a fixed linewidth \"width\" for drawing all lines\n"
"        -gamma \"factor\"\n"
"               Set the gamma correction factor when saving scaled frames.\n"
"               Valid gamma correction factors are in the range [0.1 - 10].\n"
"        -compress\n"
"               Save frames compressed using the \"compress\" program.\n"
"        -fps \"value\", -FPS \"value\"\n"
"               Set the number of frames per second used for replay and\n"
"               recording.\n"
"        -play\n"
"               Start playing immediately.\n"
"        -loop\n"
"               Loop after playing.\n"
"        -debug\n"
"        -verbose\n"
"        -help\n"
"        -version\n"
"    In addition to the pushbuttons you can use the following keys:\n"
"        f  -  move forwards to the next frame.\n"
"        b  -  move backwards to the next frame.\n"
"        z  -  move backwards to the first frame.\n"
"        [  -  mark the current frame as the first frame to be saved.\n"
"        ]  -  mark the current frame as the last frame to be saved.\n"
"        *  -  save the marked frames in PPM format.\n"
"              WARNING: saving many frames takes HUGE amounts of diskspace!\n"
"        &  -  save the marked frames in XPilot Recording format.\n"
"        q  -  quit the program.\n"
    );
    exit(0);
}

static void version(void)
{
    printf("xpilot-ng-replay %s\n", VERSION);
    exit(0);
}

int main(int argc, char **argv)
{
    FILE		*fp;
    int			argi;
    char		*filename;
    struct xprc		*rc;
    struct xui		*ui;
    int			fps = 0;
    double		scale = 0;
    double		gamma_val = 0;
    int 		linewidth = 0;

    Argc = argc;
    Argv = argv;

    for (argi = 1; argi < argc; argi++) {
	if (argv[argi][0] != '-' || argv[argi][1] == '\0')
	    break;
	else if (!strcmp(argv[argi], "-debug")) {
	    debug = 1;
	    verbose = 1;
	}
	else if (!strcmp(argv[argi], "-verbose"))
	    verbose = 1;
	else if (!strcmp(argv[argi], "-compress"))
	    compress = 1;
	else if (!strcmp(argv[argi], "-fps") || !strcmp(argv[argi], "-FPS")) {
	    if (++argi == argc || sscanf(argv[argi], "%d", &fps) != 1)
		usage();
	    if (fps < 0)
		usage();
	}
	else if (!strcmp(argv[argi], "-scale")) {
	    if (++argi == argc || sscanf(argv[argi], "%lf", &scale) != 1)
		usage();
	    if (scale < 0.01 || scale > 1.0)
		usage();
	    if (scale >= 1.0)
		scale = 0;
	}
	else if (!strcmp(argv[argi], "-gamma")) {
	    if (++argi == argc || sscanf(argv[argi], "%lf", &gamma_val) != 1)
		usage();
	    if (gamma_val < 0.1 || gamma_val > 100)
		usage();
	    if (gamma_val == 1.0)
		gamma_val = 0;
	}
	else if (!strcmp(argv[argi], "-linewidth")) {
	    if (++argi == argc || sscanf(argv[argi], "%d", &linewidth) != 1)
		usage();
	    if (linewidth < 1 || gamma_val > 100)
		usage();
	}
	else if (!strcmp(argv[argi], "-play"))
	    currentSpeed = 1;
	else if (!strcmp(argv[argi], "-loop"))
	    loopAtEnd = 1;
	else if (!strcmp(argv[argi], "-version") ||
		 !strcmp(argv[argi], "--version"))
	    version();
	else {
	    if (!strncmp(argv[argi], "-h", 2) ||
		!strncmp(argv[argi], "--h", 3) ||
		!strcmp(argv[argi], "-?"))
		usage();
	    else if (argi < argc - 1) {
		fprintf(stderr, "%s: Unknown option \"%s\"\n",
			Argv[0], argv[argi]);
		fprintf(stderr, "\tType: \"%s -help\" for some help.\n",
			Argv[0]);
		exit(2);
	    }
	    else
		break;
	}
    }
    if (argi != argc - 1)
	usage();
    filename = argv[argc - 1];
    if (!strcmp(filename, "-"))
	fp = stdin;
    else {
	if ((fp = fopen(filename, "r")) == NULL) {
	    perror("Unable to open record file");
	    fprintf(stderr, "Type: \"%s -help\" to get some help.\n", *argv);
	    return 1;
	}
    }

    if ((dpy = XOpenDisplay(NULL)) == NULL) {
	fprintf(stderr, "Cannot connect to X server %s\n", XDisplayName(NULL));
	exit(1);
    }

    ui = (struct xui *)MyMalloc(sizeof(*ui), MEM_UI);
    memset(ui, 0, sizeof(*ui));

    rc = (struct xprc *)MyMalloc(sizeof(*rc), MEM_MISC);
    memset(rc, 0, sizeof(*rc));
    rc->filename = filename;
    rc->fp = fp;
    rc->fps = fps;
    rc->scale = scale;
    rc->gamma = gamma_val;
    rc->linewidth = linewidth;
    TestInput(rc);
    purge_argument = rc;
    if (RReadHeader(rc) >= 0) {
	dox(ui, rc);
	FreeXPRCData(rc);
    }
    fp = rc->fp;

    MyFree(rc, sizeof(struct xprc), MEM_MISC);
    MyFree(ui, sizeof(struct xui), MEM_UI);
    XCloseDisplay(dpy);

    if (fp != NULL && fp != stdin)
	fclose(fp);

    MemPrint();

    return 0;
}

/* ARGSUSED */
static void quitCallback(void *data)
{
    (void)data;
    quit = 1;
}

static void stopCallback(void *data)
{
    struct xui *ui = (struct xui *) data;

    playState = STATE_PLAYING;
    currentSpeed = 0;
    forceRedraw = True;
    NonreleaseableButton(ui->buttons[BUTTON_REWIND]);
    NonreleaseableButton(ui->buttons[BUTTON_REVERSE_PLAY]);
    NonreleaseableButton(ui->buttons[BUTTON_PLAY]);
    NonreleaseableButton(ui->buttons[BUTTON_FAST_FORWARD]);
    ChangeButtonGroup(ui->buttons[BUTTON_REWIND], 1);
    ChangeButtonGroup(ui->buttons[BUTTON_REVERSE_PLAY], 1);
    ChangeButtonGroup(ui->buttons[BUTTON_PLAY], 1);
    ChangeButtonGroup(ui->buttons[BUTTON_FAST_FORWARD], 1);
}

static void pauseCallback(void *data)
{
    struct xui *ui = (struct xui *) data;

    playState = STATE_PAUSED;
    currentSpeed = 0;
    forceRedraw = True;
    ReleaseableButton(ui->buttons[BUTTON_REWIND]);
    ReleaseableButton(ui->buttons[BUTTON_REVERSE_PLAY]);
    ReleaseableButton(ui->buttons[BUTTON_PLAY]);
    ReleaseableButton(ui->buttons[BUTTON_FAST_FORWARD]);
    ChangeButtonGroup(ui->buttons[BUTTON_REWIND], 0);
    ChangeButtonGroup(ui->buttons[BUTTON_REVERSE_PLAY], 0);
    ChangeButtonGroup(ui->buttons[BUTTON_PLAY], 0);
    ChangeButtonGroup(ui->buttons[BUTTON_FAST_FORWARD], 0);
}

/* ARGSUSED */
static void rewindCallback(void *data)
{
    (void)data;
    switch(playState)
    {
    case STATE_PLAYING:
	currentSpeed = -10;
	break;
    case STATE_PAUSED:
	frameStep -= 10;
	break;
    default:
	break;
    }
}

/* ARGSUSED */
static void fastfCallback(void *data)
{
    (void)data;
    switch(playState)
    {
    case STATE_PLAYING:
	currentSpeed = 10;
	break;
    case STATE_PAUSED:
	frameStep += 10;
	break;
    default:
	break;
    }
}

/* ARGSUSED */
static void playCallback(void *data)
{
    (void)data;
    switch(playState)
    {
    case STATE_PLAYING:
	currentSpeed = 1;
	break;
    case STATE_PAUSED:
	frameStep++;
	break;
    default:
	break;
    }
}

/* ARGSUSED */
static void revplayCallback(void *data)
{
    (void)data;
    switch(playState)
    {
    case STATE_PLAYING:
	currentSpeed = -1;
	break;
    case STATE_PAUSED:
	frameStep--;
	break;
    default:
	break;
    }
}

static void recordCallback(void *data)
{
    struct xui *ui = (struct xui *) data;

    XMapWindow(dpy, ui->rwin->win);
}

static void markSaveStart(void *data)
{
    struct xprc		*rc = (struct xprc *) data;

    rc->save_first = rc->cur;
}

static void markSaveEnd(void *data)
{
    struct xprc		*rc = (struct xprc *) data;

    rc->save_last = rc->cur;
}

static void saveStartToEndPPM(void *data)
{
    struct xprc		*rc = (struct xprc *) data;

    SaveFramesPPM(rc);
}

static void saveStartToEndXPR(void *data)
{
    struct xprc		*rc = (struct xprc *) data;

    SaveFramesXPR(rc);
}
