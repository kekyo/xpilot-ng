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

#define GRAV_RANGE  10

static void Compute_global_gravity(void)
{
    int xi, yi, dx, dy;
    double xforce, yforce, strength, theta;
    vector_t *grav;

    if (options.gravityPointSource == false) {
	theta = (options.gravityAngle * PI) / 180.0;
	xforce = cos(theta) * options.gravity;
	yforce = sin(theta) * options.gravity;
	for (xi = 0; xi < world->x; xi++) {
	    grav = world->gravity[xi];

	    for (yi = 0; yi < world->y; yi++, grav++) {
		grav->x = xforce;
		grav->y = yforce;
	    }
	}
    } else {
	for (xi = 0; xi < world->x; xi++) {
	    grav = world->gravity[xi];
	    dx = (xi - options.gravityPoint.x) * BLOCK_SZ;
	    dx = WRAP_DX(dx);

	    for (yi = 0; yi < world->y; yi++, grav++) {
		dy = (yi - options.gravityPoint.y) * BLOCK_SZ;
		dy = WRAP_DX(dy);

		if (dx == 0 && dy == 0) {
		    grav->x = 0.0;
		    grav->y = 0.0;
		    continue;
		}
		strength = options.gravity / LENGTH(dx, dy);
		if (options.gravityClockwise) {
		    grav->x =  dy * strength;
		    grav->y = -dx * strength;
		}
		else if (options.gravityAnticlockwise) {
		    grav->x = -dy * strength;
		    grav->y =  dx * strength;
		}
		else {
		    grav->x =  dx * strength;
		    grav->y =  dy * strength;
		}
	    }
	}
    }
}


static void Compute_grav_tab(vector_t grav_tab[GRAV_RANGE+1][GRAV_RANGE+1])
{
    int			x, y;
    double		strength;

    grav_tab[0][0].x = grav_tab[0][0].y = 0;
    for (x = 0; x < GRAV_RANGE+1; x++) {
	for (y = (x == 0); y < GRAV_RANGE+1; y++) {
	    strength = pow((double)(sqr(x) + sqr(y)), -1.5);
	    grav_tab[x][y].x = x * strength;
	    grav_tab[x][y].y = y * strength;
	}
    }
}


static void Compute_local_gravity(void)
{
    int xi, yi, i, gx, gy, ax, ay, dx, dy, gtype;
    int first_xi, last_xi, first_yi, last_yi, mod_xi, mod_yi;
    int min_xi, max_xi, min_yi, max_yi;
    double force, fx, fy;
    vector_t *v, *grav, *tab, grav_tab[GRAV_RANGE+1][GRAV_RANGE+1];

    Compute_grav_tab(grav_tab);

    min_xi = 0;
    max_xi = world->x - 1;
    min_yi = 0;
    max_yi = world->y - 1;
    if (BIT(world->rules->mode, WRAP_PLAY)) {
	min_xi -= MIN(GRAV_RANGE, world->x);
	max_xi += MIN(GRAV_RANGE, world->x);
	min_yi -= MIN(GRAV_RANGE, world->y);
	max_yi += MIN(GRAV_RANGE, world->y);
    }
    for (i = 0; i < Num_gravs(); i++) {
	grav_t *g = Grav_by_index(i);

	gx = CLICK_TO_BLOCK(g->pos.cx);
	gy = CLICK_TO_BLOCK(g->pos.cy);
	force = g->force;

	if ((first_xi = gx - GRAV_RANGE) < min_xi)
	    first_xi = min_xi;
	if ((last_xi = gx + GRAV_RANGE) > max_xi)
	    last_xi = max_xi;
	if ((first_yi = gy - GRAV_RANGE) < min_yi)
	    first_yi = min_yi;
	if ((last_yi = gy + GRAV_RANGE) > max_yi)
	    last_yi = max_yi;

	gtype = g->type;

	mod_xi = (first_xi < 0) ? (first_xi + world->x) : first_xi;
	dx = gx - first_xi;
	fx = force;
	for (xi = first_xi; xi <= last_xi; xi++, dx--) {
	    if (dx < 0) {
		fx = -force;
		ax = -dx;
	    } else
		ax = dx;

	    mod_yi = (first_yi < 0) ? (first_yi + world->y) : first_yi;
	    dy = gy - first_yi;
	    grav = &world->gravity[mod_xi][mod_yi];
	    tab = grav_tab[ax];
	    fy = force;
	    for (yi = first_yi; yi <= last_yi; yi++, dy--) {
		if (dx || dy) {
		    if (dy < 0) {
			fy = -force;
			ay = -dy;
		    } else
			ay = dy;

		    v = &tab[ay];
		    if (gtype == CWISE_GRAV || gtype == ACWISE_GRAV) {
			grav->x -= fy * v->y;
			grav->y += fx * v->x;
		    } else if (gtype == UP_GRAV || gtype == DOWN_GRAV)
			grav->y += force * v->x;
		    else if (gtype == RIGHT_GRAV || gtype == LEFT_GRAV)
			grav->x += force * v->y;
		    else {
			grav->x += fx * v->x;
			grav->y += fy * v->y;
		    }
		}
		else {
		    if (gtype == UP_GRAV || gtype == DOWN_GRAV)
			grav->y += force;
		    else if (gtype == LEFT_GRAV || gtype == RIGHT_GRAV)
			grav->x += force;
		}
		mod_yi++;
		grav++;
		if (mod_yi >= world->y) {
		    mod_yi = 0;
		    grav = world->gravity[mod_xi];
		}
	    }
	    if (++mod_xi >= world->x)
		mod_xi = 0;
	}
    }
    /*
     * We may want to free the world->gravity memory here
     * as it is not used anywhere else.
     * e.g.: free(world->gravity);
     *       world->gravity = NULL;
     *       world->NumGravs = 0;
     * Some of the more modern maps have quite a few gravity symbols.
     */
}


void Compute_gravity(void)
{
    Compute_global_gravity();
    Compute_local_gravity();
}
