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

bool team_dead(int team)
{
    int i;

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl = Player_by_index(i);

	if (pl->team != team)
	    continue;

	if (Player_is_alive(pl)
	    || Player_is_appearing(pl)
	    || Player_is_killed(pl))
	    return false;
    }
    return true;
}

/*
 * Return true if a lock is allowed.
 */
static bool Player_lock_allowed(player_t *pl, player_t *lock_pl)
{
    /* we can never lock on ourselves, nor on NULL. */
    if (lock_pl == NULL || pl->id == lock_pl->id)
	return false;

    /* Spectators can watch freely */
    if (pl->rectype == 2)
	return true;

    /* if we are actively playing then we can lock since we are not viewing. */
    if (Player_is_active(pl))
	return true;

    /* if there is no team play then we can always lock on anyone. */
    if (!BIT(world->rules->mode, TEAM_PLAY))
	return true;

    /* we can always lock on players from our own team. */
    if (Players_are_teammates(pl, lock_pl))
	return true;

    /* if lockOtherTeam is true then we can always lock on other teams. */
    if (options.lockOtherTeam)
	return true;

    /* if our own team is dead then we can lock on anyone. */
    if (team_dead(pl->team))
	return true;

    /* can't find any reason why this lock should be allowed. */
    return false;
}

static void Player_lock_next_or_prev(player_t *pl, int key)
{
    int i, j, ind = GetInd(pl->id);
    player_t *pl_i;

    if (NumPlayers == 0) /* Spectator? */
	return;

    j = i = GetInd(pl->lock.pl_id);
    if (!BIT(pl->lock.tagged, LOCK_PLAYER))
	i = j = 0;
    if (j < 0 || j >= NumPlayers)
	/* kps - handle this some other way */
	fatal("Illegal player lock target");

    do {
	if (key == KEY_LOCK_PREV) {
	    if (--i < 0)
		i = NumPlayers - 1;
	} else {
	    if (++i >= NumPlayers)
		i = 0;
	}
	if (i == j)
	    return;

	pl_i = Player_by_index(i);
    } while (i == ind
	     || Player_is_paused(pl_i)
	     || Player_is_dead(pl_i)
	     || Player_is_waiting(pl_i)
	     || !Player_lock_allowed(pl, pl_i));

    if (i == ind)
	CLR_BIT(pl->lock.tagged, LOCK_PLAYER);
    else {
	pl->lock.pl_id = pl_i->id;
	SET_BIT(pl->lock.tagged, LOCK_PLAYER);
    }
}

int Player_lock_closest(player_t *pl, bool next)
{
    int i;
    double dist = 0.0, best, l;
    player_t *lock_pl = NULL, *new_pl = NULL;

    if (!next)
	CLR_BIT(pl->lock.tagged, LOCK_PLAYER);

    if (BIT(pl->lock.tagged, LOCK_PLAYER)) {
	lock_pl = Player_by_id(pl->lock.pl_id);
	dist = Wrap_length(lock_pl->pos.cx - pl->pos.cx,
			   lock_pl->pos.cy - pl->pos.cy);
    }
    best = FLT_MAX;
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i == lock_pl
	    || !Player_is_active(pl_i)
	    || !Player_lock_allowed(pl, pl_i)
	    || Player_owns_tank(pl, pl_i)
	    || Players_are_teammates(pl, pl_i)
	    || Players_are_allies(pl, pl_i))
	    continue;

	l = Wrap_length(pl_i->pos.cx - pl->pos.cx,
			pl_i->pos.cy - pl->pos.cy);
	if (l >= dist && l < best) {
	    best = l;
	    new_pl = pl_i;
	}
    }
    if (new_pl == NULL)
	return 0;

    SET_BIT(pl->lock.tagged, LOCK_PLAYER);
    pl->lock.pl_id = new_pl->id;

    return 1;
}

static void Player_change_home(player_t *pl)
{
    player_t *pl2 = NULL;
    base_t *base2 = NULL;
    base_t *enemybase = NULL;
    double l, dist = 1e19;
    int i;

    for (i = 0; i < Num_bases(); i++) {
	base_t *base = Base_by_index(i);

	l = Wrap_length(pl->pos.cx - base->pos.cx,
			pl->pos.cy - base->pos.cy);
	if (l < dist
	    && l < 1.5 * BLOCK_CLICKS) {
	    if (base->team != TEAM_NOT_SET
		&& base->team != pl->team) {
		enemybase = base;
		continue;
	    }
	    base2 = base;
	    dist = l;
	}
    }

    if (base2 == NULL) {
	if (enemybase)
	    Set_player_message_f(pl, "Base belongs to team %d. "
				 "Enemy home bases can't be occupied. "
				 "[*Server notice*]", enemybase->team);
	else
	    Set_player_message(pl, "You are too far away from "
			       "a suitable base to change home. "
			       "[*Server notice*]");
	return;
    }

    /* Maybe the base is our own base? */
    if (base2 == pl->home_base)
	return;

    /* Let's see if someone else in our has this base. */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->id != pl->id
	    && !Player_is_tank(pl_i)
	    && base2 == pl_i->home_base) {
	    pl2 = pl_i;
	    break;
	}
    }

#if 0
    /* kps - perhaps this isn't a good idea. */
    if (pl2 != NULL
	&& Players_are_teammates(pl, pl2)
	&& Get_Score(pl) <= Get_Score(pl2)) {
	Set_player_message(pl, "You must have a higher score than your "
			   "teammate to take over their base. "
			   "[*Server notice*]");
	return;
    }
#endif

    pl->home_base = base2;
    sound_play_all(CHANGE_HOME_SOUND);

    if (pl2 != NULL) {
	Pick_startpos(pl2);
	Set_message_f("%s has taken over %s's home base.",
		      pl->name, pl2->name);
    } else
	Set_message_f("%s has changed home base.", pl->name);

    /*
     * Send info about new bases.
     */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->conn != NULL)
	    Send_base(pl_i->conn, pl->id, pl->home_base->ind);
    }
    for (i = 0; i < NumSpectators; i++)
	Send_base(Player_by_index(i + spectatorStart)->conn,
		  pl->id, pl->home_base->ind);

    if (pl2) {
	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl_i = Player_by_index(i);

	    if (pl_i->conn != NULL)
		Send_base(pl_i->conn, pl2->id, pl2->home_base->ind);
	}
	for (i = 0; i < NumSpectators; i++)
	    Send_base(Player_by_index(i + spectatorStart)->conn,
		      pl2->id, pl2->home_base->ind);
    }
}

static void Player_refuel(player_t *pl)
{
    int i;
    double l, dist = 1e19;

    if (!BIT(pl->have, HAS_REFUEL))
	return;

    CLR_BIT(pl->used, USES_REFUEL);
    for (i = 0; i < Num_fuels(); i++) {
	fuel_t *fs = Fuel_by_index(i);

	l = Wrap_length(pl->pos.cx - fs->pos.cx,
			pl->pos.cy - fs->pos.cy);
	if (!Player_is_refueling(pl)
	    || l < dist) {
	    SET_BIT(pl->used, USES_REFUEL);
	    pl->fs = i;
	    dist = l;
	}
    }
}

/* Repair target or possibly something else. */
static void Player_repair(player_t *pl)
{
    int i;
    double l, dist = 1e19;

    if (!BIT(pl->have, HAS_REPAIR))
	return;

    CLR_BIT(pl->used, USES_REPAIR);
    for (i = 0; i < Num_targets(); i++) {
	target_t *targ = Target_by_index(i);

	if (targ->team == pl->team
	    && targ->dead_ticks <= 0) {
	    l = Wrap_length(pl->pos.cx - targ->pos.cx,
			    pl->pos.cy - targ->pos.cy);
	    if (!Player_is_repairing(pl)
		|| l < dist) {
		SET_BIT(pl->used, USES_REPAIR);
		pl->repair_target = i;
		dist = l;
	    }
	}
    }
}

/* Player pressed pause key. */
static void Player_toggle_pause(player_t *pl)
{
    enum pausetype {
	unknown, paused, hoverpaused
    } pausetype = unknown;

    if (Player_is_paused(pl))
	pausetype = paused;
    else if (Player_is_hoverpaused(pl))
	pausetype = hoverpaused;
    else if (Player_is_appearing(pl))
	pausetype = paused;
    else {
	base_t *base = pl->home_base;
	double dist = Wrap_length(pl->pos.cx - base->pos.cx,
				  pl->pos.cy - base->pos.cy);
	double minv;

	if (dist < 1.5 * BLOCK_CLICKS) {
	    minv = 3.0;
	    pausetype = paused;
	} else {
	    minv = 5.0;
	    pausetype = hoverpaused;
	}
	minv += VECTOR_LENGTH(World_gravity(pl->pos));
	if (pl->velocity > minv) {
	    Set_player_message(pl,
		       "You need to slow down to pause. [*Server notice*]");
	    return;
	}
    }

    switch (pausetype) {
    case paused:
	if (Player_is_hoverpaused(pl))
	    break;

	if (Player_uses_autopilot(pl))
	    Autopilot(pl, false);

	Pause_player(pl, !Player_is_paused(pl));
	break;

    case hoverpaused:
	if (Player_is_paused(pl))
	    break;

	if (!Player_is_hoverpaused(pl)) {
	    /*
	     * Turn hover pause on, together with shields.
	     */
	    pl->pause_count = 5 * 12;
	    Player_self_destruct(pl, false);
	    SET_BIT(pl->pl_status, HOVERPAUSE);

	    if (Player_uses_emergency_thrust(pl))
		Emergency_thrust(pl, false);

	    if (BIT(pl->used, HAS_EMERGENCY_SHIELD))
		Emergency_shield(pl, false);

	    if (!Player_uses_autopilot(pl))
		Autopilot(pl, true);

	    if (Player_is_phasing(pl))
		Phasing(pl, false);

	    /*
	     * Don't allow firing while paused. Similar
	     * reasons exist for refueling, connector and
	     * tractor beams.  Other items are allowed (esp.
	     * cloaking).
	     */
	    Player_used_kill(pl);
	    if (BIT(pl->have, HAS_SHIELD))
		SET_BIT(pl->used, HAS_SHIELD);
	} else if (pl->pause_count <= 0) {
	    Autopilot(pl, false);
	    CLR_BIT(pl->pl_status, HOVERPAUSE);
	    if (!BIT(pl->have, HAS_SHIELD))
		CLR_BIT(pl->used, HAS_SHIELD);
	}
	break;
    default:
	warn("Player_toggle_pause: BUG: unknown pause type.");
	break;
    }
}

#define FOOBARSWAP(a, b)	    {double tmp = a; a = b; b = tmp;}

static void Player_swap_settings(player_t *pl)
{
    if (Player_is_hoverpaused(pl)
	|| Player_uses_autopilot(pl))
	return;

    /* kps - turnacc == 0.0 ? */
    if (pl->turnacc == 0.0) {
	FOOBARSWAP(pl->power, pl->power_s);
	FOOBARSWAP(pl->turnspeed, pl->turnspeed_s);
	FOOBARSWAP(pl->turnresistance, pl->turnresistance_s);
    }
}
#undef FOOBARSWAP


static void Player_toggle_compass(player_t *pl)
{
    int i, k, ind = GetInd(pl->id);

    if (!BIT(pl->have, HAS_COMPASS))
	return;
    TOGGLE_BIT(pl->used, USES_COMPASS);

    if (!Player_uses_compass(pl))
	return;

    /*
     * Verify if the lock has ever been initialized at all
     * and if the lock is still valid.
     */
    if (BIT(pl->lock.tagged, LOCK_PLAYER)
	&& NumPlayers > 1
	&& (k = pl->lock.pl_id) > 0
	&& (i = GetInd(k)) > 0
	&& i < NumPlayers
	&& Player_by_index(i)->id == k
	&& i != ind)
	return;

    Player_lock_closest(pl, false);
}


void Pause_player(player_t *pl, bool on)
{
    int i;

    /* kps - add support for pausing robots ? */
    if (!Player_is_human(pl))
	return;
    if (on && !Player_is_paused(pl)) { /* Turn pause mode on */
	if (pl->team != TEAM_NOT_SET)
	    world->teams[pl->team].SwapperId = NO_ID;
	/* Minimum pause time is 10 seconds at gamespeed 12. */
	pl->pause_count = 10 * 12;
	/* player might have paused when recovering */
	pl->recovery_count = 0;
	pl->updateVisibility = true;
	Player_set_state(pl, PL_STATE_PAUSED);
	pl->pauseTime = 0;
	if (options.baselessPausing) {
	    if (pl->team != TEAM_NOT_SET)
		world->teams[pl->team].NumMembers--;
	    pl->pl_prev_team = pl->team;
	    /* kps - probably broken if no team play */
	    pl->team = 0;
	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (pl_i->conn != NULL) {
		    Send_base(pl_i->conn, NO_ID, pl->home_base->ind);
		    Send_team(pl_i->conn, pl->id, 0);
		}
	    }
	    for (i = spectatorStart; i < spectatorStart + NumSpectators; i++) {
		player_t *pl_i = Player_by_index(i);

		Send_base(pl_i->conn, NO_ID, pl->home_base->ind);
		Send_team(pl_i->conn, pl->id, 0);
	    }
	    pl->home_base = NULL;
	}
	updateScores = true;

	Detach_ball(pl, NULL);
	if (Player_uses_autopilot(pl)
	    || Player_is_hoverpaused(pl)) {
	    CLR_BIT(pl->pl_status, HOVERPAUSE);
	    Autopilot(pl, false);
	}

	pl->vel.x		= pl->vel.y	= 0.0;
	pl->acc.x		= pl->acc.y	= 0.0;

	pl->obj_status	&= ~(KILL_OBJ_BITS);

	/*
	 * kps - possibly add option to make items reset
	 * to initial when pausing
	 */

	pl->forceVisible	= 0;
	pl->ecmcount		= 0;
	pl->emergency_thrust_left = 0;
	pl->emergency_shield_left = 0;
	pl->phasing_left	= 0;
	pl->self_destruct_count = 0;
	pl->damaged 		= 0;
	pl->stunned		= 0;
	pl->lock.distance	= 0;
	pl->used		= DEF_USED;

	for (i = 0; i < MAX_TEAMS ; i++) {
	    if (world->teams[i].SwapperId == pl->id)
		world->teams[i].SwapperId = NO_ID;
	}
    }
    else if (!on && Player_is_paused(pl)) { /* Turn pause mode off */

	if (pl->pause_count > 0) {
	    Set_player_message(pl, "You can't unpause so soon after pausing. "
			       "[*Server notice*]");
	    return;
	}
	/* there seems to be a race condition if idleTime is set later */
	pl->idleTime = 0;

	if (pl->home_base == NULL) {
	    int team = pl->pl_prev_team;

	    /* kps - code copied from Cmd_team() */
	    if (team > 0 && team < MAX_TEAMS
		&& (world->teams[team].NumBases
		    > world->teams[team].NumMembers)) {
		pl->team = team;
		world->teams[pl->team].NumMembers++;
		Set_swapper_state(pl);
		Pick_startpos(pl);
		Send_info_about_player(pl);
	    }
	    else {
		Set_player_message(pl, "You don't have a base. "
				   "Select team to unpause. "
				   "[*Server notice*]");
		return;
	    }
	}

	updateScores = true;
	if (BIT(world->rules->mode, LIMITED_LIVES)) {
	    /* too late, wait for next round */
	    Player_set_state(pl, PL_STATE_WAITING);
	} else {
	    Player_set_state(pl, PL_STATE_APPEARING);
	    Go_home(pl);
	}
	Player_reset_timing(pl);
    }
}


int Handle_keyboard(player_t *pl)
{
    int i, key;
    bool pressed;

    /*assert(!Player_is_killed(pl));*/

    for (key = 0; key < NUM_KEYS; key++) {
	/* Find first keyv element where last_keyv isn't equal to prev_keyv. */
	if (pl->last_keyv[key / BITV_SIZE] == pl->prev_keyv[key / BITV_SIZE]) {
	    /* Skip to next keyv element. */
	    key |= (BITV_SIZE - 1);
	    continue;
	}
	/* Now check which specific key it is that has changed state. */
	while (BITV_ISSET(pl->last_keyv, key)
	       == BITV_ISSET(pl->prev_keyv, key)) {
	    if (++key >= NUM_KEYS)
		break;
	}
	if (key >= NUM_KEYS)
	    break;

	pressed = (BITV_ISSET(pl->last_keyv, key) != 0) ? true : false;
	BITV_TOGGLE(pl->prev_keyv, key);
	/*
	 * KEY_SHIELD would interfere with auto-idle-pause
	 * due to client auto-shield hack.
	 */
	if (key != KEY_SHIELD)
	    pl->idleTime = 0;

	/*
	 * Allow these functions while you're 'dead'.
	 */
	if (!Player_is_alive(pl)
	    || Player_is_hoverpaused(pl)) {
	    switch (key) {
	    case KEY_PAUSE:
	    case KEY_LOCK_NEXT:
	    case KEY_LOCK_PREV:
	    case KEY_LOCK_CLOSE:
	    case KEY_LOCK_NEXT_CLOSE:
	    case KEY_TOGGLE_NUCLEAR:
	    case KEY_TOGGLE_CLUSTER:
	    case KEY_TOGGLE_IMPLOSION:
	    case KEY_TOGGLE_VELOCITY:
	    case KEY_TOGGLE_MINI:
	    case KEY_TOGGLE_SPREAD:
	    case KEY_TOGGLE_POWER:
	    case KEY_TOGGLE_LASER:
	    case KEY_TOGGLE_COMPASS:
	    case KEY_CLEAR_MODIFIERS:
	    case KEY_LOAD_MODIFIERS_1:
	    case KEY_LOAD_MODIFIERS_2:
	    case KEY_LOAD_MODIFIERS_3:
	    case KEY_LOAD_MODIFIERS_4:
	    case KEY_LOAD_LOCK_1:
	    case KEY_LOAD_LOCK_2:
	    case KEY_LOAD_LOCK_3:
	    case KEY_LOAD_LOCK_4:
	    case KEY_REPROGRAM:
	    case KEY_SWAP_SETTINGS:
	    case KEY_TANK_NEXT:
	    case KEY_TANK_PREV:
	    case KEY_TURN_LEFT:		/* Needed so that we don't get */
	    case KEY_TURN_RIGHT:	/* out-of-sync with the turnacc */
		break;
	    default:
		continue;
	    }
	}

	/* allow these functions while you're phased */
	if (Player_is_phasing(pl) && pressed) {
	    switch (key) {
	    case KEY_LOCK_NEXT:
	    case KEY_LOCK_PREV:
	    case KEY_LOCK_CLOSE:
	    case KEY_SHIELD:
	    case KEY_TOGGLE_NUCLEAR:
	    case KEY_TURN_LEFT:
	    case KEY_TURN_RIGHT:
	    case KEY_SELF_DESTRUCT:
	    case KEY_LOSE_ITEM:
	    case KEY_PAUSE:
	    case KEY_TANK_NEXT:
	    case KEY_TANK_PREV:
	    case KEY_TOGGLE_VELOCITY:
	    case KEY_TOGGLE_CLUSTER:
	    case KEY_SWAP_SETTINGS:
	    case KEY_THRUST:
	    case KEY_CLOAK:
	    case KEY_DROP_BALL:
	    case KEY_TALK:
	    case KEY_LOCK_NEXT_CLOSE:
	    case KEY_TOGGLE_COMPASS:
	    case KEY_TOGGLE_MINI:
	    case KEY_TOGGLE_SPREAD:
	    case KEY_TOGGLE_POWER:
	    case KEY_TOGGLE_LASER:
	    case KEY_EMERGENCY_THRUST:
	    case KEY_CLEAR_MODIFIERS:
	    case KEY_LOAD_MODIFIERS_1:
	    case KEY_LOAD_MODIFIERS_2:
	    case KEY_LOAD_MODIFIERS_3:
	    case KEY_LOAD_MODIFIERS_4:
	    case KEY_SELECT_ITEM:
	    case KEY_PHASING:
	    case KEY_TOGGLE_IMPLOSION:
	    case KEY_REPROGRAM:
	    case KEY_LOAD_LOCK_1:
	    case KEY_LOAD_LOCK_2:
	    case KEY_LOAD_LOCK_3:
	    case KEY_LOAD_LOCK_4:
	    case KEY_EMERGENCY_SHIELD:
	    case KEY_TOGGLE_AUTOPILOT:
		break;
	    default:
		continue;
	    }
	}

	if (pressed) { /* --- KEYPRESS --- */
	    switch (key) {

	    case KEY_TANK_NEXT:
	    case KEY_TANK_PREV:
		if (pl->fuel.num_tanks) {
		    pl->fuel.current += (key==KEY_TANK_NEXT) ? 1 : -1;
		    if (pl->fuel.current < 0)
			pl->fuel.current = pl->fuel.num_tanks;
		    else if (pl->fuel.current > pl->fuel.num_tanks)
			pl->fuel.current = 0;
		}
		break;

	    case KEY_TANK_DETACH:
		Tank_handle_detach(pl);
		break;

	    case KEY_LOCK_NEXT:
	    case KEY_LOCK_PREV:
		Player_lock_next_or_prev(pl, key);
		break;

	    case KEY_TOGGLE_COMPASS:
		Player_toggle_compass(pl);
		break;

	    case KEY_LOCK_NEXT_CLOSE:
		if (!Player_lock_closest(pl, true))
		    Player_lock_closest(pl, false);
		break;

	    case KEY_LOCK_CLOSE:
		Player_lock_closest(pl, false);
		break;

	    case KEY_CHANGE_HOME:
		Player_change_home(pl);
		break;

	    case KEY_SHIELD:
		if (BIT(pl->have, HAS_SHIELD)) {
		    SET_BIT(pl->used, HAS_SHIELD);
		    CLR_BIT(pl->used, HAS_LASER);	/* don't remove! */
		}
		break;

	    case KEY_DROP_BALL:
		Detach_ball(pl, NULL);
		break;

	    case KEY_FIRE_SHOT:
		if (!BIT(pl->used, HAS_SHIELD|HAS_SHOT)
		    && BIT(pl->have, HAS_SHOT)) {
		    SET_BIT(pl->used, HAS_SHOT);
		    /* Set this so that one shot is fired even if player
		     * releases the key during the same frame */
		    pl->did_shoot = true;
		}
		break;

	    case KEY_FIRE_MISSILE:
		if (pl->item[ITEM_MISSILE] > 0)
		    Fire_shot(pl, OBJ_SMART_SHOT, pl->dir);
		break;

	    case KEY_FIRE_HEAT:
		if (pl->item[ITEM_MISSILE] > 0)
		    Fire_shot(pl, OBJ_HEAT_SHOT, pl->dir);
		break;

	    case KEY_FIRE_TORPEDO:
		if (pl->item[ITEM_MISSILE] > 0)
		    Fire_shot(pl, OBJ_TORPEDO, pl->dir);
		break;

	    case KEY_FIRE_LASER:
		if (pl->item[ITEM_LASER] > 0 && BIT(pl->used, HAS_SHIELD) == 0)
		    SET_BIT(pl->used, HAS_LASER);
		break;

	    case KEY_TOGGLE_NUCLEAR:
		switch (Mods_get(pl->mods, ModsNuclear)) {
		case MODS_NUCLEAR:
		    Mods_set(&pl->mods, ModsNuclear,
			     MODS_NUCLEAR|MODS_FULLNUCLEAR);
		    break;
		case 0:
		    Mods_set(&pl->mods, ModsNuclear, MODS_NUCLEAR);
		    break;
		default:
		    Mods_set(&pl->mods, ModsNuclear, 0);
		    break;
		}

		break;

	    case KEY_TOGGLE_CLUSTER:
		{
		    int cluster = Mods_get(pl->mods, ModsCluster);

		    Mods_set(&pl->mods, ModsCluster, !cluster);
		}
		break;

	    case KEY_TOGGLE_IMPLOSION:
		{
		    int implosion = Mods_get(pl->mods, ModsImplosion);

		    Mods_set(&pl->mods, ModsImplosion, !implosion);
		}
		break;

	    case KEY_TOGGLE_VELOCITY:
		{
		    int velocity = Mods_get(pl->mods, ModsVelocity);

		    if (velocity == MODS_VELOCITY_MAX)
			velocity = 0;
		    else
			velocity++;
		    Mods_set(&pl->mods, ModsVelocity, velocity);
		}
		break;

	    case KEY_TOGGLE_MINI:
		{
		    int mini = Mods_get(pl->mods, ModsMini);

		    if (mini == MODS_MINI_MAX)
			mini = 0;
		    else
			mini++;
		    Mods_set(&pl->mods, ModsMini, mini);
		}
		break;

	    case KEY_TOGGLE_SPREAD:
		{
		    int spread = Mods_get(pl->mods, ModsSpread);

		    if (spread == MODS_SPREAD_MAX)
			spread = 0;
		    else
			spread++;
		    Mods_set(&pl->mods, ModsSpread, spread);
		}
		break;

	    case KEY_TOGGLE_LASER:
		{
		    int laser = Mods_get(pl->mods, ModsLaser);

		    if (laser == MODS_LASER_MAX)
			laser = 0;
		    else
			laser++;
		    Mods_set(&pl->mods, ModsLaser, laser);
		}
		break;

	    case KEY_TOGGLE_POWER:
		{
		    int power = Mods_get(pl->mods, ModsPower);

		    if (power == MODS_POWER_MAX)
			power = 0;
		    else
			power++;
		    Mods_set(&pl->mods, ModsPower, power);
		}
		break;

	    case KEY_CLEAR_MODIFIERS:
		Mods_clear(&pl->mods);
		break;

	    case KEY_REPROGRAM:
		SET_BIT(pl->pl_status, REPROGRAM);
		break;

	    case KEY_LOAD_MODIFIERS_1:
	    case KEY_LOAD_MODIFIERS_2:
	    case KEY_LOAD_MODIFIERS_3:
	    case KEY_LOAD_MODIFIERS_4: {
		modifiers_t *m = &(pl->modbank[key - KEY_LOAD_MODIFIERS_1]);

		if (BIT(pl->pl_status, REPROGRAM))
		    *m = pl->mods;
		else {
		    pl->mods = *m;
		    Mods_filter(&pl->mods);
		}
		break;
	    }

	    case KEY_LOAD_LOCK_1:
	    case KEY_LOAD_LOCK_2:
	    case KEY_LOAD_LOCK_3:
	    case KEY_LOAD_LOCK_4: {
		int *l = &(pl->lockbank[key - KEY_LOAD_LOCK_1]);

		if (BIT(pl->pl_status, REPROGRAM)) {
		    if (BIT(pl->lock.tagged, LOCK_PLAYER))
			*l = pl->lock.pl_id;
		} else {
		    if (*l != -1
			    && Player_lock_allowed(pl, Player_by_id(*l))) {
			pl->lock.pl_id = *l;
			SET_BIT(pl->lock.tagged, LOCK_PLAYER);
		    }
		}
		break;
	    }

	    case KEY_TOGGLE_AUTOPILOT:
		if (Player_has_autopilot(pl))
		    Autopilot(pl, !Player_uses_autopilot(pl));
		break;

	    case KEY_EMERGENCY_THRUST:
		if (Player_has_emergency_thrust(pl))
		    Emergency_thrust(pl,
				     !Player_uses_emergency_thrust(pl));
		break;

	    case KEY_EMERGENCY_SHIELD:
		if (BIT(pl->have, HAS_EMERGENCY_SHIELD))
		    Emergency_shield(pl,
				     !BIT(pl->used, HAS_EMERGENCY_SHIELD));
		break;

	    case KEY_DROP_MINE:
		Place_mine(pl);
		break;

	    case KEY_DETACH_MINE:
		Place_moving_mine(pl);
		break;

	    case KEY_DETONATE_MINES:
		Detonate_mines(pl);
		break;

	    case KEY_TURN_LEFT:
	    case KEY_TURN_RIGHT:
		if (Player_uses_autopilot(pl))
		    Autopilot(pl, false);
		pl->turnacc = 0;
#if 0
		if (frame_loops % 50 == 0)
		    Set_player_message(pl, "You should use the mouse to turn."
				       " [*Server notice*]");
#endif
		if (BITV_ISSET(pl->last_keyv, KEY_TURN_LEFT))
		    pl->turnacc += pl->turnspeed;
		if (BITV_ISSET(pl->last_keyv, KEY_TURN_RIGHT))
		    pl->turnacc -= pl->turnspeed;
		break;

	    case KEY_SELF_DESTRUCT:
		if (Player_is_self_destructing(pl))
		    Player_self_destruct(pl, false);
		else
		    Player_self_destruct(pl, true);
		break;

	    case KEY_PAUSE:
		Player_toggle_pause(pl);
		break;

	    case KEY_SWAP_SETTINGS:
		Player_swap_settings(pl);
		break;

	    case KEY_REFUEL:
		Player_refuel(pl);
		break;

	    case KEY_REPAIR:
		Player_repair(pl);
		break;

	    case KEY_CONNECTOR:
		if (BIT(pl->have, HAS_CONNECTOR))
		    SET_BIT(pl->used, USES_CONNECTOR);
		break;

	    case KEY_PRESSOR_BEAM:
		if (Player_has_tractor_beam(pl)) {
		    pl->tractor_is_pressor = true;
		    SET_BIT(pl->used, USES_TRACTOR_BEAM);
		}
		break;

	    case KEY_TRACTOR_BEAM:
		if (Player_has_tractor_beam(pl)) {
		    pl->tractor_is_pressor = false;
		    SET_BIT(pl->used, USES_TRACTOR_BEAM);
		}
		break;

	    case KEY_THRUST:
		if (Player_uses_autopilot(pl))
		    Autopilot(pl, false);
		Thrust(pl, true);
		break;

	    case KEY_CLOAK:
		if (pl->item[ITEM_CLOAK] > 0)
		    Cloak(pl, !Player_is_cloaked(pl));
		break;

	    case KEY_ECM:
		Fire_ecm(pl);
		break;

	    case KEY_TRANSPORTER:
		Do_transporter(pl);
		break;

	    case KEY_DEFLECTOR:
		if (pl->item[ITEM_DEFLECTOR] > 0)
		    Deflector(pl, !BIT(pl->used, USES_DEFLECTOR));
		break;

	    case KEY_HYPERJUMP:
		Initiate_hyperjump(pl);
		break;

	    case KEY_PHASING:
		if (Player_has_phasing_device(pl))
		    Phasing(pl, !Player_is_phasing(pl));
		break;

	    case KEY_SELECT_ITEM:
		for (i = 0; i < NUM_ITEMS; i++) {
		    if (++pl->lose_item >= NUM_ITEMS)
			pl->lose_item = 0;
		    if (pl->lose_item == ITEM_FUEL
			|| pl->lose_item == ITEM_TANK)
			/* can't lose fuel or tanks. */
			continue;
		    if (pl->item[pl->lose_item] > 0) {
			/* 2: key down; 1: key up */
			pl->lose_item_state = 2;
			break;
		    }
		}
		break;

	    case KEY_LOSE_ITEM:
		do_lose_item(pl);
		break;

	    default:
		break;
	    }
	} else {
	    /* --- KEYRELEASE --- */
	    switch (key) {
	    case KEY_TURN_LEFT:
	    case KEY_TURN_RIGHT:
		if (Player_uses_autopilot(pl))
		    Autopilot(pl, false);
		pl->turnacc = 0;
		if (BITV_ISSET(pl->last_keyv, KEY_TURN_LEFT))
		    pl->turnacc += pl->turnspeed;
		if (BITV_ISSET(pl->last_keyv, KEY_TURN_RIGHT))
		    pl->turnacc -= pl->turnspeed;
		break;

	    case KEY_REFUEL:
		CLR_BIT(pl->used, USES_REFUEL);
		break;

	    case KEY_REPAIR:
		CLR_BIT(pl->used, USES_REPAIR);
		break;

	    case KEY_CONNECTOR:
		CLR_BIT(pl->used, USES_CONNECTOR);
		break;

	    case KEY_TRACTOR_BEAM:
	    case KEY_PRESSOR_BEAM:
		CLR_BIT(pl->used, USES_TRACTOR_BEAM);
		break;

	    case KEY_SHIELD:
		if (BIT(pl->used, HAS_SHIELD)) {
		    CLR_BIT(pl->used, HAS_SHIELD|HAS_LASER);
		    /*
		     * Insert the default fireRepeatRate between lowering
		     * shields and firing in order to prevent macros
		     * and hacked clients.
		     */
		    pl->shot_time = frame_time;
		    pl->laser_time = frame_time;
		}
		break;

	    case KEY_FIRE_SHOT:
		CLR_BIT(pl->used, HAS_SHOT);
		break;

	    case KEY_FIRE_LASER:
		CLR_BIT(pl->used, HAS_LASER);
		break;

	    case KEY_THRUST:
		if (Player_uses_autopilot(pl))
		    Autopilot(pl, false);
		Thrust(pl, false);
		break;

	    case KEY_REPROGRAM:
		CLR_BIT(pl->pl_status, REPROGRAM);
		break;

	    case KEY_SELECT_ITEM:
		pl->lose_item_state = 1;
		break;

	    default:
		break;
	    }
	}
    }
    memcpy(pl->prev_keyv, pl->last_keyv, sizeof(pl->last_keyv));

    return 1;
}
