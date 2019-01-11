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

#ifndef RECORDFMT_H
#define RECORDFMT_H

/*
 * Protocol version history:
 * 0.0: first protocol implementation.
 * 0.1: addition of tiled fills.
 */
#define RC_MAJORVERSION		'0'
#define RC_MINORVERSION		'1'

#define RC_NEWFRAME		11
#define RC_DRAWARC		12
#define RC_DRAWLINES		13
#define RC_DRAWLINE		14
#define RC_DRAWRECTANGLE	15
#define RC_DRAWSTRING		16
#define RC_FILLARC		17
#define RC_FILLPOLYGON		18
#define RC_PAINTITEMSYMBOL	19
#define RC_FILLRECTANGLE	20
#define RC_ENDFRAME		21
#define RC_FILLRECTANGLES	22
#define RC_DRAWARCS		23
#define RC_DRAWSEGMENTS		24
#define RC_GC			25
#define RC_NOGC			26
#define RC_DAMAGED		27
#define RC_TILE			28
#define RC_NEW_TILE		29

#define RC_GC_FG		(1 << 0)
#define RC_GC_BG		(1 << 1)
#define RC_GC_LW		(1 << 2)
#define RC_GC_LS		(1 << 3)
#define RC_GC_DO		(1 << 4)
#define RC_GC_FU		(1 << 5)
#define RC_GC_DA		(1 << 6)
#define RC_GC_B2		(1 << 7)
#define RC_GC_FS		(1 << 8)
#define RC_GC_XO		(1 << 9)
#define RC_GC_YO		(1 << 10)
#define RC_GC_TI		(1 << 11)

#endif
