/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
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

#include "xpclient_sdl.h"

#include <SDL3/SDL_main.h>

#include "sdlinit.h"
#include "sdlmeta.h"
#include "transport_display.h"

static void Main_shutdown(void)
{
    Net_cleanup();
    Client_cleanup();
}

static void sigcatch(int signum)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    Main_shutdown();
    error("got signal %d\n", signum);
    exit(1);

}

const char *Program_name(void)
{
    return "xpilot-infinity-sdl";
}

static void Main_report_connection_failure(
    int target_count, const Connect_target_t *targets, bool show_dialog)
{
    const Connect_target_t *last_target = &targets[target_count - 1];
    const char *contact_transport =
	Transport_display_name(last_target->contact_transport);
    const char *gameplay_transport =
	Transport_display_name(last_target->game_transport);
    const char *transport_advice;
    char message[1024];
    int written;

    if (last_target->contact_transport == GAME_TRANSPORT_TCP
	&& last_target->game_transport == GAME_TRANSPORT_TCP) {
	transport_advice =
	    "\nFor TCP on both transports, start the server with -tcp "
	    "or -transport tcp.";
    } else if (last_target->contact_transport == GAME_TRANSPORT_WEBSOCKET
	       && last_target->game_transport == GAME_TRANSPORT_WEBSOCKET) {
	transport_advice =
	    "\nFor WebSocket on both transports, start the server with "
	    "-websocket or -transport websocket.";
    } else {
	transport_advice =
	    "\nConfigure the server contact and gameplay transports to match.";
    }

    if (target_count == 1) {
	written = snprintf(
	    message, sizeof(message),
	    "Could not contact %s:%d.\n\n"
	    "Contact/Lobby: %s\n"
	    "Gameplay: %s\n\n"
	    "Verify that the server is running and uses matching transport "
	    "settings.%s",
	    last_target->address, last_target->contact_port,
	    contact_transport, gameplay_transport, transport_advice);
    } else {
	written = snprintf(
	    message, sizeof(message),
	    "Could not contact any of %d server targets.\n\n"
	    "Last target: %s:%d\n"
	    "Contact/Lobby: %s\n"
	    "Gameplay: %s\n\n"
	    "Verify that the servers are running and use matching transport "
	    "settings.%s",
	    target_count, last_target->address, last_target->contact_port,
	    contact_transport, gameplay_transport, transport_advice);
    }
    if (written < 0 || (size_t)written >= sizeof(message)) {
	strlcpy(message,
		"Could not contact the requested server targets.",
		sizeof(message));
    }

    fprintf(stderr, "%s: ERROR: Connection failed:\n%s\n",
	    Program_name(), message);
    fflush(stderr);
    if (show_dialog
	&& !SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
				     "XPilot Infinity - Connection failed",
				     message, NULL)) {
	fprintf(stderr, "%s: ERROR: Could not show the connection failure "
		"dialog: %s\n", Program_name(), SDL_GetError());
	fflush(stderr);
    }
}

int main(int argc, char *argv[])
{
    bool auto_shutdown = false;
    Connect_target_t *targets = NULL;
    Connect_target_status_t target_status;
    int target_count;
    int invalid_target_index;
    int result;

    init_error(argv[0]);

    seedMT((unsigned)time(NULL) ^ Get_process_id());

    memset(&connectParam, 0, sizeof(Connect_param_t));
    connectParam.team = TEAM_NOT_SET;

    Store_default_options();
    Store_talk_macro_options();
    Store_key_options();
    Store_sdlinit_options();
    Store_sdlgui_options();
    Store_radar_options();

    memset(&xpArgs, 0, sizeof(xp_args_t));
    Parse_options(&argc, argv);
    target_count = argc - 1;
    target_status = Connect_targets_parse(target_count, &argv[1],
					  &connectDefaults, &targets,
					  &invalid_target_index);
    if (target_status != CONNECT_TARGET_STATUS_OK) {
	if (invalid_target_index >= 0) {
	    error("Invalid server target '%s': %s",
		  argv[invalid_target_index + 1],
		  Connect_target_status_message(target_status));
	} else {
	    error("Cannot prepare server targets: %s",
		  Connect_target_status_message(target_status));
	}
	return 1;
    }
    if (xpArgs.list_servers)
	xpArgs.auto_connect = true;

    /* CLIENTRANK */
    Init_saved_scores();

    if (sock_startup()) {
	error("failed to initialize networking");
	exit(1);
    }

    if (xpArgs.text || xpArgs.auto_connect || argv[1]) {
	int connected;

	if (target_count > 0) {
	    Contact_servers_result_t contact_result =
		Contact_servers_detailed(
		    target_count, targets,
		    xpArgs.auto_connect, xpArgs.list_servers,
		    auto_shutdown, xpArgs.shutdown_reason, &connectParam);

	    connected = contact_result.connected;
	    if (!contact_result.contacted) {
		Main_report_connection_failure(
		    target_count, targets,
		    !xpArgs.text && !xpArgs.list_servers && !auto_shutdown);
		free(targets);
		return 1;
	    }
	} else {
	    connected = Contact_local_servers(
		&connectDefaults, xpArgs.auto_connect, xpArgs.list_servers,
		auto_shutdown, xpArgs.shutdown_reason,
		0, NULL, NULL, NULL, NULL, &connectParam);
	}

	free(targets);
	targets = NULL;
	if (!connected)
	    return 0;
	if (Init_window()) {
	    error("Could not initialize SDL, check your settings.");
	    exit(1);
	}
    } else {
	if (Init_window()) {
	    error("Could not initialize SDL, check your settings.");
	    exit(1);
	}
	while (1) {
	    result = Meta_window(&connectParam);
	    if (result < 0) return 0;
	    if (result == 0) break;
	}
    }

    /* If something goes wrong before Client_setup I'll leave the
     * cleanup to the OS because afaik Client_cleanup will clean
     * stuff initialized in Client_setup. */

    if (Client_init(connectParam.server_name, connectParam.server_version)) {
	error("failed to initialize client"); 
	exit(1);
    }

    if (Net_init(connectParam.server_addr, connectParam.login_port,
		 connectParam.game_transport)) {
	error("failed to initialize networking"); 
	exit(1);
    }
    if (Net_verify(connectParam.user_name, 
		   connectParam.nick_name, 
		   connectParam.disp_name)) {
	error("failed to verify networking"); 
	exit(1);
    }
    if (Net_setup()) {
	error("failed to setup networking"); 
	exit(1);
    }
    if (Client_setup()) {
	error("failed to setup client"); 
	exit(1);
    }

    signal(SIGINT, sigcatch);
    signal(SIGTERM, sigcatch);

    if (Net_start()) {
	Main_shutdown();
	error("failed to start networking"); 
	exit(1);
    }
    if (Client_start()) {
	Main_shutdown();
	error("failed to start client"); 
	exit(1);
    }
    Game_loop();
    Main_shutdown();
    return 0;
}
