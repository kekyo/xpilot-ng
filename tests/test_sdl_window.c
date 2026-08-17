#include "test_helpers.h"

#include <stdbool.h>

#include "sdlinit.h"
#include "sdlrenderer.h"
#include "sdlwindow.h"

#include <SDL3/SDL.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURES 16
#define MAX_PAINT_EVENTS 8
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

typedef struct FakeTexture {
    RendererTexture handle;
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    int destroy_count;
} FakeTexture;

typedef struct FakeSprite {
    RendererTexture *texture;
    float left;
    float top;
    float right;
    float bottom;
    float u0;
    float v0;
    float u1;
    float v1;
    RendererColor tint;
} FakeSprite;

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
} FakeFill;

typedef struct FakeColoredStroke {
    RendererPoint2D points[4];
    RendererColor colors[4];
    size_t point_count;
    float width;
    int closed;
} FakeColoredStroke;

typedef enum PaintEvent {
    PAINT_EVENT_BLEND,
    PAINT_EVENT_FILL,
    PAINT_EVENT_SPRITE,
    PAINT_EVENT_STROKE,
    PAINT_EVENT_FLUSH
} PaintEvent;

static Renderer fake_renderer;
static Renderer foreign_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer, RENDERER_STATUS_OK
};
static FakeTexture fake_textures[MAX_TEXTURES];
static FakeSprite last_sprite;
static FakeFill last_fill;
static FakeColoredStroke last_stroke;
static PaintEvent paint_events[MAX_PAINT_EVENTS];
static int paint_event_count;
static int texture_create_attempts;
static int texture_create_successes;
static int fail_texture_create_attempt;
static int texture_update_calls;
static int texture_destroy_calls;
static int resource_calls_during_frame;
static int operation_result_pending;
static int preflight_attempts;
static int tracked_operation_results;
static int blend_attempts;
static int fill_attempts;
static int successful_fills;
static int sprite_attempts;
static int successful_sprites;
static int stroke_attempts;
static int successful_strokes;
static int flush_attempts;
static RendererStatus blend_result;
static RendererStatus fill_result;
static RendererStatus sprite_result;
static RendererStatus stroke_result;
static RendererStatus flush_result;

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

static void record_paint_event(PaintEvent event)
{
    if (paint_event_count < MAX_PAINT_EVENTS)
        paint_events[paint_event_count++] = event;
}

static void reset_paint_frame(void)
{
    memset(&last_sprite, 0, sizeof(last_sprite));
    memset(&last_fill, 0, sizeof(last_fill));
    memset(&last_stroke, 0, sizeof(last_stroke));
    memset(paint_events, 0, sizeof(paint_events));
    paint_event_count = 0;
    operation_result_pending = 0;
    preflight_attempts = 0;
    tracked_operation_results = 0;
    blend_attempts = 0;
    fill_attempts = 0;
    successful_fills = 0;
    sprite_attempts = 0;
    successful_sprites = 0;
    stroke_attempts = 0;
    successful_strokes = 0;
    flush_attempts = 0;
    blend_result = RENDERER_STATUS_OK;
    fill_result = RENDERER_STATUS_OK;
    sprite_result = RENDERER_STATUS_OK;
    stroke_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    fake_renderer.frame_active = 1;
    foreign_renderer.frame_active = 1;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
}

static FakeTexture *fake_texture_from_handle(RendererTexture *texture)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        if (&fake_textures[index].handle == texture)
            return &fake_textures[index];
    }
    return NULL;
}

static int pixel_equals(const FakeTexture *texture, int x, int y,
                        uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t alpha)
{
    const uint8_t *pixel;

    if (texture == NULL || x < 0 || x >= texture->desc.width
        || y < 0 || y >= texture->desc.height) {
        return 0;
    }
    pixel = texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color = {
        (uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16),
        (uint8_t)(rgba >> 8), (uint8_t)rgba
    };

    return color;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    FakeTexture *record;
    int y;

    texture_create_attempts++;
    if (renderer == NULL || desc == NULL || rgba_pixels == NULL
        || texture == NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (texture_create_attempts == fail_texture_create_attempt)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (texture_create_successes >= MAX_TEXTURES)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    record = &fake_textures[texture_create_successes];
    memset(record, 0, sizeof(*record));
    record->handle.identifier = (unsigned int)texture_create_successes + 1;
    record->desc = *desc;
    record->pitch = (size_t)desc->width * 4;
    record->pixels = malloc(record->pitch * (size_t)desc->height);
    if (record->pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    for (y = 0; y < desc->height; y++) {
        memcpy(record->pixels + (size_t)y * record->pitch,
               rgba_pixels + (size_t)y * pitch, record->pitch);
    }
    texture_create_successes++;
    *texture = &record->handle;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *rgba_pixels,
                                       size_t pitch)
{
    (void)texture;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    texture_update_calls++;
    if (renderer != NULL && renderer->frame_active)
        resource_calls_during_frame++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    if (renderer == NULL || record == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    record->destroy_count++;
    texture_destroy_calls++;
    return record->destroy_count == 1 ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_STATE;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    blend_attempts++;
    operation_result_pending = 1;
    record_paint_event(PAINT_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return blend_result;
}

RendererStatus Renderer_fill_rect(Renderer *renderer,
                                  float x, float y,
                                  float width, float height,
                                  RendererColor color)
{
    fill_attempts++;
    operation_result_pending = 1;
    record_paint_event(PAINT_EVENT_FILL);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (fill_result != RENDERER_STATUS_OK)
        return fill_result;
    last_fill.x = x;
    last_fill.y = y;
    last_fill.width = width;
    last_fill.height = height;
    last_fill.color = color;
    successful_fills++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    sprite_attempts++;
    operation_result_pending = 1;
    record_paint_event(PAINT_EVENT_SPRITE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || record == NULL || record->destroy_count != 0) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (sprite_result != RENDERER_STATUS_OK)
        return sprite_result;
    last_sprite.texture = texture;
    last_sprite.left = left;
    last_sprite.top = top;
    last_sprite.right = right;
    last_sprite.bottom = bottom;
    last_sprite.u0 = u0;
    last_sprite.v0 = v0;
    last_sprite.u1 = u1;
    last_sprite.v1 = v1;
    last_sprite.tint = tint;
    successful_sprites++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_stroke_colored_path(
    Renderer *renderer, const RendererPoint2D *points,
    const RendererColor *colors, size_t point_count,
    float width, int closed)
{
    stroke_attempts++;
    operation_result_pending = 1;
    record_paint_event(PAINT_EVENT_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || colors == NULL || point_count != 4) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (stroke_result != RENDERER_STATUS_OK)
        return stroke_result;
    memcpy(last_stroke.points, points,
           point_count * sizeof(*last_stroke.points));
    memcpy(last_stroke.colors, colors,
           point_count * sizeof(*last_stroke.colors));
    last_stroke.point_count = point_count;
    last_stroke.width = width;
    last_stroke.closed = closed;
    successful_strokes++;
    return RENDERER_STATUS_OK;
}

SdlRenderer *Get_sdl_renderer(void)
{
    return &fake_sdl_renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer == &fake_sdl_renderer ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status)
{
    if (operation_result_pending) {
        operation_result_pending = 0;
        tracked_operation_results++;
    } else if (status == RENDERER_STATUS_OK) {
        preflight_attempts++;
    }
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
    return renderer == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                            : renderer->frame_result;
}

RendererStatus Sdl_renderer_flush(SdlRenderer *renderer)
{
    flush_attempts++;
    operation_result_pending = 1;
    record_paint_event(PAINT_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer || renderer->frontend == NULL
        || !renderer->frontend->frame_active) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return flush_result;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

static int check_initial_prepare(sdl_window_t *window,
                                 RendererTexture **first_texture)
{
    SDL_Surface *surface_before_failure;
    Uint32 background;
    Uint32 marker;
    SDL_Rect marker_rect = {2, 1, 1, 1};
    FakeTexture *texture;

    memset(window, 0, sizeof(*window));
    TEST_CHECK(sdl_window_init(NULL, 1, 2, 3, 4) == -1);
    TEST_CHECK(sdl_window_init(window, 1, 2, 0, 4) == -1);
    TEST_CHECK(window->surface == NULL);
    TEST_CHECK(window->x == 0 && window->y == 0);
    TEST_CHECK(window->w == 0 && window->h == 0);

    TEST_CHECK(sdl_window_init(window, 7, 9, 5, 3) == 0);
    TEST_CHECK(window->surface != NULL);
    TEST_CHECK(window->surface->w == 8 && window->surface->h == 4);
    TEST_CHECK(window->x == 7 && window->y == 9);
    TEST_CHECK(window->w == 5 && window->h == 3);
    TEST_CHECK(texture_create_attempts == 0);
    TEST_CHECK(texture_destroy_calls == 0);

    background = SDL_MapSurfaceRGBA(window->surface, 10, 20, 30, 40);
    marker = SDL_MapSurfaceRGBA(window->surface, 70, 80, 90, 100);
    TEST_CHECK(SDL_FillSurfaceRect(window->surface, NULL, background));
    TEST_CHECK(SDL_FillSurfaceRect(window->surface, &marker_rect, marker));
    sdl_window_refresh(window);

    surface_before_failure = window->surface;
    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == -1);
    TEST_CHECK(window->surface == surface_before_failure);
    TEST_CHECK(window->x == 7 && window->y == 9);
    TEST_CHECK(window->w == 5 && window->h == 3);
    TEST_CHECK(texture_create_successes == 0);
    TEST_CHECK(texture_destroy_calls == 0);

    fail_texture_create_attempt = 0;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 1);
    TEST_CHECK(texture_destroy_calls == 0);
    texture = &fake_textures[0];
    TEST_CHECK(texture->desc.width == 8 && texture->desc.height == 4);
    TEST_CHECK(texture->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(texture->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(pixel_equals(texture, 0, 0, 10, 20, 30, 40));
    TEST_CHECK(pixel_equals(texture, 2, 1, 70, 80, 90, 100));
    *first_texture = &texture->handle;

    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 1);
    TEST_CHECK(texture_destroy_calls == 0);
    return 0;
}

static int check_dirty_refresh_and_semantic_paint(
    sdl_window_t *window, RendererTexture *old_texture,
    RendererTexture **refreshed_texture)
{
    SDL_Rect changed_rect = {4, 2, 1, 1};
    Uint32 changed;
    int attempts_before;
    int destroys_before;
    FakeTexture *texture;
    const RendererColor white = {255, 255, 255, 255};
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor green = {0, 144, 0, 255};

    changed = SDL_MapSurfaceRGBA(window->surface, 101, 102, 103, 104);
    TEST_CHECK(SDL_FillSurfaceRect(window->surface, &changed_rect, changed));
    attempts_before = texture_create_attempts;
    destroys_before = texture_destroy_calls;
    reset_paint_frame();
    sdl_window_refresh(window);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);

    TEST_CHECK(sdl_window_paint(window, NULL) == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 4);
    TEST_CHECK(paint_event_count == 4);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(paint_events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(sprite_attempts == 1);
    TEST_CHECK(successful_sprites == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(last_sprite.texture == old_texture);
    TEST_CHECK(float_equal(last_sprite.left, 7.0f));
    TEST_CHECK(float_equal(last_sprite.top, 9.0f));
    TEST_CHECK(float_equal(last_sprite.right, 12.0f));
    TEST_CHECK(float_equal(last_sprite.bottom, 12.0f));
    TEST_CHECK(float_equal(last_sprite.u0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.v0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.u1, 5.0f / 8.0f));
    TEST_CHECK(float_equal(last_sprite.v1, 3.0f / 4.0f));
    TEST_CHECK(color_equal(last_sprite.tint, white));
    TEST_CHECK(last_stroke.point_count == 4);
    TEST_CHECK(float_equal(last_stroke.points[0].x, 7.0f));
    TEST_CHECK(float_equal(last_stroke.points[0].y, 14.0f));
    TEST_CHECK(float_equal(last_stroke.points[1].x, 7.0f));
    TEST_CHECK(float_equal(last_stroke.points[1].y, 9.0f));
    TEST_CHECK(float_equal(last_stroke.points[2].x, 12.0f));
    TEST_CHECK(float_equal(last_stroke.points[2].y, 9.0f));
    TEST_CHECK(float_equal(last_stroke.points[3].x, 12.0f));
    TEST_CHECK(float_equal(last_stroke.points[3].y, 14.0f));
    TEST_CHECK(color_equal(last_stroke.colors[0], black));
    TEST_CHECK(color_equal(last_stroke.colors[1], green));
    TEST_CHECK(color_equal(last_stroke.colors[2], black));
    TEST_CHECK(color_equal(last_stroke.colors[3], green));
    TEST_CHECK(float_equal(last_stroke.width, 1.0f));
    TEST_CHECK(last_stroke.closed == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);
    fake_renderer.frame_active = 0;

    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 2);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_textures[0].destroy_count == 1);
    texture = &fake_textures[1];
    TEST_CHECK(pixel_equals(texture, 4, 2, 101, 102, 103, 104));
    *refreshed_texture = &texture->handle;

    attempts_before = texture_create_attempts;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    return 0;
}

static int check_atomic_resize_and_retry(sdl_window_t *window,
                                         RendererTexture *old_texture,
                                         RendererTexture **resized_texture)
{
    SDL_Surface *old_surface = window->surface;
    int old_x = window->x;
    int old_y = window->y;
    int old_w = window->w;
    int old_h = window->h;
    int successes_before = texture_create_successes;
    int destroys_before = texture_destroy_calls;
    FakeTexture *texture;

    TEST_CHECK(sdl_window_resize(window, 0, 5) == -1);
    TEST_CHECK(window->surface == old_surface);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == old_w && window->h == old_h);
    TEST_CHECK(texture_create_successes == successes_before);
    TEST_CHECK(texture_destroy_calls == destroys_before);

    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(sdl_window_resize(window, 7, 5) == -1);
    TEST_CHECK(window->surface == old_surface);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == old_w && window->h == old_h);
    TEST_CHECK(texture_create_successes == successes_before);
    TEST_CHECK(texture_destroy_calls == destroys_before);
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 0);

    fail_texture_create_attempt = 0;
    TEST_CHECK(sdl_window_resize(window, 7, 5) == 0);
    TEST_CHECK(window->surface != old_surface);
    TEST_CHECK(window->surface->w == 8 && window->surface->h == 8);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == 7 && window->h == 5);
    TEST_CHECK(texture_create_successes == successes_before + 1);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 1);
    texture = &fake_textures[texture_create_successes - 1];
    TEST_CHECK(texture->desc.width == 8 && texture->desc.height == 8);
    TEST_CHECK(texture->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(texture->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(pixel_equals(texture, 0, 0, 10, 20, 30, 40));
    TEST_CHECK(pixel_equals(texture, 2, 1, 70, 80, 90, 100));
    TEST_CHECK(pixel_equals(texture, 4, 2, 101, 102, 103, 104));
    *resized_texture = &texture->handle;
    return 0;
}

static int check_paint_preconditions_and_sticky_preflight(
    sdl_window_t *window)
{
    sdl_window_t unprepared = *window;

    reset_paint_frame();
    TEST_CHECK(sdl_window_paint(NULL, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(paint_event_count == 0);

    unprepared.renderer = NULL;
    unprepared.texture = NULL;
    reset_paint_frame();
    TEST_CHECK(sdl_window_paint(&unprepared, NULL)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(paint_event_count == 0);

    reset_paint_frame();
    fake_sdl_renderer.frontend = &foreign_renderer;
    TEST_CHECK(sdl_window_paint(window, NULL)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(paint_event_count == 0);

    reset_paint_frame();
    fake_renderer.frame_active = 0;
    TEST_CHECK(sdl_window_paint(window, NULL)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(paint_event_count == 0);

    reset_paint_frame();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(sdl_window_paint(window, NULL)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 0);
    TEST_CHECK(paint_event_count == 0);
    return 0;
}

static int check_paint_failures_and_flush(sdl_window_t *window)
{
    const RendererColor background = {35, 69, 103, 137};

    reset_paint_frame();
    blend_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(sdl_window_paint(window, &background)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 1);
    TEST_CHECK(paint_event_count == 1);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(sprite_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    reset_paint_frame();
    fill_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(sdl_window_paint(window, &background)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 2);
    TEST_CHECK(paint_event_count == 2);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(sprite_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);

    reset_paint_frame();
    sprite_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(sdl_window_paint(window, NULL)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 2);
    TEST_CHECK(paint_event_count == 2);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_SPRITE);
    TEST_CHECK(successful_sprites == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_paint_frame();
    sprite_result = RENDERER_STATUS_BACKEND_ERROR;
    flush_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(sdl_window_paint(window, &background)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 4);
    TEST_CHECK(paint_event_count == 4);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(paint_events[2] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(successful_sprites == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    reset_paint_frame();
    stroke_result = RENDERER_STATUS_OUT_OF_MEMORY;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(sdl_window_paint(window, NULL)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 4);
    TEST_CHECK(paint_event_count == 4);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(paint_events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_sprites == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);

    reset_paint_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(sdl_window_paint(window, &background)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 5);
    TEST_CHECK(paint_event_count == 5);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(successful_sprites == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    flush_result = RENDERER_STATUS_OK;
    TEST_CHECK(sdl_window_paint(window, &background)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(tracked_operation_results == 5);
    TEST_CHECK(paint_event_count == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(sprite_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    return 0;
}

static int check_move_paint_and_exact_cleanup(sdl_window_t *window,
                                              RendererTexture *texture)
{
    const RendererColor background = {35, 69, 103, 137};
    int destroys_before;

    sdl_window_move(window, 20, 30);
    reset_paint_frame();
    TEST_CHECK(sdl_window_paint(window, &background) == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(tracked_operation_results == 5);
    TEST_CHECK(paint_event_count == 5);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(paint_events[2] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(paint_events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(float_equal(last_fill.x, 20.0f));
    TEST_CHECK(float_equal(last_fill.y, 30.0f));
    TEST_CHECK(float_equal(last_fill.width, 7.0f));
    TEST_CHECK(float_equal(last_fill.height, 7.0f));
    TEST_CHECK(color_equal(last_fill.color, background));
    TEST_CHECK(sprite_attempts == 1);
    TEST_CHECK(successful_sprites == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(last_sprite.texture == texture);
    TEST_CHECK(float_equal(last_sprite.left, 20.0f));
    TEST_CHECK(float_equal(last_sprite.top, 30.0f));
    TEST_CHECK(float_equal(last_sprite.right, 27.0f));
    TEST_CHECK(float_equal(last_sprite.bottom, 35.0f));
    TEST_CHECK(float_equal(last_sprite.u1, 7.0f / 8.0f));
    TEST_CHECK(float_equal(last_sprite.v1, 5.0f / 8.0f));
    TEST_CHECK(float_equal(last_stroke.points[0].x, 20.0f));
    TEST_CHECK(float_equal(last_stroke.points[0].y, 37.0f));
    TEST_CHECK(float_equal(last_stroke.points[1].x, 20.0f));
    TEST_CHECK(float_equal(last_stroke.points[1].y, 30.0f));
    TEST_CHECK(float_equal(last_stroke.points[2].x, 27.0f));
    TEST_CHECK(float_equal(last_stroke.points[2].y, 30.0f));
    TEST_CHECK(float_equal(last_stroke.points[3].x, 27.0f));
    TEST_CHECK(float_equal(last_stroke.points[3].y, 37.0f));
    fake_renderer.frame_active = 0;

    destroys_before = texture_destroy_calls;
    sdl_window_destroy(window);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_texture_from_handle(texture)->destroy_count == 1);
    TEST_CHECK(window->surface == NULL);
    TEST_CHECK(window->x == 0 && window->y == 0);
    TEST_CHECK(window->w == 0 && window->h == 0);

    sdl_window_destroy(window);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(resource_calls_during_frame == 0);
    TEST_CHECK(texture_update_calls == 0);
    return 0;
}

static void release_fake_pixels(void)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        free(fake_textures[index].pixels);
        fake_textures[index].pixels = NULL;
    }
}

int main(void)
{
    sdl_window_t window;
    RendererTexture *initial_texture;
    RendererTexture *refreshed_texture;
    RendererTexture *resized_texture;
    int result = 1;

    if (check_initial_prepare(&window, &initial_texture) != 0)
        goto cleanup;
    if (check_dirty_refresh_and_semantic_paint(
            &window, initial_texture, &refreshed_texture) != 0) {
        goto cleanup;
    }
    if (check_atomic_resize_and_retry(
            &window, refreshed_texture, &resized_texture) != 0) {
        goto cleanup;
    }
    if (check_paint_preconditions_and_sticky_preflight(&window) != 0)
        goto cleanup;
    if (check_paint_failures_and_flush(&window) != 0)
        goto cleanup;
    if (check_move_paint_and_exact_cleanup(
            &window, resized_texture) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (result != 0 && window.surface != NULL) {
        fake_renderer.frame_active = 0;
        sdl_window_destroy(&window);
    }
    release_fake_pixels();
    return result;
}
