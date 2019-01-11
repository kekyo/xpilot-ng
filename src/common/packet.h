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

#ifndef PACKET_H
#define PACKET_H

/* before version 3.8.0 this was 8 bytes. */
#define KEYBOARD_SIZE		9

/*
 * Definition of various client/server packet types.
 */

/* packet types: 0 - 9 */
#define PKT_UNDEFINED		0
#define PKT_VERIFY		1
#define PKT_REPLY		2
#define PKT_PLAY		3
#define PKT_QUIT		4
#define PKT_MESSAGE		5
#define PKT_START		6
#define PKT_END			7
#define PKT_SELF		8
#define PKT_DAMAGED		9

/* packet types: 10 - 19 */
#define PKT_CONNECTOR		10
#define PKT_REFUEL		11
#define PKT_SHIP		12
#define PKT_ECM			13
#define PKT_PAUSED		14
#define PKT_ITEM		15
#define PKT_MINE		16
#define PKT_BALL		17
#define PKT_MISSILE		18
#define PKT_SHUTDOWN		19

/* packet types: 20 - 29 */
#define PKT_STRING		20
#define PKT_DESTRUCT		21
#define PKT_RADAR		22
#define PKT_TARGET		23
#define PKT_KEYBOARD		24
#define PKT_SEEK		25
#define PKT_SELF_ITEMS		26	/* still under development */
#define PKT_TEAM_SCORE		27	/* was PKT_SEND_BUFSIZE */
#define PKT_PLAYER		28
#define PKT_SCORE		29

/* packet types: 30 - 39 */
#define PKT_FUEL		30
#define PKT_BASE		31
#define PKT_CANNON		32
#define PKT_LEAVE		33
#define PKT_POWER		34
#define PKT_POWER_S		35
#define PKT_TURNSPEED		36
#define PKT_TURNSPEED_S		37
#define PKT_TURNRESISTANCE	38
#define PKT_TURNRESISTANCE_S	39

/* packet types: 40 - 49 */
#define PKT_WAR			40
#define PKT_MAGIC		41
#define PKT_RELIABLE		42
#define PKT_ACK			43
#define PKT_FASTRADAR		44
#define PKT_TRANS		45
#define PKT_ACK_CANNON		46
#define PKT_ACK_FUEL		47
#define PKT_ACK_TARGET		48
#define	PKT_SCORE_OBJECT	49

/* packet types: 50 - 59 */
#define PKT_AUDIO		50
#define PKT_TALK		51
#define PKT_TALK_ACK		52
#define PKT_TIME_LEFT		53
#define PKT_LASER		54
#define PKT_DISPLAY		55
#define PKT_EYES		56
#define PKT_SHAPE		57
#define PKT_MOTD		58
#define PKT_LOSEITEM		59

/* packet types: 60 - 69 */
#define PKT_APPEARING		60
#define PKT_TEAM		61
#define PKT_POLYSTYLE		62
#define PKT_ACK_POLYSTYLE	63
#define PKT_NOT_USED_64		64
#define PKT_NOT_USED_65		65
#define PKT_NOT_USED_66		66
#define PKT_NOT_USED_67		67
#define PKT_MODIFIERS		68
#define PKT_FASTSHOT		69	/* replaces SHOT/TEAMSHOT */

/* packet types: 70 - 79 */
#define PKT_THRUSTTIME		70
#define PKT_MODIFIERBANK	71
#define PKT_SHIELDTIME		72
#define PKT_POINTER_MOVE	73
#define PKT_REQUEST_AUDIO	74
#define PKT_ASYNC_FPS		75
#define PKT_TIMING		76
#define PKT_PHASINGTIME		77
#define PKT_ROUNDDELAY		78
#define PKT_WRECKAGE		79

/* packet types: 80 - 89 */
#define PKT_ASTEROID		80
#define PKT_WORMHOLE		81
#define PKT_NOT_USED_82		82
#define PKT_NOT_USED_83		83
#define PKT_NOT_USED_84		84
#define PKT_NOT_USED_85		85
#define PKT_NOT_USED_86		86
#define PKT_NOT_USED_87		87
#define PKT_NOT_USED_88		88
#define PKT_NOT_USED_89		89

/* packet types: 90 - 99 */
/*
 * Use these 10 packet type numbers for
 * experimenting with new packet types.
 */

/* status reports: 101 - 102 */
#define PKT_FAILURE		101
#define PKT_SUCCESS		102

/* optimized packet types: 128 - 255 */
#define PKT_DEBRIS		128		/* + color + x + y */

#endif
