#include "test_helpers.h"

#include "text_atlas.h"

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ASCII_FIRST 0x20
#define ASCII_LAST 0x7e
#define ASCII_COUNT (ASCII_LAST - ASCII_FIRST + 1)
#define MAX_TEXTURES 4

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    unsigned int identifier;
};

typedef struct FakeTexture {
    RendererTexture handle;
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    int destroy_count;
} FakeTexture;

typedef struct GlyphFixture {
    int calls[TEXT_GEOMETRY_GLYPH_COUNT];
    int load_count;
    int release_count;
    int release_errors;
    int fail_byte;
    int missing_byte;
    SDL_Surface *live_surfaces[TEXT_GEOMETRY_GLYPH_COUNT];
} GlyphFixture;

static Renderer fake_renderer;
static FakeTexture fake_textures[MAX_TEXTURES];
static int texture_create_attempts;
static int texture_create_successes;
static int texture_destroy_attempts;
static int texture_destroy_successes;
static int fail_texture_create;

static int glyph_width(unsigned char byte)
{
    return 1 + byte % 3;
}

static int glyph_height(unsigned char byte)
{
    return 2 + byte % 2;
}

static float glyph_bearing_x(unsigned char byte)
{
    return (float)((int)(byte % 5) - 2);
}

static float glyph_bearing_top(unsigned char byte)
{
    return (float)(4 + byte % 4);
}

static float glyph_advance(unsigned char byte)
{
    return (float)(glyph_width(byte) + 3);
}

static uint8_t expected_red(unsigned char byte, int x, int y)
{
    (void)x;
    (void)y;
    return byte;
}

static uint8_t expected_green(unsigned char byte, int x, int y)
{
    (void)byte;
    (void)y;
    return (uint8_t)(10 + x);
}

static uint8_t expected_blue(unsigned char byte, int x, int y)
{
    (void)byte;
    (void)x;
    return (uint8_t)(20 + y);
}

static uint8_t expected_alpha(unsigned char byte, int x, int y)
{
    return (uint8_t)(160 + (byte + x * 3 + y * 5) % 80);
}

static SDL_Surface *create_glyph_surface(unsigned char byte)
{
    SDL_Surface *surface;
    int x;
    int y;

    surface = SDL_CreateSurface(glyph_width(byte), glyph_height(byte),
                                SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL)
        return NULL;
    for (y = 0; y < surface->h; y++) {
        for (x = 0; x < surface->w; x++) {
            uint8_t *pixel = (uint8_t *)surface->pixels
                + (size_t)y * surface->pitch + (size_t)x * 4;

            pixel[0] = expected_red(byte, x, y);
            pixel[1] = expected_green(byte, x, y);
            pixel[2] = expected_blue(byte, x, y);
            pixel[3] = expected_alpha(byte, x, y);
        }
    }
    return surface;
}

static RendererStatus fixture_load_glyph(
    void *context, unsigned char byte, TextAtlasGlyphSurface *glyph)
{
    GlyphFixture *fixture = context;

    if (fixture == NULL || glyph == NULL
        || byte < ASCII_FIRST || byte > ASCII_LAST) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    fixture->calls[byte]++;
    fixture->load_count++;
    memset(glyph, 0, sizeof(*glyph));

    if (byte == fixture->fail_byte)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (byte == fixture->missing_byte)
        return RENDERER_STATUS_OK;

    glyph->available = 1;
    glyph->bearing_x = glyph_bearing_x(byte);
    glyph->bearing_top = glyph_bearing_top(byte);
    glyph->advance = byte == (unsigned char)' '
        ? 4.0f : glyph_advance(byte);
    if (byte == (unsigned char)' ')
        return RENDERER_STATUS_OK;

    glyph->surface = create_glyph_surface(byte);
    if (glyph->surface == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (fixture->live_surfaces[byte] != NULL) {
        SDL_DestroySurface(glyph->surface);
        glyph->surface = NULL;
        return RENDERER_STATUS_INVALID_STATE;
    }
    fixture->live_surfaces[byte] = glyph->surface;
    return RENDERER_STATUS_OK;
}

static void fixture_release_glyph(void *context,
                                  TextAtlasGlyphSurface *glyph)
{
    GlyphFixture *fixture = context;
    int byte;

    if (fixture == NULL || glyph == NULL || glyph->surface == NULL)
        return;
    for (byte = ASCII_FIRST; byte <= ASCII_LAST; byte++) {
        if (fixture->live_surfaces[byte] == glyph->surface) {
            fixture->live_surfaces[byte] = NULL;
            fixture->release_count++;
            SDL_DestroySurface(glyph->surface);
            glyph->surface = NULL;
            return;
        }
    }
    fixture->release_errors++;
}

static TextAtlasSource make_source(GlyphFixture *fixture)
{
    TextAtlasSource source;

    memset(&source, 0, sizeof(source));
    source.context = fixture;
    source.ascent = 11.0f;
    source.line_height = 16.0f;
    source.line_spacing = 19.0f;
    source.load_glyph = fixture_load_glyph;
    source.release_glyph = fixture_release_glyph;
    return source;
}

static int fixture_has_no_live_surfaces(const GlyphFixture *fixture)
{
    int byte;

    for (byte = 0; byte < TEXT_GEOMETRY_GLYPH_COUNT; byte++) {
        if (fixture->live_surfaces[byte] != NULL)
            return 0;
    }
    return 1;
}

static int fixture_calls_are_bounded(const GlyphFixture *fixture)
{
    int byte;

    for (byte = 0; byte < TEXT_GEOMETRY_GLYPH_COUNT; byte++) {
        int expected_max = byte >= ASCII_FIRST && byte <= ASCII_LAST ? 1 : 0;

        if (fixture->calls[byte] > expected_max)
            return 0;
    }
    return 1;
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

static int fake_textures_have_no_live_resources(void)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        if (fake_textures[index].destroy_count != 1)
            return 0;
    }
    return 1;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    FakeTexture *record;
    int y;

    texture_create_attempts++;
    if (texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *texture = NULL;
    if (renderer == NULL || desc == NULL || rgba_pixels == NULL
        || desc->width <= 0 || desc->height <= 0
        || pitch < (size_t)desc->width * 4) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (fail_texture_create)
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

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record;

    texture_destroy_attempts++;
    if (renderer == NULL || texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    record = fake_texture_from_handle(texture);
    if (record == NULL || record->destroy_count != 0)
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    record->destroy_count++;
    texture_destroy_successes++;
    return RENDERER_STATUS_OK;
}

static const uint8_t *fake_texture_pixel(const FakeTexture *texture,
                                         int x, int y)
{
    return texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
}

static int glyph_pixels_match(const FakeTexture *texture,
                              const TextGeometryGlyph *glyph,
                              unsigned char byte)
{
    int x;
    int y;

    for (y = 0; y < glyph->atlas_bounds.height; y++) {
        for (x = 0; x < glyph->atlas_bounds.width; x++) {
            const uint8_t *pixel = fake_texture_pixel(
                texture, glyph->atlas_bounds.x + x,
                glyph->atlas_bounds.y + y);

            if (pixel[0] != expected_red(byte, x, y)
                || pixel[1] != expected_green(byte, x, y)
                || pixel[2] != expected_blue(byte, x, y)
                || pixel[3] != expected_alpha(byte, x, y)) {
                return 0;
            }
        }
    }
    return 1;
}

static void reset_fake_renderer(void)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        free(fake_textures[index].pixels);
        fake_textures[index].pixels = NULL;
    }
    memset(fake_textures, 0, sizeof(fake_textures));
    memset(&fake_renderer, 0, sizeof(fake_renderer));
    texture_create_attempts = 0;
    texture_create_successes = 0;
    texture_destroy_attempts = 0;
    texture_destroy_successes = 0;
    fail_texture_create = 0;
}

static int check_ascii_prewarm_and_top_left_atlas(void)
{
    GlyphFixture fixture;
    TextAtlasSource source;
    TextAtlas *atlas = NULL;
    TextAtlas *retained_atlas;
    const TextGeometryFont *font;
    RendererTexture *texture;
    FakeTexture *record;
    int byte;
    int visible_count = 0;

    reset_fake_renderer();
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_byte = -1;
    fixture.missing_byte = (unsigned char)'~';
    source = make_source(&fixture);

    TEST_CHECK(Text_atlas_create(&fake_renderer, &source, &atlas)
               == RENDERER_STATUS_OK);
    TEST_CHECK(atlas != NULL);
    TEST_CHECK(fixture.load_count == ASCII_COUNT);
    for (byte = 0; byte < TEXT_GEOMETRY_GLYPH_COUNT; byte++) {
        int expected = byte >= ASCII_FIRST && byte <= ASCII_LAST ? 1 : 0;

        TEST_CHECK(fixture.calls[byte] == expected);
    }
    TEST_CHECK(fixture.release_errors == 0);
    TEST_CHECK(fixture_has_no_live_surfaces(&fixture));
    TEST_CHECK(texture_create_attempts == 1);
    TEST_CHECK(texture_create_successes == 1);

    font = Text_atlas_geometry_font(atlas);
    texture = Text_atlas_texture(atlas);
    record = fake_texture_from_handle(texture);
    TEST_CHECK(font != NULL);
    TEST_CHECK(Text_atlas_renderer(atlas) == &fake_renderer);
    TEST_CHECK(texture != NULL && record != NULL);
    TEST_CHECK(font->atlas_width == record->desc.width);
    TEST_CHECK(font->atlas_height == record->desc.height);
    TEST_CHECK(font->ascent == source.ascent);
    TEST_CHECK(font->line_height == source.line_height);
    TEST_CHECK(font->line_spacing == source.line_spacing);
    TEST_CHECK(font->fallback_byte == (unsigned char)'?');
    TEST_CHECK(record->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(record->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);

    for (byte = 0; byte < TEXT_GEOMETRY_GLYPH_COUNT; byte++) {
        const TextGeometryGlyph *glyph = &font->glyphs[byte];

        if (byte < ASCII_FIRST || byte > ASCII_LAST
            || byte == (unsigned char)'~') {
            TEST_CHECK(!glyph->available);
            continue;
        }
        TEST_CHECK(glyph->available);
        if (byte == (unsigned char)' ') {
            TEST_CHECK(glyph->atlas_bounds.width == 0);
            TEST_CHECK(glyph->atlas_bounds.height == 0);
            TEST_CHECK(glyph->advance == 4.0f);
            continue;
        }
        visible_count++;
        TEST_CHECK(glyph->atlas_bounds.width == glyph_width(byte));
        TEST_CHECK(glyph->atlas_bounds.height == glyph_height(byte));
        TEST_CHECK(glyph->atlas_bounds.x >= 0);
        TEST_CHECK(glyph->atlas_bounds.y >= 0);
        TEST_CHECK(glyph->atlas_bounds.x + glyph->atlas_bounds.width
                   <= font->atlas_width);
        TEST_CHECK(glyph->atlas_bounds.y + glyph->atlas_bounds.height
                   <= font->atlas_height);
        TEST_CHECK(glyph->bearing_x == glyph_bearing_x(byte));
        TEST_CHECK(glyph->bearing_top == glyph_bearing_top(byte));
        TEST_CHECK(glyph->advance == glyph_advance(byte));
        TEST_CHECK(glyph_pixels_match(record, glyph, (unsigned char)byte));
    }
    TEST_CHECK(fixture.release_count == visible_count);

    retained_atlas = atlas;
    fake_renderer.frame_active = 1;
    TEST_CHECK(Text_atlas_destroy(&atlas) == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(atlas == retained_atlas);
    TEST_CHECK(Text_atlas_texture(atlas) == texture);
    TEST_CHECK(texture_destroy_attempts == 1);
    TEST_CHECK(texture_destroy_successes == 0);

    fake_renderer.frame_active = 0;
    TEST_CHECK(Text_atlas_destroy(&atlas) == RENDERER_STATUS_OK);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(record->destroy_count == 1);
    TEST_CHECK(texture_destroy_attempts == 2);
    TEST_CHECK(texture_destroy_successes == 1);
    TEST_CHECK(Text_atlas_destroy(&atlas) == RENDERER_STATUS_OK);
    TEST_CHECK(texture_destroy_attempts == 2);
    return 0;
}

static int check_raster_failure_rolls_back(void)
{
    GlyphFixture fixture;
    TextAtlasSource source;
    TextAtlas *atlas = NULL;
    int byte;

    reset_fake_renderer();
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_byte = (unsigned char)'G';
    fixture.missing_byte = -1;
    source = make_source(&fixture);

    TEST_CHECK(Text_atlas_create(&fake_renderer, &source, &atlas)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(fixture.calls[(unsigned char)'G'] == 1);
    TEST_CHECK(fixture_calls_are_bounded(&fixture));
    TEST_CHECK(fixture.release_errors == 0);
    TEST_CHECK(fixture_has_no_live_surfaces(&fixture));
    TEST_CHECK(texture_destroy_successes == texture_create_successes);
    TEST_CHECK(fake_textures_have_no_live_resources());
    for (byte = 0; byte < TEXT_GEOMETRY_GLYPH_COUNT; byte++)
        TEST_CHECK(fixture.calls[byte] <= 1);
    return 0;
}

static int check_missing_fallback_is_not_published(void)
{
    GlyphFixture fixture;
    TextAtlasSource source;
    TextAtlas *atlas = NULL;
    RendererStatus status;

    reset_fake_renderer();
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_byte = -1;
    fixture.missing_byte = (unsigned char)'?';
    source = make_source(&fixture);

    status = Text_atlas_create(&fake_renderer, &source, &atlas);
    TEST_CHECK(status != RENDERER_STATUS_OK);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(fixture.calls[(unsigned char)'?'] == 1);
    TEST_CHECK(fixture_calls_are_bounded(&fixture));
    TEST_CHECK(fixture.release_errors == 0);
    TEST_CHECK(fixture_has_no_live_surfaces(&fixture));
    TEST_CHECK(texture_destroy_successes == texture_create_successes);
    TEST_CHECK(fake_textures_have_no_live_resources());
    return 0;
}

static int check_texture_failure_rolls_back(void)
{
    GlyphFixture fixture;
    TextAtlasSource source;
    TextAtlas *atlas = NULL;

    reset_fake_renderer();
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_byte = -1;
    fixture.missing_byte = (unsigned char)'~';
    source = make_source(&fixture);
    fail_texture_create = 1;

    TEST_CHECK(Text_atlas_create(&fake_renderer, &source, &atlas)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(fixture.load_count == ASCII_COUNT);
    TEST_CHECK(fixture_calls_are_bounded(&fixture));
    TEST_CHECK(fixture.release_errors == 0);
    TEST_CHECK(fixture_has_no_live_surfaces(&fixture));
    TEST_CHECK(texture_create_attempts == 1);
    TEST_CHECK(texture_create_successes == 0);
    TEST_CHECK(texture_destroy_attempts == 0);
    return 0;
}

static int check_active_frame_creation_is_rejected(void)
{
    GlyphFixture fixture;
    TextAtlasSource source;
    TextAtlas *atlas = NULL;

    reset_fake_renderer();
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_byte = -1;
    fixture.missing_byte = (unsigned char)'~';
    source = make_source(&fixture);
    fake_renderer.frame_active = 1;

    TEST_CHECK(Text_atlas_create(&fake_renderer, &source, &atlas)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(atlas == NULL);
    TEST_CHECK(fixture_calls_are_bounded(&fixture));
    TEST_CHECK(fixture.release_errors == 0);
    TEST_CHECK(fixture_has_no_live_surfaces(&fixture));
    TEST_CHECK(texture_create_successes == 0);
    fake_renderer.frame_active = 0;
    return 0;
}

int main(void)
{
    int result = 1;

    if (!SDL_Init(0))
        return 1;
    if (check_ascii_prewarm_and_top_left_atlas() != 0)
        goto cleanup;
    if (check_raster_failure_rolls_back() != 0)
        goto cleanup;
    if (check_missing_fallback_is_not_published() != 0)
        goto cleanup;
    if (check_texture_failure_rolls_back() != 0)
        goto cleanup;
    if (check_active_frame_creation_is_rejected() != 0)
        goto cleanup;
    result = 0;

cleanup:
    reset_fake_renderer();
    SDL_Quit();
    return result;
}
