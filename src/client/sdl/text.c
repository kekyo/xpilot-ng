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

    The SDL_GL_* functions in this file are available in the public domain.

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: text.c,v 1.36 2005/09/08 11:09:05 maximan Exp $ */
/* modified for xpilot by Erik Andersson deity_at_home.se */

#ifdef _WINDOWS
# include <windows.h>
# ifndef GL_BGR
#  define GL_BGR 0x80E0 /* OpenGL 1.2, for which I did not have headers */
# endif
#endif

#include <limits.h>

#include "xpclient_sdl.h"

#include "sdlcompat.h"
#include "text.h"
#include "text_ttf_atlas.h"

#define BUFSIZE 1024

int renderstyle;
enum rendertype rendertype;

int next_p2 ( int a );

static RendererStatus Init_legacy_font(font_data *font,
				       const char *fontname, int ptsize);
static bool Font_is_empty(const font_data *font);
static void Release_legacy_font_resources(font_data *font);

int next_p2 ( int a )
{
	int rval=1;
	while(rval<a) rval<<=1;
	return rval;
}

GLuint SDL_GL_LoadTexture(SDL_Surface *surface, texcoord_t *texcoord)
{
    GLuint texture;
    int w, h;
    SDL_Surface *image;
    SDL_Rect area;

    /* Use the surface width and height expanded to powers of 2 */
    w = next_p2(surface->w);
    h = next_p2(surface->h);
    texcoord->MinX = 0.0f;		 /* Min X */
    texcoord->MinY = 0.0f;		 /* Min Y */
    texcoord->MaxX = (GLfloat)surface->w / w;  /* Max X */
    texcoord->MaxY = (GLfloat)surface->h / h;  /* Max Y */

    image = SDL_CreateRGBSurface(
		    0,
    		    w, h,
    		    32,
		    RMASK,
		    GMASK,
		    BMASK,
		    AMASK
    		   );
    if ( image == NULL ) {
    	    return 0;
    }

    /* Copy the surface into the GL texture image */
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    if (Sdl_blit_surface_unblended(surface, &area, image, &area) < 0) {
	fprintf(stderr, "Couldn't copy font surface: %s\n", SDL_GetError());
	SDL_FreeSurface(image);
	return 0;
    }

    /* Create an OpenGL texture for the image */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
    		 0,
    		 GL_RGBA,
    		 w, h,
    		 0,
    		 GL_RGBA,
    		 GL_UNSIGNED_BYTE,
    		 image->pixels);
    SDL_FreeSurface(image); /* No longer needed */

    return texture;
}

static RendererStatus Init_legacy_font(font_data *font,
				       const char *fontname, int ptsize)
{
    int i;
    SDL_Color white = { 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE };
    SDL_Color *forecol;
    GLenum gl_error;
    texcoord_t texcoords;
    int minx = 0,miny = 0,maxx = 0,maxy = 0;

    /* We might support changing theese later */
    /* Look for special rendering types */
    renderstyle = TTF_STYLE_NORMAL;
    rendertype = RENDER_LATIN1;
    /* Default is black and white */
    forecol = &white;
    
    /* Initialize the TTF library */
    /*if ( TTF_Init() < 0 ) {
    	fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
    	return(2);
    }*/
    font->ttffont = TTF_OpenFont(fontname, ptsize);
    if ( font->ttffont == NULL ) {
	fprintf(stderr, "Couldn't load %d pt font from %s: %s\n", ptsize,
		fontname, TTF_GetError());
	return RENDERER_STATUS_BACKEND_ERROR;
    }
    TTF_SetFontStyle(font->ttffont, renderstyle);
    font->list_base=glGenLists(next_p2(NUMCHARS));
    if (font->list_base == 0) {
	fprintf(stderr, "Couldn't create OpenGL font display lists\n");
	return RENDERER_STATUS_BACKEND_ERROR;
    }
    /* Get the recommended spacing between lines of text for this font */
    font->linespacing = TTF_FontLineSkip(font->ttffont);
    font->h = ptsize;

    for( i = 0; i < NUMCHARS; i++ ) {
	SDL_Surface *glyph = NULL;
	GLuint height = 0; /* kps - added default value */

	memset(&texcoords, 0, sizeof(texcoords));
	minx = miny = maxx = maxy = 0;

	forecol = &white;
	
    	glyph = TTF_RenderGlyph_Blended( font->ttffont, i, *forecol );
    	if(glyph) {
	    glGetError();
    	    font->textures[i] = SDL_GL_LoadTexture(glyph, &texcoords);
	    gl_error = glGetError();
	    if (font->textures[i] == 0 || gl_error != GL_NO_ERROR)
	    	printf("Warning: Couldn't create texture: 0x%x\n", gl_error);
	    
    	    font->W[i] = glyph->w;
    	    height = glyph->h;
    	    TTF_GlyphMetrics( font->ttffont, i, &minx,&maxx,&miny,&maxy,NULL);
   	}    
    	SDL_FreeSurface(glyph);
		
    	glNewList(font->list_base+i,GL_COMPILE);

    	glBindTexture(GL_TEXTURE_2D, font->textures[i]);
    	glTranslatef(1,0,0);
    	glPushMatrix();
    	glBegin(GL_TRIANGLE_STRIP);
    	    glTexCoord2f(texcoords.MinX, texcoords.MaxY);
	    glVertex2i(0 , miny);
     	    glTexCoord2f(texcoords.MaxX, texcoords.MaxY);
	    glVertex2i(font->W[i] , miny);
    	    glTexCoord2f(texcoords.MinX, texcoords.MinY);
	    glVertex2i(0 ,miny+height );
   	    glTexCoord2f(texcoords.MaxX, texcoords.MinY);
	    glVertex2i(font->W[i] , miny+height);
    	glEnd();
    	glPopMatrix();
    	glTranslatef((font->W[i]>3)?font->W[i]:(font->W[i] = 3) + 1,0,0);
	/*one would think this should be += 2... I guess they overlap or the edge
	 * isn't painted
	 */
	font->W[i] += 1;

    	glEndList();
    }
    
    /*TTF_CloseFont(font->ttffont);*/
    /*TTF_Quit();*/
    return RENDERER_STATUS_OK;
}

static bool Font_is_empty(const font_data *font)
{
    size_t index;

    if (font->ttffont != NULL || font->atlas != NULL
	|| font->text_renderer != NULL
	|| font->list_base != 0 || font->h != 0 || font->linespacing != 0) {
	return false;
    }
    for (index = 0; index < NUMCHARS; index++) {
	if (font->textures[index] != 0 || font->W[index] != 0)
	    return false;
    }
    return true;
}

static void Release_legacy_font_resources(font_data *font)
{
    if (font->list_base != 0) {
	glDeleteLists(font->list_base, next_p2(NUMCHARS));
	font->list_base = 0;
    }
    glDeleteTextures(NUMCHARS, font->textures);
    memset(font->textures, 0, sizeof(font->textures));

    if (font->ttffont != NULL) {
	TTF_CloseFont(font->ttffont);
	font->ttffont = NULL;
    }
}

RendererStatus fontinit(font_data *ft_font, Renderer *renderer,
			const char *fname, unsigned int size)
{
    font_data candidate;
    RendererStatus status;

    if (ft_font == NULL || renderer == NULL || fname == NULL
	|| size == 0 || size > INT_MAX || !Font_is_empty(ft_font)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    memset(&candidate, 0, sizeof(candidate));
    status = Init_legacy_font(&candidate, fname, (int)size);
    if (status == RENDERER_STATUS_OK) {
	status = Text_ttf_atlas_create(renderer, candidate.ttffont,
				       &candidate.atlas);
    }
    if (status != RENDERER_STATUS_OK) {
	Release_legacy_font_resources(&candidate);
	memset(&candidate, 0, sizeof(candidate));
	return status;
    }

    *ft_font = candidate;
    return RENDERER_STATUS_OK;
}

RendererStatus font_text_renderer_attach(font_data *ft_font,
					 SdlRenderer *sdl_renderer)
{
    if (ft_font == NULL || sdl_renderer == NULL || ft_font->atlas == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (ft_font->text_renderer != NULL)
	return RENDERER_STATUS_OK;
    return Text_renderer_create(sdl_renderer, ft_font->atlas,
				&ft_font->text_renderer);
}

RendererStatus fontclean(font_data *ft_font)
{
    RendererStatus status;

    if (ft_font == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    status = Text_atlas_destroy(&ft_font->atlas);
    if (status != RENDERER_STATUS_OK)
	return status;

    /* The facade destroy path is deliberately CPU-only. Keeping it until the
     * fallible atlas release succeeds preserves all font state for retry. */
    Text_renderer_destroy(&ft_font->text_renderer);
    Release_legacy_font_resources(ft_font);
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

static RendererStatus Text_measure_bytes(font_data *ft_font,
					 const char *text, size_t text_length,
					 fontbounds *bounds)
{
    TextGeometryMetrics metrics;
    fontbounds candidate;
    RendererStatus status;

    if (ft_font == NULL || ft_font->text_renderer == NULL || text == NULL
	|| bounds == NULL) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
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
    RendererStatus status;
    char *candidate_text;
    char *old_text;
    size_t text_length;
    int width;
    int height;

    if (ft_font == NULL || ft_font->text_renderer == NULL
	|| text == NULL || string_tex == NULL || ft_font->h > INT_MAX) {
	return false;
    }

    /* Keep the old empty-author compatibility without creating a transient
     * surface or texture for the replacement string. */
    normalized_text = text[0] != '\0' ? text : " ";
    if (string_tex->text_renderer == ft_font->text_renderer
	&& string_tex->cache != NULL && string_tex->text != NULL
	&& strcmp(string_tex->text, normalized_text) == 0) {
	return true;
    }

    text_length = strlen(normalized_text);
    status = Text_renderer_cache_replace(
	&candidate_cache, ft_font->text_renderer,
	(const unsigned char *)normalized_text, text_length);
    if (status != RENDERER_STATUS_OK)
	return false;
    status = Text_renderer_cache_metrics(candidate_cache, &metrics);
    if (status != RENDERER_STATUS_OK || metrics.width != metrics.width
	|| metrics.height != metrics.height || metrics.width < 0.0f
	|| metrics.height < 0.0f || (double)metrics.width > (double)INT_MAX
	|| (double)metrics.height > (double)INT_MAX) {
	Text_renderer_cache_destroy(&candidate_cache);
	return false;
    }
    width = (int)metrics.width;
    height = (int)metrics.height;
    candidate_text = xp_strdup(normalized_text);
    if (candidate_text == NULL) {
	Text_renderer_cache_destroy(&candidate_cache);
	return false;
    }

    old_cache = string_tex->cache;
    old_text = string_tex->text;
    string_tex->text_renderer = ft_font->text_renderer;
    string_tex->cache = candidate_cache;
    string_tex->text = candidate_text;
    string_tex->width = width;
    string_tex->height = height;
    string_tex->font_height = (int)ft_font->h;
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

    if (string_tex == NULL || string_tex->text_renderer == NULL
	|| string_tex->cache == NULL || string_tex->text == NULL) {
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
    RendererStatus status;

    if (ft_font == NULL || ft_font->text_renderer == NULL || text == NULL)
	return Text_adapter_failure(ft_font,
				    RENDERER_STATUS_INVALID_ARGUMENT);
    status = Text_horizontal_alignment(XALIGN, &horizontal);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    status = Text_vertical_alignment(YALIGN, &vertical);
    if (status != RENDERER_STATUS_OK)
	return Text_adapter_failure(ft_font, status);
    if (text_length == 0) {
	TextGeometryMetrics metrics;

	return Text_renderer_measure(
	    ft_font->text_renderer, (const unsigned char *)text,
	    text_length, &metrics);
    }

    anchor.x = (float)x;
    anchor.y = onHUD ? (float)draw_height - (float)y : (float)y;
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
