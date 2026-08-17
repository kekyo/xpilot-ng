/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "surface_rgba8.h"

#include "sdlcompat.h"

#include <limits.h>
#include <stdlib.h>

static int Power_of_two_ceil(int value, int *result)
{
    int power = 1;

    if (value <= 0 || result == NULL)
        return 0;
    while (power < value) {
        if (power > INT_MAX / 2)
            return 0;
        power *= 2;
    }
    *result = power;
    return 1;
}

SdlRgba8Image *Sdl_rgba8_image_create(SDL_Surface *source)
{
    SdlRgba8Image *image = NULL;
    SDL_Surface *destination = NULL;
    SDL_Rect source_rect;
    SDL_Rect destination_rect = {0, 0, 0, 0};
    size_t allocation_size;

    if (source == NULL || source->w <= 0 || source->h <= 0) {
        SDL_SetError("Source surface must be nonempty");
        return NULL;
    }
    image = calloc(1, sizeof(*image));
    if (image == NULL) {
        SDL_SetError("Could not allocate RGBA8 image metadata");
        return NULL;
    }
    if (!Power_of_two_ceil(source->w, &image->texture_width)
        || !Power_of_two_ceil(source->h, &image->texture_height)
        || image->texture_width > INT_MAX / 4) {
        SDL_SetError("Surface dimensions exceed RGBA8 texture limits");
        goto failure;
    }
    image->pitch = (size_t)image->texture_width * 4;
    if (image->pitch > INT_MAX
        || (size_t)image->texture_height > SIZE_MAX / image->pitch) {
        SDL_SetError("RGBA8 texture allocation size overflow");
        goto failure;
    }
    allocation_size = (size_t)image->texture_height * image->pitch;
    image->pixels = calloc(1, allocation_size);
    if (image->pixels == NULL) {
        SDL_SetError("Could not allocate RGBA8 texture pixels");
        goto failure;
    }
    destination = SDL_CreateSurfaceFrom(
        image->texture_width, image->texture_height, SDL_PIXELFORMAT_RGBA32,
        image->pixels, (int)image->pitch);
    if (destination == NULL)
        goto failure;

    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = source->w;
    source_rect.h = source->h;
    if (Sdl_blit_surface_unblended(source, &source_rect, destination,
                                   &destination_rect) < 0) {
        goto failure;
    }
    SDL_DestroySurface(destination);
    image->content_width = source->w;
    image->content_height = source->h;
    return image;

failure:
    SDL_DestroySurface(destination);
    Sdl_rgba8_image_destroy(image);
    return NULL;
}

void Sdl_rgba8_image_destroy(SdlRgba8Image *image)
{
    if (image == NULL)
        return;
    free(image->pixels);
    free(image);
}
