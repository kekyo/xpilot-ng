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

/* this gets rid of missing initializer warnings */
#define XP_PIXMAP_INITIALIZER(f, c) { f,c,0,0,0,false,NULL,{0,0,0,NULL,NULL} }

xp_pixmap_t object_pixmaps[] = {
    XP_PIXMAP_INITIALIZER("holder1.ppm", 1),
    XP_PIXMAP_INITIALIZER("holder2.ppm", 1),
    XP_PIXMAP_INITIALIZER("ball.ppm", 1),
    XP_PIXMAP_INITIALIZER("ship_red.ppm", 128),
    XP_PIXMAP_INITIALIZER("ship_blue.ppm", 128),
    XP_PIXMAP_INITIALIZER("ship_red2.ppm", 128),
    XP_PIXMAP_INITIALIZER("bullet.ppm", -8),
    XP_PIXMAP_INITIALIZER("bullet_blue.ppm", -8),
    XP_PIXMAP_INITIALIZER("base_down.ppm", 1),
    XP_PIXMAP_INITIALIZER("base_left.ppm", 1),
    XP_PIXMAP_INITIALIZER("base_up.ppm", 1),
    XP_PIXMAP_INITIALIZER("base_right.ppm", 1),
    XP_PIXMAP_INITIALIZER("fuelcell.ppm", 1),
    XP_PIXMAP_INITIALIZER("fuel2.ppm", -16),
    XP_PIXMAP_INITIALIZER("allitems.ppm", -30),
    XP_PIXMAP_INITIALIZER("cannon_down.ppm", 1),
    XP_PIXMAP_INITIALIZER("cannon_left.ppm", 1),
    XP_PIXMAP_INITIALIZER("cannon_up.ppm", 1),
    XP_PIXMAP_INITIALIZER("cannon_right.ppm", 1),
    XP_PIXMAP_INITIALIZER("sparks.ppm", -8),
    XP_PIXMAP_INITIALIZER("paused.ppm", -2),
    XP_PIXMAP_INITIALIZER("wall_top.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_left.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_bottom.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_right.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_ul.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_ur.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_dl.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_dr.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_fi.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_url.ppm", 1),
    XP_PIXMAP_INITIALIZER("wall_ull.ppm", 1),
    XP_PIXMAP_INITIALIZER("clouds.ppm", 1),
    XP_PIXMAP_INITIALIZER("logo.ppm", 1),
    XP_PIXMAP_INITIALIZER("refuel.ppm", -4),
    XP_PIXMAP_INITIALIZER("wormhole.ppm", 8),
    XP_PIXMAP_INITIALIZER("mine_team.ppm", 1),
    XP_PIXMAP_INITIALIZER("mine_other.ppm", 1),
    XP_PIXMAP_INITIALIZER("concentrator.ppm", 32),
    XP_PIXMAP_INITIALIZER("plus.ppm", 1),
    XP_PIXMAP_INITIALIZER("minus.ppm", 1),
    XP_PIXMAP_INITIALIZER("checkpoint.ppm", -2),
    XP_PIXMAP_INITIALIZER("meter.ppm", -2),
    XP_PIXMAP_INITIALIZER("asteroidconcentrator.ppm", 32),
    XP_PIXMAP_INITIALIZER("ball_gray16.ppm", -16)
};

xp_pixmap_t *pixmaps = 0;
int num_pixmaps = 0, max_pixmaps = 0;


static int Bitmap_init(int img);
static void Bitmap_picture_copy(xp_pixmap_t * xp_pixmap, int image);
static void Bitmap_picture_scale(xp_pixmap_t * xp_pixmap, int image);



/*
 * Adds the standard images into global pixmaps array.
 */
int Bitmaps_init(void)
{
    int i;
    xp_pixmap_t pixmap;

    for (i = 0; i < NUM_OBJECT_BITMAPS; i++) {
	pixmap = object_pixmaps[i];
	pixmap.scalable = (i == BM_LOGO
			   || i == BM_SCORE_BG) ? false : true;
	pixmap.state = BMS_UNINITIALIZED;
	STORE(xp_pixmap_t, pixmaps, num_pixmaps, max_pixmaps, pixmap);
    }

    return 0;
}

void Bitmaps_cleanup(void)
{
    if (pixmaps)
	free(pixmaps);
    pixmaps = 0;
}


/**
 * Adds a new bitmap needed by the current map into global pixmaps.
 * Returns the index of the newly added bitmap in the array.
 */
int Bitmap_add(char *filename, int count, bool scalable)
{
    xp_pixmap_t pixmap;

    pixmap.filename = xp_strdup(filename);
    pixmap.count = count;
    pixmap.scalable = scalable;
    pixmap.state = BMS_UNINITIALIZED;
    STORE(xp_pixmap_t, pixmaps, num_pixmaps, max_pixmaps, pixmap);
    return num_pixmaps - 1;
}


/**
 * Creates the Pixmaps needed for the given image.
 */
int Bitmap_create(Drawable d, int img)
{
    int j;
    xp_pixmap_t *pix = &pixmaps[img];

    if (pix->state == BMS_UNINITIALIZED)
	Bitmap_init(img);
    if (pix->state != BMS_INITIALIZED)
	return -1;


    for (j = 0; j < ABS(pix->count); j++) {
	if (pix->scalable) {
	    pix->width = UWINSCALE(pix->picture.width);
	    pix->height = UWINSCALE(pix->picture.height);
	}

	if (Bitmap_create_begin(d, pix, j) == -1) {
	    pix->state = BMS_ERROR;
	    return -1;
	}

	if (pix->height == pix->picture.height &&
	    pix->width == pix->picture.width) {
	    Bitmap_picture_copy(pix, j);
	} else
	    Bitmap_picture_scale(pix, j);

	if (Bitmap_create_end(d) == -1) {
	    pix->state = BMS_ERROR;
	    return -1;
	}
    }

    pix->state = BMS_READY;

    return 0;
}


/**
 * Causes all scalable bitmaps to be rescaled (recreated actually)
 * next time needed.
 */
void Bitmap_update_scale(void)
{
    /* This should do the trick.
     * All "good" scalable bitmaps are marked as initialized
     * causing the next Bitmap_get to recreate the bitmap using
     * the current scale factor. Bitmap_create should take care of
     * releasing the device pixmaps no longer needed. */

    int i;
    for (i = 0; i < num_pixmaps; i++)
	if (pixmaps[i].state == BMS_READY && pixmaps[i].scalable)
	    pixmaps[i].state = BMS_INITIALIZED;
}


/**
 * Gets a pointer to the bitmap specified with img and bmp.
 * Ensures that the bitmap returned has been initialized and created
 * properly. Returns NULL if the specified bitmap is not in appropriate
 * state.
 */
xp_bitmap_t *Bitmap_get(Drawable d, int img, int bmp)
{
    if (!fullColor || img < 0 || img >= num_pixmaps)
	return NULL;

    if (pixmaps[img].state != BMS_READY) {
	if (Bitmap_create(d, img) == -1)
	    return NULL;
    }

    return &pixmaps[img].bitmaps[bmp];
}

static void Bitmap_blend_with_color(int img, int bmp, int rgb)
{
    int x, y, r, g, b, r2, g2, b2;
    bool scaled;
    RGB_COLOR color;
    double x_scaled = 0.0, y_scaled  = 0.0, dx_scaled  = 0.0, dy_scaled = 0.0;
    xp_pixmap_t *pix = &pixmaps[img];

    pix->bitmaps[bmp].rgb = rgb;
    scaled = pix->height != pix->picture.height ||
	pix->width != pix->picture.width;

    if (scaled) {
	dx_scaled = ((double)pix->picture.width) / pix->width;
	dy_scaled = ((double)pix->picture.height) / pix->height;
	y_scaled = 0;
    }

    r2 = (rgb >> 16) & 0xff;
    g2 = (rgb >> 8) & 0xff;
    b2 = rgb & 0xff;
    
    for (y = 0; y < (int)pix->height; y++) {
	if (scaled)
	    x_scaled = 0;
	for (x = 0; x < (int)pix->width; x++) {
	    color = scaled ?
		Picture_get_pixel_area(&(pix->picture), bmp,
				       x_scaled, y_scaled,
				       dx_scaled, dy_scaled) :
		Picture_get_pixel(&(pix->picture), bmp, x, y);
	    r = RED_VALUE(color) * r2 / 0xff;
	    g = GREEN_VALUE(color) * g2 / 0xff;
	    b = BLUE_VALUE(color) * b2 / 0xff;
	    Bitmap_set_pixel(pix, bmp, x, y, RGB24(r, g, b));
	    if (scaled)
		x_scaled += dx_scaled;
	}
	if (scaled)
	    y_scaled += dy_scaled;
    }    
}


/**
 * Gets a pointer to the bitmap of img blended with color rgb.
 * Ensures that the bitmap returned has been initialized and created
 * properly. Returns NULL if the specified bitmap is not in appropriate
 * state or cannot be created.
 */
xp_bitmap_t *Bitmap_get_blended(Drawable d, int img, int rgb)
{
    int i;

    if (!fullColor || img < 0 || img >= num_pixmaps)
	return NULL;

    if (pixmaps[img].state != BMS_READY) {
	if (Bitmap_create(d, img) == -1)
	    return NULL;
    }

    for (i = 0; i < ABS(pixmaps[img].count); i++) {
	if (pixmaps[img].bitmaps[i].rgb == rgb)
	    return &pixmaps[img].bitmaps[i];
	if (pixmaps[img].bitmaps[i].rgb == -1) {
	    Bitmap_blend_with_color(img, i, rgb);
	    return &pixmaps[img].bitmaps[i];
	}
    }

    /* fall back on the first bitmap */
    return &pixmaps[img].bitmaps[0];
}


/**
 * Loads and initializes the given image.
 */
static int Bitmap_init(int img)
{
    int j, count;

    count = ABS(pixmaps[img].count);

    if (!(pixmaps[img].bitmaps = malloc(count * sizeof(xp_bitmap_t)))) {
	error("not enough memory for bitmaps");
	pixmaps[img].state = BMS_ERROR;
	return -1;
    }

    for (j = 0; j < count; j++) {
	pixmaps[img].bitmaps[j].bitmap =
	    pixmaps[img].bitmaps[j].mask = None;
	pixmaps[img].bitmaps[j].rgb = -1;
    }

    if (Picture_init
	(&pixmaps[img].picture,
	 pixmaps[img].filename, pixmaps[img].count) == -1) {
	pixmaps[img].state = BMS_ERROR;
	return -1;
    }

    pixmaps[img].width = pixmaps[img].picture.width;
    pixmaps[img].height = pixmaps[img].picture.height;
    pixmaps[img].state = BMS_INITIALIZED;

    return 0;
}


/*
 * Purpose: Take a device independent picture and create a
 * device/os dependent image.
 * This is only used in the scalefactor 1.0 special case.
 *
 * Actually this function could be killed, but it's very fast
 * and it uses the intended original image.
 */
static void Bitmap_picture_copy(xp_pixmap_t * xp_pixmap, int image)
{
    int x, y;
    RGB_COLOR color;

    for (y = 0; y < (int)xp_pixmap->height; y++) {
	for (x = 0; x < (int)xp_pixmap->width; x++) {
	    color = Picture_get_pixel(&(xp_pixmap->picture), image, x, y);
	    Bitmap_set_pixel(xp_pixmap, image, x, y, color);
	}
    }

    /* copy bounding box from original picture. */
    xp_pixmap->bitmaps[image].bbox = xp_pixmap->picture.bbox[image];
}


/*
 * Purpose: Take a device independent picture and create a
 * scaled device/os dependent image.
 * This is for some of us the general case.
 * The trick is for each pixel in the target image
 * to find the area it responds to in the original image, and then
 * find an average of the colors in this area.
 */
static void Bitmap_picture_scale(xp_pixmap_t * xp_pixmap, int image)
{
    int x, y;
    RGB_COLOR color;
    double x_scaled, y_scaled;
    double dx_scaled, dy_scaled;
    double orig_height, orig_width;
    int height, width;

    orig_height = xp_pixmap->picture.height;
    orig_width = xp_pixmap->picture.width;
    height = xp_pixmap->height;
    width = xp_pixmap->width;

    dx_scaled = orig_width / width;
    dy_scaled = orig_height / height;
    y_scaled = 0;

    for (y = 0; y < height; y++) {
	x_scaled = 0;
	for (x = 0; x < width; x++) {
	    color =
		Picture_get_pixel_area
		(&(xp_pixmap->picture), image,
		 x_scaled, y_scaled, dx_scaled, dy_scaled);

	    Bitmap_set_pixel(xp_pixmap, image, x, y, color);
	    x_scaled += dx_scaled;
	}
	y_scaled += dy_scaled;
    }

    /* scale bounding box as well. */
    {
	bbox_t *src = &xp_pixmap->picture.bbox[image];
	bbox_t *dst = &xp_pixmap->bitmaps[image].bbox;

	dst->xmin = (int) ((width * src->xmin) / orig_width);
	dst->ymin = (int) ((height * src->ymin) / orig_height);
	dst->xmax = (int) (((width * src->xmax) + (orig_width - 1)) /
			   orig_width);
	dst->ymax = (int) (((height * src->ymax) + (orig_height - 1)) /
			   orig_height);
    }
}


/*
 * Purpose: Paint a the bitmap specified with img and bmp
 * so that only the pixels inside the bounding box are
 * painted.
 */
void Bitmap_paint(Drawable d, int img, int x, int y, int bmp)
{
    xp_bitmap_t *bit;
    bbox_t *box;
    irec_t area;

    if ((bit = Bitmap_get(d, img, bmp)) == NULL)
	return;
    box = &bit->bbox;

    area.x = box->xmin;
    area.y = box->ymin;
    area.w = box->xmax + 1 - box->xmin;
    area.h = box->ymax + 1 - box->ymin;

    Bitmap_paint_area(d, bit, x + area.x, y + area.y, &area);
}

/*
 * Paints the given image blending it with the given RGB color if
 * possible.
 */
void Bitmap_paint_blended(Drawable d, int img, int x, int y, int rgb)
{
    xp_bitmap_t *bit;
    bbox_t *box;
    irec_t area;

    if ((bit = Bitmap_get_blended(d, img, rgb)) == NULL)
	return;
    box = &bit->bbox;

    area.x = box->xmin;
    area.y = box->ymin;
    area.w = box->xmax + 1 - box->xmin;
    area.h = box->ymax + 1 - box->ymin;

    Bitmap_paint_area(d, bit, x + area.x, y + area.y, &area);
}


