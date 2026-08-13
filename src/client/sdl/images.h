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
#include "renderer.h"

/** Preparation state of a registered image. */
typedef enum image_state_e {
    /** The registration has no GPU resources yet. */
    IMG_STATE_UNINITIALIZED,
    /** Both semantic and transitional texture resources are ready. */
    IMG_STATE_READY
} image_state_e;

/**
 * Information needed to paint a registered horizontal image atlas.
 *
 * One image may contain multiple frames that represent the same object in
 * different states. If rotate flag is true, the image will be rotated
 * when it is created to generate num_frames-1 of new frames.
 * Width is the natural semantic atlas width across all frames. Frame_width is
 * the width of any single frame. Data_width and data_height include
 * power-of-two atlas padding retained only for the compatibility renderer.
 */
typedef struct image_t {
    /** Transitional fixed-function OpenGL texture name. */
    GLuint legacy_name;
    /** Backend-neutral texture used by semantic sprite commands. */
    RendererTexture *texture;
    /** Renderer that owns @ref texture. */
    Renderer *renderer;
    /** Image filename resolved by the picture loader. */
    char *filename;
    /** Number of frames in the horizontal atlas. */
    int num_frames;
    /** Whether the picture loader generates rotated source frames. */
    bool rotate;
    /** Whether this registration may be scaled to fill its allocation. */
    bool scale;
    /** Immutable semantic texture filter selection. */
    RendererTextureFilter filter;
    /** Current preparation state. */
    image_state_e state;
    /** Natural semantic texture width across all frames. */
    int width;
    /** Natural semantic texture height. */
    int height;
    /** Width of the power-of-two compatibility texture data. */
    int data_width;
    /** Height of the power-of-two compatibility texture data. */
    int data_height;
    /** Width of one frame. */
    int frame_width;
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

/**
 * Register the built-in image set without loading it.
 *
 * @return 0 after registrations are added.
 */
int Images_init(void);

/**
 * Prepare every newly registered image as an atomic batch.
 *
 * @param renderer Renderer that will own semantic textures.
 * @return 0 on success, or -1 if any image could not be prepared.
 *
 * @remarks Resource creation must occur outside an active frame. On failure,
 * all resources created by this call are released and registrations remain
 * unprepared for a later retry. Already prepared registrations are unchanged.
 */
int Images_prepare(Renderer *renderer);

/** Release all prepared resources and reset the image registry. */
void Images_cleanup(void);

/**
 * Paint one complete image frame in bottom-left-origin world coordinates.
 *
 * @param ind Registration index.
 * @param x Destination left edge.
 * @param y Destination bottom edge.
 * @param frame Zero-based atlas frame.
 * @param c Packed 0xRRGGBBAA tint.
 *
 * @remarks Drawing requires an active frame. Invalid geometry, ownership
 * mismatches, and renderer failures are retained as the frame's first failure.
 * An undefined registration remains a no-op.
 */
void Image_paint(int ind, int x, int y, int frame, int c);

/**
 * Paint an image-frame subrectangle in bottom-left-origin world coordinates.
 *
 * @param ind Registration index.
 * @param x Destination left edge.
 * @param y Destination bottom edge.
 * @param frame Zero-based atlas frame.
 * @param r Top-left-origin source area, or NULL for the complete frame.
 * @param c Packed 0xRRGGBBAA tint.
 *
 * @remarks Drawing requires an active frame. Invalid geometry, ownership
 * mismatches, and renderer failures are retained as the frame's first failure.
 * An undefined registration remains a no-op.
 */
void Image_paint_area(int ind, int x, int y, int frame, irec_t *r, int c);

/**
 * Paint one complete image frame in top-left-origin HUD coordinates.
 *
 * @param ind Registration index.
 * @param x Destination left edge.
 * @param y Destination top edge.
 * @param frame Zero-based atlas frame.
 * @param c Packed 0xRRGGBBAA tint.
 *
 * @remarks Drawing requires an active frame. Invalid geometry, ownership
 * mismatches, and renderer failures are retained as the frame's first failure.
 * An undefined registration remains a no-op.
 */
void Image_paint_hud(int ind, int x, int y, int frame, int c);

/**
 * Paint frame zero around a world-coordinate center with table-unit rotation.
 *
 * @param ind Registration index.
 * @param center_x Destination center X coordinate.
 * @param center_y Destination center Y coordinate.
 * @param dir Counterclockwise angle in TABLE_SIZE units per full turn.
 * @param color Packed 0xRRGGBBAA tint.
 *
 * @remarks Drawing requires an active frame. Invalid geometry, ownership
 * mismatches, and renderer failures are retained as the frame's first failure.
 * An undefined registration remains a no-op.
 */
void Image_paint_rotated(int ind, int center_x, int center_y, int dir, int color);

/**
 * Return a prepared registered image.
 *
 * @param ind Registration index.
 * @return Borrowed image, or NULL if it is invalid or unprepared.
 */
image_t *Image_get(int ind);

/**
 * Return a prepared map texture by its texture-relative index.
 *
 * @param ind Zero-based index relative to the first map texture.
 * @return Borrowed image, or NULL if it is invalid or unprepared.
 */
image_t *Image_get_texture(int ind);

/**
 * Bind a prepared map texture for transitional fixed-function drawing.
 *
 * @param ind Zero-based index relative to the first map texture.
 */
void Image_use_texture(int ind);

/** Disable transitional fixed-function texturing. */
void Image_no_texture(void);

/**
 * Add an image registration without loading it.
 *
 * @param filename Picture filename.
 * @param count Positive to generate rotated frames, or negative when the
 * source contains an existing horizontal frame atlas.
 * @param scalable Whether the registration may be scaled.
 * @return Zero-based registration index, or -1 for invalid input or memory
 * allocation failure.
 */
int Bitmap_add(const char *filename, int count, bool scalable);

#endif
