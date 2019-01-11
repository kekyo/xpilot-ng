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

#define MAX_FUEL                10000
#define MAX_WIDEANGLE           99
#define MAX_REARSHOT            99
#define MAX_CLOAK               99
#define MAX_SENSOR              99
#define MAX_TRANSPORTER         99
#define MAX_MINE                99
#define MAX_MISSILE             99
#define MAX_ECM                 99
#define MAX_ARMOR		99
#define MAX_EMERGENCY_THRUST    99
#define MAX_AUTOPILOT           99
#define MAX_EMERGENCY_SHIELD    99
#define MAX_DEFLECTOR           99
#define MAX_MIRROR		99
#define MAX_PHASING             99
#define MAX_HYPERJUMP           99
#define MAX_LASER		99
#define MAX_TRACTOR_BEAM	99

uint32_t	KILLING_SHOTS = (OBJ_SHOT_BIT|OBJ_CANNON_SHOT_BIT
				 |OBJ_SMART_SHOT_BIT|OBJ_TORPEDO_BIT
				 |OBJ_HEAT_SHOT_BIT|OBJ_PULSE_BIT);
uint16_t	KILL_OBJ_BITS = (THRUSTING|WARPING|WARPED);
uint32_t	DEF_HAVE = (HAS_SHIELD|HAS_COMPASS|HAS_REFUEL|HAS_REPAIR
			    |HAS_CONNECTOR|HAS_SHOT|HAS_LASER);
uint32_t	DEF_USED = (HAS_SHIELD|HAS_COMPASS);
uint32_t	USED_KILL =
	(HAS_REFUEL|HAS_REPAIR|HAS_CONNECTOR|HAS_SHOT|HAS_LASER|HAS_ARMOR
	|HAS_TRACTOR_BEAM|HAS_CLOAKING_DEVICE|HAS_PHASING_DEVICE
	|HAS_DEFLECTOR|HAS_MIRROR|HAS_EMERGENCY_SHIELD|HAS_EMERGENCY_THRUST);



/*
 * Convert between probability for something to happen a given second on a
 * given block, to chance for such an event to happen on any block this tick.
 */
static void Set_item_chance(int item)
{
    double max
	= options.itemProbMult * options.maxItemDensity * world->x * world->y;
    double sum = 0;
    int i, num = 0;

    if (options.itemProbMult * world->items[item].prob > 0) {
	world->items[item].chance = (int)(1.0
	    / (options.itemProbMult * world->items[item].prob
	       * world->x * world->y * FPS));
	world->items[item].chance = MAX(world->items[item].chance, 1);
    } else
	world->items[item].chance = 0;

    if (max > 0) {
	if (max < 1)
	    world->items[item].max = 1;
	else
	    world->items[item].max = (int)max;
    } else
	world->items[item].max = 0;

    if (!BIT(CANNON_USE_ITEM, 1U << item)) {
	world->items[item].cannonprob = 0;
	return;
    }
    for (i = 0; i < NUM_ITEMS; i++) {
	if (world->items[i].prob > 0
	    && BIT(CANNON_USE_ITEM, 1U << i)) {
	    sum += world->items[i].prob;
	    num++;
	}
    }
    if (num)
	world->items[item].cannonprob = world->items[item].prob
				       * (num / sum)
				       * (options.maxItemDensity / 0.00012);
    else
	world->items[item].cannonprob = 0;
}


/*
 * An item probability has been changed during game play.
 * Update the world->items structure and test if there are more items
 * in the world than wanted for the new item probabilities.
 * This function is also called when option itemProbMult or
 * option maxItemDensity changes.
 */
void Tune_item_probs(void)
{
    int i, j, excess;

    for (i = 0; i < NUM_ITEMS; i++) {
	Set_item_chance(i);
	excess = world->items[i].num - world->items[i].max;
	if (excess > 0) {
	    for (j = 0; j < NumObjs; j++) {
		object_t *obj = Obj[j];

		if (obj->type == OBJ_ITEM) {
		    itemobject_t *item = ITEM_PTR(obj);

		    if (item->item_type == i) {
			Delete_shot(j);
			j--;
			if (--excess == 0)
			    break;
		    }
		}
	    }
	}
    }
}

void Tune_asteroid_prob(void)
{
    double max = options.maxAsteroidDensity * world->x * world->y;

    if (world->asteroids.prob > 0) {
	world->asteroids.chance = (int)(1.0
			/ (world->asteroids.prob * world->x * world->y * FPS));
	world->asteroids.chance = MAX(world->asteroids.chance, 1);
    } else
	world->asteroids.chance = 0;

    if (max > 0) {
	if (max < 1)
	    world->asteroids.max = 1;
	else
	    world->asteroids.max = (int)max;
    } else
	world->asteroids.max = 0;

    /* superfluous asteroids are handled by Asteroid_update() */

    /* Tune asteroid concentrator parameters */
    LIMIT(options.asteroidConcentratorRadius, 1.0, world->diagonal);
    LIMIT(options.asteroidConcentratorProb, 0.0, 1.0);
}

/*
 * Postprocess a change command for the number of items per pack.
 */
void Tune_item_packs(void)
{
    world->items[ITEM_MINE].max_per_pack = options.maxMinesPerPack;
    world->items[ITEM_MISSILE].max_per_pack = options.maxMissilesPerPack;
}


/*
 * Initializes special items.
 * First parameter is type,
 * second and third parameters are minimum and maximum number
 * of elements one item gives when picked up by a ship.
 */
static void Init_item(int item, int minpp, int maxpp)
{
    world->items[item].num = 0;

    world->items[item].min_per_pack = minpp;
    world->items[item].max_per_pack = maxpp;

    Set_item_chance(item);
}


/*
 * Give (or remove) capabilities of the ships depending upon
 * the availability of initial items.
 * Limit the initial resources between minimum and maximum possible values.
 */
void Set_initial_resources(void)
{
    int i;

    LIMIT(world->items[ITEM_FUEL].limit, 0, MAX_FUEL);
    LIMIT(world->items[ITEM_WIDEANGLE].limit, 0, MAX_WIDEANGLE);
    LIMIT(world->items[ITEM_REARSHOT].limit, 0, MAX_REARSHOT);
    LIMIT(world->items[ITEM_AFTERBURNER].limit, 0, MAX_AFTERBURNER);
    LIMIT(world->items[ITEM_CLOAK].limit, 0, MAX_CLOAK);
    LIMIT(world->items[ITEM_SENSOR].limit, 0, MAX_SENSOR);
    LIMIT(world->items[ITEM_TRANSPORTER].limit, 0, MAX_TRANSPORTER);
    LIMIT(world->items[ITEM_TANK].limit, 0, MAX_TANKS);
    LIMIT(world->items[ITEM_MINE].limit, 0, MAX_MINE);
    LIMIT(world->items[ITEM_MISSILE].limit, 0, MAX_MISSILE);
    LIMIT(world->items[ITEM_ECM].limit, 0, MAX_ECM);
    LIMIT(world->items[ITEM_LASER].limit, 0, MAX_LASER);
    LIMIT(world->items[ITEM_EMERGENCY_THRUST].limit, 0, MAX_EMERGENCY_THRUST);
    LIMIT(world->items[ITEM_TRACTOR_BEAM].limit, 0, MAX_TRACTOR_BEAM);
    LIMIT(world->items[ITEM_AUTOPILOT].limit, 0, MAX_AUTOPILOT);
    LIMIT(world->items[ITEM_EMERGENCY_SHIELD].limit, 0, MAX_EMERGENCY_SHIELD);
    LIMIT(world->items[ITEM_DEFLECTOR].limit, 0, MAX_DEFLECTOR);
    LIMIT(world->items[ITEM_PHASING].limit, 0, MAX_PHASING);
    LIMIT(world->items[ITEM_HYPERJUMP].limit, 0, MAX_HYPERJUMP);
    LIMIT(world->items[ITEM_MIRROR].limit, 0, MAX_MIRROR);
    LIMIT(world->items[ITEM_ARMOR].limit, 0, MAX_ARMOR);

    for (i = 0; i < NUM_ITEMS; i++)
	LIMIT(world->items[i].initial, 0, world->items[i].limit);

    for (i = 0; i < NUM_ITEMS; i++)
	LIMIT(world->items[i].cannon_initial, 0, world->items[i].limit);

    CLR_BIT(DEF_HAVE,
	HAS_CLOAKING_DEVICE |
	HAS_EMERGENCY_THRUST |
	HAS_EMERGENCY_SHIELD |
	HAS_PHASING_DEVICE |
	HAS_TRACTOR_BEAM |
	HAS_AUTOPILOT |
	HAS_DEFLECTOR |
	HAS_MIRROR |
	HAS_ARMOR);

    if (world->items[ITEM_CLOAK].initial > 0){
	SET_BIT(DEF_HAVE, HAS_CLOAKING_DEVICE);
	SET_BIT(DEF_USED, USES_CLOAKING_DEVICE);
	}
    if (world->items[ITEM_EMERGENCY_THRUST].initial > 0)
	SET_BIT(DEF_HAVE, HAS_EMERGENCY_THRUST);
    if (world->items[ITEM_EMERGENCY_SHIELD].initial > 0)
	SET_BIT(DEF_HAVE, HAS_EMERGENCY_SHIELD);
    if (world->items[ITEM_PHASING].initial > 0)
	SET_BIT(DEF_HAVE, HAS_PHASING_DEVICE);
    if (world->items[ITEM_TRACTOR_BEAM].initial > 0)
	SET_BIT(DEF_HAVE, HAS_TRACTOR_BEAM);
    if (world->items[ITEM_AUTOPILOT].initial > 0)
	SET_BIT(DEF_HAVE, HAS_AUTOPILOT);
    if (world->items[ITEM_DEFLECTOR].initial > 0)
	SET_BIT(DEF_HAVE, HAS_DEFLECTOR);
    if (world->items[ITEM_MIRROR].initial > 0)
	SET_BIT(DEF_HAVE, HAS_MIRROR);
    if (world->items[ITEM_ARMOR].initial > 0)
	SET_BIT(DEF_HAVE, HAS_ARMOR);
}


void Set_misc_item_limits(void)
{
    LIMIT(options.dropItemOnKillProb, 0.0, 1.0);
    LIMIT(options.detonateItemOnKillProb, 0.0, 1.0);
    LIMIT(options.movingItemProb, 0.0, 1.0);
    LIMIT(options.randomItemProb, 0.0, 1.0);
    LIMIT(options.destroyItemInCollisionProb, 0.0, 1.0);

    LIMIT(options.itemConcentratorRadius, 1, world->diagonal);
    LIMIT(options.itemConcentratorProb, 0.0, 1.0);

    LIMIT(options.asteroidItemProb, 0.0, 1.0);

    if (options.asteroidMaxItems < 0)
	options.asteroidMaxItems = 0;
}


/*
 * First time initialization of all global item stuff.
 */
void Set_world_items(void)
{
    Init_item(ITEM_FUEL, 0, 0);
    Init_item(ITEM_TANK, 1, 1);
    Init_item(ITEM_ECM, 1, 1);
    Init_item(ITEM_ARMOR, 1, 1);
    Init_item(ITEM_MINE, 1, options.maxMinesPerPack);
    Init_item(ITEM_MISSILE, 1, options.maxMissilesPerPack);
    Init_item(ITEM_CLOAK, 1, 1);
    Init_item(ITEM_SENSOR, 1, 1);
    Init_item(ITEM_WIDEANGLE, 1, 1);
    Init_item(ITEM_REARSHOT, 1, 1);
    Init_item(ITEM_AFTERBURNER, 1, 1);
    Init_item(ITEM_TRANSPORTER, 1, 1);
    Init_item(ITEM_MIRROR, 1, 1);
    Init_item(ITEM_DEFLECTOR, 1, 1);
    Init_item(ITEM_HYPERJUMP, 1, 1);
    Init_item(ITEM_PHASING, 1, 1);
    Init_item(ITEM_LASER, 1, 1);
    Init_item(ITEM_EMERGENCY_THRUST, 1, 1);
    Init_item(ITEM_EMERGENCY_SHIELD, 1, 1);
    Init_item(ITEM_TRACTOR_BEAM, 1, 1);
    Init_item(ITEM_AUTOPILOT, 1, 1);

    Set_misc_item_limits();

    Set_initial_resources();
}


void Set_world_rules(void)
{
    static rules_t rules;

    rules.mode =
      ((options.allowPlayerCrashes ? CRASH_WITH_PLAYER : 0)
       | (options.allowPlayerBounces ? BOUNCE_WITH_PLAYER : 0)
       | (options.allowPlayerKilling ? PLAYER_KILLINGS : 0)
       | (options.allowShields ? PLAYER_SHIELDING : 0)
       | (options.limitedVisibility ? LIMITED_VISIBILITY : 0)
       | (options.limitedLives ? LIMITED_LIVES : 0)
       | (options.teamPlay ? TEAM_PLAY : 0)
       | (options.allowAlliances ? ALLIANCES : 0)
       | (options.timing ? TIMING : 0)
       | (options.allowNukes ? ALLOW_NUKES : 0)
       | (options.allowClusters ? ALLOW_CLUSTERS : 0)
       | (options.allowModifiers ? ALLOW_MODIFIERS : 0)
       | (options.allowLaserModifiers ? ALLOW_LASER_MODIFIERS : 0)
       | (options.edgeWrap ? WRAP_PLAY : 0));
    rules.lives = options.worldLives;
    world->rules = &rules;

    if (BIT(world->rules->mode, TEAM_PLAY))
	CLR_BIT(world->rules->mode, ALLIANCES);

    if (!BIT(world->rules->mode, PLAYER_KILLINGS))
	CLR_BIT(KILLING_SHOTS,
		OBJ_SHOT_BIT|OBJ_CANNON_SHOT_BIT|OBJ_SMART_SHOT_BIT
		|OBJ_TORPEDO_BIT|OBJ_HEAT_SHOT_BIT|OBJ_PULSE_BIT);

    if (!BIT(world->rules->mode, PLAYER_SHIELDING))
	CLR_BIT(DEF_HAVE, HAS_SHIELD);

    DEF_USED &= DEF_HAVE;
}

void Set_world_asteroids(void)
{
    world->asteroids.num = 0;
    Tune_asteroid_prob();
}
