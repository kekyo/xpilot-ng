/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2004 Uoti Urpala <uau@users.sourceforge.net>
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

/*
 * This is the server side of the network connnection stuff.
 *
 * We try very hard to not let the game be disturbed by
 * players logging in.  Therefore a new connection
 * passes through several states before it is actively
 * playing.
 * First we make a new connection structure available
 * with a new socket to listen on.  This socket port
 * number is told to the client via the pack mechanism.
 * In this state the client has to send a packet to this
 * newly created socket with its name and playing parameters.
 * If this succeeds the connection advances to its second state.
 * In this second state the essential server configuration
 * like the map and so on is transmitted to the client.
 * If the client has acknowledged all this data then it
 * advances to the third state, which is the
 * ready-but-not-playing-yet state.  In this state the client
 * has some time to do its final initializations, like mapping
 * its user interface windows and so on.
 * When the client is ready to accept frame updates and process
 * keyboard events then it sends the start-play packet.
 * This play packet advances the connection state into the
 * actively-playing state.  A player structure is allocated and
 * initialized and the other human players are told about this new player.
 * The newly started client is told about the already playing players and
 * play has begun.
 * Apart from these four states there are also two intermediate states.
 * These intermediate states are entered when the previous state
 * has filled the reliable data buffer and the client has not
 * acknowledged all the data yet that is in this reliable data buffer.
 * They are so called output drain states.  Not doing anything else
 * then waiting until the buffer is empty.
 * The difference between these two intermediate states is tricky.
 * The second intermediate state is entered after the
 * ready-but-not-playing-yet state and before the actively-playing state.
 * The difference being that in this second intermediate state the client
 * is already considered an active player by the rest of the server
 * but should not get frame updates yet until it has acknowledged its last
 * reliable data.
 *
 * Communication between the server and the clients is only done
 * using UDP datagrams.  The first client/serverized version of XPilot
 * was using TCP only, but this was too unplayable across the Internet,
 * because TCP is a data stream always sending the next byte.
 * If a packet gets lost then the server has to wait for a
 * timeout before a retransmission can occur.  This is too slow
 * for a real-time program like this game, which is more interested
 * in recent events than in sequenced/reliable events.
 * Therefore UDP is now used which gives more network control to the
 * program.
 * Because some data is considered crucial, like the names of
 * new players and so on, there also had to be a mechanism which
 * enabled reliable data transmission.  Here this is done by creating
 * a data stream which is piggybacked on top of the unreliable data
 * packets.  The client acknowledges this reliable data by sending
 * its byte position in the reliable data stream.  So if the client gets
 * a new reliable data packet and it has not had this data before and
 * there is also no data packet missing inbetween, then it advances
 * its byte position and acknowledges this new position to the server.
 * Otherwise it discards the packet and sends its old byte position
 * to the server meaning that it detected a packet loss.
 * The server maintains an acknowledgement timeout timer for each
 * connection so that it can retransmit a reliable data packet
 * if the acknowledgement timer expires.
 */

#include "xpserver.h"

static int Init_setup(void);
static int Handle_listening(connection_t *connp);
static int Handle_setup(connection_t *connp);
static int Handle_login(connection_t *connp, char *errmsg, size_t errsize);
static void Handle_input(int fd, void *arg);

static int Receive_keyboard(connection_t *connp);
static int Receive_quit(connection_t *connp);
static int Receive_play(connection_t *connp);
static int Receive_power(connection_t *connp);
static int Receive_ack(connection_t *connp);
static int Receive_ack_cannon(connection_t *connp);
static int Receive_ack_fuel(connection_t *connp);
static int Receive_ack_target(connection_t *connp);
static int Receive_ack_polystyle(connection_t *connp);
static int Receive_discard(connection_t *connp);
static int Receive_undefined(connection_t *connp);
static int Receive_talk(connection_t *connp);
static int Receive_display(connection_t *connp);
static int Receive_modifier_bank(connection_t *connp);
static int Receive_motd(connection_t *connp);
static int Receive_shape(connection_t *connp);
static int Receive_pointer_move(connection_t *connp);
static int Receive_audio_request(connection_t *connp);
static int Receive_fps_request(connection_t *connp);

static int Send_motd(connection_t *connp);

#define MAX_SELECT_FD			(sizeof(int) * 8 - 1)
#define MAX_RELIABLE_DATA_PACKET_SIZE	1024

#define MAX_MOTD_CHUNK			512
#define MAX_MOTD_SIZE			(30*1024)
#define MAX_MOTD_LOOPS			(10*FPS)

static connection_t	*Conn = NULL;
static int		max_connections = 0;
static setup_t		*Setup = NULL;
static setup_t		*Oldsetup = NULL;
static int		(*playing_receive[256])(connection_t *connp),
			(*login_receive[256])(connection_t *connp),
			(*drain_receive[256])(connection_t *connp);
int			login_in_progress;
static int		num_logins, num_logouts;

static void Feature_init(connection_t *connp)
{
    int v = connp->version;
    int features = 0;

    if (v < 0x4F00) {
	if (v >= 0x4210)
	    SET_BIT(features, F_TEAMRADAR);
	if (v >= 0x4300)
	    SET_BIT(features, F_SEPARATEPHASING);
	if (v >= 0x4400)
	    SET_BIT(features, F_ASTEROID);
	if (v >= 0x4401)
	    SET_BIT(features, F_FASTRADAR);
	if (v >= 0x4500)
	    SET_BIT(features, F_FLOATSCORE);
	if (v >= 0x4501)
	    SET_BIT(features, F_TEMPWORM);
    }
    else {
	SET_BIT(features, F_POLY);
	SET_BIT(features, F_TEAMRADAR);

	SET_BIT(features, F_SEPARATEPHASING);
	if (v >= 0x4F10)
	    SET_BIT(features, F_EXPLICITSELF);
	if (v >= 0x4F11) {
	    SET_BIT(features, F_ASTEROID);
	    SET_BIT(features, F_FASTRADAR);
	    SET_BIT(features, F_FLOATSCORE);
	    SET_BIT(features, F_TEMPWORM);
	}
	if (v >= 0x4F12) {
	    SET_BIT(features, F_SHOW_APPEARING);
	    SET_BIT(features, F_SENDTEAM);
	}
	if (v >= 0x4F13)
	    SET_BIT(features, F_CUMULATIVETURN);
	if (v >= 0x4F14)
	    SET_BIT(features, F_BALLSTYLE);
	if (v >= 0x4F15)
	    SET_BIT(features, F_POLYSTYLE);
    }
    connp->features = features;
    return;
}

/*
 * Initialize the structure that gives the client information
 * about our setup.  Like the map and playing rules.
 * We only setup this structure once to save time when new
 * players log in during play.
 */
static int Init_setup(void)
{
    size_t size;
    unsigned char *mapdata;

    Oldsetup = Xpmap_init_setup();

    if (!is_polygon_map) {
	if (Oldsetup)
	    return 0;
	else
	    return -1;
    }

    size = Polys_to_client(&mapdata);
    xpprintf("%s Server->client polygon map transfer size is %d bytes.\n",
	     showtime(), size);

    if ((Setup = (setup_t *)malloc(sizeof(setup_t) + size)) == NULL) {
	error("No memory to hold setup");
	free(mapdata);
	return -1;
    }
    memset(Setup, 0, sizeof(setup_t) + size);
    memcpy(Setup->map_data, mapdata, size);
    free(mapdata);
    Setup->setup_size = ((char *) &Setup->map_data[0] - (char *) Setup) + size;
    Setup->map_data_len = size;
    Setup->lives = world->rules->lives;
    Setup->mode = world->rules->mode;
    Setup->width = world->width;
    Setup->height = world->height;
    strlcpy(Setup->name, world->name, sizeof(Setup->name));
    strlcpy(Setup->author, world->author, sizeof(Setup->author));
    strlcpy(Setup->data_url, options.dataURL, sizeof(Setup->data_url));

    return 0;
}

/*
 * Initialize the function dispatch tables for the various client
 * connection states.  Some states use the same table.
 */
static void Init_receive(void)
{
    int i;

    for (i = 0; i < 256; i++) {
	login_receive[i] = Receive_undefined;
	playing_receive[i] = Receive_undefined;
	drain_receive[i] = Receive_undefined;
    }

    drain_receive[PKT_QUIT]			= Receive_quit;
    drain_receive[PKT_ACK]			= Receive_ack;
    drain_receive[PKT_VERIFY]			= Receive_discard;
    drain_receive[PKT_PLAY]			= Receive_discard;
    drain_receive[PKT_SHAPE]			= Receive_discard;

    login_receive[PKT_PLAY]			= Receive_play;
    login_receive[PKT_QUIT]			= Receive_quit;
    login_receive[PKT_ACK]			= Receive_ack;
    login_receive[PKT_VERIFY]			= Receive_discard;
    login_receive[PKT_POWER]			= Receive_power;
    login_receive[PKT_POWER_S]			= Receive_power;
    login_receive[PKT_TURNSPEED]		= Receive_power;
    login_receive[PKT_TURNSPEED_S]		= Receive_power;
    login_receive[PKT_TURNRESISTANCE]		= Receive_power;
    login_receive[PKT_TURNRESISTANCE_S]		= Receive_power;
    login_receive[PKT_DISPLAY]			= Receive_display;
    login_receive[PKT_MODIFIERBANK]		= Receive_modifier_bank;
    login_receive[PKT_MOTD]			= Receive_motd;
    login_receive[PKT_SHAPE]			= Receive_shape;
    login_receive[PKT_REQUEST_AUDIO]		= Receive_audio_request;
    login_receive[PKT_ASYNC_FPS]		= Receive_fps_request;

    playing_receive[PKT_ACK]			= Receive_ack;
    playing_receive[PKT_VERIFY]			= Receive_discard;
    playing_receive[PKT_PLAY]			= Receive_play;
    playing_receive[PKT_QUIT]			= Receive_quit;
    playing_receive[PKT_KEYBOARD]		= Receive_keyboard;
    playing_receive[PKT_POWER]			= Receive_power;
    playing_receive[PKT_POWER_S]		= Receive_power;
    playing_receive[PKT_TURNSPEED]		= Receive_power;
    playing_receive[PKT_TURNSPEED_S]		= Receive_power;
    playing_receive[PKT_TURNRESISTANCE]		= Receive_power;
    playing_receive[PKT_TURNRESISTANCE_S]	= Receive_power;
    playing_receive[PKT_ACK_CANNON]		= Receive_ack_cannon;
    playing_receive[PKT_ACK_FUEL]		= Receive_ack_fuel;
    playing_receive[PKT_ACK_TARGET]		= Receive_ack_target;
    playing_receive[PKT_ACK_POLYSTYLE]		= Receive_ack_polystyle;
    playing_receive[PKT_TALK]			= Receive_talk;
    playing_receive[PKT_DISPLAY]		= Receive_display;
    playing_receive[PKT_MODIFIERBANK]		= Receive_modifier_bank;
    playing_receive[PKT_MOTD]			= Receive_motd;
    playing_receive[PKT_SHAPE]			= Receive_shape;
    playing_receive[PKT_POINTER_MOVE]		= Receive_pointer_move;
    playing_receive[PKT_REQUEST_AUDIO]		= Receive_audio_request;
    playing_receive[PKT_ASYNC_FPS]		= Receive_fps_request;
}

/*
 * Initialize the connection structures.
 */
int Setup_net_server(void)
{
    Init_receive();

    if (Init_setup() == -1)
	return -1;
    /*
     * The number of connections is limited by the number of bases
     * and the max number of possible file descriptors to use in
     * the select(2) call minus those for stdin, stdout, stderr,
     * the contact socket, and the socket for the resolver library routines.
     */
    max_connections
	= MIN((int)MAX_SELECT_FD - 5,
	      options.playerLimit_orig + MAX_SPECTATORS * !!rplayback);
    if ((Conn = XCALLOC(connection_t, max_connections)) == NULL) {
	error("Cannot allocate memory for connections");
	return -1;
    }

    return 0;
}

static void Conn_set_state(connection_t *connp, int state, int drain_state)
{
    static int num_conn_busy;
    static int num_conn_playing;

    if ((connp->state & (CONN_PLAYING | CONN_READY)) != 0)
	num_conn_playing--;
    else if (connp->state == CONN_FREE)
	num_conn_busy++;

    connp->state = state;
    connp->drain_state = drain_state;
    connp->start = main_loops;

    if (connp->state == CONN_PLAYING) {
	num_conn_playing++;
	connp->timeout = IDLE_TIMEOUT;
    }
    else if (connp->state == CONN_READY) {
	num_conn_playing++;
	connp->timeout = READY_TIMEOUT;
    }
    else if (connp->state == CONN_LOGIN)
	connp->timeout = LOGIN_TIMEOUT;
    else if (connp->state == CONN_SETUP)
	connp->timeout = SETUP_TIMEOUT;
    else if (connp->state == CONN_LISTENING)
	connp->timeout = LISTEN_TIMEOUT;
    else if (connp->state == CONN_FREE) {
	num_conn_busy--;
	connp->timeout = IDLE_TIMEOUT;
    }

    login_in_progress = num_conn_busy - num_conn_playing;
}

/*
 * Cleanup a connection.  The client may not know yet that
 * it is thrown out of the game so we send it a quit packet.
 * We send it twice because of UDP it could get lost.
 * Since 3.0.6 the client receives a short message
 * explaining why the connection was terminated.
 */
void Destroy_connection(connection_t *connp, const char *reason)
{
    int id, len;
    sock_t *sock;
    char pkt[MAX_CHARS];

    if (connp->state == CONN_FREE) {
	warn("Cannot destroy empty connection (\"%s\")", reason);
	return;
    }

    sock = &connp->w.sock;
    remove_input(sock->fd);

    pkt[0] = PKT_QUIT;
    strlcpy(&pkt[1], reason, sizeof(pkt) - 1);
    len = strlen(pkt) + 1;
    if (sock_writeRec(sock, pkt, len) != len) {
	sock_get_errorRec(sock);
	sock_writeRec(sock, pkt, len);
    }
    xpprintf("%s Goodbye %s=%s@%s|%s (\"%s\")\n",
	     showtime(),
	     connp->nick ? connp->nick : "",
	     connp->user ? connp->user : "",
	     connp->host ? connp->host : "",
	     connp->dpy ? connp->dpy : "",
	     reason);

    Conn_set_state(connp, CONN_FREE, CONN_FREE);

    if (connp->id != NO_ID) {
	player_t *pl;

	id = connp->id;
	connp->id = NO_ID;
	pl = Player_by_id(id);
	pl->conn = NULL;
	if (pl->rectype != 2)
	    Delete_player(pl);
	else
	    Delete_spectator(pl);
    }

    XFREE(connp->user);
    XFREE(connp->nick);
    XFREE(connp->dpy);
    XFREE(connp->addr);
    XFREE(connp->host);

    Sockbuf_cleanup(&connp->w);
    Sockbuf_cleanup(&connp->r);
    Sockbuf_cleanup(&connp->c);

    num_logouts++;

    if (sock_writeRec(sock, pkt, len) != len) {
	sock_get_errorRec(sock);
	sock_writeRec(sock, pkt, len);
    }
    sock_closeRec(sock);

    memset(connp, 0, sizeof(*connp));
}

int Check_connection(char *user, char *nick, char *dpy, char *addr)
{
    int i;
    connection_t *connp;

    for (i = 0; i < max_connections; i++) {
	connp = &Conn[i];
	if (connp->state == CONN_LISTENING) {
	    if (strcasecmp(connp->nick, nick) == 0) {
		if (!strcmp(user, connp->user)
		    && !strcmp(dpy, connp->dpy)
		    && !strcmp(addr, connp->addr))
		    return connp->my_port;
		return -1;
	    }
	}
    }
    return -1;
}

static void Create_client_socket(sock_t *sock, int *port)
{
    int i;

    if (options.clientPortStart
	&& (!options.clientPortEnd || options.clientPortEnd > 65535))
	options.clientPortEnd = 65535;

    if (options.clientPortEnd
	&& (!options.clientPortStart || options.clientPortStart < 1024))
	options.clientPortStart = 1024;

    if (!options.clientPortStart || !options.clientPortEnd ||
	(options.clientPortStart > options.clientPortEnd)) {

        if (sock_open_udp(sock, serverAddr, 0) == SOCK_IS_ERROR) {
            error("Cannot create datagram socket (%d)", sock->error.error);
	    sock->fd = -1;
	    return;
        }
    }
    else {
        for (i = options.clientPortStart; i <= options.clientPortEnd; i++) {
            if (sock_open_udp(sock, serverAddr, i) != SOCK_IS_ERROR)
		goto found;
	}
	error("Could not find a usable port in given port range");
	sock->fd = -1;
	return;
    }
 found:
    if ((*port = sock_get_port(sock)) == -1) {
	error("Cannot get port from socket");
	goto error;
    }
    if (sock_set_non_blocking(sock, 1) == -1) {
	error("Cannot make client socket non-blocking");
	goto error;
    }
    if (sock_set_receive_buffer_size(sock, SERVER_RECV_SIZE + 256) == -1)
	error("Cannot set receive buffer size to %d", SERVER_RECV_SIZE + 256);

    if (sock_set_send_buffer_size(sock, SERVER_SEND_SIZE + 256) == -1)
	error("Cannot set send buffer size to %d", SERVER_SEND_SIZE + 256);

    return;
 error:
    sock_close(sock);
    sock->fd = -1;
    return;
}


#if 0
/*
 * Banning of players
 */
static void dcase(char *str)
{
    while (*str) {
	*str = tolower(*str);
	str++;
    }
}

char *banned_users[] = { "<", ">", "\"", "'", NULL };
char *banned_nicks[] = { "<", ">", "\"", "'", NULL };
char *banned_addrs[] = { NULL };
char *banned_hosts[] = { "<", ">", "\"", "'", NULL };

int CheckBanned(char *user, char *nick, char *addr, char *host)
{
    int ret = 0, i;

    user = strdup(user);
    nick = strdup(nick);
    addr = strdup(addr);
    host = strdup(host);
    dcase(user);
    dcase(nick);
    dcase(addr);
    dcase(host);

    for (i = 0; banned_users[i] != NULL; i++) {
	if (strstr(user, banned_users[i]) != NULL) {
	    ret = 1;
	    goto out;
	}
    }
    for (i = 0; banned_nicks[i] != NULL; i++) {
	if (strstr(nick, banned_nicks[i]) != NULL) {
	    ret = 1;
	    goto out;
	}
    }
    for (i = 0; banned_addrs[i] != NULL; i++) {
	if (strstr(addr, banned_addrs[i]) != NULL) {
	    ret = 1;
	    goto out;
	}
    }
    for (i = 0; banned_hosts[i] != NULL; i++) {
	if (strstr(host, banned_hosts[i]) != NULL) {
	    ret = 1;
	    goto out;
	}
    }
 out:
    free(user);
    free(nick);
    free(addr);
    free(host);

    return ret;
}

struct restrict {
    char *nick;
    char *addr;
    char *mail;
};

struct restrict restricted[] = {
    { NULL, NULL, NULL }
};

int CheckAllowed(char *user, char *nick, char *addr, char *host)
{
    int i, allowed = 1;
    /*char *realnick = nick;*/
    char *mail = NULL;

    nick = strdup(nick);
    addr = strdup(addr);
    dcase(nick);
    dcase(addr);

    for (i = 0; restricted[i].nick != NULL; i++) {
	if (strstr(nick, restricted[i].nick) != NULL) {
	    if (strncmp(addr, restricted[i].addr, strlen(restricted[i].addr))
		== 0) {
		allowed = 1;
		break;
	    }
	    allowed = 0;
	    mail = restricted[i].mail;
	}
    }
    if (!allowed) {
	/* Do whatever you want here... */
    }

    free(nick);
    free(addr);

    return allowed;
}
#endif


/*
 * A client has requested a playing connection with this server.
 * See if we have room for one more player and if his name is not
 * already in use by some other player.  Because the confirmation
 * may get lost we are willing to send it another time if the
 * client connection is still in the CONN_LISTENING state.
 */

extern int min_fd;

int Setup_connection(char *user, char *nick, char *dpy, int team,
		     char *addr, char *host, unsigned version)
{
    int i, free_conn_index = max_connections, my_port;
    sock_t sock;
    connection_t *connp;

    if (rrecord) {
	*playback_ei++ = main_loops;
	strcpy(playback_es, user);
	while (*playback_es++);
	strcpy(playback_es, nick);
	while (*playback_es++);
	strcpy(playback_es, dpy);
	while (*playback_es++);
	*playback_ei++ = team;
	strcpy(playback_es, addr);
	while (*playback_es++);
	strcpy(playback_es, host);
	while (*playback_es++);
	*playback_ei++ = version;
    }

    for (i = 0; i < max_connections; i++) {
	if (playback) {
	    if (i >= options.playerLimit_orig)
		break;
	}
	else if (rplayback && i < options.playerLimit_orig)
	    continue;
	connp = &Conn[i];
	if (connp->state == CONN_FREE) {
	    if (free_conn_index == max_connections)
		free_conn_index = i;
	    continue;
	}
	if (strcasecmp(connp->nick, nick) == 0) {
	    if (connp->state == CONN_LISTENING
		&& strcmp(user, connp->user) == 0
		&& strcmp(dpy, connp->dpy) == 0
		&& version == connp->version)
		/*
		 * May happen for multi-homed hosts
		 * and if previous packet got lost.
		 */
		return connp->my_port;
	    else
		/*
		 * Nick already in use.
		 */
		return -1;
	}
    }

    if (free_conn_index >= max_connections) {
	xpprintf("%s Full house for %s(%s)@%s(%s)\n",
		 showtime(), user, nick, host, dpy);
	return -1;
    }
    connp = &Conn[free_conn_index];

    if (!playback) {
	Create_client_socket(&sock, &my_port);
	if (rrecord) {
	    *playback_ei++ = sock.fd - min_fd;
	    *playback_ei++ = my_port;
	}
    } else {
	sock_init(&sock);
	sock.flags |= SOCK_FLAG_UDP;
	sock.fd = *playback_ei++;
	my_port = *playback_ei++;
    }
    if (sock.fd == -1)
 	return -1;

    Sockbuf_init(&connp->w, &sock, SERVER_SEND_SIZE,
		 SOCKBUF_WRITE | SOCKBUF_DGRAM);

    Sockbuf_init(&connp->r, &sock, SERVER_RECV_SIZE,
		 SOCKBUF_READ | SOCKBUF_DGRAM);

    Sockbuf_init(&connp->c, (sock_t *) NULL, MAX_SOCKBUF_SIZE,
		 SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK);

    connp->ind = free_conn_index;
    connp->my_port = my_port;
    connp->user = xp_strdup(user);
    connp->nick = xp_strdup(nick);
    connp->dpy = xp_strdup(dpy);
    connp->addr = xp_strdup(addr);
    connp->host = xp_strdup(host);
    connp->ship = NULL;
    connp->team = team;
    connp->version = version;
    Feature_init(connp);
    connp->start = main_loops;
    connp->magic = /*randomMT() +*/ my_port + sock.fd + team + main_loops;
    connp->id = NO_ID;
    connp->timeout = LISTEN_TIMEOUT;
    connp->last_key_change = 0;
    connp->reliable_offset = 0;
    connp->reliable_unsent = 0;
    connp->last_send_loops = 0;
    connp->retransmit_at_loop = 0;
    connp->rtt_retransmit = DEFAULT_RETRANSMIT;
    connp->rtt_smoothed = 0;
    connp->rtt_dev = 0;
    connp->rtt_timeouts = 0;
    connp->acks = 0;
    connp->setup = 0;
    connp->motd_offset = -1;
    connp->motd_stop = 0;
    connp->view_width = DEF_VIEW_SIZE;
    connp->view_height = DEF_VIEW_SIZE;
    connp->debris_colors = 0;
    connp->spark_rand = DEF_SPARK_RAND;
    connp->last_mouse_pos = 0;
    connp->rectype = rplayback ? 2-playback : 0;
    Conn_set_state(connp, CONN_LISTENING, CONN_FREE);
    if (connp->w.buf == NULL
	|| connp->r.buf == NULL
	|| connp->c.buf == NULL
	|| connp->user == NULL
	|| connp->nick == NULL
	|| connp->dpy == NULL
	|| connp->addr == NULL
	|| connp->host == NULL
	) {
	error("Not enough memory for connection");
	/* socket is not yet connected, but it doesn't matter much. */
	Destroy_connection(connp, "no memory");
	return -1;
    }

    install_input(Handle_input, sock.fd, connp);

    return my_port;
}

/*
 * Handle a connection that is in the listening state.
 */
static int Handle_listening(connection_t *connp)
{
    unsigned char type;
    int n;
    char nick[MAX_CHARS], user[MAX_CHARS];

    if (connp->state != CONN_LISTENING) {
	Destroy_connection(connp, "not listening");
	return -1;
    }
    Sockbuf_clear(&connp->r);
    errno = 0;
    n = sock_receive_anyRec(&connp->r.sock, connp->r.buf, connp->r.size);
    if (n <= 0) {
	if (n == 0 || errno == EWOULDBLOCK || errno == EAGAIN)
	    n = 0;
	else if (n != 0)
	    Destroy_connection(connp, "read first packet error");
	return n;
    }
    connp->r.len = n;
    connp->his_port = sock_get_last_portRec(&connp->r.sock);
    if (sock_connectRec(&connp->w.sock, connp->addr, connp->his_port) == -1) {
	error("Cannot connect datagram socket (%s,%d,%d,%d,%d)",
	      connp->addr, connp->his_port,
	      connp->w.sock.error.error,
	      connp->w.sock.error.call,
	      connp->w.sock.error.line);
	if (sock_get_error(&connp->w.sock)) {
	    error("sock_get_error fails too, giving up");
	    Destroy_connection(connp, "connect error");
	    return -1;
	}
	errno = 0;
	if (sock_connectRec(&connp->w.sock, connp->addr, connp->his_port)
	    == -1) {
	    error("Still cannot connect datagram socket (%s,%d,%d,%d,%d)",
		  connp->addr, connp->his_port,
		  connp->w.sock.error.error,
		  connp->w.sock.error.call,
		  connp->w.sock.error.line);
	    Destroy_connection(connp, "connect error");
	    return -1;
	}
    }
    xpprintf("%s Welcome %s=%s@%s|%s (%s/%d)", showtime(),
	     connp->nick, connp->user, connp->host, connp->dpy,
	     connp->addr, connp->his_port);
    xpprintf(" (version %04x)\n", connp->version);
    if (connp->r.ptr[0] != PKT_VERIFY) {
	Send_reply(connp, PKT_VERIFY, PKT_FAILURE);
	Send_reliable(connp);
	Destroy_connection(connp, "not connecting");
	return -1;
    }
    if ((n = Packet_scanf(&connp->r, "%c%s%s",
			  &type, user, nick)) <= 0) {
	Send_reply(connp, PKT_VERIFY, PKT_FAILURE);
	Send_reliable(connp);
	Destroy_connection(connp, "verify incomplete");
	return -1;
    }
    Fix_user_name(user);
    Fix_nick_name(nick);
    if (strcmp(user, connp->user) || strcmp(nick, connp->nick)) {
	xpprintf("%s Client verified incorrectly (%s,%s)(%s,%s)\n",
		 showtime(), user, nick, connp->user, connp->nick);
	Send_reply(connp, PKT_VERIFY, PKT_FAILURE);
	Send_reliable(connp);
	Destroy_connection(connp, "verify incorrect");
	return -1;
    }
    Sockbuf_clear(&connp->w);
    if (Send_reply(connp, PKT_VERIFY, PKT_SUCCESS) == -1
	|| Packet_printf(&connp->c, "%c%u", PKT_MAGIC, connp->magic) <= 0
	|| Send_reliable(connp) <= 0) {
	Destroy_connection(connp, "confirm failed");
	return -1;
    }

    Conn_set_state(connp, CONN_DRAIN, CONN_SETUP);

    return 1;	/* success! */
}

/*
 * Handle a connection that is in the transmit-server-configuration-data state.
 */
static int Handle_setup(connection_t *connp)
{
    char *buf;
    int n, len;
    setup_t *S;

    if (connp->state != CONN_SETUP) {
	Destroy_connection(connp, "not setup");
	return -1;
    }

    if (FEATURE(connp, F_POLY))
	S = Setup;
    else
	S = Oldsetup;

    if (connp->setup == 0) {
	if (!FEATURE(connp, F_POLY) || !is_polygon_map)
	    n = Packet_printf(&connp->c,
			      "%ld" "%ld%hd" "%hd%hd" "%hd%hd" "%s%s",
			      S->map_data_len,
			      S->mode, S->lives,
			      S->x, S->y,
			      options.framesPerSecond, S->map_order,
			      S->name, S->author);
	else
	    n = Packet_printf(&connp->c,
			      "%ld" "%ld%hd" "%hd%hd" "%hd%s" "%s%S",
			      S->map_data_len,
			      S->mode, S->lives,
			      S->width, S->height,
			      options.framesPerSecond, S->name,
			      S->author, S->data_url);
	if (n <= 0) {
	    Destroy_connection(connp, "setup 0 write error");
	    return -1;
	}
	connp->setup = (char *) &S->map_data[0] - (char *) S;
    }
    else if (connp->setup < S->setup_size) {
	if (connp->c.len > 0) {
	    /* If there is still unacked reliable data test for acks. */
	    Handle_input(-1, connp);
	    if (connp->state == CONN_FREE)
		return -1;
	}
    }
    if (connp->setup < S->setup_size) {
	len = MIN(connp->c.size, 4096) - connp->c.len;
	if (len <= 0)
	    /* Wait for acknowledgement of previously transmitted data. */
	    return 0;

	if (len > S->setup_size - connp->setup)
	    len = S->setup_size - connp->setup;
	buf = (char *) S;
	if (Sockbuf_writeRec(&connp->c, &buf[connp->setup], len) != len) {
	    Destroy_connection(connp, "sockbuf write setup error");
	    return -1;
	}
	connp->setup += len;
	if (len >= 512)
	    connp->start += (len * FPS) / (8 * 512) + 1;
    }
    if (connp->setup >= S->setup_size)
	Conn_set_state(connp, CONN_DRAIN, CONN_LOGIN);
#if 0
    if (CheckBanned(connp->user, connp->nick, connp->addr, connp->host)) {
	Destroy_connection(connp, "Banned from server, contact " LOCALGURU);
	return -1;
    }
    if (!CheckAllowed(connp->user, connp->nick, connp->addr, connp->host)) {
	Destroy_connection(connp, "Restricted nick, contact " LOCALGURU);
	return -1;
    }
#endif

    return 0;
}

/*
 * Ugly hack to prevent people from confusing Mara BMS.
 * This should be removed ASAP.
 */
static void UglyHack(char *string)
{
    static const char *we_dont_want_these_substrings[] = {
	"BALL", "Ball", "VAKK", "B A L L", "ball",
	"SAFE", "Safe", "safe", "S A F E",
	"COVER", "Cover", "cover", "INCOMING", "Incoming", "incoming",
	"POP", "Pop", "pop"
    };
    int i;

    for (i = 0; i < NELEM(we_dont_want_these_substrings); i++) {
	const char *substr = we_dont_want_these_substrings[i];
	char *s;

	/* not really needed, but here for safety */
	if (substr == NULL)
	    break;

	while ((s = strstr(string, substr)) != NULL)
	    *s = 'X';
    }
}

static void LegalizeName(char *string, bool hack)
{
    char *s = string;
    static char allowed_chars[] =
	" !#%&'()-.0123456789"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_abcdefghijklmnopqrstuvwxyz";

    if (hack)
	UglyHack(s);

    while (*s != '\0') {
	char ch = *s;
	if (!strchr(allowed_chars, ch))
	    ch = 'x';
	*s++ = ch;
    }
}

static void LegalizeHost(char *string)
{
    while (*string != '\0') {
	char ch = *string;
	if (!isalnum(ch) && ch != '.')
	    ch = '.';
	*string++ = ch;
    }
}

/*
 * A client has requested to start active play.
 * See if we can allocate a player structure for it
 * and if this succeeds update the player information
 * to all connected players.
 */
static int Handle_login(connection_t *connp, char *errmsg, size_t errsize)
{
    player_t *pl;
    int i, conn_bit;
    const char sender[] = "[*Server notice*]";

    if (BIT(world->rules->mode, TEAM_PLAY)) {
	if (connp->team < 0 || connp->team >= MAX_TEAMS
	    || (options.reserveRobotTeam
		&& (connp->team == options.robotTeam)))
	    connp->team = TEAM_NOT_SET;
	else if (world->teams[connp->team].NumBases <= 0)
	    connp->team = TEAM_NOT_SET;
	else {
	    Check_team_members(connp->team);
	    if (world->teams[connp->team].NumMembers
		- world->teams[connp->team].NumRobots
		>= world->teams[connp->team].NumBases)
		connp->team = TEAM_NOT_SET;
	}
	if (connp->team == TEAM_NOT_SET)
	    connp->team = Pick_team(PL_TYPE_HUMAN);
    } else
	connp->team = TEAM_NOT_SET;

    for (i = 0; i < NumPlayers; i++) {
	if (strcasecmp(Player_by_index(i)->name, connp->nick) == 0) {
	    warn("Name already in use %s", connp->nick);
	    strlcpy(errmsg, "Name already in use", errsize);
	    return -1;
	}
    }

    if (connp->rectype < 2) {
	if (!Init_player(NumPlayers, connp->ship, PL_TYPE_HUMAN)) {
	    strlcpy(errmsg, "Init_player failed: no free ID", errsize);
	    return -1;
	}
	pl = Player_by_index(NumPlayers);
    } else {
	if (!Init_player(spectatorStart + NumSpectators,
			 connp->ship, PL_TYPE_HUMAN))
	    return -1;
	pl = Player_by_index(spectatorStart + NumSpectators);
    }
    pl->rectype = connp->rectype;

    strlcpy(pl->name, connp->nick, MAX_CHARS);
    strlcpy(pl->username, connp->user, MAX_CHARS);
    strlcpy(pl->hostname, connp->host, MAX_CHARS);

    LegalizeName(pl->name, true);
    LegalizeName(pl->username, false);
    LegalizeHost(pl->hostname);

    pl->team = connp->team;
    pl->version = connp->version;

    if (pl->rectype < 2) {
	if (BIT(world->rules->mode, TEAM_PLAY) && pl->team == TEAM_NOT_SET) {
	    Player_set_state(pl, PL_STATE_PAUSED);
	    pl->home_base = NULL;
	    pl->team = 0;
	}
	else {
	    Pick_startpos(pl);
	    Go_home(pl);
	}
	Rank_get_saved_score(pl);
	if (pl->team != TEAM_NOT_SET && pl->home_base != NULL) {
	    team_t *teamp = Team_by_index(pl->team);

	    teamp->NumMembers++;
	}
	NumPlayers++;
	request_ID();
    } else {
	pl->id = NUM_IDS + 1 + connp->ind - spectatorStart;
	Add_spectator(pl);
    }

    connp->id = pl->id;
    pl->conn = connp;
    memset(pl->last_keyv, 0, sizeof(pl->last_keyv));
    memset(pl->prev_keyv, 0, sizeof(pl->prev_keyv));

    Conn_set_state(connp, CONN_READY, CONN_PLAYING);

    if (Send_reply(connp, PKT_PLAY, PKT_SUCCESS) <= 0) {
	strlcpy(errmsg, "Cannot send play reply", errsize);
	error("%s", errmsg);
	return -1;
    }

    if (pl->rectype < 2)
	xpprintf("%s %s (%d) starts at startpos %d.\n", showtime(),
		 pl->name, NumPlayers, pl->home_base ? pl->home_base->ind :
		 -1);
    else
	xpprintf("%s spectator %s (%d) starts.\n", showtime(), pl->name,
		 NumSpectators);

    /*
     * Tell him about himself first.
     */
    Send_player(pl->conn, pl->id);
    Send_score(pl->conn, pl->id,  Get_Score(pl),
	       pl->pl_life, pl->mychar, pl->alliance);
    if (pl->home_base) /* Spectators don't have bases */
	Send_base(pl->conn, pl->id, pl->home_base->ind);
    /*
     * And tell him about all the others.
     */
    for (i = 0; i < spectatorStart + NumSpectators - 1; i++) {
	player_t *pl_i;

	if (i == NumPlayers - 1 && pl->rectype != 2)
	    break;
	if (i == NumPlayers) {
	    i = spectatorStart - 1;
	    continue;
	}
	pl_i = Player_by_index(i);
	Send_player(pl->conn, pl_i->id);
	Send_score(pl->conn, pl_i->id, Get_Score(pl_i),
		   pl_i->pl_life, pl_i->mychar, pl_i->alliance);
	if (!Player_is_tank(pl_i) && pl_i->home_base != NULL)
	    Send_base(pl->conn, pl_i->id, pl_i->home_base->ind);
    }
    /*
     * And tell all the others about him.
     */
    for (i = 0; i < spectatorStart + NumSpectators - 1; i++) {
	player_t *pl_i;

	if (i == NumPlayers - 1) {
	    i = spectatorStart - 1;
	    continue;
	}
	pl_i = Player_by_index(i);
	if (pl_i->rectype == 1 && pl->rectype == 2)
	    continue;
	if (pl_i->conn != NULL) {
	    Send_player(pl_i->conn, pl->id);
	    Send_score(pl_i->conn, pl->id,  Get_Score(pl),
		       pl->pl_life, pl->mychar, pl->alliance);
	    if (pl->home_base)
		Send_base(pl_i->conn, pl->id, pl->home_base->ind);
	}
    }

    if (pl->rectype < 2) {
	if (NumPlayers == 1)
	    Set_message_f("Welcome to \"%s\", made by %s.",
			  world->name, world->author);
	else if (BIT(world->rules->mode, TEAM_PLAY))
	    Set_message_f("%s (%s, team %d) has entered \"%s\", made by %s.",
			  pl->name, pl->username, pl->team,
			  world->name, world->author);
	else
	    Set_message_f("%s (%s) has entered \"%s\", made by %s.",
			  pl->name, pl->username, world->name, world->author);
    }

    if (options.greeting)
	Set_player_message_f(pl, "%s [*Server greeting*]", options.greeting);

    if (connp->version < MY_VERSION) {
	Set_player_message_f(pl, "Server runs %s version %s. %s",
			     PACKAGE_NAME, VERSION, sender);

	if (!FEATURE(connp, F_FASTRADAR))
	    Set_player_message_f(pl, "Your client does not support the "
			       "fast radar packet. %s", sender);

	if (!FEATURE(connp, F_ASTEROID) && options.maxAsteroidDensity > 0)
	    Set_player_message_f(pl, "Your client will see asteroids as "
				 "balls. %s", sender);

	if (is_polygon_map && !FEATURE(connp, F_POLY)) {
	    Set_player_message_f(pl, "Your client doesn't support "
				 "polygon maps. What you see might not match "
				 "the real map. %s", sender);
	    Set_player_message_f(pl, "See http://xpilot.sourceforge.net/ for "
				 "for a client that supports polygon maps. %s",
				 sender);
	}
    }

    conn_bit = (1 << connp->ind);
    for (i = 0; i < Num_cannons(); i++) {
	cannon_t *cannon = Cannon_by_index(i);
	/*
	 * The client assumes at startup that all cannons are active.
	 */
	if (cannon->dead_ticks == 0)
	    SET_BIT(cannon->conn_mask, conn_bit);
	else
	    CLR_BIT(cannon->conn_mask, conn_bit);
    }
    for (i = 0; i < Num_fuels(); i++) {
	fuel_t *fs = Fuel_by_index(i);
	/*
	 * The client assumes at startup that all fuelstations are filled.
	 */
	if (fs->fuel == MAX_STATION_FUEL)
	    SET_BIT(fs->conn_mask, conn_bit);
	else
	    CLR_BIT(fs->conn_mask, conn_bit);
    }
    for (i = 0; i < Num_targets(); i++) {
	target_t *targ = Target_by_index(i);
	/*
	 * The client assumes at startup that all targets are not damaged.
	 */
	if (targ->dead_ticks == 0
	    && targ->damage == TARGET_DAMAGE) {
	    SET_BIT(targ->conn_mask, conn_bit);
	    CLR_BIT(targ->update_mask, conn_bit);
	} else {
	    CLR_BIT(targ->conn_mask, conn_bit);
	    SET_BIT(targ->update_mask, conn_bit);
	}
    }
    for (i = 0; i < num_polys; i++) {
	poly_t *poly = &pdata[i];

	/*
	 * The client assumes at startup that all polygons have their original
	 * style.
	 */
	if (poly->style == poly->current_style)
	    CLR_BIT(poly->update_mask, conn_bit);
	else
	    SET_BIT(poly->update_mask, conn_bit);
    }

    sound_player_init(pl);

    sound_play_all(START_SOUND);

    num_logins++;

    if (options.resetOnHuman > 0
	&& ((NumPlayers - NumPseudoPlayers - NumRobots)
	    <= options.resetOnHuman)) {
	if (BIT(world->rules->mode, TIMING))
	    Race_game_over();
	else if (BIT(world->rules->mode, TEAM_PLAY))
	    Team_game_over(-1, "");
	else if (BIT(world->rules->mode, LIMITED_LIVES))
	    Individual_game_over(-1);
    }

    if (NumPlayers == 1) {
	if (options.maxRoundTime > 0)
	    roundtime = options.maxRoundTime * FPS;
	else
	    roundtime = -1;
	Set_message_f("Player entered. Delaying 0 seconds until next %s.",
		      (BIT(world->rules->mode, TIMING) ? "race" : "round"));
    }

    return 0;
}

/*
 * Process a client packet.
 * The client may be in one of several states,
 * therefore we use function dispatch tables for easy processing.
 * Some functions may process requests from clients being
 * in different states.
 */
static int bytes[256];
static int bytes2;
int recSpecial;

static void Handle_input(int fd, void *arg)
{
    connection_t *connp = (connection_t *)arg;
    int type, result, (**receive_tbl)(connection_t *);
    short *pbscheck = NULL;
    char *pbdcheck = NULL;

    UNUSED_PARAM(fd);
    if (connp->state & (CONN_PLAYING | CONN_READY))
	receive_tbl = &playing_receive[0];
    else if (connp->state == CONN_LOGIN)
	receive_tbl = &login_receive[0];
    else if (connp->state & (CONN_DRAIN | CONN_SETUP))
	receive_tbl = &drain_receive[0];
    else if (connp->state == CONN_LISTENING) {
	Handle_listening(connp);
	return;
    } else {
	if (connp->state != CONN_FREE)
	    Destroy_connection(connp, "not input");
	return;
    }
    connp->num_keyboard_updates = 0;

    Sockbuf_clear(&connp->r);

    if (!recOpt || (!record && !playback)) {
	if (Sockbuf_readRec(&connp->r) == -1) {
	    Destroy_connection(connp, "input error");
	    return;
	}
    }
    else if (record) {
	if (Sockbuf_read(&connp->r) == -1) {
	    Destroy_connection(connp, "input error");
	    *playback_shorts++ = (short)0xffff;
	    return;
	}
	*playback_shorts++ = connp->r.len;
	memcpy(playback_data, connp->r.buf, (size_t)connp->r.len);
	playback_data += connp->r.len;
	pbdcheck = playback_data;
	pbscheck = playback_shorts;
    }
    else if (playback) {
	if ( (connp->r.len = *playback_shorts++) == 0xffff) {
	    Destroy_connection(connp, "input error");
	    return;
	}
	memcpy(connp->r.buf, playback_data, (size_t)connp->r.len);
	playback_data += connp->r.len;
    }

    if (connp->r.len <= 0)
	/* No input. */
	return;

    while (connp->r.ptr < connp->r.buf + connp->r.len) {
	char *pkt = connp->r.ptr;

	type = (connp->r.ptr[0] & 0xFF);
	recSpecial = 0;
	result = (*receive_tbl[type])(connp);
	if (result == -1)
	    /*
	     * Unrecoverable error.
	     * Connection has been destroyed.
	     */
	    return;

	if (record && recOpt && recSpecial && playback_data == pbdcheck &&
	    playback_shorts == pbscheck) {
	    int len = connp->r.ptr - pkt;

	    memmove(playback_data - (connp->r.buf + connp->r.len - pkt),
		    playback_data
		    - (connp->r.buf + connp->r.len - connp->r.ptr),
		    (size_t)(connp->r.buf + connp->r.len - connp->r.ptr));
	    playback_data -= len;
	    pbdcheck = playback_data;
	    if ( !(*(playback_shorts - 1) -= len) ) {
		playback_sched--;
		playback_shorts--;
	    }
	}

	if (playback == rplayback) {
	    bytes[type] += connp->r.ptr - pkt;
	    bytes2 += connp->r.ptr - pkt;
	}
	if (result == 0) {
	    /*
	     * Incomplete client packet.
	     * Drop rest of packet.
	     * OPTIMIZED RECORDING MIGHT NOT WORK CORRECTLY
	     */
	    Sockbuf_clear(&connp->r);
	    xpprintf("Incomplete packet\n");
	    break;
	}
	if (connp->state == CONN_PLAYING)
	    connp->start = main_loops;
    }
}

int Input(void)
{
    int i, num_reliable = 0;
    connection_t *input_reliable[MAX_SELECT_FD], *connp;
    char msg[MSG_LEN];

    for (i = 0; i < max_connections; i++) {
	connp = &Conn[i];
	playback = (connp->rectype == 1);
	if (connp->state == CONN_FREE)
	    continue;
	if ((!(playback && recOpt)
	     && connp->start + connp->timeout * FPS < main_loops)
	    || (playback && recOpt && *playback_opttout == main_loops
		&& *(playback_opttout + 1) == i)) {
	    if (playback && recOpt)
		playback_opttout += 2;
	    else if (record & recOpt) {
		*playback_opttout++ = main_loops;
		*playback_opttout++ = i;
	    }
	    /*
	     * Timeout this fellow if we have not heard a single thing
	     * from him for a long time.
	     */
	    if (connp->state & (CONN_PLAYING | CONN_READY))
		Set_message_f("%s mysteriously disappeared!?", connp->nick);

	    snprintf(msg, sizeof(msg), "timeout %02x", connp->state);
	    Destroy_connection(connp, msg);
	    continue;
	}
	if (connp->state != CONN_PLAYING) {
	    input_reliable[num_reliable++] = connp;
	    if (connp->state == CONN_SETUP) {
		Handle_setup(connp);
		continue;
	    }
	}
    }

    for (i = 0; i < num_reliable; i++) {
	connp = input_reliable[i];
	playback = (connp->rectype == 1);
	if (connp->state & (CONN_DRAIN | CONN_READY | CONN_SETUP
			    | CONN_LOGIN)) {
	    if (connp->c.len > 0) {
		if (Send_reliable(connp) == -1)
		    continue;
	    }
	}
    }

    if (num_logins | num_logouts) {
	/* Tell the meta server */
	Meta_update(true);
	num_logins = 0;
	num_logouts = 0;
    }

    playback = rplayback;
    record = rrecord;

    return login_in_progress;
}

/*
 * Send a reply to a special client request.
 * Not used consistently everywhere.
 * It could be used to setup some form of reliable
 * communication from the client to the server.
 */
int Send_reply(connection_t *connp, int replyto, int result)
{
    int n;

    n = Packet_printf(&connp->c, "%c%c%c", PKT_REPLY, replyto, result);
    if (n == -1) {
	Destroy_connection(connp, "write error");
	return -1;
    }
    return n;
}

static int Send_modifiers(connection_t *connp, char *mods)
{
    return Packet_printf(&connp->w, "%c%s", PKT_MODIFIERS, mods);
}

/*
 * Send items.
 * The advantage of this scheme is that it only uses bytes for items
 * which the player actually owns.  This reduces the packet size.
 * Another advantage is that here it doesn't matter if an old client
 * receives counts for items it doesn't know about.
 * This is new since pack version 4203.
 */
static int Send_self_items(connection_t *connp, player_t *pl)
{
    unsigned item_mask = 0;
    int i, n, item_count = 0;

    /* build mask with one bit for each item type which the player owns. */
    for (i = 0; i < NUM_ITEMS; i++) {
	if (pl->item[i] > 0) {
	    item_mask |= (1 << i);
	    item_count++;
	}
    }
    /* don't send anything if there are no items. */
    if (item_count == 0)
	return 1;

    /* check if enough buffer space is available for the complete packet. */
    if (connp->w.size - connp->w.len <= 5 + item_count)
	return 0;

    /* build the header. */
    n = Packet_printf(&connp->w, "%c%u", PKT_SELF_ITEMS, item_mask);
    if (n <= 0)
	return n;

    /* build rest of packet containing the per item counts. */
    for (i = 0; i < NUM_ITEMS; i++) {
	if (item_mask & (1 << i))
	    connp->w.buf[connp->w.len++] = pl->item[i];
    }
    /* return the number of bytes added to the packet. */
    return 5 + item_count;
}

/*
 * Send all frame data related to the player self and his HUD.
 */
int Send_self(connection_t *connp,
	      player_t *pl,
	      int lock_id,
	      int lock_dist,
	      int lock_dir,
	      int autopilotlight,
	      int status,
	      char *mods)
{
    int n;

    /* assumes connp->version >= 0x4203 */
    n = Packet_printf(&connp->w,
		      "%c"
		      "%hd%hd%hd%hd%c"
		      "%c%c%c"
		      "%hd%hd%c%c"
		      "%c%hd%hd"
		      "%hd%hd%c"
		      "%c%c"
		      ,
		      PKT_SELF,
		      CLICK_TO_PIXEL(pl->pos.cx), CLICK_TO_PIXEL(pl->pos.cy),
		      (int) pl->vel.x, (int) pl->vel.y,
		      pl->dir,
		      (int) (pl->power + 0.5),
		      (int) (pl->turnspeed + 0.5),
		      (int) (pl->turnresistance * 255.0 + 0.5),
		      lock_id, lock_dist, lock_dir,
		      pl->check,

		      pl->fuel.current,
		      (int)(pl->fuel.sum + 0.5),
		      (int)(pl->fuel.max + 0.5),

		      connp->view_width, connp->view_height,
		      connp->debris_colors,

		      (uint8_t)status,
		      autopilotlight
	);
    if (n <= 0)
	return n;

    n = Send_self_items(connp, pl);
    if (n <= 0)
	return n;

    return Send_modifiers(connp, mods);
}

/*
 * Somebody is leaving the game.
 */
int Send_leave(connection_t *connp, int id)
{
    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for leave info (%d,%d)",
	      connp->state, connp->id);
	return 0;
    }
    return Packet_printf(&connp->c, "%c%hd", PKT_LEAVE, id);
}

/*
 * Somebody is joining the game.
 */
int Send_player(connection_t *connp, int id)
{
    player_t *pl = Player_by_id(id);
    int n, sbuf_len = connp->c.len, himself = (pl->conn == connp);
    char buf[MSG_LEN], ext[MSG_LEN];

    if (!BIT(connp->state, CONN_PLAYING|CONN_READY)) {
	warn("Connection not ready for player info (%d,%d)",
	     connp->state, connp->id);
	return 0;
    }
    Convert_ship_2_string(pl->ship, buf, ext, 0x3200);
    n = Packet_printf(&connp->c,
		      "%c%hd" "%c%c" "%s%s%s" "%S",
		      PKT_PLAYER, pl->id,
		      pl->team, pl->mychar,
		      pl->name, pl->username, pl->hostname,
		      buf);
    if (n > 0) {
	if (!FEATURE(connp, F_EXPLICITSELF))
	    n = Packet_printf(&connp->c, "%S", ext);
	else
	    n = Packet_printf(&connp->c, "%S%c", ext, himself);
	if (n <= 0)
	    connp->c.len = sbuf_len;
    }
    return n;
}

int Send_team(connection_t *connp, int id, int team)
{
    /*
     * No way to send only team to old clients, all player info has to be
     * resent. This only works if pl->team really is the same as team.
     */
    if (!FEATURE(connp, F_SENDTEAM))
	return Send_player(connp, id);

    if (!BIT(connp->state, CONN_PLAYING|CONN_READY)) {
	warn("Connection not ready for team info (%d,%d)",
	      connp->state, connp->id);
	return 0;
    }

    return Packet_printf(&connp->c, "%c%hd%c", PKT_TEAM, id, team);
}

/*
 * Send the new score for some player to a client.
 */
int Send_score(connection_t *connp, int id, double score,
	       int life, int mychar, int alliance)
{
    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for score(%d,%d)",
	    connp->state, connp->id);
	return 0;
    }
    if (!FEATURE(connp, F_FLOATSCORE))
	/* older clients don't get alliance info or decimals of the score */
	return Packet_printf(&connp->c, "%c%hd%hd%hd%c", PKT_SCORE,
			     id, (int)(score + (score > 0 ? 0.5 : -0.5)),
			     life, mychar);
    else {
	int allchar = ' ';

	if (alliance != ALLIANCE_NOT_SET) {
	    if (options.announceAlliances)
		allchar = alliance + '0';
	    else {
		if (Player_by_id(connp->id)->alliance == alliance)
		    allchar = '+';
	    }
	}
	return Packet_printf(&connp->c, "%c%hd%d%hd%c%c", PKT_SCORE, id,
			     (int)(score * 100 + (score > 0 ? 0.5 : -0.5)),
			     life, mychar, allchar);
    }
}

/*
 * Send the new race info for some player to a client.
 */
int Send_timing(connection_t *connp, int id, int check, int round)
{
    int num_checks = OLD_MAX_CHECKS;

    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for timing(%d,%d)",
	      connp->state, connp->id);
	return 0;
    }
    if (is_polygon_map)
	num_checks = world->NumChecks;
    return Packet_printf(&connp->c, "%c%hd%hu", PKT_TIMING,
			 id, round * num_checks + check);
}

/*
 * Send info about a player having which base.
 */
int Send_base(connection_t *connp, int id, int num)
{
    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for base info (%d,%d)",
	    connp->state, connp->id);
	return 0;
    }
    return Packet_printf(&connp->c, "%c%hd%hu", PKT_BASE, id, num);
}

/*
 * Send the amount of fuel in a fuelstation.
 */
int Send_fuel(connection_t *connp, int num, double fuel)
{
    return Packet_printf(&connp->w, "%c%hu%hu", PKT_FUEL,
			 num, (int)(fuel + 0.5));
}

int Send_score_object(connection_t *connp, double score, clpos_t pos,
		      const char *string)
{
    blkpos_t bpos = Clpos_to_blkpos(pos);

    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for base info (%d,%d)",
	    connp->state, connp->id);
	return 0;
    }

    if (!FEATURE(connp, F_FLOATSCORE))
	return Packet_printf(&connp->c, "%c%hd%hu%hu%s", PKT_SCORE_OBJECT,
			     (int)(score + (score > 0 ? 0.5 : -0.5)),
			     bpos.bx, bpos.by, string);
    else
	return Packet_printf(&connp->c, "%c%d%hu%hu%s", PKT_SCORE_OBJECT,
			     (int)(score * 100 + (score > 0 ? 0.5 : -0.5)),
			     bpos.bx, bpos.by, string);
}

int Send_cannon(connection_t *connp, int num, int dead_ticks)
{
    if (FEATURE(connp, F_POLY))
	return 0;
    return Packet_printf(&connp->w, "%c%hu%hu", PKT_CANNON, num, dead_ticks);
}

int Send_destruct(connection_t *connp, int count)
{
    return Packet_printf(&connp->w, "%c%hd", PKT_DESTRUCT, count);
}

int Send_shutdown(connection_t *connp, int count, int delay)
{
    return Packet_printf(&connp->w, "%c%hd%hd", PKT_SHUTDOWN,
			 count, delay);
}

int Send_thrusttime(connection_t *connp, int count, int max)
{
    return Packet_printf(&connp->w, "%c%hd%hd", PKT_THRUSTTIME, count, max);
}

int Send_shieldtime(connection_t *connp, int count, int max)
{
    return Packet_printf(&connp->w, "%c%hd%hd", PKT_SHIELDTIME, count, max);
}

int Send_phasingtime(connection_t *connp, int count, int max)
{
    return Packet_printf(&connp->w, "%c%hd%hd", PKT_PHASINGTIME, count, max);
}

int Send_debris(connection_t *connp, int type, unsigned char *p, unsigned n)
{
    int avail;
    sockbuf_t *w = &connp->w;

    if ((n & 0xFF) != n) {
	warn("Bad number of debris %d", n);
	return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 2;
    if ((int)n * 2 >= avail) {
	if (avail > 2)
	    n = (avail - 1) / 2;
	else
	    return 0;
    }
    w->buf[w->len++] = PKT_DEBRIS + type;
    w->buf[w->len++] = n;
    memcpy(&w->buf[w->len], p, n * 2);
    w->len += n * 2;

    return n;
}

int Send_wreckage(connection_t *connp, clpos_t pos,
		  int wrtype, int size, int rot)
{
    if (options.wreckageCollisionMayKill)
	/* Set the highest bit when wreckage is deadly. */
	wrtype |= 0x80;
    else
	wrtype &= ~0x80;

    return Packet_printf(&connp->w, "%c%hd%hd%c%c%c", PKT_WRECKAGE,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 wrtype, size, rot);
}

int Send_asteroid(connection_t *connp, clpos_t pos,
		  int type, int size, int rot)
{
    u_byte type_size;
    int x = CLICK_TO_PIXEL(pos.cx), y = CLICK_TO_PIXEL(pos.cy);

    if (!FEATURE(connp, F_ASTEROID))
	return Send_ecm(connp, pos, 2 * (int) ASTEROID_RADIUS(size) / CLICK);

    type_size = ((type & 0x0F) << 4) | (size & 0x0F);

    return Packet_printf(&connp->w, "%c%hd%hd%c%c", PKT_ASTEROID,
		         x, y, type_size, rot);
}

int Send_fastshot(connection_t *connp, int type, unsigned char *p, unsigned n)
{
    int avail;
    sockbuf_t *w = &connp->w;

    if ((n & 0xFF) != n) {
	warn("Bad number of fastshot %d", n);
	return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 3;
    if ((int)n * 2 >= avail) {
	if (avail > 2)
	    n = (avail - 1) / 2;
	else
	    return 0;
    }
    w->buf[w->len++] = PKT_FASTSHOT;
    w->buf[w->len++] = type;
    w->buf[w->len++] = n;
    memcpy(&w->buf[w->len], p, n * 2);
    w->len += n * 2;

    return n;
}

int Send_missile(connection_t *connp, clpos_t pos, int len, int dir)
{
    return Packet_printf(&connp->w, "%c%hd%hd%c%c", PKT_MISSILE,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 len, dir);
}

int Send_ball(connection_t *connp, clpos_t pos, int id, int style)
{
    if (FEATURE(connp, F_BALLSTYLE))
	return Packet_printf(&connp->w, "%c%hd%hd%hd%c", PKT_BALL,
			     CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			     id, style);

     return Packet_printf(&connp->w, "%c%hd%hd%hd", PKT_BALL,
 			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy), id);
}

int Send_mine(connection_t *connp, clpos_t pos, int teammine, int id)
{
    return Packet_printf(&connp->w, "%c%hd%hd%c%hd", PKT_MINE,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 teammine, id);
}

int Send_target(connection_t *connp, int num, int dead_ticks, double damage)
{
    if (FEATURE(connp, F_POLY))
	return 0;
    return Packet_printf(&connp->w, "%c%hu%hu%hu", PKT_TARGET,
			 num, dead_ticks, (int)(damage * 256.0));
}

int Send_polystyle(connection_t *connp, int polyind, int newstyle)
{
    if (!FEATURE(connp, F_POLYSTYLE))
	return 0;
    return Packet_printf(&connp->w, "%c%hu%hu", PKT_POLYSTYLE,
			 polyind, newstyle);
}

int Send_wormhole(connection_t *connp, clpos_t pos)
{
    int x = CLICK_TO_PIXEL(pos.cx), y = CLICK_TO_PIXEL(pos.cy);

    if (!FEATURE(connp, F_TEMPWORM))
	return Send_ecm(connp, pos, BLOCK_SZ - 2);
    return Packet_printf(&connp->w, "%c%hd%hd", PKT_WORMHOLE, x, y);
}

int Send_item(connection_t *connp, clpos_t pos, int type)
{
    return Packet_printf(&connp->w, "%c%hd%hd%c", PKT_ITEM,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy), type);
}

int Send_paused(connection_t *connp, clpos_t pos, int count)
{
    return Packet_printf(&connp->w, "%c%hd%hd%hd", PKT_PAUSED,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 count);
}

int Send_appearing(connection_t *connp, clpos_t pos, int id, int count)
{
    if (!FEATURE(connp, F_SHOW_APPEARING))
	return 0;

    return Packet_printf(&connp->w, "%c%hd%hd%hd%hd", PKT_APPEARING,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 id, count);
}

int Send_ecm(connection_t *connp, clpos_t pos, int size)
{
    return Packet_printf(&connp->w, "%c%hd%hd%hd", PKT_ECM,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy), size);
}

int Send_trans(connection_t *connp, clpos_t pos1, clpos_t pos2)
{
    return Packet_printf(&connp->w,"%c%hd%hd%hd%hd", PKT_TRANS,
			 CLICK_TO_PIXEL(pos1.cx), CLICK_TO_PIXEL(pos1.cy),
			 CLICK_TO_PIXEL(pos2.cx), CLICK_TO_PIXEL(pos2.cy));
}

int Send_ship(connection_t *connp, clpos_t pos, int id, int dir,
	      int shield, int cloak, int emergency_shield, int phased,
	      int deflector)
{
    if (!FEATURE(connp, F_SEPARATEPHASING))
	cloak |= phased;
    return Packet_printf(&connp->w,
			 "%c%hd%hd%hd" "%c" "%c",
			 PKT_SHIP,
			 CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy), id,
			 dir,
			 (shield != 0)
			 | ((cloak != 0) << 1)
			 | ((emergency_shield != 0) << 2)
			 | ((phased != 0) << 3)
			 | ((deflector != 0) << 4)
			);
}

int Send_refuel(connection_t *connp, clpos_t pos1, clpos_t pos2)
{
    return Packet_printf(&connp->w,
			 "%c%hd%hd%hd%hd",
			 PKT_REFUEL,
			 CLICK_TO_PIXEL(pos1.cx), CLICK_TO_PIXEL(pos1.cy),
			 CLICK_TO_PIXEL(pos2.cx), CLICK_TO_PIXEL(pos2.cy));
}

int Send_connector(connection_t *connp, clpos_t pos1, clpos_t pos2,
		   int tractor)
{
    return Packet_printf(&connp->w,
			 "%c%hd%hd%hd%hd%c",
			 PKT_CONNECTOR,
			 CLICK_TO_PIXEL(pos1.cx), CLICK_TO_PIXEL(pos1.cy),
			 CLICK_TO_PIXEL(pos2.cx), CLICK_TO_PIXEL(pos2.cy),
			 tractor);
}

int Send_laser(connection_t *connp, int color, clpos_t pos, int len, int dir)
{
    return Packet_printf(&connp->w, "%c%c%hd%hd%hd%c", PKT_LASER,
			 color, CLICK_TO_PIXEL(pos.cx), CLICK_TO_PIXEL(pos.cy),
			 len, dir);
}

int Send_radar(connection_t *connp, int x, int y, int size)
{
    if (!FEATURE(connp, F_TEAMRADAR))
	size &= ~0x80;

    return Packet_printf(&connp->w, "%c%hd%hd%c", PKT_RADAR, x, y, size);
}

int Send_fastradar(connection_t *connp, unsigned char *buf, unsigned n)
{
    int avail;
    sockbuf_t *w = &connp->w;

    if ((n & 0xFF) != n) {
	warn("Bad number of fastradar %d", n);
	return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 3;
    if ((int)n * 3 >= avail) {
	if (avail > 3)
	    n = (avail - 2) / 3;
	else
	    return 0;
    }
    w->buf[w->len++] = PKT_FASTRADAR;
    w->buf[w->len++] = (unsigned char)(n & 0xFF);
    memcpy(&w->buf[w->len], buf, (size_t)n * 3);
    w->len += n * 3;

    return (2 + (n * 3));
}

int Send_damaged(connection_t *connp, int damaged)
{
    return Packet_printf(&connp->w, "%c%c", PKT_DAMAGED, damaged);
}

int Send_audio(connection_t *connp, int type, int vol)
{
    if (connp->w.size - connp->w.len <= 32)
	return 0;
    return Packet_printf(&connp->w, "%c%c%c", PKT_AUDIO, type, vol);
}

int Send_time_left(connection_t *connp, long sec)
{
    return Packet_printf(&connp->w, "%c%ld", PKT_TIME_LEFT, sec);
}

int Send_eyes(connection_t *connp, int id)
{
    return Packet_printf(&connp->w, "%c%hd", PKT_EYES, id);
}

int Send_message(connection_t *connp, const char *msg)
{
    if (!BIT(connp->state, CONN_PLAYING | CONN_READY)) {
	warn("Connection not ready for message (%d,%d)",
	    connp->state, connp->id);
	return 0;
    }
    return Packet_printf(&connp->c, "%c%S", PKT_MESSAGE, msg);
}

int Send_loseitem(connection_t *connp, int lose_item_index)
{
    return Packet_printf(&connp->w, "%c%c", PKT_LOSEITEM, lose_item_index);
}

int Send_start_of_frame(connection_t *connp)
{
    if (connp->state != CONN_PLAYING) {
	if (connp->state != CONN_READY)
	    warn("Connection not ready for frame (%d,%d)",
		 connp->state, connp->id);
	return -1;
    }
    /*
     * We tell the client which frame number this is and
     * which keyboard update we have last received.
     */
    Sockbuf_clear(&connp->w);
    if (Packet_printf(&connp->w,
		      "%c%ld%ld",
		      PKT_START, frame_loops, connp->last_key_change) <= 0) {
	Destroy_connection(connp, "write error");
	return -1;
    }

    /* Return ok */
    return 0;
}

int Send_end_of_frame(connection_t *connp)
{
    int			n;

    last_packet_of_frame = 1;
    n = Packet_printf(&connp->w, "%c%ld", PKT_END, frame_loops);
    last_packet_of_frame = 0;
    if (n == -1) {
	Destroy_connection(connp, "write error");
	return -1;
    }
    if (n == 0) {
	/*
	 * Frame update size exceeded buffer size.
	 * Drop this packet.
	 */
	Sockbuf_clear(&connp->w);
	return 0;
    }
    while (connp->motd_offset >= 0
	&& connp->c.len + connp->w.len < MAX_RELIABLE_DATA_PACKET_SIZE)
	Send_motd(connp);

    if (connp->c.len > 0 && connp->w.len < MAX_RELIABLE_DATA_PACKET_SIZE) {
	if (Send_reliable(connp) == -1)
	    return -1;
	if (connp->w.len == 0)
	    return 1;
    }
    if (Sockbuf_flushRec(&connp->w) == -1) {
	Destroy_connection(connp, "flush error");
	return -1;
    }
    Sockbuf_clear(&connp->w);
    return 0;
}

static int Receive_keyboard(connection_t *connp)
{
    player_t *pl;
    long change;
    u_byte ch;
    size_t size = KEYBOARD_SIZE;

    if (connp->r.ptr - connp->r.buf + (int)size + 1 + 4 > connp->r.len)
	/*
	 * Incomplete client packet.
	 */
	return 0;

    Packet_scanf(&connp->r, "%c%ld", &ch, &change);
    if (change <= connp->last_key_change)
	/*
	 * We already have this key.
	 * Nothing to do.
	 */
	connp->r.ptr += size;
    else {
	connp->last_key_change = change;
	pl = Player_by_id(connp->id);
	memcpy(pl->last_keyv, connp->r.ptr, size);
	connp->r.ptr += size;
	Handle_keyboard(pl);
    }
    if (connp->num_keyboard_updates++ && (connp->state & CONN_PLAYING)) {
	Destroy_connection(connp, "no macros");
	return -1;
    }

    return 1;
}

static int Receive_quit(connection_t *connp)
{
    Destroy_connection(connp, "client quit");

    return -1;
}

static int Receive_play(connection_t *connp)
{
    unsigned char ch;
    int n;
    char errmsg[MAX_CHARS];

    if ((n = Packet_scanf(&connp->r, "%c", &ch)) != 1) {
	warn("Cannot receive play packet");
	Destroy_connection(connp, "receive error");
	return -1;
    }
    if (ch != PKT_PLAY) {
	warn("Packet is not of play type");
	Destroy_connection(connp, "not play");
	return -1;
    }
    if (connp->state != CONN_LOGIN) {
	if (connp->state != CONN_PLAYING) {
	    if (connp->state == CONN_READY) {
		connp->r.ptr = connp->r.buf + connp->r.len;
		return 0;
	    }
	    warn("Connection not in login state (%02x)", connp->state);
	    Destroy_connection(connp, "not login");
	    return -1;
	}
	if (Send_reliable(connp) == -1)
	    return -1;
	return 0;
    }
    Sockbuf_clear(&connp->w);
    strlcpy(errmsg, "login failed", sizeof(errmsg));
    if (Handle_login(connp, errmsg, sizeof(errmsg)) == -1) {
	Destroy_connection(connp, errmsg);
	return -1;
    }

    return 2;
}

static int Receive_power(connection_t *connp)
{
    player_t *pl;
    unsigned char ch;
    short tmp;
    int n, autopilot;
    double power;

    if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &tmp)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    power = (double) tmp / 256.0F;
    pl = Player_by_id(connp->id);
    autopilot = Player_uses_autopilot(pl) ? 1 : 0;

    switch (ch) {
    case PKT_POWER:
	LIMIT(power, MIN_PLAYER_POWER, MAX_PLAYER_POWER);
	if (autopilot)
	    pl->auto_power_s = power;
	else
	    pl->power = power;
	break;
    case PKT_POWER_S:
	LIMIT(power, MIN_PLAYER_POWER, MAX_PLAYER_POWER);
	pl->power_s = power;
	break;
    case PKT_TURNSPEED:
	LIMIT(power, MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED);
	if (autopilot)
	    pl->auto_turnspeed_s = power;
	else
	    pl->turnspeed = power;
	break;
    case PKT_TURNSPEED_S:
	LIMIT(power, MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED);
	pl->turnspeed_s = power;
	break;
    case PKT_TURNRESISTANCE:
	LIMIT(power, MIN_PLAYER_TURNRESISTANCE, MAX_PLAYER_TURNRESISTANCE);
	if (autopilot)
	    pl->auto_turnresistance_s = power;
	else
	    pl->turnresistance = power;
	break;
    case PKT_TURNRESISTANCE_S:
	LIMIT(power, MIN_PLAYER_TURNRESISTANCE, MAX_PLAYER_TURNRESISTANCE);
	pl->turnresistance_s = power;
	break;
    default:
	warn("Not a power packet (%d,%02x)", ch, connp->state);
	Destroy_connection(connp, "not power");
	return -1;
    }
    return 1;
}

/*
 * Send the reliable data.
 * If the client is in the receive-frame-updates state then
 * all reliable data is piggybacked at the end of the
 * frame update packets.  (Except maybe for the MOTD data, which
 * could be transmitted in its own packets since MOTDs can be big.)
 * Otherwise if the client is not actively playing yet then
 * the reliable data is sent in its own packets since there
 * is no other data to combine it with.
 *
 * This thing still is not finished, but it works better than in 3.0.0 I hope.
 */
int Send_reliable(connection_t *connp)
{
    char *read_buf;
    int i, n, len, todo, max_todo;
    long rel_off;
    const int max_packet_size = MAX_RELIABLE_DATA_PACKET_SIZE,
	min_send_size = 1;  /* was 4 in 3.0.7, 1 in 3.1.0 */

    if (connp->c.len <= 0
	|| connp->last_send_loops == main_loops) {
	connp->last_send_loops = main_loops;
	return 0;
    }
    read_buf = connp->c.buf;
    max_todo = connp->c.len;
    rel_off = connp->reliable_offset;
    if (connp->w.len > 0) {
	/* We are piggybacking on a frame update. */
	if (connp->w.len >= max_packet_size - min_send_size)
	    /* Frame already too big */
	    return 0;

	if (max_todo > max_packet_size - connp->w.len)
	    /* Do not exceed minimum fragment size. */
	    max_todo = max_packet_size - connp->w.len;
    }
    if (connp->retransmit_at_loop > main_loops) {
	/*
	 * It is no time to retransmit yet.
	 */
	if (max_todo <= connp->reliable_unsent - connp->reliable_offset
			+ min_send_size
	    || connp->w.len == 0)
	    /*
	     * And we cannot send anything new either
	     * and we do not want to introduce a new packet.
	     */
	    return 0;
    }
    else if (connp->retransmit_at_loop != 0)
	/*
	 * Timeout.
	 * Either our packet or the acknowledgement got lost,
	 * so retransmit.
	 */
	connp->acks >>= 1;

    todo = max_todo;
    for (i = 0; i <= connp->acks && todo > 0; i++) {
	len = (todo > max_packet_size) ? max_packet_size : todo;
	if (Packet_printf(&connp->w, "%c%hd%ld%ld", PKT_RELIABLE,
			  len, rel_off, main_loops) <= 0
	    || Sockbuf_write(&connp->w, read_buf, len) != len) {
	    error("Cannot write reliable data");
	    Destroy_connection(connp, "write error");
	    return -1;
	}
	if ((n = Sockbuf_flushRec(&connp->w)) < len) {
	    if (n == 0
		&& (errno == EWOULDBLOCK
		    || errno == EAGAIN)) {
		connp->acks = 0;
		break;
	    } else {
		error("Cannot flush reliable data (%d)", n);
		Destroy_connection(connp, "flush error");
		return -1;
	    }
	}
	todo -= len;
	rel_off += len;
	read_buf += len;
    }

    /*
     * Drop rest of outgoing data packet if something remains at all.
     */
    Sockbuf_clear(&connp->w);

    connp->last_send_loops = main_loops;

    if (max_todo - todo <= 0)
	/*
	 * We have not transmitted anything at all.
	 */
	return 0;

    /*
     * Retransmission timer with exponential backoff.
     */
    if (connp->rtt_retransmit > MAX_RETRANSMIT)
	connp->rtt_retransmit = MAX_RETRANSMIT;
    if (connp->retransmit_at_loop <= main_loops) {
	connp->retransmit_at_loop = main_loops + connp->rtt_retransmit;
	connp->rtt_retransmit <<= 1;
	connp->rtt_timeouts++;
    } else
	connp->retransmit_at_loop = main_loops + connp->rtt_retransmit;

    if (rel_off > connp->reliable_unsent)
	connp->reliable_unsent = rel_off;

    return (max_todo - todo);
}

static int Receive_ack(connection_t *connp)
{
    int n;
    unsigned char ch;
    long rel, rtt;	/* RoundTrip Time */
    long diff, delta, rel_loops;
    
    if ((n = Packet_scanf(&connp->r, "%c%ld%ld",
			  &ch, &rel, &rel_loops)) <= 0) {
	warn("Cannot read ack packet (%d)", n);
	Destroy_connection(connp, "read error");
	return -1;
    }
    if (ch != PKT_ACK) {
	warn("Not an ack packet (%d)", ch);
	Destroy_connection(connp, "not ack");
	return -1;
    }
    rtt = main_loops - rel_loops;
    if (rtt > 0 && rtt <= MAX_RTT) {
	/*
	 * These roundtrip estimation calculations are derived from Comer's
	 * books "Internetworking with TCP/IP" parts I & II.
	 */
	if (connp->rtt_smoothed == 0)
	    /*
	     * Initialize the rtt estimator by this first measurement.
	     * The estimator is scaled by 3 bits.
	     */
	    connp->rtt_smoothed = rtt << 3;
	/*
	 * Scale the estimator back by 3 bits before calculating the error.
	 */
	delta = rtt - (connp->rtt_smoothed >> 3);
	/*
	 * Add one eigth of the error to the estimator.
	 */
	connp->rtt_smoothed += delta;
	/*
	 * Now we need the absolute value of the error.
	 */
	if (delta < 0)
	    delta = -delta;
	/*
	 * The rtt deviation is scaled by 2 bits.
	 * Now we add one fourth of the difference between the
	 * error and the previous deviation to the deviation.
	 */
	connp->rtt_dev += delta - (connp->rtt_dev >> 2);
	/*
	 * The calculation of the retransmission timeout is what this is
	 * all about.  We take the smoothed rtt plus twice the deviation
	 * as the next retransmission timeout to use.  Because of the
	 * scaling used we get the following statement:
	 */
	connp->rtt_retransmit = ((connp->rtt_smoothed >> 2)
	    + connp->rtt_dev) >> 1;
	/*
	 * Now keep it within reasonable bounds.
	 */
	if (connp->rtt_retransmit < MIN_RETRANSMIT)
	    connp->rtt_retransmit = MIN_RETRANSMIT;
    }
    diff = rel - connp->reliable_offset;
    if (diff > connp->c.len) {
	/* Impossible to ack data that has not been send */
	warn("Bad ack (diff=%ld,cru=%ld,c=%ld,len=%d)",
	    diff, rel, connp->reliable_offset, connp->c.len);
	Destroy_connection(connp, "bad ack");
	return -1;
    }
    else if (diff <= 0)
	/* Late or duplicate ack of old data.  Discard. */
	return 1;
    Sockbuf_advance(&connp->c, (int) diff);
    connp->reliable_offset += diff;
    if ((n = ((diff + 512 - 1) / 512)) > connp->acks)
	connp->acks = n;
    else
	connp->acks++;

    if (connp->reliable_offset >= connp->reliable_unsent) {
	/*
	 * All reliable data has been sent and acked.
	 */
	connp->retransmit_at_loop = 0;
	if (connp->state == CONN_DRAIN)
	    Conn_set_state(connp, connp->drain_state, connp->drain_state);
    }
    if (connp->state == CONN_READY
	&& (connp->c.len <= 0
	    || (connp->c.buf[0] != PKT_REPLY
		&& connp->c.buf[0] != PKT_PLAY
		&& connp->c.buf[0] != PKT_SUCCESS
		&& connp->c.buf[0] != PKT_FAILURE)))
	Conn_set_state(connp, connp->drain_state, connp->drain_state);

    connp->rtt_timeouts = 0;

    return 1;
}

static int Receive_discard(connection_t *connp)
{
    warn("Discarding packet %d while in state %02x",
	  connp->r.ptr[0], connp->state);
    connp->r.ptr = connp->r.buf + connp->r.len;

    return 0;
}

static int Receive_undefined(connection_t *connp)
{
    warn("Unknown packet type (%d,%02x)", connp->r.ptr[0], connp->state);
    Destroy_connection(connp, "undefined packet");
    return -1;
}

static int Receive_ack_cannon(connection_t *connp)
{
    long loops_ack;
    unsigned char ch;
    int n;
    unsigned short num;
    cannon_t *cannon;

    if ((n = Packet_scanf(&connp->r, "%c%ld%hu",
			  &ch, &loops_ack, &num)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (num >= Num_cannons()) {
	Destroy_connection(connp, "bad cannon ack");
	return -1;
    }
    cannon = Cannon_by_index(num);
    if (loops_ack > cannon->last_change)
	SET_BIT(cannon->conn_mask, 1 << connp->ind);

    return 1;
}

static int Receive_ack_fuel(connection_t *connp)
{
    long loops_ack;
    unsigned char ch;
    int n;
    unsigned short num;
    fuel_t *fs;

    if ((n = Packet_scanf(&connp->r, "%c%ld%hu",
			  &ch, &loops_ack, &num)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (num >= Num_fuels()) {
	Destroy_connection(connp, "bad fuel ack");
	return -1;
    }
    fs = Fuel_by_index(num);
    if (loops_ack > fs->last_change)
	SET_BIT(fs->conn_mask, 1 << connp->ind);
    return 1;
}

static int Receive_ack_target(connection_t *connp)
{
    long loops_ack;
    unsigned char ch;
    int n;
    unsigned short num;
    target_t *targ;

    if ((n = Packet_scanf(&connp->r, "%c%ld%hu",
			  &ch, &loops_ack, &num)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (num >= Num_targets()) {
	Destroy_connection(connp, "bad target ack");
	return -1;
    }
    /*
     * Because the "loops" value as received by the client as part
     * of a frame update is 1 higher than the actual change to the
     * target in collision.c a valid map object change
     * acknowledgement must be at least 1 higher.
     * That's why we should use the '>' symbol to compare
     * and not the '>=' symbol.
     * The same applies to cannon and fuelstation updates.
     * This fix was discovered for 3.2.7, previously some
     * destroyed targets could have been displayed with
     * a diagonal cross through them.
     */
    targ = Target_by_index(num);
    if (loops_ack > targ->last_change) {
	SET_BIT(targ->conn_mask, 1 << connp->ind);
	CLR_BIT(targ->update_mask, 1 << connp->ind);
    }
    return 1;
}

static int Receive_ack_polystyle(connection_t *connp)
{
    long loops_ack;
    unsigned char ch;
    int n;
    unsigned short num;
    poly_t *poly;

    if ((n = Packet_scanf(&connp->r, "%c%ld%hu",
			  &ch, &loops_ack, &num)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (num >= num_polys) {
	Destroy_connection(connp, "bad polystyle ack");
	return -1;
    }
    poly = &pdata[num];
    if (loops_ack > poly->last_change)
	CLR_BIT(poly->update_mask, 1 << connp->ind);
    return 1;
}

/*
 * If a message contains a colon then everything before that colon is
 * either a unique player name prefix, or a team number with players.
 * If the string does not match one team or one player the message is not sent.
 * If no colon, the message is general.
 */
static void Handle_talk(connection_t *connp, char *str)
{
    player_t *pl = Player_by_id(connp->id);
    int i, sent, team;
    unsigned int len;
    char *cp, msg[MSG_LEN * 2];
    const char *sender = " [*Server reply*]";

    pl->flooding += FPS/3;

    if ((cp = strchr (str, ':')) == NULL
	|| cp == str
	|| strchr("-_~)(/\\}{[]", cp[1])	/* smileys are smileys */
	) {
	sprintf(msg, "%s [%s]", str, pl->name);
	if (!(mute_baseless && pl->home_base == NULL) && !pl->muted)
	    Set_message(msg);
	else {
	    for (sent = i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (pl_i->home_base == NULL)
		    Set_player_message (pl_i, msg);
	    }
	}
	return;
    }
    *cp++ = '\0';
    if (*cp == ' ')
	cp++;
    len = strlen (str);
    sprintf(msg, "%s [%s]", cp, pl->name);

    if (strspn(str, "0123456789") == len) {		/* Team message */
	team = atoi (str);
	sprintf(msg + strlen(msg), ":[%d]", team);
	sent = 0;
	if (!(mute_baseless && pl->home_base == NULL)) {
	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (pl_i->team == team) {
		    sent++;
		    Set_player_message(pl_i, msg);
		}
	    }
	}
	if (sent) {
	    if (pl->team != team)
		Set_player_message (pl, msg);
	} else {
	    if (!(mute_baseless && pl->home_base == NULL))
		sprintf(msg, "Message not sent, nobody in team %d!", team);
	    else
		sprintf(msg, "You may not send messages to active teams!");
	    strlcat(msg, sender, sizeof(msg));
	    Set_player_message(pl, msg);
	}
    }
    else if (strcasecmp(str, "god") == 0)
	Server_log_admin_message(pl, cp);
    else {						/* Player message */
	const char *errmsg;
	player_t *other_pl = Get_player_by_name(str, NULL, &errmsg);

	if (!other_pl) {
	    sprintf(msg, "Message not sent. ");
	    strlcat(msg, errmsg, sizeof(msg));
	    strlcat(msg, sender, sizeof(msg));
	    Set_player_message(pl, msg);
	    return;
	}

	if (other_pl != pl) {
	    if (!(mute_baseless && pl->home_base == NULL &&
		  other_pl->home_base != NULL)) {
		sprintf(msg + strlen(msg), ":[%s]", other_pl->name);
		Set_player_message(other_pl, msg);
	    } else {
		sprintf(msg, "You may not send messages to active players!");
		strlcat(msg, sender, sizeof(msg));
	    }
	    Set_player_message(pl, msg);
	}
    }
}

static int Receive_talk(connection_t *connp)
{
    unsigned char ch;
    int n;
    long seq;
    char str[MAX_CHARS];

    if ((n = Packet_scanf(&connp->r, "%c%ld%s", &ch, &seq, str)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (seq > connp->talk_sequence_num) {
	if ((n = Packet_printf(&connp->c, "%c%ld", PKT_TALK_ACK, seq)) <= 0) {
	    if (n == -1)
		Destroy_connection(connp, "write error");
	    return n;
	}
	connp->talk_sequence_num = seq;
	if (*str == '/')
	    Handle_player_command(Player_by_id(connp->id), str + 1);
	else
	    Handle_talk(connp, str);
    }
    return 1;
}

static int Receive_display(connection_t *connp)
{
    unsigned char ch, debris_colors, spark_rand;
    short width, height;
    int n;

    if ((n = Packet_scanf(&connp->r, "%c%hd%hd%c%c", &ch, &width, &height,
			  &debris_colors, &spark_rand)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    LIMIT(width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    if (record && recOpt && connp->view_width == width
	&& connp->view_height == height
	&& connp->debris_colors == debris_colors &&
	connp->spark_rand == spark_rand)
	/* This probably isn't that useful any more, but when this code
	 * was part of a server compatible with old clients, version
	 * 4.1.0 had a bug that could cause clients to send unnecessary
	 * packets like this every frame. Left here as an example of how
	 * recSpecial can be used. */
	recSpecial = 1;

    connp->view_width = width;
    connp->view_height = height;
    connp->debris_colors = debris_colors;
    connp->spark_rand = spark_rand;
    return 1;
}

static int Receive_modifier_bank(connection_t *connp)
{
    player_t *pl;
    unsigned char bank, ch;
    char str[MAX_CHARS];
    int n;

    if ((n = Packet_scanf(&connp->r, "%c%c%s", &ch, &bank, str)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read modbank");
	return n;
    }
    pl = Player_by_id(connp->id);
    Player_set_modbank(pl, bank, str);

    return 1;
}

void Get_display_parameters(connection_t *connp, int *width, int *height,
			    int *debris_colors, int *spark_rand)
{
    *width = connp->view_width;
    *height = connp->view_height;
    *debris_colors = connp->debris_colors;
    *spark_rand = connp->spark_rand;
}

int Get_player_id(connection_t *connp)
{
    return connp->id;
}

const char *Player_get_addr(player_t *pl)
{
    if (pl->conn != NULL)
	return pl->conn->addr;
    return NULL;
}

const char *Player_get_dpy(player_t *pl)
{
    if (pl->conn != NULL)
	return pl->conn->dpy;
    return NULL;
}

static int Receive_shape(connection_t *connp)
{
    int n;
    char ch, str[2*MSG_LEN];

    if ((n = Packet_scanf(&connp->r, "%c%S", &ch, str)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read shape");
	return n;
    }
    if ((n = Packet_scanf(&connp->r, "%S", &str[strlen(str)])) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read shape ext");
	return n;
    }
    if (connp->state == CONN_LOGIN && connp->ship == NULL)
	connp->ship = Parse_shape_str(str);
    return 1;
}

static int Receive_motd(connection_t *connp)
{
    unsigned char ch;
    long offset, nbytes;
    int n;

    if ((n = Packet_scanf(&connp->r,
			  "%c%ld%ld",
			  &ch, &offset, &nbytes)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    connp->motd_offset = offset;
    connp->motd_stop = offset + nbytes;

    return 1;
}

/*
 * Return part of the MOTD into buf starting from offset
 * and continueing at most for maxlen bytes.
 * Return the total MOTD size in size_ptr.
 * The return value is the actual amount of MOTD bytes copied
 * or -1 on error.  A value of 0 means EndOfMOTD.
 *
 * The MOTD is completely read into a dynamic buffer.
 * If this MOTD buffer hasn't been accessed for a while
 * then on the next access the MOTD file is checked for changes.
 */
#ifdef _WINDOWS
#define	close(__a)	_close(__a)
#endif
static int Get_motd(char *buf, int offset, int maxlen, int *size_ptr)
{
    static size_t motd_size;
    static char *motd_buf;
    static long motd_loops;
    static time_t motd_mtime;

    if (size_ptr)
	*size_ptr = 0;

    if (offset < 0 || maxlen < 0)
	return -1;

    if (!motd_loops
	|| (motd_loops + MAX_MOTD_LOOPS < main_loops
	    && offset == 0)) {

	int fd;
	size_t size;
	struct stat st;

	motd_loops = main_loops;

	if ((fd = open(options.motdFileName, O_RDONLY)) == -1) {
	    motd_size = 0;
	    return -1;
	}
	if (fstat(fd, &st) == -1 || st.st_size == 0) {
	    motd_size = 0;
	    close(fd);
	    return -1;
	}
	size = st.st_size;
	if (size > MAX_MOTD_SIZE)
	    size = MAX_MOTD_SIZE;

	if (size != motd_size) {
	    motd_mtime = 0;
	    motd_size = size;
	    if (motd_size == 0) {
		close(fd);
		return 0;
	    }
	    XFREE(motd_buf);
	    if ((motd_buf = XMALLOC(char, size)) == NULL) {
		close(fd);
		return -1;
	    }
	}
	if (motd_mtime != st.st_mtime) {
	    motd_mtime = st.st_mtime;
	    if ((size = read(fd, motd_buf, motd_size)) <= 0) {
		XFREE(motd_buf);
		close(fd);
		motd_size = 0;
		return -1;
	    }
	    motd_size = size;
	}
	close(fd);
    }

    motd_loops = main_loops;

    if (size_ptr)
	*size_ptr = motd_size;

    if (offset + maxlen > (int)motd_size)
	maxlen = motd_size - offset;

    if (maxlen <= 0)
	return 0;

    memcpy(buf, motd_buf + offset, (size_t)maxlen);
    return maxlen;
}

/*
 * Send the server MOTD to the client.
 * The last time we send a motd packet it should
 * have datalength zero to mean EOMOTD.
 */
static int Send_motd(connection_t *connp)
{
    int len, off = connp->motd_offset, size = 0;
    char buf[MAX_MOTD_CHUNK];

    len = MIN(MAX_MOTD_CHUNK, MAX_RELIABLE_DATA_PACKET_SIZE
	      - connp->c.len - 10);
    if (len >= 10) {
	len = Get_motd(buf, off, len, &size);
	if (len <= 0) {
	    len = 0;
	    connp->motd_offset = -1;
	}
	if (Packet_printf(&connp->c,
			  "%c%ld%hd%ld",
			  PKT_MOTD, off, len, size) <= 0) {
	    Destroy_connection(connp, "motd header");
	    return -1;
	}
	if (len > 0) {
	    connp->motd_offset += len;
	    if (Sockbuf_write(&connp->c, buf, len) != len) {
		Destroy_connection(connp, "motd data");
		return -1;
	    }
	}
    }

    /* Return ok */
    return 1;
}

static int Receive_pointer_move(connection_t *connp)
{
    player_t *pl;
    unsigned char ch;
    short movement;
    int n;
    double turnspeed, turndir;

    if ((n = Packet_scanf(&connp->r, "%c%hd", &ch, &movement)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    pl = Player_by_id(connp->id);

    /* kps - ??? */
    if (Player_is_hoverpaused(pl))
	return 1;

    if (FEATURE(connp, F_CUMULATIVETURN)) {
	int16_t delta;

	delta = movement - connp->last_mouse_pos;
	connp->last_mouse_pos = movement;
	movement = delta;
    }

    if (Player_uses_autopilot(pl))
	Autopilot(pl, false);
    turnspeed = movement * pl->turnspeed / MAX_PLAYER_TURNSPEED;
    if (turnspeed < 0) {
	turndir = -1.0;
	turnspeed = -turnspeed;
    }
    else
	turndir = 1.0;

    if (pl->turnresistance)
	LIMIT(turnspeed, MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED);
      /* Minimum amount of turning if you want to turn at all?
       * And the only effect of that maximum is making
       * finding the correct settings harder for new mouse players,
       * because the limit is checked BEFORE multiplying by turnres!
       * Kept here to avoid changing the feeling for old players who
       * are already used to this odd behavior. New players should set
       * turnresistance to 0.
       */
    else
	LIMIT(turnspeed, 0, 5*RES);

    pl->turnvel -= turndir * turnspeed;
    pl->idleTime = 0;

    recSpecial = 1;

    return 1;
}

static int Receive_fps_request(connection_t *connp)
{
    player_t *pl;
    int n;
    unsigned char ch, fps;

    if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &fps)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (connp->id != NO_ID) {
	pl = Player_by_id(connp->id);
	/*
	 * kps - 0 could be made to mean "no limit" ?
	 * Now both 0 and 1 mean 1.
	 */
	if (fps == 0)
	    fps = 1;
	if (!FEATURE(connp, F_POLY) && (fps == 20) && options.ignore20MaxFPS)
	    fps = MAX_SERVER_FPS;
 	pl->player_fps = fps;
    }

    return 1;
}

static int Receive_audio_request(connection_t *connp)
{
    player_t *pl;
    int n;
    unsigned char ch, on;

    if ((n = Packet_scanf(&connp->r, "%c%c", &ch, &on)) <= 0) {
	if (n == -1)
	    Destroy_connection(connp, "read error");
	return n;
    }
    if (connp->id != NO_ID) {
	pl = Player_by_id(connp->id);
	sound_player_on(pl, on);
    }

    return 1;
}

int Check_max_clients_per_IP(char *host_addr)
{
    int i, clients_per_ip = 0;
    connection_t *connp;

    if (options.maxClientsPerIP <= 0)
	return 0;

    for (i = 0; i < max_connections; i++) {
	connp = &Conn[i];
	if (connp->state != CONN_FREE && !strcasecmp(connp->addr, host_addr))
	    clients_per_ip++;
    }

    if (clients_per_ip >= options.maxClientsPerIP)
	return 1;

    return 0;
}
