/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SURFACE_RGBA8_H
#define SURFACE_RGBA8_H

#include <SDL3/SDL.h>

#include <stddef.h>
#include <stdint.h>

/** Owned top-left-origin RGBA8 pixels prepared for a renderer texture. */
typedef struct SdlRgba8Image {
    /** Pixel bytes in R, G, B, A order. */
    uint8_t *pixels;
    /** Power-of-two pixel-buffer width. */
    int texture_width;
    /** Power-of-two pixel-buffer height. */
    int texture_height;
    /** Width of meaningful source content. */
    int content_width;
    /** Height of meaningful source content. */
    int content_height;
    /** Number of bytes from one pixel row to the next. */
    size_t pitch;
} SdlRgba8Image;

/**
 * Convert an SDL surface to an RGBA8 power-of-two staging image.
 *
 * Source pixels retain their top-left orientation. Padding and color-keyed
 * pixels are transparent black. The source surface remains owned by the
 * caller, and its blend and modulation state is restored before return.
 *
 * @param source Borrowed nonempty SDL surface.
 * @return Owned staging image, or NULL with an SDL error on failure.
 */
SdlRgba8Image *Sdl_rgba8_image_create(SDL_Surface *source);

/**
 * Release a staging image.
 *
 * @param image Image returned by Sdl_rgba8_image_create, or NULL.
 */
void Sdl_rgba8_image_destroy(SdlRgba8Image *image);

#endif /* SURFACE_RGBA8_H */
