/* SDL window and OpenGL core renderer coordination. */

#ifndef SDLRENDERER_H
#define SDLRENDERER_H

#include "renderer.h"

#include <SDL.h>

/** Renderer facade borrowing one SDL OpenGL window and its current context. */
typedef struct SdlRenderer SdlRenderer;

/**
 * Create a semantic renderer for the current SDL OpenGL core context.
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
 * Retain the first semantic rendering failure for the active frame.
 *
 * @param renderer Renderer facade with an active frame.
 * @param status Result of a semantic rendering operation.
 * @return The first retained failure, or OK when no failure is retained.
 *
 * @remarks A successful status never replaces a retained failure. The result
 * is reset only after a subsequent frame begins successfully.
 */
RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status);

/**
 * Return the semantic rendering result retained for the current or most
 * recently completed frame.
 *
 * @param renderer Renderer facade.
 * @return The retained frame result, or INVALID_ARGUMENT for NULL.
 */
RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer);

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
 * Set the semantic draw transform and retain any failure for the frame.
 *
 * @param renderer Renderer facade with an active frame.
 * @param transform Column-major local-to-framebuffer transform.
 * @return The operation status or the first failure retained for the frame.
 *
 * @remarks Once a semantic operation has failed, later transform changes are
 * rejected until a subsequent frame begins successfully.
 */
RendererStatus Sdl_renderer_set_transform_2d(
    SdlRenderer *renderer, RendererTransform2D transform);

/**
 * Begin and clear a frame using the window's current drawable dimensions.
 *
 * @param renderer Renderer facade.
 * @param logical_width SDL logical window width used for coordinate scaling.
 * @param logical_height SDL logical window height used for coordinate scaling.
 * @param clear_color Clear color for the whole drawable.
 * @return Operation status.
 */
RendererStatus Sdl_renderer_begin_frame(SdlRenderer *renderer,
                                        int logical_width,
                                        int logical_height,
                                        RendererColor clear_color);

/**
 * Set or disable semantic clipping in logical window coordinates.
 *
 * @param renderer Renderer facade with an active frame.
 * @param scissor Positive logical top-left-origin rectangle, or NULL to
 *        disable clipping.
 * @return Operation status.
 *
 * @remarks The rectangle is clamped to the logical frame before its edges
 * are scaled outward to physical drawable pixels. The resulting
 * framebuffer-space scissor is independent of the current draw transform.
 */
RendererStatus Sdl_renderer_set_logical_scissor(
    SdlRenderer *renderer, const RendererRect *scissor);

/**
 * Flush pending semantic commands.
 *
 * @param renderer Renderer facade with an active frame.
 * @return Operation status. Pending commands remain retryable after failure.
 *
 * @remarks This is an ordering barrier. It does not finish the active frame.
 */
RendererStatus Sdl_renderer_flush(SdlRenderer *renderer);

/**
 * Flush pending semantic commands and finish the active frame.
 *
 * @param renderer Renderer facade.
 * @return Operation status. A failed frame remains active for retry.
 */
RendererStatus Sdl_renderer_end_frame(SdlRenderer *renderer);

#endif /* SDLRENDERER_H */
