/* SDL window and compatibility OpenGL renderer coordination. */

#ifndef SDLRENDERER_H
#define SDLRENDERER_H

#include "renderer.h"

#include <SDL.h>

/** Renderer facade borrowing one SDL OpenGL window and its current context. */
typedef struct SdlRenderer SdlRenderer;

/** Coordinate origin restored for transitional fixed-function drawing. */
typedef enum SdlRendererLegacyOrigin {
    /** Logical coordinates start at the bottom-left and increase upward. */
    SDL_RENDERER_LEGACY_BOTTOM_LEFT,
    /** Logical coordinates start at the top-left and increase downward. */
    SDL_RENDERER_LEGACY_TOP_LEFT
} SdlRendererLegacyOrigin;

/**
 * Create a compatibility renderer for the current SDL OpenGL context.
 *
 * @param window Borrowed SDL window owning the current context.
 * @param renderer Receives the new facade on success.
 * @return Operation status.
 *
 * @remarks The window and its context remain caller-owned. The same context
 * must be current for every operation, and the facade must be destroyed
 * before the context or window.
 */
RendererStatus Sdl_renderer_create(SDL_Window *window,
                                   SdlRenderer **renderer);

/**
 * Destroy a renderer facade without destroying its SDL window or context.
 *
 * @param renderer Facade to destroy, or NULL for no operation.
 */
void Sdl_renderer_destroy(SdlRenderer *renderer);

/**
 * Return the backend-neutral renderer used by a facade.
 *
 * @param renderer Renderer facade.
 * @return Borrowed renderer, or NULL for an invalid facade.
 */
Renderer *Sdl_renderer_frontend(SdlRenderer *renderer);

/**
 * Query the current OpenGL drawable size in physical pixels.
 *
 * @param renderer Renderer facade.
 * @param width Receives the drawable width.
 * @param height Receives the drawable height.
 * @return Operation status.
 */
RendererStatus Sdl_renderer_get_drawable_size(SdlRenderer *renderer,
                                              int *width, int *height);

/**
 * Begin and clear a frame using the window's current drawable dimensions.
 *
 * @param renderer Renderer facade.
 * @param logical_width SDL logical window width used by legacy drawing.
 * @param logical_height SDL logical window height used by legacy drawing.
 * @param clear_color Clear color for the whole drawable.
 * @return Operation status.
 */
RendererStatus Sdl_renderer_begin_frame(SdlRenderer *renderer,
                                        int logical_width,
                                        int logical_height,
                                        RendererColor clear_color);

/**
 * Flush semantic commands and restore deterministic fixed-function state.
 *
 * @param renderer Renderer facade with an active frame.
 * @param origin Logical-coordinate origin to establish.
 * @return Operation status.
 *
 * @remarks This is the ordering barrier used while semantic renderer calls
 * and direct compatibility OpenGL calls coexist. It is removed once all
 * production drawing uses the semantic renderer.
 */
RendererStatus Sdl_renderer_prepare_legacy(
    SdlRenderer *renderer, SdlRendererLegacyOrigin origin);

/**
 * Flush pending semantic commands and finish the active frame.
 *
 * @param renderer Renderer facade.
 * @return Operation status. A failed frame remains active for retry.
 */
RendererStatus Sdl_renderer_end_frame(SdlRenderer *renderer);

#endif /* SDLRENDERER_H */
