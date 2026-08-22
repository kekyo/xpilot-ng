#include "test_helpers.h"

#include "gameinput.h"

#include <SDL3/SDL.h>

#include <string.h>

static int processed_events;

int Process_event(SDL_Event *event)
{
    (void)event;
    processed_events++;
    return 1;
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

int main(void)
{
    const int event_count = 2049;
    int remaining_events;

    if (!SDL_Init(SDL_INIT_EVENTS))
	return 1;

    TEST_CHECK(Queue_event_burst(event_count) == 0);
    TEST_CHECK(Game_input_process_batch() == 0);
    remaining_events = Drain_remaining_events();
    TEST_CHECK(processed_events > 0);
    TEST_CHECK(remaining_events > 0);
    TEST_CHECK(processed_events + remaining_events == event_count);

    SDL_Quit();
    return 0;
}
