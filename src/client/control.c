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
 */

#include "xpclient.h"

#define CONTROL_SESSION_TIMEOUT 5

static double Control_now(void)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    return now.tv_sec + now.tv_usec / 1000000.0;
}

static int Control_wait(sock_t *socket, bool writable, double deadline)
{
    fd_set read_fds;
    fd_set write_fds;

    for (;;) {
	double remaining = deadline - Control_now();
	struct timeval timeout;
	int status;

	if (remaining <= 0.0) {
	    errno = ETIMEDOUT;
	    return -1;
	}
	timeout.tv_sec = (long)remaining;
	timeout.tv_usec = (long)((remaining - timeout.tv_sec) * 1000000.0);
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	if (writable)
	    FD_SET(socket->fd, &write_fds);
	else
	    FD_SET(socket->fd, &read_fds);

	status = select(socket->fd + 1,
			writable ? NULL : &read_fds,
			writable ? &write_fds : NULL, NULL, &timeout);
	if (status > 0)
	    return 0;
	if (status == 0) {
	    errno = ETIMEDOUT;
	    return -1;
	}
	if (errno != EINTR)
	    return -1;
    }
}

static bool Control_output_pending(const sockbuf_t *output)
{
    return output->len > 0 || output->frame_output_len > 0;
}

static int Control_flush(sockbuf_t *output, double deadline)
{
    while (Control_output_pending(output)) {
	if (Sockbuf_flush(output) < 0)
	    return -1;
	if (Control_output_pending(output)
	    && Control_wait(&output->sock, true, deadline) == -1)
	    return -1;
    }
    return 0;
}

static int Control_read(sockbuf_t *input, double deadline)
{
    Sockbuf_clear(input);
    for (;;) {
	int status;

	if (Control_wait(&input->sock, false, deadline) == -1)
	    return -1;
	status = Sockbuf_read(input);
	if (status != 0)
	    return status;
    }
}

static void Control_print_reply(FILE *output, unsigned char command,
				const char *payload)
{
    size_t length = strlen(payload);

    if (length == 0)
	return;
    fputs(payload, output);
    if (command != REPORT_STATUS_pack && payload[length - 1] != '\n')
	fputc('\n', output);
}

int Control_request(const char *server, int port, const char *user,
		    unsigned char command, const char *argument,
		    FILE *output)
{
    session_control_open_t request;
    session_control_reply_t reply;
    sock_t socket;
    sockbuf_t input;
    sockbuf_t write_buffer;
    double deadline;
    int result = -1;
    int input_initialized = false;
    int output_initialized = false;

    if (server == NULL || user == NULL || argument == NULL || output == NULL
	|| port <= 0 || port > 65535) {
	errno = EINVAL;
	return -1;
    }

    sock_init(&socket);
    memset(&input, 0, sizeof(input));
    memset(&write_buffer, 0, sizeof(write_buffer));
    if (sock_open_tcp_bound(&socket, NULL, 0) == SOCK_IS_ERROR
	|| sock_connect_with_timeout(&socket, (char *)server, port,
				     CONTROL_SESSION_TIMEOUT) == SOCK_IS_ERROR) {
	error("Can't connect to server %s on port %d", server, port);
	goto cleanup;
    }
    if (sock_set_tcp_nodelay(&socket, 1) == SOCK_IS_ERROR) {
	error("Can't configure TCP control connection");
	goto cleanup;
    }
    if (Sockbuf_init(&input, &socket, CLIENT_RECV_SIZE,
		     SOCKBUF_READ | SOCKBUF_FRAMED) == -1)
	goto cleanup;
    input_initialized = true;
    if (Sockbuf_init(&write_buffer, &socket, CLIENT_SEND_SIZE,
		     SOCKBUF_WRITE | SOCKBUF_FRAMED | SOCKBUF_ORDERED) == -1)
	goto cleanup;
    output_initialized = true;

    memset(&request, 0, sizeof(request));
    request.polygon_version = POLYGON_VERSION;
    request.legacy_version = OLD_VERSION;
    strlcpy(request.user, user, sizeof(request.user));
    request.command = command;
    strlcpy(request.argument, argument, sizeof(request.argument));

    deadline = Control_now() + CONTROL_SESSION_TIMEOUT;
    if (Session_encode_control_open(&write_buffer, &request) <= 0
	|| Control_flush(&write_buffer, deadline) == -1) {
	error("Can't send TCP control request");
	goto cleanup;
    }

    for (;;) {
	if (Control_read(&input, deadline) <= 0
	    || Session_decode_control_reply(&input, &reply) <= 0
	    || reply.command != command) {
	    error("Can't read TCP control response");
	    goto cleanup;
	}
	Control_print_reply(output, command, reply.payload);
	if (!reply.more) {
	    result = reply.status == SUCCESS ? 0 : -1;
	    break;
	}
	deadline = Control_now() + CONTROL_SESSION_TIMEOUT;
    }
    fflush(output);

cleanup:
    if (output_initialized)
	Sockbuf_cleanup(&write_buffer);
    if (input_initialized)
	Sockbuf_cleanup(&input);
    if (socket.fd != SOCK_FD_INVALID)
	sock_close(&socket);
    return result;
}

static char *Control_argument(char *line)
{
    char *argument = line;

    while (*argument != '\0' && !isspace((unsigned char)*argument))
	argument++;
    if (*argument != '\0')
	*argument++ = '\0';
    while (isspace((unsigned char)*argument))
	argument++;
    return argument;
}

static void Control_help(void)
{
    puts("Supported commands:\n"
	 "  status                 Show server and player status.\n"
	 "  options                Show visible server options.\n"
	 "  tune NAME:VALUE        Change one server option.\n"
	 "  kick NAME              Disconnect one player.\n"
	 "  message TEXT           Broadcast an administrator message.\n"
	 "  lock on|off            Lock or unlock new player admission.\n"
	 "  shutdown REASON        Shut down the server.\n"
	 "  help                   Show this help.\n"
	 "  quit                   Exit the text interface.");
}

int Control_interactive(const char *server, int port, const char *user)
{
    char line[MSG_LEN];

    Control_help();
    for (;;) {
	char *argument;
	unsigned char command;

	fputs("xpilot control> ", stdout);
	fflush(stdout);
	if (fgets(line, sizeof(line), stdin) == NULL) {
	    fputc('\n', stdout);
	    return 0;
	}
	line[strcspn(line, "\r\n")] = '\0';
	argument = Control_argument(line);
	if (line[0] == '\0')
	    continue;
	if (!strcasecmp(line, "quit") || !strcasecmp(line, "exit"))
	    return 0;
	if (!strcasecmp(line, "help") || !strcmp(line, "?")) {
	    Control_help();
	    continue;
	}
	if (!strcasecmp(line, "status"))
	    command = REPORT_STATUS_pack;
	else if (!strcasecmp(line, "options"))
	    command = OPTION_LIST_pack;
	else if (!strcasecmp(line, "tune"))
	    command = OPTION_TUNE_pack;
	else if (!strcasecmp(line, "kick"))
	    command = KICK_PLAYER_pack;
	else if (!strcasecmp(line, "message"))
	    command = MESSAGE_pack;
	else if (!strcasecmp(line, "lock"))
	    command = LOCK_GAME_pack;
	else if (!strcasecmp(line, "shutdown"))
	    command = SHUTDOWN_pack;
	else {
	    printf("Unknown command: %s\n", line);
	    continue;
	}
	Control_request(server, port, user, command, argument, stdout);
    }
}
