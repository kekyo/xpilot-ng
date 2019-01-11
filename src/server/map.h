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

#ifndef	MAP_H
#define	MAP_H

#ifndef TYPES_H
# include "types.h"
#endif
#ifndef RULES_H
# include "rules.h"
#endif
#ifndef ITEM_H
# include "item.h"
#endif

#define SPACE			0
#define BASE			1
#define FILLED			2
#define REC_LU			3
#define REC_LD			4
#define REC_RU			5
#define REC_RD			6
#define FUEL			7
#define CANNON			8
#define CHECK			9
#define POS_GRAV		10
#define NEG_GRAV		11
#define CWISE_GRAV		12
#define ACWISE_GRAV		13
#define WORMHOLE		14
#define TREASURE		15
#define TARGET			16
#define ITEM_CONCENTRATOR	17
#define DECOR_FILLED		18
#define DECOR_LU		19
#define DECOR_LD		20
#define DECOR_RU		21
#define DECOR_RD		22
#define UP_GRAV			23
#define DOWN_GRAV		24
#define RIGHT_GRAV		25
#define LEFT_GRAV		26
#define FRICTION		27
#define ASTEROID_CONCENTRATOR	28
#define BASE_ATTRACTOR		127

#define DIR_RIGHT		0
#define DIR_UP			(RES/4)
#define DIR_LEFT		(RES/2)
#define DIR_DOWN		(3*RES/4)

typedef struct world world_t;
extern world_t		World, *world;

typedef struct fuel {
    clpos_t	pos;
    double	fuel;
    uint32_t	conn_mask;
    long	last_change;
    int		team;
} fuel_t;

typedef struct grav {
    clpos_t	pos;
    double	force;
    int		type;
} grav_t;

typedef struct base {
    clpos_t	pos;
    int		dir;
    int		ind;
    int		team;
    int		order;
    int		initial_items[NUM_ITEMS];
} base_t;

typedef struct cannon {
    clpos_t	pos;
    int		dir;
    uint32_t	conn_mask;
    long	last_change;
    int		item[NUM_ITEMS];
    int		tractor_target_id;
    bool	tractor_is_pressor;
    int		team;
    long	used;
    double	dead_ticks;
    double	damaged;
    double	tractor_count;
    double	emergency_shield_left;
    double	phasing_left;
    int		group;
    double	score;
    short	id;
    short	smartness;
    float	shot_speed;
    int		initial_items[NUM_ITEMS];
} cannon_t;

typedef struct check {
    clpos_t	pos;
} check_t;

typedef struct item {
    double	prob;		/* Probability [0..1] for item to appear */
    int		max;		/* Max on world at a given time */
    int		num;		/* Number active right now */
    int		chance;		/* Chance [0..127] for this item to appear */
    double	cannonprob;	/* Relative probability for item to appear */
    int		min_per_pack;	/* minimum number of elements per item. */
    int		max_per_pack;	/* maximum number of elements per item. */
    int		initial;	/* initial number of elements per player. */
    int		cannon_initial;	/* initial number of elements per cannon. */
    int		limit;		/* max number of elements per player/cannon. */
} item_t;

typedef struct asteroid {
    double	prob;		/* Probability [0..1] for asteroid to appear */
    int		max;		/* Max on world at a given time */
    int		num;		/* Number active right now */
    int		chance;		/* Chance [0..127] for asteroid to appear */
} asteroid_t;

typedef enum {
    WORM_NORMAL,
    WORM_IN,
    WORM_OUT,
    WORM_FIXED
} wormtype_t;

typedef struct wormhole {
    clpos_t	pos;
    int		lastdest;	/* last destination wormhole */
    double	countdown;	/* >0 warp to lastdest else random */
    wormtype_t	type;
    int		lastID;
    int		lastblock;	/* block it occluded */
    int		group;
} wormhole_t;

typedef struct treasure {
    clpos_t	pos;
    bool	have;		/* true if this treasure has ball in it */
    int		team;		/* team of this treasure */
    int 	destroyed;	/* how often this treasure destroyed */
    bool	empty;		/* true if this treasure never had a ball */
    int		ball_style;	/* polystyle to use for color */
} treasure_t;

typedef struct target {
    clpos_t	pos;
    int		team;
    double	dead_ticks;
    double	damage;
    uint32_t	conn_mask;
    uint32_t 	update_mask;
    long	last_change;
    int		group;
} target_t;

typedef struct team {
    int		NumMembers;		/* Number of current members */
    int		NumRobots;		/* Number of robot players */
    int		NumBases;		/* Number of bases owned */
    int		NumTreasures;		/* Number of treasures owned */
    int		NumEmptyTreasures;	/* Number of empty treasures owned */
    int		TreasuresDestroyed;	/* Number of destroyed treasures */
    int		TreasuresLeft;		/* Number of treasures left */
    int		SwapperId;		/* Player swapping to this full team */
} team_t;

typedef struct item_concentrator {
    clpos_t	pos;
} item_concentrator_t;

typedef struct asteroid_concentrator {
    clpos_t	pos;
} asteroid_concentrator_t;

typedef struct friction_area {
    clpos_t	pos;
    double	friction_setting;	/* Setting from map */
    double	friction;		/* Changes with gameSpeed */
    int		group;
} friction_area_t;

#define MAX_PLAYER_ECMS		8	/* Maximum simultaneous per player */
typedef struct {
    double	size;
    clpos_t	pos;
    int		id;
} ecm_t;

/*
 * Transporter info.
 */
typedef struct {
    clpos_t	pos;
    int		victim_id;
    int		id;
    double	count;
} transporter_t;


extern bool is_polygon_map;

struct world {
    int		x, y;		/* Size of world in blocks, rounded up */
    int		bwidth_floor;	/* Width of world in blocks, rounded down */
    int		bheight_floor;	/* Height of world in blocks, rounded down */
    double	diagonal;	/* Diagonal length in blocks */
    int		width, height;	/* Size of world in pixels (optimization) */
    int		cwidth, cheight;/* Size of world in clicks */
    double	hypotenuse;	/* Diagonal length in pixels (optimization) */
    rules_t	*rules;
    char	name[MAX_CHARS];
    char	author[MAX_CHARS];
    char	dataURL[MAX_CHARS];

    u_byte	**block;	/* type of item in each block */
    vector_t	**gravity;
    item_t	items[NUM_ITEMS];
    asteroid_t	asteroids;
    team_t	teams[MAX_TEAMS];

    int		NumTeamBases;	/* How many 'different' teams are allowed */

    arraylist_t	*asteroidConcs;
    arraylist_t	*bases;
    arraylist_t	*cannons;
    arraylist_t	*ecms;
    arraylist_t	*fuels;
    arraylist_t	*frictionAreas;
    arraylist_t	*gravs;
    arraylist_t	*itemConcs;
    arraylist_t	*targets;
    arraylist_t	*transporters;
    arraylist_t	*treasures;
    arraylist_t	*wormholes;

    int		NumChecks, MaxChecks;
    check_t	*checks;

    bool	have_options;
};

static inline void World_set_block(blkpos_t blk, int type)
{
    assert (! (blk.bx < 0 || blk.bx >= world->x
	       || blk.by < 0 || blk.by >= world->y));
    world->block[blk.bx][blk.by] = type;
}

static inline int World_get_block(blkpos_t blk)
{
    assert (! (blk.bx < 0 || blk.bx >= world->x
	       || blk.by < 0 || blk.by >= world->y));
    return world->block[blk.bx][blk.by];
}

static inline bool World_contains_clpos(clpos_t pos)
{
    if (pos.cx < 0 || pos.cx >= world->cwidth)
	return false;
    if (pos.cy < 0 || pos.cy >= world->cheight)
	return false;
    return true;
}

static inline clpos_t World_get_random_clpos(void)
{
    clpos_t pos;

    pos.cx = (int)(rfrac() * world->cwidth);
    pos.cy = (int)(rfrac() * world->cheight);

    return pos;
}

static inline int World_wrap_xclick(int cx)
{
    while (cx < 0)
	cx += world->cwidth;
    while (cx >= world->cwidth)
	cx -= world->cwidth;

    return cx;
}

static inline int World_wrap_yclick(int cy)
{
    while (cy < 0)
	cy += world->cheight;
    while (cy >= world->cheight)
	cy -= world->cheight;

    return cy;
}

static inline clpos_t World_wrap_clpos(clpos_t pos)
{
    pos.cx = World_wrap_xclick(pos.cx);
    pos.cy = World_wrap_yclick(pos.cy);

    return pos;
}


/*
 * Two inline function for edge wrap of x and y coordinates measured
 * in clicks.
 *
 * Note that even when wrap play is off, ships will wrap around the map
 * if there is not walls that hinder it.
 */
static inline int WRAP_XCLICK(int cx)
{
    return World_wrap_xclick(cx);
}

static inline int WRAP_YCLICK(int cy)
{
    return World_wrap_yclick(cy);
}


/*
 * Two macros for edge wrap of differences in position.
 * If the absolute value of a difference is bigger than
 * half the map size then it is wrapped.
 */
#define WRAP_DCX(dcx)	\
	(BIT(world->rules->mode, WRAP_PLAY) \
	    ? ((dcx) < - (world->cwidth >> 1) \
		? (dcx) + world->cwidth \
		: ((dcx) > (world->cwidth >> 1) \
		    ? (dcx) - world->cwidth \
		    : (dcx))) \
	    : (dcx))

#define WRAP_DCY(dcy)	\
	(BIT(world->rules->mode, WRAP_PLAY) \
	    ? ((dcy) < - (world->cheight >> 1) \
		? (dcy) + world->cheight \
		: ((dcy) > (world->cheight >> 1) \
		    ? (dcy) - world->cheight \
		    : (dcy))) \
	    : (dcy))

#define TWRAP_XCLICK(x_) \
     ((x_) > 0 ? (x_) % world->cwidth : \
      ((x_) % world->cwidth + world->cwidth))

#define TWRAP_YCLICK(y_) \
     ((y_) > 0 ? (y_) % world->cheight : \
      ((y_) % world->cheight + world->cheight))


#define CENTER_XCLICK(X) \
        (((X) < -(world->cwidth >> 1)) ? \
             (X) + world->cwidth : \
             (((X) >= (world->cwidth >> 1)) ? \
                 (X) - world->cwidth : \
                 (X)))

#define CENTER_YCLICK(X) \
        (((X) < -(world->cheight >> 1)) ? \
	     (X) + world->cheight : \
	     (((X) >= (world->cheight >> 1)) ? \
	         (X) - world->cheight : \
	         (X)))



#define Num_asteroidConcs()	Arraylist_get_num_elements(world->asteroidConcs)
#define Num_bases()		Arraylist_get_num_elements(world->bases)
#define Num_cannons()		Arraylist_get_num_elements(world->cannons)
#define Num_ecms()		Arraylist_get_num_elements(world->ecms)
#define Num_frictionAreas()	Arraylist_get_num_elements(world->frictionAreas)
#define Num_fuels()		Arraylist_get_num_elements(world->fuels)
#define Num_gravs()		Arraylist_get_num_elements(world->gravs)
#define Num_itemConcs()	Arraylist_get_num_elements(world->itemConcs)
#define Num_targets()		Arraylist_get_num_elements(world->targets)
#define Num_transporters()	Arraylist_get_num_elements(world->transporters)
#define Num_treasures()	Arraylist_get_num_elements(world->treasures)
#define Num_wormholes()	Arraylist_get_num_elements(world->wormholes)

#define AsteroidConc_by_index(i) \
	((asteroid_concentrator_t *)Arraylist_get(world->asteroidConcs, (i)))
#define Base_by_index(i)	((base_t *)Arraylist_get(world->bases, (i)))
#define Cannon_by_index(i)	((cannon_t *)Arraylist_get(world->cannons, (i)))
#define Ecm_by_index(i)	((ecm_t *)Arraylist_get(world->ecms, (i)))
#define FrictionArea_by_index(i) \
	((friction_area_t *)Arraylist_get(world->frictionAreas, (i)))
#define Fuel_by_index(i)	((fuel_t *)Arraylist_get(world->fuels, (i)))
#define Grav_by_index(i)	((grav_t *)Arraylist_get(world->gravs, (i)))
#define ItemConc_by_index(i) \
	((item_concentrator_t *)Arraylist_get(world->itemConcs, (i)))
#define Target_by_index(i)	((target_t *)Arraylist_get(world->targets, (i)))
#define Treasure_by_index(i)	((treasure_t *)Arraylist_get(world->treasures, (i)))
#define Wormhole_by_index(i)	((wormhole_t *)Arraylist_get(world->wormholes, (i)))
#define Transporter_by_index(i) \
	((transporter_t *)Arraylist_get(world->transporters, (i)))


static inline check_t *Check_by_index(int ind)
{
    if (ind >= 0 && ind < world->NumChecks)
	return &world->checks[ind];
    return NULL;
}

/*
 * Here the index is the team number.
 */
static inline team_t *Team_by_index(int ind)
{
    if (ind >= 0 && ind < MAX_TEAMS)
	return &world->teams[ind];
    return NULL;
}

#endif
