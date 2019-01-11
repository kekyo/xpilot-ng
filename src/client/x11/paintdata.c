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

#include "xpclient_x11.h"

static XRectangle	*rect_ptr[MAX_COLORS];
static int		num_rect[MAX_COLORS], max_rect[MAX_COLORS];
static XArc		*arc_ptr[MAX_COLORS];
static int		num_arc[MAX_COLORS], max_arc[MAX_COLORS];
static XSegment		*seg_ptr[MAX_COLORS];
static int		num_seg[MAX_COLORS], max_seg[MAX_COLORS];

typedef struct {
    unsigned long color;
    XArc arc;
} rgb_arc_t;

static rgb_arc_t	*rgb_arc_ptr;
static int		num_rgb_arc, max_rgb_arc;

unsigned long	current_foreground;

void Rectangle_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
	num_rect[i] = 0;
}

void Rectangle_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++) {
	if (num_rect[i] > 0) {
	    SET_FG(colors[i].pixel);
	    rd.fillRectangles(dpy, drawPixmap, gameGC,
			      rect_ptr[i], num_rect[i]);
	    RELEASE(rect_ptr[i], num_rect[i], max_rect[i]);
	}
    }
}

int Rectangle_add(int color, int x, int y, int width, int height)
{
    XRectangle		t;

    t.x = WINSCALE(x);
    t.y = WINSCALE(y);
    t.width = WINSCALE(width);
    t.height = WINSCALE(height);

    STORE(XRectangle, rect_ptr[color], num_rect[color], max_rect[color], t);
    return 0;
}

void Arc_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
	num_arc[i] = 0;
    num_rgb_arc = 0;
}

void Arc_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++) {
	if (num_arc[i] > 0) {
	    SET_FG(colors[i].pixel);
	    rd.drawArcs(dpy, drawPixmap, gameGC, arc_ptr[i], num_arc[i]);
	    RELEASE(arc_ptr[i], num_arc[i], max_arc[i]);
	}
    }

    /* fullcolor arcs */
    for (i = 0; i < num_rgb_arc; i++) {
	rgb_arc_t *p = &rgb_arc_ptr[i];

	SET_FG(p->color);
	rd.drawArc(dpy, drawPixmap, gameGC,
		   p->arc.x, p->arc.y,
		   p->arc.width, p->arc.height,
		   p->arc.angle1, p->arc.angle2);
    }
    if (num_rgb_arc > 0)
	RELEASE(rgb_arc_ptr, num_rgb_arc, max_rgb_arc);
}

int Arc_add(int color,
	    int x, int y,
	    int width, int height,
	    int angle1, int angle2)
{
    XArc t;

    t.x = WINSCALE(x);
    t.y = WINSCALE(y);
    t.width = WINSCALE(width+x) - t.x;
    t.height = WINSCALE(height+y) - t.y;

    t.angle1 = angle1;
    t.angle2 = angle2;
    STORE(XArc, arc_ptr[color], num_arc[color], max_arc[color], t);
    return 0;
}

int Arc_add_rgb(unsigned long color,
		int fallback_color,
		int x, int y,
		int width, int height,
		int angle1, int angle2)
{
    rgb_arc_t t;

    /* hack */
    if (!fullColor)
	return Arc_add(fallback_color, x, y, width, height, angle1, angle2);

    t.color = color;
    t.arc.x = WINSCALE(x);
    t.arc.y = WINSCALE(y);
    t.arc.width = WINSCALE(width+x) - t.arc.x;
    t.arc.height = WINSCALE(height+y) - t.arc.y;

    t.arc.angle1 = angle1;
    t.arc.angle2 = angle2;
    STORE(rgb_arc_t, rgb_arc_ptr, num_rgb_arc, max_rgb_arc, t);
    return 0;
}

void Segment_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
	num_seg[i] = 0;
}

void Segment_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++) {
	if (num_seg[i] > 0) {
	    SET_FG(colors[i].pixel);
	    rd.drawSegments(dpy, drawPixmap, gameGC,
			    seg_ptr[i], num_seg[i]);
	    RELEASE(seg_ptr[i], num_seg[i], max_seg[i]);
	}
    }
}

int Segment_add(int color, int x_1, int y_1, int x_2, int y_2)
{
    XSegment t;

    t.x1 = WINSCALE(x_1);
    t.y1 = WINSCALE(y_1);
    t.x2 = WINSCALE(x_2);
    t.y2 = WINSCALE(y_2);
    STORE(XSegment, seg_ptr[color], num_seg[color], max_seg[color], t);
    return 0;
}

void paintdataCleanup(void)
{
    int i;

    for (i = 0; i < MAX_COLORS; i++) {
	if (max_rect[i] > 0 && rect_ptr[i]) {
	    max_rect[i] = 0;
	    free(rect_ptr[i]);
	}
	if (max_arc[i] > 0 && arc_ptr[i]) {
	    max_arc[i] = 0;
	    free(arc_ptr[i]);
	}
	if (max_seg[i] > 0 && seg_ptr[i]) {
	    max_seg[i] = 0;
	    free(seg_ptr[i]);
	}
    }
}
