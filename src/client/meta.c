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

#include "xpclient.h"

static struct Meta metas[NUM_METAS] = {
    {META_HOST,     META_IP,     META_INIT_SOCK, MetaConnecting},
    {META_HOST_TWO, META_IP_TWO, META_INIT_SOCK, MetaConnecting}
};

list_t server_list;
time_t server_list_creation_time;
list_iter_t server_it;

/*
 * Convert a string to lowercase.
 */
void string_to_lower(char *s)
{
    for (; *s; s++)
	*s = tolower(*s);
}

/*
 * From a hostname return the part after the last dot.
 * E.g.: Vincent.CS.Berkeley.EDU will return EDU.
 */
char *Get_domain_from_hostname(char *host_name)
{
    static char last_domain[] = "\x7E\x7E";
    char *dom;

    if ((dom = strrchr(host_name, '.')) != NULL) {
	if (dom[1] == '\0') {
	    dom[0] = '\0';
	    dom = strrchr(host_name, '.');
	}
    }
    if (dom && !isdigit(dom[1]) && strlen(dom + 1) >= 2)
	return dom + 1;

    if (dom) {
	dom++;			/* skip dot */
	/* test toplevel domain for validity */
	if (!isdigit(*dom) && strlen(dom) >= 2 && strlen(dom) <= 3)
	    return dom;
    }

    return last_domain;
}

/*
 * Sort servers based on:
 *	1) number of players.
 *	2) pingtime.
 *	3) country.
 *	4) hostname.
 */
int Welcome_sort_server_list(void)
{
    list_t old_list = server_list;
    list_t new_list = List_new();
    list_iter_t it;
    int delta;
    void *vp;
    server_info_t *sip_old;
    server_info_t *sip_new;

    if (!new_list) {
        error("Not enough memory\n");
	return -1;
    }
    while ((vp = List_pop_front(old_list)) != NULL) {
	sip_old = (server_info_t *) vp;
	string_to_lower(sip_old->hostname);
	if (!strncmp(sip_old->hostname, "xpilot", 6)) {
	    sip_old->hostname[0] = 'X';
	    sip_old->hostname[1] = 'P';
	}
	sip_old->domain = Get_domain_from_hostname(sip_old->hostname);
	for (it = List_begin(new_list); it != List_end(new_list);
	     LI_FORWARD(it)) {
	    sip_new = SI_DATA(it);
	    delta = sip_new->users - sip_old->users;
	    if (delta < 0)
		break;
	    else if (delta > 0)
		continue;
	    delta = sip_old->pingtime - sip_new->pingtime;
	    if (delta < 0)
		break;
	    else if (delta > 0)
		continue;
	    delta = strcmp(sip_old->domain, sip_new->domain);
	    if (delta < 0)
		break;
	    else if (delta > 0)
		continue;
	    delta = strcmp(sip_old->hostname, sip_new->hostname);
	    if (delta < 0)
		break;
	    else if (delta > 0)
		continue;
	    if (sip_old->port < sip_new->port)
		break;
	}
	if (!List_insert(new_list, it, sip_old)) {
	    error("Not enough memory\n");
	    Delete_server_info(sip_old);
	}
    }

#if 0
    /* print for debugging */
    printf("\n");
    printf("Printing server list:\n");
    for (it = List_begin(new_list); it != List_end(new_list);
	 LI_FORWARD(it)) {
	sip_new = SI_DATA(it);
	printf("%2d %5s %-31s %u", sip_new->users, sip_new->domain,
	       sip_new->hostname, sip_new->port);
	if (sip_new->pingtime == PING_UNKNOWN)
	    printf("%8s", "unknown");
	else if (sip_new->pingtime == PING_NORESP)
	    printf("%8s", "no resp");
	else if (sip_new->pingtime == PING_SLOW)
	    printf("%8s", "s-l-o-w");
	else
	    printf("%8u", sip_new->pingtime);
	printf("\n");
    }
    printf("\n");
#endif

    List_delete(old_list);
    server_list = new_list;

    return 0;
}

/*
 * Put server info on a sorted list.
 */
int Add_server_info(server_info_t * sip)
{
    list_iter_t it;
    server_info_t *it_sip;

    if (!server_list) {
	server_list = List_new();
	if (!server_list) {
	    error("Not enough memory\n");
	    return -1;
	}
    }
    for (it = List_begin(server_list); it != List_end(server_list);
	 LI_FORWARD(it)) {
	it_sip = SI_DATA(it);
	/* sort on IP. */
	if (it_sip->ip < sip->ip)
	    continue;

	if (it_sip->ip == sip->ip) {
	    /* same server when same IP + port. */
	    if (it_sip->port < sip->port)
		continue;

	    if (it_sip->port == sip->port) {
		/* work around bug in meta: keep server with highest uptime. */
		if (it_sip->uptime > sip->uptime) {
		    return -1;
		} else {
		    it = List_erase(server_list, it);
		}
	    }
	}
	break;
    }
    if (!List_insert(server_list, it, sip)) {
        error("Not enough memory\n");
	return -1;
    }

    /* print for debugging */
    D(printf("list size = %d after %08x, %d\n",
	     List_size(server_list), sip->ip, sip->port));

    return 0;
}

/*
 * Variant on strtok which does not skip empty fields.
 * Two delimiters after another returns the empty string ("").
 */
char *my_strtok(char *buf, const char *sep)
{
    static char *oldbuf;
    char *ptr;
    char *start;


    if (buf)
	oldbuf = buf;

    start = oldbuf;
    if (!start || !*start)
	return NULL;

    for (ptr = start; *ptr; ptr++) {
	if (strchr(sep, *ptr))
	    break;
    }
    oldbuf = (*ptr) ? (ptr + 1) : (ptr);
    *ptr = '\0';
    return start;
}

/*
 * Parse one line of meta output and
 * put the fields in a structure.
 * The structure is put on a sorted list.
 */
void Add_meta_line(char *meta_line)
{
    char *fields[NUM_META_DATA_FIELDS];
    int i;
    int num = 0;
    char *p;
    unsigned ip0, ip1, ip2, ip3 = 0;
    char *text = xp_strdup(meta_line);
    server_info_t *sip;

    if (!text) {
        error("Not enough memory\n");
	return;
    }

    /* split line into fields. */
    for (p = my_strtok(text, ":"); p; p = my_strtok(NULL, ":")) {
	if (num < NUM_META_DATA_FIELDS) {
	    fields[num++] = p;
	}
    }
    if (num < NUM_META_DATA_FIELDS) {
	/* should not happen, except maybe for last line. */
	free(text);
	return;
    }
    if (fields[0] != text) {
	/* sanity check, should not happen. */
	free(text);
	return;
    }

    if ((sip = (server_info_t *) malloc(sizeof(server_info_t))) == NULL) {
        error("Not enough memory\n");
	free(text);
	return;
    }
    memset(sip, 0, sizeof(*sip));
    sip->pingtime = PING_UNKNOWN;
    sip->version = fields[0];
    sip->hostname = fields[1];
    sip->users_str = fields[3];
    sip->mapname = fields[4];
    sip->mapsize = fields[5];
    sip->author = fields[6];
    sip->status = fields[7];
    sip->bases_str = fields[8];
    sip->fps_str = fields[9];
    sip->playlist = fields[10];
    sip->sound = fields[11];
    sip->teambases_str = fields[13];
    sip->timing = fields[14];
    sip->ip_str = fields[15];
    sip->freebases = fields[16];
    sip->queue_str = fields[17];
    if (sscanf(fields[i = 2], "%u", &sip->port) != 1 ||
	sscanf(fields[i = 3], "%u", &sip->users) != 1 ||
	sscanf(fields[i = 8], "%u", &sip->bases) != 1 ||
	sscanf(fields[i = 9], "%u", &sip->fps) != 1 ||
	sscanf(fields[i = 12], "%u", &sip->uptime) != 1 ||
	sscanf(fields[i = 13], "%u", &sip->teambases) != 1 ||
	sscanf(fields[i = 15], "%u.%u.%u.%u", &ip0, &ip1, &ip2, &ip3) != 4
	|| (ip0 | ip1 | ip2 | ip3) > 255
	|| sscanf(fields[i = 17], "%u", &sip->queue) != 1) {
	printf("error %d in: %s\n", i, meta_line);
	free(sip);
	free(text);
	return;
    } else {
	sip->ip = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
	if (Add_server_info(sip) == -1) {
	    free(sip);
	    free(text);
	    return;
	}
    }
}

/*
 * Connect to the meta servers asynchronously.
 * Return the number of connections made,
 * and the highest fd.
 */
void Meta_connect(int *connections_ptr, int *maxfd_ptr)
{
    int i;
    int status;
    int connections = 0;
    int max = -1;

    for (i = 0; i < NUM_METAS; i++) {
	if (metas[i].sock.fd != SOCK_FD_INVALID)
	    sock_close(&metas[i].sock);

	status = sock_open_tcp_connected_non_blocking(&metas[i].sock,
						      metas[i].addr,
						      META_PROG_PORT);
	if (status == SOCK_IS_ERROR) {
	    error("%s\n", metas[i].addr);
	} else {
	    connections++;
	    if (metas[i].sock.fd > max)
		max = metas[i].sock.fd;
	}
    }
    if (connections_ptr)
	*connections_ptr = connections;

    if (maxfd_ptr)
	*maxfd_ptr = max;
}

/*
 * Lookup the IPs of the metas.
 */
void Meta_dns_lookup(void)
{
    int i;
    char *addr;

    for (i = 0; i < NUM_METAS; i++) {
	if (metas[i].sock.fd == -2) {
	    metas[i].sock.fd = SOCK_FD_INVALID;
	    addr = sock_get_addr_by_name(metas[i].name);
	    if (addr)
		strlcpy(metas[i].addr, addr, sizeof(metas[i].addr));
	}
    }
}

void Ping_servers(void)
{
    static int serial;		/* mark pings to identify stale reply */
    const int interval = 1000 / 14;	/* assumes we can do 14fps of pings */
    const int tries = 1;	/* at least 1 ping for ever server.
				 * in practice we get several */
    int maxwait = tries * interval * List_size(server_list);
    sock_t sock;
    fd_set input_mask, readmask;
    struct timeval start, end, timeout;
    list_iter_t it, that;
    server_info_t *it_sip;
    sockbuf_t sbuf, rbuf;
    int ms;
    char *reply_ip;
    int reply_port;
    unsigned reply_magic;
    unsigned char reply_serial, reply_status;
    int outstanding;
    
    if (sock_open_udp(&sock, NULL, 0) == -1) {
	return;
    }
    if (sock_set_non_blocking(&sock, 1) == -1) {
	sock_close(&sock);
	return;
    }
    if (Sockbuf_init(&sbuf, &sock, CLIENT_RECV_SIZE,
		     SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1) {
	sock_close(&sock);
	return;
    }
    if (Sockbuf_init(&rbuf, &sock, CLIENT_RECV_SIZE,
		     SOCKBUF_READ | SOCKBUF_DGRAM) == -1) {
	Sockbuf_cleanup(&sbuf);
	sock_close(&sock);
	return;
    }

    FD_ZERO(&input_mask);
    FD_SET(sock.fd, &input_mask);

    it = List_end(server_list);
    outstanding = 0;
    ms = 0;
    gettimeofday(&start, NULL);
    do {
	while (outstanding < (ms / interval + 1)) {
	    if (it == List_end(server_list)) {
		++serial;
		serial &= 0xFF;
		if (serial == 0)
		    serial = 1;

		/*
		 * Send a packet to the contact port with
		 * a valid magic number but client version
		 * zero.  The server will reply to this
		 * so that the client can tell the user
		 * what version they need.

		 * Normally this would be a CONTACT_pack but
		 * we cheat and use the packet type field as
		 * a serial number, since the server is
		 * nice enough to send back whatever we send.
		 */
		Sockbuf_clear(&sbuf);
		Packet_printf(&sbuf, "%u%s%hu%c",
			      MAGIC & 0xffff, "p",
			      sock_get_port(&sock), serial);

		/*
		 * Assuming sort order is the most to least
		 * desirable servers, give the interesting
		 * servers first crack at more pings, making
		 * their results more accurate.
		 */
		Welcome_sort_server_list();
		it = List_begin(server_list);
	    }
	    it_sip = SI_DATA(it);
	    sock_send_dest(&sock, it_sip->ip_str, it_sip->port,
			   sbuf.buf, sbuf.len);
	    gettimeofday(&it_sip->start, NULL);
	    /* if it has never been pinged (pung?) mark it now
	     * as "not responding" instead of just blank.
	     */
	    if (it_sip->pingtime == PING_UNKNOWN)
		it_sip->pingtime = PING_NORESP;

	    it_sip->serial = serial;
	    outstanding++;
	    LI_FORWARD(it);
	}
	timeout.tv_sec = 0;
	timeout.tv_usec = (interval - (ms % interval)) * 1000;
	readmask = input_mask;
	if (select(sock.fd + 1, &readmask, 0, 0, &timeout) == -1
	    && errno != EINTR)
	    break;

	gettimeofday(&end, NULL);
	ms = (end.tv_sec - start.tv_sec) * 1000 +
	    (end.tv_usec - start.tv_usec) / 1000;

	Sockbuf_clear(&rbuf);
	if ((rbuf.len = sock_receive_any(&sock, rbuf.buf, rbuf.size)) < 4)
	    continue;

	if (outstanding > 0)
	    --outstanding;

	if (Packet_scanf(&rbuf, "%u%c%c",
			 &reply_magic, &reply_serial,
			 &reply_status) <= 0)
	    continue;

	reply_ip = sock_get_last_addr(&sock);
	reply_port = sock_get_last_port(&sock);
	for (that = List_begin(server_list);
	     that != List_end(server_list); LI_FORWARD(that)) {
	    it_sip = SI_DATA(that);
	    if (!strcmp(it_sip->ip_str, reply_ip)
		&& reply_port == it_sip->port) {
		int n;

		if (reply_serial != it_sip->serial)
		    /* replied to an old ping, alive but
		     * slower than 'interval' at least
		     */
		    it_sip->pingtime = MIN(it_sip->pingtime, PING_SLOW);
		else {
		    n = (end.tv_sec - it_sip->start.tv_sec) * 1000 +
			(end.tv_usec - it_sip->start.tv_usec) / 1000;

		    /* kps - current value is more useful than minimum value */
#if 0
		    it_sip->pingtime = MIN(it_sip->pingtime, n);
#else
		    it_sip->pingtime = n;
#endif
		}
		break;
	    }
	}
    } while (ms < maxwait);

    Sockbuf_cleanup(&sbuf);
    Sockbuf_cleanup(&rbuf);
    sock_close(&sock);
}


/*
 * Deallocate the server list.
 */
void Delete_server_list(void)
{
    server_info_t *sip;

    if (server_list) {
	while ((sip =
		(server_info_t *) List_pop_front(server_list)) != NULL)
	    Delete_server_info(sip);
	List_delete(server_list);
	server_list = NULL;
	server_list_creation_time = 0;
    }
    server_it = NULL;
}

/*
 * Deallocate a ServerInfo structure.
 */
void Delete_server_info(server_info_t * sip)
{
    if (sip) {
	if (sip->version) {
	    free(sip->version);
	    sip->version = NULL;
	}
	free(sip);
    }
}

/*
 * User pressed the Internet button.
 */
int Get_meta_data(char *errorstr)
{
    int i;
    int max = -1;
    int connections = 0;
    int descriptor_count;
    int readers = 0;
    int senders = 0;
    int bytes_read;
    int buffer_space;
    int total_bytes_read = 0;
    int server_count;
    time_t start, now;
    fd_set rset_in, wset_in;
    fd_set rset_out, wset_out;
    struct timeval tv;
    char *newline;
 
    /*
     * Buffer to hold data from a socket connection to a Meta.
     * The ptr points to the first byte of the unprocessed data.
     * The end points to where the next new data should be loaded.
     */
    struct MetaData {
	char *ptr;
	char *end;
	char buf[4096];
    };
    struct MetaData md[NUM_METAS];

    
    /* lookup addresses. */
    Meta_dns_lookup();

    /* connect asynchronously. */
    Meta_connect(&connections, &max);
    if (!connections) {
	sprintf(errorstr, "Could not establish connections with any metaserver" \
		     ", either the meta is not responding or you have" \
		     " network problems" );
	return -1;
    }

    sprintf(errorstr, "Establishing %s with %d metaserver%s ... ",
	    ((connections > 1) ? "connections" : "a connection"),
	    connections, ((connections > 1) ? "s" : ""));

    /* setup select(2) structures. */
    FD_ZERO(&rset_in);
    FD_ZERO(&wset_in);
    for (i = 0; i < NUM_METAS; i++) {
	metas[i].state = MetaConnecting;
	if (metas[i].sock.fd != SOCK_FD_INVALID)
	    FD_SET(metas[i].sock.fd, &wset_in);

	md[i].ptr = NULL;
	md[i].end = NULL;
    }
    /*
     * First wait for the asynchronously connected sockets to become writable.
     * When a socket becomes writable it means that the connection attempt
     * has completed.  After that has happened we can test the socket for
     * readability.  When the connection attempt failed the read will
     * return -1 and probably set errno to ENOTCONN.
     *
     * We try to connect and read for a limited number of seconds.
     * Whenever a connection has succeeded we add another 5 seconds.
     * Whenever a read has succeeded we also add another 5 seconds.
     *
     * Keep administration of the number of sockets in the connected state,
     * the readability state, or the meta-is-sending-data state.
     */
    for (start = time(&now) + 5;
	 connections > 0 && now < start + 5; time(&now)) {
	tv.tv_sec = start + 5 - now;
	tv.tv_usec = 0;

	D(printf("select for %ld (con %d, read %d, send %d) at %ld\n",
		 tv.tv_sec, connections, readers, senders, time(0)));

	rset_out = rset_in;
	wset_out = wset_in;
	descriptor_count =
	    select(max + 1, &rset_out, &wset_out, NULL, &tv);

	D(printf("select = %d at %ld\n", descriptor_count, time(0)));

	if (descriptor_count <= 0)
	    break;

	for (i = 0; i < NUM_METAS; i++) {
	    if (metas[i].sock.fd == SOCK_FD_INVALID)
		continue;
	    else if (FD_ISSET(metas[i].sock.fd, &wset_out)) {
		/* promote socket from writable to readable. */
		FD_CLR(metas[i].sock.fd, &wset_in);
		FD_SET(metas[i].sock.fd, &rset_in);
		metas[i].state = MetaReadable;
		readers++;
		time(&start);
	    } else if (FD_ISSET(metas[i].sock.fd, &rset_out)) {
		if (md[i].ptr == NULL && md[i].end == NULL) {
		    md[i].ptr = md[i].buf;
		    md[i].end = md[i].buf;
		    time(&start);
		}
		buffer_space = &md[i].buf[sizeof(md[i].buf)] - md[i].end;
		bytes_read =
		    read(metas[i].sock.fd, md[i].end, buffer_space);
		if (bytes_read <= 0) {
  		    FD_CLR(metas[i].sock.fd, &rset_in);
		    close(metas[i].sock.fd);
		    metas[i].sock.fd = SOCK_FD_INVALID;
		    --connections;
		    --readers;
		    if (metas[i].state == MetaReceiving) {
			--senders;
			if (senders == 0 &&
			    server_list && List_size(server_list) >= 30) {
			    /*
			     * Assume that this meta has sent us all there is
			     */
			    connections = 0;
			}
		    }
		    if (connections == 0)
			break;
		} else {
		    /* Recevied some bytes from this connection. */
		    total_bytes_read += bytes_read;

		    /* If this connection wasn't marked
		     * as receiving do so now.
		     */
		    if (metas[i].state != MetaReceiving) {
			metas[i].state = MetaReceiving;
			++senders;
		    }

		    /* adjust buffer for newly read bytes. */
		    md[i].end += bytes_read;

		    /* process data up to the last line ending in a '\n'.
		     */
		    while ((newline
			    = (char *) memchr(md[i].ptr, '\n',
					      md[i].end - md[i].ptr))
			   != NULL) {
			*newline = '\0';
			if (newline > md[i].ptr && newline[-1] == '\r')
			    newline[-1] = '\0';

			Add_meta_line(md[i].ptr);
			md[i].ptr = newline + 1;
		    }
		    /* move partial data to the start of the buffer. */
		    if (md[i].ptr > md[i].buf) {
			int incomplete_data = (md[i].end - md[i].ptr);
			memmove(md[i].buf, md[i].ptr, incomplete_data);
			md[i].ptr = md[i].buf;
			md[i].end = md[i].ptr + incomplete_data;
		    }
		    /* allow more time to receive more data */
		    time(&start);
		}
	    }
	}
    }

    for (i = 0; i < NUM_METAS; i++) {
	if (metas[i].sock.fd != SOCK_FD_INVALID) {
	    close(metas[i].sock.fd);
	    metas[i].sock.fd = SOCK_FD_INVALID;
	}
    }

    server_count = 0;
    if (server_list)
	server_count = List_size(server_list);

    if (server_count > 0) {
	sprintf(errorstr, "Received information about %d Internet servers",
		server_count);
	server_list_creation_time = time(NULL);
    } else
	sprintf(errorstr, "Could not contact any Internet Meta server");


    return server_count;
}
