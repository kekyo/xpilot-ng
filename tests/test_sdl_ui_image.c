#include "test_helpers.h"

#include "sdlrenderer.h"
#include "sdluiimage.h"

#include <SDL.h>
#include <GL/gl.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURES 16
#define MAX_EVENTS 16
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
static int texture_destroy_attempts;
static int texture_destroy_successes;
static int texture_update_calls;
static int resource_calls_during_frame;
static int blend_calls;
static int sprite_calls;
static int preserving_flush_calls;
static int legacy_texture_calls;
static char events[MAX_EVENTS];
static int event_count;

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

static void record_event(char event)
{
    if (event_count < MAX_EVENTS)
        events[event_count++] = event;
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

static const uint8_t *fake_pixel(const FakeTexture *texture, int x, int y)
{
    return texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
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
    pixel = fake_pixel(texture, x, y);
    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
}

static SDL_Surface *create_fixture_surface(int width, int height,
                                           uint8_t base)
{
    SDL_Surface *surface;
    int x;
    int y;

    surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL)
        return NULL;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint8_t *pixel = (uint8_t *)surface->pixels
                + (size_t)y * surface->pitch + (size_t)x * 4;
            uint8_t offset = (uint8_t)(y * 20 + x * 4);

            pixel[0] = (uint8_t)(base + offset);
            pixel[1] = (uint8_t)(base + offset + 1);
            pixel[2] = (uint8_t)(base + offset + 2);
            pixel[3] = (uint8_t)(base + offset + 3);
        }
    }
    return surface;
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
    (void)renderer;
    (void)texture;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    texture_update_calls++;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    texture_destroy_attempts++;
    if (renderer == NULL || record == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    record->destroy_count++;
    if (record->destroy_count != 1)
        return RENDERER_STATUS_INVALID_STATE;
    texture_destroy_successes++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    blend_calls++;
    record_event('B');
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
    record_event('S');
    return RENDERER_STATUS_OK;
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
    record_event('F');
    return RENDERER_STATUS_OK;
}

void GLAPIENTRY glGenTextures(GLsizei count, GLuint *textures)
{
    (void)count;
    (void)textures;
    legacy_texture_calls++;
}

void GLAPIENTRY glDeleteTextures(GLsizei count, const GLuint *textures)
{
    (void)count;
    (void)textures;
    legacy_texture_calls++;
}

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    (void)texture;
    legacy_texture_calls++;
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
    legacy_texture_calls++;
}

void GLAPIENTRY glTexParameterf(GLenum target, GLenum name, GLfloat value)
{
    (void)target;
    (void)name;
    (void)value;
    legacy_texture_calls++;
}

static int check_atomic_single_image(SDL_Surface *surface,
                                     SdlUiImage *image)
{
    FakeTexture *texture;

    memset(image, 0, sizeof(*image));
    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(Sdl_ui_image_init(image, &fake_renderer, surface)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(image->renderer == NULL);
    TEST_CHECK(image->texture == NULL);
    TEST_CHECK(image->content_width == 0 && image->content_height == 0);
    TEST_CHECK(image->texture_width == 0 && image->texture_height == 0);
    TEST_CHECK(texture_create_successes == 0);

    fail_texture_create_attempt = 0;
    TEST_CHECK(Sdl_ui_image_init(image, &fake_renderer, surface)
               == RENDERER_STATUS_OK);
    TEST_CHECK(image->renderer == &fake_renderer);
    TEST_CHECK(image->texture != NULL);
    TEST_CHECK(image->content_width == 3 && image->content_height == 2);
    TEST_CHECK(image->texture_width == 4 && image->texture_height == 2);
    texture = fake_texture_from_handle(image->texture);
    TEST_CHECK(texture != NULL);
    TEST_CHECK(texture->desc.width == 4 && texture->desc.height == 2);
    TEST_CHECK(texture->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(texture->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(pixel_equals(texture, 0, 0, 10, 11, 12, 13));
    TEST_CHECK(pixel_equals(texture, 2, 1, 38, 39, 40, 41));
    TEST_CHECK(pixel_equals(texture, 3, 0, 0, 0, 0, 0));
    TEST_CHECK(pixel_equals(texture, 3, 1, 0, 0, 0, 0));

    /* The source stays caller-owned and usable after initialization. */
    TEST_CHECK(((uint8_t *)surface->pixels)[0] == 10);
    return 0;
}

static int check_semantic_draw(const SdlUiImage *image)
{
    const RendererRect bounds = {11, 13, 9, 5};
    const RendererColor tint = {201, 202, 203, 204};

    fake_renderer.frame_active = 1;
    event_count = 0;
    TEST_CHECK(Sdl_ui_image_draw(&fake_sdl_renderer, image, &bounds, tint)
               == RENDERER_STATUS_OK);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(sprite_calls == 1);
    TEST_CHECK(preserving_flush_calls == 1);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == 'B' && events[1] == 'S' && events[2] == 'F');
    TEST_CHECK(last_sprite.texture == image->texture);
    TEST_CHECK(float_equal(last_sprite.left, 11.0f));
    TEST_CHECK(float_equal(last_sprite.top, 13.0f));
    TEST_CHECK(float_equal(last_sprite.right, 20.0f));
    TEST_CHECK(float_equal(last_sprite.bottom, 18.0f));
    TEST_CHECK(float_equal(last_sprite.u0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.v0, 0.0f));
    TEST_CHECK(float_equal(last_sprite.u1, 3.0f / 4.0f));
    TEST_CHECK(float_equal(last_sprite.v1, 1.0f));
    TEST_CHECK(color_equal(last_sprite.tint, tint));
    TEST_CHECK(legacy_texture_calls == 0);
    fake_renderer.frame_active = 0;
    return 0;
}

static int check_cleanup_retry(SdlUiImage *image)
{
    Renderer *owner = image->renderer;
    RendererTexture *texture = image->texture;
    int successful_before = texture_destroy_successes;

    fake_renderer.frame_active = 1;
    TEST_CHECK(Sdl_ui_image_cleanup(image) == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(image->renderer == owner);
    TEST_CHECK(image->texture == texture);
    TEST_CHECK(image->content_width == 3 && image->content_height == 2);
    TEST_CHECK(texture_destroy_successes == successful_before);

    fake_renderer.frame_active = 0;
    TEST_CHECK(Sdl_ui_image_cleanup(image) == RENDERER_STATUS_OK);
    TEST_CHECK(texture_destroy_successes == successful_before + 1);
    TEST_CHECK(fake_texture_from_handle(texture)->destroy_count == 1);
    TEST_CHECK(image->renderer == NULL && image->texture == NULL);
    TEST_CHECK(image->content_width == 0 && image->content_height == 0);
    TEST_CHECK(image->texture_width == 0 && image->texture_height == 0);

    TEST_CHECK(Sdl_ui_image_cleanup(image) == RENDERER_STATUS_OK);
    TEST_CHECK(texture_destroy_successes == successful_before + 1);
    return 0;
}

static int check_pair_rollback_and_cleanup(SDL_Surface *first_surface,
                                           SDL_Surface *second_surface)
{
    SdlUiImagePair pair;
    RendererTexture *first_texture;
    RendererTexture *second_texture;
    int successes_before = texture_create_successes;
    int destroys_before = texture_destroy_successes;

    memset(&pair, 0, sizeof(pair));
    fail_texture_create_attempt = texture_create_attempts + 2;
    TEST_CHECK(Sdl_ui_image_pair_init(
                   &pair, &fake_renderer, first_surface, second_surface)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(texture_create_successes == successes_before + 1);
    TEST_CHECK(texture_destroy_successes == destroys_before + 1);
    TEST_CHECK(pair.first.renderer == NULL && pair.first.texture == NULL);
    TEST_CHECK(pair.second.renderer == NULL && pair.second.texture == NULL);

    fail_texture_create_attempt = 0;
    TEST_CHECK(Sdl_ui_image_pair_init(
                   &pair, &fake_renderer, first_surface, second_surface)
               == RENDERER_STATUS_OK);
    TEST_CHECK(pair.first.renderer == &fake_renderer);
    TEST_CHECK(pair.second.renderer == &fake_renderer);
    TEST_CHECK(pair.first.content_width == 2
               && pair.first.content_height == 3);
    TEST_CHECK(pair.first.texture_width == 2
               && pair.first.texture_height == 4);
    TEST_CHECK(pair.second.content_width == 5
               && pair.second.content_height == 1);
    TEST_CHECK(pair.second.texture_width == 8
               && pair.second.texture_height == 1);
    first_texture = pair.first.texture;
    second_texture = pair.second.texture;

    fake_renderer.frame_active = 1;
    destroys_before = texture_destroy_successes;
    TEST_CHECK(Sdl_ui_image_pair_cleanup(&pair)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(texture_destroy_successes == destroys_before);
    TEST_CHECK(pair.first.texture == first_texture);
    TEST_CHECK(pair.second.texture == second_texture);

    fake_renderer.frame_active = 0;
    TEST_CHECK(Sdl_ui_image_pair_cleanup(&pair) == RENDERER_STATUS_OK);
    TEST_CHECK(texture_destroy_successes == destroys_before + 2);
    TEST_CHECK(fake_texture_from_handle(first_texture)->destroy_count == 1);
    TEST_CHECK(fake_texture_from_handle(second_texture)->destroy_count == 1);
    TEST_CHECK(pair.first.renderer == NULL && pair.first.texture == NULL);
    TEST_CHECK(pair.second.renderer == NULL && pair.second.texture == NULL);

    TEST_CHECK(Sdl_ui_image_pair_cleanup(&pair) == RENDERER_STATUS_OK);
    TEST_CHECK(texture_destroy_successes == destroys_before + 2);
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
    SDL_Surface *single_surface = NULL;
    SDL_Surface *first_surface = NULL;
    SDL_Surface *second_surface = NULL;
    SdlUiImage image;
    int result = 1;

    memset(&image, 0, sizeof(image));
    single_surface = create_fixture_surface(3, 2, 10);
    first_surface = create_fixture_surface(2, 3, 50);
    second_surface = create_fixture_surface(5, 1, 90);
    if (single_surface == NULL || first_surface == NULL
        || second_surface == NULL) {
        goto cleanup;
    }
    if (check_atomic_single_image(single_surface, &image) != 0)
        goto cleanup;
    if (check_semantic_draw(&image) != 0)
        goto cleanup;
    if (check_cleanup_retry(&image) != 0)
        goto cleanup;
    if (check_pair_rollback_and_cleanup(first_surface, second_surface) != 0)
        goto cleanup;
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(legacy_texture_calls == 0);
    result = 0;

cleanup:
    fake_renderer.frame_active = 0;
    if (image.texture != NULL)
        Sdl_ui_image_cleanup(&image);
    SDL_FreeSurface(single_surface);
    SDL_FreeSurface(first_surface);
    SDL_FreeSurface(second_surface);
    release_fake_pixels();
    return result;
}
