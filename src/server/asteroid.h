/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-1998 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *  	Guido Koopman        <guido@xpilot.org>
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

#ifndef ASTEROID_H
#define ASTEROID_H

/* maximum size of asteroid */
#define ASTEROID_MAX_SIZE	4
/* mass of asteroid size 1 */
#define ASTEROID_BASE_MASS	(options.shipMass * 3)
/* amount of mass lost in breaking, relative to asteroids size n - 1 */
#define ASTEROID_DUST_MASS	0.25
/* factor of above, relative to asteroid size n */
#define ASTEROID_DUST_FACT	(1 / (1 + 2 / ASTEROID_DUST_MASS))
/* mass of asteroid size n */
#define ASTEROID_MASS(size)	(ASTEROID_BASE_MASS \
				 * pow(2.0 + ASTEROID_DUST_MASS, (size) - 1.0))
/* maximum angle between asteroids produced by breaking */
#define ASTEROID_DELTA_DIR	(RES / 8)
/* lifetime of asteroid before breaking */
#define ASTEROID_LIFE		1000
/* number of hits asteroid can take before breaking */
#define ASTEROID_HITS(size)	(1 << ((size) - 1))
/* fuel cost to lifetime reduction conversion */
#define ASTEROID_FUEL_HIT(fuel, size)	(((fuel) * ASTEROID_LIFE) / \
				(25.0 * ASTEROID_HITS(size)))
/* initial speed of asteroid */
#define ASTEROID_START_SPEED	(8 + rfrac() * 10)
/* minimum distance asteroids start away from any player */
#define ASTEROID_MIN_DIST	(5 * BLOCK_CLICKS)
/* radius of asteroid size n */
#define ASTEROID_RADIUS(size)	((0.8 * SHIP_SZ * (size)) * CLICK)

extern shape_t asteroid_wire1;
extern shape_t asteroid_wire2;
extern shape_t asteroid_wire3;
extern shape_t asteroid_wire4;

static inline shape_t *Asteroid_get_shape_by_size(int size)
{
    switch (size) {
    case 1:
	return &asteroid_wire1;
    case 2:
	return &asteroid_wire2;
    case 3:
	return &asteroid_wire3;
    case 4:
	return &asteroid_wire4;
    default:
	return NULL;
    }
}

#endif
