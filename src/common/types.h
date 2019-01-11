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

#ifndef	TYPES_H
#define	TYPES_H

#ifndef	_WINDOWS
typedef signed char	byte;
#endif
typedef unsigned char	u_byte;

/*
 * On some systems an enum is smaller than an int.
 * On others bool is already a builtin type.
 * Using preprocessor macros to circumvent both situations.
 */
#ifndef __cplusplus
# define false	0
# define true	1
# define bool	char
#endif

typedef struct { float x, y; }		vector_t;
typedef vector_t			position_t;
typedef struct { int x, y; }		ivec_t;
typedef ivec_t				ipos_t;
typedef struct { int x, y, w, h;}	irec_t;
typedef struct { int bx, by; }		blkvec_t;
typedef blkvec_t			blkpos_t;

#ifdef _WINDOWS
# define strncasecmp(__s, __t, __l)	strnicmp(__s, __t, __l)
# define strcasecmp(__s, __t)	stricmp(__s, __t)
#endif

#endif
