/* Native-only bridge used by renderer adapters. */

#ifndef NATIVE_TEXT_INTERNAL_H
#define NATIVE_TEXT_INTERNAL_H

#include "native_text.h"

#include <SDL3_ttf/SDL_ttf.h>

/**
 * Borrow the primary SDL_ttf face owned by a font set.
 *
 * @param font Native font set.
 * @return Borrowed primary face, or NULL.
 */
TTF_Font *Xp_text_font_primary_ttf(XpTextFont *font);

#endif /* NATIVE_TEXT_INTERNAL_H */
