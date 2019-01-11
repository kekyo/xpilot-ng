/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
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

#ifndef IMAGES_H
#define IMAGES_H

#include "xpclient_sdl.h"

typedef enum {
    IMG_STATE_UNINITIALIZED,
    IMG_STATE_ERROR,
    IMG_STATE_READY
} image_state_e;

/*
 * This structure holds the information needed to paint an image with OpenGL.
 * One image may contain multiple frames that represent the same object in
 * different states. If rotate flag is true, the image will be rotated
 * when it is created to generate num_frames-1 of new frames.
 * Width is the cumulative width of all frames. Frame_width is the width
 * of any single frame. Because OpenGL requires the dimensions of the images
 * to be powers of 2, data_width and data_height are the nearest powers of 2
 * corresponding to width and height respectively.
 */
typedef struct {
    GLuint          name;         /* OpenGL texture "name" */
    char            *filename;    /* the name of the image file */
    int             num_frames;   /* the number of frames */
    bool            rotate;       /* should this image be rotated */
    bool            scale;        /* should this image be scaled to fill data */
    image_state_e   state;        /* the state of the image */
    int             width;        /* width of the whole image */
    int             height;       /* height of the whole image */
    int             data_width;   /* width of the image data */
    int             data_height;  /* height of the image data */
    int             frame_width;  /* width of one image frame */
    unsigned int    *data;        /* the image data */
} image_t;

#define IMG_HOLDER_FRIEND 0
#define IMG_HOLDER_ENEMY  1
#define IMG_BALL          2
#define IMG_SHIP_SELF     3
#define IMG_SHIP_FRIEND   4
#define IMG_SHIP_ENEMY    5
#define IMG_BULLET        6
#define IMG_BULLET_OWN    7
#define IMG_BASE_DOWN     8
#define IMG_BASE_LEFT     9
#define IMG_BASE_UP       10
#define IMG_BASE_RIGHT    11
#define IMG_FUELCELL      12
#define IMG_FUEL          13
#define IMG_ALL_ITEMS     14
#define IMG_CANNON_DOWN   15
#define IMG_CANNON_LEFT   16
#define IMG_CANNON_UP     17
#define IMG_CANNON_RIGHT  18
#define IMG_SPARKS        19
#define IMG_PAUSED        20
#define IMG_REFUEL        21
#define IMG_WORMHOLE      22
#define IMG_MINE_TEAM     23
#define IMG_MINE_OTHER    24
#define IMG_CONCENTRATOR  25
#define IMG_PLUSGRAVITY   26
#define IMG_MINUSGRAVITY  27
#define IMG_CHECKPOINT	  28
#define IMG_METER	  29
#define IMG_ASTEROIDCONC  30
#define IMG_SHIELD        31
#define IMG_ACWISEGRAV    32
#define IMG_CWISEGRAV     33
#define IMG_MISSILE       34
#define IMG_ASTEROID      35
#define IMG_TARGET        36
#define IMG_HUD_ITEMS     37

int Images_init(void);
void Images_cleanup(void);
void Image_paint(int ind, int x, int y, int frame, int c);
void Image_paint_area(int ind, int x, int y, int frame, irec_t *r, int c);
void Image_paint_rotated(int ind, int center_x, int center_y, int dir, int color);
image_t *Image_get(int ind);
image_t *Image_get_texture(int ind);
void Image_use_texture(int ind);
void Image_no_texture(void);

#endif
