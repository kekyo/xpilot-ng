#include "test_helpers.h"

#include "text_geometry.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FLOAT_TOLERANCE 0.0001f

static int float_equal(float actual, float expected)
{
    return fabsf(actual - expected) <= FLOAT_TOLERANCE;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red
        && actual.green == expected.green
        && actual.blue == expected.blue
        && actual.alpha == expected.alpha;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, float u, float v,
                        RendererColor color)
{
    return float_equal(vertex->x, x)
        && float_equal(vertex->y, y)
        && float_equal(vertex->u, u)
        && float_equal(vertex->v, v)
        && color_equal(vertex->color, color);
}

static int quad_equal(const RendererVertex2D vertices[6],
                      float left, float top, float right, float bottom,
                      float u_left, float v_top,
                      float u_right, float v_bottom,
                      RendererColor color)
{
    return vertex_equal(&vertices[0], left, top, u_left, v_top, color)
        && vertex_equal(&vertices[1], right, top, u_right, v_top, color)
        && vertex_equal(&vertices[2], right, bottom,
                        u_right, v_bottom, color)
        && vertex_equal(&vertices[3], left, top, u_left, v_top, color)
        && vertex_equal(&vertices[4], right, bottom,
                        u_right, v_bottom, color)
        && vertex_equal(&vertices[5], left, bottom,
                        u_left, v_bottom, color);
}

static TextGeometryFont make_font(void)
{
    TextGeometryFont font;

    memset(&font, 0, sizeof(font));
    font.atlas_width = 64;
    font.atlas_height = 32;
    font.ascent = 7.0f;
    font.line_height = 10.0f;
    font.line_spacing = 12.0f;
    font.fallback_byte = (unsigned char)'?';

    font.glyphs[(unsigned char)'A'].atlas_bounds
        = (RendererRect){0, 0, 5, 8};
    font.glyphs[(unsigned char)'A'].bearing_x = 1.0f;
    font.glyphs[(unsigned char)'A'].bearing_top = 7.0f;
    font.glyphs[(unsigned char)'A'].advance = 7.0f;
    font.glyphs[(unsigned char)'A'].available = 1;

    font.glyphs[(unsigned char)'B'].atlas_bounds
        = (RendererRect){5, 0, 4, 6};
    font.glyphs[(unsigned char)'B'].bearing_x = -1.0f;
    font.glyphs[(unsigned char)'B'].bearing_top = 5.0f;
    font.glyphs[(unsigned char)'B'].advance = 6.0f;
    font.glyphs[(unsigned char)'B'].available = 1;

    font.glyphs[(unsigned char)'?'].atlas_bounds
        = (RendererRect){9, 0, 3, 7};
    font.glyphs[(unsigned char)'?'].bearing_x = 0.0f;
    font.glyphs[(unsigned char)'?'].bearing_top = 6.0f;
    font.glyphs[(unsigned char)'?'].advance = 4.0f;
    font.glyphs[(unsigned char)'?'].available = 1;

    font.glyphs[(unsigned char)'\r'].atlas_bounds
        = (RendererRect){12, 0, 2, 3};
    font.glyphs[(unsigned char)'\r'].bearing_x = 0.0f;
    font.glyphs[(unsigned char)'\r'].bearing_top = 3.0f;
    font.glyphs[(unsigned char)'\r'].advance = 5.0f;
    font.glyphs[(unsigned char)'\r'].available = 1;

    /* A whitespace glyph advances the pen without consuming atlas pixels. */
    font.glyphs[(unsigned char)' '].atlas_bounds
        = (RendererRect){64, 32, 0, 0};
    font.glyphs[(unsigned char)' '].advance = 3.0f;
    font.glyphs[(unsigned char)' '].available = 1;

    return font;
}

static int metrics_equal(const TextGeometryMetrics *metrics,
                         float width, float height,
                         size_t line_count, size_t glyph_count,
                         size_t vertex_count)
{
    return float_equal(metrics->width, width)
        && float_equal(metrics->height, height)
        && metrics->line_count == line_count
        && metrics->glyph_count == glyph_count
        && metrics->vertex_count == vertex_count;
}

static int check_metrics_and_top_left_glyph_quads(void)
{
    const TextGeometryFont font = make_font();
    const unsigned char text[] = {'A', 'B'};
    const RendererPoint2D anchor = {10.0f, 20.0f};
    const RendererColor tint = {17, 33, 65, 129};
    TextGeometryMetrics metrics;
    RendererVertex2D vertices[12];

    TEST_CHECK(Text_geometry_measure(&font, text, sizeof(text), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 13.0f, 10.0f, 1, 2, 12));

    TEST_CHECK(Text_geometry_build(&font, text, sizeof(text), anchor,
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   tint, vertices, 12)
               == RENDERER_STATUS_OK);
    TEST_CHECK(quad_equal(&vertices[0],
                          11.0f, 20.0f, 16.0f, 28.0f,
                          0.0f, 0.0f, 5.0f / 64.0f, 8.0f / 32.0f,
                          tint));
    TEST_CHECK(quad_equal(&vertices[6],
                          16.0f, 22.0f, 20.0f, 28.0f,
                          5.0f / 64.0f, 0.0f,
                          9.0f / 64.0f, 6.0f / 32.0f,
                          tint));
    return 0;
}

static int check_unsupported_bytes_use_fallback(void)
{
    TextGeometryFont font = make_font();
    const unsigned char unsupported[] = {0x80};
    const unsigned char carriage_return[] = {'A', '\r'};
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryMetrics metrics;
    RendererVertex2D vertices[12];

    TEST_CHECK(Text_geometry_measure(&font, unsupported,
                                     sizeof(unsupported), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 4.0f, 10.0f, 1, 1, 6));
    TEST_CHECK(Text_geometry_build(&font, unsupported, sizeof(unsupported),
                                   (RendererPoint2D){0.0f, 0.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, vertices, 6)
               == RENDERER_STATUS_OK);
    TEST_CHECK(quad_equal(vertices, 0.0f, 1.0f, 3.0f, 8.0f,
                          9.0f / 64.0f, 0.0f,
                          12.0f / 64.0f, 7.0f / 32.0f, white));

    /* Only line feed is structural; carriage return resolves as a byte. */
    TEST_CHECK(Text_geometry_measure(&font, carriage_return,
                                     sizeof(carriage_return), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 12.0f, 10.0f, 1, 2, 12));

    font.glyphs[(unsigned char)'?'].atlas_bounds
        = (RendererRect){64, 32, 0, 0};
    TEST_CHECK(Text_geometry_measure(&font, unsupported,
                                     sizeof(unsupported), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 4.0f, 10.0f, 1, 0, 0));
    TEST_CHECK(Text_geometry_build(&font, unsupported, sizeof(unsupported),
                                   (RendererPoint2D){0.0f, 0.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, NULL, 0)
               == RENDERER_STATUS_OK);
    return 0;
}

static int check_alignment_uses_advance_bounds(void)
{
    const TextGeometryFont font = make_font();
    const unsigned char text[] = {'A', 'B'};
    const TextGeometryHorizontalAlign horizontal[] = {
        TEXT_GEOMETRY_ALIGN_LEFT,
        TEXT_GEOMETRY_ALIGN_CENTER,
        TEXT_GEOMETRY_ALIGN_RIGHT
    };
    const TextGeometryVerticalAlign vertical[] = {
        TEXT_GEOMETRY_ALIGN_TOP,
        TEXT_GEOMETRY_ALIGN_MIDDLE,
        TEXT_GEOMETRY_ALIGN_BOTTOM
    };
    const float horizontal_factor[] = {0.0f, 0.5f, 1.0f};
    const float vertical_factor[] = {0.0f, 0.5f, 1.0f};
    const RendererPoint2D anchor = {100.0f, 200.0f};
    const RendererColor white = {255, 255, 255, 255};
    RendererVertex2D vertices[12];
    size_t x_alignment;
    size_t y_alignment;

    for (y_alignment = 0; y_alignment < 3; y_alignment++) {
        for (x_alignment = 0; x_alignment < 3; x_alignment++) {
            float expected_x = anchor.x
                - 13.0f * horizontal_factor[x_alignment] + 1.0f;
            float expected_y = anchor.y
                - 10.0f * vertical_factor[y_alignment];

            TEST_CHECK(Text_geometry_build(
                           &font, text, sizeof(text), anchor,
                           horizontal[x_alignment], vertical[y_alignment],
                           white, vertices, 12) == RENDERER_STATUS_OK);
            TEST_CHECK(float_equal(vertices[0].x, expected_x));
            TEST_CHECK(float_equal(vertices[0].y, expected_y));
        }
    }
    return 0;
}

static int check_newlines_align_each_line_and_use_line_spacing(void)
{
    const TextGeometryFont font = make_font();
    const unsigned char text[] = {'A', '\n', 'B', 'B'};
    const unsigned char trailing_newline[] = {'A', '\n'};
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryMetrics metrics;
    RendererVertex2D vertices[18];

    TEST_CHECK(Text_geometry_measure(&font, text, sizeof(text), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 12.0f, 22.0f, 2, 3, 18));
    TEST_CHECK(Text_geometry_build(&font, text, sizeof(text),
                                   (RendererPoint2D){50.0f, 100.0f},
                                   TEXT_GEOMETRY_ALIGN_CENTER,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, vertices, 18)
               == RENDERER_STATUS_OK);
    TEST_CHECK(float_equal(vertices[0].x, 47.5f));
    TEST_CHECK(float_equal(vertices[0].y, 100.0f));
    TEST_CHECK(float_equal(vertices[6].x, 43.0f));
    TEST_CHECK(float_equal(vertices[6].y, 114.0f));
    TEST_CHECK(float_equal(vertices[12].x, 49.0f));
    TEST_CHECK(float_equal(vertices[12].y, 114.0f));

    TEST_CHECK(Text_geometry_measure(&font, trailing_newline,
                                     sizeof(trailing_newline), &metrics)
               == RENDERER_STATUS_OK);
    /* Legacy text ignores the empty line introduced by a terminal LF. */
    TEST_CHECK(metrics_equal(&metrics, 7.0f, 10.0f, 1, 1, 6));
    return 0;
}

static int check_zero_bitmap_glyphs_only_advance_the_pen(void)
{
    const TextGeometryFont font = make_font();
    const unsigned char text[] = {'A', ' ', 'A'};
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryMetrics metrics;
    RendererVertex2D vertices[12];

    TEST_CHECK(Text_geometry_measure(&font, text, sizeof(text), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 17.0f, 10.0f, 1, 2, 12));
    TEST_CHECK(Text_geometry_build(&font, text, sizeof(text),
                                   (RendererPoint2D){10.0f, 20.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, vertices, 12)
               == RENDERER_STATUS_OK);
    TEST_CHECK(float_equal(vertices[0].x, 11.0f));
    TEST_CHECK(float_equal(vertices[6].x, 21.0f));
    return 0;
}

static int check_empty_text_has_no_logical_lines_or_geometry(void)
{
    const TextGeometryFont font = make_font();
    const unsigned char empty[] = "";
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryMetrics metrics;
    RendererVertex2D sentinel;
    RendererVertex2D original_sentinel;

    memset(&sentinel, 0xa5, sizeof(sentinel));
    original_sentinel = sentinel;

    TEST_CHECK(Text_geometry_measure(&font, empty, 0, &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 0.0f, 0.0f, 0, 0, 0));
    TEST_CHECK(Text_geometry_build(&font, empty, 0,
                                   (RendererPoint2D){10.0f, 20.0f},
                                   TEXT_GEOMETRY_ALIGN_CENTER,
                                   TEXT_GEOMETRY_ALIGN_MIDDLE,
                                   white, NULL, 0)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Text_geometry_build(&font, empty, 0,
                                   (RendererPoint2D){10.0f, 20.0f},
                                   TEXT_GEOMETRY_ALIGN_CENTER,
                                   TEXT_GEOMETRY_ALIGN_MIDDLE,
                                   white, &sentinel, 0)
               == RENDERER_STATUS_OK);
    TEST_CHECK(memcmp(&sentinel, &original_sentinel, sizeof(sentinel)) == 0);
    return 0;
}

static int check_invalid_inputs_are_rejected_atomically(void)
{
    const TextGeometryFont valid_font = make_font();
    const unsigned char text[] = {'A', 'B'};
    const unsigned char overflow_text[] = {'A', 'A'};
    const unsigned char *unreadable = (const unsigned char *)(uintptr_t)1;
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryFont invalid_font;
    TextGeometryMetrics metrics;
    TextGeometryMetrics original_metrics;
    RendererVertex2D vertices[12];
    RendererVertex2D original_vertices[12];

    memset(&metrics, 0xa5, sizeof(metrics));
    memcpy(&original_metrics, &metrics, sizeof(metrics));
    memset(vertices, 0x5a, sizeof(vertices));
    memcpy(original_vertices, vertices, sizeof(vertices));

    TEST_CHECK(Text_geometry_measure(NULL, text, sizeof(text), &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_measure(&valid_font, NULL, 0, &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_measure(&valid_font, text, sizeof(text), NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(&metrics, &original_metrics, sizeof(metrics)) == 0);

    /* Reject impossible vertex arithmetic before inspecting text bytes. */
    TEST_CHECK(Text_geometry_measure(&valid_font, unreadable, SIZE_MAX,
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(&metrics, &original_metrics, sizeof(metrics)) == 0);

    invalid_font = valid_font;
    invalid_font.atlas_width = 0;
    TEST_CHECK(Text_geometry_measure(&invalid_font, text, sizeof(text),
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_font = valid_font;
    invalid_font.glyphs[(unsigned char)'A'].atlas_bounds
        = (RendererRect){63, 0, 2, 8};
    TEST_CHECK(Text_geometry_measure(&invalid_font, text, sizeof(text),
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_font = valid_font;
    invalid_font.glyphs[(unsigned char)'?'].available = 0;
    TEST_CHECK(Text_geometry_measure(&invalid_font, text, sizeof(text),
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_font = valid_font;
    invalid_font.glyphs[(unsigned char)'A'].bearing_x = NAN;
    TEST_CHECK(Text_geometry_measure(&invalid_font, text, sizeof(text),
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_font = valid_font;
    invalid_font.line_spacing = 0.0f;
    TEST_CHECK(Text_geometry_measure(&invalid_font, text, sizeof(text),
                                     &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_font = valid_font;
    invalid_font.glyphs[(unsigned char)'A'].advance = FLT_MAX;
    TEST_CHECK(Text_geometry_measure(&invalid_font, overflow_text,
                                     sizeof(overflow_text), &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(&metrics, &original_metrics, sizeof(metrics)) == 0);

    TEST_CHECK(Text_geometry_build(&valid_font, text, sizeof(text),
                                   (RendererPoint2D){0.0f, 0.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, vertices, 11)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_build(&valid_font, text, sizeof(text),
                                   (RendererPoint2D){0.0f, 0.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, NULL, 12)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_build(&valid_font, text, sizeof(text),
                                   (RendererPoint2D){NAN, 0.0f},
                                   TEXT_GEOMETRY_ALIGN_LEFT,
                                   TEXT_GEOMETRY_ALIGN_TOP,
                                   white, vertices, 12)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_build(
                   &valid_font, text, sizeof(text),
                   (RendererPoint2D){0.0f, 0.0f},
                   (TextGeometryHorizontalAlign)99,
                   TEXT_GEOMETRY_ALIGN_TOP, white, vertices, 12)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_geometry_build(
                   &valid_font, text, sizeof(text),
                   (RendererPoint2D){0.0f, 0.0f},
                   TEXT_GEOMETRY_ALIGN_LEFT,
                   (TextGeometryVerticalAlign)99,
                   white, vertices, 12)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(vertices, original_vertices, sizeof(vertices)) == 0);
    return 0;
}

int main(void)
{
    if (check_metrics_and_top_left_glyph_quads() != 0)
        return 1;
    if (check_unsupported_bytes_use_fallback() != 0)
        return 1;
    if (check_alignment_uses_advance_bounds() != 0)
        return 1;
    if (check_newlines_align_each_line_and_use_line_spacing() != 0)
        return 1;
    if (check_zero_bitmap_glyphs_only_advance_the_pen() != 0)
        return 1;
    if (check_empty_text_has_no_logical_lines_or_geometry() != 0)
        return 1;
    if (check_invalid_inputs_are_rejected_atomically() != 0)
        return 1;
    return 0;
}
