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

#ifndef	DBUFF_H
#define	DBUFF_H

#include "xpcommon.h"

#ifdef DBE
#  ifdef HAVE_X11_EXTENSIONS_XDBE_H
#    include <X11/extensions/Xdbe.h>
#  endif
#  undef MBX
#else
#  undef XdbeBackBuffer
#  define XdbeBackBuffer	unsigned int
#endif

#ifdef MBX
#  ifdef HAVE_X11_EXTENSIONS_MULTIBUF_H
#    include <X11/extensions/multibuf.h>
#  endif
#else
#   undef Multibuffer
#   define Multibuffer	unsigned int
#endif


typedef enum {
    PIXMAP_COPY,
    COLOR_SWITCH,
    MULTIBUFFER
} dbuff_t;

typedef enum {
    MULTIBUFFER_NONE,
    MULTIBUFFER_MBX,
    MULTIBUFFER_DBE
} dbuff_multibuffer_t;

typedef struct {
    XdbeBackBuffer	dbe_draw;
    int			dbe_major;
    int			dbe_minor;
} dbuff_dbe_state_t;

typedef struct {
    Multibuffer		mbx_draw[2];
    int			mbx_ev_base;
    int			mbx_err_base;
} dbuff_mbx_state_t;

typedef struct {
    Display		*display;
    dbuff_t		type;
    dbuff_multibuffer_t	multibuffer_type;
    Colormap		xcolormap;
    unsigned long	drawing_planes;
    int			colormap_index;
    XColor		*colormaps[2];
    int			colormap_size;
    unsigned long	drawing_plane_masks[2];
    unsigned long	*planes;
    unsigned long	pixel;
    dbuff_dbe_state_t	dbe;
    dbuff_mbx_state_t	mbx;
} dbuff_state_t;

extern dbuff_state_t   *dbuf_state;    /* Holds current dbuff state */

dbuff_state_t *start_dbuff(Display *display, Colormap cmap,
			   dbuff_t type,
			   unsigned num_planes, XColor *colors);
void dbuff_switch(dbuff_state_t *state);
void dbuff_init_buffer(dbuff_state_t *state);
void end_dbuff(dbuff_state_t *state);
void dbuff_list(Display *display);

#ifndef MBX
#  undef Multibuffer
#endif
#ifndef DBE
#  undef XdbeBackBuffer
#endif

#endif
