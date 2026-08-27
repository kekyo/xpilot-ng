/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sdlmetaframe.h"

static int Sdl_meta_frame_ops_valid(const SdlMetaFrameOps *ops)
{
    return ops != NULL
        && ops->begin != NULL
        && ops->draw != NULL
        && ops->frame_result != NULL
        && ops->end != NULL
        && ops->swap != NULL;
}

void Sdl_meta_frame_state_init(SdlMetaFrameState *state)
{
    if (state == NULL)
        return;

    state->pending_end = 0;
    state->abort_requested = 0;
    state->abort_status = RENDERER_STATUS_OK;
}

static RendererStatus Sdl_meta_frame_finish(SdlMetaFrameState *state,
                                            const SdlMetaFrameOps *ops,
                                            void *context)
{
    RendererStatus status = ops->end(context);
    RendererStatus abort_status;

    if (status != RENDERER_STATUS_OK)
        return status;

    if (state->abort_requested) {
        abort_status = state->abort_status;
        Sdl_meta_frame_state_init(state);
        return abort_status;
    }

    Sdl_meta_frame_state_init(state);
    ops->swap(context);
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_meta_frame_tick(SdlMetaFrameState *state,
                                   const SdlMetaFrameOps *ops,
                                   void *context)
{
    RendererStatus status;

    if (state == NULL || !Sdl_meta_frame_ops_valid(ops))
        return RENDERER_STATUS_INVALID_ARGUMENT;

    if (state->pending_end)
        return Sdl_meta_frame_finish(state, ops, context);

    status = ops->begin(context);
    if (status != RENDERER_STATUS_OK)
        return status;

    /* From this point cleanup must wait until end succeeds. */
    state->pending_end = 1;
    status = ops->draw(context);
    if (status != RENDERER_STATUS_OK) {
        state->abort_requested = 1;
        state->abort_status = status;
    } else {
        status = ops->frame_result(context);
        if (status != RENDERER_STATUS_OK) {
            state->abort_requested = 1;
            state->abort_status = status;
        }
    }

    return Sdl_meta_frame_finish(state, ops, context);
}

int Sdl_meta_frame_cleanup_allowed(const SdlMetaFrameState *state)
{
    return state != NULL && !state->pending_end;
}
