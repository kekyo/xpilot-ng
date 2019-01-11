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


bool		updateScores = true;

int		playerArrayNumber;
player_t	**PlayersArray;
static int	GetIndArray[NUM_IDS + MAX_SPECTATORS + 1];

/*
 * Get index in Players array for player with id 'id'.
 */
int GetInd(int id)
{
    if (id == NO_ID)
	return NO_IND;

    /*
     * kps - in some places where we look at the id we don't
     * bother about spectators.
     * This should be cleaned up in general.
     */
    if (id < 0 || id >= NELEM(GetIndArray)) {
	/*warn("GetInd: id = %d, array size = %d\n",
	  id, NUM_IDS + MAX_SPECTATORS + 1);*/
	return NO_IND;
    }
    return GetIndArray[id];
}

/********* **********
 * Functions on player array.
 */

void Pick_startpos(player_t *pl)
{
    int ind = GetInd(pl->id), i, num_free, pick = 0, seen = 0, order, min_order = INT_MAX;
    static int prev_num_bases = 0;
    static char	*free_bases = NULL;

    if (Player_is_tank(pl)) {
	pl->home_base = Base_by_index(0);
	return;
    }

    if (prev_num_bases != Num_bases()) {
	prev_num_bases = Num_bases();
	XFREE(free_bases);
	free_bases = XMALLOC(char, Num_bases());
	if (free_bases == NULL) {
	    error("Can't allocate memory for free_bases");
	    End_game();
	}
    }

    for (i = 0; i < Num_bases(); i++) {
	if (Base_by_index(i)->team == pl->team) {
	    free_bases[i] = 1;
	} else {
	    free_bases[i] = 0;	/* other team */
	}
    }

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->id != pl->id
	    && !Player_is_tank(pl_i)
	    && pl_i->home_base
	    && free_bases[pl_i->home_base->ind]) {
	    free_bases[pl_i->home_base->ind] = 0;	/* occupado */
	}
    }

    /* find out the lowest order of all free bases */
    for (i = 0; i < Num_bases(); i++) {
	if (free_bases[i] != 0) {
	    order = Base_by_index(i)->order;
	    if (order < min_order) {
	        min_order = order;
	    }
	}
    }
	
    /* mark all bases with higher order as occupied */
    num_free = 0;
    for (i = 0; i < Num_bases(); i++) {
	if (free_bases[i] != 0) {
	    if (Base_by_index(i)->order <= min_order) {
		num_free++;
	    } else {
		free_bases[i] = 0;
	    }
	}
    }

    /* pick a random base of all bases marked free */
    pick = (int)(rfrac() * num_free);
    seen = 0;
    for (i = 0; i < Num_bases(); i++) {
	if (free_bases[i] != 0) {
	    if (seen < pick)
		seen++;
	    else
		break;
	}
    }

    if (i == Num_bases()) {
	error("Can't pick startpos (ind=%d,num=%d,free=%d,pick=%d,seen=%d)",
	      ind, Num_bases(), num_free, pick, seen);
	End_game();
    } else {
	pl->home_base = Base_by_index(i);
	if (ind < NumPlayers) {
	    for (i = 0; i < spectatorStart + NumSpectators; i++) {
		player_t *pl_i;

		if (i == NumPlayers) {
		    i = spectatorStart - 1;
		    continue;
		}
		pl_i = Player_by_index(i);
		if (pl_i->conn != NULL)
		    Send_base(pl_i->conn, pl->id, pl->home_base->ind);
	    }
	    if (Player_is_paused(pl)
		|| Player_is_waiting(pl)
		|| Player_is_dead(pl))
		Go_home(pl);
	}
    }
}

void Go_home(player_t *pl)
{
    int ind = GetInd(pl->id), i, dir, check;
    double vx, vy, velo;
    clpos_t pos, initpos;

    if (Player_is_tank(pl)) {
	/*NOTREACHED*/
	/* Tanks have no homebase. */
	warn("BUG: gohome tank");
	return;
    }

    if (BIT(world->rules->mode, TIMING)
	&& pl->round
	&& !(Player_is_waiting(pl)
	     || Player_is_dead(pl))) {
	if (pl->check)
	    check = pl->check - 1;
	else
	    check = world->NumChecks - 1;
	pos = Check_by_index(check)->pos;
	vx = (rfrac() - 0.5) * 0.1;
	vy = (rfrac() - 0.5) * 0.1;
	velo = LENGTH(vx, vy);
	dir = pl->last_check_dir;
	dir = MOD2(dir + (int)((rfrac() - 0.5) * (RES / 8)), RES);
    } else if (pl->home_base != NULL) {
	pos = pl->home_base->pos;
	dir = pl->home_base->dir;
	vx = vy = velo = 0;
    } else {
	pos.cx = pos.cy = dir = 0;
	vx = vy = velo = 0.0;
    }

    pl->dir = dir;
    Player_set_float_dir(pl, (double)dir);
    pl->wanted_float_dir = pl->float_dir;/*TURNQUEUE*/
    initpos.cx = (click_t)(pos.cx + CLICK * vx);
    initpos.cy = (click_t)(pos.cy + CLICK * vy);
    Object_position_init_clpos(OBJ_PTR(pl), initpos);
    pl->vel.x = vx;
    pl->vel.y = vy;
    pl->velocity = velo;
    pl->acc.x = pl->acc.y = 0.0;
    pl->turnacc = pl->turnvel = 0.0;
    memset(pl->last_keyv, 0, sizeof(pl->last_keyv));
    memset(pl->prev_keyv, 0, sizeof(pl->prev_keyv));
    Emergency_shield(pl, false);
    Player_used_kill(pl);
    /*Player_init_items(pl);*/

    if (options.playerStartsShielded) {
	SET_BIT(pl->used, HAS_SHIELD);
	if (!options.allowShields) {
	    pl->shield_time = SHIELD_TIME;
	    SET_BIT(pl->have, HAS_SHIELD);
	}
	if (Player_has_deflector(pl))
	    Deflector(pl, true);
    }
    Thrust(pl, false);
    pl->updateVisibility = true;
    for (i = 0; i < NumPlayers; i++) {
	pl->visibility[i].lastChange = 0;
	Player_by_index(i)->visibility[ind].lastChange = 0;
    }

    if (Player_is_robot(pl))
	Robot_go_home(pl);
}

void Base_set_option(base_t *base, const char *name, const char *value)
{
    Item_t item;

    item = Item_by_option_name(name);
    if (item != NO_ITEM) {
	base->initial_items[item] = atoi(value);
	return;
    }

    warn("This server doesn't support option %s for bases.", name);
}

/*
 * Compute the current sensor range for player 'pl'.  This is based on the
 * amount of fuel, the number of sensor items (each one adds 25%), and the
 * minimum and maximum visibility limits in effect.
 */
void Compute_sensor_range(player_t *pl)
{
    static int init = 0;
    static double EnergyRangeFactor;

    if (!init) {
	if (options.minVisibilityDistance <= 0.0)
	    options.minVisibilityDistance = VISIBILITY_DISTANCE;
	else
	    options.minVisibilityDistance *= BLOCK_SZ;
	if (options.maxVisibilityDistance <= 0.0)
	    options.maxVisibilityDistance = world->hypotenuse;
	else
	    options.maxVisibilityDistance *= BLOCK_SZ;

	if (world->items[ITEM_FUEL].initial > 0.0) {
	    EnergyRangeFactor = options.minVisibilityDistance /
		(world->items[ITEM_FUEL].initial
		 * (1.0 + ((double)world->items[ITEM_SENSOR].initial * 0.25)));
	} else
	    EnergyRangeFactor = ENERGY_RANGE_FACTOR;
	init = 1;
    }

    pl->sensor_range = pl->fuel.sum * EnergyRangeFactor;
    pl->sensor_range *= (1.0 + ((double)pl->item[ITEM_SENSOR] * 0.25));
    LIMIT(pl->sensor_range,
	  options.minVisibilityDistance, options.maxVisibilityDistance);
}

/*
 * Give ship one more tank, if possible.
 */
void Player_add_tank(player_t *pl, double tank_fuel)
{
    double tank_cap, add_fuel;

    if (pl->fuel.num_tanks < MAX_TANKS) {
	pl->fuel.num_tanks++;
	tank_cap = TANK_CAP(pl->fuel.num_tanks);
	add_fuel = tank_fuel;
	LIMIT(add_fuel, 0.0, tank_cap);
	pl->fuel.sum += add_fuel;
	pl->fuel.max += tank_cap;
	pl->fuel.tank[pl->fuel.num_tanks] = add_fuel;
	pl->emptymass += TANK_MASS;
	pl->item[ITEM_TANK] = pl->fuel.num_tanks;
    }
}

/*
 * Remove a tank from a ship, if possible.
 */
void Player_remove_tank(player_t *pl, int which_tank)
{
    int i, tank_ind;
    double tank_fuel, tank_cap;

    if (pl->fuel.num_tanks > 0) {
	tank_ind = which_tank;
	LIMIT(tank_ind, 1, pl->fuel.num_tanks);
	pl->emptymass -= TANK_MASS;
	tank_fuel = pl->fuel.tank[tank_ind];
	tank_cap = TANK_CAP(tank_ind);
	pl->fuel.max -= tank_cap;
	pl->fuel.sum -= tank_fuel;
	pl->fuel.num_tanks--;
	if (pl->fuel.current > pl->fuel.num_tanks)
	    pl->fuel.current = 0;
	else {
	    for (i = tank_ind; i <= pl->fuel.num_tanks; i++)
		pl->fuel.tank[i] = pl->fuel.tank[i + 1];
	}
	pl->item[ITEM_TANK] = pl->fuel.num_tanks;
    }
}

void Player_hit_armor(player_t *pl)
{
    if (--pl->item[ITEM_ARMOR] <= 0)
	CLR_BIT(pl->have, HAS_ARMOR);
}

/*
 * Clear used bits.
 */
void Player_used_kill(player_t *pl)
{
    pl->used &= ~USED_KILL;
    if (!BIT(DEF_HAVE, HAS_SHIELD))
	CLR_BIT(pl->have, HAS_SHIELD);
    pl->used |= DEF_USED;
}

/*
 * Calculate the mass of a player.
 */
void Player_set_mass(player_t *pl)
{
    double sum_item_mass = 0.0, item_mass;
    int item;

    for (item = 0; item < NUM_ITEMS; item++) {
	switch (item) {
	case ITEM_FUEL:
	case ITEM_TANK:
	    item_mass = 0.0;
	    break;
	case ITEM_ARMOR:
	    item_mass = pl->item[ITEM_ARMOR] * ARMOR_MASS;
	    break;
	default:
	    item_mass = pl->item[item] * options.minItemMass;
	    break;
	}
	sum_item_mass += item_mass;
    }

    pl->mass = pl->emptymass
	+ FUEL_MASS(pl->fuel.sum)
	+ sum_item_mass;
}

/*
 * Give player the initial number of tanks and amount of fuel.
 * Upto the maximum allowed.
 */
static void Player_init_fuel(player_t *pl, double total_fuel)
{
    double fuel = total_fuel;
    int i;

    pl->fuel.num_tanks  = 0;
    pl->fuel.current    = 0;
    pl->fuel.max	= TANK_CAP(0);
    pl->fuel.sum	= MIN(fuel, pl->fuel.max);
    pl->fuel.tank[0]	= pl->fuel.sum;
    pl->emptymass	= options.shipMass;
    pl->item[ITEM_TANK]	= pl->fuel.num_tanks;

    fuel -= pl->fuel.sum;

    for (i = 1; i <= world->items[ITEM_TANK].initial; i++) {
	Player_add_tank(pl, fuel);
	fuel -= pl->fuel.tank[i];
    }
}

#if 0
/*
 * Set initial items for a player.
 * Number of initial items can depend on which base the player starts from.
 */
void Player_init_items(player_t *pl)
{
    int i, num_tanks;
    double total_fuel;
    base_t *base = pl->home_base;

    for (i = 0; i < NUM_ITEMS; i++) {
	if (i == ITEM_FUEL || i == ITEM_TANK))
	    continue;

	if (base && base->initial_items[i] >= 0)
	    pl->item[i] = base->initial_items[i];
	else
	    pl->item[i] = world->items[i].initial;
    }

    if (base && base->initial_items[ITEM_TANK] >= 0)
	num_tanks = base->initial_items[ITEM_TANK];
    else
	num_tanks = world->items[ITEM_TANK].initial;

    if (base && base->initial_items[ITEM_FUEL] >= 0)
	total_fuel = (double)base->initial_items[ITEM_FUEL];
    else
	total_fuel = (double)world->items[ITEM_FUEL].initial;

    Player_init_fuel(pl, num_tanks, total_fuel);
}
#endif

void Player_init_items(player_t *pl)
{
    int i;

    /*
     * Give player an initial set of items.
     */
    for (i = 0; i < NUM_ITEMS; i++) {
	if (i == ITEM_FUEL || i == ITEM_TANK)
	    pl->item[i] = 0;
	else
	    pl->item[i] = world->items[i].initial;
    }

    Player_init_fuel(pl, (double)world->items[ITEM_FUEL].initial);

    /*
     * Remember the amount of initial items. This way we can
     * later figure out what items the player has picked up.
     */
    for (i = 0; i < NUM_ITEMS; i++)
	pl->initial_item[i] = pl->item[i];
}

int Init_player(int ind, shipshape_t *ship, int type)
{
    player_t *pl = Player_by_index(ind);
    visibility_t *v = pl->visibility;
    int i;

    memset(pl, 0, sizeof(player_t));
    pl->visibility = v;

    /*
     * Make sure floats, doubles and pointers are correctly zeroed.
     */
    assert(pl->wall_time == 0);
    assert(pl->turnspeed == 0);
    assert(pl->conn  == NULL);

    pl->dir = DIR_UP;
    Player_set_float_dir(pl, (double)pl->dir);

    pl->mass = options.shipMass;
    pl->emptymass = options.shipMass;

    Player_init_items(pl);

    if (options.allowShipShapes && ship)
	pl->ship = ship;
    else {
	shipshape_t *tryship = Parse_shape_str(options.defaultShipShape);

	if (tryship)
	    pl->ship = tryship;
	else
	    pl->ship = Default_ship();
    }

    pl->power = pl->power_s = MAX_PLAYER_POWER;
    pl->turnspeed = pl->turnspeed_s = MIN_PLAYER_TURNSPEED;
    pl->type = OBJ_PLAYER;
    pl->pl_type = type;
    if (type == PL_TYPE_HUMAN)
	pl->pl_type_mychar = ' ';
    else if (type == PL_TYPE_ROBOT)
	pl->pl_type_mychar = 'R';
    else if (type == PL_TYPE_TANK)
	pl->pl_type_mychar = 'T';

    /*Player_init_items(pl);*/
    Compute_sensor_range(pl);

    pl->obj_status = GRAVITY;
    assert(pl->pl_status == 0);
    assert(pl->pl_state == PL_STATE_UNDEFINED);
    Player_set_state(pl, PL_STATE_ALIVE);
    pl->have = DEF_HAVE;
    pl->used = DEF_USED;

    if (pl->item[ITEM_CLOAK] > 0)
	SET_BIT(pl->have, HAS_CLOAKING_DEVICE);

    Mods_clear(&pl->mods);
    for (i = 0; i < NUM_MODBANKS; i++)
	Mods_clear(&pl->modbank[i]);

    for (i = 0; i < LOCKBANK_MAX; i++)
	pl->lockbank[i] = NO_ID;

    {
	static unsigned short	pseudo_team_no = 0;

	pl->pseudo_team = pseudo_team_no++;
    }
    pl->survival_time=0;
    Player_set_life(pl, world->rules->lives);

    pl->player_fps = 50; /* Client should send a value after startup */
    pl->maxturnsps = MAX_SERVER_FPS;

    pl->kills = 0;
    pl->deaths = 0;

    /*
     * If limited lives you will have to wait 'til everyone gets GAME OVER.
     *
     * Indeed you have to! (Mara)
     *
     * At least don't make the player wait for a new round if he's the
     * only one on the server. Mara's change (always too_late) meant
     * there was a round reset when the first player joined. -uau
     *
     * In individual games, make the new players appear after a small delay.
     */
    if (NumPlayers > 0
	&& !Player_is_tank(pl)) {
	if (BIT(world->rules->mode, LIMITED_LIVES))
	    Player_set_state(pl, PL_STATE_WAITING);
	else
	    Player_set_state(pl, PL_STATE_APPEARING);
    }

    pl->team		= TEAM_NOT_SET;

    pl->alliance	= ALLIANCE_NOT_SET;
    pl->invite		= NO_ID;

    pl->lock.tagged	= LOCK_NONE;
    pl->lock.pl_id	= 0; /* kps - ??? */

    pl->id		= peek_ID();
    GetIndArray[pl->id]	= ind;
    if (!Is_player_id(pl->id))
	warn("Init_player: Not a player id: %d", pl->id);

    for (i = 0; i < MAX_RECORDED_SHOVES; i++)
	pl->shove_record[i].pusher_id = NO_ID;
	
    pl->update_score = true;

    return pl->id;
}


static player_t *playerArray;
static visibility_t *visibilityArray;

void Alloc_players(int number)
{
    player_t *p;
    visibility_t *t;
    size_t n = number;
    int i;

    /* Allocate space for pointers */
    PlayersArray = XCALLOC(player_t *, n);

    /* Allocate space for all entries, all player structs */
    p = playerArray = XCALLOC(player_t, n);

    /* Allocate space for all visibility arrays, n arrays of n entries */
    t = visibilityArray = XCALLOC(visibility_t, n * n);

    if (!PlayersArray || !playerArray || !visibilityArray) {
	error("Not enough memory for Players.");
	exit(1);
    }

    for (i = 0; i < number; i++) {
	PlayersArray[i] = p++;
	PlayersArray[i]->visibility = t;
	/* Advance to next block/array */
	t += number;
    }

    playerArrayNumber = number;

    /* Initialize player id to index lookup table */
    for (i = 0; i < NELEM(GetIndArray); i++)
	GetIndArray[i] = NO_IND;
}



void Free_players(void)
{
    XFREE(PlayersArray);
    XFREE(playerArray);
    XFREE(visibilityArray);
}



void Update_score_table(void)
{
    int i, j, check;
    player_t *pl;

    for (j = 0; j < NumPlayers; j++) {
	pl = Player_by_index(j);
	if (pl->update_score) {
	    pl->update_score = false;
	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (pl_i->conn != NULL)
		    Send_score(pl_i->conn, pl->id,  Get_Score(pl), pl->pl_life,
			       pl->mychar, pl->alliance);
	    }
	    for (i = 0; i < NumSpectators; i++)
		Send_score(Player_by_index(i + spectatorStart)->conn, pl->id,
			    Get_Score(pl), pl->pl_life, pl->mychar, pl->alliance);
	}
	if (BIT(world->rules->mode, TIMING)) {
	    if (pl->check != pl->prev_check
		|| pl->round != pl->prev_round) {
		pl->prev_check = pl->check;
		pl->prev_round = pl->round;
		check = (pl->round == 0)
			    ? 0
			    : (pl->check == 0)
				? (world->NumChecks - 1)
				: (pl->check - 1);
		for (i = 0; i < NumPlayers; i++) {
		    player_t *pl_i = Player_by_index(i);

		    if (pl_i->conn != NULL)
			Send_timing(pl_i->conn, pl->id, check, pl->round);
		}
	    }
	}
    }
    updateScores = false;
}


void Reset_all_players(void)
{
    player_t *pl;
    int i, j;

    updateScores = true;

    for (i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);

	if (options.endOfRoundReset) {
	    if (Player_is_paused(pl))
		Player_death_reset(pl, false);
	    else {
		Kill_player(pl, false);
		if (pl != Player_by_index(i)) {
		    i--;
		    continue;
		}
	    }
	}

	pl->kills = 0;
	pl->deaths = 0;

	if (!Player_is_paused(pl)
	    && !Player_is_waiting(pl))
	    Rank_add_round(pl);

	CLR_BIT(pl->have, HAS_BALL);
	Player_reset_timing(pl);

	if (!Player_is_paused(pl)) {
	    Player_set_state(pl, PL_STATE_APPEARING);
	    Player_set_life(pl, world->rules->lives);
	}
    }

    if (BIT(world->rules->mode, TEAM_PLAY)) {
	/* Detach any balls and kill ball */
	/* We are starting all over again */
	for (j = NumObjs - 1; j >= 0 ; j--) {
	    if (Obj[j]->type == OBJ_BALL) {
		ballobject_t *ball = BALL_IND(j);

		ball->id = NO_ID;
		ball->life = 0;
		/*
		 * why not -1 ???
		 * naive question, obviously yet another dirty hack
		 */
		ball->ball_owner = 0;
		CLR_BIT(ball->obj_status, RECREATE);
		Delete_shot(j);
	    }
	}

	/* Reset the treasures */
	for (i = 0; i < Num_treasures(); i++) {
	    treasure_t *treasure = Treasure_by_index(i);

	    treasure->destroyed = 0;
	    treasure->have = false;
	    Make_treasure_ball(treasure);
	}

	/* Reset the teams */
	for (i = 0; i < MAX_TEAMS; i++) {
	    team_t *teamp = Team_by_index(i);

	    teamp->TreasuresDestroyed = 0;
	    teamp->TreasuresLeft
		= teamp->NumTreasures - teamp->NumEmptyTreasures;
	}

	if (options.endOfRoundReset) {
	    /* Reset the targets */
	    for (i = 0; i < Num_targets(); i++) {
		target_t *targ = Target_by_index(i);

		if (targ->damage != TARGET_DAMAGE || targ->dead_ticks > 0)
		    World_restore_target(targ);
	    }
	}
    }

    if (options.endOfRoundReset) {
	for (i = 0; i < NumObjs; i++) {
	    object_t *obj = Obj[i];

	    if (BIT(OBJ_TYPEBIT(obj->type),
		    OBJ_SHOT_BIT|OBJ_MINE_BIT|OBJ_DEBRIS_BIT|OBJ_SPARK_BIT
		    |OBJ_CANNON_SHOT_BIT|OBJ_TORPEDO_BIT|OBJ_SMART_SHOT_BIT
		    |OBJ_HEAT_SHOT_BIT|OBJ_PULSE_BIT|OBJ_ITEM_BIT)) {
		obj->life = 0;
		if (BIT(OBJ_TYPEBIT(obj->type),
			OBJ_TORPEDO_BIT|OBJ_SMART_SHOT_BIT|OBJ_HEAT_SHOT_BIT
			|OBJ_CANNON_SHOT_BIT|OBJ_MINE_BIT))
		    /* Take care that no new explosions are made. */
		    obj->mass = 0;
	    }
	}
    }

    roundtime = options.maxRoundTime * FPS;

    Update_score_table();
}


void Check_team_members(int team)
{
    player_t *pl;
    team_t *teamp;
    int members, i;

    if (!BIT(world->rules->mode, TEAM_PLAY))
	return;

    for (members = i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);
	if (!Player_is_tank(pl)
	    && pl->team == team
	    && pl->home_base != NULL)
	    members++;
    }
    teamp = Team_by_index(team);
    if (teamp->NumMembers != members) {
	warn("Server has reset team %d members from %d to %d",
	     team, teamp->NumMembers, members);
	for (i = 0; i < NumPlayers; i++) {
	    pl = Player_by_index(i);
	    if (!Player_is_tank(pl)
		&& pl->team == team
		&& pl->home_base != NULL)
		warn("Team %d currently has player %d: \"%s\"",
		     team, i+1, pl->name);
	}
	teamp->NumMembers = members;
    }
}


static void Compute_end_of_round_values(double *average_score,
					int *num_best_players,
					double *best_ratio,
					int best_players[])
{
    int i, n = 0;
    double ratio;

    /* Initialize everything */
    *average_score = 0;
    *num_best_players = 0;
    *best_ratio = -1.0;

    /* Figure out what the average score is and who has the best kill/death */
    /* ratio for this round */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl = Player_by_index(i);

	if (Player_is_tank(pl)
	    || (Player_is_paused(pl) && pl->pause_count <= 0)
	    || Player_is_waiting(pl))
	    continue;

	n++;
	*average_score +=  Get_Score(pl);
	ratio = (double) pl->kills / (pl->deaths + 1);
	if (ratio > *best_ratio) {
	    *best_ratio = ratio;
	    best_players[0] = i;
	    *num_best_players = 1;
	} else if (ratio == *best_ratio)
	    best_players[(*num_best_players)++] = i;
    }
    if (n != 0)  /* Can this be 0? */
	*average_score /= n;
}


static void Give_best_player_bonus(double average_score,
				   int num_best_players,
				   double best_ratio,
				   int best_players[])
{
    int i;
    double points;
    char msg[MSG_LEN];

    if (num_best_players == 0 || best_ratio == 0)
	sprintf(msg, "There is no Deadly Player.");
    else if (num_best_players == 1) {
	player_t *bp = Player_by_index(best_players[0]);

	sprintf(msg,
		"%s is the Deadliest Player with a kill ratio of %d/%d.",
		bp->name,
		bp->kills, bp->deaths);
	points = best_ratio * Rate(Get_Score(bp), average_score);
	if (!options.zeroSumScoring) Score(bp, points, bp->pos, "[Deadliest]");
    	Rank_add_deadliest(bp);
	/*if (options.zeroSumScoring);*//* TODO */
    } else {
	msg[0] = '\0';
	for (i = 0; i < num_best_players; i++) {
	    player_t	*bp = Player_by_index(best_players[i]);
	    double	ratio = Rate(Get_Score(bp), average_score);
	    double	score = (ratio + num_best_players) / num_best_players;

	    if (msg[0]) {
		if (i == num_best_players - 1)
		    strcat(msg, " and ");
		else
		    strcat(msg, ", ");
	    }
	    if (strlen(msg) + 8 + strlen(bp->name) >= sizeof(msg)) {
		Set_message(msg);
		msg[0] = '\0';
	    }
	    strcat(msg, bp->name);
	    points = best_ratio * score;
	    if (!options.zeroSumScoring) Score(bp, points, bp->pos, "[Deadly]");
	    Rank_add_deadliest(bp);
	    /*if (options.zeroSumScoring);*//* TODO */
	}
	if (strlen(msg) + 64 >= sizeof(msg)) {
	    Set_message(msg);
	    msg[0] = '\0';
	}
	sprintf(msg + strlen(msg),
		" are the Deadly Players with kill ratios of %d/%d.",
		Player_by_index(best_players[0])->kills,
		Player_by_index(best_players[0])->deaths);
    }
    Set_message(msg);
}

static void Give_individual_bonus(player_t *pl, double average_score)
{
    double ratio, points;

    ratio = (double) pl->kills / (pl->deaths + 1);
    points = ratio * Rate( Get_Score(pl), average_score);
    if (!options.zeroSumScoring) Score(pl, points, pl->pos, "[Winner]");
    /*if (options.zeroSumScoring);*//* TODO */
}

void Count_rounds(void)
{
    if (!options.roundsToPlay)
	return;

    ++roundsPlayed;

    Set_message_f(" < Round %d out of %d completed. >",
		  roundsPlayed, options.roundsToPlay);
    /* only do the game over once */
    if (roundsPlayed == options.roundsToPlay)
	Game_Over();
}


void Team_game_over(int winning_team, const char *reason)
{
    int i, j, num_best_players, *best_players;
    double average_score, best_ratio;

    if (!(best_players = XMALLOC(int, NumPlayers))) {
	warn("no mem");
	End_game();
    }

    /* Figure out the average score and who has the best kill/death ratio */
    /* ratio for this round */
    Compute_end_of_round_values(&average_score,
				&num_best_players,
				&best_ratio,
				best_players);

    /* Print out the results of the round */
    if (winning_team != -1) {
	Set_message_f(" < Team %d has won the round%s! >",
		      winning_team, reason);
	sound_play_all(TEAM_WIN_SOUND);
    } else {
	Set_message_f(" < We have a draw%s! >", reason);
	sound_play_all(TEAM_DRAW_SOUND);
    }

    /* Give bonus to the best player */
    Give_best_player_bonus(average_score,
			   num_best_players,
			   best_ratio,
			   best_players);

    /* Give bonuses to the winning team */
    if (winning_team != -1) {
	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl_i = Player_by_index(i);

	    if (pl_i->team != winning_team)
		continue;

	    if (Player_is_tank(pl_i)
		|| (Player_is_paused(pl_i) && pl_i->pause_count <= 0)
		|| Player_is_waiting(pl_i))
		continue;

	    for (j = 0; j < num_best_players; j++) {
		if (i == best_players[j])
		    break;
	    }
	    if (j == num_best_players)
		Give_individual_bonus(pl_i, average_score);
	}
    }

    teamcup_round_end(winning_team);

    Reset_all_players();

    Count_rounds();

    free(best_players);

    teamcup_round_start();

    /* Ranking */
    Rank_write_webpage();
    Rank_write_rankfile();
    Rank_show_ranks();
}

void Individual_game_over(int winner)
{
    int i, j, num_best_players, *best_players;
    double average_score, best_ratio;

    if (!(best_players = XMALLOC(int, NumPlayers))) {
	warn("no mem");
	End_game();
    }

    /* Figure out what the average score is and who has the best kill/death */
    /* ratio for this round */
    Compute_end_of_round_values(&average_score, &num_best_players,
				&best_ratio, best_players);

    /* Print out the results of the round */
    if (winner == -1) {
	Set_message(" < We have a draw! >");
	sound_play_all(PLAYER_DRAW_SOUND);
    }
    else if (winner == -2) {
	Set_message(" < The robots have won the round! >");
	/* Perhaps this should be a different sound? */
	sound_play_all(PLAYER_WIN_SOUND);
    } else {
	Set_message_f(" < %s has won the round! >",
		      Player_by_index(winner)->name);
	sound_play_all(PLAYER_WIN_SOUND);
    }

    /* Give bonus to the best player */
    Give_best_player_bonus(average_score,
			   num_best_players,
			   best_ratio,
			   best_players);

    /* Give bonus to the winning player */
    if (winner >= 0) {
	for (i = 0; i < num_best_players; i++) {
	    if (winner == best_players[i])
		break;
	}
	if (i == num_best_players)
	    Give_individual_bonus(Player_by_index(winner), average_score);
    }
    else if (winner == -2) {
	for (j = 0; j < NumPlayers; j++) {
	    player_t *pl_j = Player_by_index(j);

	    if (Player_is_robot(pl_j)) {
		for (i = 0; i < num_best_players; i++) {
		    if (j == best_players[i])
			break;
		}
		if (i == num_best_players)
		    Give_individual_bonus(pl_j, average_score);
	    }
	}
    }

    Reset_all_players();

    Count_rounds();

    free(best_players);
}

void Compute_game_status(void)
{
    int i;
    char msg[MSG_LEN];

    if (roundtime > 0)
	roundtime--;

    if (BIT(world->rules->mode, TIMING))
	Race_compute_game_status();
    else if (BIT(world->rules->mode, TEAM_PLAY)) {

	/* Do we have a winning team ? */

	enum TeamState {
	    TeamEmpty,
	    TeamDead,
	    TeamAlive
	} team_state[MAX_TEAMS];
	int num_dead_teams = 0;
	int num_alive_teams = 0;
	int winning_team = -1;

	for (i = 0; i < MAX_TEAMS; i++)
	    team_state[i] = TeamEmpty;

	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl_i = Player_by_index(i);

	    if (Player_is_tank(pl_i))
		/* Ignore tanks. */
		continue;
	    else if (Player_is_paused(pl_i))
		/* Ignore paused players. */
		continue;
#if 0
	    /* not all teammode maps have treasures. */
	    else if (world->teams[pl_i->team].NumTreasures == 0)
		/* Ignore players with no treasure troves */
		continue;
#endif
	    else if (Player_is_waiting(pl_i)
		     || Player_is_dead(pl_i)) {
		if (team_state[pl_i->team] == TeamEmpty) {
		    /* Assume all teammembers are dead. */
		    num_dead_teams++;
		    team_state[pl_i->team] = TeamDead;
		}
	    }
	    /*
	     * If the player is not paused and he is not in the
	     * game over mode and his team owns treasures then he is
	     * considered alive.
	     * But he may not be playing though if the rest of the team
	     * was genocided very quickly after game reset, while this
	     * player was still being transported back to his homebase.
	     */
	    else if (team_state[pl_i->team] != TeamAlive) {
		if (team_state[pl_i->team] == TeamDead)
		    /* Oops!  Not all teammembers are dead yet. */
		    num_dead_teams--;
		team_state[pl_i->team] = TeamAlive;
		++num_alive_teams;
		/* Remember a team which was alive. */
		winning_team = pl_i->team;
	    }
	}

	if (num_alive_teams > 1) {
	    char *bp;
	    int teams_with_treasure = 0, team_win[MAX_TEAMS];
	    double team_score[MAX_TEAMS], max_score = 0;
	    int winners, max_destroyed = 0, max_left = 0;
	    team_t *team_ptr, *specialballteam_ptr;
	    bool no_special_balls_present = false;
	    /*
	     * Game is not over if more than one team which have treasures
	     * still have one remaining in play.  Note that it is possible
	     * for max_destroyed to be zero, in the case where a team
	     * destroys some treasures and then all quit, and the remaining
	     * teams did not destroy any.
	     */
	    for (i = 0; i < MAX_TEAMS; i++) {
		team_score[i] = 0;
		if ((team_state[i] != TeamAlive) && (i != options.specialBallTeam)) {
		    team_win[i] = 0;
		    continue;
		}

		team_win[i] = 1;
		team_ptr = &(world->teams[i]);
		specialballteam_ptr = Team_by_index(options.specialBallTeam);
		
		if (options.specialBallTeam < 0 || options.specialBallTeam >=MAX_TEAMS ||
		    specialballteam_ptr->NumTreasures == 0)
		  no_special_balls_present = true;
		
		if (team_ptr->TreasuresDestroyed > max_destroyed)
		  max_destroyed = team_ptr->TreasuresDestroyed;
		if ((team_ptr->TreasuresLeft > 0) ||
		    ((team_ptr->NumTreasures == team_ptr->NumEmptyTreasures) &&
		     no_special_balls_present))
		  teams_with_treasure++;
	    }

	    /*
	     * Game is not over if more than one team has treasure.
	     */
	    if ((teams_with_treasure > 1 || !max_destroyed)
		&& (roundtime != 0 || options.maxRoundTime <= 0))
		return;

	    if (options.maxRoundTime > 0 && roundtime == 0)
		Set_message("Timer expired. Round ends now.");

	    /*
	     * Find the winning team;
	     *	Team destroying most number of treasures;
	     *	If drawn; the one with most saved treasures,
	     *	If drawn; the team with the most points,
	     *	If drawn; an overall draw.
	     */
	    for (winners = i = 0; i < MAX_TEAMS; i++) {
		if (!team_win[i])
		    continue;
		if (world->teams[i].TreasuresDestroyed == max_destroyed) {
		    if (world->teams[i].TreasuresLeft > max_left)
			max_left = world->teams[i].TreasuresLeft;
		    winning_team = i;
		    winners++;
		} else
		    team_win[i] = 0;
	    }
	    if (winners == 1) {
		sprintf(msg, " by destroying %d treasures", max_destroyed);
		Team_game_over(winning_team, msg);
		return;
	    }

	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (Player_is_paused(pl_i) || Player_is_tank(pl_i))
		    continue;
		team_score[pl_i->team] += Get_Score(pl_i);
	    }

	    for (winners = i = 0; i < MAX_TEAMS; i++) {
		if (!team_win[i])
		    continue;
		if (world->teams[i].TreasuresLeft == max_left) {
		    if (team_score[i] > max_score)
			max_score = team_score[i];
		    winning_team = i;
		    winners++;
		} else
		    team_win[i] = 0;
	    }
	    if (winners == 1) {
		sprintf(msg,
			" by destroying %d treasures"
			" and successfully defending %d",
			max_destroyed, max_left);
		Team_game_over(winning_team, msg);
		return;
	    }

	    for (winners = i = 0; i < MAX_TEAMS; i++) {
		if (!team_win[i])
		    continue;
		if (team_score[i] == max_score) {
		    winning_team = i;
		    winners++;
		} else
		    team_win[i] = 0;
	    }
	    if (winners == 1) {
		sprintf(msg, " by destroying %d treasures, saving %d, and "
			"scoring %.2f points",
			max_destroyed, max_left, max_score);
		Team_game_over(winning_team, msg);
		return;
	    }

	    /* Highly unlikely */

	    sprintf(msg, " between teams ");
	    bp = msg + strlen(msg);
	    for (i = 0; i < MAX_TEAMS; i++) {
		if (!team_win[i])
		    continue;
		*bp++ = "0123456789"[i]; *bp++ = ','; *bp++ = ' ';
	    }
	    bp -= 2;
	    *bp = '\0';
	    Team_game_over(-1, msg);

	}
	else if (num_dead_teams > 0) {
	    if (num_alive_teams == 1)
		Team_game_over(winning_team, " by staying alive");
	    else
		Team_game_over(-1, " as everyone died");
	}
	else {
	    /*
	     * num_alive_teams <= 1 && num_dead_teams == 0
	     *
	     * There is a possibility that the game has ended because players
	     * quit, the game over state is needed to reset treasures.  We
	     * must count how many treasures are missing, if there are any
	     * the playing team (if any) wins.
	     */
	    int	j, treasures_destroyed;

	    for (treasures_destroyed = j = 0; j < MAX_TEAMS; j++)
		treasures_destroyed += (world->teams[j].NumTreasures
					- world->teams[j].NumEmptyTreasures
					- world->teams[j].TreasuresLeft);
	    if (treasures_destroyed)
		Team_game_over(winning_team, " by staying in the game");
	}

    } else {

	/* Do we have a winner ? (No team play) */
	int num_alive_players = 0;
	int num_active_players = 0;
	int num_alive_robots = 0;
	int num_active_humans = 0;
	int winner = -1;

	for (i = 0; i < NumPlayers; i++)  {
	    player_t *pl_i = Player_by_index(i);

	    if (Player_is_paused(pl_i) || Player_is_tank(pl_i))
		continue;
	    if (!(Player_is_waiting(pl_i)
		  || Player_is_dead(pl_i))) {
		num_alive_players++;
		if (Player_is_robot(pl_i))
		    num_alive_robots++;
		winner = i; 	/* Tag player that's alive */
	    }
	    else if (Player_is_human(pl_i))
		num_active_humans++;
	    num_active_players++;
	}

	if (num_alive_players == 1 && num_active_players > 1)
	    Individual_game_over(winner);
	else if (num_alive_players == 0 && num_active_players >= 1)
	    Individual_game_over(-1);
	else if (num_alive_robots > 1
		    && num_alive_players == num_alive_robots
		    && num_active_humans > 0)
	    Individual_game_over(-2);
	else if (options.maxRoundTime > 0 && roundtime == 0) {
	    Set_message("Timer expired. Round ends now.");
	    Individual_game_over(-1);
	}
    }
}

void Delete_player(player_t *pl)
{
    int ind = GetInd(pl->id), i, j, id = pl->id;
    object_t *obj;
    team_t *teamp = Team_by_index(pl->team);

    /* call before important player structures are destroyed */
    Leave_alliance(pl);

    if (options.tagGame && tagItPlayerId == pl->id)
	tagItPlayerId = NO_ID;

    if (Player_is_robot(pl))
	Robot_destroy(pl);

    if (pl->isoperator) {
	if (!--NumOperators && game_lock) {
	    game_lock = false;
	    Set_message(" < The game has been unlocked as "
			"the last operator left! >");
	}
    }

    /* Won't be swapping anywhere */
    for (i = MAX_TEAMS - 1; i >= 0; i--)
	if (world->teams[i].SwapperId == id)
	    world->teams[i].SwapperId = -1;

    /* Delete remaining shots */
    for (i = NumObjs - 1; i >= 0; i--) {
	obj = Obj[i];
	if (obj->id == id) {
	    if (obj->type == OBJ_BALL) {
		Delete_shot(i);
		BALL_PTR(obj)->ball_owner = NO_ID;
	    }
	    else if (obj->type == OBJ_DEBRIS
		     || obj->type == OBJ_SPARK)
		/* Okay, so you want robot explosions to exist,
		 * even if the robot left the game. */
		obj->id = NO_ID;
	    else {
		if (!options.keepShots) {
		    obj->life = 0;
		    if (BIT(OBJ_TYPEBIT(obj->type),
			    OBJ_CANNON_SHOT_BIT|OBJ_MINE_BIT|OBJ_SMART_SHOT_BIT
			    |OBJ_HEAT_SHOT_BIT|OBJ_TORPEDO_BIT))
			obj->mass = 0;
		}
	        obj->id = NO_ID;
		if (obj->type == OBJ_MINE)
		    MINE_PTR(obj)->mine_owner = NO_ID;
	    }
	}
	else {
	    if (obj->type == OBJ_MINE) {
		mineobject_t *mine = MINE_PTR(obj);

		if (mine->mine_owner == id) {
		    mine->mine_owner = NO_ID;
		    if (!options.keepShots) {
			obj->life = 0;
			obj->mass = 0;
		    }
		}
	    }
	    else if (obj->type == OBJ_BALL) {
		ballobject_t *ball = BALL_PTR(obj);

		if (ball->ball_owner == id)
		    ball->ball_owner = NO_ID;
	    }
	}
    }

    Free_ship_shape(pl->ship);

    sound_close(pl);

    NumPlayers--;
    if (Player_is_tank(pl))
	NumPseudoPlayers--;

    if (pl->rank) {
	Rank_save_score(pl);
   	if (NumPlayers == NumRobots + NumPseudoPlayers) {
	    Rank_write_webpage();
	    Rank_write_rankfile();
   	}
    }

    if (teamp && !Player_is_tank(pl) && pl->home_base) {
	teamp->NumMembers--;
	if (Player_is_robot(pl))
	    teamp->NumRobots--;
    }

    if (Player_is_robot(pl))
	NumRobots--;

    /*
     * Swap entry no 'ind' with the last one.
     *
     * Change the PlayersArray[] pointer array to have
     * Player_by_index(ind) point to a valid player and move our leaving
     * player to PlayersArray[NumPlayers].
     */
    /* Swap pointers... */
    pl				= Player_by_index(NumPlayers);
    PlayersArray[NumPlayers]	= Player_by_index(ind);
    PlayersArray[ind]		= pl;
    /* Restore pointer. */
    pl				= Player_by_index(NumPlayers);

    GetIndArray[Player_by_index(ind)->id] = ind;
    GetIndArray[Player_by_index(NumPlayers)->id] = NumPlayers;

    Check_team_members(pl->team);

    for (i = NumPlayers - 1; i >= 0; i--) {
	player_t *pl_i = Player_by_index(i);

	if (Player_is_tank(pl_i)
	    && pl_i->lock.pl_id == id) {
	    /* remove tanks which were released by this player. */
	    if (options.keepShots)
		pl_i->lock.pl_id = NO_ID;
	    else
		Delete_player(pl_i);
	    continue;
	}
	if (BIT(pl_i->lock.tagged, LOCK_PLAYER|LOCK_VISIBLE)
	    && (pl_i->lock.pl_id == id || NumPlayers <= 1)) {
	    CLR_BIT(pl_i->lock.tagged, LOCK_PLAYER|LOCK_VISIBLE);
	    CLR_BIT(pl_i->used, USES_TRACTOR_BEAM);
	}
	if (Player_is_robot(pl_i)
	    && Robot_war_on_player(pl_i) == id)
	    Robot_reset_war(pl_i);

	for (j = 0; j < LOCKBANK_MAX; j++) {
	    if (pl_i->lockbank[j] == id)
		pl_i->lockbank[j] = NO_ID;
	}
	for (j = 0; j < MAX_RECORDED_SHOVES; j++) {
	    if (pl_i->shove_record[j].pusher_id == id)
		pl_i->shove_record[j].pusher_id = NO_ID;
	}
    }

    for (i = NumPlayers - 1; i >= 0; i--) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->conn != NULL)
	    Send_leave(pl_i->conn, id);
	else if (Player_is_tank(pl_i)) {
	    if (pl_i->lock.pl_id == id)
		Delete_player(pl_i);
	}
    }

    for (i = NumSpectators - 1; i >= 0; i--)
	Send_leave(Player_by_index(i + spectatorStart)->conn, id);

    GetIndArray[id] = NO_IND;
    release_ID(id);
}

void Add_spectator(player_t *pl)
{
    pl->home_base = NULL;
    pl->team = 0;
    GetIndArray[pl->id] = spectatorStart + NumSpectators;
    Player_set_score(pl,-6666);
    Player_set_mychar(pl,'S');
    NumSpectators++;
}

void Delete_spectator(player_t *pl)
{
    int i, ind = GetInd(pl->id);

    NumSpectators--;
    /* Swap leaver last */
    pl = Player_by_index(spectatorStart + NumSpectators);
    PlayersArray[spectatorStart + NumSpectators] = Player_by_index(ind);
    PlayersArray[ind] = pl;
    pl = Player_by_index(spectatorStart + NumSpectators);

    GetIndArray[Player_by_index(ind)->id] = ind;
    GetIndArray[pl->id] = spectatorStart + NumSpectators;

    Free_ship_shape(pl->ship);
    for (i = NumSpectators - 1; i >= 0; i--)
	Send_leave(Player_by_index(i + spectatorStart)->conn, pl->id);
}

void Detach_ball(player_t *pl, ballobject_t *ball)
{
    int i, cnt;

    if (ball == NULL || ball == pl->ball) {
	pl->ball = NULL;
	CLR_BIT(pl->used, USES_CONNECTOR);
    }

    if (BIT(pl->have, HAS_BALL)) {
	for (cnt = i = 0; i < NumObjs; i++) {
	    object_t *obj = Obj[i];

	    if (obj->type == OBJ_BALL && obj->id == pl->id) {
		if (ball == NULL || ball == BALL_PTR(obj))
		    obj->id = NO_ID;
		    /* Don't reset owner so you can throw balls */
		else
		    cnt++;
	    }
	}
	if (cnt == 0)
	    CLR_BIT(pl->have, HAS_BALL);
	else
	    sound_play_sensors(pl->pos, DROP_BALL_SOUND);
    }
}

void Kill_player(player_t *pl, bool add_rank_death)
{
    if (Player_is_killed(pl))
	Explode_fighter(pl);
    Player_death_reset(pl, add_rank_death);
}

void Player_death_reset(player_t *pl, bool add_rank_death)
{
    if (Player_is_tank(pl)) {
	Delete_player(pl);
	return;
    }

    if (Player_is_paused(pl))
	return;

    Detach_ball(pl, NULL);
    if (Player_uses_autopilot(pl)
	|| Player_is_hoverpaused(pl)) {
	CLR_BIT(pl->pl_status, HOVERPAUSE);
	Autopilot(pl, false);
    }

    pl->vel.x		= pl->vel.y	= 0.0;
    pl->acc.x		= pl->acc.y	= 0.0;
    pl->emptymass	= pl->mass	= options.shipMass;
    pl->obj_status	&= ~(KILL_OBJ_BITS);

    if (BIT(world->rules->mode, LIMITED_LIVES)) {
	bool waiting = Player_is_waiting(pl);

	Player_set_life(pl, pl->pl_life - 1);
	Player_set_state(pl, PL_STATE_APPEARING);

	if (pl->pl_life == -1) {
	    Player_set_life(pl, 0);
	    if (waiting)
		Player_set_state(pl, PL_STATE_WAITING);
	    else
		Player_set_state(pl, PL_STATE_DEAD);
	    Player_lock_closest(pl, false);	    
	}
    }
    else {
	Player_set_life(pl, pl->pl_life + 1);
	Player_set_state(pl, PL_STATE_APPEARING);
    }

    Player_init_items(pl);

    pl->forceVisible	= 0;
    assert(pl->recovery_count == RECOVERY_DELAY);
    pl->ecmcount	= 0;
    pl->emergency_thrust_left = 0;
    pl->emergency_shield_left = 0;
    pl->phasing_left	= 0;
    pl->self_destruct_count = 0;
    pl->damaged 	= 0;
    pl->stunned		= 0;
    pl->lock.distance	= 0;

    if (add_rank_death) {
	Rank_add_death(pl);
	pl->pl_deaths_since_join++;
    }

    pl->have	= DEF_HAVE;
    pl->used	&= ~(USED_KILL);
    pl->used	|= DEF_USED;
    pl->used	&= pl->have;
}

/* determines if two players are immune to eachother */
bool Team_immune(int id1, int id2)
{
    player_t *pl1, *pl2;

    /* owned stuff is never team immune */
    if (id1 == id2)
	return false;

    if (!options.teamImmunity)
	return false;

    if (id1 == NO_ID || id2 == NO_ID)
	/* can't find owner for cannon stuff */
	return false;

    pl1 = Player_by_id(id1);
    pl2 = Player_by_id(id2);

    if (Players_are_teammates(pl1, pl2))
	return true;

    if (Players_are_allies(pl1, pl2))
	return true;

    return false;
}

static char *old_status2str(int old_status)
{
    static char buf[256];

    buf[0] = '\0';

    if (old_status & OLD_PLAYING)
	strlcat(buf, "OLD_PLAYING ", sizeof(buf));
    if (old_status & OLD_PAUSE)
	strlcat(buf, "OLD_PAUSE ", sizeof(buf));
    if (old_status & OLD_GAME_OVER)
	strlcat(buf, "OLD_GAME_OVER ", sizeof(buf));

    return buf;
}

static char *state2str(int state)
{
    static char buf[256];

    buf[0] = '\0';

    if (state == PL_STATE_UNDEFINED)
	strlcat(buf, "PL_STATE_UNDEFINED", sizeof(buf));
    if (state == PL_STATE_WAITING)
	strlcat(buf, "PL_STATE_WAITING", sizeof(buf));
    if (state == PL_STATE_APPEARING)
	strlcat(buf, "PL_STATE_APPEARING", sizeof(buf));
    if (state == PL_STATE_ALIVE)
	strlcat(buf, "PL_STATE_ALIVE", sizeof(buf));
    if (state == PL_STATE_KILLED)
	strlcat(buf, "PL_STATE_KILLED", sizeof(buf));
    if (state == PL_STATE_DEAD)
	strlcat(buf, "PL_STATE_DEAD", sizeof(buf));
    if (state == PL_STATE_PAUSED)
	strlcat(buf, "PL_STATE_PAUSED", sizeof(buf));

    return buf;
}

void Player_print_state(player_t *pl, const char *funcname)
{
    warn("%-20s: %-16s (%c): %-20s %s ", funcname, pl->name, pl->mychar,
	 state2str(pl->pl_state), old_status2str(pl->pl_old_status));
}

void Player_set_state(player_t *pl, int state)
{
    pl->pl_state = state;

    switch (state) {
    case PL_STATE_WAITING:
	Player_set_mychar(pl, 'W');
	Player_set_life(pl, 0);
	pl->pl_old_status = OLD_GAME_OVER;
	break;
    case PL_STATE_APPEARING:
	Player_set_mychar(pl, pl->pl_type_mychar);
	/*Player_set_mychar(pl, 'A');*/
	pl->pl_old_status = 0;
	pl->recovery_count = RECOVERY_DELAY;
	break;
    case PL_STATE_ALIVE:
	Player_set_mychar(pl, pl->pl_type_mychar);
	pl->pl_old_status = OLD_PLAYING;
	break;
    case PL_STATE_KILLED:
	break;
    case PL_STATE_DEAD:
	Player_set_mychar(pl, 'D');
	pl->pl_old_status = OLD_GAME_OVER;
	break;
    case PL_STATE_PAUSED:
	Player_set_mychar(pl, 'P');
	Player_set_life(pl, 0);
	pl->pl_old_status = OLD_PAUSE;
	break;
    default:
	break;
    }
}
