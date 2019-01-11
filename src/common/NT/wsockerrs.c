/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/***************************************************************************\
*  wsockerrs.cpp - Translate winsock error numbers into text				*
*  Copyright© 1996-1997 - BuckoSoft, Inc.									*
*  Freely distributable.  No charge may be made for this or any derived		*
*  works without the express written consent of BuckoSoft, Inc.				*
*																			*
*																			*
*  				*
\***************************************************************************/
#include <winsock.h>

struct Wsockerrs {
    int error;
    char *text;
} Wsockerrs;
struct Wsockerrs wsockerrs[] = {
    WSAEINTR, "WSAEINTR",
    WSAEBADF, "WSAEBADF",
    WSAEACCES, "WSAEACCES",
    WSAEFAULT, "WSAEFAULT",
    WSAEINVAL, "WSAEINVAL",
    WSAEMFILE, "WSAEMFILE",

/*
 * Windows Sockets definitions of regular Berkeley error constants
 */
    WSAEWOULDBLOCK, "WSAEWOULDBLOCK",
    WSAEINPROGRESS, "WSAEINPROGRESS",
    WSAEALREADY, "WSAEALREADY",
    WSAENOTSOCK, "WSAENOTSOCK",
    WSAEDESTADDRREQ, "WSAEDESTADDRREQ",
    WSAEMSGSIZE, "WSAEMSGSIZE",
    WSAEPROTOTYPE, "WSAEPROTOTYPE",
    WSAENOPROTOOPT, "WSAENOPROTOOPT",
    WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT",
    WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT",
    WSAEOPNOTSUPP, "WSAEOPNOTSUPP",
    WSAEPFNOSUPPORT, "WSAEPFNOSUPPORT",
    WSAEAFNOSUPPORT, "WSAEAFNOSUPPORT",
    WSAEADDRINUSE, "WSAEADDRINUSE",
    WSAEADDRNOTAVAIL, "WSAEADDRNOTAVAIL",
    WSAENETDOWN, "WSAENETDOWN",
    WSAENETUNREACH, "WSAENETUNREACH",
    WSAENETRESET, "WSAENETRESET",
    WSAECONNABORTED, "WSAECONNABORTED",
    WSAECONNRESET, "WSAECONNRESET",
    WSAENOBUFS, "WSAENOBUFS",
    WSAEISCONN, "WSAEISCONN",
    WSAENOTCONN, "WSAENOTCONN",
    WSAESHUTDOWN, "WSAESHUTDOWN",
    WSAETOOMANYREFS, "WSAETOOMANYREFS",
    WSAETIMEDOUT, "WSAETIMEDOUT",
    WSAECONNREFUSED, "WSAECONNREFUSED",
    WSAELOOP, "WSAELOOP",
    WSAENAMETOOLONG, "WSAENAMETOOLONG",
    WSAEHOSTDOWN, "WSAEHOSTDOWN",
    WSAEHOSTUNREACH, "WSAEHOSTUNREACH",
    WSAENOTEMPTY, "WSAENOTEMPTY",
    WSAEPROCLIM, "WSAEPROCLIM",
    WSAEUSERS, "WSAEUSERS",
    WSAEDQUOT, "WSAEDQUOT",
    WSAESTALE, "WSAESTALE",
    WSAEREMOTE, "WSAEREMOTE",

    WSAEDISCON, "WSAEDISCON",

/*
 * Extended Windows Sockets error constant definitions
 */
    WSASYSNOTREADY, "WSASYSNOTREADY",
    WSAVERNOTSUPPORTED, "WSAVERNOTSUPPORTED",
    WSANOTINITIALISED, "WSANOTINITIALISED",

    -1, "UNKNOWN"
};

const char *GetWSockErrText(int error)
{
    int i;
    for (i = 0; wsockerrs[i].error != -1; i++)
	if (wsockerrs[i].error == error)
	    break;
    return (wsockerrs[i].text);
}
