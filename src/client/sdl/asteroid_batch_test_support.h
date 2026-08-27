#ifndef XPILOT_ASTEROID_BATCH_TEST_SUPPORT_H
#define XPILOT_ASTEROID_BATCH_TEST_SUPPORT_H

#include "renderer.h"

#include <stddef.h>

#ifdef XPILOT_ASTEROID_BATCH_TEST_HOOKS
/** One asteroid supplied directly to the SDL batch test hook. */
typedef struct SdlguiTestAsteroid {
    /** Horizontal center in world coordinates. */
    int x;
    /** Vertical center in world coordinates. */
    int y;
    /** Axis selector; the low three bits select the rotation axis. */
    int type;
    /** Rotation in TABLE_SIZE units per full turn. */
    int rotation;
    /** Scale selector in the range 0 through 255. */
    int size;
} SdlguiTestAsteroid;

/**
 * Paint a contiguous group of asteroids for tests.
 *
 * @param entries Asteroids in production traversal order.
 * @param entry_count Number of asteroids in @p entries.
 * @return The sticky semantic renderer result after the group finishes.
 *
 * @remarks This hook accepts full-width integers so byte-domain boundaries,
 *          numeric failures, and all-or-nothing behavior can be tested.
 */
RendererStatus Sdlgui_test_paint_asteroid_batch(
    const SdlguiTestAsteroid *entries, size_t entry_count);
#endif

#endif
