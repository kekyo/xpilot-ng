/* Backend-neutral glyph atlas geometry. */

#ifndef TEXT_GEOMETRY_H
#define TEXT_GEOMETRY_H

#include "renderer.h"

#include <stddef.h>

/** Number of byte values represented by one XPilot font. */
#define TEXT_GEOMETRY_GLYPH_COUNT 256

/** Horizontal placement of each logical text line around its anchor. */
typedef enum TextGeometryHorizontalAlign {
    /** Place the line's advance origin at the anchor. */
    TEXT_GEOMETRY_ALIGN_LEFT,
    /** Center the line's advance width on the anchor. */
    TEXT_GEOMETRY_ALIGN_CENTER,
    /** Place the line's advance end at the anchor. */
    TEXT_GEOMETRY_ALIGN_RIGHT
} TextGeometryHorizontalAlign;

/** Vertical placement of the complete logical text block. */
typedef enum TextGeometryVerticalAlign {
    /** Place the block's top edge at the anchor. */
    TEXT_GEOMETRY_ALIGN_TOP,
    /** Center the block's logical height on the anchor. */
    TEXT_GEOMETRY_ALIGN_MIDDLE,
    /** Place the block's bottom edge at the anchor. */
    TEXT_GEOMETRY_ALIGN_BOTTOM
} TextGeometryVerticalAlign;

/** Metrics and atlas bounds for one byte value. */
typedef struct TextGeometryGlyph {
    /** Top-left-origin bounds in the font atlas. */
    RendererRect atlas_bounds;
    /** Horizontal offset from the pen to the bitmap's left edge. */
    float bearing_x;
    /** Distance from the baseline up to the bitmap's top edge. */
    float bearing_top;
    /** Horizontal pen advance after this glyph. */
    float advance;
    /** Nonzero when this byte has valid metrics and atlas bounds. */
    int available;
} TextGeometryGlyph;

/** Immutable metrics for one 256-byte glyph atlas. */
typedef struct TextGeometryFont {
    /** Atlas width in pixels. */
    int atlas_width;
    /** Atlas height in pixels. */
    int atlas_height;
    /** Baseline offset below the logical line top. */
    float ascent;
    /** Logical height of a single line. */
    float line_height;
    /** Distance between consecutive baselines. */
    float line_spacing;
    /** Byte used when an input byte is unavailable. */
    unsigned char fallback_byte;
    /** Metrics indexed by unsigned byte value. */
    TextGeometryGlyph glyphs[TEXT_GEOMETRY_GLYPH_COUNT];
} TextGeometryFont;

/** Measured logical bounds and output geometry size for a byte string. */
typedef struct TextGeometryMetrics {
    /** Maximum advance width among all lines. */
    float width;
    /** Logical height of the complete block. */
    float height;
    /** Number of logical lines, or zero for empty input. */
    size_t line_count;
    /** Number of glyphs with nonempty bitmap bounds. */
    size_t glyph_count;
    /** Number of triangle vertices required by Text_geometry_build(). */
    size_t vertex_count;
} TextGeometryMetrics;

/**
 * Measure a byte string without allocating or creating GPU resources.
 *
 * @param font Valid byte-font metrics and atlas bounds.
 * @param text Borrowed byte string; line feed separates logical lines.
 * @param text_length Number of bytes to inspect.
 * @param metrics Receives measurements on success.
 * @return Operation status.
 *
 * @remarks An unavailable byte uses @c fallback_byte. A terminal line feed
 * does not add an empty line, matching the legacy XPilot renderer. The output
 * remains unchanged when validation fails.
 */
RendererStatus Text_geometry_measure(const TextGeometryFont *font,
                                     const unsigned char *text,
                                     size_t text_length,
                                     TextGeometryMetrics *metrics);

/**
 * Build two top-left-origin textured triangles for each visible glyph.
 *
 * @param font Valid byte-font metrics and atlas bounds.
 * @param text Borrowed byte string; line feed separates logical lines.
 * @param text_length Number of bytes to inspect.
 * @param anchor Alignment anchor in logical top-left-origin coordinates.
 * @param horizontal Per-line horizontal alignment.
 * @param vertical Whole-block vertical alignment.
 * @param color Unpremultiplied color copied to every vertex.
 * @param vertices Destination array, or NULL when no vertices are required.
 * @param vertex_capacity Number of elements available in @p vertices.
 * @return Operation status.
 *
 * @remarks Texture coordinates use a top-left origin. The destination remains
 * unchanged when validation fails.
 */
RendererStatus Text_geometry_build(const TextGeometryFont *font,
                                   const unsigned char *text,
                                   size_t text_length,
                                   RendererPoint2D anchor,
                                   TextGeometryHorizontalAlign horizontal,
                                   TextGeometryVerticalAlign vertical,
                                   RendererColor color,
                                   RendererVertex2D *vertices,
                                   size_t vertex_capacity);

#endif /* TEXT_GEOMETRY_H */
