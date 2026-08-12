/* Backend-neutral ownership and packing for a byte-font glyph atlas. */

#include "text_atlas.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_ATLAS_ASCII_FIRST 0x20
#define TEXT_ATLAS_ASCII_LAST 0x7e
#define TEXT_ATLAS_COLUMNS 16
#define TEXT_ATLAS_GUTTER 1

struct TextAtlas {
    Renderer *renderer;
    RendererTexture *texture;
    TextGeometryFont geometry;
};

static int Power_of_two_ceil(int value, int *result)
{
    int power = 1;

    if (value <= 0 || result == NULL)
        return 0;
    while (power < value) {
        if (power > INT_MAX / 2)
            return 0;
        power *= 2;
    }
    *result = power;
    return 1;
}

static int Source_is_valid(const TextAtlasSource *source)
{
    return source != NULL
        && source->load_glyph != NULL
        && source->release_glyph != NULL
        && isfinite(source->ascent) && source->ascent >= 0.0f
        && isfinite(source->line_height) && source->line_height > 0.0f
        && isfinite(source->line_spacing) && source->line_spacing > 0.0f;
}

static int Glyph_is_valid(const TextAtlasGlyphSurface *glyph)
{
    if (glyph == NULL)
        return 0;
    if (!glyph->available)
        return glyph->surface == NULL;
    if (!isfinite(glyph->bearing_x) || !isfinite(glyph->bearing_top)
        || !isfinite(glyph->advance) || glyph->advance < 0.0f) {
        return 0;
    }
    return glyph->surface == NULL
        || (glyph->surface->w > 0 && glyph->surface->h > 0);
}

static void Release_glyphs(const TextAtlasSource *source,
                           TextAtlasGlyphSurface *glyphs)
{
    int byte;

    if (source == NULL || glyphs == NULL)
        return;
    for (byte = TEXT_ATLAS_ASCII_FIRST;
         byte <= TEXT_ATLAS_ASCII_LAST; byte++) {
        if (glyphs[byte].surface != NULL)
            source->release_glyph(source->context, &glyphs[byte]);
    }
}

static RendererStatus Copy_surface_rgba8(SDL_Surface *source,
                                         uint8_t *pixels, size_t pitch,
                                         int destination_x,
                                         int destination_y)
{
    SDL_Surface *converted;
    int locked = 0;
    int y;

    converted = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
    if (converted == NULL)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (converted->w != source->w || converted->h != source->h
        || converted->pitch < converted->w * 4) {
        SDL_FreeSurface(converted);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (SDL_MUSTLOCK(converted)) {
        if (SDL_LockSurface(converted) < 0) {
            SDL_FreeSurface(converted);
            return RENDERER_STATUS_BACKEND_ERROR;
        }
        locked = 1;
    }
    for (y = 0; y < converted->h; y++) {
        memcpy(pixels + (size_t)(destination_y + y) * pitch
                   + (size_t)destination_x * 4,
               (const uint8_t *)converted->pixels
                   + (size_t)y * converted->pitch,
               (size_t)converted->w * 4);
    }
    if (locked)
        SDL_UnlockSurface(converted);
    SDL_FreeSurface(converted);
    return RENDERER_STATUS_OK;
}

RendererStatus Text_atlas_create(Renderer *renderer,
                                 const TextAtlasSource *source,
                                 TextAtlas **atlas)
{
    TextAtlasGlyphSurface glyphs[TEXT_GEOMETRY_GLYPH_COUNT];
    TextAtlas *candidate = NULL;
    RendererTextureDesc desc;
    RendererStatus status = RENDERER_STATUS_OK;
    uint8_t *pixels = NULL;
    size_t pitch;
    size_t allocation_size;
    int maximum_width = 0;
    int maximum_height = 0;
    int cell_width;
    int cell_height;
    int grid_width;
    int grid_height;
    int atlas_width;
    int atlas_height;
    int row_count;
    int byte;

    if (atlas == NULL || *atlas != NULL || renderer == NULL
        || !Source_is_valid(source)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    memset(glyphs, 0, sizeof(glyphs));

    for (byte = TEXT_ATLAS_ASCII_FIRST;
         byte <= TEXT_ATLAS_ASCII_LAST; byte++) {
        status = source->load_glyph(source->context, (unsigned char)byte,
                                    &glyphs[byte]);
        if (status != RENDERER_STATUS_OK || !Glyph_is_valid(&glyphs[byte])) {
            if (status == RENDERER_STATUS_OK)
                status = RENDERER_STATUS_INVALID_ARGUMENT;
            goto failure;
        }
        if (glyphs[byte].surface != NULL) {
            if (glyphs[byte].surface->w > maximum_width)
                maximum_width = glyphs[byte].surface->w;
            if (glyphs[byte].surface->h > maximum_height)
                maximum_height = glyphs[byte].surface->h;
        }
    }
    if (!glyphs[(unsigned char)'?'].available) {
        status = RENDERER_STATUS_INVALID_ARGUMENT;
        goto failure;
    }
    if (maximum_width > INT_MAX - 2 * TEXT_ATLAS_GUTTER
        || maximum_height > INT_MAX - 2 * TEXT_ATLAS_GUTTER) {
        status = RENDERER_STATUS_OUT_OF_MEMORY;
        goto failure;
    }
    cell_width = maximum_width + 2 * TEXT_ATLAS_GUTTER;
    cell_height = maximum_height + 2 * TEXT_ATLAS_GUTTER;
    row_count = (TEXT_ATLAS_ASCII_LAST - TEXT_ATLAS_ASCII_FIRST + 1
                 + TEXT_ATLAS_COLUMNS - 1) / TEXT_ATLAS_COLUMNS;
    if (cell_width > INT_MAX / TEXT_ATLAS_COLUMNS
        || cell_height > INT_MAX / row_count) {
        status = RENDERER_STATUS_OUT_OF_MEMORY;
        goto failure;
    }
    grid_width = cell_width * TEXT_ATLAS_COLUMNS;
    grid_height = cell_height * row_count;
    if (!Power_of_two_ceil(grid_width, &atlas_width)
        || !Power_of_two_ceil(grid_height, &atlas_height)
        || atlas_width > INT_MAX / 4) {
        status = RENDERER_STATUS_OUT_OF_MEMORY;
        goto failure;
    }
    pitch = (size_t)atlas_width * 4;
    if ((size_t)atlas_height > SIZE_MAX / pitch) {
        status = RENDERER_STATUS_OUT_OF_MEMORY;
        goto failure;
    }
    allocation_size = (size_t)atlas_height * pitch;
    pixels = calloc(1, allocation_size);
    candidate = calloc(1, sizeof(*candidate));
    if (pixels == NULL || candidate == NULL) {
        status = RENDERER_STATUS_OUT_OF_MEMORY;
        goto failure;
    }
    candidate->renderer = renderer;
    candidate->geometry.atlas_width = atlas_width;
    candidate->geometry.atlas_height = atlas_height;
    candidate->geometry.ascent = source->ascent;
    candidate->geometry.line_height = source->line_height;
    candidate->geometry.line_spacing = source->line_spacing;
    candidate->geometry.fallback_byte = (unsigned char)'?';

    for (byte = TEXT_ATLAS_ASCII_FIRST;
         byte <= TEXT_ATLAS_ASCII_LAST; byte++) {
        TextGeometryGlyph *geometry = &candidate->geometry.glyphs[byte];
        int offset = byte - TEXT_ATLAS_ASCII_FIRST;

        if (!glyphs[byte].available)
            continue;
        geometry->available = 1;
        geometry->bearing_x = glyphs[byte].bearing_x;
        geometry->bearing_top = glyphs[byte].bearing_top;
        geometry->advance = glyphs[byte].advance;
        if (glyphs[byte].surface == NULL)
            continue;
        geometry->atlas_bounds.x = (offset % TEXT_ATLAS_COLUMNS) * cell_width
            + TEXT_ATLAS_GUTTER;
        geometry->atlas_bounds.y = (offset / TEXT_ATLAS_COLUMNS) * cell_height
            + TEXT_ATLAS_GUTTER;
        geometry->atlas_bounds.width = glyphs[byte].surface->w;
        geometry->atlas_bounds.height = glyphs[byte].surface->h;
        status = Copy_surface_rgba8(
            glyphs[byte].surface, pixels, pitch,
            geometry->atlas_bounds.x, geometry->atlas_bounds.y);
        if (status != RENDERER_STATUS_OK)
            goto failure;
    }
    Release_glyphs(source, glyphs);

    desc.width = atlas_width;
    desc.height = atlas_height;
    desc.filter = RENDERER_TEXTURE_FILTER_NEAREST;
    desc.wrap = RENDERER_TEXTURE_WRAP_CLAMP;
    status = Renderer_texture_create_with_desc(
        renderer, &desc, pixels, pitch, &candidate->texture);
    if (status != RENDERER_STATUS_OK)
        goto failure_without_glyphs;
    free(pixels);
    *atlas = candidate;
    return RENDERER_STATUS_OK;

failure:
    Release_glyphs(source, glyphs);
failure_without_glyphs:
    free(pixels);
    free(candidate);
    return status;
}

RendererStatus Text_atlas_destroy(TextAtlas **atlas)
{
    RendererStatus status;
    TextAtlas *owned;

    if (atlas == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    owned = *atlas;
    if (owned == NULL)
        return RENDERER_STATUS_OK;
    status = Renderer_texture_destroy(owned->renderer, owned->texture);
    if (status != RENDERER_STATUS_OK)
        return status;
    owned->texture = NULL;
    owned->renderer = NULL;
    free(owned);
    *atlas = NULL;
    return RENDERER_STATUS_OK;
}

const TextGeometryFont *Text_atlas_geometry_font(const TextAtlas *atlas)
{
    return atlas != NULL ? &atlas->geometry : NULL;
}

RendererTexture *Text_atlas_texture(const TextAtlas *atlas)
{
    return atlas != NULL ? atlas->texture : NULL;
}

Renderer *Text_atlas_renderer(const TextAtlas *atlas)
{
    return atlas != NULL ? atlas->renderer : NULL;
}
