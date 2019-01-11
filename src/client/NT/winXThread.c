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
*  winXThread.c - X11 to Windoze converter									*
*																			*
*  XPilot specific:															*
*  This module is an attempt at running the BitBlt in another thread.		*
*																			*
*  					*
\***************************************************************************/

#include "../../common/NT/winX.h"
#include "../../common/NT/winX_.h"
#include "winXThread.h"
#include "../../common/error.h"

HANDLE dthread = NULL;
_dinfo dinfo;

DWORD tid;			// threadID

DWORD WINAPI DrawThreadProc(LPVOID data);

void winXTDraw(HDC hDCx, Window w, RECT * rect)
{

    if (!dthread) {
	dinfo.eventDraw = CreateEvent(NULL, FALSE, FALSE, NULL);
	dinfo.eventNotDrawing = CreateEvent(NULL, TRUE, TRUE, NULL);	// manual reset, initially set
	dinfo.eventKillServerThread = CreateEvent(NULL, FALSE, FALSE, NULL);	// auto reset, initially reset
	dinfo.eventServerThreadKilled = CreateEvent(NULL, FALSE, FALSE, NULL);	// auto reset, initially reset
	dinfo.isDrawing = FALSE;
	dinfo.hDC = GetDC(xid[w].hwnd.hWnd);

	dthread =
	    CreateThread(NULL, 16000, DrawThreadProc, &dinfo, 0, &tid);
	SetThreadPriority(dthread, THREAD_PRIORITY_BELOW_NORMAL);
//              SetThreadPriority(dthread, THREAD_PRIORITY_HIGHEST);

    }
    if (WaitForSingleObject(dinfo.eventNotDrawing, 0)	// 1 second timeout
	== WAIT_OBJECT_0) {
//              dinfo.hDC       = hDC;
	dinfo.w = w;
	dinfo.hWnd = xid[w].hwnd.hWnd;
	dinfo.rect = *rect;
	dinfo.hBmpDC = xid[w].hwnd.hBmpDCa[xid[w].hwnd.filling];
	dinfo.myPal = myPal;
	xid[w].hwnd.filling ^= 1;
	xid[w].hwnd.hBmpDC = xid[w].hwnd.hBmpDCa[xid[w].hwnd.filling];
	xid[w].hwnd.hBmp = xid[w].hwnd.hBmpa[xid[w].hwnd.filling];

//              SelectPalette(xid[w].hwnd.hBmpDC, myPal, FALSE);
//              RealizePalette(xid[w].hwnd.hBmpDC);

	SetEvent(dinfo.eventDraw);
    } else {
//              error("Draw Thread is dead");
	// redraw this frame
    }
}

int serverkilled;

DWORD WINAPI DrawThreadProc(LPVOID data)
{
    _dinfo *d = (_dinfo *) data;
    BOOL ServerKilled = FALSE;
    HDC myDC;

    // This draw thread runs in an infinite loop, waiting to recalculate the
    // result whenever the main application thread sets the "start recalc" event.
    // The recalc thread exits the loop only when the main application sets the
    // "kill recalc" event.

    while (TRUE) {

	// Wait until the main application thread asks this thread to do
	//      something
	if (WaitForSingleObject(d->eventDraw, INFINITE)
	    != WAIT_OBJECT_0)
	    break;
//              d->isDrawing = TRUE;

	// Exit the thread if the main application sets the "kill server"
	// event. The main application will set the "kill server" event
	// before setting the "draw" event.

	if (WaitForSingleObject(d->eventKillServerThread, 0)
	    == WAIT_OBJECT_0)
	    break;		// Terminate this thread by existing the proc.

	// Reset event to indicate "not done", that is, game is in progress.
//              ResetEvent(d->m_hEventGameTerminated);
	ResetEvent(d->eventNotDrawing);
//              main(pServerInfo->argc, pServerInfo->argv);
	myDC = GetDC(d->hWnd);
	SelectPalette(myDC, d->myPal, FALSE);
	RealizePalette(myDC);
//              BitBlt(d->hDC, d->rect.left, d->rect.top, d->rect.right, d->rect.bottom,
	BitBlt(myDC, d->rect.left, d->rect.top, d->rect.right,
	       d->rect.bottom, d->hBmpDC, d->rect.left, d->rect.top,
	       SRCCOPY);
//              if (xid[d->w].hwnd.filling)
//                      FillRect(myDC, &d->rect, GetStockObject(WHITE_BRUSH));
//              else
//                      FillRect(myDC, &d->rect, GetStockObject(DKGRAY_BRUSH));
	ReleaseDC(d->hWnd, myDC);
	SetEvent(d->eventNotDrawing);

	// Sleep(10*1000);                      // debug
	// Set event to indicate that drawing is done (i.e., no longer in progres),
	// even if perhaps interrupted by "kill recalc" event detected in the SlowAdd function.
//              SetEvent(pServerInfo->m_hEventGameTerminated);

	if (ServerKilled)	// If interrupted by kill then...
	    break;		// terminate this thread by exiting the proc.

	//PostMessage(pServerInfo->m_hwndNotifyRecalcDone,
	//      WM_USER_RECALC_DONE, 0, 0);
    }

    if (ServerKilled)
	SetEvent(d->eventServerThreadKilled);
    return 0;
}
