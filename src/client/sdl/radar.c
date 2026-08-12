/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client. Copyright (C) 2003-2004 by 
 *
 *     Juha Lindström <juhal@users.sourceforge.net>
 *     Erik Andersson <deity_at_home.se>
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

#include "sdlpaint.h"
#include "SDL_gfxPrimitives.h"
#include "radar.h"
#include "glwidgets.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <limits.h>
#include <stddef.h>

/* kps - had to add prefix so that these would not conflict with options */
color_t wallRadarColorValue = 0xa0;
color_t targetRadarColorValue = 0xa0;
color_t decorRadarColorValue = 0xff0000;
color_t bgRadarColorValue = 0xa00000ff;

static SDL_Surface *radar_surface;     /* offscreen image with walls */
static Renderer *radar_renderer;      /* owner of the semantic texture */
static RendererTexture *radar_texture;/* above as a renderer texture */
static SDL_Rect radar_bounds;
static SDL_Rect pending_radar_bounds;
static GLWidget *radar_widget;
static int radar_dirty;
static int radar_bounds_pending;

static void Radar_cleanup( GLWidget *widget );
static void Radar_paint( GLWidget *widget );
static void move(Sint16 xrel,Sint16 yrel,Uint16 x,Uint16 y, void *data);

static int pow2_ceil(int value, int *result)
{
    int power = 1;

    if (value <= 0 || result == NULL)
	return -1;
    while (power < value) {
	if (power > INT_MAX / 2)
	    return -1;
	power <<= 1;
    }
    *result = power;
    return 0;
}

static Uint32 Radar_surface_color(SDL_Surface *surface, color_t color)
{
    Uint8 red = (Uint8)(color >> 16);
    Uint8 green = (Uint8)(color >> 8);
    Uint8 blue = (Uint8)color;
    Uint8 alpha = SDL_ALPHA_OPAQUE;

    /* Preserve the old RGBA macro's overloaded color representation: a
     * nonzero high byte meant an alpha-only black radar background. */
    if ((color & 0xff000000) != 0) {
	red = 0;
	green = 0;
	blue = 0;
	alpha = (Uint8)(color >> 24);
    }

    return SDL_MapRGBA(surface->format, red, green, blue, alpha);
}

static int Radar_scale_bounds(const SDL_Rect *requested, SDL_Rect *scaled)
{
    int64_t height;

    if (requested == NULL || scaled == NULL
	|| requested->w <= 0 || requested->h <= 0
	|| RadarWidth == 0 || RadarHeight == 0) {
	return -1;
    }
    height = (int64_t)requested->h * (int64_t)RadarHeight
	/ (int64_t)RadarWidth;
    if (height <= 0 || height > INT_MAX)
	return -1;
    *scaled = *requested;
    scaled->h = (int)height;
    return 0;
}

/*
 * Maps map coordinates or radar coordinates to screen 
 * coordinates.
 */
static void to_screen(GLWidget *radar, int *x, int *y, int from_w, int from_h)
{
    float fx, fy, sx, sy;
    SDL_Rect rb = radar->bounds;

    fx = *x * rb.w / from_w;
    fy = *y * rb.h / from_h;

    if (instruments.slidingRadar) {
	sx = selfPos.x * rb.w / Setup->width;
	sy = selfPos.y * rb.h / Setup->height;
	fx = fx - sx;
	fy = fy - sy;
	if (fx < -rb.w/2)
	    fx += rb.w;
	else if (fx > rb.w/2)
	    fx -= rb.w;
	if (fy < -rb.h/2)
	    fy += rb.h;
	else if (fy > rb.h/2)
	    fy -= rb.h;
	fx += rb.w/2;
	fy += rb.h/2;
    }

    *x = (int)(rb.x + (fx + 0.5));
    *y = (int)(rb.y + rb.h - (fy + 0.5));
}

static void Radar_paint_border(GLWidget *radar)
{
    glBegin(GL_LINE_LOOP);
    glColor4ub(0, 0, 0, 0xff);
    glVertex2i(radar->bounds.x, radar->bounds.y + radar->bounds.h);
    glColor4ub(0, 0x00, 0x90, 0xff);
    glVertex2i(radar->bounds.x, radar->bounds.y);
    glColor4ub(0, 0, 0, 0xff);
    glVertex2i(radar->bounds.x + radar->bounds.w - 1, radar->bounds.y);
    glColor4ub(0, 0x00, 0x90, 0xff);
    glVertex2i(radar->bounds.x + radar->bounds.w - 1,
	       radar->bounds.y + radar->bounds.h);
    glEnd();
}

/*
 * Paints a block in the radar to the given position using the
 * given color. This one doesn't do any locking on the surface.
 */
static void Radar_paint_block(GLWidget *radar, SDL_Surface *s, int xi, int yi, color_t color)
{
    SDL_Rect block;
    block.x = xi * radar->bounds.w / Setup->x;
    block.y = radar->bounds.h - (yi + 1) * radar->bounds.h / Setup->y;
    block.w = (xi + 1) * radar->bounds.w / Setup->x - block.x;
    block.h = radar->bounds.h - yi * radar->bounds.h / Setup->y - block.y;

    SDL_FillRect(s, &block, Radar_surface_color(s, color));
}

/*
 * Paints an image of the world on the radar surface when the map
 * is a block map.
 */
static void Radar_paint_world_blocks(GLWidget *radar, SDL_Surface *s)
{
    double damage;
    int i, xi, yi, type, color;
    color_t bcolor[256]; /* color of a block indexed by block type */

    /*
     * Calculate an array which returns the color to use
     * for drawing when indexed with a map block type.
     */
    memset(bcolor, 0, sizeof bcolor);
    bcolor[SETUP_FILLED] =
	bcolor[SETUP_FILLED_NO_DRAW] =
	bcolor[SETUP_REC_LU] =
	bcolor[SETUP_REC_RU] =
	bcolor[SETUP_REC_LD] =
	bcolor[SETUP_REC_RD] =
	bcolor[SETUP_FUEL] = wallRadarColorValue;
    for (i = 0; i < 10; i++)
	bcolor[SETUP_TARGET+i] = targetRadarColorValue;
    for (i = BLUE_BIT; i < 256; i++)
	bcolor[i] = wallRadarColorValue;
    if (instruments.showDecor) {
	bcolor[SETUP_DECOR_FILLED] =
	    bcolor[SETUP_DECOR_LU] =
	    bcolor[SETUP_DECOR_RU] =
	    bcolor[SETUP_DECOR_LD] =
	    bcolor[SETUP_DECOR_RD] = decorRadarColorValue;
    }

    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    SDL_FillRect(s, NULL, Radar_surface_color(s, bgRadarColorValue));

    /* Scan the map and paint the blocks */
    for (xi = 0; xi < Setup->x; xi++) {
        for (yi = 0; yi < Setup->y; yi++) {

            type = Setup->map_data[xi * Setup->y + yi];

            if (type >= SETUP_TARGET
                && type < SETUP_TARGET + 10
                && !Target_alive(xi, yi, &damage))
                type = SETUP_SPACE;

            color = bcolor[type];
            if (color & 0xffffff)
                Radar_paint_block(radar, s, xi, yi, color);
        }
    }

    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);

}

static void Compute_bounds_radar(ipos_t *min, ipos_t *max, const irec_t *b)
{
    min->x = (0 - (b->x + b->w)) / Setup->width;
    if (0 > b->x + b->w) min->x++;
    max->x = (0 + Setup->width - b->x) / Setup->width;
    if (0 + Setup->width < b->x) max->x--;
    min->y = (0 - (b->y + b->h)) / Setup->height;
    if (0 > b->y + b->h) min->y++;
    max->y = (0 + Setup->height - b->y) / Setup->height;
    if (0 + Setup->height < b->y) max->y--;
}

/*
 * Paints an image of the world on the radar surface when the map
 * is a polygon map.
 */
static void Radar_paint_world_polygons(GLWidget *radar, SDL_Surface *s)
{
    int i, j, xoff, yoff;
    ipos_t min, max;
    Sint16 vx[MAX_VERTICES], vy[MAX_VERTICES];
    color_t color;

    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    SDL_FillRect(s, NULL, Radar_surface_color(s, bgRadarColorValue));

    for (i = 0; i < num_polygons; i++) {

	if (BIT(polygon_styles[polygons[i].style].flags,
		STYLE_INVISIBLE_RADAR)) continue;
	Compute_bounds_radar(&min, &max, &polygons[i].bounds);

	for (xoff = min.x; xoff <= max.x; xoff++) {
	    for (yoff = min.y; yoff <= max.y; yoff++) {

		int x = polygons[i].points[0].x + xoff * Setup->width;
		int y = -polygons[i].points[0].y + (1-yoff) * Setup->height;
		vx[0] = (x * radar->bounds.w) / Setup->width;
		vy[0] = (y * radar->bounds.h) / Setup->height;

		for (j = 1; j < polygons[i].num_points; j++) {
		    x += polygons[i].points[j].x;
		    y -= polygons[i].points[j].y;
		    vx[j]= (x * radar->bounds.w) / Setup->width;
		    vy[j] = (y * radar->bounds.h) / Setup->height;
		}

		color = polygon_styles[polygons[i].style].rgb;
		filledPolygonRGBA(s, vx, vy, j,
				  (color >> 16) & 0xff,
				  (color >> 8) & 0xff,
				  color & 0xff,
				  0xff);
	    }
	}
    }

    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
}

/*
 * Paints objects (ships, etc.) visible in the radar.
 */
static void Radar_paint_objects( GLWidget *radar )
{
    int	i, x, y, s;

    for (i = 0; i < num_radar; i++) {
	x = radar_ptr[i].x;
	y = radar_ptr[i].y;
	s = radar_ptr[i].size;
	to_screen(radar, &x, &y, RadarWidth, RadarHeight);
	x -= s/2;
	y -= s/2;
	if (radar_ptr[i].type == RadarFriend) glColor3ub(0, 0xff, 0);
	else glColor3ub(0xff, 0xff, 0xff);
	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x + s, y);
	glVertex2i(x + s, y + s);
	glVertex2i(x, y + s);
	glEnd();
    }

    if (num_radar)
	RELEASE(radar_ptr, num_radar, max_radar);
}

/*
 * Paints player's ship and direction.
 */
static void Radar_paint_self(GLWidget *radar)
{
    int x, y, x2, y2;
    SDL_Rect rb = radar->bounds;

    if (!selfVisible) return;

    if (instruments.slidingRadar) {
	x = rb.x + rb.w/2;
	/* the sliding radar seems to be off by roughly 1 pixel */
	y = rb.y + rb.h/2 + 1;
    } else {
	x = rb.x + selfPos.x * rb.w / Setup->width;
	y = rb.y + rb.h - selfPos.y * rb.h / Setup->height;
    }
    x2 = (int)(x + 8 * tcos(heading));
    y2 = (int)(y - 8 * tsin(heading));

    glColor4ub(0xff, 0xff, 0xff, 0xff);
    glBegin(GL_LINES);
    glVertex2i(x, y);
    glVertex2i(x2, y2);
    glEnd();

}

/*
 * Paints the next checkpoint.
 */
static void Radar_paint_checkpoint(GLWidget *radar)
{
    int x, y;
    if (BIT(Setup->mode, TIMING)) {
	if (oldServer) {
	    Check_pos_by_index(nextCheckPoint, &x, &y);
	    x = x * BLOCK_SZ + BLOCK_SZ/2;
	    y = y * BLOCK_SZ + BLOCK_SZ/2;
	} else {
	    irec_t b = checks[nextCheckPoint].bounds;
	    x = b.x + b.w/2;
	    y = b.y + b.h/2;
	}
	to_screen(radar, &x, &y, Setup->width, Setup->height);
	
	glColor4ub(0x50, 0x50, 0xff, 0xff);
	glBegin(GL_QUADS);
	glVertex2i(x - 3, y);
	glVertex2i(x, y - 3);
	glVertex2i(x + 3, y);
	glVertex2i(x, y + 3);
	glEnd();
	
    }    
}

static void move(Sint16 xrel,Sint16 yrel,Uint16 x,Uint16 y, void *data)
{
    char buf[40];
    SDL_Rect bounds = radar_bounds_pending
	? pending_radar_bounds : radar_bounds;

    (void)x;
    (void)y;
    (void)data;
    bounds.x += xrel;
    bounds.y += yrel;
    sprintf(buf, "%dx%d+%d+%d", 
	    bounds.w,
	    bounds.h,
	    bounds.x,
	    bounds.y);
    Set_string_option(Find_option("radarGeometry"), buf, xp_option_origin_config);
}

static void button( Uint8 button, Uint8 state , Uint16 x , Uint16 y, void *data )
{
    GLWidget *widget = (GLWidget *)data;
    if (state == SDL_PRESSED) {
    	if (button == 1) {
    	    if (DelGLWidgetListItem( widget->list, widget ))
	    	AppendGLWidgetList( widget->list, widget );
    	}
    	if (button == 2) {
    	    if (DelGLWidgetListItem( widget->list, widget ))
	    	PrependGLWidgetList( widget->list, widget );
	}
    }
}

static void Radar_paint_surface(SDL_Surface *surface,
				const SDL_Rect *bounds)
{
    GLWidget painting_widget;

    memset(&painting_widget, 0, sizeof(painting_widget));
    painting_widget.bounds = *bounds;
    if (oldServer)
	Radar_paint_world_blocks(&painting_widget, surface);
    else
	Radar_paint_world_polygons(&painting_widget, surface);
}

static SDL_Surface *Radar_create_surface(const SDL_Rect *bounds)
{
    SDL_Surface *surface;
    int texture_width;
    int texture_height;

    if (bounds == NULL
	|| pow2_ceil(bounds->w, &texture_width) != 0
	|| pow2_ceil(bounds->h, &texture_height) != 0) {
	error("Radar dimensions are invalid or too large");
	return NULL;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(
	0, texture_width, texture_height, 32, SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
	error("Could not create radar surface: %s", SDL_GetError());
	return NULL;
    }
    if (SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE) < 0
	|| SDL_SetSurfaceAlphaMod(surface, SDL_ALPHA_OPAQUE) < 0) {
	error("Could not configure radar surface: %s", SDL_GetError());
	SDL_FreeSurface(surface);
	return NULL;
    }
    Radar_paint_surface(surface, bounds);
    return surface;
}

static int Radar_create_texture(Renderer *renderer, SDL_Surface *surface,
				RendererTexture **texture)
{
    RendererTextureDesc desc;

    if (renderer == NULL || surface == NULL || surface->pixels == NULL
	|| surface->pitch <= 0 || texture == NULL) {
	return -1;
    }

    desc.width = surface->w;
    desc.height = surface->h;
    desc.filter = RENDERER_TEXTURE_FILTER_NEAREST;
    desc.wrap = RENDERER_TEXTURE_WRAP_CLAMP;
    if (Renderer_texture_create_with_desc(
	    renderer, &desc, surface->pixels, (size_t)surface->pitch,
	    texture) != RENDERER_STATUS_OK) {
	warn("Could not create radar texture");
	return -1;
    }
    return 0;
}

static int Radar_replace(Renderer *renderer, const SDL_Rect *bounds)
{
    SDL_Surface *candidate_surface;
    RendererTexture *candidate_texture = NULL;

    candidate_surface = Radar_create_surface(bounds);
    if (candidate_surface == NULL)
	return -1;
    if (Radar_create_texture(
	    renderer, candidate_surface, &candidate_texture) != 0) {
	SDL_FreeSurface(candidate_surface);
	return -1;
    }

    if (radar_texture != NULL
	&& Renderer_texture_destroy(radar_renderer, radar_texture)
	   != RENDERER_STATUS_OK) {
	warn("Could not replace radar texture");
	if (Renderer_texture_destroy(renderer, candidate_texture)
	    != RENDERER_STATUS_OK) {
	    warn("Could not release replacement radar texture");
	}
	SDL_FreeSurface(candidate_surface);
	return -1;
    }

    SDL_FreeSurface(radar_surface);
    radar_surface = candidate_surface;
    radar_renderer = renderer;
    radar_texture = candidate_texture;
    radar_bounds = *bounds;
    radar_widget->bounds.x = bounds->x;
    radar_widget->bounds.y = bounds->y;
    radar_widget->bounds.w = bounds->w + 1;
    radar_widget->bounds.h = bounds->h + 1;
    radar_dirty = 0;
    radar_bounds_pending = 0;
    return 0;
}

void Radar_update(void)
{
    if (radar_widget == NULL || radar_surface == NULL)
	return;
    radar_dirty = 1;
}

int Radar_prepare(Renderer *renderer)
{
    SDL_Rect target_bounds;
    RendererTexture *texture = NULL;

    if (renderer == NULL || radar_widget == NULL || radar_surface == NULL)
	return -1;
    if (radar_texture != NULL && radar_renderer != renderer)
	return -1;
    if (!radar_dirty && !radar_bounds_pending)
	return radar_texture != NULL ? 0 : -1;

    target_bounds = radar_bounds_pending
	? pending_radar_bounds : radar_bounds;
    if (radar_texture != NULL || radar_bounds_pending)
	return Radar_replace(renderer, &target_bounds);

    /* Before the first texture exists, repainting the already-published
     * surface cannot make an older GPU resource inconsistent. */
    Radar_paint_surface(radar_surface, &target_bounds);
    if (Radar_create_texture(renderer, radar_surface, &texture) != 0)
	return -1;
    radar_renderer = renderer;
    radar_texture = texture;
    radar_dirty = 0;
    return 0;
}

/*
 * The radar is drawn so that first the walls are painted to an offscreen
 * SDL surface. This surface is then converted into an OpenGL texture.
 * For each frame OpenGL is used to paint rectangles with this walls
 * texture and on top of that the radar objects.
 */
GLWidget *Init_RadarWidget(void)
{
    GLWidget *tmp	= Init_EmptyBaseGLWidget();
    SDL_Surface *surface;
    SDL_Rect scaled_bounds;

    if ( !tmp ) {
        error("Failed to malloc in Init_RadarWidget");
	return NULL;
    }
    if (Radar_scale_bounds(&radar_bounds, &scaled_bounds) != 0) {
	error("Invalid radar geometry");
	free(tmp);
	return NULL;
    }
    tmp->WIDGET     	= RADARWIDGET;
    tmp->bounds.x   	= scaled_bounds.x;
    tmp->bounds.y   	= scaled_bounds.y;
    tmp->bounds.w   	= scaled_bounds.w + 1;
    tmp->bounds.h   	= scaled_bounds.h + 1;
    tmp->Draw	    	= Radar_paint;
    tmp->Close	    	= Radar_cleanup;
    tmp->button     	= button;
    tmp->buttondata 	= tmp;
    tmp->motion     	= move;
    tmp->motiondata 	= tmp;
    
    surface = Radar_create_surface(&scaled_bounds);
    if (surface == NULL) {
	free(tmp);
	return NULL;
    }

    radar_surface = surface;
    radar_renderer = NULL;
    radar_texture = NULL;
    radar_bounds = scaled_bounds;
    radar_widget = tmp;
    radar_dirty = 1;
    radar_bounds_pending = 0;
    return tmp;
}

static void Radar_cleanup( GLWidget *widget )
{
    (void)widget;

    if (radar_texture != NULL) {
	if (radar_renderer == NULL
	    || Renderer_texture_destroy(radar_renderer, radar_texture)
	       != RENDERER_STATUS_OK) {
	    warn("Could not destroy radar texture");
	    return;
	}
    }
    radar_texture = NULL;
    radar_renderer = NULL;
    SDL_FreeSurface(radar_surface);
    radar_surface = NULL;
    radar_widget = NULL;
    radar_dirty = 0;
    radar_bounds_pending = 0;
}

static void Radar_set_bounds(GLWidget *widget, int x, int y, int w, int h)
{
    SDL_Rect requested = {x, y, w, h};
    SDL_Rect scaled;

    if (widget == NULL) {
	radar_bounds = requested;
	return;
    }
    if (Radar_scale_bounds(&requested, &scaled) != 0)
	return;
    pending_radar_bounds = scaled;
    radar_bounds_pending = 1;
    radar_dirty = 1;
}

static RendererStatus Radar_blit_world(Renderer *renderer,
				       const SDL_Rect *sr,
				       const SDL_Rect *dr,
				       int *submitted)
{
    float tx1, ty1, tx2, ty2;

    if (sr->w <= 0 || sr->h <= 0 || dr->w <= 0 || dr->h <= 0)
	return RENDERER_STATUS_OK;
    tx1 = (float)sr->x / radar_surface->w;
    ty1 = (float)sr->y / radar_surface->h;
    tx2 = ((float)sr->x + sr->w) / radar_surface->w;
    ty2 = ((float)sr->y + sr->h) / radar_surface->h;

    (*submitted)++;
    return Renderer_draw_sprite(
	renderer, radar_texture,
	(float)dr->x, (float)dr->y,
	(float)(dr->x + dr->w), (float)(dr->y + dr->h),
	tx1, ty1, tx2, ty2,
	(RendererColor){255, 255, 255, 255});
}

/*
 * Paints the radar surface and objects to the screen.
 */
static void Radar_paint( GLWidget *widget )
{
    SdlRenderer *sdl_renderer = Get_sdl_renderer();
    Renderer *renderer = sdl_renderer != NULL
	? Sdl_renderer_frontend(sdl_renderer) : NULL;
    RendererStatus status = RENDERER_STATUS_INVALID_STATE;
    int submitted = 0;

    if (renderer != NULL && renderer == radar_renderer
	&& radar_texture != NULL) {
	status = Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA);
    }

    if (status == RENDERER_STATUS_OK && instruments.slidingRadar) {

        int x, y, w, h;
        float xp, yp, xo, yo;
        SDL_Rect sr, dr;

        xp = (float) (selfPos.x * radar_bounds.w) / Setup->width;
        yp = (float) (selfPos.y * radar_bounds.h) / Setup->height;
        xo = (float) radar_bounds.w / 2;
        yo = (float) radar_bounds.h / 2;
        if (xo <= xp)
            x = (int) (xp - xo + 0.5);
	else
            x = (int) (radar_bounds.w + xp - xo + 0.5);
        if (yo <= yp)
            y = (int) (yp - yo + 0.5);
	else
            y = (int) (radar_bounds.h + yp - yo + 0.5);
	/* CB fixed radar bug   y = radar_bounds.h - y - 1; */
	y = radar_bounds.h - y;
        w = radar_bounds.w - x;
        h = radar_bounds.h - y;

        sr.x = 0; sr.y = 0; sr.w = x; sr.h = y;
        dr.x = w + radar_bounds.x; dr.y = h + radar_bounds.y;
        dr.w = x; dr.h = y;
        status = Radar_blit_world(renderer, &sr, &dr, &submitted);

        sr.x = x; sr.y = 0; sr.w = w; sr.h = y;
        dr.x = 0 + radar_bounds.x; dr.y = h + radar_bounds.y;
        dr.w = w; dr.h = y;
        if (status == RENDERER_STATUS_OK)
	    status = Radar_blit_world(renderer, &sr, &dr, &submitted);

        sr.x = 0; sr.y = y; sr.w = x; sr.h = h;
        dr.x = w + radar_bounds.x; dr.y = 0 + radar_bounds.y;
        dr.w = x; dr.h = h;
        if (status == RENDERER_STATUS_OK)
	    status = Radar_blit_world(renderer, &sr, &dr, &submitted);

        sr.x = x; sr.y = y; sr.w = w; sr.h = h;
        dr.x = 0 + radar_bounds.x; dr.y = 0 + radar_bounds.y;
        dr.w = w; dr.h = h;
        if (status == RENDERER_STATUS_OK)
	    status = Radar_blit_world(renderer, &sr, &dr, &submitted);
    } else if (status == RENDERER_STATUS_OK) {
	SDL_Rect sr;
	sr.x = sr.y = 0;
	sr.w = radar_bounds.w; sr.h = radar_bounds.h;
	status = Radar_blit_world(
	    renderer, &sr, &radar_bounds, &submitted);
    }

    if (submitted > 0) {
	RendererStatus flush_status =
	    Sdl_renderer_flush_preserving_legacy(sdl_renderer);

	if (status == RENDERER_STATUS_OK)
	    status = flush_status;
    }
    if (status != RENDERER_STATUS_OK)
	warn("Could not draw radar texture");

    Radar_paint_checkpoint( widget );
    Radar_paint_self( widget );
    Radar_paint_objects( widget );
    Radar_paint_border( widget );
}

/* these 2 are here to allow linking to libxpclient */
void Paint_sliding_radar(void)
{
    return;
}

void Paint_world_radar(void)
{
    return;
}

void Radar_show_target(int x, int y) {}

void Radar_hide_target(int x, int y) {}

static bool Set_geometry(xp_option_t *opt, const char *s)
{
    int x = 0, y = 0, w = 0, h = 0;

    if (s[0] == '=') {
	sscanf(s, "%*c%d%*c%d%*c%d%*c%d", &w, &h, &x, &y);
    } else {
	sscanf(s, "%d%*c%d%*c%d%*c%d", &w, &h, &x, &y);
    }
    if (w == 0 || h == 0) return false;
    Radar_set_bounds(radar_widget, x, y, w, h);
    return true;
}

static const char* Get_geometry(xp_option_t *opt)
{
    static char buf[40];
    SDL_Rect bounds = radar_bounds_pending
	? pending_radar_bounds : radar_bounds;

    sprintf(buf, "%dx%d+%d+%d", 
	    bounds.w,
	    bounds.h,
	    bounds.x,
	    bounds.y);
    return buf;
}


static xp_option_t radar_options[] = {
    XP_STRING_OPTION(
	"radarGeometry",
	"200x200+10+10",
	NULL,
	0,
	Set_geometry, NULL, Get_geometry,
	XP_OPTFLAG_DEFAULT,
	"Set the radar geometry.\n")
};

void Store_radar_options(void)
{
    STORE_OPTIONS(radar_options);
}

#ifdef XPILOT_RADAR_TEST_HOOKS
SDL_Surface *Radar_test_surface(void)
{
    return radar_surface;
}

RendererTexture *Radar_test_texture(void)
{
    return radar_texture;
}

SDL_Rect Radar_test_bounds(void)
{
    return radar_bounds;
}
#endif
