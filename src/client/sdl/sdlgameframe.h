/* SDL game-frame completion and presentation state. */

#ifndef SDLGAMEFRAME_H
#define SDLGAMEFRAME_H

#include "renderer.h"

/** Callback that completes the currently active renderer frame. */
typedef RendererStatus (*SdlGameFrameEnd)(void *context);

/** Callback that presents one successfully completed renderer frame. */
typedef void (*SdlGameFrameSwap)(void *context);

/** State retained while a game renderer frame awaits completion. */
typedef struct SdlGameFrameState {
    /** Nonzero while an active frame still requires a successful end. */
    int pending_end;
    /** First drawing failure that prevents presentation. */
    RendererStatus abort_status;
    /** Nonzero while the current back buffer contains an aborted frame. */
    int back_buffer_unsafe;
} SdlGameFrameState;

/**
 * Initialize game-frame completion state.
 *
 * @param state State to initialize. A NULL pointer is ignored.
 */
void Sdl_game_frame_state_init(SdlGameFrameState *state);

/**
 * Report whether an active frame still requires completion.
 *
 * @param state Initialized state.
 * @return Nonzero while completion is pending, otherwise zero.
 */
int Sdl_game_frame_pending(const SdlGameFrameState *state);

/**
 * Mark a newly begun renderer frame as active.
 *
 * @param state Initialized idle state.
 * @return Operation status.
 */
RendererStatus Sdl_game_frame_activate(SdlGameFrameState *state);

/**
 * Retain the first drawing failure for the active game frame.
 *
 * @param state State with a pending active frame.
 * @param status Non-OK drawing failure.
 * @return The first retained drawing failure.
 */
RendererStatus Sdl_game_frame_abort(SdlGameFrameState *state,
                                    RendererStatus status);

/**
 * Present an unchanged back buffer when no renderer frame was begun.
 *
 * @param state Initialized idle state.
 * @param swap Callback that presents the unchanged buffer.
 * @param context Opaque value passed to @p swap.
 * @return Operation status.
 *
 * @remarks This preserves the legacy damaged-frame path. It is rejected
 * while a renderer frame awaits completion or while the back buffer contains
 * a previously aborted partial frame.
 */
RendererStatus Sdl_game_frame_present_idle(
    const SdlGameFrameState *state, SdlGameFrameSwap swap, void *context);

/**
 * End and, when not aborted, present the active game frame.
 *
 * @param state State with a pending active frame.
 * @param end Callback that retries renderer-frame completion.
 * @param swap Callback that presents a non-aborted completed frame.
 * @param context Opaque value passed to both callbacks.
 * @return End failure while completion remains pending, the retained draw
 *         failure after an aborted frame completes, or OK after presentation.
 *
 * @remarks A successful completion always makes the state idle. Presentation
 * occurs exactly once and only when no drawing failure was retained.
 */
RendererStatus Sdl_game_frame_finish(SdlGameFrameState *state,
                                     SdlGameFrameEnd end,
                                     SdlGameFrameSwap swap,
                                     void *context);

#endif /* SDLGAMEFRAME_H */
