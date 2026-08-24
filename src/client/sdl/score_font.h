#ifndef SCORE_FONT_H
#define SCORE_FONT_H

#include "native_text.h"

#include <SDL3/SDL.h>

#include <stdbool.h>

/** A score-list font resolved by logical family name with system fallbacks. */
typedef struct ScoreFont {
    /** Owned backend-neutral font set. */
    XpTextFont *font;
} ScoreFont;

/**
 * Open a score-list font from installed family names.
 *
 * @param font Zero-initialized destination to populate.
 * @param system Borrowed shared native text service.
 * @param family_list Comma-separated preferred family names.
 * @param pixel_height Desired logical pixel height.
 * @return true when at least one face and its fallback chain were opened.
 */
bool Score_font_open(ScoreFont *font, XpTextSystem *system,
		     const char *family_list, float pixel_height);

/**
 * Render one left-to-right UTF-8 score-list string.
 *
 * @param font Open score-list font.
 * @param utf8 NUL-terminated UTF-8 text.
 * @param color Glyph color.
 * @return Owned SDL surface, or NULL on failure.
 */
SDL_Surface *Score_font_render(ScoreFont *font, const char *utf8,
			       SDL_Color color);

/**
 * Return the score font's line spacing.
 *
 * @param font Open score-list font.
 * @return Positive spacing in pixels, or zero on failure.
 */
int Score_font_line_spacing(ScoreFont *font);

/**
 * Test whether the resolved score font chain supplies one code point.
 *
 * @param font Open score-list font.
 * @param codepoint Unicode scalar value.
 * @return Nonzero when a glyph is available.
 */
int Score_font_has_glyph(const ScoreFont *font, uint32_t codepoint);

/**
 * Close a score-list font.
 *
 * @param font Font to close. NULL and already-closed values are accepted.
 */
void Score_font_close(ScoreFont *font);

#endif
