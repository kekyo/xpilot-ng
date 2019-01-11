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

shape_t wormhole_wire;

/*
 * Initialization functions.
 */

void Wormhole_line_init(void)
{
    int i;
    static clpos_t coords[MAX_SHIP_PTS];

    wormhole_wire.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++) {
	wormhole_wire.pts[i] = coords + i;
	coords[i].cx = (int)(cos(i * 2 * PI / MAX_SHIP_PTS) * WORMHOLE_RADIUS);
	coords[i].cy = (int)(sin(i * 2 * PI / MAX_SHIP_PTS) * WORMHOLE_RADIUS);
    }

    return;
}

bool Verify_wormhole_consistency(void)
{
    int i, worm_in = 0, worm_out = 0, worm_norm = 0;

    /* count wormhole types */
    for (i = 0; i < Num_wormholes(); i++) {
	int type = Wormhole_by_index(i)->type;

	if (type == WORM_NORMAL)
	    worm_norm++;
	else if (type == WORM_IN)
	    worm_in++;
	else if (type == WORM_OUT)
	    worm_out++;
    }

    /*
     * Verify that the wormholes are consistent, i.e. that if
     * we have no 'out' wormholes, make sure that we don't have
     * any 'in' wormholes, and (less critical) if we have no 'in'
     * wormholes, make sure that we don't have any 'out' wormholes.
     */
    if (worm_norm > 0) {
	if (worm_norm + worm_out < 2) {
	    warn("Map has only one 'normal' wormhole.");
	    warn("Add at least one 'normal' or 'out' wormhole.");
	    return false;
	}
    } else if (worm_in > 0) {
	if (worm_out < 1) {
	    warn("Map has %d 'in' wormholes, "
		 "but no 'normal' or 'out' wormholes.", worm_in);
	    warn("Add at least one 'normal' or 'out' wormhole.");
	    return false;
	}
    } else if (worm_out > 0) {
	warn("Map has %d 'out' wormholes, but no 'normal' or 'in' wormholes.",
	     worm_out);
	warn("Add at least one 'normal' or 'in' wormhole.");
	return false;
    }

    return true;
}

/*
 * Functions used in game.
 */

hitmask_t Wormhole_hitmask(wormhole_t *wormhole)
{
    if (wormhole->type == WORM_OUT)
	return ALL_BITS;
    return 0;
}

bool Wormhole_hitfunc(group_t *gp, const move_t *move)
{
    const object_t *obj = move->obj;
    wormhole_t *wormhole = Wormhole_by_index(gp->mapobj_ind);

    if (wormhole->type == WORM_OUT)
	return false;

    if (obj == NULL)
	return true;

    if (BIT(obj->obj_status, WARPED|WARPING))
	return false;

    return true;
}

void Object_hits_wormhole(object_t *obj, int ind)
{
    SET_BIT(obj->obj_status, WARPING);
    obj->wormHoleHit = ind;
}

/*
 * Warp balls connected to warped player.
 */
static void Warp_balls(player_t *pl, clpos_t dest)
{
    /*
     * Don't connect to balls while warping.
     */
    if (Player_uses_connector(pl))
	pl->ball = NULL;

    if (BIT(pl->have, HAS_BALL)) {
	/*
	 * Warp every ball associated with player.
	 * NB. the connector can cross a wall boundary this is
	 * allowed, so long as the ball itself doesn't collide.
	 */
	int k;

	for (k = 0; k < NumObjs; k++) {
	    object_t *b = Obj[k];

	    if (b->type == OBJ_BALL && b->id == pl->id) {
		clpos_t ballpos;
		hitmask_t hitmask = BALL_BIT|HITMASK(pl->team);

		ballpos.cx = b->pos.cx + dest.cx - pl->pos.cx;
		ballpos.cy = b->pos.cy + dest.cy - pl->pos.cy;
		ballpos = World_wrap_clpos(ballpos);
		if (!World_contains_clpos(ballpos)
		    || (shape_is_inside(ballpos.cx, ballpos.cy, hitmask,
					(object_t *)b, &ball_wire, 0)
			!= NO_GROUP)) {
		    b->life = 0.0;
		    continue;
		}
		Object_position_set_clpos(b, ballpos);
		Object_position_remember(b);
		Cell_add_object(b);
	    }
	}
    }
}

static int Find_wormhole_dest(int wh_hit_ind)
{
    int wh_ind;
    wormhole_t *wh, *wh_hit = Wormhole_by_index(wh_hit_ind);

    if (wh_hit->type == WORM_FIXED)
	return wh_hit_ind;

    if (wh_hit->countdown > 0)
	return wh_hit->lastdest;

    do {
	wh_ind = (int)(rfrac() * Num_wormholes());
	wh = Wormhole_by_index(wh_ind);
    }
    while (wh->type == WORM_IN
	   || wh->type == WORM_FIXED
	   || wh_hit_ind == wh_ind);

    return wh_ind;
}

/*
 * Move player trough wormhole.
 */
static void Traverse_wormhole(player_t *pl)
{
    clpos_t dest;
    int wh_dest;
    wormhole_t *wh_hit = Wormhole_by_index(pl->wormHoleHit);

    wh_dest = Find_wormhole_dest(pl->wormHoleHit);
    /*assert(wh_dest != pl->wormHoleHit);*/
    sound_play_sensors(pl->pos, WORM_HOLE_SOUND);
    dest = Wormhole_by_index(wh_dest)->pos;
    Warp_balls(pl, dest);
    pl->wormHoleDest = wh_dest;
    Object_position_init_clpos(OBJ_PTR(pl), dest);
    pl->forceVisible += 15;
    /*assert(pl->wormHoleHit != NO_IND);*/

    if (wh_dest != pl->wormHoleHit) {
	wh_hit->lastdest = wh_dest;
	wh_hit->countdown = options.wormholeStableTicks;
    }
    /*else
      assert(0);*/

    CLR_BIT(pl->obj_status, WARPING);
    SET_BIT(pl->obj_status, WARPED);

    sound_play_sensors(pl->pos, WORM_HOLE_SOUND);
}

/*
 * Returns true if warp status was achieved.
 */
bool Initiate_hyperjump(player_t *pl)
{
    if (pl->item[ITEM_HYPERJUMP] <= 0)
	return false;
    if (pl->fuel.sum < -ED_HYPERJUMP)
	return false;
    pl->item[ITEM_HYPERJUMP]--;
    Player_add_fuel(pl, ED_HYPERJUMP);
    SET_BIT(pl->obj_status, WARPING);
    pl->wormHoleHit = -1;
    return true;
}

/*
 * Player has used hyperjump item.
 */
static void Hyperjump(player_t *pl)
{
    clpos_t dest;
    int counter;
    hitmask_t hitmask = NONBALL_BIT | HITMASK(pl->team); /* kps - ok ? */

    /* Try to find empty space to hyperjump to. */
    for (counter = 100; counter > 0; counter--) {
	dest = World_get_random_clpos();
	if (shape_is_inside(dest.cx, dest.cy, hitmask, OBJ_PTR(pl),
			    (shape_t *)pl->ship, pl->dir) == NO_GROUP)
	    break;
    }

    /* We can't find an empty space, hyperjump failed. */
    if (!counter) {
	/* need to do something else here ? */
	Set_player_message(pl, "Could not hyperjump. [*Server notice*]");
	CLR_BIT(pl->obj_status, WARPING);
	sound_play_sensors(pl->pos, HYPERJUMP_SOUND);
	return;
    }

    sound_play_sensors(pl->pos, HYPERJUMP_SOUND);

    Warp_balls(pl, dest);

    Object_position_init_clpos(OBJ_PTR(pl), dest);
    pl->forceVisible += 15;

    CLR_BIT(pl->obj_status, WARPING);
}

void Player_warp(player_t *pl)
{
    if (pl->wormHoleHit == NO_IND)
	Hyperjump(pl);
    else
	Traverse_wormhole(pl);
}

void Player_finish_warp(player_t *pl)
{
    int group;
    hitmask_t hitmask = NONBALL_BIT | HITMASK(pl->team);
    /*
     * clear warped, so we can use shape_is inside,
     * Wormhole_hitfunc check for WARPED bit.
     */
    CLR_BIT(pl->obj_status, WARPED);
    group = shape_is_inside(pl->pos.cx, pl->pos.cy, hitmask,
			    OBJ_PTR(pl), (shape_t *)pl->ship,
			    pl->dir);
    /*
     * kps - we might possibly have entered another polygon, e.g.
     * a wormhole ?
     */
    if (group != NO_GROUP)
	SET_BIT(pl->obj_status, WARPED);
}

void Object_warp(object_t *obj)
{
    clpos_t dest;
    int wh_dest;
    wormhole_t *wh_hit = Wormhole_by_index(obj->wormHoleHit);

    wh_dest = Find_wormhole_dest(obj->wormHoleHit);
    /*assert(wh_dest != obj->wormHoleHit);*/
    dest = Wormhole_by_index(wh_dest)->pos;
    obj->wormHoleDest = wh_dest;
    Object_position_init_clpos(obj, dest);
    /*assert(obj->wormHoleHit != NO_IND);*/

    if (wh_dest != obj->wormHoleHit) {
	wh_hit->lastdest = wh_dest;
	wh_hit->countdown = options.wormholeStableTicks;
    }
    /*else
      assert(0);*/

    CLR_BIT(obj->obj_status, WARPING);
    SET_BIT(obj->obj_status, WARPED);
}

void Object_finish_warp(object_t *obj)
{
    int group;
    hitmask_t hitmask = NONBALL_BIT | HITMASK(obj->team);
    /*
     * clear warped, so we can use shape_is inside,
     * Wormhole_hitfunc check for WARPED bit.
     */
    CLR_BIT(obj->obj_status, WARPED);
    group = is_inside(obj->pos.cx, obj->pos.cy, hitmask, obj);

    /*
     * kps - we might possibly have entered another polygon, e.g.
     * a wormhole ?
     */
    if (group != NO_GROUP)
	SET_BIT(obj->obj_status, WARPED);
}
