/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client. Copyright (C) 2003-2004 by 
 *
 *     Juha Lindström <juhal@users.sourceforge.net>
 *     Erik Andersson <maximan@users.sourceforge.net>
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

#ifndef RADAR_H
#define RADAR_H

#include "renderer.h"

/**
 * Mark the radar world image for recreation during the next prepare pass.
 *
 * This function only records CPU-side state and is safe while a renderer
 * frame is active.
 */
void Radar_update(void);

/**
 * Create or replace dirty radar surface and texture resources atomically.
 *
 * @param renderer Renderer that owns the resulting texture.
 * @return Zero on success, or -1 without changing published resources.
 *
 * @remarks This function must be called outside an active renderer frame.
 */
int Radar_prepare(Renderer *renderer);

#ifdef XPILOT_RADAR_TEST_HOOKS
#include <SDL3/SDL.h>

/** @return Borrowed radar surface currently published to drawing code. */
SDL_Surface *Radar_test_surface(void);

/** @return Borrowed semantic texture currently published to drawing code. */
RendererTexture *Radar_test_texture(void);

/** @return Current committed visible radar bounds. */
SDL_Rect Radar_test_bounds(void);
#endif

#endif
