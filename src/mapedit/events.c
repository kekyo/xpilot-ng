/*
 * XPilot NG XP-MapEdit, a map editor for xp maps.  Copyright (C) 1993 by
 *
 *      Aaron Averill           <averila@oes.orst.edu>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modifications:
 * 1996:
 *      Robert Templeman        <mbcaprt@mphhpd.ph.man.ac.uk>
 * 1997:
 *      William Docter          <wad2@lehigh.edu>
 */

#include "xpmapedit.h"

/***************************************************************************/
/* MainEventLoop                                                           */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void MainEventLoop(void)
{
    XEvent report;

    while (1) {
	XNextEvent(display, &report);

	if ((report.type == Expose) && (report.xexpose.window == mapwin) &&
	    (report.xexpose.count == 0)) {
	    T_ClearArea(mapwin, 0, 0, TOOLSWIDTH, mapwin_height);
	}

	T_FormEventCheck(&report);

	switch (report.type) {

	case ConfigureNotify:
	    if (report.xconfigure.window == mapwin) {
		mapwin_width = report.xconfigure.width;
		mapwin_height = report.xconfigure.height;
		T_FormScrollArea(mapwin, "draw_map_icon", T_SCROLL_UNBOUND,
				 TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH,
				 mapwin_height, DrawMapIcon);
		break;
	    }

	case Expose:
	    if (report.xexpose.window == mapwin) {
		if (report.xexpose.count == 0)
		    DrawTools();
		DrawMap(report.xexpose.x, report.xexpose.y,
			report.xexpose.width, report.xexpose.height);
		DrawSelectArea();
		break;
	    } else if ((report.xexpose.window == helpwin) &&
		       (report.xexpose.count == 0)) {
		DrawHelpWin();
		break;
	    }

	case KeyPress:
	    MapwinKeyPress(&report);
	    break;

	case ClientMessage:
	    if (report.xclient.message_type == ProtocolAtom
		&& report.xclient.format == 32
		&& report.xclient.data.l[0] == KillAtom) {
		if (report.xclient.window == mapwin) {
		    XDestroyWindow(display, mapwin);
		    XSync(display, True);
		    XCloseDisplay(display);
		    exit(0);
		} else {
		    XUnmapWindow(display, report.xclient.window);
		}
	    }
	}			/* end switch */
    }

}

/***************************************************************************/
/* MapwinKeyPres                                                           */
/* Arguments :                                                             */
/*    report                                                               */
/* Purpose : If keypress is number or letter, draw numbered base or        */
/*           checkpoint.                                                   */
/***************************************************************************/
void MapwinKeyPress(XEvent * report)
{
    int x, y;
    char buffer[1], icon;
    int bufsize = 1;
    KeySym keysym;
    XComposeStatus compose;
    int count;

    if ((report->xkey.x < TOOLSWIDTH) || (report->xkey.window != mapwin)) {
	return;
    }
    x = report->xkey.x - TOOLSWIDTH;
    y = report->xkey.y;
    x /= map.view_zoom;
    y /= map.view_zoom;
    x += map.view_x;
    y += map.view_y;
    count =
	XLookupString(&report->xkey, buffer, bufsize, &keysym, &compose);
    icon = buffer[0];
    if ((icon >= 'a') && (icon <= 'z')) {
	ChangeMapData(x, y, toupper(icon), 1);
    } else if ((icon >= '0') && (icon <= '9')) {
	ChangeMapData(x, y, icon, 1);
    }
}
