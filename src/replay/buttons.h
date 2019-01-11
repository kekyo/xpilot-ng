/* 
 * XP-Replay, playback an XPilot session.  Copyright (C) 1994-98 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Steven Singer        (S.Singer@ph.surrey.ac.uk)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERC_HANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

union button_image {
    const char		*string;
    Pixmap		icon;
};

typedef struct button *Button;

#define BUTTON_PRESSED	1	/* Button is currently pressed in */
#define BUTTON_RELEASE	2	/* Button pops out when mouse button released */
#define BUTTON_DISABLED	4	/* Button is disabled */
#define BUTTON_TEXT	8	/* Button has text on, rather than bitmap */

/*
 * If a button is marked as BUTTON_RELEASE then callback action is taken when
 * the button is released. Otherwise it is taken when the button is pressed
 */

void SetGlobalButtonAttributes(unsigned long, unsigned long,
			       unsigned long, unsigned long);

Button CreateButton(Display *, Window, int, int, unsigned int, unsigned int,
		    union button_image, unsigned int, unsigned int,
		    unsigned long, void (*)(void *), void *, int, int);
int CheckButtonEvent(XEvent *);
void RedrawButton(Button);
void EnableButton(Button);
void DisableButton(Button);
void ReleaseableButton(Button);
void NonreleaseableButton(Button);
void ChangeButtonGroup(Button, int);
void MoveButton(Button, int, int);
void GetButtonSize(Button, unsigned *, unsigned *);

#endif
