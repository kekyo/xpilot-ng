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
 * @remarks The SDL renderer and atlas must remain valid while the returned
 * facade is used. Destroying the facade does not dereference either borrowed
 * object, so an owner may first release a fallible atlas resource and then
 * destroy the facade after that release succeeds. CPU caches do not borrow
 * the facade itself. Creation does not allocate GPU resources.
 */
RendererStatus Text_renderer_create(SdlRenderer *sdl_renderer,
                                    TextAtlas *atlas,
                                    TextRenderer **text_renderer);

/**
 * Destroy a text drawing facade.
 *
 * @param text_renderer Address of a facade pointer; an already-NULL value is
 *        valid.
 *
 * @remarks Destruction only releases CPU facade storage and does not inspect
 * its borrowed SDL renderer or atlas.
 */
void Text_renderer_destroy(TextRenderer **text_renderer);

/**
 * Retain a failure produced by a text adapter before geometry submission.
 *
 * @param text_renderer Text drawing facade for the active frame.
 * @param status Failure to retain.
 * @return The first failure retained for the frame.
 *
 * @remarks This is intended for formatting and legacy-adapter validation
 * that occurs before Text_renderer_measure() or Text_renderer_draw(). The
 * status must be a non-OK failure and an SDL renderer frame must be active.
 */
RendererStatus Text_renderer_track_failure(
    TextRenderer *text_renderer, RendererStatus status);

/**
 * Measure a byte string using the facade's immutable font metrics.
 *
 * @param text_renderer Text drawing facade.
 * @param text Borrowed byte string, or NULL when @p text_length is zero.
 * @param text_length Number of bytes to measure.
 * @param metrics Receives the logical measurements on success.
 * @return Operation status.
 *
 * @remarks This operation requires an active SDL renderer frame. It returns
 * the first retained semantic failure without changing @p metrics. A new
 * measurement failure becomes the retained result so presentation code can
 * reject the incomplete frame.
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
 * @remarks This operation requires an active SDL renderer frame. A
 * successful nonempty draw is flushed through the transitional
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
 *
 * @remarks This operation requires an active SDL renderer frame. A
 * successful nonempty draw is flushed through the transitional
 * legacy-state-preserving ordering barrier before this function returns.
 */
RendererStatus Text_renderer_draw_cached(
    TextRenderer *text_renderer, const TextRendererCache *cache,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space);

#endif /* TEXT_RENDERER_H */
