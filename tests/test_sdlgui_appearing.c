#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "appearing_batch_test_support.h"
#include "paint.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <GL/gl.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PLAYER_COUNT 3
#define MAX_RENDER_EVENTS 8
#define MAX_CAPTURED_VERTICES 64
#define MAX_LOOKUPS 32

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

typedef enum RenderEvent {
    RENDER_EVENT_BLEND,
    RENDER_EVENT_DRAW,
    RENDER_EVENT_FLUSH
} RenderEvent;

typedef enum LookupKind {
    LOOKUP_OTHER,
    LOOKUP_HOMEBASE
} LookupKind;

typedef struct LookupCall {
    LookupKind kind;
    int id;
} LookupCall;

typedef struct CapturedDraw {
    RendererVertex2D vertices[MAX_CAPTURED_VERTICES];
    size_t vertex_count;
    RendererBlendMode blend;
    int lookup_count_at_draw;
} CapturedDraw;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer;
static RenderEvent render_events[MAX_RENDER_EVENTS];
static LookupCall lookups[MAX_LOOKUPS];
static CapturedDraw captured_draw;
static other_t others[TEST_PLAYER_COUNT];
static homebase_t homebases[TEST_PLAYER_COUNT];
static setup_t fake_setup;
static int render_event_count;
static int lookup_count;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int flush_attempts;
static int legacy_calls;
static int realloc_attempts;
static int fail_realloc_attempt;
static RendererBlendMode current_blend;
static RendererStatus blend_result;
static RendererStatus draw_result;
static RendererStatus flush_result;

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

int eyesId;
other_t *eyes;
int eyeTeam;

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

extern Uint32 manyLivesColorRGBA;
extern Uint32 oneLifeColorRGBA;
extern Uint32 redRGBA;

void *__real_realloc(void *pointer, size_t size);

void *__wrap_realloc(void *pointer, size_t size)
{
    realloc_attempts++;
    if (realloc_attempts == fail_realloc_attempt)
        return NULL;
    return __real_realloc(pointer, size);
}

static void record_render_event(RenderEvent event)
{
    if (render_event_count < MAX_RENDER_EVENTS)
        render_events[render_event_count++] = event;
}

static void record_lookup(LookupKind kind, int id)
{
    if (lookup_count < MAX_LOOKUPS) {
        lookups[lookup_count].kind = kind;
        lookups[lookup_count].id = id;
    }
    lookup_count++;
}

static void reset_state(void)
{
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    memset(&fake_sdl_renderer, 0, sizeof(fake_sdl_renderer));
    memset(render_events, 0, sizeof(render_events));
    memset(lookups, 0, sizeof(lookups));
    memset(&captured_draw, 0, sizeof(captured_draw));
    memset(others, 0, sizeof(others));
    memset(homebases, 0, sizeof(homebases));
    memset(&fake_setup, 0, sizeof(fake_setup));
    memset(&clData, 0, sizeof(clData));

    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    fake_setup.mode = LIMITED_LIVES;
    render_event_count = 0;
    lookup_count = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    flush_attempts = 0;
    legacy_calls = 0;
    realloc_attempts = 0;
    fail_realloc_attempt = 0;
    current_blend = RENDERER_BLEND_OPAQUE;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;

    others[0].id = 10;
    others[0].mychar = ' ';
    others[0].life = 4;
    others[1].id = 20;
    others[1].mychar = 'R';
    others[1].life = 1;
    others[2].id = 40;
    others[2].mychar = ' ';
    others[2].life = 2;
    homebases[0].id = 10;
    homebases[1].id = 20;
    homebases[2].id = 30;

    manyLivesColorRGBA = UINT32_C(0x11223344);
    oneLifeColorRGBA = UINT32_C(0x55667788);
    redRGBA = UINT32_C(0x99aabbcc);
    version = UINT32_C(0x4F12);
    loops = 1000;
    loopsSlow = 0;
    clientFPS = 60.0;
    world.x = -1000;
    world.y = -1000;
    realWorld = world;
    ext_view_width = 2000;
    ext_view_height = 2000;

    eyesId = 0;
    eyes = NULL;
    eyeTeam = TEAM_NOT_SET;
    refuel_ptr = NULL;
    num_refuel = 0;
    max_refuel = 0;
    connector_ptr = NULL;
    num_connector = 0;
    max_connector = 0;
    ship_ptr = NULL;
    num_ship = 0;
    max_ship = 0;
    ecm_ptr = NULL;
    num_ecm = 0;
    max_ecm = 0;
    trans_ptr = NULL;
    num_trans = 0;
    max_trans = 0;
    paused_ptr = NULL;
    num_paused = 0;
    max_paused = 0;
    appearing_ptr = NULL;
    num_appearing = 0;
    max_appearing = 0;
}

static int player_index_by_id(int id)
{
    int player_index;

    for (player_index = 0; player_index < TEST_PLAYER_COUNT;
         player_index++) {
        if (others[player_index].id == id)
            return player_index;
    }
    return -1;
}

static int homebase_index_by_id(int id)
{
    int homebase_index;

    for (homebase_index = 0; homebase_index < TEST_PLAYER_COUNT;
         homebase_index++) {
        if (homebases[homebase_index].id == id)
            return homebase_index;
    }
    return -1;
}

other_t *Other_by_id(int id)
{
    int player_index;

    record_lookup(LOOKUP_OTHER, id);
    player_index = player_index_by_id(id);
    return player_index >= 0 ? &others[player_index] : NULL;
}

homebase_t *Homebase_by_id(int id)
{
    int homebase_index;

    record_lookup(LOOKUP_HOMEBASE, id);
    homebase_index = homebase_index_by_id(id);
    return homebase_index >= 0 ? &homebases[homebase_index] : NULL;
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
    record_render_event(RENDER_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (blend_result != RENDERER_STATUS_OK)
        return blend_result;
    current_blend = blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_triangles(
    Renderer *renderer, RendererTexture *texture,
    const RendererVertex2D *vertices, size_t vertex_count)
{
    draw_attempts++;
    record_render_event(RENDER_EVENT_DRAW);
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture != NULL || vertices == NULL || vertex_count == 0
        || vertex_count % 3 != 0
        || vertex_count > MAX_CAPTURED_VERTICES) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    memcpy(captured_draw.vertices, vertices,
           vertex_count * sizeof(captured_draw.vertices[0]));
    captured_draw.vertex_count = vertex_count;
    captured_draw.blend = current_blend;
    captured_draw.lookup_count_at_draw = lookup_count;
    successful_draws++;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(
    SdlRenderer *renderer)
{
    flush_attempts++;
    record_render_event(RENDER_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer)
        return RENDERER_STATUS_INVALID_STATE;
    return flush_result;
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

void GLAPIENTRY glEnable(GLenum capability)
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

void GLAPIENTRY glBegin(GLenum mode)
{
    (void)mode;
    legacy_calls++;
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
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

void __wrap_Gui_paint_ecm(int x, int y, int size)
{
    (void)x;
    (void)y;
    (void)size;
}

void __wrap_Gui_paint_ship(
    int x, int y, int dir, int id, int cloak, int phased,
    int shield, int deflector, int eshield)
{
    (void)x;
    (void)y;
    (void)dir;
    (void)id;
    (void)cloak;
    (void)phased;
    (void)shield;
    (void)deflector;
    (void)eshield;
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

void __wrap_Gui_paint_transporter(int x_0, int y_0, int x_1, int y_1)
{
    (void)x_0;
    (void)y_0;
    (void)x_1;
    (void)y_1;
}

static int color_equal(RendererColor actual, uint32_t expected_rgba)
{
    RendererColor expected = Renderer_color_from_rgba32(expected_rgba);

    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int check_lookup_order(const int *ids, size_t id_count)
{
    size_t id_index;

    TEST_CHECK(lookup_count == (int)(2 * id_count));
    for (id_index = 0; id_index < id_count; id_index++) {
        TEST_CHECK(lookups[2 * id_index].kind == LOOKUP_OTHER);
        TEST_CHECK(lookups[2 * id_index].id == ids[id_index]);
        TEST_CHECK(lookups[2 * id_index + 1].kind == LOOKUP_HOMEBASE);
        TEST_CHECK(lookups[2 * id_index + 1].id == ids[id_index]);
    }
    return 0;
}

static int check_quad(
    size_t first_vertex, int left, int top, int right, int bottom,
    uint32_t rgba)
{
    static const int corners[6][2] = {
        {0, 0}, {1, 0}, {1, 1},
        {0, 0}, {1, 1}, {0, 1}
    };
    size_t corner_index;

    TEST_CHECK(first_vertex + 6 <= captured_draw.vertex_count);
    for (corner_index = 0; corner_index < 6; corner_index++) {
        const RendererVertex2D *vertex =
            &captured_draw.vertices[first_vertex + corner_index];
        float expected_x = (float)(corners[corner_index][0]
                                       ? right : left);
        float expected_y = (float)(corners[corner_index][1]
                                       ? bottom : top);

        TEST_CHECK(vertex->x == expected_x);
        TEST_CHECK(vertex->y == expected_y);
        TEST_CHECK(vertex->u == 0.0f);
        TEST_CHECK(vertex->v == 0.0f);
        TEST_CHECK(color_equal(vertex->color, rgba));
    }
    return 0;
}

static int check_no_legacy_calls(void)
{
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_successful_semantic_batch(size_t vertex_count)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(render_event_count == 3);
    TEST_CHECK(render_events[0] == RENDER_EVENT_BLEND);
    TEST_CHECK(render_events[1] == RENDER_EVENT_DRAW);
    TEST_CHECK(render_events[2] == RENDER_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(current_blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(captured_draw.vertex_count == vertex_count);
    TEST_CHECK(captured_draw.blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(flush_attempts == 1);
    return check_no_legacy_calls();
}

static int set_actual_appearing_entries(
    const SdlguiTestAppearing *entries, size_t entry_count)
{
    size_t entry_index;

    appearing_ptr = malloc(entry_count * sizeof(*appearing_ptr));
    if (appearing_ptr == NULL)
        return 1;
    for (entry_index = 0; entry_index < entry_count; entry_index++) {
        appearing_ptr[entry_index].x = (short)entries[entry_index].x;
        appearing_ptr[entry_index].y = (short)entries[entry_index].y;
        appearing_ptr[entry_index].id = (short)entries[entry_index].id;
        appearing_ptr[entry_index].count = (short)entries[entry_index].count;
    }
    num_appearing = (int)entry_count;
    max_appearing = (int)entry_count;
    return 0;
}

static int check_actual_traversal_batches_geometry_and_colors(void)
{
    static const SdlguiTestAppearing entries[] = {
        {100, 200, 10, 0},
        {-50, 25, 20, 180},
        {300, -400, 30, 360}
    };
    static const int ids[] = {10, 20, 30};

    reset_state();
    TEST_CHECK(set_actual_appearing_entries(entries, NELEM(entries)) == 0);

    Paint_ships();
    appearing_ptr = NULL;

    TEST_CHECK(num_appearing == 0);
    TEST_CHECK(max_appearing == 0);
    TEST_CHECK(check_lookup_order(ids, NELEM(ids)) == 0);
    TEST_CHECK(homebases[0].appeartime == 1000);
    TEST_CHECK(homebases[1].appeartime == 1090);
    TEST_CHECK(homebases[2].appeartime == 1180);
    TEST_CHECK(check_successful_semantic_batch(18) == 0);
    TEST_CHECK(captured_draw.lookup_count_at_draw == 6);
    TEST_CHECK(check_quad(
        0, 85, 185, 116, 186, UINT32_C(0x11223344)) == 0);
    TEST_CHECK(check_quad(
        6, -65, 10, -34, 26, UINT32_C(0x55667788)) == 0);
    return check_quad(
        12, 285, -415, 316, -384, UINT32_C(0x99aabbcc));
}

static int check_standalone_appearing_is_semantic(void)
{
    reset_state();

    Gui_paint_appearing(100, 200, 10, 180);

    TEST_CHECK(lookup_count == 2);
    TEST_CHECK(lookups[0].kind == LOOKUP_OTHER);
    TEST_CHECK(lookups[0].id == 10);
    TEST_CHECK(lookups[1].kind == LOOKUP_HOMEBASE);
    TEST_CHECK(lookups[1].id == 10);
    TEST_CHECK(homebases[0].appeartime == 1090);
    TEST_CHECK(check_successful_semantic_batch(6) == 0);
    TEST_CHECK(captured_draw.lookup_count_at_draw == 2);
    return check_quad(
        0, 85, 185, 116, 201, UINT32_C(0x11223344));
}

static int check_invalid_geometry_is_batch_atomic(void)
{
    static const SdlguiTestAppearing entries[] = {
        {10, 20, 10, 180},
        {INT32_C(1073741824), 20, 20, 180},
        {30, 40, 30, 360}
    };
    static const int ids[] = {10, 20, 30};
    int previous_render_event_count;
    int previous_blend_attempts;
    int previous_draw_attempts;
    int previous_flush_attempts;

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, NELEM(entries)) == RENDERER_STATUS_INVALID_ARGUMENT);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(check_lookup_order(ids, NELEM(ids)) == 0);
    TEST_CHECK(homebases[0].appeartime == 1090);
    TEST_CHECK(homebases[1].appeartime == 1090);
    TEST_CHECK(homebases[2].appeartime == 1180);
    TEST_CHECK(render_event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_no_legacy_calls() == 0);

    previous_render_event_count = render_event_count;
    previous_blend_attempts = blend_attempts;
    previous_draw_attempts = draw_attempts;
    previous_flush_attempts = flush_attempts;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, 1) == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(lookup_count == 8);
    TEST_CHECK(render_event_count == previous_render_event_count);
    TEST_CHECK(blend_attempts == previous_blend_attempts);
    TEST_CHECK(draw_attempts == previous_draw_attempts);
    TEST_CHECK(flush_attempts == previous_flush_attempts);
    return check_no_legacy_calls();
}

static int check_actual_invalid_batch_keeps_traversal_and_cleanup(void)
{
    static const SdlguiTestAppearing entries[] = {
        {10, 20, 10, 180},
        {30, 40, 20, -1},
        {50, 60, 30, 360}
    };
    static const int ids[] = {10, 20, 30};

    reset_state();
    TEST_CHECK(set_actual_appearing_entries(entries, NELEM(entries)) == 0);

    Paint_ships();
    appearing_ptr = NULL;

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(num_appearing == 0);
    TEST_CHECK(max_appearing == 0);
    TEST_CHECK(check_lookup_order(ids, NELEM(ids)) == 0);
    TEST_CHECK(homebases[0].appeartime == 1090);
    TEST_CHECK(homebases[1].appeartime == 999);
    TEST_CHECK(homebases[2].appeartime == 1180);
    TEST_CHECK(render_event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_calls();
}

static int check_invalid_numeric_inputs_are_sticky(void)
{
    static const SdlguiTestAppearing invalid_entries[] = {
        {INT_MAX, 20, 10, 0},
        {INT_MIN, 20, 10, 0},
        {10, INT_MAX, 10, 360},
        {10, INT_MIN, 10, 360},
        {10, 20, 10, -1},
        {INT32_C(1073741824), 20, 10, 180},
        {10, INT32_C(16777231), 10, 0}
    };
    size_t entry_index;

    for (entry_index = 0; entry_index < NELEM(invalid_entries);
         entry_index++) {
        reset_state();
        TEST_CHECK(Sdlgui_test_paint_appearing_batch(
            &invalid_entries[entry_index], 1)
            == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(fake_sdl_renderer.frame_result
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(draw_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        TEST_CHECK(lookup_count == 2);
        TEST_CHECK(check_no_legacy_calls() == 0);
    }
    return 0;
}

static int check_unclamped_large_counts_are_valid(void)
{
    static const SdlguiTestAppearing entries[] = {
        {10, 20, 10, 361},
        {30, 40, 20, INT_MAX}
    };
    int maximum_bottom = 25
        + (int)((double)INT_MAX / 180.0 * 15.0 + 1.0);

    reset_state();
    version = UINT32_C(0x4F11);
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, NELEM(entries)) == RENDERER_STATUS_OK);

    TEST_CHECK(lookup_count == 2);
    TEST_CHECK(lookups[0].kind == LOOKUP_OTHER);
    TEST_CHECK(lookups[0].id == 10);
    TEST_CHECK(lookups[1].kind == LOOKUP_OTHER);
    TEST_CHECK(lookups[1].id == 20);
    TEST_CHECK(check_successful_semantic_batch(12) == 0);
    TEST_CHECK(check_quad(
        0, -5, 5, 26, 36, UINT32_C(0x11223344)) == 0);
    return check_quad(
        6, 15, 25, 46, maximum_bottom, UINT32_C(0x55667788));
}

static int check_invalid_appeartime_inputs_do_not_mutate_bases(void)
{
    static const double invalid_fps[] = {
        NAN, INFINITY, -INFINITY, DBL_MAX
    };
    static const SdlguiTestAppearing entry = {10, 20, 10, 180};
    size_t fps_index;

    for (fps_index = 0; fps_index < NELEM(invalid_fps); fps_index++) {
        reset_state();
        homebases[0].appeartime = 777;
        clientFPS = invalid_fps[fps_index];

        TEST_CHECK(Sdlgui_test_paint_appearing_batch(&entry, 1)
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(fake_sdl_renderer.frame_result
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(homebases[0].appeartime == 777);
        TEST_CHECK(lookup_count == 2);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(draw_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        TEST_CHECK(check_no_legacy_calls() == 0);
    }

    reset_state();
    homebases[0].appeartime = 777;
    loops = LONG_MAX;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(&entry, 1)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(homebases[0].appeartime == 777);
    TEST_CHECK(lookup_count == 2);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_calls();
}

static int check_allocation_failure_is_sticky(void)
{
    static const SdlguiTestAppearing entries[] = {
        {10, 20, 10, 180},
        {30, 40, 20, 360}
    };
    int previous_blend_attempts;
    int previous_draw_attempts;
    int previous_flush_attempts;
    int previous_realloc_attempts;

    reset_state();
    fail_realloc_attempt = 1;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, NELEM(entries)) == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(realloc_attempts == 1);
    TEST_CHECK(lookup_count == 4);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(homebases[0].appeartime == 1090);
    TEST_CHECK(homebases[1].appeartime == 1180);
    TEST_CHECK(check_no_legacy_calls() == 0);

    previous_blend_attempts = blend_attempts;
    previous_draw_attempts = draw_attempts;
    previous_flush_attempts = flush_attempts;
    previous_realloc_attempts = realloc_attempts;
    fail_realloc_attempt = 0;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, 1) == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(lookup_count == 6);
    TEST_CHECK(blend_attempts == previous_blend_attempts);
    TEST_CHECK(draw_attempts == previous_draw_attempts);
    TEST_CHECK(flush_attempts == previous_flush_attempts);
    TEST_CHECK(realloc_attempts == previous_realloc_attempts);
    return check_no_legacy_calls();
}

static int check_blend_failure_keeps_side_effects_and_is_sticky(void)
{
    static const SdlguiTestAppearing entries[] = {
        {10, 20, 10, 180},
        {30, 40, 20, 360}
    };
    int previous_render_event_count;

    reset_state();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, NELEM(entries)) == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(lookup_count == 4);
    TEST_CHECK(render_event_count == 1);
    TEST_CHECK(render_events[0] == RENDER_EVENT_BLEND);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(homebases[0].appeartime == 1090);
    TEST_CHECK(homebases[1].appeartime == 1180);
    TEST_CHECK(check_no_legacy_calls() == 0);

    previous_render_event_count = render_event_count;
    blend_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        entries, 1) == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(lookup_count == 6);
    TEST_CHECK(render_event_count == previous_render_event_count);
    return check_no_legacy_calls();
}

static int check_draw_and_flush_failures_are_sticky(void)
{
    static const SdlguiTestAppearing entry = {10, 20, 10, 180};
    int previous_render_event_count;

    reset_state();
    draw_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        &entry, 1) == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(render_event_count == 2);
    TEST_CHECK(render_events[0] == RENDER_EVENT_BLEND);
    TEST_CHECK(render_events[1] == RENDER_EVENT_DRAW);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(lookup_count == 2);
    TEST_CHECK(check_no_legacy_calls() == 0);

    reset_state();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        &entry, 1) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(render_event_count == 3);
    TEST_CHECK(render_events[0] == RENDER_EVENT_BLEND);
    TEST_CHECK(render_events[1] == RENDER_EVENT_DRAW);
    TEST_CHECK(render_events[2] == RENDER_EVENT_FLUSH);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(check_no_legacy_calls() == 0);

    previous_render_event_count = render_event_count;
    flush_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdlgui_test_paint_appearing_batch(
        &entry, 1) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(lookup_count == 4);
    TEST_CHECK(render_event_count == previous_render_event_count);
    return check_no_legacy_calls();
}

static int check_failed_frame_still_runs_actual_side_effects_and_cleanup(void)
{
    static const SdlguiTestAppearing entries[] = {
        {100, 200, 10, 0},
        {-50, 25, 20, 180},
        {300, -400, 30, 360}
    };
    static const int ids[] = {10, 20, 30};

    reset_state();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(set_actual_appearing_entries(entries, NELEM(entries)) == 0);

    Paint_ships();
    appearing_ptr = NULL;

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(num_appearing == 0);
    TEST_CHECK(max_appearing == 0);
    TEST_CHECK(check_lookup_order(ids, NELEM(ids)) == 0);
    TEST_CHECK(homebases[0].appeartime == 1000);
    TEST_CHECK(homebases[1].appeartime == 1090);
    TEST_CHECK(homebases[2].appeartime == 1180);
    TEST_CHECK(render_event_count == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return check_no_legacy_calls();
}

int main(void)
{
    int failed = 0;

    failed |= check_allocation_failure_is_sticky();
    failed |= check_actual_traversal_batches_geometry_and_colors();
    failed |= check_standalone_appearing_is_semantic();
    failed |= check_invalid_geometry_is_batch_atomic();
    failed |= check_actual_invalid_batch_keeps_traversal_and_cleanup();
    failed |= check_invalid_numeric_inputs_are_sticky();
    failed |= check_unclamped_large_counts_are_valid();
    failed |= check_invalid_appeartime_inputs_do_not_mutate_bases();
    failed |= check_blend_failure_keeps_side_effects_and_is_sticky();
    failed |= check_draw_and_flush_failures_are_sticky();
    failed |= check_failed_frame_still_runs_actual_side_effects_and_cleanup();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
