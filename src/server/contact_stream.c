/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * Copyright (C) 2026 XPilot Infinity contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpserver.h"

#include "contact_stream.h"

#define CONTACT_STREAM_LIMIT 32
#define CONTACT_STREAM_PER_IP_LIMIT 4
#define CONTACT_STREAM_TIMEOUT 5

typedef enum {
    CONTACT_STREAM_READING,
    CONTACT_STREAM_WRITING
} contact_stream_state_t;

typedef struct {
    bool active;
    contact_stream_state_t state;
    time_t opened_at;
    sock_t socket;
    sockbuf_t input;
    sockbuf_t output;
    char peer_address[SOCK_HOSTNAME_LENGTH];
    int peer_port;
} pending_contact_stream_t;

static sock_t *contact_listener;
static contact_stream_request_fn request_handler;
static bool listener_installed;
static pending_contact_stream_t pending_streams[CONTACT_STREAM_LIMIT];

static bool Contact_stream_output_empty(const sockbuf_t *output)
{
    return output->len == 0 && output->frame_output_len == 0;
}

static void Contact_stream_reset(pending_contact_stream_t *pending)
{
    if (pending->input.buf != NULL)
	Sockbuf_cleanup(&pending->input);
    if (pending->output.buf != NULL)
	Sockbuf_cleanup(&pending->output);
    if (pending->socket.fd != SOCK_FD_INVALID)
	sock_close(&pending->socket);
    memset(pending, 0, sizeof(*pending));
    pending->socket.fd = SOCK_FD_INVALID;
}

static pending_contact_stream_t *Contact_stream_find_free(void)
{
    int i;

    for (i = 0; i < CONTACT_STREAM_LIMIT; i++) {
	if (!pending_streams[i].active)
	    return &pending_streams[i];
    }
    return NULL;
}

static int Contact_stream_count_address(const char *address)
{
    int count = 0;
    int i;

    for (i = 0; i < CONTACT_STREAM_LIMIT; i++) {
	if (pending_streams[i].active
	    && strcmp(pending_streams[i].peer_address, address) == 0)
	    count++;
    }
    return count;
}

static void Contact_stream_accept(socket_handle_t fd, void *arg)
{
    pending_contact_stream_t *pending;
    sock_t accepted;
    char address[SOCK_HOSTNAME_LENGTH];

    UNUSED_PARAM(fd);
    UNUSED_PARAM(arg);

    for (;;) {
	if (sock_accept(contact_listener, &accepted) == SOCK_IS_ERROR) {
	    if (!sock_error_is_temporary(&accepted))
		error("Cannot accept TCP contact stream (%d)",
		      accepted.error.error);
	    return;
	}

	strlcpy(address, sock_get_last_addr(&accepted), sizeof(address));
	pending = Contact_stream_find_free();
	if (pending == NULL
	    || Contact_stream_count_address(address)
	       >= CONTACT_STREAM_PER_IP_LIMIT) {
	    sock_close(&accepted);
	    continue;
	}
	if (sock_set_non_blocking(&accepted, 1) == SOCK_IS_ERROR
	    || sock_set_tcp_nodelay(&accepted, 1) == SOCK_IS_ERROR) {
	    sock_close(&accepted);
	    continue;
	}

	memset(pending, 0, sizeof(*pending));
	pending->active = true;
	pending->state = CONTACT_STREAM_READING;
	pending->opened_at = time(NULL);
	pending->socket = accepted;
	pending->peer_port = sock_get_last_port(&accepted);
	strlcpy(pending->peer_address, address,
		 sizeof(pending->peer_address));
	if (Sockbuf_init(&pending->input, &accepted, SERVER_SEND_SIZE,
			 SOCKBUF_READ | SOCKBUF_FRAMED) == -1
	    || Sockbuf_init(&pending->output, &accepted, MAX_SOCKBUF_SIZE,
			    SOCKBUF_WRITE | SOCKBUF_FRAMED
			    | SOCKBUF_ORDERED) == -1) {
	    Contact_stream_reset(pending);
	    continue;
	}
    }
}

int Contact_stream_init(sock_t *listener, char *address, int port,
			contact_stream_request_fn handler)
{
    int i;

    if (listener == NULL || handler == NULL || port <= 0 || port > 65535) {
	errno = EINVAL;
	return SOCK_IS_ERROR;
    }
    for (i = 0; i < CONTACT_STREAM_LIMIT; i++)
	pending_streams[i].socket.fd = SOCK_FD_INVALID;

    if (sock_open_tcp_listener(listener, address, port,
			       CONTACT_STREAM_LIMIT) == SOCK_IS_ERROR)
	return SOCK_IS_ERROR;
    if (sock_set_non_blocking(listener, 1) == SOCK_IS_ERROR) {
	sock_close(listener);
	return SOCK_IS_ERROR;
    }
    contact_listener = listener;
    request_handler = handler;
    if (install_input(Contact_stream_accept, listener->fd, NULL)
	== SOCK_IS_ERROR) {
	sock_close(listener);
	contact_listener = NULL;
	request_handler = NULL;
	return SOCK_IS_ERROR;
    }
    listener_installed = true;
    return SOCK_IS_OK;
}

static void Contact_stream_process(pending_contact_stream_t *pending)
{
    int status;

    if (time(NULL) >= pending->opened_at + CONTACT_STREAM_TIMEOUT) {
	Contact_stream_reset(pending);
	return;
    }

    if (pending->state == CONTACT_STREAM_READING) {
	status = Sockbuf_read(&pending->input);
	if (status < 0) {
	    Contact_stream_reset(pending);
	    return;
	}
	if (status == 0)
	    return;
	Sockbuf_clear(&pending->output);
	if (request_handler(&pending->input, &pending->output,
			    pending->peer_address,
			    pending->peer_port) <= 0) {
	    Contact_stream_reset(pending);
	    return;
	}
	pending->state = CONTACT_STREAM_WRITING;
    }

    if (Sockbuf_flush(&pending->output) < 0) {
	Contact_stream_reset(pending);
	return;
    }
    if (Contact_stream_output_empty(&pending->output))
	Contact_stream_reset(pending);
}

void Contact_stream_poll(void)
{
    int i;

    for (i = 0; i < CONTACT_STREAM_LIMIT; i++) {
	if (pending_streams[i].active)
	    Contact_stream_process(&pending_streams[i]);
    }
}

void Contact_stream_cleanup(void)
{
    int i;

    for (i = 0; i < CONTACT_STREAM_LIMIT; i++) {
	if (pending_streams[i].active)
	    Contact_stream_reset(&pending_streams[i]);
    }
    if (listener_installed && contact_listener != NULL) {
	remove_input(contact_listener->fd);
	listener_installed = false;
    }
    if (contact_listener != NULL
	&& contact_listener->fd != SOCK_FD_INVALID)
	sock_close(contact_listener);
    contact_listener = NULL;
    request_handler = NULL;
}
