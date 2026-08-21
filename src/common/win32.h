/*
 * XPilot NG, a multiplayer space war game.
 *
 * Windows declarations shared by the MinGW server and SDL client.
 */

#ifndef XPILOT_WIN32_H
#define XPILOT_WIN32_H

#ifdef _WINDOWS
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef FD_SETSIZE
#  define FD_SETSIZE 256
# endif
# include <winsock2.h>
# include <ws2tcpip.h>

# ifndef MAXHOSTNAMELEN
#  define MAXHOSTNAMELEN 64
# endif
#endif

#endif
