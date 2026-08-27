/* Backend-neutral ownership and packing for a byte-font glyph atlas. */

#ifndef TEXT_ATLAS_H
#define TEXT_ATLAS_H

#include "text_geometry.h"

#include <SDL3/SDL.h>

/** Opaque atlas owning one renderer texture and immutable glyph metrics. */
typedef struct TextAtlas TextAtlas;

/** Rasterized glyph returned by a TextAtlasSource callback. */
typedef struct TextAtlasGlyphSurface {
    /** Owned surface, or NULL for an advance-only glyph. */
    SDL_Surface *surface;
    /** Horizontal offset from the pen to the bitmap's left edge. */
    float bearing_x;
    /** Distance from the baseline up to the bitmap's top edge. */
    float bearing_top;
    /** Horizontal pen advance. */
    float advance;
    /** Nonzero when the source provides this byte. */
    int available;
} TextAtlasGlyphSurface;

/** Raster source used to construct one immutable ASCII glyph atlas. */
typedef struct TextAtlasSource {
    /** Borrowed context passed to both callbacks. */
    void *context;
    /** Baseline offset below the logical line top. */
    float ascent;
    /** Logical height of one text line. */
    float line_height;
    /** Distance between consecutive baselines. */
    float line_spacing;
    /**
     * Load one printable ASCII glyph.
     *
     * @param context Borrowed source context.
     * @param byte Printable ASCII byte to rasterize.
     * @param glyph Zeroed destination receiving metrics and optional surface.
     * @return Operation status.
     *
     * The callback transfers ownership of a non-NULL surface to the atlas
     * builder, which later passes it to @c release_glyph exactly once.
     */
    RendererStatus (*load_glyph)(void *context, unsigned char byte,
                                 TextAtlasGlyphSurface *glyph);
    /**
     * Release a surface returned by @c load_glyph.
     *
     * @param context Borrowed source context.
     * @param glyph Glyph whose transferred non-NULL surface must be released.
     */
    void (*release_glyph)(void *context, TextAtlasGlyphSurface *glyph);
} TextAtlasSource;

/**
 * Prewarm printable ASCII into one top-left-origin RGBA8 texture.
 *
 * @param renderer Renderer that owns the resulting texture.
 * @param source Borrowed raster source used only during this call.
 * @param atlas Empty destination that receives the owned atlas on success.
 * @return Operation status.
 *
 * @remarks Bytes 0x20 through 0x7e are requested exactly once. Missing bytes
 * remain unavailable and are resolved to '?' by Text_geometry_*(). The '?'
 * glyph itself must be available. Creation is atomic and must occur outside
 * an active renderer frame. The atlas borrows @p renderer, which must remain
 * alive until Text_atlas_destroy() succeeds.
 */
RendererStatus Text_atlas_create(Renderer *renderer,
                                 const TextAtlasSource *source,
                                 TextAtlas **atlas);

/**
 * Destroy an atlas and its renderer texture.
 *
 * @param atlas Address of an atlas pointer; an already-NULL value is valid.
 * @return Operation status.
 *
 * @remarks If texture destruction is rejected, the pointer and all owned
 * state remain intact so the operation can be retried after the frame ends.
 */
RendererStatus Text_atlas_destroy(TextAtlas **atlas);

/**
 * Borrow immutable geometry metrics from an atlas.
 *
 * @param atlas Atlas instance.
 * @return Borrowed metrics, or NULL for a NULL atlas.
 */
const TextGeometryFont *Text_atlas_geometry_font(const TextAtlas *atlas);

/**
 * Borrow the renderer texture owned by an atlas.
 *
 * @param atlas Atlas instance.
 * @return Borrowed texture, or NULL for a NULL atlas.
 */
RendererTexture *Text_atlas_texture(const TextAtlas *atlas);

/**
 * Borrow the renderer that owns an atlas texture.
 *
 * @param atlas Atlas instance.
 * @return Borrowed owning renderer, or NULL for a NULL atlas.
 */
Renderer *Text_atlas_renderer(const TextAtlas *atlas);

#endif /* TEXT_ATLAS_H */
