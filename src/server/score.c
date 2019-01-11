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

#include "xpserver.h"

void Score(player_t * pl, double points, clpos_t pos, const char *msg)
{
    Rank_add_score(pl, points);
    if (pl->conn != NULL)
	Send_score_object(pl->conn, points, pos, msg);
    updateScores = true;
}

double Rate(double winner, double loser)
{
    double t;

    if (options.constantScoring)
	return RATE_SIZE / 2;
    t = ((RATE_SIZE / 2) * RATE_RANGE) / (ABS(loser - winner) +
					  RATE_RANGE);
    if (loser > winner)
	t = RATE_SIZE - t;
    return t;
}

/*
 * Cause 'winner' to get 'winner_score' points added with message
 * 'winner_msg', and similarly with the 'loser' and equivalent
 * variables.
 *
 * In general the winner_score should be positive, and the loser_score
 * negative, but this need not be true.
 *
 * If the winner and loser players are on the same team, the scores are
 * made negative, since you shouldn't gain points by killing team members,
 * or being killed by a team member (it is both players faults).
 *
 * KK 28-4-98: Same for killing your own tank.
 * KK 7-11-1: And for killing a member of your alliance
 */
void Score_players(player_t * winner_pl, double winner_score,
		   char *winner_msg, player_t * loser_pl,
		   double loser_score, char *loser_msg, bool transfer_tag)
{
    if (Players_are_teammates(winner_pl, loser_pl)
	|| Players_are_allies(winner_pl, loser_pl)
	|| (Player_is_tank(loser_pl)
	    && loser_pl->lock.pl_id == winner_pl->id)) {
	if (winner_score > 0)
	    winner_score = -winner_score;
	if (loser_score > 0)
	    loser_score = -loser_score;
    }

    if (options.tagGame
	&& winner_score > 0.0 && loser_score < 0.0 && transfer_tag) {
	if (tagItPlayerId == winner_pl->id) {
	    winner_score *= options.tagItKillScoreMult;
	    loser_score *= options.tagItKillScoreMult;
	} else if (tagItPlayerId == loser_pl->id) {
	    winner_score *= options.tagKillItScoreMult;
	    loser_score *= options.tagKillItScoreMult;
	    Transfer_tag(loser_pl, winner_pl);
	}
    }

    Score(winner_pl, winner_score, loser_pl->pos, winner_msg);
    Score(loser_pl, loser_score, loser_pl->pos, loser_msg);
}

double Get_Score(player_t * pl)
{
    return pl->score;
}

void Set_Score(player_t * pl, double score)
{
    pl->score = score;
}

void Add_Score(player_t * pl, double score)
{
    pl->score += score;
}

void Handle_Scoring(scoretype_t st, player_t * killer, player_t * victim,
		    void *extra, const char *somemsg)
{
    double sc = 0.0, sc2 = 0.0, factor = 0.0;
    int i_tank_owner = 0, j = 0;
    player_t *true_killer;

    if (!(killer || victim)) {
	warn("Attempted to score with neither victim nor killer");
	return;
    }

    switch (st) {
    case SCORE_CANNON_KILL:
	sc = Rate(Get_Score(killer), ((cannon_t *) extra)->score)
	    * options.cannonKillScoreMult;
	if (BIT(world->rules->mode, TEAM_PLAY)
	    && killer->team == ((cannon_t *) extra)->team)
	    sc = -sc;
	if (!options.zeroSumScoring)
	    Score(killer, sc, ((cannon_t *) extra)->pos, "");
	break;
    case SCORE_WALL_DEATH:
	sc = Rate(WALL_SCORE, Get_Score(victim));
	if (somemsg) {
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, somemsg);
	} else {
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, victim->name);
	}
	break;
    case SCORE_COLLISION:
	if (!Player_is_tank(killer) && !Player_is_tank(victim)) {
	    sc = Rate(Get_Score(victim),
		      Get_Score(killer)) * options.crashScoreMult;
	    sc2 =
		Rate(Get_Score(killer),
		     Get_Score(victim)) * options.crashScoreMult;
	    if (!options.zeroSumScoring)
		Score_players(killer, -sc, victim->name, victim, -sc2,
			      killer->name, false);
	    else
		Score_players(killer, sc - sc2, victim->name, victim,
			      sc2 - sc, killer->name, false);
	} else if (Player_is_tank(killer)) {
	    player_t *i_tank_owner_pl = Player_by_id(killer->lock.pl_id);
	    sc = Rate(Get_Score(i_tank_owner_pl), Get_Score(victim))
		* options.tankKillScoreMult;
	    Score_players(i_tank_owner_pl, sc, victim->name, victim, -sc,
			  killer->name, true);
	} else if (Player_is_tank(victim)) {
	    player_t *j_tank_owner_pl = Player_by_id(victim->lock.pl_id);
	    sc = Rate(Get_Score(j_tank_owner_pl), Get_Score(killer))
		* options.tankKillScoreMult;
	    Score_players(j_tank_owner_pl, sc, killer->name, killer, -sc,
			  victim->name, true);
	}
	/* don't bother scoring two tanks */
	break;
    case SCORE_ROADKILL:
	true_killer = killer;
	if (Player_is_tank(killer)) {
	    i_tank_owner = GetInd(killer->lock.pl_id);
	    if (i_tank_owner == GetInd(victim->id))
		i_tank_owner = GetInd(killer->id);
	    true_killer = Player_by_index(i_tank_owner);
	    Rank_add_tank_kill(true_killer);
	    sc = Rate(Get_Score(true_killer), Get_Score(victim))
		* options.tankKillScoreMult;
	} else {
	    Rank_add_runover_kill(killer);
	    sc = Rate(Get_Score(killer),
		      Get_Score(victim)) * options.runoverKillScoreMult;
	}
	Score_players(true_killer, sc, victim->name, victim, -sc,
		      killer->name, true);
	break;
    case SCORE_BALL_KILL:
	if (!killer) {
	    sc = Rate(0.0, Get_Score(victim)) * options.ballKillScoreMult
		* options.unownedKillScoreMult;
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, "Ball");
	} else {
	    if (killer == victim) {
		sc = Rate(0.0,
			  Get_Score(victim)) * options.ballKillScoreMult *
		    options.selfKillScoreMult;
		if (!options.zeroSumScoring)
		    Score(victim, -sc, victim->pos, killer->name);
	    } else {
		Rank_add_ball_kill(killer);
		sc = Rate(Get_Score(killer),
			  Get_Score(victim)) * options.ballKillScoreMult;
		Score_players(killer, sc, victim->name, victim, -sc,
			      killer->name, true);
	    }
	}
	break;
    case SCORE_HIT_MINE:
	sc = Rate(Get_Score(killer),
		  Get_Score(victim)) * options.mineScoreMult;
	Score_players(killer, sc, victim->name, victim, -sc, killer->name,
		      false);
	break;
    case SCORE_EXPLOSION:
	if (!killer || killer->id == victim->id) {
	    sc = Rate(0.0,
		      Get_Score(victim)) * options.explosionKillScoreMult *
		options.selfKillScoreMult;
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos,
		      (killer == NULL) ? "[Explosion]" : victim->name);
	} else {
	    Rank_add_explosion_kill(killer);
	    sc = Rate(Get_Score(killer), Get_Score(victim))
		* options.explosionKillScoreMult;
	    Score_players(killer, sc, victim->name, victim, -sc,
			  killer->name, true);
	}
	break;
    case SCORE_ASTEROID_KILL:
	sc = Rate(Get_Score(killer),
		  ASTEROID_SCORE) * options.unownedKillScoreMult;
	if (!options.zeroSumScoring)
	    Score(killer, sc, ((wireobject_t *) extra)->pos, "");
	break;
    case SCORE_ASTEROID_DEATH:
	sc = Rate(0.0, Get_Score(victim)) * options.unownedKillScoreMult;
	if (!options.zeroSumScoring)
	    Score(victim, -sc, victim->pos, "[Asteroid]");
	break;
    case SCORE_SHOT_DEATH:
	if (BIT(((object_t *) extra)->obj_status, FROMCANNON)) {
	    cannon_t *cannon = Cannon_by_id(((object_t *) extra)->id);

	    /*KHS: for cannon dodgers; cannon hit substracts */
	    /* fixed percentage of score */

	    if (options.survivalScore != 0.0)
		sc = Get_Score(victim) * 0.02;
	    else if (cannon != NULL)
		sc = Rate(cannon->score, Get_Score(victim))
		    * options.cannonKillScoreMult;
	    else {
		assert(((object_t *) extra)->id == NO_ID);
		sc = Rate(UNOWNED_SCORE, Get_Score(victim))
		    * options.cannonKillScoreMult;
	    }
	} else if (((object_t *) extra)->id == NO_ID) {
	    sc = Rate(0.0,
		      Get_Score(victim)) * options.unownedKillScoreMult;
	} else {
	    if (killer->id == victim->id) {
		sc = Rate(0.0,
			  Get_Score(victim)) * options.selfKillScoreMult;
	    } else {
		Rank_add_shot_kill(killer);
		sc = Rate(Get_Score(killer), Get_Score(victim));
	    }
	}

	switch (((object_t *) extra)->type) {
	case OBJ_SHOT:
	    if (Mods_get(((object_t *) extra)->mods, ModsCluster))
		factor = options.clusterKillScoreMult;
	    else
		factor = options.shotKillScoreMult;
	    break;
	case OBJ_TORPEDO:
	    factor = options.torpedoKillScoreMult;
	    break;
	case OBJ_SMART_SHOT:
	    factor = options.smartKillScoreMult;
	    break;
	case OBJ_HEAT_SHOT:
	    factor = options.heatKillScoreMult;
	    break;
	default:
	    factor = options.shotKillScoreMult;
	    break;
	}

	sc *= factor;
	if (BIT(((object_t *) extra)->obj_status, FROMCANNON)) {
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, "Cannon");
	} else if ((((object_t *) extra)->id == NO_ID)
		   || (killer && (killer->id == victim->id))) {
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos,
		      ((object_t *) extra)->id ==
		      NO_ID ? "" : victim->name);
	} else {
	    Score_players(killer, sc, victim->name, victim, -sc,
			  killer->name, true);
	}

	break;
    case SCORE_LASER:
	if (killer) {
	    if (victim->id == killer->id) {
		sc = Rate(0.0,
			  Get_Score(killer)) * options.laserKillScoreMult *
		    options.selfKillScoreMult;
		if (!options.zeroSumScoring)
		    Score(killer, -sc, killer->pos, killer->name);
	    } else {
		sc = Rate(Get_Score(killer),
			  Get_Score(victim)) * options.laserKillScoreMult;
		Score_players(killer, sc, victim->name, victim, -sc,
			      killer->name, true);
		Rank_add_laser_kill(killer);
	    }
	} else if (((cannon_t *) extra) != NULL) {
	    sc = Rate(((cannon_t *) extra)->score, Get_Score(victim))
		* options.cannonKillScoreMult;
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, "Cannon");
	} else {
	    sc = Rate(UNOWNED_SCORE,
		      Get_Score(victim)) * options.unownedKillScoreMult;
	    if (!options.zeroSumScoring)
		Score(victim, -sc, victim->pos, "");
	    Set_message_f("%s got roasted alive.", victim->name);
	}
	break;
    case SCORE_TARGET:
	{
	    double por, win_score = 0.0, lose_score = 0.0;
	    int win_team_members = 0, lose_team_members =
		0, targets_remaining = 0, targets_total = 0;
	    target_t *targ = (target_t *) extra;
	    bool somebody = false;

	    if (BIT(world->rules->mode, TEAM_PLAY)) {
		for (j = 0; j < NumPlayers; j++) {
		    player_t *pl = Player_by_index(j);

		    if (Player_is_tank(pl)
			|| (Player_is_paused(pl) && pl->pause_count <= 0)
			|| Player_is_waiting(pl))
			continue;

		    if (pl->team == targ->team) {
			lose_score += Get_Score(pl);
			lose_team_members++;
			if (!Player_is_dead(pl))
			    somebody = true;
		    } else if (pl->team == killer->team) {
			win_score += Get_Score(pl);
			win_team_members++;
		    }
		}
	    }
	    if (somebody) {
		for (j = 0; j < Num_targets(); j++) {
		    target_t *t = Target_by_index(j);

		    if (t->team == targ->team) {
			targets_total++;
			if (t->dead_ticks <= 0)
			    targets_remaining++;
		    }
		}
	    }
	    if (!somebody)
		break;

	    sound_play_sensors(targ->pos, DESTROY_TARGET_SOUND);

	    if (targets_remaining > 0) {
		sc = Rate(Get_Score(killer), TARGET_SCORE) / 4;
		sc = sc * (targets_total -
			   targets_remaining) / (targets_total + 1);
		if (sc >= 0.01)
		    if (!options.zeroSumScoring)
			Score(killer, sc, targ->pos, "Target: ");
		break;
	    }

	    if (options.targetKillTeam)
		Rank_add_target_kill(killer);

	    sc = Rate(win_score, lose_score);
	    por = (sc * lose_team_members) / win_team_members;

	    for (j = 0; j < NumPlayers; j++) {
		player_t *pl = Player_by_index(j);

		if (Player_is_tank(pl)
		    || (Player_is_paused(pl) && pl->pause_count <= 0)
		    || Player_is_waiting(pl))
		    continue;

		if (pl->team == targ->team) {
		    if (options.targetKillTeam
			&& targets_remaining == 0 && Player_is_alive(pl))
			Player_set_state(pl, PL_STATE_KILLED);
		    if (!options.zeroSumScoring)
			Score(pl, -sc, targ->pos, "Target: ");
		} else if (pl->team == killer->team &&
			   (pl->team != TEAM_NOT_SET
			    || pl->id == killer->id))
		    if (!options.zeroSumScoring)
			Score(pl, por, targ->pos, "Target: ");
	    }
	    break;
	}
    case SCORE_TREASURE:
	{
	    double win_score = 0.0, lose_score = 0.0, por;
	    int i, win_team_members = 0, lose_team_members = 0;
	    treasure_t *treasure = (treasure_t *) extra;
	    bool somebody = false;

	    if (BIT(world->rules->mode, TEAM_PLAY)) {
		for (i = 0; i < NumPlayers; i++) {
		    player_t *pl_i = Player_by_index(i);

		    if (Player_is_tank(pl_i)
			|| (Player_is_paused(pl_i)
			    && pl_i->pause_count <= 0)
			|| Player_is_waiting(pl_i))
			continue;
		    if (pl_i->team == treasure->team) {
			lose_score += Get_Score(pl_i);
			lose_team_members++;
			if (!Player_is_dead(pl_i))
			    somebody = true;
		    } else if (pl_i->team == killer->team) {
			win_score += Get_Score(pl_i);
			win_team_members++;
		    }
		}
	    }

	    if (!somebody) {
		if (!options.zeroSumScoring)
		    Score(killer, Rate(Get_Score(killer),
				       TREASURE_SCORE) / 2, treasure->pos,
			  "Treasure:");
		break;
	    }

	    treasure->destroyed++;
	    world->teams[treasure->team].TreasuresLeft--;
	    world->teams[killer->team].TreasuresDestroyed++;

	    sc = 3 * Rate(win_score, lose_score);
	    por = (sc * lose_team_members) / (2 * win_team_members + 1);

	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (Player_is_tank(pl_i)
		    || (Player_is_paused(pl_i) && pl_i->pause_count <= 0)
		    || Player_is_waiting(pl_i))
		    continue;

		if (pl_i->team == treasure->team) {
		    Score(pl_i, -sc, treasure->pos, "Treasure: ");
		    Rank_lost_ball(pl_i);
		    if (options.treasureKillTeam)
			Player_set_state(pl_i, PL_STATE_KILLED);
		} else if (pl_i->team == killer->team &&
			   (pl_i->team != TEAM_NOT_SET
			    || pl_i->id == killer->id)) {
		    if (lose_team_members > 0) {
			if (pl_i->id == killer->id)
			    Rank_cashed_ball(pl_i);
			Rank_won_ball(pl_i);
		    }
		    Score(pl_i,
			  (pl_i->id == killer->id ? 3 * por : 2 * por),
			  treasure->pos, "Treasure: ");
		}
	    }

	    if (options.treasureKillTeam)
		Rank_add_treasure_kill(killer);

	    break;
	}
    case SCORE_SELF_DESTRUCT:
	if (options.selfDestructScoreMult != 0) {
	    sc = Rate(0.0,
		      Get_Score(killer)) * options.selfDestructScoreMult;
	    if (!options.zeroSumScoring)
		Score(killer, -sc, killer->pos, "Self-Destruct");
	}
	break;
    case SCORE_SHOVE_KILL:
	sc = (*((double *) extra)) * Rate(Get_Score(killer),
					  Get_Score(victim)) *
	    options.shoveKillScoreMult;
	if (!options.zeroSumScoring)
	    Score(killer, sc, victim->pos, victim->name);
	break;
    case SCORE_SHOVE_DEATH:
	sc = (*((double *) extra)) * Rate(killer->score,
					  Get_Score(victim)) *
	    options.shoveKillScoreMult;
	if (!options.zeroSumScoring)
	    Score(victim, -sc, victim->pos, "[Shove]");
	break;
    default:
	error("unknown scoring type!");
	break;
    }
}
