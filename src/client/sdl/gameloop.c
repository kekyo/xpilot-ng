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

void Game_loop(void)
{
    fd_set rfds;
    int n, netfd;
    struct timeval tv;
    SDL_Event evt;

    if ((netfd = Net_fd()) == -1) {
        error("Bad net fd");
        return;
    }

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(netfd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 5000; /* wait max 5 ms */

	/*
	 * don't bother about return value, since we wait only 5 ms anyway
	 */
	if (maxMouseTurnsPS > 0)
	    Client_check_pointer_move_interval();

        n = select(netfd + 1, &rfds, NULL, NULL, &tv);
	if (n == -1) {
	    if (errno == EINTR)
		continue;
	    error("Select failed");
	    return;
        }
	if (n > 0) {
	    if (Net_input() == -1) {
		warn("Bad net input.  Have a nice day!");
		return;
	    }
	}
	while (SDL_PollEvent(&evt)) 
	    Process_event(&evt);
    }
}

