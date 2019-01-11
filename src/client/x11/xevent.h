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

#ifndef XEVENT_H
#define XEVENT_H

extern int		talk_key_repeating;
extern XEvent		talk_key_repeat_event;
extern struct timeval	talk_key_repeat_time;

extern ipos_t		mousePosition;	/* position of mouse pointer. */
extern int		mouseMovement;	/* horizontal mouse movement. */

bool Key_binding_callback(keys_t key, const char *str);
keys_t Lookup_key(XEvent *event, KeySym ks, bool reset);
void Key_event(XEvent *event);
void Talk_event(XEvent *event);
void xevent_keyboard(int queued);
void xevent_pointer(void);
int x_event(int new_input);

#endif
