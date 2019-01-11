/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2002-2004 by
 *
 *      Kimiko Koopman       <kimiko@xpilot.org
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#include "xpserver.h"

int tagItPlayerId = NO_ID;	/* player who is 'it' */

void Transfer_tag(player_t *oldtag_pl, player_t *newtag_pl)
{
    char msg[MSG_LEN];

    if (tagItPlayerId != oldtag_pl->id
 	|| oldtag_pl->id == newtag_pl->id)
 	return;

    tagItPlayerId = newtag_pl->id;
    sprintf(msg, " < %s killed %s and gets to be 'it' now. >",
	    newtag_pl->name, oldtag_pl->name);
    Set_message(msg);
}

static inline bool Player_can_be_tagged(player_t *pl)
{
    if (Player_is_tank(pl))
	return false;
    if (Player_is_paused(pl))
	return false;
    return true;
}

/*
 * Called from update during tag game to check that a non-paused
 * player is tagged.
 */
void Check_tag(void)
{
    int num = 0, i, candidate;
    player_t *tag_pl = Player_by_id(tagItPlayerId);

    if (tag_pl && Player_can_be_tagged(tag_pl))
	return;

    /* Find number of players that might get the tag */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl = Player_by_index(i);
	if (Player_can_be_tagged(pl))
	    num++;
    }

    if (num == 0) {
	tagItPlayerId = NO_ID;
	return;
    }

    /* select first candidate for tag */
    candidate = (int)(rfrac() * num);
    for (i = candidate; i < NumPlayers; i++) {
	player_t *pl = Player_by_index(i);
	if (Player_can_be_tagged(pl)) {
	    tagItPlayerId = pl->id;
	    break;
	}
    }

    if (tagItPlayerId == NO_ID) {
	for (i = 0; i < candidate; i++) {
	    player_t *pl = Player_by_index(i);
	    if (Player_can_be_tagged(pl)) {
		tagItPlayerId = pl->id;
		break;
	    }
	}
    }

    /* someone should be tagged by now */
    assert(tagItPlayerId != NO_ID);
}
