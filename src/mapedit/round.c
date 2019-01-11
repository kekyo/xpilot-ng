/*
 * XPilot NG XP-MapEdit, a map editor for xp maps.  Copyright (C) 1993 by
 *
 *      Aaron Averill           <averila@oes.orst.edu>
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
 *
 * Modifications:
 * 1996:
 *      Robert Templeman        <mbcaprt@mphhpd.ph.man.ac.uk>
 * 1997:
 *      William Docter          <wad2@lehigh.edu>
 */

#include "xpmapedit.h"

/*
 * Completely rewritten/redesigned by Bert Gijsbers.
 * First try to do smoothing by adding blocks.
 * If this doesn't change anything, then try to smooth by changing blocks.
 * If this is exhausted too then try to smooth by removing blocks.
 * The idea is that the user can do several stages of smoothing
 * by pressing the Round button more than once.
 */
int RoundMapArea(HandlerInfo_t info)
{
    int x, y, xmax, ymax, mapcursorx, mapcursory, type, mask, change = 0;
    unsigned char up_filled[256],
	left_filled[256],
	down_filled[256],
	right_filled[256],
	lu_filled[256], ld_filled[256], rd_filled[256], ru_filled[256];

    DrawSelectArea();
    ClearUndo();
    if (selectfrom_x < 0) {	/* no area selected, do entire screen */
	mapcursorx = map.view_x;
	mapcursory = map.view_y;
	xmax = map.view_x + (mapwin_width - TOOLSWIDTH) / map.view_zoom;
	ymax = map.view_y + mapwin_height / map.view_zoom;
    } else {
	if (selectfrom_x < selectto_x) {
	    mapcursorx = selectfrom_x + map.view_x;
	    xmax = selectto_x + 1 + map.view_x;
	} else {
	    mapcursorx = selectto_x + map.view_x;
	    xmax = selectfrom_x + 1 + map.view_x;
	}
	if (selectfrom_y < selectto_y) {
	    mapcursory = selectfrom_y + map.view_y;
	    ymax = selectto_y + 1 + map.view_y;
	} else {
	    mapcursory = selectto_y + map.view_y;
	    ymax = selectfrom_y + 1 + map.view_y;
	}
    }

    memset(up_filled, 0, sizeof up_filled);
    memset(left_filled, 0, sizeof left_filled);
    memset(down_filled, 0, sizeof down_filled);
    memset(right_filled, 0, sizeof right_filled);
    memset(lu_filled, 0, sizeof lu_filled);
    memset(ld_filled, 0, sizeof ld_filled);
    memset(rd_filled, 0, sizeof rd_filled);
    memset(ru_filled, 0, sizeof ru_filled);

    up_filled[XPMAP_FILLED] = 1;
    up_filled[XPMAP_FUEL] = 1;
    up_filled[XPMAP_REC_RD] = 1;
    up_filled[XPMAP_REC_LD] = 1;

    left_filled[XPMAP_FILLED] = 1;
    left_filled[XPMAP_FUEL] = 1;
    left_filled[XPMAP_REC_RU] = 1;
    left_filled[XPMAP_REC_RD] = 1;

    down_filled[XPMAP_FILLED] = 1;
    down_filled[XPMAP_FUEL] = 1;
    down_filled[XPMAP_REC_RU] = 1;
    down_filled[XPMAP_REC_LU] = 1;

    right_filled[XPMAP_FILLED] = 1;
    right_filled[XPMAP_FUEL] = 1;
    right_filled[XPMAP_REC_LD] = 1;
    right_filled[XPMAP_REC_LU] = 1;

    lu_filled[XPMAP_FILLED] = 1;
    lu_filled[XPMAP_FUEL] = 1;
    lu_filled[XPMAP_REC_LD] = 1;
    lu_filled[XPMAP_REC_RD] = 1;
    lu_filled[XPMAP_REC_RU] = 1;

    ld_filled[XPMAP_FILLED] = 1;
    ld_filled[XPMAP_FUEL] = 1;
    ld_filled[XPMAP_REC_LU] = 1;
    ld_filled[XPMAP_REC_RU] = 1;
    ld_filled[XPMAP_REC_RD] = 1;

    rd_filled[XPMAP_FILLED] = 1;
    rd_filled[XPMAP_FUEL] = 1;
    rd_filled[XPMAP_REC_RU] = 1;
    rd_filled[XPMAP_REC_LU] = 1;
    rd_filled[XPMAP_REC_LD] = 1;

    ru_filled[XPMAP_FILLED] = 1;
    ru_filled[XPMAP_FUEL] = 1;
    ru_filled[XPMAP_REC_RD] = 1;
    ru_filled[XPMAP_REC_LD] = 1;
    ru_filled[XPMAP_REC_LU] = 1;

#define UP_FILLED       (up_filled[MapData(x,y-1) & 0xFF] == 1)
#define LEFT_FILLED     (left_filled[MapData(x-1,y) & 0xFF] == 1)
#define DOWN_FILLED     (down_filled[MapData(x,y+1) & 0xFF] == 1)
#define RIGHT_FILLED    (right_filled[MapData(x+1,y) & 0xFF] == 1)
#define LU_FILLED       (lu_filled[MapData(x-1,y-1) & 0xFF] == 1)
#define LD_FILLED       (ld_filled[MapData(x-1,y+1) & 0xFF] == 1)
#define RD_FILLED       (rd_filled[MapData(x+1,y+1) & 0xFF] == 1)
#define RU_FILLED       (ru_filled[MapData(x+1,y-1) & 0xFF] == 1)

#define UP_BIT          (1 << 0)
#define LEFT_BIT        (1 << 1)
#define DOWN_BIT        (1 << 2)
#define RIGHT_BIT       (1 << 3)
#define LU_BIT          (1 << 4)
#define LD_BIT          (1 << 5)
#define RD_BIT          (1 << 6)
#define RU_BIT          (1 << 7)

    /*
     * see if we can convert spaces into (half) blocks if
     * the space is surrounded by (half) blocks.
     */
    for (y = mapcursory; y < ymax; y++) {
	for (x = mapcursorx; x < xmax; x++) {
	    type = MapData(x, y);
	    if (type == XPMAP_SPACE) {
		mask = (UP_FILLED << 0) |
		    (LEFT_FILLED << 1) |
		    (DOWN_FILLED << 2) | (RIGHT_FILLED << 3);
		switch (mask) {
		case UP_BIT | LEFT_BIT | DOWN_BIT | RIGHT_BIT:
		case UP_BIT | LEFT_BIT | DOWN_BIT:
		case UP_BIT | LEFT_BIT | RIGHT_BIT:
		case UP_BIT | DOWN_BIT | RIGHT_BIT:
		case LEFT_BIT | DOWN_BIT | RIGHT_BIT:
		case DOWN_BIT | UP_BIT:
		case LEFT_BIT | RIGHT_BIT:
		    type = XPMAP_FILLED;
		    break;
		case UP_BIT | LEFT_BIT:
		    type = XPMAP_REC_LU;
		    break;
		case UP_BIT | RIGHT_BIT:
		    type = XPMAP_REC_RU;
		    break;
		case DOWN_BIT | RIGHT_BIT:
		    type = XPMAP_REC_RD;
		    break;
		case DOWN_BIT | LEFT_BIT:
		    type = XPMAP_REC_LD;
		    break;
		}
		if (type != MapData(x, y)) {
		    ChangeMapData(x, y, type, 1);
		    change = 1;
		    x++;
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can convert half diagonal
	 * blocks into filled blocks if they're surrounded by (half) blocks.
	 */
	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		switch (type) {
		case XPMAP_REC_RD:
		case XPMAP_REC_RU:
		case XPMAP_REC_LD:
		case XPMAP_REC_LU:
		    mask = (UP_FILLED << 0) |
			(LEFT_FILLED << 1) |
			(DOWN_FILLED << 2) | (RIGHT_FILLED << 3);
		    switch (mask) {
		    case UP_BIT | LEFT_BIT | DOWN_BIT | RIGHT_BIT:
		    case UP_BIT | LEFT_BIT | DOWN_BIT:
		    case UP_BIT | LEFT_BIT | RIGHT_BIT:
		    case UP_BIT | DOWN_BIT | RIGHT_BIT:
		    case LEFT_BIT | DOWN_BIT | RIGHT_BIT:
		    case DOWN_BIT | UP_BIT:
		    case LEFT_BIT | RIGHT_BIT:
			type = XPMAP_FILLED;
			break;
		    case UP_BIT | LEFT_BIT:
			type = XPMAP_REC_LU;
			break;
		    case UP_BIT | RIGHT_BIT:
			type = XPMAP_REC_RU;
			break;
		    case DOWN_BIT | RIGHT_BIT:
			type = XPMAP_REC_RD;
			break;
		    case DOWN_BIT | LEFT_BIT:
			type = XPMAP_REC_LD;
			break;
		    }
		    if (type != MapData(x, y)) {
			ChangeMapData(x, y, type, 1);
			change = 1;
			x++;
		    }
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can convert half diagonal blocks into
	 * filled blocks if their diagonal is next to a (half) block.
	 */
	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		switch (type) {
		case XPMAP_REC_LU:
		    if (RIGHT_FILLED == 1 || DOWN_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_LD:
		    if (RIGHT_FILLED == 1 || UP_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_RU:
		    if (LEFT_FILLED == 1 || DOWN_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_RD:
		    if (LEFT_FILLED == 1 || UP_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		default:
		    continue;
		}
		if (type != MapData(x, y)) {
		    ChangeMapData(x, y, type, 1);
		    change = 1;
		    x++;
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can convert half diagonal blocks into
	 * filled blocks if one of their backs is next to a space.
	 */
	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		switch (type) {
		case XPMAP_REC_RD:
		    if (RIGHT_FILLED == 0 || DOWN_FILLED == 0) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_RU:
		    if (RIGHT_FILLED == 0 || UP_FILLED == 0) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_LD:
		    if (LEFT_FILLED == 0 || DOWN_FILLED == 0) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_LU:
		    if (LEFT_FILLED == 0 || UP_FILLED == 0) {
			type = XPMAP_FILLED;
		    }
		    break;
		default:
		    continue;
		}
		if (type != MapData(x, y)) {
		    ChangeMapData(x, y, type, 1);
		    change = 1;
		    x++;
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * check if we can convert half diagonal blocks into filled
	 * blocks if their diagonal is next to a (half) filled block.
	 */
	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		switch (type) {
		case XPMAP_REC_RD:
		    if (LEFT_FILLED == 1 || UP_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_RU:
		    if (LEFT_FILLED == 1 || DOWN_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_LD:
		    if (RIGHT_FILLED == 1 || UP_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		case XPMAP_REC_LU:
		    if (RIGHT_FILLED == 1 || DOWN_FILLED == 1) {
			type = XPMAP_FILLED;
		    }
		    break;
		default:
		    continue;
		}
		if (type != MapData(x, y)) {
		    ChangeMapData(x, y, type, 1);
		    change = 1;
		    x++;
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can convert filled blocks
	 * into half blocks if they're surrounded by spaces on two sides.
	 */

	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		if (type == XPMAP_FILLED) {
		    mask = (LU_FILLED << 4) |
			(LD_FILLED << 5) |
			(RD_FILLED << 6) |
			(RU_FILLED << 7) |
			(UP_FILLED << 0) |
			(LEFT_FILLED << 1) |
			(DOWN_FILLED << 2) | (RIGHT_FILLED << 3);
		    switch (mask) {
		    case RIGHT_BIT | DOWN_BIT | RD_BIT:
		    case RIGHT_BIT | DOWN_BIT:
			type = XPMAP_REC_RD;
			break;
		    case RIGHT_BIT | UP_BIT | RU_BIT:
		    case RIGHT_BIT | UP_BIT:
			type = XPMAP_REC_RU;
			break;
		    case LEFT_BIT | DOWN_BIT | LD_BIT:
		    case LEFT_BIT | DOWN_BIT:
			type = XPMAP_REC_LD;
			break;
		    case LEFT_BIT | UP_BIT | LU_BIT:
		    case LEFT_BIT | UP_BIT:
			type = XPMAP_REC_LU;
			break;
		    }
		    if (type != MapData(x, y)) {
			ChangeMapData(x, y, type, 1);
			change = 1;
			x++;
		    }
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can convert filled blocks
	 * into half blocks if they're surrounded by spaces on two sides
	 * but with a possible half block on the far side.
	 */

	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		if (type == XPMAP_FILLED) {
		    mask = (LU_FILLED << 4) |
			(LD_FILLED << 5) |
			(RD_FILLED << 6) |
			(RU_FILLED << 7) |
			(UP_FILLED << 0) |
			(LEFT_FILLED << 1) |
			(DOWN_FILLED << 2) | (RIGHT_FILLED << 3);
		    switch (mask) {
		    case LD_BIT | RU_BIT | RIGHT_BIT | DOWN_BIT | RD_BIT:
		    case LD_BIT | RU_BIT | RIGHT_BIT | DOWN_BIT:
		    case LD_BIT | RIGHT_BIT | DOWN_BIT | RD_BIT:
		    case LD_BIT | RIGHT_BIT | DOWN_BIT:
		    case RU_BIT | RIGHT_BIT | DOWN_BIT | RD_BIT:
		    case RU_BIT | RIGHT_BIT | DOWN_BIT:
			type = XPMAP_REC_RD;
			break;
		    case LU_BIT | RD_BIT | RIGHT_BIT | UP_BIT | RU_BIT:
		    case LU_BIT | RD_BIT | RIGHT_BIT | UP_BIT:
		    case LU_BIT | RIGHT_BIT | UP_BIT | RU_BIT:
		    case LU_BIT | RIGHT_BIT | UP_BIT:
		    case RD_BIT | RIGHT_BIT | UP_BIT | RU_BIT:
		    case RD_BIT | RIGHT_BIT | UP_BIT:
			type = XPMAP_REC_RU;
			break;
		    case LU_BIT | RD_BIT | LEFT_BIT | DOWN_BIT | LD_BIT:
		    case LU_BIT | RD_BIT | LEFT_BIT | DOWN_BIT:
		    case LU_BIT | LEFT_BIT | DOWN_BIT | LD_BIT:
		    case LU_BIT | LEFT_BIT | DOWN_BIT:
		    case RD_BIT | LEFT_BIT | DOWN_BIT | LD_BIT:
		    case RD_BIT | LEFT_BIT | DOWN_BIT:
			type = XPMAP_REC_LD;
			break;
		    case LD_BIT | RU_BIT | LEFT_BIT | UP_BIT | LU_BIT:
		    case LD_BIT | RU_BIT | LEFT_BIT | UP_BIT:
		    case LD_BIT | LEFT_BIT | UP_BIT | LU_BIT:
		    case LD_BIT | LEFT_BIT | UP_BIT:
		    case RU_BIT | LEFT_BIT | UP_BIT | LU_BIT:
		    case RU_BIT | LEFT_BIT | UP_BIT:
			type = XPMAP_REC_LU;
			break;
		    }
		    if (type != MapData(x, y)) {
			ChangeMapData(x, y, type, 1);
			change = 1;
			x++;
		    }
		}
	    }
	}
    }

    if (change == 0) {
	/*
	 * now also check if we can remove filled blocks
	 * if they're surrounded by spaces on three or four sides.
	 */

	for (y = mapcursory; y < ymax; y++) {
	    for (x = mapcursorx; x < xmax; x++) {
		type = MapData(x, y);
		if (type == XPMAP_FILLED) {
		    mask = (UP_FILLED << 0) |
			(LEFT_FILLED << 1) |
			(DOWN_FILLED << 2) | (RIGHT_FILLED << 3);
		    switch (mask) {
		    case UP_BIT:
		    case LEFT_BIT:
		    case DOWN_BIT:
		    case RIGHT_BIT:
		    case 0:
			type = XPMAP_SPACE;
			break;
		    }
		    if (type != MapData(x, y)) {
			ChangeMapData(x, y, type, 1);
			change = 1;
			x++;
		    }
		}
	    }
	}
    }

    if (change) {
	map.changed = 1;
    }

    DrawSelectArea();
    return 0;
}
