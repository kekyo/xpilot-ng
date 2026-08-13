#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "guimap.h"
#include "images.h"
#include "polygon_cache_test_support.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <GL/gl.h>

#include <stdarg.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CAPTURED_DRAWS 8
#define MAX_CAPTURED_STROKES 16
#define MAX_CAPTURED_VERTICES 64
#define MAX_CAPTURED_POINTS 16
#define MAX_EVENTS 32

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    Renderer *owner;
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

typedef struct CapturedDraw {
    RendererVertex2D vertices[MAX_CAPTURED_VERTICES];
    size_t vertex_count;
    RendererTexture *texture;
    RendererBlendMode blend;
} CapturedDraw;

typedef struct CapturedStroke {
    RendererPoint2D points[MAX_CAPTURED_POINTS];
    size_t point_count;
    float width;
    RendererColor color;
    int closed;
    RendererBlendMode blend;
} CapturedStroke;

static Renderer fake_renderer;
static Renderer other_renderer;
static RendererTexture fake_texture;
static SdlRenderer fake_sdl_renderer;
static image_t fake_map_texture;
static setup_t fake_setup;
static xp_polygon_t scene_polygons[3];
static polygon_style_t scene_polygon_styles[4];
static edge_style_t scene_edge_styles[4];
static ipos_t first_points[7];
static ipos_t second_points[4];
static ipos_t invalid_points[3];
static int first_edge_styles[7];
static CapturedDraw draws[MAX_CAPTURED_DRAWS];
static CapturedStroke strokes[MAX_CAPTURED_STROKES];
static TestEvent events[MAX_EVENTS];
static int draw_count;
static int stroke_count;
static int event_count;
static int blend_attempts;
static int draw_attempts;
static int stroke_attempts;
static int flush_attempts;
static int texture_lookups;
static int last_texture_index;
static int texture_available;
static int legacy_calls;
static RendererBlendMode current_blend;
static RendererStatus blend_result;
static RendererStatus draw_result;
static RendererStatus stroke_result;
static RendererStatus flush_result;
static size_t allocation_attempts;
static size_t fail_allocation_attempt;

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);

void *__wrap_malloc(size_t size)
{
    allocation_attempts++;
    if (allocation_attempts == fail_allocation_attempt)
        return NULL;
    return __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    allocation_attempts++;
    if (allocation_attempts == fail_allocation_attempt)
        return NULL;
    return __real_calloc(count, size);
}

void *__wrap_realloc(void *pointer, size_t size)
{
    allocation_attempts++;
    if (allocation_attempts == fail_allocation_attempt)
        return NULL;
    return __real_realloc(pointer, size);
}

setup_t *Setup = &fake_setup;
client_data_t clData;
instruments_t instruments;
ipos_t world;
ipos_t realWorld;
short ext_view_width;
short ext_view_height;
int active_view_width;
int active_view_height;
int ext_view_x_offset;
int ext_view_y_offset;
unsigned draw_width;
unsigned draw_height;

xp_polygon_t *polygons;
int num_polygons;
int max_polygons;
polygon_style_t *polygon_styles;
int num_polygon_styles;
int max_polygon_styles;
edge_style_t *edge_styles;
int num_edge_styles;
int max_edge_styles;

extern void Gui_cleanup(void);
extern int Gui_init(void);

static void record_event(TestEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count] = event;
    event_count++;
}

static void reset_capture(void)
{
    memset(draws, 0, sizeof(draws));
    memset(strokes, 0, sizeof(strokes));
    memset(events, 0, sizeof(events));
    draw_count = 0;
    stroke_count = 0;
    event_count = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    stroke_attempts = 0;
    flush_attempts = 0;
    texture_lookups = 0;
    last_texture_index = -1;
    legacy_calls = 0;
    current_blend = RENDERER_BLEND_OPAQUE;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    stroke_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    allocation_attempts = 0;
    fail_allocation_attempt = 0;
}

static void reset_scene(void)
{
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    memset(&other_renderer, 0, sizeof(other_renderer));
    memset(&fake_texture, 0, sizeof(fake_texture));
    memset(&fake_sdl_renderer, 0, sizeof(fake_sdl_renderer));
    memset(&fake_map_texture, 0, sizeof(fake_map_texture));
    memset(&fake_setup, 0, sizeof(fake_setup));
    memset(scene_polygons, 0, sizeof(scene_polygons));
    memset(scene_polygon_styles, 0, sizeof(scene_polygon_styles));
    memset(scene_edge_styles, 0, sizeof(scene_edge_styles));
    memset(first_points, 0, sizeof(first_points));
    memset(second_points, 0, sizeof(second_points));
    memset(invalid_points, 0, sizeof(invalid_points));
    memset(first_edge_styles, 0, sizeof(first_edge_styles));
    memset(&clData, 0, sizeof(clData));
    memset(&instruments, 0, sizeof(instruments));

    fake_renderer.frame_active = 1;
    fake_texture.owner = &fake_renderer;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    fake_map_texture.texture = &fake_texture;
    fake_map_texture.renderer = &fake_renderer;
    fake_map_texture.state = IMG_STATE_READY;
    fake_map_texture.width = 3;
    fake_map_texture.height = 3;
    fake_map_texture.frame_width = 3;
    fake_map_texture.num_frames = 1;
    fake_map_texture.filter = RENDERER_TEXTURE_FILTER_NEAREST;
    texture_available = 1;

    fake_setup.width = 500;
    fake_setup.height = 400;
    clData.scale = 1.0;
    clData.fscale = 1.0f;
    world.x = 100;
    world.y = 200;
    realWorld = world;
    ext_view_width = 800;
    ext_view_height = 600;
    active_view_width = 800;
    active_view_height = 600;
    draw_width = 800;
    draw_height = 600;
    instruments.filledWorld = true;
    instruments.outlineWorld = true;
    instruments.texturedWalls = true;

    /* The first point is the world anchor. Remaining points are deltas;
     * the final delta closes the contour at (0, 0). */
    first_points[0] = (ipos_t){1000, 2000};
    first_points[1] = (ipos_t){6, 0};
    first_points[2] = (ipos_t){0, 4};
    first_points[3] = (ipos_t){-2, 0};
    first_points[4] = (ipos_t){-8, 0};
    first_points[5] = (ipos_t){0, -4};
    first_points[6] = (ipos_t){4, 0};
    first_edge_styles[0] = 1;
    first_edge_styles[1] = 2;
    first_edge_styles[2] = 2;
    first_edge_styles[3] = 0;
    first_edge_styles[4] = 1;
    first_edge_styles[5] = 0;
    first_edge_styles[6] = 0;
    scene_polygons[0].points = first_points;
    scene_polygons[0].num_points = 7;
    scene_polygons[0].bounds = (irec_t){996, 2000, 10, 4};
    scene_polygons[0].edge_styles = first_edge_styles;
    scene_polygons[0].style = 0;

    /* This triangle deliberately has no repeated terminal point. Its third
     * edge is the implicit last-to-first closing edge. */
    second_points[0] = (ipos_t){30, 40};
    second_points[1] = (ipos_t){5, 0};
    second_points[2] = (ipos_t){0, 3};
    scene_polygons[1].points = second_points;
    scene_polygons[1].num_points = 3;
    scene_polygons[1].bounds = (irec_t){30, 40, 5, 3};
    scene_polygons[1].style = 1;

    scene_polygon_styles[0].rgb = 0x112233;
    scene_polygon_styles[0].texture = 0;
    scene_polygon_styles[0].def_edge_style = 1;
    scene_polygon_styles[0].flags = STYLE_TEXTURED | STYLE_FILLED;
    scene_polygon_styles[1].rgb = 0x445566;
    scene_polygon_styles[1].texture = 2;
    scene_polygon_styles[1].def_edge_style = 2;
    scene_polygon_styles[1].flags = STYLE_FILLED;
    scene_polygon_styles[2].rgb = 0xa1b2c3;
    scene_polygon_styles[2].texture = 255;
    scene_polygon_styles[2].def_edge_style = 3;
    scene_polygon_styles[2].flags = STYLE_TEXTURED | STYLE_FILLED;
    scene_polygon_styles[3].rgb = 0xd4e5f6;
    scene_polygon_styles[3].texture = 7;
    scene_polygon_styles[3].def_edge_style = 3;
    scene_polygon_styles[3].flags = STYLE_INVISIBLE
        | STYLE_TEXTURED | STYLE_FILLED;

    scene_edge_styles[0].width = -1;
    scene_edge_styles[0].rgb = 0x000000;
    scene_edge_styles[1].width = 2;
    scene_edge_styles[1].rgb = 0x334455;
    scene_edge_styles[2].width = -1;
    scene_edge_styles[2].rgb = 0x667788;
    scene_edge_styles[3].width = 5;
    scene_edge_styles[3].rgb = 0x99aabb;

    polygons = scene_polygons;
    num_polygons = 2;
    max_polygons = 3;
    polygon_styles = scene_polygon_styles;
    num_polygon_styles = 4;
    max_polygon_styles = 4;
    edge_styles = scene_edge_styles;
    num_edge_styles = 4;
    max_edge_styles = 4;
    reset_capture();
}

static int point_equal(RendererPoint2D point, float x, float y)
{
    return point.x == x && point.y == y;
}

static int color_equal(RendererColor color,
                       uint8_t red, uint8_t green,
                       uint8_t blue, uint8_t alpha)
{
    return color.red == red && color.green == green
        && color.blue == blue && color.alpha == alpha;
}

static int vertices_have_color(const CapturedDraw *draw,
                               uint8_t red, uint8_t green, uint8_t blue)
{
    size_t index;

    for (index = 0; index < draw->vertex_count; index++) {
        if (!color_equal(draw->vertices[index].color,
                         red, green, blue, 255)) {
            return 0;
        }
    }
    return 1;
}

static double triangle_twice_area(const RendererVertex2D *first,
                                  const RendererVertex2D *second,
                                  const RendererVertex2D *third)
{
    return ((double)second->x - first->x)
               * ((double)third->y - first->y)
        - ((double)second->y - first->y)
               * ((double)third->x - first->x);
}

static double captured_area(const CapturedDraw *draw)
{
    double area = 0.0;
    size_t index;

    for (index = 0; index < draw->vertex_count; index += 3) {
        double twice_area = triangle_twice_area(
            &draw->vertices[index], &draw->vertices[index + 1],
            &draw->vertices[index + 2]);

        if (twice_area < 0.0)
            twice_area = -twice_area;
        area += twice_area * 0.5;
    }
    return area;
}

static int check_stroke_point(const CapturedStroke *stroke, size_t index,
                              float x, float y)
{
    return index < stroke->point_count
        && point_equal(stroke->points[index], x, y);
}

static int check_textured_fill_repeat_uv_and_special_edge_mask(void)
{
    size_t index;
    int saw_u_repeat = 0;

    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(legacy_calls == 0);

    Gui_paint_polygon(0, 1, -1);

    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].vertex_count > 0);
    TEST_CHECK(draws[0].vertex_count % 3 == 0);
    TEST_CHECK(draws[0].texture == &fake_texture);
    TEST_CHECK(draws[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(vertices_have_color(&draws[0], 255, 255, 255));
    TEST_CHECK(captured_area(&draws[0]) == 40.0);
    TEST_CHECK(texture_lookups == 1);
    TEST_CHECK(last_texture_index == 0);
    for (index = 0; index < draws[0].vertex_count; index++) {
        const RendererVertex2D *vertex = &draws[0].vertices[index];
        double twice_area;

        TEST_CHECK(vertex->x >= 1496.0f && vertex->x <= 1506.0f);
        TEST_CHECK(vertex->y >= 1600.0f && vertex->y <= 1604.0f);
        TEST_CHECK(vertex->u == (vertex->x - 1496.0f) / 3.0f);
        TEST_CHECK(vertex->v == (1604.0f - vertex->y) / 3.0f);
        if (vertex->u > 1.0f)
            saw_u_repeat = 1;
        if (index % 3 == 2) {
            twice_area = triangle_twice_area(
                &draws[0].vertices[index - 2],
                &draws[0].vertices[index - 1],
                &draws[0].vertices[index]);
            TEST_CHECK(twice_area != 0.0);
        }
    }
    TEST_CHECK(saw_u_repeat);
    TEST_CHECK(stroke_count == 4);
    TEST_CHECK(point_equal(strokes[0].points[0], 1500.0f, 1600.0f));
    TEST_CHECK(point_equal(strokes[0].points[1], 1506.0f, 1600.0f));
    TEST_CHECK(strokes[0].width == 2.0f);
    TEST_CHECK(color_equal(strokes[0].color, 0x33, 0x44, 0x55, 255));
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(point_equal(strokes[1].points[0], 1506.0f, 1600.0f));
    TEST_CHECK(point_equal(strokes[1].points[1], 1506.0f, 1604.0f));
    TEST_CHECK(strokes[1].width == 2.0f);
    TEST_CHECK(color_equal(strokes[1].color, 0x33, 0x44, 0x55, 255));
    TEST_CHECK(strokes[1].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(point_equal(strokes[2].points[0], 1506.0f, 1604.0f));
    TEST_CHECK(point_equal(strokes[2].points[1], 1504.0f, 1604.0f));
    TEST_CHECK(strokes[2].width == 2.0f);
    TEST_CHECK(color_equal(strokes[2].color, 0x33, 0x44, 0x55, 255));
    TEST_CHECK(strokes[2].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(point_equal(strokes[3].points[0], 1496.0f, 1604.0f));
    TEST_CHECK(point_equal(strokes[3].points[1], 1496.0f, 1600.0f));
    TEST_CHECK(strokes[3].width == 2.0f);
    TEST_CHECK(color_equal(strokes[3].color, 0x33, 0x44, 0x55, 255));
    TEST_CHECK(strokes[3].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(!strokes[0].closed && !strokes[1].closed
               && !strokes[2].closed && !strokes[3].closed);
    TEST_CHECK(event_count == 7);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_STROKE);
    TEST_CHECK(events[3] == TEST_EVENT_STROKE);
    TEST_CHECK(events[4] == TEST_EVENT_STROKE);
    TEST_CHECK(events[5] == TEST_EVENT_STROKE);
    TEST_CHECK(events[6] == TEST_EVENT_FLUSH);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(legacy_calls == 0);

    Sdlgui_test_discard_polygon_cache();
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_dynamic_style_uses_cached_geometry(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    scene_polygons[0].style = 2;
    Gui_paint_polygon(0, 0, 0);

    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].vertex_count > 0);
    TEST_CHECK(draws[0].texture == &fake_texture);
    TEST_CHECK(draws[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(texture_lookups == 1);
    TEST_CHECK(last_texture_index == 255);
    TEST_CHECK(vertices_have_color(&draws[0], 255, 255, 255));
    TEST_CHECK(stroke_count == 4);
    TEST_CHECK(strokes[0].width == 5.0f);
    TEST_CHECK(strokes[1].width == 5.0f);
    TEST_CHECK(strokes[2].width == 5.0f);
    TEST_CHECK(strokes[3].width == 5.0f);
    TEST_CHECK(color_equal(strokes[0].color, 0x99, 0xaa, 0xbb, 255));
    TEST_CHECK(color_equal(strokes[1].color, 0x99, 0xaa, 0xbb, 255));
    TEST_CHECK(color_equal(strokes[2].color, 0x99, 0xaa, 0xbb, 255));
    TEST_CHECK(color_equal(strokes[3].color, 0x99, 0xaa, 0xbb, 255));

    reset_capture();
    instruments.texturedWalls = false;
    scene_polygons[0].style = 2;
    Gui_paint_polygon(0, 0, 0);

    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(vertices_have_color(&draws[0], 0xa1, 0xb2, 0xc3));
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(stroke_count == 4);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_hidden_outline_and_invisible_style(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    instruments.texturedWalls = false;
    scene_polygons[1].style = 1;
    Gui_paint_polygon(1, 0, 0);

    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(vertices_have_color(&draws[0], 0x44, 0x55, 0x66));
    TEST_CHECK(stroke_count == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);

    reset_capture();
    scene_polygons[0].style = 3;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_unfilled_hidden_default_gets_fallback_outline(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    instruments.filledWorld = false;
    instruments.texturedWalls = false;
    scene_polygons[1].style = 1;

    Gui_paint_polygon(1, 0, 0);

    TEST_CHECK(draw_count == 0);
    TEST_CHECK(stroke_count == 1);
    TEST_CHECK(strokes[0].point_count == 3);
    TEST_CHECK(check_stroke_point(&strokes[0], 0, 30.0f, 40.0f));
    TEST_CHECK(check_stroke_point(&strokes[0], 1, 35.0f, 40.0f));
    TEST_CHECK(check_stroke_point(&strokes[0], 2, 35.0f, 43.0f));
    TEST_CHECK(strokes[0].closed);
    TEST_CHECK(strokes[0].width == 1.0f);
    TEST_CHECK(color_equal(strokes[0].color, 0x66, 0x77, 0x88, 255));
    TEST_CHECK(strokes[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_STROKE);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_implicit_closing_edge_and_scaled_widths(void)
{
    reset_scene();
    scene_polygons[1].edge_styles = first_edge_styles;
    first_edge_styles[0] = 0;
    first_edge_styles[1] = 0;
    /* A special-edge selector is only a visibility mask. The final selector
     * controls the implicit last-to-first edge, while the current polygon
     * default supplies its color and width. */
    first_edge_styles[2] = 1;
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    /* Geometry and the special-edge visibility mask are owned snapshots.
     * Style selectors and style-table contents remain dynamic. */
    second_points[1] = (ipos_t){50, 0};
    first_edge_styles[0] = 1;
    first_edge_styles[2] = 0;
    instruments.filledWorld = false;
    instruments.texturedWalls = false;
    scene_polygon_styles[1].def_edge_style = 1;
    clData.scale = 1.5;
    clData.fscale = 1.5f;
    Gui_paint_polygon(1, 0, 0);

    TEST_CHECK(stroke_count == 1);
    TEST_CHECK(strokes[0].point_count == 2);
    TEST_CHECK(check_stroke_point(&strokes[0], 0, 35.0f, 43.0f));
    TEST_CHECK(check_stroke_point(&strokes[0], 1, 30.0f, 40.0f));
    TEST_CHECK(!strokes[0].closed);
    TEST_CHECK(strokes[0].width == 3.0f);
    TEST_CHECK(color_equal(strokes[0].color, 0x33, 0x44, 0x55, 255));

    reset_capture();
    scene_edge_styles[1].width = 0;
    Gui_paint_polygon(1, 0, 0);
    TEST_CHECK(stroke_count == 1);
    TEST_CHECK(strokes[0].width == 1.0f);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_scene_cache_is_all_or_nothing(void)
{
    reset_scene();
    invalid_points[0] = (ipos_t){50, 60};
    invalid_points[1] = (ipos_t){1, 0};
    invalid_points[2] = (ipos_t){0, 1};
    scene_polygons[2].points = invalid_points;
    scene_polygons[2].num_points = 2;
    scene_polygons[2].style = 1;
    num_polygons = 3;

    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(legacy_calls == 0);

    num_polygons = 2;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(draw_count == 1);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_actual_gui_init_uses_atomic_cache_builder(void)
{
    int init_result;

    reset_scene();
    init_result = Gui_init();
    if (init_result == 0)
        Gui_paint_polygon(0, 0, 0);
    Gui_cleanup();
    TEST_CHECK(init_result == 0);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(legacy_calls == 0);

    reset_scene();
    fail_allocation_attempt = 1;
    init_result = Gui_init();
    Gui_cleanup();
    TEST_CHECK(init_result == -1);
    TEST_CHECK(allocation_attempts == 1);
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_cache_allocation_failures_are_atomic_and_retryable(void)
{
    size_t ordinal;
    size_t successful_attempt_count;

    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    successful_attempt_count = allocation_attempts;
    TEST_CHECK(successful_attempt_count > 2);
    Sdlgui_test_discard_polygon_cache();

    for (ordinal = 1; ordinal <= successful_attempt_count; ordinal++) {
        RendererStatus status;

        reset_scene();
        fail_allocation_attempt = ordinal;
        status = Sdlgui_test_prepare_polygon_cache();

        TEST_CHECK(status == RENDERER_STATUS_OUT_OF_MEMORY);
        TEST_CHECK(allocation_attempts == ordinal);
        Gui_paint_polygon(0, 0, 0);
        TEST_CHECK(fake_sdl_renderer.frame_result
                   == RENDERER_STATUS_INVALID_STATE);
        TEST_CHECK(texture_lookups == 0);
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(draw_attempts == 0);
        TEST_CHECK(stroke_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
        TEST_CHECK(legacy_calls == 0);

        fail_allocation_attempt = 0;
        allocation_attempts = 0;
        fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
        TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
                   == RENDERER_STATUS_OK);
        Gui_paint_polygon(0, 0, 0);
        TEST_CHECK(draw_count == 1);
        Sdlgui_test_discard_polygon_cache();
    }

    return 0;
}

static int check_float_coordinate_collapse_is_sticky(void)
{
    reset_scene();
    second_points[0] = (ipos_t){16777216, 40};
    second_points[1] = (ipos_t){1, 0};
    second_points[2] = (ipos_t){0, 3};
    scene_polygons[1].bounds = (irec_t){16777216, 40, 1, 3};
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_INVALID_ARGUMENT);

    Gui_paint_polygon(1, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    Gui_paint_polygon(1, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    return 0;
}

static int check_dynamic_offset_float_collapse_is_sticky(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    Gui_paint_polygon(0, INT_MAX, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_invalid_dynamic_selectors_are_cpu_failures(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    scene_polygons[0].style = num_polygon_styles;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    scene_polygons[0].style = 0;
    scene_polygon_styles[0].texture = 256;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    scene_polygon_styles[0].texture = -1;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    scene_polygon_styles[0].texture = 0;
    scene_polygon_styles[0].def_edge_style = num_edge_styles;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    reset_capture();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    scene_polygon_styles[0].def_edge_style = 1;
    edge_styles = NULL;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);

    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_texture_state_failures_are_sticky_and_atomic(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    texture_available = 0;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(vertices_have_color(&draws[0], 0x11, 0x22, 0x33));
    TEST_CHECK(texture_lookups == 1);
    TEST_CHECK(last_texture_index == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(legacy_calls == 0);

    reset_capture();
    texture_available = 1;
    fake_map_texture.texture = NULL;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 1);

    fake_map_texture.texture = &fake_texture;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 1);

    reset_capture();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    fake_map_texture.texture = &fake_texture;
    fake_map_texture.renderer = &other_renderer;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 1);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_renderer_failures_are_sticky_and_ordered(void)
{
    reset_scene();
    scene_polygons[0].edge_styles = NULL;
    scene_polygon_styles[0].def_edge_style = 0;
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    blend_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    blend_result = RENDERER_STATUS_OK;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_capture();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    Sdlgui_test_discard_polygon_cache();
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    stroke_result = RENDERER_STATUS_BACKEND_ERROR;
    flush_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_STROKE);
    TEST_CHECK(events[3] == TEST_EVENT_FLUSH);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(stroke_count == 0);
    TEST_CHECK(flush_attempts == 1);

    Sdlgui_test_discard_polygon_cache();
    reset_scene();
    scene_polygons[0].edge_styles = NULL;
    scene_polygon_styles[0].def_edge_style = 0;
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);
    reset_capture();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    Gui_paint_polygon(0, 0, 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    Sdlgui_test_discard_polygon_cache();
    return 0;
}

static int check_actual_cleanup_discards_the_scene_cache(void)
{
    reset_scene();
    TEST_CHECK(Sdlgui_test_prepare_polygon_cache()
               == RENDERER_STATUS_OK);

    Gui_cleanup();
    Gui_cleanup();
    Gui_paint_polygon(0, 0, 0);

    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(stroke_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(texture_lookups == 0);
    TEST_CHECK(legacy_calls == 0);
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

RendererStatus Renderer_draw_triangles(
    Renderer *renderer, RendererTexture *texture,
    const RendererVertex2D *vertices, size_t vertex_count)
{
    CapturedDraw *capture;

    draw_attempts++;
    record_event(TEST_EVENT_DRAW);
    if (renderer != &fake_renderer || !renderer->frame_active
        || vertices == NULL || vertex_count == 0
        || vertex_count % 3 != 0
        || vertex_count > MAX_CAPTURED_VERTICES
        || draw_count >= MAX_CAPTURED_DRAWS) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (texture != NULL && texture->owner != renderer)
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    capture = &draws[draw_count++];
    memcpy(capture->vertices, vertices,
           vertex_count * sizeof(capture->vertices[0]));
    capture->vertex_count = vertex_count;
    capture->texture = texture;
    capture->blend = current_blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_stroke_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, float width, RendererColor color, int closed)
{
    CapturedStroke *capture;

    stroke_attempts++;
    record_event(TEST_EVENT_STROKE);
    if (renderer != &fake_renderer || !renderer->frame_active
        || points == NULL || point_count < 2
        || point_count > MAX_CAPTURED_POINTS
        || stroke_count >= MAX_CAPTURED_STROKES) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (stroke_result != RENDERER_STATUS_OK)
        return stroke_result;
    capture = &strokes[stroke_count++];
    memcpy(capture->points, points,
           point_count * sizeof(capture->points[0]));
    capture->point_count = point_count;
    capture->width = width;
    capture->color = color;
    capture->closed = closed;
    capture->blend = current_blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(
    SdlRenderer *renderer)
{
    flush_attempts++;
    record_event(TEST_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer)
        return RENDERER_STATUS_INVALID_STATE;
    return flush_result;
}

image_t *Image_get_texture(int index)
{
    texture_lookups++;
    last_texture_index = index;
    return texture_available ? &fake_map_texture : NULL;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

#define LEGACY_STUB(name, signature, body) \
    void GLAPIENTRY name signature          \
    {                                      \
        body;                              \
        legacy_calls++;                    \
    }

LEGACY_STUB(glMatrixMode, (GLenum mode), (void)mode)
LEGACY_STUB(glPushMatrix, (void), (void)0)
LEGACY_STUB(glPopMatrix, (void), (void)0)
LEGACY_STUB(glLoadIdentity, (void), (void)0)
LEGACY_STUB(glTranslatef, (GLfloat x, GLfloat y, GLfloat z),
            ((void)x, (void)y, (void)z))
LEGACY_STUB(glScalef, (GLfloat x, GLfloat y, GLfloat z),
            ((void)x, (void)y, (void)z))
LEGACY_STUB(glColor4ub,
            (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha),
            ((void)red, (void)green, (void)blue, (void)alpha))
LEGACY_STUB(glLineWidth, (GLfloat width), (void)width)
LEGACY_STUB(glEnable, (GLenum capability), (void)capability)
LEGACY_STUB(glDisable, (GLenum capability), (void)capability)
LEGACY_STUB(glBlendFunc, (GLenum source, GLenum destination),
            ((void)source, (void)destination))
LEGACY_STUB(glNewList, (GLuint list, GLenum mode),
            ((void)list, (void)mode))
LEGACY_STUB(glEndList, (void), (void)0)
LEGACY_STUB(glBegin, (GLenum mode), (void)mode)
LEGACY_STUB(glEnd, (void), (void)0)
LEGACY_STUB(glVertex2i, (GLint x, GLint y), ((void)x, (void)y))
LEGACY_STUB(glTexCoord2f, (GLfloat u, GLfloat v), ((void)u, (void)v))
LEGACY_STUB(glCallList, (GLuint list), (void)list)
LEGACY_STUB(glDeleteLists, (GLuint list, GLsizei range),
            ((void)list, (void)range))

#undef LEGACY_STUB

GLuint GLAPIENTRY glGenLists(GLsizei range)
{
    legacy_calls++;
    return range > 0 ? 100U : 0U;
}

int main(void)
{
    int failed = 0;

    failed |= check_textured_fill_repeat_uv_and_special_edge_mask();
    failed |= check_dynamic_style_uses_cached_geometry();
    failed |= check_hidden_outline_and_invisible_style();
    failed |= check_unfilled_hidden_default_gets_fallback_outline();
    failed |= check_implicit_closing_edge_and_scaled_widths();
    failed |= check_scene_cache_is_all_or_nothing();
    failed |= check_actual_gui_init_uses_atomic_cache_builder();
    failed |= check_cache_allocation_failures_are_atomic_and_retryable();
    failed |= check_float_coordinate_collapse_is_sticky();
    failed |= check_dynamic_offset_float_collapse_is_sticky();
    failed |= check_invalid_dynamic_selectors_are_cpu_failures();
    failed |= check_texture_state_failures_are_sticky_and_atomic();
    failed |= check_renderer_failures_are_sticky_and_ordered();
    failed |= check_actual_cleanup_discards_the_scene_cache();
    return failed != 0;
}
