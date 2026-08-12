/* Semantic glyph-atlas text drawing for the SDL renderer. */

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "sdlrenderer.h"
#include "text_atlas.h"

#include <stddef.h>

/** Opaque text drawing facade borrowing one SDL renderer and glyph atlas. */
typedef struct TextRenderer TextRenderer;

/** Opaque CPU-side byte-string cache bound to one renderer font. */
typedef struct TextRendererCache TextRendererCache;

/** Coordinate convention used for generated text vertices. */
typedef enum TextRendererSpace {
    /** Top-left-origin screen coordinates whose Y axis grows downward. */
    TEXT_RENDERER_SPACE_HUD,
    /** Bottom-left-origin world coordinates whose Y axis grows upward. */
    TEXT_RENDERER_SPACE_WORLD
} TextRendererSpace;

/**
 * Create a semantic text drawing facade.
 *
 * @param sdl_renderer Borrowed SDL renderer facade.
 * @param atlas Borrowed immutable glyph atlas.
 * @param text_renderer Empty destination receiving the new facade.
 * @return Operation status.
 *
 * @remarks The SDL renderer and atlas must outlive the returned facade and
 * every cache created from it. Creation does not allocate GPU resources.
 */
RendererStatus Text_renderer_create(SdlRenderer *sdl_renderer,
                                    TextAtlas *atlas,
                                    TextRenderer **text_renderer);

/**
 * Destroy a text drawing facade.
 *
 * @param text_renderer Address of a facade pointer; an already-NULL value is
 *        valid.
 */
void Text_renderer_destroy(TextRenderer **text_renderer);

/**
 * Measure a byte string using the facade's immutable font metrics.
 *
 * @param text_renderer Text drawing facade.
 * @param text Borrowed byte string, or NULL when @p text_length is zero.
 * @param text_length Number of bytes to measure.
 * @param metrics Receives the logical measurements on success.
 * @return Operation status.
 */
RendererStatus Text_renderer_measure(const TextRenderer *text_renderer,
                                     const unsigned char *text,
                                     size_t text_length,
                                     TextGeometryMetrics *metrics);

/**
 * Build and immediately queue one byte string.
 *
 * @param text_renderer Text drawing facade.
 * @param text Borrowed byte string, or NULL when @p text_length is zero.
 * @param text_length Number of bytes to draw.
 * @param anchor Alignment anchor in the selected coordinate space.
 * @param horizontal Per-line horizontal alignment.
 * @param vertical Whole-block vertical alignment.
 * @param color Unpremultiplied glyph tint.
 * @param space Coordinate convention for the generated vertices.
 * @return Operation status.
 *
 * @remarks A successful nonempty draw is flushed through the transitional
 * legacy-state-preserving ordering barrier before this function returns.
 */
RendererStatus Text_renderer_draw(
    TextRenderer *text_renderer, const unsigned char *text,
    size_t text_length, RendererPoint2D anchor,
    TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space);

/**
 * Atomically replace a CPU-side byte-string cache.
 *
 * @param cache Address of a cache pointer. An existing cache must have been
 *        created for the same renderer font.
 * @param text_renderer Facade supplying the immutable font metrics.
 * @param text Borrowed byte string, or NULL when @p text_length is zero.
 * @param text_length Number of bytes to copy and measure.
 * @return Operation status.
 *
 * @remarks Replacing a cache with identical bytes preserves its pointer.
 * Failure leaves an existing cache and all its measurements unchanged. The
 * cache owns no GPU resources.
 */
RendererStatus Text_renderer_cache_replace(TextRendererCache **cache,
                                           const TextRenderer *text_renderer,
                                           const unsigned char *text,
                                           size_t text_length);

/**
 * Destroy a CPU-side byte-string cache.
 *
 * @param cache Address of a cache pointer; an already-NULL value is valid.
 */
void Text_renderer_cache_destroy(TextRendererCache **cache);

/**
 * Copy the immutable measurements stored in a cache.
 *
 * @param cache Byte-string cache.
 * @param metrics Receives cached logical measurements on success.
 * @return Operation status.
 */
RendererStatus Text_renderer_cache_metrics(
    const TextRendererCache *cache, TextGeometryMetrics *metrics);

/**
 * Queue a byte string retained in a CPU-side cache.
 *
 * @param text_renderer Text drawing facade using the cache's font.
 * @param cache Cache to draw.
 * @param anchor Alignment anchor in the selected coordinate space.
 * @param horizontal Per-line horizontal alignment.
 * @param vertical Whole-block vertical alignment.
 * @param color Unpremultiplied glyph tint.
 * @param space Coordinate convention for the generated vertices.
 * @return Operation status.
 */
RendererStatus Text_renderer_draw_cached(
    TextRenderer *text_renderer, const TextRendererCache *cache,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space);

#endif /* TEXT_RENDERER_H */
