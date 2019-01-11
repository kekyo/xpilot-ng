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
#ifndef WINBITMAP_H
#define WINBITMAP_H
#include "../gfx2d.h"
#include "bitmaps.h"
extern void PaintBitmap(Drawable d, int type, int x, int y,
			int width, int height, int number);

void Bitmap_set_pixel(xp_pixmap_t * xp_pixmap, int image,
			    int x, int y, RGB_COLOR color);
void Bitmap_paint(Drawable d, int img, int x, int y, int bmp);
int Bitmap_create_begin(Drawable d, xp_pixmap_t * pm, int bmp);
int Bitmap_create_end(Drawable d);

#endif
