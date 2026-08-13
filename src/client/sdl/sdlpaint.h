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

/**
 * Draw the SDL HUD and report semantic renderer failures.
 *
 * @return The sticky result for the active renderer frame.
 * @remarks HUD drawing uses semantic renderer operations. The generic
 *          Paint_HUD() entry point remains available as a void wrapper for
 *          the shared client paint interface.
 */
extern RendererStatus Paint_HUD_checked(void);

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

/**
 * Draw one semantic HUD radar dot for tests.
 *
 * @param x Horizontal center in inherited HUD coordinates.
 * @param y Vertical center in inherited HUD coordinates.
 * @param color Packed 0xRRGGBBAA color.
 * @param shape HUD radar shape number in the inclusive range 2 through 7.
 * @param size Signed shape radius; zero suppresses drawing.
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when the dot is suppressed.
 * @remarks This hook preserves the renderer's current transform and scissor
 *          state and is available only to the SDL GUI primitive test target.
 */
extern RendererStatus Sdlgui_test_paint_hud_radar_dot(
    int x, int y, Uint32 color, int shape, int size);

/**
 * Draw the semantic two-pass HUD radar and message scan rings for tests.
 *
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when no radar dot or scan ring is visible.
 * @remarks This hook preserves the renderer's current transform and scissor
 *          state and is available only to the SDL GUI primitive test target.
 */
extern RendererStatus Sdlgui_test_paint_hud_radar_and_scans(void);

/**
 * Draw only the semantic dashed horizontal and vertical HUD frame lines.
 *
 * @param hud_pos_x Horizontal HUD center in inherited HUD coordinates.
 * @param hud_pos_y Vertical HUD center in inherited HUD coordinates.
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when both frame colors are disabled.
 * @remarks This hook exercises the production HUD frame visibility and
 *          geometry while preserving the renderer's current transform and
 *          scissor state. It is available only to the SDL GUI primitive test
 *          target.
 */
extern RendererStatus Sdlgui_test_paint_hud_frame(
    int hud_pos_x, int hud_pos_y);

/**
 * Paint the real HUD item list, including the semantic lose-item outline.
 *
 * @param hud_pos_x Horizontal HUD center in inherited HUD coordinates.
 * @param hud_pos_y Vertical HUD center in inherited HUD coordinates.
 * @return The sticky result for the active renderer frame.
 * @remarks This hook exercises the production item visibility, state-update,
 *          image, outline, measurement, and text path. It is available only
 *          to the SDL GUI primitive test target.
 */
extern RendererStatus Sdlgui_test_paint_hud_items(
    int hud_pos_x, int hud_pos_y);

/**
 * Paint the semantic HUD fuel gauge when its production visibility rule
 * selects it.
 *
 * @param hud_pos_x Horizontal HUD center in inherited HUD coordinates.
 * @param hud_pos_y Vertical HUD center in inherited HUD coordinates.
 * @return The sticky result for the active renderer frame, or
 *         RENDERER_STATUS_OK when the gauge is hidden.
 * @remarks This hook preserves the renderer's current transform and scissor
 *          state and is available only to the SDL GUI primitive test target.
 */
extern RendererStatus Sdlgui_test_paint_hud_fuel_gauge(
    int hud_pos_x, int hud_pos_y);
#endif

#endif
