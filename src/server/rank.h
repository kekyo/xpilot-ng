/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1999-2004 by
 *
 *      Marcus Sundberg      <mackan@stacken.kth.se>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#ifndef RANK_H
#define RANK_H

#include "xpcommon.h"

#ifndef PLAYER_H
/* need player */
#include "player.h"
#endif

typedef struct ranknode {

    char name[MAX_NAME_LEN];
    char user[MAX_NAME_LEN];
    char host[MAX_HOST_LEN];

    time_t timestamp;

    int kills, deaths;
    int rounds, shots;
    int deadliest;
    int ballsCashed, ballsSaved;
    int ballsWon, ballsLost;
    double bestball;
    double score;
    player_t *pl;
    double max_survival_time;
} ranknode_t;

bool Rank_get_stats(const char *name, char *buf, size_t size);
ranknode_t *Rank_get_by_name(const char *name);
void Rank_init_saved_scores(void);
void Rank_get_saved_score(player_t *pl);
void Rank_save_score(player_t *pl);
void Rank_write_rankfile(void);
void Rank_write_webpage(void);
void Rank_show_ranks(void);

static inline void Rank_add_score(player_t *pl, double points)
{
    Add_Score(pl,points);
    pl->update_score = true;
    if (pl->rank)
	pl->rank->score += points;
}

static inline void Rank_set_score(player_t *pl, double points)
{
    Set_Score(pl,points);
    pl->update_score = true;
    if (pl->rank)
	pl->rank->score = points;
}

static inline void Rank_fire_shot(player_t *pl)
{
    pl->shots++;
    if (pl->rank)
	pl->rank->shots++;
}

static inline void Rank_add_kill(player_t *pl)
{
    pl->kills++;
    if (pl->rank)
	pl->rank->kills++;
}

static inline void Rank_add_death(player_t *pl)
{
    pl->deaths++;
    if (pl->rank)
	pl->rank->deaths++;
}

static inline void Rank_add_round(player_t *pl)
{
    if (pl->rank)
	pl->rank->rounds++;
}

static inline void Rank_add_deadliest(player_t *pl)
{
    if (pl->rank)
	pl->rank->deadliest++;
}

static inline void Rank_cashed_ball(player_t *pl)
{
    if (pl->rank)
	pl->rank->ballsCashed++;
}

static inline void Rank_saved_ball(player_t *pl)
{
    if (pl->rank)
	pl->rank->ballsSaved++;
}

static inline void Rank_won_ball(player_t *pl)
{
    if (pl->rank)
	pl->rank->ballsWon++;
}

static inline void Rank_lost_ball(player_t *pl)
{
    if (pl->rank)
	pl->rank->ballsLost++;
}

static inline void Rank_ballrun(player_t *pl, double tim)
{
    if (pl->rank) {
	if (pl->rank->bestball == 0.0 || tim < pl->rank->bestball)
	    pl->rank->bestball = tim;
    }
}

static inline void Rank_survival(player_t *pl, double tim)
{
    if (pl->rank) {
        if (pl->rank->max_survival_time == 0
            || tim > pl->rank->max_survival_time)
	    pl->rank->max_survival_time = tim;
    }
}


static inline double Rank_get_max_survival_time(player_t *pl)
{
    return pl->rank ? pl->rank->max_survival_time : 0;
}


static inline double Rank_get_best_ballrun(player_t *pl)
{
    return pl->rank ? pl->rank->bestball : 0.0;
}

static inline void Rank_add_ball_kill(player_t *pl)      { Rank_add_kill(pl); }
static inline void Rank_add_explosion_kill(player_t *pl) { Rank_add_kill(pl); }
static inline void Rank_add_laser_kill(player_t *pl)     { Rank_add_kill(pl); }
static inline void Rank_add_runover_kill(player_t *pl)   { Rank_add_kill(pl); }
static inline void Rank_add_shot_kill(player_t *pl)      { Rank_add_kill(pl); }
static inline void Rank_add_shove_kill(player_t *pl)     { Rank_add_kill(pl); }
static inline void Rank_add_tank_kill(player_t *pl)      { Rank_add_kill(pl); }
static inline void Rank_add_target_kill(player_t *pl)    { Rank_add_kill(pl); }
static inline void Rank_add_treasure_kill(player_t *pl)  { Rank_add_kill(pl); }


#endif /* RANK_H */
