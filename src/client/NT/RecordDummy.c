/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// RecordDummy.c 
// Used by Windows.
// Dummy up the recording of xpilot sessions
// cause i don't feel like dealing w/it.

#include "../../common/NT/winX.h"
#include "../record.h"

int recording = False;		/* Are we recording or not. */

extern void paintItemSymbol(unsigned char type, Drawable drawable, GC mygc,
			    int x, int y, int color);

static void Dummy_newFrame(void)
{
}
static void Dummy_endFrame(void)
{
}

/*
 * X windows drawing
 */
static struct recordable_drawing Xdrawing = {
    Dummy_newFrame,
    Dummy_endFrame,
    XDrawArc,
    XDrawLines,
    XDrawLine,
    XDrawRectangle,
    XDrawString,
    XFillArc,
    XFillPolygon,
    paintItemSymbol,
    XFillRectangle,
    XFillRectangles,
    XDrawArcs,
    XDrawSegments,
    XSetDashes,
};

/*
 * Publicly accessible drawing routines.
 * This is either a copy of Xdrawing or of Rdrawing.
 */
struct recordable_drawing rd;


/*
 * Store the name of the file where the user
 * wants recordings to be written to.
 */
void Record_init(char *filename)
{
    rd = Xdrawing;
    // if (filename != NULL && filename[0] != '\0') {
    // record_filename = strdup(filename);
    // }
}

void Record_cleanup(void)
{
}

long Record_size(void)
{
    return (0L);
}

void Record_toggle(void)
{
}
