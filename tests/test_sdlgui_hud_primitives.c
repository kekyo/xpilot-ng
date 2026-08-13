#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "guimap.h"
#include "guiobjects.h"
#include "renderer.h"
#include "images.h"
#include "sdlinit.h"
#include "sdlpaint.h"
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
    PAINT_EVENT_MAP_TEXT,
    PAINT_EVENT_MEASURE,
    PAINT_EVENT_HUD_TEXT,
    PAINT_EVENT_SETUP_MOVING,
    PAINT_EVENT_SETUP_STATIONARY
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

typedef struct FakeMapText {
    font_data *font;
    int color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    char text[50];
} FakeMapText;

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
static FakeMapText last_map_text;
static FakeHudText last_hud_text;
static image_t fake_asteroid_image;
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
static int image_lookup_attempts;
static int map_text_attempts;
static int measure_attempts;
static int hud_text_attempts;
static int setup_moving_attempts;
static int setup_stationary_attempts;
static int fake_paint_setup_mode;
static int legacy_begin_calls;
static int legacy_vertex_calls;
static int legacy_end_calls;
static int legacy_color_calls;
static int legacy_blend_calls;
static int legacy_enable_calls;
static int legacy_disable_calls;
static int legacy_line_stipple_calls;
static int legacy_line_width_calls;
static int legacy_texture_bind_calls;
static int legacy_light_calls;
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
static RendererStatus image_result;
static RendererStatus map_text_result;
static RendererStatus measure_result;
static RendererStatus hud_text_result;
static RendererStatus setup_moving_result;
static RendererStatus setup_stationary_result;
static float measured_width;
static float measured_height;

static setup_t fake_setup = {
    .frames_per_second = 60
};

setup_t *Setup = &fake_setup;
font_data gamefont;
font_data mapfont;
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
long loops;

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
extern Uint32 whiteRGBA;
extern Uint32 blueRGBA;
extern Uint32 redRGBA;
extern Uint32 wallColorRGBA;
extern Uint32 decorColorRGBA;
extern Uint32 fuelColorRGBA;
extern Uint32 connColorRGBA;
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

static int check_world_stroke(
    int stroke_index, const float expected_points[][2],
    size_t expected_point_count, uint32_t rgba, int closed,
    RendererBlendMode expected_blend, int expected_transform_token)
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
    TEST_CHECK(stroke->blend == expected_blend);
    TEST_CHECK(stroke->transform_token == expected_transform_token);
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

static int check_world_stippled_stroke(
    int stroke_index, const float expected_points[][2], uint32_t rgba,
    unsigned int expected_factor, int expected_transform_token)
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
    TEST_CHECK(stroke->factor == expected_factor);
    TEST_CHECK(stroke->pattern == UINT16_C(0xAAAA));
    TEST_CHECK(stroke->blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke->transform_token == expected_transform_token);
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

static int check_directional_gravity_draw(
    int draw_index, const float section_points[4][2][2],
    const uint32_t section_colors[4])
{
    static const unsigned char section_indices[18] = {
        0, 0, 1, 0, 1, 1,
        1, 1, 2, 1, 2, 2,
        2, 2, 3, 2, 3, 3
    };
    static const unsigned char side_indices[18] = {
        0, 1, 1, 0, 1, 0,
        0, 1, 1, 0, 1, 0,
        0, 1, 1, 0, 1, 0
    };
    const FakeDraw *draw;
    size_t vertex_index;

    TEST_CHECK(draw_index >= 0);
    TEST_CHECK(draw_index < successful_draws);
    draw = &draws[draw_index];
    TEST_CHECK(draw->texture == NULL);
    TEST_CHECK(draw->vertex_count == NELEM(section_indices));
    TEST_CHECK(draw->blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(draw->transform_token == 71);
    TEST_CHECK(draw->scissor_token == 83);
    for (vertex_index = 0; vertex_index < NELEM(section_indices);
         vertex_index++) {
        size_t section = section_indices[vertex_index];
        size_t side = side_indices[vertex_index];

        TEST_CHECK(draw->vertices[vertex_index].x
                   == section_points[section][side][0]);
        TEST_CHECK(draw->vertices[vertex_index].y
                   == section_points[section][side][1]);
        TEST_CHECK(draw->vertices[vertex_index].u == 0.0f);
        TEST_CHECK(draw->vertices[vertex_index].v == 0.0f);
        TEST_CHECK(color_equal(
            draw->vertices[vertex_index].color,
            Renderer_color_from_rgba32(section_colors[section])));
    }
    return 0;
}

static int check_directional_gravity_draws_equal(
    int first_index, int second_index)
{
    const FakeDraw *first;
    const FakeDraw *second;
    size_t vertex_index;

    TEST_CHECK(first_index >= 0 && first_index < successful_draws);
    TEST_CHECK(second_index >= 0 && second_index < successful_draws);
    first = &draws[first_index];
    second = &draws[second_index];
    TEST_CHECK(first->texture == second->texture);
    TEST_CHECK(first->vertex_count == second->vertex_count);
    TEST_CHECK(first->blend == second->blend);
    TEST_CHECK(first->transform_token == second->transform_token);
    TEST_CHECK(first->scissor_token == second->scissor_token);
    for (vertex_index = 0; vertex_index < first->vertex_count;
         vertex_index++) {
        TEST_CHECK(first->vertices[vertex_index].x
                   == second->vertices[vertex_index].x);
        TEST_CHECK(first->vertices[vertex_index].y
                   == second->vertices[vertex_index].y);
        TEST_CHECK(first->vertices[vertex_index].u
                   == second->vertices[vertex_index].u);
        TEST_CHECK(first->vertices[vertex_index].v
                   == second->vertices[vertex_index].v);
        TEST_CHECK(color_equal(
            first->vertices[vertex_index].color,
            second->vertices[vertex_index].color));
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
    memset(&last_map_text, 0, sizeof(last_map_text));
    memset(&last_hud_text, 0, sizeof(last_hud_text));
    memset(&fake_asteroid_image, 0, sizeof(fake_asteroid_image));
    memset(last_measured_text, 0, sizeof(last_measured_text));
    memset(&gamefont, 0, sizeof(gamefont));
    memset(&mapfont, 0, sizeof(mapfont));
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
    image_lookup_attempts = 0;
    map_text_attempts = 0;
    measure_attempts = 0;
    hud_text_attempts = 0;
    setup_moving_attempts = 0;
    setup_stationary_attempts = 0;
    fake_paint_setup_mode = STATIONARY_MODE;
    legacy_begin_calls = 0;
    legacy_vertex_calls = 0;
    legacy_end_calls = 0;
    legacy_color_calls = 0;
    legacy_blend_calls = 0;
    legacy_enable_calls = 0;
    legacy_disable_calls = 0;
    legacy_line_stipple_calls = 0;
    legacy_line_width_calls = 0;
    legacy_texture_bind_calls = 0;
    legacy_light_calls = 0;
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
    image_result = RENDERER_STATUS_OK;
    map_text_result = RENDERER_STATUS_OK;
    measure_result = RENDERER_STATUS_OK;
    hud_text_result = RENDERER_STATUS_OK;
    setup_moving_result = RENDERER_STATUS_OK;
    setup_stationary_result = RENDERER_STATUS_OK;
    measured_width = 7.0f;
    measured_height = 12.0f;
    fake_renderer.frame_active = 1;
    fake_renderer.transform_token = 71;
    fake_renderer.scissor_token = 83;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    fake_asteroid_image.legacy_name = 97;
    draw_width = 200;
    draw_height = 120;
    ext_view_width = 200;
    ext_view_height = 120;
    active_view_width = 200;
    active_view_height = 120;
    fake_setup.width = 256;
    fake_setup.height = 128;
    fake_setup.mode = 0;
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
    loops = 0;
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
    if (operation_result_pending > 0) {
        operation_result_pending--;
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

RendererStatus setupPaint_moving(void)
{
    setup_moving_attempts++;
    record_event(PAINT_EVENT_SETUP_MOVING);
    if (fake_sdl_renderer.frame_result != RENDERER_STATUS_OK)
        return fake_sdl_renderer.frame_result;
    if (setup_moving_result != RENDERER_STATUS_OK) {
        fake_sdl_renderer.frame_result = setup_moving_result;
        return setup_moving_result;
    }
    fake_paint_setup_mode = MOVING_MODE;
    fake_renderer.transform_token = 72;
    return RENDERER_STATUS_OK;
}

RendererStatus setupPaint_stationary(void)
{
    setup_stationary_attempts++;
    record_event(PAINT_EVENT_SETUP_STATIONARY);
    if (fake_sdl_renderer.frame_result != RENDERER_STATUS_OK)
        return fake_sdl_renderer.frame_result;
    if (setup_stationary_result != RENDERER_STATUS_OK) {
        fake_sdl_renderer.frame_result = setup_stationary_result;
        return setup_stationary_result;
    }
    fake_paint_setup_mode = STATIONARY_MODE;
    fake_renderer.transform_token = 71;
    return RENDERER_STATUS_OK;
}

RendererStatus setupPaint_stationary_cleanup(void)
{
    setup_stationary_attempts++;
    record_event(PAINT_EVENT_SETUP_STATIONARY);
    if (setup_stationary_result != RENDERER_STATUS_OK)
        return setup_stationary_result;
    fake_paint_setup_mode = STATIONARY_MODE;
    fake_renderer.transform_token = 71;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    blend_attempts++;
    if (blend_attempts <= MAX_EVENTS)
        blend_modes[blend_attempts - 1] = blend;
    operation_result_pending++;
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
    operation_result_pending++;
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
    operation_result_pending++;
    record_event(PAINT_EVENT_FILL);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!isfinite(x) || !isfinite(y)
        || !isfinite(width) || !isfinite(height)
        || !isfinite(x + width) || !isfinite(y + height)
        || width <= 0.0f || height <= 0.0f
        || x + width <= x || y + height <= y) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
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
    operation_result_pending++;
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
    operation_result_pending++;
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
    operation_result_pending++;
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
    operation_result_pending++;
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

void Image_paint(int index, int x, int y, int frame, int color)
{
    RendererStatus status = Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, RENDERER_STATUS_OK);

    if (status != RENDERER_STATUS_OK)
        return;
    image_attempts++;
    record_event(PAINT_EVENT_IMAGE);
    last_image.index = index;
    last_image.x = x;
    last_image.y = y;
    last_image.frame = frame;
    last_image.color = color;
    operation_result_pending++;
    (void)Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, image_result);
}

image_t *Image_get(int index)
{
    image_lookup_attempts++;
    return index == IMG_ASTEROID ? &fake_asteroid_image : NULL;
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

RendererStatus mapprint(font_data *font, int color,
                        int horizontal_alignment,
                        int vertical_alignment, int x, int y,
                        const char *format, ...)
{
    va_list arguments;

    map_text_attempts++;
    record_event(PAINT_EVENT_MAP_TEXT);
    last_map_text.font = font;
    last_map_text.color = color;
    last_map_text.horizontal_alignment = horizontal_alignment;
    last_map_text.vertical_alignment = vertical_alignment;
    last_map_text.x = x;
    last_map_text.y = y;
    va_start(arguments, format);
    vsnprintf(last_map_text.text, sizeof(last_map_text.text),
              format, arguments);
    va_end(arguments);
    return map_text_result;
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
    legacy_enable_calls++;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    (void)capability;
    legacy_blend_calls++;
    legacy_disable_calls++;
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

void GLAPIENTRY glLineWidth(GLfloat width)
{
    (void)width;
    legacy_line_width_calls++;
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

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    (void)texture;
    legacy_texture_bind_calls++;
}

void GLAPIENTRY glLightfv(GLenum light, GLenum parameter,
                          const GLfloat *values)
{
    (void)light;
    (void)parameter;
    (void)values;
    legacy_light_calls++;
}

static int check_no_legacy_primitives(void)
{
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_end_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    TEST_CHECK(legacy_line_stipple_calls == 0);
    TEST_CHECK(legacy_line_width_calls == 0);
    return 0;
}

typedef struct PaintOperationCounts {
    int event_count;
    int blend_attempts;
    int draw_attempts;
    int fill_attempts;
    int stroke_attempts;
    int stippled_stroke_attempts;
    int flush_attempts;
    int transform_attempts;
    int scissor_attempts;
    int text_attempts;
    int image_attempts;
    int image_lookup_attempts;
    int map_text_attempts;
    int measure_attempts;
    int hud_text_attempts;
    int setup_moving_attempts;
    int setup_stationary_attempts;
    int legacy_begin_calls;
    int legacy_vertex_calls;
    int legacy_end_calls;
    int legacy_color_calls;
    int legacy_blend_calls;
    int legacy_enable_calls;
    int legacy_disable_calls;
    int legacy_line_stipple_calls;
    int legacy_line_width_calls;
    int legacy_texture_bind_calls;
    int legacy_light_calls;
} PaintOperationCounts;

static PaintOperationCounts paint_operation_counts(void)
{
    PaintOperationCounts counts = {
        event_count,
        blend_attempts,
        draw_attempts,
        fill_attempts,
        stroke_attempts,
        stippled_stroke_attempts,
        flush_attempts,
        transform_attempts,
        scissor_attempts,
        text_attempts,
        image_attempts,
        image_lookup_attempts,
        map_text_attempts,
        measure_attempts,
        hud_text_attempts,
        setup_moving_attempts,
        setup_stationary_attempts,
        legacy_begin_calls,
        legacy_vertex_calls,
        legacy_end_calls,
        legacy_color_calls,
        legacy_blend_calls,
        legacy_enable_calls,
        legacy_disable_calls,
        legacy_line_stipple_calls,
        legacy_line_width_calls,
        legacy_texture_bind_calls,
        legacy_light_calls
    };

    return counts;
}

static int check_paint_operation_counts_unchanged(
    PaintOperationCounts before)
{
    PaintOperationCounts after = paint_operation_counts();

#define CHECK_OPERATION_COUNT(field) \
    TEST_CHECK(after.field == before.field)
    CHECK_OPERATION_COUNT(event_count);
    CHECK_OPERATION_COUNT(blend_attempts);
    CHECK_OPERATION_COUNT(draw_attempts);
    CHECK_OPERATION_COUNT(fill_attempts);
    CHECK_OPERATION_COUNT(stroke_attempts);
    CHECK_OPERATION_COUNT(stippled_stroke_attempts);
    CHECK_OPERATION_COUNT(flush_attempts);
    CHECK_OPERATION_COUNT(transform_attempts);
    CHECK_OPERATION_COUNT(scissor_attempts);
    CHECK_OPERATION_COUNT(text_attempts);
    CHECK_OPERATION_COUNT(image_attempts);
    CHECK_OPERATION_COUNT(image_lookup_attempts);
    CHECK_OPERATION_COUNT(map_text_attempts);
    CHECK_OPERATION_COUNT(measure_attempts);
    CHECK_OPERATION_COUNT(hud_text_attempts);
    CHECK_OPERATION_COUNT(setup_moving_attempts);
    CHECK_OPERATION_COUNT(setup_stationary_attempts);
    CHECK_OPERATION_COUNT(legacy_begin_calls);
    CHECK_OPERATION_COUNT(legacy_vertex_calls);
    CHECK_OPERATION_COUNT(legacy_end_calls);
    CHECK_OPERATION_COUNT(legacy_color_calls);
    CHECK_OPERATION_COUNT(legacy_blend_calls);
    CHECK_OPERATION_COUNT(legacy_enable_calls);
    CHECK_OPERATION_COUNT(legacy_disable_calls);
    CHECK_OPERATION_COUNT(legacy_line_stipple_calls);
    CHECK_OPERATION_COUNT(legacy_line_width_calls);
    CHECK_OPERATION_COUNT(legacy_texture_bind_calls);
    CHECK_OPERATION_COUNT(legacy_light_calls);
#undef CHECK_OPERATION_COUNT
    return 0;
}

static void preset_sticky_frame_failure(void)
{
    (void)Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, RENDERER_STATUS_BACKEND_ERROR);
}

static int check_sticky_failure_gates_direct_world_leaf(void)
{
    PaintOperationCounts before;
    int prior_preflight_attempts;

    reset_frame();
    preset_sticky_frame_failure();
    before = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_border(10, 20, 30, 40);

    TEST_CHECK(check_paint_operation_counts_unchanged(before) == 0);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_sticky_failure_gates_mixed_image_world_leaf(void)
{
    PaintOperationCounts before;
    int prior_preflight_attempts;

    reset_frame();
    preset_sticky_frame_failure();
    before = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_item_object(3, 20, 30);

    TEST_CHECK(check_paint_operation_counts_unchanged(before) == 0);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_sticky_failure_gates_setup_switching_world_leaf(void)
{
    PaintOperationCounts before;
    int prior_preflight_attempts;

    reset_frame();
    preset_sticky_frame_failure();
    before = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(check_paint_operation_counts_unchanged(before) == 0);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_sticky_failure_gates_semantic_and_text_world_leaf(void)
{
    PaintOperationCounts before;
    int prior_preflight_attempts;

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    preset_sticky_frame_failure();
    before = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_setup_target(10, 20, 7, 125.0, true);

    TEST_CHECK(check_paint_operation_counts_unchanged(before) == 0);
    TEST_CHECK(preflight_attempts > prior_preflight_attempts);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_sticky_failure_gates_inactive_asteroid_batch(void)
{
    PaintOperationCounts before;
    int prior_preflight_attempts;

    reset_frame();
    preset_sticky_frame_failure();
    before = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_asteroids_begin();
    Gui_paint_asteroids_end();

    TEST_CHECK(check_paint_operation_counts_unchanged(before) == 0);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 2);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_asteroid_batch_cleanup_survives_sticky_failure(void)
{
    PaintOperationCounts before_end;
    int prior_preflight_attempts;

    reset_frame();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_asteroids_begin();

    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(legacy_enable_calls > 0);
    TEST_CHECK(legacy_disable_calls == 0);
    before_end = paint_operation_counts();
    preset_sticky_frame_failure();
    prior_preflight_attempts = preflight_attempts;

    Gui_paint_asteroids_end();

    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(legacy_disable_calls == legacy_enable_calls);
    TEST_CHECK(event_count == before_end.event_count);
    TEST_CHECK(blend_attempts == before_end.blend_attempts);
    TEST_CHECK(draw_attempts == before_end.draw_attempts);
    TEST_CHECK(fill_attempts == before_end.fill_attempts);
    TEST_CHECK(stroke_attempts == before_end.stroke_attempts);
    TEST_CHECK(stippled_stroke_attempts
               == before_end.stippled_stroke_attempts);
    TEST_CHECK(flush_attempts == before_end.flush_attempts);
    TEST_CHECK(transform_attempts == before_end.transform_attempts);
    TEST_CHECK(scissor_attempts == before_end.scissor_attempts);
    TEST_CHECK(text_attempts == before_end.text_attempts);
    TEST_CHECK(image_attempts == before_end.image_attempts);
    TEST_CHECK(image_lookup_attempts == before_end.image_lookup_attempts);
    TEST_CHECK(map_text_attempts == before_end.map_text_attempts);
    TEST_CHECK(measure_attempts == before_end.measure_attempts);
    TEST_CHECK(hud_text_attempts == before_end.hud_text_attempts);
    TEST_CHECK(setup_moving_attempts == before_end.setup_moving_attempts);
    TEST_CHECK(setup_stationary_attempts
               == before_end.setup_stationary_attempts);
    TEST_CHECK(legacy_begin_calls == before_end.legacy_begin_calls);
    TEST_CHECK(legacy_vertex_calls == before_end.legacy_vertex_calls);
    TEST_CHECK(legacy_end_calls == before_end.legacy_end_calls);
    TEST_CHECK(legacy_color_calls == before_end.legacy_color_calls);
    TEST_CHECK(legacy_line_stipple_calls
               == before_end.legacy_line_stipple_calls);
    TEST_CHECK(legacy_line_width_calls
               == before_end.legacy_line_width_calls);
    TEST_CHECK(legacy_texture_bind_calls
               == before_end.legacy_texture_bind_calls);
    TEST_CHECK(legacy_light_calls == before_end.legacy_light_calls);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
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

static int check_no_world_legacy_state(void)
{
    if (check_no_legacy_primitives() != 0)
        return 1;
    TEST_CHECK(legacy_enable_calls == 0);
    TEST_CHECK(legacy_disable_calls == 0);
    TEST_CHECK(legacy_texture_bind_calls == 0);
    TEST_CHECK(legacy_light_calls == 0);
    return 0;
}

static int check_world_alpha_stroke_batch(int expected_stroke_count)
{
    int event_index;

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == expected_stroke_count + 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    for (event_index = 0; event_index < expected_stroke_count;
         event_index++) {
        TEST_CHECK(events[event_index + 1] == PAINT_EVENT_STROKE);
    }
    TEST_CHECK(events[event_count - 1] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == expected_stroke_count);
    TEST_CHECK(successful_strokes == expected_stroke_count);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(setup_moving_attempts == 0);
    TEST_CHECK(setup_stationary_attempts == 0);
    return check_no_world_legacy_state();
}

static int check_world_stippled_batch(void)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 1);
    TEST_CHECK(successful_stippled_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(setup_moving_attempts == 0);
    TEST_CHECK(setup_stationary_attempts == 0);
    return check_no_world_legacy_state();
}

static int check_world_decor_paths_are_semantic(void)
{
    static const float filled_lines[][2][2] = {
        {{10.0f, 20.0f}, {10.0f, 55.0f}},
        {{10.0f, 20.0f}, {45.0f, 20.0f}},
        {{45.0f, 20.0f}, {45.0f, 55.0f}},
        {{10.0f, 55.0f}, {45.0f, 55.0f}}
    };
    static const struct {
        int type;
        float lines[3][2][2];
    } corner_cases[] = {
        {
            SETUP_DECOR_RU,
            {
                {{45.0f, 20.0f}, {45.0f, 55.0f}},
                {{10.0f, 55.0f}, {45.0f, 55.0f}},
                {{10.0f, 55.0f}, {45.0f, 20.0f}}
            }
        },
        {
            SETUP_DECOR_RD,
            {
                {{10.0f, 20.0f}, {45.0f, 20.0f}},
                {{45.0f, 20.0f}, {45.0f, 55.0f}},
                {{10.0f, 20.0f}, {45.0f, 55.0f}}
            }
        },
        {
            SETUP_DECOR_LU,
            {
                {{10.0f, 20.0f}, {10.0f, 55.0f}},
                {{10.0f, 55.0f}, {45.0f, 55.0f}},
                {{10.0f, 20.0f}, {45.0f, 55.0f}}
            }
        },
        {
            SETUP_DECOR_LD,
            {
                {{10.0f, 20.0f}, {10.0f, 55.0f}},
                {{10.0f, 20.0f}, {45.0f, 20.0f}},
                {{10.0f, 55.0f}, {45.0f, 20.0f}}
            }
        }
    };
    const uint32_t color = UINT32_C(0x9a7b5c00);
    size_t case_index;
    int line_index;

    reset_frame();
    decorColorRGBA = color;
    Gui_paint_decor(
        10, 20, -777, 888, SETUP_DECOR_FILLED, true, true);

    TEST_CHECK(check_world_alpha_stroke_batch(4) == 0);
    for (line_index = 0; line_index < 4; line_index++) {
        TEST_CHECK(check_world_stroke(
            line_index, filled_lines[line_index], 2, color, 0,
            RENDERER_BLEND_ALPHA, 71) == 0);
    }

    for (case_index = 0; case_index < NELEM(corner_cases);
         case_index++) {
        reset_frame();
        decorColorRGBA = color;
        Gui_paint_decor(
            10, 20, 0, 0, corner_cases[case_index].type,
            false, false);

        TEST_CHECK(check_world_alpha_stroke_batch(3) == 0);
        for (line_index = 0; line_index < 3; line_index++) {
            TEST_CHECK(check_world_stroke(
                line_index, corner_cases[case_index].lines[line_index],
                2, color, 0, RENDERER_BLEND_ALPHA, 71) == 0);
        }
    }
    return 0;
}

static int check_world_border_rectangles_are_semantic(void)
{
    static const float border_points[][2] = {
        {-8.0f, 11.0f}, {17.0f, -23.0f}
    };
    static const float rectangle_points[][2] = {
        {-8.0f, 11.0f}, {-8.0f, -23.0f},
        {17.0f, -23.0f}, {17.0f, 11.0f}
    };
    static const float vertical_rectangle_points[][2] = {
        {5.0f, 11.0f}, {5.0f, -23.0f},
        {5.0f, -23.0f}, {5.0f, 11.0f}
    };

    reset_frame();
    wallColorRGBA = UINT32_C(0x10203000);
    Gui_paint_border(-8, 11, 17, -23);

    TEST_CHECK(check_world_alpha_stroke_batch(1) == 0);
    TEST_CHECK(check_world_stroke(
        0, border_points, 2, UINT32_C(0x10203000), 0,
        RENDERER_BLEND_ALPHA, 71) == 0);

    reset_frame();
    blueRGBA = UINT32_C(0x40506070);
    Gui_paint_hudradar_limit(-8, 11, 17, -23);

    TEST_CHECK(check_world_alpha_stroke_batch(1) == 0);
    TEST_CHECK(check_world_stroke(
        0, rectangle_points, 4, UINT32_C(0x40506070), 1,
        RENDERER_BLEND_ALPHA, 71) == 0);

    reset_frame();
    hudColorRGBA = UINT32_C(0x8090a0b0);
    Gui_paint_visible_border(-8, 11, 17, -23);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(setup_moving_attempts == 1);
    TEST_CHECK(setup_stationary_attempts == 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_paint_setup_mode == STATIONARY_MODE);
    TEST_CHECK(fake_renderer.transform_token == 71);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(check_world_stroke(
        0, rectangle_points, 4, UINT32_C(0x8090a0b0), 1,
        RENDERER_BLEND_ALPHA, 72) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    blueRGBA = UINT32_C(0x40506070);
    Gui_paint_hudradar_limit(5, 11, 5, -23);

    TEST_CHECK(check_world_alpha_stroke_batch(1) == 0);
    return check_world_stroke(
        0, vertical_rectangle_points, 4, UINT32_C(0x40506070), 1,
        RENDERER_BLEND_ALPHA, 71);
}

static int check_world_wall_paths_and_fuel_precedence(void)
{
    static const float all_lines[][2][2] = {
        {{10.0f, 20.0f}, {10.0f, 55.0f}},
        {{10.0f, 20.0f}, {45.0f, 20.0f}},
        {{45.0f, 20.0f}, {45.0f, 55.0f}},
        {{10.0f, 55.0f}, {45.0f, 55.0f}},
        {{10.0f, 20.0f}, {45.0f, 55.0f}}
    };
    static const float closed_line[][2] = {
        {10.0f, 55.0f}, {45.0f, 20.0f}
    };
    const uint32_t color = UINT32_C(0x12345600);
    int line_index;

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_walls(
        10, 20,
        BLUE_LEFT | BLUE_DOWN | BLUE_RIGHT | BLUE_UP | BLUE_OPEN);

    TEST_CHECK(check_world_alpha_stroke_batch(5) == 0);
    for (line_index = 0; line_index < 5; line_index++) {
        TEST_CHECK(check_world_stroke(
            line_index, all_lines[line_index], 2, color, 0,
            RENDERER_BLEND_ALPHA, 71) == 0);
    }

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_walls(
        10, 20,
        BLUE_LEFT | BLUE_DOWN | BLUE_RIGHT | BLUE_UP | BLUE_FUEL);

    TEST_CHECK(check_world_alpha_stroke_batch(4) == 0);
    for (line_index = 0; line_index < 4; line_index++) {
        TEST_CHECK(check_world_stroke(
            line_index, all_lines[line_index], 2, color, 0,
            RENDERER_BLEND_ALPHA, 71) == 0);
    }

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_walls(10, 20, BLUE_CLOSED);

    TEST_CHECK(check_world_alpha_stroke_batch(1) == 0);
    return check_world_stroke(
        0, closed_line, 2, color, 0, RENDERER_BLEND_ALPHA, 71);
}

static int check_world_stippled_lines_use_no_legacy_smoothing_state(void)
{
    static const float points[][2] = {
        {-100.0f, 250.0f}, {300.0f, -450.0f}
    };

    reset_frame();
    fuelColorRGBA = UINT32_C(0x31415900);
    Gui_paint_refuel(-100, 250, 300, -450);

    TEST_CHECK(check_world_stippled_batch() == 0);
    TEST_CHECK(check_world_stippled_stroke(
        0, points, UINT32_C(0x31415900), 4, 71) == 0);

    reset_frame();
    connColorRGBA = UINT32_C(0x27182818);
    Gui_paint_connector(-100, 250, 300, -450, 0);

    TEST_CHECK(check_world_stippled_batch() == 0);
    TEST_CHECK(check_world_stippled_stroke(
        0, points, UINT32_C(0x27182818), 4, 71) == 0);

    reset_frame();
    connColorRGBA = UINT32_C(0x27182818);
    Gui_paint_connector(-100, 250, 300, -450, 1);

    TEST_CHECK(check_world_stippled_batch() == 0);
    return check_world_stippled_stroke(
        0, points, UINT32_C(0x27182818), 2, 71);
}

static int check_world_line_preflight_only_noop(void)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(setup_moving_attempts == 0);
    TEST_CHECK(setup_stationary_attempts == 0);
    return check_no_world_legacy_state();
}

static int check_world_line_empty_geometry_is_successful_noop(void)
{
    static const int empty_wall_types[] = {
        0, BLUE_BELOW, BLUE_FUEL
    };
    size_t type_index;

    reset_frame();
    Gui_paint_decor(10, 20, 0, 0, 0, false, false);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_decor(
        INT_MAX, INT_MAX, 0, 0, 0, false, false);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    for (type_index = 0; type_index < NELEM(empty_wall_types);
         type_index++) {
        reset_frame();
        Gui_paint_walls(10, 20, empty_wall_types[type_index]);
        TEST_CHECK(check_world_line_preflight_only_noop() == 0);
    }

    reset_frame();
    Gui_paint_walls(INT_MAX, INT_MAX, 0);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_border(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_refuel(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_connector(INT_MAX, INT_MIN, INT_MAX, INT_MIN, 1);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_hudradar_limit(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    TEST_CHECK(check_world_line_preflight_only_noop() == 0);

    reset_frame();
    Gui_paint_visible_border(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    return check_world_line_preflight_only_noop();
}

static int check_invalid_world_line_result_is_sticky(void)
{
    PaintOperationCounts before_blocked_leaf;

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(setup_moving_attempts == 0);
    TEST_CHECK(setup_stationary_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    before_blocked_leaf = paint_operation_counts();
    Gui_paint_border(10, 20, 30, 40);
    return check_paint_operation_counts_unchanged(before_blocked_leaf);
}

static int check_world_line_numeric_boundaries_are_checked(void)
{
    static const int invalid_decor_types[] = {-1, 256};
    static const float int_min_points[][2] = {
        {(float)INT_MIN, (float)INT_MIN},
        {(float)(INT_MIN + 256), (float)(INT_MIN + 256)}
    };
    size_t type_index;

    for (type_index = 0; type_index < NELEM(invalid_decor_types);
         type_index++) {
        reset_frame();
        Gui_paint_decor(
            10, 20, 0, 0, invalid_decor_types[type_index],
            false, false);
        TEST_CHECK(check_invalid_world_line_result_is_sticky() == 0);
    }

    reset_frame();
    Gui_paint_walls(INT_MAX, 20, BLUE_RIGHT);
    TEST_CHECK(check_invalid_world_line_result_is_sticky() == 0);

    reset_frame();
    Gui_paint_border(INT_MAX - 1, 20, INT_MAX, 20);
    TEST_CHECK(check_invalid_world_line_result_is_sticky() == 0);

    reset_frame();
    Gui_paint_refuel(INT_MAX - 1, 20, INT_MAX, 20);
    TEST_CHECK(check_invalid_world_line_result_is_sticky() == 0);

    reset_frame();
    Gui_paint_visible_border(INT_MAX - 1, 20, INT_MAX, 40);
    TEST_CHECK(check_invalid_world_line_result_is_sticky() == 0);

    reset_frame();
    wallColorRGBA = UINT32_C(0x01020304);
    Gui_paint_border(
        INT_MIN, INT_MIN, INT_MIN + 256, INT_MIN + 256);
    TEST_CHECK(check_world_alpha_stroke_batch(1) == 0);
    return check_world_stroke(
        0, int_min_points, 2, UINT32_C(0x01020304), 0,
        RENDERER_BLEND_ALPHA, 71);
}

static int check_world_line_failures_preserve_prefix_and_sticky_status(void)
{
    static const float first_wall_line[][2] = {
        {10.0f, 20.0f}, {10.0f, 55.0f}
    };
    static const float second_wall_line[][2] = {
        {10.0f, 20.0f}, {45.0f, 20.0f}
    };
    static const float border_points[][2] = {
        {10.0f, 20.0f}, {30.0f, 40.0f}
    };
    PaintOperationCounts before_blocked_leaf;

    reset_frame();
    wallColorRGBA = UINT32_C(0x12345678);
    stroke_failure_attempt = 3;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_walls(
        10, 20,
        BLUE_LEFT | BLUE_DOWN | BLUE_RIGHT | BLUE_UP | BLUE_OPEN);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(stroke_attempts == 3);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_world_stroke(
        0, first_wall_line, 2, UINT32_C(0x12345678), 0,
        RENDERER_BLEND_ALPHA, 71) == 0);
    TEST_CHECK(check_world_stroke(
        1, second_wall_line, 2, UINT32_C(0x12345678), 0,
        RENDERER_BLEND_ALPHA, 71) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    before_blocked_leaf = paint_operation_counts();
    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_connector(1, 2, 3, 4, 0);
    TEST_CHECK(check_paint_operation_counts_unchanged(
        before_blocked_leaf) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);

    reset_frame();
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_refuel(1, 2, 3, 4);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    stippled_stroke_failure_attempt = 1;
    stippled_stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_connector(1, 2, 3, 4, 1);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STIPPLED_STROKE);
    TEST_CHECK(stippled_stroke_attempts == 1);
    TEST_CHECK(successful_stippled_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    wallColorRGBA = UINT32_C(0xabcdef12);
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_world_stroke(
        0, border_points, 2, UINT32_C(0xabcdef12), 0,
        RENDERER_BLEND_ALPHA, 71) == 0);
    return check_no_world_legacy_state();
}

static int check_directional_gravity_geometry_and_order(void)
{
    static const float direction_sections[4][4][2][2] = {
        {
            {{100.0f, 200.0f}, {135.0f, 200.0f}},
            {{100.0f, 205.0f}, {135.0f, 205.0f}},
            {{100.0f, 222.0f}, {135.0f, 222.0f}},
            {{100.0f, 235.0f}, {135.0f, 235.0f}}
        },
        {
            {{100.0f, 235.0f}, {135.0f, 235.0f}},
            {{100.0f, 230.0f}, {135.0f, 230.0f}},
            {{100.0f, 213.0f}, {135.0f, 213.0f}},
            {{100.0f, 200.0f}, {135.0f, 200.0f}}
        },
        {
            {{100.0f, 200.0f}, {100.0f, 235.0f}},
            {{105.0f, 200.0f}, {105.0f, 235.0f}},
            {{122.0f, 200.0f}, {122.0f, 235.0f}},
            {{135.0f, 200.0f}, {135.0f, 235.0f}}
        },
        {
            {{135.0f, 200.0f}, {135.0f, 235.0f}},
            {{130.0f, 200.0f}, {130.0f, 235.0f}},
            {{113.0f, 200.0f}, {113.0f, 235.0f}},
            {{100.0f, 200.0f}, {100.0f, 235.0f}}
        }
    };
    static const uint32_t section_colors[4] = {
        UINT32_C(0x11223325),
        UINT32_C(0x11223300),
        UINT32_C(0x11223380),
        UINT32_C(0x11223325)
    };
    static void (*const painters[4])(int, int) = {
        Gui_paint_setup_up_grav,
        Gui_paint_setup_down_grav,
        Gui_paint_setup_right_grav,
        Gui_paint_setup_left_grav
    };
    size_t direction;

    reset_frame();
    loops = 5;
    /* The animated alpha replaces, rather than borrows from, packed alpha. */
    redRGBA = UINT32_C(0x11223300);
    for (direction = 0; direction < NELEM(painters); direction++)
        painters[direction](100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 4);
    TEST_CHECK(event_count == 12);
    TEST_CHECK(blend_attempts == 4);
    TEST_CHECK(draw_attempts == 4);
    TEST_CHECK(successful_draws == 4);
    TEST_CHECK(flush_attempts == 4);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    for (direction = 0; direction < NELEM(painters); direction++) {
        size_t event = direction * 3;

        TEST_CHECK(events[event] == PAINT_EVENT_BLEND);
        TEST_CHECK(events[event + 1] == PAINT_EVENT_TRIANGLES);
        TEST_CHECK(events[event + 2] == PAINT_EVENT_FLUSH);
        TEST_CHECK(blend_modes[direction] == RENDERER_BLEND_ALPHA);
        TEST_CHECK(check_directional_gravity_draw(
            (int)direction, direction_sections[direction],
            section_colors) == 0);
    }
    return check_no_world_legacy_state();
}

static int check_directional_gravity_phase_boundaries_and_negative(void)
{
    static const float phase_17_sections[4][2][2] = {
        {{100.0f, 200.0f}, {135.0f, 200.0f}},
        {{100.0f, 217.0f}, {135.0f, 217.0f}},
        {{100.0f, 234.0f}, {135.0f, 234.0f}},
        {{100.0f, 235.0f}, {135.0f, 235.0f}}
    };
    static const uint32_t phase_17_colors[4] = {
        UINT32_C(0x44556680),
        UINT32_C(0x44556600),
        UINT32_C(0x44556680),
        UINT32_C(0x44556680)
    };
    static const float phase_18_sections[4][2][2] = {
        {{100.0f, 200.0f}, {135.0f, 200.0f}},
        {{100.0f, 200.0f}, {135.0f, 200.0f}},
        {{100.0f, 218.0f}, {135.0f, 218.0f}},
        {{100.0f, 235.0f}, {135.0f, 235.0f}}
    };
    static const uint32_t phase_18_colors[4] = {
        UINT32_C(0x44556680),
        UINT32_C(0x44556680),
        UINT32_C(0x44556600),
        UINT32_C(0x44556680)
    };
    long normalized_extreme;

    reset_frame();
    redRGBA = UINT32_C(0x44556600);
    loops = 17;
    Gui_paint_setup_up_grav(100, 200);
    loops = 18;
    Gui_paint_setup_up_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(draw_attempts == 2);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 2);
    TEST_CHECK(check_directional_gravity_draw(
        0, phase_17_sections, phase_17_colors) == 0);
    TEST_CHECK(check_directional_gravity_draw(
        1, phase_18_sections, phase_18_colors) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    redRGBA = UINT32_C(0x77889900);
    loops = -1;
    Gui_paint_setup_right_grav(100, 200);
    loops = 34;
    Gui_paint_setup_right_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 2);
    TEST_CHECK(check_directional_gravity_draws_equal(0, 1) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    normalized_extreme = LONG_MIN % (long)BLOCK_SZ;
    if (normalized_extreme < 0)
        normalized_extreme += BLOCK_SZ;
    reset_frame();
    redRGBA = UINT32_C(0xaabbcc00);
    loops = LONG_MIN;
    Gui_paint_setup_down_grav(100, 200);
    loops = normalized_extreme;
    Gui_paint_setup_down_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 2);
    TEST_CHECK(check_directional_gravity_draws_equal(0, 1) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    normalized_extreme = LONG_MAX % (long)BLOCK_SZ;
    reset_frame();
    redRGBA = UINT32_C(0xddeeff00);
    loops = LONG_MAX;
    Gui_paint_setup_left_grav(100, 200);
    loops = normalized_extreme;
    Gui_paint_setup_left_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(successful_draws == 2);
    TEST_CHECK(flush_attempts == 2);
    TEST_CHECK(check_directional_gravity_draws_equal(0, 1) == 0);
    return check_no_world_legacy_state();
}

static int check_invalid_directional_gravity_result_is_sticky(void)
{
    PaintOperationCounts before_blocked_leaf;

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    before_blocked_leaf = paint_operation_counts();
    Gui_paint_setup_down_grav(10, 20);
    TEST_CHECK(check_paint_operation_counts_unchanged(
        before_blocked_leaf) == 0);
    return fake_sdl_renderer.frame_result == RENDERER_STATUS_INVALID_ARGUMENT
        ? 0 : 1;
}

static int check_directional_gravity_numeric_boundaries_are_checked(void)
{
    reset_frame();
    Gui_paint_setup_up_grav(INT_MAX, 20);
    TEST_CHECK(check_invalid_directional_gravity_result_is_sticky() == 0);

    reset_frame();
    Gui_paint_setup_right_grav(20, INT_MAX);
    TEST_CHECK(check_invalid_directional_gravity_result_is_sticky() == 0);

    /* These remain in int range but lose a 35-unit extent as float. */
    reset_frame();
    Gui_paint_setup_down_grav(INT_MIN, 20);
    TEST_CHECK(check_invalid_directional_gravity_result_is_sticky() == 0);

    reset_frame();
    Gui_paint_setup_left_grav(20, INT_MIN);
    return check_invalid_directional_gravity_result_is_sticky();
}

static int check_directional_gravity_failures_are_sticky(void)
{
    PaintOperationCounts before_blocked_leaf;
    int prior_preflight_attempts;

    reset_frame();
    loops = 5;
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_setup_up_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);
    before_blocked_leaf = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;
    Gui_paint_setup_right_grav(100, 200);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(check_paint_operation_counts_unchanged(
        before_blocked_leaf) == 0);

    reset_frame();
    loops = 5;
    draw_failure_attempt = 1;
    draw_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_setup_down_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    loops = 5;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_setup_left_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    preset_sticky_frame_failure();
    before_blocked_leaf = paint_operation_counts();
    prior_preflight_attempts = preflight_attempts;
    Gui_paint_setup_up_grav(100, 200);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == prior_preflight_attempts + 1);
    TEST_CHECK(check_paint_operation_counts_unchanged(
        before_blocked_leaf) == 0);
    return check_no_world_legacy_state();
}

static int check_visible_border_setup_failures_restore_safely(void)
{
    static const float rectangle_points[][2] = {
        {10.0f, 20.0f}, {10.0f, 40.0f},
        {30.0f, 40.0f}, {30.0f, 20.0f}
    };

    reset_frame();
    setup_moving_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(setup_moving_attempts == 1);
    TEST_CHECK(setup_stationary_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_paint_setup_mode == STATIONARY_MODE);
    TEST_CHECK(fake_renderer.transform_token == 71);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    hudColorRGBA = UINT32_C(0x12345678);
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(setup_moving_attempts == 1);
    TEST_CHECK(setup_stationary_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_paint_setup_mode == STATIONARY_MODE);
    TEST_CHECK(fake_renderer.transform_token == 71);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    hudColorRGBA = UINT32_C(0x12345678);
    setup_stationary_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(setup_moving_attempts == 1);
    TEST_CHECK(setup_stationary_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_paint_setup_mode == MOVING_MODE);
    TEST_CHECK(fake_renderer.transform_token == 72);
    TEST_CHECK(check_world_stroke(
        0, rectangle_points, 4, UINT32_C(0x12345678), 1,
        RENDERER_BLEND_ALPHA, 72) == 0);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(setup_stationary_attempts == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_paint_setup_mode == STATIONARY_MODE);
    TEST_CHECK(fake_renderer.transform_token == 71);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_paint_setup_mode == STATIONARY_MODE);
    TEST_CHECK(fake_renderer.transform_token == 71);
    TEST_CHECK(check_no_world_legacy_state() == 0);

    reset_frame();
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    setup_stationary_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_visible_border(10, 20, 30, 40);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_SETUP_MOVING);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_SETUP_STATIONARY);
    TEST_CHECK(setup_stationary_attempts == 1);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_world_legacy_state();
}

static int check_laser_stroke(
    int stroke_index, const float expected_points[][2],
    float expected_width, uint32_t rgba)
{
    const FakeStroke *stroke;

    TEST_CHECK(stroke_index >= 0);
    TEST_CHECK(stroke_index < successful_strokes);
    stroke = &strokes[stroke_index];
    TEST_CHECK(stroke->point_count == 2);
    TEST_CHECK(stroke->points[0].x == expected_points[0][0]);
    TEST_CHECK(stroke->points[0].y == expected_points[0][1]);
    TEST_CHECK(stroke->points[1].x == expected_points[1][0]);
    TEST_CHECK(stroke->points[1].y == expected_points[1][1]);
    TEST_CHECK(stroke->width == expected_width);
    TEST_CHECK(color_equal(
        stroke->color, Renderer_color_from_rgba32(rgba)));
    TEST_CHECK(stroke->closed == 0);
    TEST_CHECK(stroke->blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke->transform_token == 71);
    TEST_CHECK(stroke->scissor_token == 83);
    return 0;
}

static int check_laser_batch_is_semantic(void)
{
    static const float first_points[][2] = {
        {10.0f, 20.0f}, {15.0f, 16.0f}
    };
    static const float second_points[][2] = {
        {-2.0f, 3.0f}, {-5.0f, 5.0f}
    };
    static const float third_points[][2] = {
        {0.0f, 0.0f}, {3.0f, -2.0f}
    };

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    blueRGBA = UINT32_C(0x445566aa);
    whiteRGBA = UINT32_C(0x77889980);
    tbl_cos[7] = 0.75;
    tbl_sin[7] = -0.5;

    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_laser(BLUE, -2, 3, -5, 7);
    Gui_paint_laser(99, 0, 0, 4, 7);
    Gui_paint_lasers_end();

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 8);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[5] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[6] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[7] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == 6);
    TEST_CHECK(successful_strokes == 6);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_laser_stroke(
        0, first_points, 5.0f, UINT32_C(0x1122337f)) == 0);
    TEST_CHECK(check_laser_stroke(
        1, first_points, 1.0f, UINT32_C(0x112233ff)) == 0);
    TEST_CHECK(check_laser_stroke(
        2, second_points, 5.0f, UINT32_C(0x4455662a)) == 0);
    TEST_CHECK(check_laser_stroke(
        3, second_points, 1.0f, UINT32_C(0x445566aa)) == 0);
    TEST_CHECK(check_laser_stroke(
        4, third_points, 5.0f, UINT32_C(0x77889900)) == 0);
    TEST_CHECK(check_laser_stroke(
        5, third_points, 1.0f, UINT32_C(0x77889980)) == 0);
    return check_no_legacy_primitives();
}

static int check_empty_and_first_failure_laser_batches(void)
{
    reset_frame();
    Gui_paint_lasers_begin();
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    tbl_cos[7] = 0.75;
    tbl_sin[7] = -0.5;
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_primitives();
}

static int check_laser_failure_flushes_accepted_prefix(void)
{
    static const float points[][2] = {
        {10.0f, 20.0f}, {15.0f, 16.0f}
    };
    int prior_event_count;
    int prior_blend_attempts;
    int prior_stroke_attempts;
    int prior_flush_attempts;

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    tbl_cos[7] = 0.75;
    tbl_sin[7] = -0.5;
    stroke_failure_attempt = 2;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;

    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_laser_stroke(
        0, points, 5.0f, UINT32_C(0x1122337f)) == 0);

    prior_event_count = event_count;
    prior_blend_attempts = blend_attempts;
    prior_stroke_attempts = stroke_attempts;
    prior_flush_attempts = flush_attempts;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == prior_event_count);
    TEST_CHECK(blend_attempts == prior_blend_attempts);
    TEST_CHECK(stroke_attempts == prior_stroke_attempts);
    TEST_CHECK(flush_attempts == prior_flush_attempts);
    return check_no_legacy_primitives();
}

static int check_laser_flush_failure_is_sticky(void)
{
    int prior_event_count;
    int prior_stroke_attempts;
    int prior_flush_attempts;

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    tbl_cos[7] = 0.75;
    tbl_sin[7] = -0.5;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;

    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(successful_strokes == 2);
    TEST_CHECK(flush_attempts == 1);

    prior_event_count = event_count;
    prior_stroke_attempts = stroke_attempts;
    prior_flush_attempts = flush_attempts;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == prior_event_count);
    TEST_CHECK(stroke_attempts == prior_stroke_attempts);
    TEST_CHECK(flush_attempts == prior_flush_attempts);
    return check_no_legacy_primitives();
}

static int check_invalid_laser_geometry_is_sticky(void)
{
    reset_frame();
    redRGBA = UINT32_C(0x112233ff);

    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, -1);
    Gui_paint_lasers_end();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, TABLE_SIZE);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    tbl_cos[7] = 1.0;
    tbl_sin[7] = 0.0;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, INT_MAX, 20, INT_MAX, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    redRGBA = UINT32_C(0x112233ff);
    tbl_cos[7] = NAN;
    tbl_sin[7] = 0.0;
    Gui_paint_lasers_begin();
    Gui_paint_laser(RED, 10, 20, 7, 7);
    Gui_paint_lasers_end();
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_primitives();
}

static int check_single_map_fill(
    const float expected_points[][2], uint32_t rgba)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    TEST_CHECK(check_draw(
        0, expected_points, 6, rgba, RENDERER_BLEND_ALPHA) == 0);
    return check_no_legacy_primitives();
}

static int check_map_constant_fills_are_semantic(void)
{
    static const float decor_dot[][2] = {
        {24.0f, 10.0f}, {24.0f, 16.0f}, {30.0f, 16.0f},
        {24.0f, 10.0f}, {30.0f, 16.0f}, {30.0f, 10.0f}
    };
    static const float filled_slice[][2] = {
        {-4.0f, 20.0f}, {1.0f, 55.0f}, {9.0f, 55.0f},
        {-4.0f, 20.0f}, {9.0f, 55.0f}, {13.0f, 20.0f}
    };
    const uint32_t transparent_color = UINT32_C(0x12345600);

    reset_frame();
    wallColorRGBA = transparent_color;
    Gui_paint_decor_dot(10, -4, 6);
    TEST_CHECK(check_single_map_fill(decor_dot, transparent_color) == 0);

    reset_frame();
    wallColorRGBA = transparent_color;
    Gui_paint_filled_slice(-4, 1, 9, 13, 20);
    return check_single_map_fill(filled_slice, transparent_color);
}

static int check_map_fill_degenerate_geometry_is_preserved(void)
{
    static const float zero_size_dot[][2] = {
        {14.0f, 25.0f}, {14.0f, 25.0f}, {14.0f, 25.0f},
        {14.0f, 25.0f}, {14.0f, 25.0f}, {14.0f, 25.0f}
    };
    static const float negative_size_dot[][2] = {
        {16.0f, 27.0f}, {16.0f, 23.0f}, {12.0f, 23.0f},
        {16.0f, 27.0f}, {12.0f, 23.0f}, {12.0f, 27.0f}
    };
    static const float negative_odd_half_dot[][2] = {
        {33.0f, 44.0f}, {33.0f, 6.0f}, {-5.0f, 6.0f},
        {33.0f, 44.0f}, {-5.0f, 6.0f}, {-5.0f, 44.0f}
    };
    static const float degenerate_slice[][2] = {
        {7.0f, -35.0f}, {7.0f, 0.0f}, {7.0f, 0.0f},
        {7.0f, -35.0f}, {7.0f, 0.0f}, {7.0f, -35.0f}
    };
    const uint32_t color = UINT32_C(0xabcdef00);

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_decor_dot(-3, 8, 0);
    TEST_CHECK(check_single_map_fill(zero_size_dot, color) == 0);

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_decor_dot(-3, 8, -4);
    TEST_CHECK(check_single_map_fill(negative_size_dot, color) == 0);

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_decor_dot(-3, 8, -38);
    TEST_CHECK(check_single_map_fill(negative_odd_half_dot, color) == 0);

    reset_frame();
    wallColorRGBA = color;
    Gui_paint_filled_slice(7, 7, 7, 7, -35);
    return check_single_map_fill(degenerate_slice, color);
}

static int check_map_fill_failures_are_sticky(void)
{
    reset_frame();
    wallColorRGBA = UINT32_C(0x12345600);
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_decor_dot(10, -4, 6);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    Gui_paint_filled_slice(-4, 1, 9, 13, 20);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    wallColorRGBA = UINT32_C(0x12345600);
    draw_failure_attempt = 1;
    draw_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_filled_slice(-4, 1, 9, 13, 20);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    Gui_paint_decor_dot(10, -4, 6);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    wallColorRGBA = UINT32_C(0x12345600);
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_decor_dot(10, -4, 6);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[1] == PAINT_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == PAINT_EVENT_FLUSH);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    Gui_paint_filled_slice(-4, 1, 9, 13, 20);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    return check_no_legacy_primitives();
}

static int check_target_image(void)
{
    TEST_CHECK(last_image.index == IMG_TARGET);
    TEST_CHECK(last_image.x == 10);
    TEST_CHECK(last_image.y == 20);
    TEST_CHECK(last_image.frame == 0);
    TEST_CHECK(last_image.color == (int)UINT32_C(0x11223344));
    return 0;
}

static int check_target_outline(uint32_t color)
{
    static const float expected_points[][2] = {
        {10.0f, 23.0f}, {10.0f, 55.0f},
        {15.0f, 55.0f}, {15.0f, 23.0f}
    };

    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    return check_stroke(0, expected_points, 4, color, 1);
}

static int check_target_fill(
    float expected_y, float expected_height, uint32_t color)
{
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(last_fill.x == 10.0f);
    TEST_CHECK(last_fill.y == expected_y);
    TEST_CHECK(last_fill.width == 5.0f);
    TEST_CHECK(last_fill.height == expected_height);
    TEST_CHECK(color_equal(
        last_fill.color, Renderer_color_from_rgba32(color)));
    TEST_CHECK(last_fill.blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(last_fill.transform_token == 71);
    TEST_CHECK(last_fill.scissor_token == 83);
    return 0;
}

static int check_target_primitives(
    float expected_fill_y, float expected_fill_height, uint32_t color)
{
    TEST_CHECK(check_target_image() == 0);
    TEST_CHECK(check_target_outline(color) == 0);
    TEST_CHECK(check_target_fill(
        expected_fill_y, expected_fill_height, color) == 0);
    TEST_CHECK(transform_attempts == 0);
    TEST_CHECK(scissor_attempts == 0);
    return check_no_legacy_primitives();
}

static int check_target_full_order_and_geometry(void)
{
    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    whiteRGBA = UINT32_C(0x11223344);
    blueRGBA = UINT32_C(0x55667788);

    Gui_paint_setup_target(10, 20, 7, 125.0, true);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 3);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_MAP_TEXT);
    TEST_CHECK(events[2] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[3] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[4] == PAINT_EVENT_FILL);
    TEST_CHECK(events[5] == PAINT_EVENT_FLUSH);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(map_text_attempts == 1);
    TEST_CHECK(last_map_text.font == &mapfont);
    TEST_CHECK(last_map_text.color == (int)UINT32_C(0x11223344));
    TEST_CHECK(last_map_text.horizontal_alignment == RIGHT);
    TEST_CHECK(last_map_text.vertical_alignment == UP);
    TEST_CHECK(last_map_text.x == 45);
    TEST_CHECK(last_map_text.y == 20);
    TEST_CHECK(strcmp(last_map_text.text, "7") == 0);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(blend_modes[0] == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    return check_target_primitives(
        23.0f, 13.0f, UINT32_C(0x55667788));
}

static int check_target_optional_paths_and_own_color(void)
{
    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    redRGBA = UINT32_C(0xa1b2c3d4);

    Gui_paint_setup_target(10, 20, 9, 0.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FILL);
    TEST_CHECK(events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(map_text_attempts == 0);
    TEST_CHECK(check_target_primitives(
        20.0f, 3.0f, UINT32_C(0xa1b2c3d4)) == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(10, 20, -3, TARGET_DAMAGE, true);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(preflight_attempts == 3);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_MAP_TEXT);
    TEST_CHECK(strcmp(last_map_text.text, "-3") == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_target_image() == 0);
    return check_no_legacy_primitives();
}

static int check_target_unbounded_finite_damage(void)
{
    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    blueRGBA = UINT32_C(0x55667788);

    Gui_paint_setup_target(10, -4, 0, 1.0, true);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(last_image.index == IMG_TARGET);
    TEST_CHECK(last_image.x == 10);
    TEST_CHECK(last_image.y == -4);
    TEST_CHECK(last_image.frame == 0);
    TEST_CHECK(last_image.color == (int)UINT32_C(0x11223344));
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(strokes[0].points[0].x == 10.0f);
    TEST_CHECK(strokes[0].points[0].y == -1.0f);
    TEST_CHECK(strokes[0].points[1].x == 10.0f);
    TEST_CHECK(strokes[0].points[1].y == 31.0f);
    TEST_CHECK(strokes[0].points[2].x == 15.0f);
    TEST_CHECK(strokes[0].points[2].y == 31.0f);
    TEST_CHECK(strokes[0].points[3].x == 15.0f);
    TEST_CHECK(strokes[0].points[3].y == -1.0f);
    TEST_CHECK(check_target_fill(
        -4.0f, 3.0f, UINT32_C(0x55667788)) == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    blueRGBA = UINT32_C(0x55667788);

    Gui_paint_setup_target(10, 20, 0, 23.4375, true);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_BLEND);
    TEST_CHECK(events[2] == PAINT_EVENT_STROKE);
    TEST_CHECK(events[3] == PAINT_EVENT_FLUSH);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(check_target_image() == 0);
    TEST_CHECK(check_target_outline(UINT32_C(0x55667788)) == 0);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    redRGBA = UINT32_C(0xa1b2c3d4);

    Gui_paint_setup_target(10, 20, 0, -125.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(check_target_primitives(
        4.0f, 19.0f, UINT32_C(0xa1b2c3d4)) == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    redRGBA = UINT32_C(0xa1b2c3d4);

    Gui_paint_setup_target(10, 20, 0, -1.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(check_target_primitives(
        20.0f, 3.0f, UINT32_C(0xa1b2c3d4)) == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);
    redRGBA = UINT32_C(0xa1b2c3d4);

    Gui_paint_setup_target(10, 20, 0, 500.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    return check_target_primitives(
        23.0f, 61.0f, UINT32_C(0xa1b2c3d4));
}

static int check_target_invalid_damage_is_sticky(void)
{
    static const double invalid_damage[] = {
        NAN, INFINITY, -INFINITY,
        ((double)INT_MAX + 1.0) * TARGET_DAMAGE / (BLOCK_SZ - 3),
        ((double)INT_MIN - 1.0) * TARGET_DAMAGE / (BLOCK_SZ - 3),
        DBL_MAX, -DBL_MAX
    };
    size_t damage_index;

    for (damage_index = 0; damage_index < NELEM(invalid_damage);
         damage_index++) {
        reset_frame();
        whiteRGBA = UINT32_C(0x11223344);

        Gui_paint_setup_target(
            10, 20, 0, invalid_damage[damage_index], false);

        TEST_CHECK(fake_sdl_renderer.frame_result
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(event_count == 1);
        TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(stroke_attempts == 0);
        TEST_CHECK(fill_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        Gui_paint_setup_target(10, 20, 0, 125.0, false);
        TEST_CHECK(event_count == 1);
        TEST_CHECK(image_attempts == 1);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(check_no_legacy_primitives() == 0);
    }
    return 0;
}

static int check_target_int_coordinate_overflow_is_sticky(void)
{
    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(INT_MAX, 20, 0, 125.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    Gui_paint_setup_target(10, 20, 0, 125.0, false);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(10, INT_MAX, 0, 125.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(
        INT_MAX, 20, 7, TARGET_DAMAGE, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(map_text_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(10, INT_MIN, 0, -125.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_primitives();
}

static int check_target_float_extent_collapse_is_sticky(void)
{
    reset_frame();
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(INT_MAX - 5, 20, 0, 125.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(map_text_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    Gui_paint_setup_target(10, 20, 0, 125.0, false);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(image_attempts == 1);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    whiteRGBA = UINT32_C(0x11223344);

    Gui_paint_setup_target(10, INT64_C(1) << 26, 7, 0.0, false);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == PAINT_EVENT_IMAGE);
    TEST_CHECK(events[1] == PAINT_EVENT_MAP_TEXT);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(map_text_attempts == 1);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    Gui_paint_setup_target(10, 20, 7, 125.0, false);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(map_text_attempts == 1);
    return check_no_legacy_primitives();
}

static int check_failed_target_stage_is_sticky(
    PaintEvent expected_last_event, RendererStatus expected_result)
{
    int prior_event_count;
    int prior_image_attempts;
    int prior_map_text_attempts;
    int prior_blend_attempts;
    int prior_stroke_attempts;
    int prior_fill_attempts;
    int prior_flush_attempts;

    Gui_paint_setup_target(10, 20, 7, 125.0, true);
    TEST_CHECK(fake_sdl_renderer.frame_result == expected_result);
    TEST_CHECK(event_count > 0);
    TEST_CHECK(events[event_count - 1] == expected_last_event);
    TEST_CHECK(check_no_legacy_primitives() == 0);

    prior_event_count = event_count;
    prior_image_attempts = image_attempts;
    prior_map_text_attempts = map_text_attempts;
    prior_blend_attempts = blend_attempts;
    prior_stroke_attempts = stroke_attempts;
    prior_fill_attempts = fill_attempts;
    prior_flush_attempts = flush_attempts;
    Gui_paint_setup_target(10, 20, 7, 125.0, true);
    TEST_CHECK(fake_sdl_renderer.frame_result == expected_result);
    TEST_CHECK(event_count == prior_event_count);
    TEST_CHECK(image_attempts == prior_image_attempts);
    TEST_CHECK(map_text_attempts == prior_map_text_attempts);
    TEST_CHECK(blend_attempts == prior_blend_attempts);
    TEST_CHECK(stroke_attempts == prior_stroke_attempts);
    TEST_CHECK(fill_attempts == prior_fill_attempts);
    TEST_CHECK(flush_attempts == prior_flush_attempts);
    return check_no_legacy_primitives();
}

static int check_target_failures_stop_later_work(void)
{
    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    image_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_IMAGE, RENDERER_STATUS_RESOURCE_MISMATCH) == 0);
    TEST_CHECK(map_text_attempts == 0);
    TEST_CHECK(blend_attempts == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    map_text_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_MAP_TEXT, RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    TEST_CHECK(blend_attempts == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    blend_failure_attempt = 1;
    blend_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_BLEND, RENDERER_STATUS_BACKEND_ERROR) == 0);
    TEST_CHECK(stroke_attempts == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    stroke_failure_attempt = 1;
    stroke_failure_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_STROKE, RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    fill_result = RENDERER_STATUS_INVALID_ARGUMENT;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_FLUSH, RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 1);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    fill_result = RENDERER_STATUS_INVALID_ARGUMENT;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_FLUSH, RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 1);

    reset_frame();
    fake_setup.mode = TEAM_PLAY;
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(check_failed_target_stage_is_sticky(
        PAINT_EVENT_FLUSH, RENDERER_STATUS_BACKEND_ERROR) == 0);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    return 0;
}

int main(void)
{
    if (check_sticky_failure_gates_direct_world_leaf() != 0)
        return 1;
    if (check_sticky_failure_gates_mixed_image_world_leaf() != 0)
        return 1;
    if (check_sticky_failure_gates_setup_switching_world_leaf() != 0)
        return 1;
    if (check_sticky_failure_gates_semantic_and_text_world_leaf() != 0)
        return 1;
    if (check_sticky_failure_gates_inactive_asteroid_batch() != 0)
        return 1;
    if (check_asteroid_batch_cleanup_survives_sticky_failure() != 0)
        return 1;
    if (check_directional_gravity_geometry_and_order() != 0)
        return 1;
    if (check_directional_gravity_phase_boundaries_and_negative() != 0)
        return 1;
    if (check_directional_gravity_numeric_boundaries_are_checked() != 0)
        return 1;
    if (check_directional_gravity_failures_are_sticky() != 0)
        return 1;
    if (check_world_decor_paths_are_semantic() != 0)
        return 1;
    if (check_world_border_rectangles_are_semantic() != 0)
        return 1;
    if (check_world_wall_paths_and_fuel_precedence() != 0)
        return 1;
    if (check_world_stippled_lines_use_no_legacy_smoothing_state() != 0)
        return 1;
    if (check_world_line_empty_geometry_is_successful_noop() != 0)
        return 1;
    if (check_world_line_numeric_boundaries_are_checked() != 0)
        return 1;
    if (check_world_line_failures_preserve_prefix_and_sticky_status() != 0)
        return 1;
    if (check_visible_border_setup_failures_restore_safely() != 0)
        return 1;
    if (check_target_full_order_and_geometry() != 0)
        return 1;
    if (check_target_optional_paths_and_own_color() != 0)
        return 1;
    if (check_target_unbounded_finite_damage() != 0)
        return 1;
    if (check_target_invalid_damage_is_sticky() != 0)
        return 1;
    if (check_target_int_coordinate_overflow_is_sticky() != 0)
        return 1;
    if (check_target_float_extent_collapse_is_sticky() != 0)
        return 1;
    if (check_target_failures_stop_later_work() != 0)
        return 1;
    if (check_map_constant_fills_are_semantic() != 0)
        return 1;
    if (check_map_fill_degenerate_geometry_is_preserved() != 0)
        return 1;
    if (check_map_fill_failures_are_sticky() != 0)
        return 1;
    if (check_laser_batch_is_semantic() != 0)
        return 1;
    if (check_empty_and_first_failure_laser_batches() != 0)
        return 1;
    if (check_laser_failure_flushes_accepted_prefix() != 0)
        return 1;
    if (check_laser_flush_failure_is_sticky() != 0)
        return 1;
    if (check_invalid_laser_geometry_is_sticky() != 0)
        return 1;
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
