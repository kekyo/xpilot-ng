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
 * Copyright (C) 2000-2001 Juha Lindström <juhal@users.sourceforge.net>
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

#include "xpclient_x11.h"

Window	radarWindow;
Pixmap	radarPixmap, radarPixmap2;
				/* Pixmaps for the radar (implements */
				/* the planes hack on the radar for */
				/* monochromes) */
long	dpl_1[2], dpl_2[2];	/* Used by radar hack */
int	radar_exposures;
int	(*radarDrawRectanglePtr)	/* Function to draw player on radar */
	(Display *disp, Drawable d, GC gc,
	 int x, int y, unsigned width, unsigned height);


static int slidingradar_x;	/* sliding radar offsets for windows */
static int slidingradar_y;

static int wallRadarColor;	/* Color index for walls on radar. */
static int targetRadarColor;	/* Color index for targets on radar. */
static int decorRadarColor;	/* Color index for decorations on radar. */


static void Copy_static_radar(void)
{
    if (radarPixmap2 != radarPixmap) {
	/* Draw static radar onto radar */
	XCopyArea(dpy, radarPixmap2, radarPixmap, gameGC,
		  0, 0, 256, RadarHeight, 0, 0);
    } else {
	/* Clear radar */
	XSetForeground(dpy, radarGC, colors[BLACK].pixel);
	XFillRectangle(dpy, radarPixmap,
		       radarGC, 0, 0, 256, RadarHeight);
    }
    XSetForeground(dpy, radarGC, colors[WHITE].pixel);
}

static void Paint_checkpoint_radar(double xf, double yf)
{
    int			x, y;
    XPoint		points[5];

    if (BIT(Setup->mode, TIMING)) {
	if (oldServer) {
	    Check_pos_by_index(nextCheckPoint, &x, &y);
	    x = ((int) (x * BLOCK_SZ * xf + 0.5)) - slidingradar_x;
	    y = (RadarHeight - (int) (y * BLOCK_SZ * yf + 0.5) + DSIZE -
		 1) - slidingradar_y;
	} else {
	    irec_t b = checks[nextCheckPoint].bounds;
	    x = (int) (b.x * xf + 0.5) - slidingradar_x;
	    y = (RadarHeight - (int) (b.y * yf + 0.5) + DSIZE - 1) -
		slidingradar_y;
	}
	if (x <= 0)
	    x += 256;
	if (y <= 0)
	    y += RadarHeight;

	/* top */
	points[0].x = x ;
	points[0].y = y ;
	/* right */
	points[1].x = x + DSIZE ;
	points[1].y = y - DSIZE ;
	/* bottom */
	points[2].x = x ;
	points[2].y = y - 2*DSIZE ;
	/* left */
	points[3].x = x - DSIZE ;
	points[3].y = y - DSIZE ;
	/* top */
	points[4].x = x ;
	points[4].y = y ;
	XDrawLines(dpy, radarPixmap, radarGC,
		   points, 5, 0);
    }
}

static void Paint_self_radar(double xf, double yf)
{
    int		x, y, x_1, y_1, xw, yw;

    if (selfVisible != 0 && loops % 16 < 13) {
	x = (int)(selfPos.x * xf + 0.5) - slidingradar_x;
	y = RadarHeight - (int)(selfPos.y * yf + 0.5) - 1 - slidingradar_y;
	if (x <= 0)
	    x += 256;
	if (y <= 0)
	    y += RadarHeight;

	x_1 = (int)(x + 8 * tcos(heading));
	y_1 = (int)(y - 8 * tsin(heading));
	XDrawLine(dpy, radarPixmap, radarGC,
		  x, y, x_1, y_1);
	if (BIT(Setup->mode, WRAP_PLAY)) {
	    xw = x_1 - (x_1 + 256) % 256;
	    yw = y_1 - (y_1 + RadarHeight) % RadarHeight;
	    if (xw != 0)
		XDrawLine(dpy, radarPixmap, radarGC,
			  x - xw, y, x_1 - xw, y_1);
	    if (yw != 0) {
		XDrawLine(dpy, radarPixmap, radarGC,
			  x, y - yw, x_1, y_1 - yw);
		if (xw != 0)
		    XDrawLine(dpy, radarPixmap, radarGC,
			      x - xw, y - yw, x_1 - xw, y_1 - yw);
	    }
	}
    }
}

static void Paint_objects_radar(void)
{
    int			i, x, y, xw, yw, color;

    for (i = 0; i < num_radar; i++) {
	int rs = radar_ptr[i].size;
	unsigned s = (rs <= 0 ? 1 : radar_ptr[i].size);

	color = WHITE;
	if (radar_ptr[i].type == RadarFriend) {
	    if (maxColors > 4)
		color = 4;
	    else if (!colorSwitch)
		color = RED;
	}
	XSetForeground(dpy, radarGC, colors[color].pixel);
	x = radar_ptr[i].x - s / 2 - slidingradar_x;
	y = RadarHeight - radar_ptr[i].y - 1 - s / 2 - slidingradar_y;

	if (x <= 0)
	    x += 256;
	if (y <= 0)
	    y += RadarHeight;

	(*radarDrawRectanglePtr)(dpy, radarPixmap, radarGC, x, y, s, s);
	if (BIT(Setup->mode, WRAP_PLAY)) {
	    xw = (x < 0) ? -256 : (x + s >= 256) ? 256 : 0;
	    yw = (y < 0) ? -RadarHeight
			     : (y + s >= RadarHeight) ? RadarHeight : 0;
	    if (xw != 0)
		(*radarDrawRectanglePtr)(dpy, radarPixmap, radarGC,
					 x - xw, y, s, s);
	    if (yw != 0) {
		(*radarDrawRectanglePtr)(dpy, radarPixmap, radarGC,
					 x, y - yw, s, s);

		if (xw != 0)
		    (*radarDrawRectanglePtr)(dpy, radarPixmap, radarGC,
					     x - xw, y - yw, s, s);
	    }
	}
	/*XSetForeground(dpy, radarGC, colors[WHITE].pixel);*/
    }
    if (num_radar)
	RELEASE(radar_ptr, num_radar, max_radar);
}


void Paint_radar(void)
{
    const double	xf = 256.0 / (double)Setup->width,
			yf = (double)RadarHeight / (double)Setup->height;

    if (radar_exposures == 0)
	return;

    slidingradar_x = 0;
    slidingradar_y = 0;

    Copy_static_radar();

    /* Checkpoints */
    Paint_checkpoint_radar(xf, yf);

    Paint_self_radar(xf, yf);
    Paint_objects_radar();
}


void Paint_sliding_radar(void)
{
    if (!Setup)
	return;

    if (BIT(Setup->mode, WRAP_PLAY) == 0)
	return;

    if (radarPixmap != radarPixmap2)
	return;

    if (instruments.slidingRadar) {
	if (radarPixmap2 != radarWindow)
	    return;

	radarPixmap2 = XCreatePixmap(dpy, radarWindow,
				256, RadarHeight,
				dispDepth);
	radarPixmap = radarPixmap2;
	if (radar_exposures > 0)
	    Paint_world_radar();
    } else {
	if (radarPixmap2 == radarWindow)
	    return;
	XFreePixmap(dpy, radarPixmap2);
	radarPixmap2 = radarWindow;
	radarPixmap = radarWindow;
	if (radar_exposures > 0)
	    Paint_world_radar();
    }
}

/*
 * Try and draw an area of the radar which represents block position
 * 'xi' 'yi'.  If 'draw' is zero the area is cleared.
 */
static void Paint_radar_block(int xi, int yi, int color)
{
    double	xs, ys;
    int		xp, yp, xw, yw;

    if (radarPixmap2 == radarPixmap) {
	XSetPlaneMask(dpy, radarGC, AllPlanes & ~(dpl_1[0] | dpl_1[1]));
    }
    XSetForeground(dpy, radarGC, colors[color].pixel);

    if (Setup->x >= 256) {
	xs = (double)(256 - 1) / (Setup->x - 1);
	ys = (double)(RadarHeight - 1) / (Setup->y - 1);
	xp = (int)(xi * xs + 0.5);
	yp = RadarHeight - 1 - (int)(yi * ys + 0.5);
	XDrawPoint(dpy, radarPixmap2, radarGC, xp, yp);
    } else {
	xs = (double)(Setup->x - 1) / (256 - 1);
	ys = (double)(Setup->y - 1) / (RadarHeight - 1);
	/*
	 * Calculate the min and max points on the radar that would show
	 * block position 'xi' and 'yi'.  Note 'xp' is the minimum x coord
	 * for 'xi',which is one more than the previous xi value would give,
	 * and 'xw' is the maximum, which is then changed to a width value.
	 * Similarly for 'yw' and 'yp' (the roles are reversed because the
	 * radar is upside down).
	 */
	xp = (int)((xi - 0.5) / xs) + 1;
	xw = (int)((xi + 0.5) / xs);
	yw = (int)((yi - 0.5) / ys) + 1;
	yp = (int)((yi + 0.5) / ys);
	xw -= xp;
	yw = yp - yw;
	yp = RadarHeight - 1 - yp;
	XFillRectangle(dpy, radarPixmap2, radarGC, xp, yp,
		       (unsigned)xw+1, (unsigned)yw+1);
    }
    if (radarPixmap2 == radarPixmap)
	XSetPlaneMask(dpy, radarGC,
		      AllPlanes & ~(dpl_2[0] | dpl_2[1]));
}

static void Paint_world_radar_old(void)
{
    int			i, xi, yi, xm, ym, xp, yp = 0;
    int			xmoff, xioff;
    int			type, vis;
    double		damage;
    double		xs, ys;
    int			npoint = 0, nsegment = 0;
    int			start, end;
    int			currColor, visibleColorChange;
    const int		max = 256;
    u_byte		visible[256];
    u_byte		visibleColor[256];
    XSegment		segments[256];
    XPoint		points[256];

    radar_exposures = 2;

    if (radarPixmap2 == radarPixmap)
	XSetPlaneMask(dpy, radarGC,
		      AllPlanes & ~(dpl_1[0] | dpl_1[1]));

    if (radarPixmap2 != radarWindow) {
	/* Clear radar */
	XSetForeground(dpy, radarGC, colors[BLACK].pixel);
	XFillRectangle(dpy, radarPixmap2, radarGC, 0, 0, 256, RadarHeight);
    } else
	XClearWindow(dpy, radarWindow);

    /*
     * Calculate an array which is later going to be indexed
     * by map block type.  The indexing should return a true
     * value when the map block should be visible on the radar
     * and a false value otherwise.
     */
    memset(visible, 0, sizeof visible);
    visible[SETUP_FILLED] = 1;
    visible[SETUP_FILLED_NO_DRAW] = 1;
    visible[SETUP_REC_LU] = 1;
    visible[SETUP_REC_RU] = 1;
    visible[SETUP_REC_LD] = 1;
    visible[SETUP_REC_RD] = 1;
    visible[SETUP_FUEL] = 1;
    for (i = 0; i < 10; i++)
	visible[SETUP_TARGET+i] = 1;

    for (i = BLUE_BIT; i < (int)sizeof visible; i++)
	visible[i] = 1;

    if (instruments.showDecor) {
	visible[SETUP_DECOR_FILLED] = 1;
	visible[SETUP_DECOR_LU] = 1;
	visible[SETUP_DECOR_RU] = 1;
	visible[SETUP_DECOR_LD] = 1;
	visible[SETUP_DECOR_RD] = 1;
    }

    /*
     * Calculate an array which returns the color to use
     * for drawing when indexed with a map block type.
     */
    memset(visibleColor, 0, sizeof visibleColor);
    visibleColor[SETUP_FILLED] =
	visibleColor[SETUP_FILLED_NO_DRAW] =
	visibleColor[SETUP_REC_LU] =
	visibleColor[SETUP_REC_RU] =
	visibleColor[SETUP_REC_LD] =
	visibleColor[SETUP_REC_RD] =
	visibleColor[SETUP_FUEL] = wallRadarColor;
    for (i = 0; i < 10; i++)
	visibleColor[SETUP_TARGET+i] = targetRadarColor;

    for (i = BLUE_BIT; i < (int)sizeof visible; i++)
	visibleColor[i] = wallRadarColor;

    if (instruments.showDecor)
	visibleColor[SETUP_DECOR_FILLED] =
	    visibleColor[SETUP_DECOR_LU] =
	    visibleColor[SETUP_DECOR_RU] =
	    visibleColor[SETUP_DECOR_LD] =
	    visibleColor[SETUP_DECOR_RD] = decorRadarColor;

    /* The following code draws the map on the radar.  Segments and
     * points arrays are use to build lists of things to be drawn.
     * Normally the segments and points are drawn when the arrays are
     * full, but now they are also drawn when the color changes.  The
     * visibleColor array is used to determine the color to be used
     * for the given visible block type.
     *
     * Another (and probably better) way to do this would be use
     * different segments and points arrays for each visible color.
     */
    if (Setup->x >= 256) {
	xs = (double)(256 - 1) / (Setup->x - 1);
	ys = (double)(RadarHeight - 1) / (Setup->y - 1);
	currColor = -1;
	for (xi = 0; xi < Setup->x; xi++) {
	    start = end = -1;
	    xp = (int)(xi * xs + 0.5);
	    xioff = xi * Setup->y;
	    for (yi = 0; yi < Setup->y; yi++) {
		visibleColorChange = 0;
		type = Setup->map_data[xioff + yi];
		if (type >= SETUP_TARGET && type < SETUP_TARGET + 10)
		    vis = (Target_alive(xi, yi, &damage) == 0);
		else
		    vis = visible[type];

		if (vis) {
		    yp = (int)(yi * ys + 0.5);
		    if (start == -1) {
			if ((nsegment > 0 || npoint > 0)
			    && currColor != visibleColor[type]) {
			    if (nsegment > 0) {
				XDrawSegments(dpy, radarPixmap2, radarGC,
					      segments, nsegment);
				nsegment = 0;
			    }
			    if (npoint > 0) {
				XDrawPoints(dpy, radarPixmap2, radarGC,
					    points, npoint, CoordModeOrigin);
				npoint = 0;
			    }
			}
			start = end = yp;
			currColor = visibleColor[type];
			XSetForeground(dpy, radarGC, colors[currColor].pixel);
		    } else {
			end = yp;
			visibleColorChange = (visibleColor[type] != currColor);
		    }
		}

		if (start != -1
		    && (!vis || yi == Setup->y - 1 || visibleColorChange)) {
		    if (end > start) {
			segments[nsegment].x1 = xp;
			segments[nsegment].y1 = RadarHeight - 1 - start;
			segments[nsegment].x2 = xp;
			segments[nsegment].y2 = RadarHeight - 1 - end;
			nsegment++;
			if (nsegment >= max || yi == Setup->y - 1) {
			    XDrawSegments(dpy, radarPixmap2, radarGC,
					  segments, nsegment);
			    nsegment = 0;
			}
		    } else {
			points[npoint].x = xp;
			points[npoint].y = RadarHeight - 1 - start;
			npoint++;
			if (npoint >= max || yi == Setup->y - 1) {
			    XDrawPoints(dpy, radarPixmap2, radarGC,
					points, npoint, CoordModeOrigin);
			    npoint = 0;
			}
		    }
		    start = end = -1;
		}

		if (visibleColorChange) {
		    if (nsegment > 0) {
			XDrawSegments(dpy, radarPixmap2, radarGC,
				      segments, nsegment);
			nsegment = 0;
		    }
		    if (npoint > 0) {
			XDrawPoints(dpy, radarPixmap2, radarGC,
				    points, npoint, CoordModeOrigin);
			npoint = 0;
		    }
		    start = end = yp;
		    currColor = visibleColor[type];
		    XSetForeground(dpy, radarGC, colors[currColor].pixel);
		}
	    }
	}
    } else {
	xs = (double)(Setup->x - 1) / (256 - 1);
	ys = (double)(Setup->y - 1) / (RadarHeight - 1);
	currColor = -1;
	for (xi = 0; xi < 256; xi++) {
	    xm = (int)(xi * xs + 0.5);
	    xmoff = xm * Setup->y;
	    start = end = -1;
	    xp = xi;
	    for (yi = 0; yi < (int)RadarHeight; yi++) {
		visibleColorChange = 0;
		ym = (int)(yi * ys + 0.5);
		type = Setup->map_data[xmoff + ym];
		vis = visible[type];
		if (type >= SETUP_TARGET && type < SETUP_TARGET + 10)
		    vis = (Target_alive(xm, ym, &damage) == 0);
		if (vis) {
		    yp = yi;
		    if (start == -1) {
			if ((nsegment > 0 || npoint > 0)
			    && currColor != visibleColor[type]) {
			    if (nsegment > 0) {
				XDrawSegments(dpy, radarPixmap2, radarGC,
					      segments, nsegment);
				nsegment = 0;
			    }
			    if (npoint > 0) {
				XDrawPoints(dpy, radarPixmap2, radarGC,
					    points, npoint, CoordModeOrigin);
				npoint = 0;
			    }
			}
			start = end = yp;
			currColor = visibleColor[type];
			XSetForeground(dpy, radarGC, colors[currColor].pixel);
		    } else {
			end = yp;
			visibleColorChange = visibleColor[type] != currColor;
		    }
		}

		if (start != -1
		    && (!vis || yi == (int)RadarHeight - 1
			|| visibleColorChange)) {
		    if (end > start) {
			segments[nsegment].x1 = xp;
			segments[nsegment].y1 = RadarHeight - 1 - start;
			segments[nsegment].x2 = xp;
			segments[nsegment].y2 = RadarHeight - 1 - end;
			nsegment++;
			if (nsegment >= max || yi == (int)RadarHeight - 1) {
			    XDrawSegments(dpy, radarPixmap2, radarGC,
					  segments, nsegment);
			    nsegment = 0;
			}
		    } else {
			points[npoint].x = xp;
			points[npoint].y = RadarHeight - 1 - start;
			npoint++;
			if (npoint >= max || yi == (int)RadarHeight - 1) {
			    XDrawPoints(dpy, radarPixmap2, radarGC,
					points, npoint, CoordModeOrigin);
			    npoint = 0;
			}
		    }
		    start = end = -1;
		}

		if (visibleColorChange) {
		    if (nsegment > 0) {
			XDrawSegments(dpy, radarPixmap2, radarGC,
				      segments, nsegment);
			nsegment = 0;
		    }
		    if (npoint > 0) {
			XDrawPoints(dpy, radarPixmap2, radarGC,
				    points, npoint, CoordModeOrigin);
			npoint = 0;
		    }
		    start = end = yp;
		    currColor = visibleColor[type];
		    XSetForeground(dpy, radarGC, colors[currColor].pixel);
		}
	    }
	}
    }
    if (nsegment > 0)
	XDrawSegments(dpy, radarPixmap2, radarGC, segments, nsegment);

    if (npoint > 0)
	XDrawPoints(dpy, radarPixmap2, radarGC,
		    points, npoint, CoordModeOrigin);

    if (radarPixmap2 == radarPixmap)
	XSetPlaneMask(dpy, radarGC,
		      AllPlanes & ~(dpl_2[0] | dpl_2[1]));

    for (i = 0;; i++) {
	int dead_time;
	double targ_damage;

	if (Target_by_index(i, &xi, &yi, &dead_time, &targ_damage) == -1)
	    break;
	if (dead_time)
	    continue;
	Paint_radar_block(xi, yi, targetRadarColor);
    }
}


static void Compute_radar_bounds(ipos_t *min, ipos_t *max, const irec_t *b)
{
    min->x = (0 - (b->x + b->w)) / Setup->width;
    if (0 > b->x + b->w) min->x++;
    max->x = (0 + Setup->width - b->x) / Setup->width;
    if (0 + Setup->width < b->x) max->x--;
    min->y = (0 - (b->y + b->h)) / Setup->height;
    if (0 > b->y + b->h) min->y++;
    max->y = (0 + Setup->height - b->y) / Setup->height;
    if (0 + Setup->height < b->y) max->y--;
}

static void Paint_world_radar_new(void)
{
    int i, j, xoff, yoff;
    ipos_t min, max;
    static XPoint poly[10000];
    
    /* what the heck is this? */
    radar_exposures = 2;

    if (radarPixmap2 == radarPixmap)
	XSetPlaneMask(dpy, radarGC, AllPlanes & (~(dpl_1[0] | dpl_1[1])));

    if (radarPixmap2 != radarWindow) {
	/* Clear radar */
	XSetForeground(dpy, radarGC, colors[BLACK].pixel);
	XFillRectangle(dpy, radarPixmap2, radarGC, 0, 0, 256, RadarHeight);
    } else
	XClearWindow(dpy, radarWindow);
        
    XSetForeground(dpy, radarGC, colors[wallRadarColor].pixel);
    
    /* loop through all the polygons */
    for (i = 0; i < num_polygons; i++) {
	if (BIT(polygon_styles[polygons[i].style].flags,
		STYLE_INVISIBLE_RADAR)) continue;
	Compute_radar_bounds(&min, &max, &polygons[i].bounds);
	for (xoff = min.x; xoff <= max.x; xoff++) {
	    for (yoff = min.y; yoff <= max.y; yoff++) {
		int x, y;

		x = xoff * Setup->width;
		y = yoff * Setup->height;
		
		/* loop through the points in the current polygon */
		for (j = 0; j < polygons[i].num_points; j++) {		    
		    x += polygons[i].points[j].x;
		    y += polygons[i].points[j].y;
		    poly[j].x = (x * 256) / Setup->width;
		    poly[j].y = (int)RadarHeight 
			- ((y * (int)RadarHeight) / Setup->height);
		}

		XSetForeground(dpy, radarGC, fullColor ?
			       polygon_styles[polygons[i].style].color :
			       colors[wallRadarColor].pixel);
		XFillPolygon(dpy, radarPixmap2, radarGC, poly,
			     polygons[i].num_points,
			     Nonconvex, CoordModeOrigin);
	    }
	}
    }
    
    if (radarPixmap2 == radarPixmap)
	XSetPlaneMask(dpy, radarGC, AllPlanes & (~(dpl_2[0] | dpl_2[1])));
}

void Paint_world_radar(void)
{
    if (oldServer)
	Paint_world_radar_old();
    else
	Paint_world_radar_new();
}

void Radar_show_target(int x, int y)
{
    Paint_radar_block(x, y, targetRadarColor);
}

void Radar_hide_target(int x, int y)
{
    Paint_radar_block(x, y, BLACK);
}


static bool Set_wallRadarColor(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    wallRadarColor = value;
    return true;
}

static bool Set_decorRadarColor(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    decorRadarColor = value;
    return true;
}

static bool Set_targetRadarColor(xp_option_t *opt, int value)
{
    UNUSED_PARAM(opt);

    targetRadarColor = value;
    return true;
}


static xp_option_t paintradar_options[] = {

    COLOR_INDEX_OPTION_WITH_SETFUNC(
	"wallRadarColor",
	BLUE,
	&wallRadarColor,
	Set_wallRadarColor,
	"Which color number to use for drawing walls on the radar.\n"
	"Valid values all even numbers smaller than maxColors.\n"),

    COLOR_INDEX_OPTION_WITH_SETFUNC(
	"decorRadarColor",
	6,
	&decorRadarColor,
	Set_decorRadarColor,
	"Which color number to use for drawing decorations on the radar.\n"
	"Valid values are all even numbers smaller than maxColors.\n"),

    COLOR_INDEX_OPTION_WITH_SETFUNC(
	"targetRadarColor",
	4,
	&targetRadarColor,
	Set_targetRadarColor,
	"Which color number to use for drawing targets on the radar.\n"
	"Valid values are all even numbers smaller than maxColors.\n"),
};

void Store_paintradar_options(void)
{
    STORE_OPTIONS(paintradar_options);
}
