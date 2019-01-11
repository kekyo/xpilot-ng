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

#include "xpcommon.h"

#ifndef HAVE_STRCASECMP
/*
 * By Ian Malcom Brown.
 * Changes by BG: prototypes with const,
 * moved the ++ expressions out of the macro.
 * Only test for the null byte in one string.
 */
int strcasecmp(const char *str1, const char *str2)
{
    int	c1, c2;

    do {
	c1 = *str1++;
	c2 = *str2++;
	c1 = tolower(c1);
	c2 = tolower(c2);
    } while (c1 == c2 && c1 != 0);

    return (c1 - c2);
}
#endif

#ifndef HAVE_STRNCASECMP
/*
 * By Bert Gijsbers, derived from Ian Malcom Brown's strcasecmp().
 */
int strncasecmp(const char *str1, const char *str2, size_t n)
{
    int	c1, c2;

    do {
	if (n-- <= 0)
	    return 0;
	c1 = *str1++;
	c2 = *str2++;
	c1 = tolower(c1);
	c2 = tolower(c2);
    } while (c1 == c2 && c1 != 0);

    return (c1 - c2);
}
#endif

