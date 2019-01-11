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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "xpclient.h"


int			dgram_one_socket = 0;


int create_dgram_addr_socket(sock_t *sock, char *dotaddr, int port)
{
    static int		saved;
    static sock_t	save_sock;
    int			status = SOCK_IS_ERROR;
    int			i;

    if (saved == 0) {
	if (clientPortStart && (!clientPortEnd || clientPortEnd > 65535))
	    clientPortEnd = 65535;
	if (clientPortEnd && (!clientPortStart || clientPortStart < 1024))
	    clientPortStart = 1024;

	if (port || !clientPortStart || (clientPortStart > clientPortEnd)) {
	    status = sock_open_udp(sock, dotaddr, port);
	    if (status == SOCK_IS_ERROR) {
		error("Cannot create datagram socket (%d)", sock->error.error);
		return -1;
	    }
	}
	else {
	    int found_socket = 0;
	    for (i = clientPortStart; i <= clientPortEnd; i++) {
		status = sock_open_udp(sock, dotaddr, i);
		if (status != SOCK_IS_ERROR) {
		    found_socket = 1;
		    break;
		}
	    }
	    if (found_socket == 0) {
		error("Could not find a usable port in port range [%d,%d]",
		      clientPortStart, clientPortEnd);
		return -1;
	    }
	}

	if (status == SOCK_IS_OK) {
	    if (dgram_one_socket)
		save_sock = *sock;
	}
    } else {
	*sock = save_sock;
	status = SOCK_IS_OK;
    }

    return status;
}

int create_dgram_socket(sock_t *sock, int port)
{
    static char any_addr[] = "0.0.0.0";

    return create_dgram_addr_socket(sock, any_addr, port);
}

void close_dgram_socket(sock_t *sock)
{
    if (!dgram_one_socket)
	sock_close(sock);
}

