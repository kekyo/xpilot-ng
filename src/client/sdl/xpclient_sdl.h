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

#ifndef XPCLIENT_SDL_H
#define XPCLIENT_SDL_H

#include "xpclient.h"
#ifdef _WINDOWS
# include <windows.h>
#endif

/* determine if we use usual style including */
#ifndef MACOSX_FRAMEWORKS
# define USUAL_SDL_INCLUDE_CONVENTION 1
#endif

#ifdef USUAL_SDL_INCLUDE_CONVENTION
# include <GL/gl.h>
# include <GL/glu.h>
# include "SDL.h"
# include "SDL_events.h"
# include "SDL_video.h"
# include "SDL_version.h"
# include "SDL_syswm.h"
# ifdef HAVE_SDL_IMAGE
#  include "SDL_image.h"
# endif
# include "SDL_ttf.h"
#endif

#ifdef MACOSX_FRAMEWORKS
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <SDL/SDL.h>
# include <SDL/SDL_events.h>
# include <SDL/SDL_video.h>
# include <SDL/SDL_version.h>
# include <SDL/SDL_syswm.h>
# ifdef HAVE_SDL_IMAGE
#  include <SDL_image/SDL_image.h>
# endif
# include <SDL_ttf/SDL_ttf.h>
#endif

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff
#else
#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#endif

extern void Game_loop(void);
extern void Options_cleanup(void);
extern void Store_sdlinit_options(void);
extern void Store_sdlgui_options(void);
extern void Store_radar_options(void);

#endif /* XPCLIENT_SDL_H */
