#include "test_helpers.h"

#include "text_ttf_atlas.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdint.h>
#include <string.h>

#ifndef XPILOT_TEST_FONT_PATH
#error "XPILOT_TEST_FONT_PATH must name the bundled test font"
#endif

struct Renderer {
    int unused;
};

struct RendererTexture {
    unsigned int identifier;
};

static Renderer fake_renderer;
static RendererTexture fake_texture;
static RendererTextureDesc created_desc;
static int texture_create_count;
static int texture_destroy_count;

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    if (renderer != &fake_renderer || desc == NULL || rgba_pixels == NULL
        || texture == NULL || *texture != NULL
        || desc->width <= 0 || desc->height <= 0
        || pitch < (size_t)desc->width * 4) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    texture_create_count++;
    created_desc = *desc;
    fake_texture.identifier = 1;
    *texture = &fake_texture;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    if (renderer != &fake_renderer || texture != &fake_texture
        || texture_destroy_count != 0) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    texture_destroy_count++;
    return RENDERER_STATUS_OK;
}

static int check_invalid_arguments_do_not_publish(TTF_Font *font)
{
    TextAtlas *atlas = NULL;
    TextAtlas *occupied = (TextAtlas *)(uintptr_t)1;

    TEST_CHECK(Text_ttf_atlas_create(NULL, font, &atlas)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(Text_ttf_atlas_create(&fake_renderer, NULL, &atlas)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(Text_ttf_atlas_create(&fake_renderer, font, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_ttf_atlas_create(&fake_renderer, font, &occupied)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(occupied == (TextAtlas *)(uintptr_t)1);
    TEST_CHECK(texture_create_count == 0);
    return 0;
}

static int check_real_font_creates_self_contained_atlas(TTF_Font **font_pointer)
{
    TextAtlas *atlas = NULL;
    TTF_Font *font = *font_pointer;
    const TextGeometryFont *geometry;
    TextGeometryMetrics fallback_metrics;
    TextGeometryMetrics unknown_metrics;
    int font_ascent = TTF_FontAscent(font);
    int font_height = TTF_FontHeight(font);
    int font_line_skip = TTF_FontLineSkip(font);
    int a_min_x;
    int a_max_x;
    int a_min_y;
    int a_max_y;
    int a_advance;
    int space_min_x;
    int space_max_x;
    int space_min_y;
    int space_max_y;
    int space_advance;
    static const unsigned char fallback_text[] = "?";
    static const unsigned char unknown_text[] = {0x80};

    TEST_CHECK(TTF_GlyphMetrics(font, (Uint16)'A',
                                &a_min_x, &a_max_x,
                                &a_min_y, &a_max_y, &a_advance) == 0);
    TEST_CHECK(TTF_GlyphMetrics(font, (Uint16)' ',
                                &space_min_x, &space_max_x,
                                &space_min_y, &space_max_y,
                                &space_advance) == 0);
    TEST_CHECK(Text_ttf_atlas_create(&fake_renderer, font, &atlas)
               == RENDERER_STATUS_OK);
    TEST_CHECK(atlas != NULL);
    TEST_CHECK(texture_create_count == 1);
    TEST_CHECK(created_desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(created_desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);

    geometry = Text_atlas_geometry_font(atlas);
    TEST_CHECK(geometry != NULL);
    TEST_CHECK(geometry->ascent == (float)font_ascent);
    TEST_CHECK(geometry->line_height == (float)font_height);
    TEST_CHECK(geometry->line_spacing == (float)font_line_skip);
    TEST_CHECK(geometry->glyphs[(unsigned char)'A'].available);
    TEST_CHECK(geometry->glyphs[(unsigned char)'?'].available);
    TEST_CHECK(geometry->glyphs[(unsigned char)'A'].advance
               == (float)a_advance);
    TEST_CHECK(geometry->glyphs[(unsigned char)'A'].bearing_x
               == (float)(a_min_x < 0 ? a_min_x : 0));
    TEST_CHECK(geometry->glyphs[(unsigned char)'A'].bearing_top
               == (float)font_ascent);
    TEST_CHECK(geometry->glyphs[(unsigned char)' '].available);
    TEST_CHECK(geometry->glyphs[(unsigned char)' '].advance
               == (float)space_advance);
    if (space_max_x <= space_min_x || space_max_y <= space_min_y) {
        TEST_CHECK(geometry->glyphs[(unsigned char)' ']
                       .atlas_bounds.width == 0);
        TEST_CHECK(geometry->glyphs[(unsigned char)' ']
                       .atlas_bounds.height == 0);
    }

    TTF_CloseFont(font);
    *font_pointer = NULL;

    TEST_CHECK(Text_geometry_measure(geometry, fallback_text,
                                     sizeof(fallback_text) - 1,
                                     &fallback_metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Text_geometry_measure(geometry, unknown_text,
                                     sizeof(unknown_text),
                                     &unknown_metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(unknown_metrics.width == fallback_metrics.width);
    TEST_CHECK(unknown_metrics.height == fallback_metrics.height);
    TEST_CHECK(unknown_metrics.line_count == fallback_metrics.line_count);
    TEST_CHECK(unknown_metrics.glyph_count == fallback_metrics.glyph_count);
    TEST_CHECK(unknown_metrics.vertex_count == fallback_metrics.vertex_count);

    TEST_CHECK(Text_atlas_destroy(&atlas) == RENDERER_STATUS_OK);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(texture_destroy_count == 1);
    return 0;
}

int main(void)
{
    TTF_Font *font = NULL;
    int result = 1;

    memset(&created_desc, 0, sizeof(created_desc));
    if (SDL_Init(0) < 0)
        return 1;
    if (TTF_Init() < 0)
        goto cleanup_sdl;
    font = TTF_OpenFont(XPILOT_TEST_FONT_PATH, 17);
    if (font == NULL)
        goto cleanup_ttf;
    if (check_invalid_arguments_do_not_publish(font) != 0)
        goto cleanup_font;
    if (check_real_font_creates_self_contained_atlas(&font) != 0)
        goto cleanup_font;
    result = 0;
    goto cleanup_ttf;

cleanup_font:
    if (font != NULL)
        TTF_CloseFont(font);
cleanup_ttf:
    TTF_Quit();
cleanup_sdl:
    SDL_Quit();
    return result;
}
