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

#include "xpclient_x11.h"

char			**Argv;
int			Argc;

static void printfile(const char *filename)
{
    FILE		*fp;
    int			c;

    if ((fp = fopen(filename, "r")) == NULL)
	return;

    while ((c = fgetc(fp)) != EOF)
	putchar(c);

    fclose(fp);
}

const char *Program_name(void)
{
    return "xpilot-ng-x11";
}

/*
 * Oh glorious main(), without thee we cannot exist.
 */
int main(int argc, char *argv[])
{
    int result, retval = 1;
    bool auto_shutdown = false;
    Connect_param_t *conpar = &connectParam;

    /*
     * --- Output copyright notice ---
     */
    printf("  " COPYRIGHT ".\n"
	   "  " TITLE " comes with ABSOLUTELY NO WARRANTY; "
	      "for details see the\n"
	   "  provided COPYING file.\n\n");
    if (strcmp(Conf_localguru(), PACKAGE_BUGREPORT))
	printf("  %s is responsible for the local installation.\n\n",
	       Conf_localguru());

    Conf_print();

    Argc = argc;
    Argv = argv;

    /*
     * --- Miscellaneous initialization ---
     */
    init_error(argv[0]);

    seedMT( (unsigned)time(NULL) ^ Get_process_id());

    memset(conpar, 0, sizeof(Connect_param_t));

    /*
     * --- Create global option array ---
     */
    Store_default_options();
    Store_X_options();
    Store_hud_options();
    Store_paintradar_options();
    Store_xpaint_options();
    Store_guimap_options();
    Store_guiobject_options();
    Store_talk_macro_options();
    Store_key_options();
    Store_record_options();
    Store_color_options();

    /*
     * --- Check commandline arguments and resource files ---
     */
    memset(&xpArgs, 0, sizeof(xp_args_t));
    Parse_options(&argc, argv);
    /*strcpy(clientname,connectParam.nick_name); */

    Config_init();
    IFNWINDOWS(Handle_X_options();)
    
    /* CLIENTRANK */
    Init_saved_scores();

    if (xpArgs.list_servers)
	xpArgs.auto_connect = true;

    if (xpArgs.shutdown_reason[0] != '\0') {
	auto_shutdown = true;
	xpArgs.auto_connect = true;
    }

    /*
     * --- Message of the Day ---
     */
    printfile(Conf_localmotdfile());

    if (xpArgs.text || xpArgs.auto_connect || argv[1] || is_this_windows()) {
	if (xpArgs.list_servers)
	    printf("LISTING AVAILABLE SERVERS:\n");

	result = Contact_servers(argc - 1, &argv[1],
				 xpArgs.auto_connect, xpArgs.list_servers,
				 auto_shutdown, xpArgs.shutdown_reason,
				 0, NULL, NULL, NULL, NULL,
				 conpar);
    }
    else {
	IFNWINDOWS(result = Welcome_screen(conpar));
    }

    if (result == 1)
	retval = Join(conpar);
    
    if (instruments.clientRanker)
	Print_saved_scores();

    return retval;
}
