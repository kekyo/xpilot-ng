/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include <limits.h>
#include <stdint.h>

#include "session_protocol.h"

typedef enum {
    SESSION_MESSAGE_OPEN = 1,
    SESSION_MESSAGE_GAME_REPLY = 2,
    SESSION_MESSAGE_CONTROL_REPLY = 3,
    SESSION_MESSAGE_RESUME_REPLY = 4
} session_message_t;

typedef struct {
    unsigned char *payload;
    size_t capacity;
    size_t offset;
} session_writer_t;

typedef struct {
    const unsigned char *payload;
    size_t length;
    size_t offset;
} session_reader_t;

static int Session_protocol_invalid(int error_number)
{
    errno = error_number;
    return -1;
}

static int Session_writer_reserve(session_writer_t *writer, size_t length)
{
    if (length > writer->capacity - writer->offset)
        return Session_protocol_invalid(EMSGSIZE);
    return 0;
}

static int Session_writer_byte(session_writer_t *writer, unsigned value)
{
    if (Session_writer_reserve(writer, 1) == -1)
        return -1;
    writer->payload[writer->offset++] = (unsigned char)value;
    return 0;
}

static int Session_writer_u16(session_writer_t *writer, unsigned value)
{
    if (value > UINT16_MAX)
        return Session_protocol_invalid(EINVAL);
    if (Session_writer_reserve(writer, 2) == -1)
        return -1;
    writer->payload[writer->offset++] = (unsigned char)(value >> 8);
    writer->payload[writer->offset++] = (unsigned char)value;
    return 0;
}

static int Session_writer_u32(session_writer_t *writer, uint32_t value)
{
    if (Session_writer_reserve(writer, 4) == -1)
        return -1;
    writer->payload[writer->offset++] = (unsigned char)(value >> 24);
    writer->payload[writer->offset++] = (unsigned char)(value >> 16);
    writer->payload[writer->offset++] = (unsigned char)(value >> 8);
    writer->payload[writer->offset++] = (unsigned char)value;
    return 0;
}

static int Session_writer_string(session_writer_t *writer, const char *value,
                                 size_t maximum_length)
{
    const char *terminator;
    size_t length;

    terminator = memchr(value, '\0', maximum_length);
    if (terminator == NULL)
        return Session_protocol_invalid(EINVAL);
    length = (size_t)(terminator - value) + 1;
    if (Session_writer_reserve(writer, length) == -1)
        return -1;
    memcpy(writer->payload + writer->offset, value, length);
    writer->offset += length;
    return 0;
}

static int Session_writer_header(session_writer_t *writer,
                                 session_message_t message)
{
    return Session_writer_u16(writer, MAGIC_WORD)
        || Session_writer_byte(writer, SESSION_PROTOCOL_VERSION)
        || Session_writer_byte(writer, message)
        ? -1 : 0;
}

static int Session_writer_begin(session_writer_t *writer, char *destination,
                                size_t capacity, size_t *length,
                                const void *value)
{
    if (destination == NULL || capacity == 0 || length == NULL
        || value == NULL)
        return Session_protocol_invalid(EINVAL);
    *length = 0;
    writer->payload = (unsigned char *)destination;
    writer->capacity = capacity;
    writer->offset = 0;
    return 0;
}

static int Session_writer_finish(const session_writer_t *writer,
                                 size_t *length)
{
    *length = writer->offset;
    return 0;
}

static int Session_reader_reserve(session_reader_t *reader, size_t length)
{
    if (length > reader->length - reader->offset)
        return Session_protocol_invalid(EPROTO);
    return 0;
}

static int Session_reader_byte(session_reader_t *reader,
                               unsigned char *value)
{
    if (Session_reader_reserve(reader, 1) == -1)
        return -1;
    *value = reader->payload[reader->offset++];
    return 0;
}

static int Session_reader_u16(session_reader_t *reader,
                              unsigned short *value)
{
    unsigned high;
    unsigned low;

    if (Session_reader_reserve(reader, 2) == -1)
        return -1;
    high = reader->payload[reader->offset++];
    low = reader->payload[reader->offset++];
    *value = (unsigned short)((high << 8) | low);
    return 0;
}

static int Session_reader_u32(session_reader_t *reader, uint32_t *value)
{
    uint32_t result;

    if (Session_reader_reserve(reader, 4) == -1)
        return -1;
    result = (uint32_t)reader->payload[reader->offset++] << 24;
    result |= (uint32_t)reader->payload[reader->offset++] << 16;
    result |= (uint32_t)reader->payload[reader->offset++] << 8;
    result |= reader->payload[reader->offset++];
    *value = result;
    return 0;
}

static int Session_reader_string(session_reader_t *reader, char *destination,
                                 size_t capacity)
{
    const unsigned char *start;
    const unsigned char *terminator;
    size_t available;
    size_t length;

    available = reader->length - reader->offset;
    start = reader->payload + reader->offset;
    terminator = memchr(start, '\0', available);
    if (terminator == NULL)
        return Session_protocol_invalid(EPROTO);
    length = (size_t)(terminator - start) + 1;
    if (length > capacity)
        return Session_protocol_invalid(EPROTO);
    memcpy(destination, start, length);
    reader->offset += length;
    return 0;
}

static int Session_reader_begin(session_reader_t *reader,
                                const char *payload, size_t length,
                                void *value, session_message_t expected)
{
    unsigned char version;
    unsigned char message;
    unsigned short magic;

    if (payload == NULL || value == NULL)
        return Session_protocol_invalid(EINVAL);
    if (length == 0 || length > SESSION_PROTOCOL_MAX_RECORD_SIZE)
        return Session_protocol_invalid(EPROTO);
    reader->payload = (const unsigned char *)payload;
    reader->length = length;
    reader->offset = 0;
    if (Session_reader_u16(reader, &magic) == -1
        || Session_reader_byte(reader, &version) == -1
        || Session_reader_byte(reader, &message) == -1)
        return -1;
    if (magic != MAGIC_WORD
        || version != SESSION_PROTOCOL_VERSION || message != expected)
        return Session_protocol_invalid(EPROTO);
    return 0;
}

static int Session_reader_finish(const session_reader_t *reader)
{
    if (reader->offset != reader->length)
        return Session_protocol_invalid(EPROTO);
    return 0;
}

int Session_protocol_encode_game_open(
    char *destination, size_t capacity, size_t *length,
    const session_game_request_t *request)
{
    session_writer_t writer;

    if (Session_writer_begin(
            &writer, destination, capacity, length, request) == -1)
        return -1;
    if (request->polygon_version == 0 || request->legacy_version == 0
        || request->team < 0 || request->team > UINT16_MAX)
        return Session_protocol_invalid(EINVAL);
    if (Session_writer_header(&writer, SESSION_MESSAGE_OPEN) == -1
        || Session_writer_byte(&writer, SESSION_PURPOSE_GAME) == -1
        || Session_writer_u16(&writer, request->polygon_version) == -1
        || Session_writer_u16(&writer, request->legacy_version) == -1
        || Session_writer_string(
               &writer, request->user, sizeof(request->user)) == -1
        || Session_writer_string(
               &writer, request->nick, sizeof(request->nick)) == -1
        || Session_writer_string(
               &writer, request->display, sizeof(request->display)) == -1
        || Session_writer_string(
               &writer, request->host, sizeof(request->host)) == -1
        || Session_writer_u16(&writer, (unsigned)request->team) == -1)
        return -1;
    return Session_writer_finish(&writer, length);
}

int Session_protocol_encode_control_open(
    char *destination, size_t capacity, size_t *length,
    const session_control_request_t *request)
{
    session_writer_t writer;

    if (Session_writer_begin(
            &writer, destination, capacity, length, request) == -1)
        return -1;
    if (request->polygon_version == 0 || request->legacy_version == 0)
        return Session_protocol_invalid(EINVAL);
    if (Session_writer_header(&writer, SESSION_MESSAGE_OPEN) == -1
        || Session_writer_byte(&writer, SESSION_PURPOSE_CONTROL) == -1
        || Session_writer_u16(&writer, request->polygon_version) == -1
        || Session_writer_u16(&writer, request->legacy_version) == -1
        || Session_writer_string(
               &writer, request->user, sizeof(request->user)) == -1
        || Session_writer_byte(&writer, request->command) == -1
        || Session_writer_string(
               &writer, request->argument, sizeof(request->argument)) == -1)
        return -1;
    return Session_writer_finish(&writer, length);
}

int Session_protocol_encode_resume_open(
    char *destination, size_t capacity, size_t *length,
    const session_resume_request_t *request)
{
    session_writer_t writer;
    int i;

    if (Session_writer_begin(
            &writer, destination, capacity, length, request) == -1)
        return -1;
    if (Session_writer_header(&writer, SESSION_MESSAGE_OPEN) == -1
        || Session_writer_byte(&writer, SESSION_PURPOSE_RESUME) == -1)
        return -1;
    for (i = 0; i < SESSION_TOKEN_WORDS; i++) {
        if (Session_writer_u32(&writer, request->token.words[i]) == -1)
            return -1;
    }
    return Session_writer_finish(&writer, length);
}

int Session_protocol_decode_open(const char *payload, size_t length,
                                 session_open_t *open)
{
    session_reader_t reader;
    unsigned char purpose;
    unsigned short team;

    if (Session_reader_begin(
            &reader, payload, length, open, SESSION_MESSAGE_OPEN) == -1)
        return -1;
    memset(open, 0, sizeof(*open));
    if (Session_reader_byte(&reader, &purpose) == -1)
        return -1;
    if (purpose == SESSION_PURPOSE_GAME) {
        open->purpose = SESSION_PURPOSE_GAME;
        if (Session_reader_u16(
                &reader, &open->request.game.polygon_version) == -1
            || Session_reader_u16(
                   &reader, &open->request.game.legacy_version) == -1
            || Session_reader_string(
                   &reader, open->request.game.user,
                   sizeof(open->request.game.user)) == -1
            || Session_reader_string(
                   &reader, open->request.game.nick,
                   sizeof(open->request.game.nick)) == -1
            || Session_reader_string(
                   &reader, open->request.game.display,
                   sizeof(open->request.game.display)) == -1
            || Session_reader_string(
                   &reader, open->request.game.host,
                   sizeof(open->request.game.host)) == -1
            || Session_reader_u16(&reader, &team) == -1)
            return -1;
        if (open->request.game.polygon_version == 0
            || open->request.game.legacy_version == 0)
            return Session_protocol_invalid(EPROTO);
        open->request.game.team = team;
    } else if (purpose == SESSION_PURPOSE_CONTROL) {
        open->purpose = SESSION_PURPOSE_CONTROL;
        if (Session_reader_u16(
                &reader, &open->request.control.polygon_version) == -1
            || Session_reader_u16(
                   &reader, &open->request.control.legacy_version) == -1
            || Session_reader_string(
                   &reader, open->request.control.user,
                   sizeof(open->request.control.user)) == -1
            || Session_reader_byte(
                   &reader, &open->request.control.command) == -1
            || Session_reader_string(
                   &reader, open->request.control.argument,
                   sizeof(open->request.control.argument)) == -1)
            return -1;
        if (open->request.control.polygon_version == 0
            || open->request.control.legacy_version == 0)
            return Session_protocol_invalid(EPROTO);
    } else if (purpose == SESSION_PURPOSE_RESUME) {
        int i;

        open->purpose = SESSION_PURPOSE_RESUME;
        for (i = 0; i < SESSION_TOKEN_WORDS; i++) {
            if (Session_reader_u32(
                    &reader, &open->request.resume.token.words[i]) == -1)
                return -1;
        }
    } else {
        return Session_protocol_invalid(EPROTO);
    }
    return Session_reader_finish(&reader);
}

int Session_protocol_encode_game_reply(
    char *destination, size_t capacity, size_t *length,
    const session_game_reply_t *reply)
{
    session_writer_t writer;

    if (Session_writer_begin(
            &writer, destination, capacity, length, reply) == -1)
        return -1;
    if (Session_writer_header(&writer, SESSION_MESSAGE_GAME_REPLY) == -1
        || Session_writer_byte(&writer, reply->status) == -1
        || Session_writer_u16(&writer, reply->selected_version) == -1
        || Session_writer_string(
               &writer, reply->reason, sizeof(reply->reason)) == -1)
        return -1;
    return Session_writer_finish(&writer, length);
}

int Session_protocol_decode_game_reply(const char *payload, size_t length,
                                       session_game_reply_t *reply)
{
    session_reader_t reader;

    if (Session_reader_begin(
            &reader, payload, length, reply,
            SESSION_MESSAGE_GAME_REPLY) == -1)
        return -1;
    memset(reply, 0, sizeof(*reply));
    if (Session_reader_byte(&reader, &reply->status) == -1
        || Session_reader_u16(&reader, &reply->selected_version) == -1
        || Session_reader_string(
               &reader, reply->reason, sizeof(reply->reason)) == -1)
        return -1;
    return Session_reader_finish(&reader);
}

int Session_protocol_encode_control_reply(
    char *destination, size_t capacity, size_t *length,
    const session_control_reply_t *reply)
{
    session_writer_t writer;

    if (Session_writer_begin(
            &writer, destination, capacity, length, reply) == -1)
        return -1;
    if (reply->more > 1)
        return Session_protocol_invalid(EINVAL);
    if (Session_writer_header(&writer, SESSION_MESSAGE_CONTROL_REPLY) == -1
        || Session_writer_byte(&writer, reply->command) == -1
        || Session_writer_byte(&writer, reply->status) == -1
        || Session_writer_byte(&writer, reply->more) == -1
        || Session_writer_string(
               &writer, reply->payload, sizeof(reply->payload)) == -1)
        return -1;
    return Session_writer_finish(&writer, length);
}

int Session_protocol_decode_control_reply(
    const char *payload, size_t length, session_control_reply_t *reply)
{
    session_reader_t reader;

    if (Session_reader_begin(
            &reader, payload, length, reply,
            SESSION_MESSAGE_CONTROL_REPLY) == -1)
        return -1;
    memset(reply, 0, sizeof(*reply));
    if (Session_reader_byte(&reader, &reply->command) == -1
        || Session_reader_byte(&reader, &reply->status) == -1
        || Session_reader_byte(&reader, &reply->more) == -1
        || Session_reader_string(
               &reader, reply->payload, sizeof(reply->payload)) == -1)
        return -1;
    if (reply->more > 1)
        return Session_protocol_invalid(EPROTO);
    return Session_reader_finish(&reader);
}

int Session_protocol_encode_resume_reply(
    char *destination, size_t capacity, size_t *length,
    const session_resume_reply_t *reply)
{
    session_writer_t writer;

    if (Session_writer_begin(
            &writer, destination, capacity, length, reply) == -1)
        return -1;
    if (Session_writer_header(&writer, SESSION_MESSAGE_RESUME_REPLY) == -1
        || Session_writer_byte(&writer, reply->status) == -1
        || Session_writer_string(
               &writer, reply->reason, sizeof(reply->reason)) == -1)
        return -1;
    return Session_writer_finish(&writer, length);
}

int Session_protocol_decode_resume_reply(
    const char *payload, size_t length, session_resume_reply_t *reply)
{
    session_reader_t reader;

    if (Session_reader_begin(
            &reader, payload, length, reply,
            SESSION_MESSAGE_RESUME_REPLY) == -1)
        return -1;
    memset(reply, 0, sizeof(*reply));
    if (Session_reader_byte(&reader, &reply->status) == -1
        || Session_reader_string(
               &reader, reply->reason, sizeof(reply->reason)) == -1)
        return -1;
    return Session_reader_finish(&reader);
}
