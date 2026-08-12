/* Internal seam shared by the renderer frontend and concrete backends. */

#ifndef RENDERER_BACKEND_H
#define RENDERER_BACKEND_H

#include "renderer.h"

/** Fully lowered draw request passed from the frontend to a backend. */
typedef struct RendererBackendDraw {
    /** Local-to-framebuffer transform captured at submission. */
    RendererTransform2D transform;
    /** Blend mode captured at submission. */
    RendererBlendMode blend;
    /** Nonzero when scissoring is enabled. */
    int scissor_enabled;
    /** Clamped framebuffer-space scissor rectangle. */
    RendererRect scissor;
    /** Backend texture handle, or NULL for an untextured draw. */
    void *texture;
    /** Backend mesh handle, or NULL for a vertex draw. */
    void *mesh;
    /** Triangle vertices, or NULL for a mesh draw. */
    const RendererVertex2D *vertices;
    /** Number of triangle vertices. */
    size_t vertex_count;
} RendererBackendDraw;

/** Operations implemented by a concrete renderer backend. */
typedef struct RendererBackendInterface {
    /** Begin and clear a frame. */
    RendererStatus (*begin_frame)(void *context, int width, int height,
                                  RendererColor clear_color);
    /** Accept one fully lowered draw. */
    RendererStatus (*draw)(void *context,
                           const RendererBackendDraw *draw);
    /** Submit all accepted draws to the graphics API. */
    RendererStatus (*flush)(void *context);
    /** Finish the current frame. */
    RendererStatus (*end_frame)(void *context);
    /** Create a backend RGBA8 texture handle. */
    RendererStatus (*texture_create)(void *context,
                                     const RendererTextureDesc *desc,
                                     const uint8_t *rgba_pixels,
                                     size_t pitch, void **handle);
    /** Update a backend RGBA8 texture handle. */
    RendererStatus (*texture_update)(void *context, void *handle,
                                     RendererRect region,
                                     const uint8_t *rgba_pixels,
                                     size_t pitch);
    /** Destroy a backend texture handle. */
    void (*texture_destroy)(void *context, void *handle);
    /** Create a backend triangle mesh handle. */
    RendererStatus (*mesh_create)(void *context,
                                  const RendererVertex2D *vertices,
                                  size_t vertex_count, void **handle);
    /** Update a backend triangle mesh handle. */
    RendererStatus (*mesh_update)(void *context, void *handle,
                                  const RendererVertex2D *vertices,
                                  size_t vertex_count);
    /** Destroy a backend mesh handle. */
    void (*mesh_destroy)(void *context, void *handle);
    /** Destroy backend context data after every resource is released. */
    void (*destroy)(void *context);
} RendererBackendInterface;

/**
 * Construct a renderer around a backend interface.
 *
 * This internal factory keeps the semantic frontend independent of SDL and
 * OpenGL. Concrete backend factories wrap it, while tests supply a fake
 * backend through the same boundary.
 *
 * @param interface Complete backend interface retained by the renderer.
 * @param context Opaque backend context passed to every callback.
 * @return Renderer instance, or NULL when arguments or allocation fail.
 */
Renderer *Renderer_backend_create(const RendererBackendInterface *interface,
                                  void *context);

#endif /* RENDERER_BACKEND_H */
