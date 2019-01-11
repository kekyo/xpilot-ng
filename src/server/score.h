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

#ifndef SCORE_H
#define SCORE_H

#define ASTEROID_SCORE		(-1436.0)
#define CANNON_SCORE	    	(-1436.0)
#define TARGET_SCORE	    	(-1436.0)
#define TREASURE_SCORE	    	(-1436.0)
#define UNOWNED_SCORE	    	(-1436.0)
#define WALL_SCORE	    	2000.0

#define RATE_SIZE	    	20
#define RATE_RANGE	    	1024

/* score.c */

void Score(player_t * pl, double points, clpos_t pos, const char *msg);
double Rate(double winner, double loser);
void Score_players(player_t * winner_pl, double winner_score,
		   char *winner_msg, player_t * loser_pl,
		   double loser_score, char *loser_msg, bool transfer_tag);

double Get_Score(player_t * pl);

typedef enum {
    SCORE_CANNON_KILL,
    SCORE_WALL_DEATH,
    SCORE_COLLISION,
    SCORE_ROADKILL,
    SCORE_BALL_KILL,
    SCORE_HIT_MINE,
    SCORE_EXPLOSION,
    SCORE_ASTEROID_KILL,
    SCORE_ASTEROID_DEATH,
    SCORE_SHOT_DEATH,
    SCORE_LASER,
    SCORE_TARGET,
    SCORE_TREASURE,
    SCORE_SELF_DESTRUCT,
    SCORE_SHOVE_KILL,
    SCORE_SHOVE_DEATH
} scoretype_t;

void Handle_Scoring(scoretype_t st, player_t * killer, player_t * victim,
		    void *extra, const char *somemsg);
#endif
