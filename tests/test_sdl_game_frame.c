#include "sdlgameframe.h"

#include <stdio.h>
#include <string.h>

#define MAX_END_RESULTS 4

#define TEST_CHECK(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "%s:%d: check failed: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct FakeGameFrame {
    RendererStatus end_results[MAX_END_RESULTS];
    int end_result_count;
    int end_result_index;
    int frame_start_calls;
    int begin_calls;
    int draw_calls;
    int end_calls;
    int swap_calls;
    int abort_draw;
    char events[32];
    int event_count;
} FakeGameFrame;

static void record_event(FakeGameFrame *frame, char event)
{
    if (frame->event_count < (int)sizeof(frame->events))
        frame->events[frame->event_count++] = event;
}

static RendererStatus fake_end(void *context)
{
    FakeGameFrame *frame = context;
    RendererStatus status = RENDERER_STATUS_OK;

    frame->end_calls++;
    record_event(frame, 'E');
    if (frame->end_result_index < frame->end_result_count)
        status = frame->end_results[frame->end_result_index++];
    return status;
}

static void fake_swap(void *context)
{
    FakeGameFrame *frame = context;

    frame->swap_calls++;
    record_event(frame, 'S');
}

static RendererStatus fake_tick(SdlGameFrameState *state,
                                FakeGameFrame *frame)
{
    RendererStatus status;

    /* Recovery is a complete tick: it must precede logical-frame updates and
     * must never fall through to another begin on the same back buffer. */
    if (Sdl_game_frame_pending(state))
        return Sdl_game_frame_finish(state, fake_end, fake_swap, frame);

    frame->frame_start_calls++;
    record_event(frame, 'F');
    frame->begin_calls++;
    record_event(frame, 'B');
    status = Sdl_game_frame_activate(state);
    if (status != RENDERER_STATUS_OK)
        return status;

    frame->draw_calls++;
    record_event(frame, 'D');
    if (frame->abort_draw) {
        status = Sdl_game_frame_abort(
            state, RENDERER_STATUS_BACKEND_ERROR);
        if (status != RENDERER_STATUS_BACKEND_ERROR)
            return status;
    }
    return Sdl_game_frame_finish(state, fake_end, fake_swap, frame);
}

static int check_normal_frame_presents_once(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    Sdl_game_frame_state_init(&state);

    TEST_CHECK(fake_tick(&state, &frame) == RENDERER_STATUS_OK);
    TEST_CHECK(!Sdl_game_frame_pending(&state));
    TEST_CHECK(frame.frame_start_calls == 1);
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(frame.event_count == 5);
    TEST_CHECK(memcmp(frame.events, "FBDES", 5) == 0);
    return 0;
}

static int check_pending_success_presents_before_next_frame(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    frame.end_results[0] = RENDERER_STATUS_BACKEND_ERROR;
    frame.end_results[1] = RENDERER_STATUS_OK;
    frame.end_result_count = 2;
    Sdl_game_frame_state_init(&state);

    TEST_CHECK(fake_tick(&state, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(Sdl_game_frame_pending(&state));
    TEST_CHECK(frame.frame_start_calls == 1);
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(memcmp(frame.events, "FBDE", 4) == 0);

    TEST_CHECK(fake_tick(&state, &frame) == RENDERER_STATUS_OK);
    TEST_CHECK(!Sdl_game_frame_pending(&state));
    TEST_CHECK(frame.frame_start_calls == 1);
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 2);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(frame.event_count == 6);
    TEST_CHECK(memcmp(frame.events, "FBDEES", 6) == 0);

    TEST_CHECK(fake_tick(&state, &frame) == RENDERER_STATUS_OK);
    TEST_CHECK(frame.frame_start_calls == 2);
    TEST_CHECK(frame.begin_calls == 2);
    TEST_CHECK(frame.draw_calls == 2);
    TEST_CHECK(frame.swap_calls == 2);
    TEST_CHECK(memcmp(frame.events, "FBDEESFBDES", 11) == 0);
    return 0;
}

static int check_aborted_pending_never_presents(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    frame.abort_draw = 1;
    frame.end_results[0] = RENDERER_STATUS_BACKEND_ERROR;
    frame.end_results[1] = RENDERER_STATUS_OK;
    frame.end_result_count = 2;
    Sdl_game_frame_state_init(&state);

    TEST_CHECK(fake_tick(&state, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(Sdl_game_frame_pending(&state));
    TEST_CHECK(frame.frame_start_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);

    TEST_CHECK(fake_tick(&state, &frame)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(!Sdl_game_frame_pending(&state));
    TEST_CHECK(frame.frame_start_calls == 1);
    TEST_CHECK(frame.begin_calls == 1);
    TEST_CHECK(frame.draw_calls == 1);
    TEST_CHECK(frame.end_calls == 2);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(frame.event_count == 5);
    TEST_CHECK(memcmp(frame.events, "FBDEE", 5) == 0);
    TEST_CHECK(Sdl_game_frame_present_idle(&state, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(frame.swap_calls == 0);

    frame.abort_draw = 0;
    TEST_CHECK(fake_tick(&state, &frame) == RENDERER_STATUS_OK);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(Sdl_game_frame_present_idle(&state, fake_swap, &frame)
               == RENDERER_STATUS_OK);
    TEST_CHECK(frame.swap_calls == 2);
    return 0;
}

static int check_first_abort_result_is_retained(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    Sdl_game_frame_state_init(&state);
    TEST_CHECK(Sdl_game_frame_activate(&state) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_game_frame_abort(
                   &state, RENDERER_STATUS_OUT_OF_MEMORY)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_game_frame_abort(
                   &state, RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_game_frame_finish(&state, fake_end, fake_swap, &frame)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(frame.end_calls == 1);
    TEST_CHECK(frame.swap_calls == 0);
    TEST_CHECK(!Sdl_game_frame_pending(&state));
    TEST_CHECK(Sdl_game_frame_present_idle(&state, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(frame.swap_calls == 0);
    return 0;
}

static int check_idle_damaged_tick_presents_unchanged_buffer(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    Sdl_game_frame_state_init(&state);
    TEST_CHECK(Sdl_game_frame_present_idle(&state, fake_swap, &frame)
               == RENDERER_STATUS_OK);
    TEST_CHECK(frame.swap_calls == 1);
    TEST_CHECK(frame.event_count == 1 && frame.events[0] == 'S');
    TEST_CHECK(!Sdl_game_frame_pending(&state));

    TEST_CHECK(Sdl_game_frame_activate(&state) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_game_frame_present_idle(&state, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(frame.swap_calls == 1);
    return 0;
}

static int check_invalid_contract_does_no_work(void)
{
    SdlGameFrameState state;
    FakeGameFrame frame;

    memset(&frame, 0, sizeof(frame));
    Sdl_game_frame_state_init(&state);
    TEST_CHECK(!Sdl_game_frame_pending(NULL));
    TEST_CHECK(Sdl_game_frame_activate(NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_abort(
                   &state, RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Sdl_game_frame_finish(&state, fake_end, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Sdl_game_frame_activate(&state) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_game_frame_activate(&state)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Sdl_game_frame_abort(&state, RENDERER_STATUS_OK)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_finish(NULL, fake_end, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_finish(&state, NULL, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_finish(&state, fake_end, NULL, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_present_idle(NULL, fake_swap, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_game_frame_present_idle(&state, NULL, &frame)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(frame.end_calls == 0);
    TEST_CHECK(frame.swap_calls == 0);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_normal_frame_presents_once() == 0);
    TEST_CHECK(check_pending_success_presents_before_next_frame() == 0);
    TEST_CHECK(check_aborted_pending_never_presents() == 0);
    TEST_CHECK(check_first_abort_result_is_retained() == 0);
    TEST_CHECK(check_idle_damaged_tick_presents_unchanged_buffer() == 0);
    TEST_CHECK(check_invalid_contract_does_no_work() == 0);
    return 0;
}
