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

#ifndef	OBJECT_H
#define	OBJECT_H

#ifndef MAP_H
/* need treasure_t */
#include "map.h"
#endif

#ifndef MODIFIERS_H
/* need modifiers_t */
#include "modifiers.h"
#endif

/*
 * Different types of objects, including player.
 * Robots and tanks are players but have an additional type_ext field.
 * Smart missile, heatseeker and torpedo can be merged into missile.
 */
#define OBJ_TYPEBIT(type)	(1U<<(type))

#define OBJ_PLAYER		0
#define OBJ_DEBRIS		1
#define OBJ_SPARK		2
#define OBJ_BALL		3
#define OBJ_SHOT		4
#define OBJ_SMART_SHOT		5
#define OBJ_MINE		6
#define OBJ_TORPEDO		7
#define OBJ_HEAT_SHOT		8
#define OBJ_PULSE		9
#define OBJ_ITEM		10
#define OBJ_WRECKAGE		11
#define OBJ_ASTEROID		12
#define OBJ_CANNON_SHOT		13

#define OBJ_PLAYER_BIT		OBJ_TYPEBIT(OBJ_PLAYER)
#define OBJ_DEBRIS_BIT		OBJ_TYPEBIT(OBJ_DEBRIS)
#define OBJ_SPARK_BIT		OBJ_TYPEBIT(OBJ_SPARK)
#define OBJ_BALL_BIT		OBJ_TYPEBIT(OBJ_BALL)
#define OBJ_SHOT_BIT		OBJ_TYPEBIT(OBJ_SHOT)
#define OBJ_SMART_SHOT_BIT	OBJ_TYPEBIT(OBJ_SMART_SHOT)
#define OBJ_MINE_BIT		OBJ_TYPEBIT(OBJ_MINE)
#define OBJ_TORPEDO_BIT		OBJ_TYPEBIT(OBJ_TORPEDO)
#define OBJ_HEAT_SHOT_BIT	OBJ_TYPEBIT(OBJ_HEAT_SHOT)
#define OBJ_PULSE_BIT		OBJ_TYPEBIT(OBJ_PULSE)
#define OBJ_ITEM_BIT		OBJ_TYPEBIT(OBJ_ITEM)
#define OBJ_WRECKAGE_BIT	OBJ_TYPEBIT(OBJ_WRECKAGE)
#define OBJ_ASTEROID_BIT	OBJ_TYPEBIT(OBJ_ASTEROID)
#define OBJ_CANNON_SHOT_BIT	OBJ_TYPEBIT(OBJ_CANNON_SHOT)

/*
 * Possible object status bits.
 */
#define GRAVITY			(1U<<0)
#define WARPING			(1U<<1)
#define WARPED			(1U<<2)
#define CONFUSED		(1U<<3)
#define FROMCANNON		(1U<<4)		/* Object from cannon */
#define RECREATE		(1U<<5)		/* Recreate ball */
#define THRUSTING		(1U<<6)		/* Engine is thrusting */
#define OWNERIMMUNE		(1U<<7)		/* Owner is immune to object */
#define NOEXPLOSION		(1U<<8)		/* No recreate explosion */
#define COLLISIONSHOVE		(1U<<9)		/* Collision counts as shove */
#define RANDOM_ITEM		(1U<<10)	/* Item shows up as random */

#define LOCK_NONE		0x00	/* No lock */
#define LOCK_PLAYER		0x01	/* Locked on player */
#define LOCK_VISIBLE		0x02	/* Lock information was on HUD */
					/* computed just before frame shown */
					/* and client input checked */
#define LOCKBANK_MAX		4	/* Maximum number of locks in bank */

/*
 * Node within a Cell list.
 */
typedef struct cell_node cell_node_t;
struct cell_node {
    cell_node_t		*next;
    cell_node_t		*prev;
};


#define OBJECT_BASE	\
    short		id;		/* For shots => id of player */	\
    uint16_t		team;		/* Team of player or cannon */	\
/* Object position pos must only be changed with the proper functions! */ \
    clpos_t		pos;		/* World coordinates */		\
    clpos_t		prevpos;	/* previous position */		\
    clpos_t		extmove;	/* For collision detection */	\
    float		wall_time;	/* bounce/crash time within frame */ \
    vector_t		vel;		/* speed in x,y */		\
    vector_t		acc;		/* acceleration in x,y */	\
    float		mass;		/* mass in unigrams */		\
    float		life;		/* No of ticks left to live */	\
    modifiers_t		mods;		/* Modifiers to this object */	\
    uint8_t		type;		/* one of OBJ_XXX */		\
    uint8_t		color;		/* Color of object */		\
    uint8_t		collmode;	/* collision checking mode */	\
    uint8_t		missile_dir;	/* missile direction */		\
    short		wormHoleHit;	\
    short		wormHoleDest;	\
    uint16_t		obj_status;	/* gravity, etc. */		\

/* up to here all object types are the same as all player types. */

#define OBJECT_EXTEND	\
    cell_node_t		cell;		/* node in cell linked list */	\
    short		pl_range;	/* distance for collision */	\
    short		pl_radius;	/* distance for hit */		\
    float		fuse;		/* ticks until fused */ \

/* up to here all object types are the same. */


/*
 * Generic object
 */
typedef struct xp_object object_t;
struct xp_object {

    OBJECT_BASE

    OBJECT_EXTEND

#define OBJ_IND(ind)	(Obj[(ind)])
#define OBJ_PTR(ptr)	((object_t *)(ptr))
};

/*
 * Mine object
 */
typedef struct xp_mineobject mineobject_t;
struct xp_mineobject {

    OBJECT_BASE

    OBJECT_EXTEND

    float		mine_count;	/* Misc snafus */
    float		mine_ecm_range;	/* Range from last ecm center */
    float		mine_spread_left;	/* how much spread time left */
    short 		mine_owner;		/* Who's object is this ? */

#define MINE_IND(ind)	((mineobject_t *)Obj[(ind)])
#define MINE_PTR(ptr)	((mineobject_t *)(ptr))
};


#define MISSILE_EXTEND		\
    float		missile_max_speed;	/* speed limitation */	\
    float		missile_turnspeed;	/* how fast to turn */


/* up to here all missiles types are the same. */

/*
 * Generic missile object
 */
typedef struct xp_missileobject missileobject_t;
struct xp_missileobject {

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

#define MISSILE_IND(ind)	((missileobject_t *)Obj[(ind)])
#define MISSILE_PTR(ptr)	((missileobject_t *)(ptr))
};


/*
 * Smart missile is a generic missile with extras.
 */
typedef struct xp_smartobject smartobject_t;
struct xp_smartobject {

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    float		smart_ecm_range;	/* Range from last ecm center*/
    float		smart_count;	/* Misc snafus */
    short		smart_lock_id;	/* snafu */
    short		smart_relock_id;	/* smart re-lock id */

#define SMART_IND(ind)	((smartobject_t *)Obj[(ind)])
#define SMART_PTR(ptr)	((smartobject_t *)(ptr))
};


/*
 * Torpedo is a generic missile with extras
 */
typedef struct xp_torpobject torpobject_t;
struct xp_torpobject {

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    float		torp_spread_left;	/* how much spread time left */
    float		torp_count;	/* Misc snafus */

#define TORP_IND(ind)	((torpobject_t *)Obj[(ind)])
#define TORP_PTR(ptr)	((torpobject_t *)(ptr))
};


/*
 * Heat-seeker is a generic missile with extras
 */
typedef struct xp_heatobject heatobject_t;
struct xp_heatobject {

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    float		heat_count;	/* Misc snafus */
    short		heat_lock_id;	/* snafu */

#define HEAT_IND(ind)	((heatobject_t *)Obj[(ind)])
#define HEAT_PTR(ptr)	((heatobject_t *)(ptr))
};


/*
 * The ball object.
 */
typedef struct xp_ballobject ballobject_t;
struct xp_ballobject {

    OBJECT_BASE

    OBJECT_EXTEND

    double		ball_loose_ticks;
    treasure_t		*ball_treasure;	/* treasure for ball */
    short 		ball_owner;	/* Who's object is this ? */
    short		ball_style;	/* What polystyle to use */

#define BALL_IND(ind)	((ballobject_t *)Obj[(ind)])
#define BALL_PTR(obj)	((ballobject_t *)(obj))
};


/*
 * Object with a wireframe representation.
 */
typedef struct xp_wireobject wireobject_t;
struct xp_wireobject {

    OBJECT_BASE

    OBJECT_EXTEND

    float		wire_turnspeed;	/* how fast to turn */

    uint8_t		wire_type;	/* Type of object */
    uint8_t		wire_size;	/* Size of object */
    uint8_t		wire_rotation;	/* Rotation direction */

#define WIRE_IND(ind)	((wireobject_t *)Obj[(ind)])
#define WIRE_PTR(obj)	((wireobject_t *)(obj))
};


/*
 * Pulse object used for laser pulses.
 */
typedef struct xp_pulseobject pulseobject_t;
struct xp_pulseobject {

    OBJECT_BASE

    OBJECT_EXTEND

    float		pulse_len;	/* Length of the pulse */
    uint8_t		pulse_dir;	/* Direction of the pulse */
    bool		pulse_refl;	/* Pulse was reflected ? */

#define PULSE_IND(ind)	((pulseobject_t *)Obj[(ind)])
#define PULSE_PTR(obj)	((pulseobject_t *)(obj))
};


/*
 * Item object.
 */
typedef struct xp_itemobject itemobject_t;
struct xp_itemobject {

    OBJECT_BASE

    OBJECT_EXTEND

    int			item_type;	/* One of ITEM_* */
    int			item_count;	/* Misc snafus */

#define ITEM_IND(ind)	((itemobject_t *)Obj[(ind)])
#define ITEM_PTR(obj)	((itemobject_t *)(obj))
};


/*
 * Any object type should be part of this union.
 */
typedef union xp_anyobject anyobject_t;
union xp_anyobject {
    object_t		obj;
    ballobject_t	ball;
    mineobject_t	mine;
    missileobject_t	missile;
    smartobject_t	smart;
    torpobject_t	torp;
    heatobject_t	heat;
    wireobject_t	wireobj;
    pulseobject_t	pulse;
    itemobject_t	item;
};

#endif
