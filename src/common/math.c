/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2003-2004 by
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

#include "xpcommon.h"

double		tbl_sin[TABLE_SIZE];
double		tbl_cos[TABLE_SIZE];

int ON(const char *optval)
{
    return (strncasecmp(optval, "true", 4) == 0
	    || strncasecmp(optval, "on", 2) == 0
	    || strncasecmp(optval, "yes", 3) == 0);
}


int OFF(const char *optval)
{
    return (strncasecmp(optval, "false", 5) == 0
	    || strncasecmp(optval, "off", 3) == 0
	    || strncasecmp(optval, "no", 2) == 0);
}


int mod(int x, int y)
{
    if (x >= y || x < 0)
	x = x - y*(x/y);

    if (x < 0)
	x += y;

    return x;
}

double findDir(double x, double y)
{
    double angle;

    if (x != 0.0 || y != 0.0)
	angle = atan2(y, x) / (2 * PI);
    else
	angle = 0.0;

    if (angle < 0)
	angle++;
    return angle * RES;
}


double rfrac(void)
{
    /*
     * Return a pseudo-random value in the range { 0.0 <= x < 1.0 }.
     * Use randomMT() which returns a 32 bit PRN and multiply by 1/(1<<32).
     */
    return (double) (randomMT() * 0.00000000023283064365386962890625);
}


void Make_table(void)
{
    int i;

    for (i = 0; i < TABLE_SIZE; i++) {
	tbl_sin[i] = sin(i * (2.0 * PI / TABLE_SIZE));
	tbl_cos[i] = cos(i * (2.0 * PI / TABLE_SIZE));
    }
}
