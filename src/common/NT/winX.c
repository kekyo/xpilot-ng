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
*  winX.c - X11 to Windoze converter										*
*																			*
*  This file is mostly Win32 translations of the X calls that xpilot uses.	*
*  Anything starting with WinX is a special wedge function that i needed	*
*																			*
*  						*
\***************************************************************************/
#include "../../client/NT/xpclient_x11.h"
/*
#include "winX.h"
#include "windows.h"
#include "../../client/NT/winClient.h"
#include "winX_.h"
#include "../../client/NT/winXThread.h"
#include <math.h>
#include <stdio.h>

#include "../error.h"
#include "../const.h"
#include "../draw.h"
#include "../../client/paint.h"
#include "../../client/xinit.h"
#include "../../client/widget.h"
*/
int iScaleFactor;

// Radar window is updated every RadarDivisor frames.
int RadarDivisor;
int ThreadedDraw;

extern Window drawWindow;		// we only want this one

Window rootWindow = 0;		// The whole screen

static double fTwoPi = 2.0 * PI;

HINSTANCE hInstance;
HPALETTE myPal;
LOGPALETTE *myLogPal;
HFONT hFixedFont;

HDC itemsDC;			// for blitting items onto the screen

XIDTYPE xid[MAX_XIDS];

BOOL bWinNT = 0;		// need this 'cause Win95 can't draw a simple circle
BOOL drawPending = FALSE;	// try to throttle the deadly frame backup syndrome

#define MAX_LINE_WIDTH 10
HPEN pens[WINMAXCOLORS][MAX_LINE_WIDTH][3];
HBRUSH brushes[WINMAXCOLORS];

void WinXExit();

static void WinXSetupRadarWindow()
{
    if (radarWindow) {
	/*      if (instruments & SHOW_SLIDING_RADAR)
	   {
	   if (xid[radar].hwnd.hSaveDC != NULL)
	   {
	   ReleaseDC(xid[radar].hwnd.hWnd, xid[radar].hwnd.hBmpDC);
	   xid[radar].hwnd.hBmpDC = xid[radar].hwnd.hSaveDC;
	   xid[radar].hwnd.hSaveDC = NULL;
	   }
	   }
	   else */
	{
	    if (xid[radarWindow].hwnd.hSaveDC == NULL) {
		HDC hNewDC = GetDC(xid[radarWindow].hwnd.hWnd);
		xid[radarWindow].hwnd.hSaveDC = xid[radarWindow].hwnd.hBmpDC;
		xid[radarWindow].hwnd.hBmpDC = hNewDC;
		SelectPalette(hNewDC, myPal, FALSE);
		RealizePalette(hNewDC);
	    }
	}
    }
}

static void WinXDeleteDraw(int xidno)
{
    HDC hSaveDC = xid[xidno].hwnd.hSaveDC;
    HDC hBmpDC = xid[xidno].hwnd.hBmpDC;

    if (xid[xidno].hwnd.type == DT_2) {
	if (xid[xidno].hwnd.hBmpa[0])
	    DeleteObject(xid[xidno].hwnd.hBmpa[0]);
	if (xid[xidno].hwnd.hBmpa[1])
	    DeleteObject(xid[xidno].hwnd.hBmpa[1]);
	xid[xidno].hwnd.hBmpa[0] = xid[xidno].hwnd.hBmpa[0] = NULL;
	if (xid[xidno].hwnd.hBmpDCa[0])
	    DeleteDC(xid[xidno].hwnd.hBmpDCa[0]);
	if (xid[xidno].hwnd.hBmpDCa[1])
	    DeleteDC(xid[xidno].hwnd.hBmpDCa[1]);
	xid[xidno].hwnd.hBmpDCa[0] = xid[xidno].hwnd.hBmpDCa[1] = NULL;

    } else {
	if (xid[xidno].hwnd.hBmp)
	    DeleteObject(xid[xidno].hwnd.hBmp);
    }
    xid[xidno].hwnd.hBmp = NULL;
    if (hSaveDC) {
	DeleteDC(hSaveDC);
	if (hBmpDC)
	    ReleaseDC(xid[xidno].hwnd.hWnd, hBmpDC);
    } else if (hBmpDC)
	DeleteDC(hBmpDC);

    xid[xidno].hwnd.hBmpDC = xid[xidno].hwnd.hSaveDC = NULL;
    xid[xidno].hwnd.hBmp = NULL;
}

static void WinXCreateBitmapForXid(HWND hwnd, XID xidno, int cx, int cy)
{
    HDC hBmpDC, hDC = GetDC(hwnd);
    HBITMAP hBmp;
    RECT r;

    WinXDeleteDraw(xidno);
    if (ThreadedDraw && xidno == drawWindow) {
	xid[xidno].hwnd.hBmpa[1] = CreateCompatibleBitmap(hDC, cx, cy);
	xid[xidno].hwnd.hBmpa[0] = CreateCompatibleBitmap(hDC, cx, cy);
	hBmp = xid[xidno].hwnd.hBmp = xid[xidno].hwnd.hBmpa[0];
	xid[xidno].hwnd.hBmpDCa[0] = CreateCompatibleDC(hDC);
	xid[xidno].hwnd.hBmpDCa[1] = CreateCompatibleDC(hDC);
	xid[xidno].hwnd.hBmpDC = xid[xidno].hwnd.hBmpDCa[0];
	xid[xidno].hwnd.filling = 0;
	xid[xidno].hwnd.drawtype = DT_2;
	SelectObject(xid[xidno].hwnd.hBmpDCa[0], xid[xidno].hwnd.hBmpa[0]);
	SelectObject(xid[xidno].hwnd.hBmpDCa[1], xid[xidno].hwnd.hBmpa[1]);
	SetBkMode(xid[xidno].hwnd.hBmpDCa[0], TRANSPARENT);
	SetBkMode(xid[xidno].hwnd.hBmpDCa[1], TRANSPARENT);
	SelectPalette(xid[xidno].hwnd.hBmpDCa[0], myPal, FALSE);
	RealizePalette(xid[xidno].hwnd.hBmpDCa[0]);
	SelectPalette(xid[xidno].hwnd.hBmpDCa[1], myPal, FALSE);
	RealizePalette(xid[xidno].hwnd.hBmpDCa[1]);
    } else {
	xid[xidno].hwnd.hBmp = hBmp = CreateCompatibleBitmap(hDC, cx, cy);
	xid[xidno].hwnd.hBmpDC = hBmpDC = CreateCompatibleDC(hDC);
	SelectObject(hBmpDC, hBmp);
    }
//      if (xidno == (int)draw)
//              WinXScaled(hBmpDC, cx, cy);



    SelectPalette(hBmpDC, myPal, FALSE);
    RealizePalette(hBmpDC);
    SetBkMode(hBmpDC, TRANSPARENT);
    r.left = 0;
    r.top = 0;
    r.right = cx;
    r.bottom = cy;
    FillRect(hBmpDC, &r, GetStockObject(BLACK_BRUSH));
    WinXSetupRadarWindow();
    ReleaseDC(hwnd, hDC);
}

BOOL ChangePalette(HWND hWnd)
{
    XID i;
    HDC hDC;
    HPALETTE hOldPal;

    for (i = 0; i < MAX_XIDS; i += 1) {
	if (xid[i].type == XIDTYPE_HWND) {
	    HWND hwnd = xid[i].hwnd.hWnd;

	    hDC = xid[i].hwnd.hBmpDC;
	    hOldPal = SelectPalette(hDC, myPal, FALSE);
	    RealizePalette(hDC);
	    SelectPalette(hDC, hOldPal, FALSE);

	    hDC = GetDC(hwnd);
	    hOldPal = SelectPalette(hDC, myPal, FALSE);
	    RealizePalette(hDC);
	    SelectPalette(hDC, hOldPal, FALSE);
	    ReleaseDC(hwnd, hDC);

	    InvalidateRect(hwnd, NULL, FALSE);
	}
    }

    return TRUE;
}

LRESULT CALLBACK WinXwindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
				LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
	{
	    LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
	    int xidno = (int) lpcs->lpCreateParams;

	    Trace("WM_CREATE %d %d/%d %s:%d\n", xidno, lpcs->cx, lpcs->cy,
		  xid[xidno].any.file, xid[xidno].any.line);
	    WinXCreateBitmapForXid(hwnd, xidno, lpcs->cx, lpcs->cy);
	    SetWindowWord(hwnd, 0, (WORD) xidno);
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

    case WM_DESTROY:
	{
	    XID xidno = (int) GetWindowWord(hwnd, 0);

	    if (Widget_window(motd_viewer) == xidno)
		Motd_destroy();
	    if (Widget_window(keys_viewer) == xidno)
		Keys_destroy();
	    WinXDeleteDraw(xidno);
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

    case WM_SIZE:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);
	    if (xidno > 0 && xidno < MAX_XIDS && xid[xidno].hwnd.hBmp) {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);
		Trace("WM_SIZE   %d %d/%d %s:%d\n", xidno, width, height,
		      xid[xidno].any.file, xid[xidno].any.line);
		WinXCreateBitmapForXid(hwnd, xidno, width, height);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_LBUTTONDOWN:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);
	    if (xid[xidno].hwnd.event_mask & ButtonPressMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;
		POINT pt;

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		MapWindowPoints(xid[xidno].hwnd.hWnd, xid[topWindow].hwnd.hWnd,
				&pt, 1);
		button->type = ButtonPress;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->x_root = pt.x;
		button->y_root = pt.y;
		button->button = Button1;
		win_xevent(event);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_LBUTTONUP:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);

	    if (xid[xidno].hwnd.event_mask & ButtonReleaseMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;

		Trace("ButtonUp in %d %s:%d\n", xidno, xid[xidno].any.file,
		      xid[xidno].any.line);
		button->type = ButtonRelease;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->button = Button1;
		if (win_xevent(event) == -1) {
		    WinXExit();
		}
		return (0);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_MBUTTONDOWN:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);
	    if (xid[xidno].hwnd.event_mask & ButtonPressMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;
		POINT pt;

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		MapWindowPoints(xid[xidno].hwnd.hWnd, xid[topWindow].hwnd.hWnd,
				&pt, 1);
		button->type = ButtonPress;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->x_root = pt.x;
		button->y_root = pt.y;
		button->button = Button2;
		win_xevent(event);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_MBUTTONUP:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);

	    if (xid[xidno].hwnd.event_mask & ButtonReleaseMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;

		Trace("ButtonUp in %d %s:%d\n", xidno, xid[xidno].any.file,
		      xid[xidno].any.line);
		button->type = ButtonRelease;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->button = Button2;
		if (win_xevent(event) == -1) {
		    WinXExit();
		}
		return (0);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_RBUTTONDOWN:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);
	    if (xid[xidno].hwnd.event_mask & ButtonPressMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;
		POINT pt;

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		MapWindowPoints(xid[xidno].hwnd.hWnd, xid[topWindow].hwnd.hWnd,
				&pt, 1);
		button->type = ButtonPress;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->x_root = pt.x;
		button->y_root = pt.y;
		button->button = Button3;
		win_xevent(event);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_RBUTTONUP:
	{
	    int xidno = (int) GetWindowWord(hwnd, 0);

	    if (xid[xidno].hwnd.event_mask & ButtonReleaseMask) {
		XEvent event;
		XButtonEvent *button = (XButtonEvent *) & event;

		Trace("ButtonUp in %d %s:%d\n", xidno, xid[xidno].any.file,
		      xid[xidno].any.line);
		button->type = ButtonRelease;
		button->window = xidno;
		button->x = LOWORD(lParam);
		button->y = HIWORD(lParam);
		button->button = Button3;
		if (win_xevent(event) == -1) {
		    WinXExit();
		}
		return (0);
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    case WM_MOUSEMOVE:
	{
	    XID xidno = (int) GetWindowWord(hwnd, 0);
	    XEvent event;
	    XID i;
	    XAnyEvent *enter = (XAnyEvent *) & event;

	    /* Trace("MouseMove in %d %d/%d %s:%d\n", xidno, 
	       LOWORD(lParam), HIWORD(lParam), xid[xidno].any.file, xid[xidno].any.line); */

	    enter->type = LeaveNotify;
	    for (i = 0; i < MAX_XIDS; i++) {
		if (i != xidno && xid[i].type == XIDTYPE_HWND
		    && xid[i].hwnd.mouseover
		    && xid[i].hwnd.event_mask & LeaveWindowMask) {
		    Trace("LeaveNotify %d %s:%d\n", xidno,
			  xid[xidno].any.file, xid[xidno].any.line);
		    enter->window = i;
		    win_xevent(event);
		    xid[i].hwnd.mouseover = FALSE;
		}
	    }
	    if (xid[xidno].hwnd.event_mask & PointerMotionMask) {
		XMotionEvent *me = (XMotionEvent *) & event;
		me->type = MotionNotify;
		me->window = xidno;
		me->x = LOWORD(lParam);
		me->y = HIWORD(lParam);
//                      if (me->x != draw_width/2 && me->y != draw_height/2)
		{
		    win_xevent(event);
//                              SetCursorPos(draw_width/2, draw_height/2);
		}
		//      return(0);
	    } else if (!xid[xidno].hwnd.mouseover) {	/* PointerMotionMask is only on captured window *//* so don't do the mouseover event */
		if (xid[xidno].hwnd.event_mask & EnterWindowMask) {
		    Trace("EnterNotify %d %s:%d\n", xidno,
			  xid[xidno].any.file, xid[xidno].any.line);
		    enter->type = EnterNotify;
		    enter->window = xidno;
		    win_xevent(event);
		}
		xid[xidno].hwnd.mouseover = TRUE;
	    }

	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

    case WM_ERASEBKGND:
	{
	    RECT rect;
	    if (GetClientRect(hwnd, &rect)) {
		HDC hBmpDC;
		XID xidno = (int) GetWindowWord(hwnd, 0);
		hBmpDC = xid[xidno].hwnd.hBmpDC;
		if (hBmpDC) {
		    Trace("WM_ERASEBKGND %d color=%d %d/%d %d/%d\n", xidno,
			  xid[xidno].hwnd.bgcolor, rect.left, rect.top,
			  rect.right, rect.bottom);
		    FillRect(hBmpDC, &rect,
			     (HBRUSH) WinXGetBrush(xid[xidno].hwnd.
						   bgcolor));
		}
	    }
	    return (0);
	}
    case WM_PAINT:
	{
	    RECT rect;
	    if (GetUpdateRect(hwnd, &rect, FALSE)) {
		XID xidno = (int) GetWindowWord(hwnd, 0);
//                      if (xidno == draw)
//                              return DefWindowProc(hwnd, uMsg, wParam, lParam);
		if (xidno >= 0 && xidno < MAX_XIDS) {
		    HDC hBmpDC;

		    hBmpDC = xid[xidno].hwnd.hBmpDC;
		    if (hBmpDC) {
			PAINTSTRUCT ps;
			HDC hDC;
			XEvent event;
			XExposeEvent *expose = (XExposeEvent *) & event;

			if (ThreadedDraw && xidno == (int) drawWindow) {
			    ValidateRect(hwnd, &rect);
			    winXTDraw(NULL, xidno, &rect);
			} else {
			    hDC = BeginPaint(hwnd, &ps);

			    if (xid[xidno].hwnd.event_mask & ExposureMask) {
				expose->type = Expose;
				expose->window = xidno;
				expose->x = rect.left;
				expose->y = rect.top;
				expose->width = rect.right - rect.left;
				expose->height = rect.bottom - rect.top;
				expose->count = 0;
				Trace("Expose %d %s:%d\n", xidno,
				      xid[xidno].any.file,
				      xid[xidno].any.line);
				win_xevent(event);
			    }

			    SelectPalette(hDC, myPal, FALSE);
			    RealizePalette(hDC);

			    if (xidno == (int) drawWindow) {
				//      RECT r;
				//      WinXUnscaled(hBmpDC);
				if (ThreadedDraw) {
				    ValidateRect(hwnd, &rect);
				    winXTDraw(hDC, xidno, &rect);
				} else {
				    BitBlt(hDC, rect.left, rect.top,
					   rect.right, rect.bottom, hBmpDC,
					   rect.left, rect.top, SRCCOPY);
				}
				//      GetClientRect(hwnd, &r);
				//      WinXScaled(hBmpDC, r.right - r.left, r.bottom - r.top);
				drawPending = FALSE;
			    } else {	/* not the main playfield window */
				BitBlt(hDC, rect.left, rect.top,
				       rect.right, rect.bottom, hBmpDC,
				       rect.left, rect.top, SRCCOPY);
			    }
			    EndPaint(hwnd, &ps);
			}
			return 0;
		    }
		}
	    }
	    return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
    default:
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

COLORREF WinXPColour(int ColourNo)
{
    return PALETTEINDEX(ColourNo);
}

static void InitWinXClass()
{
    WNDCLASS wc;

    // Fill in window class structure with parameters
    wc.style = 0;
    wc.lpfnWndProc = WinXwindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(WORD);	// For the xidno of the window
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "XPilotWin";

    RegisterClass(&wc);
}

void InitWinX(HWND hWnd)
{
    int i;
    HDC tDC = GetDC(hWnd);

    COLORREF cols[] = {
	RGB(0x00, 0x00, 0x00),
	RGB(0xFF, 0xFF, 0xFF),
	RGB(0x4E, 0x7C, 0xFF),
	RGB(0xFF, 0x3A, 0x27),
	RGB(0x33, 0xBB, 0x44),
	RGB(0x99, 0x22, 0x00),
	RGB(0xBB, 0x77, 0x00),
	RGB(0xEE, 0x99, 0x00),
	RGB(0x77, 0x00, 0x00),
	RGB(0xCC, 0x44, 0x00),
	RGB(0xDD, 0x88, 0x00),
	RGB(0xFF, 0xBB, 0x11),
	RGB(0x9F, 0x9F, 0x9F),
	RGB(0x5F, 0x5F, 0x5F),
	RGB(0xDF, 0xDF, 0xDF),
	RGB(0x20, 0x20, 0x20)
    };

    InitWinXClass();
    xid[0].hwnd.hWnd = hWnd;
    xid[0].type = XIDTYPE_HWND;

    itemsDC = NULL;

    bWinNT = GetVersion() & 0x80000000 ? FALSE : TRUE;

    maxColors = 16;

    myLogPal =
	malloc(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * WINMAXCOLORS);
    myLogPal->palVersion = 0x300;
    myLogPal->palNumEntries = maxColors;
    for (i = 0; i < maxColors; i++) {
	myLogPal->palPalEntry[i].peFlags = PC_RESERVED;
	myLogPal->palPalEntry[i].peRed = GetRValue(cols[i]);
	myLogPal->palPalEntry[i].peGreen = GetGValue(cols[i]);
	myLogPal->palPalEntry[i].peBlue = GetBValue(cols[i]);
    }
    myPal = CreatePalette(myLogPal);
    if (!myPal)
	error("Can't create palette");

    for (i = 0; i < NUM_ITEMS; i++) {
	itemBitmaps[i][ITEM_HUD] = XIDTYPE_UNUSED;
	itemBitmaps[i][ITEM_PLAYFIELD] = XIDTYPE_UNUSED;
    }
}
void WinXFree(XID i)
{
    switch (xid[i].type) {
    case XIDTYPE_FONT:
	free(xid[i].font.font);
	break;
    case XIDTYPE_PIXMAP:
	DeleteObject(xid[i].hpix.hbm);
	DeleteDC(xid[i].hpix.hDC);
	break;
    case XIDTYPE_HWND:
	WinXDeleteDraw(i);
	break;
    }
    xid[i].type = XIDTYPE_UNUSED;
}

void WinXShutdown()
{
    XID i;
    int j, k;

    free(myLogPal);
    for (i = 0; i < MAX_XIDS; i++) {
	WinXFree(i);
    }

    for (i = 0; i < WINMAXCOLORS; i++) {
	if (brushes[i])
	    DeleteObject(brushes[i]);

	for (j = 0; j < MAX_LINE_WIDTH; j++) {
	    for (k = 0; k < 3; k++) {
		if (pens[i][j][k])
		    DeleteObject(pens[i][j][k]);
	    }
	}
    }
}

void WinXSelectPen(int gc)
{
    XGCValues *xgcv;
    HDC hDC;
    HPEN hPen;

    xgcv = &xid[gc].hgc.xgcv;
    hDC = xid[xid[gc].hgc.xidhwnd].hwnd.hBmpDC;
    hPen = pens[xgcv->foreground][xgcv->line_width][xgcv->line_style];

    if (!hPen) {

	int styleMap[] = { PS_SOLID, PS_DOT, PS_DASHDOT };

	hPen = CreatePen(styleMap[xgcv->line_style],
			 xgcv->line_width, WinXPColour(xgcv->foreground));

	if (!hPen)
	    hPen = (HPEN) GetStockObject(WHITE_PEN);

	pens[xgcv->foreground][xgcv->line_width][xgcv->line_style] = hPen;
    }
    SelectObject(hDC, hPen);
}

HBRUSH WinXGetBrush(ULONG color)
{

    HBRUSH hBrush = brushes[color];

    if (!hBrush) {
	hBrush = CreateSolidBrush(WinXPColour(color));
	if (!hBrush)
	    hBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);
	brushes[color] = hBrush;
    }
    return hBrush;
}

void WinXSelectBrush(int gc)
{
    SelectObject(xid[xid[gc].hgc.xidhwnd].hwnd.hBmpDC,
		 WinXGetBrush(xid[gc].hgc.xgcv.foreground));
}


////////////////////////////////////////////////////////
// These are for SysInfo, we hide the details from him
HDC WinXGetDrawDC()
{
    return (GetDC(xid[drawWindow].hwnd.hWnd));
}

int WinXReleaseDrawDC(HDC hDC)
{
    return (ReleaseDC(xid[drawWindow].hwnd.hWnd, hDC));
}

////////////////////////////////////////////////////////
int WinXGetWindowRectangle(Window window, XRectangle * rect)
{
    RECT r;
    GetClientRect(xid[window].hwnd.hWnd, &r);
    rect->x = (short) WinXUnscale(r.left);
    rect->y = (short) WinXUnscale(r.top);
    rect->width = (unsigned short) WinXUnscale(r.right - r.left);
    rect->height = (unsigned short) WinXUnscale(r.bottom - r.top);
    return (1);
}

#if 0
void WinXSetBackColor(GC gc, unsigned long background)
{
    HDC hDC = xid[xid[gc].hgc.xidhwnd].hwnd.hBmpDC;
    SetBkColor(hDC, background);
    SetBkMode(hDC, TRANSPARENT);
}
#endif

// scarfed from a M$ KB article.  Apparently, Win95 doesn't support AngleArc
BOOL AngleArc2(HDC hdc, int X, int Y, DWORD dwRadius,
	       double fStartDegrees, double fSweepDegrees, BOOL bFilled)
{
    int iXStart, iYStart;	// End point of starting radial line
    int iXEnd, iYEnd;		// End point of ending radial line
    double fStartRadians;	// Start angle in radians
    double fEndRadians;		// End angle in radians
    BOOL bResult;		// Function result

    /* Get the starting and ending angle in radians */
    if (fSweepDegrees > 0.0) {
	fStartRadians = ((fStartDegrees / 360.0) * fTwoPi);
	fEndRadians = (((fStartDegrees + fSweepDegrees) / 360.0) * fTwoPi);
    } else {
	fStartRadians =
	    (((fStartDegrees + fSweepDegrees) / 360.0) * fTwoPi);
	fEndRadians = ((fStartDegrees / 360.0) * fTwoPi);
    }

    /* Calculate a point on the starting radial line via */
    /* polar -> cartesian conversion */
    iXStart = X + (int) ((double) dwRadius * (double) cos(fStartRadians));
    iYStart = Y - (int) ((double) dwRadius * (double) sin(fStartRadians));

    /* Calculate a point on the ending radial line via */
    /* polar -> cartesian conversion */
    iXEnd = X + (int) ((double) dwRadius * (double) cos(fEndRadians));
    iYEnd = Y - (int) ((double) dwRadius * (double) sin(fEndRadians));

    /* Draw a line to the starting point */
    LineTo(hdc, iXStart, iYStart);

    /* Draw the arc */
    if (bFilled)
	bResult = Pie(hdc, X - dwRadius, Y - dwRadius,
		      X + dwRadius, Y + dwRadius,
		      iXStart, iYStart, iXEnd, iYEnd);
    else
	bResult = Arc(hdc, X - dwRadius, Y - dwRadius,
		      X + dwRadius, Y + dwRadius,
		      iXStart, iYStart, iXEnd, iYEnd);

    /* Move to the ending point - Arc() wont do this and ArcTo() */
    /* wont work on Win32s or Win16 */
    MoveToEx(hdc, iXEnd, iYEnd, NULL);

    return bResult;
}

void WinXParseFont(LOGFONT * lf, const char *name)
{
#define	MAX_FFLDS	14
    static char sepa[] = "-\n\r";
    char *t[MAX_FFLDS];
    char *s = malloc(strlen(name) + 1);
    int i;

    strcpy(s, name);

    t[0] = strtok(s, sepa);
    for (i = 1; i < MAX_FFLDS; i++)
	t[i] = strtok(NULL, sepa);
//      lf->lfHeight = atoi(t[6]) * 100 / iScaleFactor;
//      lf->lfHeight = (int)(atoi(t[6]) / scaleFactor);
    lf->lfHeight = (int) (atoi(t[6]));
    if (!lf->lfHeight)
	lf->lfHeight = 14;
    lf->lfWeight = *t[2] == 'b' ? FW_BOLD : FW_NORMAL;
    lf->lfItalic = *t[3] == 'i' ? TRUE : FALSE;
    switch (*t[1]) {
    case 't':			/* times */
	lf->lfPitchAndFamily = FF_ROMAN;
	break;
    case 'f':			/* fixed */
	lf->lfPitchAndFamily = FIXED_PITCH;
	break;
    case 'c':			/* courier */
	lf->lfPitchAndFamily = FF_MODERN;
	break;
    }
    free(s);
#undef	MAX_FFLDS
}

XFontStruct *WinXLoadFont(const char *name)
{
    XID txid;
    XFontStruct *fs = malloc(sizeof(XFontStruct));
    Trace("WinXLoadFont: creating font <%s>\n", name);
    memset(fs, 0, sizeof(XFontStruct));
    WinXParseFont(&fs->lf, name);
//      fs->ascent = fs->lf.lfHeight * 100 / iScaleFactor;
//      fs->ascent = (int)(fs->lf.lfHeight / scaleFactor);
    fs->ascent = (int) (fs->lf.lfHeight);
    fs->hFont = CreateFontIndirect(&fs->lf);

    txid = GetFreeXid();
    xid[txid].type = XIDTYPE_FONT;
    xid[txid].font.font = fs;
    fs->fid = txid;
    return (fs);
}


XParseColor(Display * display, Colormap colormap, char *spec,
	    XColor * exact_def_return)
{
    Trace("Parsing color <%s>\n", spec);
    return (0);
}

Pixmap WinXCreateBitmapFromData(Display * dpy, Drawable d, char *data,
				unsigned int width, unsigned int height,
				int color)
{
    HBITMAP hbm;
    int i;
    int j;
    WORD *e;

    BITMAP bm = {
	0,			//   LONG   bmType; 
	16,			//   LONG   bmWidth; 
	16,			//   LONG   bmHeight; 
	4,			//   LONG   bmWidthBytes; 
	1,			//   WORD   bmPlanes; 
	1,			//   WORD   bmBitsPixel; 
	NULL			//   LPVOID bmBits; 
    };
    RECT rect = { 0, 0, 16, 16 };

    HDC hDC = GetDC(xid[d].hwnd.hWnd);
    HDC hDCb = CreateCompatibleDC(hDC);

    hbm = CreateCompatibleBitmap(hDC, width, height);
    SelectObject(hDCb, hbm);
    SelectPalette(hDCb, myPal, FALSE);
    RealizePalette(hDCb);

    FillRect(hDCb, &rect, GetStockObject(BLACK_BRUSH));
    if (!hbm)
	error("Can't create item bitmaps");
    if (width != 16 || height != 16)
	error("Can only create 16x16 bitmaps");
    e = (WORD *) data;
    for (i = 0; i < 16; i++) {
	WORD w = *e++;
	WORD z = 0;
	for (j = 0; j < 16; j++)	// swap the bits in the bytes
	    if (w & (1 << j))
		SetPixelV(hDCb, j, i, WinXPColour(color));
    }

    DeleteDC(hDCb);
    ReleaseDC(xid[d].hwnd.hWnd, hDC);
    return ((Pixmap) hbm);
}

XResizeWindow(Display * dpy, Window w, unsigned int width,
	      unsigned int height)
{
    HWND hWnd = xid[w].hwnd.hWnd;

    SetWindowPos(hWnd, NULL, 0, 0, width, height,
		 SWP_NOMOVE | SWP_NOZORDER);
    return (0);
}

void WinXResize(void)
{
    RECT rect;

    draw_width = WinXUnscale(draw_width);
    draw_height = WinXUnscale(draw_height);

    if (radarWindow && (instruments.slidingRadar)) {
	GetClientRect(xid[radarWindow].hwnd.hWnd, &rect);
	InvalidateRect(xid[radarWindow].hwnd.hWnd, &rect, FALSE);
    }
    if (drawWindow) {
	GetClientRect(xid[drawWindow].hwnd.hWnd, &rect);
	InvalidateRect(xid[drawWindow].hwnd.hWnd, &rect, FALSE);
    }
    if (playersWindow) {
	GetClientRect(xid[playersWindow].hwnd.hWnd, &rect);
	InvalidateRect(xid[playersWindow].hwnd.hWnd, &rect, FALSE);
    }
}

void PaintWinClient()
{
    RECT rect;
    static int updates = 0;
    if (!itemsDC)
	itemsDC = CreateCompatibleDC(NULL);

    WinXSetupRadarWindow();

    // One time stuff for score window update
    if (updates == 0) {
	GetClientRect(xid[playersWindow].hwnd.hWnd, &rect);
	InvalidateRect(xid[playersWindow].hwnd.hWnd, &rect, FALSE);
	UpdateWindow(xid[playersWindow].hwnd.hWnd);
    }
    updates += 1;
//      SelectPalette(hDC, myPal, FALSE);
//      RealizePalette(hDC);

//      xid[draw].hwnd.hDC = hDC;
//      if (!itemsDC)
//              itemsDC = CreateCompatibleDC(realDC);

//      Paint_frame();
//      xid[draw].hwnd.hBmpDC = realDC;
    GetClientRect(xid[drawWindow].hwnd.hWnd, &rect);
    if (ThreadedDraw) {
//              FillRect(xid[draw].hwnd.hBmpDC, &rect, GetStockObject(WHITE_BRUSH));
	winXTDraw(NULL, drawWindow, &rect);
    } else {
	HDC realDC = GetDC(xid[drawWindow].hwnd.hWnd);
	SelectPalette(realDC, myPal, FALSE);
	RealizePalette(realDC);
	BitBlt(realDC, 0, 0, rect.right, rect.bottom,
	       xid[drawWindow].hwnd.hBmpDC, 0, 0, SRCCOPY);
	ReleaseDC(xid[drawWindow].hwnd.hWnd, realDC);
    }
}

void MarkPlayersForRedraw()
{
    RECT rect;

    GetClientRect(xid[playersWindow].hwnd.hWnd, &rect);
    InvalidateRect(xid[playersWindow].hwnd.hWnd, &rect, FALSE);
    UpdateWindow(xid[playersWindow].hwnd.hWnd);
}

void paintItemSymbol(unsigned char type, Drawable d, GC gc, int x, int y,
		     int color)
{
    HDC hDC = xid[d].hwnd.hBmpDC;

    SelectObject(itemsDC, (HBITMAP) itemBitmaps[type][color]);
    SelectPalette(itemsDC, myPal, FALSE);
    RealizePalette(itemsDC);
    BitBlt(hDC, x, y, 16, 16, itemsDC, 0, 0, SRCPAINT);
}

void WinXBltPixToWin(Pixmap src, Window dest,
		     int src_x, int src_y, unsigned int width,
		     unsigned int height, int dest_x, int dest_y)
{
    HDC shDC = xid[src].hpix.hDC;
    HDC hDC = xid[dest].hwnd.hBmpDC;

    BitBlt(hDC, dest_x, dest_y, width, height, shDC, src_x, src_y,
	   SRCCOPY);
}

void WinXBltWinToPix(Window src, Pixmap dest,
		     int src_x, int src_y, unsigned int width,
		     unsigned int height, int dest_x, int dest_y)
{
    HDC hDC = xid[src].hwnd.hBmpDC;
    HBITMAP hbm = xid[dest].hpix.hbm;

    HDC hDCd = CreateCompatibleDC(NULL);
    int ret;
    SelectObject(hDC, hbm);
    SelectPalette(hDC, myPal, FALSE);
    RealizePalette(hDC);
    ret =
	BitBlt(hDC, dest_x, dest_y, width, height, hDC, src_x, src_y,
	       BLACKNESS);
    DeleteDC(hDCd);

}

#if 0
void WinXPaintPlayers()
{
}
#endif

void WinXFlush(Window w)
{
    RECT r;
    GetClientRect(xid[w].hwnd.hWnd, &r);
    Trace("Flushing %d (%d/%d %d/%d)\n", w, r.left, r.top, r.right,
	  r.bottom);
    InvalidateRect(xid[w].hwnd.hWnd, &r, TRUE);
}

void WinXExit()
{
    PostMessage(GetParent(xid[topWindow].hwnd.hWnd), WM_CLOSE, 0, 0);
}

void WinXSetEventMask(Window w, long mask)
{
    xid[w].hwnd.event_mask = mask;	/* this could be a macro, */
}				/* but winX_.h is hidden from everyone */

Window WinXGetParent(Window w)
{
    XID i;
    XID txid;
    HWND hwnd = GetParent(xid[w].hwnd.hWnd);
    if (!hwnd)
	return (topWindow);
    for (i = 0; i < MAX_XIDS; i++) {
	if (hwnd == xid[i].hwnd.hWnd)
	    return (i);
    }
/* create a new "Window" for the parent */
    txid = GetFreeXid();
    xid[txid].hwnd.hBmp = NULL;
    xid[txid].hwnd.hBmpDC = NULL;
    xid[txid].hwnd.hWnd = hwnd;
    xid[txid].hwnd.event_mask = 0;
    xid[txid].hwnd.event_mask = -1;	// hell, let's take em all!
    xid[txid].hwnd.mouseover = 0;	// mouse not over this window
    xid[txid].hwnd.type = XIDTYPE_HWND;
    xid[txid].hwnd.notmine = TRUE;	// we don't destroy this one...
#if 0
    if (++max_xid > MAX_XIDS) {
	error("Too many XIDS!\n");
	max_xid--;
    }
    return (max_xid - 1);
#endif
    return (txid);
}

BOOL WinXGetWindowRect(Window w, RECT * rect)
{
    return (GetWindowRect(xid[w].hwnd.hWnd, rect));
}

BOOL WinXGetWindowPlacement(Window w, WINDOWPLACEMENT * wp)
{
    return (GetWindowPlacement(xid[w].hwnd.hWnd, wp));
}

void WinXParseGeometry(const char* g, int *w, int *h)
{
	char *s = g;
	*w = *h = -1;
	if (g) {
		if (g[0] == '=') s++;
		sscanf(s, "%d%*c%d", w, h);
	}
	if (*w == -1) *w = 1024;
	if (*h == -1) *h = 768;
}

XID GetFreeXid()
{
    int i;
    for (i = 0; i < MAX_XIDS; i++)
	if (xid[i].type == XIDTYPE_UNUSED)
	    return (i);
    error("No Free XIDs");
    return (MAX_XIDS);
}

/*------------------------------------------------------*\
* stubs for motd and talk window.  I wrote these Windows *
* dialogs differently... I should revisit these to use   *
* these new APIs.                                        *
\*------------------------------------------------------*/
bool talk_mapped = FALSE;
void Talk_resize(void)
{
}

int Talk_do_event(XEvent * event)
{
    return (FALSE);
}
