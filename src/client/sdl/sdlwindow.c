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

#include "xpclient_sdl.h"

#include "sdlwindow.h"
#include "error.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static int next_power_of_two(int value, int *result)
{
    int power = 1;

    if (value <= 0 || result == NULL)
	return -1;
    while (power < value) {
	if (power > INT_MAX / 2)
	    return -1;
	power *= 2;
    }
    *result = power;
    return 0;
}

static SDL_Surface *create_surface(int width, int height)
{
    SDL_Surface *surface;
    int texture_width;
    int texture_height;

    if (next_power_of_two(width, &texture_width) != 0
	|| next_power_of_two(height, &texture_height) != 0) {
	error("off-screen window dimensions are too large");
	return NULL;
    }

    surface = SDL_CreateSurface(texture_width, texture_height,
				SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
	error("failed to create SDL surface: %s", SDL_GetError());
	return NULL;
    }
    if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)) {
	error("failed to disable SDL surface blending: %s", SDL_GetError());
	SDL_DestroySurface(surface);
	return NULL;
    }
    if (!SDL_FillSurfaceRect(surface, NULL, 0)) {
	error("failed to clear SDL surface: %s", SDL_GetError());
	SDL_DestroySurface(surface);
	return NULL;
    }
    return surface;
}

static int create_texture(Renderer *renderer, SDL_Surface *surface,
			  RendererTexture **texture)
{
    RendererTextureDesc desc;

    if (renderer == NULL || surface == NULL || surface->pixels == NULL
	|| surface->pitch <= 0 || texture == NULL) {
	return -1;
    }

    desc.width = surface->w;
    desc.height = surface->h;
    desc.filter = RENDERER_TEXTURE_FILTER_NEAREST;
    desc.wrap = RENDERER_TEXTURE_WRAP_CLAMP;
    if (Renderer_texture_create_with_desc(
	    renderer, &desc, surface->pixels, (size_t)surface->pitch,
	    texture) != RENDERER_STATUS_OK) {
	warn("failed to create off-screen window texture");
	return -1;
    }
    return 0;
}

static int replace_texture(sdl_window_t *win, Renderer *renderer,
			   SDL_Surface *surface,
			   RendererTexture **replacement)
{
    RendererTexture *candidate = NULL;

    if (create_texture(renderer, surface, &candidate) != 0)
	return -1;

    if (win->texture != NULL
	&& Renderer_texture_destroy(win->renderer, win->texture)
	   != RENDERER_STATUS_OK) {
	warn("failed to replace off-screen window texture");
	if (Renderer_texture_destroy(renderer, candidate)
	    != RENDERER_STATUS_OK) {
	    warn("failed to release replacement off-screen window texture");
	}
	return -1;
    }

    *replacement = candidate;
    return 0;
}

int sdl_window_init(sdl_window_t *win, int x, int y, int width, int height)
{
    SDL_Surface *surface;

    if (win == NULL)
	return -1;
    memset(win, 0, sizeof(*win));
    if (width <= 0 || height <= 0)
	return -1;

    surface = create_surface(width, height);
    if (surface == NULL)
	return -1;

    win->surface = surface;
    win->x = x;
    win->y = y;
    win->w = width;
    win->h = height;
    win->dirty = 1;
    return 0;
}

int sdl_window_prepare(sdl_window_t *win, Renderer *renderer)
{
    RendererTexture *replacement;

    if (win == NULL || renderer == NULL || win->surface == NULL)
	return -1;
    if (win->texture != NULL && win->renderer != renderer)
	return -1;
    if (win->texture != NULL && !win->dirty)
	return 0;

    if (replace_texture(win, renderer, win->surface, &replacement) != 0)
	return -1;

    win->renderer = renderer;
    win->texture = replacement;
    win->dirty = 0;
    return 0;
}

void sdl_window_move(sdl_window_t *win, int x, int y)
{
    if (win == NULL)
	return;
    win->x = x;
    win->y = y;
}

int sdl_window_resize(sdl_window_t *win, int width, int height)
{
    SDL_Surface *surface;
    RendererTexture *replacement = NULL;

    if (win == NULL || width <= 0 || height <= 0)
	return -1;

    surface = create_surface(width, height);
    if (surface == NULL)
	return -1;
    if (win->surface != NULL
	&& !SDL_BlitSurface(win->surface, NULL, surface, NULL)) {
	error("failed to copy SDL surface: %s", SDL_GetError());
	SDL_DestroySurface(surface);
	return -1;
    }

    if (win->texture != NULL
	&& replace_texture(win, win->renderer, surface, &replacement) != 0) {
	SDL_DestroySurface(surface);
	return -1;
    }

    if (win->surface != NULL)
	SDL_DestroySurface(win->surface);
    win->surface = surface;
    win->w = width;
    win->h = height;
    if (replacement != NULL) {
	win->texture = replacement;
	win->dirty = 0;
    } else {
	win->dirty = 1;
    }
    return 0;
}

void sdl_window_refresh(sdl_window_t *win)
{
    if (win != NULL && win->surface != NULL)
	win->dirty = 1;
}

RendererStatus sdl_window_paint(
    sdl_window_t *win, const RendererColor *background)
{
    SdlRenderer *sdl_renderer;
    Renderer *renderer;
    RendererColor white = {255, 255, 255, 255};
    RendererColor black = {0, 0, 0, 255};
    RendererColor green = {0, 144, 0, 255};
    RendererPoint2D border_points[4];
    RendererColor border_colors[4];
    RendererStatus status;
    RendererStatus operation_status;
    int accepted_commands = 0;

    if (win == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    sdl_renderer = Get_sdl_renderer();
    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_STATE;
    status = Sdl_renderer_track_frame_result(
	sdl_renderer, RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
	return status;
    if (win->surface == NULL || win->texture == NULL
	|| win->renderer == NULL || win->w <= 0 || win->h <= 0) {
	return Sdl_renderer_track_frame_result(
	    sdl_renderer, RENDERER_STATUS_INVALID_STATE);
    }
    renderer = Sdl_renderer_frontend(sdl_renderer);
    if (renderer == NULL)
	return Sdl_renderer_track_frame_result(
	    sdl_renderer, RENDERER_STATUS_INVALID_STATE);
    if (renderer != win->renderer)
	return Sdl_renderer_track_frame_result(
	    sdl_renderer, RENDERER_STATUS_RESOURCE_MISMATCH);

    border_points[0] = (RendererPoint2D){
	(float)win->x, (float)win->y + (float)win->h + 2.0f};
    border_points[1] = (RendererPoint2D){
	(float)win->x, (float)win->y};
    border_points[2] = (RendererPoint2D){
	(float)win->x + (float)win->w, (float)win->y};
    border_points[3] = (RendererPoint2D){
	(float)win->x + (float)win->w,
	(float)win->y + (float)win->h + 2.0f};
    border_colors[0] = black;
    border_colors[1] = green;
    border_colors[2] = black;
    border_colors[3] = green;

    operation_status = Renderer_set_blend(
	renderer, RENDERER_BLEND_ALPHA);
    status = Sdl_renderer_track_frame_result(
	sdl_renderer, operation_status);
    if (status == RENDERER_STATUS_OK && background != NULL) {
	operation_status = Renderer_fill_rect(
	    renderer,
	    (float)win->x, (float)win->y,
	    (float)win->w, (float)win->h + 2.0f,
	    *background);
	if (operation_status == RENDERER_STATUS_OK)
	    accepted_commands = 1;
	status = Sdl_renderer_track_frame_result(
	    sdl_renderer, operation_status);
    }
    if (status == RENDERER_STATUS_OK) {
	operation_status = Renderer_draw_sprite(
	    renderer, win->texture,
	    (float)win->x, (float)win->y,
	    (float)win->x + (float)win->w,
	    (float)win->y + (float)win->h,
	    0.0f, 0.0f,
	    (float)win->w / (float)win->surface->w,
	    (float)win->h / (float)win->surface->h,
	    white);
	if (operation_status == RENDERER_STATUS_OK)
	    accepted_commands = 1;
	status = Sdl_renderer_track_frame_result(
	    sdl_renderer, operation_status);
    }
    if (status == RENDERER_STATUS_OK) {
	operation_status = Renderer_stroke_colored_path(
	    renderer, border_points, border_colors, 4, 1.0f, 1);
	if (operation_status == RENDERER_STATUS_OK)
	    accepted_commands = 1;
	status = Sdl_renderer_track_frame_result(
	    sdl_renderer, operation_status);
    }
    if (accepted_commands) {
	operation_status = Sdl_renderer_flush(sdl_renderer);
	status = Sdl_renderer_track_frame_result(
	    sdl_renderer, operation_status);
    }
    return status;
}

void sdl_window_destroy(sdl_window_t *win)
{
    if (win == NULL)
	return;
    if (win->texture != NULL) {
	if (win->renderer == NULL
	    || Renderer_texture_destroy(win->renderer, win->texture)
	       != RENDERER_STATUS_OK) {
	    warn("failed to destroy off-screen window texture");
	    return;
	}
    }
    if (win->surface != NULL)
	SDL_DestroySurface(win->surface);
    memset(win, 0, sizeof(*win));
}
