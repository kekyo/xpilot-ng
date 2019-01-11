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
*  winX11.c - X11 to Windoze converter										*
*																			*
*  This file is the direct interface to the X11 library.					*
*  Any function that has a unix man page belongs in this file.				*
\***************************************************************************/

#include "winX.h"
#include "winX_.h"
#include "draw.h"

#include "../error.h"
#include "../../client/NT/winClient.h"	/* This needs to be removed */

const int top = 0;
/*****************************************************************************/
XFillRectangle(Display * dpy, Drawable d, GC gc, int x, int y,
	       unsigned int w, unsigned int h)
{

    HDC hDC = xid[d].hwnd.hBmpDC;
    RECT r;
    r.left = x;
    r.top = y;
    r.right = x + w;
    r.bottom = y + h;
    FillRect(hDC, &r, WinXGetBrush(xid[gc].hgc.xgcv.foreground));

    return (0);
}

/*****************************************************************************/
XDrawRectangle(Display * dpy, Drawable d, GC gc, int x, int y,
	       unsigned int w, unsigned int h)
{

    HDC hDC = xid[d].hwnd.hBmpDC;
    MoveToEx(hDC, x, y, NULL);
    LineTo(hDC, x + w, y);
    LineTo(hDC, x + w, y + h);
    LineTo(hDC, x, y + h);
    LineTo(hDC, x, y);

    return (0);
}

XFillRectangles(Display * dpy, Drawable d, GC gc,
		XRectangle * rects, int nrectangles)
{
    int i;
    HDC hDC = xid[d].hwnd.hBmpDC;
    HBRUSH hBrush = WinXGetBrush(xid[gc].hgc.xgcv.foreground);
    RECT r;

    for (i = 0; i < nrectangles; i++, rects++) {
	r.left = rects->x;
	r.top = rects->y;
	r.right = rects->x + rects->width;
	r.bottom = rects->y + rects->height;
	FillRect(hDC, &r, hBrush);
    }
    return (0);
}

/*****************************************************************************/
XDrawLine(Display * dpy, Drawable d, GC gc, int x1, int y1, int x2, int y2)
{
    HDC hDC = xid[d].hwnd.hBmpDC;
    MoveToEx(hDC, x1, y1, NULL);
    LineTo(hDC, x2, y2);
    return (0);
}

/*****************************************************************************/
XDrawLines(Display * dpy, Drawable d, GC gc, XPoint * points,
	   int npoints, int mode)
{
    int i = 0;
    HDC hDC = xid[d].hwnd.hBmpDC;

    if (mode == CoordModePrevious) {
	int x, y;
	x = points->x;
	y = points->y;
	MoveToEx(hDC, x, y, NULL);
	points++;
	for (i = 1; i < npoints; i++, points++) {
	    x += points->x;
	    y += points->y;
	    LineTo(hDC, x, y);
	}

    } else {
	MoveToEx(hDC, points->x, points->y, NULL);
	points++;
	for (i = 1; i < npoints; i++, points++) {
	    LineTo(hDC, points->x, points->y);
	}
    }
    return (0);
}

/*****************************************************************************/
XDrawSegments(Display * dpy, Drawable d, GC gc,
	      XSegment * segments, int nsegments)
{
    int i;
    HDC hDC = xid[d].hwnd.hBmpDC;

    for (i = 0; i < nsegments; i++, segments++) {
	MoveToEx(hDC, segments->x1, segments->y1, NULL);
	LineTo(hDC, segments->x2, segments->y2);
    }

    return (0);
}

/*****************************************************************************/
XDrawPoint(Display * dpy, Drawable d, GC gc, int x, int y)
{
    HDC hDC = xid[d].hwnd.hBmpDC;
    SetPixelV(hDC, x, y, WinXPColour(xid[gc].hgc.xgcv.foreground));
    return (0);
}

/*****************************************************************************/
XDrawPoints(Display * dpy, Drawable d, GC gc,
	    XPoint * points, int npoints, int mode)
{
    int i;
    HDC hDC = xid[d].hwnd.hBmpDC;
    COLORREF col = WinXPColour(xid[gc].hgc.xgcv.foreground);

    for (i = 0; i < npoints; i++, points++)
	SetPixelV(hDC, points->x, points->y, col);

    return (0);
}

/*****************************************************************************/
XFillPolygon(Display * dpy, Drawable d, GC gc, XPoint * points,
	     int npoints, int shape, int mode)
{
    int i;
    int px1, px2, py1, py2;	// bounding box for the polygon
    HDC hdc = xid[d].hwnd.hBmpDC;

    /* 
     * As Windows 95/98/ME doesn't support textured brushes with texture size 
     * over 8x8, I'll provide my own routine for painting textured polygons. 
     * It first creates a clipping region that restricts GDI operations to the
     * area of the polygon. Then it blits the texture bitmap over the polygon
     * so that it gets fully covered.
     */

    if (!BeginPath(hdc))
	return -1;

    px1 = px2 = points->x;
    py1 = py2 = points->y;
    if (!MoveToEx(hdc, points->x, points->y, NULL))
	return -1;
    points++;

    for (i = 1; i < npoints; i++, points++) {
	if (!LineTo(hdc, points->x, points->y))
	    return -1;
	if (points->x < px1)
	    px1 = points->x;
	else if (points->x > px2)
	    px2 = points->x;
	if (points->y < py1)
	    py1 = points->y;
	else if (points->y > py2)
	    py2 = points->y;
    }

    if (!EndPath(hdc))
	return -1;


    if (xid[gc].hgc.xgcv.fill_style != FillTiled) {

	if (!StrokeAndFillPath(hdc))
	    return -1;

    } else {

	int x, y;
	int sx, sy;		// where to start blitting
	SIZE bs;		// bitmap dimensions
	HBITMAP hBmp = (HBITMAP) xid[gc].hgc.xgcv.tile;
	extern HDC itemsDC;	// from winX.c

	if (!GetBitmapDimensionEx(hBmp, &bs))
	    return -1;
	if (!SelectObject(itemsDC, hBmp))
	    return -1;
	if (!SelectPalette(itemsDC, myPal, FALSE))
	    return -1;
	if (RealizePalette(itemsDC) == GDI_ERROR)
	    return -1;

	sx = xid[gc].hgc.xgcv.ts_x_origin - px1;
	sx = (sx > 0) ? px1 + sx % bs.cx - bs.cx : px1 - sx % bs.cx;

	sy = xid[gc].hgc.xgcv.ts_y_origin - py1;
	sy = (sy > 0) ? py1 + sy % bs.cy - bs.cy : py1 - sy % bs.cy;

	if (!SelectClipPath(hdc, RGN_AND))
	    return -1;

	for (x = sx; x < px2; x += bs.cx) {
	    for (y = sy; y < py2; y += bs.cy) {
		//XDrawRectangle(dpy, d, gc, x, y, bs.cx, bs.cy);
		BitBlt(hdc, x, y, bs.cx, bs.cy, itemsDC, 0, 0, SRCCOPY);
	    }
	}

	SelectClipRgn(hdc, NULL);
    }

    return (0);
}

/*****************************************************************************/
XDrawArc(Display * dpy, Drawable d, GC gc, int x, int y,
	 unsigned int width, unsigned int height, int angle1, int angle2)
{
    HDC hDC = xid[d].hwnd.hBmpDC;
    MoveToEx(hDC, x + width, y + height / 2, NULL);
    if (bWinNT)
	AngleArc(hDC, x + width / 2, y + height / 2, width / 2,
		 (float) 0.0, (float) (angle2 / 64.0));
    else
	AngleArc2(hDC, x + width / 2, y + height / 2, width / 2, 0.0,
		  angle2 / 64.0, FALSE);
    return (0);
}

/*****************************************************************************/
XDrawArcs(Display * dpy, Drawable d, GC gc, XArc * arcs, int narcs)
{
    int i;
    HDC hDC = xid[d].hwnd.hBmpDC;

    for (i = 0; i < narcs; i++, arcs++) {
	MoveToEx(hDC, arcs->x + arcs->width, arcs->y + arcs->height / 2,
		 NULL);
	if (bWinNT)
	    AngleArc(hDC, arcs->x + arcs->width / 2,
		     arcs->y + arcs->height / 2, arcs->width / 2,
		     (float) 0.0, (float) (arcs->angle2 / 64.0));
	else
	    AngleArc2(hDC, arcs->x + arcs->width / 2,
		      arcs->y + arcs->height / 2, arcs->width / 2, 0.0,
		      arcs->angle2 / 64.0, FALSE);
    }
    return (0);
}

/*****************************************************************************/
XFillArc(Display * dpy, Drawable d, GC gc, int x, int y,
	 unsigned int width, unsigned int height, int angle1, int angle2)
{

    HDC hDC = xid[d].hwnd.hBmpDC;

    // 2 separate bits 'cause Win95 doesn't put Arc into current path
    // We use Pie instead.
    if (bWinNT) {
	BeginPath(hDC);
	MoveToEx(hDC, x + width, y + height / 2, NULL);
	AngleArc(hDC, x + width / 2, y + height / 2, width / 2,
		 (float) 0.0, (float) (angle2 / 64.0));
	EndPath(hDC);
	StrokeAndFillPath(hDC);
    } else {
	MoveToEx(hDC, x + width, y + height / 2, NULL);
	AngleArc2(hDC, x + width / 2, y + height / 2, width / 2, 0.0,
		  angle2 / 64.0, TRUE);
    }
    return (0);
}

/*****************************************************************************/
XDrawString(Display * dpy, Drawable d, GC gc, int x, int y,
	    const char *string, int length)
{
    HDC hDC = xid[d].hwnd.hBmpDC;
    HFONT hOldFont = NULL;

    hOldFont = SelectObject(hDC, xid[gc].hgc.hfont);
    SetTextColor(hDC, WinXPColour(xid[gc].hgc.xgcv.foreground));
    TextOut(hDC, x, y - xid[xid[gc].hgc.xgcv.font].font.font->ascent,
	    string, length);
    if (hOldFont)
	SelectObject(hDC, hOldFont);

    return (0);
}

/*****************************************************************************/
XTextWidth(XFontStruct * font, const char *string, int length)
{
    HDC hDC = xid[top].hwnd.hBmpDC;
    SIZE size;
    XID i, f;
    for (f = 0; f < MAX_XIDS; f++) {
	if (xid[f].type == XIDTYPE_FONT && xid[f].font.font == font)
	    break;
    }
    if (f == MAX_XIDS) {
	Trace("Huh? Can't match font for string <%s>\n", string);
    } else {
	for (i = 0; i < MAX_XIDS; i++)
	    if (xid[i].type == XIDTYPE_HDC)
		if (xid[i].hgc.xgcv.font == f)
		    break;
    }
    if (i == MAX_XIDS) {
	Trace("Huh? Can't find a GC for font %d, string=<%s>\n", f,
	      string);
    } else {
	hDC = xid[xid[i].hgc.xidhwnd].hwnd.hBmpDC;
	SelectObject(hDC, xid[i].hgc.hfont);
    }

    GetTextExtentPoint32(hDC, string, length, &size);
    return (size.cx);
}

/*****************************************************************************/
XChangeGC(Display * dpy, GC gc, unsigned long valuemask,
	  XGCValues * values)
{
    XGCValues *xgcv;
    if (xid[gc].type != XIDTYPE_HDC)
	return 0;

    xgcv = &xid[gc].hgc.xgcv;

    if (valuemask & GCFunction)
	xgcv->function = values->function;
    if (valuemask & GCPlaneMask)
	xgcv->plane_mask = values->plane_mask;
    if (valuemask & GCForeground)
	xgcv->foreground = values->foreground;
    if (valuemask & GCBackground)
	xgcv->background = values->background;
    if (valuemask & GCLineWidth)
	xgcv->line_width = values->line_width;
    if (valuemask & GCLineStyle)
	xgcv->line_style = values->line_style;
    if (valuemask & GCCapStyle)
	xgcv->cap_style = values->cap_style;
    if (valuemask & GCJoinStyle)
	xgcv->join_style = values->join_style;
    if (valuemask & GCFillStyle)
	xgcv->fill_style = values->fill_style;
    if (valuemask & GCFillRule)
	xgcv->fill_rule = values->fill_rule;
    if (valuemask & GCTile)
	xgcv->tile = values->tile;
    if (valuemask & GCStipple)
	xgcv->stipple = values->stipple;
    if (valuemask & GCTileStipXOrigin)
	xgcv->ts_x_origin = values->ts_x_origin;
    if (valuemask & GCTileStipYOrigin)
	xgcv->ts_y_origin = values->ts_y_origin;
    if (valuemask & GCFont)
	xgcv->font = values->font;
    if (valuemask & GCSubwindowMode)
	xgcv->subwindow_mode = values->subwindow_mode;
    if (valuemask & GCGraphicsExposures)
	xgcv->graphics_exposures = values->graphics_exposures;
    if (valuemask & GCClipXOrigin)
	xgcv->clip_x_origin = values->clip_x_origin;
    if (valuemask & GCClipYOrigin)
	xgcv->clip_y_origin = values->clip_y_origin;
    if (valuemask & GCDashOffset)
	xgcv->dash_offset = values->dash_offset;
    if (valuemask & GCArcMode)
	xgcv->arc_mode = values->arc_mode;

    if (valuemask & (GCForeground | GCLineWidth | GCLineStyle))
	WinXSelectPen(gc);

    return (1);
}

/*****************************************************************************/
int XGetGCValues(Display * dpy, GC gc, unsigned long valuemask,
		 XGCValues * values)
{
    XGCValues *xgcv;
    if (xid[gc].type != XIDTYPE_HDC)
	return 0;

    xgcv = &xid[gc].hgc.xgcv;

    if (valuemask & GCFunction)
	values->function = xgcv->function;
    if (valuemask & GCPlaneMask)
	values->plane_mask = xgcv->plane_mask;
    if (valuemask & GCForeground)
	values->foreground = xgcv->foreground;
    if (valuemask & GCBackground)
	values->background = xgcv->background;
    if (valuemask & GCLineWidth)
	values->line_width = xgcv->line_width;
    if (valuemask & GCLineStyle)
	values->line_style = xgcv->line_style;
    if (valuemask & GCCapStyle)
	values->cap_style = xgcv->cap_style;
    if (valuemask & GCJoinStyle)
	values->join_style = xgcv->join_style;
    if (valuemask & GCFillStyle)
	values->fill_style = xgcv->fill_style;
    if (valuemask & GCFillRule)
	values->fill_rule = xgcv->fill_rule;
    if (valuemask & GCTile)
	values->tile = xgcv->tile;
    if (valuemask & GCStipple)
	values->stipple = xgcv->stipple;
    if (valuemask & GCTileStipXOrigin)
	values->ts_x_origin = xgcv->ts_x_origin;
    if (valuemask & GCTileStipYOrigin)
	values->ts_y_origin = xgcv->ts_y_origin;
    if (valuemask & GCFont)
	values->font = xgcv->font;
    if (valuemask & GCSubwindowMode)
	values->subwindow_mode = xgcv->subwindow_mode;
    if (valuemask & GCGraphicsExposures)
	values->graphics_exposures = xgcv->graphics_exposures;
    if (valuemask & GCClipXOrigin)
	values->clip_x_origin = xgcv->clip_x_origin;
    if (valuemask & GCClipYOrigin)
	values->clip_y_origin = xgcv->clip_y_origin;
    if (valuemask & GCDashOffset)
	values->dash_offset = xgcv->dash_offset;
    if (valuemask & GCArcMode)
	values->arc_mode = xgcv->arc_mode;


    if (valuemask & GCFunction) {
	values->function = GXcopy;
    }
    if (valuemask & GCBackground) {
	values->background = BLACK;	/* always black */
    }

    return 1;
}

/*****************************************************************************/
XSetLineAttributes(Display * dpy, GC gc, unsigned int lwidth,
		   int lstyle, int cap_style, int join_style)
{
    XGCValues *xgcv;

    if (xid[gc].type != XIDTYPE_HDC)
	return -1;

    xgcv = &xid[gc].hgc.xgcv;
    xgcv->line_width = lwidth;
    xgcv->line_style = lstyle;
    xgcv->cap_style = cap_style;
    xgcv->join_style = join_style;

    WinXSelectPen(gc);

    return (0);
}

/*****************************************************************************/
XCopyArea(Display * dpy, Drawable src, Drawable dest, GC gc,
	  int src_x, int src_y, unsigned int width, unsigned int height,
	  int dest_x, int dest_y)
{
    return (0);
}

/*****************************************************************************/
XSetTile(Display * dpy, GC gc, Pixmap tile)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.tile = tile;
    return (0);
}

/*****************************************************************************/
XSetTSOrigin(Display * dpy, GC gc, int ts_x_origin, int ts_y_origin)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.ts_x_origin = ts_x_origin;
    xid[gc].hgc.xgcv.ts_y_origin = ts_y_origin;
    return (0);
}

/*****************************************************************************/
XSetFillStyle(Display * dpy, GC gc, int fill_style)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.fill_style = fill_style;
    return (0);
}

/*****************************************************************************/
XSetFunction(Display * dpy, GC gc, int function)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.function = function;
    return (0);
}

/*****************************************************************************/
XBell(Display * dpy, int percent)
{
    MessageBeep(MB_ICONEXCLAMATION);
    return (0);
}

/*****************************************************************************/
XFlush(Display * dpy)
{
    return (0);
}

/*****************************************************************************/
XCreatePixmap(Display * dpy, Drawable d,
	      unsigned int width, unsigned int height, unsigned int depth)
{
    XID txid;
    HDC hDC = xid[d].hwnd.hBmpDC;
    HDC newDC = CreateCompatibleDC(NULL);
//      HDC             hScreenDC = GetDC(xid[draw].hwnd.hWnd);
    HDC hScreenDC = GetDC(xid[top].hwnd.hWnd);
    HBITMAP hbm = CreateCompatibleBitmap(hScreenDC, width, height);

//      ReleaseDC(xid[draw].hwnd.hWnd, hScreenDC);
    ReleaseDC(xid[top].hwnd.hWnd, hScreenDC);

    if (!hbm) {
	error("Can't create pixmap\n");
	return (0);
    }
    SelectObject(newDC, hbm);
    SelectPalette(newDC, myPal, FALSE);
    RealizePalette(newDC);

    txid = GetFreeXid();
    xid[txid].type = XIDTYPE_PIXMAP;
    xid[txid].hpix.hbm = hbm;
    xid[txid].hpix.hDC = newDC;
#if 0
    if (++max_xid > MAX_XIDS) {
	error("Too many XIDS!\n");
	max_xid--;
    }
    return (max_xid - 1);
#endif
    return (txid);
}

/*****************************************************************************/
XFreePixmap(Display * dpy, Pixmap pixmap)
{
    return (0);
}

/*****************************************************************************/
XSetPlaneMask(Display * dpy, GC gc, unsigned long plane_mask)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.plane_mask = plane_mask;
    return (0);
}

/*****************************************************************************/
XClearWindow(Display * dpy, Window w)
{
    HWND hWnd = xid[w].hwnd.hWnd;
    HDC hDC = xid[w].hwnd.hBmpDC;
    RECT rect;

    GetClientRect(hWnd, &rect);
    FillRect(hDC, &rect, WinXGetBrush(xid[w].hwnd.bgcolor));
    return (0);
}

/*****************************************************************************/
XClearArea(Display * d, Window w, int x, int y,
	   unsigned int width, unsigned int height, Bool exposures)
{
    HWND hWnd = xid[w].hwnd.hWnd;
    HDC hDC = xid[w].hwnd.hBmpDC;
    RECT rect;

    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    FillRect(hDC, &rect, WinXGetBrush(xid[w].hwnd.bgcolor));
    return (0);
}

/*****************************************************************************/
XStoreName(Display * dpy, Window w, const char *window_name)
{
    SetWindowText(xid[w].hwnd.hWnd, window_name);
    return (0);
}

/*****************************************************************************/
XLookupKeysym(XKeyEvent * key_event, int index)
{
    return (key_event->keycode);
}

/*****************************************************************************/
XFontStruct *XQueryFont(Display * dpy, XID font_ID)
{
    return (xid[xid[font_ID].hgc.xgcv.font].font.font);
}

/*****************************************************************************/
XFreeFontInfo(char **names, XFontStruct *free_info, int count)
{
	return (0);
}


/*****************************************************************************/
XSetFont(Display * dpy, GC gc, Font font)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.hfont = xid[font].font.font->hFont;
    xid[gc].hgc.xgcv.font = font;
    return (0);
}

/*****************************************************************************/
Window XCreateSimpleWindow_(Display * dpy, Window parent, int x, int y,
			    unsigned int width, unsigned int height,
			    unsigned int border_width,
			    unsigned long border, unsigned long background
#ifdef	_DEBUG
			    , const char *file, const int line
#endif
    )
{
    HWND hWnd;
    XID txid;
    int wstyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    txid = GetFreeXid();
    xid[txid].hwnd.type = XIDTYPE_HWND;
    xid[txid].hwnd.hBmp = NULL;
    xid[txid].hwnd.hBmpDC = NULL;
    xid[txid].hwnd.hWnd = NULL;
    xid[txid].hwnd.event_mask = ButtonPressMask | ButtonReleaseMask
	| LeaveWindowMask | EnterWindowMask;
    xid[txid].hwnd.event_mask = -1;	// hell, let's take em all!
    xid[txid].hwnd.mouseover = 0;	// mouse not over this window
    xid[txid].hwnd.bgcolor = background;
    xid[txid].hwnd.notmine = FALSE;
    xid[txid].hwnd.drawtype = DT_1;

#ifdef	_DEBUG
    strncpy(xid[txid].any.file,
	    strrchr(file, '\\') + 1, WINXFILELENGTH - 1);
    xid[txid].any.line = line;
#endif

    switch (border_width) {
    case 1:
	wstyle =
	    WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
	    WS_CLIPCHILDREN;
	break;
    case 2:
	wstyle =
	    WS_DLGFRAME | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
	    WS_CLIPCHILDREN;
	break;
    case 3:
	wstyle = WS_DLGFRAME | WS_POPUP | WS_VISIBLE;
	wstyle =
	    WS_DLGFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
	y += GetSystemMetrics(SM_CYCAPTION);
	break;
    }
    Trace("XCreateSimpleWindow_: %d p=%d, %d/%d %d/%d b_width=%d\n", txid,
	  parent, x, y, width, height, border_width);
    hWnd = xid[txid].hwnd.hWnd =
	CreateWindow("XPilotWin", "", wstyle, x, y, width, height,
		     xid[parent].hwnd.hWnd, NULL, hInstance,
		     (LPVOID) txid);
    if (!hWnd)
	error("Can't open window\n");
    else {
	HDC hDC = GetDC(hWnd);
	SelectPalette(hDC, myPal, FALSE);
	RealizePalette(hDC);
	SetBkMode(hDC, OPAQUE);
	ReleaseDC(hWnd, hDC);
    }
#if 0
    if (++max_xid > MAX_XIDS) {
	error("Too many XIDS!\n");
	max_xid--;
    }
    return (max_xid - 1);
#endif
    return (txid);
}

/*****************************************************************************/
Window XCreateWindow_(Display * dpy, Window parent, int x, int y,
		      unsigned int width, unsigned int height,
		      unsigned int border_width, int depth,
		      unsigned int c_class, Visual * visual,
		      unsigned long valuemask,
		      XSetWindowAttributes * attributes
#ifdef	_DEBUG
		      , const char *file, const int line
#endif
    )
{
    Window w = XCreateSimpleWindow_(dpy, parent, x, y, width, height,
				    border_width, 0, 0
#ifdef	_DEBUG
				    , file, line
#endif
	);
    xid[w].hwnd.event_mask = attributes->event_mask;
    return (w);
}

GC WinXCreateWinDC_(Window w
#ifdef	_DEBUG
		    , const char *file, const int line
#endif
    )
{
    XID txid;
    txid = GetFreeXid();
    xid[txid].hgc.type = XIDTYPE_HDC;
    xid[txid].hgc.xidhwnd = w;

#ifdef	_DEBUG
    strncpy(xid[txid].any.file,
	    strrchr(file, '\\') + 1, WINXFILELENGTH - 1);
    xid[txid].any.line = line;
#endif

#if 0
    if (++max_xid > MAX_XIDS) {
	error("Too many XIDS!\n");
	max_xid--;
    }
    return (max_xid - 1);
#endif
    return (txid);
}

/*****************************************************************************/
XSetForeground(Display * dpy, GC gc, unsigned long foreground)
{
#if 0
#ifdef PENS_OF_PLENTY
    if (xid[gc].type == XIDTYPE_PIXMAP) {
	HDC hDC = xid[gc].hpix.hDC;
	SetTextColor(hDC, WinXPColour(foreground));
	SelectObject(hDC, objs[foreground].brush);
	SelectObject(hDC, objs[foreground].pen);
    } else {
	int xidno = xid[gc].hgc.xidhwnd;
	xid[xidno].hwnd.cur_color = foreground;
	WinXSetPen(xidno);
    }
#else
    HDC hDC;
    Window w;
    cur_color = foreground;
    if (xid[gc].type == XIDTYPE_PIXMAP) {
	hDC = xid[gc].hpix.hDC;
	SetTextColor(hDC, WinXPColour(cur_color));
	SelectObject(hDC, objs[cur_color].pen);
	SelectObject(hDC, objs[cur_color].brush);
    } else {
	w = xid[gc].hgc.xidhwnd;
	if (xid[w].hwnd.drawtype == DT_1) {
	    hDC = xid[w].hwnd.hBmpDC;
	    SetTextColor(hDC, WinXPColour(cur_color));
	    SelectObject(hDC, objs[cur_color].pen);
	    SelectObject(hDC, objs[cur_color].brush);
	} else {		/* DT_2 */

	    SetTextColor(xid[w].hwnd.hBmpDCa[0], WinXPColour(cur_color));
	    SelectObject(xid[w].hwnd.hBmpDCa[0], objs[cur_color].pen);
	    SelectObject(xid[w].hwnd.hBmpDCa[0], objs[cur_color].brush);
	    SetTextColor(xid[w].hwnd.hBmpDCa[1], WinXPColour(cur_color));
	    SelectObject(xid[w].hwnd.hBmpDCa[1], objs[cur_color].pen);
	    SelectObject(xid[w].hwnd.hBmpDCa[1], objs[cur_color].brush);
	}
    }
#endif
#endif
    //if (xid[gc].type == XIDTYPE_HDC) {

    xid[gc].hgc.xgcv.foreground = foreground;
    WinXSelectPen(gc);
    WinXSelectBrush(gc);
/*	} else {
		// ouch, terrible hack, but fixing this properly would be difficult
		HDC hDC;
		extern HPEN pens[256][10][3];
		extern HBRUSH brushes[256];
		hDC = xid[gc].hpix.hDC;
		SelectObject(hDC, pens[foreground][0][0]);
		SelectObject(hDC, brushes[foreground]);
	}
*/
    return (0);
}


/*****************************************************************************/
XDestroyWindow(Display * dpy, Window w)
{
    WinXFree(w);
    return (0);
}

/*****************************************************************************/
XUnmapWindow(Display * dpy, Window w)
{
    ShowWindow(xid[w].hwnd.hWnd, SW_HIDE);
    return (0);
}

/*****************************************************************************/
XMapWindow(Display * dpy, Window w)
{
    //ShowWindow(xid[w].hwnd.hWnd, SW_SHOW);
    XMapRaised(dpy, w);
    return (0);
}

/*****************************************************************************/
XMapSubwindows(Display * dpy, Window w)
{
    ShowWindow(xid[w].hwnd.hWnd, SW_SHOW);
    return (0);
}

/*****************************************************************************/
XMapRaised(Display * dpy, Window w)
{
    ShowWindow(xid[w].hwnd.hWnd, SW_SHOW);
    //SetWindowPos(xid[w].hwnd.hWnd, HWND_TOPMOST,
    //      0, 0, 0, 0,
    //      SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    BringWindowToTop(xid[w].hwnd.hWnd);
    return (0);
}

/*****************************************************************************/
XMoveWindow(Display * dpy, Window w, int x, int y)
{
    RECT rect;
    GetClientRect(xid[w].hwnd.hWnd, &rect);
    MoveWindow(xid[w].hwnd.hWnd, x, y, rect.right, rect.bottom, TRUE);
    return (0);
}

/*****************************************************************************/
XMoveResizeWindow(Display * dpy, Window w, int x, int y,
		  unsigned int width, unsigned int height)
{
    HWND hWnd = xid[w].hwnd.hWnd;

    SetWindowPos(hWnd, NULL, x, y, width, height, SWP_NOZORDER);
    return (0);
}

/*****************************************************************************/
XSetDashes(Display * dpy, GC gc, int dash_offset,
	   const char *dash_list, int n)
{
    if (xid[gc].type != XIDTYPE_HDC)
	return -1;
    xid[gc].hgc.xgcv.dash_offset = dash_offset;
    xid[gc].hgc.xgcv.dashes = dash_list[0];

    xid[gc].hgc.xgcv.line_style = LineOnOffDash;

    WinXSelectPen(gc);
    return (0);
}

/*****************************************************************************/
XSelectInput(Display * dpy, Window w, long event_mask)
{
    xid[w].hwnd.event_mask =
	event_mask | ButtonPressMask | ButtonReleaseMask;
    return (0);
}


/*****************************************************************************/
Window DefaultRootWindow(Display * dpy)
{
/*	return(top); */
    return (0);
}

/*****************************************************************************/
int DisplayWidth(Display * dpy, int screen_number)
{
    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);
    GetWindowRect(xid[top].hwnd.hWnd, &rect);
    return (rect.right);
}

/*****************************************************************************/
int DisplayHeight(Display * dpy, int screen_number)
{
    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);
    GetWindowRect(xid[top].hwnd.hWnd, &rect);
    return (rect.bottom);
}

/*****************************************************************************/
Bool XCheckIfEvent(Display * dpy, XEvent * event_return,
		   Bool(*predicate) (), XPointer arg)
{
    return (FALSE);
}

/*****************************************************************************/
XSetIconName(Display * dpy, Window w, const char *icon_name)
{
    return (0);
}

/*****************************************************************************/
XSetTransientForHint(Display * dpy, Window w, Window prop_window)
{
    return (0);
}

/*****************************************************************************/
XChangeWindowAttributes(Display * dpy, Window w, unsigned long valuemask,
			XSetWindowAttributes * attributes)
{
    return (0);
}

/*****************************************************************************/
XSetWindowBackground(Display * dpy, Window w,
		     unsigned long background_pixel)
{				/* TODO */
    return (0);
}

/*****************************************************************************/
XGetWindowAttributes(Display * dpy, Window w,
		     XWindowAttributes * attributes)
{
    RECT rect;
    GetClientRect(xid[w].hwnd.hWnd, &rect);
    attributes->x = attributes->y = 0;
    attributes->width = rect.right;
    attributes->height = rect.bottom;
    return (0);
}

/*****************************************************************************/
GContext XGContextFromGC(GC gc)
{
    return (gc);
}

/*****************************************************************************/
int XGrabPointer(Display * display, Window w, Bool owner_events,
		 unsigned int event_mask, int pointer_mode,
		 int keyboard_mode, Window confine_to, Cursor cursor,
		 Time time)
{
    SetCapture(xid[w].hwnd.hWnd);
    xid[w].hwnd.event_mask = event_mask;	/* this could be a macro, */
//      WinXSetEventMask(w, event_mask);
    return (0);
}

/*****************************************************************************/
XUngrabPointer(Display * display, Time time)
{
    ReleaseCapture();
    return (0);
}

/*****************************************************************************/
XWarpPointer(Display * display, Window src_w, Window dest_w,
	     int src_x, int src_y,
	     unsigned int src_width, unsigned int src_height,
	     int dest_x, int dest_y)
{
#ifdef _WINDOWS
    SetCursorPos(dest_x, dest_y);
#else
    RECT rect;
    GetWindowRect(xid[dest_w].hwnd.hWnd, &rect);
    SetCursorPos(rect.left + dest_x, rect.top + dest_y);
#endif
    return (0);
}

/*****************************************************************************/
XDefineCursor(Display * d, Window w, Cursor c)
{
    if (c == None)
	ShowCursor(TRUE);
    else
	ShowCursor(FALSE);
    return (0);
}

/*****************************************************************************/
int DefaultDepth(Display * d, int screen)
{
    return (8);			// assume 256 colors (bad assumption but OK for now)
}

/*****************************************************************************/
int XSync(Display * display, Bool discard)
{
    return 0;
}
