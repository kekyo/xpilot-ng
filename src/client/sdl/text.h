/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004  Erik Andersson <deity_at_home.se>
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

#ifndef TEXT_H
#define TEXT_H

#include "xpclient_sdl.h"

#include "sdlpaint.h"
#include "text_atlas.h"
#include "text_renderer.h"

#define LEFT 0
#define DOWN 0
#define CENTER 1
#define RIGHT 2
#define UP 2

#define NUMCHARS 256

typedef struct {
    GLfloat MinX;
    GLfloat MinY;
    GLfloat MaxX;
    GLfloat MaxY;
} texcoord_t;    

typedef struct {
    GLuint textures[NUMCHARS]; /* texture indexes for the characters */
    GLuint W[NUMCHARS]; /* holds paint width fr each character */
    GLuint list_base; /* start of the texture list for this font */
    GLuint h; /* char height */
    GLuint linespacing; /* proper line spacing according to FT */
    TTF_Font *ttffont;
    /** Immutable renderer-backed glyph atlas for this font. */
    TextAtlas *atlas;
    /** Owned semantic text facade borrowing this font's atlas renderer. */
    TextRenderer *text_renderer;
} font_data;

/** CPU-side cached text and its semantic renderer identity. */
typedef struct {
    /** Borrowed text renderer that owns the cache's font identity. */
    TextRenderer *text_renderer;
    /** Owned CPU-side byte-string cache. */
    TextRendererCache *cache;
    /** Owned NUL-terminated copy of the normalized byte string. */
    char *text;
    /** Measured logical width in pixels. */
    int width;
    /** Measured logical height in pixels. */
    int height;
    /** Requested font height retained for compatibility layout. */
    int font_height;
} string_tex_t;

extern int renderstyle;
extern enum rendertype {
	RENDER_LATIN1,
	RENDER_UTF8,
	RENDER_UNICODE
} rendertype;

/** Logical bounds of a formatted byte string. */
typedef struct {
    /** Maximum line width in logical pixels. */
    float width;
    /** Total block height in logical pixels. */
    float height;
} fontbounds;

/**
 * Create a font and its legacy and renderer-backed drawing resources.
 *
 * @param ft_font Zero-initialized empty destination that receives the font on
 * success.
 * @param renderer Renderer that owns the font atlas.
 * @param fname Font file to load.
 * @param size Requested point size.
 * @return Operation status.
 *
 * @remarks Creation is atomic and must occur outside an active renderer
 * frame. The renderer must remain alive until fontclean() succeeds.
 */
RendererStatus fontinit(font_data *ft_font, Renderer *renderer,
			const char *fname, unsigned int size);

/**
 * Attach a semantic text drawing facade to a fully initialized font.
 *
 * @param ft_font Font whose immutable atlas is already initialized.
 * @param sdl_renderer SDL renderer facade owning the atlas renderer.
 * @return Operation status.
 *
 * @remarks Reattaching an already attached font is an idempotent operation;
 * every call for that font must pass the same SDL renderer. Failure leaves
 * every existing font resource unchanged. The SDL renderer must outlive the
 * font and all cached strings created from it.
 */
RendererStatus font_text_renderer_attach(font_data *ft_font,
					 SdlRenderer *sdl_renderer);

/**
 * Free all resources associated with a font.
 *
 * @param ft_font Font to clean; an already-empty font is valid.
 * @return Operation status.
 *
 * @remarks If atlas destruction is rejected, all font resources, including
 * the semantic text facade, remain intact so cleanup can be retried after the
 * active frame ends.
 */
RendererStatus fontclean(font_data *ft_font);

/* loads a SDL surface onto a GL texture */
GLuint SDL_GL_LoadTexture(SDL_Surface *surface, texcoord_t *texcoord);

/**
 * Measure at most a given number of formatted bytes.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param length Maximum formatted byte count; must be nonnegative.
 * @param bounds Receives logical bounds on success and remains unchanged on
 *        failure.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes before @p length is applied. Bytes unavailable in
 * the printable-ASCII atlas use the font's fallback glyph.
 */
RendererStatus nprintsize(font_data *ft_font, int length,
			  fontbounds *bounds, const char *fmt, ...);

/**
 * Measure one formatted byte string.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param bounds Receives logical bounds on success and remains unchanged on
 *        failure.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes. Bytes unavailable in the printable-ASCII atlas use
 * the font's fallback glyph.
 */
RendererStatus printsize(font_data *ft_font, fontbounds *bounds,
			 const char *fmt, ...);

/**
 * Draw at most a given number of formatted bytes in HUD coordinates.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x Legacy horizontal anchor.
 * @param y Legacy bottom-left HUD anchor.
 * @param length Maximum formatted byte count; must be nonnegative.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes before @p length is applied. Bytes unavailable in
 * the printable-ASCII atlas use the font's fallback glyph. Any failure is
 * retained so the incomplete frame is not presented.
 */
RendererStatus HUDnprint(font_data *ft_font, int color, int XALIGN,
			 int YALIGN, int x, int y, int length,
			 const char *fmt, ...);

/**
 * Draw at most a given number of formatted bytes in world coordinates.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x World-space horizontal anchor.
 * @param y World-space vertical anchor.
 * @param length Maximum formatted byte count; must be nonnegative.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes before @p length is applied. Bytes unavailable in
 * the printable-ASCII atlas use the font's fallback glyph. Any failure is
 * retained so the incomplete frame is not presented.
 */
RendererStatus mapnprint(font_data *ft_font, int color, int XALIGN,
			 int YALIGN, int x, int y, int length,
			 const char *fmt, ...);

/**
 * Draw one formatted byte string in HUD coordinates.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x Legacy horizontal anchor.
 * @param y Legacy bottom-left HUD anchor.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes. Bytes unavailable in the printable-ASCII atlas use
 * the font's fallback glyph. Any failure is retained so the incomplete
 * frame is not presented.
 */
RendererStatus HUDprint(font_data *ft_font, int color, int XALIGN,
			int YALIGN, int x, int y, const char *fmt, ...);

/**
 * Draw one formatted byte string in world coordinates.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x World-space horizontal anchor.
 * @param y World-space vertical anchor.
 * @param fmt printf-style format, or NULL for an empty string.
 * @return Operation status.
 *
 * @remarks This operation requires an active renderer frame. Formatting is
 * limited to 1023 bytes. Bytes unavailable in the printable-ASCII atlas use
 * the font's fallback glyph. Any failure is retained so the incomplete
 * frame is not presented.
 */
RendererStatus mapprint(font_data *ft_font, int color, int XALIGN,
			int YALIGN, int x, int y, const char *fmt, ...);

/**
 * Cache and draw one byte string through the semantic text renderer.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x Legacy horizontal anchor.
 * @param y Legacy vertical anchor.
 * @param text NUL-terminated byte string.
 * @param savetex Whether to retain @p string_tex after drawing.
 * @param string_tex Optional zero-initialized or previously populated cache.
 * @param onHUD Whether the anchor uses legacy HUD coordinates.
 * @return True only when caching and drawing both succeed.
 */
bool draw_text(font_data *ft_font, int color, int XALIGN, int YALIGN,
	       int x, int y, const char *text, bool savetex,
	       string_tex_t *string_tex, bool onHUD);

/**
 * Atomically replace a cached byte string without allocating GPU resources.
 *
 * @param ft_font Font with an attached semantic text renderer.
 * @param text NUL-terminated byte string. Empty input is normalized to one
 *        space for legacy layout compatibility.
 * @param string_tex Zero-initialized or previously populated destination.
 * @return True on success. Failure leaves the destination unchanged.
 */
bool render_text(font_data *ft_font, const char *text, string_tex_t *string_tex);

/**
 * Draw a cached string using legacy alignment and coordinate conventions.
 *
 * @param string_tex Populated cached string.
 * @param color Unpremultiplied RGBA value.
 * @param XALIGN Legacy horizontal alignment value.
 * @param YALIGN Legacy vertical alignment value.
 * @param x Legacy horizontal anchor.
 * @param y Legacy vertical anchor.
 * @param onHUD Whether to convert the bottom-left HUD anchor to top-left.
 * @return Operation status.
 */
RendererStatus disp_text(string_tex_t *string_tex, int color,
			 int XALIGN, int YALIGN, int x, int y,
			 bool onHUD);

/**
 * Release a cached string without issuing GPU resource operations.
 *
 * @param string_tex Cache to clear, or NULL for no operation.
 */
void free_string_texture(string_tex_t *string_tex);

extern font_data gamefont;
extern font_data mapfont;

extern int gameFontSize;
extern int mapFontSize;

extern char *gamefontname;

extern string_tex_t score_object_texs[];

/*typedef struct {
    int id;
    string_tex_t string_tex;
    void *next;
} name_tex_t;

name_tex_t *others_name_texs;*/

#define MAX_METERS 12
extern string_tex_t meter_texs[];
#define MAX_HUD_TEXS 10 
extern string_tex_t HUD_texs[];
#endif
