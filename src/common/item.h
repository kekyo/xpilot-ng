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

#ifndef ITEM_H
#define ITEM_H

typedef enum Item {
    NO_ITEM			= -1,
    ITEM_FUEL			= 0,
    ITEM_WIDEANGLE		= 1,
    ITEM_REARSHOT		= 2,
    ITEM_AFTERBURNER		= 3,
    ITEM_CLOAK			= 4,
    ITEM_SENSOR			= 5,
    ITEM_TRANSPORTER		= 6,
    ITEM_TANK			= 7,
    ITEM_MINE			= 8,
    ITEM_MISSILE		= 9,
    ITEM_ECM			= 10,
    ITEM_LASER			= 11,
    ITEM_EMERGENCY_THRUST	= 12,
    ITEM_TRACTOR_BEAM		= 13,
    ITEM_AUTOPILOT		= 14,
    ITEM_EMERGENCY_SHIELD	= 15,
    ITEM_DEFLECTOR		= 16,
    ITEM_HYPERJUMP		= 17,
    ITEM_PHASING		= 18,
    ITEM_MIRROR			= 19,
    ITEM_ARMOR			= 20,
    NUM_ITEMS			= 21
} Item_t;

#define ITEM_BIT_FUEL			(1U << ITEM_FUEL)
#define ITEM_BIT_WIDEANGLE		(1U << ITEM_WIDEANGLE)
#define ITEM_BIT_REARSHOT		(1U << ITEM_REARSHOT)
#define ITEM_BIT_AFTERBURNER		(1U << ITEM_AFTERBURNER)
#define ITEM_BIT_CLOAK			(1U << ITEM_CLOAK)
#define ITEM_BIT_SENSOR			(1U << ITEM_SENSOR)
#define ITEM_BIT_TRANSPORTER		(1U << ITEM_TRANSPORTER)
#define ITEM_BIT_TANK			(1U << ITEM_TANK)
#define ITEM_BIT_MINE			(1U << ITEM_MINE)
#define ITEM_BIT_MISSILE		(1U << ITEM_MISSILE)
#define ITEM_BIT_ECM			(1U << ITEM_ECM)
#define ITEM_BIT_LASER			(1U << ITEM_LASER)
#define ITEM_BIT_EMERGENCY_THRUST	(1U << ITEM_EMERGENCY_THRUST)
#define ITEM_BIT_TRACTOR_BEAM		(1U << ITEM_TRACTOR_BEAM)
#define ITEM_BIT_AUTOPILOT		(1U << ITEM_AUTOPILOT)
#define ITEM_BIT_EMERGENCY_SHIELD	(1U << ITEM_EMERGENCY_SHIELD)
#define ITEM_BIT_DEFLECTOR		(1U << ITEM_DEFLECTOR)
#define ITEM_BIT_HYPERJUMP		(1U << ITEM_HYPERJUMP)
#define ITEM_BIT_PHASING		(1U << ITEM_PHASING)
#define ITEM_BIT_MIRROR			(1U << ITEM_MIRROR)
#define ITEM_BIT_ARMOR			(1U << ITEM_ARMOR)

/* Each item is ITEM_SIZE x ITEM_SIZE */
#define ITEM_SIZE		16

#define ITEM_TRIANGLE_SIZE	(5*ITEM_SIZE/7 + 1)

#endif /* ITEM_H */
