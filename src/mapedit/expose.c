/*
 * XPilot NG XP-MapEdit, a map editor for xp maps.  Copyright (C) 1993 by
 *
 *      Aaron Averill           <averila@oes.orst.edu>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modifications:
 * 1996:
 *      Robert Templeman        <mbcaprt@mphhpd.ph.man.ac.uk>
 * 1997:
 *      William Docter          <wad2@lehigh.edu>
 */

#include "xpmapedit.h"

typedef struct {
    int color;
    int num_points;
    float x[5], y[5];
} segment_t;

typedef struct {
    float x, y, width, height;
    int start, end;
} arc_t;

int smlmap_x, smlmap_y, smlmap_width, smlmap_height;
float smlmap_xscale, smlmap_yscale;

segment_t mapicon_seg[35] = {
    {0, 5, {0.00, 1.00, 1.00, 0.00, 0.00}, {1.00, 1.00, 0.00, 0.00, 1.00}},	/*  0:x MAP_WALL */
    {0, 4, {1.00, 1.00, 0.00, 1.00, 0.00}, {0.00, 1.00, 1.00, 0.00, 0.00}},	/*  1:q */
    {0, 4, {0.00, 0.00, 1.00, 0.00, 0.00}, {0.00, 1.00, 1.00, 0.00, 0.00}},	/*  2:w */
    {0, 4, {0.00, 1.00, 1.00, 0.00, 0.00}, {0.00, 0.00, 1.00, 0.00, 0.00}},	/*  3:a */
    {0, 4, {0.00, 1.00, 0.00, 0.00, 0.00}, {0.00, 0.00, 1.00, 0.00, 0.00}},	/*  4:s */

    {0, 5, {0.00, 1.00, 1.00, 0.00, 0.00}, {1.00, 1.00, 0.00, 0.00, 1.00}},	/*  5:# XPMAP_FUEL */

    {1, 4, {0.70, 0.99, 0.99, 0.70, 0.00}, {0.50, 0.99, 0.01, 0.50, 0.00}},	/*  6:d MAP_CANNON */
    {1, 4, {0.01, 0.99, 0.50, 0.01, 0.00}, {0.99, 0.99, 0.70, 0.99, 0.00}},	/*  7:r MAP_CANNON */
    {1, 4, {0.50, 0.01, 0.99, 0.50, 0.00}, {0.30, 0.01, 0.01, 0.30, 0.00}},	/*  8:c MAP_CANNON */
    {1, 4, {0.30, 0.01, 0.01, 0.30, 0.00}, {0.50, 0.99, 0.01, 0.50, 0.00}},	/*  9:f MAP_CANNON */
    {1, 2, {0.01, 0.99, 0.00, 0.00, 0.00}, {0.99, 0.99, 0.00, 0.00, 0.00}},	/* 10:_ MAP_BASE */

    {2, 5, {0.50, 0.50, 0.50, 0.30, 0.70}, {0.30, 0.70, 0.50, 0.50, 0.50}},	/* 11:+ XPMAP_POS_GRAV */
    {2, 2, {0.30, 0.70, 0.00, 0.00, 0.00}, {0.50, 0.50, 0.00, 0.00, 0.00}},	/* 12:- MAP_GRAV_NET */
    {2, 0, {0.00, 0.00, 0.00, 0.00, 0.00}, {0.00, 0.00, 0.00, 0.00, 0.00}},	/* 13:@ XPMAP_WORMHOLE_BOTH */
    {2, 5, {0.10, 0.70, 0.30, 0.70, 0.70}, {0.10, 0.70, 0.70, 0.70, 0.30}},	/* 14:) XPMAP_WORMHOLE_ */
    {2, 5, {0.70, 0.10, 0.30, 0.10, 0.10}, {0.70, 0.10, 0.10, 0.10, 0.30}},	/* 15:( XPMAP_WORMHOLE_ */
    {2, 4, {0.05, 0.05, 0.95, 0.95, 0.00}, {0.50, 0.95, 0.95, 0.50, 0.00}},	/* 16:* XPMAP_TREASURE */
    {2, 3, {0.60, 0.50, 0.60, 0.00, 0.00}, {0.05, 0.15, 0.30, 0.00, 0.00}},	/* 17:< */
    {2, 3, {0.40, 0.50, 0.40, 0.00, 0.00}, {0.05, 0.15, 0.30, 0.00, 0.00}},	/* 18:> */
    {2, 5, {0.05, 0.95, 0.95, 0.05, 0.05}, {0.05, 0.05, 0.95, 0.95, 0.05}},	/* 19:! XPMAP_TARGET */

    {1, 0, {0.00, 0.00, 0.00, 0.00, 0.00}, {0.00, 0.00, 0.00, 0.00, 0.00}},	/* 20:  XPMAP_SPACE */

    {3, 5, {0.00, 1.00, 1.00, 0.00, 0.00}, {1.00, 1.00, 0.00, 0.00, 1.00}},	/* 21:b MAP_DECORATION */
    {3, 4, {1.00, 1.00, 0.00, 1.00, 0.00}, {0.00, 1.00, 1.00, 0.00, 0.00}},	/* 22:t */
    {3, 4, {0.00, 0.00, 1.00, 0.00, 0.00}, {0.00, 1.00, 1.00, 0.00, 0.00}},	/* 23:y */
    {3, 4, {0.00, 1.00, 1.00, 0.00, 0.00}, {0.00, 0.00, 1.00, 0.00, 0.00}},	/* 24:g */
    {3, 4, {0.00, 1.00, 0.00, 0.00, 0.00}, {0.00, 0.00, 1.00, 0.00, 0.00}},	/* 25:h */

    {2, 0, {0.00, 0.00, 0.00, 0.00, 0.00}, {0.00, 0.00, 0.00, 0.00, 0.00}},	/* 26:$ BASE ORIENT */
    {2, 4, {0.30, 0.70, 0.50, 0.30, 0.00}, {0.15, 0.15, 0.80, 0.15, 0.00}},	/* 27:% ITEM CONCENTRATOR */
    {2, 5, {0.50, 0.50, 0.75, 0.50, 0.25}, {0.99, 0.01, 0.50, 0.01, 0.50}},	/* 28:i CURRENT UP */
    {2, 5, {0.99, 0.01, 0.50, 0.01, 0.50}, {0.50, 0.50, 0.75, 0.50, 0.25}},	/* 29:j CURRENT LEFT */
    {2, 5, {0.01, 0.99, 0.50, 0.99, 0.50}, {0.50, 0.50, 0.75, 0.50, 0.25}},	/* 30:k CURRENT RIGHT */
    {2, 5, {0.50, 0.50, 0.75, 0.50, 0.25}, {0.01, 0.99, 0.50, 0.99, 0.50}},	/* 31:m CURRENT DOWN */
    {2, 4, {0.05, 0.05, 0.95, 0.95, 0.00}, {0.50, 0.95, 0.95, 0.50, 0.00}},	/* 32:^ XPMAP_EMPTY_TREASURE */
    {0, 5, {0.00, 1.00, 1.00, 0.00, 0.00}, {1.00, 1.00, 0.00, 0.00, 1.00}},	/* 33:z XPMAP_FRICTION_AREA */
    {0, 5, {0.10, 0.85, 0.85, 0.10, 0.10}, {0.85, 0.85, 0.10, 0.10, 0.85}},	/* 34:& XPMAP_FRICTION_AREA */
};

segment_t mapicondet_seg[4] = {
    {0, 5, {0.25, 0.75, 0.75, 0.25, 0.25}, {0.75, 0.75, 0.25, 0.25, 0.75}},	/* 0:! TARGET (DETAIL) */
    {0, 5, {0.25, 0.75, 0.50, 0.25, 0.75}, {0.25, 0.75, 0.50, 0.75, 0.25}},	/* 1:! TARGET (DETAIL) */
    {0, 5, {0.20, 0.70, 0.70, 0.20, 0.20}, {0.70, 0.70, 0.20, 0.20, 0.70}},	/* 2:& ASTEROID_CONC (DETAIL) */
    {0, 5, {0.35, 0.65, 0.65, 0.35, 0.35}, {0.60, 0.60, 0.30, 0.30, 0.60}},	/* 3:& ASTEROID_CONC (DETAIL) */
};

int mapicon_ptr[91] = {
    20, 19, 0, 5, 26,	/* ascii char -32 */
    27, 34, 0, 15, 14,
    16, 11, 0, 12, 0,
    0, 10, 10, 10, 10,
    10, 10, 10, 10, 10,
    10, 0, 0, 17, 0,
    18, 0, 13, 20, 20,
    20, 20, 20, 20, 20,
    20, 20, 20, 20, 20,
    20, 20, 20, 20, 20,
    20, 20, 20, 20, 20,
    20, 20, 20, 20, 0,
    0, 0, 32, 10, 0,
    3, 21, 8, 6, 0,
    9, 24, 25, 28, 29,
    30, 0, 31, 0, 0,
    0, 1, 7, 4, 22,
    0, 0, 2, 0, 23,
    33
};

char iconmenu[36] = { ' ',
    XPMAP_FILLED, XPMAP_REC_RD, XPMAP_REC_LD, XPMAP_REC_RU, XPMAP_REC_LU,
    XPMAP_DECOR_FILLED, XPMAP_DECOR_RD, XPMAP_DECOR_LD, XPMAP_DECOR_RU, XPMAP_DECOR_LU,
    XPMAP_FUEL, XPMAP_CANNON_LEFT, XPMAP_CANNON_RIGHT, XPMAP_CANNON_UP, XPMAP_CANNON_DOWN,
    XPMAP_TARGET, XPMAP_TREASURE, XPMAP_EMPTY_TREASURE, XPMAP_ACWISE_GRAV,
	XPMAP_CWISE_GRAV,
    XPMAP_WORMHOLE_NORMAL, XPMAP_WORMHOLE_IN, XPMAP_WORMHOLE_OUT, XPMAP_POS_GRAV, XPMAP_NEG_GRAV,
    XPMAP_UP_GRAV, XPMAP_LEFT_GRAV, XPMAP_RIGHT_GRAV, XPMAP_DOWN_GRAV, XPMAP_ASTEROID_CONCENTRATOR,
    XPMAP_BASE, XPMAP_BASE_ATTRACTOR, XPMAP_ITEM_CONCENTRATOR, XPMAP_FRICTION_AREA, XPMAP_SPACE
};



/***************************************************************************/
/* DrawTools                                                               */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void DrawTools(void)
{
    int i, j, w, sel;

    DrawSmallMap();
    /* draw button icons */
    w = (TOOLSWIDTH - 20) / 5;
    for (i = 0; i < 7; i++)
	for (j = 0; j < 5; j++) {
	    sel = i * 5 + j + 1;
	    if (sel == drawicon)
		DrawMapPic(mapwin, 14 + j * w, 9 + i * w,
			   mapicon_ptr[iconmenu[sel] - 32], w - 7);
	    else
		DrawMapPic(mapwin, 13 + j * w, 8 + i * w,
			   mapicon_ptr[iconmenu[sel] - 32], w - 7);
	}
}

/***************************************************************************/
/* DrawMap                                                                 */
/* Arguments :                                                             */
/*    x                                                                    */
/*   y                                                                     */
/*   width                                                                 */
/*   height                                                                */
/* Purpose :                                                               */
/***************************************************************************/
void DrawMap(int x, int y, int width, int height)
{
    int x1, y_1;

    x -= TOOLSWIDTH;
    if (x < 0) {
	x = 0;
    } else if ((width + x) > (mapwin_width - TOOLSWIDTH)) {
	width = (mapwin_width - TOOLSWIDTH - x);
    }
    if (y < 0) {
	y = 0;
    } else if ((height + y) > mapwin_height) {
	height = (mapwin_height - y);
    }
    x1 = ((int) (x / map.view_zoom)) * map.view_zoom + TOOLSWIDTH;
    y_1 = ((int) (y / map.view_zoom)) * map.view_zoom;
    x /= map.view_zoom;
    y /= map.view_zoom;
    width /= map.view_zoom;
    height /= map.view_zoom;

    DrawMapSection(x + map.view_x, y + map.view_y, width, height, x1, y_1);
}

/***************************************************************************/
/* DrawMapSection                                                          */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/*   width                                                                 */
/*   height                                                                */
/*   xpos                                                                  */
/*   ypos                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
void DrawMapSection(int x, int y, int width, int height, int xpos,
		    int ypos)
{
    int i, j, w, h, data;
    char strng[2];

    if (x < 0) {
	DrawMapSection(map.width + x, y, (0 - x), height, xpos, ypos);
	DrawMapSection(0, y, width + x, height, xpos - (x * map.view_zoom),
		       ypos);
	return;
    }
    if (y < 0) {
	DrawMapSection(x, map.height + y, width, (0 - y), xpos, ypos);
	DrawMapSection(x, 0, width, height + y, xpos,
		       ypos - (y * map.view_zoom));
	return;
    }
    if ((x + width) > map.width) {
	DrawMapSection(x, y, map.width - x, height, xpos, ypos);
	DrawMapSection(0, y, x + width - map.width, height,
		       xpos + ((map.width - x) * map.view_zoom), ypos);
	return;
    }
    if ((y + height) > map.height) {
	DrawMapSection(x, y, width, map.height - y, xpos, ypos);
	DrawMapSection(x, 0, width, y + height - map.height, xpos,
		       ypos + ((map.height - y) * map.view_zoom));
	return;
    }

    w = width + x;
    h = height + y;
    for (i = x; i <= w; i++)
	for (j = y; j <= h; j++) {
	    if ((i < map.width) && (j < map.height)) {
		data = map.data[i][j];
		DrawMapPic(mapwin, (i - x) * map.view_zoom + xpos,
			   (j - y) * map.view_zoom + ypos,
			   mapicon_ptr[data - 32], map.view_zoom);
		if (((data > 47) && (data < 58))
		    || ((data > 64) && (data < 91))) {
		    strng[0] = data;
		    strng[1] = '\0';
		    T_DrawString(mapwin, (i - x) * map.view_zoom + xpos,
				 (j - y) * map.view_zoom + 1 + ypos,
				 map.view_zoom, map.view_zoom - 1,
				 White_GC, strng, JUSTIFY_CENTER,
				 CROP_RIGHT, -1);
		}
	    }
	}
}

/***************************************************************************/
/* DrawMapPic                                                              */
/* Arguments :                                                             */
/*   win                                                                   */
/*   x                                                                     */
/*   y                                                                     */
/*   picnum                                                                */
/*   zoom                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
void DrawMapPic(Window win, int x, int y, int picnum, int zoom)
{
    XPoint points[5];
    int i, arc, xo = 0, yo = 0;

    if (picnum == 20)
	return;
   /*-----------------------------------------------------------------*/
    if (picnum == 8) {
	yo = 1;
    } else if (picnum == 9) {
	xo = 1;
    }
   /*-----------------------------------------------------------------*/
    for (i = 0; i < mapicon_seg[picnum].num_points; i++) {
	points[i].x = mapicon_seg[picnum].x[i] * zoom + x + xo;
	points[i].y = mapicon_seg[picnum].y[i] * zoom + y + yo;
    }
    /*-----------------------------------------------------------------*/
    if ((picnum >= 0) && (picnum <= 4)) {	/* Walls */
	XDrawLines(display, win, Wall_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 5) {
	XDrawLines(display, win, Wall_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	XFillRectangle(display, win, Fuel_GC, x + 1, y + 1, zoom - 1,
		       zoom - 1);
	return;
    } else if ((picnum >= 6) && (picnum <= 9)) {	/* Cannons */
	XDrawLines(display, win, Cannon_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 10) {	/* Bases */
	XDrawLines(display, win, Base_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if ((picnum >= 11) && (picnum <= 12)) {	/* +/- gravity and wormholes */
	XDrawLines(display, win, Gravity_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
    } else if ((picnum >= 13) && (picnum <= 15)) {	/* +/- gravity and wormholes */
	XDrawLines(display, win, Wormhole_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
    } else if ((picnum == 16) || (picnum == 32)) {	/* Treasure or Empty treasure */
	XDrawLines(display, win, Treasure_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
    } else if ((picnum >= 17) && (picnum <= 18)) {	/* clockwise and anti clockwise gravity */
	XDrawLines(display, win, Gravity_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
    } else if (picnum == 19) {	/* Target */
	XDrawLines(display, win, Target_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	picnum = 0;
	for (i = 0; i < mapicon_seg[picnum].num_points; i++) {
	    points[i].x = mapicondet_seg[picnum].x[i] * zoom + x + xo;
	    points[i].y = mapicondet_seg[picnum].y[i] * zoom + y + yo;
	}
	XDrawLines(display, win, Target_GC, points,
		   mapicondet_seg[picnum].num_points, CoordModeOrigin);
	picnum = 1;
	for (i = 0; i < mapicondet_seg[picnum].num_points; i++) {
	    points[i].x = mapicondet_seg[picnum].x[i] * zoom + x + xo;
	    points[i].y = mapicondet_seg[picnum].y[i] * zoom + y + yo;
	}
	XDrawLines(display, win, Target_GC, points,
		   mapicondet_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 20) {	/* Space */
	return;
    } else if ((picnum >= 21) && (picnum <= 25)) {	/* Decorations */
	XDrawLines(display, win, Decor_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 26) {	/* Base Orient  */
	XDrawLines(display, win, Base_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
    } else if (picnum == 27) {	/* Item Concentrator */
	XDrawLines(display, win, Item_Conc_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if ((picnum >= 28) && (picnum <= 31)) {	/* Currents */
	XDrawLines(display, win, Current_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 33) {	/* Friction */
	XDrawLines(display, win, Friction_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	return;
    } else if (picnum == 34) {	/* Asteroid Conc */
	XDrawLines(display, win, Item_Conc_GC, points,
		   mapicon_seg[picnum].num_points, CoordModeOrigin);
	picnum = 2;
	for (i = 0; i < mapicondet_seg[picnum].num_points; i++) {
	    points[i].x = mapicondet_seg[picnum].x[i] * zoom + x + xo;
	    points[i].y = mapicondet_seg[picnum].y[i] * zoom + y + yo;
	}
	XDrawLines(display, win, Item_Conc_GC, points,
		   mapicondet_seg[picnum].num_points, CoordModeOrigin);

	picnum = 3;
	for (i = 0; i < mapicondet_seg[picnum].num_points; i++) {
	    points[i].x = mapicondet_seg[picnum].x[i] * zoom + x + xo;
	    points[i].y = mapicondet_seg[picnum].y[i] * zoom + y + yo;
	}
	XDrawLines(display, win, Item_Conc_GC, points,
		   mapicondet_seg[picnum].num_points, CoordModeOrigin);
	return;
    }
   /*---------------------------------------------------------------*/
    arc = (int) (.7 * zoom);
    if (((zoom - arc) / 2) * 2 != (zoom - arc))
	arc--;
    if ((picnum >= 11) && (picnum <= 12)) {	/* +/- Gravity */
	XDrawArc(display, win, Gravity_GC, (int) (x + (zoom - arc) / 2),
		 (int) (y + (zoom - arc) / 2), arc, arc, 0, 23040);
    } else if ((picnum >= 13) && (picnum <= 15)) {	/* Wormhole */
	XDrawArc(display, win, Wormhole_GC, (int) (x + (zoom - arc) / 2),
		 (int) (y + (zoom - arc) / 2), arc, arc, 0, 23040);
	XDrawArc(display, win, Wormhole_GC, (int) (x + .15 * zoom),
		 (int) (y + .15 * zoom), (int) (.4 * zoom),
		 (int) (.4 * zoom), 0, 23040);
	return;
    } else if ((picnum == 16) || (picnum == 32)) {	/* Treasure or empty treasure */
	XDrawArc(display, win, Treasure_GC, (int) (x + .05 * zoom),
		 (int) (y + .05 * zoom), (int) (.9 * zoom),
		 (int) (1 * zoom), 0, 11520);
	if (picnum == 16)
	    XDrawArc(display, win, White_GC, (int) (x + .15 * zoom),
		     (int) (y + .4 * zoom), (int) (.5 * zoom),
		     (int) (.5 * zoom), 0, 23040);
	return;
    } else if (picnum == 17) {	/* Gravity */
	XDrawArc(display, win, Gravity_GC, (int) (x + .15 * zoom),
		 (int) (y + .15 * zoom), (int) (.7 * zoom),
		 (int) (.7 * zoom), 5760, -18880);
	return;
    } else if (picnum == 18) {	/* Gravity */
	XDrawArc(display, win, Gravity_GC, (int) (x + .15 * zoom),
		 (int) (y + .15 * zoom), (int) (.7 * zoom),
		 (int) (.7 * zoom), 5760, 18880);
	return;
    } else if (picnum == 26) {	/* Born Facing */
	XDrawArc(display, win, Base_GC, (int) (x + .15 * zoom),
		 (int) (y + .15 * zoom), (int) (.7 * zoom),
		 (int) (.7 * zoom), 0, 23040);
	XDrawArc(display, win, Base_GC, (int) (x + .35 * zoom),
		 (int) (y + .35 * zoom), (int) (.05 * zoom),
		 (int) (.05 * zoom), 0, 23040);
	XDrawArc(display, win, Base_GC, (int) (x + .60 * zoom),
		 (int) (y + .35 * zoom), (int) (.05 * zoom),
		 (int) (.05 * zoom), 0, 23040);
	XDrawArc(display, win, Base_GC, (int) (x + .25 * zoom),
		 (int) (y + .15 * zoom), (int) (.5 * zoom),
		 (int) (.5 * zoom), 0, -12000);
	return;
    }
}

/***************************************************************************/
/* DrawSmallMap                                                            */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void DrawSmallMap(void)
{
    int i, j;

    T_ClearArea(smlmap_pixmap, 0, 0, TOOLSWIDTH, TOOLSWIDTH);
    T_DrawButton(smlmap_pixmap, smlmap_x - 5, smlmap_y - 5,
		 smlmap_width + 10, smlmap_height + 10, LOWERED, 0);
    XFillRectangle(display, smlmap_pixmap, Black_GC, smlmap_x, smlmap_y,
		   smlmap_width, smlmap_height);
    for (i = 0; i < smlmap_height; i++)
	for (j = 0; j < smlmap_width; j++)
	    switch (mapicon_ptr[map.data[(int) (j * smlmap_xscale)]
				[(int) (i * smlmap_yscale)] - 32]) {

	    case 0:
	    case 1:
	    case 2:
	    case 3:
	    case 4:
		XDrawPoint(display, smlmap_pixmap, Wall_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 5:
		XDrawPoint(display, smlmap_pixmap, Fuel_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 6:
	    case 7:
	    case 8:
	    case 9:
		XDrawPoint(display, smlmap_pixmap, Cannon_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 10:
		XDrawPoint(display, smlmap_pixmap, Base_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 11:
	    case 12:
		XDrawPoint(display, smlmap_pixmap, Gravity_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 13:
	    case 14:
	    case 15:
		XDrawPoint(display, smlmap_pixmap, Wormhole_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 16:
	    case 32:
		XDrawPoint(display, smlmap_pixmap, Treasure_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 17:
	    case 18:
		XDrawPoint(display, smlmap_pixmap, Gravity_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 19:
		XDrawPoint(display, smlmap_pixmap, Target_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 21:
	    case 22:
	    case 23:
	    case 24:
	    case 25:
		XDrawPoint(display, smlmap_pixmap, Decor_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 26:
		XDrawPoint(display, smlmap_pixmap, Base_GC, j + smlmap_x,
			   i + smlmap_y);
		break;
	    case 27:
		XDrawPoint(display, smlmap_pixmap, Item_Conc_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 28:
	    case 29:
	    case 30:
	    case 31:
		XDrawPoint(display, smlmap_pixmap, Current_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    case 33:
		XDrawPoint(display, smlmap_pixmap, Friction_GC,
			   j + smlmap_x, i + smlmap_y);
		break;
	    }

    XCopyArea(display, smlmap_pixmap, mapwin, White_GC, 1, 0,
	      TOOLSWIDTH - 2, TOOLSWIDTH, 1, TOOLSHEIGHT - TOOLSWIDTH);
    DrawViewBox();
}

/***************************************************************************/
/* UpdateSmallMap                                                          */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/* Purpose :                                                               */
/***************************************************************************/
void UpdateSmallMap(int x, int y)
{
    int x2, y2, i, j, xs, ys, data;

    DrawViewBox();
    x2 = x / smlmap_xscale;
    y2 = y / smlmap_yscale;
    if ((smlmap_xscale < 1) || (smlmap_yscale < 1)) {

	xs = (1 + x) / smlmap_xscale + 1.5;
	ys = (1 + y) / smlmap_yscale + 1.5;
	for (i = x2; i < xs; i++)
	    for (j = y2; j < ys; j++) {
		data = mapicon_ptr[map.data[(int) (i * smlmap_xscale)]
				   [(int) (j * smlmap_yscale)] - 32];
		switch (data) {

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		    XDrawPoint(display, mapwin, Wall_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 5:
		    XDrawPoint(display, mapwin, Fuel_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 6:
		case 7:
		case 8:
		case 9:
		    XDrawPoint(display, mapwin, Cannon_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 10:
		    XDrawPoint(display, mapwin, Base_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 11:
		case 12:
		    XDrawPoint(display, mapwin, Gravity_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 13:
		case 14:
		case 15:
		    XDrawPoint(display, mapwin, Wormhole_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 16:
		case 32:
		    XDrawPoint(display, mapwin, Treasure_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 17:
		case 18:
		    XDrawPoint(display, mapwin, Gravity_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 19:
		    XDrawPoint(display, mapwin, Target_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		    XDrawPoint(display, mapwin, Decor_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;

		case 26:
		    XDrawPoint(display, mapwin, Base_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 27:
		    XDrawPoint(display, mapwin, Item_Conc_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 28:
		case 29:
		case 30:
		case 31:
		    XDrawPoint(display, mapwin, Current_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 33:
		    XDrawPoint(display, mapwin, Friction_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		default:
		    XDrawPoint(display, mapwin, Black_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;

		}
	    }

    } else {
	for (i = x2; i < x2 + 2; i++)
	    for (j = y2; j < y2 + 2; j++) {
		data = mapicon_ptr[map.data[(int) (i * smlmap_xscale)]
				   [(int) (j * smlmap_yscale)] - 32];
		switch (data) {

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		    XDrawPoint(display, mapwin, Wall_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 5:
		    XDrawPoint(display, mapwin, Fuel_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 6:
		case 7:
		case 8:
		case 9:
		    XDrawPoint(display, mapwin, Cannon_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 10:
		    XDrawPoint(display, mapwin, Base_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 11:
		case 12:
		    XDrawPoint(display, mapwin, Gravity_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 13:
		case 14:
		case 15:
		    XDrawPoint(display, mapwin, Wormhole_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 16:
		case 32:
		    XDrawPoint(display, mapwin, Treasure_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 17:
		case 18:
		    XDrawPoint(display, mapwin, Gravity_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		    XDrawPoint(display, mapwin, Decor_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;

		case 26:
		    XDrawPoint(display, mapwin, Base_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 27:
		    XDrawPoint(display, mapwin, Item_Conc_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 28:
		case 29:
		case 30:
		case 31:
		    XDrawPoint(display, mapwin, Current_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		case 33:
		    XDrawPoint(display, mapwin, Friction_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		default:
		    XDrawPoint(display, mapwin, Black_GC, i + smlmap_x,
			       j + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH);
		    break;
		}
	    }
    }
    DrawViewBox();
}

/***************************************************************************/
/* DrawViewBox                                                             */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void DrawViewBox(void)
{
    int x1, y_1, x2, y2;

    x1 = (map.view_x / smlmap_xscale);
    y_1 = (map.view_y / smlmap_yscale);
    x2 = (mapwin_width - TOOLSWIDTH) / (map.view_zoom * smlmap_xscale);
    y2 = (mapwin_height / smlmap_yscale) / map.view_zoom;
    DrawViewSeg(x1, y_1, x2, y2);
}

/***************************************************************************/
/* DrawViewSeg                                                             */
/* Arguments :                                                             */
/*   x1                                                                    */
/*   y_1                                                                    */
/*   x2                                                                    */
/*   y2                                                                    */
/* Purpose :                                                               */
/***************************************************************************/
void DrawViewSeg(int x1, int y_1, int x2, int y2)
{
    if (x1 < 0) {
	DrawViewSeg(0, y_1, x2 + x1, y2);
	DrawViewSeg(smlmap_width + x1, y_1, (0 - x1), y2);
	return;
    }
    if (y_1 < 0) {
	DrawViewSeg(x1, 0, x2, y2 + y_1);
	DrawViewSeg(x1, smlmap_height + y_1, x2, (0 - y_1));
	return;
    }
    if ((x1 + x2) > smlmap_width) {
	DrawViewSeg(x1, y_1, smlmap_width - x1, y2);
	DrawViewSeg(0, y_1, x1 + x2 - smlmap_width, y2);
	return;
    }
    if ((y_1 + y2) > smlmap_height) {
	DrawViewSeg(x1, y_1, x2, smlmap_height - y_1);
	DrawViewSeg(x1, 0, x2, y_1 + y2 - smlmap_height);
	return;
    }
    XDrawRectangle(display, mapwin, xorgc, x1 + smlmap_x,
		   y_1 + smlmap_y + TOOLSHEIGHT - TOOLSWIDTH, x2, y2);
}
