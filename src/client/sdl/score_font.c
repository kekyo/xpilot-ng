#include "score_font.h"

bool Score_font_open(ScoreFont *font, const char *primary_path,
		     const char *fallback_path, float point_size)
{
    TTF_Font *fallback;
    TTF_Font *primary;

    if (font == NULL || primary_path == NULL || fallback_path == NULL
	|| point_size <= 0.0f || font->primary != NULL
	|| font->fallback != NULL) {
	return false;
    }
    primary = TTF_OpenFont(primary_path, point_size);
    if (primary == NULL)
	return false;
    fallback = TTF_OpenFont(fallback_path, point_size);
    if (fallback == NULL) {
	TTF_CloseFont(primary);
	return false;
    }
    if (!TTF_AddFallbackFont(primary, fallback)) {
	TTF_CloseFont(fallback);
	TTF_CloseFont(primary);
	return false;
    }
    font->primary = primary;
    font->fallback = fallback;
    return true;
}

void Score_font_close(ScoreFont *font)
{
    if (font == NULL)
	return;
    if (font->primary != NULL && font->fallback != NULL)
	TTF_RemoveFallbackFont(font->primary, font->fallback);
    if (font->fallback != NULL) {
	TTF_CloseFont(font->fallback);
	font->fallback = NULL;
    }
    if (font->primary != NULL) {
	TTF_CloseFont(font->primary);
	font->primary = NULL;
    }
}
