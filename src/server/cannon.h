/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *      Kimiko Koopman       <kimiko@xpilot.org>
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

#ifndef CANNON_H
#define CANNON_H

#ifndef MAP_H
# include "map.h"
#endif

#ifndef WALLS_H
# include "walls.h"
#endif

extern long CANNON_USE_ITEM;

/* the different weapons a cannon can use.
   used in communication between parts of firing code */
/* plain old bullet. more if wideangles or rearshots are available.
   always available */
#define CW_SHOT		0
/* dropped or thrown. uses one mine */
#define CW_MINE		1
/* torpedo, heatseeker or smartmissile. uses one missile */
#define CW_MISSILE	2
/* blinding, stun or normal laser. needs a laser */
#define CW_LASER	3
/* uses one ECM */
#define CW_ECM		4
/* tractor or pressor beam. needs a tractorbeam */
#define CW_TRACTORBEAM	5
/* uses one transporter */
#define CW_TRANSPORTER	6
/* a big stream of exhaust particles (OBJ_SPARK). needs an afterburner and
   uses one fuel pack. even bigger with emergency thrust. more afterburners
   only increase probability of use */
#define CW_GASJET	7

/* the different defenses a cannon can use.
   used in communication between parts of defending code */
/* for four seconds, absorbs any shot. uses one emergency shield */
#define CD_EM_SHIELD	0
/* for four seconds, lets any shot pass through. uses one phasing device */
#define CD_PHASING	1

/* base visibility distance (modified by sensors) */
#define CANNON_DISTANCE		(VISIBILITY_DISTANCE * 0.5)

/* chance of throwing an item upon death (multiplied by options.dropItemOnKillProb) */
#define CANNON_DROP_ITEM_PROB	0.7

#define CANNON_MINE_MASS	(MINE_MASS * 0.6)
#define CANNON_SHOT_MASS	0.4
/* lifetime in ticks (frames) of shots, missiles and mines */
/* #define CANNON_SHOT_LIFE	(8 + (randomMT() % 24)) */
/* maximum lifetime (only used in aiming) */
/* #define CANNON_SHOT_LIFE_MAX	(8 + 24) */
/* number of laser pulses used in calculation of pulse lifetime */
#define CANNON_PULSES		1

/* sector in which cannonfire is possible */
#define CANNON_SPREAD		(RES / 3)

/* cannon smartness is 0 to this value */
#define CANNON_SMARTNESS_MAX	3

void Cannon_update(bool tick);
void Cannon_init(cannon_t *cannon);
void Cannon_init_items(cannon_t *cannon);
void Cannon_add_item(cannon_t *cannon, int type, int amount);
void Cannon_throw_items(cannon_t *cannon);
void Cannon_check_defense(cannon_t *cannon);
void Cannon_check_fire(cannon_t *cannon);
void Object_hits_cannon(object_t *obj, cannon_t *c);
void Cannon_dies(cannon_t *cannon, player_t *pl);
hitmask_t Cannon_hitmask(cannon_t *cannon);
void Cannon_set_hitmask(int group, cannon_t *cannon);
bool Cannon_hitfunc(group_t *groupptr, const move_t *move);
void World_restore_cannon(cannon_t *cannon);
void World_remove_cannon(cannon_t *cannon);
void Cannon_set_option(cannon_t *cannon, const char *name, const char *value);

static inline int Cannon_get_smartness(cannon_t *c)
{
    if (c->smartness != -1)
	return c->smartness;
    return options.cannonSmartness;
}

static inline double Cannon_get_min_shot_life(cannon_t *c)
{
    return options.minCannonShotLife;
}

static inline double Cannon_get_max_shot_life(cannon_t *c)
{
    return options.maxCannonShotLife;
}

static inline double Cannon_get_shot_life(cannon_t *cannon)
{
    double minlife, maxlife, d;

    minlife = Cannon_get_min_shot_life(cannon);
    maxlife = Cannon_get_max_shot_life(cannon);
    d = maxlife - minlife;

    return minlife + rfrac() * d;
}

static inline double Cannon_get_shot_speed(cannon_t *cannon)
{
    if (cannon->shot_speed > 0)
	return cannon->shot_speed;
    return options.cannonShotSpeed;
}

static inline cannon_t *Cannon_by_id(int id)
{
    int ind;

    if (id < MIN_CANNON_ID || id > MAX_CANNON_ID)
	return NULL;
    ind = id - MIN_CANNON_ID;
    return Cannon_by_index(ind);
}

#endif
