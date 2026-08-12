/* Backend-neutral glyph atlas geometry. */

#include "text_geometry.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

static int Text_geometry_glyph_valid(const TextGeometryFont *font,
                                     const TextGeometryGlyph *glyph)
{
    int64_t right;
    int64_t bottom;

    if (glyph == NULL || !glyph->available
        || !isfinite(glyph->bearing_x)
        || !isfinite(glyph->bearing_top)
        || !isfinite(glyph->advance) || glyph->advance < 0.0f
        || glyph->atlas_bounds.x < 0 || glyph->atlas_bounds.y < 0
        || glyph->atlas_bounds.width < 0 || glyph->atlas_bounds.height < 0
        || ((glyph->atlas_bounds.width == 0)
            != (glyph->atlas_bounds.height == 0))) {
        return 0;
    }
    right = (int64_t)glyph->atlas_bounds.x + glyph->atlas_bounds.width;
    bottom = (int64_t)glyph->atlas_bounds.y + glyph->atlas_bounds.height;
    return right <= font->atlas_width && bottom <= font->atlas_height;
}

static int Text_geometry_font_valid(const TextGeometryFont *font)
{
    const TextGeometryGlyph *fallback;

    if (font == NULL || font->atlas_width <= 0 || font->atlas_height <= 0
        || !isfinite(font->ascent) || font->ascent < 0.0f
        || !isfinite(font->line_height) || font->line_height <= 0.0f
        || !isfinite(font->line_spacing) || font->line_spacing <= 0.0f) {
        return 0;
    }
    fallback = &font->glyphs[font->fallback_byte];
    return Text_geometry_glyph_valid(font, fallback);
}

static const TextGeometryGlyph *Text_geometry_resolve_glyph(
    const TextGeometryFont *font, unsigned char byte)
{
    const TextGeometryGlyph *glyph = &font->glyphs[byte];

    if (!glyph->available)
        glyph = &font->glyphs[font->fallback_byte];
    return glyph;
}

static int Text_geometry_line_width(const TextGeometryFont *font,
                                    const unsigned char *text,
                                    size_t start, size_t end,
                                    float *width, size_t *visible_glyphs)
{
    float candidate_width = 0.0f;
    size_t candidate_glyphs = 0;
    size_t index;

    for (index = start; index < end; index++) {
        const TextGeometryGlyph *glyph = Text_geometry_resolve_glyph(
            font, text[index]);

        if (!Text_geometry_glyph_valid(font, glyph)
            || !isfinite(candidate_width + glyph->advance)) {
            return 0;
        }
        candidate_width += glyph->advance;
        if (glyph->atlas_bounds.width > 0) {
            if (candidate_glyphs == SIZE_MAX)
                return 0;
            candidate_glyphs++;
        }
    }
    *width = candidate_width;
    *visible_glyphs = candidate_glyphs;
    return 1;
}

RendererStatus Text_geometry_measure(const TextGeometryFont *font,
                                     const unsigned char *text,
                                     size_t text_length,
                                     TextGeometryMetrics *metrics)
{
    TextGeometryMetrics candidate = {0.0f, 0.0f, 0, 0, 0};
    size_t start;

    if (!Text_geometry_font_valid(font) || text == NULL || metrics == NULL
        || text_length > SIZE_MAX / 6) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (text_length == 0) {
        *metrics = candidate;
        return RENDERER_STATUS_OK;
    }

    start = 0;
    for (;;) {
        float line_width;
        size_t line_glyphs;
        size_t end = start;

        while (end < text_length && text[end] != (unsigned char)'\n')
            end++;
        if (!Text_geometry_line_width(font, text, start, end,
                                      &line_width, &line_glyphs)
            || candidate.glyph_count > SIZE_MAX - line_glyphs) {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        if (line_width > candidate.width)
            candidate.width = line_width;
        candidate.glyph_count += line_glyphs;
        candidate.line_count++;
        if (end >= text_length - 1)
            break;
        start = end + 1;
    }

    if (candidate.glyph_count > SIZE_MAX / 6)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    candidate.vertex_count = candidate.glyph_count * 6;
    candidate.height = font->line_height
        + (float)(candidate.line_count - 1) * font->line_spacing;
    if (!isfinite(candidate.width) || !isfinite(candidate.height))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *metrics = candidate;
    return RENDERER_STATUS_OK;
}

static float Text_geometry_horizontal_offset(
    TextGeometryHorizontalAlign alignment, float width)
{
    if (alignment == TEXT_GEOMETRY_ALIGN_CENTER)
        return -width * 0.5f;
    if (alignment == TEXT_GEOMETRY_ALIGN_RIGHT)
        return -width;
    return 0.0f;
}

static float Text_geometry_vertical_offset(TextGeometryVerticalAlign alignment,
                                           float height)
{
    if (alignment == TEXT_GEOMETRY_ALIGN_MIDDLE)
        return -height * 0.5f;
    if (alignment == TEXT_GEOMETRY_ALIGN_BOTTOM)
        return -height;
    return 0.0f;
}

static int Text_geometry_validate_positions(
    const TextGeometryFont *font, const unsigned char *text,
    size_t text_length, RendererPoint2D anchor,
    TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, const TextGeometryMetrics *metrics)
{
    float block_top = anchor.y
        + Text_geometry_vertical_offset(vertical, metrics->height);
    size_t line_index = 0;
    size_t start = 0;

    for (;;) {
        float line_width;
        float pen;
        float baseline;
        size_t ignored_glyphs;
        size_t index;
        size_t end = start;

        while (end < text_length && text[end] != (unsigned char)'\n')
            end++;
        if (!Text_geometry_line_width(font, text, start, end,
                                      &line_width, &ignored_glyphs)) {
            return 0;
        }
        pen = anchor.x + Text_geometry_horizontal_offset(horizontal,
                                                         line_width);
        baseline = block_top + font->ascent
            + (float)line_index * font->line_spacing;
        if (!isfinite(pen) || !isfinite(baseline))
            return 0;
        for (index = start; index < end; index++) {
            const TextGeometryGlyph *glyph = Text_geometry_resolve_glyph(
                font, text[index]);
            float left = pen + glyph->bearing_x;
            float top = baseline - glyph->bearing_top;
            float right = left + (float)glyph->atlas_bounds.width;
            float bottom = top + (float)glyph->atlas_bounds.height;

            if (!isfinite(left) || !isfinite(top)
                || !isfinite(right) || !isfinite(bottom)
                || !isfinite(pen + glyph->advance)) {
                return 0;
            }
            pen += glyph->advance;
        }
        line_index++;
        if (end >= text_length - 1)
            break;
        start = end + 1;
    }
    return 1;
}

static void Text_geometry_write_quad(RendererVertex2D *vertices,
                                     const TextGeometryFont *font,
                                     const TextGeometryGlyph *glyph,
                                     float left, float top,
                                     RendererColor color)
{
    static const size_t corner_indices[6] = {0, 1, 2, 0, 2, 3};
    RendererVertex2D corners[4];
    float right = left + (float)glyph->atlas_bounds.width;
    float bottom = top + (float)glyph->atlas_bounds.height;
    float u_left = (float)glyph->atlas_bounds.x / font->atlas_width;
    float u_right = (float)(glyph->atlas_bounds.x
                            + glyph->atlas_bounds.width) / font->atlas_width;
    float v_top = (float)glyph->atlas_bounds.y / font->atlas_height;
    float v_bottom = (float)(glyph->atlas_bounds.y
                             + glyph->atlas_bounds.height)
        / font->atlas_height;
    size_t index;

    corners[0] = (RendererVertex2D){left, top, u_left, v_top, color};
    corners[1] = (RendererVertex2D){right, top, u_right, v_top, color};
    corners[2] = (RendererVertex2D){right, bottom, u_right, v_bottom, color};
    corners[3] = (RendererVertex2D){left, bottom, u_left, v_bottom, color};
    for (index = 0; index < 6; index++)
        vertices[index] = corners[corner_indices[index]];
}

RendererStatus Text_geometry_build(const TextGeometryFont *font,
                                   const unsigned char *text,
                                   size_t text_length,
                                   RendererPoint2D anchor,
                                   TextGeometryHorizontalAlign horizontal,
                                   TextGeometryVerticalAlign vertical,
                                   RendererColor color,
                                   RendererVertex2D *vertices,
                                   size_t vertex_capacity)
{
    TextGeometryMetrics metrics;
    RendererStatus status;
    float block_top;
    size_t output_index = 0;
    size_t line_index = 0;
    size_t start;

    if (!isfinite(anchor.x) || !isfinite(anchor.y)
        || (horizontal != TEXT_GEOMETRY_ALIGN_LEFT
            && horizontal != TEXT_GEOMETRY_ALIGN_CENTER
            && horizontal != TEXT_GEOMETRY_ALIGN_RIGHT)
        || (vertical != TEXT_GEOMETRY_ALIGN_TOP
            && vertical != TEXT_GEOMETRY_ALIGN_MIDDLE
            && vertical != TEXT_GEOMETRY_ALIGN_BOTTOM)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    status = Text_geometry_measure(font, text, text_length, &metrics);
    if (status != RENDERER_STATUS_OK)
        return status;
    if (metrics.vertex_count > vertex_capacity
        || (metrics.vertex_count > 0 && vertices == NULL)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (text_length == 0)
        return RENDERER_STATUS_OK;
    if (!Text_geometry_validate_positions(font, text, text_length, anchor,
                                          horizontal, vertical, &metrics)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    block_top = anchor.y + Text_geometry_vertical_offset(vertical,
                                                          metrics.height);
    start = 0;
    for (;;) {
        float line_width;
        float pen;
        float baseline;
        size_t ignored_glyphs;
        size_t index;
        size_t end = start;

        while (end < text_length && text[end] != (unsigned char)'\n')
            end++;
        (void)Text_geometry_line_width(font, text, start, end,
                                       &line_width, &ignored_glyphs);
        pen = anchor.x + Text_geometry_horizontal_offset(horizontal,
                                                         line_width);
        baseline = block_top + font->ascent
            + (float)line_index * font->line_spacing;
        for (index = start; index < end; index++) {
            const TextGeometryGlyph *glyph = Text_geometry_resolve_glyph(
                font, text[index]);

            if (glyph->atlas_bounds.width > 0) {
                Text_geometry_write_quad(
                    &vertices[output_index], font, glyph,
                    pen + glyph->bearing_x,
                    baseline - glyph->bearing_top, color);
                output_index += 6;
            }
            pen += glyph->advance;
        }
        line_index++;
        if (end >= text_length - 1)
            break;
        start = end + 1;
    }
    return RENDERER_STATUS_OK;
}
