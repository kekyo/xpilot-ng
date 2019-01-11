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

#ifndef SERVERCONST_H
#define SERVERCONST_H

#ifndef CONST_H
#include "const.h"
#endif

/*
 * Two macros for edge wrap of x and y coordinates measured in map blocks.
 * Note that the correction needed shouldn't ever be bigger than one mapsize.
 */
#define WRAP_XBLOCK(x_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((x_) < 0 \
		? (x_) + World.x \
		: ((x_) >= World.x \
		    ? (x_) - World.x \
		    : (x_))) \
	    : (x_))

#define WRAP_YBLOCK(y_)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((y_) < 0 \
		? (y_) + World.y \
		: ((y_) >= World.y \
		    ? (y_) - World.y \
		    : (y_))) \
	    : (y_))

/*
 * Two macros for edge wrap of differences in position.
 * If the absolute value of a difference is bigger than
 * half the map size then it is wrapped.
 */
#define WRAP_DX(dx)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((dx) < - (World.width >> 1) \
		? (dx) + World.width \
		: ((dx) > (World.width >> 1) \
		    ? (dx) - World.width \
		    : (dx))) \
	    : (dx))

#define WRAP_DY(dy)	\
	(BIT(World.rules->mode, WRAP_PLAY) \
	    ? ((dy) < - (World.height >> 1) \
		? (dy) + World.height \
		: ((dy) > (World.height >> 1) \
		    ? (dy) - World.height \
		    : (dy))) \
	    : (dy))


#define PSEUDO_TEAM(pl1,pl2)\
	((pl1)->pseudo_team == (pl2)->pseudo_team)

/*
 * Used when we want to pass an index which is not in use.
 */
#define NO_IND			(-1)

#define RECOVERY_DELAY		(12 * 3)
#define ROBOT_CREATE_DELAY	(12 * 2)

/*
 * ID values: In the network protocol these are 16 bit signed values.
 */
#define NO_ID			(-1)

/* Currently there is 256 possible player IDs. */
#define NUM_IDS			256
#define MAX_PSEUDO_PLAYERS	16

/* Expired mines have id EXPIRED_MINE_ID, defined in common/const.h */

/* Cannon IDs. */
#define NUM_CANNON_IDS		10000
#define MIN_CANNON_ID		(EXPIRED_MINE_ID + 1)
#define MAX_CANNON_ID		(EXPIRED_MINE_ID + NUM_CANNON_IDS)

#define MAX_TOTAL_SHOTS		16384	/* must be <= 65536 */

/*
 * Energy drainage
 */
#define ED_SHOT			(-0.2)
#define ED_SMART_SHOT		(-30.0)
#define ED_MINE			(-60.0)
#define ED_ECM			(-60.0)
#define ED_TRANSPORTER		(-60.0)
#define ED_HYPERJUMP		(-60.0)
#define ED_SHIELD		(-0.20)
#define ED_PHASING_DEVICE	(-0.40)
#define ED_CLOAKING_DEVICE	(-0.07)
#define ED_DEFLECTOR		(-0.15)
#define ED_SHOT_HIT		(-25.0)
#define ED_SMART_SHOT_HIT	(-120.0)
#define ED_PL_CRASH		(-options.playerCollisionFuelDrain)
#define ED_BALL_HIT		(-options.ballCollisionFuelDrain)
#define ED_LASER		(-10.0)
#define ED_LASER_HIT		(-100.0)

#define MAX_PLAYER_FUEL		2600.0
#define ENERGY_PACK_FUEL	(500.0 + rfrac() * 511.0)

#define LG2_MAX_AFTERBURNER     4
#define ALT_SPARK_MASS_FACT     4.2
#define ALT_FUEL_FACT	   3
#define MAX_AFTERBURNER	((1<<LG2_MAX_AFTERBURNER)-1)
/*#define AFTER_BURN_SPARKS(s,n)  (((s)*(n))>>LG2_MAX_AFTERBURNER)*/
#define AFTER_BURN_POWER_FACTOR(n) \
 (options.afterburnerPowerMult \
  * (1.0+(n)*((ALT_SPARK_MASS_FACT-1.0)/(MAX_AFTERBURNER+1.0))))
#define AFTER_BURN_POWER(p,n)   \
 ((p)*AFTER_BURN_POWER_FACTOR(n))
#define AFTER_BURN_FUEL(f,n)    \
 (((f)*((MAX_AFTERBURNER+1)+(n)*(ALT_FUEL_FACT-1)))/(MAX_AFTERBURNER+1.0))

#define THRUST_MASS	     0.7
#define ARMOR_MASS		(options.shipMass / 14)

#define MAX_TANKS	       8
#define TANK_MASS	       (options.shipMass / 10)
#define TANK_CAP(n)	     (!(n)?MAX_PLAYER_FUEL:(MAX_PLAYER_FUEL/3))
#define TANK_FUEL(n)	    ((TANK_CAP(n)*(5+(randomMT()&3)))/32)
#define TANK_REFILL_LIMIT       (350.0/8.0)
#define TANK_THRUST_FACT	0.7
#define TANK_NOTHRUST_TIME      (HEAT_CLOSE_TIMEOUT/2+2)
#define TANK_THRUST_TIME	(TANK_NOTHRUST_TIME/2+1)

#define GRAVS_POWER		2.7

#define ECM_DISTANCE		(VISIBILITY_DISTANCE*0.4)
#define TRANSPORTER_DISTANCE	(VISIBILITY_DISTANCE*0.2)

#define MINE_RADIUS		8
#define MINE_RANGE	 	(VISIBILITY_DISTANCE*0.1)
#define MINE_SENSE_BASE_RANGE   (MINE_RANGE*1.3)
#define MINE_SENSE_RANGE_FACTOR (MINE_RANGE*0.3)
#define MINE_MASS		30.0
#define MINE_SPEED_FACT		1.3

#define MISSILE_MASS		5.0
#define MISSILE_RANGE		4
#define SMART_SHOT_ACC		0.6
#define SMART_SHOT_DECFACT	3
#define SMART_SHOT_MIN_SPEED	(SMART_SHOT_ACC*8)
#define SMART_TURNSPEED		2.6
#define SMART_SHOT_MAX_SPEED	22.0
#define SMART_SHOT_LOOK_AH      4
#define CONFUSED_TIME		3
#define TORPEDO_SPEED_TIME      (2 * 12)
#define TORPEDO_ACC		((18.0*SMART_SHOT_MAX_SPEED)/\
				(12*TORPEDO_SPEED_TIME))
#define TORPEDO_RANGE		(MINE_RANGE*0.45)

#define NUKE_SPEED_TIME		(2 * 12)
#define NUKE_ACC 		(5*TORPEDO_ACC)
#define NUKE_RANGE		(MINE_RANGE*1.5)
#define NUKE_MASS_MULT		1
#define NUKE_MINE_EXPL_MULT	3
#define NUKE_SMART_EXPL_MULT	4

#define HEAT_RANGE		(VISIBILITY_DISTANCE/2)
#define HEAT_SPEED_FACT		1.7
#define HEAT_CLOSE_TIMEOUT      (2 * 12)
#define HEAT_CLOSE_RANGE	HEAT_RANGE
#define HEAT_CLOSE_ERROR	0
#define HEAT_MID_TIMEOUT	(4 * 12)
#define HEAT_MID_RANGE		(2 * HEAT_RANGE)
#define HEAT_MID_ERROR		8
#define HEAT_WIDE_TIMEOUT       (8 * 12)
#define HEAT_WIDE_ERROR		16

#define CLUSTER_MASS_SHOTS(mass) ((mass) * 0.9 / options.shotMass)
#define CLUSTER_MASS_DRAIN(mass) (CLUSTER_MASS_SHOTS(mass)*ED_SHOT)

#define SMART_SHOT_LEN		12
#define HEAT_SHOT_LEN		15
#define TORPEDO_LEN		18

#define CANNON_PULSE_LIFE	(4.75)

#define TRACTOR_MAX_RANGE(items)  (200 + (items) * 50)
#define TRACTOR_MAX_FORCE(items)  (-40 + (items) * -20)
#define TRACTOR_PERCENT(dist, maxdist) \
	(1.0 - (0.5 * (dist) / (maxdist)))
#define TRACTOR_COST(percent) (-1.5 * (percent))
#define TRACTOR_FORCE(tr_pr, percent, maxforce) \
	((percent) * (maxforce) * ((tr_pr) ? -1 : 1))

#define WARN_TIME		(2 * 12)
#define EMERGENCY_SHIELD_TIME	(4 * 12)
#define SHIELD_TIME		(2 * 12)
#define PHASING_TIME		(4 * 12)
#define EMERGENCY_THRUST_TIME	(4 * 12)

#define FUEL_MASS(f)		((f) * 0.005)
#define START_STATION_FUEL	MAX_STATION_FUEL
#define STATION_REGENERATION	0.06
#define REFUEL_RATE		5.0
#define TARGET_FUEL_REPAIR_PER_FRAME (TARGET_DAMAGE / (12 * 10))
#define TARGET_REPAIR_PER_FRAME	(TARGET_DAMAGE / (12 * 600))
#define TARGET_UPDATE_DELAY	(TARGET_DAMAGE / (TARGET_REPAIR_PER_FRAME \
				    * BLOCK_SZ))

#define ALLIANCE_NOT_SET	(-1)

#define DEBRIS_MASS		4.5

#define ENERGY_RANGE_FACTOR	2.5

/* Wall code only considers one way of wrapping around the map, and
 * assumes that after moving the length of one line or one unit of object
 * movement (max 32000 clicks) the shortest distance from the start to
 * the end position is the path that was moved (there are some fudge
 * factors in the length). For this to hold, the map must be somewhat
 * larger than 2 * 32000 clicks = 1000 pixels. */
#define MIN_MAP_SIZE		1100
/* map dimension limitation: (0x7FFF - 1280) */
/* Where does 1280 come from? uau */
#define MAX_MAP_SIZE		31500

#define POLYGON_MAX_OFFSET	30000
#define NO_GROUP		(-1)

/* Maximum frames per second the server code supports. */
#define MAX_SERVER_FPS		255

#endif
