#include "test_helpers.h"

#include "xpclient_sdl.h"

#include "asteroid_batch_test_support.h"
#include "images.h"
#include "paint.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <GL/gl.h>

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ASTEROID_SOURCE_VERTEX_COUNT 408
#define MAX_CAPTURED_VERTICES 1024
#define MAX_EVENTS 16
#define FULL_TURN_RADIANS 6.283185307179586476925286766559

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
    TEST_EVENT_FLUSH
} TestEvent;

typedef struct CapturedTriangleDraw {
    RendererVertex2D vertices[MAX_CAPTURED_VERTICES];
    size_t vertex_count;
    RendererTexture *texture;
    RendererBlendMode blend;
} CapturedTriangleDraw;

typedef struct ReferenceVector3 {
    double x;
    double y;
    double z;
} ReferenceVector3;

static Renderer fake_renderer;
static Renderer other_renderer;
static RendererTexture fake_texture;
static RendererTexture other_texture;
static SdlRenderer fake_sdl_renderer;
static image_t fake_asteroid_image;
static CapturedTriangleDraw captured_draw;
static TestEvent events[MAX_EVENTS];
static int event_count;
static int image_available;
static int image_lookup_attempts;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int flush_attempts;
static int legacy_calls;
static int realloc_attempts;
static int failed_reallocs;
static int fail_initial_realloc;
static int fail_growth_realloc;
static void *tracked_batch_allocation;
static int tracked_batch_frees;
static void *tracked_input_allocation;
static int tracked_input_frees;
static RendererBlendMode current_blend;
static RendererStatus blend_result;
static RendererStatus draw_result;
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
double tbl_sin[TABLE_SIZE];
double tbl_cos[TABLE_SIZE];

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

extern float vertex_vectors[ASTEROID_SOURCE_VERTEX_COUNT][3];
extern float normal_vectors[ASTEROID_SOURCE_VERTEX_COUNT][3];
extern float uv_vectors[ASTEROID_SOURCE_VERTEX_COUNT][2];
extern void Gui_cleanup(void);

void *__real_realloc(void *pointer, size_t size);
void __real_free(void *pointer);

void *__wrap_realloc(void *pointer, size_t size)
{
    void *replacement;

    realloc_attempts++;
    if ((pointer == NULL && fail_initial_realloc)
        || (pointer != NULL && fail_growth_realloc)) {
        failed_reallocs++;
        return NULL;
    }
    replacement = __real_realloc(pointer, size);
    if (replacement != NULL)
        tracked_batch_allocation = replacement;
    return replacement;
}

void __wrap_free(void *pointer)
{
    if (pointer != NULL && pointer == tracked_batch_allocation) {
        tracked_batch_allocation = NULL;
        tracked_batch_frees++;
    }
    if (pointer != NULL && pointer == tracked_input_allocation) {
        tracked_input_allocation = NULL;
        tracked_input_frees++;
    }
    __real_free(pointer);
}

static void record_event(TestEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count] = event;
    event_count++;
}

static void reset_trigonometry(void)
{
    int index;

    for (index = 0; index < TABLE_SIZE; index++) {
        double angle = FULL_TURN_RADIANS * (double)index
            / (double)TABLE_SIZE;

        tbl_sin[index] = sin(angle);
        tbl_cos[index] = cos(angle);
    }
}

static void reset_state(void)
{
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    memset(&other_renderer, 0, sizeof(other_renderer));
    memset(&fake_texture, 0, sizeof(fake_texture));
    memset(&other_texture, 0, sizeof(other_texture));
    memset(&fake_sdl_renderer, 0, sizeof(fake_sdl_renderer));
    memset(&fake_asteroid_image, 0, sizeof(fake_asteroid_image));
    memset(&captured_draw, 0, sizeof(captured_draw));
    memset(events, 0, sizeof(events));
    memset(&clData, 0, sizeof(clData));
    memset(debris_ptr, 0, sizeof(debris_ptr));
    memset(num_debris, 0, sizeof(num_debris));
    memset(max_debris, 0, sizeof(max_debris));
    memset(fastshot_ptr, 0, sizeof(fastshot_ptr));
    memset(num_fastshot, 0, sizeof(num_fastshot));
    memset(max_fastshot, 0, sizeof(max_fastshot));

    fake_renderer.frame_active = 1;
    other_renderer.frame_active = 1;
    fake_texture.owner = &fake_renderer;
    other_texture.owner = &other_renderer;
    fake_sdl_renderer.frontend = &fake_renderer;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    fake_asteroid_image.texture = &fake_texture;
    fake_asteroid_image.renderer = &fake_renderer;
    fake_asteroid_image.state = IMG_STATE_READY;
    image_available = 1;
    event_count = 0;
    image_lookup_attempts = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    flush_attempts = 0;
    legacy_calls = 0;
    realloc_attempts = 0;
    failed_reallocs = 0;
    fail_initial_realloc = 0;
    fail_growth_realloc = 0;
    tracked_batch_frees = 0;
    tracked_input_frees = 0;
    current_blend = RENDERER_BLEND_ALPHA;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;

    reset_trigonometry();

    world.x = 1000;
    world.y = -500;
    realWorld.x = 800;
    realWorld.y = -800;
    ext_view_width = 256;
    ext_view_height = 300;
    active_view_width = 256;
    active_view_height = 256;
    ext_view_x_offset = 0;
    ext_view_y_offset = 0;
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

static int set_actual_asteroid_fixture(void)
{
    asteroid_t entries[3];

    memset(entries, 0, sizeof(entries));
    entries[0].x = 1050;
    entries[0].y = -400;
    entries[0].type = 0;
    entries[0].size = 1;
    entries[0].rotation = 0;
    entries[1].x = 856;
    entries[1].y = -600;
    entries[1].type = 5;
    entries[1].size = 2;
    entries[1].rotation = 160;
    entries[2].x = 700;
    entries[2].y = -900;
    entries[2].type = 7;
    entries[2].size = 255;
    entries[2].rotation = 255;

    asteroid_ptr = malloc(sizeof(entries));
    if (asteroid_ptr == NULL)
        return 1;
    memcpy(asteroid_ptr, entries, sizeof(entries));
    tracked_input_allocation = asteroid_ptr;
    num_asteroids = 3;
    max_asteroids = 3;
    return 0;
}

static ReferenceVector3 rotate_reference(
    ReferenceVector3 vector, ReferenceVector3 axis,
    double cosine, double sine)
{
    ReferenceVector3 cross;
    ReferenceVector3 result;
    double dot;
    double one_minus_cosine = 1.0 - cosine;

    cross.x = axis.y * vector.z - axis.z * vector.y;
    cross.y = axis.z * vector.x - axis.x * vector.z;
    cross.z = axis.x * vector.y - axis.y * vector.x;
    dot = axis.x * vector.x + axis.y * vector.y + axis.z * vector.z;
    result.x = vector.x * cosine + cross.x * sine
        + axis.x * dot * one_minus_cosine;
    result.y = vector.y * cosine + cross.y * sine
        + axis.y * dot * one_minus_cosine;
    result.z = vector.z * cosine + cross.z * sine
        + axis.z * dot * one_minus_cosine;
    return result;
}

static uint8_t quantize_reference_intensity(double normal_z)
{
    double intensity = 0.18 + 0.8 * fmax(normal_z, 0.0);

    if (intensity < 0.0)
        intensity = 0.0;
    if (intensity > 1.0)
        intensity = 1.0;
    return (uint8_t)floor(intensity * 255.0 + 0.5);
}

static size_t build_reference_asteroid(
    int x, int y, int type, int rotation, int size,
    RendererVertex2D *output, size_t output_capacity)
{
    ReferenceVector3 axis;
    double axis_length = sqrt(21.0);
    double cosine = cos(FULL_TURN_RADIANS
                        * (double)(rotation % TABLE_SIZE)
                        / (double)TABLE_SIZE);
    double sine = sin(FULL_TURN_RADIANS
                      * (double)(rotation % TABLE_SIZE)
                      / (double)TABLE_SIZE);
    double scale = 0.9 * (double)SHIP_SZ * (double)size;
    size_t output_count = 0;
    int triangle_start;

    axis.x = ((type & 1) != 0 ? 1.0 : -1.0) / axis_length;
    axis.y = ((type & 2) != 0 ? 2.0 : -2.0) / axis_length;
    axis.z = ((type & 4) != 0 ? 4.0 : -4.0) / axis_length;
    for (triangle_start = 0;
         triangle_start < ASTEROID_SOURCE_VERTEX_COUNT;
         triangle_start += 3) {
        ReferenceVector3 positions[3];
        RendererVertex2D triangle[3];
        double area;
        int vertex_offset;

        for (vertex_offset = 0; vertex_offset < 3; vertex_offset++) {
            int source_index = triangle_start + vertex_offset;
            ReferenceVector3 source = {
                vertex_vectors[source_index][0],
                vertex_vectors[source_index][1],
                vertex_vectors[source_index][2]
            };

            positions[vertex_offset] = rotate_reference(
                source, axis, cosine, sine);
            ReferenceVector3 source_normal = {
                normal_vectors[source_index][0],
                normal_vectors[source_index][1],
                normal_vectors[source_index][2]
            };
            ReferenceVector3 normal = rotate_reference(
                source_normal, axis, cosine, sine);
            RendererVertex2D *vertex = &triangle[vertex_offset];
            uint8_t intensity = quantize_reference_intensity(normal.z);

            vertex->x = (float)((double)x
                                + positions[vertex_offset].x * scale);
            vertex->y = (float)((double)y
                                + positions[vertex_offset].y * scale);
            vertex->u = uv_vectors[source_index][0];
            vertex->v = 1.0f - uv_vectors[source_index][1];
            vertex->color.red = intensity;
            vertex->color.green = intensity;
            vertex->color.blue = intensity;
            vertex->color.alpha = 255;
        }
        area = ((double)triangle[1].x - triangle[0].x)
                   * ((double)triangle[2].y - triangle[0].y)
            - ((double)triangle[1].y - triangle[0].y)
                   * ((double)triangle[2].x - triangle[0].x);
        if (!(area > 0.0))
            continue;
        if (output_count + 3 > output_capacity)
            return SIZE_MAX;
        memcpy(&output[output_count], triangle, sizeof(triangle));
        output_count += 3;
    }
    return output_count;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int float_near(float actual, float expected)
{
    return fabsf(actual - expected) <= 0.00025f;
}

static int vertex_near(
    const RendererVertex2D *actual, const RendererVertex2D *expected)
{
    return float_near(actual->x, expected->x)
        && float_near(actual->y, expected->y)
        && float_near(actual->u, expected->u)
        && float_near(actual->v, expected->v)
        && color_equal(actual->color, expected->color);
}

static int vertex_exact(
    const RendererVertex2D *actual,
    float x, float y, float u, float v, uint8_t intensity)
{
    RendererColor expected_color = {
        intensity, intensity, intensity, 255
    };

    return actual->x == x && actual->y == y
        && actual->u == u && actual->v == v
        && color_equal(actual->color, expected_color);
}

static int all_triangles_are_front_facing(void)
{
    size_t index;

    if (captured_draw.vertex_count % 3 != 0)
        return 0;
    for (index = 0; index < captured_draw.vertex_count; index += 3) {
        const RendererVertex2D *first = &captured_draw.vertices[index];
        const RendererVertex2D *second = &captured_draw.vertices[index + 1];
        const RendererVertex2D *third = &captured_draw.vertices[index + 2];
        double area = ((double)second->x - first->x)
                          * ((double)third->y - first->y)
            - ((double)second->y - first->y)
                          * ((double)third->x - first->x);

        if (!(area > 0.0))
            return 0;
    }
    return 1;
}

static int no_renderer_commands(RendererStatus expected_status)
{
    TEST_CHECK(fake_sdl_renderer.frame_result == expected_status);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int batch_allocation_is_released(int expected_free_count)
{
    TEST_CHECK(tracked_batch_allocation == NULL);
    TEST_CHECK(tracked_batch_frees == expected_free_count);
    return 0;
}

static int input_allocation_is_released(void)
{
    TEST_CHECK(tracked_input_allocation == NULL);
    TEST_CHECK(tracked_input_frees == 1);
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
    draw_attempts++;
    record_event(TEST_EVENT_DRAW);
    if (renderer != &fake_renderer || !renderer->frame_active
        || vertices == NULL || vertex_count == 0
        || vertex_count % 3 != 0
        || vertex_count > MAX_CAPTURED_VERTICES) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (texture != NULL && texture->owner != renderer)
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    memcpy(captured_draw.vertices, vertices,
           vertex_count * sizeof(captured_draw.vertices[0]));
    captured_draw.vertex_count = vertex_count;
    captured_draw.texture = texture;
    captured_draw.blend = current_blend;
    successful_draws++;
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

image_t *Image_get(int index)
{
    image_lookup_attempts++;
    if (index != IMG_ASTEROID || !image_available)
        return NULL;
    return &fake_asteroid_image;
}

other_t *Other_by_id(int id)
{
    (void)id;
    return NULL;
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

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    (void)texture;
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

void GLAPIENTRY glLightfv(
    GLenum light, GLenum parameter, const GLfloat *values)
{
    (void)light;
    (void)parameter;
    (void)values;
    legacy_calls++;
}

void GLAPIENTRY glPushMatrix(void)
{
    legacy_calls++;
}

void GLAPIENTRY glPopMatrix(void)
{
    legacy_calls++;
}

void GLAPIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    (void)x;
    (void)y;
    (void)z;
    legacy_calls++;
}

void GLAPIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    (void)x;
    (void)y;
    (void)z;
    legacy_calls++;
}

void GLAPIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    (void)angle;
    (void)x;
    (void)y;
    (void)z;
    legacy_calls++;
}

void GLAPIENTRY glCallList(GLuint list)
{
    (void)list;
    legacy_calls++;
}

void GLAPIENTRY glDeleteLists(GLuint list, GLsizei range)
{
    (void)list;
    (void)range;
    legacy_calls++;
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

void __wrap_Gui_paint_sparks_begin(void)
{
}

void __wrap_Gui_paint_spark(int color, int x, int y)
{
    (void)color;
    (void)x;
    (void)y;
}

void __wrap_Gui_paint_sparks_end(void)
{
}

void __wrap_Gui_paint_wreck(
    int x, int y, bool deadly, int type, int rotation, int size)
{
    (void)x;
    (void)y;
    (void)deadly;
    (void)type;
    (void)rotation;
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

void __wrap_Gui_paint_missile(int x, int y, int length, int direction)
{
    (void)x;
    (void)y;
    (void)length;
    (void)direction;
}

void __wrap_Gui_paint_lasers_begin(void)
{
}

void __wrap_Gui_paint_lasers_end(void)
{
}

void __wrap_Gui_paint_laser(
    int color, int x, int y, int length, int direction)
{
    (void)color;
    (void)x;
    (void)y;
    (void)length;
    (void)direction;
}

static int check_actual_traversal_exact_geometry_and_release(void)
{
    RendererVertex2D expected[MAX_CAPTURED_VERTICES];
    size_t first_count;
    size_t second_count;
    size_t expected_count;
    size_t index;

    reset_state();
    TEST_CHECK(set_actual_asteroid_fixture() == 0);

    Paint_shots();

    TEST_CHECK(num_asteroids == 0);
    TEST_CHECK(max_asteroids == 0);
    asteroid_ptr = NULL;
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(captured_draw.vertex_count == 480);
    TEST_CHECK(captured_draw.texture == &fake_texture);
    TEST_CHECK(captured_draw.blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(image_lookup_attempts == 1);
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    TEST_CHECK(input_allocation_is_released() == 0);

    first_count = build_reference_asteroid(
        1050, -400, 0, 0, 1, expected, MAX_CAPTURED_VERTICES);
    TEST_CHECK(first_count == 228);
    second_count = build_reference_asteroid(
        1056, -300, 5, 160, 2, expected + first_count,
        MAX_CAPTURED_VERTICES - first_count);
    TEST_CHECK(second_count == 252);
    expected_count = first_count + second_count;
    TEST_CHECK(expected_count == captured_draw.vertex_count);
    for (index = 0; index < expected_count; index++)
        TEST_CHECK(vertex_near(&captured_draw.vertices[index],
                               &expected[index]));
    TEST_CHECK(all_triangles_are_front_facing());
    TEST_CHECK(vertex_exact(
        &captured_draw.vertices[0],
        1057.4290771484375f, -392.0078125f,
        1.5835200548171997f, 1.181725025177002f, 78));
    TEST_CHECK(vertex_exact(
        &captured_draw.vertices[227],
        1054.1080322265625f, -396.9984130859375f,
        1.4756499528884888f, 0.8394420146942139f, 243));
    TEST_CHECK(vertex_exact(
        &captured_draw.vertices[228],
        1039.8260498046875f, -288.82818603515625f,
        1.5835200548171997f, 1.181725025177002f, 152));
    TEST_CHECK(vertex_exact(
        &captured_draw.vertices[479],
        1056.0361328125f, -276.7991638183594f,
        0.5525919795036316f, 0.9224420189857483f, 149));
    return 0;
}

static int check_missing_image_draws_untextured(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};

    reset_state();
    image_available = 0;

    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_OK);
    TEST_CHECK(captured_draw.vertex_count == 228);
    TEST_CHECK(captured_draw.texture == NULL);
    TEST_CHECK(captured_draw.blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(image_lookup_attempts == 1);
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_inconsistent_image_state_is_rejected(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};

    reset_state();
    fake_asteroid_image.texture = NULL;

    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_STATE) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_texture_owner_mismatch_is_rejected(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};

    reset_state();
    fake_asteroid_image.texture = &other_texture;
    fake_asteroid_image.renderer = &other_renderer;

    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(no_renderer_commands(
                   RENDERER_STATUS_RESOURCE_MISMATCH) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_full_byte_type_and_rotation_have_low_bit_parity(void)
{
    static const SdlguiTestAsteroid base = {100, 200, 5, 32, 2};
    static const SdlguiTestAsteroid byte_wide = {100, 200, 253, 160, 2};
    static const SdlguiTestAsteroid maximum = {100, 200, 255, 255, 255};
    RendererVertex2D base_vertices[MAX_CAPTURED_VERTICES];
    RendererVertex2D maximum_vertices[MAX_CAPTURED_VERTICES];
    size_t base_count;
    size_t maximum_count;
    size_t index;

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&base, 1)
               == RENDERER_STATUS_OK);
    TEST_CHECK(captured_draw.vertex_count == 252);
    base_count = captured_draw.vertex_count;
    memcpy(base_vertices, captured_draw.vertices,
           base_count * sizeof(base_vertices[0]));
    TEST_CHECK(batch_allocation_is_released(1) == 0);

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&byte_wide, 1)
               == RENDERER_STATUS_OK);
    TEST_CHECK(captured_draw.vertex_count == base_count);
    for (index = 0; index < base_count; index++)
        TEST_CHECK(vertex_near(&captured_draw.vertices[index],
                               &base_vertices[index]));
    TEST_CHECK(batch_allocation_is_released(1) == 0);

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&maximum, 1)
               == RENDERER_STATUS_OK);
    maximum_count = build_reference_asteroid(
        maximum.x, maximum.y, maximum.type, maximum.rotation, maximum.size,
        maximum_vertices, NELEM(maximum_vertices));
    TEST_CHECK(maximum_count != SIZE_MAX);
    TEST_CHECK(maximum_count > 0);
    TEST_CHECK(captured_draw.vertex_count == maximum_count);
    for (index = 0; index < maximum_count; index++)
        TEST_CHECK(vertex_near(&captured_draw.vertices[index],
                               &maximum_vertices[index]));
    TEST_CHECK(all_triangles_are_front_facing());
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_empty_and_zero_size_batches_are_noops(void)
{
    static const SdlguiTestAsteroid zero_size = {100, 200, 255, 255, 0};

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(NULL, 0)
               == RENDERER_STATUS_OK);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OK) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);
    TEST_CHECK(image_lookup_attempts == 0);

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&zero_size, 1)
               == RENDERER_STATUS_OK);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OK) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);
    TEST_CHECK(image_lookup_attempts == 0);
    return 0;
}

static int check_all_culled_batch_is_a_noop(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};
    float saved_positions[ASTEROID_SOURCE_VERTEX_COUNT][3];
    RendererStatus status;

    memcpy(saved_positions, vertex_vectors, sizeof(saved_positions));
    memset(vertex_vectors, 0, sizeof(saved_positions));
    reset_state();

    status = Sdlgui_test_paint_asteroid_batch(&entry, 1);

    memcpy(vertex_vectors, saved_positions, sizeof(saved_positions));
    TEST_CHECK(status == RENDERER_STATUS_OK);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OK) == 0);
    TEST_CHECK(image_lookup_attempts == 0);
    return 0;
}

static int check_impossible_entry_count_is_rejected_without_access(void)
{
    const size_t impossible_count = (size_t)INT_MAX
        / ASTEROID_SOURCE_VERTEX_COUNT + 1;

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(
                   (const SdlguiTestAsteroid *)(uintptr_t)1,
                   impossible_count)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    TEST_CHECK(image_lookup_attempts == 0);
    TEST_CHECK(realloc_attempts == 0);
    return 0;
}

static int check_cleanup_discards_unfinished_batch(void)
{
    reset_state();

    Gui_paint_asteroids_begin();
    Gui_paint_asteroid(100, 200, 0, 0, 1);
    Gui_cleanup();
    Gui_paint_asteroids_end();

    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OK) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_invalid_byte_boundaries_are_cpu_atomic(void)
{
    SdlguiTestAsteroid invalid_entries[] = {
        {100, 200, -1, 0, 1},
        {100, 200, 256, 0, 1},
        {100, 200, 0, -1, 1},
        {100, 200, 0, 256, 1},
        {100, 200, 0, 0, -1},
        {100, 200, 0, 0, 256}
    };
    size_t index;

    for (index = 0; index < NELEM(invalid_entries); index++) {
        reset_state();
        TEST_CHECK(Sdlgui_test_paint_asteroid_batch(
                       &invalid_entries[index], 1)
                   == RENDERER_STATUS_INVALID_ARGUMENT);
        TEST_CHECK(no_renderer_commands(
                       RENDERER_STATUS_INVALID_ARGUMENT) == 0);
        TEST_CHECK(batch_allocation_is_released(0) == 0);
    }
    return 0;
}

static int check_later_invalid_entry_is_cpu_atomic(void)
{
    static const SdlguiTestAsteroid entries[] = {
        {100, 200, 0, 0, 1},
        {300, 400, 0, 256, 1},
        {500, 600, 7, 127, 2}
    };

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(entries, NELEM(entries))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_numeric_failures_are_cpu_atomic(void)
{
    static const SdlguiTestAsteroid collapsed = {
        INT_MAX, INT_MAX, 0, 0, 1
    };
    static const SdlguiTestAsteroid nonfinite_entries[] = {
        {100, 200, 0, 0, 1},
        {300, 400, 5, 32, 2}
    };

    reset_state();
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&collapsed, 1)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);

    reset_state();
    tbl_sin[32] = NAN;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(
                   nonfinite_entries, NELEM(nonfinite_entries))
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    return 0;
}

static int check_nonfinite_culled_source_data_is_cpu_atomic(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};
    RendererStatus status;
    float saved_value;

    reset_state();
    saved_value = vertex_vectors[0][2];
    vertex_vectors[0][2] = NAN;
    status = Sdlgui_test_paint_asteroid_batch(&entry, 1);
    vertex_vectors[0][2] = saved_value;
    TEST_CHECK(status == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);

    reset_state();
    saved_value = normal_vectors[0][0];
    normal_vectors[0][0] = INFINITY;
    status = Sdlgui_test_paint_asteroid_batch(&entry, 1);
    normal_vectors[0][0] = saved_value;
    TEST_CHECK(status == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);

    reset_state();
    saved_value = uv_vectors[0][1];
    uv_vectors[0][1] = NAN;
    status = Sdlgui_test_paint_asteroid_batch(&entry, 1);
    uv_vectors[0][1] = saved_value;
    TEST_CHECK(status == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);
    return 0;
}

static int check_allocation_failures_are_atomic_and_release_actual_input(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};

    reset_state();
    fail_initial_realloc = 1;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(failed_reallocs == 1);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    TEST_CHECK(batch_allocation_is_released(0) == 0);

    reset_state();
    TEST_CHECK(set_actual_asteroid_fixture() == 0);
    fail_growth_realloc = 1;

    Paint_shots();

    TEST_CHECK(num_asteroids == 0);
    TEST_CHECK(max_asteroids == 0);
    asteroid_ptr = NULL;
    TEST_CHECK(failed_reallocs == 1);
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);
    TEST_CHECK(input_allocation_is_released() == 0);
    return 0;
}

static int check_renderer_failures_are_sticky(void)
{
    static const SdlguiTestAsteroid entry = {100, 200, 0, 0, 1};
    int previous_event_count;
    int previous_image_lookups;
    int previous_realloc_attempts;

    reset_state();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);

    reset_state();
    draw_result = RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);

    reset_state();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == TEST_EVENT_BLEND);
    TEST_CHECK(events[1] == TEST_EVENT_DRAW);
    TEST_CHECK(events[2] == TEST_EVENT_FLUSH);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(legacy_calls == 0);
    TEST_CHECK(batch_allocation_is_released(1) == 0);

    previous_event_count = event_count;
    previous_image_lookups = image_lookup_attempts;
    previous_realloc_attempts = realloc_attempts;
    flush_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdlgui_test_paint_asteroid_batch(&entry, 1)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(event_count == previous_event_count);
    TEST_CHECK(image_lookup_attempts == previous_image_lookups);
    TEST_CHECK(realloc_attempts == previous_realloc_attempts);
    TEST_CHECK(legacy_calls == 0);
    return 0;
}

static int check_preexisting_failure_still_releases_actual_input(void)
{
    reset_state();
    fake_sdl_renderer.frame_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(set_actual_asteroid_fixture() == 0);

    Paint_shots();

    TEST_CHECK(num_asteroids == 0);
    TEST_CHECK(max_asteroids == 0);
    asteroid_ptr = NULL;
    TEST_CHECK(no_renderer_commands(RENDERER_STATUS_BACKEND_ERROR) == 0);
    TEST_CHECK(input_allocation_is_released() == 0);
    TEST_CHECK(image_lookup_attempts == 0);
    TEST_CHECK(realloc_attempts == 0);
    return 0;
}

int main(void)
{
    int failed;

    failed = check_actual_traversal_exact_geometry_and_release();
    if (failed)
        return EXIT_FAILURE;
    failed |= check_missing_image_draws_untextured();
    failed |= check_inconsistent_image_state_is_rejected();
    failed |= check_texture_owner_mismatch_is_rejected();
    failed |= check_full_byte_type_and_rotation_have_low_bit_parity();
    failed |= check_empty_and_zero_size_batches_are_noops();
    failed |= check_all_culled_batch_is_a_noop();
    failed |= check_impossible_entry_count_is_rejected_without_access();
    failed |= check_cleanup_discards_unfinished_batch();
    failed |= check_invalid_byte_boundaries_are_cpu_atomic();
    failed |= check_later_invalid_entry_is_cpu_atomic();
    failed |= check_numeric_failures_are_cpu_atomic();
    failed |= check_nonfinite_culled_source_data_is_cpu_atomic();
    failed |= check_allocation_failures_are_atomic_and_release_actual_input();
    failed |= check_renderer_failures_are_sticky();
    failed |= check_preexisting_failure_still_releases_actual_input();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
