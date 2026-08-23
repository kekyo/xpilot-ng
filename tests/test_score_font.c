#include "test_helpers.h"

#include "score_font.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#ifndef XPILOT_TEST_SCORE_FONT_PATH
#error "XPILOT_TEST_SCORE_FONT_PATH must name the score-list font"
#endif
#ifndef XPILOT_TEST_SCORE_FALLBACK_FONT_PATH
#error "XPILOT_TEST_SCORE_FALLBACK_FONT_PATH must name the fallback font"
#endif

static int Score_font_renders_japanese(void)
{
    const SDL_Color white = { 255, 255, 255, 255 };
    const char japanese_name[] = "\xe6\x97\xa5\xe6\x9c\xac";
    ScoreFont score_font = { NULL, NULL };
    SDL_Surface *surface;
    int result = 0;

    if (!Score_font_open(&score_font, XPILOT_TEST_SCORE_FONT_PATH,
			 XPILOT_TEST_SCORE_FALLBACK_FONT_PATH, 11)) {
	fprintf(stderr, "score fonts could not be opened: %s\n", SDL_GetError());
	return 1;
    }
    if (!TTF_FontHasGlyph(score_font.primary, 0x65e5)) {
	fprintf(stderr, "score font does not provide the Japanese glyph U+65E5\n");
	result = 1;
    }
    surface = TTF_RenderText_Blended(
	score_font.primary, japanese_name, 0, white);
    if (surface == NULL || surface->w <= 0 || surface->h <= 0) {
	fprintf(stderr, "Japanese score name could not be rendered: %s\n",
		SDL_GetError());
	result = 1;
    }
    if (surface != NULL)
	SDL_DestroySurface(surface);
    Score_font_close(&score_font);
    if (score_font.primary != NULL || score_font.fallback != NULL) {
	fprintf(stderr, "score fonts were not fully closed\n");
	result = 1;
    }
    return result;
}

int main(void)
{
    int result;

    TEST_CHECK(TTF_Init());
    result = Score_font_renders_japanese();
    TTF_Quit();
    return result;
}
