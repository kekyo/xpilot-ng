/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
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

#ifndef SDLMETA_H
#define SDLMETA_H

#include "xpclient_sdl.h"

int Meta_window(Connect_param_t *conpar);

#ifdef XPILOT_SDLMETA_TEST_HOOKS
/**
 * Paint a player-list background through the production widget painter.
 *
 * @param bounds Widget bounds in logical top-left coordinates.
 */
void Sdl_meta_test_paint_player_list(SDL_Rect bounds);

/**
 * Paint a metadata row through the production widget painter.
 *
 * @param bounds Widget bounds in logical top-left coordinates.
 * @param background Packed 0xRRGGBBAA background for an unselected row.
 * @param selected Whether to use the selected-row background instead.
 */
void Sdl_meta_test_paint_row(SDL_Rect bounds, Uint32 background,
                             bool selected);
#endif

#endif
