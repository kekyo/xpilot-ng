#include "test_helpers.h"

#include "score_font.h"

#include <SDL3/SDL.h>
#ifndef XPILOT_TEST_FONT_DIR
#error "XPILOT_TEST_FONT_DIR must name the application font directory"
#endif

static int Score_font_renders_japanese(void)
{
    const SDL_Color white = { 255, 255, 255, 255 };
    const char japanese_name[] = "\xe6\x97\xa5\xe6\x9c\xac";
    const char ascii_name[] = "Pilot";
    XpTextSystem *system = NULL;
    ScoreFont score_font = { NULL };
    SDL_Surface *surface;
    const char *text;
    int result = 0;


    if (Xp_text_system_create(XPILOT_TEST_FONT_DIR, &system)
	!= XP_TEXT_STATUS_OK) {
	fprintf(stderr, "native text system could not be created\n");
	return 1;
    }
    if (!Score_font_open(&score_font, system,
			 "Bitstream Vera Sans Mono", 11)) {
	fprintf(stderr, "score fonts could not be opened: %s\n", SDL_GetError());
	Xp_text_system_destroy(&system);
	return 1;
    }
    text = Score_font_has_glyph(&score_font, 0x65e5)
	? japanese_name : ascii_name;
    surface = Score_font_render(&score_font, text, white);
    if (surface == NULL || surface->w <= 0 || surface->h <= 0) {
	fprintf(stderr, "UTF-8 score name could not be rendered: %s\n",
		SDL_GetError());
	result = 1;
    }
    if (Score_font_line_spacing(&score_font) <= 0) {
	fprintf(stderr, "score font line spacing is unavailable\n");
	result = 1;
    }
    if (surface != NULL)
	SDL_DestroySurface(surface);
    Score_font_close(&score_font);

    if (score_font.font != NULL) {
	fprintf(stderr, "score fonts were not fully closed\n");
	result = 1;
    }
    Xp_text_system_destroy(&system);
    return result;
}

int main(void)
{
    return Score_font_renders_japanese();
}
