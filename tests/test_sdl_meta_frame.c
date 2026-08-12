#include "test_helpers.h"

#include "sdlmetaframe.h"

#include <string.h>

#define MAX_END_RESULTS 4
#define MAX_EVENTS 16

typedef struct FakeFrame {
    RendererStatus begin_result;
    RendererStatus draw_result;
    RendererStatus end_results[MAX_END_RESULTS];
    int end_result_count;
    int end_result_index;
    int begin_calls;
    int draw_calls;
    int end_calls;
    int swap_calls;
    int cleanup_calls;
    char events[MAX_EVENTS];
    int event_count;
} FakeFrame;

static void record_event(FakeFrame *frame, char event)
{
    if (frame->event_count < MAX_EVENTS)
        frame->events[frame->event_count++] = event;
}

static RendererStatus fake_begin(void *context)
{
    FakeFrame *frame = context;

    frame->begin_calls++;
    record_event(frame, 'B');
    return frame->begin_result;
}

static RendererStatus fake_draw(void *context)
{
    FakeFrame *frame = context;

    frame->draw_calls++;
    record_event(frame, 'D');
    return frame->draw_result;
}

static RendererStatus fake_end(void *context)
{
    FakeFrame *frame = context;
    RendererStatus result = RENDERER_STATUS_OK;

    frame->end_calls++;
    record_event(frame, 'E');
    if (frame->end_result_index < frame->end_result_count) {
        result = frame->end_results[frame->end_result_index];
        frame->end_result_index++;
    }
    return result;
}

static void fake_swap(void *context)
{
    FakeFrame *frame = context;

    frame->swap_calls++;
    record_event(frame, 'S');
}

static const SdlMetaFrameOps fake_ops = {
    fake_begin,
    fake_draw,
    fake_end,
    fake_swap
};

static void fake_frame_init(FakeFrame *frame)
{
    memset(frame, 0, sizeof(*frame));
    frame->begin_result = RENDERER_STATUS_OK;
    frame->draw_result = RENDERER_STATUS_OK;
}

static void cleanup_when_allowed(const SdlMetaFrameState *state,
                                 FakeFrame *frame)
{
    if (Sdl_meta_frame_cleanup_allowed(state)) {
        frame->cleanup_calls++;
        record_event(frame, 'C');
    }
}

static int state_is_idle(const SdlMetaFrameState *state)
{
    return state->pending_end == 0
        && state->abort_requested == 0
        && state->abort_status == RENDERER_STATUS_OK;
}

static int check_normal_present(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;

    memset(&state, 0xff, sizeof(state));
    fake_frame_init(&frame);
    Sdl_meta_frame_state_init(&state);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_OK);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(frame.event_count == 4);
    TEST_CHECK(memcmp(frame.events, "BDES", 4) == 0);
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));
    cleanup_when_allowed(&state, &frame);
    TEST_CHECK(frame.cleanup_calls == 1);
    return 0;
}

static int check_begin_failure_stays_inactive(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;

    fake_frame_init(&frame);
    frame.begin_result = RENDERER_STATUS_BACKEND_ERROR;
    Sdl_meta_frame_state_init(&state);

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 0);
    TEST_CHECK(frame.end_calls == 0);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(frame.event_count == 1 && frame.events[0] == 'B');
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));
    return 0;
}

static int check_end_retry_does_not_redraw(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;

    fake_frame_init(&frame);
    frame.end_results[0] = RENDERER_STATUS_BACKEND_ERROR;
    frame.end_results[1] = RENDERER_STATUS_OK;
    frame.end_result_count = 2;
    Sdl_meta_frame_state_init(&state);

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(state.pending_end == 1);
    TEST_CHECK(state.abort_requested == 0);
    TEST_CHECK(state.abort_status == RENDERER_STATUS_OK);
    TEST_CHECK(!Sdl_meta_frame_cleanup_allowed(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(frame.event_count == 3);
    TEST_CHECK(memcmp(frame.events, "BDE", 3) == 0);
    cleanup_when_allowed(&state, &frame);
    TEST_CHECK(frame.cleanup_calls == 0);

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_OK);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 2);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(frame.event_count == 5);
    TEST_CHECK(memcmp(frame.events, "BDEES", 5) == 0);
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));
    cleanup_when_allowed(&state, &frame);
    TEST_CHECK(frame.cleanup_calls == 1);
    TEST_CHECK(frame.events[5] == 'C');
    return 0;
}

static int check_draw_failure_aborts_without_swap(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;

    fake_frame_init(&frame);
    frame.draw_result = RENDERER_STATUS_OUT_OF_MEMORY;
    Sdl_meta_frame_state_init(&state);

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(frame.event_count == 3);
    TEST_CHECK(memcmp(frame.events, "BDE", 3) == 0);
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));
    return 0;
}

static int check_aborted_end_retry_returns_original_failure(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;

    fake_frame_init(&frame);
    frame.draw_result = RENDERER_STATUS_INVALID_STATE;
    frame.end_results[0] = RENDERER_STATUS_BACKEND_ERROR;
    frame.end_results[1] = RENDERER_STATUS_OK;
    frame.end_result_count = 2;
    Sdl_meta_frame_state_init(&state);

    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(state.pending_end == 1);
    TEST_CHECK(state.abort_requested == 1);
    TEST_CHECK(state.abort_status == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(!Sdl_meta_frame_cleanup_allowed(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);
    cleanup_when_allowed(&state, &frame);
    TEST_CHECK(frame.cleanup_calls == 0);

    /* Finishing an aborted frame reports the saved draw failure.  It must
     * not masquerade as a successfully presented frame. */
    TEST_CHECK(Sdl_meta_frame_tick(&state, &fake_ops, &frame)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 2);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(frame.event_count == 4);
    TEST_CHECK(memcmp(frame.events, "BDEE", 4) == 0);
    TEST_CHECK(Sdl_meta_frame_cleanup_allowed(&state));
    cleanup_when_allowed(&state, &frame);
    TEST_CHECK(frame.cleanup_calls == 1);
    TEST_CHECK(frame.events[4] == 'C');
    return 0;
}

static int check_invalid_contract_does_no_work(void)
{
    SdlMetaFrameState state;
    FakeFrame frame;
    SdlMetaFrameOps incomplete_ops = fake_ops;

    fake_frame_init(&frame);
    Sdl_meta_frame_state_init(&state);
    incomplete_ops.swap = NULL;
    TEST_CHECK(Sdl_meta_frame_tick(NULL, &fake_ops, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_meta_frame_tick(&state, NULL, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_meta_frame_tick(&state, &incomplete_ops, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(frame.begin_calls == 0);
    TEST_CHECK(frame.draw_calls == 0);
    TEST_CHECK(frame.end_calls == 0);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(state_is_idle(&state));
    TEST_CHECK(!Sdl_meta_frame_cleanup_allowed(NULL));
    return 0;
}

int main(void)
{
    TEST_CHECK(check_normal_present() == 0);
    TEST_CHECK(check_begin_failure_stays_inactive() == 0);
    TEST_CHECK(check_end_retry_does_not_redraw() == 0);
    TEST_CHECK(check_draw_failure_aborts_without_swap() == 0);
    TEST_CHECK(check_aborted_end_retry_returns_original_failure() == 0);
    TEST_CHECK(check_invalid_contract_does_no_work() == 0);
    return 0;
}
