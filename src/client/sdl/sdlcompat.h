/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDLCOMPAT_H
#define SDLCOMPAT_H

#include <SDL3/SDL.h>

/**
 * Copy an SDL surface without applying its blend or modulation state.
 *
 * @param source Source surface whose pixel values are copied.
 * @param source_rect Optional source rectangle, or NULL for the full surface.
 * @param destination Destination surface.
 * @param destination_rect Optional destination rectangle updated by SDL.
 * @return 0 on success, or -1 on an SDL error.
 *
 * @remarks The source surface's blend mode, alpha modulation, and color
 * modulation are restored before this function returns.  Its color key, if
 * any, remains active, matching SDL 1.x SDL_SetAlpha(surface, 0, 0).
 */
int Sdl_blit_surface_unblended(SDL_Surface *source,
			       const SDL_Rect *source_rect,
			       SDL_Surface *destination,
			       SDL_Rect *destination_rect);

#endif
