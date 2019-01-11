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

#include "xpserver.h"

static int Cannon_select_weapon(cannon_t *cannon);
static void Cannon_aim(cannon_t *cannon, int weapon,
		       player_t **pl_p, int *dir);
static void Cannon_fire(cannon_t *cannon, int weapon, player_t *pl, int dir);
static int Cannon_in_danger(cannon_t *cannon);
static int Cannon_select_defense(cannon_t *cannon);
static void Cannon_defend(cannon_t *cannon, int defense);

/* the items that are useful to cannons.
   these are the items that cannon get 'for free' once in a while.
   cannons can get other items, but only by picking them up or
   stealing them from players. */
long CANNON_USE_ITEM = (ITEM_BIT_FUEL|ITEM_BIT_WIDEANGLE
		    	|ITEM_BIT_REARSHOT|ITEM_BIT_AFTERBURNER
			|ITEM_BIT_SENSOR|ITEM_BIT_TRANSPORTER
			|ITEM_BIT_TANK|ITEM_BIT_MINE
			|ITEM_BIT_ECM|ITEM_BIT_LASER
			|ITEM_BIT_EMERGENCY_THRUST|ITEM_BIT_ARMOR
			|ITEM_BIT_TRACTOR_BEAM|ITEM_BIT_MISSILE
			|ITEM_BIT_PHASING);

void Cannon_update(bool tick)
{
    int i;

    for (i = 0; i < Num_cannons(); i++) {
	cannon_t *c = Cannon_by_index(i);

	if (c->dead_ticks > 0) {
	    if ((c->dead_ticks -= timeStep) <= 0)
		World_restore_cannon(c);
	    continue;
	}

	/*
	 * Call cannon "AI" routines only once per tick.
	 */
	if (tick) {
	    if (rfrac() < 0.65)
		Cannon_check_defense(c);

	    if (!BIT(c->used, HAS_EMERGENCY_SHIELD)
		&& !BIT(c->used, USES_PHASING_DEVICE)
		&& (c->damaged <= 0)
		&& (c->tractor_count <= 0)
		&& rfrac() * 16 < 1)
		Cannon_check_fire(c);

	    if (options.itemProbMult > 0
		&& options.cannonItemProbMult > 0) {
		int item = (int)(rfrac() * NUM_ITEMS);
		/* this gives the cannon an item about once every minute */
		if (world->items[item].cannonprob > 0
		    && options.cannonItemProbMult > 0
		    && (int)(rfrac() * (60 * 12))
		    < (options.cannonItemProbMult
		       * world->items[item].cannonprob))
		    Cannon_add_item(c, item, (item == ITEM_FUEL
					      ?  ENERGY_PACK_FUEL : 1));
	    }
	}

	if ((c->damaged -= timeStep) <= 0)
	    c->damaged = 0;
	if (c->tractor_count > 0) {
	    player_t *tpl = Player_by_id(c->tractor_target_id);

	    if (tpl == NULL) {
		c->tractor_target_id = NO_ID;
		c->tractor_count = 0;
	    }
	    else if ((Wrap_length(tpl->pos.cx - c->pos.cx,
				  tpl->pos.cy - c->pos.cy)
		 < TRACTOR_MAX_RANGE(c->item[ITEM_TRACTOR_BEAM]) * CLICK)
		&& Player_is_alive(tpl)) {
		General_tractor_beam(c->id, c->pos,
				     c->item[ITEM_TRACTOR_BEAM],
				     tpl, c->tractor_is_pressor);
		if ((c->tractor_count -= timeStep) <= 0)
		    c->tractor_count = 0;
	    } else
		c->tractor_count = 0;
	}
	if (c->emergency_shield_left > 0) {
	    if ((c->emergency_shield_left -= timeStep) <= 0) {
		CLR_BIT(c->used, HAS_EMERGENCY_SHIELD);
		sound_play_sensors(c->pos, EMERGENCY_SHIELD_OFF_SOUND);
	    }
	}
	if (c->phasing_left > 0) {
	    if ((c->phasing_left -= timeStep) <= 0) {
		CLR_BIT(c->used, USES_PHASING_DEVICE);
	        sound_play_sensors(c->pos, PHASING_OFF_SOUND);
	    }
	}
    }
}



/* adds the given amount of an item to the cannon's inventory. the number of
   tanks is taken to be 1. amount is then the amount of fuel in that tank.
   fuel is given in 'units', but is stored in fuelpacks. */
void Cannon_add_item(cannon_t *c, int item_type, int amount)
{
    switch (item_type) {
    case ITEM_TANK:
	c->item[ITEM_TANK] += amount;
	LIMIT(c->item[ITEM_TANK], 0, world->items[ITEM_TANK].limit);
	/* FALLTHROUGH */
    case ITEM_FUEL:
	c->item[ITEM_FUEL] += (int)(amount / ENERGY_PACK_FUEL + 0.5);
	LIMIT(c->item[ITEM_FUEL],
	      0,
	      (int)(world->items[ITEM_FUEL].limit / ENERGY_PACK_FUEL + 0.5));
	break;
    default:
	c->item[item_type] += amount;
	LIMIT(c->item[item_type], 0, world->items[item_type].limit);
	break;
    }
}

static inline int Cannon_get_initial_item(cannon_t *c, Item_t i)
{
    int init_amount;

    init_amount = c->initial_items[i];
    if (init_amount < 0)
	init_amount = world->items[i].cannon_initial;

    return init_amount;
}

void Cannon_throw_items(cannon_t *c)
{
    int i, dir;
    itemobject_t *item;
    double velocity;

    for (i = 0; i < NUM_ITEMS; i++) {
	if (i == ITEM_FUEL)
	    continue;
	c->item[i] -= Cannon_get_initial_item(c, (Item_t)i);
	while (c->item[i] > 0) {
	    int amount = world->items[i].max_per_pack
			 - (int)(rfrac() * (1 + world->items[i].max_per_pack
					    - world->items[i].min_per_pack));
	    LIMIT(amount, 0, c->item[i]);
	    if (rfrac() < (options.dropItemOnKillProb * CANNON_DROP_ITEM_PROB)
		&& (item = ITEM_PTR(Object_allocate())) != NULL) {

		item->type = OBJ_ITEM;
		item->item_type = i;
		item->color = RED;
		item->obj_status = GRAVITY;
		dir = (int)(c->dir
			   - (CANNON_SPREAD * 0.5)
			   + (rfrac() * CANNON_SPREAD));
		dir = MOD2(dir, RES);
		item->id = NO_ID;
		item->team = TEAM_NOT_SET;
		Object_position_init_clpos(OBJ_PTR(item), c->pos);
		velocity = rfrac() * 6;
		item->vel.x = tcos(dir) * velocity;
		item->vel.y = tsin(dir) * velocity;
		item->acc.x = 0;
		item->acc.y = 0;
		item->mass = 10;
		item->life = 1500 + rfrac() * 512;
		item->item_count = amount;
		item->pl_range = ITEM_SIZE / 2;
		item->pl_radius = ITEM_SIZE / 2;
		world->items[i].num++;
		Cell_add_object(OBJ_PTR(item));
	    }
	    c->item[i] -= amount;
	}
    }
}

/* initializes the given cannon at startup or after death and gives it some
   items. */
void Cannon_init(cannon_t *c)
{
    Cannon_init_items(c);
    c->last_change = frame_loops;
    c->damaged = 0;
    c->tractor_target_id = NO_ID;
    c->tractor_count = 0;
    c->tractor_is_pressor = false;
    c->used = 0;
    c->emergency_shield_left = 0;
    c->phasing_left = 0;
}

void Cannon_init_items(cannon_t *c)
{
    int i;

    for (i = 0; i < NUM_ITEMS; i++) {
	c->item[i] = 0;
	Cannon_add_item(c, i, Cannon_get_initial_item(c, (Item_t)i));
    }
}

void Cannon_check_defense(cannon_t *c)
{
    int defense = Cannon_select_defense(c);

    if (defense >= 0 && Cannon_in_danger(c))
	Cannon_defend(c, defense);
}

void Cannon_check_fire(cannon_t *c)
{
    player_t *pl = NULL;
    int	dir = 0,
	weapon = Cannon_select_weapon(c);

    Cannon_aim(c, weapon, &pl, &dir);
    if (pl)
	Cannon_fire(c, weapon, pl, dir);
}

/* selects one of the available defenses. see cannon.h for descriptions. */
static int Cannon_select_defense(cannon_t *c)
{
    int smartness = Cannon_get_smartness(c);

    /* mode 0 does not defend */
    if (smartness == 0)
	return -1;

    /* still protected */
    if (BIT(c->used, HAS_EMERGENCY_SHIELD)
	|| BIT(c->used, USES_PHASING_DEVICE))
	return -1;

    if (c->item[ITEM_EMERGENCY_SHIELD])
	return CD_EM_SHIELD;

    if (c->item[ITEM_PHASING])
	return CD_PHASING;

    /* no defense available */
    return -1;
}

/* checks if a cannon is about to be hit by a hazardous object.
   mode 0 does not detect danger.
   modes 1 - 3 use progressively more accurate detection. */
static int Cannon_in_danger(cannon_t *c)
{
    const int range = 4 * BLOCK_SZ;
    const uint32_t kill_shots = (KILLING_SHOTS) | OBJ_MINE_BIT | OBJ_SHOT_BIT
	| OBJ_PULSE_BIT | OBJ_SMART_SHOT_BIT | OBJ_HEAT_SHOT_BIT
	| OBJ_TORPEDO_BIT | OBJ_ASTEROID_BIT;
    object_t *shot, **obj_list;
    const int max_objs = 100;
    int obj_count, i, danger = false;
    int npx, npy, tdx, tdy;
    int cpx = CLICK_TO_PIXEL(c->pos.cx), cpy = CLICK_TO_PIXEL(c->pos.cy);
    int smartness = Cannon_get_smartness(c);

    if (smartness == 0)
	return false;

    if (NumObjs >= options.cellGetObjectsThreshold)
	Cell_get_objects(c->pos, range, max_objs,
			 &obj_list, &obj_count);
    else {
	obj_list = Obj;
	obj_count = NumObjs;
    }

    for (i = 0; (i < obj_count) && !danger; i++) {
	shot = obj_list[i];

	if (shot->life <= 0)
	    continue;
	if (!BIT(OBJ_TYPEBIT(shot->type), kill_shots))
	    continue;
	if (BIT(shot->obj_status, FROMCANNON))
	    continue;
	if (BIT(world->rules->mode, TEAM_PLAY)
	    && options.teamImmunity
	    && shot->team == c->team)
	    continue;

	npx = CLICK_TO_PIXEL(shot->pos.cx);
	npy = CLICK_TO_PIXEL(shot->pos.cy);
	if (smartness > 1) {
	    npx += (int)shot->vel.x;
	    npy += (int)shot->vel.y;
	    if (smartness > 2) {
		npx += (int)shot->acc.x;
		npy += (int)shot->acc.y;
	    }
	}
	tdx = WRAP_DX(npx - cpx);
	tdy = WRAP_DY(npy - cpy);
	if (LENGTH(tdx, tdy) <= ((4.5 - smartness) * BLOCK_SZ)) {
	    danger = true;
	    break;
	}
    }

    return danger;
}

/* activates the selected defense. */
static void Cannon_defend(cannon_t *c, int defense)
{
    switch (defense) {
    case CD_EM_SHIELD:
	c->emergency_shield_left += 4 * 12;
	SET_BIT(c->used, HAS_EMERGENCY_SHIELD);
	c->item[ITEM_EMERGENCY_SHIELD]--;
	sound_play_sensors(c->pos, EMERGENCY_SHIELD_ON_SOUND);
	break;
    case CD_PHASING:
	c->phasing_left += 4 * 12;
	SET_BIT(c->used, USES_PHASING_DEVICE);
	c->tractor_count = 0;
	c->item[ITEM_PHASING]--;
	sound_play_sensors(c->pos, PHASING_ON_SOUND);
	break;
    default:
	warn("Cannon_defend: Unknown defense.");
	break;
    }
}

/* selects one of the available weapons. see cannon.h for descriptions. */
static int Cannon_select_weapon(cannon_t *c)
{
    if (c->item[ITEM_MINE]
	&& rfrac() < 0.5)
	return CW_MINE;
    if (c->item[ITEM_MISSILE]
	&& rfrac() < 0.5)
	return CW_MISSILE;
    if (c->item[ITEM_LASER]
	&& (int)(rfrac() * (c->item[ITEM_LASER] + 1)))
	return CW_LASER;
    if (c->item[ITEM_ECM]
	&& rfrac() < 0.333)
	return CW_ECM;
    if (c->item[ITEM_TRACTOR_BEAM]
	&& rfrac() < 0.5)
	return CW_TRACTORBEAM;
    if (c->item[ITEM_TRANSPORTER]
	&& rfrac() < 0.333)
	return CW_TRANSPORTER;
    if ((c->item[ITEM_AFTERBURNER]
	 || c->item[ITEM_EMERGENCY_THRUST])
	&& c->item[ITEM_FUEL]
	&& (int)(rfrac() * ((c->item[ITEM_EMERGENCY_THRUST] ?
		      MAX_AFTERBURNER :
		      c->item[ITEM_AFTERBURNER]) + 3)) > 2)
	return CW_GASJET;
    return CW_SHOT;
}

/* determines in which direction to fire.
   mode 0 fires straight ahead.
   mode 1 in a random direction.
   mode 2 aims at the current position of the closest player,
          then limits that to the sector in front of the cannon,
          then adds a small error.
   mode 3 calculates where the player will be when the shot reaches her,
          checks if that position is within limits and selects the player
          who will be closest in this way.
   the targeted player is also returned (for all modes).
   mode 0 always fires if it sees a player.
   modes 1 and 2 only fire if a player is within range of the selected weapon.
   mode 3 only fires if a player will be in range when the shot is expected to hit.
 */
static void Cannon_aim(cannon_t *c, int weapon, player_t **pl_p, int *dir)
{
    double speed = Cannon_get_shot_speed(c);
    double range = Cannon_get_max_shot_life(c) * speed;
    double visualrange = (CANNON_DISTANCE
			      + 2 * c->item[ITEM_SENSOR] * BLOCK_SZ);
    bool found = false, ready = false;
    double closest = range;
    int ddir, i, smartness = Cannon_get_smartness(c);

    switch (weapon) {
    case CW_MINE:
	speed = speed * 0.5 + 0.1 * smartness;
	range = range * 0.5 + 0.1 * smartness;
	break;
    case CW_LASER:
	speed = options.pulseSpeed;
	range = CANNON_PULSE_LIFE * speed;
	break;
    case CW_ECM:
	/* smarter cannons wait a little longer before firing an ECM */
	if (smartness > 1)
	    range = ((ECM_DISTANCE / smartness
		      + (rfrac() * (int)(ECM_DISTANCE
					 - ECM_DISTANCE / smartness))));
	else
	    range = ECM_DISTANCE;
	break;
    case CW_TRACTORBEAM:
	range = TRACTOR_MAX_RANGE(c->item[ITEM_TRACTOR_BEAM]);
	break;
    case CW_TRANSPORTER:
	/* smarter cannons have a smaller chance of using a transporter when
	   target is out of range */
	if (smartness > 2
	    || (int)(rfrac() * sqr(smartness + 1)))
	    range = TRANSPORTER_DISTANCE;
	break;
    case CW_GASJET:
	if (c->item[ITEM_EMERGENCY_THRUST]) {
	    speed *= 2.0;
	    range *= 2.0;
	}
	break;
    default:
	/* no need to do anything specail for this weapon. */
	break;
    }

    for (i = 0; i < NumPlayers && !ready; i++) {
	player_t *pl = Player_by_index(i);
	double tdist, tdx, tdy;

        /* KHS: cannon dodgers mode:               */
	/* Cannons fire on players in any range    */
	tdx = WRAP_DCX(pl->pos.cx - c->pos.cx) / CLICK;
	if (ABS(tdx) >= visualrange
	    && options.survivalScore == 0.0)
 	    continue;
	tdy = WRAP_DCY(pl->pos.cy - c->pos.cy) / CLICK;
	if (ABS(tdy) >= visualrange
	    && options.survivalScore == 0.0)
	    continue;
	tdist = LENGTH(tdx, tdy);
	if (tdist > visualrange
	    && options.survivalScore == 0.0)
	    continue;

	/* mode 3 also checks if a player is using a phasing device */
	if (Player_is_paused(pl)
	    || (BIT(world->rules->mode, TEAM_PLAY)
		&& pl->team == c->team)
	    || ((pl->forceVisible <= 0)
		&& Player_is_cloaked(pl)
		&& (int)(rfrac() * (pl->item[ITEM_CLOAK] + 1))
		   > (int)(rfrac() * (c->item[ITEM_SENSOR] + 1)))
	    || (smartness > 2
		&& Player_is_phasing(pl)))
	    continue;

	switch (smartness) {
	case 0:
	    ready = true;
	    break;
	default:
	case 1:
	    
	    /* KHS disable this range check, too */
            /* in cannon dodgers */
	    if (tdist < range 
		|| options.survivalScore != 0.0)
		ready = true;
	    break;
	case 2:
	    if (tdist < closest) {
		double a = findDir(tdx, tdy);
		*dir = (int) a;
		found = true;
	    }
	    break;
	case 3:
	    if (tdist < range) {
		double a, t = tdist / speed; /* time */
		int npx = (int)(pl->pos.cx
				+ pl->vel.x * t * CLICK
				+ pl->acc.x * t * t * CLICK);
		int npy = (int)(pl->pos.cy
				+ pl->vel.y * t * CLICK
				+ pl->acc.y * t * t * CLICK);
		int tdir;

		tdx = WRAP_DCX(npx - c->pos.cx) / CLICK;
		tdy = WRAP_DCY(npy - c->pos.cy) / CLICK;
		a = findDir(tdx, tdy);
		tdir = (int) a;
		ddir = MOD2(tdir - c->dir, RES);
		if ((ddir < (CANNON_SPREAD * 0.5)
		     || ddir > RES - (CANNON_SPREAD * 0.5))
		    && LENGTH(tdx, tdy) < closest) {
		    *dir = tdir;
		    found = true;
		}
	    }
	    break;
	}
	if (found || ready) {
	    closest = tdist;
	    *pl_p = pl;
	}
    }
    if (!(found || ready)) {
	*pl_p = NULL;
	return;
    }

    switch (smartness) {
    case 0:
	*dir = c->dir;
	break;
    default:
    case 1:
	*dir = c->dir;
	*dir += (int)((rfrac() - 0.5f) * CANNON_SPREAD);
	break;
    case 2:
	ddir = MOD2(*dir - c->dir, RES);
	if (ddir > (CANNON_SPREAD * 0.5) && ddir < RES / 2)
	    *dir = (int)(c->dir + (CANNON_SPREAD * 0.5) + 3);
	else if (ddir < RES - (CANNON_SPREAD * 0.5) && ddir > RES / 2)
	    *dir = (int)(c->dir - (CANNON_SPREAD * 0.5) - 3);
	*dir += (int)(rfrac() * 7) - 3;
	break;
    case 3:
	/* nothing to be done for mode 3 */
	break;
    }
    *dir = MOD2(*dir, RES);
}

/* does the actual firing. also determines in which way to use weapons that
   have more than one possible use. */
static void Cannon_fire(cannon_t *c, int weapon, player_t *pl, int dir)
{
    modifiers_t	mods;
    bool played = false;
    int i, smartness = Cannon_get_smartness(c);
    double speed = Cannon_get_shot_speed(c);
    vector_t zero_vel = { 0.0, 0.0 };

    Mods_clear(&mods);
    switch (weapon) {
    case CW_MINE:
	if (rfrac() < 0.25)
	    Mods_set(&mods, ModsCluster, 1);

	if (rfrac() >= 0.2)
	    Mods_set(&mods, ModsImplosion, 1);

	Mods_set(&mods, ModsPower,
		 (int)(rfrac() * (MODS_POWER_MAX + 1)));
	Mods_set(&mods, ModsVelocity, 
		 (int)(rfrac() * (MODS_VELOCITY_MAX + 1)));

	if (rfrac() < 0.5) {	/* place mine in front of cannon */
	    Place_general_mine(c->id, c->team, FROMCANNON,
			       c->pos, zero_vel, mods);
	    sound_play_sensors(c->pos, DROP_MINE_SOUND);
	    played = true;
	} else {		/* throw mine at player */
	    vector_t vel;

	    Mods_set(&mods, ModsMini,
		     (int)(rfrac() * MODS_MINI_MAX) + 1);
	    Mods_set(&mods, ModsSpread,
		     (int)(rfrac() * (MODS_SPREAD_MAX + 1)));

	    speed = speed * 0.5 + 0.1 * smartness;
	    vel.x = tcos(dir) * speed;
	    vel.y = tsin(dir) * speed;
	    Place_general_mine(c->id, c->team, GRAVITY|FROMCANNON,
			       c->pos, vel, mods);
	    sound_play_sensors(c->pos, DROP_MOVING_MINE_SOUND);
	    played = true;
	}
	c->item[ITEM_MINE]--;
	break;
    case CW_MISSILE:
	if (rfrac() < 0.333)
	    Mods_set(&mods, ModsCluster, 1);

	if (rfrac() >= 0.25)
	    Mods_set(&mods, ModsImplosion, 1);

	Mods_set(&mods, ModsPower,
		 (int)(rfrac() * (MODS_POWER_MAX + 1)));
	Mods_set(&mods, ModsVelocity, 
		 (int)(rfrac() * (MODS_VELOCITY_MAX + 1)));

	/* Because cannons don't have missile racks, all mini missiles
	   would be fired from the same point and appear to the players
	   as 1 missile (except heatseekers, which would appear to split
	   in midair because of navigation errors (see Move_smart_shot)).
	   Therefore, we don't minify cannon missiles.
	   mods.mini = (int)(rfrac() * MODS_MINI_MAX) + 1;
	   mods.spread = (int)(rfrac() * (MODS_SPREAD_MAX + 1));
	*/

	/* smarter cannons use more advanced missile types */
	switch ((int)(rfrac() * (1 + smartness))) {
	default:
	    if (options.allowSmartMissiles) {
		Fire_general_shot(c->id, c->team, c->pos,
				  OBJ_SMART_SHOT, dir, mods, pl->id);
		sound_play_sensors(c->pos, FIRE_SMART_SHOT_SOUND);
		played = true;
		break;
	    }
	    /* FALLTHROUGH */
	case 1:
	    if (options.allowHeatSeekers
		&& Player_is_thrusting(pl)) {
		Fire_general_shot(c->id, c->team, c->pos,
				  OBJ_HEAT_SHOT, dir, mods, pl->id);
		sound_play_sensors(c->pos, FIRE_HEAT_SHOT_SOUND);
		played = true;
		break;
	    }
	    /* FALLTHROUGH */
	case 0:
	    Fire_general_shot(c->id, c->team, c->pos,
			      OBJ_TORPEDO, dir, mods, NO_ID);
	    sound_play_sensors(c->pos, FIRE_TORPEDO_SOUND);
	    played = true;
	    break;
	}
	c->item[ITEM_MISSILE]--;
	break;
    case CW_LASER:
	/* stun and blinding lasers are very dangerous,
	   so we don't use them often */
	if ((rfrac() * (8 - smartness)) >= 1)
	    Mods_set(&mods, ModsLaser,
		     (int)(rfrac() * (MODS_LASER_MAX + 1)));

	Fire_general_laser(c->id, c->team, c->pos, dir, mods);
	sound_play_sensors(c->pos, FIRE_LASER_SOUND);
	played = true;
	break;
    case CW_ECM:
	Fire_general_ecm(c->id, c->team, c->pos);
	c->item[ITEM_ECM]--;
	sound_play_sensors(c->pos, ECM_SOUND);
	played = true;
	break;
    case CW_TRACTORBEAM:
	/* smarter cannons use tractors more often and also push/pull longer */
	c->tractor_is_pressor = (rfrac() * (smartness + 1) >= 1);
	c->tractor_target_id = pl->id;
	c->tractor_count = 11 + rfrac() * (3 * smartness + 1);
	break;
    case CW_TRANSPORTER:
	c->item[ITEM_TRANSPORTER]--;
	if (Wrap_length(pl->pos.cx - c->pos.cx, pl->pos.cy - c->pos.cy)
	    < TRANSPORTER_DISTANCE * CLICK) {
	    int item = -1;
	    double amount = 0.0;

	    Do_general_transporter(c->id, c->pos, pl, &item, &amount);
	    if (item != -1)
		Cannon_add_item(c, item, amount);
	} else {
	    sound_play_sensors(c->pos, TRANSPORTER_FAIL_SOUND);
	    played = true;
	}
	break;
    case CW_GASJET:
	/* use emergency thrusts to make extra big jets */
	if ((rfrac() * (c->item[ITEM_EMERGENCY_THRUST] + 1)) >= 1) {
	    Make_debris(c->pos,
			zero_vel,
			NO_ID,
			c->team,
			OBJ_SPARK,
			THRUST_MASS,
			GRAVITY|FROMCANNON,
			RED,
			8,
			(int)(300 + 400 * rfrac()),
			dir - 4 * (4 - smartness),
			dir + 4 * (4 - smartness),
			0.1, speed * 4,
			3.0, 20.0);
	    c->item[ITEM_EMERGENCY_THRUST]--;
	} else {
	    Make_debris(c->pos,
			zero_vel,
			NO_ID,
			c->team,
			OBJ_SPARK,
			THRUST_MASS,
			GRAVITY|FROMCANNON,
			RED,
			8,
			(int)(150 + 200 * rfrac()),
			dir - 3 * (4 - smartness),
			dir + 3 * (4 - smartness),
			0.1, speed * 2,
			3.0, 20.0);
	}
	c->item[ITEM_FUEL]--;
	sound_play_sensors(c->pos, THRUST_SOUND);
	played = true;
	break;
    case CW_SHOT:
    default:
	if (options.cannonFlak)
	    Mods_set(&mods, ModsCluster, 1);
	/* smarter cannons fire more accurately and
	   can therefore narrow their bullet streams */
	for (i = 0; i < (1 + 2 * c->item[ITEM_WIDEANGLE]); i++) {
	    int a_dir = dir
			+ (4 - smartness)
			* (-c->item[ITEM_WIDEANGLE] +  i);
	    a_dir = MOD2(a_dir, RES);
	    Fire_general_shot(c->id, c->team, c->pos,
			      OBJ_CANNON_SHOT, a_dir, mods, NO_ID);
	}
	/* I'm not sure cannons should use rearshots.
	   After all, they are restricted to 60 degrees when picking their
	   target. */
	for (i = 0; i < c->item[ITEM_REARSHOT]; i++) {
	    int a_dir = (int)(dir + (RES / 2)
			+ (4 - smartness)
			* (-((c->item[ITEM_REARSHOT] - 1) * 0.5) + i));
	    a_dir = MOD2(a_dir, RES);
	    Fire_general_shot(c->id, c->team, c->pos,
			      OBJ_CANNON_SHOT, a_dir, mods, NO_ID);
	}
    }

    /* finally, play sound effect */
    if (!played) {
	sound_play_sensors(c->pos, CANNON_FIRE_SOUND);
    }
}

void Object_hits_cannon(object_t *obj, cannon_t *c)
{
    if (obj->type == OBJ_ITEM) {
	itemobject_t *item = ITEM_PTR(obj);

	Cannon_add_item(c, item->item_type, item->item_count);
    }
    else {
	player_t *pl = Player_by_id(obj->id);

	if (!BIT(c->used, HAS_EMERGENCY_SHIELD)) {
	    if (c->item[ITEM_ARMOR] > 0)
		c->item[ITEM_ARMOR]--;
	    else
		Cannon_dies(c, pl);
	}
    }
}

void Cannon_dies(cannon_t *c, player_t *pl)
{
    vector_t zero_vel = { 0.0, 0.0 };

    World_remove_cannon(c);
    Cannon_throw_items(c);
    Cannon_init(c);
    sound_play_sensors(c->pos, CANNON_EXPLOSION_SOUND);
    Make_debris(c->pos,
		zero_vel,
		NO_ID,
		c->team,
		OBJ_DEBRIS,
		4.5,
		GRAVITY,
		RED,
		6,
		(int)(20 + 20 * rfrac()),
		(int)(c->dir - (RES * 0.2)), (int)(c->dir + (RES * 0.2)),
		20.0, 50.0,
		8.0, 68.0);
    Make_wreckage(c->pos,
		  zero_vel,
		  NO_ID,
		  c->team,
		  3.5, 23.0,
		  28.0,
		  GRAVITY,
		  10,
		  (int)(c->dir - (RES * 0.2)), (int)(c->dir + (RES * 0.2)),
		  10.0, 25.0,
		  8.0, 68.0);

    if (pl) {
    	Handle_Scoring(SCORE_CANNON_KILL,pl,NULL,c,NULL);
    }
}


hitmask_t Cannon_hitmask(cannon_t *cannon)
{
    if (cannon->dead_ticks > 0)
	return ALL_BITS;
    if (BIT(world->rules->mode, TEAM_PLAY) && options.teamImmunity)
	return HITMASK(cannon->team);
    return 0;
}

void Cannon_set_hitmask(int group, cannon_t *cannon)
{
    assert(group == cannon->group);

    P_set_hitmask(cannon->group, Cannon_hitmask(cannon));
}


void World_restore_cannon(cannon_t *cannon)
{
    blkpos_t blk = Clpos_to_blkpos(cannon->pos);
    int i;

    World_set_block(blk, CANNON);

    for (i = 0; i < num_polys; i++) {
	poly_t *poly = &pdata[i];

	if (poly->group == cannon->group) {
	    poly->current_style = poly->style;
	    poly->update_mask = ~0;
	    poly->last_change = frame_loops;
	}
    }

    cannon->conn_mask = 0;
    cannon->last_change = frame_loops;
    cannon->dead_ticks = 0;

    P_set_hitmask(cannon->group, Cannon_hitmask(cannon));
}

void World_remove_cannon(cannon_t *cannon)
{
    blkpos_t blk = Clpos_to_blkpos(cannon->pos);
    int i;

    cannon->dead_ticks = options.cannonDeadTicks;
    cannon->conn_mask = 0;

    World_set_block(blk, SPACE);

    for (i = 0; i < num_polys; i++) {
	poly_t *poly = &pdata[i];

	if (poly->group == cannon->group) {
	    poly->current_style = poly->destroyed_style;
	    poly->update_mask = ~0;
	    poly->last_change = frame_loops;
	}
    }

    P_set_hitmask(cannon->group, Cannon_hitmask(cannon));
}


extern struct move_parameters mp;
/*
 * This function is called when something would hit a cannon.
 *
 * Ideas stolen from Move_segment in walls_old.c
 */
bool Cannon_hitfunc(group_t *gp, const move_t *move)
{
    const object_t *obj = move->obj;
    cannon_t *cannon = Cannon_by_index(gp->mapobj_ind);
    unsigned long cannon_mask;

    /* this should never happen if hitmasks are ok */
    assert (! (cannon->dead_ticks > 0));

    /* if cannon is phased nothing will hit it */
    if (BIT(cannon->used, USES_PHASING_DEVICE))
	return false;

    if (obj == NULL)
	return true;

    cannon_mask = mp.obj_cannon_mask | OBJ_PLAYER_BIT;
    if (!BIT(cannon_mask, OBJ_TYPEBIT(obj->type)))
	return false;

    /*
     * kps - if no team play, both cannons have team == TEAM_NOT_SET,
     * this code should work, no matter if team play is true or not.
     */
     if (BIT(obj->obj_status, FROMCANNON)
         && obj->team == cannon->team) {
         return false;
     }

     return true;
}

void Cannon_set_option(cannon_t *cannon, const char *name, const char *value)
{
    Item_t item;
    const char *origname = name;

    /* Remove possible cannon prefix from option name. */
    if (!strncasecmp(name, "cannon", 6))
	name += 6;

    item = Item_by_option_name(name);
    if (item != NO_ITEM) {
	cannon->initial_items[item] = atoi(value);
	return;
    }

    if (!strcasecmp(name, "smartness")) {
	int smartness = atoi(value);

	LIMIT(smartness, 0, CANNON_SMARTNESS_MAX);
	cannon->smartness = smartness;
	return;
    }

    if (!strcasecmp(name, "shotspeed")) {
	float shot_speed = atof(value);

	/* limit ? */
	cannon->shot_speed = shot_speed;
	return;
    }

    warn("This server doesn't support option %s for cannons.", origname);
}
