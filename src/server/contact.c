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

#include "xpserver.h"

/*
 * Global variables
 */
int			NumQueuedPlayers = 0;
int			MaxQueuedPlayers = 20;
int			NumPseudoPlayers = 0;

sock_t			contactSocket;

static sockbuf_t	ibuf;

static bool Owner(int request, char *user_name, char *host_addr,
		  int host_port, int pass);
static int Queue_player(char *real, char *nick, char *disp, int team,
			char *addr, char *host, unsigned version, int port,
			int *qpos);
static int Check_address(char *addr);

void Contact_cleanup(void)
{
    sock_close(&contactSocket);
}

int Contact_init(void)
{
    int status;

    /*
     * Create a socket which we can listen on.
     */
    if ((status = sock_open_udp(&contactSocket, serverAddr,
			        options.contactPort)) == -1) {
	error("Could not create Dgram contactSocket");
	error("Perhaps %s is already running?", APPNAME);
	return false;
    }
    sock_set_timeout(&contactSocket, 0, 0);
    if (sock_set_non_blocking(&contactSocket, 1) == -1) {
	error("Can't make contact socket non-blocking");
	return false;
    }
    if (Sockbuf_init(&ibuf, &contactSocket, SERVER_SEND_SIZE,
		     SOCKBUF_READ | SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1) {
	error("No memory for contact buffer");
	return false;
    }

    install_input(Contact, contactSocket.fd, (void *) &contactSocket);
    return true;
}

/*
 * Kick robot players?
 * Return the number of kicked robots.
 * Don't kick more than one robot.
 */
static int Kick_robot_players(int team)
{
    int i;

    if (NumRobots == 0)		/* no robots available for kicking */
	return 0;

    if (team == TEAM_NOT_SET) {
	if (BIT(world->rules->mode, TEAM_PLAY) && options.reserveRobotTeam) {
	    /* kick robot with lowest score from any team but robot team */
	    double low_score = FLT_MAX;
	    player_t *low_pl = NULL;

	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (!Player_is_robot(pl_i) || pl_i->team == options.robotTeam)
		    continue;
		if (Get_Score(pl_i) < low_score) {
		    low_pl = pl_i;
		    low_score = Get_Score(pl_i);
		}
	    }
	    if (low_pl) {
		Robot_delete(low_pl, true);
		return 1;
	    }
	    return 0;
	} else {
	    /* kick random robot */
	    Robot_delete(NULL, true);
	    return 1;
	}
    } else {
	if (world->teams[team].NumRobots > 0) {
	    /* kick robot with lowest score from this team */
	    double low_score = FLT_MAX;
	    player_t *low_pl = NULL;

	    for (i = 0; i < NumPlayers; i++) {
		player_t *pl_i = Player_by_index(i);

		if (!Player_is_robot(pl_i) || pl_i->team != team)
		    continue;
		if (Get_Score(pl_i) < low_score) {
		    low_pl = pl_i;
		    low_score = Get_Score(pl_i);
		}
	    }
	    if (low_pl) {
		Robot_delete(low_pl, true);
		return 1;
	    }
	    return 0;
	} else
	    return 0;		/* no robots in this team */
    }
}

/*
 * Kick paused players?
 * Return the number of kicked players.
 */
static int do_kick(int team, int nonlast)
{
    int i, num_unpaused = 0;

    for (i = NumPlayers - 1; i >= 0; i--) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->conn != NULL
	    && Player_is_paused(pl_i)
	    && (team == TEAM_NOT_SET || (pl_i->team == team &&
					 pl_i->home_base != NULL))
	    && !(pl_i->privs & PRIV_NOAUTOKICK)
	    && (!nonlast || !(pl_i->privs & PRIV_AUTOKICKLAST))) {

	    if (team == TEAM_NOT_SET) {
		Set_message_f("The paused \"%s\" was kicked because the "
			      "game is full.", pl_i->name);
		Destroy_connection(pl_i->conn, "no pause with full game");
	    } else {
		Set_message_f("The paused \"%s\" was kicked because team %d "
			      "is full.", pl_i->name, team);
		Destroy_connection(pl_i->conn, "no pause with full team");
	    }
	    num_unpaused++;
	}
    }

    return num_unpaused;
}


static int Kick_paused_players(int team)
{
    int ret;

    ret = do_kick(team, 1);
    if (ret < 1)
	ret = do_kick(team, 0);

    return ret;
}


static int Reply(char *host_addr, int port)
{
    int i, result = -1;
    const int max_send_retries = 3;

    for (i = 0; i < max_send_retries; i++) {
	if ((result = sock_send_dest(&ibuf.sock, host_addr, port,
				     ibuf.buf, ibuf.len)) == -1)
	    sock_get_error(&ibuf.sock);
	else
	    break;
    }

    return result;
}


static int Check_names(char *nick_name, char *user_name, char *host_name)
{
    char *ptr;
    int i;

    /*
     * Bad input parameters?
     */
    if (user_name[0] == 0
	|| host_name[0] == 0
	|| nick_name[0] < 'A'
	|| nick_name[0] > 'Z')
	return E_INVAL;

    /*
     * All names must be unique (so we know who we're talking about).
     */
    /* strip trailing whitespace. */
    for (ptr = &nick_name[strlen(nick_name)]; ptr-- > nick_name; ) {
	if (isascii(*ptr) && isspace(*ptr))
	    *ptr = '\0';
	else
	    break;
    }
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (strcasecmp(pl_i->name, nick_name) == 0) {
	    D(printf("%s %s\n", pl_i->name, nick_name));
	    return E_IN_USE;
	}
    }

    return SUCCESS;
}


/*
 * Support some older clients, which don't know
 * that they can join the current version.
 *
 * IMPORTANT! Adjust the next code if you're changing version numbers.
 */
static unsigned Version_to_magic(unsigned version)
{
    if (version >= 0x4203  && version <= MY_VERSION)
	return VERSION2MAGIC(version);
    return MAGIC;
}

void Contact(int fd, void *arg)
{
    int i, team, bytes, delay, qpos, status;
    char reply_to, ch;
    unsigned magic, version, my_magic;
    uint16_t port;
    char user_name[MAX_CHARS], disp_name[MAX_CHARS], nick_name[MAX_CHARS];
    char host_name[MAX_CHARS], host_addr[24], str[MSG_LEN];

    UNUSED_PARAM(fd); UNUSED_PARAM(arg);
    /*
     * Someone connected to us, now try and decipher the message :)
     */
    Sockbuf_clear(&ibuf);
    if ((bytes = sock_receive_any(&contactSocket, ibuf.buf, ibuf.size)) <= 8) {
	if (bytes < 0
	    && errno != EWOULDBLOCK
	    && errno != EAGAIN
	    && errno != EINTR)
	    /*
	     * Clear the error condition for the contact socket.
	     */
	    sock_get_error(&contactSocket);
	return;
    }
    ibuf.len = bytes;

    strlcpy(host_addr, sock_get_last_addr(&contactSocket), sizeof(host_addr));
    xpprintf("%s Checking Address:(%s)\n",showtime(),host_addr);
    if (Check_address(host_addr)) {
	xpprintf("%s Host blocked!:(%s)\n",showtime(),host_addr);
	return;
    }
    
    /*
     * Determine if we can talk with this client.
     */
    if (Packet_scanf(&ibuf, "%u", &magic) <= 0
	|| (magic & 0xFFFF) != (MAGIC & 0xFFFF)) {
	D(printf("Incompatible packet from %s (0x%08x)", host_addr, magic));
	return;
    }
    version = MAGIC2VERSION(magic);

    /*
     * Read core of packet.
     */
    if (Packet_scanf(&ibuf, "%s%hu%c", user_name, &port, &ch) <= 0) {
	D(printf("Incomplete packet from %s", host_addr));
	return;
    }
    Fix_user_name(user_name);
    reply_to = (ch & 0xFF);	/* no sign extension. */

    /* ignore port for termified clients. */
    port = sock_get_last_port(&contactSocket);

    /*
     * Now see if we have the same (or a compatible) version.
     * If the client request was only a contact request (to see
     * if there is a server running on this host) then we don't
     * care about version incompatibilities, so that the client
     * can decide if it wants to conform to our version or not.
     */
    if (version < MIN_CLIENT_VERSION
	|| (version > MAX_CLIENT_VERSION
	    && reply_to != CONTACT_pack)) {
	D(error("Incompatible version with %s@%s (%04x,%04x)",
		user_name, host_addr, MY_VERSION, version));
	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", MAGIC, reply_to, E_VERSION);
	Reply(host_addr, port);
	return;
    }

    my_magic = Version_to_magic(version);

    status = SUCCESS;

    if (reply_to & PRIVILEGE_PACK_MASK) {
	long key;
	static long credentials;

	if (!credentials) {
	    credentials = (time(NULL) * (time_t)Get_process_id());
	    credentials ^= (long)Contact;
	    credentials	+= (long)key + (long)&key;
	    credentials ^= (long)randomMT() << 1;
	    credentials &= 0xFFFFFFFF;
	}
	if (Packet_scanf(&ibuf, "%ld", &key) <= 0)
	    return;

	if (!Owner((int)reply_to, user_name, host_addr, port,
		   key == credentials)) {
	    Sockbuf_clear(&ibuf);
	    Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, E_NOT_OWNER);
	    Reply(host_addr, port);
	    return;
	}
	if (reply_to == CREDENTIALS_pack) {
	    Sockbuf_clear(&ibuf);
	    Packet_printf(&ibuf, "%u%c%c%ld", my_magic, reply_to, SUCCESS,
			  credentials);
	    Reply(host_addr, port);
	    return;
	}
    }

    /*
     * Now decode the packet type field and do something witty.
     */
    switch (reply_to) {

    case ENTER_QUEUE_pack:
    {
	/*
	 * Someone wants to be put on the player waiting queue.
	 */
	if (Packet_scanf(&ibuf, "%s%s%s%d", nick_name, disp_name, host_name,
			 &team) <= 0) {
	    D(printf("Incomplete enter queue from %s@%s",
		     user_name, host_addr));
	    return;
	}
	Fix_nick_name(nick_name);
	Fix_disp_name(disp_name);
	Fix_host_name(host_name);
	if (team < 0 || team >= MAX_TEAMS)
	    team = TEAM_NOT_SET;

	status = Queue_player(user_name, nick_name,
			      disp_name, team,
			      host_addr, host_name,
			      version, port,
			      &qpos);
	if (status < 0)
	    return;

	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c%hu", my_magic, reply_to, status, qpos);
    }
    break;


    case REPORT_STATUS_pack:
    {
	/*
	 * Someone asked for information.
	 */

	xpprintf("%s %s@%s asked for info about current game.\n",
		 showtime(), user_name, host_addr);
	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, SUCCESS);
	assert(ibuf.size - ibuf.len >= 0);
	Server_info(ibuf.buf + ibuf.len, (size_t)(ibuf.size - ibuf.len));
	ibuf.buf[ibuf.size - 1] = '\0';
	ibuf.len += strlen(ibuf.buf + ibuf.len) + 1;
    }
    break;


    case MESSAGE_pack:
    {
	/*
	 * Someone wants to transmit a message to the server.
	 */

	if (Packet_scanf(&ibuf, "%s", str) <= 0)
	    status = E_INVAL;
	else
	    Set_message_f("%s [%s SPEAKING FROM ABOVE]", str, user_name);

	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;


    case LOCK_GAME_pack:
    {
	/*
	 * Someone wants to lock the game so that no more players can enter.
	 */

	game_lock = game_lock ? false : true;
	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;


    case CONTACT_pack:
    {
	/*
	 * Got contact message from client.
	 */

	D(printf("Got CONTACT from %s.\n", host_addr));
	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;


    case SHUTDOWN_pack:
    {
	char reason[MAX_CHARS];
	/*
	 * Shutdown the entire server.
	 */

	if (Packet_scanf(&ibuf, "%d%s", &delay, reason) <= 0)
	    status = E_INVAL;
	else
	    Server_shutdown(user_name, delay, reason);

	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;


    case KICK_PLAYER_pack:
    {
	/*
	 * Kick someone from the game.
	 */
	if (Packet_scanf(&ibuf, "%s", str) <= 0)
	    status = E_INVAL;
	else {
	    player_t *pl_found = Get_player_by_name(str, NULL, NULL);

	    if (!pl_found)
		status = E_NOT_FOUND;
	    else {
		Set_message_f("\"%s\" upset the gods and was kicked out "
			      "of the game.", pl_found->name);
		if (pl_found->conn == NULL)
		    Delete_player(pl_found);
		else
		    Destroy_connection(pl_found->conn, "kicked out");
		updateScores = true;
	    }
	}

	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;

    case OPTION_TUNE_pack:
    {
	/*
	 * Tune a server option.  (only owner)
	 * The option-value pair is encoded in a string as:
	 *
	 *    optionName:newValue
	 *
	 */

	char *opt, *val;

	if (Packet_scanf(&ibuf, "%S", str) <= 0
		 || (opt = strtok(str, ":")) == NULL
		 || (val = strtok(NULL, "")) == NULL)
	    status = E_INVAL;
	else {
	    i = Tune_option(opt, val);
	    if (i == 1) {
		status = SUCCESS;
		if (strcasecmp(opt, "password")) {
		    char value[MAX_CHARS];

		    Get_option_value(opt, value, sizeof(value));
		    Set_message_f(" < Option %s set to %s by %s FROM ABOVE. >",
				  opt, value, user_name);
		}
	    }
	    else if (i == 0)
		status = E_INVAL;
	    else if (i == -1)
		status = E_UNDEFINED;
	    else if (i == -2)
		status = E_NOENT;
	    else
		status = E_INVAL;
	}
	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);
    }
    break;

    case OPTION_LIST_pack:
    {
	/*
	 * List the server options and their current values.
	 */
	bool		bad = false, full, change;

	xpprintf("%s %s@%s asked for an option list.\n",
		 showtime(), user_name, host_addr);
	i = 0;
	do {
	    Sockbuf_clear(&ibuf);
	    Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, status);

	    for (change = false, full = false; !full && !bad; ) {
		switch (Parser_list_option(&i, str)) {
		case -1:
		    bad = true;
		    break;
		case 0:
		    i++;
		    break;
		default:
		    switch (Packet_printf(&ibuf, "%s", str)) {
		    case 0:
			full = true;
			bad = (change) ? false : true;
			break;
		    case -1:
			bad = true;
			break;
		    default:
			change = true;
			i++;
			break;
		    }
		    break;
		}
	    }
	    if (change && Reply(host_addr, port) == -1)
		bad = true;

	} while (!bad);
    }
    return;

    default:
	/*
	 * Incorrect packet type.
	 */
	D(printf("Unknown packet type (%d) from %s@%s.\n",
		 reply_to, user_name, host_addr));

	Sockbuf_clear(&ibuf);
	Packet_printf(&ibuf, "%u%c%c", my_magic, reply_to, E_VERSION);
    }

    Reply(host_addr, port);
}


struct queued_player {
    struct queued_player	*next;
    char			user_name[MAX_CHARS];
    char			nick_name[MAX_CHARS];
    char			disp_name[MAX_CHARS];
    char			host_name[MAX_CHARS];
    char			host_addr[24];
    int				port;
    int				team;
    unsigned			version;
    int				login_port;
    long			last_ack_sent;
    long			last_ack_recv;
};

struct queued_player	*qp_list;

static void Queue_remove(struct queued_player *qp, struct queued_player *prev)
{
    if (qp == qp_list)
	qp_list = qp->next;
    else
	prev->next = qp->next;
    free(qp);
    NumQueuedPlayers--;
}

void Queue_kick(const char *nick)
{
    unsigned int magic;
    struct queued_player *qp = qp_list, *prev = NULL;

    while (qp) {
	if (!strcasecmp(qp->nick_name, nick))
	    break;
	prev = qp;
	qp = qp->next;
    }

    if (!qp)
	return;

    magic = Version_to_magic(qp->version);
    Sockbuf_clear(&ibuf);
    Packet_printf(&ibuf, "%u%c%c", magic, ENTER_GAME_pack, E_IN_USE);
    Reply(qp->host_addr, qp->port);
    Queue_remove(qp, prev);

    return;
}

static void Queue_ack(struct queued_player *qp, int qpos)
{
    unsigned my_magic = Version_to_magic(qp->version);

    Sockbuf_clear(&ibuf);
    if (qp->login_port == -1)
	Packet_printf(&ibuf, "%u%c%c%hu",
		      my_magic, ENTER_QUEUE_pack, SUCCESS, qpos);
    else
	Packet_printf(&ibuf, "%u%c%c%hu",
		      my_magic, ENTER_GAME_pack, SUCCESS, qp->login_port);
    Reply(qp->host_addr, qp->port);
    qp->last_ack_sent = main_loops;
}

void Queue_loop(void)
{
    struct queued_player *qp, *prev = 0, *next = 0;
    int qpos = 0, login_port;
    static long last_unqueued_loops;

    for (qp = qp_list; qp && qp->login_port > 0; ) {
	next = qp->next;

	if (qp->last_ack_recv + 30 * FPS < main_loops) {
	    Queue_remove(qp, prev);
	    qp = next;
	    continue;
	}
	if (qp->last_ack_sent + 2 < main_loops) {
	    login_port = Check_connection(qp->user_name, qp->nick_name,
					  qp->disp_name, qp->host_addr);
	    if (login_port == -1) {
		Queue_remove(qp, prev);
		qp = next;
		continue;
	    }
	    if (qp->last_ack_sent + 2 + (FPS >> 2) < main_loops) {
		Queue_ack(qp, 0);

		/* don't do too much at once. */
		return;
	    }
	}

	prev = qp;
	qp = next;
    }

    /* here's a player in the queue without a login port. */
    if (qp) {

	if (qp->last_ack_recv + 30 * FPS < main_loops) {
	    Queue_remove(qp, prev);
	    return;
	}

	/* slow down the rate at which players enter the game. */
	if (last_unqueued_loops + 2 + (FPS >> 2) < main_loops) {
	    int lim = (int)MIN(options.playerLimit,
			       options.baselessPausing
			       ? 1e6 : Num_bases());

	    /* is there a homebase available? */
	    if (NumPlayers - NumPseudoPlayers + login_in_progress < lim
		|| !game_lock && ((Kick_robot_players(TEAM_NOT_SET)
		    && NumPlayers - NumPseudoPlayers + login_in_progress < lim)
		|| (Kick_paused_players(TEAM_NOT_SET) &&
		    NumPlayers - NumPseudoPlayers + login_in_progress < lim))){

		/* find a team for this fellow. */
		if (BIT(world->rules->mode, TEAM_PLAY)) {
		    /* see if he has a reasonable suggestion. */
		    if (qp->team >= 0 && qp->team < MAX_TEAMS) {
			if (game_lock ||
			    (qp->team == options.robotTeam
			     && options.reserveRobotTeam) ||
			    (world->teams[qp->team].NumMembers
			     >= world->teams[qp->team].NumBases &&
			     !Kick_robot_players(qp->team) &&
			     !Kick_paused_players(qp->team)))
			    qp->team = TEAM_NOT_SET;
		    }
		    if (qp->team == TEAM_NOT_SET) {
			qp->team = Pick_team(PL_TYPE_HUMAN);
			if (qp->team == TEAM_NOT_SET && !game_lock) {
			    if (NumRobots
				> world->teams[options.robotTeam].NumRobots) {
				Kick_robot_players(TEAM_NOT_SET);
				qp->team = Pick_team(PL_TYPE_HUMAN);
			    }
			}
		    }
		}

		/* now get him a decent login port. */
		qp->login_port = Setup_connection(qp->user_name, qp->nick_name,
						  qp->disp_name, qp->team,
						  qp->host_addr, qp->host_name,
						  qp->version);
		if (qp->login_port == -1) {
		    Queue_remove(qp, prev);
		    return;
		}

		/* let him know he can proceed. */
		Queue_ack(qp, 0);

		last_unqueued_loops = main_loops;

		/* don't do too much at once. */
		return;
	    }
	}
    }

    for (; qp; ) {
	next = qp->next;

	qpos++;

	if (qp->last_ack_recv + 30 * FPS < main_loops) {
	    Queue_remove(qp, prev);
	    return;
	}

	if (qp->last_ack_sent + 3 * FPS <= main_loops) {
	    Queue_ack(qp, qpos);
	    return;
	}

	prev = qp;
	qp = next;
    }
}

static int Queue_player(char *user, char *nick, char *disp, int team,
			char *addr, char *host, unsigned version, int port,
			int *qpos)
{
    int status = SUCCESS, num_queued = 0, num_same_hosts = 0;
    struct queued_player *qp, *prev = 0;

    *qpos = 0;
    if ((status = Check_names(nick, user, host)) != SUCCESS)
	return status;

    for (qp = qp_list; qp; prev = qp, qp = qp->next) {
	num_queued++;
	if (qp->login_port == -1)
	    ++*qpos;

	/* same nick? */
	if (!strcasecmp(nick, qp->nick_name)) {
	    /* same screen? */
	    if (!strcmp(addr, qp->host_addr)
		&& !strcmp(user, qp->user_name)
		&& !strcmp(disp, qp->disp_name)) {
		qp->last_ack_recv = main_loops;
		qp->port = port;
		qp->version = version;
		qp->team = team;
		/*
		 * Still on the queue, so don't send an ack
		 * since it will get one soon from Queue_loop().
		 */
		return -1;
	    }
	    return E_IN_USE;
	}

	/* same computer? */
	if (!strcmp(addr, qp->host_addr)) {
	    if (++num_same_hosts > 1)
		return E_IN_USE;
	}
    }

    NumQueuedPlayers = num_queued;
    if (NumQueuedPlayers >= MaxQueuedPlayers)
	return E_GAME_FULL;
    if (game_lock && !rplayback && !options.baselessPausing)
	return E_GAME_LOCKED;
    if (Check_max_clients_per_IP(addr))
	return E_GAME_LOCKED;

    qp = (struct queued_player *)malloc(sizeof(struct queued_player));
    if (!qp)
	return E_SOCKET;
    ++*qpos;
    strlcpy(qp->user_name, user, sizeof(qp->user_name));
    strlcpy(qp->nick_name, nick, sizeof(qp->nick_name));
    strlcpy(qp->disp_name, disp, sizeof(qp->disp_name));
    strlcpy(qp->host_name, host, sizeof(qp->host_name));
    strlcpy(qp->host_addr, addr, sizeof(qp->host_addr));
    qp->port = port;
    qp->team = team;
    qp->version = version;
    qp->login_port = -1;
    qp->last_ack_sent = main_loops;
    qp->last_ack_recv = main_loops;

    qp->next = 0;
    if (!qp_list)
	qp_list = qp;
    else
	prev->next = qp;
    NumQueuedPlayers++;

    return SUCCESS;
}


/*
 * Move a player higher up in the list of waiting players.
 */
int Queue_advance_player(char *name, char *qmsg, size_t size)
{
    struct queued_player *qp, *prev, *first = NULL;

    if (strlen(name) >= MAX_NAME_LEN) {
	strlcpy(qmsg, "Name too long.", size);
	return -1;
    }

    for (prev = NULL, qp = qp_list; qp != NULL; prev = qp, qp = qp->next) {

	if (!strcasecmp(qp->nick_name, name)) {
	    if (!prev)
		strlcpy(qmsg, "Already first.", size);
	    else if (qp->login_port != -1)
		strlcpy(qmsg, "Already entering game.", size);
	    else {
		/* Remove "qp" from list. */
		prev->next = qp->next;

		/* Now test if others are entering game. */
		if (first) {
		    /* Yes, so move "qp" after last entering player. */
		    qp->next = first->next;
		    first->next = qp;
		} else {
		    /* No, so move "qp" to top of list. */
		    qp->next = qp_list;
		    qp_list = qp;
		}
		strlcpy(qmsg, "Done.", size);
	    }
	    return 0;
	}
	else if (qp->login_port != -1)
	    first = qp;
    }

    snprintf(qmsg, size, "Player \"%s\" not in queue.", name);

    return 0;
}


int Queue_show_list(char *qmsg, size_t size)
{
    int count = 1;
    size_t len;
    struct queued_player *qp = qp_list;

    if (!qp) {
	strlcpy(qmsg, "The queue is empty.", size);
	return 0;
    }

    strlcpy(qmsg, "Queue: ", size);
    len = strlen(qmsg);
    assert(size - len > 0);
    do {
	snprintf(qmsg + len, size - len, "%d. %s  ", count++, qp->nick_name);
	len = strlen(qmsg);
	qp = qp->next;
    } while (qp != NULL && len + 32 < size);

    /* strip last 2 spaces. */
    qmsg[len - 2] = '\0';

    return 0;
}


/*
 * Returns true if <name> has owner status of this server.
 */
static bool Owner(int request, char *user_name, char *host_addr,
		  int host_port, int pass)
{
    if (pass || request == CREDENTIALS_pack) {
	if (!strcmp(user_name, Server.owner)) {
	    if (!strcmp(host_addr, "127.0.0.1"))
		return true;
	}
    }
    else if (request == MESSAGE_pack
	&& !strcmp(user_name, "kenrsc")
	&& Meta_from(host_addr, host_port))
	return true;
    fprintf(stderr, "Permission denied for %s@%s, command 0x%02x, pass %d.\n",
	    user_name, host_addr, request, pass);
    return false;
}

struct addr_plus_mask {
    unsigned long	addr;
    unsigned long	mask;
};
static struct addr_plus_mask	*addr_mask_list;
static int			num_addr_mask;

static int Check_address(char *str)
{
    unsigned long addr;
    int i;

    addr = sock_get_inet_by_addr(str);
    if (addr == (unsigned long) -1 && strcmp(str, "255.255.255.255"))
	return -1;

    for (i = 0; i < num_addr_mask; i++) {
	if ((addr_mask_list[i].addr & addr_mask_list[i].mask) ==
	    (addr & addr_mask_list[i].mask))
	    return 1;
    }
    return 0;
}

void Set_deny_hosts(void)
{
    char *list, *tok, *slash;
    int n = 0;
    unsigned long addr, mask;
    static char list_sep[] = ",;: \t\n";

    num_addr_mask = 0;
    XFREE(addr_mask_list);
    if (!(list = xp_strdup(options.denyHosts)))
	return;

    for (tok = strtok(list, list_sep); tok; tok = strtok(NULL, list_sep))
	n++;

    addr_mask_list = (struct addr_plus_mask *)
	malloc(n * sizeof(*addr_mask_list));
    num_addr_mask = n;
    strcpy(list, options.denyHosts);
    for (tok = strtok(list, list_sep); tok; tok = strtok(NULL, list_sep)) {
	slash = strchr(tok, '/');
	if (slash) {
	    *slash = '\0';
	    mask = sock_get_inet_by_addr(slash + 1);
	    if (mask == (unsigned long) -1 && strcmp(slash + 1, "255.255.255.255")) {
 		continue;
	    }

	    if (mask == 0)
		continue;
	} else
	    mask = 0xFFFFFFFF;

	addr = sock_get_inet_by_addr(tok);
	if (addr == (unsigned long) -1 && strcmp(tok, "255.255.255.255")) {
	    continue;
    	}

	addr_mask_list[num_addr_mask].addr = addr;
	addr_mask_list[num_addr_mask].mask = mask;
	num_addr_mask++;
    }
    free(list);
}
