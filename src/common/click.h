/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2004 by
 *
 *      Uoti Urpala          <uau@users.sourceforge.net>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#ifndef CLICK_H
#define CLICK_H

#ifndef CONST_H
# include "const.h"
#endif

/*
 * The wall collision detection routines depend on repeatability
 * (getting the same result even after some "neutral" calculations)
 * and an exact determination whether a point is in space,
 * inside the wall (crash!) or on the edge.
 * This will be hard to achieve if only floating point would be used.
 * However, a resolution of a pixel is a bit rough and ugly.
 * Therefore a fixed point sub-pixel resolution is used called clicks.
 */

typedef int click_t;

typedef struct {
    click_t		cx, cy;
} clpos_t;

typedef struct {
    click_t		cx, cy;
} clvec_t;

#define CLICK_SHIFT		6
#define CLICK			(1 << CLICK_SHIFT)
#define PIXEL_CLICKS		CLICK
#define BLOCK_CLICKS		(BLOCK_SZ << CLICK_SHIFT)
#define CLICK_TO_PIXEL(C)	((int)((C) >> CLICK_SHIFT))
#define CLICK_TO_BLOCK(C)	((int)((C) / (BLOCK_SZ << CLICK_SHIFT)))
#define CLICK_TO_FLOAT(C)	((double)(C) * (1.0 / CLICK))
#define PIXEL_TO_CLICK(I)	((click_t)(I) << CLICK_SHIFT)
#define FLOAT_TO_CLICK(F)	((int)((F) * CLICK))

/*
 * Return the block position this click position is in.
 */
static inline blkpos_t Clpos_to_blkpos(clpos_t pos)
{
    blkpos_t bpos;

    bpos.bx = CLICK_TO_BLOCK(pos.cx);
    bpos.by = CLICK_TO_BLOCK(pos.cy);

    return bpos;
}

#define BLOCK_CENTER(B) ((int)((B) * BLOCK_CLICKS) + BLOCK_CLICKS / 2)

/* calculate the clpos of the center of a block */
static inline clpos_t Block_get_center_clpos(blkpos_t bpos)
{
    clpos_t pos;

    pos.cx = (bpos.bx * BLOCK_CLICKS) + BLOCK_CLICKS / 2;
    pos.cy = (bpos.by * BLOCK_CLICKS) + BLOCK_CLICKS / 2;

    return pos;
}

#endif
