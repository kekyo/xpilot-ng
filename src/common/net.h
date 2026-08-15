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

#ifndef	NET_H
#define	NET_H

#ifndef SOCKLIB_H
/* need sock_t */
#include "socklib.h"
#endif

#define MIN_SOCKBUF_SIZE	1024
#define MAX_SOCKBUF_SIZE	(50*1024)

#define SERVER_RECV_SIZE	MIN_SOCKBUF_SIZE
#define SERVER_SEND_SIZE	(4*1024)

#define CLIENT_SEND_SIZE	SERVER_RECV_SIZE
/* I added 1024 to this because the client can get 4 1035 byte packets
   at once when starting a game (from Handle_setup). Seems there is some
   overhead in storing multiple packets - I had to increase this by at
   least 657 to avoid losing packets on Linux. That's why the change here
   instead of changing the size to 1024 in netserver.c */
#define CLIENT_RECV_SIZE	(SERVER_SEND_SIZE + 1024)

/*
 * Definitions for the states a socket buffer can be in.
 */
#define SOCKBUF_READ		0x01	/* if readable */
#define SOCKBUF_WRITE		0x02	/* if writeable */
#define SOCKBUF_LOCK		0x04	/* if locked against kernel i/o */
#define SOCKBUF_ERROR		0x08	/* if i/o error occurred */
#define SOCKBUF_DGRAM		0x10	/* if datagram socket */
/** Preserve logical packet boundaries on a stream with a 16-bit length. */
#define SOCKBUF_FRAMED		0x20
/** Preserve a staged framed record while a previous record is blocked. */
#define SOCKBUF_ORDERED		0x40

/*
 * Hack: leave some spare room for the last terminating packet
 * of a frame update.
 */
#define SOCKBUF_WRITE_SPARE	8

/*
 * Maximum number of socket i/o retries if datagram socket.
 */
#define MAX_SOCKBUF_RETRIES	2

/*
 * A buffer to reduce the number of system calls made and to reduce
 * the number of network packets.
 */
typedef struct {
    sock_t	sock;		/* socket filedescriptor */
    char	*buf;		/* i/o data buffer */
    int		size;		/* size of buffer */
    int		len;		/* amount of data in buffer (writing/reading) */
    char	*ptr;		/* current position in buffer (reading) */
    int		state;		/* read/write/locked/error status flags */
    /** Partially received two-byte network-order record header. */
    unsigned char frame_header[2];
    /** Number of header bytes received for the current record. */
    int		frame_header_len;
    /** Expected payload size, or -1 before a complete header is available. */
    int		frame_length;
    /** Number of payload bytes received for the current record. */
    int		frame_received;
    /** Wire-format buffer for a partially written framed record. */
    char	*frame_output;
    /** Total wire-format bytes in frame_output. */
    int		frame_output_len;
    /** Number of wire-format bytes already written. */
    int		frame_output_offset;
} sockbuf_t;

/** Socket read/write callback used by the framed transport adapters. */
typedef int (*sockbuf_io_fn)(sock_t *sock, char *buf, int len);

extern int last_packet_of_frame;

int Sockbuf_init(sockbuf_t *sbuf, sock_t *sock, size_t size, int state);
int Sockbuf_cleanup(sockbuf_t *sbuf);
int Sockbuf_clear(sockbuf_t *sbuf);
int Sockbuf_advance(sockbuf_t *sbuf, int len);
int Sockbuf_flush(sockbuf_t *sbuf);
/**
 * Flush one framed record through the supplied socket writer.
 *
 * @param sbuf Framed writable socket buffer.
 * @param writer Function used for the underlying stream write.
 * @return Accepted payload length, zero when blocked or empty, or -1 on error.
 */
int Sockbuf_flush_framed(sockbuf_t *sbuf, sockbuf_io_fn writer);
int Sockbuf_write(sockbuf_t *sbuf, char *buf, int len);
int Sockbuf_read(sockbuf_t *sbuf);
/**
 * Read one complete framed record through the supplied socket reader.
 *
 * @param sbuf Framed readable socket buffer.
 * @param reader Function used for the underlying stream read.
 * @return Complete payload length, zero when incomplete, or -1 on error/EOF.
 */
int Sockbuf_read_framed(sockbuf_t *sbuf, sockbuf_io_fn reader);
int Sockbuf_copy(sockbuf_t *dest, sockbuf_t *src, int len);

int Packet_printf(sockbuf_t *, const char *fmt, ...);
int Packet_scanf(sockbuf_t *, const char *fmt, ...);

#endif
