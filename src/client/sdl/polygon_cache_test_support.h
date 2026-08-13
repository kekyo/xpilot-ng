#ifndef XPILOT_POLYGON_CACHE_TEST_SUPPORT_H
#define XPILOT_POLYGON_CACHE_TEST_SUPPORT_H

#include "renderer.h"

#ifdef XPILOT_POLYGON_CACHE_TEST_HOOKS
/**
 * Build the semantic geometry cache for the current polygon scene.
 *
 * @return Operation status.
 *
 * @remarks The cache must be empty when this function is called. The complete
 *          scene is published on success; every failure leaves it empty.
 */
RendererStatus Sdlgui_test_prepare_polygon_cache(void);

/**
 * Release the semantic polygon cache used by integration tests.
 *
 * @remarks Calling this function repeatedly is safe.
 */
void Sdlgui_test_discard_polygon_cache(void);
#endif

#endif /* XPILOT_POLYGON_CACHE_TEST_SUPPORT_H */
