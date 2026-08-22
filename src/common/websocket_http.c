/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot NG contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpcommon.h"

#include "websocket_http.h"

#define SHA1_BLOCK_SIZE 64
#define SHA1_DIGEST_SIZE 20

typedef struct {
    uint32_t state[5];
    uint64_t byte_count;
    unsigned char block[SHA1_BLOCK_SIZE];
    size_t block_length;
} sha1_context_t;

typedef struct {
    bool host;
    bool upgrade;
    bool connection;
    bool key;
    bool version_seen;
    bool version;
    bool protocol_seen;
    bool protocol;
    bool accept_seen;
    bool extension;
    char key_value[WEBSOCKET_HTTP_KEY_CAPACITY];
    char accept_value[WEBSOCKET_HTTP_ACCEPT_CAPACITY];
} websocket_headers_t;

static uint32_t Sha1_rotate_left(uint32_t value, unsigned count)
{
    return (value << count) | (value >> (32U - count));
}

static uint32_t Sha1_load_be32(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24)
        | ((uint32_t)bytes[1] << 16)
        | ((uint32_t)bytes[2] << 8)
        | (uint32_t)bytes[3];
}

static void Sha1_transform(sha1_context_t *context,
                           const unsigned char block[SHA1_BLOCK_SIZE])
{
    uint32_t words[80];
    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    unsigned index;

    for (index = 0; index < 16; index++)
        words[index] = Sha1_load_be32(block + index * 4U);
    for (index = 16; index < 80; index++)
        words[index] = Sha1_rotate_left(
            words[index - 3] ^ words[index - 8]
                ^ words[index - 14] ^ words[index - 16],
            1);

    for (index = 0; index < 80; index++) {
        uint32_t function;
        uint32_t constant;
        uint32_t temporary;

        if (index < 20) {
            function = (b & c) | ((~b) & d);
            constant = UINT32_C(0x5a827999);
        } else if (index < 40) {
            function = b ^ c ^ d;
            constant = UINT32_C(0x6ed9eba1);
        } else if (index < 60) {
            function = (b & c) | (b & d) | (c & d);
            constant = UINT32_C(0x8f1bbcdc);
        } else {
            function = b ^ c ^ d;
            constant = UINT32_C(0xca62c1d6);
        }
        temporary = Sha1_rotate_left(a, 5) + function + e
            + constant + words[index];
        e = d;
        d = c;
        c = Sha1_rotate_left(b, 30);
        b = a;
        a = temporary;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}

static void Sha1_init(sha1_context_t *context)
{
    memset(context, 0, sizeof(*context));
    context->state[0] = UINT32_C(0x67452301);
    context->state[1] = UINT32_C(0xefcdab89);
    context->state[2] = UINT32_C(0x98badcfe);
    context->state[3] = UINT32_C(0x10325476);
    context->state[4] = UINT32_C(0xc3d2e1f0);
}

static void Sha1_update(sha1_context_t *context,
                        const unsigned char *bytes, size_t length)
{
    context->byte_count += length;
    while (length > 0) {
        size_t available = SHA1_BLOCK_SIZE - context->block_length;
        size_t copied = length < available ? length : available;

        memcpy(context->block + context->block_length, bytes, copied);
        context->block_length += copied;
        bytes += copied;
        length -= copied;
        if (context->block_length == SHA1_BLOCK_SIZE) {
            Sha1_transform(context, context->block);
            context->block_length = 0;
        }
    }
}

static void Sha1_finish(sha1_context_t *context,
                        unsigned char digest[SHA1_DIGEST_SIZE])
{
    uint64_t bit_count = context->byte_count * 8U;
    unsigned char suffix[SHA1_BLOCK_SIZE + 8] = { 0x80 };
    size_t padding = context->block_length < 56
        ? 56 - context->block_length
        : SHA1_BLOCK_SIZE + 56 - context->block_length;
    unsigned index;

    for (index = 0; index < 8; index++)
        suffix[padding + index] =
            (unsigned char)(bit_count >> (56U - index * 8U));
    Sha1_update(context, suffix, padding + 8);
    for (index = 0; index < 5; index++) {
        digest[index * 4] = (unsigned char)(context->state[index] >> 24);
        digest[index * 4 + 1] =
            (unsigned char)(context->state[index] >> 16);
        digest[index * 4 + 2] =
            (unsigned char)(context->state[index] >> 8);
        digest[index * 4 + 3] = (unsigned char)context->state[index];
    }
}

static bool Base64_encode(const unsigned char *input, size_t length,
                          char *output, size_t capacity)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t required = ((length + 2) / 3) * 4;
    size_t input_offset = 0;
    size_t output_offset = 0;

    if (capacity <= required)
        return false;
    while (input_offset < length) {
        size_t remaining = length - input_offset;
        uint32_t value = (uint32_t)input[input_offset] << 16;

        if (remaining > 1)
            value |= (uint32_t)input[input_offset + 1] << 8;
        if (remaining > 2)
            value |= input[input_offset + 2];
        output[output_offset++] = alphabet[(value >> 18) & 0x3f];
        output[output_offset++] = alphabet[(value >> 12) & 0x3f];
        output[output_offset++] = remaining > 1
            ? alphabet[(value >> 6) & 0x3f] : '=';
        output[output_offset++] = remaining > 2
            ? alphabet[value & 0x3f] : '=';
        input_offset += remaining < 3 ? remaining : 3;
    }
    output[output_offset] = '\0';
    return true;
}

static int Base64_value(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A';
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 26;
    if (ch >= '0' && ch <= '9')
        return ch - '0' + 52;
    if (ch == '+')
        return 62;
    if (ch == '/')
        return 63;
    return -1;
}

static bool Base64_decode_nonce(const char *input,
                                unsigned char output[WEBSOCKET_HTTP_NONCE_SIZE])
{
    size_t length = strlen(input);
    size_t input_offset;
    size_t output_offset = 0;

    if (length != 24 || input[22] != '=' || input[23] != '=')
        return false;
    for (input_offset = 0; input_offset < length; input_offset += 4) {
        int first = Base64_value((unsigned char)input[input_offset]);
        int second = Base64_value((unsigned char)input[input_offset + 1]);
        int third = input[input_offset + 2] == '='
            ? 0 : Base64_value((unsigned char)input[input_offset + 2]);
        int fourth = input[input_offset + 3] == '='
            ? 0 : Base64_value((unsigned char)input[input_offset + 3]);
        uint32_t value;

        if (first < 0 || second < 0 || third < 0 || fourth < 0)
            return false;
        if (input_offset + 4 < length
            && (input[input_offset + 2] == '='
                || input[input_offset + 3] == '='))
            return false;
        value = ((uint32_t)first << 18) | ((uint32_t)second << 12)
            | ((uint32_t)third << 6) | (uint32_t)fourth;
        if (output_offset < WEBSOCKET_HTTP_NONCE_SIZE)
            output[output_offset++] = (unsigned char)(value >> 16);
        if (input[input_offset + 2] != '='
            && output_offset < WEBSOCKET_HTTP_NONCE_SIZE)
            output[output_offset++] = (unsigned char)(value >> 8);
        if (input[input_offset + 3] != '='
            && output_offset < WEBSOCKET_HTTP_NONCE_SIZE)
            output[output_offset++] = (unsigned char)value;
    }
    return output_offset == WEBSOCKET_HTTP_NONCE_SIZE;
}

static char Ascii_lower(char ch)
{
    return ch >= 'A' && ch <= 'Z' ? (char)(ch - 'A' + 'a') : ch;
}

static bool Text_equal_case_insensitive(const char *left, size_t left_length,
                                        const char *right)
{
    size_t index;

    if (left_length != strlen(right))
        return false;
    for (index = 0; index < left_length; index++) {
        if (Ascii_lower(left[index]) != Ascii_lower(right[index]))
            return false;
    }
    return true;
}

static bool Text_equal(const char *left, size_t left_length,
                       const char *right)
{
    return left_length == strlen(right)
        && memcmp(left, right, left_length) == 0;
}

static void Trim_ows(const char **value, size_t *length)
{
    while (*length > 0 && (**value == ' ' || **value == '\t')) {
        (*value)++;
        (*length)--;
    }
    while (*length > 0
           && ((*value)[*length - 1] == ' '
               || (*value)[*length - 1] == '\t'))
        (*length)--;
}

static bool Token_list_contains(const char *value, size_t length,
                                const char *expected, bool case_sensitive)
{
    size_t offset = 0;

    while (offset < length) {
        size_t start;
        size_t end;

        while (offset < length
               && (value[offset] == ' ' || value[offset] == '\t'
                   || value[offset] == ','))
            offset++;
        start = offset;
        while (offset < length && value[offset] != ',')
            offset++;
        end = offset;
        while (end > start
               && (value[end - 1] == ' ' || value[end - 1] == '\t'))
            end--;
        if (case_sensitive
            ? Text_equal(value + start, end - start, expected)
            : Text_equal_case_insensitive(
                value + start, end - start, expected))
            return true;
    }
    return false;
}

static const char *Find_header_end(const char *input, size_t length)
{
    size_t index;

    if (length < 4)
        return NULL;
    for (index = 0; index <= length - 4; index++) {
        if (memcmp(input + index, "\r\n\r\n", 4) == 0)
            return input + index + 4;
    }
    return NULL;
}

static const char *Find_crlf(const char *start, const char *end)
{
    const char *cursor;

    for (cursor = start; cursor + 1 < end; cursor++) {
        if (cursor[0] == '\r' && cursor[1] == '\n')
            return cursor;
    }
    return NULL;
}

static bool Copy_header_value(char *destination, size_t capacity,
                              const char *value, size_t length)
{
    if (length == 0 || length >= capacity)
        return false;
    memcpy(destination, value, length);
    destination[length] = '\0';
    return true;
}

static bool Parse_header_lines(const char *start, const char *end,
                               websocket_headers_t *headers,
                               bool server_request)
{
    const char *line = start;

    memset(headers, 0, sizeof(*headers));
    while (line < end - 2) {
        const char *line_end = Find_crlf(line, end);
        const char *colon;
        const char *value;
        size_t name_length;
        size_t value_length;

        if (line_end == NULL || line_end > end || line_end == line)
            return false;
        if (*line == ' ' || *line == '\t')
            return false;
        colon = memchr(line, ':', (size_t)(line_end - line));
        if (colon == NULL || colon == line)
            return false;
        name_length = (size_t)(colon - line);
        value = colon + 1;
        value_length = (size_t)(line_end - value);
        Trim_ows(&value, &value_length);

        if (Text_equal_case_insensitive(line, name_length, "Host")) {
            if (headers->host || value_length == 0)
                return false;
            headers->host = true;
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Upgrade")) {
            headers->upgrade = headers->upgrade
                || Token_list_contains(
                    value, value_length, "websocket", false);
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Connection")) {
            headers->connection = headers->connection
                || Token_list_contains(
                    value, value_length, "Upgrade", false);
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Sec-WebSocket-Key")) {
            if (!server_request || headers->key
                || !Copy_header_value(headers->key_value,
                                      sizeof(headers->key_value),
                                      value, value_length))
                return false;
            headers->key = true;
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Sec-WebSocket-Version")) {
            if (!server_request || headers->version_seen)
                return false;
            headers->version_seen = true;
            headers->version = value_length == 2
                && memcmp(value, "13", 2) == 0;
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Sec-WebSocket-Protocol")) {
            if (headers->protocol_seen)
                return false;
            headers->protocol_seen = true;
            headers->protocol = server_request
                ? Token_list_contains(value, value_length,
                                      WEBSOCKET_HTTP_SUBPROTOCOL, true)
                : Text_equal(
                      value, value_length, WEBSOCKET_HTTP_SUBPROTOCOL);
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Sec-WebSocket-Accept")) {
            if (server_request || headers->accept_seen
                || !Copy_header_value(headers->accept_value,
                                      sizeof(headers->accept_value),
                                      value, value_length))
                return false;
            headers->accept_seen = true;
        } else if (Text_equal_case_insensitive(
                       line, name_length, "Sec-WebSocket-Extensions")) {
            headers->extension = true;
        }
        line = line_end + 2;
    }
    return line == end - 2;
}

bool WebSocket_http_create_accept(const char *key, char *accept,
                                  size_t capacity)
{
    static const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char nonce[WEBSOCKET_HTTP_NONCE_SIZE];
    unsigned char digest[SHA1_DIGEST_SIZE];
    sha1_context_t context;

    if (key == NULL || accept == NULL
        || capacity < WEBSOCKET_HTTP_ACCEPT_CAPACITY
        || !Base64_decode_nonce(key, nonce))
        return false;
    Sha1_init(&context);
    Sha1_update(&context, (const unsigned char *)key, strlen(key));
    Sha1_update(&context, (const unsigned char *)guid, sizeof(guid) - 1);
    Sha1_finish(&context, digest);
    return Base64_encode(digest, sizeof(digest), accept, capacity);
}

int WebSocket_http_create_client_request(
    char *output, size_t capacity, size_t *length, const char *host, int port,
    const unsigned char nonce[WEBSOCKET_HTTP_NONCE_SIZE],
    char *expected_accept, size_t expected_accept_capacity)
{
    char key[WEBSOCKET_HTTP_KEY_CAPACITY];
    int written;

    if (output == NULL || capacity == 0 || length == NULL
        || host == NULL || host[0] == '\0' || strchr(host, '\r') != NULL
        || strchr(host, '\n') != NULL || port <= 0 || port > 65535
        || nonce == NULL || expected_accept == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (!Base64_encode(nonce, WEBSOCKET_HTTP_NONCE_SIZE,
                       key, sizeof(key))
        || !WebSocket_http_create_accept(
            key, expected_accept, expected_accept_capacity)) {
        errno = EINVAL;
        return -1;
    }
    written = snprintf(
        output, capacity,
        "GET " WEBSOCKET_HTTP_RESOURCE " HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: " WEBSOCKET_HTTP_SUBPROTOCOL "\r\n"
        "\r\n",
        host, port, key);
    if (written < 0 || (size_t)written >= capacity) {
        errno = EMSGSIZE;
        return -1;
    }
    *length = (size_t)written;
    return 0;
}

websocket_http_status_t WebSocket_http_parse_server_request(
    const char *input, size_t length, websocket_http_server_request_t *request)
{
    const char *header_end;
    const char *request_line_end;
    websocket_headers_t headers;

    if (input == NULL || request == NULL)
        return WEBSOCKET_HTTP_BAD_REQUEST;
    header_end = Find_header_end(input, length);
    if (header_end == NULL)
        return WEBSOCKET_HTTP_INCOMPLETE;
    memset(request, 0, sizeof(*request));
    request->consumed = (size_t)(header_end - input);
    request_line_end = Find_crlf(input, header_end);
    if (request_line_end == NULL || request_line_end >= header_end)
        return WEBSOCKET_HTTP_BAD_REQUEST;
    if ((size_t)(request_line_end - input)
            != strlen("GET " WEBSOCKET_HTTP_RESOURCE " HTTP/1.1")
        || memcmp(input, "GET " WEBSOCKET_HTTP_RESOURCE " HTTP/1.1",
                  strlen("GET " WEBSOCKET_HTTP_RESOURCE " HTTP/1.1"))
            != 0) {
        if ((size_t)(request_line_end - input) > strlen("GET ")
            && memcmp(input, "GET ", strlen("GET ")) == 0)
            return WEBSOCKET_HTTP_NOT_FOUND;
        return WEBSOCKET_HTTP_BAD_REQUEST;
    }
    if (!Parse_header_lines(request_line_end + 2, header_end,
                            &headers, true))
        return WEBSOCKET_HTTP_BAD_REQUEST;
    if (!headers.version_seen || !headers.version)
        return WEBSOCKET_HTTP_UPGRADE_REQUIRED;
    if (!headers.host || !headers.upgrade || !headers.connection
        || !headers.key || !headers.protocol
        || !WebSocket_http_create_accept(
            headers.key_value, request->accept, sizeof(request->accept)))
        return WEBSOCKET_HTTP_BAD_REQUEST;
    return WEBSOCKET_HTTP_SWITCHING_PROTOCOLS;
}

int WebSocket_http_create_server_response(
    char *output, size_t capacity, size_t *length,
    websocket_http_status_t status, const char *accept)
{
    const char *format;
    int written;

    if (output == NULL || capacity == 0 || length == NULL) {
        errno = EINVAL;
        return -1;
    }
    switch (status) {
    case WEBSOCKET_HTTP_SWITCHING_PROTOCOLS:
        if (accept == NULL
            || strlen(accept) != WEBSOCKET_HTTP_ACCEPT_CAPACITY - 1) {
            errno = EINVAL;
            return -1;
        }
        written = snprintf(
            output, capacity,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "Sec-WebSocket-Protocol: " WEBSOCKET_HTTP_SUBPROTOCOL "\r\n"
            "\r\n", accept);
        break;
    case WEBSOCKET_HTTP_BAD_REQUEST:
        format = "HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\nContent-Length: 0\r\n\r\n";
        written = snprintf(output, capacity, "%s", format);
        break;
    case WEBSOCKET_HTTP_NOT_FOUND:
        format = "HTTP/1.1 404 Not Found\r\n"
            "Connection: close\r\nContent-Length: 0\r\n\r\n";
        written = snprintf(output, capacity, "%s", format);
        break;
    case WEBSOCKET_HTTP_UPGRADE_REQUIRED:
        format = "HTTP/1.1 426 Upgrade Required\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Connection: close\r\nContent-Length: 0\r\n\r\n";
        written = snprintf(output, capacity, "%s", format);
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    if (written < 0 || (size_t)written >= capacity) {
        errno = EMSGSIZE;
        return -1;
    }
    *length = (size_t)written;
    return 0;
}

websocket_http_status_t WebSocket_http_parse_client_response(
    const char *input, size_t length, const char *expected_accept,
    websocket_http_client_response_t *response)
{
    const char *header_end;
    const char *status_line_end;
    websocket_headers_t headers;

    if (input == NULL || expected_accept == NULL || response == NULL)
        return WEBSOCKET_HTTP_BAD_REQUEST;
    header_end = Find_header_end(input, length);
    if (header_end == NULL)
        return WEBSOCKET_HTTP_INCOMPLETE;
    memset(response, 0, sizeof(*response));
    response->consumed = (size_t)(header_end - input);
    status_line_end = Find_crlf(input, header_end);
    if (status_line_end == NULL || status_line_end >= header_end
        || (size_t)(status_line_end - input) < strlen("HTTP/1.1 101")
        || memcmp(input, "HTTP/1.1 101", strlen("HTTP/1.1 101")) != 0
        || (input[strlen("HTTP/1.1 101")] != ' '
            && input[strlen("HTTP/1.1 101")] != '\r'))
        return WEBSOCKET_HTTP_BAD_REQUEST;
    if (!Parse_header_lines(status_line_end + 2, header_end,
                            &headers, false)
        || !headers.upgrade || !headers.connection || !headers.protocol_seen
        || !headers.protocol || headers.extension || !headers.accept_seen
        || strcmp(headers.accept_value, expected_accept) != 0)
        return WEBSOCKET_HTTP_BAD_REQUEST;
    return WEBSOCKET_HTTP_SWITCHING_PROTOCOLS;
}
