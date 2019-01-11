/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-98 by
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

#include "../xpclient.h"

#define REDVAL(c) (((c) >> 16) & 0xff)
#define GREENVAL(c) (((c) >> 8) & 0xff)
#define BLUEVAL(c) ((c) & 0xff)


static int Colors_get_palette_index(int rgb);
void Colors_init_style_colors(void);


int Colors_init_bitmaps(void)
{
    Colors_init_style_colors();
    return 0;
}

void Colors_free_bitmaps(void)
{
}

/*
 * Converts the RGB colors used by polygon and edge styles 
 * to device colors.
 */
void Colors_init_style_colors(void)
{
    int i, oldNum;

    if (!num_polygon_styles && !num_edge_styles)
	return;

    oldNum = myLogPal->palNumEntries;

    for (i = 0; i < num_polygon_styles; i++)
	polygon_styles[i].color =
	    Colors_get_palette_index(polygon_styles[i].rgb);

    for (i = 0; i < num_edge_styles; i++)
	edge_styles[i].color =
	    Colors_get_palette_index(edge_styles[i].rgb);

    if (myLogPal->palNumEntries != oldNum) {
	DeleteObject(myPal);
	myPal = CreatePalette(myLogPal);
	if (!myPal)
	    error("Can't create palette");
	//ChangePalette(NULL);
    }
}


/*
 * Returns the index of the given rgb color in the current 
 * palette. If the palette doesn't contain it, a new entry
 * will be added to myLogPal.
 */
static int Colors_get_palette_index(int rgb)
{

    int i;

    for (i = 0; i < myLogPal->palNumEntries; i++)
	if (myLogPal->palPalEntry[i].peRed == REDVAL(rgb)
	    && myLogPal->palPalEntry[i].peGreen == GREENVAL(rgb)
	    && myLogPal->palPalEntry[i].peBlue == BLUEVAL(rgb))
	    return i;

    if (i == WINMAXCOLORS)
	error("Out of palette entries");

    myLogPal->palPalEntry[i].peFlags = PC_RESERVED;
    myLogPal->palPalEntry[i].peRed = REDVAL(rgb);
    myLogPal->palPalEntry[i].peGreen = GREENVAL(rgb);
    myLogPal->palPalEntry[i].peBlue = BLUEVAL(rgb);
    myLogPal->palNumEntries++;

    return i;
}

