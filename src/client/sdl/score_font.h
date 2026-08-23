#ifndef SCORE_FONT_H
#define SCORE_FONT_H

#include <SDL3_ttf/SDL_ttf.h>

#include <stdbool.h>

/** A primary score-list font and its Unicode fallback. */
typedef struct ScoreFont {
    /** Font used for glyphs present in the original score-list typeface. */
    TTF_Font *primary;
    /** Font used when the primary font does not contain a requested glyph. */
    TTF_Font *fallback;
} ScoreFont;

/**
 * Open a score-list font with one fallback font.
 *
 * @param font Zero-initialized font pair to populate.
 * @param primary_path Path to the primary font file.
 * @param fallback_path Path to the fallback font file.
 * @param point_size Point size shared by both fonts.
 * @return true when both fonts were opened and linked; otherwise false.
 * @remarks The fonts must be opened and closed on the same thread. On failure,
 *          both members remain NULL.
 */
bool Score_font_open(ScoreFont *font, const char *primary_path,
		     const char *fallback_path, float point_size);

/**
 * Close a score-list font and detach its fallback.
 *
 * @param font Font pair to close. NULL and already-closed pairs are accepted.
 */
void Score_font_close(ScoreFont *font);

#endif
