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


void Object_position_set_clpos(object_t *obj, clpos_t pos)
{
    if (!World_contains_clpos(pos)) {
	if (0) {
	    printf("BUG!  Illegal object position %d,%d\n", pos.cx, pos.cy);
	    printf("      Type = %d (%s)\n", obj->type, Object_typename(obj));
	    *(double *)(-1) = 4321.0;
	    abort();
	} else {
	    if (obj->type == OBJ_PLAYER)
		Player_crash((player_t *)obj, CrashUnknown, NO_IND, 1);
	    else
		Object_crash(obj, CrashUnknown, NO_IND);
	    return;
	}
    }

    obj->pos = pos;
}

void Object_position_init_clpos(object_t *obj, clpos_t pos)
{
    Object_position_set_clpos(obj, pos);
    Object_position_remember(obj);
    obj->collmode = 0;
}

void Object_position_restore(object_t *obj)
{
    Object_position_set_clpos(obj, obj->prevpos);
}

void Object_position_limit(object_t *obj)
{
    clpos_t pos = obj->pos, oldpos = pos;

    LIMIT(pos.cx, 0, world->cwidth - 1);
    LIMIT(pos.cy, 0, world->cheight - 1);
    if (pos.cx != oldpos.cx || pos.cy != oldpos.cy)
	Object_position_set_clpos(obj, pos);
}

#ifdef DEVELOPMENT
void Player_position_debug(player_t *pl, const char *msg)
{
    int			i;

    printf("pl %s pos dump: ", pl->name);
    if (msg) printf("(%s)", msg);
    printf("\n");
    printf("\tB %d, %d, P %d, %d, C %d, %d, O %d, %d\n",
	   CLICK_TO_BLOCK(pl->pos.cx),
	   CLICK_TO_BLOCK(pl->pos.cy),
	   CLICK_TO_PIXEL(pl->pos.cx),
	   CLICK_TO_PIXEL(pl->pos.cy),
	   pl->pos.cx,
	   pl->pos.cy,
	   pl->prevpos.cx,
	   pl->prevpos.cy);
    for (i = 0; i < pl->ship->num_points; i++) {
	clpos_t pts = Ship_get_point_clpos(pl->ship, i, pl->dir);
	clpos_t pt;

	pt.cx = pl->pos.cx + pts.cx;
	pt.cy = pl->pos.cy + pts.cy;

	printf("\t%2d\tB %d, %d, P %d, %d, C %d, %d, O %d, %d\n",
	       i,
	       CLICK_TO_BLOCK(pt.cx),
	       CLICK_TO_BLOCK(pt.cy),
	       CLICK_TO_PIXEL(pt.cx),
	       CLICK_TO_PIXEL(pt.cy),
	       pt.cx,
	       pt.cy,
	       pl->prevpos.cx + pts.cx,
	       pl->prevpos.cy + pts.cy);
    }
}
#endif
