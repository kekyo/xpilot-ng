#include "test_helpers.h"

#include "gameinput.h"

#include <SDL3/SDL.h>

#include <string.h>

#define MAX_RECORDED_EVENTS 16

static int processed_events;
static SDL_Event recorded_events[MAX_RECORDED_EVENTS];

int Process_event(SDL_Event *event)
{
    if (processed_events < MAX_RECORDED_EVENTS)
	recorded_events[processed_events] = *event;
    processed_events++;
    return 1;
}

static void Reset_processed_events(void)
{
    processed_events = 0;
    memset(recorded_events, 0, sizeof(recorded_events));
}

static int Queue_event_burst(int event_count)
{
    SDL_Event event;
    int index;

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_USER;
    for (index = 0; index < event_count; index++) {
	if (!SDL_PushEvent(&event))
	    return -1;
    }
    return 0;
}

static int Drain_remaining_events(void)
{
    SDL_Event event;
    int remaining = 0;

    while (SDL_PollEvent(&event))
	remaining++;
    return remaining;
}

static int Queue_mouse_motion(Uint32 window_id, SDL_MouseID mouse_id,
			      float x, float y, float xrel, float yrel)
{
    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = window_id;
    event.motion.which = mouse_id;
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = xrel;
    event.motion.yrel = yrel;
    return SDL_PushEvent(&event) ? 0 : -1;
}

static int Queue_user_event(void)
{
    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_USER;
    return SDL_PushEvent(&event) ? 0 : -1;
}

static int Test_batch_is_bounded(void)
{
    const int event_count = 2049;
    int remaining_events;

    Reset_processed_events();
    TEST_CHECK(Queue_event_burst(event_count) == 0);
    TEST_CHECK(Game_input_process_batch() == 0);
    remaining_events = Drain_remaining_events();
    TEST_CHECK(processed_events > 0);
    TEST_CHECK(remaining_events > 0);
    TEST_CHECK(processed_events + remaining_events == event_count);
    return 0;
}

static int Test_batch_coalesces_consecutive_mouse_motion(void)
{
    Reset_processed_events();
    TEST_CHECK(Queue_mouse_motion(7, 11, 10.0f, 20.0f,
				  0.25f, -0.5f) == 0);
    TEST_CHECK(Queue_mouse_motion(7, 11, 30.0f, 40.0f,
				  0.75f, -1.5f) == 0);
    TEST_CHECK(Queue_mouse_motion(7, 11, 50.0f, 60.0f,
				  2.0f, -3.0f) == 0);

    TEST_CHECK(Game_input_process_batch() == 0);
    TEST_CHECK(processed_events == 1);
    TEST_CHECK(recorded_events[0].type == SDL_EVENT_MOUSE_MOTION);
    TEST_CHECK(recorded_events[0].motion.windowID == 7);
    TEST_CHECK(recorded_events[0].motion.which == 11);
    TEST_CHECK(recorded_events[0].motion.x == 50.0f);
    TEST_CHECK(recorded_events[0].motion.y == 60.0f);
    TEST_CHECK(recorded_events[0].motion.xrel == 3.0f);
    TEST_CHECK(recorded_events[0].motion.yrel == -5.0f);
    return 0;
}

static int Test_batch_preserves_motion_boundaries(void)
{
    Reset_processed_events();
    TEST_CHECK(Queue_mouse_motion(1, 2, 3.0f, 4.0f,
				  1.0f, 2.0f) == 0);
    TEST_CHECK(Queue_mouse_motion(1, 2, 5.0f, 6.0f,
				  3.0f, 4.0f) == 0);
    TEST_CHECK(Queue_user_event() == 0);
    TEST_CHECK(Queue_mouse_motion(1, 2, 7.0f, 8.0f,
				  5.0f, 6.0f) == 0);
    TEST_CHECK(Queue_mouse_motion(9, 2, 9.0f, 10.0f,
				  7.0f, 8.0f) == 0);

    TEST_CHECK(Game_input_process_batch() == 0);
    TEST_CHECK(processed_events == 4);
    TEST_CHECK(recorded_events[0].type == SDL_EVENT_MOUSE_MOTION);
    TEST_CHECK(recorded_events[0].motion.xrel == 4.0f);
    TEST_CHECK(recorded_events[0].motion.yrel == 6.0f);
    TEST_CHECK(recorded_events[1].type == SDL_EVENT_USER);
    TEST_CHECK(recorded_events[2].type == SDL_EVENT_MOUSE_MOTION);
    TEST_CHECK(recorded_events[2].motion.windowID == 1);
    TEST_CHECK(recorded_events[3].type == SDL_EVENT_MOUSE_MOTION);
    TEST_CHECK(recorded_events[3].motion.windowID == 9);
    return 0;
}

int main(void)
{
    if (!SDL_Init(SDL_INIT_EVENTS))
	return 1;

    if (Test_batch_is_bounded() != 0
	|| Test_batch_coalesces_consecutive_mouse_motion() != 0
	|| Test_batch_preserves_motion_boundaries() != 0) {
	SDL_Quit();
	return 1;
    }

    SDL_Quit();
    return 0;
}
