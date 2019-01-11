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

/*
 * Fast conversion of 'num' into 'str' starting at position 'i', returns
 * index of character after converted number.
 */
static int num2str(int num, char *str, int i)
{
    int	digits, t;

    if (num < 0) {
	str[i++] = '-';
	num = -num;
    }
    if (num < 10) {
	str[i++] = '0' + num;
	return i;
    }
    for (t = num, digits = 0; t; t /= 10, digits++)
	;
    for (t = i+digits-1; t >= 0; t--) {
	str[t] = num % 10;
	num /= 10;
    }
    return i + digits;
}


#define MODS_BIT0	(1<<0)
#define MODS_BIT1	(1<<1)

#define MODS_N_BIT0	(1<<0)		/* nuclear */
#define MODS_N_BIT1	(1<<1)		/* fullnuclear */
#define MODS_C_BIT	(1<<2)		/* cluster */
#define MODS_I_BIT	(1<<3)		/* implosion */
#define MODS_V_BIT0	(1<<4)		/* velocity */
#define MODS_V_BIT1	(1<<5)
#define MODS_X_BIT0	(1<<6)		/* mini */
#define MODS_X_BIT1	(1<<7)
#define MODS_Z_BIT0	(1<<8)		/* spread */
#define MODS_Z_BIT1	(1<<9)
#define MODS_B_BIT0	(1<<10)		/* power */
#define MODS_B_BIT1	(1<<11)
#define MODS_LS_BIT	(1<<12)		/* stun laser */
#define MODS_LB_BIT	(1<<13)		/* blinding laser */

static inline int Get_nuclear_modifier(modifiers_t mods)
{
    int n0, n1;

    n0 = BIT(mods, MODS_N_BIT0) ? 1 : 0;
    n1 = BIT(mods, MODS_N_BIT1) ? 1 : 0;

    return (n1 << 1) + n0;
}
static inline void Set_nuclear_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_NUCLEAR_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_N_BIT0);
    else
	CLR_BIT(*mods, MODS_N_BIT0);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_N_BIT1);
    else
	CLR_BIT(*mods, MODS_N_BIT1);
}

static inline int Get_cluster_modifier(modifiers_t mods)
{
    return (int) BIT(mods, MODS_C_BIT) ? 1 : 0;
}
static inline void Set_cluster_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, 1);
    if (value)
	SET_BIT(*mods, MODS_C_BIT);
    else
	CLR_BIT(*mods, MODS_C_BIT);
}

static inline int Get_implosion_modifier(modifiers_t mods)
{
    return (int) BIT(mods, MODS_I_BIT) ? 1 : 0;
}
static inline void Set_implosion_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, 1);
    if (value)
	SET_BIT(*mods, MODS_I_BIT);
    else
	CLR_BIT(*mods, MODS_I_BIT);
}

static inline int Get_velocity_modifier(modifiers_t mods)
{
    int v0, v1;

    v0 = BIT(mods, MODS_V_BIT0) ? 1 : 0;
    v1 = BIT(mods, MODS_V_BIT1) ? 1 : 0;

    return (v1 << 1) + v0;
}
static inline void Set_velocity_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_VELOCITY_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_V_BIT0);
    else
	CLR_BIT(*mods, MODS_V_BIT0);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_V_BIT1);
    else
	CLR_BIT(*mods, MODS_V_BIT1);
}

static inline int Get_mini_modifier(modifiers_t mods)
{
    int x0, x1;

    x0 = BIT(mods, MODS_X_BIT0) ? 1 : 0;
    x1 = BIT(mods, MODS_X_BIT1) ? 1 : 0;

    return (x1 << 1) + x0;
}
static inline void Set_mini_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_MINI_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_X_BIT0);
    else
	CLR_BIT(*mods, MODS_X_BIT0);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_X_BIT1);
    else
	CLR_BIT(*mods, MODS_X_BIT1);
}

static inline int Get_spread_modifier(modifiers_t mods)
{
    int z0, z1;

    z0 = BIT(mods, MODS_Z_BIT0) ? 1 : 0;
    z1 = BIT(mods, MODS_Z_BIT1) ? 1 : 0;

    return (z1 << 1) + z0;
}
static inline void Set_spread_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_SPREAD_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_Z_BIT0);
    else
	CLR_BIT(*mods, MODS_Z_BIT0);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_Z_BIT1);
    else
	CLR_BIT(*mods, MODS_Z_BIT1);
}

static inline int Get_power_modifier(modifiers_t mods)
{
    int b0, b1;

    b0 = BIT(mods, MODS_B_BIT0) ? 1 : 0;
    b1 = BIT(mods, MODS_B_BIT1) ? 1 : 0;

    return (b1 << 1) + b0;
}
static inline void Set_power_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_POWER_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_B_BIT0);
    else
	CLR_BIT(*mods, MODS_B_BIT0);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_B_BIT1);
    else
	CLR_BIT(*mods, MODS_B_BIT1);
}

static inline int Get_laser_modifier(modifiers_t mods)
{
    int ls, lb;

    ls = BIT(mods, MODS_LS_BIT) ? 1 : 0;
    lb = BIT(mods, MODS_LB_BIT) ? 1 : 0;

    return (lb << 1) + ls;
}
static inline void Set_laser_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_LASER_MAX);
    if (BIT(value, MODS_BIT0))
	SET_BIT(*mods, MODS_LS_BIT);
    else
	CLR_BIT(*mods, MODS_LS_BIT);
    if (BIT(value, MODS_BIT1))
	SET_BIT(*mods, MODS_LB_BIT);
    else
	CLR_BIT(*mods, MODS_LB_BIT);
}

/*
 * Returns 0 if ok, -1 if not allowed.
 */
int Mods_set(modifiers_t *mods, modifier_t modifier, int val)
{
    bool allow = false;

    if (val == 0)
	allow = true;
    else if (modifier == ModsNuclear) {
	if (BIT(world->rules->mode, ALLOW_NUKES)) 
	    allow = true;
    }
    else if (modifier == ModsCluster) {
	if (BIT(world->rules->mode, ALLOW_CLUSTERS)) 
	    allow = true;
    }
    else if (modifier == ModsLaser) {
	if (BIT(world->rules->mode, ALLOW_LASER_MODIFIERS)) 
	    allow = true;
    }
    else {
	if (BIT(world->rules->mode, ALLOW_MODIFIERS))
	    allow = true;
    }

    if (!allow)
	return -1;

    switch (modifier) {
    case ModsNuclear:
	Set_nuclear_modifier(mods, val);
	break;
    case ModsCluster:
	Set_cluster_modifier(mods, val);
	break;
    case ModsImplosion:
	Set_implosion_modifier(mods, val);
	break;
    case ModsVelocity:
	Set_velocity_modifier(mods, val);
	break;
    case ModsMini:
	Set_mini_modifier(mods, val);
	break;
    case ModsSpread:
	Set_spread_modifier(mods, val);
	break;
    case ModsPower:
	Set_power_modifier(mods, val);
	break;
    case ModsLaser:
	Set_laser_modifier(mods, val);
	break;
    default:
	warn("No such modifier: %d", modifier);
	assert(0);
	break;
    }

    return 0;
}

int Mods_get(modifiers_t mods, modifier_t modifier)
{
    switch (modifier) {
    case ModsNuclear:
	return Get_nuclear_modifier(mods);
    case ModsCluster:
	return Get_cluster_modifier(mods);
    case ModsImplosion:
	return Get_implosion_modifier(mods);
    case ModsVelocity:
	return Get_velocity_modifier(mods);
    case ModsMini:
	return Get_mini_modifier(mods);
    case ModsSpread:
	return Get_spread_modifier(mods);
    case ModsPower:
	return Get_power_modifier(mods);
    case ModsLaser:
	return Get_laser_modifier(mods);
    default:
	assert(0);
	break;
    }
    return 0;
}

/*
 * modstr must be able to hold at least MAX_CHARS chars.
 */
void Mods_to_string(modifiers_t mods, char *modstr, size_t size)
{
    int i = 0, t;

    if (size < MAX_CHARS)
	return;
    t = Get_nuclear_modifier(mods);
    if (t & MODS_FULLNUCLEAR)
	modstr[i++] = 'F';
    if (t & MODS_NUCLEAR)
	modstr[i++] = 'N';
    if (Get_cluster_modifier(mods))
	modstr[i++] = 'C';
    if (Get_implosion_modifier(mods))
	modstr[i++] = 'I';
    t = Get_velocity_modifier(mods);
    if (t) {
	if (i) modstr[i++] = ' ';
	modstr[i++] = 'V';
	i = num2str(t, modstr, i);
    }
    t = Get_mini_modifier(mods);
    if (t) {
	if (i) modstr[i++] = ' ';
	modstr[i++] = 'X';
	i = num2str(t + 1, modstr, i);
    }
    t = Get_spread_modifier(mods);
    if (t) {
	if (i) modstr[i++] = ' ';
	modstr[i++] = 'Z';
	i = num2str(t, modstr, i);
    }
    t = Get_power_modifier(mods);
    if (t) {
	if (i) modstr[i++] = ' ';
	modstr[i++] = 'B';
	i = num2str(t, modstr, i);
    }
    t = Get_laser_modifier(mods);
    if (t) {
	if (i) modstr[i++] = ' ';
	modstr[i++] = 'L';
	if (t & MODS_LASER_STUN)
	    modstr[i++] = 'S';
	if (t & MODS_LASER_BLIND)
	    modstr[i++] = 'B';
    }
    modstr[i] = '\0';
}


void Mods_filter(modifiers_t *mods)
{
    if (!BIT(world->rules->mode, ALLOW_NUKES))
	Mods_set(mods, ModsNuclear, 0);

    if (!BIT(world->rules->mode, ALLOW_CLUSTERS))
	Mods_set(mods, ModsCluster, 0);

    if (!BIT(world->rules->mode, ALLOW_MODIFIERS)) {
	Mods_set(mods, ModsImplosion, 0);
	Mods_set(mods, ModsVelocity, 0);
	Mods_set(mods, ModsMini, 0);
	Mods_set(mods, ModsSpread, 0);
	Mods_set(mods, ModsPower, 0);
    }

    if (!BIT(world->rules->mode, ALLOW_LASER_MODIFIERS))
	Mods_set(mods, ModsLaser, 0);
}

static int str2num (const char **strp, int min, int max)
{
    const char *str = *strp;
    int num = 0;

    while (isdigit(*str)) {
	num *= 10;
	num += *str++ - '0';
    }
    *strp = str;
    LIMIT(num, min, max);
    return num;
}

void Player_set_modbank(player_t *pl, int bank, const char *str)
{
    const char *cp;
    modifiers_t mods;
    int mini, velocity, spread, power;

    if (bank >= NUM_MODBANKS)
	return;

    Mods_clear(&mods);

    for (cp = str; *cp; cp++) {
	switch (*cp) {
	case 'F': case 'f':
	    if (*(cp+1) == 'N' || *(cp+1) == 'n')
		Mods_set(&mods, ModsNuclear,
			 MODS_NUCLEAR|MODS_FULLNUCLEAR);
	    break;
	case 'N': case 'n':
	    if (Mods_get(mods, ModsNuclear) == 0)
		Mods_set(&mods, ModsNuclear, MODS_NUCLEAR);
	    break;
	case 'C': case 'c':
	    Mods_set(&mods, ModsCluster, 1);
	    break;
	case 'I': case 'i':
	    Mods_set(&mods, ModsImplosion, 1);
	    break;
	case 'V': case 'v':
	    cp++;
	    velocity = str2num (&cp, 0, MODS_VELOCITY_MAX);
	    Mods_set(&mods, ModsVelocity, velocity);
	    cp--;
	    break;
	case 'X': case 'x':
	    cp++;
	    mini = str2num (&cp, 1, MODS_MINI_MAX+1) - 1;
	    Mods_set(&mods, ModsMini, mini);
	    cp--;
	    break;
	case 'Z': case 'z':
	    cp++;
	    spread = str2num (&cp, 0, MODS_SPREAD_MAX);
	    Mods_set(&mods, ModsSpread, spread);
	    cp--;
	    break;
	case 'B': case 'b':
	    cp++;
	    power = str2num (&cp, 0, MODS_POWER_MAX);
	    Mods_set(&mods, ModsPower, power);
	    cp--;
	    break;
	case 'L': case 'l':
	    cp++;
	    if (*cp == 'S' || *cp == 's')
		Mods_set(&mods, ModsLaser, MODS_LASER_STUN);
	    if (*cp == 'B' || *cp == 'b')
		Mods_set(&mods, ModsLaser, MODS_LASER_BLIND);
	    break;
	default:
	    /* Ignore unknown modifiers. */
	    break;
	}
    }
    pl->modbank[bank] = mods;
}
