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

typedef struct undo_t {
    int x, y;
    char icon;
    struct undo_t *next;
} undo_t;

Window changedwin;
int prefx = PREF_X, prefy = PREF_Y;
int prevdraw_x, prevdraw_y;
int prevline_x, prevline_y, prevlinend_x, prevlinend_y;
int selectfrom_x = -1, selectfrom_y, selectto_x, selectto_y;
undo_t *undolist = NULL;

/***************************************************************************/
/* DrawMapIcon                                                             */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/*   x                                                                     */
/*   y                                                                     */
/*   count                                                                 */
/*   btn                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
int DrawMapIcon(HandlerInfo_t info)
{
    char icon = ' ';
    int deltax, deltay, sign, i, x2, y2, x, y, count;
    unsigned int btn;


    x = info.x - info.field->x;
    y = info.y - info.field->y;
    btn = info.button;
    count = info.count;

    switch (drawmode) {

    case 1:
	if (btn == SELECT_BTN) {
	    SelectArea(x, y, count);
	    return 0;
	}
	if (btn == DRAW_ICON_BTN) {
	    icon = iconmenu[drawicon];
	}
	if (x < 0) {
	    x = 0;
	} else if (x > mapwin_width - TOOLSWIDTH) {
	    x = mapwin_width - TOOLSWIDTH - map.view_zoom;
	}
	if (y < 0) {
	    y = 0;
	} else if (y > mapwin_height) {
	    y = mapwin_height - map.view_zoom;
	}
	ClearSelectArea();
	x /= map.view_zoom;
	y /= map.view_zoom;
	x += map.view_x;
	y += map.view_y;
	if (count <= 0)
	    return 0;
	if (count == 1) {
	    ClearUndo();
	    prevdraw_x = x;
	    prevdraw_y = y;
	    ChangeMapData(x, y, icon, 1);
	    return 0;
	} else {
	    if (prevdraw_x == x && prevdraw_y == y) {
		return 0;
	    }
	    deltax = x - prevdraw_x;
	    deltay = y - prevdraw_y;
	    if (abs(deltax) >= abs(deltay)) {
		sign = (deltax < 0) ? -1 : 1;
		for (i = sign; abs(i) <= abs(deltax); i += sign) {
		    x2 = prevdraw_x + i;
		    y2 = prevdraw_y + (i * deltay) / deltax;
		    ChangeMapData(x2, y2, icon, 1);
		}
		prevdraw_x = x;
		prevdraw_y = y;
	    } else {
		sign = (deltay < 0) ? -1 : 1;
		for (i = sign; abs(i) <= abs(deltay); i += sign) {
		    x2 = prevdraw_x + (i * deltax) / deltay;
		    y2 = prevdraw_y + i;
		    ChangeMapData(x2, y2, icon, 1);
		}
		prevdraw_x = x;
		prevdraw_y = y;
	    }
	}
	break;

    case 2:
	if (btn == SELECT_BTN) {
	    SelectArea(x, y, count);
	    return 0;
	}
	if (btn == LINE_ICON_BTN) {
	    icon = iconmenu[drawicon];
	}
	ClearSelectArea();
	if (count == 0) {
	    XDrawLine(display, mapwin, xorgc,
		      (int) ((prevline_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevline_y - map.view_y +
			      .5) * map.view_zoom),
		      (int) ((prevlinend_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevlinend_y - map.view_y +
			      .5) * map.view_zoom));
	    if ((x < 0) || (y < 0) || (x > mapwin_width - TOOLSWIDTH)
		|| (y > mapwin_height)) {
		return 0;
	    } else {
		ClearUndo();
		deltax = prevlinend_x - prevline_x;
		deltay = prevlinend_y - prevline_y;
		if ((deltax == 0) && (deltay == 0)) {
		    ChangeMapData(prevline_x, prevline_y, icon, 1);
		    return 0;
		}
		if (abs(deltax) >= abs(deltay)) {
		    sign = (deltax < 0) ? -1 : 1;
		    for (i = 0; abs(i) <= abs(deltax); i += sign) {
			x2 = prevline_x + i;
			y2 = prevline_y + (i * deltay) / deltax;
			ChangeMapData(x2, y2, icon, 1);
		    }
		} else {
		    sign = (deltay < 0) ? -1 : 1;
		    for (i = 0; abs(i) <= abs(deltay); i += sign) {
			x2 = prevline_x + (i * deltax) / deltay;
			y2 = prevline_y + i;
			ChangeMapData(x2, y2, icon, 1);
		    }
		}
	    }
	    return 0;
	}
	x /= map.view_zoom;
	y /= map.view_zoom;
	x += map.view_x;
	y += map.view_y;
	if (count == 1) {
	    ClearSelectArea();
	    prevline_x = prevlinend_x = x;
	    prevline_y = prevlinend_y = y;
	    XDrawLine(display, mapwin, xorgc,
		      (int) ((prevline_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevline_y - map.view_y +
			      .5) * map.view_zoom),
		      (int) ((prevlinend_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevlinend_y - map.view_y +
			      .5) * map.view_zoom));
	    return 0;
	} else {
	    XDrawLine(display, mapwin, xorgc,
		      (int) ((prevline_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevline_y - map.view_y +
			      .5) * map.view_zoom),
		      (int) ((prevlinend_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevlinend_y - map.view_y +
			      .5) * map.view_zoom));
	    prevlinend_x = x;
	    prevlinend_y = y;
	    XDrawLine(display, mapwin, xorgc,
		      (int) ((prevline_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevline_y - map.view_y +
			      .5) * map.view_zoom),
		      (int) ((prevlinend_x - map.view_x +
			      .5) * map.view_zoom + TOOLSWIDTH),
		      (int) ((prevlinend_y - map.view_y +
			      .5) * map.view_zoom));
	}
	break;

    case 3:
	SelectArea(x, y, count);
	break;

    }				/* end switch */
    return 0;
}

void SelectArea(int x, int y, int count)
{
    if (x < 0) {
	x = 0;
    } else if (x > mapwin_width - TOOLSWIDTH) {
	x = mapwin_width - TOOLSWIDTH - map.view_zoom;
    }
    if (y < 0) {
	y = 0;
    } else if (y > mapwin_height) {
	y = mapwin_height - map.view_zoom;
    }
    if (count == 0) {
	return;
    }
    if (count == 1) {
	ClearSelectArea();
	selectfrom_x = selectto_x = x / map.view_zoom;
	selectfrom_y = selectto_y = y / map.view_zoom;
	DrawSelectArea();
	return;
    }
    DrawSelectArea();
    x /= map.view_zoom;
    y /= map.view_zoom;
    selectto_x = x;
    selectto_y = y;
    DrawSelectArea();
}

/***************************************************************************/
/* ChangeMapData                                                           */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/*   icon                                                                  */
/*   save                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
/* RTT also made this do something too */
void ChangeMapData(int x, int y, char icon, int save)
{
    int x2, y2, xo = 0, yo = 0, wo = 0, ho = 0;
    char data;

    map.changed = 1;
    x2 = (x - map.view_x) * map.view_zoom;
    y2 = (y - map.view_y) * map.view_zoom;

    if (x < 0)
	x += map.width;
    if (y < 0)
	y += map.height;
    if (x > (map.width - 1))
	x -= map.width;
    if (y > (map.height - 1))
	y -= map.height;
    if (map.data[x][y] == icon)
	return;
    if (save)
	SaveUndoIcon(x, y, map.data[x][y]);
    map.data[x][y] = icon;
    UpdateSmallMap(x, y);

    if (x2 < 0)
	x2 += map.width * map.view_zoom;
    if (y2 < 0)
	y2 += map.height * map.view_zoom;
    if (x2 > (mapwin_width - TOOLSWIDTH))
	x2 -= map.width * map.view_zoom;
    if (y2 > mapwin_height)
	y2 -= map.width * map.view_zoom;
    if ((x2 < 0) || (y2 < 0) || (x2 > (mapwin_width - TOOLSWIDTH))
	|| (y2 > mapwin_height))
	return;
    data = MapData(x - 1, y);
    if ((data == XPMAP_FILLED) || (data == XPMAP_FUEL) ||
	(data == XPMAP_REC_RU) || (data == XPMAP_REC_RD)
	|| (data == XPMAP_DECOR_RU) || (data == XPMAP_DECOR_RD)
	|| (data == XPMAP_DECOR_FILLED)) {
	xo++;
	wo--;
    }
    data = MapData(x + 1, y);
    if ((data == XPMAP_FILLED) || (data == XPMAP_FUEL) ||
	(data == XPMAP_REC_LU) || (data == XPMAP_REC_LD)
	|| (data == XPMAP_DECOR_LU) || (data == XPMAP_DECOR_LD)
	|| (data == XPMAP_DECOR_FILLED)) {
	wo--;
    }
    data = MapData(x, y - 1);
    if ((data == XPMAP_FILLED) || (data == XPMAP_FUEL) ||
	(data == XPMAP_REC_RD) || (data == XPMAP_REC_LD)
	|| (data == XPMAP_DECOR_RD) || (data == XPMAP_DECOR_LD)
	|| (data == XPMAP_DECOR_FILLED)) {
	yo++;
	ho--;
    }
    data = MapData(x, y + 1);
    if ((data == XPMAP_FILLED) || (data == XPMAP_FUEL) ||
	(data == XPMAP_REC_RU) || (data == XPMAP_REC_LU)
	|| (data == XPMAP_DECOR_RU) || (data == XPMAP_DECOR_LU)
	|| (data == XPMAP_DECOR_FILLED)) {
	ho--;
    }
    XFillRectangle(display, mapwin, Black_GC, x2 + TOOLSWIDTH + xo,
		   y2 + yo, map.view_zoom + 1 + wo,
		   map.view_zoom + 1 + ho);
    DrawMapSection(x, y, 0, 0, x2 + TOOLSWIDTH, y2);
}

/***************************************************************************/
/* MoveMapView                                                             */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/*   x                                                                     */
/*   y                                                                     */
/*   count                                                                 */
/*   btn                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
int MoveMapView(HandlerInfo_t info)
{
    int oldx, oldy;
    int width, height, xclear, yclear;
    int xfrom, yfrom, xto, yto;
    int x, y, count;
    unsigned int btn;

    x = info.x - info.field->x;
    y = info.y - info.field->y;
    count = info.count;
    btn = info.button;
    if (count == 0)
	return 1;
    ClearSelectArea();
    oldx = map.view_x;
    oldy = map.view_y;
    DrawViewBox();
    map.view_x =
	(x * smlmap_xscale) - (mapwin_width -
			       TOOLSWIDTH) / (map.view_zoom * 2);
    map.view_y = (y * smlmap_yscale) - mapwin_height / (map.view_zoom * 2);
    while (map.view_x < 0)
	map.view_x += map.width;
    while (map.view_y < 0)
	map.view_y += map.height;
    while (map.view_x > map.width)
	map.view_x -= map.width;
    while (map.view_y > map.height)
	map.view_y -= map.height;
    if ((map.view_x != oldx) || (map.view_y != oldy)) {
	width =
	    mapwin_width - abs(oldx - map.view_x) * map.view_zoom -
	    TOOLSWIDTH;
	height = mapwin_height - abs(oldy - map.view_y) * map.view_zoom;
	if (oldx < map.view_x) {
	    xfrom = (map.view_x - oldx) * map.view_zoom;
	    xto = 0;
	    xclear = width;
	} else {
	    xfrom = 0;
	    xto = (oldx - map.view_x) * map.view_zoom;
	    xclear = 0;
	}
	if (oldy < map.view_y) {
	    yfrom = (map.view_y - oldy) * map.view_zoom;
	    yto = 0;
	    yclear = height;
	} else {
	    yfrom = 0;
	    yto = (oldy - map.view_y) * map.view_zoom;
	    yclear = 0;
	}

	if ((width > 0) && (height > 0)) {
	    XCopyArea(display, mapwin, mapwin, White_GC,
		      xfrom + TOOLSWIDTH, yfrom, width, height,
		      xto + TOOLSWIDTH, yto);
	    if ((mapwin_width - TOOLSWIDTH) != width) {
		XClearArea(display, mapwin, xclear + TOOLSWIDTH, 0,
			   mapwin_width - width - TOOLSWIDTH,
			   mapwin_height, 0);
		DrawMap(xclear + TOOLSWIDTH - 1, 0,
			mapwin_width - width - TOOLSWIDTH + 1,
			mapwin_height);
	    }
	    if (((mapwin_width - TOOLSWIDTH) != xto)
		&& (mapwin_height != height)) {
		XClearArea(display, mapwin, xto + TOOLSWIDTH, yclear,
			   mapwin_width - xto - TOOLSWIDTH,
			   mapwin_height - height, 0);
		DrawMap(xto + TOOLSWIDTH - 1, yclear - 1,
			mapwin_width - xto - TOOLSWIDTH + 1,
			mapwin_height - height + 1);
	    }
	} else {
	    XClearArea(display, mapwin, TOOLSWIDTH, 0,
		       mapwin_width - TOOLSWIDTH, mapwin_height, 0);
	    DrawMap(TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH,
		    mapwin_height);
	}
    }
    DrawViewBox();
    return 0;
}

/***************************************************************************/
/* ZoomOut                                                                 */
/* Arguments :                                                             */
/*   win                                                                   */
/*   btnname                                                               */
/* Purpose :                                                               */
/***************************************************************************/
int ZoomOut(HandlerInfo_t info)
{
    int x, y, oldvx = 0, oldvy = 0;

    if (map.view_zoom < 2)
	return 1;
    DrawViewBox();
    if (selectfrom_x >= 0) {
	x = map.view_x + (selectfrom_x + selectto_x) / 2;
	y = map.view_y + (selectfrom_y + selectto_y) / 2;
	oldvx = map.view_x;
	oldvy = map.view_y;
	DrawSelectArea();
    } else {
	x = map.view_x + (mapwin_width - TOOLSWIDTH) / (map.view_zoom * 2);
	y = map.view_y + (mapwin_height) / (map.view_zoom * 2);
    }
    if (map.view_zoom < 10)
	map.view_zoom--;
    else if (map.view_zoom < 20)
	map.view_zoom -= 2;
    else
	map.view_zoom -= 4;
    while ((((mapwin_width - TOOLSWIDTH) / map.view_zoom) > map.width) ||
	   ((mapwin_height / map.view_zoom) > map.height))
	map.view_zoom++;
    map.view_x = x - (mapwin_width - TOOLSWIDTH) / (map.view_zoom * 2);
    map.view_y = y - (mapwin_height) / (map.view_zoom * 2);
    DrawViewBox();
    XClearArea(display, mapwin, TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH,
	       mapwin_height, 0);
    DrawMap(TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH, mapwin_height);
    T_SetWindowSizeLimits(mapwin, TOOLSWIDTH + 50, TOOLSHEIGHT,
			  TOOLSWIDTH + map.width * map.view_zoom,
			  map.height * map.view_zoom, 0, 0);
    SizeSelectBounds(oldvx, oldvy);
    return 0;
}

/***************************************************************************/
/* ZoomIn                                                                  */
/* Arguments :                                                             */
/*   win                                                                   */
/*   btnname                                                               */
/* Purpose :                                                               */
/***************************************************************************/
int ZoomIn(HandlerInfo_t info)
{
    int x, y, oldvx = 0, oldvy = 0;

    if (map.view_zoom > 46)
	return 1;
    DrawViewBox();
    if (selectfrom_x >= 0) {
	x = map.view_x + (selectfrom_x + selectto_x) / 2;
	y = map.view_y + (selectfrom_y + selectto_y) / 2;
	oldvx = map.view_x;
	oldvy = map.view_y;
	DrawSelectArea();
    } else {
	x = map.view_x + (mapwin_width - TOOLSWIDTH) / (map.view_zoom * 2);
	y = map.view_y + (mapwin_height) / (map.view_zoom * 2);
    }
    if (map.view_zoom > 20)
	map.view_zoom += 4;
    else if (map.view_zoom > 10)
	map.view_zoom += 2;
    else
	map.view_zoom++;
    map.view_x = x - (mapwin_width - TOOLSWIDTH) / (map.view_zoom * 2);
    map.view_y = y - (mapwin_height) / (map.view_zoom * 2);
    DrawViewBox();
    XClearArea(display, mapwin, TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH,
	       mapwin_height, 0);
    DrawMap(TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH, mapwin_height);
    T_SetWindowSizeLimits(mapwin, TOOLSWIDTH + 50, TOOLSHEIGHT,
			  TOOLSWIDTH + map.width * map.view_zoom,
			  map.height * map.view_zoom, 0, 0);
    SizeSelectBounds(oldvx, oldvy);
    return 0;
}

void SizeSelectBounds(int oldvx, int oldvy)
{
    if (selectfrom_x < 0)
	return;
    selectfrom_x += (oldvx - map.view_x);
    selectfrom_y += (oldvy - map.view_y);
    selectto_x += (oldvx - map.view_x);
    selectto_y += (oldvy - map.view_y);
    if (selectfrom_x < 0)
	selectfrom_x = 0;
    if (selectfrom_y < 0)
	selectfrom_y = 0;
    if (selectto_x < 0)
	selectto_x = 0;
    if (selectto_y < 0)
	selectto_y = 0;
    if (selectfrom_x > (mapwin_width - TOOLSWIDTH) / map.view_zoom)
	selectfrom_x = (mapwin_width - TOOLSWIDTH) / map.view_zoom;
    if (selectfrom_y > (mapwin_height) / map.view_zoom)
	selectfrom_y = mapwin_height / map.view_zoom;
    if (selectto_x > (mapwin_width - TOOLSWIDTH) / map.view_zoom)
	selectto_x = (mapwin_width - TOOLSWIDTH) / map.view_zoom;
    if (selectto_y > (mapwin_height) / map.view_zoom)
	selectto_y = mapwin_height / map.view_zoom;
    DrawSelectArea();
}

/***************************************************************************/
/* ExitApplication                                                         */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int ExitApplication(HandlerInfo_t info)
{
    if (ChangedPrompt(ExitApplication))
	return 0;
    if (T_IsPopupOpen(changedwin)) {
	T_PopupClose(changedwin);
	changedwin = (Window) NULL;
    }
    T_FormCloseWindow(mapwin);
    T_FormCloseWindow(prefwin);
    T_FormCloseWindow(filepromptwin);

    XFreeGC(display, White_GC);
    XFreeGC(display, Black_GC);
    XFreeGC(display, xorgc);
    XFreeGC(display, Wall_GC);
    XFreeGC(display, Decor_GC);
    XFreeGC(display, Fuel_GC);
    XFreeGC(display, Treasure_GC);
    XFreeGC(display, Target_GC);
    XFreeGC(display, Item_Conc_GC);
    XFreeGC(display, Gravity_GC);
    XFreeGC(display, Current_GC);
    XFreeGC(display, Wormhole_GC);
    XFreeGC(display, Base_GC);
    XFreeGC(display, Cannon_GC);
    T_CloseServerConnection();
    exit(0);
}

/***************************************************************************/
/* SaveUndoIcon                                                            */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/*   icon                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int SaveUndoIcon(int x, int y, char icon)
{
    struct undo_t *undo = NULL;

    undo = (struct undo_t *) undolist;
    undolist = (undo_t *) malloc(sizeof(undo_t));
    undolist->next = undo;
    undolist->x = x;
    undolist->y = y;
    undolist->icon = icon;
    return 0;
}

/***************************************************************************/
/* Undo                                                                    */
/* Arguments :                                                             */
/*   win                                                                   */
/*   btnname                                                               */
/* Purpose :                                                               */
/***************************************************************************/
int Undo(HandlerInfo_t info)
{
    undo_t *traverse;

    ClearSelectArea();
    /* if the first icon is a breakpoint, skip it */
    if (undolist != NULL) {
	if (undolist->icon == '\n') {
	    traverse = (undo_t *) undolist->next;
	    free(undolist);
	    undolist = traverse;
	}
    }
    while (undolist != NULL) {

	/* check if we are at a breakpoint */
	if (undolist->icon == '\n') {
	    return 0;
	}
	ChangeMapData(undolist->x, undolist->y, undolist->icon, 0);
	traverse = (undo_t *) undolist->next;
	free(undolist);
	undolist = traverse;
    }
    return 0;
}

/***************************************************************************/
/* ClearUndo                                                               */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void ClearUndo(void)
{
    /* insert a breakpoint */
    SaveUndoIcon(0, 0, '\n');
}

/***************************************************************************/
/* NewMap                                                                  */
/* Arguments :                                                             */
/*   win                                                                   */
/*   btnname                                                               */
/* Purpose :                                                               */
/***************************************************************************/
int NewMap(HandlerInfo_t info)
{
    int i, j;

    ClearSelectArea();
    if (ChangedPrompt(NewMap))
	return 1;
    if (T_IsPopupOpen(changedwin)) {
	T_PopupClose(changedwin);
	changedwin = (Window) NULL;
    }
    free(map.comments);
    map.comments = (char *) NULL;
    map.mapName[0] = map.mapAuthor[0] = map.mapFileName[0] =
	map.gravity[0] = '\0';
    map.shipMass[0] = map.maxRobots[0] = map.worldLives[0] = '\0';
    map.width = DEFAULT_WIDTH;
    map.height = DEFAULT_HEIGHT;
    sprintf(map.width_str, "%d", map.width);
    sprintf(map.height_str, "%d", map.height);
    for (i = 0; i < MAX_MAP_SIZE; i++)
	for (j = 0; j < MAX_MAP_SIZE; j++)
	    map.data[i][j] = ' ';
    map.view_zoom = DEFAULT_MAP_ZOOM;
    map.changed = map.edgeWrap = map.edgeBounce = map.teamPlay = 0;
    map.timing = map.allowPlayerCrashes = map.allowPlayerKilling = 0;
    map.limitedVisibility = map.allowShields = 0;
    ResetMap();
    ClearUndo();
    return 0;
}

/***************************************************************************/
/* ResizeWidth                                                             */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int ResizeWidth(HandlerInfo_t info)
{
    int width;

    width = atoi(info.field->charvar);
    if ((width < 20) || (width > MAX_MAP_SIZE)) {
	strcpy(info.field->charvar, info.form->entry_restore);
    } else {
	sprintf(info.field->charvar, "%d", width);
	map.width = width;
	ResetMap();
	ClearUndo();
    }
    return 0;
}

/***************************************************************************/
/* ResizeHeight                                                            */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int ResizeHeight(HandlerInfo_t info)
{
    int height;

    height = atoi(info.field->charvar);
    if ((height < 20) || (height > MAX_MAP_SIZE)) {
	strcpy(info.field->charvar, info.form->entry_restore);
    } else {
	sprintf(info.field->charvar, "%d", height);
	map.height = height;
	ResetMap();
	ClearUndo();
    }
    return 0;
}

/***************************************************************************/
/* OpenPreferencesPopup                                                    */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int OpenPreferencesPopup(HandlerInfo_t info)
{
    XMapWindow(display, prefwin);
    return 0;
}

/***************************************************************************/
/* OpenMapInfoPopup                                                        */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenMapInfoPopup()
{
    Window *temp;

    switch (prefssheet) {
    case 0:
	temp = &mapinfo;
	break;
    case 1:
	temp = &robots;
	break;
    case 2:
	temp = &visibility;
	break;
    case 3:
	temp = &cannons;
	break;
    case 4:
	temp = &rounds;
	break;
    case 5:
	temp = &inititems;
	break;
    case 6:
	temp = &maxitems;
	break;
    case 7:
	temp = &probs;
	break;
    case 8:
	temp = &scoring;
	break;
    }

    XMapWindow(display, mapinfo);
    return 0;
}

/***************************************************************************/
/* OpenRobotsPopup                                                         */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenRobotsPopup()
{
    XMapWindow(display, robots);
    return 0;
}

/***************************************************************************/
/* OpenVisibilityPopup                                                     */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenVisibilityPopup()
{
    XMapWindow(display, visibility);
    return 0;
}

/***************************************************************************/
/* OpenCannonsPopup                                                        */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenCannonsPopup()
{
    XMapWindow(display, cannons);
    return 0;
}

/***************************************************************************/
/* OpenRoundsPopup                                                         */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenRoundsPopup()
{
    XMapWindow(display, rounds);
    return 0;
}

/***************************************************************************/
/* OpenInitItemsPopup                                                      */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenInitItemsPopup()
{
    XMapWindow(display, inititems);
    return 0;
}

/***************************************************************************/
/* OpenMaxItemsPopup                                                       */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenMaxItemsPopup()
{
    XMapWindow(display, maxitems);
    return 0;
}

/***************************************************************************/
/* OpenProbsPopup                                                          */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenProbsPopup()
{
    XMapWindow(display, probs);
    return 0;
}

/***************************************************************************/
/* OpenScoringPopup                                                        */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int OpenScoringPopup()
{
    XMapWindow(display, scoring);
    return 0;
}

/***************************************************************************/
/* ValidateCoordHandler                                                    */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/*   variable                                                              */
/* Purpose :                                                               */
/***************************************************************************/
int ValidateCoordHandler(HandlerInfo_t info)
{
    char *returnval;
    char *string, *start;

    returnval = malloc(strlen(info.field->charvar) + 1);
    returnval[0] = '\0';
    string = malloc(strlen(info.field->charvar) + 1);
    start = string;
    strcpy(string, info.field->charvar);

    while (string[0] != '\0') {
	if (((string[0] >= '0') && (string[0] <= '9'))
	    || (string[0] == ','))
	    sprintf(returnval, "%s%c", returnval, string[0]);
	string++;
    }

    strcpy(info.field->charvar, returnval);
    free(returnval);
    free(start);
    return 0;
}

/***************************************************************************/
/* ShowHoles                                                               */
/* Arguments :                                                             */
/*   win                                                                   */
/*   btnname                                                               */
/* Purpose :                                                               */
/***************************************************************************/
int ShowHoles(HandlerInfo_t info)
{
    int i, j, w, h, x, y;

    ClearSelectArea();
    w = mapwin_width / map.view_zoom;
    h = mapwin_height / map.view_zoom;
    for (i = 0; i < w; i++)
	for (j = 0; j < w; j++) {
	    x = i + map.view_x;
	    y = j + map.view_y;
	    if ((mapicon_ptr[MapData(x, y) - 32] == 20) &&
		(mapicon_ptr[MapData(x - 1, y) - 32] < 6) &&
		(mapicon_ptr[MapData(x, y - 1) - 32] < 6) &&
		(mapicon_ptr[MapData(x + 1, y) - 32] < 6) &&
		(mapicon_ptr[MapData(x, y + 1) - 32] < 6))
		XFillRectangle(display, mapwin, White_GC,
			       TOOLSWIDTH + i * map.view_zoom + 1,
			       j * map.view_zoom + 1, map.view_zoom - 1,
			       map.view_zoom - 1);
	}
    return 0;
}

/***************************************************************************/
/* MapData                                                                 */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/* Purpose :                                                               */
/***************************************************************************/
char MapData(int x, int y)
{
    if (x < 0) {
	x += map.width;
    } else if (x >= map.width) {
	x -= map.width;
    }
    if (y < 0) {
	y += map.height;
    } else if (y >= map.height) {
	y -= map.height;
    }

    return map.data[x][y];
}

/***************************************************************************/
/* ChangedPrompt                                                           */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int ChangedPrompt(handler_t handler)
{
    if (changedwin != (Window) NULL)
	return 0;
    if (map.changed == 0)
	return 0;
    map.changed = 0;
    changedwin =
	T_PopupAlert(2,
		     "Would you like to save the changes made to this map?",
		     "Yes", "No", SavePrompt, handler);
    return 1;
}

/***************************************************************************/
/* ClearSelectArea                                                         */
/* Arguments :                                                             */
/* Purpose : Remove the select rectangle and turn the select box off.      */
/***************************************************************************/
void ClearSelectArea(void)
{
    DrawSelectArea();
    selectfrom_x = -1;
}

/***************************************************************************/
/* DrawSelectArea                                                          */
/* Arguments :                                                             */
/* Purpose : Draw the select rectangle.                                    */
/***************************************************************************/
void DrawSelectArea(void)
{
    int x, y, w, h;

    if (selectfrom_x < 0)
	return;
    w = (selectto_x - selectfrom_x) * map.view_zoom;
    h = (selectto_y - selectfrom_y) * map.view_zoom;
    if (w < 0) {
	w = -w;
	x = selectto_x * map.view_zoom + TOOLSWIDTH;
    } else {
	x = selectfrom_x * map.view_zoom + TOOLSWIDTH;
    }
    if (h < 0) {
	h = -h;
	y = selectto_y * map.view_zoom;
    } else {
	y = selectfrom_y * map.view_zoom;
    }

    XDrawRectangle(display, mapwin, xorgc, x - 1, y - 1,
		   w + map.view_zoom + 2, h + map.view_zoom + 2);
}

/***************************************************************************/
/* FillMapArea                                                             */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int FillMapArea(HandlerInfo_t info)
{
    int i, j, x1, y_1, x2, y2;

    DrawSelectArea();
    ClearUndo();
    if (selectfrom_x < 0)
	return 1;
    if (selectfrom_x < selectto_x) {
	x1 = selectfrom_x + map.view_x;
	x2 = selectto_x + 1 + map.view_x;
    } else {
	x1 = selectto_x + map.view_x;
	x2 = selectfrom_x + 1 + map.view_x;
    }
    if (selectfrom_y < selectto_y) {
	y_1 = selectfrom_y + map.view_y;
	y2 = selectto_y + 1 + map.view_y;
    } else {
	y_1 = selectto_y + map.view_y;
	y2 = selectfrom_y + 1 + map.view_y;
    }
    for (i = x1; i < x2; i++)
	for (j = y_1; j < y2; j++)
	    ChangeMapData(i, j, iconmenu[drawicon], 1);
    DrawSelectArea();
    return 0;
}

/***************************************************************************/
/* CopyMapArea                                                             */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int CopyMapArea(HandlerInfo_t info)
{
    int i, j, x1, y_1, x2, y2;

    DrawSelectArea();
    if (selectfrom_x < 0)
	return 1;
    if (selectfrom_x < selectto_x) {
	x1 = selectfrom_x + map.view_x;
	x2 = selectto_x + 1 + map.view_x;
    } else {
	x1 = selectto_x + map.view_x;
	x2 = selectfrom_x + 1 + map.view_x;
    }
    if (selectfrom_y < selectto_y) {
	y_1 = selectfrom_y + map.view_y;
	y2 = selectto_y + 1 + map.view_y;
    } else {
	y_1 = selectto_y + map.view_y;
	y2 = selectfrom_y + 1 + map.view_y;
    }
    for (i = 0; i < MAX_MAP_SIZE; i++)
	for (j = 0; j < MAX_MAP_SIZE; j++)
	    clipdata[i][j] = ' ';
    for (i = x1; i < x2; i++)
	for (j = y_1; j < y2; j++)
	    clipdata[i - x1][j - y_1] = MapData(i, j);
    DrawSelectArea();
    return 0;
}

/***************************************************************************/
/* CutMapArea                                                              */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int CutMapArea(HandlerInfo_t info)
{
    int i, j, x1, y_1, x2, y2;

    DrawSelectArea();
    ClearUndo();
    if (selectfrom_x < 0)
	return 1;
    if (selectfrom_x < selectto_x) {
	x1 = selectfrom_x + map.view_x;
	x2 = selectto_x + 1 + map.view_x;
    } else {
	x1 = selectto_x + map.view_x;
	x2 = selectfrom_x + 1 + map.view_x;
    }
    if (selectfrom_y < selectto_y) {
	y_1 = selectfrom_y + map.view_y;
	y2 = selectto_y + 1 + map.view_y;
    } else {
	y_1 = selectto_y + map.view_y;
	y2 = selectfrom_y + 1 + map.view_y;
    }
    for (i = 0; i < MAX_MAP_SIZE; i++)
	for (j = 0; j < MAX_MAP_SIZE; j++)
	    clipdata[i][j] = ' ';
    for (i = x1; i < x2; i++)
	for (j = y_1; j < y2; j++) {
	    clipdata[i - x1][j - y_1] = MapData(i, j);
	    ChangeMapData(i, j, ' ', 1);
	}
    DrawSelectArea();
    return 0;
}

/***************************************************************************/
/* PasteMapArea                                                            */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int PasteMapArea(HandlerInfo_t info)
{
    int i, j, x1, y_1, x2, y2;

    DrawSelectArea();
    ClearUndo();
    if (selectfrom_x < 0)
	return 1;
    if (selectfrom_x < selectto_x) {
	x1 = selectfrom_x + map.view_x;
	x2 = selectto_x + 1 + map.view_x;
    } else {
	x1 = selectto_x + map.view_x;
	x2 = selectfrom_x + 1 + map.view_x;
    }
    if (selectfrom_y < selectto_y) {
	y_1 = selectfrom_y + map.view_y;
	y2 = selectto_y + 1 + map.view_y;
    } else {
	y_1 = selectto_y + map.view_y;
	y2 = selectfrom_y + 1 + map.view_y;
    }
    for (i = x1; i < x2; i++)
	for (j = y_1; j < y2; j++)
	    if (clipdata[i - x1][j - y_1] != ' ')
		ChangeMapData(i, j, clipdata[i - x1][j - y_1], 1);
    DrawSelectArea();
    return 0;
}

/***************************************************************************/
/* NegativeMapArea                                                         */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
int NegativeMapArea(HandlerInfo_t info)
{
    int i, j, x1, y_1, x2, y2;

    DrawSelectArea();
    ClearUndo();
    if (selectfrom_x < 0)
	return 1;
    if (selectfrom_x < selectto_x) {
	x1 = selectfrom_x + map.view_x;
	x2 = selectto_x + 1 + map.view_x;
    } else {
	x1 = selectto_x + map.view_x;
	x2 = selectfrom_x + 1 + map.view_x;
    }
    if (selectfrom_y < selectto_y) {
	y_1 = selectfrom_y + map.view_y;
	y2 = selectto_y + 1 + map.view_y;
    } else {
	y_1 = selectto_y + map.view_y;
	y2 = selectfrom_y + 1 + map.view_y;
    }
    for (i = x1; i < x2; i++)
	for (j = y_1; j < y2; j++) {
	    switch (MapData(i, j)) {

	    case XPMAP_SPACE:
		ChangeMapData(i, j, XPMAP_FILLED, 1);
		break;

	    case XPMAP_FILLED:
		ChangeMapData(i, j, XPMAP_SPACE, 1);
		break;

	    case XPMAP_REC_RD:
		ChangeMapData(i, j, XPMAP_REC_LU, 1);
		break;

	    case XPMAP_REC_LU:
		ChangeMapData(i, j, XPMAP_REC_RD, 1);
		break;

	    case XPMAP_REC_LD:
		ChangeMapData(i, j, XPMAP_REC_RU, 1);
		break;

	    case XPMAP_REC_RU:
		ChangeMapData(i, j, XPMAP_REC_LD, 1);
		break;
	    }
	}
    DrawSelectArea();
    return 0;
}
