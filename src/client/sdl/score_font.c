#include "score_font.h"

#include <stddef.h>
#include <string.h>

bool Score_font_open(ScoreFont *font, XpTextSystem *system,
		     const char *family_list, float pixel_height)
{
    XpTextFontRequest request;

    if (font == NULL || system == NULL || family_list == NULL
	|| font->font != NULL) {
	return false;
    }
    request.family_list = family_list;
    request.pixel_height = pixel_height;
    request.weight = XP_TEXT_WEIGHT_BOLD;
    request.slant = XP_TEXT_SLANT_NORMAL;
    request.spacing = XP_TEXT_SPACING_MONOSPACE;
    return Xp_text_font_open(system, &request, &font->font)
	== XP_TEXT_STATUS_OK;
}

SDL_Surface *Score_font_render(ScoreFont *font, const char *utf8,
			       SDL_Color color)
{
    XpTextLayoutRequest request;
    XpTextLayout *layout = NULL;
    XpTextBitmap bitmap = { NULL, 0, 0, 0 };
    SDL_Surface *surface = NULL;
    int locked = 0;
    int x;
    int y;

    if (font == NULL || font->font == NULL || utf8 == NULL)
	return NULL;
    request.utf8 = utf8;
    request.byte_length = strlen(utf8);
    request.direction = XP_TEXT_DIRECTION_LTR;
    request.language_bcp47 = NULL;
    if (Xp_text_layout_create(font->font, &request, &layout)
	!= XP_TEXT_STATUS_OK
	|| Xp_text_layout_rasterize(layout, &bitmap) != XP_TEXT_STATUS_OK
	|| bitmap.width <= 0 || bitmap.height <= 0) {
	goto cleanup;
    }
    surface = SDL_CreateSurface(
	bitmap.width, bitmap.height, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL)
	goto cleanup;
    if (SDL_MUSTLOCK(surface)) {
	if (!SDL_LockSurface(surface)) {
	    SDL_DestroySurface(surface);
	    surface = NULL;
	    goto cleanup;
	}
	locked = 1;
    }
    for (y = 0; y < bitmap.height; y++) {
	const unsigned char *source = bitmap.pixels
	    + (size_t)y * bitmap.pitch;
	unsigned char *destination = (unsigned char *)surface->pixels
	    + (size_t)y * (size_t)surface->pitch;

	for (x = 0; x < bitmap.width; x++) {
	    const unsigned int alpha = source[(size_t)x * 4 + 3];

	    destination[(size_t)x * 4] = color.r;
	    destination[(size_t)x * 4 + 1] = color.g;
	    destination[(size_t)x * 4 + 2] = color.b;
	    destination[(size_t)x * 4 + 3] =
		(unsigned char)((alpha * color.a + 127) / 255);
	}
    }
    if (locked) {
	SDL_UnlockSurface(surface);
	locked = 0;
    }

cleanup:
    if (locked)
	SDL_UnlockSurface(surface);
    Xp_text_bitmap_destroy(&bitmap);
    Xp_text_layout_destroy(&layout);
    return surface;
}

int Score_font_line_spacing(ScoreFont *font)
{
    XpTextLayoutRequest request;
    XpTextLayout *layout = NULL;
    XpTextMetrics metrics;
    int spacing = 0;

    if (font == NULL || font->font == NULL)
	return 0;
    request.utf8 = "M";
    request.byte_length = 1;
    request.direction = XP_TEXT_DIRECTION_LTR;
    request.language_bcp47 = NULL;
    if (Xp_text_layout_create(font->font, &request, &layout)
	== XP_TEXT_STATUS_OK
	&& Xp_text_layout_measure(layout, &metrics) == XP_TEXT_STATUS_OK) {
	spacing = metrics.line_spacing;
    }
    Xp_text_layout_destroy(&layout);
    return spacing;
}

int Score_font_has_glyph(const ScoreFont *font, uint32_t codepoint)
{
    return font != NULL
	&& Xp_text_font_has_glyph(font->font, codepoint);
}

void Score_font_close(ScoreFont *font)
{
    if (font == NULL)
	return;
    Xp_text_font_close(&font->font);
}
