/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <X11/Xlib.h>

#include "xpclient_sdl.h"

extern int Process_event(SDL_Event *evt);

static int Poll_input(void)
{
    SDL_Event evt;
    while (SDL_PollEvent(&evt))
	if (Process_event(&evt) == 0) 
	    return 1;
    return 0;
}

/*
 * This is a Game_loop that uses X specific hacks to improve
 * responsiveness. Basically it uses the same mechanism as the 
 * X client to listen to network and user input.
 */
void Game_loop(void)
{
    fd_set rfds, tfds;
    int max, n, netfd, result, clientfd;
    struct timeval tv;
    SDL_SysWMinfo info;

    SDL_VERSION(&info.version);
    if (!SDL_GetWMInfo(&info)) {
	error("SDL_GetWMInfo not supported");
	return;
    }

    if ((result = Net_input()) == -1) {
	error("Bad server input");
	return;
    }
    if (Poll_input())
	return;

    if (Net_flush() == -1)
	return;

    if ((clientfd = ConnectionNumber(info.info.x11.display)) == -1) {
	error("Bad client filedescriptor");
	return;
    }
    if ((netfd = Net_fd()) == -1) {
	error("Bad socket filedescriptor");
	return;
    }
    Net_key_change();
    FD_ZERO(&rfds);
    FD_SET(clientfd, &rfds);
    FD_SET(netfd, &rfds);
    max = (clientfd > netfd) ? clientfd : netfd;
    for (tfds = rfds; ; rfds = tfds) {

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if (maxMouseTurnsPS > 0) {
	    int t = Client_check_pointer_move_interval();

	    assert(t > 0);
	    tv.tv_sec = t / 1000000;
	    tv.tv_usec = t % 1000000;
	}

	if ((n = select(max + 1, &rfds, NULL, NULL, &tv)) == -1) {
	    if (errno == EINTR)
		continue;
	    error("Select failed");
	    return;
	}
	
	if (n == 0) {
	    if (maxMouseTurnsPS > 0 &&
		cumulativeMouseMovement != 0)
		continue;

	    if (result <= 1) {
		warn("No response from server");
		continue;
	    }
	}
	if (FD_ISSET(clientfd, &rfds)) {
	    if (Poll_input())
		return;
	    if (Net_flush() == -1) {
		error("Bad net flush after input");
		return;
	    }
	}
	if (FD_ISSET(netfd, &rfds) || result > 1) {
	    if ((result = Net_input()) == -1) {
		warn("Bad net input.  Have a nice day!");
		return;
	    }
	    if (result > 0) {
		if (Poll_input())
		    return;
		if (Net_flush() == -1) {
		    error("Bad net flush");
		    return;
		}
		if (Poll_input())
		    return;
	    }
	}
    }
}
