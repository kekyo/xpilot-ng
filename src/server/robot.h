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

/* Robot code originally submitted by Maurice Abraham. */

#ifndef ROBOT_H
#define ROBOT_H

#ifndef MAP_H
# include "map.h"
#endif

/*
 * We would like to support several different robot types.
 * Each robot type is defined by one name and is accessed
 * by several functions or methods.
 * The name is used to enable configuration files to
 * specify which robot type to use for which robot name.
 * The functions are:
 *
 *    1) The global initialization function for this specific robot type.
 *       It is called once at startup to initialise a structure with
 *       function pointers to the robot type action routines.
 *
 *    2) The next round function, which is called only once per round
 *       to allow the robot type adjust type generic parameters.
 *
 *    3) The initialization function to enable robot type
 *       specific adjustments and possible memory allocation
 *       of private data structures for one specific robot instance.
 *       This function will be called with an extra string argument.
 *       The string argument may contain robot type specific
 *       configuration data, like attack and defend mode.
 *       Each robot type is free to define the format
 *       for this string as it sees fit.  But when this string
 *       is empty the robot type initialization code will
 *       have to provide suitable default configuration values.
 *
 *    4) The go home function which is called whenever the robot
 *       is placed at its homebase.
 *
 *    5) The playing function which gets called each loop.
 *       The programming challenge here is to implement
 *       different characters per robot, while at the
 *       same time not calculating 'everything' every
 *       time this function is called.  i.e., try to give
 *       the robot some long term goals which are only
 *       recalculated once every couple of seconds or so.
 *
 *    6) The war function which is called when the robot declares war.
 *       This function takes an extra killer argument, which is
 *       the player id of the player the robot has declared war against.
 *       If this id equals NO_ID then the routine should reset the war state.
 *
 *    7) The war_on_player function returns the id of the player
 *       who the robot is in a state of war with.
 *       If the robot is not in war with another player
 *       then this function should return NO_ID.
 *
 *    8) The message function can accept any possible commands to the robot.
 *       The suggestion here is that if you plan to let the robot type
 *       be controllable by messages that you give feedback to all
 *       of the players what commands the robot has accepted.
 *
 *    9) The cleanup function should free any allocated resources
 *       for this specific robot instance.
 *
 * The recommended practice is to define your new robot types
 * in a separate file and to only declare your robot type
 * specific initialisation function prototype in robot.c and add one
 * function pointer to the robot_type_setups array.
 */
typedef struct {
    const char		*name;
    void		(*robot_round_tick)(void);
    void		(*robot_create)(player_t *robot, char *str);
    void		(*robot_go_home)(player_t *robot);
    void		(*robot_play)(player_t *robot);
    void		(*robot_set_war)(player_t *victim, int killer);
    int			(*robot_war_on_player)(player_t *robot);
    void		(*robot_message)(player_t *robot, const char *str);
    void		(*robot_destroy)(player_t *robot);
    void		(*robot_invite)(player_t *robot, player_t *inviter);
} robot_type_t;

/*
 * Different talk commands.
 * The talk code is done by the robot manager,
 * not by the robot type implementation.
 */
enum robot_talk_t {
    ROBOT_TALK_ENTER,
    ROBOT_TALK_LEAVE,
    ROBOT_TALK_KILL,
    ROBOT_TALK_WAR,
    NUM_ROBOT_TALK_TYPES
};

/*
 * Configuration data for each robot available.
 */
typedef struct {
    char	driver[MAX_NAME_LEN];	/* Which code controls robot? */
    char	name[MAX_NAME_LEN];	/* Name of this robot. */
    char	config[MAX_CHARS];	/* Attack + defense ~ 100 */
    unsigned	used;			/* How often has this robot been used */
    char	shape[2*MSG_LEN];	/* shipshape string definition */
} robot_t;

/*
 * Robot manager data for each robot instance.
 */
typedef struct robot_data {
    int		robots_ind;		/* index into Robots[] */
    int		robot_types_ind;	/* index into robot_types[] */
    void	*private_data;		/* robot type private data */
} robot_data_t;

/*
 * The private robot instance data for the default robot.
 */
typedef struct robot_default_data {
    int		robot_lock;		/* lock mode */
    int		robot_lock_id;		/* target player if in war mode */
    int		robot_mode;		/* ultrashort term mode of robot. */
    int		robot_count;		/* Misc timings, minimizes rand use */
    int		attack;			/* how aggressive (1-99) */
    int		defense;		/* how defensive (1-99) */
    double	robot_normal_speed;
    double	robot_attack_speed;
    double	robot_max_speed;
    int		last_dropped_mine;      /* relative to robot_count */
    int		last_fired_missile;     /* relative to robot_count */
    int		last_thrown_ball;	/* relative to robot_count */
    int		longterm_mode;		/* long term robot mode */
    double	fuel_l1;		/* Fuel critical level */
    double	fuel_l2;		/* Fuel warning level */
    double	fuel_l3;		/* Fuel notify level */
} robot_default_data_t;

/*
 * The private robot instance data for the simple ng robot.
 */
typedef struct robot_suibot_data {
    int         robot_lock;             /* lock mode */
    int         robot_lock_id;          /* target player if in war mode */
    int         robot_mode;             /* ultrashort term mode of robot. */
    int         robot_count;            /* Misc timings, minimizes rand use */
    int         attack;                 /* how aggressive (1-99) */
    int         defense;                /* how defensive (1-99) */
    int         longterm_mode;          /* long term robot mode */
    double      fuel_l1;                /* Fuel critical level */
    double      fuel_l2;                /* Fuel warning level */
    int last_attacked_player; /* relative to robot_count */
} robot_suibot_data_t;


/*
 * Prototypes for robot.c
 */
void Parse_robot_file(void);
void Robot_init(void);
void Robot_delete(player_t *robot, bool kicked);
void Robot_destroy(player_t *robot);
void Robot_update(bool tick);
void Robot_invite(player_t *robot, player_t *inviter);
void Robot_war(player_t *robot, player_t *killer);
void Robot_reset_war(player_t *robot);
int Robot_war_on_player(player_t *robot);
void Robot_go_home(player_t *robot);
void Robot_program(player_t *robot, int victim_id);
void Robot_message(player_t *robot, const char *message);

#endif
