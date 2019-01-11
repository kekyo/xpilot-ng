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

void Race_compute_game_status(void)
{
    /*
     * We need a completely separate scoring system for race mode.
     * I'm not sure how race mode should interact with team mode,
     * so for the moment race mode takes priority.
     *
     * Race mode and limited lives mode interact. With limited lives on,
     * race ends after all players have completed the course, or have died.
     * With limited lives mode off, the race ends when the first player
     * completes the course - all remaining players are then killed to
     * reset them.
     *
     * In limited lives mode, where the race can be run to completion,
     * points are awarded not just to the winner but to everyone who
     * completes the course (with more going to the winner). These
     * points are awarded as the player crosses the line. At the end
     * of the race, a bonus is awarded to the player with the fastest lap.
     *
     * In unlimited lives mode, just the winner and the holder of the
     * fastest lap get points.
     */

    player_t *alive = NULL, *pl;
    int num_alive_players = 0, num_active_players = 0,
	num_finished_players = 0, num_race_over_players = 0,
	num_waiting_players = 0, pos = 1, total_pts, i;
    double pts;
    char msg[MSG_LEN];

    /*
     * kps - ng wants to handle laps here, requires change in collision.c
     * too, maybe I'll fix it later.
     */
#if 0
    /* Handle finishing of laps */
    for (i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);
	if (!BIT(pl->pl_status, FINISH))
	    continue;
	pl->last_lap_time = pl->time - pl->last_lap;
	if ((pl->best_lap > pl->last_lap_time || pl->best_lap == 0)
	    && pl->time != 0 && pl->round != 1) {
	    pl->best_lap = pl->last_lap_time;
	}
	pl->last_lap = pl->time;
	if (pl->round > options.raceLaps) {
	    Player_death_reset(pl);
	    Player_set_state(pl, PL_STATE_DEAD);
	    sprintf(msg, "%s finished the race. Last lap time: %.2fs. "
		    "Personal race best lap time: %.2fs.",
		    pl->name,
		    (double) pl->last_lap_time / FPS,
		    (double) pl->best_lap / FPS);
	}
	else if (pl->round > 1)
	    sprintf(msg, "%s completes lap %d in %.2fs. "
		    "Personal race best lap time: %.2fs.",
		    pl->name,
		    pl->round-1,
		    (double) pl->last_lap_time / FPS,
		    (double) pl->best_lap / FPS);
	else {
	    sprintf(msg, "%s starts lap 1 of %d", pl->name,
		    options.raceLaps);
	    CLR_BIT(pl->pl_status, FINISH); /* no elimination from starting */
	}
	Set_message(msg);
    }
    if (options.eliminationRace) {
	for (;;) {
	    int pli, count = 0, lap = INT_MAX;
	    player_t *pl_i;

	    for (i = 0; i < NumPlayers; i++) {
		pl = Player_by_index(i);
		if (BIT(pl->pl_status, FINISH) && pl->round < lap) {
		    lap = pl->round;
		    pli = i;
		}
	    }
	    if (lap == INT_MAX)
		break;
	    pl_i = Player_by_index(pli);
	    CLR_BIT(pl_i->pl_status, FINISH);
	    lap = 0;
	    for (i = 0; i < NumPlayers; i++) {
		pl = Player_by_index(i);
		if (!Player_is_active(pl))
		    continue;
		if (pl->round < pl_i->round) {
		    count++;
		    if (pl->round > lap)
			lap = pl->round;
		}
	    }
	    if (pl_i->round < lap + count)
		continue;
	    for (i = 0; i < NumPlayers; i++) {
		pl = Player_by_index(i);
		if (!Player_is_active(pl))
		    continue;
		if (pl->round < pl_i->round) {
		    Player_death_reset(pl);
		    Player_set_state(pl, PL_STATE_DEAD);
		    if (count == 1) {
			sprintf(msg, "%s was the last to complete lap "
				"%d and is out of the race.",
				pl->name, pl_i->round - 1);
			Set_message(msg);
		    }
		    else {
			sprintf(msg, "%s was the last to complete some "
				"lap between %d and %d.", pl->name,
				pl->round, pl_i->round - 1);
			Set_message(msg);
		    }
		}
	    }
	}
    }
#endif

    /* First count the players */
    for (i = 0; i < NumPlayers; i++)  {
	pl = Player_by_index(i);
	if (Player_is_paused(pl)
	    || Player_is_tank(pl))
	    continue;

	if (Player_is_waiting(pl)) {
	    num_waiting_players++;
	    continue;
	} else if (!Player_is_dead(pl))
	    num_alive_players++;

	if (BIT(pl->pl_status, RACE_OVER)) {
	    num_race_over_players++;
	    pos++;
	}
	else if (BIT(pl->pl_status, FINISH)) {
	    if (pl->round > options.raceLaps)
		num_finished_players++;
	    else
		CLR_BIT(pl->pl_status, FINISH);
	}
	else if (!Player_is_dead(pl))
	    alive = pl;

	/*
	 * An active player is one who is:
	 *   still in the race.
	 *   reached the finish line just now.
	 *   has finished the race in a previous frame.
	 *   died too often.
	 */
	num_active_players++;
    }
    if (num_active_players == 0 && num_waiting_players == 0)
	return;

    /* Now if any players are unaccounted for */
    if (num_finished_players > 0) {
	/*
	 * Ok, update positions. Everyone who finished the race in the last
	 * frame gets the current position.
	 */

	/* Only play the sound for the first person to cross the finish */
	if (pos == 1)
	    sound_play_all(PLAYER_WIN_SOUND);

	total_pts = 0;
	for (i = 0; i < num_finished_players; i++)
	    total_pts
		+= (10 + 2 * num_active_players) >> (pos - 1 + i);
	pts = total_pts / num_finished_players;

	for (i = 0; i < NumPlayers; i++)  {
	    pl = Player_by_index(i);
	    if (Player_is_paused(pl)
		|| Player_is_waiting(pl)
		|| Player_is_tank(pl))
		continue;

	    if (BIT(pl->pl_status, FINISH)) {
		CLR_BIT(pl->pl_status, FINISH);
		SET_BIT(pl->pl_status, RACE_OVER);
		if (pts > 0) {
		    sprintf(msg,
			    "%s finishes %sin position %d "
			    "scoring %.2f point%s.",
			    pl->name,
			    (num_finished_players == 1) ? "" : "jointly ",
			    pos, pts,
			    (pts == 1) ? "" : "s");
		    Set_message(msg);
		    sprintf(msg, "[Position %d%s]", pos,
			    (num_finished_players == 1) ? "" : " (jointly)");
		    if (!options.zeroSumScoring) Score(pl, pts, pl->pos, msg);
		}
		else {
		    sprintf(msg,
			    "%s finishes %sin position %d.",
			    pl->name,
			    (num_finished_players == 1) ? "" : "jointly ",
			    pos);
		    Set_message(msg);
		}
	    }
	}
    }

    /*
     * If the maximum allowed time for this race is over, end it.
     */
    if (options.maxRoundTime > 0 && roundtime == 0) {
	Set_message("Timer expired. Race ends now.");
	Race_game_over();
	return;
    }

    /*
     * In limited lives mode, wait for everyone to die, except
     * for the last player.
     */
    if (BIT(world->rules->mode, LIMITED_LIVES)) {
	if (num_alive_players > 1)
	    return;
	if (num_alive_players == 1 && num_active_players == 1)
	    return;
    }
    /* !@# fix
     * No meaningful messages / scores if someone wins by staying alive
     */
    else if (num_finished_players == 0 || num_alive_players > 1)
	return;

    Race_game_over();
}

void Race_game_over(void)
{
    player_t *pl;
    int i, j, k,
	bestlap = 0, num_best_players = 0,
	num_active_players = 0, num_ordered_players = 0, *order;

    /*
     * Reassign players's starting positions based upon
     * personal best lap times.
     */
    if ((order = XMALLOC(int, NumPlayers)) != NULL) {
	for (i = 0; i < NumPlayers; i++) {
	    pl = Player_by_index(i);
	    if (Player_is_tank(pl))
		continue;
	    if (Player_is_paused(pl)
		|| Player_is_waiting(pl)
		|| pl->best_lap <= 0)
		j = i;
	    else {
		for (j = 0; j < i; j++) {
		    player_t *pl_j = Player_by_index(order[j]);

		    if (pl->best_lap < pl_j->best_lap)
			break;

		    if (Player_is_paused(pl_j)
			|| Player_is_waiting(pl_j))
			break;
		}
	    }
	    for (k = i - 1; k >= j; k--)
		order[k + 1] = order[k];
	    order[j] = i;
	    num_ordered_players++;
	}
	for (i = 0; i < num_ordered_players; i++) {
	    pl = Player_by_index(order[i]);
	    if (pl->home_base->ind != i) {
		pl->home_base = Base_by_index(i);
		for (j = 0; j < spectatorStart + NumSpectators; j++) {
		    if (j == NumPlayers) {
			if (NumSpectators)
			    j = spectatorStart;
			else
			    break;
		    }
		    if (Player_by_index(j)->conn != NULL)
			Send_base(Player_by_index(j)->conn,
				  pl->id, pl->home_base->ind);
		}
		if (Player_is_paused(pl))
		    Go_home(pl);
	    }
	}
	free(order);
    }

    for (i = NumPlayers - 1; i >= 0; i--)  {
	pl = Player_by_index(i);
	CLR_BIT(pl->pl_status, RACE_OVER | FINISH);
	if (Player_is_paused(pl)
	    || Player_is_waiting(pl)
	    || Player_is_tank(pl))
	    continue;
	num_active_players++;

	/* Kill any remaining players */
	if (!Player_is_dead(pl))
	    Kill_player(pl, false);
	else
	    Player_death_reset(pl, false);

	if (pl != Player_by_index(i))
	    continue;

	if ((pl->best_lap < bestlap || bestlap == 0) &&
	    pl->best_lap > 0) {
	    bestlap = pl->best_lap;
	    num_best_players = 0;
	}
	if (pl->best_lap == bestlap)
	    num_best_players++;
    }

    /* If someone completed a lap */
    if (bestlap > 0) {
	for (i = 0; i < NumPlayers; i++)  {
	    pl = Player_by_index(i);
	    if (Player_is_paused(pl)
		|| Player_is_waiting(pl)
		|| Player_is_tank(pl))
		continue;

	    if (pl->best_lap == bestlap) {
		Set_message_f("%s %s the best lap time of %.2fs",
			      pl->name,
			      (num_best_players == 1) ? "had" : "shares",
			      (double) bestlap / FPS);
		if (!options.zeroSumScoring) Score(pl, 5.0 + num_active_players, pl->pos,
		      (num_best_players == 1)
		      ? "[Fastest lap]" : "[Joint fastest lap]");
	    }
	}

	updateScores = true;
    }
    else if (num_active_players > NumRobots)
	Set_message("No-one even managed to complete one lap, you should be "
		    "ashamed of yourselves.");

    Count_rounds();
    Reset_all_players();
}

void Player_reset_timing(player_t *pl)
{
    pl->round = 0;
    pl->check = 0;
    pl->time = 0;
    pl->best_lap = 0;
    pl->last_lap = 0;
    pl->last_lap_time = 0;
}

void Player_pass_checkpoint(player_t *pl)
{
    int j;

    if (pl->check == 0) {
	pl->round++;
#if 1
	pl->last_lap_time = pl->time - pl->last_lap;
	if ((pl->best_lap > pl->last_lap_time
	     || pl->best_lap == 0)
	    && pl->time != 0
	    && pl->round != 1) {
	    pl->best_lap = pl->last_lap_time;
	}
	pl->last_lap = pl->time;
	if (pl->round > options.raceLaps) {
	    if (options.ballrace) {
		/* Balls are made unowned when their owner finishes the race
		   This way, they can be reused by other players */
		for (j = 0; j < NumObjs; j++) {
		    if (Obj[j]->type == OBJ_BALL) {
			ballobject_t *ball = BALL_PTR(Obj[j]);

			if (ball->ball_owner == pl->id)
			    ball->ball_owner = NO_ID;
		    }
		}
	    }
	    Player_death_reset(pl, false);
	    Player_set_state(pl, PL_STATE_DEAD);
	    SET_BIT(pl->pl_status, FINISH);
	    Set_message_f("%s finished the race. Last lap time: %.2fs. "
			  "Personal race best lap time: %.2fs.",
			  pl->name,
			  (double) pl->last_lap_time / FPS,
			  (double) pl->best_lap / FPS);
	} else if (pl->round > 1) {
	    Set_message_f("%s completes lap %d in %.2fs. "
			  "Personal race best lap time: %.2fs.",
			  pl->name,
			  pl->round-1,
			  (double) pl->last_lap_time / FPS,
			  (double) pl->best_lap / FPS);
	} else
	    Set_message_f("%s starts lap 1 of %d.",
			  pl->name, options.raceLaps);
#else
	/* this is how 4.3.1X did this */
	SET_BIT(pl->pl_status, FINISH);
	/* Rest done in Compute_game_status() */
#endif
    }

    if (++pl->check == world->NumChecks)
	pl->check = 0;
    pl->last_check_dir = pl->dir;

    updateScores = true;
}

void PlayerCheckpointCollision(player_t *pl)
{
    if (!BIT(world->rules->mode, TIMING))
	return;

    if (Player_is_active(pl)) {
	check_t *check = Check_by_index(pl->check);

	if (pl->round != 0)
	    pl->time++;
	if (Player_is_alive(pl)
	    && Wrap_length(pl->pos.cx - check->pos.cx,
			   pl->pos.cy - check->pos.cy)
	    < options.checkpointRadius * BLOCK_CLICKS
	    && !Player_is_tank(pl)
	    && !options.ballrace)
	    Player_pass_checkpoint(pl);
    }
}
