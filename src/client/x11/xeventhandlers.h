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

#ifndef XEVENTHANDLERS_H
#define XEVENTHANDLERS_H

/* avoid trouble with Atoms and 64 bit archs */
typedef CARD32  Atom32;

void SelectionNotify_event(XEvent *event);
void SelectionRequest_event(XEvent *event);
void MapNotify_event(XEvent *event);
int ClientMessage_event(XEvent *event);
void FocusIn_event(XEvent *event);
void UnmapNotify_event(XEvent *event);
void ConfigureNotify_event(XEvent *event);
void Expose_event(XEvent *event);
void KeyChanged_event(XEvent *event);
void ButtonPress_event(XEvent *xevent);
void MotionNotify_event(XEvent *event);
int ButtonRelease_event(XEvent *event);
#endif
