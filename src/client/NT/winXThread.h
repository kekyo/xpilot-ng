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
*  winXThread.h - X11 to Windoze converter									*
*																			*
*  XPilot specific:															*
*  This module is an attempt at running the BitBlt in another thread.		*
*																			*
*  					*
\***************************************************************************/

#ifndef WINXTHREAD_H
#define WINXTHREAD_H

typedef struct {
    HDC hDC;
    HDC hBmpDC;
    RECT rect;
    HWND hWnd;
    Window w;
    int isDrawing;
    HPALETTE myPal;


    HANDLE eventDraw;
    HANDLE eventNotDrawing;
    HANDLE eventKillServerThread;
    HANDLE eventServerThreadKilled;
} _dinfo;

extern _dinfo dinfo;

extern void winXTDraw(HDC hDC, Window xidno, RECT * rect);

#endif
