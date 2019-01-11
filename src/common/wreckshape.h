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

#ifndef WRECKSHAPE_H
#define WRECKSHAPE_H

#include "types.h"

#define NUM_WRECKAGE_SHAPES 3
#define NUM_WRECKAGE_POINTS 12

#define WRECKAGE_SHAPE_0 \
      {-9, 6}, {-2, 8}, { 5, 2}, { 9, 3}, {10, 0}, { 5,-1}, \
      { 3, 0}, {-2,-9}, {-5,-6}, {-3,-2}, {-7,-1}, {-5, 2}

#define WRECKAGE_SHAPE_1 \
      {-8,-9}, {-9,-3}, {-7, 3}, {-1, 7}, { 8, 9}, { 9, 6}, \
      { 2, 5}, {-2, 2}, { 4,-1}, { 2,-5}, { 0,-2}, {-5,-2}

#define WRECKAGE_SHAPE_2 \
      {-9,-2}, {-7, 2}, {-2,-3}, { 2,-3}, { 0, 1}, { 1,10}, \
      { 4, 9}, { 4, 2}, { 7,-2}, { 7,-5}, { 2,-8}, {-4,-7}

extern position_t *wreckageShapes[NUM_WRECKAGE_SHAPES][NUM_WRECKAGE_POINTS];

#endif
