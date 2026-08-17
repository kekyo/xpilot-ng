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

#ifndef SDLKEYS_H
#define SDLKEYS_H

#include "xpclient_sdl.h"

#define NUM_MOUSE_BUTTONS 5

/**
 * Convert an XPilot key name to an SDL3 keycode.
 *
 * @param name Key name from the XPilot configuration.
 * @return The matching keycode, or SDLK_UNKNOWN.
 */
SDL_Keycode Get_key_by_name(const char *name);

/**
 * Find the canonical XPilot name for an SDL3 keycode.
 *
 * @param key SDL3 keycode to look up.
 * @return Static key name, or NULL when the key is unknown.
 */
const char *Get_name_by_key(SDL_Keycode key);

#endif
