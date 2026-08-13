#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "images.h"
#include "option.h"
#include "paint.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"
#include "text.h"

#include <GL/gl.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PLAYER_COUNT 3
#define TEST_DIRECTION 37
#define MAX_EVENTS 64
#define MAX_CAPTURED_POINTS MAX_SHIP_PTS2

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef enum TestEvent {
    TEST_EVENT_APPEARING_BEGIN,
    TEST_EVENT_APPEARING_ENTRY,
    TEST_EVENT_APPEARING_END,
    TEST_EVENT_SHIP_LOOKUP,
    TEST_EVENT_IMAGE,
    TEST_EVENT_ROTATED_IMAGE,
    TEST_EVENT_BLEND,
    TEST_EVENT_STROKE,
    TEST_EVENT_STIPPLED_STROKE,
    TEST_EVENT_FLUSH,
    TEST_EVENT_NAME,
    TEST_EVENT_LIFE
} TestEvent;

typedef struct CapturedStroke {
    RendererPoint2D points[MAX_CAPTURED_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    RendererBlendMode blend;
} CapturedStroke;

typedef struct CapturedStippledStroke {
    RendererPoint2D points[MAX_CAPTURED_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    unsigned int factor;
    uint16_t pattern;
    RendererBlendMode blend;
} CapturedStippledStroke;

typedef struct CapturedImage {
    int index;
    int x;
    int y;
    int frame;
    int direction;
    int color;
} CapturedImage;

typedef struct CapturedText {
    font_data *font;
    int color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    int maximum_length;
    char text[64];
} CapturedText;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer;
static setup_t fake_setup;
static other_t players[TEST_PLAYER_COUNT];
static shipshape_t player_shapes[TEST_PLAYER_COUNT];
static shipshape_t default_shape;
static clpos_t player_shape_points
    [TEST_PLAYER_COUNT][MAX_SHIP_PTS2][RES];
static clpos_t default_shape_points[MAX_SHIP_PTS2][RES];
static TestEvent events[MAX_EVENTS];
static CapturedStroke captured_stroke;
static CapturedStippledStroke captured_stippled_stroke;
static CapturedImage captured_image;
static CapturedText captured_name;
static CapturedText captured_life;
static bool *textured_ships_option;
static double *ship_line_width_option;
static int event_count;
static int lookup_count;
static int image_attempts;
static int rotated_image_attempts;
static int blend_attempts;
static int stroke_attempts;
static int successful_strokes;
static int stippled_stroke_attempts;
static int successful_stippled_strokes;
static int flush_attempts;
static int name_attempts;
static int life_attempts;
static int legacy_calls;
static RendererBlendMode current_blend;
static RendererStatus image_result;
static RendererStatus blend_result;
static RendererStatus stroke_result;
static RendererStatus stippled_stroke_result;
static RendererStatus flush_result;
static RendererStatus name_result;
static RendererStatus life_result;

setup_t *Setup = &fake_setup;
client_data_t clData;
ipos_t world;
ipos_t realWorld;
short ext_view_width;
short ext_view_height;
long loops;
long loopsSlow;
unsigned version;
double clientFPS;
instruments_t instruments;
font_data mapfont;

int eyesId;
other_t *eyes;
int eyeTeam;
other_t *self;
int roundDelay;
int maxCharsInNames;

refuel_t *refuel_ptr;
int num_refuel;
int max_refuel;
connector_t *connector_ptr;
int num_connector;
int max_connector;
ship_t *ship_ptr;
int num_ship;
int max_ship;
ecm_t *ecm_ptr;
int num_ecm;
int max_ecm;
trans_t *trans_ptr;
int num_trans;
int max_trans;
paused_t *paused_ptr;
int num_paused;
int max_paused;
appearing_t *appearing_ptr;
int num_appearing;
int max_appearing;

extern Uint32 whiteRGBA;
extern Uint32 blueRGBA;
extern Uint32 redRGBA;
extern Uint32 shipNameColorRGBA;
extern Uint32 selfLWColorRGBA;
extern Uint32 teamLWColorRGBA;
extern Uint32 enemyLWColorRGBA;
extern Uint32 teamShipColorRGBA;
extern Uint32 team0ColorRGBA;
extern Uint32 team1ColorRGBA;
extern Uint32 team2ColorRGBA;
extern Uint32 team3ColorRGBA;
extern Uint32 team4ColorRGBA;
extern Uint32 team5ColorRGBA;
extern Uint32 team6ColorRGBA;
extern Uint32 team7ColorRGBA;
extern Uint32 team8ColorRGBA;
extern Uint32 team9ColorRGBA;
extern Uint32 manyLivesColorRGBA;
extern Uint32 twoLivesColorRGBA;
extern Uint32 oneLifeColorRGBA;
extern Uint32 zeroLivesColorRGBA;

static void record_event(TestEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count] = event;
    event_count++;
}

static other_t *player_by_id(int id)
{
    int player_index;

    for (player_index = 0;
         player_index < TEST_PLAYER_COUNT;
         player_index++) {
        if (players[player_index].id == id)
            return &players[player_index];
    }
    return NULL;
}

static void assign_shape_storage(
    shipshape_t *shape, clpos_t points[MAX_SHIP_PTS2][RES])
{
    int point_index;

    memset(shape, 0, sizeof(*shape));
    memset(points, 0, sizeof(clpos_t) * MAX_SHIP_PTS2 * RES);
    for (point_index = 0;
         point_index < MAX_SHIP_PTS2;
         point_index++) {
        shape->pts[point_index] = points[point_index];
    }
}

static void set_triangle(
    shipshape_t *shape, clpos_t points[MAX_SHIP_PTS2][RES])
{
    assign_shape_storage(shape, points);
    shape->num_points = 3;
    points[0][TEST_DIRECTION].cx = 160;
    points[0][TEST_DIRECTION].cy = -80;
    points[1][TEST_DIRECTION].cx = -192;
    points[1][TEST_DIRECTION].cy = 256;
    points[2][TEST_DIRECTION].cx = 64;
    points[2][TEST_DIRECTION].cy = -352;
}

static void set_maximum_shape(
    shipshape_t *shape, clpos_t points[MAX_SHIP_PTS2][RES])
{
    int point_index;

    assign_shape_storage(shape, points);
    shape->num_points = MAX_SHIP_PTS2;
    for (point_index = 0;
         point_index < MAX_SHIP_PTS2;
         point_index++) {
        points[point_index][TEST_DIRECTION].cx = point_index * CLICK;
        points[point_index][TEST_DIRECTION].cy
            = (point_index % 2 == 0 ? -2 : 2) * CLICK;
    }
}

static void reset_shapes_and_players(void)
{
    int player_index;

    memset(players, 0, sizeof(players));
    memset(player_shape_points, 0, sizeof(player_shape_points));
    for (player_index = 0;
         player_index < TEST_PLAYER_COUNT;
         player_index++) {
        set_triangle(
            &player_shapes[player_index],
            player_shape_points[player_index]);
        players[player_index].id = (short)((player_index + 1) * 10);
        players[player_index].team = (uint16_t)(player_index + 2);
        players[player_index].life = (short)(3 - player_index);
        players[player_index].mychar = ' ';
        players[player_index].ship = &player_shapes[player_index];
    }
    strcpy(players[0].nick_name, "self");
    strcpy(players[0].id_string, "SELF");
    strcpy(players[1].nick_name, "target");
    strcpy(players[1].id_string, "TARGET");
    strcpy(players[2].nick_name, "enemy");
    strcpy(players[2].id_string, "ENEMY");
    set_triangle(&default_shape, default_shape_points);
    default_shape_points[0][TEST_DIRECTION].cx = 6 * CLICK;
    default_shape_points[0][TEST_DIRECTION].cy = 0;
    default_shape_points[1][TEST_DIRECTION].cx = -4 * CLICK;
    default_shape_points[1][TEST_DIRECTION].cy = 3 * CLICK;
    default_shape_points[2][TEST_DIRECTION].cx = -4 * CLICK;
    default_shape_points[2][TEST_DIRECTION].cy = -3 * CLICK;
}

static void reset_state(void)
{
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    memset(&fake_sdl_renderer, 0, sizeof(fake_sdl_renderer));
    memset(&fake_setup, 0, sizeof(fake_setup));
    memset(&clData, 0, sizeof(clData));
    memset(&instruments, 0, sizeof(instruments));
    memset(&mapfont, 0, sizeof(mapfont));
    memset(events, 0, sizeof(events));
    memset(&captured_stroke, 0, sizeof(captured_stroke));
    memset(&captured_stippled_stroke, 0,
           sizeof(captured_stippled_stroke));
    memset(&captured_image, 0, sizeof(captured_image));
    memset(&captured_name, 0, sizeof(captured_name));
    memset(&captured_life, 0, sizeof(captured_life));
    reset_shapes_and_players();

    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    event_count = 0;
    lookup_count = 0;
    image_attempts = 0;
    rotated_image_attempts = 0;
    blend_attempts = 0;
    stroke_attempts = 0;
    successful_strokes = 0;
    stippled_stroke_attempts = 0;
    successful_stippled_strokes = 0;
    flush_attempts = 0;
    name_attempts = 0;
    life_attempts = 0;
    legacy_calls = 0;
    current_blend = RENDERER_BLEND_OPAQUE;
    image_result = RENDERER_STATUS_OK;
    blend_result = RENDERER_STATUS_OK;
    stroke_result = RENDERER_STATUS_OK;
    stippled_stroke_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    name_result = RENDERER_STATUS_OK;
    life_result = RENDERER_STATUS_OK;

    world.x = 0;
    world.y = 0;
    realWorld = world;
    ext_view_width = 1000;
    ext_view_height = 1000;
    loops = 0;
    loopsSlow = 0;
    version = UINT32_C(0x4f12);
    clientFPS = 60.0;
    instruments.showShipShapes = true;
    instruments.showMyShipShape = true;
    instruments.showLivesByShip = false;
    self = &players[0];
    eyes = NULL;
    eyesId = 0;
    eyeTeam = TEAM_NOT_SET;
    roundDelay = 0;
    maxCharsInNames = 12;

    whiteRGBA = UINT32_C(0xffffffff);
    blueRGBA = UINT32_C(0x0000ffff);
    redRGBA = UINT32_C(0xff0000ff);
    shipNameColorRGBA = 0;
    selfLWColorRGBA = UINT32_C(0x110000ff);
    teamLWColorRGBA = UINT32_C(0x220000ff);
    enemyLWColorRGBA = UINT32_C(0x330000ff);
    teamShipColorRGBA = UINT32_C(0x440000ff);
    team0ColorRGBA = 0;
    team1ColorRGBA = 0;
    team2ColorRGBA = 0;
    team3ColorRGBA = 0;
    team4ColorRGBA = 0;
    team5ColorRGBA = 0;
    team6ColorRGBA = 0;
    team7ColorRGBA = 0;
    team8ColorRGBA = 0;
    team9ColorRGBA = 0;
    manyLivesColorRGBA = UINT32_C(0x555555ff);
    twoLivesColorRGBA = UINT32_C(0x666666ff);
    oneLifeColorRGBA = UINT32_C(0x777777ff);
    zeroLivesColorRGBA = UINT32_C(0x888888ff);

    paused_ptr = NULL;
    num_paused = 0;
    max_paused = 0;
    appearing_ptr = NULL;
    num_appearing = 0;
    max_appearing = 0;
    ecm_ptr = NULL;
    num_ecm = 0;
    max_ecm = 0;
    ship_ptr = NULL;
    num_ship = 0;
    max_ship = 0;
    refuel_ptr = NULL;
    num_refuel = 0;
    max_refuel = 0;
    connector_ptr = NULL;
    num_connector = 0;
    max_connector = 0;
    trans_ptr = NULL;
    num_trans = 0;
    max_trans = 0;

    *textured_ships_option = false;
    *ship_line_width_option = 2.5;
}

void Store_option(xp_option_t *option)
{
    if (option == NULL)
        return;
    if (strcmp(option->name, "texturedShips") == 0)
        textured_ships_option = option->bool_ptr;
    else if (strcmp(option->name, "shipLineWidth") == 0)
        ship_line_width_option = option->dbl_ptr;
}

size_t strlcpy(char *destination, const char *source, size_t size)
{
    size_t source_length = strlen(source);

    if (size != 0) {
        size_t copied = source_length < size - 1
                          ? source_length : size - 1;

        memcpy(destination, source, copied);
        destination[copied] = '\0';
    }
    return source_length;
}

size_t strlcat(char *destination, const char *source, size_t size)
{
    size_t destination_length = strlen(destination);
    size_t source_length = strlen(source);

    if (destination_length < size) {
        size_t remaining = size - destination_length;
        size_t copied = source_length < remaining - 1
                          ? source_length : remaining - 1;

        memcpy(destination + destination_length, source, copied);
        destination[destination_length + copied] = '\0';
    }
    return destination_length + source_length;
}

other_t *Other_by_id(int id)
{
    lookup_count++;
    record_event(TEST_EVENT_SHIP_LOOKUP);
    return player_by_id(id);
}

shipshape_t *Ship_by_id(int id)
{
    other_t *other = player_by_id(id);

    return other == NULL ? NULL : other->ship;
}

shipshape_t *Default_ship(void)
{
    return &default_shape;
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

static int points_are_finite(
    const RendererPoint2D *points, size_t point_count)
{
    size_t point_index;

    for (point_index = 0; point_index < point_count; point_index++) {
        if (!isfinite(points[point_index].x)
            || !isfinite(points[point_index].y)) {
            return 0;
        }
    }
    return 1;
}

RendererStatus Renderer_stroke_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed)
{
    stroke_attempts++;
    record_event(TEST_EVENT_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count < 2
        || point_count > MAX_CAPTURED_POINTS
        || !isfinite(width) || width <= 0.0f
        || !points_are_finite(points, point_count)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
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

RendererStatus Renderer_stroke_stippled_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed,
    unsigned int factor, uint16_t pattern)
{
    stippled_stroke_attempts++;
    record_event(TEST_EVENT_STIPPLED_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count < 2
        || point_count > MAX_CAPTURED_POINTS
        || !isfinite(width) || width <= 0.0f
        || factor == 0 || factor > 256
        || !points_are_finite(points, point_count)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (stippled_stroke_result != RENDERER_STATUS_OK)
        return stippled_stroke_result;
    memcpy(captured_stippled_stroke.points, points,
           point_count * sizeof(captured_stippled_stroke.points[0]));
    captured_stippled_stroke.point_count = point_count;
    captured_stippled_stroke.width = width;
    captured_stippled_stroke.color = color;
    captured_stippled_stroke.closed = closed;
    captured_stippled_stroke.factor = factor;
    captured_stippled_stroke.pattern = pattern;
    captured_stippled_stroke.blend = current_blend;
    successful_stippled_strokes++;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(
    SdlRenderer *renderer)
{
    flush_attempts++;
    record_event(TEST_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer
        || renderer->frontend == NULL
        || !renderer->frontend->frame_active) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return flush_result;
}

static void capture_image(
    int index, int x, int y, int frame, int direction, int color)
{
    captured_image.index = index;
    captured_image.x = x;
    captured_image.y = y;
    captured_image.frame = frame;
    captured_image.direction = direction;
    captured_image.color = color;
}

void Image_paint(int index, int x, int y, int frame, int color)
{
    RendererStatus status = Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, RENDERER_STATUS_OK);

    if (status != RENDERER_STATUS_OK)
        return;
    image_attempts++;
    record_event(TEST_EVENT_IMAGE);
    capture_image(index, x, y, frame, 0, color);
    (void)Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, image_result);
}

void Image_paint_rotated(
    int index, int center_x, int center_y, int direction, int color)
{
    RendererStatus status = Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, RENDERER_STATUS_OK);

    if (status != RENDERER_STATUS_OK)
        return;
    rotated_image_attempts++;
    record_event(TEST_EVENT_ROTATED_IMAGE);
    capture_image(index, center_x, center_y, 0, direction, color);
    (void)Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, image_result);
}

static RendererStatus capture_text(
    TestEvent event, CapturedText *captured, int *attempts,
    RendererStatus result, font_data *font, int color,
    int horizontal_alignment, int vertical_alignment,
    int x, int y, int maximum_length, const char *format,
    va_list arguments)
{
    RendererStatus status = Sdl_renderer_track_frame_result(
        &fake_sdl_renderer, RENDERER_STATUS_OK);

    if (status != RENDERER_STATUS_OK)
        return status;
    (*attempts)++;
    record_event(event);
    captured->font = font;
    captured->color = color;
    captured->horizontal_alignment = horizontal_alignment;
    captured->vertical_alignment = vertical_alignment;
    captured->x = x;
    captured->y = y;
    captured->maximum_length = maximum_length;
    vsnprintf(captured->text, sizeof(captured->text), format, arguments);
    return Sdl_renderer_track_frame_result(&fake_sdl_renderer, result);
}

RendererStatus mapnprint(
    font_data *font, int color, int horizontal_alignment,
    int vertical_alignment, int x, int y, int maximum_length,
    const char *format, ...)
{
    RendererStatus status;
    va_list arguments;

    va_start(arguments, format);
    status = capture_text(
        TEST_EVENT_NAME, &captured_name, &name_attempts, name_result,
        font, color, horizontal_alignment, vertical_alignment,
        x, y, maximum_length, format, arguments);
    va_end(arguments);
    return status;
}

RendererStatus mapprint(
    font_data *font, int color, int horizontal_alignment,
    int vertical_alignment, int x, int y, const char *format, ...)
{
    RendererStatus status;
    va_list arguments;

    va_start(arguments, format);
    status = capture_text(
        TEST_EVENT_LIFE, &captured_life, &life_attempts, life_result,
        font, color, horizontal_alignment, vertical_alignment,
        x, y, -1, format, arguments);
    va_end(arguments);
    return status;
}

void GLAPIENTRY glEnable(GLenum capability)
{
    (void)capability;
    legacy_calls++;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    (void)capability;
    legacy_calls++;
}

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
    legacy_calls++;
}

void GLAPIENTRY glLineWidth(GLfloat width)
{
    (void)width;
    legacy_calls++;
}

void GLAPIENTRY glLineStipple(GLint factor, GLushort pattern)
{
    (void)factor;
    (void)pattern;
    legacy_calls++;
}

void GLAPIENTRY glColor4ub(
    GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    legacy_calls++;
}

void GLAPIENTRY glBegin(GLenum mode)
{
    (void)mode;
    legacy_calls++;
}

void GLAPIENTRY glVertex2d(GLdouble x, GLdouble y)
{
    (void)x;
    (void)y;
    legacy_calls++;
}

void GLAPIENTRY glEnd(void)
{
    legacy_calls++;
}

void __wrap_Gui_paint_paused(int x, int y, int count)
{
    (void)x;
    (void)y;
    (void)count;
}

void __wrap_Gui_paint_appearing_begin(void)
{
    record_event(TEST_EVENT_APPEARING_BEGIN);
}

void __wrap_Gui_paint_appearing(int x, int y, int id, int count)
{
    (void)x;
    (void)y;
    (void)id;
    (void)count;
    record_event(TEST_EVENT_APPEARING_ENTRY);
}

void __wrap_Gui_paint_appearing_end(void)
{
    record_event(TEST_EVENT_APPEARING_END);
}

void __wrap_Gui_paint_ecm(int x, int y, int size)
{
    (void)x;
    (void)y;
    (void)size;
}

void __wrap_Gui_paint_all_connectors_begin(void)
{
}

void __wrap_Gui_paint_refuel(int x_0, int y_0, int x_1, int y_1)
{
    (void)x_0;
    (void)y_0;
    (void)x_1;
    (void)y_1;
}

void __wrap_Gui_paint_connector(
    int x_0, int y_0, int x_1, int y_1, int tractor)
{
    (void)x_0;
    (void)y_0;
    (void)x_1;
    (void)y_1;
    (void)tractor;
}

void __wrap_Gui_paint_transporter(
    int x_0, int y_0, int x_1, int y_1)
{
    (void)x_0;
    (void)y_0;
    (void)x_1;
    (void)y_1;
}

static int color_equal(RendererColor actual, uint32_t expected_rgba)
{
    RendererColor expected = Renderer_color_from_rgba32(expected_rgba);

    return actual.red == expected.red
        && actual.green == expected.green
        && actual.blue == expected.blue
        && actual.alpha == expected.alpha;
}

static int check_triangle_points(const RendererPoint2D *points)
{
    TEST_CHECK(points[0].x == 102.5f);
    TEST_CHECK(points[0].y == 198.75f);
    TEST_CHECK(points[1].x == 97.0f);
    TEST_CHECK(points[1].y == 204.0f);
    TEST_CHECK(points[2].x == 101.0f);
    TEST_CHECK(points[2].y == 194.5f);
    return 0;
}

static int check_solid_outline(
    float width, uint32_t color, size_t point_count)
{
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(current_blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == 1);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(captured_stroke.point_count == point_count);
    TEST_CHECK(captured_stroke.width == width);
    TEST_CHECK(color_equal(captured_stroke.color, color));
    TEST_CHECK(captured_stroke.closed == 1);
    TEST_CHECK(captured_stroke.blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int allocate_actual_entries(void)
{
    appearing_ptr = malloc(sizeof(*appearing_ptr));
    ship_ptr = malloc(sizeof(*ship_ptr));
    if (appearing_ptr == NULL || ship_ptr == NULL) {
        free(appearing_ptr);
        free(ship_ptr);
        appearing_ptr = NULL;
        ship_ptr = NULL;
        return 1;
    }
    memset(appearing_ptr, 0, sizeof(*appearing_ptr));
    appearing_ptr[0].x = 50;
    appearing_ptr[0].y = 60;
    appearing_ptr[0].id = 20;
    appearing_ptr[0].count = 180;
    num_appearing = 1;
    max_appearing = 1;
    memset(ship_ptr, 0, sizeof(*ship_ptr));
    ship_ptr[0].x = 100;
    ship_ptr[0].y = 200;
    ship_ptr[0].id = 10;
    ship_ptr[0].dir = TEST_DIRECTION;
    num_ship = 1;
    max_ship = 1;
    return 0;
}

static int check_actual_traversal_orders_appearance_ship_and_release(void)
{
    reset_state();
    TEST_CHECK(allocate_actual_entries() == 0);

    Paint_ships();
    appearing_ptr = NULL;
    ship_ptr = NULL;

    TEST_CHECK(num_appearing == 0);
    TEST_CHECK(max_appearing == 0);
    TEST_CHECK(num_ship == 0);
    TEST_CHECK(max_ship == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 7);
    TEST_CHECK(events[0] == TEST_EVENT_APPEARING_BEGIN);
    TEST_CHECK(events[1] == TEST_EVENT_APPEARING_ENTRY);
    TEST_CHECK(events[2] == TEST_EVENT_APPEARING_END);
    TEST_CHECK(events[3] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[4] == TEST_EVENT_BLEND);
    TEST_CHECK(events[5] == TEST_EVENT_STROKE);
    TEST_CHECK(events[6] == TEST_EVENT_FLUSH);
    TEST_CHECK(lookup_count == 1);
    TEST_CHECK(check_solid_outline(
        2.5f, UINT32_C(0xffffffff), 3) == 0);
    return check_triangle_points(captured_stroke.points);
}

static int check_shield_outline_and_name_order(void)
{
    reset_state();
    fake_setup.mode = TEAM_PLAY;
    team3ColorRGBA = UINT32_C(0x12345680);
    shipNameColorRGBA = UINT32_C(0xabcdefe0);

    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 1, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 6);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_IMAGE);
    TEST_CHECK(events[2] == TEST_EVENT_BLEND);
    TEST_CHECK(events[3] == TEST_EVENT_STROKE);
    TEST_CHECK(events[4] == TEST_EVENT_FLUSH);
    TEST_CHECK(events[5] == TEST_EVENT_NAME);
    TEST_CHECK(image_attempts == 1);
    TEST_CHECK(captured_image.index == IMG_SHIELD);
    TEST_CHECK(captured_image.x == 73);
    TEST_CHECK(captured_image.y == 173);
    TEST_CHECK(captured_image.frame == 0);
    TEST_CHECK(captured_image.color == (int)UINT32_C(0x12345640));
    TEST_CHECK(check_solid_outline(
        2.5f, UINT32_C(0x12345680), 3) == 0);
    TEST_CHECK(check_triangle_points(captured_stroke.points) == 0);
    TEST_CHECK(name_attempts == 1);
    TEST_CHECK(captured_name.font == &mapfont);
    TEST_CHECK(captured_name.color == (int)UINT32_C(0xabcdefe0));
    TEST_CHECK(captured_name.horizontal_alignment == CENTER);
    TEST_CHECK(captured_name.vertical_alignment == DOWN);
    TEST_CHECK(captured_name.x == 100);
    TEST_CHECK(captured_name.y == 200 - SHIP_SZ);
    TEST_CHECK(captured_name.maximum_length == maxCharsInNames);
    TEST_CHECK(strcmp(captured_name.text, "TARGET") == 0);
    TEST_CHECK(life_attempts == 0);
    return legacy_calls == 0 ? 0 : 1;
}

static int check_stippled_outline(int cloak, int phased)
{
    reset_state();
    *ship_line_width_option = 4.25;

    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10,
        cloak, phased, 0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_BLEND);
    TEST_CHECK(events[2] == TEST_EVENT_STIPPLED_STROKE);
    TEST_CHECK(events[3] == TEST_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(current_blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 1);
    TEST_CHECK(successful_stippled_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(captured_stippled_stroke.point_count == 3);
    TEST_CHECK(captured_stippled_stroke.width == 4.25f);
    TEST_CHECK(color_equal(
        captured_stippled_stroke.color, UINT32_C(0xffffffff)));
    TEST_CHECK(captured_stippled_stroke.closed == 1);
    TEST_CHECK(captured_stippled_stroke.factor == 3);
    TEST_CHECK(captured_stippled_stroke.pattern == UINT16_C(0xaaaa));
    TEST_CHECK(captured_stippled_stroke.blend
               == RENDERER_BLEND_ALPHA);
    TEST_CHECK(check_triangle_points(
        captured_stippled_stroke.points) == 0);
    return legacy_calls == 0 ? 0 : 1;
}

static int check_solid_cloaked_and_phased_outlines(void)
{
    reset_state();
    fake_setup.mode = TEAM_PLAY;
    team3ColorRGBA = UINT32_C(0x102030a0);
    *ship_line_width_option = 3.5;
    shipNameColorRGBA = 0;

    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_BLEND);
    TEST_CHECK(events[2] == TEST_EVENT_STROKE);
    TEST_CHECK(events[3] == TEST_EVENT_FLUSH);
    TEST_CHECK(check_solid_outline(
        3.5f, UINT32_C(0x102030a0), 3) == 0);
    TEST_CHECK(name_attempts == 0);

    TEST_CHECK(check_stippled_outline(1, 0) == 0);
    return check_stippled_outline(0, 1);
}

static int check_textured_image(
    int id, int cloak, int phased, int expected_image,
    int expected_color)
{
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, id,
        cloak, phased, 0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_ROTATED_IMAGE);
    TEST_CHECK(rotated_image_attempts == 1);
    TEST_CHECK(captured_image.index == expected_image);
    TEST_CHECK(captured_image.x == 100);
    TEST_CHECK(captured_image.y == 200);
    TEST_CHECK(captured_image.direction == TEST_DIRECTION);
    TEST_CHECK(captured_image.color == expected_color);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(name_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_textured_ship_regression(void)
{
    reset_state();
    *textured_ships_option = true;
    fake_setup.mode = TEAM_PLAY;
    team3ColorRGBA = UINT32_C(0x11223380);
    players[1].team = players[0].team;
    team2ColorRGBA = UINT32_C(0x11223380);
    TEST_CHECK(check_textured_image(
        20, 1, 0, IMG_SHIP_FRIEND,
        (int)UINT32_C(0x11223340)) == 0);

    reset_state();
    *textured_ships_option = true;
    fake_setup.mode = TEAM_PLAY;
    team3ColorRGBA = UINT32_C(0x44556680);
    TEST_CHECK(check_textured_image(
        20, 0, 0, IMG_SHIP_ENEMY,
        (int)UINT32_C(0x44556680)) == 0);

    reset_state();
    *textured_ships_option = true;
    fake_setup.mode = 0;
    TEST_CHECK(check_textured_image(
        10, 0, 1, IMG_SHIP_SELF,
        (int)UINT32_C(0xffffff7f)) == 0);
    return 0;
}

static int check_maximum_shape_is_supported(void)
{
    reset_state();
    set_maximum_shape(&player_shapes[0], player_shape_points[0]);

    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(check_solid_outline(
        2.5f, UINT32_C(0xffffffff), MAX_SHIP_PTS2) == 0);
    TEST_CHECK(captured_stroke.points[0].x == 100.0f);
    TEST_CHECK(captured_stroke.points[0].y == 198.0f);
    TEST_CHECK(captured_stroke.points[MAX_SHIP_PTS2 - 1].x
               == (float)(100 + MAX_SHIP_PTS2 - 1));
    TEST_CHECK(captured_stroke.points[MAX_SHIP_PTS2 - 1].y == 202.0f);
    return 0;
}

static int check_invalid_ship_is_cpu_atomic(void)
{
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(lookup_count == 1);
    TEST_CHECK(image_attempts == 0);
    TEST_CHECK(rotated_image_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(name_attempts == 0);
    TEST_CHECK(life_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_direction_and_point_validation(void)
{
    reset_state();
    Gui_paint_ship(100, 200, -1, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    Gui_paint_ship(100, 200, -1, 10, 0, 0, 1, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    Gui_paint_ship(100, 200, RES, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    player_shapes[0].num_points = 0;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    player_shapes[0].num_points = MIN_SHIP_PTS - 1;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    player_shapes[0].num_points = MAX_SHIP_PTS2 + 1;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    players[0].ship = NULL;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    player_shapes[0].pts[2] = NULL;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    return check_invalid_ship_is_cpu_atomic();
}

static int check_numeric_and_float_collapse_validation(void)
{
    reset_state();
    Gui_paint_ship(
        INT_MAX, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    Gui_paint_ship(
        100, INT_MIN, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);

    reset_state();
    player_shape_points[0][0][TEST_DIRECTION].cx = 0;
    player_shape_points[0][0][TEST_DIRECTION].cy = 0;
    player_shape_points[0][1][TEST_DIRECTION].cx = CLICK;
    player_shape_points[0][1][TEST_DIRECTION].cy = 2 * CLICK;
    player_shape_points[0][2][TEST_DIRECTION].cx = -CLICK;
    player_shape_points[0][2][TEST_DIRECTION].cy = -2 * CLICK;
    Gui_paint_ship(
        1 << 24, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    return check_invalid_ship_is_cpu_atomic();
}

static int check_line_width_validation(void)
{
    static const double invalid_widths[] = {
        0.0, -1.0, NAN, INFINITY, -INFINITY, DBL_MAX
    };
    size_t width_index;

    for (width_index = 0;
         width_index < NELEM(invalid_widths);
         width_index++) {
        reset_state();
        *ship_line_width_option = invalid_widths[width_index];
        Gui_paint_ship(
            100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
        TEST_CHECK(check_invalid_ship_is_cpu_atomic() == 0);
    }
    return 0;
}

static int check_degenerate_outline_is_a_noop(void)
{
    int point_index;

    reset_state();
    for (point_index = 0;
         point_index < player_shapes[1].num_points;
         point_index++) {
        player_shape_points[1][point_index][TEST_DIRECTION].cx = 0;
        player_shape_points[1][point_index][TEST_DIRECTION].cy = 0;
    }
    shipNameColorRGBA = UINT32_C(0xabcdefe0);

    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_NAME);
    TEST_CHECK(lookup_count == 1);
    TEST_CHECK(image_attempts == 0);
    TEST_CHECK(rotated_image_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(name_attempts == 1);
    TEST_CHECK(life_attempts == 0);
    TEST_CHECK(captured_name.font == &mapfont);
    TEST_CHECK(captured_name.color == (int)UINT32_C(0xabcdefe0));
    TEST_CHECK(captured_name.horizontal_alignment == CENTER);
    TEST_CHECK(captured_name.vertical_alignment == DOWN);
    TEST_CHECK(captured_name.x == 100);
    TEST_CHECK(captured_name.y == 200 - SHIP_SZ);
    TEST_CHECK(captured_name.maximum_length == maxCharsInNames);
    TEST_CHECK(strcmp(captured_name.text, "TARGET") == 0);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_renderer_failure_is_sticky(
    RendererStatus expected, int expected_events,
    TestEvent final_event)
{
    int previous_event_count;

    TEST_CHECK(fake_sdl_renderer.frame_result == expected);
    TEST_CHECK(event_count == expected_events);
    TEST_CHECK(events[expected_events - 1] == final_event);
    TEST_CHECK(name_attempts == 0);
    TEST_CHECK(life_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    previous_event_count = event_count;
    image_result = RENDERER_STATUS_OK;
    blend_result = RENDERER_STATUS_OK;
    stroke_result = RENDERER_STATUS_OK;
    stippled_stroke_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(event_count == previous_event_count + 1);
    TEST_CHECK(events[previous_event_count]
               == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(fake_sdl_renderer.frame_result == expected);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_renderer_failures_are_sticky(void)
{
    reset_state();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_renderer_failure_is_sticky(
        RENDERER_STATUS_RESOURCE_MISMATCH, 2,
        TEST_EVENT_BLEND) == 0);

    reset_state();
    stroke_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(successful_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_renderer_failure_is_sticky(
        RENDERER_STATUS_OUT_OF_MEMORY, 3,
        TEST_EVENT_STROKE) == 0);

    reset_state();
    stippled_stroke_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 1, 0, 0, 0, 0);
    TEST_CHECK(successful_stippled_strokes == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_renderer_failure_is_sticky(
        RENDERER_STATUS_OUT_OF_MEMORY, 3,
        TEST_EVENT_STIPPLED_STROKE) == 0);

    reset_state();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 10, 0, 0, 0, 0, 0);
    TEST_CHECK(successful_strokes == 1);
    TEST_CHECK(flush_attempts == 1);
    return check_renderer_failure_is_sticky(
        RENDERER_STATUS_BACKEND_ERROR, 4,
        TEST_EVENT_FLUSH);
}

static int check_shield_and_name_failures_are_sticky(void)
{
    int previous_event_count;

    reset_state();
    image_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 1, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_IMAGE);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(name_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    reset_state();
    shipNameColorRGBA = UINT32_C(0xabcdefe0);
    name_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 5);
    TEST_CHECK(events[0] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(events[1] == TEST_EVENT_BLEND);
    TEST_CHECK(events[2] == TEST_EVENT_STROKE);
    TEST_CHECK(events[3] == TEST_EVENT_FLUSH);
    TEST_CHECK(events[4] == TEST_EVENT_NAME);
    TEST_CHECK(name_attempts == 1);
    TEST_CHECK(legacy_calls == 0);
    previous_event_count = event_count;
    name_result = RENDERER_STATUS_OK;
    Gui_paint_ship(
        100, 200, TEST_DIRECTION, 20, 0, 0, 0, 0, 0);
    TEST_CHECK(event_count == previous_event_count + 1);
    TEST_CHECK(events[previous_event_count]
               == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(name_attempts == 1);
    return legacy_calls == 0 ? 0 : 1;
}

static int check_preexisting_failure_still_traverses_and_releases(void)
{
    reset_state();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(allocate_actual_entries() == 0);

    Paint_ships();
    appearing_ptr = NULL;
    ship_ptr = NULL;

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(num_appearing == 0);
    TEST_CHECK(max_appearing == 0);
    TEST_CHECK(num_ship == 0);
    TEST_CHECK(max_ship == 0);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == TEST_EVENT_APPEARING_BEGIN);
    TEST_CHECK(events[1] == TEST_EVENT_APPEARING_ENTRY);
    TEST_CHECK(events[2] == TEST_EVENT_APPEARING_END);
    TEST_CHECK(events[3] == TEST_EVENT_SHIP_LOOKUP);
    TEST_CHECK(image_attempts == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(stippled_stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(name_attempts == 0);
    return legacy_calls == 0 ? 0 : 1;
}

int main(void)
{
    int failed;

    Store_sdlgui_options();
    if (textured_ships_option == NULL
        || ship_line_width_option == NULL) {
        return EXIT_FAILURE;
    }

    /* Stop on the intended legacy-vs-semantic RED before invalid inputs. */
    failed = check_actual_traversal_orders_appearance_ship_and_release();
    if (failed)
        return EXIT_FAILURE;

    failed |= check_shield_outline_and_name_order();
    failed |= check_solid_cloaked_and_phased_outlines();
    failed |= check_textured_ship_regression();
    failed |= check_maximum_shape_is_supported();
    failed |= check_direction_and_point_validation();
    failed |= check_numeric_and_float_collapse_validation();
    failed |= check_line_width_validation();
    failed |= check_degenerate_outline_is_a_noop();
    failed |= check_renderer_failures_are_sticky();
    failed |= check_shield_and_name_failures_are_sticky();
    failed |= check_preexisting_failure_still_traverses_and_releases();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
