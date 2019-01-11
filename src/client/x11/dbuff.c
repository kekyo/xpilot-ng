/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
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

#include "xpclient_x11.h"

dbuff_state_t   *dbuf_state;	/* Holds current dbuff state */


static void dbuff_release(dbuff_state_t *state)
{
    if (state != NULL) {
	XFREE(state->colormaps[0]);
	XFREE(state->colormaps[1]);
	XFREE(state->planes);
#ifdef MBX
	if (state->type == MULTIBUFFER
	    && state->colormap_index != 2)
	    XmbufDestroyBuffers(state->display, drawWindow);
#endif

	XFREE(state);
    }
}


static long dbuff_color(dbuff_state_t *state, long simple_color)
{
    long		i, plane, computed_color;

    computed_color = state->pixel;
    for (i = 0; simple_color != 0; i++) {
	plane = (1 << i);
	if (plane & simple_color) {
	    computed_color |= state->planes[i];
	    simple_color &= ~plane;
	}
    }

    return computed_color;
}


dbuff_state_t *start_dbuff(Display *display, Colormap xcolormap,
			   dbuff_t type, unsigned num_planes,
			   XColor *colorarray)
{
    dbuff_state_t	*state;
    int			i, high_mask, low_mask;

    state = XCALLOC(dbuff_state_t, 1);
    if (state == NULL)
	return NULL;

    state->colormap_size = 1 << (2 * num_planes);
    state->colormaps[0] = XMALLOC(XColor ,state->colormap_size);
    state->colormaps[1]	= XMALLOC(XColor, state->colormap_size);
    state->planes = XCALLOC(unsigned long, 2 * num_planes);
    if (state->colormaps[1] == NULL ||
	state->colormaps[0] == NULL ||
	state->planes == NULL) {

	dbuff_release(state);
	return NULL;
    }
    state->display = display;
    state->xcolormap = xcolormap;

    state->type = type;
    state->multibuffer_type = MULTIBUFFER_NONE;

    switch (type) {

    case PIXMAP_COPY:
	state->colormap_index = 0;
	break;

    case COLOR_SWITCH:
	if (XAllocColorCells(state->display,
			     state->xcolormap,
			     False,
			     state->planes,
			     2 * num_planes,
			     &state->pixel,
			     1) == 0) {
	    dbuff_release(state);
	    return NULL;
	}
	break;

    case MULTIBUFFER:
#ifdef DBE
	state->colormap_index = 2;
	state->multibuffer_type = MULTIBUFFER_DBE;
	if (!XdbeQueryExtension(display,
				&state->dbe.dbe_major,
				&state->dbe.dbe_minor)) {
	    dbuff_release(state);
	    warn("XdbeQueryExtension failed.");
	    return NULL;
	}
#elif defined(MBX)
	state->colormap_index = 2;
	state->multibuffer_type = MULTIBUFFER_MBX;
	if (!XmbufQueryExtension(display,
				 &state->mbx.mbx_ev_base,
				 &state->mbx.mbx_err_base)) {
	    dbuff_release(state);
	    warn("XmbufQueryExtension failed.");
	    return NULL;
	}
#else
	warn("Support for multibuffering was not configured.");
	dbuff_release(state);
	return NULL;
#endif
	break;

    default:
	warn("Illegal dbuff_t %d.");
	exit(1);
    }

    state->drawing_plane_masks[0] = AllPlanes;
    state->drawing_plane_masks[1] = AllPlanes;

    for (i = 0; i < (int)num_planes; i++) {
	state->drawing_plane_masks[0] &= ~state->planes[i];
	state->drawing_plane_masks[1] &= ~state->planes[num_planes + i];
    }

    if (state->type == COLOR_SWITCH) {
	for (i = 0; i < (1 << num_planes); i++) {
	    colorarray[i].pixel = dbuff_color(state, i | (i << num_planes));
	    colorarray[i].flags = DoRed | DoGreen | DoBlue;
	}
    }
    else if (num_planes > 1) {
	for (i = 0; i < (1 << num_planes); i++) {
	    if (XAllocColor(display, xcolormap, &colorarray[i]) == False) {
		while (--i >= 0)
		    XFreeColors(display, xcolormap, &colorarray[i].pixel,
				1, 0);
		dbuff_release(state);
		return NULL;
	    }
	}
    } else {
	colorarray[WHITE].pixel = WhitePixel(display, DefaultScreen(display));
	colorarray[BLACK].pixel = BlackPixel(display, DefaultScreen(display));
	colorarray[BLUE].pixel  = WhitePixel(display, DefaultScreen(display));
	colorarray[RED].pixel   = WhitePixel(display, DefaultScreen(display));
    }

    low_mask = (1 << num_planes) - 1;
    high_mask = low_mask << num_planes;
    for (i = state->colormap_size - 1; i >= 0; i--) {
	state->colormaps[0][i] = colorarray[i & low_mask];
	state->colormaps[0][i].pixel = dbuff_color(state, i);
	state->colormaps[1][i] = colorarray[(i & high_mask) >> num_planes];
	state->colormaps[1][i].pixel = dbuff_color(state, i);
    }

    state->drawing_planes = state->drawing_plane_masks[state->colormap_index];

    if (state->type == COLOR_SWITCH)
	XStoreColors(state->display,
		     state->xcolormap,
		     state->colormaps[state->colormap_index],
		     state->colormap_size);

    return state;
}


void dbuff_init_buffer(dbuff_state_t *state)
{
#ifdef MBX
    if (state->type == MULTIBUFFER) {
	if (state->colormap_index == 2) {
	    state->colormap_index = 0;
	    if (XmbufCreateBuffers(state->display, drawWindow, 2,
				   MultibufferUpdateActionUndefined,
				   MultibufferUpdateHintFrequent,
				   state->mbx.mbx_draw) != 2) {
		error("Couldn't create double buffering buffers.");
		exit(1);
	    }
	}
	drawPixmap = state->mbx.mbx_draw[state->colormap_index];
    }
#elif defined DBE
    if (state->type == MULTIBUFFER) {
	if (state->colormap_index == 2) {
	    state->colormap_index = 0;
	    state->dbe.dbe_draw =
		XdbeAllocateBackBufferName(state->display,
					   drawWindow,
					   XdbeBackground);
	    if (state->dbe.dbe_draw == 0) {
		error("Couldn't create double buffering back buffer.");
		exit(1);
	    }
	}
	drawPixmap = state->dbe.dbe_draw;
    }
#else
    UNUSED_PARAM(state);
#endif
}


void dbuff_switch(dbuff_state_t *state)
{
#ifdef MBX
    if (state->type == MULTIBUFFER)
	drawPixmap = state->mbx.mbx_draw[state->colormap_index];
#endif

    state->colormap_index ^= 1;

    if (state->type == COLOR_SWITCH)
	XStoreColors(state->display, state->xcolormap,
		     state->colormaps[state->colormap_index],
		     state->colormap_size);
#ifdef DBE
    else if (state->type == MULTIBUFFER) {
	XdbeSwapInfo		swap;

	swap.swap_window	= drawWindow;
	swap.swap_action	= XdbeBackground;
	if (!XdbeSwapBuffers(state->display, &swap, 1)) {
	    error("XdbeSwapBuffers failed.");
	    exit(1);
	}
    }
#endif
#ifdef MBX
    else if (state->type == MULTIBUFFER)
	XmbufDisplayBuffers(state->display, 1,
			    &state->mbx.mbx_draw[state->colormap_index],
			    0, 200);
#endif

    state->drawing_planes = state->drawing_plane_masks[state->colormap_index];
}


void end_dbuff(dbuff_state_t *state)
{
    if (state->type == COLOR_SWITCH)
	XFreeColors(state->display, state->xcolormap,
		    &state->pixel, 1,
		    ~(state->drawing_plane_masks[0] &
		      state->drawing_plane_masks[1]));
    dbuff_release(state);
}


#ifdef DBE
static void dbuff_list_dbe(Display *display)
{
    XdbeScreenVisualInfo	*info;
    XdbeVisualInfo		*visinfo;
    int				n = 0;
    int				i, j;

    printf("\n");
    info = XdbeGetVisualInfo(display, NULL, &n);
    if (!info) {
	warn("Could not obtain double buffer extension info.");
	return;
    }
    for (i = 0; i < n; i++) {
	printf("Visuals supporting double buffering on screen %d:\n", i);
	printf("%9s%9s%11s\n", "visual", "depth", "perflevel");
	for (j = 0; j < info[i].count; j++) {
	    visinfo = &info[i].visinfo[j];
	    printf("    0x%02x  %6d  %8d\n",
		    (unsigned) visinfo->visual,
		    visinfo->depth,
		    visinfo->perflevel);
	}
    }
    XdbeFreeVisualInfo(info);
}
#endif


void dbuff_list(Display *display)
{
#ifdef DBE
    dbuff_list_dbe(display);
#else
    UNUSED_PARAM(display);
#endif
}

