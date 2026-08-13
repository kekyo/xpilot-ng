#include "test_helpers.h"

#include "glwidgets.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <GL/gl.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 8
#define MAX_DRAWS 8
#define MAX_FILLS 8

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef enum DrawEvent {
    DRAW_EVENT_BLEND,
    DRAW_EVENT_TRIANGLES,
    DRAW_EVENT_FILL_RECT,
    DRAW_EVENT_FLUSH
} DrawEvent;

typedef struct FakeDraw {
    RendererTexture *texture;
    RendererVertex2D vertices[6];
    size_t vertex_count;
} FakeDraw;

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
} FakeFill;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer,
    RENDERER_STATUS_OK
};
static DrawEvent events[MAX_EVENTS];
static FakeDraw draws[MAX_DRAWS];
static FakeFill fills[MAX_FILLS];
static int event_count;
static int call_order;
static int preflight_attempts;
static int preflight_order;
static int blend_order;
static int draw_order;
static int fill_order;
static int flush_order;
static int operation_result_pending;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int fill_attempts;
static int successful_fills;
static int flush_attempts;
static int legacy_begin_calls;
static int legacy_vertex_calls;
static int legacy_color_calls;
static RendererStatus blend_result;
static RendererStatus draw_result;
static RendererStatus fill_result;
static RendererStatus flush_result;

long loopsSlow;
Uint32 redRGBA = 0xff0000ff;
Uint32 greenRGBA = 0x00ff00ff;

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, RendererColor color)
{
    return vertex->x == x && vertex->y == y
        && vertex->u == 0.0f && vertex->v == 0.0f
        && color_equal(vertex->color, color);
}

static void record_event(DrawEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count++] = event;
}

static void reset_frame(void)
{
    memset(events, 0, sizeof(events));
    memset(draws, 0, sizeof(draws));
    memset(fills, 0, sizeof(fills));
    event_count = 0;
    call_order = 0;
    preflight_attempts = 0;
    preflight_order = 0;
    blend_order = 0;
    draw_order = 0;
    fill_order = 0;
    flush_order = 0;
    operation_result_pending = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    fill_attempts = 0;
    successful_fills = 0;
    flush_attempts = 0;
    legacy_begin_calls = 0;
    legacy_vertex_calls = 0;
    legacy_color_calls = 0;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
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
        if (preflight_order == 0)
            preflight_order = ++call_order;
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
    if (blend_order == 0)
        blend_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return blend_result;
}

RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    FakeDraw *draw;

    draw_attempts++;
    if (draw_order == 0)
        draw_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_TRIANGLES);
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture != NULL || vertices == NULL
        || (vertex_count != 3 && vertex_count != 6)) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    if (successful_draws >= MAX_DRAWS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    draw = &draws[successful_draws++];
    draw->texture = texture;
    draw->vertex_count = vertex_count;
    memcpy(draw->vertices, vertices, vertex_count * sizeof(*vertices));
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_fill_rect(Renderer *renderer,
                                  float x, float y,
                                  float width, float height,
                                  RendererColor color)
{
    FakeFill *fill;

    fill_attempts++;
    if (fill_order == 0)
        fill_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_FILL_RECT);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (fill_result != RENDERER_STATUS_OK)
        return fill_result;
    if (successful_fills >= MAX_FILLS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    fill = &fills[successful_fills++];
    fill->x = x;
    fill->y = y;
    fill->width = width;
    fill->height = height;
    fill->color = color;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
{
    flush_attempts++;
    if (flush_order == 0)
        flush_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer
        || renderer->frontend == NULL
        || !renderer->frontend->frame_active) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return flush_result;
}

void set_alphacolor(Uint32 color)
{
    (void)color;
    legacy_color_calls++;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green,
                          GLubyte blue, GLubyte alpha)
{
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    legacy_color_calls++;
}

void GLAPIENTRY glBegin(GLenum mode)
{
    (void)mode;
    legacy_begin_calls++;
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
{
    (void)x;
    (void)y;
    legacy_vertex_calls++;
}

void GLAPIENTRY glEnd(void)
{
}

static void destroy_widget(GLWidget *widget)
{
    GLWidget *child;
    GLWidget *next;

    if (widget == NULL)
        return;
    child = widget->children;
    while (child != NULL) {
        next = child->next;
        child->next = NULL;
        destroy_widget(child);
        child = next;
    }
    free(widget->wid_info);
    free(widget);
}

static int check_successful_draw(RendererColor expected_color,
                                 const float expected_points[3][2])
{
    int index;

    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].vertex_count == 3);
    for (index = 0; index < 3; index++) {
        TEST_CHECK(vertex_equal(
            &draws[0].vertices[index],
            expected_points[index][0], expected_points[index][1],
            expected_color));
    }
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_successful_gradient_quad(
    float x, float y, float width, float height,
    RendererColor top_color, RendererColor bottom_color)
{
    const float points[6][2] = {
        {x, y},
        {x + width, y},
        {x + width, y + height},
        {x, y},
        {x + width, y + height},
        {x, y + height}
    };
    const RendererColor colors[6] = {
        top_color, top_color, bottom_color,
        top_color, bottom_color, bottom_color
    };
    int index;

    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].vertex_count == 6);
    for (index = 0; index < 6; index++) {
        TEST_CHECK(vertex_equal(
            &draws[0].vertices[index],
            points[index][0], points[index][1], colors[index]));
    }
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_successful_fill(float expected_x, float expected_y,
                                 float expected_width,
                                 float expected_height,
                                 RendererColor expected_color)
{
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(fills[0].x == expected_x);
    TEST_CHECK(fills[0].y == expected_y);
    TEST_CHECK(fills[0].width == expected_width);
    TEST_CHECK(fills[0].height == expected_height);
    TEST_CHECK(color_equal(fills[0].color, expected_color));
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_direction_geometry(void)
{
    static const ArrowWidget_dir_t directions[4] = {
        RIGHTARROW, UPARROW, LEFTARROW, DOWNARROW
    };
    static const float points[4][3][2] = {
        {{11.0f, 17.0f}, {11.0f, 24.0f}, {20.0f, 20.0f}},
        {{15.0f, 17.0f}, {11.0f, 24.0f}, {20.0f, 24.0f}},
        {{20.0f, 17.0f}, {11.0f, 20.0f}, {20.0f, 24.0f}},
        {{11.0f, 17.0f}, {15.0f, 24.0f}, {20.0f, 17.0f}}
    };
    const RendererColor normal_color = {0xff, 0x00, 0x00, 0xff};
    SDL_Rect bounds = {11, 17, 9, 7};
    int index;

    for (index = 0; index < 4; index++) {
        GLWidget *widget = Init_ArrowWidget(
            directions[index], 2, 3, NULL, NULL);

        TEST_CHECK(widget != NULL);
        SetBounds_GLWidget(widget, &bounds);
        reset_frame();
        widget->Draw(widget);
        TEST_CHECK(check_successful_draw(normal_color, points[index]) == 0);
        destroy_widget(widget);
    }
    return 0;
}

static void count_action(void *data)
{
    int *count = data;

    (*count)++;
}

static int check_slide_geometry_colors_and_state(void)
{
    const RendererColor normal_top = {0xff, 0x00, 0x00, 0xff};
    const RendererColor normal_bottom = {0xff, 0x00, 0x00, 0x77};
    const RendererColor sliding_top = {0x00, 0xff, 0x00, 0xff};
    const RendererColor sliding_bottom = {0x00, 0xff, 0x00, 0x77};
    const RendererColor locked_top = {0x33, 0x33, 0x33, 0xff};
    const RendererColor locked_bottom = {0x33, 0x33, 0x33, 0x77};
    SDL_Rect bounds = {11, 17, 9, 7};
    GLWidget *widget;
    SlideWidget *info;
    int release_count = 0;

    widget = Init_SlideWidget(
        false, NULL, NULL, count_action, &release_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(!info->locked);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        normal_top, normal_bottom) == 0);

    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(release_count == 0);

    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(info->sliding);
    TEST_CHECK(release_count == 0);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        sliding_top, sliding_bottom) == 0);

    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(info->sliding);
    TEST_CHECK(release_count == 0);
    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(release_count == 1);
    destroy_widget(widget);

    widget = Init_SlideWidget(
        true, NULL, NULL, count_action, &release_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(info->locked);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        locked_top, locked_bottom) == 0);
    TEST_CHECK(release_count == 1);

    destroy_widget(widget);
    return 0;
}

static int check_button_geometry_colors_and_state(void)
{
    Uint32 normal_rgba = 0x12345678;
    Uint32 pressed_rgba = 0x90abcdef;
    const RendererColor normal_color = {0x12, 0x34, 0x56, 0x78};
    const RendererColor pressed_color = {0x90, 0xab, 0xcd, 0xef};
    const RendererColor default_normal_color = {0x00, 0xff, 0x00, 0xff};
    const RendererColor default_pressed_color = {0xff, 0x00, 0x00, 0xff};
    SDL_Rect bounds = {11, 17, 9, 7};
    GLWidget *widget;
    ButtonWidget *info;
    int action_count = 0;

    widget = Init_ButtonWidget(&normal_rgba, &pressed_rgba, 3,
                               count_action, &action_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    loopsSlow = 100;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, normal_color) == 0);
    TEST_CHECK(!info->pressed);
    TEST_CHECK(action_count == 0);

    loopsSlow = 200;
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(info->pressed);
    TEST_CHECK(info->press_time == 200);
    TEST_CHECK(action_count == 1);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 1);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, pressed_color) == 0);
    TEST_CHECK(info->pressed);
    TEST_CHECK(action_count == 1);

    loopsSlow = 203;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, pressed_color) == 0);
    TEST_CHECK(!info->pressed);
    TEST_CHECK(action_count == 1);

    loopsSlow = 204;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, normal_color) == 0);
    TEST_CHECK(action_count == 1);
    destroy_widget(widget);

    widget = Init_ButtonWidget(NULL, NULL, 2,
                               count_action, &action_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    loopsSlow = 300;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, default_normal_color) == 0);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 2);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, default_pressed_color) == 0);

    destroy_widget(widget);
    return 0;
}

static int check_scrollbar_geometry_and_color(void)
{
    const RendererColor background_color = {0x00, 0x00, 0x00, 0x44};
    SDL_Rect bounds = {13, 19, 21, 27};
    GLWidget *widget = Init_ScrollbarWidget(
        false, 0.25f, 0.5f, SB_VERTICAL, NULL, NULL);

    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        13.0f, 19.0f, 21.0f, 27.0f, background_color) == 0);

    destroy_widget(widget);
    return 0;
}

static int check_state_colors(void)
{
    const float points[3][2] = {
        {0.0f, 0.0f}, {0.0f, 8.0f}, {10.0f, 4.0f}
    };
    const RendererColor normal_color = {0xff, 0x00, 0x00, 0xff};
    const RendererColor press_color = {0x00, 0xff, 0x00, 0xff};
    const RendererColor tap_color = {0xff, 0xff, 0xff, 0xff};
    const RendererColor lock_color = {0x88, 0x00, 0x00, 0x88};
    GLWidget *widget;
    ArrowWidget *info;
    int action_count = 0;

    widget = Init_ArrowWidget(RIGHTARROW, 10, 8,
                              count_action, &action_count);
    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(normal_color, points) == 0);
    TEST_CHECK(action_count == 0);

    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(press_color, points) == 0);
    TEST_CHECK(action_count == 1);
    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);

    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 2);
    TEST_CHECK(info->tap);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(tap_color, points) == 0);
    TEST_CHECK(!info->tap);
    TEST_CHECK(action_count == 2);

    info->press = true;
    info->tap = true;
    info->locked = true;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(lock_color, points) == 0);
    TEST_CHECK(action_count == 2);

    destroy_widget(widget);
    return 0;
}

static int check_triangle_failure_short_circuit_for_widget(GLWidget *widget)
{
    int event_count_after_failure;
    int blend_attempts_after_failure;
    int draw_attempts_after_failure;
    int successful_draws_after_failure;
    int flush_attempts_after_failure;

    TEST_CHECK(widget != NULL);

    reset_frame();
    blend_result = RENDERER_STATUS_INVALID_ARGUMENT;
    widget->Draw(widget);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    draw_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    event_count_after_failure = event_count;
    blend_attempts_after_failure = blend_attempts;
    draw_attempts_after_failure = draw_attempts;
    successful_draws_after_failure = successful_draws;
    flush_attempts_after_failure = flush_attempts;
    flush_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == event_count_after_failure);
    TEST_CHECK(blend_attempts == blend_attempts_after_failure);
    TEST_CHECK(draw_attempts == draw_attempts_after_failure);
    TEST_CHECK(successful_draws == successful_draws_after_failure);
    TEST_CHECK(flush_attempts == flush_attempts_after_failure);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    return 0;
}

static int check_failure_short_circuit(void)
{
    GLWidget *widget = Init_ArrowWidget(
        RIGHTARROW, 10, 8, NULL, NULL);

    TEST_CHECK(widget != NULL);
    TEST_CHECK(check_triangle_failure_short_circuit_for_widget(widget) == 0);
    destroy_widget(widget);
    return 0;
}

static int check_slide_failure_short_circuit(void)
{
    GLWidget *widget = Init_SlideWidget(
        false, NULL, NULL, NULL, NULL);

    TEST_CHECK(widget != NULL);
    TEST_CHECK(check_triangle_failure_short_circuit_for_widget(widget) == 0);
    destroy_widget(widget);
    return 0;
}

static int check_fill_failure_short_circuit_for_widget(GLWidget *widget)
{
    TEST_CHECK(widget != NULL);

    reset_frame();
    blend_result = RENDERER_STATUS_INVALID_ARGUMENT;
    widget->Draw(widget);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    reset_frame();
    fill_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    return 0;
}

static int check_fill_failure_short_circuit(void)
{
    Uint32 normal_rgba = 0x12345678;
    Uint32 pressed_rgba = 0x90abcdef;
    SDL_Rect bounds = {7, 9, 11, 13};
    GLWidget *button = Init_ButtonWidget(
        &normal_rgba, &pressed_rgba, 3, NULL, NULL);
    GLWidget *scrollbar = Init_ScrollbarWidget(
        false, 0.25f, 0.5f, SB_HORISONTAL, NULL, NULL);

    TEST_CHECK(button != NULL);
    TEST_CHECK(scrollbar != NULL);
    SetBounds_GLWidget(button, &bounds);
    SetBounds_GLWidget(scrollbar, &bounds);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(button) == 0);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(scrollbar) == 0);

    destroy_widget(button);
    destroy_widget(scrollbar);
    return 0;
}

static int check_tap_is_consumed_when_flush_fails(void)
{
    GLWidget *widget = Init_ArrowWidget(
        RIGHTARROW, 10, 8, NULL, NULL);
    ArrowWidget *info;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    info->tap = true;
    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(!info->tap);

    destroy_widget(widget);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_direction_geometry() == 0);
    TEST_CHECK(check_state_colors() == 0);
    TEST_CHECK(check_failure_short_circuit() == 0);
    TEST_CHECK(check_tap_is_consumed_when_flush_fails() == 0);
    TEST_CHECK(check_slide_geometry_colors_and_state() == 0);
    TEST_CHECK(check_slide_failure_short_circuit() == 0);
    TEST_CHECK(check_button_geometry_colors_and_state() == 0);
    TEST_CHECK(check_scrollbar_geometry_and_color() == 0);
    TEST_CHECK(check_fill_failure_short_circuit() == 0);
    return 0;
}
