/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Erik Andersson <deity_at_home.se>
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

/*
    glfont:  An example of using the SDL_ttf library with OpenGL.
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: text.c,v 1.36 2005/09/08 11:09:05 maximan Exp $ */
/* modified for xpilot by Erik Andersson deity_at_home.se */

#include <limits.h>

#include "xpclient_sdl.h"

#include "text.h"
#include "text_ttf_atlas.h"

#define BUFSIZE 1024

static bool Font_is_empty(const font_data *font);

static bool Font_is_empty(const font_data *font)
{
    return font->requested_height == 0 && font->atlas == NULL
	&& font->text_renderer == NULL && font->unicode_font == NULL
	&& font->unicode_renderer == NULL;
}

static RendererStatus Text_native_status(XpTextStatus status)
{
    switch (status) {
    case XP_TEXT_STATUS_OK:
	return RENDERER_STATUS_OK;
    case XP_TEXT_STATUS_OUT_OF_MEMORY:
	return RENDERER_STATUS_OUT_OF_MEMORY;
    case XP_TEXT_STATUS_INVALID_ARGUMENT:
    case XP_TEXT_STATUS_INVALID_UTF8:
    case XP_TEXT_STATUS_UNSUPPORTED_DIRECTION:
	return RENDERER_STATUS_INVALID_ARGUMENT;
    case XP_TEXT_STATUS_FONT_UNAVAILABLE:
    case XP_TEXT_STATUS_BACKEND_ERROR:
	return RENDERER_STATUS_BACKEND_ERROR;
    default:
	return RENDERER_STATUS_BACKEND_ERROR;
    }
}

RendererStatus fontinit(font_data *ft_font, Renderer *renderer,
			const char *fname, unsigned int size)
{
    TTF_Font *ttf_font;
    TextAtlas *atlas = NULL;
    RendererStatus status;

    if (ft_font == NULL || renderer == NULL || fname == NULL
	|| size == 0 || size > INT_MAX || !Font_is_empty(ft_font)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    ttf_font = TTF_OpenFont(fname, (int)size);
    if (ttf_font == NULL) {
	fprintf(stderr, "Couldn't load %u pt font from %s: %s\n", size,
		fname, SDL_GetError());
	return RENDERER_STATUS_BACKEND_ERROR;
    }
    TTF_SetFontStyle(ttf_font, TTF_STYLE_NORMAL);
    status = Text_ttf_atlas_create(renderer, ttf_font, &atlas);
    TTF_CloseFont(ttf_font);
    if (status != RENDERER_STATUS_OK)
	return status;

    ft_font->requested_height = size;
    ft_font->atlas = atlas;
    return RENDERER_STATUS_OK;
}

RendererStatus font_text_renderer_attach(font_data *ft_font,
					 SdlRenderer *sdl_renderer)
{
    if (ft_font == NULL || sdl_renderer == NULL || ft_font->atlas == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (ft_font->text_renderer != NULL) {
	return Text_renderer_matches(ft_font->text_renderer, sdl_renderer,
				     ft_font->atlas)
	    ? RENDERER_STATUS_OK : RENDERER_STATUS_RESOURCE_MISMATCH;
    }
    return Text_renderer_create(sdl_renderer, ft_font->atlas,
				&ft_font->text_renderer);
}

RendererStatus font_unicode_renderer_attach(
    font_data *ft_font, XpTextSystem *text_system,
    SdlRenderer *sdl_renderer, const char *family_list)
{
    XpTextFontRequest request;
    XpTextFont *candidate_font = NULL;
    UnicodeTextRenderer *candidate_renderer = NULL;
    XpTextStatus text_status;
    RendererStatus status;

    if (ft_font == NULL || text_system == NULL || sdl_renderer == NULL
	|| family_list == NULL || family_list[0] == '\0'
	|| ft_font->requested_height == 0 || ft_font->text_renderer == NULL) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (ft_font->unicode_font != NULL
	|| ft_font->unicode_renderer != NULL) {
	return ft_font->unicode_font != NULL
	    && Unicode_text_renderer_matches(
		ft_font->unicode_renderer, sdl_renderer,
		ft_font->unicode_font)
	    ? RENDERER_STATUS_OK : RENDERER_STATUS_RESOURCE_MISMATCH;
    }
    request.family_list = family_list;
    request.pixel_height = (float)ft_font->requested_height;
    request.weight = XP_TEXT_WEIGHT_BOLD;
    request.slant = XP_TEXT_SLANT_ITALIC;
    request.spacing = XP_TEXT_SPACING_PROPORTIONAL;
    text_status = Xp_text_font_open(text_system, &request,
				    &candidate_font);
    if (text_status != XP_TEXT_STATUS_OK)
	return Text_native_status(text_status);
    status = Unicode_text_renderer_create(
	sdl_renderer, candidate_font, &candidate_renderer);
    if (status != RENDERER_STATUS_OK) {
	Xp_text_font_close(&candidate_font);
	return status;
    }
    ft_font->unicode_font = candidate_font;
    ft_font->unicode_renderer = candidate_renderer;
    return RENDERER_STATUS_OK;
}

RendererStatus fontclean(font_data *ft_font)
{
    RendererStatus status;

    if (ft_font == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    status = Text_atlas_destroy(&ft_font->atlas);
    if (status != RENDERER_STATUS_OK)
	return status;

    status = Unicode_text_renderer_destroy(&ft_font->unicode_renderer);
    if (status != RENDERER_STATUS_OK)
	return status;
    Xp_text_font_close(&ft_font->unicode_font);

    /* The facade destroy path is deliberately CPU-only. Keeping it until the
     * fallible atlas release succeeds preserves all font state for retry. */
    Text_renderer_destroy(&ft_font->text_renderer);
    memset(ft_font, 0, sizeof(*ft_font));
    return RENDERER_STATUS_OK;
}

static RendererStatus Text_format(char text[BUFSIZE], const char *fmt,
				  va_list args, size_t *text_length)
{
    int result;

    if (text == NULL || text_length == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (fmt == NULL) {
	text[0] = '\0';
	*text_length = 0;
	return RENDERER_STATUS_OK;
    }
    result = vsnprintf(text, BUFSIZE, fmt, args);
    text[BUFSIZE - 1] = '\0';
    if (result < 0) {
	text[0] = '\0';
	return RENDERER_STATUS_BACKEND_ERROR;
    }
    *text_length = strlen(text);
    return RENDERER_STATUS_OK;
}

static RendererStatus Text_adapter_failure(font_data *ft_font,
					    RendererStatus status)
{
    if (ft_font != NULL && ft_font->text_renderer != NULL
	&& status != RENDERER_STATUS_OK) {
	return Text_renderer_track_failure(ft_font->text_renderer, status);
    }
    return status;
}

static bool Text_has_non_ascii(const char *text, size_t text_length)
{
    size_t index;

    for (index = 0; index < text_length; index++) {
	if ((unsigned char)text[index] >= 0x80)
	    return true;
    }
    return false;
}

static RendererStatus Text_complete_length(font_data *ft_font,
					    const char *text,
					    size_t byte_length,
					    size_t *complete_length)
{
    XpTextStatus text_status;

    text_status = Xp_text_complete_utf8_prefix(
	text, byte_length, complete_length);
    if (text_status != XP_TEXT_STATUS_OK) {
	return Text_adapter_failure(ft_font,
				    Text_native_status(text_status));
    }
    return RENDERER_STATUS_OK;
}

static XpTextLayoutRequest Text_layout_request(
    const char *text, size_t text_length)
{
    XpTextLayoutRequest request;

    request.utf8 = text;
    request.byte_length = text_length;
    request.direction = XP_TEXT_DIRECTION_LTR;
    request.language_bcp47 = "";
    return request;
}

static RendererStatus Text_measure_bytes(font_data *ft_font,
					 const char *text, size_t text_length,
					 fontbounds *bounds)
{
    TextGeometryMetrics metrics;
    XpTextMetrics unicode_metrics;
    XpTextLayoutRequest request;
    fontbounds candidate;
    RendererStatus status;
    size_t complete_length;

    if (ft_font == NULL || ft_font->text_renderer == NULL || text == NULL
	|| bounds == NULL) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    status = Text_complete_length(
	ft_font, text, text_length, &complete_length);
    if (status != RENDERER_STATUS_OK)
	return status;
    text_length = complete_length;
    if (Text_has_non_ascii(text, text_length)) {
	if (ft_font->unicode_renderer == NULL) {
	    return Text_adapter_failure(
		ft_font, RENDERER_STATUS_INVALID_ARGUMENT);
	}
	request = Text_layout_request(text, text_length);
	status = Unicode_text_renderer_measure(
	    ft_font->unicode_renderer, &request, &unicode_metrics);
	if (status != RENDERER_STATUS_OK)
	    return status;
	candidate.width = (float)unicode_metrics.width;
	candidate.height = (float)unicode_metrics.height;
	*bounds = candidate;
	return RENDERER_STATUS_OK;
    }
    status = Text_renderer_measure(
	ft_font->text_renderer, (const unsigned char *)text, text_length,
	&metrics);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    candidate.width = metrics.width;
    candidate.height = metrics.height;
    *bounds = candidate;
    return RENDERER_STATUS_OK;
}

RendererStatus nprintsize(font_data *ft_font, int length,
			  fontbounds *bounds, const char *fmt, ...)
{
    char text[BUFSIZE];
    size_t text_length;
    RendererStatus status;
    va_list args;

    if (length < 0 || bounds == NULL || ft_font == NULL
	|| ft_font->text_renderer == NULL) {
	return Text_adapter_failure(ft_font,
				    RENDERER_STATUS_INVALID_ARGUMENT);
    }
    va_start(args, fmt);
    status = Text_format(text, fmt, args, &text_length);
    va_end(args);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    if (text_length > (size_t)length)
	text_length = (size_t)length;
    return Text_measure_bytes(ft_font, text, text_length, bounds);
}

RendererStatus printsize(font_data *ft_font, fontbounds *bounds,
			 const char *fmt, ...)
{
    char text[BUFSIZE];
    size_t text_length;
    RendererStatus status;
    va_list args;

    if (bounds == NULL || ft_font == NULL
	|| ft_font->text_renderer == NULL) {
	return Text_adapter_failure(ft_font,
				    RENDERER_STATUS_INVALID_ARGUMENT);
    }
    va_start(args, fmt);
    status = Text_format(text, fmt, args, &text_length);
    va_end(args);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    return Text_measure_bytes(ft_font, text, text_length, bounds);
}

bool render_text(font_data *ft_font, const char *text, string_tex_t *string_tex)
{
    const char *normalized_text;
    TextRendererCache *candidate_cache = NULL;
    TextRendererCache *old_cache;
    TextGeometryMetrics metrics;
    XpTextMetrics unicode_metrics;
    XpTextLayoutRequest request;
    RendererStatus status;
    char *candidate_text;
    char *old_text;
    size_t text_length;
    int width;
    int height;
    bool use_unicode;

    if (ft_font == NULL || ft_font->text_renderer == NULL
	|| text == NULL || string_tex == NULL
	|| ft_font->requested_height > INT_MAX) {
	return false;
    }

    /* Keep the old empty-author compatibility without creating a transient
     * surface or texture for the replacement string. */
    normalized_text = text[0] != '\0' ? text : " ";
    text_length = strlen(normalized_text);
    use_unicode = Text_has_non_ascii(normalized_text, text_length);
    if (use_unicode && ft_font->unicode_renderer == NULL)
	return false;
    if (((use_unicode
	  && string_tex->unicode_renderer == ft_font->unicode_renderer
	  && string_tex->cache == NULL)
	 || (!use_unicode
	     && string_tex->text_renderer == ft_font->text_renderer
	     && string_tex->cache != NULL))
	&& string_tex->text != NULL
	&& strcmp(string_tex->text, normalized_text) == 0) {
	return true;
    }

    if (use_unicode) {
	request = Text_layout_request(normalized_text, text_length);
	status = Unicode_text_renderer_measure(
	    ft_font->unicode_renderer, &request, &unicode_metrics);
	if (status != RENDERER_STATUS_OK || unicode_metrics.width < 0
	    || unicode_metrics.height < 0) {
	    return false;
	}
	width = unicode_metrics.width;
	height = unicode_metrics.height;
    } else {
	status = Text_renderer_cache_replace(
	    &candidate_cache, ft_font->text_renderer,
	    (const unsigned char *)normalized_text, text_length);
	if (status != RENDERER_STATUS_OK)
	    return false;
	status = Text_renderer_cache_metrics(candidate_cache, &metrics);
	if (status != RENDERER_STATUS_OK || metrics.width != metrics.width
	    || metrics.height != metrics.height || metrics.width < 0.0f
	    || metrics.height < 0.0f
	    || (double)metrics.width > (double)INT_MAX
	    || (double)metrics.height > (double)INT_MAX) {
	    Text_renderer_cache_destroy(&candidate_cache);
	    return false;
	}
	width = (int)metrics.width;
	height = (int)metrics.height;
    }
    candidate_text = xp_strdup(normalized_text);
    if (candidate_text == NULL) {
	Text_renderer_cache_destroy(&candidate_cache);
	return false;
    }

    old_cache = string_tex->cache;
    old_text = string_tex->text;
    string_tex->text_renderer = use_unicode ? NULL : ft_font->text_renderer;
    string_tex->unicode_renderer = use_unicode
	? ft_font->unicode_renderer : NULL;
    string_tex->cache = candidate_cache;
    string_tex->text = candidate_text;
    string_tex->width = width;
    string_tex->height = height;
    string_tex->font_height = (int)ft_font->requested_height;
    Text_renderer_cache_destroy(&old_cache);
    free(old_text);
    return true;
}

bool draw_text(font_data *ft_font, int color, int XALIGN, int YALIGN,
	       int x, int y, const char *text, bool savetex,
	       string_tex_t *string_tex, bool onHUD)
{
    bool remove_tex = false;
    bool rendered;
    RendererStatus draw_status = RENDERER_STATUS_INVALID_ARGUMENT;

    if (ft_font == NULL || ft_font->text_renderer == NULL)
	return false;
        
    if (string_tex == NULL) {
    	remove_tex = true;
	string_tex = XCALLOC(string_tex_t, 1);
	if (string_tex == NULL)
	    return false;
    }

    rendered = render_text(ft_font, text, string_tex);
    if (rendered)
	draw_status = disp_text(string_tex, color, XALIGN, YALIGN,
				x, y, onHUD);
    
    if (!savetex || remove_tex)
    	free_string_texture(string_tex);
    if (remove_tex)
	XFREE(string_tex);

    return rendered && draw_status == RENDERER_STATUS_OK;
}

static RendererStatus Text_horizontal_alignment(
    int legacy_alignment, TextGeometryHorizontalAlign *alignment)
{
    if (alignment == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    switch (legacy_alignment) {
    case LEFT:
	*alignment = TEXT_GEOMETRY_ALIGN_LEFT;
	return RENDERER_STATUS_OK;
    case CENTER:
	*alignment = TEXT_GEOMETRY_ALIGN_CENTER;
	return RENDERER_STATUS_OK;
    case RIGHT:
	*alignment = TEXT_GEOMETRY_ALIGN_RIGHT;
	return RENDERER_STATUS_OK;
    default:
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
}

static RendererStatus Text_vertical_alignment(
    int legacy_alignment, TextGeometryVerticalAlign *alignment)
{
    if (alignment == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    switch (legacy_alignment) {
    case DOWN:
	*alignment = TEXT_GEOMETRY_ALIGN_TOP;
	return RENDERER_STATUS_OK;
    case CENTER:
	*alignment = TEXT_GEOMETRY_ALIGN_MIDDLE;
	return RENDERER_STATUS_OK;
    case UP:
	*alignment = TEXT_GEOMETRY_ALIGN_BOTTOM;
	return RENDERER_STATUS_OK;
    default:
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
}

RendererStatus disp_text(string_tex_t *string_tex, int color,
			 int XALIGN, int YALIGN, int x, int y,
			 bool onHUD)
{
    TextGeometryHorizontalAlign horizontal;
    TextGeometryVerticalAlign vertical;
    RendererPoint2D anchor;
    RendererStatus status;

    if (string_tex == NULL || string_tex->text == NULL) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    status = Text_horizontal_alignment(XALIGN, &horizontal);
    if (status != RENDERER_STATUS_OK)
	return status;
    status = Text_vertical_alignment(YALIGN, &vertical);
    if (status != RENDERER_STATUS_OK)
	return status;

    anchor.x = (float)x;
    anchor.y = onHUD ? (float)draw_height - (float)y : (float)y;
    if (string_tex->unicode_renderer != NULL) {
	XpTextLayoutRequest request;

	if (string_tex->text_renderer != NULL || string_tex->cache != NULL)
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	request = Text_layout_request(
	    string_tex->text, strlen(string_tex->text));
	return Unicode_text_renderer_draw(
	    string_tex->unicode_renderer, &request, anchor,
	    horizontal, vertical,
	    Renderer_color_from_rgba32((uint32_t)color),
	    onHUD ? TEXT_RENDERER_SPACE_HUD : TEXT_RENDERER_SPACE_WORLD);
    }
    if (string_tex->text_renderer == NULL || string_tex->cache == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    return Text_renderer_draw_cached(
	string_tex->text_renderer, string_tex->cache, anchor,
	horizontal, vertical, Renderer_color_from_rgba32((uint32_t)color),
	onHUD ? TEXT_RENDERER_SPACE_HUD : TEXT_RENDERER_SPACE_WORLD);
}

void free_string_texture(string_tex_t *string_tex)
{
    if (string_tex == NULL)
	return;
    Text_renderer_cache_destroy(&string_tex->cache);
    XFREE(string_tex->text);
    string_tex->text_renderer = NULL;
    string_tex->unicode_renderer = NULL;
    string_tex->width = 0;
    string_tex->height = 0;
    string_tex->font_height = 0;
}

static RendererStatus Text_draw_bytes(
    font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y,
    const char *text, size_t text_length, bool onHUD)
{
    TextGeometryHorizontalAlign horizontal;
    TextGeometryVerticalAlign vertical;
    RendererPoint2D anchor;
    XpTextLayoutRequest request;
    RendererStatus status;
    size_t complete_length;

    if (ft_font == NULL || ft_font->text_renderer == NULL || text == NULL)
	return Text_adapter_failure(ft_font,
				    RENDERER_STATUS_INVALID_ARGUMENT);
    status = Text_horizontal_alignment(XALIGN, &horizontal);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    status = Text_vertical_alignment(YALIGN, &vertical);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    status = Text_complete_length(
	ft_font, text, text_length, &complete_length);
    if (status != RENDERER_STATUS_OK)
	return status;
    text_length = complete_length;
    if (text_length == 0) {
	TextGeometryMetrics metrics;

	return Text_renderer_measure(
	    ft_font->text_renderer, (const unsigned char *)text,
	    text_length, &metrics);
    }

    anchor.x = (float)x;
    anchor.y = onHUD ? (float)draw_height - (float)y : (float)y;
    if (Text_has_non_ascii(text, text_length)) {
	if (ft_font->unicode_renderer == NULL) {
	    return Text_adapter_failure(
		ft_font, RENDERER_STATUS_INVALID_ARGUMENT);
	}
	request = Text_layout_request(text, text_length);
	return Unicode_text_renderer_draw(
	    ft_font->unicode_renderer, &request, anchor,
	    horizontal, vertical,
	    Renderer_color_from_rgba32((uint32_t)color),
	    onHUD ? TEXT_RENDERER_SPACE_HUD : TEXT_RENDERER_SPACE_WORLD);
    }
    return Text_renderer_draw(
	ft_font->text_renderer, (const unsigned char *)text, text_length,
	anchor, horizontal, vertical,
	Renderer_color_from_rgba32((uint32_t)color),
	onHUD ? TEXT_RENDERER_SPACE_HUD : TEXT_RENDERER_SPACE_WORLD);
}

static RendererStatus Text_vprint(
    font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y,
    int length, const char *fmt, va_list args, bool onHUD)
{
    char text[BUFSIZE];
    size_t text_length;
    RendererStatus status;

    if (length < 0)
	return Text_adapter_failure(ft_font,
				    RENDERER_STATUS_INVALID_ARGUMENT);
    status = Text_format(text, fmt, args, &text_length);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    if (text_length > (size_t)length)
	text_length = (size_t)length;
    return Text_draw_bytes(ft_font, color, XALIGN, YALIGN, x, y,
			   text, text_length, onHUD);
}

RendererStatus mapnprint(font_data *ft_font, int color, int XALIGN,
			 int YALIGN, int x, int y, int length,
			 const char *fmt, ...)
{
    RendererStatus status;
    va_list args;

    va_start(args, fmt);
    status = Text_vprint(ft_font, color, XALIGN, YALIGN, x, y, length,
			 fmt, args, false);
    va_end(args);
    return status;
}

RendererStatus HUDnprint(font_data *ft_font, int color, int XALIGN,
			 int YALIGN, int x, int y, int length,
			 const char *fmt, ...)
{
    RendererStatus status;
    va_list args;

    va_start(args, fmt);
    status = Text_vprint(ft_font, color, XALIGN, YALIGN, x, y, length,
			 fmt, args, true);
    va_end(args);
    return status;
}

RendererStatus mapprint(font_data *ft_font, int color, int XALIGN,
			int YALIGN, int x, int y, const char *fmt, ...)
{
    RendererStatus status;
    va_list args;

    va_start(args, fmt);
    status = Text_vprint(ft_font, color, XALIGN, YALIGN, x, y,
			 BUFSIZE - 1, fmt, args, false);
    va_end(args);
    return status;
}

RendererStatus HUDprint(font_data *ft_font, int color, int XALIGN,
			int YALIGN, int x, int y, const char *fmt, ...)
{
    RendererStatus status;
    va_list args;

    va_start(args, fmt);
    status = Text_vprint(ft_font, color, XALIGN, YALIGN, x, y,
			 BUFSIZE - 1, fmt, args, true);
    va_end(args);
    return status;
}
