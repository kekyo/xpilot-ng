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
#include "../xpclient.h"

/* for blitting items onto the screen */
extern HDC itemsDC;

/* very unfortunate, but about the only way i could do this */
HDC hDCb;
HDC hDC;

xp_picture_t radar_colors;

extern int texturedObjects;

void delete_bitmaps()
{
    int i, j;
    SelectObject(itemsDC, GetStockObject(BLACK_PEN));

    for (i = 0; i < num_pixmaps; i++) {
	for (j = 0; j < pixmaps[i].picture.count; j++) {
	    if (pixmaps[i].bitmaps[j].bitmap) {
		DeleteObject((HBITMAP) pixmaps[i].bitmaps[j].bitmap);
	    }
	}
    }
}

int Bitmap_create_begin(Drawable d, xp_pixmap_t * pm, int bmp)
{
    HBITMAP hbm;
    hDC = GetDC(xid[d].hwnd.hWnd);
    hDCb = CreateCompatibleDC(hDC);

    if (pm->bitmaps[bmp].bitmap) {
	SelectObject(itemsDC, GetStockObject(BLACK_PEN));
	DeleteObject((HBITMAP) pm->bitmaps[bmp].bitmap);
    }

    hbm = CreateCompatibleBitmap(hDC, pm->width, pm->height);
    SelectObject(hDCb, hbm);
    SelectPalette(hDCb, myPal, FALSE);
    RealizePalette(hDCb);

    if (!hbm) {
	error("Can't create item bitmaps");
	return -1;
    }
    pm->bitmaps[bmp].bitmap = (Pixmap) hbm;
    // The following hack is used to pass the dimension information 
    // to XFillPolygon
    SetBitmapDimensionEx(hbm, pm->width, pm->height, NULL);
    return 0;
}

int Bitmap_create_end(Drawable d)
{
    DeleteDC(hDCb);
    ReleaseDC(xid[d].hwnd.hWnd, hDC);
    return 0;
}

void Bitmap_set_pixel(xp_pixmap_t * xp_pixmap, int image, int x, int y,
		      RGB_COLOR color)
{
    SetPixelV(hDCb, x, y, color);
}

void Bitmap_paint_area(Drawable d, xp_bitmap_t * bit, int x, int y,
		       irec_t * r)
{
    HDC hDC = xid[d].hwnd.hBmpDC;

    SelectObject(itemsDC, (HBITMAP) bit->bitmap);
    SelectPalette(itemsDC, myPal, FALSE);
    RealizePalette(itemsDC);
    BitBlt(hDC, x, y, r->w, r->h, itemsDC, r->x, r->y, SRCPAINT);
}


/****************************IRRELEVANT STUFF HERE ***************************/
#if 0

int radar_light(int *radar, int x, int y)
{
/*   int light_matrix[25] ={ 0, -1, -2, -2, -3,
			   1,  0, -1, -2, -2,
			   2,  1,  0, -1, -2, 
			   2,  2,  1,  0, -1,
			   3,  2,  2,  1,  0 
			};
*/

    int light_matrix[25] = { 0, 1, 2, 1, 0,
	1, 2, 3, 2, 1,
	2, 3, 0, 3, 2,
	1, 2, 3, 2, 1,
	0, 1, 2, 1, 0
    };
/*   int light_matrix[25] ={ 0,  1,  -1, -1,  0,
			   1,  2,  -1, -2, -1,
			   2,  3,  0, -2, -2, 
			   1,  2,  3,  -1, -1,
			   0,  1,  2,  1, 0 
			};
*/
    int xi, yi, light;
    int cy, cx;

    light = 0;
    for (xi = -2; xi < 3; xi++) {
	cx = (xi + x + 256) % 256;
	for (yi = -2; yi < 3; yi++) {
	    cy = (yi + y + RadarHeight) % RadarHeight;
	    if (radar[cy * 256 + cx])
		light += light_matrix[(xi + 2) + ((yi + 2) * 5)];
	}
    }
    return light /* + 20 */ ;
}


void Winpaint_world_radar(void)
{
    HDC hDC = xid[radarPixmap2].hwnd.hBmpDC;
    int radar[256 * 300];
    int cr, cg, cb;
    int i, xi, yi, xm, ym, xp, yp = 0, xmoff;
    int type, vis;
    float xs, ys;
    int npoint = 0, nsegment = 0, start, end;
    const int max = 256;
    u_byte visible[256], visibleColor[256];
    int light;
    RGB_COLOR color;
    radar_exposures = 2;

    memset(visible, 0, sizeof visible);
    visible[SETUP_FILLED] = 1;
    visible[SETUP_FILLED_NO_DRAW] = 1;
    visible[SETUP_REC_LU] = 1;
    visible[SETUP_REC_RU] = 1;
    visible[SETUP_REC_LD] = 1;
    visible[SETUP_REC_RD] = 1;
    visible[SETUP_FUEL] = 1;
    for (i = 0; i < 10; i++) {
	visible[SETUP_TARGET + i] = 1;
    }
    for (i = BLUE_BIT; i < sizeof visible; i++) {
	visible[i] = 1;
    }
    if (instruments.showDecor) {
	visible[SETUP_DECOR_FILLED] = 1;
	visible[SETUP_DECOR_LU] = 1;
	visible[SETUP_DECOR_RU] = 1;
	visible[SETUP_DECOR_LD] = 1;
	visible[SETUP_DECOR_RD] = 1;
    }

    memset(visibleColor, 0, sizeof visibleColor);
    visibleColor[SETUP_FILLED] =
	visibleColor[SETUP_FILLED_NO_DRAW] =
	visibleColor[SETUP_REC_LU] =
	visibleColor[SETUP_REC_RU] =
	visibleColor[SETUP_REC_LD] =
	visibleColor[SETUP_REC_RD] =
	visibleColor[SETUP_FUEL] = wallRadarColor;
    for (i = 0; i < 10; i++) {
	visibleColor[SETUP_TARGET + i] = targetRadarColor;
    }
    for (i = BLUE_BIT; i < sizeof visible; i++) {
	visibleColor[i] = wallRadarColor;
    }
    if (instruments.showDecor) {
	visibleColor[SETUP_DECOR_FILLED] =
	    visibleColor[SETUP_DECOR_LU] =
	    visibleColor[SETUP_DECOR_RU] =
	    visibleColor[SETUP_DECOR_LD] =
	    visibleColor[SETUP_DECOR_RD] = decorRadarColor;
    }

    xs = (float) (Setup->x - 1) / (256 - 1);
    ys = (float) (Setup->y - 1) / (RadarHeight - 1);
    for (xi = 0; xi < 256; xi++) {
	xm = (int) (xi * xs + 0.5);
	xmoff = xm * Setup->y;
	start = end = -1;
	xp = xi;
	for (yi = 0; yi < RadarHeight; yi++) {
	    ym = (int) (yi * ys + 0.5);
	    type = Setup->map_data[xmoff + ym];
	    vis = visible[type];
	    radar[((RadarHeight - 1) - yi) * 256 + xi] = vis;
	}
    }
    for (xi = 0; xi < 256; xi++) {
	for (yi = 0; yi < RadarHeight; yi++) {
	    light = radar_light(radar, xi, yi);
	    color = Picture_get_pixel(&radar_colors, 0,
				      xi % RADAR_TEXTURE_SIZE,
				      yi % RADAR_TEXTURE_SIZE);
	    cr = RED_VALUE(color);
	    cg = GREEN_VALUE(color);
	    cb = BLUE_VALUE(color);
	    if (radar[yi * 256 + xi]) {
		SetPixelV(hDC, xi, yi, RGB((cr * light) / 36,
					   (cg * light) / 36,
					   (cb * light) / 36));
	    } else {
		SetPixelV(hDC, xi, yi, RGB((cr * (20 + light)) / 72,
					   (cg * (20 + light)) / 72,
					   (cb * (20 + light)) / 72));
	    }
	}
    }

    for (i = 0;; i++) {
	int dead_time, damage;
	if (Target_by_index(i, &xi, &yi, &dead_time, &damage) == -1) {
	    break;
	}
	if (dead_time) {
	    continue;
	}
	Paint_radar_block(xi, yi, targetRadarColor);
    }
}

void Block_bitmap_paint_fuel_slice(Drawable d, int type, int x, int y,
				   int width, int height, int image,
				   int size)
{

    HDC hDC = xid[d].hwnd.hBmpDC;

    SelectObject(itemsDC,
		 (HBITMAP) xp_pixmaps[type].bitmaps[image].bitmap);

    if (bHasPal) {
	SelectPalette(itemsDC, myPal, FALSE);
	RealizePalette(itemsDC);
    }

    BitBlt(hDC, x, y, width, size, itemsDC, 0, 0, SRCPAINT);

}


void Block_bitmap_paint_meter(Drawable d, int type, int x, int y,
			      int width, int height, int size)
{
    HDC hDC = xid[d].hwnd.hBmpDC;

    SelectObject(itemsDC, (HBITMAP) xp_pixmaps[type].bitmaps[1].bitmap);

    if (bHasPal) {
	SelectPalette(itemsDC, myPal, FALSE);
	RealizePalette(itemsDC);
    }

    BitBlt(hDC, x, y, size, height, itemsDC, 0, 0, SRCPAINT);

    SelectObject(itemsDC, (HBITMAP) xp_pixmaps[type].bitmaps[0].bitmap);

    if (bHasPal) {
	SelectPalette(itemsDC, myPal, FALSE);
	RealizePalette(itemsDC);
    }

    BitBlt(hDC, x + size, y, width - size, height, itemsDC, size, 0,
	   SRCPAINT);

}

/*
void paintItemSymbol(unsigned char type, Drawable d, GC gc, int x, int y, int color)
{
    PaintBitmap(d, BM_ALL_ITEMS, x, y, WINSCALE(16), WINSCALE(16), type); 
//    PaintBitmap(d, BM_ALL_ITEMS, x, y, 16, 16, type); 
}
*/

#endif
