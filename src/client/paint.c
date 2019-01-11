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

#include "xpclient.h"

/*
 * Globals.
 */
ipos_t	world;
ipos_t	realWorld;

bool	players_exposed;
short	ext_view_width;		/* Width of extended visible area */
short	ext_view_height;	/* Height of extended visible area */
int	active_view_width;	/* Width of active map area displayed. */
int	active_view_height;	/* Height of active map area displayed. */
int	ext_view_x_offset;	/* Offset ext_view_width */
int	ext_view_y_offset;	/* Offset ext_view_height */

long	loops = 0;
long	loopsSlow = 0;	/* Proceeds slower than loops */
double	timePerFrame = 0.0;
static double   time_counter = 0.0;

unsigned	draw_width, draw_height;
int	num_spark_colors;


int Check_view_dimensions(void)
{
    int			width_wanted, height_wanted;
    int			srv_width, srv_height;

    width_wanted = (int)(draw_width * clData.scaleFactor + 0.5);
    height_wanted = (int)(draw_height * clData.scaleFactor + 0.5);

    srv_width = width_wanted;
    srv_height = height_wanted;
    LIMIT(srv_height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(srv_width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    if (server_display.view_width != srv_width ||
	server_display.view_height != srv_height ||
	server_display.num_spark_colors != num_spark_colors ||
	server_display.spark_rand != spark_rand) {
	if (Send_display(srv_width, 
			 srv_height, 
			 spark_rand, 
			 num_spark_colors))
	    return -1;
    }
    spark_rand = server_display.spark_rand;
    active_view_width = server_display.view_width;
    active_view_height = server_display.view_height;
    ext_view_x_offset = 0;
    ext_view_y_offset = 0;
    if (width_wanted > active_view_width) {
	ext_view_width = width_wanted;
	ext_view_x_offset = (width_wanted - active_view_width) / 2;
    } else
	ext_view_width = active_view_width;

    if (height_wanted > active_view_height) {
	ext_view_height = height_wanted;
	ext_view_y_offset = (height_wanted - active_view_height) / 2;
    } else
	ext_view_height = active_view_height;

    return 0;
}

void Paint_frame_start(void)
{
    Check_view_dimensions();

    world.x = selfPos.x - (ext_view_width / 2);
    world.y = selfPos.y - (ext_view_height / 2);
    realWorld = world;
    if (BIT(Setup->mode, WRAP_PLAY)) {
	if (world.x < 0 && world.x + ext_view_width < Setup->width)
	    world.x += Setup->width;
	else if (world.x > 0 && world.x + ext_view_width >= Setup->width)
	    realWorld.x -= Setup->width;
	if (world.y < 0 && world.y + ext_view_height < Setup->height)
	    world.y += Setup->height;
	else if (world.y > 0 && world.y + ext_view_height >= Setup->height)
	    realWorld.y -= Setup->height;
    }

    if (start_loops != end_loops)
	warn("Start neq. End (%ld,%ld,%ld)", start_loops, end_loops, loops);
    loops = end_loops;

    /*
     * If time() changed from previous value, assume one second has passed.
     */
    if (newSecond) {
	/* kps - improve */
	recordFPS = (int)(clientFPS+0.5);
	timePerFrame = 1.0 / recordFPS;

	/* TODO: move this somewhere else */
	/* check once per second if we are playing */
	if (newSecond && self && !strchr("PW", self->mychar))
	    played_this_round = true;
    }

    /*
     * Instead of using loops to determining if things are drawn this frame,
     * loopsSlow should be used. We don't want things to be drawn too fast
     * at high fps.
     */
    time_counter += timePerFrame;
    if (time_counter >= (1.0 / 12)) {
	loopsSlow++;
	time_counter -= (1.0 / 12);
    }
}


struct team_score {
    double	score;
    int		life;
    int		playing;
};


static void Determine_team_order(struct team_score *team_order[],
				 struct team_score team[])
{
    int i, j, k;

    num_playing_teams = 0;
    for (i = 0; i < MAX_TEAMS; i++) {
	if (team[i].playing) {
	    for (j = 0; j < num_playing_teams; j++) {
		if (team[i].score > team_order[j]->score
		    || (team[i].score == team_order[j]->score
			&& ((BIT(Setup->mode, LIMITED_LIVES))
			    ? (team[i].life > team_order[j]->life)
			    : (team[i].life < team_order[j]->life)))) {
		    for (k = i; k > j; k--)
			team_order[k] = team_order[k - 1];
		    break;
		}
	    }
	    team_order[j] = &team[i];
	    num_playing_teams++;
	}
    }
}

static void Determine_order(other_t **order, struct team_score team[])
{
    other_t		*other;
    int			i, j, k;

    for (i = 0; i < num_others; i++) {
	other = &Others[i];
	if (BIT(Setup->mode, TIMING)) {
	    /*
	     * Sort the score table on position in race.
	     * Put paused and waiting players last as well as tanks.
	     */
	    if (strchr("PTW", other->mychar))
		j = i;
	    else {
		for (j = 0; j < i; j++) {
		    if (order[j]->timing < other->timing)
			break;
		    if (strchr("PTW", order[j]->mychar))
			break;
		    if (order[j]->timing == other->timing) {
			if (order[j]->timing_loops > other->timing_loops)
			    break;
		    }
		}
	    }
	}
	else {
	    for (j = 0; j < i; j++) {
		if (order[j]->score < other->score)
		    break;
	    }
	}
	for (k = i; k > j; k--)
	    order[k] = order[k - 1];
	order[j] = other;

	if (BIT(Setup->mode, TEAM_PLAY|TIMING) == TEAM_PLAY) {
	    switch (other->mychar) {
	    case 'P':
	    case 'W':
	    case 'T':
		break;
	    case ' ':
	    case 'R':
		if (BIT(Setup->mode, LIMITED_LIVES))
		    team[other->team].life += other->life + 1;
		else
		    team[other->team].life += other->life;
		/*FALLTHROUGH*/
	    default:
		team[other->team].playing++;
		team[other->team].score += other->score;
		break;
	    }
	}
    }
    return;
}

#define TEAM_PAUSEHACK 100

static int Team_heading(int entrynum, int teamnum,
			int teamlives, double teamscore)
{
    other_t tmp;
    tmp.id = -1;
    tmp.team = teamnum;
    tmp.name_width = 0;
    tmp.ship = NULL;
    if (teamnum != TEAM_PAUSEHACK)
	sprintf(tmp.nick_name, "TEAM %d", tmp.team);
    else
	sprintf(tmp.nick_name, "Pause Wusses");
    strcpy(tmp.user_name, tmp.nick_name);
    strcpy(tmp.host_name, "");
#if 0
    if (BIT(Setup->mode, LIMITED_LIVES) && teamlives == 0)
	tmp.mychar = 'D';
    else
	tmp.mychar = ' ';
#else
    tmp.mychar = ' ';
#endif
    tmp.score = teamscore;
    tmp.life = teamlives;

    Paint_score_entry(entrynum++, &tmp, true);
    return entrynum;
}

static int Team_score_table(int entrynum, int teamnum,
			    struct team_score team, other_t **order)
{
    other_t *other;
    int i, j;
    bool drawn = false;

    for (i = 0; i < num_others; i++) {
	other = order[i];

	if (teamnum == TEAM_PAUSEHACK) {
	    if (other->mychar != 'P')
		continue;
	} else {
	    if (other->team != teamnum || other->mychar == 'P')
		continue;
	}

	if (!drawn)
	    entrynum = Team_heading(entrynum, teamnum, team.life, team.score);
	j = other - Others;
	Paint_score_entry(entrynum++, other, false);
	drawn = true;
    }

    if (drawn)
	entrynum += 1;
    return entrynum;
}


void Paint_score_table(void)
{

    struct team_score	team[MAX_TEAMS],
			pausers,
			*team_order[MAX_TEAMS];
    other_t		*other,
			**order;
    int			i, j, entrynum = 0;

    if (!scoresChanged || !players_exposed)
	return;

    if (num_others < 1) {
	Paint_score_start();
	scoresChanged = false;
	return;
    }

    if ((order = (other_t **)malloc(num_others * sizeof(other_t *))) == NULL) {
	error("No memory for score");
	return;
    }
    if (BIT(Setup->mode, TEAM_PLAY|TIMING) == TEAM_PLAY) {
	memset(&team[0], 0, sizeof team);
	memset(&pausers, 0, sizeof pausers);
    }
    Determine_order(order, team);
    Paint_score_start();
    if (!(BIT(Setup->mode, TEAM_PLAY|TIMING) == TEAM_PLAY)) {
	for (i = 0; i < num_others; i++) {
	    other = order[i];
	    j = other - Others;
	    Paint_score_entry(i, other, false);
	}
    } else {
	Determine_team_order(team_order, team);

	/* add an empty line */
	entrynum++;
	for (i = 0; i < MAX_TEAMS; i++)
	    entrynum = Team_score_table(entrynum, i, team[i], order);
	/* paint pausers */
	entrynum = Team_score_table(entrynum, TEAM_PAUSEHACK, pausers, order);
#if 0
	for (i = 0; i < num_playing_teams; i++) {
	    entrynum = Team_heading(entrynum,
				    team_order[i] - &team[0],
				    team_order[i]->life,
				    team_order[i]->score);
	}
#endif
    }

    if (roundend)
	Add_roundend_messages(order);

    free(order);

    IFWINDOWS( MarkPlayersForRedraw() );

    scoresChanged = false;
}
