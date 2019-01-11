/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2003-2004 Kristian Söderblom <kps@users.sourceforge.net>
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

static int Compress_map(unsigned char *map, size_t size);

static void Xpmap_treasure_to_polygon(int treasure_ind);
static void Xpmap_target_to_polygon(int target_ind);
static void Xpmap_cannon_to_polygon(int cannon_ind);
static void Xpmap_wormhole_to_polygon(int wormhole_ind);
static void Xpmap_friction_area_to_polygon(int fa_ind);

static bool		compress_maps = true;

static void Xpmap_extra_error(int line_num)
{
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num) {
	prev_line_num = line_num;
	if (++error_count <= max_error)
	    warn("Map file contains extraneous characters on line %d",
		 line_num);
	else if (error_count - max_error == 1)
	    warn("And so on...");
    }
}


static void Xpmap_missing_error(int line_num)
{
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num) {
	prev_line_num = line_num;
	if (++error_count <= max_error)
	    warn("Not enough map data on map data line %d", line_num);
	else if (error_count - max_error == 1)
	    warn("And so on...");
    }
}



/*
 * Compress the map data using a simple Run Length Encoding algorithm.
 * If there is more than one consecutive byte with the same type
 * then we set the high bit of the byte and then the next byte
 * gives the number of repetitions.
 * This works well for most maps which have lots of series of the
 * same map object and is simple enough to got implemented quickly.
 */
static int Compress_map(unsigned char *map, size_t size)
{
    int			i, j, k;

    for (i = j = 0; i < (int)size; i++, j++) {
	if (i + 1 < (int)size
	    && map[i] == map[i + 1]) {
	    for (k = 2; i + k < (int)size; k++) {
		if (map[i] != map[i + k])
		    break;
		if (k == 255)
		    break;
	    }
	    map[j] = (map[i] | SETUP_COMPRESSED);
	    map[++j] = k;
	    i += k - 1;
	} else
	    map[j] = map[i];
    }
    return j;
}


void Create_blockmap_from_polygons(void)
{
    int i, h, type;
    blkpos_t blk;
    clpos_t pos;
    shape_t r_wire, u_wire, l_wire, d_wire;
    clpos_t r_coords[3], u_coords[3], l_coords[3], d_coords[3];

    r_wire.num_points = 3;
    u_wire.num_points = 3;
    l_wire.num_points = 3;
    d_wire.num_points = 3;

    for (i = 0; i < 3; i++)
	r_wire.pts[i] = &r_coords[i];
    for (i = 0; i < 3; i++)
	u_wire.pts[i] = &u_coords[i];
    for (i = 0; i < 3; i++)
	l_wire.pts[i] = &l_coords[i];
    for (i = 0; i < 3; i++)
	d_wire.pts[i] = &d_coords[i];

    /*
     * Block is divided to 4 parts, r, u, l and d, the middle of the block
     * looking like this. Each char represents a square click. The middle
     * of the block is at 'x', square clicks with 'x' or ' ' are considered
     * not part of any of the block parts r, u, l or d.
     *
     *  uuu
     * l U r
     * lLxRr
     * l D r
     *  ddd
     *
     */

    h = BLOCK_CLICKS / 2 - 2;

    /* right part of block */
    r_coords[0].cx = 0;
    r_coords[0].cy = 0; /* this is the R position in the block */
    r_coords[1].cx = h;
    r_coords[1].cy = -h;
    r_coords[2].cx = h;
    r_coords[2].cy = h;

    /* up part of block */
    u_coords[0].cx = 0;
    u_coords[0].cy = 0;
    u_coords[1].cx = h;
    u_coords[1].cy = h;
    u_coords[2].cx = -h;
    u_coords[2].cy = h;

    /* left part of block */
    l_coords[0].cx = 0;
    l_coords[0].cy = 0;
    l_coords[1].cx = -h;
    l_coords[1].cy = h;
    l_coords[2].cx = -h;
    l_coords[2].cy = -h;

    /* down part of block */
    d_coords[0].cx = 0;
    d_coords[0].cy = 0;
    d_coords[1].cx = -h;
    d_coords[1].cy = -h;
    d_coords[2].cx = h;
    d_coords[2].cy = -h;

    /*
     * Create blocks out of polygons.
     */
    for (blk.by = 0; blk.by < world->y; blk.by++)
	for (blk.bx = 0; blk.bx < world->x; blk.bx++)
	    World_set_block(blk, SPACE);


    for (blk.by = 0; blk.by < world->bheight_floor; blk.by++) {
	for (blk.bx = 0; blk.bx < world->bwidth_floor; blk.bx++) {
	    int num_inside = 0;
	    bool r_inside = false, u_inside = false;
	    bool l_inside = false, d_inside = false;

	    pos = Block_get_center_clpos(blk);

	    if (shape_is_inside(pos.cx+1, pos.cy, 0, NULL, &r_wire, 0) == 0) {
		r_inside = true;
		num_inside++;
	    }
	    if (shape_is_inside(pos.cx, pos.cy+1, 0, NULL, &u_wire, 0) == 0) {
		u_inside = true;
		num_inside++;
	    }
	    if (shape_is_inside(pos.cx-1, pos.cy, 0, NULL, &l_wire, 0) == 0) {
		l_inside = true;
		num_inside++;
	    }
	    if (shape_is_inside(pos.cx, pos.cy-1, 0, NULL, &d_wire, 0) == 0) {
		d_inside = true;
		num_inside++;
	    }

	    if (num_inside > 2)
		World_set_block(blk, FILLED);

	    if (num_inside == 2) {
		if (r_inside && u_inside)
		    World_set_block(blk, REC_RU);
		if (u_inside && l_inside)
		    World_set_block(blk, REC_LU);
		if (l_inside && d_inside)
		    World_set_block(blk, REC_LD);
		if (d_inside && r_inside)
		    World_set_block(blk, REC_RD);
		if (u_inside && d_inside)
		    World_set_block(blk, FILLED);
		if (r_inside && l_inside)
		    World_set_block(blk, FILLED);
	    }

	    if (num_inside == 1) {
		if (r_inside)
		    World_set_block(blk, REC_RU);
		if (u_inside)
		    World_set_block(blk, REC_LU);
		if (l_inside)
		    World_set_block(blk, REC_LD);
		if (d_inside)
		    World_set_block(blk, REC_RD);
	    }
	}
    }

    /*
     * Create blocks out of map objects. Note that some of these
     * may be in the same block, which might cause a client error.
     */
    for (i = 0; i < Num_fuels(); i++) {
	fuel_t *fs = Fuel_by_index(i);

	blk = Clpos_to_blkpos(fs->pos);
	World_set_block(blk, FUEL);
    }

    for (i = 0; i < Num_asteroidConcs(); i++) {
	asteroid_concentrator_t *aconc = AsteroidConc_by_index(i);

	blk = Clpos_to_blkpos(aconc->pos);
	World_set_block(blk, ASTEROID_CONCENTRATOR);
    }

    for (i = 0; i < Num_itemConcs(); i++) {
	item_concentrator_t *iconc = ItemConc_by_index(i);

	blk = Clpos_to_blkpos(iconc->pos);
	World_set_block(blk, ITEM_CONCENTRATOR);
    }

    for (i = 0; i < Num_wormholes(); i++) {
	wormhole_t *wh = Wormhole_by_index(i);

	blk = Clpos_to_blkpos(wh->pos);
	World_set_block(blk, WORMHOLE);
    }

    /* find balltargets */
    for (blk.by = 0; blk.by < world->bheight_floor; blk.by++) {
	for (blk.bx = 0; blk.bx < world->bwidth_floor; blk.bx++) {
	    int group;
	    group_t *gp;

	    pos = Block_get_center_clpos(blk);
	    group = shape_is_inside(pos.cx, pos.cy,
				    BALL_BIT, NULL, &filled_wire, 0);
	    if (group == NO_GROUP || group == 0)
		continue;
	    gp = groupptr_by_id(group);
	    if (gp == NULL)
		continue;
	    if (gp->type == TREASURE && gp->hitmask == NONBALL_BIT)
		World_set_block(blk, TREASURE);
	}
    }

    /*
     * Handle bases. Note that on polygon maps there can be several
     * bases on the area of a block. To handle this so that old clients
     * won't get "Bad homebase index" error, we put excess bases somewhere
     * else than on the same block.
     */

    /*
     * First mark all blocks having a base.
     * We use a base attractor for this.
     */
    for (i = 0; i < Num_bases(); i++) {
	base_t *base = Base_by_index(i);

	blk = Clpos_to_blkpos(base->pos);
	type = World_get_block(blk);

	/* don't put the base on top of a fuel or treasure */
	if (type == FUEL || type == TREASURE)
	    continue;
	World_set_block(blk, BASE_ATTRACTOR);
    }

    /*
     * Put bases where there are base attractors or somewhere else
     * if the block already has some other important type.
     */
    for (i = 0; i < Num_bases(); i++) {
	base_t *base = Base_by_index(i);
	bool done;

	blk = Clpos_to_blkpos(base->pos);
	type = World_get_block(blk);
	done = false;

	if (type == FUEL || type == TREASURE || type == BASE) {
	    /*
	     * The block where the base should be put already has some
	     * important type. We need to put this base somewhere else.
	     * Let's just line up excess bases close to the origin of the map.
	     */
	    for (blk.by = 0; blk.by < world->y; blk.by++) {
		for (blk.bx = 0; blk.bx < world->x; blk.bx++) {
		    type = World_get_block(blk);
		    /* 
		     * Check for base attractor here too because we might
		     * have marked this block in the earlier loop over all
		     * bases.
		     */
		    if (type == FUEL || type == TREASURE
			|| type == BASE || type == BASE_ATTRACTOR)
			continue;
		    /* put base attractor here so that assert is happy */
		    type = BASE_ATTRACTOR;
		    World_set_block(blk, type);
		    done = true;
		    break;
		}
		if (done)
		    break;
	    }

	    /* this probably doesn't happen very often */
	    if (!done)
		fatal("Create_blockmap_from_polygons:\n"
		      "Couldn't find any place on map to put base on "
		      "(%d, %d).", base->pos.cx, base->pos.cy);
	}

	assert(type == BASE_ATTRACTOR);
	World_set_block(blk, BASE);
    }
}

setup_t *Xpmap_init_setup(void)
{
    int i, x, y, team, type = -1, dir, wtype;
    int wormhole_i = 0, treasure_i = 0, target_i = 0, base_i = 0, cannon_i = 0;
    unsigned char *mapdata, *mapptr;
    size_t size, numblocks;
    setup_t *setup;

    numblocks = world->x * world->y;
    if ((mapdata = XMALLOC(unsigned char, numblocks)) == NULL) {
	error("No memory for mapdata");
	return NULL;
    }
    memset(mapdata, SETUP_SPACE, numblocks);
    mapptr = mapdata;
    errno = 0;
    for (x = 0; x < world->x; x++) {
	for (y = 0; y < world->y; y++, mapptr++) {
	    type = world->block[x][y];
	    switch (type) {
	    case ACWISE_GRAV:
	    case CWISE_GRAV:
	    case POS_GRAV:
	    case NEG_GRAV:
	    case UP_GRAV:
	    case DOWN_GRAV:
	    case RIGHT_GRAV:
	    case LEFT_GRAV:
		if (!options.gravityVisible)
		    type = SPACE;
		break;
	    case WORMHOLE:
		if (!options.wormholeVisible)
		    type = SPACE;
		break;
	    case ITEM_CONCENTRATOR:
		if (!options.itemConcentratorVisible)
		    type = SPACE;
		break;
	    case ASTEROID_CONCENTRATOR:
		if (!options.asteroidConcentratorVisible)
		    type = SPACE;
		break;
	    case FRICTION:
		if (!options.blockFrictionVisible)
		    type = SPACE;
		else
		    type = DECOR_FILLED;
		break;
	    default:
		break;
	    }
	    switch (type) {
	    case SPACE:		*mapptr = SETUP_SPACE; break;
	    case FILLED:	*mapptr = SETUP_FILLED; break;
	    case REC_RU:	*mapptr = SETUP_REC_RU; break;
	    case REC_RD:	*mapptr = SETUP_REC_RD; break;
	    case REC_LU:	*mapptr = SETUP_REC_LU; break;
	    case REC_LD:	*mapptr = SETUP_REC_LD; break;
	    case FUEL:		*mapptr = SETUP_FUEL; break;
	    case ACWISE_GRAV:	*mapptr = SETUP_ACWISE_GRAV; break;
	    case CWISE_GRAV:	*mapptr = SETUP_CWISE_GRAV; break;
	    case POS_GRAV:	*mapptr = SETUP_POS_GRAV; break;
	    case NEG_GRAV:	*mapptr = SETUP_NEG_GRAV; break;
	    case UP_GRAV:	*mapptr = SETUP_UP_GRAV; break;
	    case DOWN_GRAV:	*mapptr = SETUP_DOWN_GRAV; break;
	    case RIGHT_GRAV:	*mapptr = SETUP_RIGHT_GRAV; break;
	    case LEFT_GRAV:	*mapptr = SETUP_LEFT_GRAV; break;
	    case ITEM_CONCENTRATOR:
		*mapptr = SETUP_ITEM_CONCENTRATOR; break;

	    case ASTEROID_CONCENTRATOR:
		*mapptr = SETUP_ASTEROID_CONCENTRATOR; break;

	    case DECOR_FILLED:	*mapptr = SETUP_DECOR_FILLED; break;
	    case DECOR_RU:	*mapptr = SETUP_DECOR_RU; break;
	    case DECOR_RD:	*mapptr = SETUP_DECOR_RD; break;
	    case DECOR_LU:	*mapptr = SETUP_DECOR_LU; break;
	    case DECOR_LD:	*mapptr = SETUP_DECOR_LD; break;

	    case WORMHOLE:
		if (wormhole_i >= Num_wormholes()) {
		    /*
		     * This can happen on an xp2 map if the block mapdata
		     * contains more wormholes than is specified in the
		     * xml data.
		     */
		    warn("Too many wormholes in block mapdata.");
		    *mapptr = SETUP_SPACE;
		    break;
		}
		wtype = Wormhole_by_index(wormhole_i)->type;
		wormhole_i++;
		switch (wtype) {
		case WORM_NORMAL:
		case WORM_FIXED:
		    *mapptr = SETUP_WORM_NORMAL;
		    break;
		case WORM_IN:
		    *mapptr = SETUP_WORM_IN;
		    break;
		case WORM_OUT:
		    *mapptr = SETUP_WORM_OUT;
		    break;
		default:
		    warn("Bad wormhole (%d,%d).", x, y);
		    *mapptr = SETUP_SPACE;
		    break;
		}
		break;

	    case TREASURE:
		if (treasure_i >= Num_treasures()) {
		    warn("Too many treasures in block mapdata.");
		    *mapptr = SETUP_SPACE;
		    break;
		}
		team = Treasure_by_index(treasure_i)->team;
		treasure_i++;
		if (team == TEAM_NOT_SET)
		    team = 0;
		*mapptr = SETUP_TREASURE + team;
		break;

	    case TARGET:
		if (target_i >= Num_targets()) {
		    warn("Too many targets in block mapdata.");
		    *mapptr = SETUP_SPACE;
		    break;
		}
		team = Target_by_index(target_i)->team;
		target_i++;
		if (team == TEAM_NOT_SET)
		    team = 0;
		*mapptr = SETUP_TARGET + team;
		break;

	    case BASE:
		if (base_i >= Num_bases()) {
		    warn("Too many bases in block mapdata.");
		    *mapptr = SETUP_SPACE;
		    break;
		}
		team = Base_by_index(base_i)->team;
		if (team == TEAM_NOT_SET)
		    team = 0;
		dir = Base_by_index(base_i)->dir;
		base_i++;
		/* other code should take care of this */
		assert(dir >= 0);
		assert(dir < RES);
		/* round to nearest direction */
		dir = (((dir + (RES/8)) / (RES/4)) * (RES/4)) % RES;
		assert(dir == DIR_UP || dir == DIR_RIGHT
		       || dir == DIR_DOWN || dir == DIR_LEFT);
		switch (dir) {
		case DIR_UP:    *mapptr = SETUP_BASE_UP + team; break;
		case DIR_RIGHT: *mapptr = SETUP_BASE_RIGHT + team; break;
		case DIR_DOWN:  *mapptr = SETUP_BASE_DOWN + team; break;
		case DIR_LEFT:  *mapptr = SETUP_BASE_LEFT + team; break;
		default:
		    /* should never happen */
		    warn("Bad base at (%d,%d), (dir = %d).", x, y, dir);
		    *mapptr = SETUP_BASE_UP + team;
		    break;
		}
		break;

	    case CANNON:
		if (cannon_i >= Num_cannons()) {
		    warn("Too many cannons in block mapdata.");
		    *mapptr = SETUP_SPACE;
		    break;
		}
		dir = Cannon_by_index(cannon_i)->dir;
		cannon_i++;
		switch (dir) {
		case DIR_UP:	*mapptr = SETUP_CANNON_UP; break;
		case DIR_RIGHT:	*mapptr = SETUP_CANNON_RIGHT; break;
		case DIR_DOWN:	*mapptr = SETUP_CANNON_DOWN; break;
		case DIR_LEFT:	*mapptr = SETUP_CANNON_LEFT; break;
		default:
		    warn("Bad cannon at (%d,%d), (dir = %d).", x, y, dir);
		    *mapptr = SETUP_CANNON_UP;
		    break;
		}
		break;

	    case CHECK:
		for (i = 0; i < world->NumChecks; i++) {
		    check_t *check = Check_by_index(i);
		    blkpos_t bpos = Clpos_to_blkpos(check->pos);

		    if (x != bpos.bx || y != bpos.by)
			continue;
		    *mapptr = SETUP_CHECK + i;
		    break;
		}
		if (i >= world->NumChecks) {
		    warn("Bad checkpoint at (%d,%d).", x, y);
		    *mapptr = SETUP_SPACE;
		    break;
		}
		break;

	    default:
		warn("Unknown map type (%d) at (%d,%d).", type, x, y);
		*mapptr = SETUP_SPACE;
		break;
	    }
	}
    }
    if (!compress_maps) {
	type = SETUP_MAP_UNCOMPRESSED;
	size = numblocks;
    } else {
	type = SETUP_MAP_ORDER_XY;
	size = Compress_map(mapdata, numblocks);
	if (size <= 0 || size > numblocks) {
	    warn("Map compression error (%d)", size);
	    free(mapdata);
	    return NULL;
	}
	if ((mapdata = XREALLOC(unsigned char, mapdata, size)) == NULL) {
	    error("Cannot reallocate mapdata");
	    return NULL;
	}
    }

    if (type != SETUP_MAP_UNCOMPRESSED)
	xpprintf("%s Block map compression ratio is %-4.2f%%\n",
		 showtime(), 100.0 * size / numblocks);

    if ((setup = (setup_t *)malloc(sizeof(setup_t) + size)) == NULL) {
	error("No memory to hold oldsetup");
	free(mapdata);
	return NULL;
    }
    memset(setup, 0, sizeof(setup_t) + size);
    memcpy(setup->map_data, mapdata, size);
    free(mapdata);
    setup->setup_size = ((char *) &setup->map_data[0]
			 - (char *) setup) + size;
    setup->map_data_len = size;
    setup->map_order = type;
    setup->lives = world->rules->lives;
    setup->mode = world->rules->mode;
    setup->x = world->x;
    setup->y = world->y;
    strlcpy(setup->name, world->name, sizeof(setup->name));
    strlcpy(setup->author, world->author, sizeof(setup->author));

    return setup;
}




/*
 * Grok block based map data.
 *
 * Create world->block using options.mapData.
 * Free options.mapData.
 */
void Xpmap_grok_map_data(void)
{
    int x = -1, y = world->y - 1, c;
    char *s = options.mapData;
    blkpos_t blk;

    if (options.mapData == NULL) {
	warn("Map didn't have any mapData.");
	return;
    }

    while (y >= 0) {

	x++;

	if (options.extraBorder && (x == 0 || x == world->x - 1
	    || y == 0 || y == world->y - 1)) {
	    if (x >= world->x) {
		x = -1;
		y--;
		continue;
	    } else
		/* make extra border of solid rock */
		c = XPMAP_FILLED;
	}
	else {
	    c = *s;
	    if (c == '\0' || c == EOF) {
		if (x < world->x) {
		    /* not enough map data on this line */
		    Xpmap_missing_error(world->y - y);
		    c = XPMAP_SPACE;
		} else
		    c = '\n';
	    } else {
		if (c == '\n' && x < world->x) {
		    /* not enough map data on this line */
		    Xpmap_missing_error(world->y - y);
		    c = XPMAP_SPACE;
		} else
		    s++;
	    }
	}
	if (x >= world->x || c == '\n') {
	    y--; x = -1;
	    if (c != '\n') {			/* Get rest of line */
		Xpmap_extra_error(world->y - y);
		while (c != '\n' && c != EOF)
		    c = *s++;
	    }
	    continue;
	}
	blk.bx = x;
	blk.by = y;
	World_set_block(blk, c);
    }

    XFREE(options.mapData);
}


/*
 * Determining which team these belong to is done later,
 * in Find_closest_team().
 */
static void Xpmap_place_cannon(blkpos_t blk, int dir)
{
    clpos_t pos;
    int ind;

    switch (dir) {
    case DIR_UP:
	pos.cx = (click_t)((blk.bx + 0.5) * BLOCK_CLICKS);
	pos.cy = (click_t)((blk.by + 0.333) * BLOCK_CLICKS);
	break;
    case DIR_LEFT:
	pos.cx = (click_t)((blk.bx + 0.667) * BLOCK_CLICKS);
	pos.cy = (click_t)((blk.by + 0.5) * BLOCK_CLICKS);
	break;
    case DIR_RIGHT:
	pos.cx = (click_t)((blk.bx + 0.333) * BLOCK_CLICKS);
	pos.cy = (click_t)((blk.by + 0.5) * BLOCK_CLICKS);
	break;
    case DIR_DOWN:
	pos.cx = (click_t)((blk.bx + 0.5) * BLOCK_CLICKS);
	pos.cy = (click_t)((blk.by + 0.667) * BLOCK_CLICKS);
	break;
    default:
 	/* can't happen */
	assert(0 && "Unknown cannon direction.");
	break;
    }

    World_set_block(blk, CANNON);
    ind = World_place_cannon(pos, dir, TEAM_NOT_SET);
    Cannon_init(Cannon_by_index(ind));
}

/*
 * The direction of the base should be so that it points
 * up with respect to the gravity in the region.  This
 * is fixed in Find_base_dir() when the gravity has
 * been computed.
 */
static void Xpmap_place_base(blkpos_t blk, int team)
{
    World_set_block(blk, BASE);
    World_place_base(Block_get_center_clpos(blk), DIR_UP, team, 0);
}

static void Xpmap_place_fuel(blkpos_t blk)
{
    World_set_block(blk, FUEL);
    World_place_fuel(Block_get_center_clpos(blk), TEAM_NOT_SET);
}

static void Xpmap_place_treasure(blkpos_t blk, bool empty)
{
    World_set_block(blk, TREASURE);
    World_place_treasure(Block_get_center_clpos(blk),
			 TEAM_NOT_SET, empty, 0xff);
}

static void Xpmap_place_wormhole(blkpos_t blk, wormtype_t type)
{
    World_set_block(blk, WORMHOLE);
    World_place_wormhole(Block_get_center_clpos(blk), type);
}

static void Xpmap_place_target(blkpos_t blk)
{
    World_set_block(blk, TARGET);
    World_place_target(Block_get_center_clpos(blk), TEAM_NOT_SET);
}

static void Xpmap_place_check(blkpos_t blk, int ind)
{
    if (!BIT(world->rules->mode, TIMING)) {
	World_set_block(blk, SPACE);
	return;
    }

    World_set_block(blk, CHECK);
    World_place_check(Block_get_center_clpos(blk), ind);
}

static void Xpmap_place_item_concentrator(blkpos_t blk)
{
    World_set_block(blk, ITEM_CONCENTRATOR);
    World_place_item_concentrator(Block_get_center_clpos(blk));
}

static void Xpmap_place_asteroid_concentrator(blkpos_t blk)
{
    World_set_block(blk, ASTEROID_CONCENTRATOR);
    World_place_asteroid_concentrator(Block_get_center_clpos(blk));
}

static void Xpmap_place_grav(blkpos_t blk,
			     double force, int type)
{
    World_set_block(blk, type);
    World_place_grav(Block_get_center_clpos(blk), force, type);
}

static void Xpmap_place_friction_area(blkpos_t blk)
{
    World_set_block(blk, FRICTION);
    World_place_friction_area(Block_get_center_clpos(blk),
			      options.blockFriction);
}

static void Xpmap_place_block(blkpos_t blk, int type)
{
    World_set_block(blk, type);
}

/*
 * Change read tags to internal data, create objects if 'create' is true.
 */
void Xpmap_tags_to_internal_data(void)
{
    int x, y;
    char c;

    for (x = 0; x < world->x; x++) {

	for (y = 0; y < world->y; y++) {

	    blkpos_t blk;

	    blk.bx = x;
	    blk.by = y;

	    c = world->block[x][y];

	    switch (c) {
	    case XPMAP_SPACE:
	    case XPMAP_SPACE_ALT:
	    default:
		Xpmap_place_block(blk, SPACE);
		break;

	    case XPMAP_FILLED:
		Xpmap_place_block(blk, FILLED);
		break;
	    case XPMAP_REC_LU:
		Xpmap_place_block(blk, REC_LU);
		break;
	    case XPMAP_REC_RU:
		Xpmap_place_block(blk, REC_RU);
		break;
	    case XPMAP_REC_LD:
		Xpmap_place_block(blk, REC_LD);
		break;
	    case XPMAP_REC_RD:
		Xpmap_place_block(blk, REC_RD);
		break;

	    case XPMAP_CANNON_UP:
		Xpmap_place_cannon(blk, DIR_UP);
		break;
	    case XPMAP_CANNON_LEFT:
		Xpmap_place_cannon(blk, DIR_LEFT);
		break;
	    case XPMAP_CANNON_RIGHT:
		Xpmap_place_cannon(blk, DIR_RIGHT);
		break;
	    case XPMAP_CANNON_DOWN:
		Xpmap_place_cannon(blk, DIR_DOWN);
		break;

	    case XPMAP_FUEL:
		Xpmap_place_fuel(blk);
		break;
	    case XPMAP_TREASURE:
		Xpmap_place_treasure(blk, false);
		break;
	    case XPMAP_EMPTY_TREASURE:
		Xpmap_place_treasure(blk, true);
		break;
	    case XPMAP_TARGET:
		Xpmap_place_target(blk);
		break;
	    case XPMAP_ITEM_CONCENTRATOR:
		Xpmap_place_item_concentrator(blk);
		break;
	    case XPMAP_ASTEROID_CONCENTRATOR:
		Xpmap_place_asteroid_concentrator(blk);
		break;
	    case XPMAP_BASE_ATTRACTOR:
		Xpmap_place_block(blk, BASE_ATTRACTOR);
		break;
	    case XPMAP_BASE:
		Xpmap_place_base(blk, TEAM_NOT_SET);
		break;
	    case XPMAP_BASE_TEAM_0:
	    case XPMAP_BASE_TEAM_1:
	    case XPMAP_BASE_TEAM_2:
	    case XPMAP_BASE_TEAM_3:
	    case XPMAP_BASE_TEAM_4:
	    case XPMAP_BASE_TEAM_5:
	    case XPMAP_BASE_TEAM_6:
	    case XPMAP_BASE_TEAM_7:
	    case XPMAP_BASE_TEAM_8:
	    case XPMAP_BASE_TEAM_9:
		Xpmap_place_base(blk, (int) (c - XPMAP_BASE_TEAM_0));
		break;

	    case XPMAP_POS_GRAV:
		Xpmap_place_grav(blk, -GRAVS_POWER, POS_GRAV);
		break;
	    case XPMAP_NEG_GRAV:
		Xpmap_place_grav(blk, GRAVS_POWER, NEG_GRAV);
		break;
	    case XPMAP_CWISE_GRAV:
		Xpmap_place_grav(blk, GRAVS_POWER, CWISE_GRAV);
		break;
	    case XPMAP_ACWISE_GRAV:
		Xpmap_place_grav(blk, -GRAVS_POWER, ACWISE_GRAV);
		break;
	    case XPMAP_UP_GRAV:
		Xpmap_place_grav(blk, GRAVS_POWER, UP_GRAV);
		break;
	    case XPMAP_DOWN_GRAV:
		Xpmap_place_grav(blk, -GRAVS_POWER, DOWN_GRAV);
		break;
	    case XPMAP_RIGHT_GRAV:
		Xpmap_place_grav(blk, GRAVS_POWER, RIGHT_GRAV);
		break;
	    case XPMAP_LEFT_GRAV:
		Xpmap_place_grav(blk, -GRAVS_POWER, LEFT_GRAV);
		break;

	    case XPMAP_WORMHOLE_NORMAL:
		Xpmap_place_wormhole(blk, WORM_NORMAL);
		break;
	    case XPMAP_WORMHOLE_IN:
		Xpmap_place_wormhole(blk, WORM_IN);
		break;
	    case XPMAP_WORMHOLE_OUT:
		Xpmap_place_wormhole(blk, WORM_OUT);
		break;

	    case XPMAP_CHECK_0:	    case XPMAP_CHECK_1:
	    case XPMAP_CHECK_2:	    case XPMAP_CHECK_3:
	    case XPMAP_CHECK_4:	    case XPMAP_CHECK_5:
	    case XPMAP_CHECK_6:	    case XPMAP_CHECK_7:
	    case XPMAP_CHECK_8:	    case XPMAP_CHECK_9:
	    case XPMAP_CHECK_10:    case XPMAP_CHECK_11:
	    case XPMAP_CHECK_12:    case XPMAP_CHECK_13:
	    case XPMAP_CHECK_14:    case XPMAP_CHECK_15:
	    case XPMAP_CHECK_16:    case XPMAP_CHECK_17:
	    case XPMAP_CHECK_18:    case XPMAP_CHECK_19:
	    case XPMAP_CHECK_20:    case XPMAP_CHECK_21:
	    case XPMAP_CHECK_22:    case XPMAP_CHECK_23:
	    case XPMAP_CHECK_24:    case XPMAP_CHECK_25:
		Xpmap_place_check(blk, (int) (c - XPMAP_CHECK_0));
		break;

	    case XPMAP_FRICTION_AREA:
		Xpmap_place_friction_area(blk);
		break;

	    case XPMAP_DECOR_FILLED:
		Xpmap_place_block(blk, DECOR_FILLED);
		break;
	    case XPMAP_DECOR_LU:
		Xpmap_place_block(blk, DECOR_LU);
		break;
	    case XPMAP_DECOR_RU:
		Xpmap_place_block(blk, DECOR_RU);
		break;
	    case XPMAP_DECOR_LD:
		Xpmap_place_block(blk, DECOR_LD);
		break;
	    case XPMAP_DECOR_RD:
		Xpmap_place_block(blk, DECOR_RD);
		break;
	    }
	}
    }
}


void Xpmap_find_map_object_teams(void)
{
    int i;
    clpos_t pos = { 0, 0 };

    if (!BIT(world->rules->mode, TEAM_PLAY))
	return;

    /* This could return -1 */
    if (Find_closest_team(pos) == TEAM_NOT_SET) {
	warn("Broken map: Couldn't find teams for map objects.");
	return;
    }

    /*
     * Determine which team a stuff belongs to.
     */
    for (i = 0; i < Num_treasures(); i++) {
	treasure_t *treasure = Treasure_by_index(i);
	team_t *teamp;

	treasure->team = Find_closest_team(treasure->pos);
	teamp = Team_by_index(treasure->team);
	assert(teamp != NULL);

	teamp->NumTreasures++;
	if (treasure->empty)
	    teamp->NumEmptyTreasures++;
	else
	    teamp->TreasuresLeft++;
    }

    for (i = 0; i < Num_targets(); i++) {
	target_t *targ = Target_by_index(i);

	targ->team = Find_closest_team(targ->pos);
    }

    if (options.teamCannons) {
	for (i = 0; i < Num_cannons(); i++) {
	    cannon_t *cannon = Cannon_by_index(i);

	    cannon->team = Find_closest_team(cannon->pos);
	}
    }

    for (i = 0; i < Num_fuels(); i++) {
	fuel_t *fs = Fuel_by_index(i);

	fs->team = Find_closest_team(fs->pos);
    }
}


/*
 * Find the correct direction of the base, according to the gravity in
 * the base region.
 *
 * If a base attractor is adjacent to a base then the base will point
 * to the attractor.
 */
void Xpmap_find_base_direction(void)
{
    int	i;
    blkpos_t blk;

    for (i = 0; i < Num_bases(); i++) {
	base_t *base = Base_by_index(i);
	int x, y, dir, att;
	vector_t gravity = World_gravity(base->pos);

	if (gravity.x == 0.0 && gravity.y == 0.0)
	    /*
	     * Undefined direction
	     * Should be set to direction of gravity!
	     */
	    dir = DIR_UP;
	else {
	    double a = findDir(-gravity.x, -gravity.y);

	    dir = MOD2((int) (a + 0.5), RES);
	    dir = ((dir + RES/8) / (RES/4)) * (RES/4);	/* round it */
	    dir = MOD2(dir, RES);
	}
	att = -1;

	x = CLICK_TO_BLOCK(base->pos.cx);
	y = CLICK_TO_BLOCK(base->pos.cy);

	/* First check upwards attractor */
	if (y == world->y - 1 && world->block[x][0] == BASE_ATTRACTOR
	    && BIT(world->rules->mode, WRAP_PLAY)) {
	    if (att == -1 || dir == DIR_UP)
		att = DIR_UP;
	}
	if (y < world->y - 1 && world->block[x][y + 1] == BASE_ATTRACTOR) {
	    if (att == -1 || dir == DIR_UP)
		att = DIR_UP;
	}

	/* then downwards */
	if (y == 0 && world->block[x][world->y-1] == BASE_ATTRACTOR
	    && BIT(world->rules->mode, WRAP_PLAY)) {
	    if (att == -1 || dir == DIR_DOWN)
		att = DIR_DOWN;
	}
	if (y > 0 && world->block[x][y - 1] == BASE_ATTRACTOR) {
	    if (att == -1 || dir == DIR_DOWN)
		att = DIR_DOWN;
	}

	/* then rightwards */
	if (x == world->x - 1 && world->block[0][y] == BASE_ATTRACTOR
	    && BIT(world->rules->mode, WRAP_PLAY)) {
	    if (att == -1 || dir == DIR_RIGHT)
		att = DIR_RIGHT;
	}
	if (x < world->x - 1 && world->block[x + 1][y] == BASE_ATTRACTOR) {
	    if (att == -1 || dir == DIR_RIGHT)
		att = DIR_RIGHT;
	}

	/* then leftwards */
	if (x == 0 && world->block[world->x-1][y] == BASE_ATTRACTOR
	    && BIT(world->rules->mode, WRAP_PLAY)) {
	    if (att == -1 || dir == DIR_LEFT)
		att = DIR_LEFT;
	}
	if (x > 0 && world->block[x - 1][y] == BASE_ATTRACTOR) {
	    if (att == -1 || dir == DIR_LEFT)
		att = DIR_LEFT;
	}

	if (att != -1)
	    dir = att;
	base->dir = dir;
    }
    for (blk.bx = 0; blk.bx < world->x; blk.bx++) {
	for (blk.by = 0; blk.by < world->y; blk.by++) {
	    if (World_get_block(blk) == BASE_ATTRACTOR)
		World_set_block(blk, SPACE);
	}
    }
}


/*
 * The following functions is for converting the block based map data
 * to polygons.
 */

/* number of vertices in polygon */
#define N (2 + 12)
static void Xpmap_treasure_to_polygon(int treasure_ind)
{
    int cx, cy, i, r, n;
    double angle;
    int polystyle, edgestyle;
    treasure_t *treasure = Treasure_by_index(treasure_ind);
    clpos_t pos[N + 1];

    polystyle = P_get_poly_id("treasure_ps");
    edgestyle = P_get_edge_id("treasure_es");

    cx = treasure->pos.cx - BLOCK_CLICKS / 2;
    cy = treasure->pos.cy - BLOCK_CLICKS / 2;

    pos[0].cx = cx;
    pos[0].cy = cy;
    pos[1].cx = cx + (BLOCK_CLICKS - 1);
    pos[1].cy = cy;

    cx = treasure->pos.cx;
    cy = treasure->pos.cy;
    /* -1 is to ensure the vertices are inside the block */
    r = (BLOCK_CLICKS / 2) - 1;
    /* number of points in half circle */
    n = N - 2;

    for (i = 0; i < n; i++) {
	angle = (((double)i)/(n - 1)) * PI;
	pos[i + 2].cx = (click_t)(cx + r * cos(angle));
	pos[i + 2].cy = (click_t)(cy + r * sin(angle));
    }

    pos[N] = pos[0];

    /* create balltarget */
    P_start_balltarget(treasure->team, treasure_ind);
    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= N; i++)
	P_vertex(pos[i], edgestyle);
    P_end_polygon();
    P_end_balltarget();

    /* create ballarea */
    P_start_ballarea();
    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= N; i++)
	P_vertex(pos[i], edgestyle);
    P_end_polygon();
    P_end_ballarea();
}
#undef N


static void Xpmap_block_polygon(clpos_t bpos, int polystyle, int edgestyle,
				int destroyed_style)
{
    clpos_t pos[5];
    int i;

    bpos.cx = CLICK_TO_BLOCK(bpos.cx) * BLOCK_CLICKS;
    bpos.cy = CLICK_TO_BLOCK(bpos.cy) * BLOCK_CLICKS;

    pos[0].cx = bpos.cx;
    pos[0].cy = bpos.cy;
    pos[1].cx = bpos.cx + (BLOCK_CLICKS - 1);
    pos[1].cy = bpos.cy;
    pos[2].cx = bpos.cx + (BLOCK_CLICKS - 1);
    pos[2].cy = bpos.cy + (BLOCK_CLICKS - 1);
    pos[3].cx = bpos.cx;
    pos[3].cy = bpos.cy + (BLOCK_CLICKS - 1);
    pos[4] = pos[0];

    P_start_polygon(pos[0], polystyle);
    if (destroyed_style >= 0)
	P_style("destroyed", destroyed_style);
    for (i = 1; i <= 4; i++)
	P_vertex(pos[i], edgestyle);
    P_end_polygon();
}


static void Xpmap_target_to_polygon(int target_ind)
{
    int ps, es, ds;
    target_t *targ = Target_by_index(target_ind);

    ps = P_get_poly_id("target_ps");
    es = P_get_edge_id("target_es");
    ds = P_get_poly_id("destroyed_ps");

    /* create target polygon */
    P_start_target(target_ind);
    Xpmap_block_polygon(targ->pos, ps, es, ds);
    P_end_target();
}


static void Xpmap_cannon_polygon(cannon_t *cannon,
				 int polystyle, int edgestyle)
{
    clpos_t pos[4], cpos = cannon->pos;
    int i, ds;

    pos[0] = cannon->pos;

    cpos.cx = CLICK_TO_BLOCK(cpos.cx) * BLOCK_CLICKS;
    cpos.cy = CLICK_TO_BLOCK(cpos.cy) * BLOCK_CLICKS;

    switch (cannon->dir) {
    case DIR_RIGHT:
	pos[1].cx = cpos.cx;
	pos[1].cy = cpos.cy + (BLOCK_CLICKS - 1);
	pos[2].cx = cpos.cx;
	pos[2].cy = cpos.cy;
	break;
    case DIR_UP:
	pos[1].cx = cpos.cx;
	pos[1].cy = cpos.cy;
	pos[2].cx = cpos.cx + (BLOCK_CLICKS - 1);
	pos[2].cy = cpos.cy;
	break;
    case DIR_LEFT:
	pos[1].cx = cpos.cx + (BLOCK_CLICKS - 1);
	pos[1].cy = cpos.cy;
	pos[2].cx = cpos.cx + (BLOCK_CLICKS - 1);
	pos[2].cy = cpos.cy + (BLOCK_CLICKS - 1);
	break;
    case DIR_DOWN:
	pos[1].cx = cpos.cx + (BLOCK_CLICKS - 1);
	pos[1].cy = cpos.cy + (BLOCK_CLICKS - 1);
	pos[2].cx = cpos.cx;
	pos[2].cy = cpos.cy + (BLOCK_CLICKS - 1);
	break;
    default:
 	/* can't happen */
	assert(0 && "Unknown cannon direction.");
	break;
    }
    pos[3] = pos[0];

    ds = P_get_poly_id("destroyed_ps");
    P_start_polygon(pos[0], polystyle);
    P_style("destroyed", ds);
    for (i = 1; i <= 3; i++)
	P_vertex(pos[i], edgestyle);
    P_end_polygon();
}


static void Xpmap_cannon_to_polygon(int cannon_ind)
{
    int ps, es;
    cannon_t *cannon = Cannon_by_index(cannon_ind);

    ps = P_get_poly_id("cannon_ps");
    es = P_get_edge_id("cannon_es");

    P_start_cannon(cannon_ind);
    Xpmap_cannon_polygon(cannon, ps, es);
    P_end_cannon();
}

#define N 12
static void Xpmap_wormhole_to_polygon(int wormhole_ind)
{
    int ps, es, i, r;
    double angle;
    wormhole_t *wormhole = Wormhole_by_index(wormhole_ind);
    clpos_t pos[N + 1], wpos;

    /* don't make a polygon for an out wormhole */
    if (wormhole->type == WORM_OUT)
	return;

    ps = P_get_poly_id("wormhole_ps");
    es = P_get_edge_id("wormhole_es");

    wpos = wormhole->pos;
    r = WORMHOLE_RADIUS;

    for (i = 0; i < N; i++) {
	angle = (((double)i)/ N) * 2 * PI;
	pos[i].cx = (click_t)(wpos.cx + r * cos(angle));
	pos[i].cy = (click_t)(wpos.cy + r * sin(angle));
    }
    pos[N] = pos[0];

    P_start_wormhole(wormhole_ind);
    P_start_polygon(pos[0], ps);
    for (i = 1; i <= N; i++)
	P_vertex(pos[i], es);
    P_end_polygon();
    P_end_wormhole();
}

static void Xpmap_friction_area_to_polygon(int fa_ind)
{
    int ps, es;
    friction_area_t *fa = FrictionArea_by_index(fa_ind);

    ps = P_get_poly_id("fa_ps");
    es = P_get_edge_id("fa_es");

    P_start_friction_area(fa_ind);
    Xpmap_block_polygon(fa->pos, ps, es, -1);
    P_end_friction_area();
}

/*
 * Add a wall polygon
 *
 * The polygon consists of a start block and and endblock and possibly
 * some full wall/fuel blocks in between. A total number of numblocks
 * blocks are part of the polygon and must be 1 or more. If numblocks
 * is one, the startblock and endblock are the same block.
 *
 * The block coordinates of the first block is (bx, by)
 *
 * The polygon will have 3 or 4 vertices.
 *
 * Idea: first assume the polygon is a rectangle, then move
 * the vertices depending on the start and end blocks.
 *
 * The vertex index:
 * 0: upper left vertex
 * 1: lower left vertex
 * 2: lower right vertex
 * 3: upper right vertex
 * 4: upper left vertex, second time
 */
static void Xpmap_wall_poly(int bx, int by,
			    int startblock, int endblock, int numblocks,
			    int polystyle, int edgestyle)
{
    int i;
    clpos_t pos[5]; /* positions of vertices */

    if (numblocks < 1)
	return;

    /* first assume we have a rectangle */
    pos[0].cx = bx * BLOCK_CLICKS;
    pos[0].cy = (by + 1) * BLOCK_CLICKS - 1;
    pos[1].cx = bx * BLOCK_CLICKS;
    pos[1].cy = by * BLOCK_CLICKS;
    pos[2].cx = (bx + numblocks) * BLOCK_CLICKS - 1;
    pos[2].cy = by * BLOCK_CLICKS;
    pos[3].cx = (bx + numblocks) * BLOCK_CLICKS - 1;
    pos[3].cy = (by + 1) * BLOCK_CLICKS - 1;

    /* move the vertices depending on the startblock and endblock */
    switch (startblock) {
    case FILLED:
    case REC_LU:
    case REC_LD:
    case FUEL:
	/* no need to move the leftmost 2 vertices */
	break;
    case REC_RU:
	/* move lower left vertex to the right */
	pos[1].cx += (BLOCK_CLICKS - 1);
	break;
    case REC_RD:
	/* move upper left vertex to the right */
	pos[0].cx += (BLOCK_CLICKS - 1);
	break;
    default:
	return;
    }

    switch (endblock) {
    case FILLED:
    case FUEL:
    case REC_RU:
    case REC_RD:
	/* no need to move the rightmost 2 vertices */
	break;
    case REC_LU:
	pos[2].cx -= (BLOCK_CLICKS - 1);
	break;
    case REC_LD:
	pos[3].cx -= (BLOCK_CLICKS - 1);
	break;
    default:
	return;
    }

    /*
     * Since we want to form a closed loop of line segments, the
     * last vertex must equal the first.
     */
    pos[4] = pos[0];

    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= 4; i++)
	P_vertex(pos[i], edgestyle);
    P_end_polygon();
}


static void Xpmap_walls_to_polygons(void)
{
    int x, y, x0 = 0;
    int numblocks = 0;
    int inside = false;
    int startblock = 0, endblock = 0, block;
    int maxblocks = POLYGON_MAX_OFFSET / BLOCK_CLICKS;
    int ps, es;

    ps = P_get_poly_id("wall_ps");
    es = P_get_edge_id("wall_es");

    /*
     * x, FILLED = solid wall
     * s, REC_LU = wall triangle pointing left and up
     * a, REC_RU = wall triangle pointing right and up
     * w, REC_LD = wall triangle pointing left and down
     * q, REC_RD = wall triangle pointing right and down
     * #, FUEL   = fuel block
     */

    for (y = world->y - 1; y >= 0; y--) {
	for (x = 0; x < world->x; x++) {
	    block = world->block[x][y];

	    if (!inside) {
		switch (block) {
		case FILLED:
		case REC_RU:
		case REC_RD:
		case FUEL:
		    x0 = x;
		    startblock = endblock = block;
		    inside = true;
		    numblocks = 1;
		    break;

		case REC_LU:
		case REC_LD:
		    Xpmap_wall_poly(x, y, block, block, 1, ps, es);
		    break;
		default:
		    break;
		}
	    } else {

		switch (block) {
		case FILLED:
		case FUEL:
		    numblocks++;
		    endblock = block;
		    break;

		case REC_RU:
		case REC_RD:
		    /* old polygon ends */
		    Xpmap_wall_poly(x0, y, startblock, endblock,
				    numblocks, ps, es);
		    /* and a new one starts */
		    x0 = x;
		    startblock = endblock = block;
		    numblocks = 1;
		    break;

		case REC_LU:
		case REC_LD:
		    numblocks++;
		    endblock = block;
		    Xpmap_wall_poly(x0, y, startblock, endblock,
				    numblocks, ps, es);
		    inside = false;
		    break;

		default:
		    /* none of the above, polygon ends */
		    Xpmap_wall_poly(x0, y, startblock, endblock,
				    numblocks, ps, es);
		    inside = false;
		    break;
		}
	    }

	    /*
	     * We don't want the polygon to have offsets that are too big.
	     */
	    if (inside && numblocks == maxblocks) {
		Xpmap_wall_poly(x0, y, startblock, endblock,
				numblocks, ps, es);
		inside = false;
	    }

	}

	/* end of row */
	if (inside) {
	    Xpmap_wall_poly(x0, y, startblock, endblock,
			    numblocks, ps, es);
	    inside = false;
	}
    }
}


void Xpmap_blocks_to_polygons(void)
{
    int i;

    /* create edgestyles and polystyles */
    P_edgestyle("wall_es", -1, 0x2244EE, 0);
    P_polystyle("wall_ps", 0x0033AA, 0, P_get_edge_id("wall_es"), 0);

    P_edgestyle("treasure_es", -1, 0xFF0000, 0);
    P_polystyle("treasure_ps", 0xFF0000, 0, P_get_edge_id("treasure_es"), 0);

    P_edgestyle("target_es", 3, 0xFF7700, 0);
    P_polystyle("target_ps", 0xFF7700, 3, P_get_edge_id("target_es"), 0);

    P_edgestyle("cannon_es", 3, 0xFFFFFF, 0);
    P_polystyle("cannon_ps", 0xFFFFFF, 2, P_get_edge_id("cannon_es"), 0);

    P_edgestyle("destroyed_es", 3, 0xFF0000, 0);
    P_polystyle("destroyed_ps", 0xFF0000, 2, P_get_edge_id("destroyed_es"),
		STYLE_INVISIBLE|STYLE_INVISIBLE_RADAR);

    P_edgestyle("wormhole_es", -1, 0x00FFFF, 0);
    P_polystyle("wormhole_ps", 0x00FFFF, 2, P_get_edge_id("wormhole_es"), 0);

    P_edgestyle("fa_es", 2, 0xFF1F00, 0);
    P_polystyle("fa_ps", 0xCF1F00, 2, P_get_edge_id("fa_es"), 0);

    Xpmap_walls_to_polygons();

    if (options.polygonMode)
	is_polygon_map = true;

    for (i = 0; i < Num_treasures(); i++)
	Xpmap_treasure_to_polygon(i);

    for (i = 0; i < Num_targets(); i++)
	Xpmap_target_to_polygon(i);

    for (i = 0; i < Num_cannons(); i++)
	Xpmap_cannon_to_polygon(i);

    for (i = 0; i < Num_wormholes(); i++)
	Xpmap_wormhole_to_polygon(i);

    for (i = 0; i < Num_frictionAreas(); i++)
	Xpmap_friction_area_to_polygon(i);

    /*xpprintf("Created %d polygons.\n", num_polys);*/
}
