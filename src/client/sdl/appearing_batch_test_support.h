#ifndef XPILOT_APPEARING_BATCH_TEST_SUPPORT_H
#define XPILOT_APPEARING_BATCH_TEST_SUPPORT_H

#include "renderer.h"

#include <stddef.h>

#ifdef XPILOT_APPEARING_BATCH_TEST_HOOKS
/** One appearing indicator supplied directly to the SDL batch test hook. */
typedef struct SdlguiTestAppearing {
    /** Horizontal center in inherited world coordinates. */
    int x;
    /** Vertical center in inherited world coordinates. */
    int y;
    /** Player identifier used for life color and base-warning state. */
    int id;
    /** Server appearing progress value. */
    int count;
} SdlguiTestAppearing;

/**
 * Paint a contiguous group of appearing indicators for tests.
 *
 * @param entries Indicators in production traversal order.
 * @param entry_count Number of indicators in @p entries.
 * @return The sticky semantic renderer result after the group finishes.
 *
 * @remarks This hook accepts full-width integers so numeric boundary behavior
 *          can be tested independently of the network-facing short fields.
 */
RendererStatus Sdlgui_test_paint_appearing_batch(
    const SdlguiTestAppearing *entries, size_t entry_count);
#endif

#endif
