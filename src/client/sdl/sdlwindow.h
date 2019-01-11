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

#ifndef SDLWINDOW_H
#define SDLWINDOW_H

#include "xpclient_sdl.h"

typedef struct {
    GLuint tx_id;
    SDL_Surface *surface;
    int x, y, w, h;
} sdl_window_t;

int sdl_window_init(sdl_window_t *win, int x, int y, int w, int h);
void sdl_window_move(sdl_window_t *win, int x, int y);
int sdl_window_resize(sdl_window_t *win, int w, int h);
void sdl_window_refresh(sdl_window_t *win);
void sdl_window_paint(sdl_window_t *win);
void sdl_window_destroy(sdl_window_t *win);

#endif
