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

#ifndef SDLPAINT_H
#define SDLPAINT_H

#include "xpclient_sdl.h"

#include "renderer.h"

#define MAX_VERTICES 10000

typedef unsigned int color_t;

#define STATIONARY_MODE 1
#define MOVING_MODE 	2
#define HUD_MODE    	4

extern int paintSetupMode;

void setupPaint_stationary(void);
void setupPaint_moving(void);
void setupPaint_HUD(void);

/* helper function in sdlgui.c, some may be removed soon */
extern Uint32 nullRGBA;
extern Uint32 blackRGBA;
extern Uint32 whiteRGBA;
extern Uint32 blueRGBA;
extern Uint32 redRGBA;
extern Uint32 greenRGBA;
extern Uint32 yellowRGBA;

extern Uint32 scoreInactiveSelfColorRGBA;
extern Uint32 scoreInactiveColorRGBA;
extern Uint32 scoreSelfColorRGBA;
extern Uint32 scoreColorRGBA;
extern Uint32 scoreOwnTeamColorRGBA;
extern Uint32 scoreEnemyTeamColorRGBA;

extern int GL_X(int x);
extern int GL_Y(int y);
extern void set_alphacolor(Uint32 color);
extern void Circle(Uint32 color, int x, int y, int radius, int filled);

extern irec_t *select_bounds;

/**
 * Draw the current selection bounds as a semantic HUD outline.
 *
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when select_bounds is NULL.
 * @remarks The operation preserves the renderer's current transform and
 *          scissor state. It requires an active renderer frame whenever
 *          select_bounds is non-NULL.
 */
extern RendererStatus Paint_select(void);

#ifdef XPILOT_SDLGUI_TEST_HOOKS
/**
 * Draw only the semantic HUD speed and direction pointers for tests.
 *
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when neither pointer is visible.
 * @remarks This hook preserves the renderer's current transform and scissor
 *          state and is available only to the SDL GUI primitive test target.
 */
extern RendererStatus Sdlgui_test_paint_hud_pointers(void);
#endif

#endif
