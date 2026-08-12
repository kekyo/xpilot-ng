/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sdluiimage.h"

#include "surface_rgba8.h"

#include <string.h>

static int Sdl_ui_image_is_empty(const SdlUiImage *image)
{
    return image->renderer == NULL && image->texture == NULL
        && image->content_width == 0 && image->content_height == 0
        && image->texture_width == 0 && image->texture_height == 0;
}

void Sdl_ui_draw_state_init(SdlUiDrawState *state)
{
    if (state != NULL) {
        state->status = RENDERER_STATUS_OK;
        state->successful_draws = 0;
    }
}

RendererStatus Sdl_ui_draw_state_status(const SdlUiDrawState *state)
{
    return state != NULL ? state->status : RENDERER_STATUS_INVALID_ARGUMENT;
}

size_t Sdl_ui_draw_state_successful_draws(const SdlUiDrawState *state)
{
    return state != NULL ? state->successful_draws : 0;
}

static RendererStatus Sdl_ui_image_create(SdlUiImage *candidate,
                                          Renderer *renderer,
                                          SDL_Surface *surface)
{
    SdlRgba8Image *staging;
    RendererTextureDesc desc;
    RendererTexture *texture = NULL;
    RendererStatus status;

    memset(candidate, 0, sizeof(*candidate));
    staging = Sdl_rgba8_image_create(surface);
    if (staging == NULL)
        return RENDERER_STATUS_BACKEND_ERROR;

    desc.width = staging->texture_width;
    desc.height = staging->texture_height;
    desc.filter = RENDERER_TEXTURE_FILTER_NEAREST;
    desc.wrap = RENDERER_TEXTURE_WRAP_CLAMP;
    status = Renderer_texture_create_with_desc(
        renderer, &desc, staging->pixels, staging->pitch, &texture);
    if (status == RENDERER_STATUS_OK) {
        candidate->renderer = renderer;
        candidate->texture = texture;
        candidate->content_width = staging->content_width;
        candidate->content_height = staging->content_height;
        candidate->texture_width = staging->texture_width;
        candidate->texture_height = staging->texture_height;
    }
    Sdl_rgba8_image_destroy(staging);
    return status;
}

RendererStatus Sdl_ui_image_init(SdlUiImage *image, Renderer *renderer,
                                 SDL_Surface *surface)
{
    SdlUiImage candidate;
    RendererStatus status;

    if (image == NULL || renderer == NULL || surface == NULL
        || surface->w <= 0 || surface->h <= 0) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (!Sdl_ui_image_is_empty(image))
        return RENDERER_STATUS_INVALID_STATE;

    status = Sdl_ui_image_create(&candidate, renderer, surface);
    if (status == RENDERER_STATUS_OK)
        *image = candidate;
    return status;
}

RendererStatus Sdl_ui_image_pair_init(SdlUiImagePair *pair,
                                      Renderer *renderer,
                                      SDL_Surface *first_surface,
                                      SDL_Surface *second_surface)
{
    SdlUiImagePair candidate;
    RendererStatus status;
    RendererStatus cleanup_status;

    if (pair == NULL || renderer == NULL || first_surface == NULL
        || second_surface == NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (!Sdl_ui_image_is_empty(&pair->first)
        || !Sdl_ui_image_is_empty(&pair->second)) {
        return RENDERER_STATUS_INVALID_STATE;
    }

    memset(&candidate, 0, sizeof(candidate));
    status = Sdl_ui_image_init(&candidate.first, renderer, first_surface);
    if (status != RENDERER_STATUS_OK)
        return status;
    status = Sdl_ui_image_init(&candidate.second, renderer, second_surface);
    if (status != RENDERER_STATUS_OK) {
        cleanup_status = Sdl_ui_image_cleanup(&candidate.first);
        if (cleanup_status != RENDERER_STATUS_OK) {
            /* Keep ownership reachable when a backend cannot roll it back. */
            *pair = candidate;
            return cleanup_status;
        }
        return status;
    }

    *pair = candidate;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_ui_image_draw(SdlRenderer *sdl_renderer,
                                 const SdlUiImage *image,
                                 const RendererRect *bounds,
                                 RendererColor tint)
{
    Renderer *renderer;
    RendererStatus status;
    float left;
    float top;

    if (sdl_renderer == NULL || image == NULL || bounds == NULL
        || bounds->width <= 0 || bounds->height <= 0) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (image->renderer == NULL || image->texture == NULL
        || image->content_width <= 0 || image->content_height <= 0
        || image->texture_width <= 0 || image->texture_height <= 0) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    renderer = Sdl_renderer_frontend(sdl_renderer);
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer != image->renderer)
        return RENDERER_STATUS_RESOURCE_MISMATCH;

    left = (float)bounds->x;
    top = (float)bounds->y;
    status = Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA);
    if (status == RENDERER_STATUS_OK) {
        status = Renderer_draw_sprite(
            renderer, image->texture,
            left, top,
            left + (float)bounds->width,
            top + (float)bounds->height,
            0.0f, 0.0f,
            (float)image->content_width / (float)image->texture_width,
            (float)image->content_height / (float)image->texture_height,
            tint);
    }
    if (status == RENDERER_STATUS_OK)
        status = Sdl_renderer_flush_preserving_legacy(sdl_renderer);
    return status;
}

RendererStatus Sdl_ui_image_draw_tracked(SdlUiDrawState *state,
                                         SdlRenderer *sdl_renderer,
                                         const SdlUiImage *image,
                                         const RendererRect *bounds,
                                         RendererColor tint)
{
    if (state == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (state->status != RENDERER_STATUS_OK)
        return state->status;

    state->status = Sdl_ui_image_draw(sdl_renderer, image, bounds, tint);
    if (state->status == RENDERER_STATUS_OK)
        state->successful_draws++;
    return state->status;
}

RendererStatus Sdl_ui_image_cleanup(SdlUiImage *image)
{
    RendererStatus status;

    if (image == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (image->texture == NULL) {
        memset(image, 0, sizeof(*image));
        return RENDERER_STATUS_OK;
    }
    if (image->renderer == NULL)
        return RENDERER_STATUS_INVALID_STATE;

    status = Renderer_texture_destroy(image->renderer, image->texture);
    if (status == RENDERER_STATUS_OK)
        memset(image, 0, sizeof(*image));
    return status;
}

RendererStatus Sdl_ui_image_pair_cleanup(SdlUiImagePair *pair)
{
    RendererStatus status;

    if (pair == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Sdl_ui_image_cleanup(&pair->first);
    if (status != RENDERER_STATUS_OK)
        return status;
    return Sdl_ui_image_cleanup(&pair->second);
}
