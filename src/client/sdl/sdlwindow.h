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

#ifndef SDLWINDOW_H
#define SDLWINDOW_H

#include "renderer.h"

#include <SDL.h>

/** CPU surface and renderer texture backing one off-screen HUD window. */
typedef struct sdl_window_t {
    /** Power-of-two RGBA32 surface modified by the window's producer. */
    SDL_Surface *surface;
    /** Renderer that owns @ref texture after the first successful prepare. */
    Renderer *renderer;
    /** Current immutable renderer texture, or NULL before preparation. */
    RendererTexture *texture;
    /** Left position in top-left-origin HUD coordinates. */
    int x;
    /** Top position in top-left-origin HUD coordinates. */
    int y;
    /** Visible width in pixels. */
    int w;
    /** Visible height in pixels. */
    int h;
    /** Nonzero when surface changes have not yet reached a texture. */
    int dirty;
} sdl_window_t;

/**
 * Initialize an off-screen RGBA32 surface without creating GPU resources.
 *
 * @param win Window storage to initialize.
 * @param x Left position in top-left-origin HUD coordinates.
 * @param y Top position in top-left-origin HUD coordinates.
 * @param w Positive visible width.
 * @param h Positive visible height.
 * @return Zero on success, or -1 on invalid dimensions or allocation failure.
 */
int sdl_window_init(sdl_window_t *win, int x, int y, int w, int h);

/**
 * Create or replace the texture for dirty surface contents.
 *
 * @param win Initialized off-screen window.
 * @param renderer Renderer that owns the resulting texture.
 * @return Zero on success, or -1 without changing published resources.
 *
 * @remarks This function must be called outside an active renderer frame.
 */
int sdl_window_prepare(sdl_window_t *win, Renderer *renderer);

/**
 * Move an off-screen window in screen coordinates.
 *
 * @param win Initialized off-screen window.
 * @param x New left position.
 * @param y New top position.
 */
void sdl_window_move(sdl_window_t *win, int x, int y);

/**
 * Resize an off-screen window and preserve overlapping surface contents.
 *
 * @param win Initialized off-screen window.
 * @param w Positive new visible width.
 * @param h Positive new visible height.
 * @return Zero on success, or -1 with the old surface and texture preserved.
 *
 * @remarks If the window has a texture, this function must be called outside
 * an active renderer frame.
 */
int sdl_window_resize(sdl_window_t *win, int w, int h);

/**
 * Mark surface contents dirty for the next preparation pass.
 *
 * @param win Initialized off-screen window.
 */
void sdl_window_refresh(sdl_window_t *win);

/**
 * Draw a prepared off-screen window and its colored frame.
 *
 * @param win Prepared off-screen window.
 * @param background Optional color drawn behind the sprite and frame.
 * @return The operation status or the first failure retained for the frame.
 *
 * @remarks Drawing requires an active renderer frame. All accepted semantic
 * commands are flushed together while preserving transitional compatibility
 * state, including commands accepted before a later operation fails.
 */
RendererStatus sdl_window_paint(
    sdl_window_t *win, const RendererColor *background);

/**
 * Release an off-screen window surface and texture.
 *
 * @param win Window to reset, or NULL for no operation.
 *
 * @remarks This function is idempotent and must run outside a renderer frame.
 */
void sdl_window_destroy(sdl_window_t *win);

#endif /* SDLWINDOW_H */
