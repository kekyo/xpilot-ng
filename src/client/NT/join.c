/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
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

/* kps - this file could be made X11 independent easily. */

#include "xpclient_x11.h"

static int Handle_input(int new_input)
{
#ifndef _WINDOWS
    return x_event(new_input);
#else
    return 0;
#endif
}

#ifndef _WINDOWS
static void Input_loop(void)
{
    fd_set rfds, tfds;
    int max, n, netfd, result, clientfd;
    struct timeval tv;

    if ((result = Net_input()) == -1) {
	error("Bad server input");
	return;
    }
    if (Handle_input(2) == -1)
	return;

    if (Net_flush() == -1)
	return;

    if ((clientfd = ConnectionNumber(dpy)) == -1) {
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
	    if (Handle_input(1) == -1)
		return;

	    if (Net_flush() == -1) {
		error("Bad net flush after X input");
		return;
	    }
	}
	if (FD_ISSET(netfd, &rfds) || result > 1) {
	    if ((result = Net_input()) == -1) {
		warn("Bad net input.  Have a nice day!");
		return;
	    }
	    if (result > 0) {
		/*
		 * Now there's a frame being drawn by the X server.
		 * So we shouldn't try to send more drawing
		 * requests to the X server or it might get
		 * overloaded which could cause problems with
		 * keyboard input.  Besides, we wouldn't even want
		 * to send more drawing requests because there
		 * may arive a more recent frame update soon
		 * and using the CPU now may even slow down the X server
		 * if it is running on the same CPU.
		 * So we only check if the X server has sent any more
		 * keyboard events and then we wait until the X server
		 * has finished the drawing of our current frame.
		 */
		if (Handle_input(1) == -1)
		    return;

		if (Net_flush() == -1) {
		    error("Bad net flush before sync");
		    return;
		}
		XSync(dpy, False);
		if (Handle_input(1) == -1)
		    return;
	    }
	}
    }
}
#endif	/* _WINDOWS */

/* MFC client needs this to be global */
void xpilotShutdown(void)
{
    Net_cleanup();
    Client_cleanup();
    Record_cleanup();
    defaultCleanup();
    aboutCleanup();
    paintdataCleanup();
}

static void sigcatch(int signum)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    xpilotShutdown();
    error("Got signal %d\n", signum);
    exit(1);
}


int Join(Connect_param_t *conpar)
{
    signal(SIGINT, sigcatch);
    signal(SIGTERM, sigcatch);
#ifndef _WINDOWS
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
#endif

    IFWINDOWS( received_self = FALSE );
    IFWINDOWS( Progress("Client_init") );
    if (Client_init(conpar->server_name, conpar->server_version) == -1)
	return -1;

    IFWINDOWS( Progress("Net_init %s", conpar->server_addr) );
    if (Net_init(conpar->server_addr, conpar->login_port) == -1) {
	Client_cleanup();
	return -1;
    }
    IFWINDOWS( Progress("Net_verify '%s'= '%s'",
			conpar->nick_name, conpar->user_name) );
    if (Net_verify(conpar->user_name,
		   conpar->nick_name,
		   conpar->disp_name) == -1) {
	Net_cleanup();
	Client_cleanup();
	return -1;
    }
    IFWINDOWS( Progress("Net_setup") );
    if (Net_setup() == -1) {
	Net_cleanup();
	Client_cleanup();
	return -1;
    }
    IFWINDOWS( Progress("Client_setup") );
    if (Client_setup() == -1) {
	Net_cleanup();
	Client_cleanup();
	return -1;
    }
    IFWINDOWS( Progress("Net_start") );
    if (Net_start() == -1) {
	warn("Network start failed");
	Net_cleanup();
	Client_cleanup();
	return -1;
    }
    IFWINDOWS( Progress("Client_start") );
    if (Client_start() == -1) {
	warn("Window init failed");
	Net_cleanup();
	Client_cleanup();
	return -1;
    }

#ifndef _WINDOWS	/* windows continues to run at this point */
    Input_loop();
    xpilotShutdown();
#endif

    return 0;
}

