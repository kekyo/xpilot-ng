/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2003-2004 by
 *
 *      Uoti Urpala          <uau@users.sourceforge.net>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#ifndef SETUP_H
#define SETUP_H

/*
 * Definitions to tell the client how the server has been setup.
 */

/*
 * If the high bit of a map block is set then the next block holds
 * the number of contiguous map blocks that have the same block type.
 */
#define SETUP_COMPRESSED	0x80

/*
 * Tell the client how and if the map is compressed.
 * This is only for client compatibility with the old protocol.
 */
#define SETUP_MAP_ORDER_XY	1
#define SETUP_MAP_ORDER_YX	2
#define SETUP_MAP_UNCOMPRESSED	3

/*
 * Definitions for the map layout which permit a compact definition
 * of map data.
 */
#define SETUP_SPACE		0
#define SETUP_FILLED		1
#define SETUP_FILLED_NO_DRAW	2
#define SETUP_FUEL		3
#define SETUP_REC_RU		4
#define SETUP_REC_RD		5
#define SETUP_REC_LU		6
#define SETUP_REC_LD		7
#define SETUP_ACWISE_GRAV	8
#define SETUP_CWISE_GRAV	9
#define SETUP_POS_GRAV		10
#define SETUP_NEG_GRAV		11
#define SETUP_WORM_NORMAL	12
#define SETUP_WORM_IN		13
#define SETUP_WORM_OUT		14
#define SETUP_CANNON_UP		15
#define SETUP_CANNON_RIGHT	16
#define SETUP_CANNON_DOWN	17
#define SETUP_CANNON_LEFT	18
#define SETUP_SPACE_DOT		19
#define SETUP_TREASURE		20	/* + team number (10) */
#define SETUP_BASE_LOWEST	30	/* lowest base number */
#define SETUP_BASE_UP		30	/* + team number (10) */
#define SETUP_BASE_RIGHT	40	/* + team number (10) */
#define SETUP_BASE_DOWN		50	/* + team number (10) */
#define SETUP_BASE_LEFT		60	/* + team number (10) */
#define SETUP_BASE_HIGHEST	69	/* highest base number */
#define SETUP_TARGET		70	/* + team number (10) */
#define SETUP_CHECK		80	/* + check point number (26) */
#define SETUP_ITEM_CONCENTRATOR	110
#define SETUP_DECOR_FILLED	111
#define SETUP_DECOR_RU		112
#define SETUP_DECOR_RD		113
#define SETUP_DECOR_LU		114
#define SETUP_DECOR_LD		115
#define SETUP_DECOR_DOT_FILLED	116
#define SETUP_DECOR_DOT_RU	117
#define SETUP_DECOR_DOT_RD	118
#define SETUP_DECOR_DOT_LU	119
#define SETUP_DECOR_DOT_LD	120
#define SETUP_UP_GRAV		121
#define SETUP_DOWN_GRAV		122
#define SETUP_RIGHT_GRAV	123
#define SETUP_LEFT_GRAV		124
#define SETUP_ASTEROID_CONCENTRATOR	125

#define BLUE_UP			0x01
#define BLUE_RIGHT		0x02
#define BLUE_DOWN		0x04
#define BLUE_LEFT		0x08
#define BLUE_OPEN		0x10	/* diagonal botleft -> rightup */
#define BLUE_CLOSED		0x20	/* diagonal topleft -> rightdown */
#define BLUE_FUEL		0x30	/* when filled block is fuelstation */
#define BLUE_BELOW		0x40	/* when triangle is below diagonal */
#define BLUE_BIT		0x80	/* set when drawn with blue lines */

#define DECOR_LEFT		0x01
#define DECOR_RIGHT		0x02
#define DECOR_DOWN		0x04
#define DECOR_UP		0x08
#define DECOR_OPEN		0x10
#define DECOR_CLOSED		0x20
#define DECOR_BELOW		0x40

/*
 * Structure defining the server configuration, including the map layout.
 */
typedef struct {
    long		setup_size;		/* size including map data */
    long		map_data_len;		/* num. compressed map bytes */
    long		mode;			/* playing mode */
    short		lives;			/* max. number of lives */
    short		x;			/* OLD width in blocks */
    short		y;			/* OLD height in blocks */
    short		width;			/* width in pixels */
    short		height;			/* height in pixels */
    short		frames_per_second;	/* FPS */
    short		map_order;		/* OLD row or col major */
    short		unused1;		/* padding */
    char		name[MAX_CHARS];	/* name of map */
    char		author[MAX_CHARS];	/* name of author of map */
    char		data_url[MSG_LEN];	/* location where client
						   can load additional data
						   like bitmaps; MSG_LEN to
						   allow >80 chars */
    unsigned char	map_data[4];		/* compressed map data */
    /* plus more mapdata here (HACK) */
} setup_t;

#ifndef SERVER
# ifdef FPS
#  error "FPS needs a different definition in the client"
#  undef FPS
# endif
# define FPS		(Setup->frames_per_second)

extern setup_t *Setup;

#endif

#endif

