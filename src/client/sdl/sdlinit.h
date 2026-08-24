/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Darel Cullen <darelcullen@users.sourceforge.net>
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

#ifndef SDLINIT_H
#define SDLINIT_H

#include "sdlrenderer.h"
#include "native_text.h"

/**
 * Initialize SDL3, the application window, and an OpenGL 3.3 core context.
 *
 * @return 0 on success, or -1 when initialization fails.
 */
int Init_window(void);

/**
 * Return the renderer bound to the application OpenGL context.
 *
 * @return Borrowed renderer facade, or NULL before initialization and after
 * cleanup.
 */
SdlRenderer *Get_sdl_renderer(void);

/**
 * Return the shared native text service.
 *
 * @return Borrowed service, or NULL outside window-system lifetime.
 */
XpTextSystem *Get_text_system(void);

/**
 * Request a new application window size.
 *
 * @param width Requested width in screen coordinates.
 * @param height Requested height in screen coordinates.
 * @return 0 on success, or -1 when SDL cannot apply the size.
 */
int Resize_Window(int width, int height);

/**
 * Apply a size reported by SDL without requesting another resize.
 *
 * @param width Current SDL window width.
 * @param height Current SDL window height.
 */
void Window_size_changed(int width, int height);

/** Swap the OpenGL buffers of the application window. */
void Swap_buffers(void);

/**
 * Grab or release the application window for pointer control.
 *
 * @param on true to grab the window, false to release it.
 */
void Set_window_grab(bool on);

/**
 * Enable or disable relative pointer motion for the application window.
 *
 * @param on true to enable relative motion, false to disable it.
 * @return true on success, or false when SDL rejects the request.
 */
bool Set_relative_mouse_mode(bool on);

#endif
