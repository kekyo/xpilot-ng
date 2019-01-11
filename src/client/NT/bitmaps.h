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

#ifndef	BITMAPS_H
#define	BITMAPS_H

#include "gfx2d.h"
#include "types.h"

#define BM_HOLDER_FRIEND 0
#define BM_HOLDER_ENEMY  1
#define BM_BALL		 2
#define BM_SHIP_SELF	 3
#define BM_SHIP_FRIEND	 4
#define BM_SHIP_ENEMY	 5
#define BM_BULLET	 6
#define BM_BULLET_OWN	 7
#define BM_BASE_DOWN	 8
#define BM_BASE_LEFT	 9
#define BM_BASE_UP	10
#define BM_BASE_RIGHT	11
#define BM_FUELCELL	12
#define BM_FUEL		13
#define BM_ALL_ITEMS	14
#define BM_CANNON_DOWN  15
#define BM_CANNON_LEFT  16
#define BM_CANNON_UP	17
#define BM_CANNON_RIGHT 18
#define BM_SPARKS	19
#define BM_PAUSED	20
#define BM_WALL_TOP	21
#define BM_WALL_LEFT	22
#define BM_WALL_BOTTOM	23
#define BM_WALL_RIGHT	24
#define BM_WALL_LU	25
#define BM_WALL_RU	26
#define BM_WALL_LD	27
#define BM_WALL_RD	28

#define BM_WALL_FILLED  29
#define BM_WALL_UR	30
#define BM_WALL_UL	31

#define BM_SCORE_BG	32
#define BM_LOGO		33
#define BM_REFUEL	34
#define BM_WORMHOLE	35
#define BM_MINE_TEAM    36
#define BM_MINE_OTHER	37
#define BM_CONCENTRATOR 38
#define BM_PLUSGRAVITY  39
#define BM_MINUSGRAVITY 40
#define BM_CHECKPOINT	41
#define BM_METER	42
#define BM_ASTEROIDCONC	43
#define BM_BALL_GRAY    44

#define NUM_OBJECT_BITMAPS 45
#define NUM_BITMAPS	45

#define BMS_UNINITIALIZED 0
#define BMS_INITIALIZED 1
#define BMS_READY 2
#define BMS_ERROR -1

#define BG_IMAGE_HEIGHT 442  
#define LOGO_HEIGHT     223

#define RADAR_TEXTURE_SIZE 32

typedef struct {
    Pixmap		bitmap;
    Pixmap		mask;
    bbox_t		bbox;
    int                 rgb; /* the color this image is blended with */
} xp_bitmap_t;

/* xp_pixmap_t holds all data related to one "logical" image.
 * One logical image can consists of several rectangular pixel 
 * arrays (physical images). All physical images share the same 
 * overall dimensions. 
 *
 * Note: if the count is negative it means that the other images
 * are rotated copies of the original image.
 */
typedef struct {
    const char		*filename;     /* the file containing the image */
    int			count;         /* amount of images (see above) */
    int			state;         /* the state of the image (BMS_*) */
    unsigned		width, height; /* the (scaled) dimensions */
    bool		scalable;      /* should this image be scaled */
    xp_bitmap_t		*bitmaps;      /* platform dependent image data */
    xp_picture_t	picture;       /* the image data in RGB format */
} xp_pixmap_t;

extern xp_pixmap_t *pixmaps;
extern int num_pixmaps, max_pixmaps;
extern xp_pixmap_t xp_pixmaps[];

int Bitmaps_init(void);
void Bitmaps_cleanup(void);
int Bitmap_create (Drawable d, int img);
void Bitmap_update_scale (void);

xp_bitmap_t *Bitmap_get (Drawable d, int img, int bmp);
void Bitmap_paint (Drawable d, int img, int x, int y, int bmp);
void Bitmap_paint_area (Drawable d, xp_bitmap_t *bit, int x, int y, irec_t *r);
xp_bitmap_t *Bitmap_get_blended(Drawable d, int img, int rgb);
void Bitmap_paint_blended(Drawable d, int img, int x, int y, int rgb);

#endif
