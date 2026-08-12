/* Semantic glyph-atlas text drawing for the SDL renderer. */

#include "text_renderer.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct TextRenderer {
    SdlRenderer *sdl_renderer;
    Renderer *frontend;
    TextAtlas *atlas;
    const TextGeometryFont *font;
    RendererTexture *texture;
};

struct TextRendererCache {
    Renderer *frontend_identity;
    const TextGeometryFont *font_identity;
    unsigned char *text;
    size_t text_length;
    TextGeometryMetrics metrics;
};

static const unsigned char Text_renderer_empty_text[] = {0};

static const unsigned char *Text_renderer_normalize_text(
    const unsigned char *text, size_t text_length)
{
    if (text != NULL)
        return text;
    if (text_length == 0)
        return Text_renderer_empty_text;
    return NULL;
}

RendererStatus Text_renderer_create(SdlRenderer *sdl_renderer,
                                    TextAtlas *atlas,
                                    TextRenderer **text_renderer)
{
    TextRenderer *candidate;
    Renderer *frontend;
    const TextGeometryFont *font;
    RendererTexture *texture;

    if (sdl_renderer == NULL || atlas == NULL || text_renderer == NULL
        || *text_renderer != NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    frontend = Sdl_renderer_frontend(sdl_renderer);
    font = Text_atlas_geometry_font(atlas);
    texture = Text_atlas_texture(atlas);
    if (frontend == NULL || font == NULL || texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;

    candidate = malloc(sizeof(*candidate));
    if (candidate == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    candidate->sdl_renderer = sdl_renderer;
    candidate->frontend = frontend;
    candidate->atlas = atlas;
    candidate->font = font;
    candidate->texture = texture;
    *text_renderer = candidate;
    return RENDERER_STATUS_OK;
}

void Text_renderer_destroy(TextRenderer **text_renderer)
{
    if (text_renderer == NULL || *text_renderer == NULL)
        return;
    free(*text_renderer);
    *text_renderer = NULL;
}

RendererStatus Text_renderer_measure(const TextRenderer *text_renderer,
                                     const unsigned char *text,
                                     size_t text_length,
                                     TextGeometryMetrics *metrics)
{
    const unsigned char *normalized_text;

    if (text_renderer == NULL || metrics == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    normalized_text = Text_renderer_normalize_text(text, text_length);
    if (normalized_text == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return Text_geometry_measure(text_renderer->font, normalized_text,
                                 text_length, metrics);
}

static RendererStatus Text_renderer_build_vertices(
    const TextRenderer *text_renderer, const unsigned char *text,
    size_t text_length, const TextGeometryMetrics *metrics,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space, RendererVertex2D **vertices)
{
    RendererVertex2D *candidate = NULL;
    RendererStatus status;
    size_t index;

    if (space != TEXT_RENDERER_SPACE_HUD
        && space != TEXT_RENDERER_SPACE_WORLD) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (metrics->vertex_count > SIZE_MAX / sizeof(*candidate))
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (metrics->vertex_count > 0) {
        candidate = malloc(metrics->vertex_count * sizeof(*candidate));
        if (candidate == NULL)
            return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    status = Text_geometry_build(text_renderer->font, text, text_length,
                                 anchor, horizontal, vertical, color,
                                 candidate, metrics->vertex_count);
    if (status != RENDERER_STATUS_OK) {
        free(candidate);
        return status;
    }

    if (space == TEXT_RENDERER_SPACE_WORLD) {
        for (index = 0; index < metrics->vertex_count; index++) {
            candidate[index].y = 2.0f * anchor.y - candidate[index].y;
            if (!isfinite(candidate[index].y)) {
                free(candidate);
                return RENDERER_STATUS_INVALID_ARGUMENT;
            }
        }
    }
    *vertices = candidate;
    return RENDERER_STATUS_OK;
}

static RendererStatus Text_renderer_draw_measured(
    TextRenderer *text_renderer, const unsigned char *text,
    size_t text_length, const TextGeometryMetrics *metrics,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space)
{
    RendererVertex2D *vertices = NULL;
    RendererStatus status;

    if (metrics->vertex_count > 0) {
        status = Sdl_renderer_frame_result(text_renderer->sdl_renderer);
        if (status != RENDERER_STATUS_OK)
            return status;
    }
    status = Text_renderer_build_vertices(
        text_renderer, text, text_length, metrics, anchor, horizontal,
        vertical, color, space, &vertices);
    if (status != RENDERER_STATUS_OK) {
        if (status == RENDERER_STATUS_OUT_OF_MEMORY) {
            return Sdl_renderer_track_frame_result(
                text_renderer->sdl_renderer, status);
        }
        return status;
    }
    if (metrics->vertex_count == 0) {
        free(vertices);
        return RENDERER_STATUS_OK;
    }

    status = Renderer_set_blend(text_renderer->frontend,
                                RENDERER_BLEND_ALPHA);
    if (status != RENDERER_STATUS_OK) {
        free(vertices);
        return Sdl_renderer_track_frame_result(
            text_renderer->sdl_renderer, status);
    }
    status = Renderer_draw_triangles(
        text_renderer->frontend, text_renderer->texture, vertices,
        metrics->vertex_count);
    free(vertices);
    if (status != RENDERER_STATUS_OK) {
        return Sdl_renderer_track_frame_result(
            text_renderer->sdl_renderer, status);
    }
    status = Sdl_renderer_flush_preserving_legacy(
        text_renderer->sdl_renderer);
    return Sdl_renderer_track_frame_result(
        text_renderer->sdl_renderer, status);
}

RendererStatus Text_renderer_draw(
    TextRenderer *text_renderer, const unsigned char *text,
    size_t text_length, RendererPoint2D anchor,
    TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space)
{
    const unsigned char *normalized_text;
    TextGeometryMetrics metrics;
    RendererStatus status;

    if (text_renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    normalized_text = Text_renderer_normalize_text(text, text_length);
    if (normalized_text == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Text_geometry_measure(text_renderer->font, normalized_text,
                                   text_length, &metrics);
    if (status != RENDERER_STATUS_OK)
        return status;
    return Text_renderer_draw_measured(
        text_renderer, normalized_text, text_length, &metrics, anchor,
        horizontal, vertical, color, space);
}

static int Text_renderer_cache_matches(
    const TextRendererCache *cache, const TextRenderer *text_renderer)
{
    return cache->frontend_identity == text_renderer->frontend
        && cache->font_identity == text_renderer->font;
}

RendererStatus Text_renderer_cache_replace(TextRendererCache **cache,
                                           const TextRenderer *text_renderer,
                                           const unsigned char *text,
                                           size_t text_length)
{
    const unsigned char *normalized_text;
    TextRendererCache *candidate;
    TextGeometryMetrics metrics;
    RendererStatus status;

    if (cache == NULL || text_renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    normalized_text = Text_renderer_normalize_text(text, text_length);
    if (normalized_text == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (*cache != NULL) {
        if (!Text_renderer_cache_matches(*cache, text_renderer))
            return RENDERER_STATUS_RESOURCE_MISMATCH;
        if ((*cache)->text_length == text_length
            && (text_length == 0
                || memcmp((*cache)->text, normalized_text,
                          text_length) == 0)) {
            return RENDERER_STATUS_OK;
        }
    }

    status = Text_geometry_measure(text_renderer->font, normalized_text,
                                   text_length, &metrics);
    if (status != RENDERER_STATUS_OK)
        return status;
    candidate = malloc(sizeof(*candidate));
    if (candidate == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    candidate->text = NULL;
    if (text_length > 0) {
        candidate->text = malloc(text_length);
        if (candidate->text == NULL) {
            free(candidate);
            return RENDERER_STATUS_OUT_OF_MEMORY;
        }
        memcpy(candidate->text, normalized_text, text_length);
    }
    candidate->frontend_identity = text_renderer->frontend;
    candidate->font_identity = text_renderer->font;
    candidate->text_length = text_length;
    candidate->metrics = metrics;

    Text_renderer_cache_destroy(cache);
    *cache = candidate;
    return RENDERER_STATUS_OK;
}

void Text_renderer_cache_destroy(TextRendererCache **cache)
{
    if (cache == NULL || *cache == NULL)
        return;
    free((*cache)->text);
    free(*cache);
    *cache = NULL;
}

RendererStatus Text_renderer_cache_metrics(
    const TextRendererCache *cache, TextGeometryMetrics *metrics)
{
    if (cache == NULL || metrics == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *metrics = cache->metrics;
    return RENDERER_STATUS_OK;
}

RendererStatus Text_renderer_draw_cached(
    TextRenderer *text_renderer, const TextRendererCache *cache,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space)
{
    const unsigned char *text;

    if (text_renderer == NULL || cache == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!Text_renderer_cache_matches(cache, text_renderer))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    text = Text_renderer_normalize_text(cache->text, cache->text_length);
    return Text_renderer_draw_measured(
        text_renderer, text, cache->text_length, &cache->metrics, anchor,
        horizontal, vertical, color, space);
}
