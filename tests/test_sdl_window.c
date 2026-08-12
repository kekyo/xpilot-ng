#include "test_helpers.h"

#include <stdbool.h>

#include "sdlinit.h"
#include "sdlrenderer.h"
#include "sdlwindow.h"

#include <SDL.h>
#include <GL/gl.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURES 16
#define FLOAT_TOLERANCE 0.0001f

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    unsigned int identifier;
};

struct SdlRenderer {
    Renderer *frontend;
};

typedef struct FakeTexture {
    RendererTexture handle;
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    int destroy_count;
} FakeTexture;

typedef struct FakeSprite {
    RendererTexture *texture;
    float left;
    float top;
    float right;
    float bottom;
    float u0;
    float v0;
    float u1;
    float v1;
    RendererColor tint;
} FakeSprite;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {&fake_renderer};
static FakeTexture fake_textures[MAX_TEXTURES];
static FakeSprite last_sprite;
static int texture_create_attempts;
static int texture_create_successes;
static int fail_texture_create_attempt;
static int texture_update_calls;
static int texture_destroy_calls;
static int resource_calls_during_frame;
static int blend_calls;
static int sprite_calls;
static int preserving_flush_calls;
static int legacy_calls;

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

static FakeTexture *fake_texture_from_handle(RendererTexture *texture)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        if (&fake_textures[index].handle == texture)
            return &fake_textures[index];
    }
    return NULL;
}

static int pixel_equals(const FakeTexture *texture, int x, int y,
                        uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t alpha)
{
    const uint8_t *pixel;

    if (texture == NULL || x < 0 || x >= texture->desc.width
        || y < 0 || y >= texture->desc.height) {
        return 0;
    }
    pixel = texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
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
        || texture == NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (texture_create_attempts == fail_texture_create_attempt)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (texture_create_successes >= MAX_TEXTURES)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    record = &fake_textures[texture_create_successes];
    memset(record, 0, sizeof(*record));
    record->handle.identifier = (unsigned int)texture_create_successes + 1;
    record->desc = *desc;
    record->pitch = (size_t)desc->width * 4;
    record->pixels = malloc(record->pitch * (size_t)desc->height);
    if (record->pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    for (y = 0; y < desc->height; y++) {
        memcpy(record->pixels + (size_t)y * record->pitch,
               rgba_pixels + (size_t)y * pitch, record->pitch);
    }
    texture_create_successes++;
    *texture = &record->handle;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *rgba_pixels,
                                       size_t pitch)
{
    (void)texture;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    texture_update_calls++;
    if (renderer != NULL && renderer->frame_active)
        resource_calls_during_frame++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    if (renderer == NULL || record == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    record->destroy_count++;
    texture_destroy_calls++;
    return record->destroy_count == 1 ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_STATE;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    blend_calls++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    if (renderer != &fake_renderer || !renderer->frame_active
        || record == NULL || record->destroy_count != 0) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    last_sprite.texture = texture;
    last_sprite.left = left;
    last_sprite.top = top;
    last_sprite.right = right;
    last_sprite.bottom = bottom;
    last_sprite.u0 = u0;
    last_sprite.v0 = v0;
    last_sprite.u1 = u1;
    last_sprite.v1 = v1;
    last_sprite.tint = tint;
    sprite_calls++;
    return RENDERER_STATUS_OK;
}

SdlRenderer *Get_sdl_renderer(void)
{
    return &fake_sdl_renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer == &fake_sdl_renderer ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
{
    if (renderer != &fake_sdl_renderer || !fake_renderer.frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    preserving_flush_calls++;
    return RENDERER_STATUS_OK;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

void GLAPIENTRY glGenTextures(GLsizei count, GLuint *textures)
{
    GLsizei index;

    legacy_calls++;
    for (index = 0; index < count; index++)
        textures[index] = (GLuint)(index + 1);
}

void GLAPIENTRY glDeleteTextures(GLsizei count, const GLuint *textures)
{
    (void)count;
    (void)textures;
    legacy_calls++;
}

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    (void)texture;
    legacy_calls++;
}

void GLAPIENTRY glTexImage2D(GLenum target, GLint level,
                            GLint internal_format, GLsizei width,
                            GLsizei height, GLint border, GLenum format,
                            GLenum type, const GLvoid *pixels)
{
    (void)target;
    (void)level;
    (void)internal_format;
    (void)width;
    (void)height;
    (void)border;
    (void)format;
    (void)type;
    (void)pixels;
    legacy_calls++;
}

void GLAPIENTRY glTexParameterf(GLenum target, GLenum name, GLfloat value)
{
    (void)target;
    (void)name;
    (void)value;
    legacy_calls++;
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

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green,
                          GLubyte blue, GLubyte alpha)
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

void GLAPIENTRY glTexCoord2f(GLfloat u, GLfloat v)
{
    (void)u;
    (void)v;
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

static int check_initial_prepare(sdl_window_t *window,
                                 RendererTexture **first_texture)
{
    SDL_Surface *surface_before_failure;
    Uint32 background;
    Uint32 marker;
    SDL_Rect marker_rect = {2, 1, 1, 1};
    FakeTexture *texture;

    memset(window, 0, sizeof(*window));
    TEST_CHECK(sdl_window_init(NULL, 1, 2, 3, 4) == -1);
    TEST_CHECK(sdl_window_init(window, 1, 2, 0, 4) == -1);
    TEST_CHECK(window->surface == NULL);
    TEST_CHECK(window->x == 0 && window->y == 0);
    TEST_CHECK(window->w == 0 && window->h == 0);

    TEST_CHECK(sdl_window_init(window, 7, 9, 5, 3) == 0);
    TEST_CHECK(window->surface != NULL);
    TEST_CHECK(window->surface->w == 8 && window->surface->h == 4);
    TEST_CHECK(window->x == 7 && window->y == 9);
    TEST_CHECK(window->w == 5 && window->h == 3);
    TEST_CHECK(texture_create_attempts == 0);
    TEST_CHECK(texture_destroy_calls == 0);

    background = SDL_MapRGBA(window->surface->format, 10, 20, 30, 40);
    marker = SDL_MapRGBA(window->surface->format, 70, 80, 90, 100);
    TEST_CHECK(SDL_FillRect(window->surface, NULL, background) == 0);
    TEST_CHECK(SDL_FillRect(window->surface, &marker_rect, marker) == 0);
    sdl_window_refresh(window);

    surface_before_failure = window->surface;
    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == -1);
    TEST_CHECK(window->surface == surface_before_failure);
    TEST_CHECK(window->x == 7 && window->y == 9);
    TEST_CHECK(window->w == 5 && window->h == 3);
    TEST_CHECK(texture_create_successes == 0);
    TEST_CHECK(texture_destroy_calls == 0);

    fail_texture_create_attempt = 0;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 1);
    TEST_CHECK(texture_destroy_calls == 0);
    texture = &fake_textures[0];
    TEST_CHECK(texture->desc.width == 8 && texture->desc.height == 4);
    TEST_CHECK(texture->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(texture->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(pixel_equals(texture, 0, 0, 10, 20, 30, 40));
    TEST_CHECK(pixel_equals(texture, 2, 1, 70, 80, 90, 100));
    *first_texture = &texture->handle;

    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 1);
    TEST_CHECK(texture_destroy_calls == 0);
    return 0;
}

static int check_dirty_refresh_and_semantic_paint(
    sdl_window_t *window, RendererTexture *old_texture,
    RendererTexture **refreshed_texture)
{
    SDL_Rect changed_rect = {4, 2, 1, 1};
    Uint32 changed;
    int attempts_before;
    int destroys_before;
    int legacy_before;
    FakeTexture *texture;
    const RendererColor white = {255, 255, 255, 255};

    changed = SDL_MapRGBA(window->surface->format, 101, 102, 103, 104);
    TEST_CHECK(SDL_FillRect(window->surface, &changed_rect, changed) == 0);
    attempts_before = texture_create_attempts;
    destroys_before = texture_destroy_calls;
    fake_renderer.frame_active = 1;
    sdl_window_refresh(window);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);

    legacy_before = legacy_calls;
    sdl_window_paint(window);
    TEST_CHECK(sprite_calls == 1);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(preserving_flush_calls == 1);
    TEST_CHECK(last_sprite.texture == old_texture);
    TEST_CHECK(float_equal(last_sprite.left, 7.0f));
    TEST_CHECK(float_equal(last_sprite.top, 9.0f));
    TEST_CHECK(float_equal(last_sprite.right, 12.0f));
    TEST_CHECK(float_equal(last_sprite.bottom, 12.0f));
    TEST_CHECK(float_equal(last_sprite.u0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.v0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.u1, 5.0f / 8.0f));
    TEST_CHECK(float_equal(last_sprite.v1, 3.0f / 4.0f));
    TEST_CHECK(color_equal(last_sprite.tint, white));
    TEST_CHECK(legacy_calls == legacy_before);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);
    fake_renderer.frame_active = 0;

    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_successes == 2);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_textures[0].destroy_count == 1);
    texture = &fake_textures[1];
    TEST_CHECK(pixel_equals(texture, 4, 2, 101, 102, 103, 104));
    *refreshed_texture = &texture->handle;

    attempts_before = texture_create_attempts;
    TEST_CHECK(sdl_window_prepare(window, &fake_renderer) == 0);
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    return 0;
}

static int check_atomic_resize_and_retry(sdl_window_t *window,
                                         RendererTexture *old_texture,
                                         RendererTexture **resized_texture)
{
    SDL_Surface *old_surface = window->surface;
    int old_x = window->x;
    int old_y = window->y;
    int old_w = window->w;
    int old_h = window->h;
    int successes_before = texture_create_successes;
    int destroys_before = texture_destroy_calls;
    FakeTexture *texture;

    TEST_CHECK(sdl_window_resize(window, 0, 5) == -1);
    TEST_CHECK(window->surface == old_surface);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == old_w && window->h == old_h);
    TEST_CHECK(texture_create_successes == successes_before);
    TEST_CHECK(texture_destroy_calls == destroys_before);

    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(sdl_window_resize(window, 7, 5) == -1);
    TEST_CHECK(window->surface == old_surface);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == old_w && window->h == old_h);
    TEST_CHECK(texture_create_successes == successes_before);
    TEST_CHECK(texture_destroy_calls == destroys_before);
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 0);

    fail_texture_create_attempt = 0;
    TEST_CHECK(sdl_window_resize(window, 7, 5) == 0);
    TEST_CHECK(window->surface != old_surface);
    TEST_CHECK(window->surface->w == 8 && window->surface->h == 8);
    TEST_CHECK(window->x == old_x && window->y == old_y);
    TEST_CHECK(window->w == 7 && window->h == 5);
    TEST_CHECK(texture_create_successes == successes_before + 1);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 1);
    texture = &fake_textures[texture_create_successes - 1];
    TEST_CHECK(texture->desc.width == 8 && texture->desc.height == 8);
    TEST_CHECK(texture->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(texture->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(pixel_equals(texture, 0, 0, 10, 20, 30, 40));
    TEST_CHECK(pixel_equals(texture, 2, 1, 70, 80, 90, 100));
    TEST_CHECK(pixel_equals(texture, 4, 2, 101, 102, 103, 104));
    *resized_texture = &texture->handle;
    return 0;
}

static int check_move_paint_and_exact_cleanup(sdl_window_t *window,
                                              RendererTexture *texture)
{
    int destroys_before;
    int legacy_before;

    sdl_window_move(window, 20, 30);
    fake_renderer.frame_active = 1;
    legacy_before = legacy_calls;
    sdl_window_paint(window);
    TEST_CHECK(sprite_calls == 2);
    TEST_CHECK(preserving_flush_calls == 2);
    TEST_CHECK(last_sprite.texture == texture);
    TEST_CHECK(float_equal(last_sprite.left, 20.0f));
    TEST_CHECK(float_equal(last_sprite.top, 30.0f));
    TEST_CHECK(float_equal(last_sprite.right, 27.0f));
    TEST_CHECK(float_equal(last_sprite.bottom, 35.0f));
    TEST_CHECK(float_equal(last_sprite.u1, 7.0f / 8.0f));
    TEST_CHECK(float_equal(last_sprite.v1, 5.0f / 8.0f));
    TEST_CHECK(legacy_calls == legacy_before);
    fake_renderer.frame_active = 0;

    destroys_before = texture_destroy_calls;
    sdl_window_destroy(window);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(fake_texture_from_handle(texture)->destroy_count == 1);
    TEST_CHECK(window->surface == NULL);
    TEST_CHECK(window->x == 0 && window->y == 0);
    TEST_CHECK(window->w == 0 && window->h == 0);

    sdl_window_destroy(window);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(resource_calls_during_frame == 0);
    TEST_CHECK(texture_update_calls == 0);
    return 0;
}

static void release_fake_pixels(void)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        free(fake_textures[index].pixels);
        fake_textures[index].pixels = NULL;
    }
}

int main(void)
{
    sdl_window_t window;
    RendererTexture *initial_texture;
    RendererTexture *refreshed_texture;
    RendererTexture *resized_texture;
    int result = 1;

    if (check_initial_prepare(&window, &initial_texture) != 0)
        goto cleanup;
    if (check_dirty_refresh_and_semantic_paint(
            &window, initial_texture, &refreshed_texture) != 0) {
        goto cleanup;
    }
    if (check_atomic_resize_and_retry(
            &window, refreshed_texture, &resized_texture) != 0) {
        goto cleanup;
    }
    if (check_move_paint_and_exact_cleanup(
            &window, resized_texture) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (result != 0 && window.surface != NULL) {
        fake_renderer.frame_active = 0;
        sdl_window_destroy(&window);
    }
    release_fake_pixels();
    return result;
}
