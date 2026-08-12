/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDLUIIMAGE_H
#define SDLUIIMAGE_H

#include "renderer.h"
#include "sdlrenderer.h"

#include <SDL.h>

/** Renderer-owned texture and dimensions for one top-left-origin UI image. */
typedef struct SdlUiImage {
    /** Renderer that owns texture, or NULL when the image is empty. */
    Renderer *renderer;
    /** Semantic texture containing the image, or NULL when empty. */
    RendererTexture *texture;
    /** Width of meaningful image pixels. */
    int content_width;
    /** Height of meaningful image pixels. */
    int content_height;
    /** Width of the power-of-two texture allocation. */
    int texture_width;
    /** Height of the power-of-two texture allocation. */
    int texture_height;
} SdlUiImage;

/** Two UI images published and normally released as one resource group. */
typedef struct SdlUiImagePair {
    /** First image in the group. */
    SdlUiImage first;
    /** Second image in the group. */
    SdlUiImage second;
} SdlUiImagePair;

/**
 * Create one nearest-filtered, edge-clamped UI texture.
 *
 * @param image Empty destination image. It is changed only after texture
 *        creation succeeds.
 * @param renderer Renderer that will own the texture, outside a frame.
 * @param surface Borrowed nonempty SDL surface; ownership remains unchanged.
 * @return Operation status.
 *
 * @remarks The destination must be zero-initialized. The image must be
 * cleaned up before its owning renderer is destroyed.
 */
RendererStatus Sdl_ui_image_init(SdlUiImage *image, Renderer *renderer,
                                 SDL_Surface *surface);

/**
 * Create two UI textures and publish them together.
 *
 * @param pair Empty destination pair. If either creation fails, a successfully
 *        created candidate is released before failure is returned.
 * @param renderer Renderer that will own both textures, outside a frame.
 * @param first_surface Borrowed nonempty surface for the first image.
 * @param second_surface Borrowed nonempty surface for the second image.
 * @return Operation status.
 *
 * @remarks The destination must be zero-initialized. The pair must be
 * cleaned up before its owning renderer is destroyed. If candidate rollback
 * itself cannot complete, the retained candidate remains reachable through
 * @p pair so cleanup can be retried.
 */
RendererStatus Sdl_ui_image_pair_init(SdlUiImagePair *pair,
                                      Renderer *renderer,
                                      SDL_Surface *first_surface,
                                      SDL_Surface *second_surface);

/**
 * Draw an image into a destination rectangle and restore legacy GL state.
 *
 * @param sdl_renderer Active SDL renderer facade.
 * @param image Image owned by the facade's frontend renderer.
 * @param bounds Positive destination rectangle in current logical coordinates.
 * @param tint Unpremultiplied color multiplied with sampled pixels.
 * @return Operation status.
 */
RendererStatus Sdl_ui_image_draw(SdlRenderer *sdl_renderer,
                                 const SdlUiImage *image,
                                 const RendererRect *bounds,
                                 RendererColor tint);

/**
 * Release one UI texture.
 *
 * @param image Image to release. An empty image is accepted.
 * @return Operation status. On failure, the image retains its ownership state
 *         so cleanup can be retried.
 */
RendererStatus Sdl_ui_image_cleanup(SdlUiImage *image);

/**
 * Release both textures in a UI image pair.
 *
 * @param pair Pair to release. An empty pair is accepted.
 * @return Operation status. Cleanup stops at the first failure; that image
 *         and every image not yet visited retain ownership for retry.
 */
RendererStatus Sdl_ui_image_pair_cleanup(SdlUiImagePair *pair);

#endif /* SDLUIIMAGE_H */
