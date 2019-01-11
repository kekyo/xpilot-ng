/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2004 by
 *
 *      Uoti Urpala          <uau@users.sourceforge.net>
 *      Erik Andersson       <maximan@users.sourceforge.net>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#include "xpserver.h"


/*
 * Look if any player's name is exactly 'str',
 * If not, look if any player's name contains 'str'.
 * The matching is case insensitive. If there is an
 * error (no matches or several matches) NULL is returned
 * and the error code is stored in 'error' if that is not NULL
 * and a string describing the error is stored in
 * 'errorstr_p' if that is not NULL.
 */
player_t *Get_player_by_name(const char *str,
			     int *error_p, const char **errorstr_p)
{
    int i, id;
    player_t *found_pl = NULL, *pl;
    size_t len;

    if (str == NULL || (len = strlen(str)) == 0)
	goto match_none;

    /* Get player by id */
    id = atoi(str);
    if (id > 0) {
	found_pl = Player_by_id(id);
	if (!found_pl)
	    goto match_none;
	return found_pl;
    }

    /* Look for an exact match on player nickname. */
    for (i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);
	if (!strcasecmp(pl->name, str))
	    return pl;
    }

    /* Look if 'str' matches beginning of only one nick. */
    for (i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);

	if (!strncasecmp(pl->name, str, len)) {
	    if (found_pl)
		goto match_several;
	    found_pl = pl;
	    continue;
	}
    }
    if (found_pl)
	return found_pl;

    /*
     * Check what players' name 'str' is a substring of (case insensitively).
     */
    for (i = 0; i < NumPlayers; i++) {
	int j;

	pl = Player_by_index(i);

	for (j = 0; j < 1 + (int)strlen(pl->name) - (int)len; j++) {
	    if (!strncasecmp(pl->name + j, str, len)) {
		if (found_pl)
		    goto match_several;
		found_pl = pl;
		break;
	    }
	}
    }
    if (found_pl)
	return found_pl;

 match_none:
    if (error_p != NULL)
	*error_p = -1;
    if (errorstr_p != NULL)
	*errorstr_p = "Name does not match any player.";
    return NULL;

 match_several:
    if (error_p != NULL)
	*error_p = -2;
    if (errorstr_p != NULL)
	*errorstr_p = "Name matches several players.";
    return NULL;
}


void Send_info_about_player(player_t *pl)
{
    int i;

    for (i = 0; i < spectatorStart + NumSpectators; i++) {
	player_t *pl_i;

	if (i == NumPlayers) {
	    i = spectatorStart - 1;
	    continue;
	}
	pl_i = Player_by_index(i);
	if (pl_i->conn != NULL) {
	    Send_team(pl_i->conn, pl->id, pl->team);
	    /*if we do either, we do both... but is either necessary?*/
	    updateScores = true;
	    pl->update_score = true;
	    if (pl->home_base != NULL)
		Send_base(pl_i->conn, pl->id, pl->home_base->ind);
	}
    }
}


void Set_swapper_state(player_t *pl)
{
    if (BIT(pl->have, HAS_BALL))
	Detach_ball(pl, NULL);

    if (BIT(world->rules->mode, LIMITED_LIVES)) {
	int i;

	for (i = 0; i < NumPlayers; i++) {
	    player_t *pl_i = Player_by_index(i);

	    if (!Players_are_teammates(pl, pl_i) &&
		!Player_is_paused(pl_i)) {
		/* put team swapping player waiting mode. */
		/* kps - there used to be a if (pl->mychar == ' ') here,
		 * not sure what that was good for.
		 */
		Player_set_state(pl, PL_STATE_WAITING);
		Player_self_destruct(pl, false);
		pl->pause_count = 0;
		pl->recovery_count = 0;
		break;
	    }
	}
    }
}


#define CMD_RESULT_SUCCESS		0
#define CMD_RESULT_ERROR		(-1)
#define CMD_RESULT_NOT_OPERATOR		(-2)
#define CMD_RESULT_NO_NAME		(-3)


static int Cmd_addr(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_advance(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_ally(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_get(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_help(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_kick(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_lock(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_maxturnsps(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_mute(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_op(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_password(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_pause(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_plinfo(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_queue(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_reset(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_set(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_shutdown(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_stats(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_team(char *arg, player_t *pl, bool oper, char *msg, size_t size);
static int Cmd_version(char *arg, player_t *pl, bool oper, char *msg, size_t size);


typedef struct {
    const char *name;
    const char *abbrev;
    const char *help;
    bool oper_only;
    int (*cmd)(char *arg, player_t *pl, bool oper, char *msg, size_t size);
} Command_info;


/*
 * A list of all the commands sorted alphabetically.
 */
static Command_info commands[] = {
    {
	"addr",
	"addr",
	"/addr <player name or ID number>. Show IP-address of player.  "
	"(operator)",
	true,
	Cmd_addr
    },
    {
	"advance",
	"ad",
	"/advance <name of player in the queue>. "
	"Move the player to the front of the queue.  (operator)",
	true,
	Cmd_advance
    },
    {
	"ally",
	"al",
	"/ally {invite|cancel|refuse|accept|leave|list} [<player name>]. "
	"Manages alliances and invitations for them.",
	false,
	Cmd_ally
    },
    {
	"get",
	"g",
	"/get <option>.  Gets a server option.",
	false,
	Cmd_get
    },
    {
	"help",
	"h",
	"Print command list.  /help <command> gives more info.",
	false,
	Cmd_help
    },
    {
	"kick",
	"k",
	"/kick <player name or ID number>.  Remove a player from game.  "
	"(operator)",
	true,
	Cmd_kick
    },
    {
	"lock",
	"l",
	"Just /lock tells lock status.  /lock 1 locks, /lock 0 unlocks.  "
	"(operator)",
	false,      /* checked in the function */
	Cmd_lock
    },
    {
	"maxturnsps",
	"maxturns",
	"/maxturnsps <number> set max amount of turns per second.  "
	"(EXPERIMENTAL FEATURE)",
	false,
	Cmd_maxturnsps
    },
    {
	"mute",
	"m",
	"Just /mute 1 mutes, /mute 0 unmutes paused players WITHOUT BASE /mute <name> toggles muting of player.  "
	"(operator)",
	false,      /* checked in the function */
	Cmd_mute
    },
    {
	"op",
	"o",
	"/op <command> [player name or ID number]. Operator commands.  "
	"(operator)",
	true,
	Cmd_op
    },
    {
	"password",
	"pas",
	"/password <string>.  If string matches -password option "
	"gives operator status.",
	false,
	Cmd_password
    },
    {
	"pause",
	"pau",
	"/pause <player name or ID number>.  Pauses player.  (operator)",
	true,
	Cmd_pause
    },
    {
	"plinfo",
	"pl",
	"/plinfo <player name or ID number>.  Show misc. player info.",
	false,
	Cmd_plinfo
    },
    {
	"queue",
	"q",
	"/queue.  Show the names of players waiting to enter.",
	false,
	Cmd_queue
    },
    {
	"reset",
	"r",
	"Just /reset re-starts the round. "
	"'/reset all' also resets all scores to 0.  (operator)",
	true,
	Cmd_reset
    },
    {
	"set",
	"s",
	"/set <option> <value>.  Sets a server option.  (operator)",
	true,
	Cmd_set
    },
    {
	"shutdown",
	"shutd",
	"/shutdown <delay in seconds> [reason].  Shutdown server. "
	"Use delay <= 0 to cancel. (operator)  "
	"Just /shutdown to query.",
	false,      /* checked in the function */
	Cmd_shutdown
    },
    {
	"stats",
	"st",
	"/stats <player name or ID number>.  Show player ranking info.",
	false,
	Cmd_stats
    },
    {
	"team",
	"t",
	"/team <team number> [name] swaps you to given team. "
	"Can be used with full teams too.",
	false,
	Cmd_team
    },
    {
	"version",
	"v",
	"Print server version.",
	false,
	Cmd_version
    },
};


/*
 * cmd parameter has no leading slash.
 */
void Handle_player_command(player_t *pl, char *cmd)
{
    int i, result;
    char *args, msg[MSG_LEN];

    if (!*cmd) {
	strlcpy(msg,
		"No command given.  Type /help for help.  [*Server reply*]",
		sizeof(msg));
	Set_player_message(pl, msg);
	return;
    }

    args = strchr(cmd + 1, ' ');
    if (!args)
	/* point to end of string. */
	args = cmd + strlen(cmd);
    else {
	/* zero terminate cmd and advance 1 byte. */
	*args++ = '\0';
	while (isspace(*args))
	    args++;
    }

    for (i = 0; i < NELEM(commands); i++) {
	size_t len1 = strlen(commands[i].abbrev);
	size_t len2 = strlen(cmd);

	if (!strncasecmp(cmd, commands[i].name, MAX(len1, len2)))
	    break;
    }

    if (i == NELEM(commands)) {
	snprintf(msg, sizeof(msg),
		 "Unknown command '%s'.  [*Server reply*]", cmd);
	Set_player_message(pl, msg);
	return;
    }
#if 0 /* kps - recording related stuff too obscure */
    else if (!pl->isoperator && (commands[i].operOnly || rplayback && !playback && commands[i].number != PASSWORD_CMD)) {
	i = NO_CMD;
	sprintf(msg, "You need operator status to use this command.");
    }
#endif

    msg[0] = '\0';
    result = (*commands[i].cmd)(args, pl, pl->isoperator, msg, sizeof(msg));
    switch (result) {
    case CMD_RESULT_SUCCESS:
	break;

    case CMD_RESULT_ERROR:
	if (msg[0] == '\0')
	    strlcpy(msg, "Error.", sizeof(msg));
	break;

    case CMD_RESULT_NOT_OPERATOR:
	if (msg[0] == '\0')
	    strlcpy(msg,
		    "You need operator status to use this command.",
		    sizeof(msg));
	break;

    case CMD_RESULT_NO_NAME:
	if (msg[0] == '\0')
	    strlcpy(msg,
		    "You must give a player name as an argument.",
		    sizeof(msg));
	break;

    default:
	strlcpy(msg, "Bug.", sizeof(msg));
	break;
    }

    if (msg[0]) {
	strlcat(msg, " [*Server reply*]", sizeof(msg));
	Set_player_message(pl, msg);
    }
}



static int Cmd_addr(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    player_t *pl2 = NULL;
    const char *errorstr;

    UNUSED_PARAM(pl);

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    pl2 = Get_player_by_name(arg, NULL, &errorstr);
    if (pl2) {
	const char *addr = Player_get_addr(pl2);

	if (addr == NULL)
	    snprintf(msg, size, "Unable to get address for %s.", pl2->name);
	else
	    snprintf(msg, size, "%s plays from: %s.", pl2->name, addr);
    } else {
	strlcpy(msg, errorstr, size);
	return CMD_RESULT_ERROR;
    }

    return CMD_RESULT_SUCCESS;
}


/*
 * The queue system from the original server is not replicated
 * during playback. Therefore interactions with it in the
 * recording can cause problems (at least different message
 * lengths in acks from client). It would be possible to work
 * around this, but not implemented now. Currently queue and advance
 * commands are disabled during recording.
 */
static int Cmd_advance(char *arg, player_t *pl, bool oper,
		       char *msg, size_t size)
{
    int result;

    UNUSED_PARAM(pl);

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (record || playback) {
	strlcpy(msg, "Command currently disabled during recording for "
	       "technical reasons.", size);
	return CMD_RESULT_ERROR;
    }

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    result = Queue_advance_player(arg, msg, size);

    if (result < 0)
	return CMD_RESULT_ERROR;

    return CMD_RESULT_SUCCESS;
}


static int Cmd_ally(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    char *command;
    int result = CMD_RESULT_SUCCESS;
    static const char usage[] =
	"Usage: /ally {invite|cancel|refuse|accept|leave|list} "
	"[<player name>]";
    static const char *cmds[] = {
	"invite",
	"cancel",
	"refuse",
	"accept",
	"leave",
	"list",
    };
    enum AllyCmds {
	AllyInvite  = 0,
	AllyCancel  = 1,
	AllyRefuse  = 2,
	AllyAccept  = 3,
	AllyLeave   = 4,
	AllyList    = 5,
	NumAllyCmds = 6
    };
    int			i, cmd;

    UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (!BIT(world->rules->mode, ALLIANCES)) {
	strlcpy(msg, "Alliances are not allowed.", size);
	result = CMD_RESULT_ERROR;
    }
    else if (!arg || !(command = strtok(arg, " \t"))) {
	strlcpy(msg, usage, size);
	result = CMD_RESULT_ERROR;
    }
    else {
	if ((arg = strtok(NULL, "")) != NULL) {
	    while (*arg == ' ')
		++arg;
	}
	cmd = -1;
	for (i = 0; i < NumAllyCmds; i++) {
	    if (!strncasecmp(cmds[i], command, strlen(command)))
		cmd = (cmd == -1) ? i : (-2);
	}
	if (cmd < 0) {
	    strlcpy(msg, usage, size);
	    result = CMD_RESULT_ERROR;
	}
	else if (arg) {
	    /* a name is specified */
	    const char *errorstr;
	    player_t *pl2 = Get_player_by_name(arg, NULL, &errorstr);

	    if (pl2) {
		if (cmd == AllyInvite)
		    Invite_player(pl, pl2);
		else if (cmd == AllyRefuse)
		    Refuse_alliance(pl, pl2);
		else if (cmd == AllyAccept)
		    Accept_alliance(pl, pl2);
		else {
		    strlcpy(msg, usage, size);
		    result = CMD_RESULT_ERROR;
		}
	    } else {
		strlcpy(msg, errorstr, size);
		result = CMD_RESULT_ERROR;
	    }
	} else {
	    /* no player name is specified */
	    if (cmd == AllyCancel)
		Cancel_invitation(pl);
	    else if (cmd == AllyRefuse)
		Refuse_all_alliances(pl);
	    else if (cmd == AllyAccept)
		Accept_all_alliances(pl);
	    else if (cmd == AllyLeave)
		Leave_alliance(pl);
	    else if (cmd == AllyList)
		Alliance_player_list(pl);
	    else {
		strlcpy(msg, usage, size);
		result = CMD_RESULT_ERROR;
	    }
	}
    }
    return result;
}

static int Cmd_get(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    char value[MAX_CHARS];
    char *valcpy, *name;
    int i, retval = CMD_RESULT_ERROR;

    UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (!arg || !*arg) {
	strlcpy(msg, "Usage: /get option.", size);
	return CMD_RESULT_ERROR;
    }

    valcpy = xp_safe_strdup(arg);
    name = strtok(valcpy, " \t\r\n");
    i = Get_option_value(name, value, sizeof(value));

    switch (i) {
    case 1:
	snprintf(msg, size, "The value of %s is %s.", name, value);
	retval = CMD_RESULT_SUCCESS;
	break;
    case -2:
	snprintf(msg, size, "No server option named %s.", name);
	break;
    case -3:
	snprintf(msg, size, "Cannot show the value of this option.");
	break;
    case -4:
	snprintf(msg, size, "No value has been set for option %s.", name);
	break;
    default:
	strlcpy(msg, "Generic error.", size);
	break;
    }

    XFREE(valcpy);

    return retval;
}


static int Cmd_help(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int			i;

    UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (!*arg) {
	strlcpy(msg, "Commands: ", size);
	for (i = 0; i < NELEM(commands); i++) {
	    if (!commands[i].oper_only) {
		strlcat(msg, commands[i].name, size);
		strlcat(msg, " ", size);
	    }
	}

	if (pl->isoperator) {
	    strlcat(msg, " [*Server reply*]", size);
	    Set_player_message(pl, msg);

	    strlcpy(msg, "Operator commands: ", size);
	    for (i = 0; i < NELEM(commands); i++) {
		if (commands[i].oper_only) {
		    strlcat(msg, commands[i].name, size);
		    strlcat(msg, " ", size);
		}
	    }
	}
    }
    else {
	for (i = 0; i < NELEM(commands); i++) {
	    size_t len1 = strlen(commands[i].abbrev);
	    size_t len2 = strlen(arg);

	    if (!strncasecmp(arg, commands[i].name, MAX(len1, len2)))
		break;
	}
	if (i == NELEM(commands))
	    snprintf(msg, size, "No help for nonexistent command '%s'.", arg);
	else
	    strlcpy(msg, commands[i].help, size);
    }

    return CMD_RESULT_SUCCESS;
}


static int Cmd_kick(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    player_t *kicked_pl;
    const char *errorstr;

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    kicked_pl = Get_player_by_name(arg, NULL, &errorstr);
    if (kicked_pl) {
	snprintf(msg, size, "%s kicked %s out! [*Server notice*]",
		 pl->name, kicked_pl->name);
	if (kicked_pl->conn == NULL)
	    Delete_player(kicked_pl);
	else
	    Destroy_connection(kicked_pl->conn, "kicked out");
	Set_message(msg);
	strlcpy(msg, "", size);
	return CMD_RESULT_SUCCESS;
    }
    strlcpy(msg, errorstr, size);

    return CMD_RESULT_ERROR;
}


static int Cmd_lock(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    bool new_lock;

    if (!arg || !*arg) {
	snprintf(msg, size, "The game is currently %s.",
		 game_lock ? "locked" : "unlocked");
	return CMD_RESULT_SUCCESS;
    }

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!strcmp(arg, "1"))
	new_lock = true;
    else if (!strcmp(arg, "0"))
	new_lock = false;
    else {
	snprintf(msg, size, "Invalid argument '%s'.  Specify either 0 or 1.",
		 arg);
	return CMD_RESULT_ERROR;
    }

    if (new_lock == game_lock)
	snprintf(msg, size, "Game is already %s.",
		 game_lock ? "locked" : "unlocked");
    else {
	game_lock = new_lock;
	snprintf(msg, size, " < The game has been %s by %s! >",
		 game_lock ? "locked" : "unlocked",
		 pl->name);
	Set_message(msg);
	strlcpy(msg, "", size);
    }

    return CMD_RESULT_SUCCESS;
}

/* temporary hack */
static int Cmd_maxturnsps(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int new_maxturnsps;

    UNUSED_PARAM(oper);

    if (!arg || !*arg) {
	snprintf(msg, size, "Your current maxturnsps is %d.", pl->maxturnsps);
	return CMD_RESULT_SUCCESS;
    }

    new_maxturnsps = atoi(arg);
    if (new_maxturnsps <= 0) {
	snprintf(msg, size, "Value of maxturnsps must be > 0.");
	return CMD_RESULT_ERROR;
    }

    pl->maxturnsps = new_maxturnsps;
    Set_player_message_f(pl, "Max number of turns/s is now %d. "
			 "[*Server reply*]", pl->maxturnsps);

    return CMD_RESULT_SUCCESS;
}

static int Cmd_mute(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int new_mute;
    player_t *mutee;
    const char *errorstr;

    if (!arg || !*arg) {
	snprintf(msg, size, "Baseless paused players are currently %s.",
		 mute_baseless ? "muted" : "unmuted");
	return CMD_RESULT_SUCCESS;
    }

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!strcmp(arg, "1"))
	new_mute = true;
    else if (!strcmp(arg, "0"))
	new_mute = false;
    else if ((mutee = Get_player_by_name(arg, NULL, &errorstr)) != NULL) {
    	mutee->muted = mutee->muted ? false : true;
	snprintf(msg, size, "Player %s is now %s.",
		 mutee->name, mutee->muted ? "muted" : "unmuted");
    	return CMD_RESULT_SUCCESS;
    } else {
    	strlcpy(msg, errorstr, size);
	return CMD_RESULT_ERROR;
    }

    if (new_mute == mute_baseless)
	snprintf(msg, size, "Already %s.",
		 mute_baseless ? "muted" : "unmuted");
    else {
	mute_baseless = new_mute;
	snprintf(msg, size, " < Baseless paused players have been %s by %s! >",
		 mute_baseless ? "muted" : "unmuted", pl->name);
	Set_message(msg);
	strlcpy(msg, "", size);
    }

    return CMD_RESULT_SUCCESS;
}

/* kps - this one is a bit obscure, maybe clean it up a bit ? */
static int Cmd_op(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    player_t *issuer = pl;
    char *origarg = arg;
    char *name;
    int cmd, priv;

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!arg || (*arg != '+' && *arg != '-')) {
	snprintf(msg, size, "Usage: /op {+|-}[nlo]+ <player name>");
	return CMD_RESULT_ERROR;
    }

    name = strpbrk(arg, " \t");
    if (name) {
	const char *errorstr;

	*name++ = '\0';
	while (isspace(*name))
	    name++;

	pl = Get_player_by_name(name, NULL, &errorstr);
	if (!pl) {
	    strlcpy(msg, errorstr, size);
	    return CMD_RESULT_ERROR;
	}
    }

    priv = 0;
    cmd = *arg;
    arg++;
    while (*arg) {
	switch (*arg) {
	case 'n':
	    priv |= PRIV_NOAUTOKICK;
	    break;
	case 'l':
	    priv |= PRIV_AUTOKICKLAST;
	    break;
	case 'o':
	    if (cmd == '+')
		pl->isoperator = true;
	    else
		pl->isoperator = false;
	    break;
	default:
	    snprintf(msg, size, "Invalid operator command '%c'.", *arg);
	    return CMD_RESULT_ERROR;
	}
	arg++;
    }
    if (cmd == '+')
	pl->privs |= priv;
    else
	pl->privs &= ~priv;

    if (pl != issuer) {
	snprintf(msg, size, "%s executed '/op %s' on you. [*Server notice*]",
		 issuer->name, origarg);
	Set_player_message(pl, msg);
    }
    snprintf(msg, size, "Executed '/op %s' on %s", origarg, pl->name);

    return CMD_RESULT_SUCCESS;
}


static int Cmd_password(char *arg, player_t *pl, bool oper,
			 char *msg, size_t size)
{
    UNUSED_PARAM(oper);

    if (!options.password || !arg || strcmp(arg, options.password)) {
	strlcpy(msg, "Wrong.", size);
	if (pl->isoperator && pl->rectype != 2) {
	    NumOperators--;
	    pl->isoperator = false;
	    strlcat(msg, "  You lost operator status.", size);
	}
    }
    else {
	if (!pl->isoperator && pl->rectype != 2) {
	    NumOperators++;
	    pl->isoperator = true;
	    pl->privs |= PRIV_AUTOKICKLAST;
	}
	strlcpy(msg, "You got operator status.", size);
    }
    return CMD_RESULT_SUCCESS;
}


static int Cmd_pause(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    const char *errorstr;
    player_t *pl2;

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    pl2 = Get_player_by_name(arg, NULL, &errorstr);
    if (!pl2) {
	strlcpy(msg, errorstr, size);
	return CMD_RESULT_ERROR;
    }

    if (pl2->conn != NULL) {
	if (Player_is_alive(pl2))
	    Kill_player(pl2, false);
	Pause_player(pl2, true);
	snprintf(msg, size, "%s was paused by %s. [*Server notice*]",
		 pl2->name, pl->name);
	Set_message(msg);
	strlcpy(msg, "", size);
    } else {
	snprintf(msg, size, "Robots and tanks can't be paused.");
	return CMD_RESULT_ERROR;
    }

    return CMD_RESULT_SUCCESS;
}

static int Cmd_plinfo(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    const char *errorstr;
    player_t *pl2;

    UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    pl2 = Get_player_by_name(arg, NULL, &errorstr);
    if (!pl2) {
	strlcpy(msg, errorstr, size);
	return CMD_RESULT_ERROR;
    }

    if (!Player_is_human(pl2)) {
	snprintf(msg, size, "Robots and tanks don't have player info.");
	return CMD_RESULT_ERROR;
    }

    snprintf(msg, size,
	     "%-15s Ver: 0x%x MaxFPS: %d Turnspeed: %.2f Turnres: %.2f "
	     "RTT: %i ms RTT_dev: %i ms",
	     pl2->name, pl2->version,
	     pl2->player_fps, pl2->turnspeed, pl2->turnresistance,
	     (int)((pl2->conn->rtt_smoothed >> 3) * timePerFrame * 1000),
	     (int)((pl2->conn->rtt_dev >> 2) * timePerFrame * 1000));

    return CMD_RESULT_SUCCESS;
}

static int Cmd_queue(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int result;

    UNUSED_PARAM(arg); UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (record || playback) {
	strlcpy(msg, "Command currently disabled during recording for "
		"technical reasons.", size);
	return CMD_RESULT_ERROR;
    }

    result = Queue_show_list(msg, size);

    if (result < 0)
	return CMD_RESULT_ERROR;

    return CMD_RESULT_SUCCESS;
}


static int Cmd_reset(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int i;

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

        for (i = NumObjs - 1; i >= 0; i--) {
        object_t *obj = Obj[i];
        obj->life = 0;
        Delete_shot(i);

    }

    if (arg && !strcasecmp(arg, "all")) {
	for (i = NumPlayers - 1; i >= 0; i--) {
	    player_t *pl_i = Player_by_index(i);

	    if (!Player_is_paused(pl_i))
		Rank_set_score(pl_i, 0.0);
	}
	Reset_all_players();
	if (options.gameDuration == -1)
	    options.gameDuration = 0;
	roundsPlayed = 0;

	snprintf(msg, size, " < Total reset by %s! >", pl->name);
	Set_message(msg);
	strlcpy(msg, "", size);

	teamcup_game_over();
	teamcup_game_start();
    }
    else {
	Reset_all_players();
	if (options.gameDuration == -1)
	    options.gameDuration = 0;

	snprintf(msg, size, " < Round reset by %s! >", pl->name);
	Set_message(msg);
	strlcpy(msg, "", size);
    }

    return CMD_RESULT_SUCCESS;
}


static int Cmd_stats(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    UNUSED_PARAM(pl); UNUSED_PARAM(oper);

    if (!arg || !*arg)
	return CMD_RESULT_NO_NAME;

    if (!Rank_get_stats(arg, msg, size)) {
	snprintf(msg, size, "Player \"%s\" doesn't have ranking stats.", arg);
	return CMD_RESULT_ERROR;
    }

    return CMD_RESULT_SUCCESS;
}


static int Cmd_team(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int i, team, swap_allowed;
    char *arg2;

    UNUSED_PARAM(oper);

    /*
     * Assume nothing will be said or done.
     */
    msg[0] = '\0';
    swap_allowed = false;
    team = pl->team;

    if (!BIT(world->rules->mode, TEAM_PLAY))
	snprintf(msg, size, "No team play going on.");
    else if (pl->team >= MAX_TEAMS)
	snprintf(msg, size, "You do not currently have a team.");
    else if (!arg)
	snprintf(msg, size, "No team specified.");
    else if (!isdigit(*arg))
	snprintf(msg, size, "Invalid team specification.");
    else {
	team = strtoul(arg, &arg2, 0);
	if (arg2 && *arg2) {
	    const char *errorstr;

	    if (!pl->isoperator) {
		snprintf(msg, size,
			"You need operator status to swap other players.");
		return CMD_RESULT_NOT_OPERATOR;
	    }
	    while (isspace(*arg2))
		arg2++;
	    pl = Get_player_by_name(arg2, NULL, &errorstr);
	    if (!pl) {
		strlcpy(msg, errorstr, size);
		return CMD_RESULT_ERROR;
	    }
	}

	for (i = 0; i < MAX_TEAMS ; i++) {
	    team_t *t = Team_by_index(i);

	    /* Can't queue to two teams at once. */
	    if (t->SwapperId == pl->id)
		t->SwapperId = NO_ID;
	}

	if (game_lock && pl->home_base == NULL)
	    snprintf(msg, size, "Playing teams are locked.");
	else if (team < 0 || team >= MAX_TEAMS)
	    snprintf(msg, size, "Team %d is not a valid team.", team);
	else if (team == pl->team && pl->home_base != NULL)
	    snprintf(msg, size, "You already are on team %d.", team);
	else if (world->teams[team].NumBases == 0)
	    snprintf(msg, size,
		     "There are no bases for team %d on this map.", team);
	else if (options.reserveRobotTeam && team == options.robotTeam)
	    snprintf(msg, size,
		     "You cannot join the robot team on this server.");
	else if (pl->rectype == 2)
	    snprintf(msg, size, "Spectators cannot change team.");
	else
	    swap_allowed = true;
    }
    if (!swap_allowed)
	return CMD_RESULT_ERROR;

    if (world->teams[team].NumBases > world->teams[team].NumMembers) {
	snprintf(msg, size, "%s has swapped to team %d.", pl->name, team);
	Set_message(msg);
	if (pl->home_base)
	    world->teams[pl->team].NumMembers--;
	pl->team = team;
	world->teams[pl->team].NumMembers++;
	Set_swapper_state(pl);
	if (pl->home_base == NULL) {
	    Pick_startpos(pl);
	    Pause_player(pl, false);
	}
	else
	    Pick_startpos(pl);
	Send_info_about_player(pl);
	strlcpy(msg, "", size);

	return CMD_RESULT_SUCCESS;
    }

    i = world->teams[pl->team].SwapperId;
    while (i != -1 && pl->home_base != NULL) {
	if ((i = Player_by_id(i)->team) != team)
	    i = world->teams[i].SwapperId;
	else {
	    /* Found a cycle, now change the teams */
	    base_t *xbase = pl->home_base, *xbase2;
	    int xteam = pl->team, xteam2;
	    player_t *pl2 = pl;

	    do {
		pl2 = Player_by_id(world->teams[xteam].SwapperId);
		world->teams[xteam].SwapperId = -1;
		xbase2 = pl2->home_base;
		xteam2 = pl2->team;
		pl2->team = xteam;
		pl2->home_base = xbase;
		Set_swapper_state(pl2);
		Send_info_about_player(pl2);
		/* This can send a huge amount of data if several
		 * players swap. Unfortunately all player data, even
		 * shipshape, has to be resent to change the team of
		 * a player. This should probably be changed somehow
		 * to prevent disturbing other players. */
		xbase = xbase2;
		xteam = xteam2;
	    } while (xteam != team);
	    xteam = pl->team;
	    pl->team = team;
	    pl->home_base = xbase;
	    Set_swapper_state(pl);
	    Send_info_about_player(pl);
	    snprintf(msg, size, "Some players swapped teams.");
	    Set_message(msg);
	    strlcpy(msg, "", size);
	    return CMD_RESULT_SUCCESS;
	}
    }
    /* Swap a paused player away from the full team */
    for (i = NumPlayers - 1; i >= 0; i--) {
	player_t *pl2 = Player_by_index(i);

	if (pl2->conn != NULL && Player_is_paused(pl2)
	    && (pl2->team == team) && pl2->home_base != NULL) {
	    base_t *temp;

	    pl2->team = pl->team;
	    pl->team = team;
	    temp = pl2->home_base;
	    pl2->home_base = pl->home_base;
	    pl->home_base = temp;
	    Set_swapper_state(pl2);
	    Set_swapper_state(pl);
	    Send_info_about_player(pl2);
	    Send_info_about_player(pl);
	    snprintf(msg, size, "%s has swapped with paused %s.",
		     pl->name, pl2->name);
	    Set_message(msg);
	    strlcpy(msg, "", size);
	    return CMD_RESULT_SUCCESS;
	}
    }
    snprintf(msg, size, "You are queued for swap to team %d.", team);
    world->teams[team].SwapperId = pl->id;
    return CMD_RESULT_SUCCESS;
}

static int Cmd_set(char *arg, player_t *pl, bool oper, char *msg, size_t size)
{
    int i;
    char *option, *value;

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    /*
     * Second argument of second strtok is " " instead of ""
     * which allows setting string options to values that contain spaces.
     */
    if (!arg
	|| !(option = strtok(arg, " "))
	|| !(value = strtok(NULL, ""))) {

	snprintf(msg, size, "Usage: /set option value.");
	return CMD_RESULT_ERROR;
    }

    i = Tune_option(option, value);
    if (i == 1) {
	if (!strcasecmp(option, "password"))
	    snprintf(msg, size, "Operation successful.");
	else {
	    char val[MAX_CHARS];

	    Get_option_value(option, val, sizeof(val));
	    snprintf(msg, size, " < Option %s set to %s by %s. >",
		     option, val, pl->name);
	    Set_message(msg);
	    strlcpy(msg, "", size);

	    return CMD_RESULT_SUCCESS;
	}
    }
    else if (i == 0)
	snprintf(msg, size, "Invalid value.");
    else if (i == -1)
	snprintf(msg, size, "This option cannot be changed at runtime.");
    else if (i == -2)
	snprintf(msg, size, "No option named '%s'.", option);
    else
	snprintf(msg, size, "Error.");

    return CMD_RESULT_ERROR;
}

static int Cmd_shutdown(char *arg, player_t *pl, bool oper,
			char *msg, size_t size)
{
    int delay;
    const char *delaystr, *reason;
    bool is_shutting_down = (ShutdownServer == -1 ? false : true);

    if (!arg || !*arg) {
	if (is_shutting_down)
	    snprintf(msg, size, "Shutting down in %d seconds: \"%s\"",
		     ShutdownServer / FPS, ShutdownReason);
	else
	    strlcpy(msg, "The server is not shutting down.", size);
	return CMD_RESULT_SUCCESS;
    }

    if (!oper)
	return CMD_RESULT_NOT_OPERATOR;

    if (!arg
	|| !(delaystr = strtok(arg, " "))) {

	snprintf(msg, size, "Usage: /shutdown <delay> [reason].");
	return CMD_RESULT_ERROR;
    }

    reason = strtok(NULL, "");
    if (reason == NULL)
	reason = "";

    delay = atoi(delaystr);

    if (is_shutting_down || delay > 0) {
	Server_shutdown(pl->name, delay, reason);
	return CMD_RESULT_SUCCESS;
    }
    else
	/* no need to cancel if not shutting down */
	snprintf(msg, size, "The server is not shutting down.");

    return CMD_RESULT_ERROR;
}

static int Cmd_version(char *arg, player_t *pl, bool oper,
		       char *msg, size_t size)
{
    UNUSED_PARAM(arg); UNUSED_PARAM(pl); UNUSED_PARAM(oper);
    snprintf(msg, size, "%s version %s.", PACKAGE_NAME, VERSION);
    return CMD_RESULT_SUCCESS;
}
