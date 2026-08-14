/* SDL game-frame completion and presentation state. */

#include "sdlgameframe.h"

void Sdl_game_frame_state_init(SdlGameFrameState *state)
{
    if (state == NULL)
        return;
    state->pending_end = 0;
    state->abort_status = RENDERER_STATUS_OK;
    state->back_buffer_unsafe = 0;
}

int Sdl_game_frame_pending(const SdlGameFrameState *state)
{
    return state != NULL && state->pending_end;
}

RendererStatus Sdl_game_frame_activate(SdlGameFrameState *state)
{
    if (state == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (state->pending_end)
        return RENDERER_STATUS_INVALID_STATE;
    state->pending_end = 1;
    state->abort_status = RENDERER_STATUS_OK;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_game_frame_abort(SdlGameFrameState *state,
                                    RendererStatus status)
{
    if (state == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!state->pending_end)
        return RENDERER_STATUS_INVALID_STATE;
    if (status == RENDERER_STATUS_OK)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (state->abort_status == RENDERER_STATUS_OK)
        state->abort_status = status;
    return state->abort_status;
}

RendererStatus Sdl_game_frame_present_idle(
    const SdlGameFrameState *state, SdlGameFrameSwap swap, void *context)
{
    if (state == NULL || swap == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (state->pending_end || state->back_buffer_unsafe)
        return RENDERER_STATUS_INVALID_STATE;
    swap(context);
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_game_frame_finish(SdlGameFrameState *state,
                                     SdlGameFrameEnd end,
                                     SdlGameFrameSwap swap,
                                     void *context)
{
    RendererStatus status;
    RendererStatus abort_status;

    if (state == NULL || end == NULL || swap == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!state->pending_end)
        return RENDERER_STATUS_INVALID_STATE;

    status = end(context);
    if (status != RENDERER_STATUS_OK)
        return status;

    abort_status = state->abort_status;
    state->pending_end = 0;
    state->abort_status = RENDERER_STATUS_OK;
    if (abort_status != RENDERER_STATUS_OK) {
        state->back_buffer_unsafe = 1;
        return abort_status;
    }
    swap(context);
    state->back_buffer_unsafe = 0;
    return RENDERER_STATUS_OK;
}
