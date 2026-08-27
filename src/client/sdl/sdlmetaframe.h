/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDLMETAFRAME_H
#define SDLMETAFRAME_H

#include "renderer.h"

/** Operations needed to draw and present one metaserver frame. */
typedef struct SdlMetaFrameOps {
    /** Begin a renderer frame. */
    RendererStatus (*begin)(void *context);
    /** Draw the complete metaserver frame. */
    RendererStatus (*draw)(void *context);
    /** Return the sticky semantic result after drawing succeeds. */
    RendererStatus (*frame_result)(void *context);
    /** Finish the active renderer frame. */
    RendererStatus (*end)(void *context);
    /** Present a successfully drawn and finished frame. */
    void (*swap)(void *context);
} SdlMetaFrameOps;

/** State retained while a metaserver renderer frame awaits completion. */
typedef struct SdlMetaFrameState {
    /** Nonzero while the active frame still requires a successful end. */
    int pending_end;
    /** Nonzero when a draw failure prevents presentation of the frame. */
    int abort_requested;
    /** Draw failure returned after an aborted frame finishes. */
    RendererStatus abort_status;
} SdlMetaFrameState;

/**
 * Initialize metaserver frame lifecycle state.
 *
 * @param state State to initialize. A NULL pointer is ignored.
 */
void Sdl_meta_frame_state_init(SdlMetaFrameState *state);

/**
 * Advance metaserver drawing or retry completion of an active frame.
 *
 * @param state Initialized lifecycle state.
 * @param ops Complete callback table. Pending retries must use callbacks for
 *        the same renderer frame as the original call.
 * @param context Opaque value passed unchanged to every invoked callback.
 * @return Operation status. If drawing failed and ending that frame also
 *         failed, the end failure is returned until ending succeeds; that
 *         successful retry then returns the saved draw failure.
 *
 * @remarks A pending frame is never begun or drawn again. The frame-result
 * callback runs only after drawing succeeds. The swap callback runs only
 * after drawing, the semantic frame result, and ending all succeed.
 */
RendererStatus Sdl_meta_frame_tick(SdlMetaFrameState *state,
                                   const SdlMetaFrameOps *ops,
                                   void *context);

/**
 * Report whether resources used by metaserver drawing may be released.
 *
 * @param state Initialized lifecycle state.
 * @return Nonzero when no renderer frame awaits completion, otherwise zero.
 */
int Sdl_meta_frame_cleanup_allowed(const SdlMetaFrameState *state);

#endif /* SDLMETAFRAME_H */
