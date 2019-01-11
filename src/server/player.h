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

#ifndef	PLAYER_H
#define	PLAYER_H

#ifndef OBJECT_H
/* need OBJECT_BASE */
#include "object.h"
#endif
#ifndef CONNECTION_H
/* need connection_t */
#include "connection.h"
#endif
#ifndef SERVERCONST_H
/* need MAX_TANKS */
#include "serverconst.h"
#endif
#ifndef KEYS_H
/* need NUM_KEYS */
#include "keys.h"
#endif
#ifndef BIT_H
/* need BITV_DECL */
#include "bit.h"
#endif
#ifndef DRAW_H
/* need shipshape_t */
#include "shipshape.h"
#endif
#ifndef ITEM_H
/* need NUM_ITEMS */
#include "item.h"
#endif
#ifndef CLICK_H
/* need CLICK */
#include "click.h"
#endif
#ifndef OPTION_H
/* need options */
#include "option.h"
#endif

extern bool		updateScores;

/*
 * These values are set in the player->pl_type field.
 */
#define PL_TYPE_HUMAN		0
#define PL_TYPE_ROBOT		1
#define PL_TYPE_TANK		2

/*
 * These values are set in the player->pl_state field.
 */
#define PL_STATE_UNDEFINED	0
#define PL_STATE_WAITING	1
#define PL_STATE_APPEARING	2
#define PL_STATE_ALIVE		3
#define PL_STATE_KILLED		4
#define PL_STATE_DEAD 		5
#define PL_STATE_PAUSED 	6

/*
 * Different types of attributes a player can have.
 * These are the bits of the player->have and player->used fields.
 */
#define HAS_EMERGENCY_THRUST	(1U<<30)
#define HAS_AUTOPILOT		(1U<<29)
#define HAS_TRACTOR_BEAM	(1U<<28)
#define HAS_LASER		(1U<<27)
#define HAS_CLOAKING_DEVICE	(1U<<26)
#define HAS_SHIELD		(1U<<25)
#define HAS_REFUEL		(1U<<24)
#define HAS_REPAIR		(1U<<23)
#define HAS_COMPASS		(1U<<22)
#define HAS_AFTERBURNER		(1U<<21)
#define HAS_CONNECTOR		(1U<<20)
#define HAS_EMERGENCY_SHIELD	(1U<<19)
#define HAS_DEFLECTOR		(1U<<18)
#define HAS_PHASING_DEVICE	(1U<<17)
#define HAS_MIRROR		(1U<<16)
#define HAS_ARMOR		(1U<<15)
#define HAS_SHOT		(1U<<4)
#define HAS_BALL		(1U<<3)

#define USES_EMERGENCY_THRUST	HAS_EMERGENCY_THRUST
#define USES_AUTOPILOT		HAS_AUTOPILOT
#define USES_TRACTOR_BEAM	HAS_TRACTOR_BEAM
#define USES_LASER		HAS_LASER
#define USES_CLOAKING_DEVICE	HAS_CLOAKING_DEVICE
#define USES_SHIELD		HAS_SHIELD
#define USES_REFUEL		HAS_REFUEL
#define USES_REPAIR		HAS_REPAIR
#define USES_COMPASS		HAS_COMPASS
#define USES_AFTERBURNER	HAS_AFTERBURNER
#define USES_CONNECTOR		HAS_CONNECTOR
#define USES_EMERGENCY_SHIELD	HAS_EMERGENCY_SHIELD
#define USES_DEFLECTOR		HAS_DEFLECTOR
#define USES_PHASING_DEVICE	HAS_PHASING_DEVICE
#define USES_MIRROR		HAS_MIRROR
#define USES_ARMOR		HAS_ARMOR
#define USES_SHOT		HAS_SHOT
#define USES_BALL		HAS_BALL

/*
 * Possible player status bits.
 * The bits that the client needs must fit into a byte,
 * so the first 8 bitvalues are reserved for that purpose,
 * those are defined in common/rules.h.
 */
#define HOVERPAUSE		(1U<<9)		/* Hovering pause */
#define REPROGRAM		(1U<<10)	/* Player reprogramming */
#define FINISH			(1U<<11)	/* Finished a lap this frame */
#define RACE_OVER		(1U<<12)	/* After finished and score */

#define LOCK_NONE		0x00	/* No lock */
#define LOCK_PLAYER		0x01	/* Locked on player */
#define LOCK_VISIBLE		0x02	/* Lock information was on HUD */
					/* computed just before frame shown */
					/* and client input checked */
#define LOCKBANK_MAX		4	/* Maximum number of locks in bank */

/*
 * Fuel structure, used by player
 */
typedef struct {
    double	sum;			/* Sum of fuel in all tanks */
    double	max;			/* How much fuel can you take? */
    int		current;		/* Number of currently used tank */
    int		num_tanks;		/* Number of tanks */
    double	tank[1 + MAX_TANKS];	/* main fixed tank + extra tanks. */
} pl_fuel_t;

typedef struct visibility {
    bool	canSee;
    long	lastChange;
} visibility_t;

/*
 * Shove-information.
 *
 * This is for keeping a record of the last N times the player was shoved,
 * for assigning wall-smash-blame, where N=MAX_RECORDED_SHOVES.
 */
#define MAX_RECORDED_SHOVES 4

typedef struct {
    int		pusher_id;
    int		time;
} shove_t;

struct robot_data;
struct ranknode;

/* IMPORTANT
 *
 * This is the player structure, the first part MUST be similar to object_t,
 * this makes it possible to use the same basic operations on both of them
 * (mainly used in update.c).
 */
typedef struct player {

    OBJECT_BASE

    /* up to here the player type should be the same as an object. */

    int		pl_type;		/* extended type info (tank, robot) */
    char	pl_type_mychar;		/* Special char for player type */
    uint8_t	pl_old_status;		/* OLD_PLAYING etc. */

    uint16_t	pl_status;		/* HOVERPAUSE etc. */
    uint16_t	pl_state;		/* one of PL_STATE_* */
    int		pl_life;		/* Lives left (if lives limited) */
    int		pl_deaths_since_join;	/* Deaths since last joining server */

    uint16_t	pl_prev_team;		/* Team before pause */

    double	turnspeed;		/* How fast player acc-turns */
    double	velocity;		/* Absolute speed */

    int		kills;			/* Number of kills this round */
    int		deaths;			/* Number of deaths this round */

    long	used;			/* Items you use */
    long	have;			/* Items you have */

    double	shield_time;		/* Shields if no options.playerShielding */
    pl_fuel_t	fuel;			/* ship tanks and the stored fuel */
    double	emptymass;		/* Mass of empty ship */
    double	float_dir;		/* Direction, in float var */
    double	wanted_float_dir;   	/*TURNQUEUE*//* Direction, in float var */
    double	float_dir_cos;		/* Cosine of float_dir */
    double	float_dir_sin;		/* Sine of float_dir */
    double	turnresistance;		/* How much is lost in % */
    double	turnvel;		/* Current velocity of turn (right) */
    double	oldturnvel;		/* Last velocity of turn (right) */
    double	turnacc;		/* Current acceleration of turn */
    double	score;			/* Current score of player */
    bool    	update_score;	    	/* score table info needs to be sent */
    shipshape_t	*ship;			/* wire model of ship shape */
    double	power;			/* Force of thrust */
    double	power_s;		/* Saved power fiks */
    double	turnspeed_s;		/* Saved turnspeed */
    double	turnresistance_s;	/* Saved (see above) */
    double	sensor_range;		/* Range of sensors (radar) */
    int		shots;			/* Number of active shots by player */
    int		missile_rack;		/* Next missile rack to be active */

    int		num_pulses;		/* Number of laser pulses "flying". */

    double	emergency_thrust_left;	/* how much emergency thrust left */
    double	emergency_shield_left;	/* how much emergency shield left */
    double	phasing_left;		/* how much time left */

    double	pause_count;		/* ticks until unpause possible */
    double	recovery_count;		/* ticks to recovery */
    double	self_destruct_count;	/* if > 0, ticks before boom */

    int		item[NUM_ITEMS];	/* for each item type how many */
    int		initial_item[NUM_ITEMS];/* items player had initially */
    int		lose_item;		/* which item to drop */
    int		lose_item_state;	/* lose item key state, 2=up,1=down */

    double	auto_power_s;		/* autopilot saves of current */
					/* power, turnspeed and */
    double	auto_turnspeed_s;	/* turnresistance settings. Restored */
    double	auto_turnresistance_s;	/* when autopilot turned off */
    modifiers_t	modbank[NUM_MODBANKS];	/* useful modifier settings */
    double	shot_time;		/* Time of last shot fired by player */
    double	laser_time;		/* Time of last laser pulse fired by player */
    bool	did_shoot;		/* Pressed fire during this frame */
    bool	tractor_is_pressor;	/* on if tractor is pressor */
    int		repair_target;		/* Repairing this target */
    int		fs;			/* Connected to fuel station fs */
    int		check;			/* Next check point to pass */
    int		prev_check;		/* Previous check point for score */
    int		time;			/* The time a player has used */
    int		round;			/* Number of rounds player have done */
    int		prev_round;		/* Previous rounds value for score */
    int		best_lap;		/* Players best lap time */
    int		last_lap;		/* Time on last pass */
    int		last_lap_time;		/* What was your last pass? */
    int		last_check_dir;		/* player dir at last checkpoint */
    long	last_wall_touch;	/* last time player touched a wall */
    double      survival_time;          /* time player has survived unshielded*/

    base_t	*home_base;
    struct {
	int	    tagged;		/* Flag, what is tagged? */
	int	    pl_id;		/* Tagging player id */
	double	    distance;		/* Distance to object */
    } lock;
    int		lockbank[LOCKBANK_MAX]; /* Saved player locks */

    short	dir;			/* Direction of acceleration */
    char	mychar;			/* Special char for player */
    char	name[MAX_CHARS];	/* Nick-name of player */
    char	username[MAX_CHARS];	/* Real name of player */
    char	hostname[MAX_CHARS];	/* Hostname of client player uses */
    uint16_t	pseudo_team;		/* Which team for detaching tanks */
    int		alliance;		/* Member of which alliance? */
    int		invite;			/* Invitation for alliance */
    ballobject_t	*ball;

    /*
     * Pointer to robot private data (dynamically allocated).
     * Only used in robot code.
     */
    struct robot_data	*robot_data_ptr;

    /*
     * A record of who's been pushing me (a circular buffer).
     */
    shove_t     shove_record[MAX_RECORDED_SHOVES];
    int	 	shove_next;

    visibility_t *visibility;

    double	forceVisible;
    double	damaged;
    double	stunned;
    bool	updateVisibility;

    int		last_target_update;	/* index of last updated target */
    int		last_cannon_update;	/* index of last updated cannon */
    int		last_fuel_update;	/* index of last updated fuel */
    int		last_polystyle_update;	/* index of last updated polygon */

    int		ecmcount;		/* number of active ecms */

    connection_t *conn;			/* connection index, NULL if robot */
    unsigned	version;		/* XPilot version number of client */

    BITV_DECL(last_keyv, NUM_KEYS);	/* Keyboard state */
    BITV_DECL(prev_keyv, NUM_KEYS);	/* Keyboard state */

    long	frame_last_busy;	/* When player touched keyboard. */

    void	*audio;			/* audio private data */

    int		player_fps;		/* FPS that this player can do */
    int		maxturnsps;		/* turns per second limit */

    int		rectype;		/* normal, saved or spectator */
    struct ranknode	*rank;

    double	pauseTime;		/* seconds player has paused */
    double	idleTime;		/* seconds player has idled */

    int	 	flooding;

    bool	muted;			/* player started is muted? */
    bool	isoperator;		/* player has operator privileges? */
    bool	want_audio;		/* player wants audio from server */

    int		privs;			/* Player privileges */

#define PRIV_NOAUTOKICK		1
#define PRIV_AUTOKICKLAST	2

    double	snafu_count;		/* Misc. snafus */

} player_t;

extern int	playerArrayNumber;
extern player_t	**PlayersArray;

int GetInd(int id);

/*
 * Get player with index 'ind' from Players array.
 */
static inline player_t *Player_by_index(int ind)
{
    if (ind < 0 || ind >= playerArrayNumber)
	return NULL;
    return PlayersArray[ind];
}

static inline player_t *Player_by_id(int id)
{
    return Player_by_index(GetInd(id));
}

static inline bool Player_is_waiting(player_t *pl)
{
    return pl->pl_state == PL_STATE_WAITING ? true : false;
}

static inline bool Player_is_appearing(player_t *pl)
{
    return pl->pl_state == PL_STATE_APPEARING ? true : false;
}

static inline bool Player_is_alive(player_t *pl)
{
    return pl->pl_state == PL_STATE_ALIVE ? true : false;
}

/* player was killed this frame ? */
static inline bool Player_is_killed(player_t *pl)
{
    return pl->pl_state == PL_STATE_KILLED ? true : false;
}

static inline bool Player_is_dead(player_t *pl)
{
    return pl->pl_state == PL_STATE_DEAD ? true : false;
}

static inline bool Player_is_paused(player_t *pl)
{
    return pl->pl_state == PL_STATE_PAUSED ? true : false;
}

static inline bool Player_is_hoverpaused(player_t *pl)
{
    if (BIT(pl->pl_status, HOVERPAUSE))
	return true;
    return false;
}

extern void Set_Score(player_t *pl, double score);
extern void Add_Score(player_t *pl, double score);

static inline void Player_add_score(player_t *pl, double points)
{
    Add_Score(pl,points);
    pl->update_score = true;
    updateScores = true;
}

static inline void Player_set_score(player_t *pl, double points)
{
    Set_Score(pl,points);
    pl->update_score = true;
    updateScores = true;
}

static inline void Player_set_mychar(player_t *pl, char mychar)
{
    pl->mychar = mychar;
    pl->update_score = true;
    updateScores = true;
}

static inline void Player_set_life(player_t *pl, int life)
{
    pl->pl_life = life;
    pl->update_score = true;
    updateScores = true;
}

static inline void Player_set_alliance(player_t *pl, int alliance)
{
    pl->alliance = alliance;
    pl->update_score = true;
    updateScores = true;
}

/*
 * Should be:
 * if (pl->item[ITEM_AFTERBURNER] > 0) return true; else return false;
 */
static inline bool Player_has_afterburner(player_t *pl)
{
    if (BIT(pl->have, HAS_AFTERBURNER))
	return true;
    return false;
}

/*
 * Should be:
 * if (pl->item[ITEM_ARMOR] > 0) return true; else return false;
 */
static inline bool Player_has_armor(player_t *pl)
{
    if (BIT(pl->have, HAS_ARMOR))
	return true;
    return false;
}

/*
 * Should be:
 * if (pl->item[ITEM_AUTOPILOT] > 0) return true; else return false;
 */
static inline bool Player_has_autopilot(player_t *pl)
{
    if (BIT(pl->have, HAS_AUTOPILOT))
	return true;
    return false;
}

/*
 * Should be:
 * if (pl->item[ITEM_CLOAK] > 0) return true; else return false;
 */
static inline bool Player_has_cloaking_device(player_t *pl)
{
    if (BIT(pl->have, HAS_CLOAKING_DEVICE))
	return true;
    return false;
}

/*
 * Should be:
 * if (pl->item[ITEM_DEFLECTOR] > 0) return true; else return false;
 */
static inline bool Player_has_deflector(player_t *pl)
{
    if (BIT(pl->have, HAS_DEFLECTOR))
	return true;
    return false;
}

/*
 * Should be:
 * if (pl->item[ITEM_MIRROR] > 0) return true; else return false;
 */
static inline bool Player_has_mirror(player_t *pl)
{
    if (BIT(pl->have, HAS_MIRROR))
	return true;
    return false;
}


/*
 * Should be:
 * if (pl->item[ITEM_TRACTOR_BEAM] > 0) return true; else return false;
 */
static inline bool Player_has_tractor_beam(player_t *pl)
{
    if (BIT(pl->have, HAS_TRACTOR_BEAM))
	return true;
    return false;
}

static inline bool Player_is_thrusting(player_t *pl)
{
    if (BIT(pl->obj_status, THRUSTING))
	return true;
    return false;
}

static inline bool Player_is_refueling(player_t *pl)
{
    if (BIT(pl->used, USES_REFUEL))
	return true;
    return false;
}

static inline bool Player_is_repairing(player_t *pl)
{
    if (BIT(pl->used, USES_REPAIR))
	return true;
    return false;
}

static inline bool Player_is_self_destructing(player_t *pl)
{
    return (pl->self_destruct_count > 0.0) ? true : false;
}

static inline void Player_self_destruct(player_t *pl, bool on)
{
    if (on) {
	if (Player_is_self_destructing(pl))
	    return;
	pl->self_destruct_count = SELF_DESTRUCT_DELAY;
    }
    else
	pl->self_destruct_count = 0.0;
}

static inline bool Player_is_human(player_t *pl)
{
    return pl->pl_type == PL_TYPE_HUMAN ? true : false;
}

static inline bool Player_is_robot(player_t *pl)
{
    return pl->pl_type == PL_TYPE_ROBOT ? true : false;
}

static inline bool Player_is_tank(player_t *pl)
{
    return pl->pl_type == PL_TYPE_TANK ? true : false;
}

static inline bool Player_owns_tank(player_t *pl, player_t *tank)
{
    if (Player_is_tank(tank)
	&& tank->lock.pl_id != NO_ID  /* kps - probably redundant */
	&& tank->lock.pl_id == pl->id)
	return true;
    return false;
}

/*
 * Used where we wish to know if a player is simply on the same team.
 */
static inline bool Players_are_teammates(player_t *pl1, player_t *pl2)
{
    if (BIT(world->rules->mode, TEAM_PLAY)
	&& pl1->team != TEAM_NOT_SET
	&& pl1->team == pl2->team)
	return true;
    return false;
}

/*
 * Used where we wish to know if two players are members of the same alliance.
 */
static inline bool Players_are_allies(player_t *pl1, player_t *pl2)
{
    if (pl1->alliance != ALLIANCE_NOT_SET
	&& pl1->alliance == pl2->alliance)
	return true;
    return false;
}

static inline bool Player_uses_autopilot(player_t *pl)
{
    if (BIT(pl->used, USES_AUTOPILOT))
	return true;
    return false;
}

static inline bool Player_uses_compass(player_t *pl)
{
    if (BIT(pl->used, USES_COMPASS))
	return true;
    return false;
}

static inline bool Player_uses_connector(player_t *pl)
{
    if (BIT(pl->used, USES_CONNECTOR))
	return true;
    return false;
}

static inline bool Player_uses_tractor_beam(player_t *pl)
{
    if (BIT(pl->used, USES_TRACTOR_BEAM))
	return true;
    return false;
}

static inline bool Player_uses_emergency_shield(player_t *pl)
{
    if (BIT(pl->used, (HAS_SHIELD|HAS_EMERGENCY_SHIELD)) ==
	(HAS_SHIELD|HAS_EMERGENCY_SHIELD))
	return true;
    return false;
}

static inline bool Player_has_emergency_thrust(player_t *pl)
{
#if 0
    if (pl->item[ITEM_EMERGENCY_THRUST] > 0
	|| pl->emergency_thrust_left > 0.0)
	return true;
    return false;
#else
    if (BIT(pl->have, HAS_EMERGENCY_THRUST))
	return true;
    return false;
#endif
}

static inline bool Player_uses_emergency_thrust(player_t *pl)
{
    if (BIT(pl->used, USES_EMERGENCY_THRUST))
	return true;
    return false;
}

static inline bool Player_has_phasing_device(player_t *pl)
{
#if 0
    if (pl->item[ITEM_PHASING] > 0
	|| pl->phasing_left > 0.0)
	return true;
#else
    if (BIT(pl->have, HAS_PHASING_DEVICE))
	return true;
    return false;
#endif
    return false;
}

static inline bool Player_is_phasing(player_t *pl)
{
    if (BIT(pl->used, USES_PHASING_DEVICE))
	return true;
    return false;
}

static inline bool Player_is_cloaked(player_t *pl)
{
    if (BIT(pl->used, USES_CLOAKING_DEVICE))
	return true;
    return false;
}

static inline bool Player_is_active(player_t *pl)
{
    if (Player_is_alive(pl)
	|| Player_is_killed(pl))
	return true;
    return false;
}

/* kps - add id.h ? */
static inline bool Is_player_id(int id)
{
    if (id >= 1 && id <= NUM_IDS)
	return true;
    return false;
}

static inline bool Is_cannon_id(int id)
{
    if (id >= MIN_CANNON_ID && id <= MAX_CANNON_ID)
	return true;
    return false;
}

/*
 * Prototypes for player.c
 */

void Pick_startpos(player_t *pl);
void Go_home(player_t *pl);
Item_t Item_by_option_name(const char *name);
void Base_set_option(base_t *base, const char *name, const char *value);
void Compute_sensor_range(player_t *pl);
void Player_add_tank(player_t *pl, double tank_fuel);
void Player_remove_tank(player_t *pl, int which_tank);
void Player_hit_armor(player_t *pl);
void Player_used_kill(player_t *pl);
void Player_set_mass(player_t *pl);
void Player_init_items(player_t *pl);
int Init_player(int ind, shipshape_t *ship, int type);
void Alloc_players(int number);
void Free_players(void);
void Update_score_table(void);
void Reset_all_players(void);
void Check_team_members(int);
void Compute_game_status(void);
void Delete_player(player_t *pl);
void Add_spectator(player_t *pl);
void Delete_spectator(player_t *pl);
void Detach_ball(player_t *pl, ballobject_t *ball);
void Kill_player(player_t *pl, bool add_rank_death);
void Player_death_reset(player_t *pl, bool add_rank_death);
void Count_rounds(void);
void Team_game_over(int winning_team, const char *reason);
void Individual_game_over(int winner);
bool Team_immune(int id1, int id2);

static inline void Player_set_float_dir(player_t *pl, double new_float_dir)
{
    if (options.ngControls && new_float_dir != pl->float_dir) {
	pl->float_dir = new_float_dir;
	pl->float_dir_cos = cos(pl->float_dir * 2.0 * PI / RES);
	pl->float_dir_sin = sin(pl->float_dir * 2.0 * PI / RES);
    } else
	pl->float_dir = new_float_dir;
}

void Player_print_state(player_t *pl, const char *funcname);
void Player_set_state(player_t *pl, int state);
void Player_set_modbank(player_t *pl, int bank, const char *str);

#endif
