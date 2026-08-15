/*
 * XPilot NG, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "session.h"

static int Session_invalid(sockbuf_t *buffer, char *original)
{
    if (buffer != NULL)
	buffer->ptr = original;
    errno = EPROTO;
    return -1;
}

static int Session_complete(sockbuf_t *buffer, char *original)
{
    if (buffer->ptr != buffer->buf + buffer->len)
	return Session_invalid(buffer, original);
    return buffer->ptr - original;
}

int Session_encode_game_open(sockbuf_t *buffer,
                             const session_game_open_t *request)
{
    if (buffer == NULL || request == NULL) {
	errno = EINVAL;
	return -1;
    }
    return Packet_printf(buffer, "%c%hu%hu%hu%c%s%s%s%s%d",
			 PKT_SESSION_OPEN, MAGIC_WORD,
			 request->polygon_version, request->legacy_version,
			 SESSION_GAME, request->user, request->nick,
			 request->display, request->host, request->team);
}

int Session_decode_game_open(sockbuf_t *buffer,
                             session_game_open_t *request)
{
    char *original;
    unsigned char type;
    unsigned char purpose;
    unsigned short magic;

    if (buffer == NULL || request == NULL) {
	errno = EINVAL;
	return -1;
    }
    original = buffer->ptr;
    if (Packet_scanf(buffer, "%c%hu%hu%hu%c%s%s%s%s%d",
		     &type, &magic, &request->polygon_version,
		     &request->legacy_version, &purpose, request->user,
		     request->nick, request->display, request->host,
		     &request->team) != 10
	|| type != PKT_SESSION_OPEN || magic != MAGIC_WORD
	|| purpose != SESSION_GAME)
	return Session_invalid(buffer, original);
    return Session_complete(buffer, original);
}

int Session_encode_control_open(sockbuf_t *buffer,
                                const session_control_open_t *request)
{
    if (buffer == NULL || request == NULL) {
	errno = EINVAL;
	return -1;
    }
    return Packet_printf(buffer, "%c%hu%hu%hu%c%s%c%S",
			 PKT_SESSION_OPEN, MAGIC_WORD,
			 request->polygon_version, request->legacy_version,
			 SESSION_CONTROL, request->user, request->command,
			 request->argument);
}

int Session_decode_control_open(sockbuf_t *buffer,
                                session_control_open_t *request)
{
    char *original;
    unsigned char type;
    unsigned char purpose;
    unsigned short magic;

    if (buffer == NULL || request == NULL) {
	errno = EINVAL;
	return -1;
    }
    original = buffer->ptr;
    if (Packet_scanf(buffer, "%c%hu%hu%hu%c%s%c%S",
		     &type, &magic, &request->polygon_version,
		     &request->legacy_version, &purpose, request->user,
		     &request->command, request->argument) != 8
	|| type != PKT_SESSION_OPEN || magic != MAGIC_WORD
	|| purpose != SESSION_CONTROL)
	return Session_invalid(buffer, original);
    return Session_complete(buffer, original);
}

int Session_encode_reply(sockbuf_t *buffer, const session_reply_t *reply)
{
    if (buffer == NULL || reply == NULL) {
	errno = EINVAL;
	return -1;
    }
    return Packet_printf(buffer, "%c%c%hu%S", PKT_SESSION_REPLY,
			 reply->status, reply->selected_version, reply->reason);
}

int Session_decode_reply(sockbuf_t *buffer, session_reply_t *reply)
{
    char *original;
    unsigned char type;

    if (buffer == NULL || reply == NULL) {
	errno = EINVAL;
	return -1;
    }
    original = buffer->ptr;
    if (Packet_scanf(buffer, "%c%c%hu%S", &type, &reply->status,
		     &reply->selected_version, reply->reason) != 4
	|| type != PKT_SESSION_REPLY)
	return Session_invalid(buffer, original);
    return Session_complete(buffer, original);
}

int Session_encode_control_reply(sockbuf_t *buffer,
                                 const session_control_reply_t *reply)
{
    if (buffer == NULL || reply == NULL) {
	errno = EINVAL;
	return -1;
    }
    return Packet_printf(buffer, "%c%c%c%c%S", PKT_CONTROL_REPLY,
			 reply->command, reply->status, reply->more,
			 reply->payload);
}

int Session_decode_control_reply(sockbuf_t *buffer,
                                 session_control_reply_t *reply)
{
    char *original;
    unsigned char type;

    if (buffer == NULL || reply == NULL) {
	errno = EINVAL;
	return -1;
    }
    original = buffer->ptr;
    if (Packet_scanf(buffer, "%c%c%c%c%S", &type, &reply->command,
		     &reply->status, &reply->more, reply->payload) != 5
	|| type != PKT_CONTROL_REPLY || reply->more > 1)
	return Session_invalid(buffer, original);
    return Session_complete(buffer, original);
}
