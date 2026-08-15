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

#include "xpserver.h"

#define MAX_PENDING_SESSIONS 4
#define MAX_PENDING_PER_IP 2
#define SESSION_ADMISSION_TIMEOUT 4

typedef enum {
    PENDING_READING,
    PENDING_GAME_REPLY,
    PENDING_CONTROL_REPLY
} pending_state_t;

typedef enum {
    CONTROL_RESPONSE_DONE,
    CONTROL_RESPONSE_TEXT,
    CONTROL_RESPONSE_OPTIONS
} control_response_t;

typedef struct {
    bool active;
    pending_state_t state;
    time_t opened_at;
    sock_t socket;
    sockbuf_t input;
    sockbuf_t output;
    char address[SOCK_HOSTNAME_LENGTH];
    int peer_port;
    session_game_open_t game;
    unsigned selected_version;
    bool promote_game;
    session_control_open_t control;
    control_response_t control_response;
    unsigned char control_status;
    char response_text[SERVER_SEND_SIZE];
    size_t response_offset;
    int option_index;
    char option_value[MSG_LEN];
    bool option_ready;
} pending_session_t;

int NumQueuedPlayers = 0;
int NumPseudoPlayers = 0;
sock_t contactSocket = { .fd = SOCK_FD_INVALID };

static bool listener_installed;
static pending_session_t pending_sessions[MAX_PENDING_SESSIONS];

static int Check_address(char *address);

static bool Pending_output_empty(const pending_session_t *pending)
{
    return pending->output.len == 0
	&& pending->output.frame_output_len == 0;
}

static void Pending_cleanup(pending_session_t *pending)
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

static pending_session_t *Pending_find_free(void)
{
    int i;

    for (i = 0; i < MAX_PENDING_SESSIONS; i++) {
	if (!pending_sessions[i].active)
	    return &pending_sessions[i];
    }
    return NULL;
}

static int Pending_count_address(const char *address)
{
    int i;
    int count = 0;

    for (i = 0; i < MAX_PENDING_SESSIONS; i++) {
	if (pending_sessions[i].active
	    && strcmp(pending_sessions[i].address, address) == 0)
	    count++;
    }
    return count;
}

static void Contact_accept(int fd, void *arg)
{
    pending_session_t *pending;
    sock_t accepted;
    char address[SOCK_HOSTNAME_LENGTH];

    UNUSED_PARAM(fd);
    UNUSED_PARAM(arg);

    for (;;) {
	errno = 0;
	if (sock_accept(&contactSocket, &accepted) == SOCK_IS_ERROR) {
	    if (errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
		error("Cannot accept TCP session (%d)", errno);
	    return;
	}

	strlcpy(address, sock_get_last_addr(&accepted), sizeof(address));
	pending = Pending_find_free();
	if (rplayback || pending == NULL
	    || Pending_count_address(address) >= MAX_PENDING_PER_IP) {
	    sock_close(&accepted);
	    continue;
	}
	if (sock_set_non_blocking(&accepted, 1) == SOCK_IS_ERROR
	    || sock_set_tcp_nodelay(&accepted, 1) == SOCK_IS_ERROR) {
	    sock_close(&accepted);
	    continue;
	}
	if (sock_set_receive_buffer_size(&accepted,
					 SERVER_SEND_SIZE + 256) == SOCK_IS_ERROR)
	    error("Cannot set session receive buffer size");
	if (sock_set_send_buffer_size(&accepted,
				      SERVER_SEND_SIZE + 256) == SOCK_IS_ERROR)
	    error("Cannot set session send buffer size");

	memset(pending, 0, sizeof(*pending));
	pending->socket = accepted;
	pending->active = true;
	pending->state = PENDING_READING;
	pending->opened_at = time(NULL);
	pending->peer_port = sock_get_last_port(&accepted);
	strlcpy(pending->address, address, sizeof(pending->address));
	if (Sockbuf_init(&pending->input, &accepted, SERVER_SEND_SIZE,
			 SOCKBUF_READ | SOCKBUF_FRAMED) == -1
	    || Sockbuf_init(&pending->output, &accepted, SERVER_SEND_SIZE,
			    SOCKBUF_WRITE | SOCKBUF_FRAMED
			    | SOCKBUF_ORDERED) == -1) {
	    Pending_cleanup(pending);
	    continue;
	}
    }
}

void Contact_cleanup(void)
{
    int i;

    for (i = 0; i < MAX_PENDING_SESSIONS; i++) {
	if (pending_sessions[i].active)
	    Pending_cleanup(&pending_sessions[i]);
    }
    if (listener_installed) {
	remove_input(contactSocket.fd);
	listener_installed = false;
    }
    if (contactSocket.fd != SOCK_FD_INVALID)
	sock_close(&contactSocket);
}

int Contact_init(void)
{
    int backlog = Net_server_connection_limit() + MAX_PENDING_SESSIONS;

    sock_init(&contactSocket);
    if (backlog <= 0)
	backlog = MAX_PENDING_SESSIONS;
    if (sock_open_tcp_listener(&contactSocket, serverAddr,
			       options.contactPort, backlog) == SOCK_IS_ERROR) {
	error("Could not create TCP session listener");
	error("Perhaps %s is already running?", APPNAME);
	return false;
    }
    if (sock_set_non_blocking(&contactSocket, 1) == SOCK_IS_ERROR) {
	error("Cannot make TCP session listener non-blocking");
	sock_close(&contactSocket);
	return false;
    }
    install_input(Contact_accept, contactSocket.fd, NULL);
    listener_installed = true;
    xpprintf("%s TCP session listener is ready on port %d.\n",
	     showtime(), options.contactPort);
    return true;
}

/*
 * Kick at most one robot to make room for a human player.
 */
static int Kick_robot_players(int team)
{
    int i;

    if (NumRobots == 0)
	return 0;

    if (team == TEAM_NOT_SET) {
	if (BIT(world->rules->mode, TEAM_PLAY) && options.reserveRobotTeam) {
	    double low_score = FLT_MAX;
	    player_t *low_player = NULL;

	    for (i = 0; i < NumPlayers; i++) {
		player_t *player = Player_by_index(i);

		if (!Player_is_robot(player)
		    || player->team == options.robotTeam)
		    continue;
		if (Get_Score(player) < low_score) {
		    low_player = player;
		    low_score = Get_Score(player);
		}
	    }
	    if (low_player != NULL) {
		Robot_delete(low_player, true);
		return 1;
	    }
	    return 0;
	}
	Robot_delete(NULL, true);
	return 1;
    }

    if (world->teams[team].NumRobots > 0) {
	double low_score = FLT_MAX;
	player_t *low_player = NULL;

	for (i = 0; i < NumPlayers; i++) {
	    player_t *player = Player_by_index(i);

	    if (!Player_is_robot(player) || player->team != team)
		continue;
	    if (Get_Score(player) < low_score) {
		low_player = player;
		low_score = Get_Score(player);
	    }
	}
	if (low_player != NULL) {
	    Robot_delete(low_player, true);
	    return 1;
	}
    }
    return 0;
}

static int Kick_paused_pass(int team, bool preserve_last)
{
    int i;
    int kicked = 0;

    for (i = NumPlayers - 1; i >= 0; i--) {
	player_t *player = Player_by_index(i);

	if (player->conn != NULL
	    && Player_is_paused(player)
	    && (team == TEAM_NOT_SET
		|| (player->team == team && player->home_base != NULL))
	    && !(player->privs & PRIV_NOAUTOKICK)
	    && (!preserve_last
		|| !(player->privs & PRIV_AUTOKICKLAST))) {
	    if (team == TEAM_NOT_SET) {
		Set_message_f("The paused \"%s\" was kicked because the "
			      "game is full.", player->name);
		Destroy_connection(player->conn, "no pause with full game");
	    } else {
		Set_message_f("The paused \"%s\" was kicked because team %d "
			      "is full.", player->name, team);
		Destroy_connection(player->conn, "no pause with full team");
	    }
	    kicked++;
	}
    }
    return kicked;
}

static int Kick_paused_players(int team)
{
    int kicked = Kick_paused_pass(team, true);

    if (kicked == 0)
	kicked = Kick_paused_pass(team, false);
    return kicked;
}

static int Check_names(char *nick, char *user, char *host)
{
    char *end;
    int i;

    if (user[0] == '\0' || host[0] == '\0'
	|| nick[0] < 'A' || nick[0] > 'Z')
	return E_INVAL;

    for (end = nick + strlen(nick); end-- > nick;) {
	if (isascii(*end) && isspace(*end))
	    *end = '\0';
	else
	    break;
    }
    for (i = 0; i < NumPlayers; i++) {
	if (strcasecmp(Player_by_index(i)->name, nick) == 0)
	    return E_IN_USE;
    }
    return SUCCESS;
}

static int Pending_reserved_games(const pending_session_t *current,
				  const char *nick)
{
    int i;
    int reserved = 0;

    for (i = 0; i < MAX_PENDING_SESSIONS; i++) {
	const pending_session_t *pending = &pending_sessions[i];

	if (pending == current || !pending->active || !pending->promote_game)
	    continue;
	reserved++;
	if (strcasecmp(pending->game.nick, nick) == 0)
	    return -1;
    }
    return reserved;
}

static int Admit_game(pending_session_t *pending)
{
    session_game_open_t *game = &pending->game;
    int limit;
    int reserved;
    int status;

    Fix_user_name(game->user);
    Fix_nick_name(game->nick);
    Fix_disp_name(game->display);
    Fix_host_name(game->host);
    if (game->team < 0 || game->team >= MAX_TEAMS)
	game->team = TEAM_NOT_SET;

    if (game->polygon_version != POLYGON_VERSION
	|| game->legacy_version != OLD_VERSION)
	return E_VERSION;
    pending->selected_version =
	is_polygon_map ? POLYGON_VERSION : OLD_VERSION;
    if (Check_address(pending->address))
	return E_GAME_LOCKED;
    status = Check_names(game->nick, game->user, game->host);
    if (status != SUCCESS)
	return status;

    reserved = Pending_reserved_games(pending, game->nick);
    if (reserved < 0)
	return E_IN_USE;
    if (game_lock && !rplayback && !options.baselessPausing)
	return E_GAME_LOCKED;
    if (Check_max_clients_per_IP(pending->address))
	return E_GAME_LOCKED;

    limit = (int)MIN(options.playerLimit,
		     options.baselessPausing ? 1000000 : Num_bases());
    if (NumPlayers - NumPseudoPlayers + login_in_progress + reserved
	>= limit) {
	if (game_lock
	    || ((!Kick_robot_players(TEAM_NOT_SET)
		 || NumPlayers - NumPseudoPlayers + login_in_progress
		    + reserved >= limit)
		&& (!Kick_paused_players(TEAM_NOT_SET)
		    || NumPlayers - NumPseudoPlayers + login_in_progress
		       + reserved >= limit)))
	    return E_GAME_FULL;
    }

    if (BIT(world->rules->mode, TEAM_PLAY)) {
	if (game->team >= 0 && game->team < MAX_TEAMS) {
	    if (game_lock
		|| (game->team == options.robotTeam
		    && options.reserveRobotTeam)
		|| (world->teams[game->team].NumMembers
		    >= world->teams[game->team].NumBases
		    && !Kick_robot_players(game->team)
		    && !Kick_paused_players(game->team)))
		game->team = TEAM_NOT_SET;
	}
	if (game->team == TEAM_NOT_SET)
	    game->team = Pick_team(PL_TYPE_HUMAN);
	if (game->team == TEAM_NOT_SET && !game_lock
	    && NumRobots > world->teams[options.robotTeam].NumRobots) {
	    Kick_robot_players(TEAM_NOT_SET);
	    game->team = Pick_team(PL_TYPE_HUMAN);
	}
	if (game->team == TEAM_NOT_SET)
	    return E_TEAM_FULL;
    } else
	game->team = TEAM_NOT_SET;

    return SUCCESS;
}

static const char *Status_reason(int status)
{
    switch (status) {
    case SUCCESS:
	return "accepted";
    case E_NOT_OWNER:
	return "permission denied";
    case E_GAME_FULL:
	return "game is full";
    case E_TEAM_FULL:
	return "team is full";
    case E_GAME_LOCKED:
	return "game is locked";
    case E_IN_USE:
	return "name is already in use";
    case E_INVAL:
	return "invalid request";
    case E_VERSION:
	return "incompatible protocol version";
    case E_NOT_FOUND:
	return "player not found";
    case E_NOENT:
	return "option not found";
    case E_UNDEFINED:
	return "command is not supported";
    default:
	return "session failed";
    }
}

static int Queue_game_reply(pending_session_t *pending, int status)
{
    session_reply_t reply;

    memset(&reply, 0, sizeof(reply));
    reply.status = status;
    reply.selected_version = pending->selected_version;
    strlcpy(reply.reason, Status_reason(status), sizeof(reply.reason));
    Sockbuf_clear(&pending->output);
    if (Session_encode_reply(&pending->output, &reply) <= 0)
	return -1;
    pending->state = PENDING_GAME_REPLY;
    pending->promote_game = status == SUCCESS;
    return 0;
}

static bool Control_is_public(unsigned char command)
{
    return command == REPORT_STATUS_pack || command == OPTION_LIST_pack;
}

static bool Control_is_owner(const pending_session_t *pending)
{
    return strncmp(pending->address, "127.", 4) == 0
	&& strcmp(pending->control.user, Server.owner) == 0;
}

static bool Control_next_option(pending_session_t *pending, char *value)
{
    int result;

    for (;;) {
	result = Parser_list_option(&pending->option_index, value);
	if (result < 0)
	    return false;
	pending->option_index++;
	if (result > 0)
	    return true;
    }
}

static int Queue_control_frame(pending_session_t *pending,
			       const char *payload, bool more)
{
    session_control_reply_t reply;

    memset(&reply, 0, sizeof(reply));
    reply.command = pending->control.command;
    reply.status = pending->control_status;
    reply.more = more;
    strlcpy(reply.payload, payload, sizeof(reply.payload));
    Sockbuf_clear(&pending->output);
    if (Session_encode_control_reply(&pending->output, &reply) <= 0)
	return -1;
    pending->state = PENDING_CONTROL_REPLY;
    return 0;
}

static int Queue_next_control_frame(pending_session_t *pending)
{
    char payload[MSG_LEN];
    bool more;

    if (pending->control_response == CONTROL_RESPONSE_TEXT) {
	size_t remaining =
	    strlen(pending->response_text) - pending->response_offset;
	size_t length = MIN(remaining, sizeof(payload) - 1);

	memcpy(payload, pending->response_text + pending->response_offset,
	       length);
	payload[length] = '\0';
	pending->response_offset += length;
	more = pending->response_offset < strlen(pending->response_text);
	if (!more)
	    pending->control_response = CONTROL_RESPONSE_DONE;
	return Queue_control_frame(pending, payload, more);
    }

    if (pending->control_response == CONTROL_RESPONSE_OPTIONS) {
	if (!pending->option_ready) {
	    pending->option_ready =
		Control_next_option(pending, pending->option_value);
	    if (!pending->option_ready) {
		pending->control_response = CONTROL_RESPONSE_DONE;
		return Queue_control_frame(pending, "", false);
	    }
	}
	strlcpy(payload, pending->option_value, sizeof(payload));
	pending->option_ready =
	    Control_next_option(pending, pending->option_value);
	more = pending->option_ready;
	if (!more)
	    pending->control_response = CONTROL_RESPONSE_DONE;
	return Queue_control_frame(pending, payload, more);
    }

    return -1;
}

static int Queue_simple_control_reply(pending_session_t *pending, int status)
{
    pending->control_status = status;
    pending->control_response = CONTROL_RESPONSE_DONE;
    return Queue_control_frame(pending, Status_reason(status), false);
}

static int Execute_control(pending_session_t *pending)
{
    session_control_open_t *control = &pending->control;
    int status = SUCCESS;
    char argument[MSG_LEN];
    char *separator;

    Fix_user_name(control->user);
    if (control->polygon_version != POLYGON_VERSION
	|| control->legacy_version != OLD_VERSION)
	return Queue_simple_control_reply(pending, E_VERSION);
    if (!Control_is_public(control->command)
	&& !Control_is_owner(pending))
	return Queue_simple_control_reply(pending, E_NOT_OWNER);

    switch (control->command) {
    case REPORT_STATUS_pack:
	xpprintf("%s %s@%s asked for info about current game.\n",
		 showtime(), control->user, pending->address);
	Server_info(pending->response_text,
		    sizeof(pending->response_text));
	pending->response_offset = 0;
	pending->control_status = SUCCESS;
	pending->control_response = CONTROL_RESPONSE_TEXT;
	return Queue_next_control_frame(pending);

    case OPTION_LIST_pack:
	xpprintf("%s %s@%s asked for an option list.\n",
		 showtime(), control->user, pending->address);
	pending->option_index = 0;
	pending->option_ready = false;
	pending->control_status = SUCCESS;
	pending->control_response = CONTROL_RESPONSE_OPTIONS;
	return Queue_next_control_frame(pending);

    case MESSAGE_pack:
	if (control->argument[0] == '\0')
	    status = E_INVAL;
	else
	    Set_message_f("%s [%s SPEAKING FROM ABOVE]",
			  control->argument, control->user);
	break;

    case LOCK_GAME_pack:
	if (!strcasecmp(control->argument, "1")
	    || !strcasecmp(control->argument, "on")
	    || !strcasecmp(control->argument, "true"))
	    game_lock = true;
	else if (!strcasecmp(control->argument, "0")
		 || !strcasecmp(control->argument, "off")
		 || !strcasecmp(control->argument, "false"))
	    game_lock = false;
	else
	    status = E_INVAL;
	break;

    case SHUTDOWN_pack:
	Server_shutdown(control->user, 0,
			control->argument[0] != '\0'
			? control->argument : "shutdown requested");
	break;

    case KICK_PLAYER_pack:
    {
	player_t *player = Get_player_by_name(control->argument, NULL, NULL);

	if (player == NULL)
	    status = E_NOT_FOUND;
	else {
	    Set_message_f("\"%s\" upset the gods and was kicked out "
			  "of the game.", player->name);
	    if (player->conn == NULL)
		Delete_player(player);
	    else
		Destroy_connection(player->conn, "kicked out");
	    updateScores = true;
	}
	break;
    }

    case OPTION_TUNE_pack:
	strlcpy(argument, control->argument, sizeof(argument));
	separator = strchr(argument, ':');
	if (separator == NULL || separator == argument
	    || separator[1] == '\0')
	    status = E_INVAL;
	else {
	    int result;

	    *separator++ = '\0';
	    result = Tune_option(argument, separator);
	    if (result == 1) {
		if (strcasecmp(argument, "password")) {
		    char value[MAX_CHARS];

		    Get_option_value(argument, value, sizeof(value));
		    Set_message_f(" < Option %s set to %s by %s "
				  "FROM ABOVE. >",
				  argument, value, control->user);
		}
	    } else if (result == -1)
		status = E_UNDEFINED;
	    else if (result == -2)
		status = E_NOENT;
	    else
		status = E_INVAL;
	}
	break;

    default:
	status = E_UNDEFINED;
	break;
    }
    return Queue_simple_control_reply(pending, status);
}

static void Promote_game(pending_session_t *pending)
{
    session_game_open_t game = pending->game;
    sock_t socket = pending->socket;
    char address[SOCK_HOSTNAME_LENGTH];

    strlcpy(address, pending->address, sizeof(address));
    pending->socket.fd = SOCK_FD_INVALID;
    Sockbuf_cleanup(&pending->input);
    Sockbuf_cleanup(&pending->output);
    memset(pending, 0, sizeof(*pending));
    pending->socket.fd = SOCK_FD_INVALID;

    if (Setup_connection(&socket, game.user, game.nick, game.display,
			 game.team, address, game.host,
			 is_polygon_map ? POLYGON_VERSION : OLD_VERSION) == -1
	&& socket.fd != SOCK_FD_INVALID)
	sock_close(&socket);
}

static void Process_pending(pending_session_t *pending)
{
    int status;

    if (time(NULL) >= pending->opened_at + SESSION_ADMISSION_TIMEOUT) {
	Pending_cleanup(pending);
	return;
    }

    if (pending->state == PENDING_READING) {
	status = Sockbuf_read(&pending->input);
	if (status < 0) {
	    Pending_cleanup(pending);
	    return;
	}
	if (status == 0)
	    return;

	if (Session_decode_game_open(&pending->input, &pending->game) > 0) {
	    status = Admit_game(pending);
	    if (Queue_game_reply(pending, status) == -1) {
		Pending_cleanup(pending);
		return;
	    }
	} else {
	    pending->input.ptr = pending->input.buf;
	    if (Session_decode_control_open(&pending->input,
					    &pending->control) <= 0
		|| Execute_control(pending) == -1) {
		Pending_cleanup(pending);
		return;
	    }
	}
    }

    if (Sockbuf_flush(&pending->output) < 0) {
	Pending_cleanup(pending);
	return;
    }
    if (!Pending_output_empty(pending))
	return;

    if (pending->state == PENDING_GAME_REPLY) {
	if (pending->promote_game)
	    Promote_game(pending);
	else
	    Pending_cleanup(pending);
	return;
    }

    if (pending->control_response == CONTROL_RESPONSE_DONE) {
	Pending_cleanup(pending);
	return;
    }
    if (Queue_next_control_frame(pending) == -1
	|| Sockbuf_flush(&pending->output) < 0)
	Pending_cleanup(pending);
}

void Session_poll(void)
{
    int i;

    for (i = 0; i < MAX_PENDING_SESSIONS; i++) {
	if (pending_sessions[i].active)
	    Process_pending(&pending_sessions[i]);
    }
}

/*
 * Transitional in-game queue commands report that no waiting queue exists.
 * The command registrations are removed with the legacy lobby code.
 */
void Queue_kick(const char *nick)
{
    UNUSED_PARAM(nick);
}

int Queue_advance_player(char *name, char *message, size_t size)
{
    UNUSED_PARAM(name);
    strlcpy(message, "The waiting queue is not available.", size);
    return 0;
}

int Queue_show_list(char *message, size_t size)
{
    strlcpy(message, "The waiting queue is not available.", size);
    return 0;
}

struct addr_plus_mask {
    unsigned long address;
    unsigned long mask;
};

static struct addr_plus_mask *denied_addresses;
static int denied_address_count;

static int Check_address(char *address_text)
{
    unsigned long address;
    int i;

    address = sock_get_inet_by_addr(address_text);
    if (address == (unsigned long)-1
	&& strcmp(address_text, "255.255.255.255"))
	return -1;

    for (i = 0; i < denied_address_count; i++) {
	if ((denied_addresses[i].address & denied_addresses[i].mask)
	    == (address & denied_addresses[i].mask))
	    return 1;
    }
    return 0;
}

void Set_deny_hosts(void)
{
    char *list;
    char *token;
    char *slash;
    int capacity = 0;
    unsigned long address;
    unsigned long mask;
    static char separators[] = ",;: \t\n";

    denied_address_count = 0;
    XFREE(denied_addresses);
    list = xp_strdup(options.denyHosts);
    if (list == NULL)
	return;

    for (token = strtok(list, separators); token != NULL;
	 token = strtok(NULL, separators))
	capacity++;
    denied_addresses = XMALLOC(struct addr_plus_mask, capacity);
    strcpy(list, options.denyHosts);

    for (token = strtok(list, separators); token != NULL;
	 token = strtok(NULL, separators)) {
	slash = strchr(token, '/');
	if (slash != NULL) {
	    *slash = '\0';
	    mask = sock_get_inet_by_addr(slash + 1);
	    if ((mask == (unsigned long)-1
		 && strcmp(slash + 1, "255.255.255.255"))
		|| mask == 0)
		continue;
	} else
	    mask = 0xFFFFFFFF;

	address = sock_get_inet_by_addr(token);
	if (address == (unsigned long)-1
	    && strcmp(token, "255.255.255.255"))
	    continue;
	denied_addresses[denied_address_count].address = address;
	denied_addresses[denied_address_count].mask = mask;
	denied_address_count++;
    }
    free(list);
}
