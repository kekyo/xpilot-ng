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

#ifndef CONNECTPARAM_H
#define CONNECTPARAM_H


#ifndef PACK_H
/* need MAX_..._LEN */
#  include "pack.h"
#endif

typedef struct Connect_param {
    int			contact_port,
			server_port,
			login_port;
    char		nick_name[MAX_NAME_LEN],
			user_name[MAX_NAME_LEN],
			host_name[SOCK_HOSTNAME_LENGTH],
			server_addr[MAX_HOST_LEN],
			server_name[MAX_HOST_LEN],
			disp_name[MAX_DISP_LEN];
    unsigned		server_version;
    int			team;
} Connect_param_t;

#endif

