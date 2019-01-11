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
 * Copyright (C) 2003-2004 Darel Cullen <darelcullen@users.sourceforge.net>
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

/* Changelog: CB Added metaserver interface improvements */
/*            CB Fixed Warnings                          */

#include "xpclient_x11.h"

/*
 * Are we in the process of quitting, or joining a game.
 */

static bool joining;

/*
 * Label Headings
 */
static char next_text[] = "Next Server Page >>>";
static char first_text[] = "<< First Server Page";
static char ping_text[] = "Ping Servers";

struct Label {
  const char *label;
  int commas;
  int yoff;
  int height;
};
    
struct Label labels[] = {
	{"Server", 0, 0, 0},
	{"IP:Port", 0, 0, 0},
	{"Version", 0, 0, 0},
	{"Users", 0, 0, 0},
	{"Map name", 0, 0, 0},
	{"Map size", 0, 0, 0},
	{"Map author", 0, 0, 0},
	{"Status", 0, 0, 0},
	{"Bases", 0, 0, 0},
	{"Teambases", 0, 0, 0},
	{"Free bases", 0, 0, 0},
	{"Queued players", 0, 0, 0},
	{"FPS", 0, 0, 0},
	{"Sound", 0, 0, 0},
	{"Timing", 0, 0, 0},
	{"Playlist", 1, 0, 0}
    };

/*
 * Some widgets.
 * form_widget is our toplevel widget.
 */
static int form_widget = NO_WIDGET;
static int subform_widget = NO_WIDGET;
static int subform_label_widget = NO_WIDGET;

/* Size hints for these widges */
static int form_x = 0;
static int form_y = 0;
static int subform_x;
static int subform_y;
static int subform_width;
static int subform_height; 
static int subform_border;

static int next_page_widget = NO_WIDGET;
static int first_page_widget = NO_WIDGET;
static int ping_servers_widget = NO_WIDGET;

/*
 * An array of structures with information to join a local server.
 */
static Connect_param_t *global_conpar;
static Connect_param_t *localnet_conpars;
static server_info_t *global_sip;



/*
 * Enum for different modes the welcome screen can be in.
 * We start out waiting for user to make a selection.
 * Then the screen can be active in any of the subfunctions.
 * When the user makes a selection again then we can call
 * certain cleanup handlers to cleanup the state from
 * the previous mode before setting up data structures
 * for to the new mode.
 */
enum Welcome_mode {
    ModeWaiting,
    ModeLocalnet,
    ModeInternet,
    ModeServer,
    ModeStatus,
    ModeHelp,
    ModeQuit
};
static enum Welcome_mode welcome_mode = ModeWaiting;
static void Welcome_set_mode(enum Welcome_mode new_welcome_mode);

/* Headings used to tabulate meta columns - if i would have coded
   this i would have had them in an indexable structure */

static const char player_header[] = "Pl";
static const char queue_header[] = "Q";
static const char bases_header[] = "Ba";
static const char team_header[] = "Tm";
static const char fps_header[] = "FPS";
static const char status_header[] = "Stat";
static const char version_header[] = "Version";
static const char map_header[] = "Map Name";
static const char server_header[] = "Server";
static const char ping_header[] = "Ping";
static const char stat_header[] = "Status";
static char err[MSG_LEN] = {0};
static char buf[MSG_LEN] = {0};
/*
 * Other prototypes.
 */
static int Welcome_process_one_event(XEvent * event,
				     Connect_param_t * conpar);
static int Welcome_show_server_list(Connect_param_t * conpar);
static void Internet_widget_cleanup(void);
static int Internet_cb(int widget, void *user_data, const char **text);



/*
 * Process only exposure events.
 */
static void Welcome_process_exposure_events(Connect_param_t * conpar)
{
    XEvent event;

    while (XCheckMaskEvent(dpy, ExposureMask, &event))
	Welcome_process_one_event(&event, conpar);
}

/*
 * Communicate a message to the user via a label widget.
 *
 * position:
 * 0 means top
 * 1 means middle
 * 2 means bottom.
 */
static int Welcome_create_label(int pos, const char *label_text)
{
    int label_x, label_y, label_width, label_height;

    Connect_param_t *conpar = (Connect_param_t *) global_conpar;

    Widget_destroy_children(subform_widget);	/*? */
    subform_label_widget = NO_WIDGET;
    (void) Widget_get_dimensions(subform_widget, 
				 &subform_width, 
				 &subform_height);

    label_width = XTextWidth(textFont, label_text, (int)strlen(label_text));
    label_width += 40;
    label_height = textFont->ascent + textFont->descent;
    label_x = (subform_width - label_width) / 2;

    switch (pos) {
    default:
    case 0:
	label_y = 10;
	label_height += 10;
	break;
    case 1:
	label_y = subform_height / 2 - textFont->ascent - 10;
	label_height += 20;
	break;
    case 2:
	label_y =
	    subform_height - 10 - textFont->ascent - textFont->descent;
	label_height += 10;
	break;
    }
    subform_label_widget =
	Widget_create_label(subform_widget,
			    label_x, label_y,
			    label_width, label_height, true, 0,
			    label_text);
    if (subform_label_widget != NO_WIDGET) {
	/* map children */
	(void) Widget_map_sub(subform_widget);
	/* wait until mapped */
	(void) XSync(dpy, False);
	/* draw widgets */
	Welcome_process_exposure_events(conpar);
    }

    return subform_label_widget;
}

/*
 * User clicked on a local server to join.
 */
static int Local_join_cb(int widget, void *user_data, const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;
    int result;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    result = Connect_to_server(1, 0, 0, NULL, conpar);
    if (result) {
	joining = true;
	/* structure copy. */
	*global_conpar = *conpar;
    }

    return 0;
}

#if 0
/*
 * User asked for status on a local server.
 */
static int Local_status_cb(int widget, void *user_data, const char **text)
{
    /* Connect_param_t          *conpar = (Connect_param_t *) user_data; */

    UNUSED_PARAM(widget); UNUSED_PARAM(user_data); UNUSED_PARAM(text);
    return 0;
}
#endif

/* 
 * Cleanup when leaving the mode ModeLocalnet.
 */
static void Localnet_cleanup(void)
{

    if (localnet_conpars) {
	free(localnet_conpars);
	localnet_conpars = NULL;
    }
}

static void Internet_widget_cleanup(void)
{

    /* unmap pings as we cannot do this in the localnet callback */
    if (ping_servers_widget)
	Widget_unmap(ping_servers_widget);
    if (first_page_widget)
	Widget_unmap(first_page_widget);
    if (next_page_widget)
	Widget_unmap(next_page_widget);

    Widget_map(form_widget);


}

/*
 * User wants us to search for servers on the local net.
 */
static int Localnet_cb(int widget, void *user_data, const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;
    int i;
    int n = 0;
    int label;
    int label_y, label_height;

    char *server_names;
    char *server_addrs;
    char *name_ptrs[MAX_LOCAL_SERVERS];
    char *addr_ptrs[MAX_LOCAL_SERVERS];
    unsigned server_versions[MAX_LOCAL_SERVERS];
    int max_width = 0;
    int button;
    int button_width;
    int button_height;
    int button_x;
    int button_y;
    
    int button3;
    int button3_width;
    int button3_height;
    int button3_x;
    int button3_y;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);

    Internet_widget_cleanup();

    Welcome_set_mode(ModeLocalnet);

    label =
	Welcome_create_label(1,
			     "Searching for XPilot servers on your local network...");
    Widget_get_dimensions(subform_widget, &subform_width, &subform_height);

    server_names = (char *) malloc(MAX_LOCAL_SERVERS * MAX_HOST_LEN);
    server_addrs = (char *) malloc(MAX_LOCAL_SERVERS * MAX_HOST_LEN);
    if (!server_names || !server_addrs) {
        error("Not enough memory\n");
	quitting = true;
	return 0;
    }
    for (i = 0; i < MAX_LOCAL_SERVERS; i++) {
	name_ptrs[i] = &server_names[i * MAX_HOST_LEN];
	addr_ptrs[i] = &server_addrs[i * MAX_HOST_LEN];
    }
    Contact_servers(0, NULL, 0, 2, 0, NULL,
		    MAX_LOCAL_SERVERS, &n,
		    addr_ptrs, name_ptrs, server_versions, conpar);
    LIMIT(n, 0, MAX_LOCAL_SERVERS);

    Widget_destroy_children(subform_widget);
    if (!n)
	Welcome_create_label(1,
			     "No servers were found on your local network.");
    else
	Welcome_create_label(0,
			     "The following local XPilot servers were found:");

    label_y = 10;
    label_height = textFont->ascent + textFont->descent;

    if (n > 0) {
	

	localnet_conpars =
	    (Connect_param_t *) malloc(n * sizeof(Connect_param_t));
	if (!localnet_conpars) {
	    error("Not enough memory\n");
	    free(server_names);
	    free(server_addrs);
	    quitting = true;
	    return 0;
	}
	for (i = 0; i < n; i++) {
	    int text_width = XTextWidth(textFont,
					name_ptrs[i],
					(int)strlen(name_ptrs[i]));
	    if (text_width > max_width)
		max_width = text_width;
	}
	for (i = 0; i < n; i++) {
	    localnet_conpars[i] = *conpar;
	    strlcpy(localnet_conpars[i].server_name, name_ptrs[i],
		    MAX_HOST_LEN);
	    strlcpy(localnet_conpars[i].server_addr, addr_ptrs[i],
		    MAX_HOST_LEN);
	    localnet_conpars[i].server_version = server_versions[i];
	    button_width = max_width + 20;
	    button_height = textFont->ascent + textFont->descent + 10;
	    button_x = 20;
	    button_y =
		label_y * 2 + label_height + i * (button_height + label_y);
	    button =
	      Widget_create_colored_label(subform_widget, button_x, button_y,
				          button_width, button_height, true, 1,
					  2, 0,
				           localnet_conpars[i].server_name);
	    

	    /* button2_x = button_x + button_width + button_x;
	     button2_y = button_y;
	     button2_width = XTextWidth(buttonFont, "Status", 6) + 40;
	     button2_height = buttonFont->ascent + buttonFont->descent + 10;
	     button2 =
	      Widget_create_activate(subform_widget,
	    			     button2_x, button2_y,
	    			     button2_width, button2_height,
	    	 		     1, "Status",
	    			     Local_status_cb,
	     			     (void *) &localnet_conpars[i]);  */

	    button3_x = button_x + button_width + button_x;
	    button3_y = button_y;
	    button3_width = XTextWidth(buttonFont, "Join game", 7) + 40;
	    button3_height = buttonFont->ascent + buttonFont->descent + 10;
	    button3 =
	      Widget_create_activate(subform_widget,
				     button3_x, button3_y,
				     button3_width, button3_height,
				     1, "Join game",
				     Local_join_cb,
				     (void *) &localnet_conpars[i]);
	}
    }
    Widget_map_sub(subform_widget);
    free(server_names);
    free(server_addrs);

    return 0;
}


/*
 * User wants to join a server.
 */
static int Internet_server_join_cb(int widget, void *user_data,
				   const char **text)
{
    server_info_t *sip = (server_info_t *) user_data;
    struct Connect_param connect_param;
    struct Connect_param *conpar = &connect_param;
    int result;
    char *server_addr_ptr = conpar->server_addr;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);

    /* structure copy */
    *conpar = *global_conpar;
    strlcpy(conpar->server_name, sip->hostname,
	    sizeof(conpar->server_name));
    strlcpy(conpar->server_addr, sip->ip_str, sizeof(conpar->server_addr));
    conpar->contact_port = sip->port;
    result = Contact_servers(1, &server_addr_ptr, 1, 0, 0, NULL,
			     0, NULL, NULL, NULL, NULL, conpar);
    if (result) {
	/* structure copy */
	*global_conpar = *conpar;
	joining = true;
    } else {
	printf("Server %s (%s) didn't respond on port %d\n",
	       conpar->server_name, conpar->server_addr,
	       conpar->contact_port);
    }

    return 0;
}


/*
 * User selected a server on the Internet page.
 *
 * The idea is to show the characteristics to the user in more detail,
 * and choose team from this page, then click join.
 * 
 */
static int Internet_server_show_cb(int widget, void *user_data,
				   const char **text)
{
    server_info_t *sip = (server_info_t *) user_data;
    int subform_width = 0;
    int subform_height = 0;
    int i;
    int label_x;
    int label_y;
    int label_x_offset;
    int label_y_offset;
    int label_width;
    int label_height;
    int label_space;
    int label_border;
    int max_label_width;
    int data_label_width;
    int player_label_width = 200;	/* fixed length */
    int host_label_width = 400;
    /* large enough for ipv6 static, as the data is rendered after the closure
       of this routine, so memory has to be persistant */
    static char ipport[MAX_HOST_LEN];
    static char *p = NULL;
    char *eqptr = NULL;
    char *playslist = xp_strdup(sip->playlist);
    static char longest_text[] = "                                 ";

    char *s;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    global_sip = sip;

    Widget_destroy_children(subform_widget);

    welcome_mode = ModeStatus;

    Widget_get_dimensions(subform_widget, &subform_width, &subform_height);

    label_y_offset = 10;
    label_x_offset = 10;
    label_x = label_x_offset;
    label_y = label_y_offset;
    label_border = 1;
    label_space = 5;
    label_height = textFont->ascent + textFont->descent + 5;

    data_label_width =
	XTextWidth(buttonFont, longest_text, (int)strlen(longest_text));

    max_label_width = 0;


    for (i = 0; i < NELEM(labels); i++) {
	label_width = XTextWidth(textFont,
				 labels[i].label,
				 (int)strlen(labels[i].label));
	max_label_width = MAX(label_width, max_label_width);

	labels[i].yoff = label_y;
	label_y += label_height + label_space;
	if (labels[i].commas) {
	    labels[i].commas = 0;
	    for (s = sip->playlist; (s = strchr(s, ',')) != NULL; s++) {
		labels[i].commas++;
		label_y += label_height + label_space;
	    }
	    /* one extra for the header */
	    label_y += label_height + label_space;
	}
	labels[i].height = (label_y - labels[i].yoff) - label_space;
    }

    label_width = max_label_width + 2 * label_space;
    
    for (i = 0; i < NELEM(labels); i++)
      {
	/* if there are no players dont print the playlist label */

	if (!strncmp(labels[i].label,"playlist",sizeof(labels[i].label)) 
	    && (sip->users == 0)) {
	  continue;	  
	}
	else
	  {
	    (void) Widget_create_colored_label(subform_widget,
					       label_x, labels[i].yoff,
					       label_width, labels[i].height, true,
					       label_border, 9, WHITE , labels[i].label);
	  }
      }


    /* if the meta string data was indexable this would be easier */

    i = 0;


    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border, BLACK, WHITE, sip->hostname);

    /* Create a join button to join this server */

    (void) Widget_create_activate(subform_widget,
			   label_x + label_width + data_label_width +
			   label_space,
			   labels[i].yoff,
			   data_label_width, labels[i].height,
			   label_border, "Join This Server",
			   Internet_server_join_cb,(void *) sip);
    i++;

    
    /* Port and IP address */

    sprintf(ipport,"%s:%d",sip->ip_str,sip->port);

    (void) Widget_create_colored_label(subform_widget,
			       label_x + label_width, labels[i].yoff,
			       data_label_width, labels[i].height, true,
			       label_border, BLACK, WHITE, ipport);
    
    /* Back to list button */
    
    (void) Widget_create_activate(subform_widget,
			   label_x + label_width + data_label_width +
			   label_space,
			   labels[i].yoff,
			   data_label_width, labels[i].height,
			   label_border, "Back to List",
			   Internet_cb, (void *) global_conpar);


    i++;

    /* Version label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE,sip->version);

    i++;

    /* Number of users label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE,sip->users ? sip->users_str : "");
    i++;

    /* Map name label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border, BLACK, WHITE, sip->mapname);
    i++;

    /* Map size label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border, BLACK, WHITE, sip->mapsize);
    i++;

    /* Map author label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border, BLACK, WHITE, sip->author);

    i++;


    /* Map status label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE,sip->status);

    i++;

    /* Number of bases label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE, sip->bases_str);
    i++;

    /* Number of teambases label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border, BLACK, WHITE, sip->teambases_str);

    i++;

    /* Freebases label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE, sip->freebases);
    i++;

    /* Number in Queue label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE, sip->queue_str);
    i++;

    /* Number of frames per second label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE, sip->fps_str);
    i++;
    /* Is there sound label */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
			label_border,  BLACK, WHITE, sip->sound);
    i++;

    /* Is this a race map label  */

    (void) Widget_create_colored_label(subform_widget,
			label_x + label_width, labels[i].yoff,
			data_label_width, labels[i].height, true,
				label_border, BLACK, WHITE,
			((strcmp(sip->timing, "0") ==
			  0) ? "Not a race" : "Race!"));
    i++;

    label_y = labels[i].yoff;

    /* Collect up the players - the game port doesnt give 
       information like score , paused state of each player so this
       could be updated later to show those things by connecting to the
       player meta port instead of the game meta port */

    /* my_strtok destroys playslist */
    /* Could introduce new resources here, but lets force some color */
    /* changes instead */
    if (sip->users != 0) {

      (void) Widget_create_colored_label(subform_widget,
					 label_x + label_width,
					 label_y,
					 player_label_width, label_height, true,
					 label_border, 9, WHITE, "Player Name");
      
      (void) Widget_create_colored_label(subform_widget,
					 label_x + label_width + player_label_width,
					 label_y,
					 host_label_width, label_height, true,
					 label_border, 9, WHITE, "Player Host");
      
      label_y += label_height + label_space;
    }
    

    for (p = my_strtok(playslist, ","); p; p = my_strtok(NULL, ",")) {

	eqptr = strchr(p, '=');
	if (eqptr != NULL) {
	    *eqptr = '\0';
	} else {
	  /* currently xpilot allows "," in nicks quit here */
	  /* until they fix their protocol problem */
	  continue;
	}
	  
	


	(void) Widget_create_colored_label(subform_widget,
			    label_x + label_width,
			    label_y,
			    player_label_width, label_height, false,
			    label_border, BLACK, WHITE, p);
	p = eqptr + 1;

	(void) Widget_create_colored_label(subform_widget,
			    label_x + label_width + player_label_width,
			    label_y,
			    host_label_width, label_height, false,
			    label_border, BLACK, WHITE, p);

	label_y += label_height + label_space;
    }

    Widget_map_sub(subform_widget);

    return 0;
}


/*
 * User pressed next page button on the Internet page.
 */
static int Internet_next_page_cb(int widget, void *user_data,
				 const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    Welcome_show_server_list(conpar);

    return 0;
}

/*
 * User pressed first page button on the Internet page.
 */
static int Internet_first_page_cb(int widget, void *user_data,
				  const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;

    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    server_it = List_begin(server_list);

    Welcome_show_server_list(conpar);

    return 0;
}

/*
 * User pressed "measure lag" button on the Internet page
 */
static int Internet_ping_cb(int widget, void *user_data, const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;
   
    
    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    
    sprintf(buf, "Pinging servers...");
	    
    Welcome_create_label(1, buf);

    Ping_servers();

    if (Welcome_sort_server_list() == -1) {
	Delete_server_list();
	Welcome_create_label(1, "Not enough memory.");
    }

    server_it = List_begin(server_list);
    Welcome_show_server_list(conpar);

    return 0;
}

/*
 * Create for each server a row on the subform_widget.
 */
static int Welcome_show_server_list(Connect_param_t * conpar)
{
    const int border = 0;
    const int extra_width = 6;
    const int extra_height = 4;
    const int space_width = 0 + 2 * border;
    const int space_height = 4 + 2 * border;
    const int max_map_length = 30;
    const int max_version_length = 11;
    const int max_server_length = 37;

    int player_width = XTextWidth(textFont, "Pl", 2)
	+ extra_width + 2 * border;
    int queue_width = XTextWidth(textFont, "99", 2)
	+ extra_width + 2 * border;
    int bases_width = XTextWidth(textFont, "Ba", 2)
	+ extra_width + 2 * border;
    int team_width = XTextWidth(textFont, "Tm", 2)
	+ extra_width + 2 * border;
    int fps_width = XTextWidth(textFont, "FPS", 3)
	+ extra_width + 2 * border;
    int status_width = XTextWidth(textFont, "Stat", 4)
	+ extra_width + 2 * border;
    int version_width = XTextWidth(textFont,
				   "4.2.0alpha7",
				   max_version_length)
	+ extra_width + 2 * border;
    int map_width = XTextWidth(textFont, "", max_map_length)
	+ extra_width + 2 * border;
    int server_width = XTextWidth(buttonFont, server_header,
				  max_server_length);
    int server_border_width = 2 * (border ? border : 1);
    int ping_width = XTextWidth(textFont, ping_header, 5);
    int stat_width = XTextWidth(textFont, "Stat", 8)
	+ extra_width + 2 * border;
    int xoff = space_width;
    int yoff = space_height;
    int text_height = textFont->ascent + textFont->descent;
    int button_height = buttonFont->ascent + buttonFont->descent;
    int label_height = MAX(text_height, button_height)
	+ extra_height + 2 * border + 4;
    int player_offset = xoff;
    int queue_offset = player_offset + player_width + space_width;
    int bases_offset = queue_offset + queue_width + space_width;
    int team_offset = bases_offset + bases_width + space_width;
    int fps_offset = team_offset + team_width + space_width;
    int status_offset = fps_offset + fps_width + space_width;
    int version_offset = status_offset + status_width + space_width;
    int map_offset = version_offset + version_width + space_width;
    int server_offset = map_offset + map_width + space_width;
    int ping_offset =
	server_offset + server_width + server_border_width + space_width;
    int stat_offset = ping_offset + ping_width + space_width;
    int w;

    int all_offset = 0;
    server_info_t *sip;
    list_iter_t start_server_it = server_it;


    int next_border, first_border, next_width, first_width;
    int pingw_width, next_height, first_height;

    Widget_get_dimensions(subform_widget, &subform_width, &subform_height);

    all_offset = player_width + queue_width + bases_width
	+ team_width + fps_width + stat_width + version_width +
	server_width + map_width + ping_width + status_width;

    Widget_destroy_children(subform_widget);


    /* Players */

    w = Widget_create_label(subform_widget,
			    player_offset, yoff,
			    player_width, label_height, true,
			    border, player_header);
    if (!w)
	return -1;


    w = Widget_create_label(subform_widget,
			    queue_offset, yoff,
			    queue_width, label_height, true,
			    border, queue_header);

    if (!w)
	return -1;

    if (all_offset < subform_width) {

	Widget_create_label(subform_widget,
			    bases_offset, yoff,
			    bases_width, label_height, true,
			    border, bases_header);

	Widget_create_label(subform_widget,
			    team_offset, yoff,
			    team_width, label_height, true,
			    border, team_header);

	Widget_create_label(subform_widget,
			    fps_offset, yoff,
			    fps_width, label_height, true,
			    border, fps_header);

	Widget_create_label(subform_widget,
			    status_offset, yoff,
			    status_width, label_height, true,
			    border, status_header);
    } else {

	version_offset = queue_offset + queue_width + space_width;
	map_offset = version_offset + version_width + space_width;
	server_offset = map_offset + map_width + space_width;
	ping_offset =
	    server_offset + server_width + server_border_width +
	    space_width;
	stat_offset = ping_offset + ping_width + space_width;
    }

    Widget_create_label(subform_widget,
			version_offset, yoff,
			version_width, label_height, true,
			border, version_header);


    Widget_create_label(subform_widget,
			map_offset, yoff,
			map_width, label_height, true, border, map_header);

    Widget_create_label(subform_widget,
			server_offset, yoff,
			server_width + server_border_width,
			label_height, true, border, server_header);

    Widget_create_label(subform_widget,
			ping_offset, yoff,
			ping_width, label_height, true,
			border, ping_header);

    Widget_create_label(subform_widget,
			stat_offset, yoff,
			stat_width, label_height, true,
			border, stat_header);

    for (; server_it != List_end(server_list); LI_FORWARD(server_it)) {
	yoff += label_height + space_height;
	if (yoff + 1 * label_height >= subform_height)
	    break;
	sip = SI_DATA(server_it);
	Widget_create_label(subform_widget,
			    player_offset, yoff,
			    player_width, label_height, true,
			    border, sip->users ? sip->users_str : "");
	Widget_create_label(subform_widget,
			    queue_offset, yoff,
			    queue_width, label_height, true,
			    border, sip->queue ? sip->queue_str : "");
	if (all_offset < subform_width) {

	    Widget_create_label(subform_widget,
				bases_offset, yoff,
				bases_width, label_height, true,
				border, sip->bases_str);
	    Widget_create_label(subform_widget,
				team_offset, yoff,
				team_width, label_height, true,
				border,
				(sip->teambases >
				 0) ? sip->teambases_str : "");
	    Widget_create_label(subform_widget, fps_offset, yoff,
				fps_width, label_height, true,
				border, sip->fps_str);
	    if (strlen(sip->status) > 4)
		sip->status[4] = '\0';

	    Widget_create_label(subform_widget,
				status_offset, yoff,
				status_width, label_height, true,
				border,
				strcmp(sip->status,
				       "ok") ? sip->status : "");

	}

	if (strlen(sip->version) > max_version_length)
	    sip->version[max_version_length] = '\0';

	string_to_lower(sip->version);
	Widget_create_label(subform_widget,
			    version_offset, yoff,
			    version_width, label_height, true,
			    border, sip->version);
	Widget_create_label(subform_widget,
			    map_offset, yoff,
			    map_width, label_height, true,
			    border, sip->mapname);
	Widget_create_activate(subform_widget,
			       server_offset,
			       yoff - (border == 0),
			       server_width, label_height,
			       border ? border : 1, sip->hostname,
			       Internet_server_join_cb, (void *) sip);
	sprintf(sip->pingtime_str, "%4d", sip->pingtime);
	Widget_create_label(subform_widget,
			    ping_offset, yoff,
			    ping_width, label_height, true,
			    border, (sip->pingtime == PING_NORESP)
			    ? "None" : ((sip->pingtime == PING_SLOW)
					? "Slow"
					: ((sip->pingtime == PING_UNKNOWN)
					   ? "" : sip->pingtime_str)));

	Widget_create_activate(subform_widget,
			       stat_offset, yoff - (border == 0),
			       stat_width, label_height,
			       border ? border : 1, stat_header,
			       Internet_server_show_cb, (void *) sip);
    }

    next_border = border ? border : 1;
    first_border = next_border;
    next_width = XTextWidth(buttonFont, next_text, (int)strlen(next_text))
	+ extra_width + 2 * next_border;
    first_width = XTextWidth(buttonFont, first_text, (int)strlen(first_text))
	+ extra_width + 2 * first_border;
    pingw_width = XTextWidth(buttonFont, ping_text, (int)strlen(ping_text))
	+ extra_width + 2 * first_border;

    next_height = label_height + 2 * (next_border - border);
    first_height = next_height;


    if (!next_page_widget)
	next_page_widget =
	    Widget_create_activate(form_widget,
				   (int) (2 * (top_width / 4) +
					  next_width),
				   top_height - (next_height) -
				   space_height, next_width, next_height,
				   next_border, next_text,
				   Internet_next_page_cb, (void *) conpar);
    if (!first_page_widget)
	first_page_widget =
	    Widget_create_activate(form_widget,
				   (int) (2 * (top_width / 4)),
				   top_height - (first_height) -
				   space_height, first_width, first_height,
				   first_border, first_text,
				   Internet_first_page_cb,
				   (void *) conpar);
    if (!ping_servers_widget)
	ping_servers_widget =
	    Widget_create_activate(form_widget,
				   next_height,
				   top_height - (next_height) -
				   space_height, pingw_width, first_height,
				   first_border, ping_text,
				   Internet_ping_cb, (void *) conpar);


    if (server_it != List_end(server_list)) {
	Widget_map(next_page_widget);
	Widget_unmap(first_page_widget);
    } else if (start_server_it != List_begin(server_list)) {
	Widget_unmap(next_page_widget);
	Widget_map(first_page_widget);
    }

    Widget_map(ping_servers_widget);
    Widget_map_sub(subform_widget);

    return -1;
}

/* 
 * Cleanup when leaving the mode ModeLocalnet.
 */
static void Internet_cleanup(void)
{
    Delete_server_list();
}

/*
 * User pressed the Internet button.
 */
static int Internet_cb(int widget, void *user_data, const char **text)
{
    Connect_param_t *conpar = (Connect_param_t *) user_data;
    

    UNUSED_PARAM(widget); UNUSED_PARAM(text);
    Welcome_set_mode(ModeInternet);

    if (!server_list ||
	List_size(server_list) < 10 ||
	server_list_creation_time + 5 < time(NULL)) {

	Delete_server_list();
        sprintf(buf, "Doing a DNS lookup .. ");
	Welcome_create_label(1, buf);
	if (Get_meta_data(err) <= 0) {
	  Welcome_create_label(1, err);
	    return 0;
	}

	/* Ping_servers(); */

	if (Welcome_sort_server_list() == -1) {
	    Delete_server_list();
	    Welcome_create_label(1, "Not enough memory.");
	}
    }

    server_it = List_begin(server_list);
    Welcome_show_server_list(conpar);

    return 0;
}

#if 0
/*
 * User pressed the Configure button.
 */
static int Configure_cb(int widget, void *user_data, const char **text)
{
    UNUSED_PARAM(widget); UNUSED_PARAM(text);

    Config(true, CONFIG_DEFAULT);

    return 0;
}
#endif

/*
 * User pressed the Server button.
 */
#if 0
static int Server_cb(int widget, void *user_data, const char **text)
{

    Welcome_set_mode(ModeServer);

    return 0;
}
#endif

/*
 * User pressed the Help button.
 */
#if 0
static int Help_cb(int widget, void *user_data, const char **text)
{
    Welcome_set_mode(ModeHelp);

    /* Proper help about this welcome screen should be displayed.
     * For now just popup the about window.
     * Hmm, about buttons don't work.  They should become widgets.
     */
    About_callback(0, 0, 0);

    return 0;
}
#endif

/*
 * User pressed the Quit button.
 */
static int Quit_cb(int widget, void *user_data, const char **text)
{
    UNUSED_PARAM(widget); UNUSED_PARAM(user_data); UNUSED_PARAM(text);
    Welcome_set_mode(ModeQuit);

    quitting = true;

    return 0;
}



/*
 * Create toplevel widgets.
 */
static int Welcome_create_windows(Connect_param_t * conpar)
{
    int i;
    int form_border = 0;
    int form_width = top_width - 2 * form_border;
    int form_height = top_height - 2 * form_border;

    const int button_border = 4;
    int button_height = buttonFont->ascent
	+ buttonFont->descent + 2 * button_border;
    int max_width;
    int text_width;
    int button_x;
    int button_y;
    int button;
    int min_height_needed;
    int height_available;
    int max_height_wanted;
    int height_per_button;
    struct MyButton {
	const char *text;
	int (*callback) (int, void *, const char **);
    };
    struct MyButton my_buttons[] = {
	{"Local", Localnet_cb},
	{"Internet", Internet_cb},
	/*	{"Config", Configure_cb}, */
#if 0
/* XXX TODO add server page to select a map and start a server. */
	{"Server", Server_cb},
/* XXX TODO add help page . */
	{"Help", Help_cb},
#endif
	{"Quit", Quit_cb},
    };

    /* set the size limitations on the window */
    /* CB: I increased these as there is no reason for a limit */
    /* also the game window now inherits the new size nicely */

    LIMIT(form_width, 400, 1600);
    LIMIT(form_height, 400, 1400);
    form_widget =
	Widget_create_form(0, topWindow,
			   form_x, form_y,
			   form_width, form_height, form_border);

    if (form_widget == NO_WIDGET)
	return -1;

    Widget_set_background(form_widget, BLACK);

    max_width = 0;
    for (i = 0; i < NELEM(my_buttons); i++) {
	text_width = XTextWidth(buttonFont,
				my_buttons[i].text,
				(int)strlen(my_buttons[i].text));
	if (text_width > max_width)
	    max_width = text_width;
    }
    max_width += 20;

    min_height_needed = NELEM(my_buttons) * button_height;
    height_available = form_height;
    max_height_wanted = NELEM(my_buttons) * (button_height + 40);
    LIMIT(height_available, min_height_needed, max_height_wanted);
    height_per_button = height_available / NELEM(my_buttons);
    button_y = height_per_button - button_height;
    button_x = 20;
    for (i = 0; i < NELEM(my_buttons); i++) {
	button =
	    Widget_create_activate(form_widget,
				   button_x, button_y,
				   max_width, button_height + 20,
				   1, my_buttons[i].text,
				   my_buttons[i].callback,
				   (void *) conpar);
	if (button == NO_WIDGET)
	    return -1;
	button_y += height_per_button;
    }

    subform_x = 2 * button_x + max_width;
    subform_y = button_x;
    subform_border = 1;
    subform_width =
	form_width - subform_x - subform_y - 2 * subform_border;
    subform_height = form_height - 2 * subform_y - 2 * subform_border
	- button_height;
    subform_widget =
	Widget_create_form(form_widget, 0,
			   subform_x, subform_y,
			   subform_width, subform_height, subform_border);
    if (subform_widget == NO_WIDGET)
	return -1;
    Widget_set_background(subform_widget, BLACK);

   

    Widget_map_sub(form_widget);
    XMapSubwindows(dpy, topWindow);

    return 0;
}

/*
 * Close all windows.
 */
static void Welcome_destroy_windows(void)
{
    Widget_destroy(form_widget);
    XFlush(dpy);
    form_widget = NO_WIDGET;
    subform_widget = NO_WIDGET;
    next_page_widget = NO_WIDGET;
    first_page_widget = NO_WIDGET;
    ping_servers_widget = NO_WIDGET;

}

/*
 * Cleanup everything.
 */
static void Welcome_cleanup(void)
{
    Welcome_destroy_windows();
    Delete_server_list();
}

/*
 * Change to a new subfunction mode.
 * Possibly call cleanup handlers for the mode
 * which we are leaving.
 */
static void Welcome_set_mode(enum Welcome_mode new_welcome_mode)
{
    enum Welcome_mode old_welcome_mode = welcome_mode;

    Widget_destroy_children(subform_widget);

    switch (old_welcome_mode) {
    case ModeWaiting:
	break;
    case ModeLocalnet:
	Localnet_cleanup();
	break;
    case ModeInternet:
	if (new_welcome_mode != old_welcome_mode)
	    Internet_cleanup();
	break;
    case ModeServer:
	break;
    case ModeHelp:
	break;
    case ModeStatus:
	break;
    case ModeQuit:
	break;
    default:
	break;
    }

    welcome_mode = new_welcome_mode;
}

/*
 * Process one event.
 */
static int Welcome_process_one_event(XEvent * event,
				     Connect_param_t * conpar)
{
    XClientMessageEvent *cmev;
    XConfigureEvent *conf;

    switch (event->type) {

    case ClientMessage:
	cmev = (XClientMessageEvent *) event;
	if (cmev->message_type == ProtocolAtom
	    && cmev->format == 32 && cmev->data.l[0] == KillAtom) {
	    /*
	     * On HP-UX 10.20 with CDE strange things happen
	     * sometimes when closing xpilot via the window
	     * manager.  Keypresses may result in funny characters
	     * after the client exits.  The remedy to this seems
	     * to be to explicitly destroy the top window with
	     * XDestroyWindow when the window manager asks the
	     * client to quit and then wait for the resulting
	     * DestroyNotify event before closing the connection
	     * with the X server.
	     */
	    XDestroyWindow(dpy, topWindow);
	    XSync(dpy, True);
	    printf("Quit\n");
	    return -1;
	}
	break;

    case MapNotify:
	if (ignoreWindowManager == 1) {
	    XSetInputFocus(dpy, topWindow, RevertToParent, CurrentTime);
	    ignoreWindowManager = 2;
	}
	break;

	/* Back in play */
    case FocusIn:
	gotFocus = true;
	XAutoRepeatOff(dpy);
	break;

	/* Probably not playing now */
    case FocusOut:
    case UnmapNotify:
	gotFocus = false;
	XAutoRepeatOn(dpy);
	break;

    case MappingNotify:
	XRefreshKeyboardMapping(&event->xmapping);
	break;

    case ConfigureNotify:
      
	conf = &event->xconfigure;
	if (conf->window == topWindow) {
	    if ((top_width == conf->width) && (top_height == conf->height)) {
		/* This event came from a window move operation */
		return 0;
	    }
	    top_width = conf->width;
	    top_height = conf->height;
	    LIMIT(top_width, MIN_TOP_WIDTH, MAX_TOP_WIDTH);
	    LIMIT(top_height, MIN_TOP_HEIGHT, MAX_TOP_HEIGHT);

	    switch (welcome_mode) {
	    case ModeInternet:
	    case ModeStatus:
	      Welcome_destroy_windows(); 
		
	      if (Welcome_create_windows(conpar) == -1)
		return -1;
	      
	      Welcome_set_mode(ModeInternet); 
	      server_it = List_begin(server_list);
	      Welcome_show_server_list(conpar);
	      
	      break;
	    case ModeLocalnet:
		Welcome_destroy_windows();
		Delete_server_list();
		if (Welcome_create_windows(conpar) == -1)
		    return -1;
		Welcome_set_mode(ModeLocalnet);

	    case ModeServer:
		break;
	    case ModeHelp:
		break;
	    case ModeQuit:
		break;
	    case ModeWaiting:
	      Welcome_destroy_windows();
	      if (Welcome_create_windows(conpar) == -1)
		return -1;
	      Welcome_set_mode(ModeWaiting);
	      
	    default:
		break;
	    }
	} else {
	    Widget_event(event);
	}
	break;

    default:
	Widget_event(event);
	break;
    }
    if (quitting) {
	quitting = false;
	return -1;
    }
    if (joining)
	return 1;

    return 0;
}

/*
 * Process all events which are in the queue, but don't block.
 */
static int Welcome_process_pending_events(Connect_param_t * conpar)
{
    int result;
    XEvent event;

    while (XEventsQueued(dpy, QueuedAfterFlush) > 0) {
	XNextEvent(dpy, &event);
	result = Welcome_process_one_event(&event, conpar);
	if (result)
	    return result;
    }
    return 0;
}

/*
 * Loop forever processing events.
 */
static int Welcome_input_loop(Connect_param_t * conpar)
{
    int result;
    XEvent event;

    Welcome_set_mode(ModeWaiting);

    /* Start main loop */
    Welcome_create_label(1, "Welcome to XPilot NG!");

    while (!quitting && !joining) {
	XNextEvent(dpy, &event);
	result = Welcome_process_one_event(&event, conpar);
	if (result)
	    return result;
    }

    return -1;
}

/*
 * Create the windows.
 */
static int Welcome_doit(Connect_param_t * conpar)
{
    int result;

#if 0
    XSynchronize(dpy, True);
#endif
    if (Init_top() == -1)
	return -1;

    XMapSubwindows(dpy, topWindow);
    XMapWindow(dpy, topWindow);
    XSync(dpy, False);

    result = Welcome_process_pending_events(conpar);
    if (result)
	return result;

    if (Welcome_create_windows(conpar) == -1)
	return -1;

    result = Welcome_process_pending_events(conpar);
    if (result)
	return result;

    result = Welcome_input_loop(conpar);
    return result;
}

/*
 * The one and only entry point into this modules.
 */
int Welcome_screen(Connect_param_t * conpar)
{
    int result;

    /* save pointer so that join callbacks can copy into it. */
    global_conpar = conpar;

    result = Welcome_doit(conpar);

    if (!quitting && joining)
	Welcome_cleanup();
    else
	/* kps - ?????? */
	Platform_specific_cleanup();

    return result;
}
