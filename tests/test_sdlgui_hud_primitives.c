#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"
#include "text.h"

#include <GL/gl.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define MAX_EVENTS 16
#define MAX_STROKES 6
#define MAX_STROKE_POINTS 4

struct Renderer {
    int frame_active;
    int transform_token;
    int scissor_token;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef enum PaintEvent {
    PAINT_EVENT_BLEND,
    PAINT_EVENT_FILL,
    PAINT_EVENT_STROKE,
    PAINT_EVENT_FLUSH,
    PAINT_EVENT_TEXT
} PaintEvent;

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
    int transform_token;
    int scissor_token;
} FakeFill;

typedef struct FakeStroke {
    RendererPoint2D points[MAX_STROKE_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    int transform_token;
    int scissor_token;
} FakeStroke;

typedef struct FakeText {
    string_tex_t *texture;
    int color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    bool on_hud;
} FakeText;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer,
    RENDERER_STATUS_OK
};
static PaintEvent events[MAX_EVENTS];
static FakeFill last_fill;
static FakeStroke strokes[MAX_STROKES];
static FakeText last_text;
static int event_count;
static int operation_result_pending;
static int preflight_attempts;
static int blend_attempts;
static int fill_attempts;
static int successful_fills;
static int stroke_attempts;
static int successful_strokes;
static int flush_attempts;
static int transform_attempts;
static int scissor_attempts;
static int text_attempts;
static int legacy_begin_calls;
static int legacy_vertex_calls;
static int legacy_end_calls;
static int legacy_color_calls;
static RendererStatus blend_result;
static RendererStatus fill_result;
static int stroke_failure_attempt;
static RendererStatus stroke_failure_result;
static RendererStatus flush_result;
static RendererStatus text_result;

static setup_t fake_setup = {
    .frames_per_second = 60
};

setup_t *Setup = &fake_setup;
font_data gamefont;
unsigned draw_width;
unsigned draw_height;
short ext_view_width;
short ext_view_height;
double fuelSum;
double fuelMax;
double displayedPower;
double displayedTurnspeed;
double controlTime;
double timePerFrame;
int packet_size;
int packet_loss;
int packet_drop;
int packet_lag;
short thrusttime;
short thrusttimemax;
short shieldtime;
short shieldtimemax;
short phasingtime;
short phasingtimemax;
short destruct;
short shutdown_count;
short shutdown_delay;

extern Uint32 meterBorderColorRGBA;
extern Uint32 fuelMeterColorRGBA;
extern Uint32 powerMeterColorRGBA;
extern Uint32 turnSpeedMeterColorRGBA;
extern Uint32 packetSizeMeterColorRGBA;
extern Uint32 packetLossMeterColorRGBA;
extern Uint32 packetDropMeterColorRGBA;
extern Uint32 packetLagMeterColorRGBA;
extern Uint32 temporaryMeterColorRGBA;
extern Uint32 selectionColorRGBA;

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int check_stroke(
    int stroke_index, const float expected_points[][2],
    size_t expected_point_count, uint32_t rgba, int closed)
{
    const FakeStroke *stroke;
    size_t point_index;

    TEST_CHECK(stroke_index >= 0);
    TEST_CHECK(stroke_index < successful_strokes);
    stroke = &strokes[stroke_index];
    TEST_CHECK(stroke->point_count == expected_point_count);
    TEST_CHECK(stroke->width == 1.0f);
    TEST_CHECK(color_equal(
        stroke->color, Renderer_color_from_rgba32(rgba)));
    TEST_CHECK(stroke->closed == closed);
    TEST_CHECK(stroke->transform_token == 71);
    TEST_CHECK(stroke->scissor_token == 83);
    for (point_index = 0; point_index < expected_point_count;
         point_index++) {
        TEST_CHECK(stroke->points[point_index].x
                   == expected_points[point_index][0]);
        TEST_CHECK(stroke->points[point_index].y
                   == expected_points[point_index][1]);
    }
    return 0;
}

static void record_event(PaintEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count++] = event;
}

static void reset_meter_options(void)
{
    meterBorderColorRGBA = 0;
    fuelMeterColorRGBA = 0;
    powerMeterColorRGBA = 0;
    turnSpeedMeterColorRGBA = 0;
    packetSizeMeterColorRGBA = 0;
    packetLossMeterColorRGBA = 0;
    packetDropMeterColorRGBA = 0;
    packetLagMeterColorRGBA = 0;
    temporaryMeterColorRGBA = 0;
}

static void reset_frame(void)
{
    memset(events, 0, sizeof(events));
    memset(&last_fill, 0, sizeof(last_fill));
    memset(strokes, 0, sizeof(strokes));
    memset(&last_text, 0, sizeof(last_text));
    memset(&gamefont, 0, sizeof(gamefont));
    memset(&meter_texs[0], 0,
           MAX_METERS * sizeof(meter_texs[0]));
    event_count = 0;
    operation_result_pending = 0;
    preflight_attempts = 0;
    blend_attempts = 0;
    fill_attempts = 0;
    successful_fills = 0;
    stroke_attempts = 0;
    successful_strokes = 0;
    flush_attempts = 0;
    transform_attempts = 0;
    scissor_attempts = 0;
    text_attempts = 0;
    legacy_begin_calls = 0;
    legacy_vertex_calls = 0;
    legacy_end_calls = 0;
    legacy_color_calls = 0;
    blend_result = RENDERER_STATUS_OK;
    fill_result = RENDERER_STATUS_OK;
    stroke_failure_attempt = 0;
    stroke_failure_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    text_result = RENDERER_STATUS_OK;
    fake_renderer.frame_active = 1;
    fake_renderer.transform_token = 71;
    fake_renderer.scissor_token = 83;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    draw_width = 200;
    draw_height = 120;
    ext_view_width = 200;
    ext_view_height = 120;
    gamefont.requested_height = 12;
    fuelSum = 25.0;
    fuelMax = 100.0;
    displayedPower = 0.0;
    displayedTurnspeed = 0.0;
    controlTime = 0.0;
    timePerFrame = 1.0 / 60.0;
    packet_size = 0;
    packet_loss = 0;
    packet_drop = 0;
    packet_lag = 0;
    thrusttime = -1;
    thrusttimemax = 0;
    shieldtime = -1;
    shieldtimemax = 0;
    phasingtime = -1;
    phasingtimemax = 0;
    destruct = 0;
    shutdown_count = -1;
    shutdown_delay = 0;
    reset_meter_options();
    fuelMeterColorRGBA = 0x12345678;
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
    return blend_result;
}

RendererStatus Renderer_set_transform_2d(
    Renderer *renderer, RendererTransform2D transform)
{
    (void)renderer;
    (void)transform;
    transform_attempts++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_scissor(
    Renderer *renderer, const RendererRect *scissor)
{
    (void)renderer;
    (void)scissor;
    scissor_attempts++;
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
    last_fill.transform_token = renderer->transform_token;
    last_fill.scissor_token = renderer->scissor_token;
    successful_fills++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_stroke_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed)
{
    FakeStroke *stroke;

    stroke_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count < 2
        || point_count > MAX_STROKE_POINTS || width <= 0.0f) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (stroke_failure_attempt == stroke_attempts
        && stroke_failure_result != RENDERER_STATUS_OK) {
        return stroke_failure_result;
    }
    if (successful_strokes >= MAX_STROKES)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    stroke = &strokes[successful_strokes++];
    memcpy(stroke->points, points,
           point_count * sizeof(stroke->points[0]));
    stroke->point_count = point_count;
    stroke->width = width;
    stroke->color = color;
    stroke->closed = closed;
    stroke->transform_token = renderer->transform_token;
    stroke->scissor_token = renderer->scissor_token;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
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

bool render_text(font_data *font, const char *text, string_tex_t *cache)
{
    (void)font;
    (void)text;
    (void)cache;
    return false;
}

RendererStatus disp_text(string_tex_t *texture, int color,
                         int horizontal_alignment,
                         int vertical_alignment, int x, int y,
                         bool on_hud)
{
    text_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_TEXT);
    last_text.texture = texture;
    last_text.color = color;
    last_text.horizontal_alignment = horizontal_alignment;
    last_text.vertical_alignment = vertical_alignment;
    last_text.x = x;
    last_text.y = y;
    last_text.on_hud = on_hud;
    return text_result;
}

void GLAPIENTRY glEnable(GLenum capability)
{
    (void)capability;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    (void)capability;
}

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
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
    legacy_end_calls++;
}

static int check_no_legacy_primitives(void)
{
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_end_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_null_selection_is_successful_noop(void)
{
    RendererStatus result;

    reset_frame();
    select_bounds = NULL;
    selectionColorRGBA = UINT32_C(0x89abcdef);

    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_selection_outline_is_semantic(void)
{
    static const float expected_points[][2] = {
        {12.0f, 23.0f},
        {46.0f, 23.0f},
        {46.0f, 68.0f},
        {12.0f, 68.0f}
    };
    irec_t bounds = {12, 23, 34, 45};
    RendererStatus result;

    reset_frame();
    select_bounds = &bounds;
    selectionColorRGBA = UINT32_C(0x89abcdef);

    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(check_stroke(
        0, expected_points, 4, UINT32_C(0x89abcdef), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_selection_requires_active_frame(void)
{
    irec_t bounds = {12, 23, 34, 45};
    RendererStatus result;

    reset_frame();
    select_bounds = &bounds;
    selectionColorRGBA = UINT32_C(0x89abcdef);
    fake_renderer.frame_active = 0;

    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_selection_stroke_failure_is_sticky(void)
{
    irec_t bounds = {12, 23, 34, 45};
    RendererStatus result;

    reset_frame();
    select_bounds = &bounds;
    selectionColorRGBA = UINT32_C(0x89abcdef);
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_selection_flush_failure_is_sticky(void)
{
    irec_t bounds = {12, 23, 34, 45};
    RendererStatus result;

    reset_frame();
    select_bounds = &bounds;
    selectionColorRGBA = UINT32_C(0x89abcdef);
    flush_result = RENDERER_STATUS_BACKEND_ERROR;

    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    if (check_no_legacy_primitives() != 0)
        return 1;

    flush_result = RENDERER_STATUS_OUT_OF_MEMORY;
    result = Paint_select();

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return check_no_legacy_primitives();
}

static int check_fuel_meter_fill_uses_inherited_hud_state(void)
{
    reset_frame();

    Paint_meters();

    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(last_fill.x == 130.0f);
    TEST_CHECK(last_fill.y == 36.0f);
    TEST_CHECK(last_fill.width == 15.0f);
    TEST_CHECK(last_fill.height == 9.0f);
    TEST_CHECK(color_equal(
        last_fill.color,
        Renderer_color_from_rgba32(0x12345678)));
    TEST_CHECK(last_fill.transform_token == 71);
    TEST_CHECK(last_fill.scissor_token == 83);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_negative_and_empty_meter_fills(void)
{
    reset_frame();
    fuelSum = -25.0;

    Paint_meters();

    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 115.0f);
    TEST_CHECK(last_fill.y == 36.0f);
    TEST_CHECK(last_fill.width == 15.0f);
    TEST_CHECK(last_fill.height == 9.0f);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    fuelSum = 0.0;

    Paint_meters();

    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_extreme_meter_value_avoids_signed_overflow(void)
{
    int64_t signed_width = INT64_C(60) * INT_MIN;

    reset_frame();
    fuelSum = (double)INT_MIN;
    fuelMax = 1.0;

    Paint_meters();

    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == (float)(INT64_C(130) + signed_width));
    TEST_CHECK(last_fill.width == (float)-signed_width);
    TEST_CHECK(last_fill.height == 9.0f);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_meter_fill_flushes_before_its_label(void)
{
    reset_frame();
    meter_texs[0].cache = (TextRendererCache *)&fake_renderer;
    meter_texs[0].text = "Fuel";

    Paint_meters();

    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(events[3] == PAINT_EVENT_TEXT);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_meter_border_and_ticks_are_semantic(void)
{
    static const float border_points[][2] = {
        {130.0f, 36.0f},
        {130.0f, 46.0f},
        {190.0f, 46.0f},
        {190.0f, 36.0f}
    };
    static const float tick_points[][2][2] = {
        {{130.0f, 32.0f}, {130.0f, 50.0f}},
        {{190.0f, 32.0f}, {190.0f, 50.0f}},
        {{160.0f, 33.0f}, {160.0f, 49.0f}},
        {{145.0f, 35.0f}, {145.0f, 47.0f}},
        {{175.0f, 35.0f}, {175.0f, 47.0f}}
    };
    int tick_index;

    reset_frame();
    meterBorderColorRGBA = 0x89abcdef;
    meter_texs[0].cache = (TextRendererCache *)&fake_renderer;
    meter_texs[0].text = "Fuel";

    Paint_meters();

    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 10);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    for (tick_index = 0; tick_index < 6; tick_index++)
        TEST_CHECK(events[2 + tick_index] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[8] == PAINT_EVENT_FLUSH);
    TEST_CHECK(events[9] == PAINT_EVENT_TEXT);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 130.0f);
    TEST_CHECK(last_fill.y == 36.0f);
    TEST_CHECK(last_fill.width == 15.0f);
    TEST_CHECK(last_fill.height == 9.0f);
    TEST_CHECK(color_equal(
        last_fill.color,
        Renderer_color_from_rgba32(0x12345678)));
    TEST_CHECK(stroke_attempts == 6);
    TEST_CHECK(successful_strokes == 6);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 1);
    if (check_stroke(0, border_points, 4, 0x89abcdef, 1) != 0)
        return 1;
    for (tick_index = 0; tick_index < 5; tick_index++) {
        if (check_stroke(
                tick_index + 1, tick_points[tick_index], 2,
                0x89abcdef, 0) != 0) {
            return 1;
        }
    }
    TEST_CHECK(last_text.texture == &meter_texs[0]);
    TEST_CHECK(last_text.color == (int)UINT32_C(0x89abcdef));
    TEST_CHECK(last_text.horizontal_alignment == RIGHT);
    TEST_CHECK(last_text.vertical_alignment == CENTER);
    TEST_CHECK(last_text.x == 125);
    TEST_CHECK(last_text.y == 79);
    TEST_CHECK(last_text.on_hud);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_tick_failure_flushes_only_accepted_meter_primitives(void)
{
    static const float border_points[][2] = {
        {130.0f, 36.0f},
        {130.0f, 46.0f},
        {190.0f, 46.0f},
        {190.0f, 36.0f}
    };

    reset_frame();
    meterBorderColorRGBA = 0x89abcdef;
    meter_texs[0].cache = (TextRendererCache *)&fake_renderer;
    meter_texs[0].text = "Fuel";
    stroke_failure_attempt = 2;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    Paint_meters();

    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(check_stroke(
        0, border_points, 4, 0x89abcdef, 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    Paint_meters();

    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_first_failure_blocks_later_same_frame_meter(void)
{
    reset_frame();
    fill_result = RENDERER_STATUS_OUT_OF_MEMORY;

    Paint_meters();

    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);

    fill_result = RENDERER_STATUS_BACKEND_ERROR;
    Paint_meters();

    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_label_failure_blocks_later_same_frame_meters(void)
{
    reset_frame();
    meterBorderColorRGBA = 0x89abcdef;
    powerMeterColorRGBA = 0x23456789;
    displayedPower = 50.0;
    meter_texs[0].cache = (TextRendererCache *)&fake_renderer;
    meter_texs[0].text = "Fuel";
    meter_texs[1].cache = (TextRendererCache *)&fake_renderer;
    meter_texs[1].text = "Power";
    text_result = RENDERER_STATUS_OUT_OF_MEMORY;

    Paint_meters();

    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 10);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_FILL);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[8] == PAINT_EVENT_FLUSH);
    TEST_CHECK(events[9] == PAINT_EVENT_TEXT);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(stroke_attempts == 6);
    TEST_CHECK(successful_strokes == 6);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(last_text.texture == &meter_texs[0]);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    text_result = RENDERER_STATUS_BACKEND_ERROR;
    Paint_meters();

    TEST_CHECK(preflight_attempts == 4);
    TEST_CHECK(event_count == 10);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(stroke_attempts == 6);
    TEST_CHECK(successful_strokes == 6);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

int main(void)
{
    if (check_null_selection_is_successful_noop() != 0)
        return 1;
    if (check_selection_outline_is_semantic() != 0)
        return 1;
    if (check_selection_requires_active_frame() != 0)
        return 1;
    if (check_selection_stroke_failure_is_sticky() != 0)
        return 1;
    if (check_selection_flush_failure_is_sticky() != 0)
        return 1;
    if (check_fuel_meter_fill_uses_inherited_hud_state() != 0)
        return 1;
    if (check_negative_and_empty_meter_fills() != 0)
        return 1;
    if (check_extreme_meter_value_avoids_signed_overflow() != 0)
        return 1;
    if (check_meter_fill_flushes_before_its_label() != 0)
        return 1;
    if (check_meter_border_and_ticks_are_semantic() != 0)
        return 1;
    if (check_tick_failure_flushes_only_accepted_meter_primitives() != 0)
        return 1;
    if (check_first_failure_blocks_later_same_frame_meter() != 0)
        return 1;
    return check_label_failure_blocks_later_same_frame_meters();
}
