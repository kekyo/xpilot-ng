/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
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

#include "sdlinit.h"
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
    return "xpilot-ng-sdl";
}

int main(int argc, char *argv[])
{
    const char *server_name;
    char *server_address;

    init_error(argv[0]);

    seedMT((unsigned)time(NULL) ^ Get_process_id());

    memset(&connectParam, 0, sizeof(Connect_param_t));
    connectParam.game_port = SERVER_PORT;
    connectParam.team = TEAM_NOT_SET;

    Store_default_options();
    Store_talk_macro_options();
    Store_key_options();
    Store_sdlinit_options();
    Store_sdlgui_options();
    Store_radar_options();

    memset(&xpArgs, 0, sizeof(xp_args_t));
    Parse_options(&argc, argv);

    /* CLIENTRANK */
    Init_saved_scores();

    if (sock_startup()) {
	error("failed to initialize networking");
	exit(1);
    }

    if (argc > 2) {
	error("Specify at most one server host.");
	exit(1);
    }
    server_name = argc == 2 ? argv[1] : "127.0.0.1";
    server_address = sock_get_addr_by_name(server_name);
    if (server_address == NULL) {
	error("Can't find the server '%s'.", server_name);
	exit(1);
    }
    strlcpy(connectParam.server_name, server_name,
	    sizeof(connectParam.server_name));
    strlcpy(connectParam.server_addr, server_address,
	    sizeof(connectParam.server_addr));
    if (xpArgs.status || xpArgs.shutdown_reason[0] != '\0' || xpArgs.text) {
	int status;

	if (xpArgs.status)
	    status = Control_request(connectParam.server_addr,
				     connectParam.game_port,
				     connectParam.user_name,
				     REPORT_STATUS_pack, "", stdout);
	else if (xpArgs.shutdown_reason[0] != '\0')
	    status = Control_request(connectParam.server_addr,
				     connectParam.game_port,
				     connectParam.user_name,
				     SHUTDOWN_pack,
				     xpArgs.shutdown_reason, stdout);
	else
	    status = Control_interactive(connectParam.server_addr,
					 connectParam.game_port,
					 connectParam.user_name);
	sock_cleanup();
	return status == 0 ? 0 : 1;
    }

    if (Init_window()) {
	error("Could not initialize SDL, check your settings.");
	exit(1);
    }

    /* If something goes wrong before Client_setup I'll leave the
     * cleanup to the OS because afaik Client_cleanup will clean
     * stuff initialized in Client_setup. */

    if (Net_init(connectParam.server_addr, connectParam.game_port)) {
	error("failed to initialize networking"); 
	exit(1);
    }
    if (Net_open_game_session(connectParam.user_name,
			      connectParam.nick_name,
			      connectParam.disp_name,
			      connectParam.host_name,
			      connectParam.team,
			      &connectParam.server_version)) {
	error("failed to open gameplay session");
	exit(1);
    }
    if (Client_init(connectParam.server_name, connectParam.server_version)) {
	error("failed to initialize client");
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
