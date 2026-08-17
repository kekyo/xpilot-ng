/* SDL_ttf raster source adapter for the backend-neutral text atlas. */

#ifndef TEXT_TTF_ATLAS_H
#define TEXT_TTF_ATLAS_H

#include "text_atlas.h"

#include <SDL3_ttf/SDL_ttf.h>

/**
 * Prewarm printable ASCII from an SDL_ttf font into one text atlas.
 *
 * @param renderer Renderer that owns the resulting texture.
 * @param font Borrowed SDL_ttf font used only during this call.
 * @param atlas Empty destination that receives the owned atlas on success.
 * @return Operation status.
 *
 * @remarks The font's 16-bit glyph APIs are used for bytes 0x20 through
 * 0x7e. Missing bytes remain unavailable and are resolved by the generic
 * atlas fallback. Creation is atomic and must occur outside an active
 * renderer frame.
 */
RendererStatus Text_ttf_atlas_create(Renderer *renderer, TTF_Font *font,
                                     TextAtlas **atlas);

#endif /* TEXT_TTF_ATLAS_H */
