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

#include "xpclient_sdl.h"

extern int Process_event(SDL_Event *evt);

static int Poll_input(void)
{
    SDL_Event evt;

    while (SDL_PollEvent(&evt)) {
	if (Process_event(&evt) == 0)
	    return 1;
    }
    return 0;
}

void Game_loop(void)
{
    fd_set rfds;
    int n, netfd, result;
    struct timeval tv;

    if ((result = Net_input()) == -1) {
	error("Bad server input");
	return;
    }
    if (Poll_input())
	return;

    if (Net_flush() == -1)
	return;

    if ((netfd = Net_fd()) == -1) {
	error("Bad socket filedescriptor");
	return;
    }
    Net_key_change();

    while (1) {
	FD_ZERO(&rfds);
	FD_SET(netfd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 5000; /* wait max 5 ms */

	if (maxMouseTurnsPS > 0)
	    Client_check_pointer_move_interval();

        n = select(netfd + 1, &rfds, NULL, NULL, &tv);
	if (n == -1) {
	    if (errno == EINTR)
		continue;
	    error("Select failed");
	    return;
        }
	if (n > 0 || result > 1) {
	    result = Net_input();
	    if (result == -1) {
		warn("Bad net input.  Have a nice day!");
		return;
	    }
	}
	if (Poll_input())
	    return;
	if (Net_flush() == -1) {
	    error("Bad net flush");
	    return;
	}
    }
}
