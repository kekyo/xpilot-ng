/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2003 Kristian Söderblom <kps@users.sourceforge.net>
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

void Fire_laser(player_t *pl)
{
    clpos_t m_gun, pos;

    if (frame_time
	<= pl->laser_time + options.laserRepeatRate - timeStep + 1e-3)
 	return;
    pl->laser_time = MAX(frame_time, pl->laser_time + options.laserRepeatRate);

    if (pl->item[ITEM_LASER] > pl->num_pulses
	&& pl->velocity < options.pulseSpeed) {
	if (pl->fuel.sum < -ED_LASER)
	    CLR_BIT(pl->used, HAS_LASER);
	else {
	    m_gun = Ship_get_m_gun_clpos(pl->ship, pl->dir);
	    pos.cx = pl->pos.cx + m_gun.cx
		+ FLOAT_TO_CLICK(pl->vel.x * timeStep);
	    pos.cy = pl->pos.cy + m_gun.cy
		+ FLOAT_TO_CLICK(pl->vel.y * timeStep);
	    pos = World_wrap_clpos(pos);
	    if (is_inside(pos.cx, pos.cy, NONBALL_BIT | NOTEAM_BIT, NULL)
		!= NO_GROUP)
		return;
	    Fire_general_laser(pl->id, pl->team, pos,
			       pl->dir, pl->mods);
	}
    }
}

void Fire_general_laser(int id, int team, clpos_t pos, int dir,
			modifiers_t mods)
{
    double life;
    pulseobject_t *pulse;
    player_t *pl = Player_by_id(id);
    /*cannon_t *cannon = Cannon_by_id(id);*/

    if (!World_contains_clpos(pos)) {
	warn("Fire_general_laser: outside world.\n");
	return;
    }

    if (NumObjs >= MAX_TOTAL_SHOTS)
	return;

    if ((pulse = PULSE_PTR(Object_allocate())) == NULL)
	return;

    if (pl) {
	Player_add_fuel(pl, ED_LASER);
	sound_play_sensors(pos, FIRE_LASER_SOUND);
	life = options.pulseLife;
	/*Rank_FireLaser(pl);*/
    } else
	life = (int)CANNON_PULSE_LIFE;

    pulse->id		= (pl ? pl->id : NO_ID);
    pulse->team 	= team;
    Object_position_init_clpos(OBJ_PTR(pulse), pos);
    pulse->vel.x 	= options.pulseSpeed * tcos(dir);
    pulse->vel.y 	= options.pulseSpeed * tsin(dir);
    pulse->acc.x 	= 0;
    pulse->acc.y 	= 0;
    pulse->mass	 	= 0;
    pulse->life 	= life;
    pulse->obj_status 	= (pl ? 0 : FROMCANNON);
    pulse->type 	= OBJ_PULSE;
    pulse->mods 	= mods;
    pulse->color	= WHITE;

    pulse->fuse		= 0;
    pulse->pl_range 	= 0;
    pulse->pl_radius 	= 0;

    pulse->pulse_dir  	= dir;
    pulse->pulse_len  	= 0 /*options.pulseLength * CLICK*/;
    pulse->pulse_refl 	= false;

    Cell_add_object(OBJ_PTR(pulse));

    if (pl)
	pl->num_pulses++;
}


/*
 * Do what needs to be done when a laser pulse
 * actually hits a player.
 */
void Laser_pulse_hits_player(player_t *pl, pulseobject_t *pulse)
{
    player_t *kp = Player_by_id(pulse->id);
    cannon_t *cannon = NULL;

    if (kp == NULL)
	/* Perhaps it was a cannon pulse? */
	cannon = Cannon_by_id(pulse->id);

    pl->forceVisible += 1;
    if (Player_has_mirror(pl)
	&& (rfrac() * (2 * pl->item[ITEM_MIRROR])) >= 1) {
	pulse->pulse_dir = (int)(Wrap_cfindDir(pl->pos.cx - pulse->pos.cx,
					       pl->pos.cy - pulse->pos.cy)
				 * 2 - RES / 2 - pulse->pulse_dir);
	pulse->pulse_dir = MOD2(pulse->pulse_dir, RES);

	pulse->vel.x = options.pulseSpeed * tcos(pulse->pulse_dir);
	pulse->vel.y = options.pulseSpeed * tsin(pulse->pulse_dir);

	pulse->life += pl->item[ITEM_MIRROR];
	pulse->pulse_len = 0 /*PULSE_LENGTH*/;
	pulse->pulse_refl = true;
	return;
    }

    sound_play_sensors(pl->pos, PLAYER_EAT_LASER_SOUND);
    if (Player_uses_emergency_shield(pl))
	return;
    assert(pulse->type == OBJ_PULSE);

    /* kps - do we need some hack so that the laser pulse is
     * not removed in the same frame that its life ends ?? */
    pulse->life = 0;
    if ((Mods_get(pulse->mods, ModsLaser) & MODS_LASER_STUN)
	|| (options.laserIsStunGun
	    && options.allowLaserModifiers == false)) {
	if (BIT(pl->used, HAS_SHIELD|HAS_LASER|HAS_SHOT)
	    || Player_is_thrusting(pl)) {
	    if (kp)
		Set_message_f("%s got paralysed by %s's stun laser.%s",
			      pl->name, kp->name,
			      pl->id == kp->id ? " How strange!" : "");
	    else
		Set_message_f("%s got paralysed by a stun laser.", pl->name);

	    CLR_BIT(pl->used,
		    HAS_SHIELD|HAS_LASER|OBJ_SHOT);
	    Thrust(pl, false);
	    pl->stunned += 5;
	}
    } else if (Mods_get(pulse->mods, ModsLaser) & MODS_LASER_BLIND) {
	pl->damaged += (12 + 6);
	pl->forceVisible += (12 + 6);
	if (kp)
	    Record_shove(pl, kp, frame_loops + 12 + 6);
    } else {
	Player_add_fuel(pl, ED_LASER_HIT);
	if (!BIT(pl->used, HAS_SHIELD)
	    && !Player_has_armor(pl)) {
	    Player_set_state(pl, PL_STATE_KILLED);
    	    Handle_Scoring(SCORE_LASER,kp,pl,cannon,NULL);
	    if (kp) {
		Set_message_f("%s got roasted alive by %s's laser.%s",
			      pl->name, kp->name,
			      pl->id == kp->id ? " How strange!" : "");
	    }
	    else if (cannon != NULL) {
		Set_message_f("%s got roasted alive by cannonfire.", pl->name);
	    }
	    else {
		assert(pulse->id == NO_ID);
		Set_message_f("%s got roasted alive.", pl->name);
	    }

	    sound_play_sensors(pl->pos, PLAYER_ROASTED_SOUND);
	    if (kp && kp->id != pl->id) {
		Robot_war(pl, kp);
	    }
	}
	if (!BIT(pl->used, HAS_SHIELD)
	    && Player_has_armor(pl))
	    Player_hit_armor(pl);
    }
}
