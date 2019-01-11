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

#ifndef	SHIPSHAPE_H
#define	SHIPSHAPE_H

#ifndef TYPES_H
/* need position */
#include "types.h"
#include "const.h"
#endif

#ifndef CLICK_H
# include "click.h"
#endif

/*
 * Please don't change any of these maxima.
 * It will create incompatibilities and frustration.
 */
#define MIN_SHIP_PTS	    3
#define MAX_SHIP_PTS	    24
/* SSHACK needs to double the vertices */
#define MAX_SHIP_PTS2	    (MAX_SHIP_PTS * 2)
#define MAX_GUN_PTS	    3
#define MAX_LIGHT_PTS	    3
#define MAX_RACK_PTS	    4

typedef struct {
    clpos_t	*pts[MAX_SHIP_PTS2];	/* the shape rotated many ways */
    int		num_points;		/* total points in object */
    int		num_orig_points;	/* points before SSHACK */
    clpos_t	cashed_pts[MAX_SHIP_PTS2];
    int		cashed_dir;
} shape_t;

typedef struct {			/* Defines wire-obj, i.e. ship */
    clpos_t	*pts[MAX_SHIP_PTS2];	/* the shape rotated many ways */
    int		num_points;		/* total points in object */
    int		num_orig_points;	/* points before SSHACK */
    clpos_t	cashed_pts[MAX_SHIP_PTS2];
    int		cashed_dir;

    clpos_t	engine[RES];		/* Engine position */
    clpos_t	m_gun[RES];		/* Main gun position */
    int		num_l_gun,
		num_r_gun,
		num_l_rgun,
		num_r_rgun;		/* number of additional cannons */
    clpos_t	*l_gun[MAX_GUN_PTS],	/* Additional cannon positions, left*/
		*r_gun[MAX_GUN_PTS],	/* Additional cannon positions, right*/
		*l_rgun[MAX_GUN_PTS],	/* Additional rear cannon positions, left*/
		*r_rgun[MAX_GUN_PTS];	/* Additional rear cannon positions, right*/
    int		num_l_light,		/* Number of lights */
		num_r_light;
    clpos_t	*l_light[MAX_LIGHT_PTS], /* Left and right light positions */
		*r_light[MAX_LIGHT_PTS];
    int		num_m_rack;		/* Number of missile racks */
    clpos_t	*m_rack[MAX_RACK_PTS];
    int		shield_radius;		/* Radius of shield used by client. */

#ifdef	_NAMEDSHIPS
    char*	name;
    char*	author;
#endif
} shipshape_t;


static inline clpos_t ipos2clpos(ipos_t pos)
{
    clpos_t pt;

    pt.cx = PIXEL_TO_CLICK(pos.x);
    pt.cy = PIXEL_TO_CLICK(pos.y);

    return pt;
}

static inline position_t clpos2position(clpos_t pt)
{
    position_t pos;

    pos.x = CLICK_TO_FLOAT(pt.cx);
    pos.y = CLICK_TO_FLOAT(pt.cy);

    return pos;
}

extern shipshape_t *Default_ship(void);
extern void Free_ship_shape(shipshape_t *ship);
extern shipshape_t *Parse_shape_str(char *str);
extern shipshape_t *Convert_shape_str(char *str);
extern void Calculate_shield_radius(shipshape_t *ship);
extern int Validate_shape_str(char *str);
extern void Convert_ship_2_string(shipshape_t *ship, char *buf, char *ext,
				  unsigned shape_version);
extern void Rotate_point(clpos_t pt[RES]);
extern void Rotate_position(position_t pt[RES]);
extern clpos_t *Shape_get_points(shape_t *s, int dir);

static inline clpos_t
Ship_get_point_clpos(shipshape_t *ship, int i, int dir)
{
    return ship->pts[i][dir];
}
static inline clpos_t
Ship_get_engine_clpos(shipshape_t *ship, int dir)
{
    return ship->engine[dir];
}
static inline clpos_t
Ship_get_m_gun_clpos(shipshape_t *ship, int dir)
{
    return ship->m_gun[dir];
}
static inline clpos_t
Ship_get_l_gun_clpos(shipshape_t *ship, int gun, int dir)
{
    return ship->l_gun[gun][dir];
}
static inline clpos_t
Ship_get_r_gun_clpos(shipshape_t *ship, int gun, int dir)
{
    return ship->r_gun[gun][dir];
}
static inline clpos_t
Ship_get_l_rgun_clpos(shipshape_t *ship, int gun, int dir)
{
    return ship->l_rgun[gun][dir];
}
static inline clpos_t
Ship_get_r_rgun_clpos(shipshape_t *ship, int gun, int dir)
{
    return ship->r_rgun[gun][dir];
}
static inline clpos_t
Ship_get_l_light_clpos(shipshape_t *ship, int l, int dir)
{
    return ship->l_light[l][dir];
}
static inline clpos_t
Ship_get_r_light_clpos(shipshape_t *ship, int l, int dir)
{
    return ship->r_light[l][dir];
}
static inline clpos_t
Ship_get_m_rack_clpos(shipshape_t *ship, int rack, int dir)
{
    return ship->m_rack[rack][dir];
}


static inline position_t
Ship_get_point_position(shipshape_t *ship, int i, int dir)
{
    return clpos2position(Ship_get_point_clpos(ship, i, dir));
}
static inline position_t
Ship_get_engine_position(shipshape_t *ship, int dir)
{
    return clpos2position(Ship_get_engine_clpos(ship, dir));
}
static inline position_t
Ship_get_m_gun_position(shipshape_t *ship, int dir)
{
    return clpos2position(Ship_get_m_gun_clpos(ship, dir));
}
static inline position_t
Ship_get_l_gun_position(shipshape_t *ship, int gun, int dir)
{
    return clpos2position(Ship_get_l_gun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_r_gun_position(shipshape_t *ship, int gun, int dir)
{
    return clpos2position(Ship_get_r_gun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_l_rgun_position(shipshape_t *ship, int gun, int dir)
{
    return clpos2position(Ship_get_l_rgun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_r_rgun_position(shipshape_t *ship, int gun, int dir)
{
    return clpos2position(Ship_get_r_rgun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_l_light_position(shipshape_t *ship, int l, int dir)
{
    return clpos2position(Ship_get_l_light_clpos(ship, l, dir));
}
static inline position_t
Ship_get_r_light_position(shipshape_t *ship, int l, int dir)
{
    return clpos2position(Ship_get_r_light_clpos(ship, l, dir));
}
static inline position_t
Ship_get_m_rack_position(shipshape_t *ship, int rack, int dir)
{
    return clpos2position(Ship_get_m_rack_clpos(ship, rack, dir));
}

#endif
