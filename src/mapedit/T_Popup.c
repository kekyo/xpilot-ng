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

T_Popup_t *T_Popup = NULL;

/***************************************************************************/
/* T_PopupCreate                                                           */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/*   width                                                                 */
/*   height                                                                */
/*   title                                                                 */
/* Purpose :                                                               */
/***************************************************************************/
Window T_PopupCreate(int x, int y, int width, int height, char *title)
{
    T_Popup_t **popup;

    popup = &T_Popup;
    while ((*popup) != NULL) {
	popup = (T_Popup_t **) & ((*popup)->next);
    }

    /* add a popup window to stack */
    (*popup) = (T_Popup_t *) malloc(sizeof(T_Popup_t));
    (*popup)->next = NULL;
    if (x < 0)
	x = (root_width - width) / 2;
    if (y < 0)
	y = (root_height - height) / 2;
    (*popup)->window =
	T_MakeWindow(x, y, width, height, T_Background, T_Foreground);
    XSelectInput(display, (*popup)->window,
		 ExposureMask | ButtonPressMask | KeyPressMask |
		 ButtonReleaseMask | PointerMotionMask |
		 StructureNotifyMask);
    T_SetWindowName((*popup)->window, title, title);
    return (*popup)->window;
}

/***************************************************************************/
/* T_PopupAlert                                                            */
/* Arguments :                                                             */
/*   type : 1 = OK button, 2 = OK, Cancel buttons                          */
/*   message                                                               */
/*   btn1                                                                  */
/*   btn2                                                                  */
/*   function                                                              */
/* Purpose :                                                               */
/***************************************************************************/
Window T_PopupAlert(int type, char *message, char *btn1, char *btn2,
		    handler_t handler1, handler_t handler2)
{
    int x, y, width, height;
    Window win;

    width = XTextWidth(T_Font, message, strlen(message)) + 40;
    height = POPUPBTNHEIGHT * 3 + 30;
    win = T_PopupCreate(-1, -1, width, height, "Alert");

    x = (width - POPUPBTNWIDTH) / 2;
    if (type == 1) {
	T_FormButton(win, "popup_btn", x, POPUPBTNHEIGHT * 2 + 20,
		     POPUPBTNWIDTH, POPUPBTNHEIGHT, "Ok",
		     PopupCloseHandler);
    } else if (type == 2) {
	if (btn1 == NULL) {
	    btn1 = malloc(3);
	    strcpy(btn1, "Ok");
	}
	if (btn2 == NULL) {
	    btn2 = malloc(7);
	    strcpy(btn2, "Cancel");
	}
	if (handler2 == NULL) {
	    handler2 = PopupCloseHandler;
	}
	x = width / 2 - POPUPBTNWIDTH - 5;
	y = height - POPUPBTNHEIGHT - 5;
	T_FormButton(win, "popup_btn1", x, y, POPUPBTNWIDTH,
		     POPUPBTNHEIGHT, btn1, handler1);
	T_FormButton(win, "popup_btn2", x + 10 + POPUPBTNWIDTH,
		     y, POPUPBTNWIDTH, POPUPBTNHEIGHT, btn2, handler2);
    }
    T_FormText(win, "popup_alert", 10, 10, width - 20, POPUPBTNHEIGHT,
	       message, JUSTIFY_CENTER);

    XMapWindow(display, win);
    return win;
}

/***************************************************************************/
/* Window T_PopupPrompt                                                    */
/* Arguments :                                                             */
/*   x                                                                     */
/*   y                                                                     */
/*   width                                                                 */
/*   height                                                                */
/*   title                                                                 */
/*   message                                                               */
/*   btn1                                                                  */
/*   btn2                                                                  */
/*   variable                                                              */
/*   length                                                                */
/*   function                                                              */
/* Purpose :                                                               */
/***************************************************************************/
Window T_PopupPrompt(int x, int y, int width, int height, char *title,
		     char *message, char *btn1, char *btn2, char *charvar,
		     int length, handler_t handler)
{
    int x2, y2, x3, y3;
    Window win;
    XEvent report;

    win = T_PopupCreate(x, y, width, height, title);

    if (btn1 == NULL) {
	btn1 = (char *) malloc(3);
	strcpy(btn1, "Ok");
    }
    if (btn2 == NULL) {
	btn2 = (char *) malloc(7);
	strcpy(btn2, "Cancel");
    }
    x2 = width / 2 - POPUPBTNWIDTH - 5;
    y2 = height - POPUPBTNHEIGHT - 5;
    T_FormButton(win, "popup_btn1", x2, y2, POPUPBTNWIDTH, POPUPBTNHEIGHT,
		 btn1, handler);
    T_FormButton(win, "popup_btn2", x2 + 10 + POPUPBTNWIDTH, y2,
		 POPUPBTNWIDTH, POPUPBTNHEIGHT, btn2, PopupCloseHandler);
    x2 = 5;
    y2 = height - POPUPBTNHEIGHT * 2 - 10;
    y3 = -(y2 + POPUPBTNHEIGHT) / 2;
    x3 = width / 2 - XTextWidth(T_Font, message, strlen(message)) / 2;
    if (x2 < 0)
	x2 = 0;
    T_FormStringEntry(win, "popup_prompt", x2, y2, width - 10,
		      POPUPBTNHEIGHT, x3, y3, message, charvar, length,
		      handler);

    XMapWindow(display, win);
    report.type = ButtonPress;
    report.xbutton.window = win;
    report.xbutton.button = Button1;
    report.xbutton.x = x2 + 1;
    report.xbutton.y = y2 + 1;
    T_FormButtonPress(&report);

    return win;
}

/***************************************************************************/
/* T_IsPopupOpen                                                           */
/* Arguments :                                                             */
/*   win                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
int T_IsPopupOpen(Window win)
{
    T_Popup_t *popup;

    popup = T_Popup;
    while (popup != NULL) {
	if (popup->window == win)
	    return 1;
	else
	    popup = (T_Popup_t *) popup->next;
    }
    return 0;
}

/***************************************************************************/
/* T_PopupClose                                                            */
/* Arguments :                                                             */
/*   win                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
void T_PopupClose(Window win)
{
    T_Popup_t **popup;
    T_Popup_t *last;

    popup = &T_Popup;
    while (((*popup) != NULL) && ((*popup)->window != win)) {
	popup = (T_Popup_t **) & ((*popup)->next);
    }

    if ((*popup) == NULL)
	return;

    /* erase this popup */
    T_FormCloseWindow(win);
    last = (*popup);
    (*popup) = (T_Popup_t *) (*popup)->next;
    free(last);

}
