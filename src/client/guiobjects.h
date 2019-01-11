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

#ifndef GUIOBJECTS_H
#define GUIOBJECTS_H

void Gui_paint_item_object(int type, int x, int y);
void Gui_paint_ball(int x, int y, int style);
void Gui_paint_ball_connector(int x_1, int y_1, int x_2, int y_2);

void Gui_paint_mine(int x, int y, int teammine, char *name);

void Gui_paint_spark(int color, int x, int y);

void Gui_paint_wreck(int x, int y, bool deadly, int wtype, int rot, int size);

void Gui_paint_asteroids_begin(void);
void Gui_paint_asteroids_end(void);
void Gui_paint_asteroid(int x, int y, int type, int rot, int size);

void Gui_paint_fastshot(int color, int x, int y);
void Gui_paint_teamshot(int x, int y);


void Gui_paint_missiles_begin(void);
void Gui_paint_missiles_end(void);
void Gui_paint_missile(int x, int y, int len, int dir);

void Gui_paint_lasers_begin(void);
void Gui_paint_lasers_end(void);
void Gui_paint_laser(int color, int x_1, int y_1, int len, int dir);

void Gui_paint_paused(int x, int y, int count);
void Gui_paint_appearing(int x, int y, int id, int count);

void Gui_paint_ecm(int x, int y, int size);

void Gui_paint_refuel(int x_0, int y_0, int x_1, int y_1);

void Gui_paint_connector(int x_0, int y_0, int x_1, int y_1, int tractor);

void Gui_paint_transporter(int x_0, int y_0, int x_1, int y_1);

void Gui_paint_all_connectors_begin(void);

void Gui_paint_ships_begin(void);
void Gui_paint_ships_end(void);

void Gui_paint_ship(int x, int y, int dir, int id, int cloak, int phased,
		    int shield, int deflector, int eshield);

void Store_guiobject_options(void);

#endif

