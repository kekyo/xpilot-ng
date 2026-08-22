/* SDL event-batch processing shared by native and scheduled game loops. */

#include "xpclient_sdl.h"

#include "gameinput.h"

#define MAX_INPUT_EVENTS_PER_BATCH 64

extern int Process_event(SDL_Event *evt);

int Game_input_process_batch(void)
{
    SDL_Event event;
    int processed = 0;

    /* Relative mouse mode can replenish the queue continuously.  Bound each
     * batch so network input and output still make progress under mouse load. */
    while (processed < MAX_INPUT_EVENTS_PER_BATCH && SDL_PollEvent(&event)) {
	processed++;
	if (Process_event(&event) == 0)
	    return 1;
    }
    return 0;
}
