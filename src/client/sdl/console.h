/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "xpclient_sdl.h"
#include "renderer.h"

/**
 * Initialize the in-game console for an SDL window.
 *
 * @param window Window that receives console text input.
 * @return Zero on success, or -1 when initialization fails.
 */
int Console_init(SDL_Window *window);
/**
 * Update console animation and publish dirty pixels to the renderer.
 *
 * @param renderer Renderer that owns the console texture.
 * @return Zero on success, or -1 when the console cannot be prepared.
 *
 * @remarks This function must be called outside an active renderer frame.
 */
int Console_prepare(Renderer *renderer);
/**
 * Draw the visible console and its frame in the active renderer frame.
 *
 * @return The renderer status, or RENDERER_STATUS_OK when hidden.
 *
 * @remarks A failure is retained by the SDL renderer and the containing
 * frame must not be presented.
 */
RendererStatus Console_paint(void);
void Console_show(void);
void Console_hide(void);
int Console_isVisible(void);
int Console_process(SDL_Event *e);
void Console_cleanup(void);
void Console_print(const char *str, ...);
/** Add printable ASCII clipboard text to the active console command. */
void Paste_String_to_Console(const char *text);
#endif
