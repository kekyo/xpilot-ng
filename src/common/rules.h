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

#ifndef RULES_H
#define RULES_H

/*
 * Bitfield definitions for playing mode.
 */
#define CRASH_WITH_PLAYER	(1<<0)
#define BOUNCE_WITH_PLAYER	(1<<1)
#define PLAYER_KILLINGS		(1<<2)
#define LIMITED_LIVES		(1<<3)
#define TIMING			(1<<4)
#define PLAYER_SHIELDING	(1<<6)
#define LIMITED_VISIBILITY	(1<<7)
#define TEAM_PLAY		(1<<8)
#define WRAP_PLAY		(1<<9)
#define ALLOW_NUKES		(1<<10)
#define ALLOW_CLUSTERS		(1<<11)
#define ALLOW_MODIFIERS		(1<<12)
#define ALLOW_LASER_MODIFIERS	(1<<13)
#define ALLIANCES		(1<<14)

/*
 * Client uses only a subset of them:
 */
#define CLIENT_RULES_MASK	(WRAP_PLAY|TEAM_PLAY|TIMING|LIMITED_LIVES|\
				 ALLIANCES)
/*
 * Old player status bits, currently only used in network protocol.
 * The bits that the client needs must fit into a byte,
 * so the first 8 bitvalues are reserved for that purpose.
 */
#define OLD_PLAYING		(1U<<0)		/* alive or killed */
#define OLD_PAUSE		(1U<<1) 	/* paused */
#define OLD_GAME_OVER		(1U<<2)		/* waiting or dead */

typedef struct {
    int		lives;
    long	mode;
} rules_t;

#endif
