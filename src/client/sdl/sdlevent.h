/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDLEVENT_H
#define SDLEVENT_H

#include "xpclient_sdl.h"

/**
 * Translate one SDL2 event into the corresponding XPilot action.
 *
 * @param evt Event to process.
 * @return Non-zero after processing the event.
 */
int Process_event(SDL_Event *evt);

#endif
