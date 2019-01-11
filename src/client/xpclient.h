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

#ifndef XPCLIENT_H
#define XPCLIENT_H

#include "xpcommon.h"

#include "keys.h"

#ifdef HAVE_LIBZ
#  include <zlib.h>
#else
#  error "Header zlib.h missing. Please install zlib."
#endif

#ifdef _WINDOWS
#ifdef _MSC_VER
# include <direct.h>
# define snprintf _snprintf
# define printf Trace
# define X_OK 0
#endif
# define F_OK 0
# define W_OK 2
# define R_OK 4
# define mkdir(A,B) _mkdir(A)
extern bool threadedDraw; /* default.c */
#endif

/*
 * Client header files that are "generic", that is common to all client
 * implementations.
 */
#include "client.h"
#include "clientcommand.h"
#include "clientrank.h"
#include "configure.h"
#include "connectparam.h"
#include "datagram.h"
#include "gfx2d.h"
#include "guimap.h"
#include "guiobjects.h"
#include "meta.h"
#include "netclient.h"
#include "option.h"
#include "paint.h"
#include "recordfmt.h"
#include "talk.h"
#ifdef SOUND
# include "caudio.h"
#endif

#endif /* XPCLIENT_H */

