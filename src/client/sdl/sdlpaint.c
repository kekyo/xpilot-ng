/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 by 
 *
 *      Juha Lindström       <juhal@users.sourceforge.net>
 *      Erik Andersson       <deity_at_home.se>
 *      Darel Cullen         <darelcullen@users.sourceforge.net>
 *
 * Copyright (C) 1991-2002 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
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

#include "surface_primitives.h"
#include "sdlcompat.h"
#include "sdlinit.h"
#include "sdlpaint.h"
#include "images.h"
#include "console.h"
#include "radar.h"
#include "sdlwindow.h"
#include "text.h"
#include "glwidgets.h"
#include "radar.h"
#include "gl_diagnostics.h"
#include "paint_transform.h"
#include "sdlgameframe.h"
#include "sdlrenderer.h"
#include "score_font.h"
#include "text_config.h"
#include "utf8_names.h"

#include <limits.h>
#include <stdio.h>

#define SCORE_BORDER 5

/*
 * Globals.
 */
static ScoreFont     scoreListFont;
static sdl_window_t scoreListWin;
static SDL_Rect     scoreEntryRect; /* Bounds for the last painted score entry */
static GLWidget     *scoreListWidget;
static bool         scoreListMoving;
static SdlGameFrameState rendererFrameState;
static SdlUiDrawState gameUiDrawState;
static bool gameFrameReadyReported;

typedef enum PaintSetupTarget {
    PAINT_SETUP_TARGET_STATIONARY,
    PAINT_SETUP_TARGET_MOVING,
    PAINT_SETUP_TARGET_HUD
} PaintSetupTarget;

typedef void (*PaintStageCallback)(void *context);

typedef struct PaintStageOps {
    PaintStageCallback paint_world;
    PaintStageCallback paint_vfuel;
    PaintStageCallback paint_vdecor;
    PaintStageCallback paint_vcannon;
    PaintStageCallback paint_vbase;
    PaintStageCallback paint_objects;
    PaintStageCallback paint_score_objects;
    PaintStageCallback paint_shots;
    PaintStageCallback setup_moving;
    PaintStageCallback paint_ships;
    RendererStatus (*frame_result)(void *context);
} PaintStageOps;

int paintSetupMode;

GLWidget *MainWidget = NULL;

static RendererStatus set_renderer_world_transform_raw(
    SdlRenderer *sdl_renderer, int rounded_translation)
{
    Renderer *renderer;
    RendererTransform2D transform;
    RendererStatus status;
    int drawable_width;
    int drawable_height;

    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Sdl_renderer_get_drawable_size(
	sdl_renderer, &drawable_width, &drawable_height);
    if (status == RENDERER_STATUS_OK
	&& Paint_transform_world(clData.scale, world.x, world.y,
				 draw_width, draw_height,
				 drawable_width, drawable_height,
				 rounded_translation, &transform) != 0) {
	status = RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (status == RENDERER_STATUS_OK) {
	renderer = Sdl_renderer_frontend(sdl_renderer);
	if (renderer == NULL)
	    status = RENDERER_STATUS_INVALID_STATE;
	else
	    status = Renderer_set_transform_2d(renderer, transform);
    }
    return status;
}

static RendererStatus set_renderer_world_transform(int rounded_translation)
{
    SdlRenderer *sdl_renderer = Get_sdl_renderer();
    RendererStatus status;

    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Sdl_renderer_track_frame_result(
	sdl_renderer, RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
	return status;
    status = set_renderer_world_transform_raw(
	sdl_renderer, rounded_translation);
    return Sdl_renderer_track_frame_result(sdl_renderer, status);
}

static RendererStatus set_renderer_hud_transform(void)
{
    SdlRenderer *sdl_renderer = Get_sdl_renderer();
    RendererTransform2D transform;
    RendererStatus status;
    int drawable_width;
    int drawable_height;

    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Sdl_renderer_get_drawable_size(
	sdl_renderer, &drawable_width, &drawable_height);
    if (status == RENDERER_STATUS_OK
	&& Paint_transform_hud(draw_width, draw_height,
			       drawable_width, drawable_height,
			       &transform) != 0) {
	status = RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (status == RENDERER_STATUS_OK) {
	status = Sdl_renderer_set_transform_2d(sdl_renderer, transform);
    }
    if (status != RENDERER_STATUS_OK)
	return Sdl_renderer_track_frame_result(sdl_renderer, status);
    return RENDERER_STATUS_OK;
}

static void Begin_logical_paint_frame(void)
{
    paintSetupMode = 0;
}

static RendererStatus End_logical_paint_frame(void)
{
    paintSetupMode = 0;
    return Sdl_renderer_frame_result(Get_sdl_renderer());
}

static RendererStatus Commit_paint_setup(
    PaintSetupTarget target, RendererStatus semantic_status)
{
    if (target != PAINT_SETUP_TARGET_STATIONARY
	&& target != PAINT_SETUP_TARGET_MOVING
	&& target != PAINT_SETUP_TARGET_HUD) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (semantic_status != RENDERER_STATUS_OK)
	return semantic_status;

    if (target == PAINT_SETUP_TARGET_STATIONARY) {
	paintSetupMode = STATIONARY_MODE;
    } else if (target == PAINT_SETUP_TARGET_MOVING) {
	paintSetupMode = MOVING_MODE;
    } else {
	paintSetupMode = HUD_MODE;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Retain_stage_result(
    RendererStatus first_result, const PaintStageOps *ops, void *context)
{
    RendererStatus observed_result = ops->frame_result(context);

    if (first_result == RENDERER_STATUS_OK)
	return observed_result;
    return first_result;
}

static RendererStatus Run_paint_stage(
    PaintStageCallback callback, const PaintStageOps *ops, void *context,
    RendererStatus first_result)
{
    callback(context);
    return Retain_stage_result(first_result, ops, context);
}

static RendererStatus Run_world_object_stages(
    bool old_server, const PaintStageOps *ops, void *context)
{
    RendererStatus first_result;

    if (ops == NULL || ops->paint_world == NULL
	|| ops->paint_vfuel == NULL || ops->paint_vdecor == NULL
	|| ops->paint_vcannon == NULL || ops->paint_vbase == NULL
	|| ops->paint_objects == NULL || ops->paint_score_objects == NULL
	|| ops->paint_shots == NULL || ops->setup_moving == NULL
	|| ops->paint_ships == NULL || ops->frame_result == NULL) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    first_result = Retain_stage_result(
	RENDERER_STATUS_OK, ops, context);
    first_result = Run_paint_stage(
	ops->paint_world, ops, context, first_result);
    if (old_server) {
	first_result = Run_paint_stage(
	    ops->paint_vfuel, ops, context, first_result);
	first_result = Run_paint_stage(
	    ops->paint_vdecor, ops, context, first_result);
	first_result = Run_paint_stage(
	    ops->paint_vcannon, ops, context, first_result);
	first_result = Run_paint_stage(
	    ops->paint_vbase, ops, context, first_result);
    } else {
	first_result = Run_paint_stage(
	    ops->paint_objects, ops, context, first_result);
    }
    first_result = Run_paint_stage(
	ops->paint_score_objects, ops, context, first_result);
    first_result = Run_paint_stage(
	ops->paint_shots, ops, context, first_result);
    if (first_result == RENDERER_STATUS_OK) {
	first_result = Run_paint_stage(
	    ops->setup_moving, ops, context, first_result);
    }
    return Run_paint_stage(
	ops->paint_ships, ops, context, first_result);
}

static void Scorelist_button(Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data)
{
    GLWidget *widget = (GLWidget *)data;
    if (state == true) {
    	if (button == 1) {
	    scoreListMoving = true;
    	    if (DelGLWidgetListItem( widget->list, widget ))
	    	AppendGLWidgetList( widget->list, widget );
	}
    	if (button == 2) {
    	    if (DelGLWidgetListItem( widget->list, widget ))
	    	PrependGLWidgetList( widget->list, widget );
	}
    }
    
    if (state == false) {
    	if (button == 1)
	    scoreListMoving = false;
    }
}

static void Scorelist_move(Sint16 xrel, Sint16 yrel, Uint16 x, Uint16 y, void *data)
{
    if (scoreListMoving) {
	((GLWidget *)data)->bounds.x = scoreListWin.x += xrel;
	((GLWidget *)data)->bounds.y = scoreListWin.y += yrel;
    }
}


static void Scorelist_cleanup( GLWidget *widget )
{
    Score_font_close(&scoreListFont);
    sdl_window_destroy(&scoreListWin);
    if (scoreListWidget == widget)
	scoreListWidget = NULL;
}

static void SetBounds_ScoreList(GLWidget *widget, SDL_Rect *b )
{
    widget->bounds.x = scoreListWin.x = b->x;
    widget->bounds.y = scoreListWin.y = b->y;
}

static int Scorelist_prepare(Renderer *renderer)
{
    int result = 0;

    if (renderer == NULL || scoreListWidget == NULL
	|| scoreListWin.surface == NULL) {
	return -1;
    }

    if (scoresChanged) {
	/* This is the easiest way to track if
	 * the height of the score window should be changed */
	int y = scoreEntryRect.y;
        Paint_score_table();
	if (y != scoreEntryRect.y) {
	    int height;

	    if (scoreEntryRect.y < 0 || scoreEntryRect.h < 0
		|| scoreEntryRect.y > INT_MAX - 2 * SCORE_BORDER
		|| scoreEntryRect.h > INT_MAX - 2 * SCORE_BORDER
		    - scoreEntryRect.y) {
		result = -1;
		scoresChanged = true;
	    } else {
		height = scoreEntryRect.y + scoreEntryRect.h
		    + 2 * SCORE_BORDER;
		if (sdl_window_resize(&scoreListWin, scoreListWin.w,
				      height) != 0) {
		    result = -1;
		    scoresChanged = true;
		} else {
		    /* The first pass may have been clipped by the old surface. */
		    scoresChanged = true;
		    Paint_score_table();
		    scoreListWidget->bounds.w = scoreListWin.w + 2;
		    scoreListWidget->bounds.h = scoreListWin.h + 2;
		}
	    }
	}
	sdl_window_refresh(&scoreListWin);
    }

    if (sdl_window_prepare(&scoreListWin, renderer) != 0)
	return -1;
    return result;
}

static void Scorelist_paint(GLWidget *widget)
{
    const RendererColor background = {0, 0x20, 0, 0x90};

    (void)widget;
    (void)sdl_window_paint(&scoreListWin, &background);
}

GLWidget *Init_ScorelistWidget(void)
{
    GLWidget *tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_ScorelistWidget");
	return NULL;
    }

    tmp->WIDGET     	= SCORELISTWIDGET;
    tmp->bounds.x   	= 10;
    tmp->bounds.y   	= 240;
    tmp->bounds.w   	= 200;
    tmp->bounds.h   	= 100;

    if (!Score_font_open(&scoreListFont, Get_text_system(),
			 Xp_text_config_families(XP_TEXT_SPACING_MONOSPACE),
			 11)) {
	error("opening scorelist system fonts failed: %s", SDL_GetError());
	free(tmp);
	return NULL;
    }
    if (sdl_window_init(&scoreListWin, tmp->bounds.x, tmp->bounds.y, tmp->bounds.w, tmp->bounds.h)) {
	error("failed to init scorelist window");
	Score_font_close(&scoreListFont);
	free(tmp);
	return NULL;
    }
    tmp->Draw	    	= Scorelist_paint;
    tmp->Close	    	= Scorelist_cleanup;
    tmp->button     	= Scorelist_button;
    tmp->SetBounds     	= SetBounds_ScoreList;
    tmp->buttondata 	= tmp;
    tmp->motion     	= Scorelist_move;
    tmp->motiondata 	= tmp;

    scoreListWidget = tmp;

    return tmp;
}

bool Set_scaleFactor(xp_option_t *opt, double val)
{
    clData.scaleFactor = val;
    clData.scale = 1.0 / val;
    clData.fscale = (float)clData.scale;
    return true;
}

bool Set_altScaleFactor(xp_option_t *opt, double val)
{
    clData.altScaleFactor = val;
    return true;
}

int Paint_init(void)
{
    Sdl_game_frame_state_init(&rendererFrameState);
    Sdl_ui_draw_state_init(&gameUiDrawState);
    if (Init_wreckage() == -1)
	return -1;
    
    if (Images_init() == -1) 
	return -1;

    scoresChanged = true;
    players_exposed = true;
    
    return 0;
}

void Paint_cleanup(void)
{
    int i;

    Sdl_game_frame_state_init(&rendererFrameState);
    Sdl_ui_draw_state_init(&gameUiDrawState);
    Images_cleanup();

    for (i = 0; i < MAX_SCORE_OBJECTS; ++i)
	free_string_texture(&score_object_texs[i]);
    for (i = 0; i < MAX_METERS; ++i)
	free_string_texture(&meter_texs[i]);
    for (i = 0; i < MAX_HUD_TEXS + MAX_SCORE_OBJECTS; ++i)
	free_string_texture(&HUD_texs[i]);
}

/* This one works best for things that are fixed in position
 * since they won't appear to move relative to eachother
 */
RendererStatus setupPaint_stationary(void)
{
    RendererStatus status;

    if (paintSetupMode == STATIONARY_MODE)
	return Sdl_renderer_frame_result(Get_sdl_renderer());
    status = set_renderer_world_transform(1);
    status = Commit_paint_setup(
	PAINT_SETUP_TARGET_STATIONARY, status);
    if (status != RENDERER_STATUS_OK)
	return status;
    return Sdl_renderer_frame_result(Get_sdl_renderer());
}

RendererStatus setupPaint_stationary_cleanup(void)
{
    SdlRenderer *sdl_renderer;
    RendererStatus status;

    if (paintSetupMode == STATIONARY_MODE)
	return RENDERER_STATUS_OK;
    sdl_renderer = Get_sdl_renderer();
    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    status = set_renderer_world_transform_raw(sdl_renderer, 1);
    return Commit_paint_setup(
	PAINT_SETUP_TARGET_STATIONARY, status);
}

/* This one works best for things that move, since they don't get
 * painted differently depending on map position
 */
RendererStatus setupPaint_moving(void)
{
    RendererStatus status;

    if (paintSetupMode == MOVING_MODE)
	return Sdl_renderer_frame_result(Get_sdl_renderer());
    status = set_renderer_world_transform(0);
    status = Commit_paint_setup(
	PAINT_SETUP_TARGET_MOVING, status);
    if (status != RENDERER_STATUS_OK)
	return status;
    return Sdl_renderer_frame_result(Get_sdl_renderer());
}

RendererStatus setupPaint_HUD(void)
{
    RendererStatus status;

    if (paintSetupMode == HUD_MODE)
	return Sdl_renderer_frame_result(Get_sdl_renderer());
    status = set_renderer_hud_transform();
    status = Commit_paint_setup(PAINT_SETUP_TARGET_HUD, status);
    if (status != RENDERER_STATUS_OK)
	return status;
    return Sdl_renderer_frame_result(Get_sdl_renderer());
}

static void Paint_world_stage(void *context)
{
    (void)context;
    Paint_world();
    Gl_diagnostics_check("world");
}

static void Paint_vfuel_stage(void *context)
{
    (void)context;
    Paint_vfuel();
}

static void Paint_vdecor_stage(void *context)
{
    (void)context;
    Paint_vdecor();
}

static void Paint_vcannon_stage(void *context)
{
    (void)context;
    Paint_vcannon();
}

static void Paint_vbase_stage(void *context)
{
    (void)context;
    Paint_vbase();
}

static void Paint_objects_stage(void *context)
{
    (void)context;
    Paint_objects();
}

static void Paint_score_objects_stage(void *context)
{
    (void)context;
    Paint_score_objects();
}

static void Paint_shots_stage(void *context)
{
    (void)context;
    Paint_shots();
}

static void Setup_moving_stage(void *context)
{
    (void)context;
    (void)setupPaint_moving();
}

static void Paint_ships_stage(void *context)
{
    (void)context;
    Paint_ships();
    Gl_diagnostics_check("objects");
}

static RendererStatus Current_frame_result(void *context)
{
    return Sdl_renderer_frame_result(context);
}

static const PaintStageOps productionStageOps = {
    Paint_world_stage,
    Paint_vfuel_stage,
    Paint_vdecor_stage,
    Paint_vcannon_stage,
    Paint_vbase_stage,
    Paint_objects_stage,
    Paint_score_objects_stage,
    Paint_shots_stage,
    Setup_moving_stage,
    Paint_ships_stage,
    Current_frame_result
};

#ifdef XPILOT_SDLPAINT_TEST_HOOKS
RendererStatus Sdlpaint_test_run_world_object_stages(
    bool old_server, const SdlPaintStageOps *ops, void *context)
{
    PaintStageOps private_ops;

    if (ops == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    private_ops.paint_world = ops->paint_world;
    private_ops.paint_vfuel = ops->paint_vfuel;
    private_ops.paint_vdecor = ops->paint_vdecor;
    private_ops.paint_vcannon = ops->paint_vcannon;
    private_ops.paint_vbase = ops->paint_vbase;
    private_ops.paint_objects = ops->paint_objects;
    private_ops.paint_score_objects = ops->paint_score_objects;
    private_ops.paint_shots = ops->paint_shots;
    private_ops.setup_moving = ops->setup_moving;
    private_ops.paint_ships = ops->paint_ships;
    private_ops.frame_result = ops->frame_result;
    return Run_world_object_stages(
	old_server, &private_ops, context);
}

void Sdlpaint_test_begin_logical_frame(void)
{
    Begin_logical_paint_frame();
}

RendererStatus Sdlpaint_test_commit_setup(
    SdlPaintTestSetupTarget target, RendererStatus semantic_status)
{
    PaintSetupTarget private_target;

    if (target == SDLPAINT_TEST_SETUP_STATIONARY)
	private_target = PAINT_SETUP_TARGET_STATIONARY;
    else if (target == SDLPAINT_TEST_SETUP_MOVING)
	private_target = PAINT_SETUP_TARGET_MOVING;
    else if (target == SDLPAINT_TEST_SETUP_HUD)
	private_target = PAINT_SETUP_TARGET_HUD;
    else
	return RENDERER_STATUS_INVALID_ARGUMENT;
    return Commit_paint_setup(private_target, semantic_status);
}

RendererStatus Sdlpaint_test_end_logical_frame(
    RendererStatus frame_result)
{
    paintSetupMode = 0;
    return frame_result;
}
#endif

static RendererStatus End_game_frame(void *context)
{
    return Sdl_renderer_end_frame(context);
}

static void Swap_game_frame(void *context)
{
    (void)context;
    Swap_buffers();
}

static void Report_successful_game_frame(RendererStatus status)
{
    if (status != RENDERER_STATUS_OK || gameFrameReadyReported)
	return;
    xpprintf("Game frame ready: semantic=ok, presented=1\n");
    fflush(stdout);
    gameFrameReadyReported = true;
}

void Paint_frame(void)
{
    struct timeval tv1, tv2;
    SdlRenderer *sdl_renderer = Get_sdl_renderer();
    RendererStatus renderer_status;

    gettimeofday(&tv1, NULL);

    /* A backend end failure keeps the old frame active so that its cleanup
     * can be retried without advancing or redrawing the logical frame. A
     * successful recovery completes this tick, presenting exactly once only
     * when no semantic draw failure was retained. */
    if (Sdl_game_frame_pending(&rendererFrameState)) {
	renderer_status = Sdl_game_frame_finish(
	    &rendererFrameState, End_game_frame, Swap_game_frame,
	    sdl_renderer);
	if (renderer_status != RENDERER_STATUS_OK) {
	    warn("Could not recover the previous renderer frame (%d)",
		 (int)renderer_status);
	}
	Report_successful_game_frame(renderer_status);
	goto timing;
    }

    Begin_logical_paint_frame();
    Paint_frame_start();
    Gl_diagnostics_check("frame start");

    if (Images_prepare(Sdl_renderer_frontend(sdl_renderer)) != 0)
	warn("Could not prepare newly registered images");
    if (Scorelist_prepare(Sdl_renderer_frontend(sdl_renderer)) != 0)
	warn("Could not prepare the score list");
    if (Console_prepare(Sdl_renderer_frontend(sdl_renderer)) != 0)
	warn("Could not prepare the console");
    if (UpdateRadar) {
	Radar_update();
	UpdateRadar = false;
    }
    if (Radar_prepare(Sdl_renderer_frontend(sdl_renderer)) != 0)
	warn("Could not prepare the radar");

    if (damaged <= 0) {
	renderer_status = Sdl_renderer_begin_frame(
	    sdl_renderer, draw_width, draw_height,
	    Renderer_color_from_rgba32(blackRGBA));
	if (renderer_status != RENDERER_STATUS_OK) {
	    warn("Could not begin renderer frame (%d)", (int)renderer_status);
	    goto timing;
	}
	renderer_status = Sdl_game_frame_activate(&rendererFrameState);
	if (renderer_status != RENDERER_STATUS_OK) {
	    warn("Could not activate renderer frame state (%d)",
		 (int)renderer_status);
	    goto timing;
	}
	Sdl_ui_draw_state_init(&gameUiDrawState);
	renderer_status = setupPaint_stationary();
	{
	    RendererStatus stage_status = Run_world_object_stages(
		oldServer, &productionStageOps, sdl_renderer);

	    if (renderer_status == RENDERER_STATUS_OK)
		renderer_status = stage_status;
	}
	if (renderer_status == RENDERER_STATUS_OK)
	    renderer_status = setupPaint_HUD();
	if (renderer_status == RENDERER_STATUS_OK) {
	    Paint_meters();
	    renderer_status = Sdl_renderer_frame_result(sdl_renderer);
	}
	if (renderer_status == RENDERER_STATUS_OK)
	    renderer_status = Paint_HUD_checked();
	if (renderer_status == RENDERER_STATUS_OK) {
	    Paint_HUD_values();
	    renderer_status = Sdl_renderer_frame_result(sdl_renderer);
	}
	if (renderer_status == RENDERER_STATUS_OK) {
	    Gl_diagnostics_check("HUD");
	    Paint_messages();
	    renderer_status = Sdl_renderer_frame_result(sdl_renderer);
	}
	if (renderer_status == RENDERER_STATUS_OK)
	    renderer_status = Console_paint();
	if (renderer_status == RENDERER_STATUS_OK) {
	    renderer_status = Paint_select();
	    if (renderer_status == RENDERER_STATUS_OK) {
		renderer_status = DrawGLWidgets_checked(
		    MainWidget, sdl_renderer, &gameUiDrawState);
	    }
	}
	{
	    RendererStatus unwind_status = End_logical_paint_frame();

	    if (renderer_status == RENDERER_STATUS_OK)
		renderer_status = unwind_status;
	}
	if (renderer_status != RENDERER_STATUS_OK) {
	    (void)Sdl_game_frame_abort(
		&rendererFrameState, renderer_status);
	    goto finish_frame;
	}
    }

finish_frame:
    Gl_diagnostics_check("frame end");
    if (Sdl_game_frame_pending(&rendererFrameState)) {
	renderer_status = Sdl_renderer_frame_result(sdl_renderer);
	if (renderer_status != RENDERER_STATUS_OK)
	    (void)Sdl_game_frame_abort(&rendererFrameState,
				       renderer_status);
	renderer_status = Sdl_game_frame_finish(
	    &rendererFrameState, End_game_frame, Swap_game_frame,
	    sdl_renderer);
	if (renderer_status != RENDERER_STATUS_OK) {
	    warn("Could not end renderer frame (%d)", (int)renderer_status);
	}
	Report_successful_game_frame(renderer_status);
    } else {
	/* A damaged tick intentionally performs no redraw, but the
	 * double-buffer cadence still presents the unchanged back buffer. */
	(void)Sdl_game_frame_present_idle(
	    &rendererFrameState, Swap_game_frame, sdl_renderer);
    }

timing:
    if (newSecond) {
	gettimeofday(&tv2, NULL);
	clData.clientLag = 1e-3 * timeval_sub(&tv2, &tv1);
    }
}

void Paint_score_start(void)
{
    char	headingStr[MSG_LEN];
    SDL_Surface *header;
	SDL_Color fg;

    if (showUserName)
	strlcpy(headingStr, "NICK=USER@HOST", sizeof(headingStr));
    else if (BIT(Setup->mode, TEAM_PLAY))
	strlcpy(headingStr, "  SCORE NAME           LIFE", sizeof(headingStr));
    else {
	strlcpy(headingStr, "  ", sizeof(headingStr));
	if (BIT(Setup->mode, TIMING))
	    strcat(headingStr, "LAP ");
	strlcpy(headingStr, " AL ", sizeof(headingStr));
	strcat(headingStr, "  SCORE  ");
	if (BIT(Setup->mode, LIMITED_LIVES))
	    strlcat(headingStr, "LIFE", sizeof(headingStr));
	strlcat(headingStr, " NAME", sizeof(headingStr));
    }
	
    fg.r = (scoreColorRGBA >> 24) & 255;
	fg.g = (scoreColorRGBA >> 16) & 255;
	fg.b = (scoreColorRGBA >> 8) & 255;
	/* SDL 1.2_ttf ignored SDL_Color's fourth byte for blended text. */
	fg.a = SDL_ALPHA_OPAQUE;
    SDL_FillSurfaceRect(scoreListWin.surface, NULL, 0);
    header = Score_font_render(&scoreListFont, headingStr, fg);
    if (header == NULL) {
	error("scorelist header rendering failed: %s", SDL_GetError());
	return;
    }
    scoreEntryRect.x = scoreEntryRect.y = SCORE_BORDER;
    if (Sdl_blit_surface_unblended(header, NULL, scoreListWin.surface,
				   &scoreEntryRect) < 0)
	warn("Could not draw scorelist header: %s", SDL_GetError());
    Sdl_surface_draw_line_rgba(
        scoreListWin.surface, SCORE_BORDER,
        scoreEntryRect.y + header->h + 2,
        scoreListWin.w - SCORE_BORDER,
        scoreEntryRect.y + header->h + 2,
        0, 128, 0, 255);
    SDL_DestroySurface(header);
}

void Paint_score_entry(int entry_num, other_t *other, bool is_team)
{
    static char		raceStr[16], teamStr[4], lifeStr[8], label[MSG_LEN];
    static int		lineSpacing = -1, firstLine;
    char		scoreStr[16];
    char		playerName[2 * MAX_CHARS + 4];
    SDL_Surface         *line;
	SDL_Color fg;
    int     	    	color;

    /*
     * First time we're here, set up miscellaneous strings for
     * efficiency and calculate some other constants.
     */
    if (lineSpacing == -1) {
	memset(raceStr, 0, sizeof raceStr);
	memset(teamStr, 0, sizeof teamStr);
	memset(lifeStr, 0, sizeof lifeStr);
	teamStr[1] = ' ';
	raceStr[2] = ' ';

	lineSpacing = Score_font_line_spacing(&scoreListFont) + 1;

	firstLine = 2*SCORE_BORDER + lineSpacing;
    }
    scoreEntryRect.y = firstLine + lineSpacing * entry_num;

    /*
     * Setup the status line
     */
    if (showUserName)
	sprintf(label, "%s=%s@%s",
	    other->nick_name, other->user_name, other->host_name);
    else {
	if (!Format_player_identity(
		playerName, sizeof(playerName),
		other->nick_name, other->user_name)) {
	    strlcpy(playerName, other->nick_name, sizeof(playerName));
	}
	if (BIT(Setup->mode, TIMING)) {
	    strlcpy(raceStr, "  ", sizeof(raceStr));
	    if ((other->mychar == ' ' || other->mychar == 'R')
		&& other->round + other->check > 0) {
		if (other->round > 99)
		    snprintf(raceStr, sizeof(raceStr), "%3d", other->round);
		else
		    snprintf(raceStr, sizeof(raceStr), "%d.%c",
			    other->round, other->check + 'a');
	    }
	}
	if (BIT(Setup->mode, TEAM_PLAY))
	    teamStr[0] = other->team + '0';
	else
	    sprintf(teamStr, "%c", other->alliance);

	if (BIT(Setup->mode, LIMITED_LIVES))
	    sprintf(lifeStr, " %3d", other->life);

	if (Using_score_decimals())
	    sprintf(scoreStr, "%*.*f",
		    7 - showScoreDecimals, showScoreDecimals,
		    other->score);
	else {
	    double score = other->score;
	    int sc = (int)(score >= 0.0 ? score + 0.5 : score - 0.5);
	    sprintf(scoreStr, "%6d", sc);
	}

	if (BIT(Setup->mode, TEAM_PLAY))
	    sprintf(label, "%c%s %-15s%s",
		    other->mychar, scoreStr, playerName, lifeStr);
	else
	    sprintf(label, "%c %s%s%s%s  %s",
		    other->mychar, raceStr, teamStr,
		    scoreStr, lifeStr,
		    playerName);
    }

    /*
     * Draw the line
     * e94_msu eKthHacks
     */
    if (!is_team && strchr("DPW", other->mychar)) {
	if (other->id == self->id)
	    color = scoreInactiveSelfColorRGBA;
	else
	    color = scoreInactiveColorRGBA;
    } else {
	if (!is_team) {
	    if (other->id == self->id)
		color = scoreSelfColorRGBA;
	    else
		color = scoreColorRGBA;
	} else {
	    color = Team_color(other->team);
	    if (!color) {
		if (other->team == self->team)
		    color = scoreOwnTeamColorRGBA;
		else
		    color = scoreEnemyTeamColorRGBA;
	    }
	}
    }
    fg.r = (color >> 24) & 255;
	fg.g = (color >> 16) & 255;
	fg.b = (color >> 8) & 255;
	/* Preserve the opaque glyphs produced by SDL 1.2_ttf. */
	fg.a = SDL_ALPHA_OPAQUE;
    line = Score_font_render(&scoreListFont, label, fg);
    if (line == NULL) {
	error("scorelist rendering failed: %s", SDL_GetError());
	return;
    }
    if (Sdl_blit_surface_unblended(line, NULL, scoreListWin.surface,
				   &scoreEntryRect) < 0)
	warn("Could not draw scorelist entry: %s", SDL_GetError());
    scoreEntryRect.h = line->h;

    /*
     * Underline the teams
     */
    if (is_team) {
	Sdl_surface_draw_line_rgba(
	    scoreListWin.surface, scoreEntryRect.x,
	    scoreEntryRect.y + line->h - 1,
	    scoreEntryRect.x + scoreEntryRect.w,
	    scoreEntryRect.y + line->h - 1,
	    fg.r, fg.g, fg.b, 255);
    }

    SDL_DestroySurface(line);
}
