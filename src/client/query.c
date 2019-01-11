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


#if defined(_WINDOWS)
# ifndef QUERY_FUDGED
#  define QUERY_FUDGED
# endif
#endif
#ifdef _IBMESA
# define _SOCKADDR_LEN
#endif

#ifndef MAX_INTERFACE
#define MAX_INTERFACE    16	/* Max. number of network interfaces. */
#endif
#ifdef DEBUGBROADCAST
#undef D
#define D(x)	x
#endif

/*
 * Query all hosts on a subnet one after another.
 * This should be avoided as much as possible.
 * It may cause network congestion and therefore fail,
 * because UDP is unreliable.
 * We only allow this horrible kludge for subnets with 8 or less
 * bits in the host part of the subnet mask.
 * Subnets with irregular subnet bits are properly handled (I hope).
 */
static int Query_subnet(sock_t *sock,
			struct sockaddr_in *host_addr,
			struct sockaddr_in *mask_addr,
			char *msg,
			size_t msglen)
{
    int i, nbits, max;
    unsigned long bit, mask, dest, host, hostmask, hostbits[256];
    struct sockaddr_in addr;

    addr = *host_addr;
    host = ntohl(host_addr->sin_addr.s_addr);
    mask = ntohl(mask_addr->sin_addr.s_addr);
    memset ((void *)hostbits, 0, sizeof hostbits);
    nbits = 0;
    hostmask = 0;

    /*
     * Only the lower 32 bits of an unsigned long are used.
     */
    for (bit = 1; (bit & 0xffffffff) != 0; bit <<= 1) {
	if ((mask & bit) != 0)
	    continue;

	if (nbits >= 8) {
	    /* break; ? */
	    warn("too many host bits in subnet mask");
	    return (-1);
	}
	hostmask |= bit;
	for (i = (1 << nbits); i < 256; i++) {
	    if ((i & (1 << nbits)) != 0)
		hostbits[i] |= bit;
	}
	nbits++;
    }
    if (nbits < 2) {
	warn("malformed subnet mask");
	return (-1);
    }

    /*
     * The first and the last address are reserved for the subnet.
     * So, for an 8 bit host part only 254 hosts are tried, not 256.
     */
    max = (1 << nbits) - 2;
    for (i=1; i <= max; i++) {
	dest = (host & ~hostmask) | hostbits[i];
	addr.sin_addr.s_addr = htonl(dest);
	sock_get_error(sock);
	sendto(sock->fd, msg, msglen, 0,
	       (struct sockaddr *)&addr, sizeof(addr));
	D( printf("sendto %s/%d\n",
		  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)) );
	/*
	 * Imagine a server responding to our query while we
	 * are still transmitting packets for non-existing servers
	 * and the server packet colliding with one of our packets.
	 */
	micro_delay((unsigned)10000);
    }

    return 0;
}


static int Query_fudged(sock_t *sock, int port, char *msg, size_t msglen)
{
    int			i, count = 0;
    unsigned char	*p;
    struct sockaddr_in	addr, subnet;
    struct hostent	*h;
    unsigned long	addrmask, netmask;
    char		host_name[64];

    gethostname(host_name, sizeof(host_name));
    if ((h = gethostbyname(host_name)) == NULL) {
	error("gethostbyname");
	return -1;
    }
    if (h->h_addrtype != AF_INET || h->h_length != 4) {
	warn("Dunno about addresses with address type %d and length %d\n",
	     h->h_addrtype, h->h_length);
	return -1;
    }
    for (i = 0; h->h_addr_list[i]; i++) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = (unsigned short)htons((unsigned short)port);
	p = (unsigned char *) h->h_addr_list[i];
	addrmask = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
	addr.sin_addr.s_addr = htonl(addrmask);
	subnet = addr;
	if (addrmask == 0x7F000001) {
	    sock_get_error(sock);
	    if (sendto(sock->fd, msg, msglen, 0,
		       (struct sockaddr *)&addr, sizeof(addr)) != -1)
		count++;
	} else {
	    netmask = 0xFFFFFF00;
	    subnet.sin_addr.s_addr = htonl(netmask);
	    if (Query_subnet(sock, &addr, &subnet, msg, msglen) != -1)
		count++;
	}
    }
    if (count == 0) {
	errno = 0;
	count = -1;
    }
    return count;
}


/*
 * Send a datagram on all network interfaces of the local host.  Return the
 * number of packets succesfully transmitted.
 * We only use the loopback interface if we didn't do a broadcast
 * on one of the other interfaces in order to reduce the chance that
 * we get multiple responses from the same server.
 */

int Query_all(sock_t *sock, int port, char *msg, size_t msglen)
{
#ifdef QUERY_FUDGED
    return Query_fudged(sock, port, msg, msglen);
#else

    int         	fd, len, ifflags, count = 0;
    /* int			broadcasts = 0; */
    int			haslb = 0;
    struct sockaddr_in	addr, mask, loopback;
    struct ifconf	ifconf;
    struct ifreq	*ifreqp, ifreq, ifbuf[MAX_INTERFACE];

    /*
     * Broadcasting on a socket must be explicitly enabled.
     */
    if (sock_set_broadcast(sock, 1) == -1) {
	error("set broadcast");
	return (-1);
    }

    /*
     * Create an unbound datagram socket.  Only used for ioctls.
     */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	error("socket");
	return (-1);
    }

    /*
     * Get names and addresses of all local network interfaces.
     */
    ifconf.ifc_len = sizeof(ifbuf);
    ifconf.ifc_buf = (caddr_t)ifbuf;
    memset((void *)ifbuf, 0, sizeof(ifbuf));
    if (ioctl(fd, SIOCGIFCONF, (char *)&ifconf) == -1) {
	error("ioctl SIOCGIFCONF");
	close(fd);
	return Query_fudged(sock, port, msg, msglen);
    }
    for (len = 0; len + (int)sizeof(struct ifreq) <= ifconf.ifc_len;) {
	ifreqp = (struct ifreq *)&ifconf.ifc_buf[len];

	D( printf("interface name %s\n", ifreqp->ifr_name) );
	D( printf("\taddress family %d\n", ifreqp->ifr_addr.sa_family) );

	len += sizeof(struct ifreq);
#if (defined(BSD) && BSD >= 199006) \
     || (defined(HAVE_SA_LEN) && HAVE_SA_LEN != 0) \
     || defined(_SOCKADDR_LEN) || defined(_AIX)
	/*
	 * Recent TCP/IP implementations have a sa_len member in the socket
	 * address structure in order to support protocol families that have
	 * bigger addresses.
	 */
	if (ifreqp->ifr_addr.sa_len > sizeof(ifreqp->ifr_addr)) {
	    len += ifreqp->ifr_addr.sa_len - sizeof(ifreqp->ifr_addr);
	    D( printf("\textra address length %d\n",
		      ifreqp->ifr_addr.sa_len - sizeof(ifreqp->ifr_addr)) );
	}
#endif
	if (ifreqp->ifr_addr.sa_family != AF_INET)
	    /*
	     * Not supported.
	     */
	    continue;

	addr = *(struct sockaddr_in *)&ifreqp->ifr_addr;
	D( printf("\taddress %s\n", inet_ntoa(addr.sin_addr)) );

	/*
	 * Get interface flags.
	 */
	ifreq = *ifreqp;
	if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifreq) == -1) {
	    error("ioctl SIOCGIFFLAGS");
	    continue;
	}
	ifflags = ifreq.ifr_flags;

	if ((ifflags & IFF_UP) == 0) {
	    D( printf("\tinterface is down\n") );
	    continue;
	}
	D( printf("\tinterface %s running\n",
		  (ifflags & IFF_RUNNING) ? "is" : "not") );

	if ((ifflags & IFF_LOOPBACK) != 0) {
	    D( printf("\tloopback interface\n") );
	    /*
	     * Only send on the loopback if we don't broadcast.
	     */
	    if (haslb == 0) {
		loopback = *(struct sockaddr_in *)&ifreq.ifr_addr;
		haslb = 1;
	    }
	    continue;
	} else if ((ifflags & IFF_POINTOPOINT) != 0) {
	    D( printf("\tpoint-to-point interface\n") );
	    ifreq = *ifreqp;
	    if (ioctl(fd, SIOCGIFDSTADDR, (char *)&ifreq) == -1) {
		error("ioctl SIOCGIFDSTADDR");
		continue;
	    }
	    addr = *(struct sockaddr_in *)&ifreq.ifr_addr;
	    D(printf("\tdestination address %s\n", inet_ntoa(addr.sin_addr)));
	} else if ((ifflags & IFF_BROADCAST) != 0) {
	    D( printf("\tbroadcast interface\n") );
	    ifreq = *ifreqp;
	    if (ioctl(fd, SIOCGIFBRDADDR, (char *)&ifreq) == -1) {
		error("ioctl SIOCGIFBRDADDR");
		continue;
	    }
	    addr = *(struct sockaddr_in *)&ifreq.ifr_addr;
	    D( printf("\tbroadcast address %s\n", inet_ntoa(addr.sin_addr)) );
	} else {
	    /*
	     * Huh?  It's not a loopback and not a point-to-point
	     * and it doesn't have a broadcast address???
	     * Something must be rotten here...
	     */
	}

	if ((ifflags & (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST)) != 0) {
	    /*
	     * Well, we have an address (at last).
	     */
	    addr.sin_port = htons(port);
	    if (sendto(sock->fd, msg, msglen, 0, (struct sockaddr *)&addr,
		       sizeof addr) == (ssize_t)msglen) {
		D(printf("\tsendto %s/%d\n", inet_ntoa(addr.sin_addr), port));
		/*
		 * Success!
		 */
		count++;
		/* if ((ifflags & (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST))
		    == IFF_BROADCAST) {
		    broadcasts++;
		} */
		continue;
	    }

	    /*
	     * Failure.
	     */
	    error("sendto %s/%d failed", inet_ntoa(addr.sin_addr), port);

	    if ((ifflags & (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST))
		!= IFF_BROADCAST)
		/*
		 * It wasn't the broadcasting that failed.
		 */
		continue;

	    /*
	     * Broadcasting failed.
	     * Try it in a different (kludgy) manner.
	     */
	}

	/*
	 * Get the netmask for this interface.
	 */
	ifreq = *ifreqp;
	if (ioctl(fd, SIOCGIFNETMASK, (char *)&ifreq) == -1) {
	    error("ioctl SIOCGIFNETMASK");
	    continue;
	}
	mask = *(struct sockaddr_in *)&ifreq.ifr_addr;
	D( printf("\tmask %s\n", inet_ntoa(mask.sin_addr)) );

	addr.sin_port = htons(port);
	if (Query_subnet(sock, &addr, &mask, msg, msglen) != -1) {
	    count++;
	    /* broadcasts++; */
	}
    }

    /*
     * Normally we wouldn't send a query over the loopback interface
     * if we successfully have sent one or more broadcast queries,
     * but it happens that some Linux machines which have firewalling
     * packet filters installed don't copy outgoing broadcast packets
     * to their local sockets.  Therefore we now always also send
     * one query to the loopback address just to be sure we reach
     * our own server.  That we now may receive two or more replies
     * from the same server is not as serious as not receiving any
     * reply would be.
     */
    if (haslb /* && broadcasts == 0 */) {
	/*
	 * We may not have reached the localhost yet.
	 */
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr = loopback.sin_addr;
	addr.sin_port = htons(port);
	if (sendto(sock->fd, msg, msglen, 0, (struct sockaddr *)&addr,
		   sizeof addr) == (ssize_t)msglen) {
	    D(printf("\tsendto %s/%d\n", inet_ntoa(addr.sin_addr), port));
	    count++;
	} else
	    error("sendto %s/%d failed", inet_ntoa(addr.sin_addr), port);
    }

    close(fd);

    if (count == 0) {
	errno = 0;
	count = -1;
    }

    return count;

#endif	/* QUERY_FUDGED */
}

