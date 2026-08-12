#include "test_helpers.h"

#include "sdlrenderer.h"
#include "text_atlas.h"
#include "text_renderer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DRAWS 8
#define MAX_VERTICES 64
#define FLOAT_TOLERANCE 0.0001f

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    unsigned int identifier;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

struct TextAtlas {
    Renderer *renderer;
    RendererTexture *texture;
    TextGeometryFont geometry;
};

typedef struct FakeDraw {
    RendererTexture *texture;
    RendererVertex2D vertices[MAX_VERTICES];
    size_t vertex_count;
} FakeDraw;

static Renderer first_frontend;
static Renderer second_frontend;
static SdlRenderer first_sdl_renderer = {
    &first_frontend,
    RENDERER_STATUS_OK
};
static SdlRenderer second_sdl_renderer = {
    &second_frontend,
    RENDERER_STATUS_OK
};
static RendererTexture first_texture = {1};
static RendererTexture second_texture = {2};
static TextAtlas first_atlas;
static TextAtlas second_atlas;
static FakeDraw draws[MAX_DRAWS];
static int draw_attempts;
static int draw_count;
static int blend_attempts;
static int blend_calls;
static int preserving_flush_attempts;
static int preserving_flush_successes;
static int fail_next_draw;
static int fail_next_blend;
static int fail_next_flush;
static int fail_next_malloc;
static int texture_create_calls;
static int texture_update_calls;
static int texture_destroy_calls;
static int mesh_create_calls;
static int mesh_update_calls;
static int mesh_destroy_calls;

void *__real_malloc(size_t size);

void *__wrap_malloc(size_t size)
{
    if (fail_next_malloc) {
        fail_next_malloc = 0;
        return NULL;
    }
    return __real_malloc(size);
}

static int float_equal(float actual, float expected)
{
    float difference = actual - expected;

    return difference >= -FLOAT_TOLERANCE
        && difference <= FLOAT_TOLERANCE;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, float u, float v,
                        RendererColor color)
{
    return float_equal(vertex->x, x) && float_equal(vertex->y, y)
        && float_equal(vertex->u, u) && float_equal(vertex->v, v)
        && color_equal(vertex->color, color);
}

static TextGeometryFont make_font(float advance_offset)
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
    font.glyphs[(unsigned char)'A'].advance = 7.0f + advance_offset;
    font.glyphs[(unsigned char)'A'].available = 1;

    font.glyphs[(unsigned char)'B'].atlas_bounds
        = (RendererRect){5, 0, 4, 6};
    font.glyphs[(unsigned char)'B'].bearing_x = -1.0f;
    font.glyphs[(unsigned char)'B'].bearing_top = 5.0f;
    font.glyphs[(unsigned char)'B'].advance = 6.0f + advance_offset;
    font.glyphs[(unsigned char)'B'].available = 1;

    font.glyphs[(unsigned char)'?'].atlas_bounds
        = (RendererRect){9, 0, 3, 7};
    font.glyphs[(unsigned char)'?'].bearing_x = 0.0f;
    font.glyphs[(unsigned char)'?'].bearing_top = 6.0f;
    font.glyphs[(unsigned char)'?'].advance = 4.0f + advance_offset;
    font.glyphs[(unsigned char)'?'].available = 1;

    font.glyphs[(unsigned char)' '].atlas_bounds
        = (RendererRect){64, 32, 0, 0};
    font.glyphs[(unsigned char)' '].advance = 3.0f + advance_offset;
    font.glyphs[(unsigned char)' '].available = 1;
    return font;
}

static void reset_fixture(void)
{
    memset(&first_frontend, 0, sizeof(first_frontend));
    memset(&second_frontend, 0, sizeof(second_frontend));
    memset(draws, 0, sizeof(draws));
    memset(&first_atlas, 0, sizeof(first_atlas));
    memset(&second_atlas, 0, sizeof(second_atlas));
    first_atlas.renderer = &first_frontend;
    first_atlas.texture = &first_texture;
    first_atlas.geometry = make_font(0.0f);
    second_atlas.renderer = &second_frontend;
    second_atlas.texture = &second_texture;
    second_atlas.geometry = make_font(10.0f);
    first_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    second_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    draw_attempts = 0;
    draw_count = 0;
    blend_attempts = 0;
    blend_calls = 0;
    preserving_flush_attempts = 0;
    preserving_flush_successes = 0;
    fail_next_draw = 0;
    fail_next_blend = 0;
    fail_next_flush = 0;
    fail_next_malloc = 0;
    texture_create_calls = 0;
    texture_update_calls = 0;
    texture_destroy_calls = 0;
    mesh_create_calls = 0;
    mesh_update_calls = 0;
    mesh_destroy_calls = 0;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer != NULL ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frontend == NULL || !renderer->frontend->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->frame_result == RENDERER_STATUS_OK
        && status != RENDERER_STATUS_OK) {
        renderer->frame_result = status;
    }
    return renderer->frame_result;
}

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return renderer->frame_result;
}

static void begin_fake_frame(SdlRenderer *renderer)
{
    renderer->frontend->frame_active = 1;
    renderer->frame_result = RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
{
    preserving_flush_attempts++;
    if (renderer == NULL || renderer->frontend == NULL
        || !renderer->frontend->frame_active) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (fail_next_flush) {
        fail_next_flush = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    preserving_flush_successes++;
    return RENDERER_STATUS_OK;
}

const TextGeometryFont *Text_atlas_geometry_font(const TextAtlas *atlas)
{
    return atlas != NULL ? &atlas->geometry : NULL;
}

RendererTexture *Text_atlas_texture(const TextAtlas *atlas)
{
    return atlas != NULL ? atlas->texture : NULL;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (blend != RENDERER_BLEND_ALPHA)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    blend_attempts++;
    if (fail_next_blend) {
        fail_next_blend = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    blend_calls++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    FakeDraw *draw;

    draw_attempts++;
    if (renderer == NULL || texture == NULL || vertices == NULL
        || vertex_count == 0 || vertex_count % 3 != 0) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if ((renderer == &first_frontend && texture != &first_texture)
        || (renderer == &second_frontend && texture != &second_texture)) {
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    }
    if (fail_next_draw) {
        fail_next_draw = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (draw_count >= MAX_DRAWS || vertex_count > MAX_VERTICES)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    draw = &draws[draw_count++];
    draw->texture = texture;
    draw->vertex_count = vertex_count;
    memcpy(draw->vertices, vertices, vertex_count * sizeof(*vertices));
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *pixels, size_t pitch, RendererTexture **texture)
{
    (void)renderer;
    (void)desc;
    (void)pixels;
    (void)pitch;
    (void)texture;
    texture_create_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *pixels,
                                       size_t pitch)
{
    (void)renderer;
    (void)texture;
    (void)region;
    (void)pixels;
    (void)pitch;
    texture_update_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    (void)renderer;
    (void)texture;
    texture_destroy_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_create(Renderer *renderer,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    RendererMesh **mesh)
{
    (void)renderer;
    (void)vertices;
    (void)vertex_count;
    (void)mesh;
    mesh_create_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_update(Renderer *renderer, RendererMesh *mesh,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count)
{
    (void)renderer;
    (void)mesh;
    (void)vertices;
    (void)vertex_count;
    mesh_update_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_destroy(Renderer *renderer, RendererMesh *mesh)
{
    (void)renderer;
    (void)mesh;
    mesh_destroy_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static int no_gpu_resource_calls(void)
{
    return texture_create_calls == 0 && texture_update_calls == 0
        && texture_destroy_calls == 0 && mesh_create_calls == 0
        && mesh_update_calls == 0 && mesh_destroy_calls == 0;
}

static int metrics_equal(const TextGeometryMetrics *metrics,
                         float width, float height, size_t line_count,
                         size_t glyph_count, size_t vertex_count)
{
    return float_equal(metrics->width, width)
        && float_equal(metrics->height, height)
        && metrics->line_count == line_count
        && metrics->glyph_count == glyph_count
        && metrics->vertex_count == vertex_count;
}

static int check_create_measure_and_empty_draw(void)
{
    const unsigned char text[] = {'A', 0x80, '\n', 'B'};
    TextGeometryMetrics metrics = {0};
    TextRenderer *renderer = NULL;

    reset_fixture();
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Text_renderer_measure(renderer, text, sizeof(text), &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 11.0f, 22.0f, 2, 3, 18));

    first_frontend.frame_active = 1;
    TEST_CHECK(Text_renderer_draw(
                   renderer, NULL, 0, (RendererPoint2D){20.0f, 30.0f},
                   TEXT_GEOMETRY_ALIGN_LEFT, TEXT_GEOMETRY_ALIGN_TOP,
                   (RendererColor){1, 2, 3, 4}, TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_OK);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(preserving_flush_attempts == 0);
    TEST_CHECK(no_gpu_resource_calls());

    Text_renderer_destroy(&renderer);
    TEST_CHECK(renderer == NULL);
    Text_renderer_destroy(&renderer);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

static int check_immediate_hud_and_world_draws(void)
{
    const unsigned char text[] = {'A', 'B'};
    const RendererPoint2D anchor = {100.0f, 200.0f};
    const RendererColor color = {17, 34, 51, 68};
    TextRenderer *renderer = NULL;
    const FakeDraw *hud;
    const FakeDraw *world;

    reset_fixture();
    first_frontend.frame_active = 1;
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_draw(renderer, text, sizeof(text), anchor,
                                  TEXT_GEOMETRY_ALIGN_CENTER,
                                  TEXT_GEOMETRY_ALIGN_MIDDLE, color,
                                  TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_OK);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(preserving_flush_attempts == 1);
    TEST_CHECK(preserving_flush_successes == 1);
    TEST_CHECK(no_gpu_resource_calls());
    hud = &draws[0];
    TEST_CHECK(hud->texture == &first_texture);
    TEST_CHECK(hud->vertex_count == 12);
    TEST_CHECK(vertex_equal(&hud->vertices[0], 94.5f, 195.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(&hud->vertices[2], 99.5f, 203.0f,
                            5.0f / 64.0f, 8.0f / 32.0f, color));
    TEST_CHECK(vertex_equal(&hud->vertices[6], 99.5f, 197.0f,
                            5.0f / 64.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(&hud->vertices[8], 103.5f, 203.0f,
                            9.0f / 64.0f, 6.0f / 32.0f, color));

    TEST_CHECK(Text_renderer_draw(renderer, text, sizeof(text), anchor,
                                  TEXT_GEOMETRY_ALIGN_CENTER,
                                  TEXT_GEOMETRY_ALIGN_MIDDLE, color,
                                  TEXT_RENDERER_SPACE_WORLD)
               == RENDERER_STATUS_OK);
    TEST_CHECK(draw_count == 2);
    TEST_CHECK(blend_calls == 2);
    TEST_CHECK(preserving_flush_attempts == 2);
    world = &draws[1];
    TEST_CHECK(world->vertex_count == hud->vertex_count);
    TEST_CHECK(vertex_equal(&world->vertices[0], 94.5f, 205.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(&world->vertices[2], 99.5f, 197.0f,
                            5.0f / 64.0f, 8.0f / 32.0f, color));
    TEST_CHECK(vertex_equal(&world->vertices[6], 99.5f, 203.0f,
                            5.0f / 64.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(&world->vertices[8], 103.5f, 197.0f,
                            9.0f / 64.0f, 6.0f / 32.0f, color));

    Text_renderer_destroy(&renderer);
    return 0;
}

static int check_cache_is_cpu_only_atomic_and_idempotent(void)
{
    const unsigned char original[] = {'A', 0x80};
    const unsigned char replacement[] = {'B', ' ', 'A'};
    TextGeometryMetrics metrics;
    TextRenderer *renderer = NULL;
    TextRendererCache *cache = NULL;
    TextRendererCache *original_cache;

    reset_fixture();
    first_frontend.frame_active = 1;
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_cache_replace(&cache, renderer, original,
                                           sizeof(original))
               == RENDERER_STATUS_OK);
    TEST_CHECK(cache != NULL);
    TEST_CHECK(no_gpu_resource_calls());
    TEST_CHECK(Text_renderer_cache_metrics(cache, &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 11.0f, 10.0f, 1, 2, 12));

    original_cache = cache;
    TEST_CHECK(Text_renderer_cache_replace(&cache, renderer, original,
                                           sizeof(original))
               == RENDERER_STATUS_OK);
    TEST_CHECK(cache == original_cache);
    TEST_CHECK(no_gpu_resource_calls());

    TEST_CHECK(Text_renderer_cache_replace(&cache, renderer, replacement,
                                           sizeof(replacement))
               == RENDERER_STATUS_OK);
    TEST_CHECK(cache != NULL);
    TEST_CHECK(Text_renderer_cache_metrics(cache, &metrics)
               == RENDERER_STATUS_OK);
    TEST_CHECK(metrics_equal(&metrics, 16.0f, 10.0f, 1, 2, 12));
    TEST_CHECK(no_gpu_resource_calls());

    Text_renderer_cache_destroy(&cache);
    TEST_CHECK(cache == NULL);
    Text_renderer_cache_destroy(&cache);
    Text_renderer_destroy(&renderer);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

static int check_cached_draw_and_font_identity(void)
{
    const unsigned char text[] = {'A', 0x80};
    const RendererPoint2D anchor = {10.0f, 20.0f};
    const RendererColor color = {80, 90, 100, 110};
    TextRenderer *first = NULL;
    TextRenderer *second = NULL;
    TextRendererCache *cache = NULL;

    reset_fixture();
    first_frontend.frame_active = 1;
    second_frontend.frame_active = 1;
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &first) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_create(&second_sdl_renderer, &second_atlas,
                                    &second) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_cache_replace(&cache, first, text, sizeof(text))
               == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_draw_cached(
                   first, cache, anchor, TEXT_GEOMETRY_ALIGN_RIGHT,
                   TEXT_GEOMETRY_ALIGN_BOTTOM, color,
                   TEXT_RENDERER_SPACE_HUD) == RENDERER_STATUS_OK);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].vertex_count == 12);
    TEST_CHECK(vertex_equal(&draws[0].vertices[0], 0.0f, 10.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(&draws[0].vertices[6], 6.0f, 11.0f,
                            9.0f / 64.0f, 0.0f, color));
    TEST_CHECK(preserving_flush_attempts == 1);

    TEST_CHECK(Text_renderer_draw_cached(
                   second, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, color,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(preserving_flush_attempts == 1);
    TEST_CHECK(no_gpu_resource_calls());

    Text_renderer_cache_destroy(&cache);
    Text_renderer_destroy(&second);
    Text_renderer_destroy(&first);
    return 0;
}

static int check_failures_preserve_cache_and_ordering(void)
{
    const unsigned char text[] = {'A', 'B'};
    const RendererPoint2D anchor = {0.0f, 0.0f};
    const RendererColor white = {255, 255, 255, 255};
    TextGeometryMetrics before;
    TextGeometryMetrics after;
    TextRenderer *renderer = NULL;
    TextRendererCache *cache = NULL;
    TextRendererCache *original_cache;

    reset_fixture();
    begin_fake_frame(&first_sdl_renderer);
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_cache_replace(&cache, renderer, text,
                                           sizeof(text))
               == RENDERER_STATUS_OK);
    original_cache = cache;
    TEST_CHECK(Text_renderer_cache_metrics(cache, &before)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Text_renderer_cache_replace(&cache, renderer, NULL, 1)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(cache == original_cache);
    TEST_CHECK(Text_renderer_cache_metrics(cache, &after)
               == RENDERER_STATUS_OK);
    TEST_CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    fail_next_blend = 1;
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(first_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_calls == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(preserving_flush_attempts == 0);

    /* Once a frame has failed, even the other public draw entry point must
     * return the first failure without queuing or flushing more work. */
    TEST_CHECK(Text_renderer_draw(
                   renderer, text, sizeof(text), anchor,
                   TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(preserving_flush_attempts == 0);

    /* Vertex allocation is part of drawing, so OOM must abort the frame even
     * though no backend state or command has been submitted yet. */
    begin_fake_frame(&first_sdl_renderer);
    fail_next_malloc = 1;
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(first_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(preserving_flush_attempts == 0);
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);

    begin_fake_frame(&first_sdl_renderer);
    fail_next_draw = 1;
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(first_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(preserving_flush_attempts == 0);
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(preserving_flush_attempts == 0);

    begin_fake_frame(&first_sdl_renderer);
    fail_next_flush = 1;
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(first_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 3);
    TEST_CHECK(blend_calls == 2);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(preserving_flush_attempts == 1);
    TEST_CHECK(preserving_flush_successes == 0);
    TEST_CHECK(Text_renderer_draw(
                   renderer, text, sizeof(text), anchor,
                   TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(blend_attempts == 3);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(preserving_flush_attempts == 1);

    /* A new successful frame clears the sticky result and permits drawing
     * the retained CPU cache again. */
    begin_fake_frame(&first_sdl_renderer);
    TEST_CHECK(Text_renderer_draw_cached(
                   renderer, cache, anchor, TEXT_GEOMETRY_ALIGN_LEFT,
                   TEXT_GEOMETRY_ALIGN_TOP, white,
                   TEXT_RENDERER_SPACE_HUD) == RENDERER_STATUS_OK);
    TEST_CHECK(blend_attempts == 4);
    TEST_CHECK(blend_calls == 3);
    TEST_CHECK(draw_attempts == 3);
    TEST_CHECK(draw_count == 2);
    TEST_CHECK(preserving_flush_attempts == 2);
    TEST_CHECK(preserving_flush_successes == 1);
    TEST_CHECK(Text_renderer_cache_metrics(cache, &after)
               == RENDERER_STATUS_OK);
    TEST_CHECK(memcmp(&before, &after, sizeof(before)) == 0);
    TEST_CHECK(no_gpu_resource_calls());

    Text_renderer_cache_destroy(&cache);
    Text_renderer_destroy(&renderer);
    return 0;
}

static int check_invalid_arguments_do_not_publish(void)
{
    const unsigned char text[] = {'A'};
    TextRenderer *renderer = NULL;
    TextRenderer *other_renderer = NULL;
    TextRenderer *occupied = (TextRenderer *)(uintptr_t)1;
    TextRendererCache *cache = NULL;
    TextRendererCache *other_cache = NULL;
    TextGeometryMetrics metrics;

    reset_fixture();
    TEST_CHECK(Text_renderer_create(NULL, &first_atlas, &renderer)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(renderer == NULL);
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, NULL, &renderer)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(renderer == NULL);
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &occupied)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(occupied == (TextRenderer *)(uintptr_t)1);

    TEST_CHECK(Text_renderer_create(&first_sdl_renderer, &first_atlas,
                                    &renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_measure(NULL, text, sizeof(text), &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_measure(renderer, NULL, sizeof(text), &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_cache_replace(NULL, renderer, text,
                                           sizeof(text))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_cache_replace(&cache, NULL, text, sizeof(text))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_create(&second_sdl_renderer, &second_atlas,
                                    &other_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_cache_replace(&other_cache, other_renderer,
                                           text, sizeof(text))
               == RENDERER_STATUS_OK);
    TEST_CHECK(Text_renderer_cache_replace(&other_cache, renderer, text,
                                           sizeof(text))
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(other_cache != NULL);
    TEST_CHECK(Text_renderer_cache_metrics(NULL, &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_cache_metrics(cache, &metrics)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Text_renderer_draw(
                   renderer, text, sizeof(text),
                   (RendererPoint2D){0.0f, 0.0f},
                   TEXT_GEOMETRY_ALIGN_LEFT, TEXT_GEOMETRY_ALIGN_TOP,
                   (RendererColor){1, 2, 3, 4}, (TextRendererSpace)99)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(preserving_flush_attempts == 0);
    TEST_CHECK(no_gpu_resource_calls());

    Text_renderer_cache_destroy(&other_cache);
    Text_renderer_destroy(&other_renderer);
    Text_renderer_destroy(&renderer);
    return 0;
}

int main(void)
{
    if (check_create_measure_and_empty_draw() != 0)
        return 1;
    if (check_immediate_hud_and_world_draws() != 0)
        return 1;
    if (check_cache_is_cpu_only_atomic_and_idempotent() != 0)
        return 1;
    if (check_cached_draw_and_font_identity() != 0)
        return 1;
    if (check_failures_preserve_cache_and_ordering() != 0)
        return 1;
    if (check_invalid_arguments_do_not_publish() != 0)
        return 1;
    return 0;
}
