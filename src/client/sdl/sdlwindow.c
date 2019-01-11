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

#include "xpclient_sdl.h"

#include "sdlwindow.h"
#include "error.h"

static int next_p2(int t) 
{
    int r = 1;
    while (r < t) r <<= 1;
    return r;   
}

int sdl_window_init(sdl_window_t *win, int x, int y, int w, int h)
{
    glGenTextures(1, &win->tx_id);
    win->surface = NULL;
    if (sdl_window_resize(win, w, h)) {
	warn("failed to resize window");
	return -1;
    }
    sdl_window_move(win, x, y);
    return 0;
}

void sdl_window_move(sdl_window_t *win, int x, int y)
{
    win->x = x;
    win->y = y;
}

int sdl_window_resize(sdl_window_t *win, int width, int height)
{
    SDL_Surface *surface = 
	SDL_CreateRGBSurface(SDL_SWSURFACE, 
			     next_p2(width), next_p2(height), 
			     32, RMASK, GMASK, BMASK, AMASK);
    if (!surface) {
	error("failed to create SDL surface: %s", SDL_GetError());
	return -1;
    }

    if (win->surface != NULL) {
	SDL_FreeSurface(win->surface);
    }

    win->surface = surface;
    win->w = width;
    win->h = height;
    return 0;
}

void sdl_window_refresh(sdl_window_t *win)
{
    glBindTexture(GL_TEXTURE_2D, win->tx_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
		 win->surface->w, win->surface->h, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 
		 win->surface->pixels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                    GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                    GL_NEAREST);
}

void sdl_window_paint(sdl_window_t *win)
{
    glBindTexture(GL_TEXTURE_2D, win->tx_id);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255, 255, 255, 255);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); 
    glVertex2i(win->x, win->y);
    glTexCoord2f((GLfloat)win->w / win->surface->w, 0); 
    glVertex2i(win->x + win->w , win->y);
    glTexCoord2f((GLfloat)win->w / win->surface->w, 
		 (GLfloat)win->h / win->surface->h);
    glVertex2i(win->x + win->w , win->y + win->h);
    glTexCoord2f(0, (GLfloat)win->h / win->surface->h);
    glVertex2i(win->x, win->y + win->h);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void sdl_window_destroy(sdl_window_t *win)
{
    glDeleteTextures(1, &win->tx_id);
    if (win->surface != NULL)
	SDL_FreeSurface(win->surface);
}

