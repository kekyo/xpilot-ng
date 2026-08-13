#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "renderer.h"
#include "images.h"
#include "sdlinit.h"
#include "sdlrenderer.h"
#include "text.h"

#include <GL/gl.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MAX_EVENTS 24
#define MAX_DRAWS 4
#define MAX_DRAW_VERTICES 42
#define MAX_STROKES 10
#define MAX_STROKE_POINTS 16
#define MAX_STIPPLED_STROKES 4

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
    PAINT_EVENT_TRIANGLES,
    PAINT_EVENT_FILL,
    PAINT_EVENT_STROKE,
    PAINT_EVENT_STIPPLED_STROKE,
    PAINT_EVENT_FLUSH,
    PAINT_EVENT_TEXT,
    PAINT_EVENT_IMAGE,
    PAINT_EVENT_MEASURE,
    PAINT_EVENT_HUD_TEXT
} PaintEvent;

typedef struct FakeDraw {
    RendererTexture *texture;
    RendererVertex2D vertices[MAX_DRAW_VERTICES];
    size_t vertex_count;
    RendererBlendMode blend;
    int transform_token;
    int scissor_token;
} FakeDraw;

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
    RendererBlendMode blend;
    int transform_token;
    int scissor_token;
} FakeFill;

typedef struct FakeStroke {
    RendererPoint2D points[MAX_STROKE_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    RendererBlendMode blend;
    int transform_token;
    int scissor_token;
} FakeStroke;

typedef struct FakeStippledStroke {
    RendererPoint2D points[2];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    unsigned int factor;
    uint16_t pattern;
    RendererBlendMode blend;
    int transform_token;
    int scissor_token;
} FakeStippledStroke;

typedef struct FakeText {
    string_tex_t *texture;
    int color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    bool on_hud;
} FakeText;

typedef struct FakeImage {
    int index;
    int x;
    int y;
    int frame;
    int color;
} FakeImage;

typedef struct FakeHudText {
    font_data *font;
    int color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    char text[50];
} FakeHudText;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer,
    RENDERER_STATUS_OK
};
static PaintEvent events[MAX_EVENTS];
static FakeDraw draws[MAX_DRAWS];
static FakeFill last_fill;
static FakeStroke strokes[MAX_STROKES];
static FakeStippledStroke stippled_strokes[MAX_STIPPLED_STROKES];
static FakeText last_text;
static FakeImage last_image;
static FakeHudText last_hud_text;
static char last_measured_text[50];
static int event_count;
static int operation_result_pending;
static int preflight_attempts;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int fill_attempts;
static int successful_fills;
static int stroke_attempts;
static int successful_strokes;
static int stippled_stroke_attempts;
static int successful_stippled_strokes;
static int flush_attempts;
static int transform_attempts;
static int scissor_attempts;
static int text_attempts;
static int image_attempts;
static int measure_attempts;
static int hud_text_attempts;
static int legacy_begin_calls;
static int legacy_vertex_calls;
static int legacy_end_calls;
static int legacy_color_calls;
static int legacy_blend_calls;
static int legacy_line_stipple_calls;
static RendererBlendMode current_blend;
static RendererBlendMode blend_modes[MAX_EVENTS];
static int blend_failure_attempt;
static RendererStatus blend_failure_result;
static int draw_failure_attempt;
static RendererStatus draw_failure_result;
static RendererStatus fill_result;
static int stroke_failure_attempt;
static RendererStatus stroke_failure_result;
static int stippled_stroke_failure_attempt;
static RendererStatus stippled_stroke_failure_result;
static RendererStatus flush_result;
static RendererStatus text_result;
static RendererStatus measure_result;
static RendererStatus hud_text_result;
static float measured_width;
static float measured_height;

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
ipos_t selfVel;
short heading;
short selfVisible;
double ptr_move_fact;
double tbl_sin[TABLE_SIZE];
double tbl_cos[TABLE_SIZE];
client_data_t clData;
ipos_t world;
unsigned RadarHeight;
radar_t *radar_ptr;
int num_radar;
int active_view_width;
int active_view_height;
int hudRadarDotSize;
double hudRadarScale;
double hudRadarLimit;
int hudSize;
instruments_t instruments;
u_byte numItems[NUM_ITEMS];
u_byte lastNumItems[NUM_ITEMS];
int numItemsTime[NUM_ITEMS];
double showItemsTime;
byte lose_item;
int lose_item_active;
double fuelTime;
double fuelCritical;
double fuelWarning;
double fuelNotify;
long loopsSlow;

static int bms_ball_state;
static int bms_cover_state;

static const double radar_ring_cosine[16] = {
    1.0, 0.875, 0.75, 0.375,
    0.0, -0.375, -0.75, -0.875,
    -1.0, -0.875, -0.75, -0.375,
    0.0, 0.375, 0.75, 0.875
};
static const double radar_ring_sine[16] = {
    0.0, 0.375, 0.75, 0.875,
    1.0, 0.875, 0.75, 0.375,
    0.0, -0.375, -0.75, -0.875,
    -1.0, -0.875, -0.75, -0.375
};

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
extern Uint32 hudColorRGBA;
extern Uint32 hudHLineColorRGBA;
extern Uint32 hudVLineColorRGBA;
extern Uint32 dirPtrColorRGBA;
extern Uint32 hudRadarEnemyColorRGBA;
extern Uint32 hudRadarOtherColorRGBA;
extern Uint32 hudRadarObjectColorRGBA;
extern Uint32 msgScanBallColorRGBA;
extern Uint32 msgScanCoverColorRGBA;
extern Uint32 hudItemsColorRGBA;
extern Uint32 fuelGaugeColorRGBA;
extern float hudRadarMapScale;
extern int hudRadarEnemyShape;
extern int hudRadarOtherShape;
extern int hudRadarObjectShape;

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

static int check_stippled_stroke(
    int stroke_index, const float expected_points[][2], uint32_t rgba)
{
    const FakeStippledStroke *stroke;
    size_t point_index;

    TEST_CHECK(stroke_index >= 0);
    TEST_CHECK(stroke_index < successful_stippled_strokes);
    stroke = &stippled_strokes[stroke_index];
    TEST_CHECK(stroke->point_count == 2);
    TEST_CHECK(stroke->width == 1.0f);
    TEST_CHECK(color_equal(
        stroke->color, Renderer_color_from_rgba32(rgba)));
    TEST_CHECK(stroke->closed == 0);
    TEST_CHECK(stroke->factor == 4);
    TEST_CHECK(stroke->pattern == UINT16_C(0xAAAA));
    TEST_CHECK(stroke->blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke->transform_token == 71);
    TEST_CHECK(stroke->scissor_token == 83);
    for (point_index = 0; point_index < 2; point_index++) {
        TEST_CHECK(stroke->points[point_index].x
                   == expected_points[point_index][0]);
        TEST_CHECK(stroke->points[point_index].y
                   == expected_points[point_index][1]);
    }
    return 0;
}

static int check_draw(
    int draw_index, const float expected_points[][2],
    size_t expected_vertex_count, uint32_t rgba,
    RendererBlendMode expected_blend)
{
    const FakeDraw *draw;
    RendererColor expected_color;
    size_t vertex_index;

    TEST_CHECK(draw_index >= 0);
    TEST_CHECK(draw_index < successful_draws);
    draw = &draws[draw_index];
    expected_color = Renderer_color_from_rgba32(rgba);
    TEST_CHECK(draw->texture == NULL);
    TEST_CHECK(draw->vertex_count == expected_vertex_count);
    TEST_CHECK(draw->blend == expected_blend);
    TEST_CHECK(draw->transform_token == 71);
    TEST_CHECK(draw->scissor_token == 83);
    for (vertex_index = 0; vertex_index < expected_vertex_count;
         vertex_index++) {
        TEST_CHECK(draw->vertices[vertex_index].x
                   == expected_points[vertex_index][0]);
        TEST_CHECK(draw->vertices[vertex_index].y
                   == expected_points[vertex_index][1]);
        TEST_CHECK(draw->vertices[vertex_index].u == 0.0f);
        TEST_CHECK(draw->vertices[vertex_index].v == 0.0f);
        TEST_CHECK(color_equal(
            draw->vertices[vertex_index].color, expected_color));
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
    int ring_index;

    memset(events, 0, sizeof(events));
    memset(draws, 0, sizeof(draws));
    memset(&last_fill, 0, sizeof(last_fill));
    memset(strokes, 0, sizeof(strokes));
    memset(stippled_strokes, 0, sizeof(stippled_strokes));
    memset(&last_text, 0, sizeof(last_text));
    memset(&last_image, 0, sizeof(last_image));
    memset(&last_hud_text, 0, sizeof(last_hud_text));
    memset(last_measured_text, 0, sizeof(last_measured_text));
    memset(&gamefont, 0, sizeof(gamefont));
    memset(blend_modes, 0, sizeof(blend_modes));
    memset(tbl_sin, 0, sizeof(tbl_sin));
    memset(tbl_cos, 0, sizeof(tbl_cos));
    memset(&meter_texs[0], 0,
           MAX_METERS * sizeof(meter_texs[0]));
    event_count = 0;
    operation_result_pending = 0;
    preflight_attempts = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    fill_attempts = 0;
    successful_fills = 0;
    stroke_attempts = 0;
    successful_strokes = 0;
    stippled_stroke_attempts = 0;
    successful_stippled_strokes = 0;
    flush_attempts = 0;
    transform_attempts = 0;
    scissor_attempts = 0;
    text_attempts = 0;
    image_attempts = 0;
    measure_attempts = 0;
    hud_text_attempts = 0;
    legacy_begin_calls = 0;
    legacy_vertex_calls = 0;
    legacy_end_calls = 0;
    legacy_color_calls = 0;
    legacy_blend_calls = 0;
    legacy_line_stipple_calls = 0;
    current_blend = RENDERER_BLEND_OPAQUE;
    blend_failure_attempt = 0;
    blend_failure_result = RENDERER_STATUS_OK;
    draw_failure_attempt = 0;
    draw_failure_result = RENDERER_STATUS_OK;
    fill_result = RENDERER_STATUS_OK;
    stroke_failure_attempt = 0;
    stroke_failure_result = RENDERER_STATUS_OK;
    stippled_stroke_failure_attempt = 0;
    stippled_stroke_failure_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    text_result = RENDERER_STATUS_OK;
    measure_result = RENDERER_STATUS_OK;
    hud_text_result = RENDERER_STATUS_OK;
    measured_width = 7.0f;
    measured_height = 12.0f;
    fake_renderer.frame_active = 1;
    fake_renderer.transform_token = 71;
    fake_renderer.scissor_token = 83;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    draw_width = 200;
    draw_height = 120;
    ext_view_width = 200;
    ext_view_height = 120;
    active_view_width = 200;
    active_view_height = 120;
    fake_setup.width = 256;
    fake_setup.height = 128;
    RadarHeight = 128;
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
    selfVel.x = 0;
    selfVel.y = 0;
    heading = 0;
    selfVisible = 0;
    ptr_move_fact = 0.0;
    memset(&clData, 0, sizeof(clData));
    clData.scale = 1.0;
    world.x = 0;
    world.y = 0;
    radar_ptr = NULL;
    num_radar = 0;
    hudRadarDotSize = 4;
    hudRadarScale = 1.0;
    hudRadarLimit = 0.5;
    hudSize = 50;
    memset(&instruments, 0, sizeof(instruments));
    instruments.showItems = true;
    memset(numItems, 0, sizeof(numItems));
    memset(lastNumItems, 0, sizeof(lastNumItems));
    memset(numItemsTime, 0, sizeof(numItemsTime));
    showItemsTime = 2.0;
    lose_item = NO_ITEM;
    lose_item_active = 0;
    fuelTime = 0.0;
    fuelCritical = 20.0;
    fuelWarning = 60.0;
    fuelNotify = 100.0;
    loopsSlow = 0;
    hudColorRGBA = 0;
    hudHLineColorRGBA = 0;
    hudVLineColorRGBA = 0;
    dirPtrColorRGBA = 0;
    hudRadarEnemyColorRGBA = 0;
    hudRadarOtherColorRGBA = 0;
    hudRadarObjectColorRGBA = 0;
    hudRadarEnemyShape = 0;
    hudRadarOtherShape = 0;
    hudRadarObjectShape = 0;
    msgScanBallColorRGBA = 0;
    msgScanCoverColorRGBA = 0;
    hudItemsColorRGBA = 0;
    fuelGaugeColorRGBA = 0;
    hudRadarMapScale = 0.0f;
    bms_ball_state = 0;
    bms_cover_state = 0;
    for (ring_index = 0; ring_index < 16; ring_index++) {
        int table_index = ring_index * TABLE_SIZE / 16;

        tbl_cos[table_index] = radar_ring_cosine[ring_index];
        tbl_sin[table_index] = radar_ring_sine[ring_index];
    }
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
    if (blend_attempts <= MAX_EVENTS)
        blend_modes[blend_attempts - 1] = blend;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active
        || (blend != RENDERER_BLEND_OPAQUE
            && blend != RENDERER_BLEND_ALPHA
            && blend != RENDERER_BLEND_ADDITIVE)) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (blend_failure_attempt == blend_attempts
        && blend_failure_result != RENDERER_STATUS_OK) {
        return blend_failure_result;
    }
    current_blend = blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_triangles(
    Renderer *renderer, RendererTexture *texture,
    const RendererVertex2D *vertices, size_t vertex_count)
{
    FakeDraw *draw;

    draw_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_TRIANGLES);
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture != NULL || vertices == NULL || vertex_count == 0
        || vertex_count % 3 != 0
        || vertex_count > MAX_DRAW_VERTICES) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_failure_attempt == draw_attempts
        && draw_failure_result != RENDERER_STATUS_OK) {
        return draw_failure_result;
    }
    if (successful_draws >= MAX_DRAWS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    draw = &draws[successful_draws++];
    draw->texture = texture;
    memcpy(draw->vertices, vertices,
           vertex_count * sizeof(draw->vertices[0]));
    draw->vertex_count = vertex_count;
    draw->blend = current_blend;
    draw->transform_token = renderer->transform_token;
    draw->scissor_token = renderer->scissor_token;
    return RENDERER_STATUS_OK;
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
    last_fill.blend = current_blend;
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
    stroke->blend = current_blend;
    stroke->transform_token = renderer->transform_token;
    stroke->scissor_token = renderer->scissor_token;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_stroke_stippled_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed,
    unsigned int factor, uint16_t pattern)
{
    FakeStippledStroke *stroke;

    stippled_stroke_attempts++;
    operation_result_pending = 1;
    record_event(PAINT_EVENT_STIPPLED_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count != 2 || width <= 0.0f
        || factor == 0 || factor > 256) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (stippled_stroke_failure_attempt == stippled_stroke_attempts
        && stippled_stroke_failure_result != RENDERER_STATUS_OK) {
        return stippled_stroke_failure_result;
    }
    if (successful_stippled_strokes >= MAX_STIPPLED_STROKES)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    stroke = &stippled_strokes[successful_stippled_strokes++];
    memcpy(stroke->points, points,
           point_count * sizeof(stroke->points[0]));
    stroke->point_count = point_count;
    stroke->width = width;
    stroke->color = color;
    stroke->closed = closed;
    stroke->factor = factor;
    stroke->pattern = pattern;
    stroke->blend = current_blend;
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

void Image_paint_hud(int index, int x, int y, int frame, int color)
{
    image_attempts++;
    record_event(PAINT_EVENT_IMAGE);
    last_image.index = index;
    last_image.x = x;
    last_image.y = y;
    last_image.frame = frame;
    last_image.color = color;
}

RendererStatus printsize(font_data *font, fontbounds *bounds,
                         const char *format, ...)
{
    va_list arguments;

    measure_attempts++;
    record_event(PAINT_EVENT_MEASURE);
    va_start(arguments, format);
    vsnprintf(last_measured_text, sizeof(last_measured_text),
              format, arguments);
    va_end(arguments);
    if (measure_result != RENDERER_STATUS_OK) {
        if (fake_sdl_renderer.frame_result == RENDERER_STATUS_OK)
            fake_sdl_renderer.frame_result = measure_result;
        return measure_result;
    }
    if (font == NULL || bounds == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    bounds->width = measured_width;
    bounds->height = measured_height;
    return RENDERER_STATUS_OK;
}

RendererStatus HUDprint(font_data *font, int color,
                        int horizontal_alignment,
                        int vertical_alignment, int x, int y,
                        const char *format, ...)
{
    va_list arguments;

    hud_text_attempts++;
    record_event(PAINT_EVENT_HUD_TEXT);
    last_hud_text.font = font;
    last_hud_text.color = color;
    last_hud_text.horizontal_alignment = horizontal_alignment;
    last_hud_text.vertical_alignment = vertical_alignment;
    last_hud_text.x = x;
    last_hud_text.y = y;
    va_start(arguments, format);
    vsnprintf(last_hud_text.text, sizeof(last_hud_text.text),
              format, arguments);
    va_end(arguments);
    if (fake_sdl_renderer.frame_result == RENDERER_STATUS_OK
        && hud_text_result != RENDERER_STATUS_OK) {
        fake_sdl_renderer.frame_result = hud_text_result;
    }
    return hud_text_result;
}

bool Bms_test_state(msg_bms_t bms)
{
    if (bms == BmsBall)
        return bms_ball_state;
    if (bms == BmsCover)
        return bms_cover_state;
    return false;
}

void GLAPIENTRY glEnable(GLenum capability)
{
    (void)capability;
    legacy_blend_calls++;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    (void)capability;
    legacy_blend_calls++;
}

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
    legacy_blend_calls++;
}

void GLAPIENTRY glLineStipple(GLint factor, GLushort pattern)
{
    (void)factor;
    (void)pattern;
    legacy_line_stipple_calls++;
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
    TEST_CHECK(legacy_blend_calls == 0);
    TEST_CHECK(legacy_line_stipple_calls == 0);
    return 0;
}

static int check_single_radar_dot_batch(PaintEvent primitive_event)
{
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == primitive_event);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(draw_attempts
               == (primitive_event == PAINT_EVENT_TRIANGLES));
    TEST_CHECK(successful_draws == draw_attempts);
    TEST_CHECK(stroke_attempts
               == (primitive_event == PAINT_EVENT_STROKE));
    TEST_CHECK(successful_strokes == stroke_attempts);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_radar_dot_shapes_are_semantic(void)
{
    static const float circle_points[][2] = {
        {28.0f, 30.0f}, {27.0f, 33.0f},
        {26.0f, 36.0f}, {23.0f, 37.0f},
        {20.0f, 38.0f}, {17.0f, 37.0f},
        {14.0f, 36.0f}, {13.0f, 33.0f},
        {12.0f, 30.0f}, {13.0f, 27.0f},
        {14.0f, 24.0f}, {17.0f, 23.0f},
        {20.0f, 22.0f}, {23.0f, 23.0f},
        {26.0f, 24.0f}, {27.0f, 27.0f}
    };
    static const float filled_circle_points[][2] = {
        {28.0f, 30.0f}, {27.0f, 33.0f}, {26.0f, 36.0f},
        {28.0f, 30.0f}, {26.0f, 36.0f}, {23.0f, 37.0f},
        {28.0f, 30.0f}, {23.0f, 37.0f}, {20.0f, 38.0f},
        {28.0f, 30.0f}, {20.0f, 38.0f}, {17.0f, 37.0f},
        {28.0f, 30.0f}, {17.0f, 37.0f}, {14.0f, 36.0f},
        {28.0f, 30.0f}, {14.0f, 36.0f}, {13.0f, 33.0f},
        {28.0f, 30.0f}, {13.0f, 33.0f}, {12.0f, 30.0f},
        {28.0f, 30.0f}, {12.0f, 30.0f}, {13.0f, 27.0f},
        {28.0f, 30.0f}, {13.0f, 27.0f}, {14.0f, 24.0f},
        {28.0f, 30.0f}, {14.0f, 24.0f}, {17.0f, 23.0f},
        {28.0f, 30.0f}, {17.0f, 23.0f}, {20.0f, 22.0f},
        {28.0f, 30.0f}, {20.0f, 22.0f}, {23.0f, 23.0f},
        {28.0f, 30.0f}, {23.0f, 23.0f}, {26.0f, 24.0f},
        {28.0f, 30.0f}, {26.0f, 24.0f}, {27.0f, 27.0f}
    };
    static const float square_points[][2] = {
        {12.0f, 22.0f}, {12.0f, 38.0f},
        {28.0f, 38.0f}, {28.0f, 22.0f}
    };
    static const float filled_square_points[][2] = {
        {12.0f, 22.0f}, {12.0f, 38.0f}, {28.0f, 38.0f},
        {12.0f, 22.0f}, {28.0f, 38.0f}, {28.0f, 22.0f}
    };
    static const float triangle_points[][2] = {
        {12.0f, 38.0f}, {20.0f, 22.0f}, {28.0f, 38.0f}
    };
    RendererStatus result;

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x10203040), 2, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(
        PAINT_EVENT_TRIANGLES) == 0);
    TEST_CHECK(check_draw(
        0, filled_circle_points, 42, UINT32_C(0x10203040),
        RENDERER_BLEND_ALPHA) == 0);

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x20304050), 3, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(PAINT_EVENT_STROKE) == 0);
    TEST_CHECK(check_stroke(
        0, circle_points, 16, UINT32_C(0x20304050), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x30405060), 4, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(
        PAINT_EVENT_TRIANGLES) == 0);
    TEST_CHECK(check_draw(
        0, filled_square_points, 6, UINT32_C(0x30405060),
        RENDERER_BLEND_ALPHA) == 0);

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x40506070), 5, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(PAINT_EVENT_STROKE) == 0);
    TEST_CHECK(check_stroke(
        0, square_points, 4, UINT32_C(0x40506070), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x50607080), 6, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(
        PAINT_EVENT_TRIANGLES) == 0);
    TEST_CHECK(check_draw(
        0, triangle_points, 3, UINT32_C(0x50607080),
        RENDERER_BLEND_ALPHA) == 0);

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        20, 30, UINT32_C(0x60708090), 7, 8);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(PAINT_EVENT_STROKE) == 0);
    TEST_CHECK(check_stroke(
        0, triangle_points, 3, UINT32_C(0x60708090), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    return 0;
}

static int check_hud_radar_dot_noops_and_signed_boundaries(void)
{
    static const struct {
        Uint32 color;
        int shape;
        int size;
    } noops[] = {
        {0, 2, 8},
        {UINT32_C(0x10203040), 1, 8},
        {UINT32_C(0x10203040), 8, 8},
        {UINT32_C(0x10203040), 4, 0}
    };
    static const float extreme_square_points[][2] = {
        {4294967296.0f, 0.0f},
        {4294967296.0f, -4294967296.0f},
        {-1.0f, -4294967296.0f},
        {4294967296.0f, 0.0f},
        {-1.0f, -4294967296.0f},
        {-1.0f, 0.0f}
    };
    RendererStatus result;
    size_t noop_index;

    for (noop_index = 0;
         noop_index < sizeof(noops) / sizeof(noops[0]);
         noop_index++) {
        reset_frame();
        result = Sdlgui_test_paint_hud_radar_dot(
            20, 30, noops[noop_index].color,
            noops[noop_index].shape, noops[noop_index].size);
        TEST_CHECK(result == RENDERER_STATUS_OK);
        TEST_CHECK(preflight_attempts == 0);
        TEST_CHECK(event_count == 0);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(draw_attempts == 0);
        TEST_CHECK(stroke_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        if (check_no_legacy_primitives() != 0)
            return 1;
    }

    reset_frame();
    result = Sdlgui_test_paint_hud_radar_dot(
        INT_MAX, INT_MIN, UINT32_C(0xabcdef80), 4, INT_MIN);
    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(check_single_radar_dot_batch(
        PAINT_EVENT_TRIANGLES) == 0);
    return check_draw(
        0, extreme_square_points, 6, UINT32_C(0xabcdef80),
        RENDERER_BLEND_ALPHA);
}

static void reset_full_hud_radar_fixture(void)
{
    static radar_t radar_entries[] = {
        {90, 40, 1, RadarFriend},
        {110, 40, 1, RadarEnemy},
        {120, 40, 0, RadarFriend},
        {75, 60, 1, RadarFriend},
        {190, 40, 1, RadarFriend}
    };

    reset_frame();
    ext_view_width = 100;
    ext_view_height = 80;
    active_view_width = 100;
    active_view_height = 80;
    radar_ptr = radar_entries;
    num_radar = (int)(sizeof(radar_entries) / sizeof(radar_entries[0]));
    hudRadarEnemyColorRGBA = UINT32_C(0x11223344);
    hudRadarOtherColorRGBA = UINT32_C(0x55667788);
    hudRadarObjectColorRGBA = UINT32_C(0x99aabbcc);
    hudRadarEnemyShape = 7;
    hudRadarOtherShape = 5;
    hudRadarObjectShape = 4;
    hudRadarScale = 2.0;
    msgScanBallColorRGBA = UINT32_C(0xddeeff80);
    msgScanCoverColorRGBA = UINT32_C(0x12345678);
    bms_ball_state = 1;
    bms_cover_state = 1;
}

static int check_hud_radar_disabled_and_scan_only_paths(void)
{
    static const float ball_scan[][2] = {
        {104.0f, 60.0f}, {103.5f, 61.5f},
        {103.0f, 63.0f}, {101.5f, 63.5f},
        {100.0f, 64.0f}, {98.5f, 63.5f},
        {97.0f, 63.0f}, {96.5f, 61.5f},
        {96.0f, 60.0f}, {96.5f, 58.5f},
        {97.0f, 57.0f}, {98.5f, 56.5f},
        {100.0f, 56.0f}, {101.5f, 56.5f},
        {103.0f, 57.0f}, {103.5f, 58.5f}
    };
    static const float cover_scan[][2] = {
        {103.0f, 60.0f}, {102.625f, 61.125f},
        {102.25f, 62.25f}, {101.125f, 62.625f},
        {100.0f, 63.0f}, {98.875f, 62.625f},
        {97.75f, 62.25f}, {97.375f, 61.125f},
        {97.0f, 60.0f}, {97.375f, 58.875f},
        {97.75f, 57.75f}, {98.875f, 57.375f},
        {100.0f, 57.0f}, {101.125f, 57.375f},
        {102.25f, 57.75f}, {102.625f, 58.875f}
    };
    RendererStatus result;

    reset_frame();

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    clData.scale = 0.5;
    msgScanBallColorRGBA = UINT32_C(0xddeeff80);
    msgScanCoverColorRGBA = UINT32_C(0x12345678);
    bms_ball_state = 1;
    bms_cover_state = 1;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(blend_modes[1] == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(check_stroke(
        0, ball_scan, 16, UINT32_C(0xddeeff80), 1) == 0);
    TEST_CHECK(check_stroke(
        1, cover_scan, 16, UINT32_C(0x12345678), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(strokes[1].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_negative_odd_object_size_uses_shift_equivalent_half(void)
{
    static radar_t object_entry = {90, 40, 0, RadarFriend};
    static const float expected_fill[][2] = {
        {182.0f, 62.0f}, {182.0f, 58.0f}, {178.0f, 58.0f},
        {182.0f, 62.0f}, {178.0f, 58.0f}, {178.0f, 62.0f}
    };
    RendererStatus result;

    reset_frame();
    ext_view_width = 100;
    ext_view_height = 80;
    active_view_width = 100;
    active_view_height = 80;
    radar_ptr = &object_entry;
    num_radar = 1;
    hudRadarScale = 2.0;
    hudRadarLimit = 0.5;
    hudRadarDotSize = -3;
    hudRadarObjectColorRGBA = UINT32_C(0x99aabbcc);
    hudRadarObjectShape = 4;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(blend_modes[1] == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_draw(
        0, expected_fill, 6, UINT32_C(0x99aabbcc),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_late_invalid_scan_geometry_rejects_whole_scene(void)
{
    const int invalid_table_index = 15 * TABLE_SIZE / 16;
    RendererStatus result;

    reset_full_hud_radar_fixture();
    bms_ball_state = 0;
    msgScanBallColorRGBA = 0;
    tbl_cos[invalid_table_index] = NAN;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    tbl_cos[invalid_table_index] = radar_ring_cosine[15];
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 3);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    return check_no_legacy_primitives();
}

static int check_hud_radar_two_pass_order_and_scans(void)
{
    static const float first_friend[][2] = {
        {176.0f, 56.0f}, {176.0f, 64.0f},
        {184.0f, 64.0f}, {184.0f, 56.0f}
    };
    static const float first_enemy[][2] = {
        {216.0f, 64.0f}, {220.0f, 56.0f}, {224.0f, 64.0f}
    };
    static const float first_object[][2] = {
        {238.0f, 58.0f}, {238.0f, 62.0f}, {242.0f, 62.0f},
        {238.0f, 58.0f}, {242.0f, 62.0f}, {242.0f, 58.0f}
    };
    static const float first_wrapped_friend[][2] = {
        {-136.0f, 56.0f}, {-136.0f, 64.0f},
        {-128.0f, 64.0f}, {-128.0f, 56.0f}
    };
    static const float second_enemy[][2] = {
        {144.0f, 76.0f}, {160.0f, 44.0f}, {176.0f, 76.0f}
    };
    static const float second_object[][2] = {
        {162.0f, 52.0f}, {162.0f, 68.0f}, {178.0f, 68.0f},
        {162.0f, 52.0f}, {178.0f, 68.0f}, {178.0f, 52.0f}
    };
    static const float second_wrapped_friend[][2] = {
        {-32.0f, 44.0f}, {-32.0f, 76.0f},
        {0.0f, 76.0f}, {0.0f, 44.0f}
    };
    static const float ball_scan[][2] = {
        {108.0f, 60.0f}, {107.0f, 63.0f},
        {106.0f, 66.0f}, {103.0f, 67.0f},
        {100.0f, 68.0f}, {97.0f, 67.0f},
        {94.0f, 66.0f}, {93.0f, 63.0f},
        {92.0f, 60.0f}, {93.0f, 57.0f},
        {94.0f, 54.0f}, {97.0f, 53.0f},
        {100.0f, 52.0f}, {103.0f, 53.0f},
        {106.0f, 54.0f}, {107.0f, 57.0f}
    };
    static const float cover_scan[][2] = {
        {106.0f, 60.0f}, {105.25f, 62.25f},
        {104.5f, 64.5f}, {102.25f, 65.25f},
        {100.0f, 66.0f}, {97.75f, 65.25f},
        {95.5f, 64.5f}, {94.75f, 62.25f},
        {94.0f, 60.0f}, {94.75f, 57.75f},
        {95.5f, 55.5f}, {97.75f, 54.75f},
        {100.0f, 54.0f}, {102.25f, 54.75f},
        {104.5f, 55.5f}, {105.25f, 57.75f}
    };
    RendererStatus result;

    reset_full_hud_radar_fixture();

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 12);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[4] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[6] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[7] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[8] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[9] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[10] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[11] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(blend_modes[1] == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(stroke_attempts == 7);
    TEST_CHECK(successful_strokes == 7);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(hudRadarMapScale == 1.0f);
    TEST_CHECK(check_stroke(
        0, first_friend, 4, UINT32_C(0x55667788), 1) == 0);
    TEST_CHECK(check_stroke(
        1, first_enemy, 3, UINT32_C(0x11223344), 1) == 0);
    TEST_CHECK(check_draw(
        0, first_object, 6, UINT32_C(0x99aabbcc),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(check_stroke(
        2, first_wrapped_friend, 4, UINT32_C(0x55667788), 1) == 0);
    TEST_CHECK(check_stroke(
        3, second_enemy, 3, UINT32_C(0x11223344), 1) == 0);
    TEST_CHECK(check_draw(
        1, second_object, 6, UINT32_C(0x99aabbcc),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(check_stroke(
        4, second_wrapped_friend, 4, UINT32_C(0x55667788), 1) == 0);
    TEST_CHECK(check_stroke(
        5, ball_scan, 16, UINT32_C(0xddeeff80), 1) == 0);
    TEST_CHECK(check_stroke(
        6, cover_scan, 16, UINT32_C(0x12345678), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[1].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[2].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[3].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[4].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[5].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(strokes[6].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_radar_failure_flushes_only_accepted_prefix(void)
{
    static const float first_friend[][2] = {
        {176.0f, 56.0f}, {176.0f, 64.0f},
        {184.0f, 64.0f}, {184.0f, 56.0f}
    };
    static const float first_enemy[][2] = {
        {216.0f, 64.0f}, {220.0f, 56.0f}, {224.0f, 64.0f}
    };
    static const float first_object[][2] = {
        {238.0f, 58.0f}, {238.0f, 62.0f}, {242.0f, 62.0f},
        {238.0f, 58.0f}, {242.0f, 62.0f}, {242.0f, 58.0f}
    };
    RendererStatus result;

    reset_full_hud_radar_fixture();
    stroke_failure_attempt = 3;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[4] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 3);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, first_friend, 4, UINT32_C(0x55667788), 1) == 0);
    TEST_CHECK(check_stroke(
        1, first_enemy, 3, UINT32_C(0x11223344), 1) == 0);
    TEST_CHECK(check_draw(
        0, first_object, 6, UINT32_C(0x99aabbcc),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 3);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_hud_radar_object_overrides_are_independent(void)
{
    static radar_t object_entry = {120, 40, 0, RadarFriend};
    static const float first_outline[][2] = {
        {238.0f, 58.0f}, {238.0f, 62.0f},
        {242.0f, 62.0f}, {242.0f, 58.0f}
    };
    static const float second_outline[][2] = {
        {162.0f, 52.0f}, {162.0f, 68.0f},
        {178.0f, 68.0f}, {178.0f, 52.0f}
    };
    static const float first_fill[][2] = {
        {238.0f, 58.0f}, {238.0f, 62.0f}, {242.0f, 62.0f},
        {238.0f, 58.0f}, {242.0f, 62.0f}, {242.0f, 58.0f}
    };
    static const float second_fill[][2] = {
        {162.0f, 52.0f}, {162.0f, 68.0f}, {178.0f, 68.0f},
        {162.0f, 52.0f}, {178.0f, 68.0f}, {178.0f, 52.0f}
    };
    RendererStatus result;

    reset_frame();
    ext_view_width = 100;
    ext_view_height = 80;
    active_view_width = 100;
    active_view_height = 80;
    radar_ptr = &object_entry;
    num_radar = 1;
    hudRadarScale = 2.0;
    hudRadarLimit = 0.5;
    hudRadarDotSize = 4;
    hudRadarOtherColorRGBA = 0;
    hudRadarOtherShape = 5;
    hudRadarObjectColorRGBA = UINT32_C(0x99aabbcc);
    hudRadarObjectShape = 0;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(blend_modes[1] == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, first_outline, 4, UINT32_C(0x99aabbcc), 1) == 0);
    TEST_CHECK(check_stroke(
        1, second_outline, 4, UINT32_C(0x99aabbcc), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(strokes[1].blend == RENDERER_BLEND_ALPHA);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    ext_view_width = 100;
    ext_view_height = 80;
    active_view_width = 100;
    active_view_height = 80;
    radar_ptr = &object_entry;
    num_radar = 1;
    hudRadarScale = 2.0;
    hudRadarLimit = 0.5;
    hudRadarDotSize = 4;
    hudRadarOtherColorRGBA = UINT32_C(0x55667788);
    hudRadarOtherShape = 5;
    hudRadarObjectColorRGBA = 0;
    hudRadarObjectShape = 4;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[3] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(blend_modes[1] == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_draw(
        0, first_fill, 6, UINT32_C(0x55667788),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(check_draw(
        1, second_fill, 6, UINT32_C(0x55667788),
        RENDERER_BLEND_ALPHA) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_radar_draw_and_blend_failures_are_sticky(void)
{
    static const float first_friend[][2] = {
        {176.0f, 56.0f}, {176.0f, 64.0f},
        {184.0f, 64.0f}, {184.0f, 56.0f}
    };
    static const float first_enemy[][2] = {
        {216.0f, 64.0f}, {220.0f, 56.0f}, {224.0f, 64.0f}
    };
    RendererStatus result;

    reset_full_hud_radar_fixture();
    draw_failure_attempt = 1;
    draw_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, first_friend, 4, UINT32_C(0x55667788), 1) == 0);
    TEST_CHECK(check_stroke(
        1, first_enemy, 3, UINT32_C(0x11223344), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    draw_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_full_hud_radar_fixture();
    blend_failure_attempt = 2;
    blend_failure_result = RENDERER_STATUS_BACKEND_ERROR;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 10);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[4] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[6] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[7] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[8] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[9] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(stroke_attempts == 5);
    TEST_CHECK(successful_strokes == 5);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    if (check_no_legacy_primitives() != 0)
        return 1;

    blend_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 10);
    TEST_CHECK(blend_attempts == 2);
    TEST_CHECK(stroke_attempts == 5);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return check_no_legacy_primitives();
}

static int check_invalid_hud_radar_math_is_sticky(void)
{
    RendererStatus result;

    reset_full_hud_radar_fixture();
    hudRadarScale = NAN;

    result = Sdlgui_test_paint_hud_radar_and_scans();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    hudRadarScale = 2.0;
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 3);
    TEST_CHECK(event_count == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_full_hud_radar_fixture();
    fake_setup.width = 0;
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(flush_attempts == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_full_hud_radar_fixture();
    clData.scale = HUGE_VAL;
    result = Sdlgui_test_paint_hud_radar_and_scans();
    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    return check_no_legacy_primitives();
}

static int check_hud_pointers_are_semantic(void)
{
    /* Odd centers and negative fractional endpoints pin truncation to zero. */
    static const float speed_points[][2] = {
        {25.0f, 15.0f},
        {-4.0f, 0.0f}
    };
    static const float direction_points[][2] = {
        {-6.0f, -4.0f},
        {-12.0f, -8.0f}
    };
    RendererStatus result;

    reset_frame();
    draw_width = 51;
    draw_height = 31;
    selfVisible = 1;
    selfVel.x = 13;
    selfVel.y = -7;
    ptr_move_fact = 2.25;
    hudColorRGBA = UINT32_C(0x10203040);
    dirPtrColorRGBA = UINT32_C(0x50607080);
    heading = 7;
    tbl_cos[heading] = -0.371;
    tbl_sin[heading] = 0.233;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(check_stroke(
        0, speed_points, 2, UINT32_C(0x10203040), 1) == 0);
    TEST_CHECK(check_stroke(
        1, direction_points, 2, UINT32_C(0x50607080), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_pointer_visibility_conditions(void)
{
    static const float speed_points[][2] = {
        {100.0f, 60.0f},
        {98.0f, 60.0f}
    };
    RendererStatus result;

    reset_frame();
    selfVisible = 1;
    selfVel.x = 2;
    ptr_move_fact = 0.0;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 0;
    selfVel.x = 2;
    ptr_move_fact = 1.0;
    dirPtrColorRGBA = UINT32_C(0x50607080);

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    ptr_move_fact = 1.0;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    selfVel.x = 2;
    ptr_move_fact = 1.0;
    hudColorRGBA = 0;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, speed_points, 2, UINT32_C(0x00000000), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_pointer_failure_flushes_only_accepted_prefix(void)
{
    static const float speed_points[][2] = {
        {100.0f, 60.0f},
        {90.0f, 45.0f}
    };
    RendererStatus result;

    reset_frame();
    selfVisible = 1;
    selfVel.x = 4;
    selfVel.y = -6;
    ptr_move_fact = 2.5;
    hudColorRGBA = UINT32_C(0x10203040);
    dirPtrColorRGBA = UINT32_C(0x50607080);
    heading = 0;
    stroke_failure_attempt = 2;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, speed_points, 2, UINT32_C(0x10203040), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stroke_attempts == 2);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_invalid_hud_pointer_geometry_is_rejected(void)
{
    RendererStatus result;

    reset_frame();
    selfVisible = 1;
    selfVel.x = 1;
    ptr_move_fact = HUGE_VAL;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    selfVel.x = 2;
    ptr_move_fact = (double)INT_MAX;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    dirPtrColorRGBA = UINT32_C(0x50607080);
    heading = -1;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    dirPtrColorRGBA = UINT32_C(0x50607080);
    heading = TABLE_SIZE;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    dirPtrColorRGBA = UINT32_C(0x50607080);
    tbl_cos[0] = NAN;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_frame();
    selfVisible = 1;
    dirPtrColorRGBA = UINT32_C(0x50607080);
    tbl_sin[0] = HUGE_VAL;

    result = Sdlgui_test_paint_hud_pointers();

    TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    return check_no_legacy_primitives();
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

static void reset_hud_frame_fixture(void)
{
    reset_frame();
    hudHLineColorRGBA = UINT32_C(0x11223344);
    hudVLineColorRGBA = UINT32_C(0xa1b2c3d4);
}

static int check_hud_frame_paths_are_semantic(void)
{
    static const float frame_paths[][2][2] = {
        {{50.0f, 30.0f}, {150.0f, 30.0f}},
        {{50.0f, 90.0f}, {150.0f, 90.0f}},
        {{70.0f, 10.0f}, {70.0f, 110.0f}},
        {{130.0f, 10.0f}, {130.0f, 110.0f}}
    };
    static const uint32_t frame_colors[] = {
        UINT32_C(0x11223344), UINT32_C(0x11223344),
        UINT32_C(0xa1b2c3d4), UINT32_C(0xa1b2c3d4)
    };
    RendererStatus result;
    int path_index;

    reset_hud_frame_fixture();

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    for (path_index = 0; path_index < 4; path_index++)
        TEST_CHECK(events[path_index + 1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stippled_stroke_attempts == 4);
    TEST_CHECK(successful_stippled_strokes == 4);
    for (path_index = 0; path_index < 4; path_index++) {
        if (check_stippled_stroke(
                path_index, frame_paths[path_index],
                frame_colors[path_index]) != 0) {
            return 1;
        }
    }
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_frame_visibility_conditions(void)
{
    static const float horizontal_paths[][2][2] = {
        {{50.0f, 30.0f}, {150.0f, 30.0f}},
        {{50.0f, 90.0f}, {150.0f, 90.0f}}
    };
    static const float vertical_paths[][2][2] = {
        {{70.0f, 10.0f}, {70.0f, 110.0f}},
        {{130.0f, 10.0f}, {130.0f, 110.0f}}
    };
    RendererStatus result;
    int path_index;

    reset_hud_frame_fixture();
    hudVLineColorRGBA = 0;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(stippled_stroke_attempts == 2);
    TEST_CHECK(successful_stippled_strokes == 2);
    for (path_index = 0; path_index < 2; path_index++) {
        if (check_stippled_stroke(
                path_index, horizontal_paths[path_index],
                UINT32_C(0x11223344)) != 0) {
            return 1;
        }
    }
    TEST_CHECK(flush_attempts == 1);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_frame_fixture();
    hudHLineColorRGBA = 0;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(stippled_stroke_attempts == 2);
    TEST_CHECK(successful_stippled_strokes == 2);
    for (path_index = 0; path_index < 2; path_index++) {
        if (check_stippled_stroke(
                path_index, vertical_paths[path_index],
                UINT32_C(0xa1b2c3d4)) != 0) {
            return 1;
        }
    }
    TEST_CHECK(flush_attempts == 1);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_frame_fixture();
    hudHLineColorRGBA = 0;
    hudVLineColorRGBA = 0;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_frame_partial_failure_flushes_accepted_prefix(void)
{
    static const float accepted_paths[][2][2] = {
        {{50.0f, 30.0f}, {150.0f, 30.0f}},
        {{50.0f, 90.0f}, {150.0f, 90.0f}}
    };
    RendererStatus result;
    int path_index;

    reset_hud_frame_fixture();
    stippled_stroke_failure_attempt = 3;
    stippled_stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(stippled_stroke_attempts == 3);
    TEST_CHECK(successful_stippled_strokes == 2);
    for (path_index = 0; path_index < 2; path_index++) {
        if (check_stippled_stroke(
                path_index, accepted_paths[path_index],
                UINT32_C(0x11223344)) != 0) {
            return 1;
        }
    }
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    stippled_stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stippled_stroke_attempts == 3);
    TEST_CHECK(successful_stippled_strokes == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    return check_no_legacy_primitives();
}

static int check_hud_frame_first_operation_failures_are_sticky(void)
{
    RendererStatus result;

    reset_hud_frame_fixture();
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    blend_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    result = Sdlgui_test_paint_hud_frame(100, 60);
    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(blend_attempts == 1);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_frame_fixture();
    stippled_stroke_failure_attempt = 1;
    stippled_stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stippled_stroke_attempts == 1);
    TEST_CHECK(successful_stippled_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    stippled_stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    result = Sdlgui_test_paint_hud_frame(100, 60);
    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(stippled_stroke_attempts == 1);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_primitives();
}

static int check_hud_frame_flush_failure_is_sticky(void)
{
    RendererStatus result;

    reset_hud_frame_fixture();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;

    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_FLUSH);
    TEST_CHECK(stippled_stroke_attempts == 4);
    TEST_CHECK(successful_stippled_strokes == 4);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    if (check_no_legacy_primitives() != 0)
        return 1;

    flush_result = RENDERER_STATUS_OUT_OF_MEMORY;
    result = Sdlgui_test_paint_hud_frame(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(stippled_stroke_attempts == 4);
    TEST_CHECK(successful_stippled_strokes == 4);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return check_no_legacy_primitives();
}

static void reset_hud_status_fixture(void)
{
    reset_frame();
    hudItemsColorRGBA = UINT32_C(0x2468ace0);
    fuelGaugeColorRGBA = UINT32_C(0x13579bdf);
}

static void show_test_lose_item(void)
{
    numItems[ITEM_MINE] = 3;
    lose_item = ITEM_MINE;
    lose_item_active = -2;
}

static int check_hud_lose_item_outline_precedes_count_text(void)
{
    static const float outline_points[][2] = {
        {49.0f, 31.0f},
        {67.0f, 31.0f},
        {67.0f, 49.0f},
        {49.0f, 49.0f}
    };
    RendererStatus result;

    reset_hud_status_fixture();
    show_test_lose_item();

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(events[4] == PAINT_EVENT_MEASURE);
    TEST_CHECK(events[5] == PAINT_EVENT_HUD_TEXT);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(last_image.index == IMG_HUD_ITEMS);
    TEST_CHECK(last_image.x == 51);
    TEST_CHECK(last_image.y == 33);
    TEST_CHECK(last_image.frame == ITEM_MINE);
    TEST_CHECK(last_image.color == (int)UINT32_C(0x2468ace0));
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(check_stroke(
        0, outline_points, 4, UINT32_C(0x2468ace0), 1) == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(measure_attempts == 1);
    TEST_CHECK(strcmp(last_measured_text, "3") == 0);
    TEST_CHECK(hud_text_attempts == 1);
    TEST_CHECK(last_hud_text.font == &gamefont);
    TEST_CHECK(last_hud_text.color == (int)UINT32_C(0x2468ace0));
    TEST_CHECK(last_hud_text.horizontal_alignment == RIGHT);
    TEST_CHECK(last_hud_text.vertical_alignment == UP);
    TEST_CHECK(last_hud_text.x == 48);
    TEST_CHECK(last_hud_text.y == 71);
    TEST_CHECK(strcmp(last_hud_text.text, "3") == 0);
    TEST_CHECK(lastNumItems[ITEM_MINE] == 3);
    TEST_CHECK(lose_item_active == -1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_no_legacy_primitives();
}

static int check_hud_item_visibility_and_active_side_effects(void)
{
    RendererStatus result;

    reset_hud_status_fixture();
    numItems[ITEM_MINE] = 3;
    lose_item = ITEM_MINE;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_MEASURE);
    TEST_CHECK(events[2] == PAINT_EVENT_HUD_TEXT);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(lose_item_active == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    lose_item = ITEM_MINE;
    lose_item_active = -2;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(preflight_attempts == 0);
    TEST_CHECK(lastNumItems[ITEM_MINE] == 0);
    TEST_CHECK(lose_item_active == -2);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    numItems[ITEM_MINE] = 3;
    lose_item = ITEM_MINE;
    lose_item_active = -1;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(lose_item_active == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    instruments.showItems = false;
    numItems[ITEM_MINE] = 4;
    lastNumItems[ITEM_MINE] = 4;
    numItemsTime[ITEM_MINE] = 1;
    lose_item = ITEM_MINE;
    lose_item_active = 1;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(numItemsTime[ITEM_MINE] == 0);
    TEST_CHECK(lose_item_active == 1);
    TEST_CHECK(image_attempts == 1);

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(numItemsTime[ITEM_MINE] == 0);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(lose_item_active == 1);
    return check_no_legacy_primitives();
}

static int check_hud_item_failures_stop_later_work(void)
{
    RendererStatus result;

    reset_hud_status_fixture();
    show_test_lose_item();
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(measure_attempts == 0);
    TEST_CHECK(hud_text_attempts == 0);
    TEST_CHECK(lose_item_active == -1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    show_test_lose_item();
    measure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(events[4] == PAINT_EVENT_MEASURE);
    TEST_CHECK(hud_text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    show_test_lose_item();
    hud_text_result = RENDERER_STATUS_BACKEND_ERROR;

    result = Sdlgui_test_paint_hud_items(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[5] == PAINT_EVENT_HUD_TEXT);
    TEST_CHECK(hud_text_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return check_no_legacy_primitives();
}

static int check_hud_fuel_visibility_table(void)
{
    static const struct {
        Uint32 color;
        double time;
        double sum;
        long loop;
        int visible;
    } cases[] = {
        {0, 1.0, 1000.0, 0, 0},
        {UINT32_C(0x13579bdf), 1.0, 1000.0, 0, 1},
        {UINT32_C(0x13579bdf), 0.0, 10.0, 0, 1},
        {UINT32_C(0x13579bdf), 0.0, 10.0, 2, 0},
        {UINT32_C(0x13579bdf), 0.0, 50.0, 3, 1},
        {UINT32_C(0x13579bdf), 0.0, 50.0, 4, 0},
        {UINT32_C(0x13579bdf), 0.0, 80.0, 7, 1},
        {UINT32_C(0x13579bdf), 0.0, 20.0, 0, 0},
        {UINT32_C(0x13579bdf), 0.0, 60.0, 0, 0},
        {UINT32_C(0x13579bdf), 0.0, 100.0, 0, 0}
    };
    size_t case_index;

    for (case_index = 0;
         case_index < sizeof(cases) / sizeof(cases[0]);
         case_index++) {
        RendererStatus result;

        reset_hud_status_fixture();
        fuelGaugeColorRGBA = cases[case_index].color;
        fuelTime = cases[case_index].time;
        fuelSum = cases[case_index].sum;
        loopsSlow = cases[case_index].loop;

        result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

        TEST_CHECK(result == RENDERER_STATUS_OK);
        TEST_CHECK(preflight_attempts == cases[case_index].visible);
        TEST_CHECK(event_count == (cases[case_index].visible ? 4 : 0));
        TEST_CHECK(blend_attempts == cases[case_index].visible);
        TEST_CHECK(stroke_attempts == cases[case_index].visible);
        TEST_CHECK(fill_attempts == cases[case_index].visible);
        TEST_CHECK(flush_attempts == cases[case_index].visible);
        TEST_CHECK(fuelTime == cases[case_index].time);
        if (check_no_legacy_primitives() != 0)
            return 1;
    }
    return 0;
}

static int check_hud_fuel_exact_and_unbounded_geometry(void)
{
    static const float outline_points[][2] = {
        {135.0f, 35.0f},
        {135.0f, 166.0f},
        {146.0f, 166.0f},
        {146.0f, 35.0f}
    };
    RendererStatus result;

    reset_hud_status_fixture();
    fuelTime = 1.0;

    result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FILL);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(check_stroke(
        0, outline_points, 4, UINT32_C(0x13579bdf), 1) == 0);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 137.0f);
    TEST_CHECK(last_fill.y == 133.0f);
    TEST_CHECK(last_fill.width == 8.0f);
    TEST_CHECK(last_fill.height == 32.0f);
    TEST_CHECK(color_equal(
        last_fill.color,
        Renderer_color_from_rgba32(UINT32_C(0x13579bdf))));
    TEST_CHECK(last_fill.blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(last_fill.transform_token == 71);
    TEST_CHECK(last_fill.scissor_token == 83);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    fuelTime = 1.0;
    fuelSum = -25.0;

    result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 137.0f);
    TEST_CHECK(last_fill.y == 165.0f);
    TEST_CHECK(last_fill.width == 8.0f);
    TEST_CHECK(last_fill.height == 32.0f);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    fuelTime = 1.0;
    fuelSum = 150.0;

    result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 137.0f);
    TEST_CHECK(last_fill.y == -27.0f);
    TEST_CHECK(last_fill.width == 8.0f);
    TEST_CHECK(last_fill.height == 192.0f);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    fuelTime = 1.0;
    fuelSum = 0.0;

    result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

    TEST_CHECK(result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 1);
    return check_no_legacy_primitives();
}

static int check_hud_fuel_invalid_math_and_partial_failure(void)
{
    static const struct {
        double sum;
        double maximum;
    } invalid_cases[] = {
        {NAN, 100.0},
        {25.0, 0.0},
        {25.0, NAN},
        {HUGE_VAL, 1.0},
        {25.0, HUGE_VAL},
        {DBL_MAX, 1.0},
        {(double)INT_MAX, 1.0},
        {(double)INT_MIN, 1.0}
    };
    static const float outline_points[][2] = {
        {135.0f, 35.0f},
        {135.0f, 166.0f},
        {146.0f, 166.0f},
        {146.0f, 35.0f}
    };
    size_t case_index;

    for (case_index = 0;
         case_index < sizeof(invalid_cases) / sizeof(invalid_cases[0]);
         case_index++) {
        RendererStatus result;

        reset_hud_status_fixture();
        fuelTime = 1.0;
        fuelSum = invalid_cases[case_index].sum;
        fuelMax = invalid_cases[case_index].maximum;

        result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);

        TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(preflight_attempts == 2);
        TEST_CHECK(event_count == 0);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(stroke_attempts == 0);
        TEST_CHECK(fill_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        TEST_CHECK(fake_sdl_renderer.frame_result
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        fuelSum = 25.0;
        fuelMax = 100.0;
        result = Sdlgui_test_paint_hud_fuel_gauge(100, 60);
        TEST_CHECK(result == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(preflight_attempts == 3);
        TEST_CHECK(event_count == 0);
        if (check_no_legacy_primitives() != 0)
            return 1;
    }

    reset_hud_status_fixture();
    fuelTime = 1.0;
    fill_result = RENDERER_STATUS_OUT_OF_MEMORY;

    {
        RendererStatus result =
            Sdlgui_test_paint_hud_fuel_gauge(100, 60);

        TEST_CHECK(result == RENDERER_STATUS_OUT_OF_MEMORY);
    }
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FILL);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_stroke(
        0, outline_points, 4, UINT32_C(0x13579bdf), 1) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    if (check_no_legacy_primitives() != 0)
        return 1;

    fill_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(Sdlgui_test_paint_hud_fuel_gauge(100, 60)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    if (check_no_legacy_primitives() != 0)
        return 1;

    reset_hud_status_fixture();
    fuelTime = 1.0;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;

    {
        RendererStatus result =
            Sdlgui_test_paint_hud_fuel_gauge(100, 60);

        TEST_CHECK(result == RENDERER_STATUS_BACKEND_ERROR);
    }
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    if (check_no_legacy_primitives() != 0)
        return 1;

    flush_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(Sdlgui_test_paint_hud_fuel_gauge(100, 60)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
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
    if (check_hud_radar_dot_shapes_are_semantic() != 0)
        return 1;
    if (check_hud_radar_dot_noops_and_signed_boundaries() != 0)
        return 1;
    if (check_hud_radar_disabled_and_scan_only_paths() != 0)
        return 1;
    if (check_negative_odd_object_size_uses_shift_equivalent_half() != 0)
        return 1;
    if (check_late_invalid_scan_geometry_rejects_whole_scene() != 0)
        return 1;
    if (check_hud_radar_two_pass_order_and_scans() != 0)
        return 1;
    if (check_hud_radar_failure_flushes_only_accepted_prefix() != 0)
        return 1;
    if (check_hud_radar_object_overrides_are_independent() != 0)
        return 1;
    if (check_hud_radar_draw_and_blend_failures_are_sticky() != 0)
        return 1;
    if (check_invalid_hud_radar_math_is_sticky() != 0)
        return 1;
    if (check_hud_pointers_are_semantic() != 0)
        return 1;
    if (check_hud_pointer_visibility_conditions() != 0)
        return 1;
    if (check_pointer_failure_flushes_only_accepted_prefix() != 0)
        return 1;
    if (check_invalid_hud_pointer_geometry_is_rejected() != 0)
        return 1;
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
    if (check_hud_frame_paths_are_semantic() != 0)
        return 1;
    if (check_hud_frame_visibility_conditions() != 0)
        return 1;
    if (check_hud_frame_partial_failure_flushes_accepted_prefix() != 0)
        return 1;
    if (check_hud_frame_first_operation_failures_are_sticky() != 0)
        return 1;
    if (check_hud_frame_flush_failure_is_sticky() != 0)
        return 1;
    if (check_hud_lose_item_outline_precedes_count_text() != 0)
        return 1;
    if (check_hud_item_visibility_and_active_side_effects() != 0)
        return 1;
    if (check_hud_item_failures_stop_later_work() != 0)
        return 1;
    if (check_hud_fuel_visibility_table() != 0)
        return 1;
    if (check_hud_fuel_exact_and_unbounded_geometry() != 0)
        return 1;
    if (check_hud_fuel_invalid_math_and_partial_failure() != 0)
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
