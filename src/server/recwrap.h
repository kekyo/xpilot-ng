/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2002 Uoti Urpala <uau@users.sourceforge.net>
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

#ifndef RECWRAP_H
#define RECWRAP_H

#include "net.h"

int sock_closeRec(sock_t *sock);
int sock_connectRec(sock_t *sock, char *host, int port);
int sock_get_last_portRec(sock_t *sock);
int sock_receive_anyRec(sock_t *sock, char *rbuf, int size);
int sock_readRec(sock_t *sock, char *rbuf, int size);
int sock_writeRec(sock_t *sock, char *wbuf, int size);
int sock_get_errorRec(sock_t *sock);
int Sockbuf_flushRec(sockbuf_t *sbuf);
int Sockbuf_writeRec(sockbuf_t *sbuf, char *buf, int len);
int Sockbuf_readRec(sockbuf_t *sbuf);

#endif  /* RECWRAP_H */
