#include "test_helpers.h"

#include "image_geometry.h"
#include "images.h"
#include "images_test_support.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FAKE_TEXTURES 128
#define MAX_DRAWS 16
#define FLOAT_TOLERANCE 0.0001f

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    unsigned int identifier;
};

struct SdlRenderer {
    Renderer *frontend;
    int frame_active;
    RendererStatus frame_result;
};

typedef struct FakeTexture {
    RendererTexture handle;
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    int destroy_count;
} FakeTexture;

typedef struct FakeDraw {
    RendererTexture *texture;
    RendererVertex2D vertices[6];
    size_t vertex_count;
} FakeDraw;

typedef struct FixtureState {
    int picture_init_calls;
    int picture_init_successes;
    int picture_cleanup_calls;
    int disabled_map_init_calls;
    int ordinary_signed_count;
    int rotatable_signed_count;
    int map_signed_count;
} FixtureState;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer,
    0,
    RENDERER_STATUS_OK
};
static FakeTexture fake_textures[MAX_FAKE_TEXTURES];
static FakeDraw fake_draws[MAX_DRAWS];
static FixtureState fixture_state;
static int fake_texture_count;
static int texture_create_attempts;
static int fail_texture_create_attempt;
static int texture_destroy_calls;
static int draw_count;
static int draw_attempts;
static int blend_attempts;
static int flush_calls;
static int tracked_result_calls;
static int pending_draws;
static RendererStatus draw_result = RENDERER_STATUS_OK;
static RendererStatus blend_result = RENDERER_STATUS_OK;
static RendererStatus flush_result = RENDERER_STATUS_OK;

instruments_t instruments;

static int float_equal(float actual, float expected)
{
    float difference = actual - expected;

    return difference >= -FLOAT_TOLERANCE
        && difference <= FLOAT_TOLERANCE;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, float u, float v,
                        RendererColor color)
{
    return float_equal(vertex->x, x) && float_equal(vertex->y, y)
        && float_equal(vertex->u, u) && float_equal(vertex->v, v)
        && color_equal(vertex->color, color);
}

static RGB_COLOR fixture_color(uint8_t base, int frame, int x, int y)
{
    uint8_t offset = (uint8_t)(frame * 27 + y * 9 + x * 3);

    if (frame == 0 && x == 1 && y == 1)
        return 0;
    return RGB24((uint8_t)(base + offset),
                 (uint8_t)(base + offset + 1),
                 (uint8_t)(base + offset + 2));
}

static int pixel_matches_color(const uint8_t *pixel, RGB_COLOR color)
{
    if (color == 0) {
        return pixel[0] == 0 && pixel[1] == 0
            && pixel[2] == 0 && pixel[3] == 0;
    }
    return pixel[0] == (uint8_t)RED_VALUE(color)
        && pixel[1] == (uint8_t)GREEN_VALUE(color)
        && pixel[2] == (uint8_t)BLUE_VALUE(color)
        && pixel[3] == 255;
}

static uint8_t fixture_base(const char *filename)
{
    if (strcmp(filename, "fixture-ordinary") == 0)
        return 11;
    if (strcmp(filename, "fixture-rotatable") == 0)
        return 61;
    if (strcmp(filename, "fixture-map") == 0)
        return 101;
    if (strcmp(filename, "fixture-late") == 0)
        return 141;
    return 201;
}

static int fixture_picture_init(xp_picture_t *picture,
                                const char *filename, int count,
                                void *context)
{
    FixtureState *state = context;
    int frame_count = ABS(count);
    int frame;
    int x;
    int y;
    uint8_t base = fixture_base(filename);

    state->picture_init_calls++;
    if (strcmp(filename, "fixture-disabled-map") == 0) {
        state->disabled_map_init_calls++;
        return -1;
    }
    if (strcmp(filename, "fixture-ordinary") == 0)
        state->ordinary_signed_count = count;
    else if (strcmp(filename, "fixture-rotatable") == 0)
        state->rotatable_signed_count = count;
    else if (strcmp(filename, "fixture-map") == 0)
        state->map_signed_count = count;

    memset(picture, 0, sizeof(*picture));
    picture->width = 3;
    picture->height = strcmp(filename, "fixture-rotatable") == 0 ? 2 : 3;
    picture->count = count;
    picture->data = calloc((size_t)frame_count, sizeof(*picture->data));
    picture->bbox = calloc((size_t)frame_count, sizeof(*picture->bbox));
    if (picture->data == NULL || picture->bbox == NULL)
        goto failure;
    for (frame = 0; frame < frame_count; frame++) {
        picture->data[frame] = calloc(
            (size_t)picture->width * picture->height,
            sizeof(*picture->data[frame]));
        if (picture->data[frame] == NULL)
            goto failure;
        for (y = 0; y < (int)picture->height; y++) {
            for (x = 0; x < (int)picture->width; x++) {
                picture->data[frame][y * picture->width + x] =
                    fixture_color(base, frame, x, y);
            }
        }
    }
    state->picture_init_successes++;
    return 0;

failure:
    for (frame = 0; frame < frame_count; frame++)
        free(picture->data == NULL ? NULL : picture->data[frame]);
    free(picture->data);
    free(picture->bbox);
    memset(picture, 0, sizeof(*picture));
    return -1;
}

static RGB_COLOR fixture_picture_get_pixel(const xp_picture_t *picture,
                                           int frame, int x, int y,
                                           void *context)
{
    (void)context;
    return picture->data[frame][y * picture->width + x];
}

static void fixture_picture_cleanup(xp_picture_t *picture, void *context)
{
    FixtureState *state = context;
    int frame_count = ABS(picture->count);
    int frame;

    for (frame = 0; frame < frame_count; frame++)
        free(picture->data == NULL ? NULL : picture->data[frame]);
    free(picture->data);
    free(picture->bbox);
    memset(picture, 0, sizeof(*picture));
    state->picture_cleanup_calls++;
}

static FakeTexture *fake_texture_from_handle(RendererTexture *texture)
{
    int index;

    for (index = 0; index < fake_texture_count; index++) {
        if (&fake_textures[index].handle == texture)
            return &fake_textures[index];
    }
    return NULL;
}

static const uint8_t *fake_pixel(const FakeTexture *texture, int x, int y)
{
    return texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
}

static int pixel_equals(const FakeTexture *texture, int x, int y,
                        uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t alpha)
{
    const uint8_t *pixel = fake_pixel(texture, x, y);

    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
}

static FakeTexture *find_texture(int width, int height, uint8_t first_red)
{
    int index;

    for (index = 0; index < fake_texture_count; index++) {
        FakeTexture *texture = &fake_textures[index];

        if (texture->desc.width == width && texture->desc.height == height
            && texture->pixels != NULL
            && fake_pixel(texture, 0, 0)[0] == first_red
            && texture->destroy_count == 0) {
            return texture;
        }
    }
    return NULL;
}

static int check_semantic_map_source(const FakeTexture *texture, uint8_t base)
{
    int x;
    int y;

    TEST_CHECK(texture != NULL);
    TEST_CHECK(texture->desc.width == 3);
    TEST_CHECK(texture->desc.height == 3);
    TEST_CHECK(texture->pitch == 3 * 4);
    for (y = 0; y < 3; y++) {
        for (x = 0; x < 3; x++) {
            TEST_CHECK(pixel_matches_color(
                fake_pixel(texture, x, y), fixture_color(base, 0, x, y)));
        }
    }
    return 0;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color = {
        (uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16),
        (uint8_t)(rgba >> 8), (uint8_t)rgba
    };

    return color;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    FakeTexture *record;
    int y;

    texture_create_attempts++;
    if (renderer == NULL || desc == NULL || rgba_pixels == NULL
        || texture == NULL || renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (pitch < (size_t)desc->width * 4)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (fail_texture_create_attempt == texture_create_attempts)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (fake_texture_count >= MAX_FAKE_TEXTURES)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    record = &fake_textures[fake_texture_count++];
    memset(record, 0, sizeof(*record));
    record->handle.identifier = (unsigned int)fake_texture_count;
    record->desc = *desc;
    record->pitch = pitch;
    record->pixels = malloc(record->pitch * (size_t)desc->height);
    if (record->pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    for (y = 0; y < desc->height; y++) {
        memcpy(record->pixels + (size_t)y * record->pitch,
               rgba_pixels + (size_t)y * pitch, record->pitch);
    }
    *texture = &record->handle;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    if (renderer != &fake_renderer || renderer->frame_active || record == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    record->destroy_count++;
    texture_destroy_calls++;
    return record->destroy_count == 1 ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_STATE;
}

RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    FakeDraw *draw;

    draw_attempts++;
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture == NULL || vertices == NULL
        || vertex_count != 6 || draw_count >= MAX_DRAWS)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    draw = &fake_draws[draw_count++];
    draw->texture = texture;
    draw->vertex_count = vertex_count;
    memcpy(draw->vertices, vertices, sizeof(draw->vertices));
    pending_draws++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    blend_attempts++;
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return blend_result;
}

RendererStatus Renderer_set_transform_2d(Renderer *renderer,
                                         RendererTransform2D transform)
{
    (void)transform;
    return renderer == &fake_renderer ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_ARGUMENT;
}

RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint)
{
    RendererVertex2D vertices[6] = {
        {left, top, u0, v0, tint}, {right, top, u1, v0, tint},
        {right, bottom, u1, v1, tint}, {left, top, u0, v0, tint},
        {right, bottom, u1, v1, tint}, {left, bottom, u0, v1, tint}
    };

    return Renderer_draw_triangles(renderer, texture, vertices, 6);
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
    tracked_result_calls++;
    if (renderer != &fake_sdl_renderer)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->frame_result == RENDERER_STATUS_OK
        && status != RENDERER_STATUS_OK) {
        renderer->frame_result = status;
    }
    return renderer->frame_result;
}

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    return renderer == &fake_sdl_renderer
        ? renderer->frame_result : RENDERER_STATUS_INVALID_ARGUMENT;
}

RendererStatus Sdl_renderer_flush(SdlRenderer *renderer)
{
    if (renderer != &fake_sdl_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    flush_calls++;
    if (flush_result != RENDERER_STATUS_OK)
        return flush_result;
    pending_draws = 0;
    return RENDERER_STATUS_OK;
}

int Picture_init(xp_picture_t *picture, const char *filename, int count)
{
    return fixture_picture_init(picture, filename, count, &fixture_state);
}

RGB_COLOR Picture_get_pixel(const xp_picture_t *picture,
                            int frame, int x, int y)
{
    return fixture_picture_get_pixel(picture, frame, x, y, &fixture_state);
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

void fatal(const char *format, ...)
{
    (void)format;
}

static void reset_active_frame_capture(void)
{
    memset(fake_draws, 0, sizeof(fake_draws));
    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frame_active = 1;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
    draw_count = 0;
    draw_attempts = 0;
    blend_attempts = 0;
    flush_calls = 0;
    tracked_result_calls = 0;
    pending_draws = 0;
    draw_result = RENDERER_STATUS_OK;
    blend_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
}

static void finish_active_frame(void)
{
    fake_sdl_renderer.frame_active = 0;
    fake_renderer.frame_active = 0;
}

static int check_registration_prepare_and_pixels(int *ordinary_id,
                                                 int *rotatable_id,
                                                 int *map_id)
{
    const ImagesTestHooks hooks = {
        fixture_picture_init,
        fixture_picture_get_pixel,
        fixture_picture_cleanup,
        &fixture_state
    };
    FakeTexture *ordinary;
    FakeTexture *rotatable;
    FakeTexture *map;
    FakeTexture *single_frame_rotated;
    image_t *wormhole;
    int attempts_after_prepare;
    int loads_after_prepare;
    int first_attempt_destroy_count;

    Images_test_set_hooks(&hooks);
    *ordinary_id = Bitmap_add("fixture-ordinary", -2, false);
    *rotatable_id = Bitmap_add("fixture-rotatable", 4, false);
    TEST_CHECK(*ordinary_id == 0);
    TEST_CHECK(*rotatable_id == 1);
    TEST_CHECK(Images_init() == 0);
    *map_id = Bitmap_add("fixture-map", 1, false);
    TEST_CHECK(*map_id > *rotatable_id);

    TEST_CHECK(Image_get(*ordinary_id) == NULL);
    TEST_CHECK(Image_get(*rotatable_id) == NULL);
    TEST_CHECK(Image_get(*map_id) == NULL);
    TEST_CHECK(fixture_state.picture_init_calls == 0);
    TEST_CHECK(texture_create_attempts == 0);

    fail_texture_create_attempt = 2;
    TEST_CHECK(Images_prepare(&fake_renderer) == -1);
    TEST_CHECK(Image_get(*ordinary_id) == NULL);
    TEST_CHECK(Image_get(*rotatable_id) == NULL);
    TEST_CHECK(Image_get(*map_id) == NULL);
    TEST_CHECK(fake_texture_count == 1);
    TEST_CHECK(fake_textures[0].destroy_count == 1);
    first_attempt_destroy_count = texture_destroy_calls;

    fail_texture_create_attempt = 0;
    TEST_CHECK(Images_prepare(&fake_renderer) == 0);
    TEST_CHECK(Image_get(*ordinary_id) != NULL);
    TEST_CHECK(Image_get(*rotatable_id) != NULL);
    TEST_CHECK(Image_get(*map_id) != NULL);
    TEST_CHECK(fixture_state.ordinary_signed_count == -2);
    TEST_CHECK(fixture_state.rotatable_signed_count == 4);
    TEST_CHECK(fixture_state.map_signed_count == -1);
    TEST_CHECK(fake_renderer.frame_active == 0);

    ordinary = find_texture(6, 3, 11);
    rotatable = find_texture(12, 2, 61);
    map = find_texture(3, 3, 101);
    TEST_CHECK(ordinary != NULL);
    TEST_CHECK(rotatable != NULL);
    TEST_CHECK(map != NULL);
    TEST_CHECK(ordinary->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(ordinary->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(ordinary->pitch == 6 * 4);
    TEST_CHECK(rotatable->desc.filter == RENDERER_TEXTURE_FILTER_LINEAR);
    TEST_CHECK(rotatable->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(rotatable->pitch == 12 * 4);
    TEST_CHECK(map->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(map->desc.wrap == RENDERER_TEXTURE_WRAP_REPEAT);
    TEST_CHECK(check_semantic_map_source(map, 101) == 0);

    wormhole = Image_get(*rotatable_id + 1 + IMG_WORMHOLE);
    TEST_CHECK(wormhole != NULL);
    single_frame_rotated = fake_texture_from_handle(wormhole->texture);
    TEST_CHECK(single_frame_rotated != NULL);
    TEST_CHECK(single_frame_rotated->desc.filter
               == RENDERER_TEXTURE_FILTER_LINEAR);

    TEST_CHECK(pixel_equals(ordinary, 0, 0, 11, 12, 13, 255));
    TEST_CHECK(pixel_equals(ordinary, 2, 0, 17, 18, 19, 255));
    TEST_CHECK(pixel_equals(ordinary, 0, 2, 29, 30, 31, 255));
    TEST_CHECK(pixel_equals(ordinary, 1, 1, 0, 0, 0, 0));
    TEST_CHECK(pixel_equals(ordinary, 3, 0, 38, 39, 40, 255));
    TEST_CHECK(pixel_equals(ordinary, 5, 2, 62, 63, 64, 255));

    attempts_after_prepare = texture_create_attempts;
    loads_after_prepare = fixture_state.picture_init_calls;
    TEST_CHECK(Images_prepare(&fake_renderer) == 0);
    TEST_CHECK(texture_create_attempts == attempts_after_prepare);
    TEST_CHECK(fixture_state.picture_init_calls == loads_after_prepare);
    TEST_CHECK(texture_destroy_calls == first_attempt_destroy_count);
    return 0;
}

static int check_late_registration(void)
{
    FakeTexture *late;
    int late_id;
    int create_attempts_before;
    int load_calls_before;

    late_id = Bitmap_add("fixture-late", 1, false);
    TEST_CHECK(late_id >= 0);
    create_attempts_before = texture_create_attempts;
    load_calls_before = fixture_state.picture_init_calls;
    TEST_CHECK(Image_get(late_id) == NULL);
    TEST_CHECK(texture_create_attempts == create_attempts_before);
    TEST_CHECK(fixture_state.picture_init_calls == load_calls_before);
    TEST_CHECK(Images_prepare(&fake_renderer) == 0);
    TEST_CHECK(Image_get(late_id) != NULL);
    TEST_CHECK(texture_create_attempts == create_attempts_before + 1);
    TEST_CHECK(fixture_state.picture_init_calls == load_calls_before + 1);
    late = find_texture(3, 3, 141);
    TEST_CHECK(late != NULL);
    TEST_CHECK(late->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(late->desc.wrap == RENDERER_TEXTURE_WRAP_REPEAT);
    TEST_CHECK(check_semantic_map_source(late, 141) == 0);
    return 0;
}

static int check_disabled_map_texture_does_not_block_preparation(void)
{
    int disabled_map_id;
    int init_calls_before;

    disabled_map_id = Bitmap_add("fixture-disabled-map", 1, false);
    TEST_CHECK(disabled_map_id >= 0);
    init_calls_before = fixture_state.picture_init_calls;
    instruments.texturedWalls = false;

    TEST_CHECK(Images_prepare(&fake_renderer) == 0);
    TEST_CHECK(Image_get(disabled_map_id) == NULL);
    TEST_CHECK(fixture_state.picture_init_calls == init_calls_before);
    TEST_CHECK(fixture_state.disabled_map_init_calls == 0);

    instruments.texturedWalls = true;
    return 0;
}

static int check_semantic_world_and_hud_draws(int ordinary_id)
{
    const RendererColor tint = {17, 34, 51, 68};
    FakeDraw *world;
    FakeDraw *hud;
    int flushes_before = flush_calls;

    Image_paint(ordinary_id, 10, 20, 1, 0x11223344);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(flush_calls == flushes_before + 1);
    world = &fake_draws[0];
    TEST_CHECK(world->vertex_count == 6);
    TEST_CHECK(vertex_equal(&world->vertices[0], 10.0f, 23.0f,
                            3.0f / 6.0f, 0.0f, tint));
    TEST_CHECK(vertex_equal(&world->vertices[1], 13.0f, 23.0f,
                            6.0f / 6.0f, 0.0f, tint));
    TEST_CHECK(vertex_equal(&world->vertices[2], 13.0f, 20.0f,
                            6.0f / 6.0f, 3.0f / 3.0f, tint));
    TEST_CHECK(vertex_equal(&world->vertices[5], 10.0f, 20.0f,
                            3.0f / 6.0f, 3.0f / 3.0f, tint));

    Image_paint_hud(ordinary_id, 30, 40, 0, 0x11223344);
    TEST_CHECK(draw_count == 2);
    TEST_CHECK(flush_calls == flushes_before + 2);
    hud = &fake_draws[1];
    TEST_CHECK(vertex_equal(&hud->vertices[0], 30.0f, 40.0f,
                            0.0f, 0.0f, tint));
    TEST_CHECK(vertex_equal(&hud->vertices[1], 33.0f, 40.0f,
                            3.0f / 6.0f, 0.0f, tint));
    TEST_CHECK(vertex_equal(&hud->vertices[2], 33.0f, 43.0f,
                            3.0f / 6.0f, 3.0f / 3.0f, tint));
    TEST_CHECK(vertex_equal(&hud->vertices[5], 30.0f, 43.0f,
                            0.0f, 3.0f / 3.0f, tint));
    return 0;
}

static int check_area_and_rotated_draws(int ordinary_id, int rotatable_id)
{
    const RendererColor area_tint = {160, 176, 192, 208};
    const RendererColor rotate_tint = {5, 6, 7, 8};
    irec_t area = {1, 1, 2, 1};
    FakeDraw *area_draw;
    FakeDraw *rotated;
    int flushes_before = flush_calls;

    Image_paint_area(ordinary_id, 4, 5, 0, &area, 0xa0b0c0d0);
    TEST_CHECK(draw_count == 3);
    area_draw = &fake_draws[2];
    TEST_CHECK(vertex_equal(&area_draw->vertices[0], 4.0f, 6.0f,
                            1.0f / 6.0f, 1.0f / 3.0f, area_tint));
    TEST_CHECK(vertex_equal(&area_draw->vertices[2], 6.0f, 5.0f,
                            3.0f / 6.0f, 2.0f / 3.0f, area_tint));

    Image_paint_rotated(rotatable_id, 10, 20, TABLE_SIZE / 4, 0x05060708);
    TEST_CHECK(draw_count == 4);
    rotated = &fake_draws[3];
    TEST_CHECK(vertex_equal(&rotated->vertices[0], 9.0f, 18.5f,
                            0.0f, 0.0f, rotate_tint));
    TEST_CHECK(vertex_equal(&rotated->vertices[1], 9.0f, 21.5f,
                            3.0f / 12.0f, 0.0f, rotate_tint));
    TEST_CHECK(vertex_equal(&rotated->vertices[2], 11.0f, 21.5f,
                            3.0f / 12.0f, 1.0f, rotate_tint));
    TEST_CHECK(vertex_equal(&rotated->vertices[5], 11.0f, 18.5f,
                            0.0f, 1.0f, rotate_tint));
    TEST_CHECK(flush_calls == flushes_before + 2);
    return 0;
}

static int check_image_draw_failures_are_frame_sticky(int ordinary_id)
{
    image_t *image;
    Renderer *original_owner;
    int tracked_before_retry;

    reset_active_frame_capture();
    Image_paint(ordinary_id, 10, 20, -1, 0x11223344);
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(tracked_result_calls == 2);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_calls == 0);
    Image_paint(ordinary_id, 30, 40, 0, 0x55667788);
    TEST_CHECK(tracked_result_calls == 3);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_calls == 0);

    reset_active_frame_capture();
    image = Image_get(ordinary_id);
    TEST_CHECK(image != NULL);
    original_owner = image->renderer;
    image->renderer = NULL;
    Image_paint(ordinary_id, 10, 20, 0, 0x11223344);
    image->renderer = original_owner;
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(tracked_result_calls == 2);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_calls == 0);

    reset_active_frame_capture();
    blend_result = RENDERER_STATUS_RESOURCE_MISMATCH;

    Image_paint(ordinary_id, 10, 20, 0, 0x11223344);

    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(tracked_result_calls == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(pending_draws == 0);
    TEST_CHECK(flush_calls == 0);

    tracked_before_retry = tracked_result_calls;
    blend_result = RENDERER_STATUS_BACKEND_ERROR;
    Image_paint(ordinary_id, 30, 40, 0, 0x55667788);
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(tracked_result_calls == tracked_before_retry + 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(flush_calls == 0);

    reset_active_frame_capture();
    draw_result = RENDERER_STATUS_OUT_OF_MEMORY;

    Image_paint(ordinary_id, 10, 20, 0, 0x11223344);

    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(tracked_result_calls == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(pending_draws == 0);
    TEST_CHECK(flush_calls == 0);

    tracked_before_retry = tracked_result_calls;
    draw_result = RENDERER_STATUS_BACKEND_ERROR;
    Image_paint(ordinary_id, 30, 40, 0, 0x55667788);
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(tracked_result_calls == tracked_before_retry + 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(draw_count == 0);
    TEST_CHECK(flush_calls == 0);

    reset_active_frame_capture();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;

    Image_paint(ordinary_id, 10, 20, 0, 0x11223344);

    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(tracked_result_calls == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(pending_draws == 1);
    TEST_CHECK(flush_calls == 1);

    tracked_before_retry = tracked_result_calls;
    flush_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Image_paint(ordinary_id, 30, 40, 0, 0x55667788);
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(tracked_result_calls == tracked_before_retry + 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(draw_count == 1);
    TEST_CHECK(pending_draws == 1);
    TEST_CHECK(flush_calls == 1);

    flush_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_flush(&fake_sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(flush_calls == 2);
    TEST_CHECK(pending_draws == 0);
    TEST_CHECK(Sdl_renderer_frame_result(&fake_sdl_renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    return 0;
}

static int check_cleanup_is_exact(void)
{
    int index;
    int destroys_after_first_cleanup;

    Images_cleanup();
    for (index = 0; index < fake_texture_count; index++)
        TEST_CHECK(fake_textures[index].destroy_count == 1);
    TEST_CHECK(fixture_state.picture_cleanup_calls
               == fixture_state.picture_init_successes);
    destroys_after_first_cleanup = texture_destroy_calls;

    Images_cleanup();
    TEST_CHECK(texture_destroy_calls == destroys_after_first_cleanup);
    TEST_CHECK(Bitmap_add("registry-reset", 1, false) == 0);
    Images_cleanup();
    Images_test_reset_hooks();
    return 0;
}

int main(void)
{
    int ordinary_id;
    int rotatable_id;
    int map_id;

    instruments.texturedWalls = true;
    if (check_registration_prepare_and_pixels(
            &ordinary_id, &rotatable_id, &map_id) != 0)
        return 1;
    if (check_late_registration() != 0)
        return 1;
    if (check_disabled_map_texture_does_not_block_preparation() != 0)
        return 1;
    reset_active_frame_capture();
    if (check_semantic_world_and_hud_draws(ordinary_id) != 0)
        return 1;
    if (check_area_and_rotated_draws(ordinary_id, rotatable_id) != 0)
        return 1;
    if (check_image_draw_failures_are_frame_sticky(ordinary_id) != 0)
        return 1;
    finish_active_frame();
    TEST_CHECK(map_id >= 0);
    if (check_cleanup_is_exact() != 0)
        return 1;
    return 0;
}
