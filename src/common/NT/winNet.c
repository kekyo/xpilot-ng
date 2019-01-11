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

/***************************************************************\
*  winNet.c - X11 to Windoze converter							*
\***************************************************************/

#include <windows.h>
#include "../../common/error.h"
//#include "winServer.h"
extern void _Trace(char *lpszFormat, ...);

static int winalarm;
HWND alarmWnd;
HWND notifyWnd;			/* Our parent's window */

long alarm(long seconds, void (__cdecl * func) (int))
{
#if 0
    if (!seconds) {
	KillTimer(alarmWnd, winalarm);
    } else {
	winalarm = SetTimer(alarmWnd, 0, seconds * 1000, (TIMERPROC) func);
	Trace("Winalarm=%d\n", winalarm);
    }
#endif
    return ((long) winalarm);
}
