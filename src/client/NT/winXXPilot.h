/* 
 *
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
*  winXPilot.h - X11 to Windoze converter									*
*																			*
*  This file contains the private winX definitions for the XPilot client	*
*																			*
*  							*
\***************************************************************************/
#ifndef	_WINXPILOT_H_
#define	_WINXPILOT_H_

#ifdef	_WINDOWS

#ifndef	_INC_WINDOWS
#include <windows.h>
#endif

#include "../../common/NT/winX.h"

#ifdef	__cplusplus
extern "C" {
#endif


    extern int WinXGetWindowRectangle(Window window, XRectangle * rect);

#ifdef	_DEBUG
#define	WinXCreateWinDC(__w) \
	WinXCreateWinDC_(__w, __FILE__, __LINE__)

    extern WinXCreateWinDC_(Window w, const char *file, const int line);
#else
#define	WinXCreateWinDC(__w) \
	WinXCreateWinDC_(__w)
    extern WinXCreateWinDC_(Window w);
#endif

    extern void WinXExit();
    extern void WinXFlush(Window w);
    extern void WinXSetEventMask(Window w, long mask);
/* extern	void WinXSetBackColor(GC gc, unsigned long background); */
/* extern	void WinXClearWindow(GC gc); */
/* extern	void WinXGetDrawRect(RECT* rect); */
    extern void WinXBltPixToWin(Pixmap src, Window dest,
				int src_x, int src_y, unsigned int width,
				unsigned int height, int dest_x,
				int dest_y);
    extern void WinXBltWinToPix(Window src, Pixmap dest, int src_x,
				int src_y, unsigned int width,
				unsigned int height, int dest_x,
				int dest_y);
    extern Pixmap WinXGetRadarBitmap(int width, int height);
/* extern	WinXSetEvent(Window w, int message, (void func)());
extern	void WinXPaintPlayers(); */

/* used for creating item bitmaps */
    extern Pixmap WinXCreateBitmapFromData(Display * dpy, Drawable d,
					   char *data, unsigned int width,
					   unsigned int height, int color);

    extern Window WinXGetParent(Window w);
    extern BOOL WinXGetWindowRect(Window w, RECT * rect);
    extern BOOL WinXGetWindowPlacement(Window w, WINDOWPLACEMENT * wp);

    extern void WinXResize(void);

#ifdef _XPDOC
    extern void Resize(Window w, int width, int height);
    extern BOOL ChangePalette(HWND hwnd);
    extern Window top;
#endif
    extern BOOL drawPending;	// try to throttle the deadly frame backup syndrome

// Windows config options
/* extern int iScaleFactor; */
    extern int RadarDivisor;
    extern int ThreadedDraw;


// temp until the new WinMotd (using the motd api) comes along 
    extern int Startup_server_motd(void);

#ifdef	__cplusplus
};
#endif


#endif				/* _WINDOWS */
#endif				/* _WINXPILOT_H_ */
