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

typedef struct grow_t {
    int x, y;
    struct grow_t *next;
} grow_t;

int grow_minx, grow_miny, grow_maxx, grow_maxy,
    grow_w, grow_h, grow_centerx, grow_centery, grow_filled = 0;
double grow_xa = 1.0, grow_ya = 1.0;
grow_t *grow = NULL;


int GrowMapArea(HandlerInfo_t info)
{
    grow_t *next, *delgrow;
    int i, j, growat;
    long seed;
    int angle;
    float x, y, dx, dy;

    if (info.count == 0) {
	RoundMapArea(info);
	DrawSelectArea();
	/* free grow structure */
	next = grow;
	while (next != NULL) {
	    delgrow = next->next;
	    free(next);
	    next = delgrow;
	}
	grow = NULL;
	return 0;
    }

    if (info.count == 1) {
	DrawSelectArea();
	ClearUndo();

	grow_xa = grow_ya = 1.0;

	if (selectfrom_x < 0) {	/* no area selected, do entire screen */
	    grow_minx = map.view_x;
	    grow_miny = map.view_y;
	    grow_maxx =
		map.view_x + (mapwin_width - TOOLSWIDTH) / map.view_zoom;
	    grow_maxy = map.view_y + mapwin_height / map.view_zoom;
	} else {
	    if (selectfrom_x < selectto_x) {
		grow_minx = selectfrom_x + map.view_x;
		grow_maxx = selectto_x + map.view_x;
	    } else {
		grow_minx = selectto_x + map.view_x;
		grow_maxx = selectfrom_x + map.view_x;
	    }
	    if (selectfrom_y < selectto_y) {
		grow_miny = selectfrom_y + map.view_y;
		grow_maxy = selectto_y + map.view_y;
	    } else {
		grow_miny = selectto_y + map.view_y;
		grow_maxy = selectfrom_y + map.view_y;
	    }
	}
	grow_w = grow_maxx - grow_minx;
	grow_h = grow_maxy - grow_miny;
	grow_centerx = (grow_minx + grow_maxx) / 2;
	grow_centery = (grow_miny + grow_maxy) / 2;

	grow_filled = 0;
	for (i = grow_minx; i < grow_maxx; i++) {
	    for (j = grow_miny; j < grow_maxy; j++) {
		if (MapData(i, j) != XPMAP_FILLED) {
		    ChangeMapData(i, j, ' ', 1);
		} else {
		    grow_filled++;
		    next = grow;
		    grow = (grow_t *) malloc(sizeof(grow_t));
		    grow->x = i;
		    grow->y = j;
		    grow->next = next;
		}
	    }
	}

	/* place a square in the center if there are none */
	if (grow == NULL) {
	    ChangeMapData(grow_centerx, grow_centery, XPMAP_FILLED, 1);
	    grow = (grow_t *) malloc(sizeof(grow_t));
	    grow->x = grow_centerx;
	    grow->y = grow_centery;
	    grow->next = NULL;
	    grow_filled = 1;
	    if (grow_w > grow_h) {
		grow_ya = ((double) grow_h) / ((double) grow_w);
	    } else {
		grow_xa = ((double) grow_w) / ((double) grow_h);
	    }
	}

	seedMT((unsigned)time(NULL) * Get_process_id());
    }

    if (grow_filled > 1) {
	growat = randomMT() % (grow_filled - 1);
	next = grow;
	while ((next != NULL) && (growat != 0)) {
	    next = next->next;
	    growat--;
	}
    } else {
	next = grow;
    }

    angle = randomMT() % 1000;

    dx = grow_xa * cos(2 * 3.14 * angle / 1000);
    dy = grow_ya * sin(2 * 3.14 * angle / 1000);
    x = next->x + dx;
    y = next->y + dy;
    while (MapData((int) x, (int) y) == XPMAP_FILLED) {
	x += dx;
	y += dy;
    }
    if (((int) x > grow_maxx) || ((int) y > grow_maxy) ||
	((int) x < grow_minx) || ((int) y < grow_miny)) {
	return 1;
    }
    ChangeMapData((int) x, (int) y, XPMAP_FILLED, 1);
    next = grow;
    grow = (grow_t *) malloc(sizeof(grow_t));
    grow->x = (int) x;
    grow->y = (int) y;
    grow->next = next;
    grow_filled++;

    return 0;
}
