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

#ifndef META_H
#define META_H

/*
 * max number of servers we can find on the local network.
 */
#define MAX_LOCAL_SERVERS	10

/*
 * Some constants for describing access to the meta servers.
 * XXX These are also defined in some other file.
 */
#define NUM_METAS		2
#define META_PROG_PORT		4401
#define META_USER_PORT		4400
#define NUM_META_DATA_FIELDS	18
#define META_INIT_SOCK {-2, {0, 0}, 0, {0, 0, 0}, NULL, NULL}

#define PING_UNKNOWN	10000	/* never transmitted a ping to it */
#define PING_NORESP	9999	/* never responded to our ping */
#define PING_SLOW	9998	/* responded to first ping after
				 * we had already retried (ie slow!) */

/*
 * Access the data field of one of the servers
 * which is listed by the meta servers.
 */
#define SI_DATA(it)		((server_info_t *)LI_DATA(it))

/********************** Data Structures *********************/

/*
 * All the fields for a server in one line of meta output.
 * the strings in this structure should really be an array
 * of char pointers to reduce the amount of code.
 */
struct ServerInfo {
    char *version,
	*hostname,
	*users_str,
	*mapname,
	*mapsize,
	*author,
	*status,
	*bases_str,
	*fps_str,
	*playlist,
	*sound,
	*teambases_str,
	*timing, *ip_str, *freebases, *queue_str, *domain, pingtime_str[5];
    unsigned port,
	ip, users, bases, fps, uptime, teambases, queue, pingtime;
    struct timeval start;
    unsigned char serial;
};

extern list_t server_list;
extern time_t server_list_creation_time;
extern list_iter_t server_it;

typedef struct ServerInfo server_info_t;

/*
 * Here we hold the servers which are listed by the meta servers.
 * We record the time we contacted Meta so as to not overload Meta.
 * server_it is an iterator pointing at the first server for the next page.
 */


enum MetaState {
    MetaConnecting = 0,
    MetaReadable = 1,
    MetaReceiving = 2
};

/*
 * Structure describing a meta server.
 * Hostname, IP address, and socket filedescriptor.
 */

struct Meta {
    char name[MAX_HOST_LEN];
    char addr[16];
    sock_t sock;
    enum MetaState state;       /* connecting, readable, receiving */
};


void  Delete_server_list(void);
void  Delete_server_info(server_info_t * sip);
void  string_to_lower(char *s);
char *Get_domain_from_hostname(char *host_name);
int   Welcome_sort_server_list(void);
int   Add_server_info(server_info_t * sip);
char *my_strtok(char *buf, const char *sep);
void  Add_meta_line(char *meta_line);
void  Meta_connect(int *connections_ptr, int *maxfd_ptr);
void  Meta_dns_lookup(void);
void  Ping_servers(void);
int   Get_meta_data(char *errorstr);

#endif


