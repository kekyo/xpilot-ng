/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sdlcompat.h"

int Sdl_blit_surface_unblended(SDL_Surface *source,
			       const SDL_Rect *source_rect,
			       SDL_Surface *destination,
			       SDL_Rect *destination_rect)
{
    SDL_BlendMode saved_blend_mode;
    Uint8 saved_alpha;
    Uint8 saved_red;
    Uint8 saved_green;
    Uint8 saved_blue;
    int result;
    int restore_result = 0;

    if (source == NULL || destination == NULL)
	return SDL_SetError("Source and destination surfaces must not be NULL");

    if (SDL_GetSurfaceBlendMode(source, &saved_blend_mode) < 0
	|| SDL_GetSurfaceAlphaMod(source, &saved_alpha) < 0
	|| SDL_GetSurfaceColorMod(source, &saved_red, &saved_green,
				  &saved_blue) < 0)
	return -1;

    if (SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_NONE) < 0)
	return -1;
    if (SDL_SetSurfaceAlphaMod(source, SDL_ALPHA_OPAQUE) < 0) {
	SDL_SetSurfaceBlendMode(source, saved_blend_mode);
	return -1;
    }
    if (SDL_SetSurfaceColorMod(source, SDL_ALPHA_OPAQUE,
			       SDL_ALPHA_OPAQUE, SDL_ALPHA_OPAQUE) < 0) {
	SDL_SetSurfaceAlphaMod(source, saved_alpha);
	SDL_SetSurfaceBlendMode(source, saved_blend_mode);
	return -1;
    }

    result = SDL_BlitSurface(source, source_rect, destination,
			     destination_rect);

    /* Attempt every restoration even if one of the SDL calls fails. */
    if (SDL_SetSurfaceColorMod(source, saved_red, saved_green,
			       saved_blue) < 0)
	restore_result = -1;
    if (SDL_SetSurfaceAlphaMod(source, saved_alpha) < 0)
	restore_result = -1;
    if (SDL_SetSurfaceBlendMode(source, saved_blend_mode) < 0)
	restore_result = -1;

    return result < 0 || restore_result < 0 ? -1 : 0;
}
