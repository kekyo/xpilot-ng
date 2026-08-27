#ifndef XPILOT_SPARK_BATCH_TEST_SUPPORT_H
#define XPILOT_SPARK_BATCH_TEST_SUPPORT_H

#include "renderer.h"

#include <stddef.h>

#ifdef XPILOT_SPARK_BATCH_TEST_HOOKS
/** One spark supplied directly to the SDL batch test hook. */
typedef struct SdlguiTestSpark {
    /** Client spark color selector. */
    int color;
    /** Horizontal screen-relative coordinate. */
    int x;
    /** Vertical screen-relative coordinate with an upward-positive axis. */
    int y;
} SdlguiTestSpark;

/**
 * Paint a contiguous group of sparks for tests.
 *
 * @param entries Sparks in production traversal order.
 * @param entry_count Number of sparks in @p entries.
 * @return The sticky semantic renderer result after the group finishes.
 *
 * @remarks This hook accepts full-width integers so numeric boundary and
 *          all-or-nothing batch behavior can be tested independently of the
 *          network-facing byte coordinates.
 */
RendererStatus Sdlgui_test_paint_spark_batch(
    const SdlguiTestSpark *entries, size_t entry_count);
#endif

#endif
