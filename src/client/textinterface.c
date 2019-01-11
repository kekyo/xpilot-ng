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
 * Copyright (C) 2001 Uoti Urpala    <uau@users.sourceforge.net>
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

#include "xpclient.h"


#define MAX_LINE	MSG_LEN	/* should not be smaller than MSG_LEN */


extern int		dgram_one_socket;	/* from datagram.c */


/*
 * just like fgets() but strips newlines like gets().
 */
static char* get_line(char* buf, int len, FILE* stream)
{
    char		*nl;

    if (fgets(buf, len, stream)) {
	nl = strchr(buf, '\n');
	if (nl) {
	    *nl = '\0';		/* strip newline */
	    if (nl > buf && nl[-1] == '\r')
		nl[-1] = '\0';
	}
    }
    return buf;
}


/*
 * Replace control characters with a space.
 */
static void Clean_string(char *buf)
{
    char		*str;
    int			c;

    for (str = buf; *str; str++) {
	c = (*str & 0xFF);
	if (!isascii(c) || iscntrl(c)) {
	    if (!strchr("\r\n", c))
		*str = ' ';
	}
    }
}


static int Get_contact_message(sockbuf_t *sbuf,
			       const char *contact_server,
			       Connect_param_t *conpar)
{
    int			len;
    int			server_version;
    unsigned		magic;
    unsigned char	reply_to, status;
    int			readable = 0;

    sock_set_timeout(&sbuf->sock, 2, 0);
    while (readable == false && sock_readable(&sbuf->sock) > 0) {

	Sockbuf_clear(sbuf);
	len = sock_receive_any(&sbuf->sock, sbuf->buf, sbuf->size);
	if (len <= 0) {
	    if (len == 0)
		continue;
	    xpprintf("Error from sock_receive_any, contact message failed.\n");
	    /* exit(1);  no good since meta gui. */
	    return 0;
	}
	sbuf->len = len;

	/*
	 * Get server's host and port.
	 */
	strlcpy(conpar->server_addr, sock_get_last_addr(&sbuf->sock),
		sizeof(conpar->server_addr));
	conpar->server_port = sock_get_last_port(&sbuf->sock);
	/*
	 * If the name of the server to contact is the same as its
	 * IP address then we don't want to do a reverse lookup.
	 * Doing a reverse lookup may result in a long and annoying delay.
	 */
	if (!strcmp(conpar->server_addr, contact_server))
	    strlcpy(conpar->server_name, conpar->server_addr,
		    sizeof(conpar->server_name));
	else
	    strlcpy(conpar->server_name, sock_get_last_name(&sbuf->sock),
		    sizeof(conpar->server_name));

	if (Packet_scanf(sbuf, "%u%c%c", &magic, &reply_to, &status) <= 0)
	    warn("Incomplete contact reply message (%d)", len);
	else if ((magic & 0xFFFF) != (MAGIC & 0xFFFF))
	    warn("Bad magic on contact message (0x%x).", magic);
	else {
	    server_version = MAGIC2VERSION(magic);
	    if (!((server_version >= MIN_SERVER_VERSION &&
		   server_version <= MAX_SERVER_VERSION) ||
		  (server_version >= MIN_OLD_SERVER_VERSION &&
		   server_version <= MAX_OLD_SERVER_VERSION))) {
		warn("Incompatible version with server %s.",
		     conpar->server_name);
		warn("We run version %04x, while server is running %04x.",
		     MY_VERSION, server_version);
		if ((MY_VERSION >> 4) < (server_version >> 4))
		    warn("Time for us to upgrade?");
		readable = 2;
	    } else {
		/*
		 * Found one which we can talk to.
		 */
		xpinfo("Using protocol version 0x%04x.", server_version);
		conpar->server_version = server_version;
		readable = 1;
	    }
	}
    }

    return readable;
}



static int Get_reply_message(sockbuf_t *ibuf,
			     Connect_param_t *conpar)
{
    int			len;
    unsigned		magic;


    if (sock_readable(&ibuf->sock)) {
	Sockbuf_clear(ibuf);
	if ((len = sock_read(&ibuf->sock, ibuf->buf, ibuf->size)) == -1) {
	    error("Can't read reply message from %s/%d",
		  conpar->server_addr, conpar->server_port);
	    exit(1);
	}

	ibuf->len = len;
	if (Packet_scanf(ibuf, "%u", &magic) <= 0) {
	    warn("Incomplete reply packet (%d)", len);
	    return 0;
	}

	if ((magic & 0xFFFF) != (MAGIC & 0xFFFF)) {
	    warn("Wrong MAGIC in reply pack (0x%x).", magic);
	    return 0;
	}

	if (MAGIC2VERSION(magic) != conpar->server_version) {
	    printf("Incompatible version with server on %s.\n",
		    conpar->server_name);
	    printf("We run version %04x, while server is running %04x.\n",
		   MY_VERSION, MAGIC2VERSION(magic));
	    return 0;
	}

	return len;
    }

    return 0;
}



static void Command_help(void)
{
    printf("Supported commands are:\n"
	   "H/?  -   Help - this text.\n"
	   "N    -   Next server, skip this one.\n"
	   "S    -   list Status.\n"
	   "T    -   set Team.\n"
	   "Q    -   Quit.\n");
    printf("K    -   Kick a player.                (only owner)\n"
	   "M    -   send a Message.               (only owner)\n"
	   "L    -   Lock/unLock server access.    (only owner)\n"
	   "D(*) -   shutDown/cancel shutDown.     (only owner)\n"
	   "O    -   Modify a server option.       (only owner)\n"
	   "V    -   View the server options.\n"
	   "J(&) or just Return enters the game.\n");
    printf("(*) If you don't specify any delay, you will signal that\n"
	   "    the server should stop an ongoing shutdown.\n"
	   "(&) You may specify a team number after the J.\n");
}



/*
 * This is the routine that interactively (if not auto_connect) prompts
 * the user on his/her next action.  Returns true if player joined this
 * server (connected to server), or false if the player wants to have a
 * look at the next server.
 */
static bool Process_commands(sockbuf_t *ibuf,
			     int auto_connect, int list_servers,
			     int auto_shutdown, char *shutdown_reason,
			     Connect_param_t *conpar)
{
    int i, len, retries, delay, success, cmd_credentials = 0, max_replies;
    char c, status, reply_to, linebuf[MAX_LINE];
    unsigned short port, qpos;
    bool has_credentials = false, privileged_cmd;
    long key = 0;
    time_t qsent = 0;
    static char localhost[] = "127.0.0.1";

#ifdef _WINDOWS
    auto_connect = TRUE;	/* I want to join */
    auto_shutdown = FALSE;
#endif

    if (auto_connect && !list_servers && !auto_shutdown)
	xpprintf("*** Connected to %s\n", conpar->server_name);

    for (;;) {

	max_replies = 1;

	/*
	 * Now, what do you want from the server?
	 */
	if (cmd_credentials) {
	    c = cmd_credentials;
	    cmd_credentials = 0;
	}
	else if (!auto_connect) {
	    printf("*** Server on %s. Enter command> ", conpar->server_name);

	    get_line(linebuf, MAX_LINE, stdin);
	    if (feof(stdin)) {
		puts("");
		c = 'Q';
	    } else {
		c = linebuf[0];
		if (c == '\0')
		    c = 'J';
	    }
	    CAP_LETTER(c);
	} else {
	    if (list_servers)
		c = 'S';
	    else if (auto_shutdown)
		c = 'D';
	    else
		c = 'J';
	    linebuf[0] = linebuf[1] = '\0';
	}

	/*
	 * For each command to the server create a new socket and bind
	 * the socket to the server's address and destination port.
	 * This assures us that we only get replies to the last command sent.
	 */
	if (ibuf->sock.fd != SOCK_FD_INVALID) {
	    close_dgram_socket(&ibuf->sock);
	    ibuf->sock.fd = SOCK_FD_INVALID;
	}

	privileged_cmd = (strchr("DKLMO", c) != NULL) ? true : false;
	if (privileged_cmd) {
	    if (!has_credentials) {
		success = create_dgram_addr_socket(
		    &ibuf->sock, conpar->server_addr, 0);
		if (success == SOCK_IS_ERROR) {
		    printf("Server %s is not local, "
			   "privileged command not possible.\n",
			   conpar->server_addr);
		    continue;
		}
		close_dgram_socket(&ibuf->sock);
	    }
	    if ((success = create_dgram_addr_socket(
		&ibuf->sock, localhost, 0)) == SOCK_IS_ERROR) {
		error("Could not create localhost socket");
		exit(1);
	    }
	    if (sock_connect(&ibuf->sock, localhost, conpar->server_port)
		== SOCK_IS_ERROR) {
		error("Can't connect to local server %s on port %d\n",
		      localhost, conpar->server_port);
		return false;
	    }
	} else {
	    if ((success = create_dgram_socket(&ibuf->sock, 0))
		== SOCK_IS_ERROR) {
		error("Could not create socket");
		exit(1);
	    }
	    if (sock_connect(
		&ibuf->sock, conpar->server_addr, conpar->server_port)
		== SOCK_IS_ERROR
		&& !dgram_one_socket) {
		error("Can't connect to server %s on port %d\n",
		      conpar->server_addr, conpar->server_port);
		return false;
	    }
	}

	Sockbuf_clear(ibuf);
	Packet_printf(ibuf, "%u%s%hu", VERSION2MAGIC(conpar->server_version),
		      conpar->user_name, sock_get_port(&ibuf->sock));

	if (privileged_cmd && !has_credentials)
	    Packet_printf(ibuf, "%c%ld", CREDENTIALS_pack, 0L);
	else {

	    switch (c) {

		/*
		 * Owner only commands:
		 */

	    case 'K':
		printf("Enter name of victim: ");
		fflush(stdout);
		if (!get_line(linebuf, MAX_LINE, stdin)) {
		    printf("Nothing changed.\n");
		    continue;
		}
		linebuf[MAX_NAME_LEN - 1] = '\0';
		Packet_printf(ibuf, "%c%ld%s", KICK_PLAYER_pack, key, linebuf);
		break;

	    case 'M':				/* Send a message to server. */
		printf("Enter message: ");
		fflush(stdout);
		if (!get_line(linebuf, MAX_LINE, stdin) || !linebuf[0]) {
		    printf("No message sent.\n");
		    continue;
		}
		linebuf[MAX_CHARS - 1] = '\0';
		Packet_printf(ibuf, "%c%ld%s", MESSAGE_pack, key, linebuf);
		break;

	    case 'L':				/* Lock the game. */
		Packet_printf(ibuf, "%c%ld", LOCK_GAME_pack, key);
		break;

	    case 'D':				/* Shutdown */
		if (!auto_shutdown) {
		    printf("Enter delay in seconds or return for cancel: ");
		    get_line(linebuf, MAX_LINE, stdin);
		    /*
		     * No argument = cancel shutdown = arg_int=0
		     */
		    if (sscanf(linebuf, "%d", &delay) <= 0)
			delay = 0;
		    else
			if (delay <= 0)
			    delay = 1;

		    printf("Enter reason: ");
		    get_line(linebuf, MAX_LINE, stdin);
		} else {
		    strlcpy(linebuf, shutdown_reason, sizeof(linebuf));
		    delay = 60;
		}
		linebuf[MAX_CHARS - 1] = '\0';
		Packet_printf(ibuf, "%c%ld%d%s",
			      SHUTDOWN_pack, key, delay, linebuf);
		break;

	    case 'O':				/* Tune an option. */
		printf("Enter option: ");
		fflush(stdout);
		if (!get_line(linebuf, MAX_LINE, stdin)
		    || (len=strlen(linebuf)) == 0) {
		    printf("Nothing changed.\n");
		    continue;
		}
		printf("Enter new value for %s: ", linebuf);
		fflush(stdout);
		strcat(linebuf, ":"); len++;
		if (!get_line(&linebuf[len], MAX_LINE-len, stdin)
		    || linebuf[len] == '\0') {
		    printf("Nothing changed.\n");
		    continue;
		}
		printf("option \"%s\"\n", linebuf); fflush(stdout);
		Packet_printf(ibuf, "%c%ld%S", OPTION_TUNE_pack, key, linebuf);
		break;

		/*
		 * Public commands:
		 */

	    case 'J':				/* Trying to enter game. */
		if (linebuf[1] == '0') {
		    printf("Team '0' is reserved for robots.");
		    conpar->team = TEAM_NOT_SET;
		}
		else if (linebuf[1] > '0' && linebuf[1] <= '9') {
		    conpar->team = linebuf[1] - '0';
		    printf("Joining team %d\n", conpar->team);
		}
		else if (linebuf[1] == '-') {
		    conpar->team = TEAM_NOT_SET;
		    printf("Team set to unspecified\n");
		}
		else if (linebuf[1] != '\0')
		    conpar->team = TEAM_NOT_SET;

		Packet_printf(ibuf, "%c%s%s%s%d", ENTER_QUEUE_pack,
			      conpar->nick_name, conpar->disp_name,
			      conpar->host_name, conpar->team);
		time(&qsent);
		break;

	    case 'S':				/* Report status. */
		Packet_printf(ibuf, "%c", REPORT_STATUS_pack);
		break;

	    case 'V':				/* View options. */
		Packet_printf(ibuf, "%c", OPTION_LIST_pack);
		max_replies = 5;
		break;

		/*
		 * User interface commands:
		 */

	    case 'N':				/* Next server. */
		return false;

	    case 'T':				/* Set team. */
		printf("Enter team: ");
		fflush(stdout);
		if (!get_line(linebuf, MAX_LINE, stdin)
		    || (len = strlen(linebuf)) == 0)
		    printf("Nothing changed.\n");
		else {
		    int newteam;
		    if (sscanf(linebuf, " %d", &newteam) != 1)
			printf("Invalid team specification: %s.\n", linebuf);
		    else if (newteam >= 0 && newteam <= 9) {
			conpar->team = newteam;
			printf("Team set to %d\n", conpar->team);
		    } else {
			conpar->team = TEAM_NOT_SET;
			printf("Team set to unspecified\n");
		    }
		}
		continue;

	    case 'Q':
		exit (0);
		break;

	    case '?':
	    case 'H':				/* Help. */
	    default:
		Command_help();

		/*
		 * Next command.
		 */
		continue;
	    }
	}

	retries = (c == 'J' || c == 'S') ? 2 : 0;
	for (i = 0; i <= retries; i++) {
	    if (i > 0) {
		sock_set_timeout(&ibuf->sock, 1, 0);
		if (sock_readable(&ibuf->sock))
		    break;
	    }
	    if (sock_write(&ibuf->sock, ibuf->buf, ibuf->len) != ibuf->len) {
		error("Couldn't send request to server.");
		exit(1);
	    }
	}

	/*
	 * Get reply message(s).  If we failed, return false (next server).
	 */
	sock_set_timeout(&ibuf->sock, 2, 0);
	do {
	    Sockbuf_clear(ibuf);
	    if (Get_reply_message(ibuf, conpar) <= 0) {
		warn("No answer from server");
		return false;
	    }
	    if (Packet_scanf(ibuf, "%c%c", &reply_to, &status) <= 0) {
		warn("Incomplete reply from server");
		return false;
	    }

	    sock_set_timeout(&ibuf->sock, 0, 500*1000);

	    /*
	     * Now try and interpret the result.
	     */
	    errno = 0;
	    switch (status) {

	    case SUCCESS:
		/*
		 * Oh glorious success.
		 */
		switch (reply_to & 0xFF) {

		case OPTION_LIST_pack:
		    while (Packet_scanf(ibuf, "%S", linebuf) > 0)
			printf("%s\n", linebuf);
		    break;

		case REPORT_STATUS_pack:
		    /*
		     * Did the reply include a string?
		     */
		    if (ibuf->len > ibuf->ptr - ibuf->buf
			&& (!auto_connect || list_servers)) {
			if (list_servers)
			    printf("SERVER HOST......: %s\n",
				   conpar->server_name);
			if (*ibuf->ptr != '\0') {
			    if (ibuf->len < ibuf->size)
				ibuf->buf[ibuf->len] = '\0';
			    else
				ibuf->buf[ibuf->size - 1] = '\0';
			    Clean_string(ibuf->ptr);
			    printf("%s", ibuf->ptr);
			    if (ibuf->ptr[strlen(ibuf->ptr) - 1] != '\n')
				printf("\n");
			}
		    }
		    break;

		case SHUTDOWN_pack:
		    if (delay == 0)
			puts("*** Shutdown stopped.");
		    else
			puts("*** Shutdown initiated.");
		    break;

		case ENTER_GAME_pack:
		    if (Packet_scanf(ibuf, "%hu", &port) <= 0) {
			warn("Incomplete login reply from server");
			conpar->login_port = -1;
		    } else {
			conpar->login_port = port;
			printf("*** Login allowed.\n");
		    }
		    break;

		case ENTER_QUEUE_pack:
		    if (Packet_scanf(ibuf, "%hu", &qpos) <= 0)
			warn("Incomplete queue reply from server");
		    else {
			printf("... queued at position %2d\n", qpos);
			IFWINDOWS(Progress("Queued at position %2d\n", qpos));
		    }
		    /*
		     * Acknowledge each 10 seconds that we are still
		     * interested to be on the waiting queue.
		     */
		    if (qsent + 10 <= time(NULL)) {
			Sockbuf_clear(ibuf);
			Packet_printf(ibuf, "%u%s%hu",
				      VERSION2MAGIC(conpar->server_version),
				      conpar->user_name,
				      sock_get_port(&ibuf->sock));
			Packet_printf(ibuf, "%c%s%s%s%d", ENTER_QUEUE_pack,
				      conpar->nick_name, conpar->disp_name,
				      conpar->host_name, conpar->team);
			if (sock_write(&ibuf->sock, ibuf->buf, ibuf->len)
			    != ibuf->len) {
			    error("Couldn't send request to server.");
			    exit(1);
			}
			time(&qsent);
		    }
		    sock_set_timeout(&ibuf->sock, 12, 0);
		    max_replies = 2;
		    break;

		case CREDENTIALS_pack:
		    if (Packet_scanf(ibuf, "%ld", &key) <= 0)
			warn("Incomplete credentials reply from server");
		    else {
			has_credentials = true;
			cmd_credentials = c;
			continue;
		    }
		    break;

		default:
		    puts("*** Operation successful.");
		    break;
		}
		break;

	    case E_NOT_OWNER:
		warn("Permission denied, not owner");
		break;
	    case E_GAME_FULL:
		warn("Sorry, game full");
		break;
	    case E_TEAM_FULL:
		warn("Sorry, team %d is full", conpar->team);
		break;
	    case E_TEAM_NOT_SET:
		warn("Sorry, team play selected "
		     "and you haven't specified your team");
		break;
	    case E_GAME_LOCKED:
		warn("Sorry, game locked");
		break;
	    case E_NOT_FOUND:
		warn("That player is not logged on this server");
		break;
	    case E_IN_USE:
		warn("Your nick is already used");
		break;
	    case E_SOCKET:
		warn("Server can't setup socket");
		break;
	    case E_INVAL:
		warn("Invalid input parameters says the server");
		break;
	    case E_VERSION:
		warn("We have an incompatible version says the server");
		break;
	    case E_NOENT:
		warn("No such variable, says the server");
		break;
	    case E_UNDEFINED:
		warn("Requested operation is undefined, says the server");
		break;
	    default:
		warn("Server answers with unknown error status '%02x'",
		     status);
		break;
	    }

	    if (list_servers)	/* If listing servers, go to next one */
		return false;

	    if (auto_shutdown)	/* Do the same if we've sent a -shutdown */
		return false;

	    if (auto_connect && status != SUCCESS)
		return false;

	    /*
	     * If we wanted to enter the game and we were allowed to, return
	     * true (we are done).  If we weren't allowed, either return false
	     * (get next server) if we are auto_connecting or get next command
	     * if we aren't auto_connecting (interactive).
	     */
	    if (reply_to == ENTER_GAME_pack) {
		if (status == SUCCESS && conpar->login_port > 0)
		    return true;
		else {
		    if (auto_connect)
			return false;
		}
	    }

	} while (--max_replies > 0 && sock_readable(&ibuf->sock));

	/*
	 * Get next command.
	 */
    }

    /*NOTREACHED*/
}



/*
 * Setup a socket and a buffer for client-server messages.
 * We do this again for each server to prevent getting
 * old messages from past servers.
 */
int Connect_to_server(int auto_connect, int list_servers,
		      int auto_shutdown, char *shutdown_reason,
		      Connect_param_t *conpar)
{
    sockbuf_t		ibuf;			/* info buffer */
    int			result;

    if (Sockbuf_init(&ibuf, NULL, CLIENT_RECV_SIZE,
		     SOCKBUF_READ | SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1) {
	error("No memory for info buffer");
	exit(1);
    }
    result = Process_commands(&ibuf,
			     auto_connect, list_servers,
			     auto_shutdown, shutdown_reason,
			     conpar);
    if (ibuf.sock.fd != SOCK_FD_INVALID) {
	close_dgram_socket(&ibuf.sock);
	ibuf.sock.fd = SOCK_FD_INVALID;
    }
    Sockbuf_cleanup(&ibuf);

    return result;
}


int Contact_servers(int count, char **servers,
		    int auto_connect, int list_servers,
		    int auto_shutdown, char *shutdown_reason,
		    int find_max, int *num_found,
		    char **server_addresses, char **server_names,
		    unsigned *server_versions,
		    Connect_param_t *conpar)
{
    int			connected = false;
    const int		max_retries = 2;
    int			i, ret;
    int			status;
    sock_t		sock;
    int			retries;
    int			contacted;
    bool		compat_mode = false;
    sockbuf_t		sbuf;			/* contact buffer */


    if ((status = create_dgram_socket(&sock, 0)) == SOCK_IS_ERROR) {
	error("Could not create connection socket");
	exit(1);
    }
    if (Sockbuf_init(&sbuf, &sock, CLIENT_RECV_SIZE,
		     SOCKBUF_READ | SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1) {
	error("No memory for contact buffer");
	exit(1);
    }
    if (!count) {
	retries = 0;
	contacted = 0;
	compat_mode = false;
	do {
	    Sockbuf_clear(&sbuf);
	    Packet_printf(&sbuf, "%u%s%hu%c", MAGIC, conpar->user_name,
			  sock_get_port(&sbuf.sock), CONTACT_pack);
	    assert(sbuf.len >= 0);
	    if (Query_all(&sbuf.sock, conpar->contact_port,
			  sbuf.buf, (size_t)sbuf.len) == -1) {
		error("Couldn't send contact requests");
		exit(1);
	    }
	    if (retries == 0) {
		printf("Searching for an XPilot "
		       "server on the local net...\n");
		IFWINDOWS( Progress("Searching for an XPilot "
				    "server on the local net...") );
	    } else {
		printf("Searching once more...\n");
		IFWINDOWS( Progress("Searching once more...") );
	    }
	    while (Get_contact_message(&sbuf, "", conpar)) {
		contacted++;
		if (list_servers == 2) {
		    if (count < find_max) {
			if (server_names) {
			    strlcpy(server_names[count],
				    conpar->server_name,
				    MAX_HOST_LEN);
			}
			if (server_addresses) {
			    strlcpy(server_addresses[count],
				    conpar->server_addr,
				    MAX_HOST_LEN);
			}
			if (server_versions)
			    server_versions[count] = conpar->server_version;
			count++;
		    }
		    if (num_found)
			*num_found = count;
		} else {
		    connected = Connect_to_server(auto_connect,
						  list_servers,
						  auto_shutdown,
						  shutdown_reason,
						  conpar);
		    if (connected)
			break;
		}
	    }
	} while (!contacted && retries++ < max_retries);
    }
    else {
	for (i = 0; i < count && !connected; i++) {
	    retries = 0;
	    contacted = 0;
	    do {
		printf("Contacting server %s.\n", servers[i]);
		IFWINDOWS( Progress("Contacting server %s", servers[i]) );
		Sockbuf_clear(&sbuf);
		Packet_printf(&sbuf, "%u%s%hu%c",
			      compat_mode ? COMPATIBILITY_MAGIC : MAGIC,
			      conpar->user_name, sock_get_port(&sbuf.sock),
			      CONTACT_pack);
		if (sock_send_dest(&sbuf.sock, servers[i],
			      conpar->contact_port,
			      sbuf.buf, sbuf.len) == -1) {
		    if (sbuf.sock.error.call == SOCK_CALL_GETHOSTBYNAME) {
			printf("Can't find the server '%s'.\n", servers[i]);
			IFWINDOWS( Progress("Can't find the server '%s'.",
					    servers[i]) );
			break;
		    }
		    error("Can't contact %s on port %d",
			  servers[i], conpar->contact_port);
		}
		if (retries) {
		    printf("Retrying %s...\n", servers[i]);
		    IFWINDOWS( Progress("Retrying %s...", servers[i]) );
		}
		ret = Get_contact_message(&sbuf, servers[i], conpar);
		if (ret == 2 && !compat_mode) {
		    printf("Trying compatibility version %04x\n",
			   MAGIC2VERSION(COMPATIBILITY_MAGIC));
		    compat_mode = true;
		    retries--;	/* a bit ugly, cancels the loop ++ */
		    continue;
		}
		if (ret == 1) {
		    contacted++;
		    IFWINDOWS( Progress("Contacted %s", servers[i]) );
		    connected = Connect_to_server(auto_connect, list_servers,
						  auto_shutdown,
						  shutdown_reason,
						  conpar);
		    if (connected)
			break;
		}
	    } while (!contacted && retries++ < max_retries);
	}
    }
    Sockbuf_cleanup(&sbuf);
    close_dgram_socket(&sock);

    return connected ? true : false;
}
