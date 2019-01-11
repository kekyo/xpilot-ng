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

/***************************************************************************\
*  winX.h - X11 to Windoze converter										*
*																			*
*  This file is the public interface to the Winodoze -> X11 translator.		*
*  Any function that has a unix man page belongs in this file.				*
*																			*
*  							*
\***************************************************************************/
#ifndef	_WINX_H_
#define	_WINX_H_

#ifdef	_WINDOWS

#ifndef	_INC_WINDOWS
#include <windows.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

    typedef unsigned long XID;
    typedef unsigned long Atom;
    typedef int GC;

    typedef XID Drawable;
    typedef XID Pixmap;
    typedef XID Window;
    typedef XID Cursor;
    typedef XID KeySym;
    typedef XID GContext;
    typedef XID Font;
    typedef XID Colormap;

    typedef int Bool;
    typedef int Status;
    typedef char *XPointer;

#define	None				0L
#define	False				0
#define	True				1

#define	Button1				1
#define	Button2				2
#define	Button3				3

#define	AllPlanes	((unsigned long)~0L)

#define QueuedAlready 0
#define QueuedAfterReading 1
#define QueuedAfterFlush 2

    struct XFontStruct {
	// char*        Face;
	int ascent;
	int descent;
	LOGFONT lf;
	CHOOSEFONT cf;
	HFONT hFont;
	Font fid;		/* which one of our object array */
    };
    typedef struct XFontStruct XFontStruct;

    struct Display {
	int unused;
    };
    typedef struct Display Display;

    struct Visual {
	int u;
    };
    typedef struct Visual Visual;

/***************\
* Event Mapping *
\***************/
#define	KeyPress			2
#define	KeyRelease			3
#define	ButtonPress			4
#define ButtonRelease		5
#define MotionNotify        6
#define	EnterNotify			7
#define	LeaveNotify			8
#define	Expose				12
#define	ConfigureNotify		22

#define NoEventMask				0L
#define	KeyPressMask			(1L<<0)
#define ButtonPressMask         (1L<<2)
#define ButtonReleaseMask       (1L<<3)
#define EnterWindowMask         (1L<<4)
#define LeaveWindowMask         (1L<<5)
#define	PointerMotionMask		(1L<<6)
#define ButtonMotionMask        (1L<<13)
#define ExposureMask            (1L<<15)
#define StructureNotifyMask     (1L<<17)
#define OwnerGrabButtonMask     (1L<<24)

    struct XKeyEvent {
	int type;		/* of event */
	Window window;
	unsigned int keycode;
	unsigned int ascii;
	int x, y;		/* pointer x, y coordinates in event window */
    };
    typedef struct XKeyEvent XKeyEvent;

    typedef struct {
	int type;		/* of event */
	Window window;		/* "event" window it is reported relative to */
	Bool send_event;	/* true if this came from a SendEvent request */
//    unsigned long serial;   /* # of last request processed by server */
//    Display *display;   /* Display the event was read from */
//    Window root;            /* root window that the event occured on */
//    Window subwindow;   /* child window */
//    /* Time time;      /* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
//    unsigned int state; /* key or button mask */
	unsigned int button;	/* detail */
	//   Bool same_screen;   /* same screen flag */
    } XButtonEvent;

    typedef struct {
	int type;		/* of event */
	Window window;		/* "event" window reported relative to */
	Bool send_event;	/* true if this came from a SendEvent request */
//    unsigned long serial;   /* # of last request processed by server */
//    Display *display;   /* Display the event was read from */
//    Window root;            /* root window that the event occured on */
//    Window subwindow;   /* child window */
//    Time time;      /* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
//    unsigned int state; /* key or button mask */
//    char is_hint;    /* detail */
	//   Bool same_screen;   /* same screen flag */
    } XMotionEvent;

    typedef struct {
	int type;
	Window window;
	Bool send_event;	/* true if this came from a SendEvent request */
//    unsigned long serial;   /* # of last request processed by server */
//    Display *display;   /* Display the event was read from */
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
    } XExposeEvent;

    typedef struct {
	int type;
	Window window;		/* window on which event was requested in event mask */
	Bool send_event;	/* true if this came from a SendEvent request */
//    unsigned long serial;   /* # of last request processed by server */
//    Display *display;/* Display the event was read from */
    } XAnyEvent;

    typedef struct {
	int type;
	Window window;
	Bool send_event;	/* true if this came from a SendEvent request */
//    unsigned long serial;   /* # of last request processed by server */
//    Display *display;   /* Display the event was read from */
//    int x, y;
	int width, height;
//    int border_width;
//    Window above;
//    Bool override_redirect;
    } XConfigureEvent;

    typedef union _XEvent {
	int type;
	XAnyEvent xany;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XExposeEvent xexpose;
	XConfigureEvent xconfigure;
    } XEvent;

/**************************\
* real live X11 structures *
\**************************/
    struct XSegment {
	short x1, y1, x2, y2;
    };
    typedef struct XSegment XSegment;

    struct XPoint {
//    short x, y;
	int x, y;
    };
    typedef struct XPoint XPoint;

// typedef      struct _SMALL_RECT XRectangle;
    struct XRectangle {
	short x, y;
	unsigned short width, height;	// The X way
//      short cx, cy;                                   // The Windoze way
    };
    typedef struct XRectangle XRectangle;

    struct XArc {
	short x, y;
	unsigned short width, height;
	short angle1, angle2;
    };
    typedef struct XArc XArc;

    struct XColor {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;		/* do_red, do_green, do_blue */
	char pad;
    };
    typedef struct XColor XColor;

/*
 * Data structure for setting graphics context.
 */

#define	GXcopy				0x3
#define	GXxor				0x6

/* LineStyle */

#define LineSolid			0
#define LineOnOffDash       1
#define LineDoubleDash      2

/* capStyle */

#define CapNotLast			0
#define CapButt				1
#define CapRound			2
#define CapProjecting       3

/* joinStyle */

#define JoinMiter			0
#define JoinRound			1
#define JoinBevel			2


/* fillStyle */
#define FillSolid			0
#define FillTiled			1
#define FillStippled		2
#define FillOpaqueStippled	3

/* Polygon shapes */
#define Complex			0	/* paths may intersect */
#define Nonconvex		1	/* no paths intersect, but not convex */
#define Convex			2	/* wholly convex */


#define GCFunction              (1L<<0)
#define GCPlaneMask             (1L<<1)
#define GCForeground            (1L<<2)
#define GCBackground            (1L<<3)
#define GCLineWidth             (1L<<4)
#define GCLineStyle             (1L<<5)
#define GCCapStyle              (1L<<6)
#define GCJoinStyle             (1L<<7)
#define GCFillStyle             (1L<<8)
#define GCFillRule              (1L<<9)
#define GCTile                  (1L<<10)
#define GCStipple               (1L<<11)
#define GCTileStipXOrigin       (1L<<12)
#define GCTileStipYOrigin       (1L<<13)
#define GCFont                  (1L<<14)
#define GCSubwindowMode         (1L<<15)
#define GCGraphicsExposures     (1L<<16)
#define GCClipXOrigin           (1L<<17)
#define GCClipYOrigin           (1L<<18)
#define GCClipMask              (1L<<19)
#define GCDashOffset            (1L<<20)
#define GCDashList              (1L<<21)
#define GCArcMode               (1L<<22)

/* CoordinateMode for drawing routines */

#define CoordModeOrigin     0	/* relative to the origin */
#define CoordModePrevious       1	/* relative to previous point */


    typedef struct {
	int function;		/* logical operation */
	unsigned long plane_mask;	/* plane mask */
	unsigned long foreground;	/* foreground pixel */
	unsigned long background;	/* background pixel */
	int line_width;		/* line width */
	int line_style;		/* LineSolid, LineOnOffDash, LineDoubleDash */
	int cap_style;		/* CapNotLast, CapButt, 
				   CapRound, CapProjecting */
	int join_style;		/* JoinMiter, JoinRound, JoinBevel */
	int fill_style;		/* FillSolid, FillTiled, 
				   FillStippled, FillOpaeueStippled */
	int fill_rule;		/* EvenOddRule, WindingRule */
	int arc_mode;		/* ArcChord, ArcPieSlice */
	Pixmap tile;		/* tile pixmap for tiling operations */
	Pixmap stipple;		/* stipple 1 plane pixmap for stipping */
	int ts_x_origin;	/* offset for tile or stipple operations */
	int ts_y_origin;
	Font font;		/* default text font for text operations */
	int subwindow_mode;	/* ClipByChildren, IncludeInferiors */
	Bool graphics_exposures;	/* boolean, should exposures be generated */
	int clip_x_origin;	/* origin for clipping */
	int clip_y_origin;
	Pixmap clip_mask;	/* bitmap clipping; other calls for rects */
	int dash_offset;	/* patterned/dashed line information */
	char dashes;
    } XGCValues;

/*
 * Data structure for setting window attributes.
 */
#define	Always				2

    typedef struct {
	Pixmap background_pixmap;	/* background or None or ParentRelative */
	unsigned long background_pixel;	/* background pixel */
	Pixmap border_pixmap;	/* border of the window */
	unsigned long border_pixel;	/* border pixel value */
	int bit_gravity;	/* one of bit gravity values */
	int win_gravity;	/* one of the window gravity values */
	int backing_store;	/* NotUseful, WhenMapped, Always */
	unsigned long backing_planes;	/* planes to be preseved if possible */
	unsigned long backing_pixel;	/* value to use in restoring planes */
	Bool save_under;	/* should bits under be saved? (popups) */
	long event_mask;	/* set of events that should be saved */
	long do_not_propagate_mask;	/* set of events that should not propagate */
	Bool override_redirect;	/* boolean value for override-redirect */
	Colormap colormap;	/* color map to be associated with window */
	Cursor cursor;		/* cursor to be displayed (or None) */
    } XSetWindowAttributes;

    typedef struct {
	int x, y;		/* location of window */
	int width, height;	/* width and height of window */
	int border_width;	/* border width of window */
	int depth;		/* depth of window */
	Visual *visual;		/* the associated visual structure */
	Window root;		/* root of screen containing window */
	int c_class;		/* InputOutput, InputOnly */
	int bit_gravity;	/* one of bit gravity values */
	int win_gravity;	/* one of the window gravity values */
	int backing_store;	/* NotUseful, WhenMapped, Always */
	unsigned long backing_planes;	/* planes to be preserved if possible */
	unsigned long backing_pixel;	/* value to be used when restoring planes */
	Bool save_under;	/* boolean, should bits under be saved? */
	Colormap colormap;	/* color map to be associated with window */
	Bool map_installed;	/* boolean, is color map currently installed */
	int map_state;		/* IsUnmapped, IsUnviewable, IsViewable */
	long all_event_masks;	/* set of events all people have interest in */
	long your_event_mask;	/* my event mask */
	long do_not_propagate_mask;	/* set of events that should not propagate */
	Bool override_redirect;	/* boolean value for override-redirect */
/*    Screen *screen;  /* back pointer to correct screen */
    } XWindowAttributes;

/* Window attributes for CreateWindow and ChangeWindowAttributes */
#define CWBackPixel			(1L<<1)
#define CWBorderPixel		(1L<<3)
#define CWBitGravity		(1L<<4)
#define CWWinGravity		(1L<<5)
#define CWBackingStore		(1L<<6)
#define CWOverrideRedirect  (1L<<9)
#define CWSaveUnder			(1L<<10)
#define CWEventMask			(1L<<11)
#define CWColormap			(1L<<13)

/* Bit Gravity */

#define ForgetGravity       0
#define NorthWestGravity    1
#define NorthGravity        2
#define NorthEastGravity    3
#define WestGravity			4
#define CenterGravity       5
#define EastGravity			6
#define SouthWestGravity    7
#define SouthGravity        8
#define SouthEastGravity    9
#define StaticGravity       10

#define InputOutput     1
#define InputOnly       2

#if 0
/*
 * new structure for manipulating TEXT properties; used with WM_NAME, 
 * WM_ICON_NAME, WM_CLIENT_MACHINE, and WM_COMMAND.
 */
    typedef struct {
	int unused;
//    unsigned char *value;       /* same as Property routines */
//    Atom encoding;          /* prop type */
//    int format;             /* prop data format: 8, 16, or 32 */
//    unsigned long nitems;       /* number of data items in value */
    } XTextProperty;
#endif

/*
 * Visual structure; contains information about colormapping possible.
 */
#if 0
    typedef struct {
	/* XExtData *ext_data; /* hook for extension to hang data */
	/* VisualID visualid;  /* visual id of this visual */
#if defined(__cplusplus) || defined(c_plusplus)
	int c_class;		/* C++ class of screen (monochrome, etc.) */
#else
	int class;		/* class of screen (monochrome, etc.) */
#endif
	unsigned long red_mask, green_mask, blue_mask;	/* mask values */
	int bits_per_rgb;	/* log base 2 of distinct color values */
	int map_entries;	/* color map entries */
    } Visual;
#endif

    typedef int XrmDatabase;
#define	XrmInitialize	;

    extern int WinXGetWindowRectangle(Window window, XRectangle * rect);
	extern void WinXParseGeometry(const char* g, int *w, int *h);

#ifdef	_DEBUG
#define	XCreateSimpleWindow(__d, __p, __x, __y, __w, __h, __bw, __b, __bk) \
	XCreateSimpleWindow_(__d, __p, __x, __y, __w, __h, __bw, __b, __bk, __FILE__, __LINE__)
#define	XCreateWindow(__d, __p, __x, __y, __w, __h, __bw, __dt, __cc, __v, __vm, __a) \
	XCreateWindow_(__d, __p, __x, __y, __w, __h, __bw, __dt, __cc, __v, __vm, __a, __FILE__, __LINE__)
#define	WinXCreateWinDC(__w) \
	WinXCreateWinDC_(__w, __FILE__, __LINE__)

    extern Window XCreateSimpleWindow_(Display * dpy, Window parent, int x,
				       int y, unsigned int width,
				       unsigned int height,
				       unsigned int border_width,
				       unsigned long border,
				       unsigned long background,
				       const char *file, const int line);
    extern Window XCreateWindow_(Display * dpy, Window parent, int x,
				 int y, unsigned int width,
				 unsigned int height,
				 unsigned int border_width, int depth,
				 unsigned int c_class, Visual * visual,
				 unsigned long valuemask,
				 XSetWindowAttributes * attributes,
				 const char *file, const int line);
    extern WinXCreateWinDC_(Window w, const char *file, const int line);
#else
#define	XCreateSimpleWindow	XCreateSimpleWindow_
#define	XCreateWindow		XCreateWindow_
#define	WinXCreateWinDC(__w) \
	WinXCreateWinDC_(__w)
    extern Window XCreateSimpleWindow_(Display * dpy, Window parent, int x,
				       int y, unsigned int width,
				       unsigned int height,
				       unsigned int border_width,
				       unsigned long border,
				       unsigned long background);
    extern Window XCreateWindow_(Display * dpy, Window parent, int x,
				 int y, unsigned int width,
				 unsigned int height,
				 unsigned int border_width, int depth,
				 unsigned int c_class, Visual * visual,
				 unsigned long valuemask,
				 XSetWindowAttributes * attributes);
    extern WinXCreateWinDC_(Window w);
#endif
    extern XDestroyWindow(Display * dpy, Window w);
    extern XUnmapWindow(Display * dpy, Window w);
    extern XMapWindow(Display * dpy, Window w);
    extern XMapSubwindows(Display * dpy, Window w);
    extern XMoveWindow(Display * dpy, Window w, int x, int y);
    extern Window DefaultRootWindow(Display * dpy);
    extern int DisplayWidth(Display * dpy, int screen_number);
    extern int DisplayHeight(Display * dpy, int screen_number);
    extern Bool XCheckIfEvent(Display * dpy, XEvent * event_return,
			      Bool(*predicate) (), XPointer arg);
    extern XNextEvent(Display * dpy, XEvent * event_return);
    extern XTextWidth(XFontStruct * font, const char *string, int lenght);
    extern XSetForeground(Display * dpy, GC gc, unsigned long foreground);
    extern XMapRaised(Display * dpy, Window w);
    extern XDrawRectangle(Display * dpy, Drawable d, GC gc, int x, int y,
			  unsigned int w, unsigned int h);
    extern XFillRectangle(Display * dpy, Drawable d, GC gc, int x, int y,
			  unsigned int w, unsigned int h);
    extern XFillRectangles(Display * dpy, Drawable d, GC gc,
			   XRectangle * rects, int nrectangles);
    extern XChangeGC(Display * dpy, GC gc, unsigned long valuemask,
		     XGCValues * values);
    extern int XGetGCValues(Display * display, GC gc,
			    unsigned long valuemask,
			    XGCValues * values_return);
    extern XSetLineAttributes(Display * dpy, GC gc,
			      unsigned int line_width, int line_style,
			      int cap_style, int join_style);
    extern XCopyArea(Display * dpy, Drawable src, Drawable dest, GC gc,
		     int src_x, int src_y, unsigned int width,
		     unsigned int height, int dest_x, int dest_y);
    extern XDrawLine(Display * dpy, Drawable d, GC gc, int x1, int x2,
		     int y1, int y2);
    extern XDrawLines(Display * dpy, Drawable d, GC gc, XPoint * points,
		      int npoints, int mode);
    extern XSetTile(Display * dpy, GC gc, Pixmap tile);
    extern XSetTSOrigin(Display * dpy, GC gc, int ts_x_origin,
			int ts_y_origin);
    extern XSetFillStyle(Display * dpy, GC gc, int fill_style);
    extern XSetFunction(Display * dpy, GC gc, int function);
    extern XBell(Display * dpy, int percent);
    extern XFlush(Display * dpy);
    extern XCreatePixmap(Display * dpy, Drawable d,
			 unsigned int width, unsigned int height,
			 unsigned int depth);
    extern XFreePixmap(Display * dpy, Pixmap pixmap);
    extern XSetPlaneMask(Display * dpy, GC gc, unsigned long plane_mask);
    extern XClearWindow(Display * dpy, Window w);
    extern XDrawSegments(Display * dpy, Drawable d, GC gc,
			 XSegment * segments, int nsegments);
    extern XDrawPoint(Display * dpy, Drawable d, GC gc, int x, int y);
    extern XDrawPoints(Display * dpy, Drawable d, GC gc,
		       XPoint * points, int npoints, int mode);
    extern XDrawString(Display * dpy, Drawable d, GC gc, int x, int y,
		       const char *string, int length);
    extern XStoreName(Display * dpy, Window w, const char *window_name);
    extern XSetIconName(Display * dpy, Window w, const char *icon_name);
    extern XSetTransientForHint(Display * dpy, Window w,
				Window prop_window);
    extern XDrawArc(Display * dpy, Drawable d, GC gc, int x, int y,
		    unsigned int width, unsigned int height, int angle1,
		    int angle2);
    extern XFillArc(Display * dpy, Drawable d, GC gc, int x, int y,
		    unsigned int width, unsigned int height, int angle1,
		    int angle2);
    extern XDrawArcs(Display * dpy, Drawable d, GC gc, XArc * arcs,
		     int narcs);

    extern XFillPolygon(Display * dpy, Drawable d, GC gc, XPoint * points,
			int npoints, int shape, int mode);
    extern XSetDashes(Display * dpy, GC gc, int dash_offset,
		      const char dash_list[], int n);
    extern XChangeWindowAttributes(Display * dpy, Window w,
				   unsigned long valuemask,
				   XSetWindowAttributes * attributes);
    extern XSetWindowBackground(Display * dpy, Window w,
				unsigned long background_pixel);
    extern XGetWindowAttributes(Display * dpy, Window w,
				XWindowAttributes * attributes);
#define	NoSymbol	0L
    extern XFontStruct *XQueryFont(Display * dpy, XID font_ID);
	extern XFreeFontInfo(char **names, XFontStruct *free_info, int count);
    extern XFontStruct *WinXLoadFont(const char *name);
    extern XSetFont(Display * dpy, GC gc, Font font);
    extern GContext XGContextFromGC(GC gc);

    extern XParseColor(Display * display, Colormap colormap, char *spec,
		       XColor * exact_def_return);
    extern XCreateBitmapFromData(Display * dpy, Drawable d, char *data,
				 unsigned int width, unsigned int height);
    extern XResizeWindow(Display * dpy, Window w, unsigned int width,
			 unsigned int height);
    extern XMoveResizeWindow(Display * dpy, Window w, int x, int y,
			     unsigned int width, unsigned int height);
    extern XSelectInput(Display * dpy, Window w, long event_mask);
    extern KeySym XStringToKeysym(char *s);
    extern char *XKeysymToString(KeySym keysym);

    typedef int Time;
#define	CurrentTime		0L	/* special Time (ignored in Windows) */
#define	GrabModeAsync	1
    extern int XGrabPointer(Display * display, Window w, Bool owner_events,
			    unsigned int event_mask, int pointer_mode,
			    int keyboard_mode, Window confine_to,
			    Cursor cursor, Time time);
    extern XUngrabPointer(Display * display, Time time);
    extern XWarpPointer(Display * display, Window src_w, Window dest_w,
			int src_x, int src_y,
			unsigned int src_width, unsigned int src_height,
			int dest_x, int dest_y);
    extern XDefineCursor(Display * d, Window w, Cursor c);

    extern XClearArea(Display * d, Window w, int x, int y,
		      unsigned int width, unsigned int height,
		      Bool exposures);
    extern XCheckMaskEvent(Display * d, long event_mask,
			   XEvent * event_return);

#define	DefaultScreen(_dpy)		(0)

    int DefaultDepth(Display * d, int screen);

#define ConnectionNumber(_dpy)		(0)

    extern XFlush(Display * display);
    extern XSync(Display * display, Bool discard);

#ifdef	__cplusplus
};
#endif


#endif				/* _WINDOWS */
#endif				/* _WINX_H_ */
