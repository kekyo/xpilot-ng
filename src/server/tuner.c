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

void tuner_plock(void)
{
    options.pLockServer
	= (plock_server(options.pLockServer) == 1) ? true : false;
}

void tuner_shipmass(void)
{
    int i;

    for (i = 0; i < NumPlayers; i++)
	Player_by_index(i)->emptymass = options.shipMass;
}

void tuner_ballmass(void)
{
    int i;

    for (i = 0; i < NumObjs; i++) {
	if (Obj[i]->type == OBJ_BALL)
	    Obj[i]->mass = options.ballMass;
    }
}

void tuner_maxrobots(void)
{
    if (options.maxRobots < 0)
	options.maxRobots = Num_bases();

    if (options.maxRobots < options.minRobots)
	options.minRobots = options.maxRobots;

    while (options.maxRobots < NumRobots)
	Robot_delete(NULL, true);
}

void tuner_minrobots(void)
{
    if (options.minRobots < 0)
	options.minRobots = options.maxRobots;

    if (options.maxRobots < options.minRobots)
	options.maxRobots = options.minRobots;
}

void tuner_allowshields(void)
{
    int i;

    Set_world_rules();

    if (options.allowShields) {
	SET_BIT(DEF_HAVE, HAS_SHIELD);

	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl_i = Player_by_index(i);

	    if (!Player_is_tank(pl_i)) {
		if (!BIT(pl_i->used, HAS_SHOT))
		    SET_BIT(pl_i->used, HAS_SHIELD);

		SET_BIT(pl_i->have, HAS_SHIELD);
		pl_i->shield_time = 0;
	    }
	}
    }
    else {
	CLR_BIT(DEF_HAVE, HAS_SHIELD);

	for (i = 0; i < NumPlayers; i++)
	    /* approx 2 seconds to get to safety */
	    Player_by_index(i)->shield_time = SHIELD_TIME;
    }
}

void tuner_playerstartsshielded(void)
{
    if (options.allowShields)
	/* Doesn't make sense to turn off when shields are on. */
	options.playerStartsShielded = true;
}

void tuner_worldlives(void)
{
    if (options.worldLives < 0)
	options.worldLives = 0;

    Set_world_rules();

    if (BIT(world->rules->mode, LIMITED_LIVES)) {
	Reset_all_players();
	if (options.gameDuration == -1)
	    options.gameDuration = 0;
    }
}

void tuner_cannonsmartness(void)
{
    LIMIT(options.cannonSmartness, 0, CANNON_SMARTNESS_MAX);
}

void tuner_teamcannons(void)
{
    int i;
    int team;

    if (options.teamCannons) {
	for (i = 0; i < Num_cannons(); i++) {
	    cannon_t *cannon = Cannon_by_index(i);

	    team = Find_closest_team(cannon->pos);
	    if (team == TEAM_NOT_SET)
		warn("Couldn't find a matching team for the cannon.");
	    cannon->team = team;
	}
    }
    else {
	for (i = 0; i < Num_cannons(); i++)
	    Cannon_by_index(i)->team = TEAM_NOT_SET;
    }
}

void tuner_mincannonshotlife(void)
{
    LIMIT(options.minCannonShotLife, 0, FLT_MAX);
    LIMIT(options.maxCannonShotLife, options.minCannonShotLife, FLT_MAX);
}

void tuner_maxcannonshotlife(void)
{
    LIMIT(options.maxCannonShotLife, 0, FLT_MAX);
    LIMIT(options.minCannonShotLife, 0, options.maxCannonShotLife);
}

void tuner_wormhole_stable_ticks(void)
{
    int i;

    if (options.wormholeStableTicks < 1.0)
	options.wormholeStableTicks = 1.0;

    /* Make sure all wormholes get a new destination */
    for (i = 0; i < Num_wormholes(); i++)
	Wormhole_by_index(i)->countdown = 0.0;
}

void tuner_modifiers(void)
{
    int i;

    Set_world_rules();

    for (i = 0; i < NumPlayers; i++)
	Mods_filter(&(Player_by_index(i))->mods);
}

void tuner_gameduration(void)
{
    if (options.gameDuration <= 0.0)
	gameOverTime = time(NULL);
    else
	gameOverTime = (time_t) (options.gameDuration * 60) + time(NULL);
}

void tuner_racelaps(void)
{
    if (BIT(world->rules->mode, TIMING)) {
	Reset_all_players();
	if (options.gameDuration == -1)
	    options.gameDuration = 0;
    }
}

void tuner_allowalliances(void)
{
    if (BIT(world->rules->mode, TEAM_PLAY))
	CLR_BIT(world->rules->mode, ALLIANCES);

    if (!BIT(world->rules->mode, ALLIANCES) && NumAlliances > 0)
	Dissolve_all_alliances();
}

void tuner_announcealliances(void)
{
    updateScores = true;
}

void tuner_playerwallbouncetype(void)
{
    int type = options.playerWallBounceType;

    if (!(type >= 0 && type <= 3))
	options.playerWallBounceType = 3;
}
