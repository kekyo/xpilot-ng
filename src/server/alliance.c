/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2003-2004 by
 *
 *      Kristian Söderblom   <kps@users.sourceforge.net>
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *      Kimiko Koopman       <kimiko@xpilot.org>
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

/*
 * Alliance information.
 */
typedef struct {
    int		id;		/* the ID of this alliance */
    int		NumMembers;	/* the number of members in this alliance */
} alliance_t;


static alliance_t	*Alliances[MAX_TEAMS];


static int New_alliance_ID(void);
static void Alliance_add_player(alliance_t *alliance, player_t *pl);
static int Alliance_remove_player(alliance_t *alliance, player_t *pl);
static void Set_alliance_message(alliance_t *alliance, const char *msg);
static int Create_alliance(player_t *pl1, player_t *pl2);
static void Dissolve_alliance(int id);
static void Merge_alliances(player_t *pl, int id2);


int Invite_player(player_t *pl, player_t *ally)
{
    if (pl->id == ally->id) {
	/* we can never form an alliance with ourselves */
	return 0;
    }
    if (Player_is_tank(ally)) {
	/* tanks can't handle invitations */
	return 0;
    }
    if (Players_are_allies(pl, ally)) {
	/* we're already in the same alliance */
	return 0;
    }
    if (pl->invite == ally->id) {
	/* player has already been invited by us */
	return 0;
    }
    if (ally->invite == pl->id) {
	/* player has already invited us. accept invitation */
	Accept_alliance(pl, ally);
	return 1;
    }
    if (pl->invite != NO_ID) {
	/* we have already invited another player. cancel that invitation */
	Cancel_invitation(pl);
    }
    /* set & send invitation */
    pl->invite = ally->id;
    if (Player_is_robot(ally)) {
	Robot_invite(ally, pl);
    }
    else if (Player_is_human(ally)) {
	char msg[MSG_LEN];
	sprintf(msg, " < %s seeks an alliance with you >", pl->name);
	Set_player_message(ally, msg);
    }
    return 1;
}

int Cancel_invitation(player_t *pl)
{
    player_t *ally;

    if (pl->invite == NO_ID) {
	/* we have not invited anyone */
	return 0;
    }
    ally = Player_by_id(pl->invite);
    pl->invite = NO_ID;
    if (Player_is_human(ally)) {
	char msg[MSG_LEN];
	sprintf(msg, " < %s has cancelled the invitation for an alliance >",
		pl->name);
	Set_player_message(ally, msg);
    }
    return 1;
}

/* refuses invitation from a specific player */
int Refuse_alliance(player_t *pl, player_t *ally)
{
    if (ally->invite != pl->id) {
	/* we were not invited anyway */
	return 0;
    }
    ally->invite = NO_ID;
    if (Player_is_human(ally)) {
	char msg[MSG_LEN];
	sprintf(msg, " < %s has declined your invitation for an alliance >",
		pl->name);
	Set_player_message(ally, msg);
    }
    return 1;
}

/* refuses invitations from any player */
int Refuse_all_alliances(player_t *pl)
{
    int		i, j = 0;

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->invite == pl->id) {
	    Refuse_alliance(pl, pl2);
	    j++;
	}
    }
    if (Player_is_human(pl)) {
	char msg[MSG_LEN];
	if (j == 0) {
	    sprintf(msg, " < You were not invited for any alliance >");
	} else {
	    sprintf(msg, " < %d invitation%s for %s declined >", j,
		    (j > 1 ? "s" : ""), (j > 1 ? "alliances" : "an alliance"));
	}
	Set_player_message(pl, msg);
    }
    return j;
}

/* accepts an invitation from a specific player */
int Accept_alliance(player_t *pl, player_t *ally)
{
    int		success = 1;

    if (ally->invite != pl->id) {
	/* we were not invited */
	return 0;
    }
    ally->invite = NO_ID;
    if (ally->alliance != ALLIANCE_NOT_SET) {
	if (pl->alliance != ALLIANCE_NOT_SET) {
	    /* both players are in alliances */
	    Merge_alliances(ally, pl->alliance);
	} else {
	    /* inviting player is in an alliance */
	    Player_join_alliance(pl, ally);
	}
    } else {
	if (pl->alliance != ALLIANCE_NOT_SET) {
	    /* accepting player is in an alliance */
	    Player_join_alliance(ally, pl);
	} else {
	    /* neither player is in an alliance */
	    success = Create_alliance(pl, ally);
	}
    }
    return success;
}

/* accepts invitations from any player */
int Accept_all_alliances(player_t *pl)
{
    int		i, j = 0;

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->invite == pl->id) {
	    Accept_alliance(pl, pl2);
	    j++;
	}
    }
    if (Player_is_human(pl)) {
	char msg[MSG_LEN];
	if (j == 0) {
	    sprintf(msg, " < You were not invited for any alliance >");
	} else {
	    sprintf(msg, " < %d invitation%s for %s accepted >", j,
		    (j > 0 ? "s" : ""), (j > 0 ? "alliances" : "an alliance"));
	}
	Set_player_message(pl, msg);
    }
    return j;
}

/* returns a pointer to the alliance with a given ID */
static alliance_t *Find_alliance(int id)
{
    int i;

    if (id != ALLIANCE_NOT_SET) {
	for (i = 0; i < NumAlliances; i++) {
	    if (Alliances[i]->id == id) {
		return Alliances[i];
	    }
	}
    }

    return NULL;
}

/*
 * Return the number of members in a particular alliance.
 */
int Get_alliance_member_count(int id)
{
    alliance_t	*alliance = Find_alliance(id);

    if (alliance != NULL)
	return alliance->NumMembers;

    return 0;
}

/* sends a message to all the members of an alliance */
static void Set_alliance_message(alliance_t *alliance, const char *msg)
{
    int	i;

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (Player_is_human(pl2)) {
	    if (pl2->alliance == alliance->id)
		Set_player_message(pl2, msg);
	}
    }
}

/* returns an unused ID for an alliance */
static int New_alliance_ID(void)
{
    int i, try_id;

    for (try_id = 0; try_id < MAX_TEAMS; try_id++) {
	for (i = 0; i < NumAlliances; i++) {
	    if (Alliances[i]->id == try_id)
		break;
	}
	if (i == NumAlliances)
	    break;
    }
    if (try_id < MAX_TEAMS)
	return try_id;

    return ALLIANCE_NOT_SET;
}

/* creates an alliance between two players */
static int Create_alliance(player_t *pl1, player_t *pl2)
{
    alliance_t	*alliance = (alliance_t *)malloc(sizeof(alliance_t));
    char	msg[MSG_LEN];

    if (alliance == NULL) {
	error("Not enough memory for new alliance.\n");
	return 0;
    }

    alliance->id = New_alliance_ID();
    if (alliance->id == ALLIANCE_NOT_SET) {
	warn("Maximum number of alliances reached.\n");
	free(alliance);
	return 0;
    }
    alliance->NumMembers = 0;
    Alliances[NumAlliances] = alliance;
    NumAlliances++;
    Alliance_add_player(alliance, pl1);
    Alliance_add_player(alliance, pl2);
    /* announcement */
    if (options.announceAlliances) {
	sprintf(msg, " < %s and %s have formed alliance %d >", pl1->name,
		pl2->name, alliance->id);
	Set_message(msg);
    } else {
	sprintf(msg, " < You have formed an alliance with %s >", pl2->name);
	Set_player_message(pl1, msg);
	sprintf(msg, " < You have formed an alliance with %s >", pl1->name);
	Set_player_message(pl2, msg);
    }
    return 1;
}

/* adds a player to an existing alliance */
void Player_join_alliance(player_t *pl, player_t *ally)
{
    alliance_t	*alliance = Find_alliance(ally->alliance);
    char	msg[MSG_LEN];

    if (!Player_is_tank(pl)) {
	/* announce first to avoid sending the player two messages */
	if (options.announceAlliances) {
	    sprintf(msg, " < %s has joined alliance %d >",
		    pl->name, alliance->id);
	    Set_message(msg);
	}
	else {
	    sprintf(msg, " < %s has joined your alliance >", pl->name);
	    Set_alliance_message(alliance, msg);
	    if (Player_is_human(pl)) {
		sprintf(msg, " < You have joined %s's alliance >", ally->name);
		Set_player_message(pl, msg);
	    }
	}
    }

    Alliance_add_player(alliance, pl);
}

/* atomic addition of player to alliance */
static void Alliance_add_player(alliance_t *alliance, player_t *pl)
{
    int	i;

    /* drop invitations for this player from other members */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->invite == pl->id)
	    Cancel_invitation(pl2);
    }
    Player_set_alliance(pl,alliance->id);
    alliance->NumMembers++;
    updateScores = true;
}

/* removes a player from an alliance and dissolves the alliance if necessary */
int Leave_alliance(player_t *pl)
{
    alliance_t	*alliance;
    char	msg[MSG_LEN];

    if (pl->alliance == ALLIANCE_NOT_SET) {
	/* we're not in any alliance */
	return 0;
    }
    alliance = Find_alliance(pl->alliance);
    Alliance_remove_player(alliance, pl);
    /* announcement */
    if (!Player_is_tank(pl)) {
	if (options.announceAlliances) {
	    sprintf(msg, " < %s has left alliance %d >", pl->name,
		    alliance->id);
	    Set_message(msg);
	} else {
	    sprintf(msg, " < %s has left your alliance >", pl->name);
	    Set_alliance_message(alliance, msg);
	    if (Player_is_human(pl)) {
		Set_player_message(pl, " < You have left the alliance >");
	    }
	}
    }
    if (alliance->NumMembers <= 1) {
	Dissolve_alliance(alliance->id);
    }
    return 1;
}

/* atomic removal of player from alliance */
static int Alliance_remove_player(alliance_t *alliance, player_t *pl)
{
    if (pl->alliance == alliance->id) {
	Player_set_alliance(pl,ALLIANCE_NOT_SET);
	alliance->NumMembers--;
	updateScores = true;
	return 1;
    }
    return 0;
}

static void Dissolve_alliance(int id)
{
    alliance_t	*alliance = Find_alliance(id);
    int		i;

    /* remove all remaining members from the alliance */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->alliance == id) {
	    Alliance_remove_player(alliance, pl2);
	    if (!options.announceAlliances && Player_is_human(pl2))
		Set_player_message(pl2,
				   " < Your alliance has been dissolved >");
	}
    }
    /* check */
    if (alliance->NumMembers != 0) {
	warn("Dissolve_alliance after dissolve %d remain!",
	     alliance->NumMembers);
    }

    /* find the index of the alliance to be removed */
    for (i = 0; i < NumAlliances; i++) {
	if (Alliances[i]->id == alliance->id) {
	    break;
	}
    }
    /* move the last alliance to that index */
    Alliances[i] = Alliances[NumAlliances - 1];
    /* announcement */
    if (options.announceAlliances) {
	char msg[MSG_LEN];
	sprintf(msg, " < Alliance %d has been dissolved >", alliance->id);
	Set_message(msg);
    }
    /* and clean up that alliance */
    free(alliance);
    NumAlliances--;
}

/*
 * Destroy all alliances.
 */
void Dissolve_all_alliances(void)
{
    int		i;

    for (i = NumAlliances - 1; i >= 0; i--) {
	Dissolve_alliance(Alliances[i]->id);
    }
}

/* merges two alliances by moving the members of the second to the first */
static void Merge_alliances(player_t *pl, int id2)
{
    alliance_t	*alliance2 = Find_alliance(id2);
    int		i;

    /* move each member of alliance2 to alliance1 */
    for (i = 0; i < NumPlayers; i++) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->alliance == id2) {
	    Alliance_remove_player(alliance2, pl2);
	    Player_join_alliance(pl2, pl);
	}
    }
    Dissolve_alliance(id2);
}

void Alliance_player_list(player_t *pl)
{
    int		i;
    char	msg[MSG_LEN];

    if (pl->alliance == ALLIANCE_NOT_SET) {
	Set_player_message(pl, " < You are not a member of any alliance >");
    }
    else {
	/* note: 80 is assumed to be much less than MSG_LEN */
	if (options.announceAlliances) {
	    sprintf(msg, " < Alliance %d:", pl->alliance);
	} else {
	    sprintf(msg, " < Your alliance: ");
	}
	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl2 = Player_by_index(i);

	    if (pl2->alliance == pl->alliance) {
		if (Player_is_human(pl2)) {
		    if (strlen(msg) > 80) {
			strlcat(msg, ">", sizeof(msg));
			Set_player_message(pl, msg);
			strlcpy(msg, " <            ", sizeof(msg));
		    }
		    strlcat(msg, pl2->name, sizeof(msg));
		    strlcat(msg, ", ", sizeof(msg));
		}
	    }
	}
	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl2 = Player_by_index(i);

	    if (pl2->alliance == pl->alliance) {
		if (Player_is_robot(pl2)) {
		    if (strlen(msg) > 80) {
			strlcat(msg, ">", sizeof(msg));
			Set_player_message(pl, msg);
			strlcpy(msg, " <            ", sizeof(msg));
		    }
		    strlcat(msg, pl2->name, sizeof(msg));
		    strlcat(msg, ", ", sizeof(msg));
		}
	    }
	}
	if (strlen(msg) >= 2 && !strcmp(msg + strlen(msg) - 2, ", ")) {
	    msg[strlen(msg) - 2] = '\0';
	}
	strlcat(msg, " >", sizeof(msg));
	Set_player_message(pl, msg);
    }
}
