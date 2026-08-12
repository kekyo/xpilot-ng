/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <stddef.h>
#include <stdint.h>

/** Renderer instance owning one backend and all resources made for it. */
typedef struct Renderer Renderer;

/** Opaque two-dimensional RGBA texture owned by one renderer. */
typedef struct RendererTexture RendererTexture;

/** Opaque immutable-topology triangle mesh owned by one renderer. */
typedef struct RendererMesh RendererMesh;

/** Result returned by renderer operations. */
typedef enum RendererStatus {
    /** The operation completed successfully. */
    RENDERER_STATUS_OK,
    /** An argument did not satisfy the documented contract. */
    RENDERER_STATUS_INVALID_ARGUMENT,
    /** The operation is not permitted in the current frame state. */
    RENDERER_STATUS_INVALID_STATE,
    /** Memory needed to retain the request could not be allocated. */
    RENDERER_STATUS_OUT_OF_MEMORY,
    /** The selected rendering backend could not complete the operation. */
    RENDERER_STATUS_BACKEND_ERROR,
    /** A resource belongs to a different renderer. */
    RENDERER_STATUS_RESOURCE_MISMATCH
} RendererStatus;

/** Unpremultiplied eight-bit RGBA color. */
typedef struct RendererColor {
    /** Red component. */
    uint8_t red;
    /** Green component. */
    uint8_t green;
    /** Blue component. */
    uint8_t blue;
    /** Alpha component. */
    uint8_t alpha;
} RendererColor;

/** Integer rectangle in top-left-origin framebuffer coordinates. */
typedef struct RendererRect {
    /** Left edge in pixels. */
    int x;
    /** Top edge in pixels. */
    int y;
    /** Width in pixels. */
    int width;
    /** Height in pixels. */
    int height;
} RendererRect;

/** Two-dimensional floating-point position. */
typedef struct RendererPoint2D {
    /** Horizontal coordinate. */
    float x;
    /** Vertical coordinate. */
    float y;
} RendererPoint2D;

/** Vertex consumed by triangle-based two-dimensional renderer commands. */
typedef struct RendererVertex2D {
    /** Horizontal position. */
    float x;
    /** Vertical position. */
    float y;
    /** Horizontal texture coordinate. */
    float u;
    /** Vertical texture coordinate. */
    float v;
    /** Unpremultiplied vertex color. */
    RendererColor color;
} RendererVertex2D;

/** Column-major local-to-framebuffer affine transform. */
typedef struct RendererTransform2D {
    /** Three-by-three column-major matrix elements. */
    float m[9];
} RendererTransform2D;

/** Blend equation selected for subsequent commands. */
typedef enum RendererBlendMode {
    /** Replace destination pixels with source pixels. */
    RENDERER_BLEND_OPAQUE,
    /** Blend with source alpha and one minus source alpha. */
    RENDERER_BLEND_ALPHA,
    /** Add source alpha-weighted color to the destination. */
    RENDERER_BLEND_ADDITIVE
} RendererBlendMode;

/** Texture sampling filter applied when a texture is drawn. */
typedef enum RendererTextureFilter {
    /** Select the nearest texel without interpolation. */
    RENDERER_TEXTURE_FILTER_NEAREST,
    /** Linearly interpolate adjacent texels. */
    RENDERER_TEXTURE_FILTER_LINEAR
} RendererTextureFilter;

/** Texture-coordinate behavior outside the normalized image bounds. */
typedef enum RendererTextureWrap {
    /** Clamp coordinates to the texture edge. */
    RENDERER_TEXTURE_WRAP_CLAMP,
    /** Repeat the texture for every integer coordinate interval. */
    RENDERER_TEXTURE_WRAP_REPEAT
} RendererTextureWrap;

/** Immutable dimensions and sampling behavior of an RGBA8 texture. */
typedef struct RendererTextureDesc {
    /** Positive texture width in pixels. */
    int width;
    /** Positive texture height in pixels. */
    int height;
    /** Minification and magnification filter. */
    RendererTextureFilter filter;
    /** Horizontal and vertical texture-coordinate behavior. */
    RendererTextureWrap wrap;
} RendererTextureDesc;

/**
 * Convert a packed 0xRRGGBBAA value to explicit components.
 *
 * @param rgba Packed color independent of host byte order.
 * @return Explicit color components.
 */
RendererColor Renderer_color_from_rgba32(uint32_t rgba);

/**
 * Convert explicit components to a packed 0xRRGGBBAA value.
 *
 * @param color Explicit color components.
 * @return Packed color independent of host byte order.
 */
uint32_t Renderer_color_to_rgba32(RendererColor color);

/**
 * Destroy a renderer, discard queued commands, and release its resources.
 *
 * @param renderer Renderer to destroy, or NULL for no operation.
 */
void Renderer_destroy(Renderer *renderer);

/**
 * Begin a frame and reset transform, blending, and clipping state.
 *
 * @param renderer Renderer instance.
 * @param width Framebuffer width in pixels; must be positive.
 * @param height Framebuffer height in pixels; must be positive.
 * @param clear_color Color used to clear the frame.
 * @return Operation status.
 */
RendererStatus Renderer_begin_frame(Renderer *renderer, int width, int height,
                                    RendererColor clear_color);

/**
 * Deliver all queued commands to the backend in submission order.
 *
 * Successfully delivered commands are not submitted again when a later
 * command or backend flush fails. A failed flush must be retried before new
 * commands are submitted.
 *
 * @param renderer Renderer with an active frame.
 * @return Operation status.
 */
RendererStatus Renderer_flush(Renderer *renderer);

/**
 * Deliver pending commands and end the active frame.
 *
 * @param renderer Renderer with an active frame.
 * @return Operation status. A failure leaves the frame active for retry.
 */
RendererStatus Renderer_end_frame(Renderer *renderer);

/**
 * Set the local-to-framebuffer transform captured by later commands.
 *
 * @param renderer Renderer with an active frame.
 * @param transform Column-major transform.
 * @return Operation status.
 */
RendererStatus Renderer_set_transform_2d(Renderer *renderer,
                                         RendererTransform2D transform);

/**
 * Set the blend mode captured by later commands.
 *
 * @param renderer Renderer with an active frame.
 * @param blend Supported semantic blend mode.
 * @return Operation status.
 */
RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend);

/**
 * Set or disable a framebuffer-space scissor rectangle.
 *
 * The rectangle is clamped to the current framebuffer and is not affected by
 * the current transform. A fully clipped rectangle suppresses later draws.
 *
 * @param renderer Renderer with an active frame.
 * @param scissor Rectangle to enable, or NULL to disable clipping.
 * @return Operation status.
 */
RendererStatus Renderer_set_scissor(Renderer *renderer,
                                    const RendererRect *scissor);

/**
 * Queue a solid axis-aligned rectangle as two triangles.
 *
 * @param renderer Renderer with an active frame.
 * @param x Left coordinate.
 * @param y Top coordinate.
 * @param width Positive width.
 * @param height Positive height.
 * @param color Vertex color.
 * @return Operation status.
 */
RendererStatus Renderer_fill_rect(Renderer *renderer, float x, float y,
                                  float width, float height,
                                  RendererColor color);

/**
 * Queue an explicit triangle list with an optional texture.
 *
 * The vertex data is copied before this function returns. When a texture is
 * supplied, sampled RGBA values are multiplied by each vertex color.
 *
 * @param renderer Renderer with an active frame.
 * @param texture Texture to sample, or NULL for untextured triangles. The
 *        texture must belong to @p renderer and remains borrowed until the
 *        command is flushed.
 * @param vertices Vertex array.
 * @param vertex_count Positive number of vertices divisible by three.
 * @return Operation status.
 */
RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count);

/**
 * Queue a screen-width path expanded into independent segment quads.
 *
 * Zero-length segments are ignored. Width is measured in screen pixels.
 *
 * @param renderer Renderer with an active frame.
 * @param points Path positions.
 * @param point_count Number of positions; must be at least two.
 * @param width Positive stroke width in pixels.
 * @param color Vertex color.
 * @param closed Nonzero to append the final-to-first segment.
 * @return Operation status.
 */
RendererStatus Renderer_stroke_path(Renderer *renderer,
                                    const RendererPoint2D *points,
                                    size_t point_count, float width,
                                    RendererColor color, int closed);

/**
 * Queue a square point centered at the supplied position.
 *
 * @param renderer Renderer with an active frame.
 * @param x Center horizontal coordinate.
 * @param y Center vertical coordinate.
 * @param size Positive square size in pixels.
 * @param color Vertex color.
 * @return Operation status.
 */
RendererStatus Renderer_draw_point(Renderer *renderer, float x, float y,
                                   float size, RendererColor color);

/**
 * Queue a textured rectangle as two triangles.
 *
 * Texture rows and UV coordinates use a top-left origin.
 *
 * @param renderer Renderer with an active frame.
 * @param texture Texture owned by renderer.
 * @param left Left destination coordinate.
 * @param top Top destination coordinate.
 * @param right Right destination coordinate.
 * @param bottom Bottom destination coordinate.
 * @param u0 Left texture coordinate.
 * @param v0 Top texture coordinate.
 * @param u1 Right texture coordinate.
 * @param v1 Bottom texture coordinate.
 * @param tint Unpremultiplied vertex tint.
 * @return Operation status.
 */
RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint);

/**
 * Queue an owned triangle mesh with an optional texture.
 *
 * When a texture is supplied, sampled RGBA values are multiplied by the mesh
 * vertex colors.
 *
 * @param renderer Renderer with an active frame.
 * @param texture Texture owned by renderer, or NULL for untextured drawing.
 * @param mesh Mesh owned by renderer.
 * @return Operation status.
 */
RendererStatus Renderer_draw_mesh(Renderer *renderer,
                                  RendererTexture *texture,
                                  RendererMesh *mesh);

/**
 * Create an RGBA8 texture from top-left-origin unpremultiplied pixels.
 *
 * Resource changes are only permitted outside a frame.
 *
 * @param renderer Renderer that will own the texture.
 * @param desc Texture dimensions and immutable sampling behavior.
 * @param rgba_pixels Source pixels.
 * @param pitch Source row length in bytes, at least desc width times four.
 * @param texture Receives the created texture on success.
 * @return Operation status.
 */
RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture);

/**
 * Update a rectangular portion of an RGBA8 texture.
 *
 * @param renderer Texture owner, outside an active frame.
 * @param texture Texture to update.
 * @param region In-bounds destination region.
 * @param rgba_pixels Source pixels.
 * @param pitch Source row length in bytes, at least region width times four.
 * @return Operation status.
 */
RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *rgba_pixels,
                                       size_t pitch);

/**
 * Destroy a texture outside an active frame.
 *
 * @param renderer Texture owner.
 * @param texture Texture to destroy.
 * @return Operation status.
 */
RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture);

/**
 * Create an owned triangle mesh.
 *
 * @param renderer Renderer that will own the mesh, outside an active frame.
 * @param vertices Triangle vertices copied by the backend.
 * @param vertex_count Positive number of vertices divisible by three.
 * @param mesh Receives the created mesh on success.
 * @return Operation status.
 */
RendererStatus Renderer_mesh_create(Renderer *renderer,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    RendererMesh **mesh);

/**
 * Replace an owned mesh's triangle vertices outside an active frame.
 *
 * @param renderer Mesh owner.
 * @param mesh Mesh to update.
 * @param vertices Replacement triangle vertices.
 * @param vertex_count Positive number of vertices divisible by three.
 * @return Operation status.
 */
RendererStatus Renderer_mesh_update(Renderer *renderer, RendererMesh *mesh,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count);

/**
 * Destroy a mesh outside an active frame.
 *
 * @param renderer Mesh owner.
 * @param mesh Mesh to destroy.
 * @return Operation status.
 */
RendererStatus Renderer_mesh_destroy(Renderer *renderer, RendererMesh *mesh);

#endif /* RENDERER_H */
