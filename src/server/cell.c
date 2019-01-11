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

#include "xpserver.h"

/* we only search for objects which are at most 28 blocks away. */
#define MAX_CELL_DIST		28

/* sqrt(2) */
#undef SQRT2
#define SQRT2	1.41421356237309504880


typedef struct cell_dist cell_dist_t;
struct cell_dist {
    double dist;
    short x;
    short y;
};

typedef struct cell_offset cell_offset_t;
struct cell_offset {
    short x;
    short y;
};


static cell_node_t **Cells;
static int object_node_offset;
static cell_dist_t *cell_dist;
static size_t cell_dist_size;


static void Free_cell_dist(void)
{
    XFREE(cell_dist);
}


static int Compare_cell_dist(const void *a, const void *b)
{
    const cell_dist_t *c = (const cell_dist_t *) a;
    const cell_dist_t *d = (const cell_dist_t *) b;
    int acx, acy, adx, ady, maxc, maxd;

    if (c->dist < d->dist)
	return -1;
    if (c->dist > d->dist)
	return +1;
    acx = ABS(c->x);
    acy = ABS(c->y);
    adx = ABS(d->x);
    ady = ABS(d->y);
    maxc = MAX(acx, acy);
    maxd = MAX(adx, ady);
    if (maxc < maxd)
	return -1;
    if (maxc > maxd)
	return +1;
    return 0;
}


static void Init_cell_dist(void)
{
    cell_dist_t *dists;
    int x, y;
    int cell_dist_width;
    int cell_dist_height;
    int cell_max_left;
    int cell_max_right;
    int cell_max_up;
    int cell_max_down;

    Free_cell_dist();

    if (BIT(world->rules->mode, WRAP_PLAY)) {
	cell_max_right = MIN(MAX_CELL_DIST, (world->x / 2));
	cell_max_left = MIN(MAX_CELL_DIST, ((world->x - 1) / 2));
	cell_max_up = MIN(MAX_CELL_DIST, (world->y / 2));
	cell_max_down = MIN(MAX_CELL_DIST, ((world->y - 1) / 2));
    } else {
	cell_max_right = MIN(MAX_CELL_DIST, (world->x - 1));
	cell_max_left = MIN(MAX_CELL_DIST, (world->x - 1));
	cell_max_up = MIN(MAX_CELL_DIST, (world->y - 1));
	cell_max_down = MIN(MAX_CELL_DIST, (world->y - 1));
    }
    cell_dist_width = cell_max_left + 1 + cell_max_right;
    cell_dist_height = cell_max_down + 1 + cell_max_up;
    cell_dist_size = cell_dist_width * cell_dist_height;

    cell_dist = XMALLOC(cell_dist_t, cell_dist_size);
    if (cell_dist == NULL) {
	error("No cell dist mem");
	End_game();
    }

    dists = cell_dist;
    for (y = -cell_max_down; y <= cell_max_up; y++) {
	for (x = -cell_max_left; x <= cell_max_right; x++) {
	    dists->x = x;
	    dists->y = y;
	    dists->dist = (double) LENGTH(x, y);
	    dists++;
	}
    }

    qsort(cell_dist, cell_dist_size, sizeof(cell_dist_t),
	  Compare_cell_dist);
}


void Free_cells(void)
{
    XFREE(Cells);
    Free_cell_dist();
}


void Alloc_cells(void)
{
    size_t size;
    cell_node_t *cell_ptr;
    int x, y;

    Free_cells();

    size = sizeof(cell_node_t *) * world->x;
    size += sizeof(cell_node_t) * world->x * world->y;
    if (!(Cells = (cell_node_t **) malloc(size))) {
	error("No Cell mem");
	End_game();
    }
    cell_ptr = (cell_node_t *) & Cells[world->x];
    for (x = 0; x < world->x; x++) {
	Cells[x] = cell_ptr;
	for (y = 0; y < world->y; y++) {
	    /* init list to point to itself. */
	    cell_ptr->next = cell_ptr;
	    cell_ptr->prev = cell_ptr;
	    cell_ptr++;
	}
    }

    Init_cell_dist();
}


void Cell_init_object(object_t *obj)
{
    /* put obj on list with only itself. */
    obj->cell.next = &(obj->cell);
    obj->cell.prev = &(obj->cell);

    if (object_node_offset == 0)
	object_node_offset = ((char *) &(obj->cell) - (char *) obj);
}


void Cell_add_object(object_t *obj)
{
    blkpos_t bpos = Clpos_to_blkpos(obj->pos);
    cell_node_t *obj_node_ptr, *cell_node_ptr;
    cell_node_t *prev, *next;

    obj_node_ptr = &(obj->cell);
    next = obj_node_ptr->next;
    prev = obj_node_ptr->prev;

    assert(next->prev == obj_node_ptr);
    assert(prev->next == obj_node_ptr);

    /* remove obj from current list */
    next->prev = prev;
    prev->next = next;

    if (!World_contains_clpos(obj->pos)) {
	/* put obj on list with only itself. */
	obj_node_ptr->next = obj_node_ptr;
	obj_node_ptr->prev = obj_node_ptr;
    } else {
	/* put obj in cell list. */
	cell_node_ptr = &Cells[bpos.bx][bpos.by];
	obj_node_ptr->next = cell_node_ptr->next;
	obj_node_ptr->prev = cell_node_ptr;
	cell_node_ptr->next->prev = obj_node_ptr;
	cell_node_ptr->next = obj_node_ptr;
    }
}


void Cell_remove_object(object_t *obj)
{
    cell_node_t *obj_node_ptr;
    cell_node_t *next, *prev;

    obj_node_ptr = &(obj->cell);
    next = obj_node_ptr->next;
    prev = obj_node_ptr->prev;

    assert(next->prev == obj_node_ptr);
    assert(prev->next == obj_node_ptr);

    /* remove obj from current list */
    next->prev = prev;
    prev->next = next;

    /* put obj on list with only itself. */
    obj_node_ptr->next = obj_node_ptr;
    obj_node_ptr->prev = obj_node_ptr;
}


void Cell_get_objects(clpos_t pos,
		      int range,
		      int max_obj_count,
		      object_t *** obj_list, int *count_ptr)
{
    static object_t *ObjectList[MAX_TOTAL_SHOTS + 1];
    int i, count, x, y, xw, yw, wrap;
    object_t *obj;
    cell_node_t *cell_node_ptr, *next;
    double dist;
    blkpos_t bpos = Clpos_to_blkpos(pos);

    x = bpos.bx;
    y = bpos.by;

    wrap = (BIT(world->rules->mode, WRAP_PLAY) != 0);
    dist = (double) (range * SQRT2);
    count = 0;
    for (i = 0; i < (int)cell_dist_size && count < max_obj_count; i++) {
	if (dist < cell_dist[i].dist)
	    break;
	else {
	    xw = x + cell_dist[i].x;
	    yw = y + cell_dist[i].y;
	    if (xw < 0) {
		if (wrap)
		    xw += world->x;
		else
		    continue;
	    } else if (xw >= world->x) {
		if (wrap)
		    xw -= world->x;
		else
		    continue;
	    }
	    if (yw < 0) {
		if (wrap)
		    yw += world->y;
		else
		    continue;
	    } else if (yw >= world->y) {
		if (wrap)
		    yw -= world->y;
		else
		    continue;
	    }
	    cell_node_ptr = &Cells[xw][yw];
	    next = cell_node_ptr->next;
	    while (next != cell_node_ptr && count < max_obj_count) {
		obj = (object_t *) ((char *) next - object_node_offset);
		ObjectList[count++] = obj;
		next = next->next;
	    }
	}
    }

    ObjectList[count] = NULL;
    *obj_list = &ObjectList[0];
    if (count_ptr != NULL)
	*count_ptr = count;
}
