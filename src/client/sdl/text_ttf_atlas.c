/* SDL_ttf raster source adapter for the backend-neutral text atlas. */

#include "text_ttf_atlas.h"

#include <SDL.h>

typedef struct TextTtfAtlasSource {
    TTF_Font *font;
    int ascent;
} TextTtfAtlasSource;

static RendererStatus Load_glyph(void *context, unsigned char byte,
                                 TextAtlasGlyphSurface *glyph)
{
    TextTtfAtlasSource *source = context;
    Uint16 character = (Uint16)byte;
    SDL_Color white = {255, 255, 255, SDL_ALPHA_OPAQUE};
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int advance;

    if (!TTF_GlyphIsProvided(source->font, character))
        return RENDERER_STATUS_OK;
    if (TTF_GlyphMetrics(source->font, character,
                         &min_x, &max_x, &min_y, &max_y, &advance) < 0) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    glyph->bearing_x = (float)(min_x < 0 ? min_x : 0);
    glyph->bearing_top = (float)source->ascent;
    glyph->advance = (float)advance;
    glyph->available = 1;
    if (max_x <= min_x || max_y <= min_y)
        return RENDERER_STATUS_OK;

    glyph->surface =
        TTF_RenderGlyph_Blended(source->font, character, white);
    if (glyph->surface == NULL)
        return RENDERER_STATUS_BACKEND_ERROR;
    return RENDERER_STATUS_OK;
}

static void Release_glyph(void *context, TextAtlasGlyphSurface *glyph)
{
    (void)context;
    SDL_FreeSurface(glyph->surface);
    glyph->surface = NULL;
}

RendererStatus Text_ttf_atlas_create(Renderer *renderer, TTF_Font *font,
                                     TextAtlas **atlas)
{
    TextTtfAtlasSource context;
    TextAtlasSource source;
    int height;
    int line_skip;

    if (renderer == NULL || font == NULL || atlas == NULL || *atlas != NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;

    context.font = font;
    context.ascent = TTF_FontAscent(font);
    height = TTF_FontHeight(font);
    line_skip = TTF_FontLineSkip(font);
    if (context.ascent < 0 || height <= 0 || line_skip <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;

    source.context = &context;
    source.ascent = (float)context.ascent;
    source.line_height = (float)height;
    source.line_spacing = (float)line_skip;
    source.load_glyph = Load_glyph;
    source.release_glyph = Release_glyph;
    return Text_atlas_create(renderer, &source, atlas);
}
