#include "test_helpers.h"

#include "text.h"
#include "text_renderer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FLOAT_TOLERANCE 0.0001f

struct TextRenderer {
    int identity;
    int frame_active;
};

struct TextRendererCache {
    TextRenderer *text_renderer;
    unsigned char *text;
    size_t text_length;
    TextGeometryMetrics metrics;
};

typedef struct FakeDraw {
    TextRenderer *text_renderer;
    const TextRendererCache *cache;
    RendererPoint2D anchor;
    TextGeometryHorizontalAlign horizontal;
    TextGeometryVerticalAlign vertical;
    RendererColor color;
    TextRendererSpace space;
} FakeDraw;

unsigned draw_width = 640;
unsigned draw_height = 480;

static TextRenderer first_text_renderer = {1, 0};
static FakeDraw last_draw;
static int cache_replace_calls;
static int cache_destroy_calls;
static int cached_draw_calls;
static int ordering_barrier_attempts;
static int fail_next_cache_replace;
static int fail_next_cached_draw;
static int gl_texture_create_calls;
static int gl_texture_destroy_calls;
static int renderer_texture_create_calls;
static int renderer_texture_update_calls;
static int renderer_texture_destroy_calls;
static int renderer_mesh_create_calls;
static int renderer_mesh_update_calls;
static int renderer_mesh_destroy_calls;

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

static int no_gpu_resource_calls(void)
{
    return gl_texture_create_calls == 0 && gl_texture_destroy_calls == 0
        && renderer_texture_create_calls == 0
        && renderer_texture_update_calls == 0
        && renderer_texture_destroy_calls == 0
        && renderer_mesh_create_calls == 0
        && renderer_mesh_update_calls == 0
        && renderer_mesh_destroy_calls == 0;
}

static void reset_fixture(void)
{
    memset(&last_draw, 0, sizeof(last_draw));
    first_text_renderer.frame_active = 0;
    cache_replace_calls = 0;
    cache_destroy_calls = 0;
    cached_draw_calls = 0;
    ordering_barrier_attempts = 0;
    fail_next_cache_replace = 0;
    fail_next_cached_draw = 0;
    gl_texture_create_calls = 0;
    gl_texture_destroy_calls = 0;
    renderer_texture_create_calls = 0;
    renderer_texture_update_calls = 0;
    renderer_texture_destroy_calls = 0;
    renderer_mesh_create_calls = 0;
    renderer_mesh_update_calls = 0;
    renderer_mesh_destroy_calls = 0;
}

static RendererStatus fake_measure(const unsigned char *text,
                                   size_t text_length,
                                   TextGeometryMetrics *metrics)
{
    TextGeometryMetrics candidate = {0.0f, 0.0f, 0, 0, 0};
    float line_width = 0.0f;
    size_t index;

    if (text == NULL || metrics == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (text_length == 0) {
        *metrics = candidate;
        return RENDERER_STATUS_OK;
    }

    candidate.line_count = 1;
    for (index = 0; index < text_length; index++) {
        float advance;

        if (text[index] == (unsigned char)'\n') {
            if (line_width > candidate.width)
                candidate.width = line_width;
            line_width = 0.0f;
            if (index + 1 < text_length)
                candidate.line_count++;
            continue;
        }
        if (text[index] == (unsigned char)' ')
            advance = 4.0f;
        else if (text[index] == (unsigned char)'A')
            advance = 7.0f;
        else if (text[index] == (unsigned char)'B')
            advance = 6.0f;
        else
            advance = 5.0f;
        line_width += advance;
        if (text[index] != (unsigned char)' ')
            candidate.glyph_count++;
    }
    if (line_width > candidate.width)
        candidate.width = line_width;
    candidate.height = 10.0f
        + (float)(candidate.line_count - 1) * 12.0f;
    candidate.vertex_count = candidate.glyph_count * 6;
    *metrics = candidate;
    return RENDERER_STATUS_OK;
}

RendererStatus Text_renderer_measure(const TextRenderer *text_renderer,
                                     const unsigned char *text,
                                     size_t text_length,
                                     TextGeometryMetrics *metrics)
{
    if (text_renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return fake_measure(text, text_length, metrics);
}

RendererStatus Text_renderer_cache_replace(TextRendererCache **cache,
                                           const TextRenderer *text_renderer,
                                           const unsigned char *text,
                                           size_t text_length)
{
    TextRendererCache *candidate;
    TextGeometryMetrics metrics;
    RendererStatus status;

    cache_replace_calls++;
    if (cache == NULL || text_renderer == NULL || text == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (*cache != NULL && (*cache)->text_renderer != text_renderer)
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (fail_next_cache_replace) {
        fail_next_cache_replace = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (*cache != NULL && (*cache)->text_length == text_length
        && (text_length == 0
            || memcmp((*cache)->text, text, text_length) == 0)) {
        return RENDERER_STATUS_OK;
    }

    status = fake_measure(text, text_length, &metrics);
    if (status != RENDERER_STATUS_OK)
        return status;
    candidate = calloc(1, sizeof(*candidate));
    if (candidate == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (text_length > 0) {
        candidate->text = malloc(text_length);
        if (candidate->text == NULL) {
            free(candidate);
            return RENDERER_STATUS_OUT_OF_MEMORY;
        }
        memcpy(candidate->text, text, text_length);
    }
    candidate->text_renderer = (TextRenderer *)text_renderer;
    candidate->text_length = text_length;
    candidate->metrics = metrics;

    Text_renderer_cache_destroy(cache);
    *cache = candidate;
    return RENDERER_STATUS_OK;
}

void Text_renderer_cache_destroy(TextRendererCache **cache)
{
    if (cache == NULL || *cache == NULL)
        return;
    free((*cache)->text);
    free(*cache);
    *cache = NULL;
    cache_destroy_calls++;
}

RendererStatus Text_renderer_cache_metrics(
    const TextRendererCache *cache, TextGeometryMetrics *metrics)
{
    if (cache == NULL || metrics == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *metrics = cache->metrics;
    return RENDERER_STATUS_OK;
}

RendererStatus Text_renderer_draw_cached(
    TextRenderer *text_renderer, const TextRendererCache *cache,
    RendererPoint2D anchor, TextGeometryHorizontalAlign horizontal,
    TextGeometryVerticalAlign vertical, RendererColor color,
    TextRendererSpace space)
{
    cached_draw_calls++;
    if (text_renderer == NULL || cache == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (cache->text_renderer != text_renderer)
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (!text_renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;

    last_draw.text_renderer = text_renderer;
    last_draw.cache = cache;
    last_draw.anchor = anchor;
    last_draw.horizontal = horizontal;
    last_draw.vertical = vertical;
    last_draw.color = color;
    last_draw.space = space;
    if (cache->metrics.vertex_count == 0)
        return RENDERER_STATUS_OK;

    ordering_barrier_attempts++;
    if (fail_next_cached_draw) {
        fail_next_cached_draw = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    return RENDERER_STATUS_OK;
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

void APIENTRY glGenTextures(GLsizei count, GLuint *textures)
{
    GLsizei index;

    gl_texture_create_calls += count;
    for (index = 0; index < count; index++)
        textures[index] = (GLuint)(index + 1);
}

void APIENTRY glDeleteTextures(GLsizei count, const GLuint *textures)
{
    (void)textures;
    gl_texture_destroy_calls += count;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *pixels, size_t pitch, RendererTexture **texture)
{
    (void)renderer;
    (void)desc;
    (void)pixels;
    (void)pitch;
    (void)texture;
    renderer_texture_create_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *pixels,
                                       size_t pitch)
{
    (void)renderer;
    (void)texture;
    (void)region;
    (void)pixels;
    (void)pitch;
    renderer_texture_update_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    (void)renderer;
    (void)texture;
    renderer_texture_destroy_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_create(Renderer *renderer,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    RendererMesh **mesh)
{
    (void)renderer;
    (void)vertices;
    (void)vertex_count;
    (void)mesh;
    renderer_mesh_create_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_update(Renderer *renderer, RendererMesh *mesh,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count)
{
    (void)renderer;
    (void)mesh;
    (void)vertices;
    (void)vertex_count;
    renderer_mesh_update_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_mesh_destroy(Renderer *renderer, RendererMesh *mesh)
{
    (void)renderer;
    (void)mesh;
    renderer_mesh_destroy_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static font_data make_font(TextRenderer *text_renderer, GLuint height)
{
    font_data font;

    memset(&font, 0, sizeof(font));
    font.h = height;
    font.ttffont = (TTF_Font *)(uintptr_t)1;
    font.text_renderer = text_renderer;
    return font;
}

static int check_render_is_cpu_only_and_empty_compatible(void)
{
    font_data font;
    string_tex_t string = {0};

    reset_fixture();
    first_text_renderer.frame_active = 1;
    font = make_font(&first_text_renderer, 9);

    TEST_CHECK(render_text(&font, "AB", &string));
    TEST_CHECK(string.text_renderer == &first_text_renderer);
    TEST_CHECK(string.cache != NULL);
    TEST_CHECK(strcmp(string.text, "AB") == 0);
    TEST_CHECK(string.width == 13);
    TEST_CHECK(string.height == 10);
    TEST_CHECK(string.font_height == 9);
    TEST_CHECK(no_gpu_resource_calls());

    TEST_CHECK(render_text(&font, "AB", &string));
    TEST_CHECK(string.cache != NULL);
    TEST_CHECK(strcmp(string.text, "AB") == 0);
    TEST_CHECK(no_gpu_resource_calls());

    TEST_CHECK(render_text(&font, "", &string));
    TEST_CHECK(string.cache != NULL);
    TEST_CHECK(string.cache->text_length == 1);
    TEST_CHECK(string.cache->text[0] == (unsigned char)' ');
    TEST_CHECK(strcmp(string.text, " ") == 0);
    TEST_CHECK(string.width == 4);
    TEST_CHECK(string.height == 10);
    TEST_CHECK(string.font_height == 9);
    TEST_CHECK(no_gpu_resource_calls());

    TEST_CHECK(disp_text(&string, 0x01020304, LEFT, DOWN, 8, 12, true)
               == RENDERER_STATUS_OK);
    TEST_CHECK(cached_draw_calls == 1);
    TEST_CHECK(ordering_barrier_attempts == 0);

    free_string_texture(&string);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

static int check_replace_is_atomic_and_cleanup_is_idempotent(void)
{
    font_data first_font;
    string_tex_t string = {0};
    TextRendererCache *original_cache;
    TextRenderer *original_renderer;
    char *original_text;
    int original_width;
    int original_height;
    int original_font_height;
    int destroys_after_first_cleanup;

    reset_fixture();
    first_text_renderer.frame_active = 1;
    first_font = make_font(&first_text_renderer, 9);

    TEST_CHECK(render_text(&first_font, "AB", &string));
    original_cache = string.cache;
    original_renderer = string.text_renderer;
    original_text = string.text;
    original_width = string.width;
    original_height = string.height;
    original_font_height = string.font_height;

    fail_next_cache_replace = 1;
    TEST_CHECK(!render_text(&first_font, "A", &string));
    TEST_CHECK(string.cache == original_cache);
    TEST_CHECK(string.text_renderer == original_renderer);
    TEST_CHECK(string.text == original_text);
    TEST_CHECK(strcmp(string.text, "AB") == 0);
    TEST_CHECK(string.width == original_width);
    TEST_CHECK(string.height == original_height);
    TEST_CHECK(string.font_height == original_font_height);

    TEST_CHECK(render_text(&first_font, "A", &string));
    TEST_CHECK(string.cache != NULL);
    TEST_CHECK(string.text_renderer == &first_text_renderer);
    TEST_CHECK(strcmp(string.text, "A") == 0);
    TEST_CHECK(string.width == 7);
    TEST_CHECK(string.height == 10);
    TEST_CHECK(string.font_height == 9);
    TEST_CHECK(no_gpu_resource_calls());

    free_string_texture(&string);
    TEST_CHECK(string.text_renderer == NULL);
    TEST_CHECK(string.cache == NULL);
    TEST_CHECK(string.text == NULL);
    TEST_CHECK(string.width == 0);
    TEST_CHECK(string.height == 0);
    TEST_CHECK(string.font_height == 0);
    TEST_CHECK(no_gpu_resource_calls());
    destroys_after_first_cleanup = cache_destroy_calls;

    free_string_texture(&string);
    free_string_texture(NULL);
    TEST_CHECK(cache_destroy_calls == destroys_after_first_cleanup);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

static int check_hud_coordinates_and_alignments(void)
{
    static const int horizontal_inputs[] = {LEFT, CENTER, RIGHT};
    static const TextGeometryHorizontalAlign horizontal_outputs[] = {
        TEXT_GEOMETRY_ALIGN_LEFT,
        TEXT_GEOMETRY_ALIGN_CENTER,
        TEXT_GEOMETRY_ALIGN_RIGHT
    };
    static const int vertical_inputs[] = {DOWN, CENTER, UP};
    static const TextGeometryVerticalAlign vertical_outputs[] = {
        TEXT_GEOMETRY_ALIGN_TOP,
        TEXT_GEOMETRY_ALIGN_MIDDLE,
        TEXT_GEOMETRY_ALIGN_BOTTOM
    };
    const RendererColor expected_color = {0x11, 0x22, 0x33, 0x44};
    font_data font;
    string_tex_t string = {0};
    size_t index;

    reset_fixture();
    first_text_renderer.frame_active = 1;
    font = make_font(&first_text_renderer, 9);
    TEST_CHECK(render_text(&font, "AB", &string));

    for (index = 0; index < 3; index++) {
        TEST_CHECK(disp_text(&string, 0x11223344,
                             horizontal_inputs[index],
                             vertical_inputs[index],
                             30, 70, true) == RENDERER_STATUS_OK);
        TEST_CHECK(last_draw.text_renderer == &first_text_renderer);
        TEST_CHECK(last_draw.cache == string.cache);
        TEST_CHECK(float_equal(last_draw.anchor.x, 30.0f));
        TEST_CHECK(float_equal(last_draw.anchor.y,
                               (float)draw_height - 70.0f));
        TEST_CHECK(last_draw.horizontal == horizontal_outputs[index]);
        TEST_CHECK(last_draw.vertical == vertical_outputs[index]);
        TEST_CHECK(color_equal(last_draw.color, expected_color));
        TEST_CHECK(last_draw.space == TEXT_RENDERER_SPACE_HUD);
    }
    TEST_CHECK(cached_draw_calls == 3);
    TEST_CHECK(ordering_barrier_attempts == 3);
    TEST_CHECK(no_gpu_resource_calls());

    free_string_texture(&string);
    return 0;
}

static int check_world_coordinates_and_ordering_failure(void)
{
    const RendererColor expected_color = {0xAA, 0xBB, 0xCC, 0xDD};
    font_data font;
    string_tex_t string = {0};
    TextRendererCache *cache_before_failure;
    char *text_before_failure;

    reset_fixture();
    first_text_renderer.frame_active = 1;
    font = make_font(&first_text_renderer, 9);
    TEST_CHECK(render_text(&font, "AB", &string));

    TEST_CHECK(disp_text(&string, 0xAABBCCDD, RIGHT, UP,
                         101, 202, false) == RENDERER_STATUS_OK);
    TEST_CHECK(float_equal(last_draw.anchor.x, 101.0f));
    TEST_CHECK(float_equal(last_draw.anchor.y, 202.0f));
    TEST_CHECK(last_draw.horizontal == TEXT_GEOMETRY_ALIGN_RIGHT);
    TEST_CHECK(last_draw.vertical == TEXT_GEOMETRY_ALIGN_BOTTOM);
    TEST_CHECK(color_equal(last_draw.color, expected_color));
    TEST_CHECK(last_draw.space == TEXT_RENDERER_SPACE_WORLD);
    TEST_CHECK(ordering_barrier_attempts == 1);

    cache_before_failure = string.cache;
    text_before_failure = string.text;
    fail_next_cached_draw = 1;
    TEST_CHECK(disp_text(&string, 0xAABBCCDD, CENTER, CENTER,
                         11, 22, false) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(cached_draw_calls == 2);
    TEST_CHECK(ordering_barrier_attempts == 2);
    TEST_CHECK(string.cache == cache_before_failure);
    TEST_CHECK(string.text == text_before_failure);
    TEST_CHECK(strcmp(string.text, "AB") == 0);
    TEST_CHECK(no_gpu_resource_calls());

    first_text_renderer.frame_active = 0;
    TEST_CHECK(disp_text(&string, 0xAABBCCDD, LEFT, DOWN,
                         0, 0, false) == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(string.cache == cache_before_failure);
    TEST_CHECK(string.text == text_before_failure);

    free_string_texture(&string);
    return 0;
}

static int check_invalid_arguments_leave_destinations_unchanged(void)
{
    font_data missing_renderer;
    font_data font;
    string_tex_t string = {0};

    reset_fixture();
    first_text_renderer.frame_active = 1;
    font = make_font(&first_text_renderer, 9);
    missing_renderer = make_font(NULL, 9);

    TEST_CHECK(!render_text(NULL, "AB", &string));
    TEST_CHECK(!render_text(&font, NULL, &string));
    TEST_CHECK(!render_text(&font, "AB", NULL));
    TEST_CHECK(!render_text(&missing_renderer, "AB", &string));
    TEST_CHECK(string.text_renderer == NULL);
    TEST_CHECK(string.cache == NULL);
    TEST_CHECK(string.text == NULL);
    TEST_CHECK(cache_replace_calls == 0);
    TEST_CHECK(no_gpu_resource_calls());

    TEST_CHECK(disp_text(NULL, 0xFFFFFFFF, LEFT, DOWN, 0, 0, true)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(disp_text(&string, 0xFFFFFFFF, LEFT, DOWN, 0, 0, true)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(cached_draw_calls == 0);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

static int check_draw_text_propagates_cached_draw_failure(void)
{
    font_data font;
    string_tex_t string = {0};

    reset_fixture();
    first_text_renderer.frame_active = 1;
    font = make_font(&first_text_renderer, 9);

    fail_next_cached_draw = 1;
    TEST_CHECK(!draw_text(&font, 0x10203040, CENTER, CENTER,
                          50, 60, "AB", true, &string, false));
    TEST_CHECK(string.cache != NULL);
    TEST_CHECK(strcmp(string.text, "AB") == 0);
    TEST_CHECK(cached_draw_calls == 1);
    TEST_CHECK(ordering_barrier_attempts == 1);
    TEST_CHECK(no_gpu_resource_calls());

    fail_next_cached_draw = 1;
    TEST_CHECK(!draw_text(&font, 0x10203040, CENTER, CENTER,
                          50, 60, "AB", false, &string, false));
    TEST_CHECK(string.text_renderer == NULL);
    TEST_CHECK(string.cache == NULL);
    TEST_CHECK(string.text == NULL);
    TEST_CHECK(string.width == 0);
    TEST_CHECK(string.height == 0);
    TEST_CHECK(string.font_height == 0);
    TEST_CHECK(cached_draw_calls == 2);
    TEST_CHECK(ordering_barrier_attempts == 2);
    TEST_CHECK(no_gpu_resource_calls());
    return 0;
}

int main(void)
{
    if (check_render_is_cpu_only_and_empty_compatible() != 0)
        return 1;
    if (check_replace_is_atomic_and_cleanup_is_idempotent() != 0)
        return 1;
    if (check_hud_coordinates_and_alignments() != 0)
        return 1;
    if (check_world_coordinates_and_ordering_failure() != 0)
        return 1;
    if (check_invalid_arguments_leave_destinations_unchanged() != 0)
        return 1;
    if (check_draw_text_propagates_cached_draw_failure() != 0)
        return 1;
    return 0;
}
