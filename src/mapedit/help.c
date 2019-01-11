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

Window helpwin;
static int helppage = 0, helpsel, helpscreens = 3;

const char *iconlabel[36] = {
    "Space",
    "Block", "Block", "Block", "Block", "Block",
    "Decor", "Decor", "Decor", "Decor", "Decor",
    "Fuel",
    "Cannon", "Cannon", "Cannon", "Cannon",
    "Treasure", "Target", "Item Concentrator",
    "CntrClock Gravity", "Clockwise Gravity",
    "Wormhole", "Wormhole In", "Wormhole Out",
    "Positive Gravity", "Negative Gravity",
    "Current", "Current", "Current", "Current",
    "Base", "Base Facing",
    "Empty (space)", "Empty Treasure", "Friction", "Asteroid Concentrator"
};

char iconhelp[36] = {
    ' ',
    XPMAP_FILLED, XPMAP_REC_RD, XPMAP_REC_LD, XPMAP_REC_RU, XPMAP_REC_LU,
    XPMAP_DECOR_FILLED, XPMAP_DECOR_RD, XPMAP_DECOR_LD, XPMAP_DECOR_RU, XPMAP_DECOR_LU,
    XPMAP_FUEL,
    XPMAP_CANNON_LEFT, XPMAP_CANNON_UP, XPMAP_CANNON_DOWN, XPMAP_CANNON_RIGHT,
    XPMAP_TREASURE, XPMAP_TARGET, XPMAP_ITEM_CONCENTRATOR,
    XPMAP_ACWISE_GRAV, XPMAP_CWISE_GRAV,
    XPMAP_WORMHOLE_NORMAL, XPMAP_WORMHOLE_IN, XPMAP_WORMHOLE_OUT,
    XPMAP_POS_GRAV, XPMAP_NEG_GRAV,
    XPMAP_UP_GRAV, XPMAP_LEFT_GRAV, XPMAP_RIGHT_GRAV, XPMAP_DOWN_GRAV,
    XPMAP_BASE, XPMAP_BASE_ATTRACTOR,
    XPMAP_SPACE, XPMAP_EMPTY_TREASURE, XPMAP_FRICTION_AREA, XPMAP_ASTEROID_CONCENTRATOR
};


/***************************************************************************/
/* OpenHelpPopup                                                           */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int OpenHelpPopup(HandlerInfo_t info)
{
    helppage = 0;
    BuildHelpForm(helpwin, helppage);
    XMapWindow(display, helpwin);
    return 0;
}

/***************************************************************************/
/* BuildHelpForm                                                           */
/* Arguments :                                                             */
/*   win                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
void BuildHelpForm(Window win, int helppage)
{
    T_FormClear(win);
    T_FormButton(win, "close_help", 10, HELP_HEIGHT - 10 - HELP_BTN_HEIGHT,
		 HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Close",
		 FormCloseHandler);
    T_FormButton(win, "next_help", 20 + HELP_BTN_WIDTH,
		 HELP_HEIGHT - 10 - HELP_BTN_HEIGHT, HELP_BTN_WIDTH,
		 HELP_BTN_HEIGHT, "Next", NextHelp);
    T_FormButton(win, "prev_help", 30 + 2 * HELP_BTN_WIDTH,
		 HELP_HEIGHT - 10 - HELP_BTN_HEIGHT, HELP_BTN_WIDTH,
		 HELP_BTN_HEIGHT, "Prev", PrevHelp);

    switch (helppage) {

    case 0:
	helpsel = 3;
	T_FormMultiButton(win, "help_icon_select", 10, 10, HELP_WIDTH * .3,
			  (int) (HELP_WIDTH * .3 * .8 * 1.75), 5, 7, "",
			  &helpsel, 1);
	break;

    case 1:
	helpsel = 1;
	T_FormMultiButton(win, "help_mode_select", 10,
			  10 + HELP_BTN_HEIGHT, HELP_WIDTH * .3,
			  HELP_BTN_HEIGHT, 3, 1, "Draw;Line;Select",
			  &helpsel, 0);
	T_FormButton(win, "help_zoom_in",
		     (int) (HELP_WIDTH * .65 - 10 - HELP_BTN_WIDTH),
		     (int) (100 + 3 * HELP_BTN_HEIGHT + .2 * HELP_WIDTH),
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Z", NULL);
	T_FormButton(win, "help_zoom_out", (int) (HELP_WIDTH * .65 + 10),
		     (int) (100 + 3 * HELP_BTN_HEIGHT + .2 * HELP_WIDTH),
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "z", NULL);
	T_FormButton(win, "help_prefs", 10,
		     (int) (120 + 3 * HELP_BTN_HEIGHT + .3 * HELP_WIDTH),
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Prefs", NULL);
	T_FormButton(win, "help_help", 10,
		     (int) (200 + 4 * HELP_BTN_HEIGHT + .3 * HELP_WIDTH),
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Help", NULL);
	break;

    case 2:
	T_FormButton(win, "help_load", 10, 10,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Load", NULL);
	T_FormButton(win, "help_save", 10, 40 + HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Save", NULL);
	T_FormButton(win, "help_new", 10, 70 + 2 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "New", NULL);
	T_FormButton(win, "help_quit", 10, 100 + 3 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Quit", NULL);
	T_FormButton(win, "help_cut", 10, 170 + 4 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Cut", NULL);
	T_FormButton(win, "help_copy", 10, 200 + 5 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Copy", NULL);
	T_FormButton(win, "help_paste", 10, 230 + 6 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Paste", NULL);
	T_FormButton(win, "help_undo", 10, 260 + 7 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Undo", NULL);
	break;

    case 3:
	T_FormButton(win, "help_round", 10, 10,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Round", NULL);
	T_FormButton(win, "help_fill", 10, 40 + HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Fill", NULL);
	T_FormButton(win, "help_grow", 10, 70 + 2 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Grow", NULL);
	T_FormButton(win, "help_neg", 10, 100 + 3 * HELP_BTN_HEIGHT,
		     HELP_BTN_WIDTH, HELP_BTN_HEIGHT, "Neg.", NULL);
	break;
    }
}

int NextHelp(HandlerInfo_t info)
{
    XEvent report;

    helppage++;
    if (helppage > helpscreens) {
	helppage = 0;
    }
    BuildHelpForm(helpwin, helppage);
    T_ClearArea(helpwin, 0, 0, HELP_WIDTH, HELP_HEIGHT);
    report.type = Expose;
    report.xexpose.count = 0;
    report.xexpose.window = helpwin;
    T_FormExpose(&report);
    DrawHelpWin();
    return 0;
}

int PrevHelp(HandlerInfo_t info)
{
    XEvent report;

    helppage--;
    if (helppage < 0) {
	helppage = helpscreens;
    }
    BuildHelpForm(helpwin, helppage);
    T_ClearArea(helpwin, 0, 0, HELP_WIDTH, HELP_HEIGHT);
    report.type = Expose;
    report.xexpose.count = 0;
    report.xexpose.window = helpwin;
    T_FormExpose(&report);
    DrawHelpWin();
    return 0;
}

/***************************************************************************/
/* DrawHelpWin                                                             */
/* Arguments :                                                             */
/*   win                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
void DrawHelpWin(void)
{
    int i, j, w, sel;

    switch (helppage) {

    case 0:
	w = (HELP_WIDTH * .3) / 5;
	for (i = 0; i < 7; i++) {
	    for (j = 0; j < 5; j++) {
		sel = i * 5 + j + 1;
		if (sel == helpsel)
		    DrawMapPic(helpwin, 14 + j * w, 14 + i * w,
			       mapicon_ptr[iconmenu[sel] - 32], w - 7);
		else
		    DrawMapPic(helpwin, 13 + j * w, 13 + i * w,
			       mapicon_ptr[iconmenu[sel] - 32], w - 7);
	    }
	}
	for (i = 0; i < 35; i++) {
	    j = i / 12;
	    DrawMapPic(helpwin, (int) 10 + j * ((HELP_WIDTH - 30) / 3),
		       (int) (30 +
			      (HELP_WIDTH * .17 / 5 + 10) * (i - j * 12)) +
		       (HELP_WIDTH * .5 / 5 * 4),
		       mapicon_ptr[iconhelp[i + 1] - 32], w - 7);
	    T_DrawString(helpwin, 20 + j * ((HELP_WIDTH - 30) / 3) + w,
			 (int) (25 +
				(HELP_WIDTH * .17 / 5 + 10) * (i -
							       j * 12)) +
			 (HELP_WIDTH * .5 / 5 * 4),
			 (int) ((HELP_WIDTH - 20) / 3), w, BKGR,
			 iconlabel[i + 1], JUSTIFY_LEFT, CROP_RIGHT, -1);
	}
	T_DrawText(helpwin, (int) (HELP_WIDTH * .3 + 20),
		   (int) (10 + HELP_WIDTH * .03),
		   (int) (HELP_WIDTH * .7 - 20),
		   (int) (HELP_WIDTH * .3 / 5 * 4), BKGR,
		   "Select a map icon to draw with from the buttons at the top of the tool panel. The selected icon will also be used in the line and fill modes.\n\nThe map icon buttons may be turned off to draw empty spaces or the second mouse button can be used.");
	T_DrawText(helpwin, 10,
		   (int) (150 +
			  (HELP_WIDTH * .3 / 5 * 4 +
			   (HELP_WIDTH * .3 / 5 + 10) * 7)),
		   HELP_WIDTH - 20,
		   (int) (HELP_HEIGHT - (30 + (HELP_WIDTH * .3 / 5 * 11))),
		   BKGR,
		   "You can draw numbered bases and checkpoints by entering 0 through 9 and A through Z on the keyboard while the pointer is in the map region.");
	break;

    case 1:
	T_DrawText(helpwin, (int) (HELP_WIDTH * .3 + 20), 10,
		   (int) (HELP_WIDTH * .7 - 20), HELP_BTN_HEIGHT * 3, BKGR,
		   "The three button set below the map icon buttons determines what mode you are in.\n\nWhile in draw and line mode, the first button draws the selected map icon and the second button draws space.");
	T_DrawButton(helpwin, 10, 80 + 3 * HELP_BTN_HEIGHT,
		     HELP_WIDTH * .3, HELP_WIDTH * .3, LOWERED, 0);
	XFillRectangle(display, helpwin, Black_GC, 15,
		       85 + 3 * HELP_BTN_HEIGHT, HELP_WIDTH * .3 - 10,
		       HELP_WIDTH * .3 - 10);
	T_DrawText(helpwin, (int) (HELP_WIDTH * .3 + 20),
		   100 + 3 * HELP_BTN_HEIGHT, (int) (HELP_WIDTH * .7 - 20),
		   HELP_WIDTH * .3, BKGR,
		   "To move the map view around, press a button and drag the cursor in the small map area.\n\n\nUse the Z and z buttons to zoom in and out. If an area is selected, it will be centered in the view.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH,
		   (int) (120 + 3 * HELP_BTN_HEIGHT + .3 * HELP_WIDTH),
		   HELP_WIDTH - 30 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT + 80,
		   BKGR,
		   "The preferences popup allows you to save parameters that will change the behavior of the game. Only non-empty selections are saved in the map file for the fields and the yes/no buttons.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH,
		   (int) (200 + 4 * HELP_BTN_HEIGHT + .3 * HELP_WIDTH),
		   HELP_WIDTH - 30 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT + 80,
		   BKGR, "The Help button displays the help screens.");
	break;

    case 2:
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 10,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "Load a map or .xbm file.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 40 + HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "Save the current map.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 70 + 2 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "Create a new, empty map.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 100 + 3 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "Quit the map editor.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 170 + 4 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "While in select mode, the Cut button removes the selected map area and places the non-space icons into the cut buffer.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 200 + 5 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "While in select mode, the Copy button places the non-space icons from the selected map area into the cut buffer.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 230 + 6 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "While in select mode, the Paste button fills the selected area with icons from the cut-buffer. Only the selected area of the map will be affected and spaces from the cut buffer will not be copied.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 260 + 7 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "The Undo button reverts the map to the state it was before the last operation. You may undo successive operations until the beginning of the map editor session.");
	break;

    case 3:
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 10,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "While in select mode, the Round button attempts to round the selected area by adding and deleting corner icons. If no area is selected the entire view area is rounded.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 40 + HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "While in select mode, the currently selected area is filled with the selected map icon.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 70 + 2 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "Hold down the Grow button to grow from existing blocks. If no blocks are drawn in the selected area, blocks will grow from the center in proportion to the selected width and height.");
	T_DrawText(helpwin, 20 + HELP_BTN_WIDTH, 100 + 3 * HELP_BTN_HEIGHT,
		   HELP_WIDTH - 20 - HELP_BTN_WIDTH, HELP_BTN_HEIGHT, BKGR,
		   "The Neg. button reverses the filled blocks and spaces.");
	T_DrawText(helpwin, 10, 300 + 3 * HELP_BTN_HEIGHT, HELP_WIDTH - 20,
		   100, BKGR,
		   "XP-Mapedit was originally written by Aaron Averill\nPlease send any bugs, patches, enhancements or comments to the current maintainer at:\n\n" PACKAGE_BUGREPORT);
    }

}
