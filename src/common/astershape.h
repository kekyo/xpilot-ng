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

#ifndef ASTERSHAPE_H
#define ASTERSHAPE_H

#include "types.h"

#define NUM_ASTEROID_SHAPES 2
#define NUM_ASTEROID_POINTS 12

#define ASTEROID_SHAPE_0 \
      {-10,0}, {-7, 6}, {-2, 8}, { 0,10}, { 5, 8}, { 9, 4}, \
      {10, 0}, { 7,-5}, { 6,-9}, {0,-10}, {-5,-7}, {-7,-5}


#define ASTEROID_SHAPE_1 \
      {-10,0}, {-8, 7}, {-4, 9}, { 0,10}, { 5, 7}, { 6, 3}, \
      {10, 0}, { 9,-4}, { 7,-7}, {0,-10}, {-6,-9}, {-9,-7}

extern position_t *asteroidShapes[NUM_ASTEROID_SHAPES][NUM_ASTEROID_POINTS];

#endif
