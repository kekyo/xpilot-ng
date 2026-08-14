#include "test_helpers.h"

#include "renderer.h"
#include "sdlmeta.h"
#include "sdlrenderer.h"

#include <SDL.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define MAX_EVENTS 4

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
} FakeFill;

typedef enum PaintEvent {
    PAINT_EVENT_BLEND,
    PAINT_EVENT_FILL,
    PAINT_EVENT_FLUSH
} PaintEvent;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {&fake_renderer, RENDERER_STATUS_OK};
static FakeFill last_fill;
static PaintEvent events[MAX_EVENTS];
static int event_count;
static int operation_result_pending;
static int preflight_attempts;
static int blend_attempts;
static int fill_attempts;
static int successful_fills;
static int flush_attempts;
static RendererStatus fill_result;
static RendererStatus flush_result;

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static void record_event(PaintEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count++] = event;
}

static void reset_frame(void)
{
    memset(&last_fill, 0, sizeof(last_fill));
    memset(events, 0, sizeof(events));
    event_count = 0;
    operation_result_pending = 0;
    preflight_attempts = 0;
    blend_attempts = 0;
    fill_attempts = 0;
    successful_fills = 0;
    flush_attempts = 0;
    fill_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color = {
        (uint8_t)(rgba >> 24),
        (uint8_t)(rgba >> 16),
        (uint8_t)(rgba >> 8),
        (uint8_t)rgba
    };

    return color;
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
    } else {
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

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    blend_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_fill_rect(Renderer *renderer,
                                  float x, float y,
                                  float width, float height,
                                  RendererColor color)
{
    fill_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_FILL);
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

RendererStatus Sdl_renderer_flush(SdlRenderer *renderer)
{
    flush_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer
        || renderer->frontend == NULL
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

static int check_successful_fill(float x, float y,
                                 float width, float height,
                                 uint32_t rgba)
{
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(last_fill.x == x);
    TEST_CHECK(last_fill.y == y);
    TEST_CHECK(last_fill.width == width);
    TEST_CHECK(last_fill.height == height);
    TEST_CHECK(color_equal(
        last_fill.color, Renderer_color_from_rgba32(rgba)));
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return 0;
}

static int check_player_list_background(void)
{
    SDL_Rect bounds = {13, 19, 21, 27};

    reset_frame();
    Sdl_meta_test_paint_player_list(bounds);
    return check_successful_fill(13.0f, 19.0f, 11.0f, 27.0f,
                                 0x000070ff);
}

static int check_narrow_player_list_is_empty(void)
{
    SDL_Rect bounds = {13, 19, 10, 27};

    reset_frame();
    Sdl_meta_test_paint_player_list(bounds);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return 0;
}

static int check_unselected_row_background(void)
{
    SDL_Rect bounds = {13, 19, 21, 27};

    reset_frame();
    Sdl_meta_test_paint_row(bounds, 0x23456789, false);
    return check_successful_fill(13.0f, 19.0f, 21.0f, 27.0f,
                                 0x23456789);
}

static int check_selected_row_background(void)
{
    SDL_Rect bounds = {13, 19, 21, 27};

    reset_frame();
    Sdl_meta_test_paint_row(bounds, 0x23456789, true);
    return check_successful_fill(13.0f, 19.0f, 21.0f, 27.0f,
                                 0x009000ff);
}

static int check_fill_failure_is_sticky_without_flush(void)
{
    SDL_Rect bounds = {13, 19, 21, 27};

    reset_frame();
    fill_result = RENDERER_STATUS_BACKEND_ERROR;
    Sdl_meta_test_paint_row(bounds, 0x23456789, false);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_flush_failure_blocks_same_frame_repaint(void)
{
    SDL_Rect bounds = {13, 19, 21, 27};

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Sdl_meta_test_paint_row(bounds, 0x23456789, false);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    flush_result = RENDERER_STATUS_OK;
    Sdl_meta_test_paint_row(bounds, 0x23456789, false);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

int main(void)
{
    if (check_player_list_background() != 0)
        return 1;
    if (check_narrow_player_list_is_empty() != 0)
        return 1;
    if (check_unselected_row_background() != 0)
        return 1;
    if (check_selected_row_background() != 0)
        return 1;
    if (check_fill_failure_is_sticky_without_flush() != 0)
        return 1;
    return check_flush_failure_blocks_same_frame_repaint();
}
