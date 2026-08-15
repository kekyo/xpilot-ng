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

#include "xpserver.h"

/* RECORDING WON'T WORK PROPERLY ON WINDOWS BECAUSE OF
 * errno = WSAGetLastError();
 */

int sock_closeRec(sock_t *sock)
{
    if (playback)
	return 0;  /* no recording code checks this value */
    return sock_close(sock);
}


int sock_readRec(sock_t *sock, char *rbuf, int size)
{
    int i;

    if (playback) {
	i = *(playback_shorts++);
	if (i > 0) {
	    memcpy(rbuf, playback_data, (size_t)i);
	    playback_data += i;
	}
	else
	    errno = *playback_errnos++;
	return i;
    }
    i = sock_read(sock, rbuf, size);
    if (record) {
	*(playback_shorts++) = i;
	if (i > 0) {
	    memcpy(playback_data, rbuf, (size_t)i);
	    playback_data += i;
	}
	else
	    *playback_errnos++ = errno;
    }
    return i;
}


int sock_writeRec(sock_t *sock, char *wbuf, int size)
{
    int i;

    if (playback) {
	return size;
/*
  errno = *(playback_ints++);
  return *(playback_ints++);
*/
    }
    i = sock_write(sock, wbuf, size);
    if (record) {
	/*
	 *(playback_ints++) = errno;
	 *(playback_ints++) = i;
	 */
    }
    return i;
}


int Sockbuf_flushRec(sockbuf_t *sbuf)
{
    int			len;

    if (BIT(sbuf->state, SOCKBUF_WRITE) == 0) {
	warn("No flush on non-writable socket buffer");
	warn("(state=%02x,buf=%08x,ptr=%08x,size=%d,len=%d,sock=%d)",
	      sbuf->state, sbuf->buf, sbuf->ptr, sbuf->size, sbuf->len,
	      sbuf->sock);
	return -1;
    }
    if (BIT(sbuf->state, SOCKBUF_LOCK) != 0) {
	warn("No flush on locked socket buffer (0x%02x)", sbuf->state);
	return -1;
    }
    if (BIT(sbuf->state, SOCKBUF_FRAMED) != 0)
	return Sockbuf_flush_framed(sbuf, sock_writeRec);
    if (sbuf->len <= 0) {
	if (sbuf->len < 0) {
	    warn("Write socket buffer length negative");
	    sbuf->len = 0;
	    sbuf->ptr = sbuf->buf;
	}
	return 0;
    }

    errno = 0;
    while ((len = sock_writeRec(&sbuf->sock, sbuf->buf, sbuf->len)) <= 0) {
	if (errno == EINTR) {
	    errno = 0;
	    continue;
	}
	if (errno != EWOULDBLOCK && errno != EAGAIN) {
	    error("Can't write on socket");
	    return -1;
	}
	return 0;
    }
    Sockbuf_advance(sbuf, len);
    return len;
}


int Sockbuf_readRec(sockbuf_t *sbuf)
{
    int			max,
	len;

    if (BIT(sbuf->state, SOCKBUF_READ) == 0) {
	warn("No read from non-readable socket buffer (%d)", sbuf->state);
	return -1;
    }
    if (BIT(sbuf->state, SOCKBUF_LOCK) != 0) {
	return 0;
    }
    if (BIT(sbuf->state, SOCKBUF_FRAMED) != 0)
	return Sockbuf_read_framed(sbuf, sock_readRec);
    if (sbuf->ptr > sbuf->buf) {
	Sockbuf_advance(sbuf, sbuf->ptr - sbuf->buf);
    }
    if ((max = sbuf->size - sbuf->len) <= 0) {
	static int before;
	if (before++ == 0) {
	    warn("Read socket buffer not big enough (%d,%d)",
		  sbuf->size, sbuf->len);
	}
	return -1;
    }
    errno = 0;
    while ((len = sock_readRec(&sbuf->sock, sbuf->buf + sbuf->len, max)) <= 0) {
	if (len == 0)
	    return 0;
	if (errno == EINTR) {
	    errno = 0;
	    continue;
	}
	if (errno != EWOULDBLOCK && errno != EAGAIN) {
	    error("Can't read on socket");
	    return -1;
	}
	return 0;
    }
    sbuf->len += len;

    return sbuf->len;
}


int Sockbuf_writeRec(sockbuf_t *sbuf, char *buf, int len)
{
    if (BIT(sbuf->state, SOCKBUF_WRITE) == 0) {
	warn("No write to non-writable socket buffer");
	return -1;
    }
    if (sbuf->size - sbuf->len < len) {
	if (BIT(sbuf->state,
		SOCKBUF_LOCK | SOCKBUF_FRAMED) != 0) {
	    warn("No write to locked socket buffer (%d,%d,%d,%d)",
		  sbuf->state, sbuf->size, sbuf->len, len);
	    return -1;
	}
	if (Sockbuf_flushRec(sbuf) == -1)
	    return -1;
	if (sbuf->size - sbuf->len < len)
	    return 0;
    }
    memcpy(sbuf->buf + sbuf->len, buf, (size_t)len);
    sbuf->len += len;

    return len;
}
