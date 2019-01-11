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

/* kps - had to add prefix so that these would not conflict with options */
color_t wallRadarColorValue = 0xa0;
color_t targetRadarColorValue = 0xa0;
color_t decorRadarColorValue = 0xff0000;
color_t bgRadarColorValue = 0xa00000ff;

static SDL_Surface *radar_surface;     /* offscreen image with walls */
static GLuint      radar_texture;     /* above as an OpenGL texture */
static SDL_Rect    radar_bounds;
static GLWidget    *radar_widget;

static void Radar_cleanup( GLWidget *widget );
static void Radar_paint( GLWidget *widget );
static void move(Sint16 xrel,Sint16 yrel,Uint16 x,Uint16 y, void *data);

#define RGBA(RGB) \
    ((RGB) & 0xff000000 ? (RGB) & 0xff000000 : 0xff000000 \
     | ((RGB) & 0xff0000) >> 16 \
     | ((RGB) & 0x00ff00) \
     | ((RGB) & 0x0000ff) << 16)

static int pow2_ceil(int t)
{
    int r;
    for (r = 1; r < t; r <<= 1);
    return r;
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

    SDL_FillRect(s, &block, RGBA(color));
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
    SDL_FillRect(s, NULL, RGBA(bgRadarColorValue));

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
    SDL_FillRect(s, NULL, RGBA(bgRadarColorValue));

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
    SDL_Rect *b;
    
    b = &(((GLWidget *)data)->bounds);
    b->x += xrel;
    b->y += yrel;
    sprintf(buf, "%dx%d+%d+%d", 
	    radar_bounds.w, 
	    radar_bounds.h,
	    b->x,
	    b->y);
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

static void Radar_init_texture(GLWidget *widget)
{
    if (oldServer) Radar_paint_world_blocks(widget, radar_surface);
    else Radar_paint_world_polygons(widget, radar_surface);

    glGenTextures(1, &radar_texture);
    glBindTexture(GL_TEXTURE_2D, radar_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		 radar_surface->w, radar_surface->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE,
		 radar_surface->pixels);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
}

static int Radar_init(GLWidget *widget)
{
    radar_surface =
	SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,
                             pow2_ceil(widget->bounds.w-1),
			     pow2_ceil(widget->bounds.h-1), 32,
                             RMASK, GMASK, BMASK, AMASK);
    if (!radar_surface) {
        error("Could not create radar surface: %s", SDL_GetError());
        return -1;
    }
    Radar_init_texture(widget);
    return 0;
}

void Radar_update(void)
{
    glDeleteTextures(1, &radar_texture);
    Radar_init_texture(radar_widget);
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
    if ( !tmp ) {
        error("Failed to malloc in Init_RadarWidget");
	return NULL;
    }
    tmp->WIDGET     	= RADARWIDGET;
    tmp->bounds.x   	= radar_bounds.x;
    tmp->bounds.y   	= radar_bounds.y;
    tmp->bounds.w   	= radar_bounds.w;
    tmp->bounds.h   	= radar_bounds.h * RadarHeight / RadarWidth;
    tmp->Draw	    	= Radar_paint;
    tmp->Close	    	= Radar_cleanup;
    tmp->button     	= button;
    tmp->buttondata 	= tmp;
    tmp->motion     	= move;
    tmp->motiondata 	= tmp;
    
    if (Radar_init(tmp) != 0) {
	free(tmp);
	return NULL;
    }

    radar_widget = tmp;
    return tmp;
}

static void Radar_cleanup( GLWidget *widget )
{
    glDeleteTextures(1, &radar_texture);
    SDL_FreeSurface(radar_surface);
}

static void Radar_set_bounds(GLWidget *widget, int x, int y, int w, int h)
{
    radar_bounds.x = x;
    radar_bounds.y = y;
    radar_bounds.w = w;
    radar_bounds.h = h;
    if (widget != NULL) {
	widget->bounds.x = x;
	widget->bounds.y = y;
	widget->bounds.w = w + 1;
	widget->bounds.h = h * RadarHeight / RadarWidth + 1;
	Radar_cleanup(widget);
	Radar_init(widget);
    }
}

static void Radar_blit_world(SDL_Rect *sr, SDL_Rect *dr)
{
    float tx1, ty1, tx2, ty2;

    tx1 = (float)sr->x / radar_surface->w;
    ty1 = (float)sr->y / radar_surface->h;
    tx2 = ((float)sr->x + sr->w) / radar_surface->w;
    ty2 = ((float)sr->y + sr->h) / radar_surface->h;

    glBindTexture(GL_TEXTURE_2D, radar_texture);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255, 255, 255, 255);

    glBegin(GL_QUADS);
    glTexCoord2f(tx1, ty1); glVertex2i(dr->x, dr->y);
    glTexCoord2f(tx2, ty1); glVertex2i(dr->x + dr->w, dr->y);
    glTexCoord2f(tx2, ty2); glVertex2i(dr->x + dr->w, dr->y + dr->h);
    glTexCoord2f(tx1, ty2); glVertex2i(dr->x, dr->y + dr->h);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

/*
 * Paints the radar surface and objects to the screen.
 */
static void Radar_paint( GLWidget *widget )
{
    float xf, yf;
    
    radar_bounds.x = ((GLWidget *)widget)->bounds.x;
    radar_bounds.y = ((GLWidget *)widget)->bounds.y;
    radar_bounds.w = ((GLWidget *)widget)->bounds.w-1;
    radar_bounds.h = ((GLWidget *)widget)->bounds.h-1;

    xf = (float)radar_bounds.w / (float)Setup->width;
    yf = (float)radar_bounds.h / (float)Setup->height;

    if (instruments.slidingRadar) {

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
        Radar_blit_world(&sr, &dr);

        sr.x = x; sr.y = 0; sr.w = w; sr.h = y;
        dr.x = 0 + radar_bounds.x; dr.y = h + radar_bounds.y;
        dr.w = w; dr.h = y;
        Radar_blit_world(&sr, &dr);

        sr.x = 0; sr.y = y; sr.w = x; sr.h = h;
        dr.x = w + radar_bounds.x; dr.y = 0 + radar_bounds.y;
        dr.w = x; dr.h = h;
        Radar_blit_world(&sr, &dr);

        sr.x = x; sr.y = y; sr.w = w; sr.h = h;
        dr.x = 0 + radar_bounds.x; dr.y = 0 + radar_bounds.y;
        dr.w = w; dr.h = h;
        Radar_blit_world(&sr, &dr);
    } else {
	SDL_Rect sr;
	sr.x = sr.y = 0;
	sr.w = radar_bounds.w; sr.h = radar_bounds.h;
	Radar_blit_world(&sr, &radar_bounds);
    }

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
    sprintf(buf, "%dx%d+%d+%d", 
	    radar_bounds.w, 
	    radar_bounds.h,
	    radar_bounds.x,
	    radar_bounds.y);
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
