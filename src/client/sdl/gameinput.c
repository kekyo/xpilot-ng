/* SDL event-batch processing shared by native and scheduled game loops. */

#include "xpclient_sdl.h"

#include "gameinput.h"

#include <float.h>

#define MAX_INPUT_EVENTS_PER_BATCH 64

extern int Process_event(SDL_Event *evt);

static float Clamp_relative_motion(double movement)
{
    if (movement > FLT_MAX)
	return FLT_MAX;
    if (movement < -FLT_MAX)
	return -FLT_MAX;
    return (float)movement;
}

static int Dispatch_mouse_motion(SDL_Event *event, double xrel, double yrel)
{
    event->motion.xrel = Clamp_relative_motion(xrel);
    event->motion.yrel = Clamp_relative_motion(yrel);
    return Process_event(event);
}

int Game_input_process_batch(void)
{
    SDL_Event event;
    SDL_Event pending_motion;
    double pending_xrel = 0.0;
    double pending_yrel = 0.0;
    bool motion_pending = false;
    int processed = 0;

    /* Relative mouse mode can replenish the queue continuously.  Bound each
     * batch so network input and output still make progress under mouse load. */
    while (processed < MAX_INPUT_EVENTS_PER_BATCH && SDL_PollEvent(&event)) {
	processed++;
	if (event.type == SDL_EVENT_MOUSE_MOTION) {
	    if (motion_pending
		&& (pending_motion.motion.windowID != event.motion.windowID
		    || pending_motion.motion.which != event.motion.which)) {
		if (Dispatch_mouse_motion(
			&pending_motion, pending_xrel, pending_yrel) == 0)
		    return 1;
		motion_pending = false;
	    }
	    if (!motion_pending) {
		pending_xrel = event.motion.xrel;
		pending_yrel = event.motion.yrel;
		motion_pending = true;
	    } else {
		pending_xrel += event.motion.xrel;
		pending_yrel += event.motion.yrel;
	    }
	    /* Use the newest absolute position and metadata with the accumulated
	     * relative movement.  Non-motion events remain ordering boundaries. */
	    pending_motion = event;
	    continue;
	}
	if (motion_pending) {
	    if (Dispatch_mouse_motion(
		    &pending_motion, pending_xrel, pending_yrel) == 0)
		return 1;
	    motion_pending = false;
	}
	if (Process_event(&event) == 0)
	    return 1;
    }
    if (motion_pending
	&& Dispatch_mouse_motion(
	    &pending_motion, pending_xrel, pending_yrel) == 0)
	return 1;
    return 0;
}
