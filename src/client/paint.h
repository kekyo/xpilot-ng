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

#ifndef PAINT_H
#define PAINT_H

#ifndef TYPES_H
# include "types.h"
#endif
#ifndef CLIENT_H
# include "client.h"
#endif

/* constants begin */
#define MAX_COLORS		16	/* Max. switched colors ever */
#define MAX_COLOR_LEN		32	/* Max. length of a color name */

#define MIN_HUD_SIZE		90	/* Size/2 of HUD lines */
#define HUD_OFFSET		20	/* Hud line offset */
#define FUEL_GAUGE_OFFSET	6
#define HUD_FUEL_GAUGE_SIZE	(2*(MIN_HUD_SIZE-HUD_OFFSET-FUEL_GAUGE_OFFSET))

#define WARNING_DISTANCE	(VISIBILITY_DISTANCE*0.8)

#define TITLE_DELAY		500	/* Should probably change to seconds */

#define DSIZE			4	/* Size of diamond (on radar) */
/* constants end */


/*
 * Global objects.
 */

extern ipos_t	world;
extern ipos_t	realWorld;

extern int	hudSize;		/* Size for HUD drawing */
extern int	hudRadarDotSize;	/* Size for hudradar dot drawing */
extern double	hudRadarScale;		/* Scale for hudradar drawing */
extern double 	hudRadarLimit;		/* Limit for hudradar drawing */

extern unsigned	draw_width, draw_height;

extern short	ext_view_width;		/* Width of extended visible area */
extern short	ext_view_height;	/* Height of extended visible area */
extern int	active_view_width;	/* Width of active map area displayed. */
extern int	active_view_height;	/* Height of active map area displayed. */
extern int	ext_view_x_offset;	/* Offset of ext_view_width */
extern int	ext_view_y_offset;	/* Offset of ext_view_height */
extern bool	markingLights;		/* Marking lights on ships */

extern int	num_spark_colors;

extern long	loops;
extern long	loopsSlow;
extern double	timePerFrame;

extern bool	players_exposed;

static inline float WINSCALE_f(float x)
{
    return x * clData.fscale;
}

static inline double WINSCALE_d(double x)
{
    return x * clData.scale;
}

static inline int WINSCALE(int x)
{
    bool negative = false;
    int y, t = x;
    float f = (float)0.0;

    if (x == 0)
	return 0;

    if (t < 0) {
	negative = true;
	t = -t;
    }

    f = WINSCALE_f(t);
    y = (int) (f + (float)0.5);

    if (y < 1)
	y = 1;

    if (negative)
	y = -y;

    return y;
}

#define	UWINSCALE(x)	((unsigned)WINSCALE(x))

#define SCALEX(co) ((int) (WINSCALE(co) - WINSCALE(world.x)))
#define SCALEY(co) ((int) (WINSCALE(world.y + ext_view_height) - WINSCALE(co)))
#define X(co)	((int) ((co) - world.x))
#define Y(co)	((int) (world.y + ext_view_height - (co)))


/*
 * Prototypes from the paint*.c files.
 */

int Paint_init(void);
void Paint_cleanup(void);
void Paint_shots(void);
void Paint_ships(void);
void Paint_radar(void);
void Paint_sliding_radar(void);
void Paint_world_radar(void);
void Radar_show_target(int x, int y);
void Radar_hide_target(int x, int y);
void Paint_vcannon(void);
void Paint_vfuel(void);
void Paint_vbase(void);
void Paint_vdecor(void);
void Paint_objects(void);
void Paint_world(void);
void Paint_score_table(void);
void Paint_score_entry(int entry_num, other_t *other, bool is_team);
void Paint_score_start(void);
void Paint_score_objects(void);
void Paint_meters(void);
void Paint_HUD(void);
int  Get_message(int *pos, char *message, int req_length, int key);
void Paint_messages(void);
void Paint_recording(void);
void Paint_HUD_values(void);
void Paint_frame(void);
void Paint_frame_start(void);
int Team_color(int);
int Life_color(other_t *other);
int Life_color_by_life(int life);
void Play_beep(void);
int Check_view_dimensions(void);
void Store_hud_options(void);
void Store_paintradar_options(void);

#endif
