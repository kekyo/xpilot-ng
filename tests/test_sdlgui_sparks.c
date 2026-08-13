#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "paint.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"
#include "spark_batch_test_support.h"
#include "wreckshape.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 16
#define MAX_CAPTURED_POINTS 32
#define MAX_CAPTURED_STROKE_POINTS 16
#define TEST_WRECK_TYPE 2
#define TEST_WRECK_ROTATION 37
#define TEST_WRECK_SIZE 255

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef enum TestEvent {
    TEST_EVENT_BLEND,
    TEST_EVENT_DRAW,
    TEST_EVENT_STROKE,
    TEST_EVENT_FLUSH
} TestEvent;

typedef struct CapturedPointDraw {
    RendererColoredPoint2D points[MAX_CAPTURED_POINTS];
    size_t point_count;
    float size;
    RendererBlendMode blend;
} CapturedPointDraw;

typedef struct CapturedStroke {
    RendererPoint2D points[MAX_CAPTURED_STROKE_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    RendererBlendMode blend;
} CapturedStroke;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer;
static TestEvent events[MAX_EVENTS];
static CapturedPointDraw captured_draw;
static CapturedStroke captured_stroke;
static position_t test_wreckage_shapes
    [NUM_WRECKAGE_SHAPES][NUM_WRECKAGE_POINTS][RES];
static int event_count;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int stroke_attempts;
static int successful_strokes;
static int flush_attempts;
static int realloc_attempts;
static int fail_realloc_attempt;
static RendererBlendMode current_blend;
static RendererStatus blend_result;
static RendererStatus draw_result;
static RendererStatus stroke_result;
static RendererStatus flush_result;

client_data_t clData;
ipos_t world;
ipos_t realWorld;
short ext_view_width;
short ext_view_height;
int active_view_width;
int active_view_height;
int ext_view_x_offset;
int ext_view_y_offset;
int num_spark_colors;
int sparkSize;

itemtype_t *itemtype_ptr;
int num_itemtype;
int max_itemtype;
ball_t *ball_ptr;
int num_ball;
int max_ball;
ship_t *ship_ptr;
int num_ship;
int max_ship;
mine_t *mine_ptr;
int num_mine;
int max_mine;
debris_t *debris_ptr[DEBRIS_TYPES];
int num_debris[DEBRIS_TYPES];
int max_debris[DEBRIS_TYPES];
wreckage_t *wreckage_ptr;
int num_wreckage;
int max_wreckage;
asteroid_t *asteroid_ptr;
int num_asteroids;
int max_asteroids;
wormhole_t *wormhole_ptr;
int num_wormholes;
int max_wormholes;
debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
int num_fastshot[DEBRIS_TYPES * 2];
int max_fastshot[DEBRIS_TYPES * 2];
missile_t *missile_ptr;
int num_missile;
int max_missile;
laser_t *laser_ptr;
int num_laser;
int max_laser;

void *__real_realloc(void *pointer, size_t size);

void *__wrap_realloc(void *pointer, size_t size)
{
    realloc_attempts++;
    if (realloc_attempts == fail_realloc_attempt)
        return NULL;
    return __real_realloc(pointer, size);
}

static void record_event(TestEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count] = event;
    event_count++;
}

static void reset_wreckage_shapes(void)
{
    static const position_t selected_points[NUM_WRECKAGE_POINTS] = {
        {-9.0f, 6.0f}, {-2.0f, 8.0f}, {5.0f, 2.0f},
        {9.0f, 3.0f}, {10.0f, 0.0f}, {5.0f, -1.0f},
        {3.0f, 0.0f}, {-2.0f, -9.0f}, {-5.0f, -6.0f},
        {-3.0f, -2.0f}, {-7.0f, -1.0f}, {-5.0f, 2.0f}
    };
    int point_index;
    int shape_index;

    memset(test_wreckage_shapes, 0, sizeof(test_wreckage_shapes));
    for (shape_index = 0;
         shape_index < NUM_WRECKAGE_SHAPES;
         shape_index++) {
        for (point_index = 0;
             point_index < NUM_WRECKAGE_POINTS;
             point_index++) {
            wreckageShapes[shape_index][point_index]
                = test_wreckage_shapes[shape_index][point_index];
        }
    }
    for (point_index = 0;
         point_index < NUM_WRECKAGE_POINTS;
         point_index++) {
        test_wreckage_shapes[TEST_WRECK_TYPE][point_index]
                             [TEST_WRECK_ROTATION]
            = selected_points[point_index];
    }
}

static void reset_state(void)
{
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    memset(&fake_sdl_renderer, 0, sizeof(fake_sdl_renderer));
    memset(events, 0, sizeof(events));
    memset(&captured_draw, 0, sizeof(captured_draw));
    memset(&captured_stroke, 0, sizeof(captured_stroke));
    memset(&clData, 0, sizeof(clData));
    memset(debris_ptr, 0, sizeof(debris_ptr));
    memset(num_debris, 0, sizeof(num_debris));
    memset(max_debris, 0, sizeof(max_debris));
    memset(fastshot_ptr, 0, sizeof(fastshot_ptr));
    memset(num_fastshot, 0, sizeof(num_fastshot));
    memset(max_fastshot, 0, sizeof(max_fastshot));

    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    event_count = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    stroke_attempts = 0;
    successful_strokes = 0;
    flush_attempts = 0;
    realloc_attempts = 0;
    fail_realloc_attempt = 0;
    current_blend = RENDERER_BLEND_OPAQUE;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    stroke_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;

    reset_wreckage_shapes();

    world.x = 1000;
    world.y = -500;
    realWorld = world;
    ext_view_width = 256;
    ext_view_height = 300;
    active_view_width = 256;
    active_view_height = 256;
    ext_view_x_offset = 10;
    ext_view_y_offset = 20;
    num_spark_colors = 8;
    sparkSize = 4;

    itemtype_ptr = NULL;
    num_itemtype = 0;
    max_itemtype = 0;
    ball_ptr = NULL;
    num_ball = 0;
    max_ball = 0;
    ship_ptr = NULL;
    num_ship = 0;
    max_ship = 0;
    mine_ptr = NULL;
    num_mine = 0;
    max_mine = 0;
    wreckage_ptr = NULL;
    num_wreckage = 0;
    max_wreckage = 0;
    asteroid_ptr = NULL;
    num_asteroids = 0;
    max_asteroids = 0;
    wormhole_ptr = NULL;
    num_wormholes = 0;
    max_wormholes = 0;
    missile_ptr = NULL;
    num_missile = 0;
    max_missile = 0;
    laser_ptr = NULL;
    num_laser = 0;
    max_laser = 0;
}

static int set_debris_bucket(
    int bucket, const debris_t *entries, size_t entry_count)
{
    debris_t *copy;

    if (bucket < 0 || bucket >= DEBRIS_TYPES || entries == NULL
        || entry_count == 0 || entry_count > (size_t)INT_MAX) {
        return 1;
    }
    copy = malloc(entry_count * sizeof(*copy));
    if (copy == NULL)
        return 1;
    memcpy(copy, entries, entry_count * sizeof(*copy));
    debris_ptr[bucket] = copy;
    num_debris[bucket] = (int)entry_count;
    max_debris[bucket] = (int)entry_count;
    return 0;
}

static int set_wreckage_after_sparks(void)
{
    wreckage_ptr = malloc(sizeof(*wreckage_ptr));
    if (wreckage_ptr == NULL)
        return 1;
    memset(wreckage_ptr, 0, sizeof(*wreckage_ptr));
    wreckage_ptr[0].x = 1050;
    wreckage_ptr[0].y = -400;
    wreckage_ptr[0].wrecktype = TEST_WRECK_TYPE;
    wreckage_ptr[0].rotation = TEST_WRECK_ROTATION;
    wreckage_ptr[0].size = TEST_WRECK_SIZE;
    num_wreckage = 1;
    max_wreckage = 1;
    return 0;
}

static int all_debris_released(void)
{
    int bucket;

    for (bucket = 0; bucket < DEBRIS_TYPES; bucket++) {
        if (num_debris[bucket] != 0 || max_debris[bucket] != 0)
            return 0;
        debris_ptr[bucket] = NULL;
    }
    return 1;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int point_equal(
    const RendererColoredPoint2D *point, float x, float y,
    RendererColor color)
{
    return point->position.x == x && point->position.y == y
        && color_equal(point->color, color);
}

static int stroke_point_equal(size_t point_index, float x, float y)
{
    return point_index < captured_stroke.point_count
        && captured_stroke.points[point_index].x == x
        && captured_stroke.points[point_index].y == y;
}

static int check_successful_wreck(RendererColor color)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(captured_stroke.point_count == NUM_WRECKAGE_POINTS);
    TEST_CHECK(captured_stroke.width == 1.0f);
    TEST_CHECK(color_equal(captured_stroke.color, color));
    TEST_CHECK(captured_stroke.closed == 1);
    TEST_CHECK(captured_stroke.blend == RENDERER_BLEND_ALPHA);
    return 0;
}

static int check_successful_batch(
    size_t point_count, float size, int expected_event_count,
    int expected_semantic_batches)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == expected_semantic_batches);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == expected_semantic_batches);
    TEST_CHECK(current_blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(captured_draw.blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(captured_draw.point_count == point_count);
    TEST_CHECK(captured_draw.size == size);
    return 0;
}

SdlRenderer *Get_sdl_renderer(void)
{
    return &fake_sdl_renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer == NULL ? NULL : renderer->frontend;
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
    return renderer == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                            : renderer->frame_result;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color;

    color.red = (uint8_t)(rgba >> 24);
    color.green = (uint8_t)(rgba >> 16);
    color.blue = (uint8_t)(rgba >> 8);
    color.alpha = (uint8_t)rgba;
    return color;
}

RendererStatus Renderer_set_blend(
    Renderer *renderer, RendererBlendMode blend)
{
    blend_attempts++;
    record_event(TEST_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (blend_result != RENDERER_STATUS_OK)
        return blend_result;
    current_blend = blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_colored_points(
    Renderer *renderer, const RendererColoredPoint2D *points,
    size_t point_count, float size)
{
    draw_attempts++;
    record_event(TEST_EVENT_DRAW);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count == 0
        || point_count > MAX_CAPTURED_POINTS || size <= 0.0f) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    memcpy(captured_draw.points, points,
           point_count * sizeof(captured_draw.points[0]));
    captured_draw.point_count = point_count;
    captured_draw.size = size;
    captured_draw.blend = current_blend;
    successful_draws++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_stroke_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed)
{
    stroke_attempts++;
    record_event(TEST_EVENT_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count < 2
        || point_count > MAX_CAPTURED_STROKE_POINTS || width <= 0.0f) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (stroke_result != RENDERER_STATUS_OK)
        return stroke_result;
    memcpy(captured_stroke.points, points,
           point_count * sizeof(captured_stroke.points[0]));
    captured_stroke.point_count = point_count;
    captured_stroke.width = width;
    captured_stroke.color = color;
    captured_stroke.closed = closed;
    captured_stroke.blend = current_blend;
    successful_strokes++;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush(
    SdlRenderer *renderer)
{
    flush_attempts++;
    record_event(TEST_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer)
        return RENDERER_STATUS_INVALID_STATE;
    return flush_result;
}

other_t *Other_by_id(int id)
{
    (void)id;
    return NULL;
}

void __wrap_Gui_paint_item_object(int type, int x, int y)
{
    (void)type;
    (void)x;
    (void)y;
}

void __wrap_Gui_paint_ball(int x, int y, int style)
{
    (void)x;
    (void)y;
    (void)style;
}

void __wrap_Gui_paint_ball_connector(int x_1, int y_1, int x_2, int y_2)
{
    (void)x_1;
    (void)y_1;
    (void)x_2;
    (void)y_2;
}

void __wrap_Gui_paint_mine(int x, int y, int teammine, char *name)
{
    (void)x;
    (void)y;
    (void)teammine;
    (void)name;
}

void __wrap_Gui_paint_asteroids_begin(void)
{
}

void __wrap_Gui_paint_asteroids_end(void)
{
}

void __wrap_Gui_paint_asteroid(int x, int y, int type, int rot, int size)
{
    (void)x;
    (void)y;
    (void)type;
    (void)rot;
    (void)size;
}

void __wrap_Gui_paint_setup_worm(int x, int y)
{
    (void)x;
    (void)y;
}

void __wrap_Gui_paint_fastshot(int color, int x, int y)
{
    (void)color;
    (void)x;
    (void)y;
}

void __wrap_Gui_paint_teamshot(int x, int y)
{
    (void)x;
    (void)y;
}

void __wrap_Gui_paint_missiles_begin(void)
{
}

void __wrap_Gui_paint_missiles_end(void)
{
}

void __wrap_Gui_paint_missile(int x, int y, int len, int dir)
{
    (void)x;
    (void)y;
    (void)len;
    (void)dir;
}

void __wrap_Gui_paint_lasers_begin(void)
{
}

void __wrap_Gui_paint_lasers_end(void)
{
}

void __wrap_Gui_paint_laser(int color, int x, int y, int len, int dir)
{
    (void)color;
    (void)x;
    (void)y;
    (void)len;
    (void)dir;
}

static int check_actual_traversal_geometry_color_order_and_release(void)
{
    static const debris_t bucket_zero[] = {{1, 2}, {3, 4}};
    static const debris_t bucket_one[] = {{5, 6}};
    static const debris_t bucket_two[] = {{7, 8}};
    static const debris_t bucket_seven[] = {{9, 10}};
    const RendererColor color_zero = {31, 0, 0, 255};
    const RendererColor color_four = {159, 63, 0, 255};
    const RendererColor color_one = {63, 3, 0, 255};
    const RendererColor color_seven = {255, 195, 0, 255};
    const RendererColor wreck_color = {255, 0, 0, 255};

    reset_state();
    TEST_CHECK(set_debris_bucket(
                   0, bucket_zero, NELEM(bucket_zero)) == 0);
    TEST_CHECK(set_debris_bucket(
                   1, bucket_one, NELEM(bucket_one)) == 0);
    TEST_CHECK(set_debris_bucket(
                   2, bucket_two, NELEM(bucket_two)) == 0);
    TEST_CHECK(set_debris_bucket(
                   7, bucket_seven, NELEM(bucket_seven)) == 0);
    TEST_CHECK(set_wreckage_after_sparks() == 0);

    Paint_shots();

    TEST_CHECK(all_debris_released());
    TEST_CHECK(num_wreckage == 0);
    TEST_CHECK(max_wreckage == 0);
    wreckage_ptr = NULL;
    TEST_CHECK(check_successful_batch(5, 4.0f, 6, 2) == 0);
    TEST_CHECK(check_successful_wreck(wreck_color) == 0);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(events[3] == TEST_EVENT_BLEND);
    TEST_CHECK(events[4] == TEST_EVENT_STROKE);
    TEST_CHECK(events[5] == TEST_EVENT_FLUSH);
    TEST_CHECK(point_equal(
                   &captured_draw.points[0], 1011.0f, -477.0f,
                   color_zero));
    TEST_CHECK(point_equal(
                   &captured_draw.points[1], 1013.0f, -475.0f,
                   color_zero));
    TEST_CHECK(point_equal(
                   &captured_draw.points[2], 1015.0f, -473.0f,
                   color_four));
    TEST_CHECK(point_equal(
                   &captured_draw.points[3], 1017.0f, -471.0f,
                   color_one));
    TEST_CHECK(point_equal(
                   &captured_draw.points[4], 1019.0f, -469.0f,
                   color_seven));
    TEST_CHECK(stroke_point_equal(0, 1041.0f, -395.0f));
    TEST_CHECK(stroke_point_equal(1, 1048.0f, -393.0f));
    TEST_CHECK(stroke_point_equal(2, 1054.0f, -399.0f));
    TEST_CHECK(stroke_point_equal(3, 1058.0f, -398.0f));
    TEST_CHECK(stroke_point_equal(4, 1059.0f, -400.0f));
    TEST_CHECK(stroke_point_equal(5, 1054.0f, -401.0f));
    TEST_CHECK(stroke_point_equal(6, 1052.0f, -400.0f));
    TEST_CHECK(stroke_point_equal(7, 1048.0f, -409.0f));
    TEST_CHECK(stroke_point_equal(8, 1045.0f, -406.0f));
    TEST_CHECK(stroke_point_equal(9, 1047.0f, -402.0f));
    TEST_CHECK(stroke_point_equal(10, 1043.0f, -401.0f));
    TEST_CHECK(stroke_point_equal(11, 1045.0f, -399.0f));
    return 0;
}

static int check_wreck_has_deadly_color(void)
{
    const RendererColor white = {255, 255, 255, 255};

    reset_state();

    Gui_paint_wreck(
        100, 200, true, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);

    TEST_CHECK(check_successful_wreck(white) == 0);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_STROKE);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(stroke_point_equal(0, 91.0f, 205.0f));
    TEST_CHECK(stroke_point_equal(11, 95.0f, 201.0f));
    return 0;
}

static int check_wreck_has_no_commands(RendererStatus expected_status)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == expected_status);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_zero_size_wreck_is_noop(void)
{
    reset_state();

    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, 0);

    TEST_CHECK(check_wreck_has_no_commands(RENDERER_STATUS_OK) == 0);
    return 0;
}

static int check_wreck_selector_and_size_validation_is_cpu_atomic(void)
{
    reset_state();
    Gui_paint_wreck(
        100, 200, false, -1,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        100, 200, false, NUM_WRECKAGE_SHAPES,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        -1, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        RES, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, -1);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    return 0;
}

static int check_wreck_nonfinite_rotated_points_are_cpu_atomic(void)
{
    reset_state();
    test_wreckage_shapes[TEST_WRECK_TYPE][NUM_WRECKAGE_POINTS - 1]
                         [TEST_WRECK_ROTATION].x = NAN;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    test_wreckage_shapes[TEST_WRECK_TYPE][NUM_WRECKAGE_POINTS - 1]
                         [TEST_WRECK_ROTATION].y = INFINITY;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    return 0;
}

static int check_wreck_numeric_failures_are_cpu_atomic(void)
{
    reset_state();
    test_wreckage_shapes[TEST_WRECK_TYPE][NUM_WRECKAGE_POINTS - 1]
                         [TEST_WRECK_ROTATION].x = (float)INT_MAX;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        INT_MAX, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    Gui_paint_wreck(
        100, INT_MIN, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    reset_state();
    test_wreckage_shapes[TEST_WRECK_TYPE][0]
                         [TEST_WRECK_ROTATION].x = 2.0f;
    Gui_paint_wreck(
        16777216, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(check_wreck_has_no_commands(
                   RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    return 0;
}

static int check_actual_wreck_failure_keeps_release_and_order(void)
{
    static const debris_t debris[] = {{1, 2}};

    reset_state();
    TEST_CHECK(set_debris_bucket(0, debris, NELEM(debris)) == 0);
    TEST_CHECK(set_wreckage_after_sparks() == 0);
    test_wreckage_shapes[TEST_WRECK_TYPE][NUM_WRECKAGE_POINTS - 1]
                         [TEST_WRECK_ROTATION].x = NAN;

    Paint_shots();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(all_debris_released());
    TEST_CHECK(num_wreckage == 0);
    TEST_CHECK(max_wreckage == 0);
    wreckage_ptr = NULL;
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    return 0;
}

static int check_wreck_renderer_failures_are_sticky(void)
{
    int previous_event_count;

    reset_state();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_state();
    stroke_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_STROKE);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_state();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_STROKE);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);

    previous_event_count = event_count;
    flush_result = RENDERER_STATUS_OK;
    Gui_paint_wreck(
        100, 200, false, TEST_WRECK_TYPE,
        TEST_WRECK_ROTATION, TEST_WRECK_SIZE);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == previous_event_count);
    return 0;
}

static int check_standalone_spark_is_one_semantic_batch(void)
{
    const RendererColor color_five = {191, 99, 0, 255};

    reset_state();
    world.x = -10;
    world.y = 20;
    sparkSize = 3;

    Gui_paint_spark(5, 12, 44);

    TEST_CHECK(check_successful_batch(1, 3.0f, 3, 1) == 0);
    TEST_CHECK(point_equal(
                   &captured_draw.points[0], 2.0f, 276.0f,
                   color_five));
    return 0;
}

static int check_empty_batch_is_a_noop_for_any_point_size(void)
{
    reset_state();
    sparkSize = 0;

    TEST_CHECK(Sdlgui_test_paint_spark_batch(NULL, 0)
               == RENDERER_STATUS_OK);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(realloc_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_later_float_edge_collapse_is_cpu_atomic(void)
{
    static const SdlguiTestSpark entries[] = {
        {0, 10, 20},
        {1, INT_MAX, 40},
        {2, 50, 60}
    };

    reset_state();
    world.x = 0;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(
                   entries, NELEM(entries))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_later_invalid_entry_is_cpu_atomic(void)
{
    static const SdlguiTestSpark entries[] = {
        {0, 10, 20},
        {8, 30, 40},
        {1, 50, 60}
    };

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_spark_batch(
                   entries, NELEM(entries))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);

    reset_state();
    sparkSize = 0;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(entries, 1)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_actual_numeric_failure_keeps_all_release(void)
{
    static const debris_t first[] = {{1, 2}};
    static const debris_t last[] = {{9, 10}};

    reset_state();
    world.x = INT_MAX;
    TEST_CHECK(set_debris_bucket(0, first, NELEM(first)) == 0);
    TEST_CHECK(set_debris_bucket(7, last, NELEM(last)) == 0);

    Paint_shots();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(all_debris_released());
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_actual_realloc_failure_keeps_all_release(void)
{
    static const debris_t first[] = {{1, 2}};
    static const debris_t last[] = {{9, 10}};

    reset_state();
    TEST_CHECK(set_debris_bucket(0, first, NELEM(first)) == 0);
    TEST_CHECK(set_debris_bucket(7, last, NELEM(last)) == 0);
    fail_realloc_attempt = 1;

    Paint_shots();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(all_debris_released());
    TEST_CHECK(realloc_attempts == 1);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

static int check_renderer_failures_are_sticky(void)
{
    static const SdlguiTestSpark entries[] = {
        {0, 10, 20}, {7, 30, 40}
    };
    int previous_event_count;

    reset_state();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(
                   entries, NELEM(entries))
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_state();
    draw_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(
                   entries, NELEM(entries))
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_state();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(
                   entries, NELEM(entries))
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);

    previous_event_count = event_count;
    flush_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdlgui_test_paint_spark_batch(entries, 1)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == previous_event_count);
    return 0;
}

static int check_preexisting_failure_still_releases_actual_traversal(void)
{
    static const debris_t first[] = {{1, 2}};
    static const debris_t last[] = {{9, 10}};

    reset_state();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(set_debris_bucket(0, first, NELEM(first)) == 0);
    TEST_CHECK(set_debris_bucket(7, last, NELEM(last)) == 0);
    TEST_CHECK(set_wreckage_after_sparks() == 0);

    Paint_shots();

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(all_debris_released());
    TEST_CHECK(num_wreckage == 0);
    TEST_CHECK(max_wreckage == 0);
    wreckage_ptr = NULL;
    TEST_CHECK(realloc_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(event_count == 0);
    return 0;
}

int main(void)
{
    int failed = 0;

    failed = check_actual_traversal_geometry_color_order_and_release();
    if (failed)
        return EXIT_FAILURE;
    failed |= check_wreck_has_deadly_color();
    failed |= check_zero_size_wreck_is_noop();
    failed |= check_wreck_selector_and_size_validation_is_cpu_atomic();
    failed |= check_wreck_nonfinite_rotated_points_are_cpu_atomic();
    failed |= check_wreck_numeric_failures_are_cpu_atomic();
    failed |= check_actual_wreck_failure_keeps_release_and_order();
    failed |= check_wreck_renderer_failures_are_sticky();
    failed |= check_standalone_spark_is_one_semantic_batch();
    failed |= check_empty_batch_is_a_noop_for_any_point_size();
    failed |= check_later_float_edge_collapse_is_cpu_atomic();
    failed |= check_later_invalid_entry_is_cpu_atomic();
    failed |= check_actual_numeric_failure_keeps_all_release();
    failed |= check_actual_realloc_failure_keeps_all_release();
    failed |= check_renderer_failures_are_sticky();
    failed |= check_preexisting_failure_still_releases_actual_traversal();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
