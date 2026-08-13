/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client. 
 *
 * Copyright (C) 2003-2004 by 
 *
 *      Juha Lindström       <juhal@users.sourceforge.net>
 *      Erik Andersson       <deity_at_home.se>
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

#include "sdlpaint.h"
#include "sdlinit.h"
#include "images.h"
#include "glwidgets.h"
#include "text.h"
#include "asteroid_data.h"

#include <float.h>
#include <limits.h>
#include <math.h>

Uint32 nullRGBA     = 0x00000000;
Uint32 blackRGBA    = 0x000000ff;
Uint32 whiteRGBA    = 0xffffffff;
Uint32 blueRGBA     = 0x0000ffff;
Uint32 redRGBA	    = 0xff0000ff;
Uint32 greenRGBA    = 0x00ff00ff;
Uint32 yellowRGBA   = 0xffff00ff;

Uint32 wallColorRGBA;
Uint32 decorColorRGBA;
Uint32 hudColorRGBA;
Uint32 connColorRGBA;
Uint32 scoreObjectColorRGBA;
Uint32 fuelColorRGBA;
Uint32 messagesColorRGBA;
Uint32 oldmessagesColorRGBA;
Uint32 msgScanBallColorRGBA;
Uint32 msgScanSafeColorRGBA;
Uint32 msgScanCoverColorRGBA;
Uint32 msgScanPopColorRGBA;
Uint32 ballColorRGBA;

Uint32 meterBorderColorRGBA;
Uint32 fuelMeterColorRGBA;
Uint32 powerMeterColorRGBA;
Uint32 turnSpeedMeterColorRGBA;
Uint32 packetSizeMeterColorRGBA;
Uint32 packetLossMeterColorRGBA;
Uint32 packetDropMeterColorRGBA;
Uint32 packetLagMeterColorRGBA;
Uint32 temporaryMeterColorRGBA;

Uint32 dirPtrColorRGBA;
Uint32 hudHLineColorRGBA;
Uint32 hudVLineColorRGBA;
Uint32 hudItemsColorRGBA;
Uint32 fuelGaugeColorRGBA;
Uint32 selfLWColorRGBA;
Uint32 teamLWColorRGBA;
Uint32 enemyLWColorRGBA;
Uint32 teamShipColorRGBA;
Uint32 team0ColorRGBA;
Uint32 team1ColorRGBA;
Uint32 team2ColorRGBA;
Uint32 team3ColorRGBA;
Uint32 team4ColorRGBA;
Uint32 team5ColorRGBA;
Uint32 team6ColorRGBA;
Uint32 team7ColorRGBA;
Uint32 team8ColorRGBA;
Uint32 team9ColorRGBA;
Uint32 shipNameColorRGBA;
Uint32 baseNameColorRGBA;
Uint32 manyLivesColorRGBA;
Uint32 twoLivesColorRGBA;
Uint32 oneLifeColorRGBA;
Uint32 zeroLivesColorRGBA;

Uint32 hudRadarEnemyColorRGBA;
Uint32 hudRadarOtherColorRGBA;
Uint32 hudRadarObjectColorRGBA;

Uint32 scoreInactiveSelfColorRGBA;
Uint32 scoreInactiveColorRGBA;
Uint32 scoreSelfColorRGBA;
Uint32 scoreColorRGBA;
Uint32 scoreOwnTeamColorRGBA;
Uint32 scoreEnemyTeamColorRGBA;

Uint32 selectionColorRGBA;

static int meterWidth	= 60;
static int meterHeight	= 10;

float hudRadarMapScale;
int   hudRadarEnemyShape;
int   hudRadarOtherShape;
int   hudRadarObjectShape;
float hudRadarDotScale;

static double shipLineWidth;
static bool smoothLines;
static bool texturedBalls;
static bool texturedShips;
static GLuint polyListBase = 0;
static GLuint polyEdgeListBase = 0;
static GLsizei polyListCount = 0;
static GLuint asteroid = 0;

irec_t *select_bounds;

string_tex_t score_object_texs[MAX_SCORE_OBJECTS];
string_tex_t meter_texs[MAX_METERS];
string_tex_t HUD_texs[MAX_HUD_TEXS+MAX_SCORE_OBJECTS];

int Gui_init(void);
void Gui_cleanup(void);

static bool Ensure_cached_text(font_data *font, const char *text,
			       string_tex_t *cache)
{
    if (font == NULL || text == NULL || cache == NULL)
	return false;
    if (cache->cache != NULL && cache->text != NULL
	&& strcmp(cache->text, text) == 0) {
	return true;
    }
    return render_text(font, text, cache);
}

/* better to use alpha everywhere, less confusion */
void set_alphacolor(Uint32 color)
{
    glColor4ub((color >> 24) & 255,
    	       (color >> 16) & 255,
	       (color >> 8) & 255,
	       color & 255);
}

static GLubyte get_alpha(Uint32 color)
{
    return (color & 255);
}

static int wrap(int *xp, int *yp)
{
    int			x = *xp, y = *yp;
    int returnval =1;

    if (x < world.x || x > world.x + ext_view_width) {
	if (x < realWorld.x || x > realWorld.x + ext_view_width)
	    returnval = 0;
	*xp += world.x - realWorld.x;
    }
    if (y < world.y || y > world.y + ext_view_height) {
	if (y < realWorld.y || y > realWorld.y + ext_view_height)
	    returnval = 0;
	*yp += world.y - realWorld.y;
    }
    return returnval;
}

#ifndef CALLBACK
#define CALLBACK
#endif

static void CALLBACK vertex_callback(ipos_t *p, irec_t *trec)
{
    if (trec != NULL) {
	glTexCoord2f((p->x + trec->x) / (GLfloat)trec->w,
		     (p->y + trec->y) / (GLfloat)trec->h);
    }
    glVertex2i(p->x, p->y);
}

static void tessellate_polygon(GLUtriangulatorObj *tess, int ind)
{
    int i, x, y, minx, miny;
    xp_polygon_t polygon;
    polygon_style_t p_style;
    image_t *texture = NULL;
    irec_t trec;
    GLdouble v[3] = { 0, 0, 0 };
    ipos_t p[MAX_VERTICES];

    polygon = polygons[ind];
    p_style = polygon_styles[polygon.style];
    
    p[0].x = p[0].y = 0;
    if (BIT(p_style.flags, STYLE_TEXTURED)) {
	texture = Image_get_texture(p_style.texture);
	if (texture != NULL) {
	    x = y = minx = miny = 0;
	    for (i = 1; i < polygon.num_points; i++) {
		x += polygon.points[i].x;
		y += polygon.points[i].y;
		if (x < minx) minx = x;
		if (y < miny) miny = y;
	    }
	    trec.x = -minx;
	    trec.y = -miny - (polygon.bounds.h % texture->height);
	    trec.w = texture->frame_width;
	    trec.h = texture->height;
	}
    }
    glNewList(polyListBase + ind,  GL_COMPILE);
    gluTessBeginPolygon(tess, texture ? &trec : NULL);
    gluTessVertex(tess, v, &p[0]);
    for (i = 1; i < polygon.num_points; i++) {
	v[0] = p[i].x = p[i - 1].x + polygon.points[i].x;
	v[1] = p[i].y = p[i - 1].y + polygon.points[i].y;
	gluTessVertex(tess, v, &p[i]);
    }
    gluTessEndPolygon(tess);
    glEndList();

    glNewList(polyEdgeListBase + ind,  GL_COMPILE);
    if (polygon.edge_styles == NULL) { /* No special edges */
	glBegin(GL_LINE_LOOP);
	x = y = 0;
	glVertex2i(x, y);
	for (i = 1; i < polygon.num_points; i++) {
	    x += polygon.points[i].x;
	    y += polygon.points[i].y;
	    glVertex2i(x, y);
	}
	glEnd();
    }
    else { 	/* This polygon has special edges */
	ipos_t pos1, pos2;
	int sindex;

	glBegin(GL_LINES);
	pos1.x = 0;
	pos1.y = 0;
	for (i = 1; i < polygon.num_points; i++) {
	    pos2.x = pos1.x + polygon.points[i].x;
	    pos2.y = pos1.y + polygon.points[i].y;
	    sindex = polygon.edge_styles[i - 1];
	    /* Style 0 means internal edges which are never shown */
	    if (sindex != 0) {
		glVertex2i(pos1.x, pos1.y);
		glVertex2i(pos2.x, pos2.y);
	    }
	    pos1 = pos2;
	}
	glEnd();
    }
    glEndList();
}

static int asteroid_init(void)
{
    int i;
    if ((asteroid = glGenLists(1)) == 0) 
	return -1;
    glNewList(asteroid, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for (i = 0; i < VERTEX_COUNT; i++) {
	glNormal3fv(normal_vectors[i]); 
	glTexCoord2fv(uv_vectors[i]);
	glVertex3fv(vertex_vectors[i]);
    }
    glEnd();
    glEndList();
    return 0;
}

static void asteroid_cleanup(void)
{
    if (asteroid) {
	glDeleteLists(asteroid, 1);
        asteroid = 0;
    }
}

int Gui_init(void)
{
    int i;
    GLUtriangulatorObj *tess = NULL;

    if (asteroid_init() == -1) {
	error("failed to initialize asteroids");
	return -1;
    }
    
    if (num_polygons == 0) return 0;

    polyListCount = num_polygons;
    polyListBase = glGenLists(polyListCount);
    if (!polyListBase) {
	error("failed to generate display lists");
	goto fail;
    }
    polyEdgeListBase = glGenLists(polyListCount);
    if (!polyEdgeListBase) {
	error("failed to generate edge display lists");
	goto fail;
    }

    tess = gluNewTess();
    if (tess == NULL) {
	error("failed to create tessellation object");
	goto fail;
    }

    /* TODO: figure out proper casting here do not use _GLUfuncptr */
    /* it doesn't work on windows  or MAC OS X */
#ifdef _MSC_VER 
    gluTessCallback(tess, GLU_TESS_BEGIN, glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, vertex_callback);
#else
    gluTessCallback(tess, GLU_TESS_BEGIN, (GLvoid (*)(void))glBegin);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid (*)(void))vertex_callback);
#endif
    gluTessCallback(tess, GLU_TESS_END, glEnd);

    for (i = 0; i < num_polygons; i++) {
	tessellate_polygon(tess, i);
    }

    gluDeleteTess(tess);

    return 0;

fail:
    if (tess != NULL)
	gluDeleteTess(tess);
    Gui_cleanup();
    return -1;
}

void Gui_cleanup(void)
{
    if (polyListBase) {
	glDeleteLists(polyListBase, polyListCount);
        polyListBase = 0;
    }
    if (polyEdgeListBase) {
	glDeleteLists(polyEdgeListBase, polyListCount);
        polyEdgeListBase = 0;
    }
    polyListCount = 0;
    asteroid_cleanup();
}

#ifdef XPILOT_GL_TEST_HOOKS
void Gui_test_get_display_lists(GLuint *asteroid_list,
				GLuint *polygon_fill_list_base,
				GLuint *polygon_edge_list_base)
{
    if (asteroid_list != NULL)
	*asteroid_list = asteroid;
    if (polygon_fill_list_base != NULL)
	*polygon_fill_list_base = polyListBase;
    if (polygon_edge_list_base != NULL)
	*polygon_edge_list_base = polyEdgeListBase;
}
#endif

/* Map painting */

void Gui_paint_cannon(int x, int y, int type)
{
    switch (type) {
    case SETUP_CANNON_UP:
        Image_paint(IMG_CANNON_DOWN, x, y, 0, whiteRGBA);
        break;
    case SETUP_CANNON_DOWN:
        Image_paint(IMG_CANNON_UP, x, y + 1, 0, whiteRGBA);
        break;
    case SETUP_CANNON_LEFT:
        Image_paint(IMG_CANNON_RIGHT, x, y, 0, whiteRGBA);
        break;
    case SETUP_CANNON_RIGHT:
        Image_paint(IMG_CANNON_LEFT, x - 1, y, 0, whiteRGBA);
        break;
    default:
        errno = 0;
        error("Bad cannon dir.");
        return;
    }
}

void Gui_paint_fuel(int x, int y, double fuel)
{
#define FUEL_BORDER 3

    int size, frame;
    irec_t area;
    image_t *img;

    img = Image_get(IMG_FUEL);
    if (img == NULL) return;

    /* x + x * y will give a pseudo random number,
     * so different fuelcells will not be displayed with the same
     * image-frame. */
    frame = ABS(loopsSlow + x + x * y) % (img->num_frames * 2);

    /* the animation is played from image 0-15 then back again
     * from image 15-0 */
    if (frame >= img->num_frames)
	frame = (2 * img->num_frames - 1) - frame;

    size = (int)((BLOCK_SZ - 2 * FUEL_BORDER) * fuel / MAX_STATION_FUEL);

    Image_paint(IMG_FUELCELL, x, y, 0, fuelColorRGBA);

    area.x = 0;
    area.y = (int)((BLOCK_SZ - 2 * FUEL_BORDER)
		   * (1 - fuel / MAX_STATION_FUEL));
    area.w = BLOCK_SZ - 2 * FUEL_BORDER;
    area.h = size;
    Image_paint_area(IMG_FUEL,
		     x + FUEL_BORDER,
		     y + FUEL_BORDER,
		     frame,
		     &area,
		     fuelColorRGBA);
}

void Gui_paint_base(int x, int y, int id, int team, int type)
{
    Uint32 color;
    homebase_t *base = NULL;
    other_t *other;
    bool do_basewarning = false;

    switch (type) {
    case SETUP_BASE_UP:
        Image_paint(IMG_BASE_DOWN, x, y, 0, whiteRGBA);
        break;
    case SETUP_BASE_DOWN:
        Image_paint(IMG_BASE_UP, x, y + 1, 0, whiteRGBA);
        break;
    case SETUP_BASE_LEFT:
        Image_paint(IMG_BASE_RIGHT, x, y, 0, whiteRGBA);
        break;
    case SETUP_BASE_RIGHT:
        Image_paint(IMG_BASE_LEFT, x - 1, y, 0, whiteRGBA);
        break;
    default:
        errno = 0;
        error("Bad base dir.");
        return;
    }

    if (!(other = Other_by_id(id))) return;

    if (baseNameColorRGBA) {
	if (!(color = Life_color(other)))
	    color = baseNameColorRGBA;
    } else
	color = whiteRGBA;

    x = x + BLOCK_SZ / 2;
    y = y + BLOCK_SZ / 2;

    base = Homebase_by_id(id);
    if (base != NULL) {
	/*
	 * Hacks to support Mara's base warning on new servers and
	 * the red "meter" basewarning on old servers.
	 */
	if (loops < base->appeartime)
	    do_basewarning = true;

	if (version < 0x4F12 && do_basewarning) {
	    if (baseWarningType & 1) {
		/* We assume the ship will appear after 3 seconds. */
		int count = (int)(360 * (base->appeartime - loops)
				  / (3 * clientFPS));
		LIMIT(count, 0, 360);
		/* red box basewarning */
		if (count > 0 && (baseWarningType & 1))
		    Gui_paint_appearing(x, y,
					id, count);
	    }
	}
    }
    /* Mara's flashy basewarning */
    if (do_basewarning && (baseWarningType & 2)) {
	if (loopsSlow & 1) {
	    if (color != whiteRGBA)
		color = whiteRGBA;
	    else
		color = redRGBA;
	}
    }

    switch (type) {
    case SETUP_BASE_UP:
	mapnprint(&mapfont,color,CENTER,DOWN ,(x) ,(y - BLOCK_SZ / 2),maxCharsInNames,"%s",other->nick_name);
        break;
    case SETUP_BASE_DOWN:
	mapnprint(&mapfont,color,CENTER,UP   ,(x) ,(int)(y + BLOCK_SZ / 1.5),maxCharsInNames,"%s",other->nick_name);
        break;
    case SETUP_BASE_LEFT:
	mapnprint(&mapfont,color,RIGHT,UP    ,(x + BLOCK_SZ / 2) ,(y),maxCharsInNames,"%s",other->nick_name);
        break;
    case SETUP_BASE_RIGHT:
	mapnprint(&mapfont,color,LEFT,UP     ,(x - BLOCK_SZ / 2) ,(y),maxCharsInNames,"%s",other->nick_name);
        break;
    default:
        errno = 0;
        error("Bad base dir.");
    }
}

void Gui_paint_decor(int x, int y, int xi, int yi, int type,
		     bool last, bool more_y)
{
	int mask;
    static unsigned char    decor[256];
    static int		    decorReady = 0;

    if (!decorReady) {
		memset(decor, 0, sizeof decor);
		decor[SETUP_DECOR_FILLED]
			= DECOR_UP | DECOR_LEFT | DECOR_DOWN | DECOR_RIGHT;
		decor[SETUP_DECOR_RU] = DECOR_UP | DECOR_RIGHT | DECOR_CLOSED;
		decor[SETUP_DECOR_RD]
			= DECOR_DOWN | DECOR_RIGHT | DECOR_OPEN | DECOR_BELOW;
		decor[SETUP_DECOR_LU] = DECOR_UP | DECOR_LEFT | DECOR_OPEN;
		decor[SETUP_DECOR_LD]
			= DECOR_LEFT | DECOR_DOWN | DECOR_CLOSED | DECOR_BELOW;
    }

    mask = decor[type];

    set_alphacolor(decorColorRGBA);
    glBegin(GL_LINES);

    if (mask & DECOR_LEFT) {
		glVertex2i(x, y);
		glVertex2i(x, y + BLOCK_SZ);
    }
    if (mask & DECOR_DOWN) {
		glVertex2i(x, y);
		glVertex2i(x + BLOCK_SZ, y);
    }
    if (mask & DECOR_RIGHT) {
		glVertex2i(x + BLOCK_SZ, y);
		glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    }
    if (mask & DECOR_UP) {
		glVertex2i(x, y + BLOCK_SZ);
		glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    }
	if (mask & DECOR_OPEN) {
		glVertex2i(x, y);
		glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    } else if (mask & DECOR_CLOSED) {
		glVertex2i(x, y + BLOCK_SZ);
		glVertex2i(x + BLOCK_SZ, y);
    }
    glEnd();
}

void Gui_paint_border(int x, int y, int xi, int yi)
{
    set_alphacolor(wallColorRGBA);
    glBegin(GL_LINES);
    	glVertex2i(x, y);
    	glVertex2i(xi, yi);
    glEnd();
}

void Gui_paint_visible_border(int x, int y, int xi, int yi)
{
    setupPaint_moving();
    set_alphacolor(hudColorRGBA);
    glBegin(GL_LINE_LOOP);
    	glVertex2i(x, y);
    	glVertex2i(x, yi);
    	glVertex2i(xi, yi);
    	glVertex2i(xi, y);
    glEnd();
    setupPaint_stationary();
}

void Gui_paint_hudradar_limit(int x, int y, int xi, int yi)
{
    set_alphacolor(blueRGBA);
    glBegin(GL_LINE_LOOP);
    	glVertex2i(x, y);
    	glVertex2i(x, yi);
    	glVertex2i(xi, yi);
    	glVertex2i(xi, y);
    glEnd();
}

void Gui_paint_setup_check(int x, int y, bool isNext)
{
    if (isNext) {
	Image_paint(IMG_CHECKPOINT, x, y, 0, whiteRGBA);
    } else {
	Image_paint(IMG_CHECKPOINT, x, y, 0, whiteRGBA - 128);
    }
}

void Gui_paint_setup_acwise_grav(int x, int y)
{
    Image_paint(IMG_ACWISEGRAV, x, y, loopsSlow % 6, whiteRGBA);
}

void Gui_paint_setup_cwise_grav(int x, int y)
{
    Image_paint(IMG_CWISEGRAV, x, y, loopsSlow % 6, whiteRGBA);
}

void Gui_paint_setup_pos_grav(int x, int y)
{
    Image_paint(IMG_PLUSGRAVITY, x, y, 0, whiteRGBA);
}

void Gui_paint_setup_neg_grav(int x, int y)
{
    Image_paint(IMG_MINUSGRAVITY, x, y, 0, whiteRGBA);
}

static void paint_dir_grav(int x, int y, int dir)
{
    const int sz = BLOCK_SZ;
    int cb, c0, c1, c2, p1, p2, swp;

    cb = redRGBA - 255;
    p1 = loops % sz;
    p2 = (loops + sz / 2) % sz;

    if (p1 < p2) {
	c0 = cb + p1 * 128 / (sz / 2);
	c1 = cb;
	c2 = cb + 128;
    } else {
	c0 = cb + (sz - p1) * 128 / (sz / 2);
	c1 = cb + 128;
	c2 = cb;
	swp = p1; p1 = p2; p2 = swp;
    }

    glEnable(GL_BLEND);

#define GRAV(x0,y0,x1,y1,x2,y2,x3,y3,x4,y4,x5,y5,x6,y6,x7,y7) \
    glBegin(GL_QUAD_STRIP);\
    set_alphacolor(c0); glVertex2i(x+x0,y+y0); glVertex2i(x+x1,y+y1);\
    set_alphacolor(c1); glVertex2i(x+x2,y+y2); glVertex2i(x+x3,y+y3);\
    set_alphacolor(c2); glVertex2i(x+x4,y+y4); glVertex2i(x+x5,y+y5);\
    set_alphacolor(c0); glVertex2i(x+x6,y+y6); glVertex2i(x+x7,y+y7);\
    glEnd();

    switch(dir) {
    case 0: /* up */
	GRAV( 0,  0, sz,  0,
	      0, p1, sz, p1,
	      0, p2, sz, p2,
	      0, sz, sz, sz);
	break;
    case 1: /* right */
	GRAV( 0,  0,  0, sz,
	     p1,  0, p1, sz,
	     p2,  0, p2, sz,
	     sz,  0, sz, sz);
	break;
    case 2: /* down */
	GRAV( 0,    sz, sz,    sz,
	      0, sz-p1, sz, sz-p1,
	      0, sz-p2, sz, sz-p2,
	      0,     0, sz,     0);
	break;
    case 3: /* left */
	GRAV(   sz, 0,    sz, sz,
	     sz-p1, 0, sz-p1, sz,
	     sz-p2, 0, sz-p2, sz,
	        0,  0,     0, sz);
	break;
    }
#undef GRAV

    glDisable(GL_BLEND);
}

void Gui_paint_setup_up_grav(int x, int y)
{
    paint_dir_grav(x, y, 0);
}

void Gui_paint_setup_down_grav(int x, int y)
{
    paint_dir_grav(x, y, 2);
}

void Gui_paint_setup_right_grav(int x, int y)
{
    paint_dir_grav(x, y, 1);
}

void Gui_paint_setup_left_grav(int x, int y)
{
    paint_dir_grav(x, y, 3);
}

void Gui_paint_setup_worm(int x, int y)
{
    Image_paint_rotated(IMG_WORMHOLE, x, y, (loopsSlow << 3) % TABLE_SIZE, whiteRGBA);
}

void Gui_paint_setup_item_concentrator(int x, int y)
{
    Image_paint_rotated(IMG_CONCENTRATOR, x, y, (loopsSlow << 3) % TABLE_SIZE, whiteRGBA);
}

void Gui_paint_setup_asteroid_concentrator(int x, int y)
{
    Image_paint_rotated(IMG_ASTEROIDCONC, x, y, (loopsSlow << 4) % TABLE_SIZE, whiteRGBA);
}

void Gui_paint_decor_dot(int x, int y, int size)
{
	set_alphacolor(wallColorRGBA);
	glBegin(GL_QUADS);
	glVertex2i(x + ((BLOCK_SZ - size) >> 1),
			   y + ((BLOCK_SZ - size) >> 1));
	glVertex2i(x + ((BLOCK_SZ - size) >> 1),
			   y + ((BLOCK_SZ + size) >> 1));
	glVertex2i(x + ((BLOCK_SZ + size) >> 1),
			   y + ((BLOCK_SZ + size) >> 1));
	glVertex2i(x + ((BLOCK_SZ + size) >> 1),
			   y + ((BLOCK_SZ - size) >> 1));
	glEnd();
}

void Gui_paint_setup_target(int x, int y, int team, double damage, bool own)
{
	int damage_y;

	Image_paint(IMG_TARGET, x, y, 0, whiteRGBA);
	if (BIT(Setup->mode, TEAM_PLAY)) {
		mapprint(&mapfont, whiteRGBA, RIGHT, UP, x + BLOCK_SZ, y, "%d", team);
	}
	if (damage != TARGET_DAMAGE) {
		damage_y = y + (int)((BLOCK_SZ - 3) * (damage / TARGET_DAMAGE));
		set_alphacolor(own ? blueRGBA : redRGBA);
		glBegin(GL_LINE_LOOP);
		glVertex2i(x, y + 3);
		glVertex2i(x, y + BLOCK_SZ);
		glVertex2i(x + 5, y + BLOCK_SZ);
		glVertex2i(x + 5, y + 3);
		glEnd();
		glBegin(GL_QUADS);
		glVertex2i(x, y + 3);
		glVertex2i(x, damage_y);
		glVertex2i(x + 5, damage_y);
		glVertex2i(x + 5, y + 3);	
		glEnd();
	}
}

void Gui_paint_setup_treasure(int x, int y, int team, bool own)
{
    Image_paint(own ? IMG_HOLDER_FRIEND : IMG_HOLDER_ENEMY, x, y, 0, whiteRGBA);
}

void Gui_paint_walls(int x, int y, int type)
{
    set_alphacolor(wallColorRGBA);
    glBegin(GL_LINES);


    if (type & BLUE_LEFT) {
	glVertex2i(x, y);
	glVertex2i(x, y + BLOCK_SZ);
    }
    if (type & BLUE_DOWN) {
	glVertex2i(x, y);
	glVertex2i(x + BLOCK_SZ, y);
    }
    if (type & BLUE_RIGHT) {
	glVertex2i(x + BLOCK_SZ, y);
	glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    }
    if (type & BLUE_UP) {
	glVertex2i(x, y + BLOCK_SZ);
	glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    }
    if ((type & BLUE_FUEL) == BLUE_FUEL) {
    } else if (type & BLUE_OPEN) {
	glVertex2i(x, y);
	glVertex2i(x + BLOCK_SZ, y + BLOCK_SZ);
    } else if (type & BLUE_CLOSED) {
	glVertex2i(x, y + BLOCK_SZ);
	glVertex2i(x + BLOCK_SZ, y);
    }
    glEnd();
}

void Gui_paint_filled_slice(int bl, int tl, int tr, int br, int y)
{
    set_alphacolor(wallColorRGBA);
    glBegin(GL_QUADS);
    glVertex2i(bl, y);
    glVertex2i(tl, y + BLOCK_SZ);
    glVertex2i(tr, y + BLOCK_SZ);
    glVertex2i(br, y);
    glEnd();
}

void Gui_paint_polygon(int i, int xoff, int yoff)
{
    xp_polygon_t polygon;
    polygon_style_t p_style;
    edge_style_t e_style;
    int width;
    bool did_fill = false;

    polygon = polygons[i];
    p_style = polygon_styles[polygon.style];
    e_style = edge_styles[p_style.def_edge_style];

    if (BIT(p_style.flags, STYLE_INVISIBLE))
	return;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(polygon.points[0].x * clData.scale +
		 rint((xoff * Setup->width - world.x) * clData.scale),
		 polygon.points[0].y * clData.scale +
		 rint((yoff * Setup->height - world.y) * clData.scale), 0);
    glScalef(clData.scale, clData.scale, 0);

    /* possibly paint the polygon as filled or textured */
    if ((instruments.texturedWalls || instruments.filledWorld) &&
	    BIT(p_style.flags, STYLE_TEXTURED | STYLE_FILLED)) {
	if (BIT(p_style.flags, STYLE_TEXTURED)
	        && instruments.texturedWalls) {
	    Image_use_texture(p_style.texture);
	    glCallList(polyListBase + i);
	    Image_no_texture();
	}
	else {
	    set_alphacolor((p_style.rgb << 8) | 0xff);
	    glCallList(polyListBase + i);
	}
	did_fill = true;
    }

    width = e_style.width;
    if (!did_fill && width == -1)
	width = 1;

    /* possibly paint the edges around the polygon */
    if (width != -1) {
	set_alphacolor((e_style.rgb << 8) | 0xff);
	glLineWidth(width * clData.scale);
	if (smoothLines) {
	    glEnable(GL_BLEND);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glEnable(GL_LINE_SMOOTH);
	}
	glCallList(polyEdgeListBase + i);
	if (smoothLines) {
	    glDisable(GL_LINE_SMOOTH);
	    glDisable(GL_BLEND);
	}
	glLineWidth(1);
    }
    glPopMatrix();
}


/* Object painting */


void Gui_paint_item_object(int type, int x, int y)
{
    int sz = 16;
    Image_paint(IMG_ALL_ITEMS, x - 8, y - 4, type, whiteRGBA);
    set_alphacolor(blueRGBA);
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x + sz, y + sz);
    glVertex2i(x, y - sz);
    glVertex2i(x - sz, y + sz);
    glEnd();
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
}

void Gui_paint_ball(int x, int y, int style)
{
    int rgba = ballColorRGBA;

    if (style >= 0 && style < num_polygon_styles)
	rgba = (polygon_styles[style].rgb << 8) | 0xff;

    if (texturedBalls)
	Image_paint(IMG_BALL, x - BALL_RADIUS, y - BALL_RADIUS, 0, rgba);
    else {
	int i, numvert = 16, ang = RES / numvert;
	/* kps hack, feel free to improve */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	set_alphacolor(ballColorRGBA);
	if (smoothLines) glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_LOOP);
	for (i = 0; i < numvert; i++)
	    glVertex2d((double)x + tcos(i * ang) * BALL_RADIUS,
		       (double)y + tsin(i * ang) * BALL_RADIUS);
	glEnd();
	if (smoothLines) glDisable(GL_LINE_SMOOTH);
    }
}

void Gui_paint_ball_connector(int x_1, int y_1, int x_2, int y_2)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    set_alphacolor(connColorRGBA);
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex2i(x_1, y_1);
    glVertex2i(x_2, y_2);
    glEnd();
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
}

void Gui_paint_mine(int x, int y, int teammine, char *name)
{
    Image_paint(teammine ? IMG_MINE_TEAM : IMG_MINE_OTHER,
		x - 10, y - 7, 0, whiteRGBA);
    if (name) {
	mapnprint(  &mapfont, 
    	    	    teammine ? blueRGBA : whiteRGBA,
    	    	    CENTER, DOWN,
    	    	    x, y - 15, 
    	    	    maxCharsInNames,"%s", name	);
    }
}

void Gui_paint_spark(int color, int x, int y)
{
    /*
    Image_paint(IMG_SPARKS,
		x + world.x,
		world.y + ext_view_height - y,
		color);
    */
    glColor3ub(255 * (color + 1) / 8,
	       255 * color * color / 64,
	       0);
    glPointSize(sparkSize);
    glBegin(GL_POINTS);
    glVertex2i(x + world.x, world.y + ext_view_height - y);
    glEnd();
}

void Gui_paint_wreck(int x, int y, bool deadly, int wtype, int rot, int size)
{
    int cnt, tx, ty;

    set_alphacolor(deadly ? whiteRGBA : redRGBA);
    glBegin(GL_LINE_LOOP);
    for (cnt = 0; cnt < NUM_WRECKAGE_POINTS; cnt++) {
	tx = (int)(wreckageShapes[wtype][cnt][rot].x * size) >> 8;
	ty = (int)(wreckageShapes[wtype][cnt][rot].y * size) >> 8;
	glVertex2i(x + tx, y + ty);
    }
    glEnd();
}

void Gui_paint_asteroids_begin(void)
{
    image_t *img;
    GLfloat ambient[] = { 0.7F, 0.7F, 0.7F, 1.0F };

    img = Image_get(IMG_ASTEROID);
    if (img != NULL) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, img->legacy_name);
    }
    glColor4ub(255, 255, 255, 255);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glEnable(GL_CULL_FACE);
}

void Gui_paint_asteroids_end(void)
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

void Gui_paint_asteroid(int x, int y, int type, int rot, int size)
{
    GLfloat real_size;

    real_size = 0.9 * SHIP_SZ * size;
    glPushMatrix();
    glTranslatef((GLfloat)x, (GLfloat)y, 0.0);
    glScalef(real_size, real_size, 1.0);
    glRotatef(360.0 * rot / TABLE_SIZE,
	   (type & 1) - 0.5,
	   (type & 2) - 1,
	   (type & 4) - 2);
    glCallList(asteroid);
    glPopMatrix();
}


/*
 * It seems that currently the screen coordinates are calculated already
 * in paintobjects.c. This should be changed.
 */
void Gui_paint_fastshot(int color, int x, int y)
{
    int size = MIN(shotSize, 16);

    Image_paint(IMG_BULLET,
		x + world.x - size/2,
		world.y - 16 + size/2 - 1 + ext_view_height - y,
		size - 1, whiteRGBA);
}

void Gui_paint_teamshot(int x, int y)
{
    int size = MIN(teamShotSize, 16);

    Image_paint(IMG_BULLET_OWN,
		x + world.x - size/2,
		world.y - 16 + size/2 - 1 + ext_view_height - y,
		size - 1, whiteRGBA);
}

void Gui_paint_missiles_begin(void)
{
}

void Gui_paint_missiles_end(void)
{
}

void Gui_paint_missile(int x, int y, int len, int dir)
{
    Image_paint_rotated(IMG_MISSILE, x - 16, y - 16, dir, whiteRGBA);
}

void Gui_paint_lasers_begin(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
}

void Gui_paint_lasers_end(void)
{
    glDisable(GL_BLEND);
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
}

void Gui_paint_laser(int color, int x_1, int y_1, int len, int dir)
{
    int	x_2, y_2, rgba;
    x_2 = (int)(x_1 + len * tcos(dir));
    y_2 = (int)(y_1 + len * tsin(dir));

    rgba = 
	(color == RED) ? redRGBA :
	(color == BLUE) ? blueRGBA : 
	whiteRGBA;

    set_alphacolor(rgba - 128);
    glLineWidth(5);
    glBegin(GL_LINES);
    glVertex2i(x_1, y_1);
    glVertex2i(x_2, y_2);
    glEnd();

    set_alphacolor(rgba);
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2i(x_1, y_1);
    glVertex2i(x_2, y_2);
    glEnd();
}

void Gui_paint_paused(int x, int y, int count)
{
    Image_paint(IMG_PAUSED,
		x - BLOCK_SZ / 2,
		y - BLOCK_SZ / 2,
		(count <= 0 || loopsSlow % 10 >= 5) ? 1 : 0, whiteRGBA);
}

void Gui_paint_appearing(int x, int y, int id, int count)
{
    const unsigned hsize = 3 * BLOCK_SZ / 7;
    int minx,miny,maxx,maxy;
    Uint32 color;
    other_t *other = Other_by_id(id);

    /* Make a note we are doing the base warning */
    if (version >= 0x4F12) {
	homebase_t *base = Homebase_by_id(id);
	if (base != NULL)
	    base->appeartime = (long)(loops + (count * clientFPS) / 120);
    }

    minx = x - (int)hsize;
    miny = y - (int)hsize;
    maxx = minx + 2 * hsize + 1;
    maxy = miny + (unsigned)(count / 180. * hsize + 1);

    color = Life_color(other);
    set_alphacolor((color)?color:redRGBA);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    	glVertex2i(minx , miny);
    	glVertex2i(maxx , miny);
    	glVertex2i(maxx , maxy);
    	glVertex2i(minx , maxy);
    glEnd();
}

void Gui_paint_ecm(int x, int y, int size)
{
}

void Gui_paint_refuel(int x_0, int y_0, int x_1, int y_1)
{
    int stipple = 4;

    set_alphacolor(fuelColorRGBA);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineStipple(stipple, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex2i(x_0, y_0);
    glVertex2i(x_1, y_1);
    glEnd();
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
    glDisable(GL_LINE_STIPPLE);
}

void Gui_paint_connector(int x_0, int y_0, int x_1, int y_1, int tractor)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    set_alphacolor(connColorRGBA);
    glLineStipple(tractor ? 2 : 4, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex2i(x_0, y_0);
    glVertex2i(x_1, y_1);
    glEnd();
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
    glDisable(GL_LINE_STIPPLE);
    /*glDisable(GL_BLEND);*/
}

void Gui_paint_transporter(int x_0, int y_0, int x_1, int y_1)
{
}

void Gui_paint_all_connectors_begin(void)
{
}

void Gui_paint_ships_begin(void)
{
}

void Gui_paint_ships_end(void)
{
}

/*
 * Assume MAX_TEAMS is 10
 */
int Team_color(int team)
{
    switch (team) {
    case 0:	return team0ColorRGBA;
    case 1:	return team1ColorRGBA;
    case 2:	return team2ColorRGBA;
    case 3:	return team3ColorRGBA;
    case 4:	return team4ColorRGBA;
    case 5:	return team5ColorRGBA;
    case 6:	return team6ColorRGBA;
    case 7:	return team7ColorRGBA;
    case 8:	return team8ColorRGBA;
    case 9:	return team9ColorRGBA;
    default:    break;
    }
    return 0;
}

int Life_color_by_life(int life)
{
    int color;

    if (life > 2)
	color = manyLivesColorRGBA;
    else if (life == 2)
	color = twoLivesColorRGBA;
    else if (life == 1)
	color = oneLifeColorRGBA;
    else /* we catch all */
	color = zeroLivesColorRGBA;
    return color;
}

int Life_color(other_t *other)
{
    int color = 0; /* default is 'no special color' */

    if (other
	&& (other->mychar == ' ' || other->mychar == 'R')
	&& BIT(Setup->mode, LIMITED_LIVES))
	color = Life_color_by_life(other->life);
    return color;
}

static int Gui_is_my_tank(other_t *other)
{
    char	tank_name[MAX_NAME_LEN];

    if (self == NULL
	|| other == NULL
	|| other->mychar != 'T'
	|| (BIT(Setup->mode, TEAM_PLAY)
	&& self->team != other->team)) {
	    return 0;
    }

    if (strlcpy(tank_name, self->nick_name, MAX_NAME_LEN) < MAX_NAME_LEN)
	strlcat(tank_name, "'s tank", MAX_NAME_LEN);

    if (strcmp(tank_name, other->nick_name))
	return 0;

    return 1;
}

static int Gui_calculate_ship_color(int id, other_t *other)
{
    Uint32 ship_color = whiteRGBA;

#ifndef NO_BLUE_TEAM
    if (BIT(Setup->mode, TEAM_PLAY)
	&& eyesId != id
	&& other != NULL
	&& eyeTeam == other->team) {
	/* Paint teammates and allies ships with last life in teamLWColorRGBA */
	if (BIT(Setup->mode, LIMITED_LIVES)
	    && (other->life == 0))
	    ship_color = teamLWColorRGBA;
	else
	    ship_color = teamShipColorRGBA;
    }

    if (eyes != NULL
	&& eyesId != id
	&& other != NULL
	&& eyes->alliance != ' '
	&& eyes->alliance == other->alliance) {
	/* Paint teammates and allies ships with last life in teamLWColorRGBA */
	if (BIT(Setup->mode, LIMITED_LIVES)
	    && (other->life == 0))
	    ship_color = teamLWColorRGBA;
	else
	    ship_color = teamShipColorRGBA;
    }

    if (Gui_is_my_tank(other))
	ship_color = blueRGBA;
#endif
    if (roundDelay > 0 && ship_color == whiteRGBA)
	ship_color = redRGBA;

    /* Check for team color */
    if (other && BIT(Setup->mode, TEAM_PLAY)) {
	int team_color = Team_color(other->team);
	if (team_color)
	    return team_color;
    }

    /* Vato color hack start, edited by mara & kps */
    if (BIT(Setup->mode, LIMITED_LIVES)) {
	/* Paint your ship in selfLWColorRGBA when on last life */
	if (eyes != NULL
	    && eyes->id == id
	    && eyes->life == 0) {
	    ship_color = selfLWColorRGBA;
	}

	/* Paint enemy ships with last life in enemyLWColorRGBA */
	if (eyes != NULL
	    && eyes->id != id
	    && other != NULL
	    && eyeTeam != other->team
	    && other->life == 0) {
	    ship_color = enemyLWColorRGBA;
	}
    }
    /* Vato color hack end */

    return ship_color;
}

static void Gui_paint_ship_name(int x, int y, other_t *other)
{
    int color = Life_color(other);

    /* TODO : do all name painting together, so we don't need
     * all theese setupPaint<foo> calls
     */
    if (shipNameColorRGBA) {
	if (!color)
	    color = shipNameColorRGBA;

	mapnprint(&mapfont, color, CENTER, DOWN,x,y - SHIP_SZ,maxCharsInNames,"%s",other->id_string);
    } else
	color = blueRGBA;

    if (instruments.showLivesByShip
	&& BIT(Setup->mode, LIMITED_LIVES)) {
	if (other->life < 1)
	    color = whiteRGBA;

	mapprint(&mapfont, color, LEFT, CENTER,x + SHIP_SZ,y,"%d", other->life);
    }
}

void Gui_paint_ship(int x, int y, int dir, int id, int cloak, int phased,
		    int shield, int deflector, int eshield)
{
    int i, color, img;
    shipshape_t *ship;
    position_t point;
    other_t *other;

    if (!(other = Other_by_id(id))) return;

    if (!(color = Gui_calculate_ship_color(id,other))) return;
    
    if ((!instruments.showShipShapes) && (self != NULL) && (self->id != id))
			ship = Default_ship();
    else if ((!instruments.showMyShipShape) && (self != NULL) && (self->id == id))
			ship = Default_ship();
		else
			ship = Ship_by_id(id);

    if (shield) {
    	Image_paint(IMG_SHIELD, x - 27, y - 27, 0, (color & 0xffffff00) + ((color & 0x000000ff)/2));
    }
	if (texturedShips) {
    	    if (BIT(Setup->mode, TEAM_PLAY)
			&& other != NULL
			&& self != NULL
			&& self->team == other->team) {
			img = IMG_SHIP_FRIEND;
    	    } else if (self != NULL && self->id != id) {
			img = IMG_SHIP_ENEMY;
    	    } else {
			img = IMG_SHIP_SELF;
    	    }
    	    if (cloak || phased) Image_paint_rotated(img, x, y, dir, (color & 0xffffff00) + ((color & 0x000000ff)/2));
	    else Image_paint_rotated(img, x, y, dir, color);
	} else {
    	    glEnable(GL_BLEND);
    	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	    glEnable(GL_LINE_SMOOTH);
    	    glLineWidth(shipLineWidth);
    	    set_alphacolor(color);
		
    	    if (cloak || phased ) {
    	    	glEnable(GL_LINE_STIPPLE);
    	    	glLineStipple( 3, 0xAAAA );
	    }
	    
    	    glBegin(GL_LINE_LOOP);
    	    	for (i = 0; i < ship->num_points; i++) {
    	    	    point = Ship_get_point_position(ship, i, dir);
    	    	    glVertex2d(x + point.x, y + point.y);
    	    	}
    	    glEnd();
	    
    	    if (cloak || phased ) glDisable(GL_LINE_STIPPLE);
	
    	    glLineWidth(1);
    	    glDisable(GL_LINE_SMOOTH);
    	    glDisable(GL_BLEND);
	}
    if (self != NULL
    	&& self->id != id
    	&& other != NULL)
		Gui_paint_ship_name(x,y,other);
}

void Paint_score_objects(void)
{
    int		i, x, y;

    if (!get_alpha(scoreObjectColorRGBA))
	return;

    for (i = 0; i < MAX_SCORE_OBJECTS; i++) {
	score_object_t*	sobj = &score_objects[i];
	if (sobj->life_time > 0) {
	    if (loopsSlow % 3) {
	    	/* approximate font width to h =( */
		x = sobj->x * BLOCK_SZ + BLOCK_SZ/2;
		y = sobj->y * BLOCK_SZ + BLOCK_SZ/2;
  		if (wrap(&x, &y)) {
		    /*mapprint(&mapfont,scoreObjectColorRGBA,CENTER,CENTER,x,y,"%s",sobj->msg);*/
		    if (Ensure_cached_text(&mapfont, sobj->msg,
					   &score_object_texs[i])) {
			disp_text(&score_object_texs[i], scoreObjectColorRGBA,
				  CENTER, CENTER, x, y, false);
		    }
		}
	    }
	    sobj->life_time -= timePerFrame;
	    if (sobj->life_time <= 0.0) {
		sobj->life_time = 0.0;
		sobj->hud_msg_len = 0;
	    }
	} else {
	    if (score_object_texs[i].cache) free_string_texture(&score_object_texs[i]);
	}
    }
}

static int Semantic_select_float(int64_t value, float *result)
{
    if (result == NULL || (double)value < -(double)FLT_MAX
	|| (double)value > (double)FLT_MAX) {
	return 0;
    }
    *result = (float)value;
    return isfinite(*result);
}

static RendererStatus Build_semantic_select_geometry(
    const irec_t *bounds, RendererPoint2D points[4])
{
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;

    if (bounds == NULL || points == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    left = (int64_t)bounds->x;
    top = (int64_t)bounds->y;
    right = left + (int64_t)bounds->w;
    bottom = top + (int64_t)bounds->h;
    if (!Semantic_select_float(left, &points[0].x)
	|| !Semantic_select_float(top, &points[0].y)
	|| !Semantic_select_float(right, &points[1].x)
	|| !Semantic_select_float(top, &points[1].y)
	|| !Semantic_select_float(right, &points[2].x)
	|| !Semantic_select_float(bottom, &points[2].y)
	|| !Semantic_select_float(left, &points[3].x)
	|| !Semantic_select_float(bottom, &points[3].y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return RENDERER_STATUS_OK;
}

RendererStatus Paint_select(void)
{
    SdlRenderer *sdl_renderer;
    Renderer *renderer;
    RendererPoint2D points[4];
    RendererStatus status;

    if (select_bounds == NULL)
	return RENDERER_STATUS_OK;

    sdl_renderer = Get_sdl_renderer();
    if (sdl_renderer == NULL)
	return RENDERER_STATUS_INVALID_STATE;
    status = Sdl_renderer_track_frame_result(
	sdl_renderer, RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
	return status;
    renderer = Sdl_renderer_frontend(sdl_renderer);
    if (renderer == NULL) {
	return Sdl_renderer_track_frame_result(
	    sdl_renderer, RENDERER_STATUS_INVALID_STATE);
    }

    status = Build_semantic_select_geometry(select_bounds, points);
    if (status != RENDERER_STATUS_OK)
	return Sdl_renderer_track_frame_result(sdl_renderer, status);
    status = Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA);
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status != RENDERER_STATUS_OK)
	return status;
    status = Renderer_stroke_path(
	renderer, points, 4, 1.0f,
	Renderer_color_from_rgba32(selectionColorRGBA), 1);
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status != RENDERER_STATUS_OK)
	return status;
    status = Sdl_renderer_flush_preserving_legacy(sdl_renderer);
    return Sdl_renderer_track_frame_result(sdl_renderer, status);
}

void Paint_HUD_values(void)
{
    int x, y;

    if (!hudColorRGBA)
	return;

    x = draw_width - 20;
    /* Better make sure it's below the meters */
    y = draw_height
	- 9 * (MAX((GLuint)meterHeight, gamefont.requested_height) + 6);

    HUDprint(&gamefont,hudColorRGBA,RIGHT,DOWN,x,y,"FPS: %.3f",clientFPS);
    HUDprint(&gamefont,hudColorRGBA,RIGHT,DOWN,x,y-20,"CL.LAG : %.1f ms", clData.clientLag);
}

typedef struct SemanticHudBatch {
    SdlRenderer *sdl_renderer;
    Renderer *renderer;
    RendererStatus status;
    int has_accepted_commands;
} SemanticHudBatch;

typedef struct SemanticHudPointerGeometry {
    RendererPoint2D speed[2];
    RendererPoint2D direction[2];
    int draw_speed;
    int draw_direction;
} SemanticHudPointerGeometry;

typedef struct SemanticHudFrameGeometry {
    RendererPoint2D horizontal[2][2];
    RendererPoint2D vertical[2][2];
} SemanticHudFrameGeometry;

#define SEMANTIC_HUD_RADAR_RING_POINTS 16
#define SEMANTIC_HUD_RADAR_FILL_VERTICES \
    ((SEMANTIC_HUD_RADAR_RING_POINTS - 2) * 3)

typedef struct SemanticHudRadarDotGeometry {
    RendererVertex2D fill[SEMANTIC_HUD_RADAR_FILL_VERTICES];
    RendererPoint2D outline[SEMANTIC_HUD_RADAR_RING_POINTS];
    size_t fill_count;
    size_t outline_count;
    RendererColor color;
} SemanticHudRadarDotGeometry;

typedef struct SemanticHudRadarPass {
    double scale;
    double xlimit;
    double ylimit;
    double xfactor;
    double yfactor;
    int width;
    int height;
    int size;
} SemanticHudRadarPass;

typedef struct SemanticHudRadarScene {
    SemanticHudRadarPass passes[2];
    RendererPoint2D ball_scan[SEMANTIC_HUD_RADAR_RING_POINTS];
    RendererPoint2D cover_scan[SEMANTIC_HUD_RADAR_RING_POINTS];
    int radar_enabled;
    int draw_ball_scan;
    int draw_cover_scan;
} SemanticHudRadarScene;

typedef struct SemanticHudFuelGaugeGeometry {
    RendererPoint2D outline[4];
    float fill_x;
    float fill_y;
    float fill_width;
    float fill_height;
    int draw_fill;
} SemanticHudFuelGaugeGeometry;

typedef struct SemanticMeterGeometry {
    float fill_x;
    float fill_y;
    float fill_width;
    float fill_height;
    RendererPoint2D border[4];
    RendererPoint2D ticks[5][2];
    int draw_fill;
    int text_x;
    int text_y;
} SemanticMeterGeometry;

static RendererStatus Begin_semantic_hud_batch(SemanticHudBatch *batch)
{
    if (batch == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    batch->sdl_renderer = Get_sdl_renderer();
    batch->renderer = NULL;
    batch->status = RENDERER_STATUS_INVALID_STATE;
    batch->has_accepted_commands = 0;
    if (batch->sdl_renderer == NULL)
	return batch->status;
    batch->status = Sdl_renderer_track_frame_result(
	batch->sdl_renderer, RENDERER_STATUS_OK);
    if (batch->status != RENDERER_STATUS_OK)
	return batch->status;
    batch->renderer = Sdl_renderer_frontend(batch->sdl_renderer);
    if (batch->renderer == NULL) {
	batch->status = Sdl_renderer_track_frame_result(
	    batch->sdl_renderer, RENDERER_STATUS_INVALID_STATE);
    }
    return batch->status;
}

static RendererStatus Track_semantic_hud_batch(
    SemanticHudBatch *batch, RendererStatus status)
{
    if (batch == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (batch->sdl_renderer == NULL) {
	batch->status = status;
	return batch->status;
    }
    batch->status = Sdl_renderer_track_frame_result(
	batch->sdl_renderer, status);
    return batch->status;
}

static RendererStatus Accept_semantic_hud_command(
    SemanticHudBatch *batch, RendererStatus status)
{
    if (batch == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (status == RENDERER_STATUS_OK)
	batch->has_accepted_commands = 1;
    return Track_semantic_hud_batch(batch, status);
}

static RendererStatus Finish_semantic_hud_batch(
    SemanticHudBatch *batch)
{
    RendererStatus flush_status;

    if (batch == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!batch->has_accepted_commands || batch->sdl_renderer == NULL)
	return batch->status;
    flush_status = Sdl_renderer_flush_preserving_legacy(
	batch->sdl_renderer);
    batch->has_accepted_commands = 0;
    return Track_semantic_hud_batch(batch, flush_status);
}

static int Semantic_hud_pointer_point(
    double x, double y, RendererPoint2D *point)
{
    int integer_x;
    int integer_y;

    if (point == NULL || !isfinite(x) || !isfinite(y)
	|| x < (double)INT_MIN || x > (double)INT_MAX
	|| y < (double)INT_MIN || y > (double)INT_MAX) {
	return 0;
    }
    integer_x = (int)x;
    integer_y = (int)y;
    point->x = (float)integer_x;
    point->y = (float)integer_y;
    return isfinite(point->x) && isfinite(point->y);
}

static RendererStatus Build_semantic_hud_pointer_geometry(
    int draw_speed, int draw_direction,
    SemanticHudPointerGeometry *geometry)
{
    double center_x;
    double center_y;

    if (geometry == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    geometry->draw_speed = draw_speed;
    geometry->draw_direction = draw_direction;
    center_x = (double)(draw_width / 2);
    center_y = (double)(draw_height / 2);

    if (draw_speed
	&& (!Semantic_hud_pointer_point(
		center_x, center_y, &geometry->speed[0])
	    || !Semantic_hud_pointer_point(
		center_x - ptr_move_fact * (double)selfVel.x,
		center_y + ptr_move_fact * (double)selfVel.y,
		&geometry->speed[1]))) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    if (draw_direction) {
	double cosine;
	double sine;

	if (heading < 0 || heading >= TABLE_SIZE)
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	cosine = tcos(heading);
	sine = tsin(heading);
	if (!isfinite(cosine) || !isfinite(sine)
	    || !Semantic_hud_pointer_point(
		center_x + 85.0 * cosine,
		center_y - 85.0 * sine,
		&geometry->direction[0])
	    || !Semantic_hud_pointer_point(
		center_x + 100.0 * cosine,
		center_y - 100.0 * sine,
		&geometry->direction[1])) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Paint_semantic_hud_pointers(void)
{
    SemanticHudBatch batch;
    SemanticHudPointerGeometry geometry;
    RendererStatus operation_status;
    int draw_speed;
    int draw_direction;

    draw_speed = ptr_move_fact != 0.0
	&& selfVisible
	&& (selfVel.x != 0 || selfVel.y != 0);
    draw_direction = selfVisible && dirPtrColorRGBA;
    if (!draw_speed && !draw_direction)
	return RENDERER_STATUS_OK;

    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_pointer_geometry(
	draw_speed, draw_direction, &geometry);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);
    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ADDITIVE);
    if (Track_semantic_hud_batch(&batch, operation_status)
	!= RENDERER_STATUS_OK) {
	return batch.status;
    }

    if (geometry.draw_speed) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, geometry.speed, 2, 1.0f,
	    Renderer_color_from_rgba32(hudColorRGBA), 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    if (batch.status == RENDERER_STATUS_OK && geometry.draw_direction) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, geometry.direction, 2, 1.0f,
	    Renderer_color_from_rgba32(dirPtrColorRGBA), 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    return Finish_semantic_hud_batch(&batch);
}

#ifdef XPILOT_SDLGUI_TEST_HOOKS
RendererStatus Sdlgui_test_paint_hud_pointers(void)
{
    return Paint_semantic_hud_pointers();
}
#endif

static int Semantic_hud_integer_float(int64_t value, float *result)
{
    if (result == NULL || (double)value < -(double)FLT_MAX
	|| (double)value > (double)FLT_MAX) {
	return 0;
    }
    *result = (float)value;
    return isfinite(*result);
}

static RendererStatus Build_semantic_hud_frame_geometry(
    int hud_pos_x, int hud_pos_y, SemanticHudFrameGeometry *geometry)
{
    int64_t left;
    int64_t right;
    int64_t top;
    int64_t bottom;
    int64_t horizontal_top;
    int64_t horizontal_bottom;
    int64_t vertical_left;
    int64_t vertical_right;

    if (geometry == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    left = (int64_t)hud_pos_x - (int64_t)hudSize;
    right = (int64_t)hud_pos_x + (int64_t)hudSize;
    top = (int64_t)hud_pos_y - (int64_t)hudSize;
    bottom = (int64_t)hud_pos_y + (int64_t)hudSize;
    horizontal_top = top + (int64_t)HUD_OFFSET;
    horizontal_bottom = bottom - (int64_t)HUD_OFFSET;
    vertical_left = left + (int64_t)HUD_OFFSET;
    vertical_right = right - (int64_t)HUD_OFFSET;

    if (!Semantic_hud_integer_float(
	    left, &geometry->horizontal[0][0].x)
	|| !Semantic_hud_integer_float(
	    horizontal_top, &geometry->horizontal[0][0].y)
	|| !Semantic_hud_integer_float(
	    right, &geometry->horizontal[0][1].x)
	|| !Semantic_hud_integer_float(
	    horizontal_top, &geometry->horizontal[0][1].y)
	|| !Semantic_hud_integer_float(
	    left, &geometry->horizontal[1][0].x)
	|| !Semantic_hud_integer_float(
	    horizontal_bottom, &geometry->horizontal[1][0].y)
	|| !Semantic_hud_integer_float(
	    right, &geometry->horizontal[1][1].x)
	|| !Semantic_hud_integer_float(
	    horizontal_bottom, &geometry->horizontal[1][1].y)
	|| !Semantic_hud_integer_float(
	    vertical_left, &geometry->vertical[0][0].x)
	|| !Semantic_hud_integer_float(
	    top, &geometry->vertical[0][0].y)
	|| !Semantic_hud_integer_float(
	    vertical_left, &geometry->vertical[0][1].x)
	|| !Semantic_hud_integer_float(
	    bottom, &geometry->vertical[0][1].y)
	|| !Semantic_hud_integer_float(
	    vertical_right, &geometry->vertical[1][0].x)
	|| !Semantic_hud_integer_float(
	    top, &geometry->vertical[1][0].y)
	|| !Semantic_hud_integer_float(
	    vertical_right, &geometry->vertical[1][1].x)
	|| !Semantic_hud_integer_float(
	    bottom, &geometry->vertical[1][1].y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Paint_semantic_hud_frame(
    int hud_pos_x, int hud_pos_y)
{
    SemanticHudBatch batch;
    SemanticHudFrameGeometry geometry;
    RendererColor color;
    RendererStatus operation_status;
    int path_index;

    if (!hudHLineColorRGBA && !hudVLineColorRGBA)
	return RENDERER_STATUS_OK;
    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_frame_geometry(
	hud_pos_x, hud_pos_y, &geometry);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);

    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ADDITIVE);
    Track_semantic_hud_batch(&batch, operation_status);

    color = Renderer_color_from_rgba32(hudHLineColorRGBA);
    for (path_index = 0;
	 batch.status == RENDERER_STATUS_OK
	     && hudHLineColorRGBA && path_index < 2;
	 path_index++) {
	operation_status = Renderer_stroke_stippled_path(
	    batch.renderer, geometry.horizontal[path_index], 2, 1.0f,
	    color, 0, 4, UINT16_C(0xAAAA));
	Accept_semantic_hud_command(&batch, operation_status);
    }

    color = Renderer_color_from_rgba32(hudVLineColorRGBA);
    for (path_index = 0;
	 batch.status == RENDERER_STATUS_OK
	     && hudVLineColorRGBA && path_index < 2;
	 path_index++) {
	operation_status = Renderer_stroke_stippled_path(
	    batch.renderer, geometry.vertical[path_index], 2, 1.0f,
	    color, 0, 4, UINT16_C(0xAAAA));
	Accept_semantic_hud_command(&batch, operation_status);
    }
    return Finish_semantic_hud_batch(&batch);
}

static RendererStatus Build_semantic_meter_geometry(
    int xoff, int y, int value, int maximum,
    SemanticMeterGeometry *geometry, int *x_alignment)
{
    static const int tick_top_offsets[5] = {-4, -4, -3, -1, -1};
    static const int tick_bottom_offsets[5] = {4, 4, 3, 1, 1};
    int64_t tick_x_offsets[5];
    int64_t divisor;
    int64_t product;
    int64_t width;
    int64_t fill_left;
    int64_t fill_height;
    int64_t x;
    int64_t xstr;
    int64_t text_y;
    int64_t border_right;
    int64_t border_bottom;
    int tick_index;

    if (geometry == NULL || x_alignment == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    if (xoff >= 0) {
	x = (int64_t)xoff;
	xstr = x + (int64_t)meterWidth + INT64_C(5);
	*x_alignment = LEFT;
    } else {
	x = (int64_t)draw_width
	    - ((int64_t)meterWidth - (int64_t)xoff);
	xstr = x - INT64_C(5);
	*x_alignment = RIGHT;
    }
    text_y = (int64_t)draw_height - (int64_t)y
	- (int64_t)(meterHeight / 2);
    if (xstr < INT_MIN || xstr > INT_MAX
	|| text_y < INT_MIN || text_y > INT_MAX) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    geometry->text_x = (int)xstr;
    geometry->text_y = (int)text_y;

    divisor = maximum != 0 ? (int64_t)maximum : INT64_C(1);
    product = (int64_t)meterWidth * (int64_t)value;
    if (product == INT64_MIN && divisor == INT64_C(-1))
	return RENDERER_STATUS_INVALID_ARGUMENT;
    width = product / divisor;
    fill_left = x;
    if (width < 0) {
	if (width == INT64_MIN || fill_left < INT64_MIN - width)
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	fill_left += width;
	width = -width;
    }
    fill_height = (int64_t)meterHeight - INT64_C(1);
    geometry->draw_fill = width != 0 && fill_height > 0;
    if (geometry->draw_fill
	&& (!Semantic_hud_integer_float(fill_left, &geometry->fill_x)
	    || !Semantic_hud_integer_float((int64_t)y, &geometry->fill_y)
	    || !Semantic_hud_integer_float(width, &geometry->fill_width)
	    || !Semantic_hud_integer_float(fill_height,
				     &geometry->fill_height))) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    border_right = x + (int64_t)meterWidth;
    border_bottom = (int64_t)y + (int64_t)meterHeight;
    if (!Semantic_hud_integer_float(x, &geometry->border[0].x)
	|| !Semantic_hud_integer_float((int64_t)y, &geometry->border[0].y)
	|| !Semantic_hud_integer_float(x, &geometry->border[1].x)
	|| !Semantic_hud_integer_float(border_bottom, &geometry->border[1].y)
	|| !Semantic_hud_integer_float(border_right, &geometry->border[2].x)
	|| !Semantic_hud_integer_float(border_bottom, &geometry->border[2].y)
	|| !Semantic_hud_integer_float(border_right, &geometry->border[3].x)
	|| !Semantic_hud_integer_float((int64_t)y,
					 &geometry->border[3].y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    tick_x_offsets[0] = 0;
    tick_x_offsets[1] = (int64_t)meterWidth;
    tick_x_offsets[2] = (int64_t)meterWidth / INT64_C(2);
    tick_x_offsets[3] = (int64_t)meterWidth / INT64_C(4);
    tick_x_offsets[4] = (INT64_C(3) * (int64_t)meterWidth)
	/ INT64_C(4);
    for (tick_index = 0; tick_index < 5; tick_index++) {
	int64_t tick_x = x + tick_x_offsets[tick_index];
	int64_t tick_top = (int64_t)y + tick_top_offsets[tick_index];
	int64_t tick_bottom = border_bottom
	    + tick_bottom_offsets[tick_index];

	if (!Semantic_hud_integer_float(
		tick_x, &geometry->ticks[tick_index][0].x)
	    || !Semantic_hud_integer_float(
		tick_top, &geometry->ticks[tick_index][0].y)
	    || !Semantic_hud_integer_float(
		tick_x, &geometry->ticks[tick_index][1].x)
	    || !Semantic_hud_integer_float(
		tick_bottom, &geometry->ticks[tick_index][1].y)) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
    }
    return RENDERER_STATUS_OK;
}

static void Paint_meter(int xoff, int y, string_tex_t *tex, int val, int max,
			int meter_color)
{
    SemanticHudBatch batch;
    SemanticMeterGeometry geometry;
    RendererColor border_color;
    RendererStatus operation_status;
    int x_alignment;
    int color;
    int tick_index;

    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return;
    operation_status = Build_semantic_meter_geometry(
	xoff, y, val, max, &geometry, &x_alignment);
    if (operation_status != RENDERER_STATUS_OK) {
	Track_semantic_hud_batch(&batch, operation_status);
	return;
    }
    if (Track_semantic_hud_batch(
	    &batch,
	    Renderer_set_blend(
		batch.renderer, RENDERER_BLEND_ALPHA))
	!= RENDERER_STATUS_OK) {
	return;
    }

    if (geometry.draw_fill) {
	operation_status = Renderer_fill_rect(
	    batch.renderer,
	    geometry.fill_x, geometry.fill_y,
	    geometry.fill_width, geometry.fill_height,
	    Renderer_color_from_rgba32((Uint32)meter_color));
	Accept_semantic_hud_command(&batch, operation_status);
    }

    /* meterBorderColorRGBA = 0 obviously means no meter borders are drawn */
    if (batch.status == RENDERER_STATUS_OK && meterBorderColorRGBA) {
	border_color = Renderer_color_from_rgba32(meterBorderColorRGBA);
	operation_status = Renderer_stroke_path(
	    batch.renderer, geometry.border, 4, 1.0f,
	    border_color, 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    for (tick_index = 0;
	 batch.status == RENDERER_STATUS_OK
	 && meterBorderColorRGBA && tick_index < 5;
	 tick_index++) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, geometry.ticks[tick_index], 2, 1.0f,
	    border_color, 0);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    if (Finish_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return;

    color = meterBorderColorRGBA ? meterBorderColorRGBA : meter_color;

    if (tex->cache != NULL) {
	operation_status = disp_text(
	    tex, color, x_alignment, CENTER,
	    geometry.text_x, geometry.text_y, true);
	if (operation_status != RENDERER_STATUS_OK)
	    Track_semantic_hud_batch(&batch, operation_status);
    }
}

void Paint_meters(void)
{
    int spacing = MAX((GLuint)meterHeight, gamefont.requested_height) + 6;
    int y = spacing, color;

    (void)Ensure_cached_text(&gamefont, "Fuel", &meter_texs[0]);
    (void)Ensure_cached_text(&gamefont, "Power", &meter_texs[1]);
    (void)Ensure_cached_text(&gamefont, "Turnspeed", &meter_texs[2]);
    (void)Ensure_cached_text(&gamefont, "Packet", &meter_texs[3]);
    (void)Ensure_cached_text(&gamefont, "Loss", &meter_texs[4]);
    (void)Ensure_cached_text(&gamefont, "Drop", &meter_texs[5]);
    (void)Ensure_cached_text(&gamefont, "Lag", &meter_texs[6]);
    (void)Ensure_cached_text(&gamefont, "Thrust Left", &meter_texs[7]);
    (void)Ensure_cached_text(&gamefont, "Shields Left", &meter_texs[8]);
    (void)Ensure_cached_text(&gamefont, "Phasing left", &meter_texs[9]);
    (void)Ensure_cached_text(&gamefont, "Self destructing",
			     &meter_texs[10]);
    (void)Ensure_cached_text(&gamefont, "SHUTDOWN", &meter_texs[11]);

    if (fuelMeterColorRGBA)
	Paint_meter(-10, y += spacing, &meter_texs[0],
		    (int)fuelSum, (int)fuelMax, fuelMeterColorRGBA);

    if (powerMeterColorRGBA)
	color = powerMeterColorRGBA;
    else if (controlTime > 0.0)
	color = temporaryMeterColorRGBA;
    else
	color = 0;

    if (color)
	Paint_meter(-10, y += spacing, &meter_texs[1],
		    (int)displayedPower, (int)MAX_PLAYER_POWER, color);

    if (turnSpeedMeterColorRGBA)
	color = turnSpeedMeterColorRGBA;
    else if (controlTime > 0.0)
	color = temporaryMeterColorRGBA;
    else
	color = 0;

    if (color)
	Paint_meter(-10, y += spacing, &meter_texs[2],
		    (int)displayedTurnspeed, (int)MAX_PLAYER_TURNSPEED, color);

    if (controlTime > 0.0) {
	controlTime -= timePerFrame;
	if (controlTime <= 0.0)
	    controlTime = 0.0;
    }

    if (packetSizeMeterColorRGBA)
	Paint_meter(-10, y += spacing, &meter_texs[3],
		   (packet_size >= 4096) ? 4096 : packet_size, 4096,
		    packetSizeMeterColorRGBA);
    if (packetLossMeterColorRGBA)
	Paint_meter(-10, y += spacing, &meter_texs[4], packet_loss, FPS,
		    packetLossMeterColorRGBA);
    if (packetDropMeterColorRGBA)
	Paint_meter(-10, y += spacing, &meter_texs[5], packet_drop, FPS,
		    packetDropMeterColorRGBA);
    if (packetLagMeterColorRGBA)
	Paint_meter(-10, y += spacing, &meter_texs[6], MIN(packet_lag, 1 * FPS), 1 * FPS,
		    packetLagMeterColorRGBA);

    if (temporaryMeterColorRGBA) {
	if (thrusttime >= 0 && thrusttimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3,
			&meter_texs[7],
			(thrusttime >= thrusttimemax
			 ? thrusttimemax : thrusttime),
			thrusttimemax, temporaryMeterColorRGBA);

	if (shieldtime >= 0 && shieldtimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + spacing,
			&meter_texs[8],
			(shieldtime >= shieldtimemax
			 ? shieldtimemax : shieldtime),
			shieldtimemax, temporaryMeterColorRGBA);

	if (phasingtime >= 0 && phasingtimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 2*spacing,
			&meter_texs[9],
			(phasingtime >= phasingtimemax
			 ? phasingtimemax : phasingtime),
			phasingtimemax, temporaryMeterColorRGBA);

	if (destruct > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 3*spacing,
			&meter_texs[10], destruct, (int)SELF_DESTRUCT_DELAY,
			temporaryMeterColorRGBA);

	if (shutdown_count >= 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 4*spacing,
			&meter_texs[11], shutdown_count, shutdown_delay,
			temporaryMeterColorRGBA);
    }
}

static RendererStatus Paint_lock(int hud_pos_x, int hud_pos_y)
{
    other_t *target;
    const int BORDER = 2;

    if ((target = Other_by_id(lock_id)) == NULL)
	return RENDERER_STATUS_OK;

    if (hudColorRGBA) {
	int color = Life_color(target);

	if (!color)
	    color = hudColorRGBA;

	return HUDnprint(
	    &gamefont, color, CENTER, CENTER, hud_pos_x,
	    hud_pos_y - (-hudSize + HUD_OFFSET - BORDER),
	    strlen(target->id_string), "%s", target->id_string);
    }
    return RENDERER_STATUS_OK;
}

static int Semantic_hud_radar_dot_visible(
    Uint32 color, int shape, int size)
{
    return color != 0 && shape >= 2 && shape <= 7 && size != 0;
}

static int Semantic_hud_radar_float(double value, float *result)
{
    if (result == NULL || !isfinite(value)
	|| value < -(double)FLT_MAX || value > (double)FLT_MAX) {
	return 0;
    }
    *result = (float)value;
    return isfinite(*result);
}

static int Semantic_hud_radar_point(
    double x, double y, RendererPoint2D *point)
{
    return point != NULL
	&& Semantic_hud_radar_float(x, &point->x)
	&& Semantic_hud_radar_float(y, &point->y);
}

static void Set_semantic_hud_radar_vertex(
    RendererVertex2D *vertex, RendererPoint2D point, RendererColor color)
{
    vertex->x = point.x;
    vertex->y = point.y;
    vertex->u = 0.0f;
    vertex->v = 0.0f;
    vertex->color = color;
}

static RendererStatus Build_semantic_hud_radar_ring(
    int64_t x, int64_t y, int radius,
    RendererPoint2D points[SEMANTIC_HUD_RADAR_RING_POINTS])
{
    int point_index;

    if (points == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    for (point_index = 0;
	 point_index < SEMANTIC_HUD_RADAR_RING_POINTS;
	 point_index++) {
	int table_index = point_index * TABLE_SIZE
	    / SEMANTIC_HUD_RADAR_RING_POINTS;
	double cosine = tcos(table_index);
	double sine = tsin(table_index);

	if (!isfinite(cosine) || !isfinite(sine)
	    || !Semantic_hud_radar_point(
		(double)x + cosine * (double)radius,
		(double)y + sine * (double)radius,
		&points[point_index])) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Build_semantic_hud_radar_dot_geometry(
    int64_t x, int64_t y, Uint32 packed_color, int shape, int size,
    SemanticHudRadarDotGeometry *geometry)
{
    RendererPoint2D points[4];
    RendererStatus status;
    size_t triangle_index;

    if (geometry == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    memset(geometry, 0, sizeof(*geometry));
    if (!Semantic_hud_radar_dot_visible(packed_color, shape, size))
	return RENDERER_STATUS_OK;
    geometry->color = Renderer_color_from_rgba32(packed_color);

    if (shape == 2 || shape == 3) {
	status = Build_semantic_hud_radar_ring(
	    x, y, size, geometry->outline);
	if (status != RENDERER_STATUS_OK)
	    return status;
	if (shape == 3) {
	    geometry->outline_count = SEMANTIC_HUD_RADAR_RING_POINTS;
	    return RENDERER_STATUS_OK;
	}
	/* GL_POLYGON's convex ring is reproduced by a fan rooted at point zero. */
	for (triangle_index = 1;
	     triangle_index + 1 < SEMANTIC_HUD_RADAR_RING_POINTS;
	     triangle_index++) {
	    size_t vertex_index = geometry->fill_count;

	    Set_semantic_hud_radar_vertex(
		&geometry->fill[vertex_index], geometry->outline[0],
		geometry->color);
	    Set_semantic_hud_radar_vertex(
		&geometry->fill[vertex_index + 1],
		geometry->outline[triangle_index], geometry->color);
	    Set_semantic_hud_radar_vertex(
		&geometry->fill[vertex_index + 2],
		geometry->outline[triangle_index + 1], geometry->color);
	    geometry->fill_count += 3;
	}
	return RENDERER_STATUS_OK;
    }

    if (shape == 4 || shape == 5) {
	if (!Semantic_hud_radar_point(
		(double)(x - (int64_t)size),
		(double)(y - (int64_t)size), &points[0])
	    || !Semantic_hud_radar_point(
		(double)(x - (int64_t)size),
		(double)(y + (int64_t)size), &points[1])
	    || !Semantic_hud_radar_point(
		(double)(x + (int64_t)size),
		(double)(y + (int64_t)size), &points[2])
	    || !Semantic_hud_radar_point(
		(double)(x + (int64_t)size),
		(double)(y - (int64_t)size), &points[3])) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
	if (shape == 5) {
	    memcpy(geometry->outline, points, sizeof(points));
	    geometry->outline_count = 4;
	    return RENDERER_STATUS_OK;
	}
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[0], points[0], geometry->color);
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[1], points[1], geometry->color);
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[2], points[2], geometry->color);
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[3], points[0], geometry->color);
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[4], points[2], geometry->color);
	Set_semantic_hud_radar_vertex(
	    &geometry->fill[5], points[3], geometry->color);
	geometry->fill_count = 6;
	return RENDERER_STATUS_OK;
    }

    if (!Semantic_hud_radar_point(
	    (double)(x - (int64_t)size),
	    (double)(y + (int64_t)size), &points[0])
	|| !Semantic_hud_radar_point(
	    (double)x, (double)(y - (int64_t)size), &points[1])
	|| !Semantic_hud_radar_point(
	    (double)(x + (int64_t)size),
	    (double)(y + (int64_t)size), &points[2])) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (shape == 7) {
	memcpy(geometry->outline, points, 3 * sizeof(points[0]));
	geometry->outline_count = 3;
	return RENDERER_STATUS_OK;
    }
    Set_semantic_hud_radar_vertex(
	&geometry->fill[0], points[0], geometry->color);
    Set_semantic_hud_radar_vertex(
	&geometry->fill[1], points[1], geometry->color);
    Set_semantic_hud_radar_vertex(
	&geometry->fill[2], points[2], geometry->color);
    geometry->fill_count = 3;
    return RENDERER_STATUS_OK;
}

static RendererStatus Submit_semantic_hud_radar_dot(
    SemanticHudBatch *batch, const SemanticHudRadarDotGeometry *geometry)
{
    RendererStatus status;

    if (batch == NULL || geometry == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    if (geometry->fill_count != 0) {
	status = Renderer_draw_triangles(
	    batch->renderer, NULL, geometry->fill, geometry->fill_count);
	return Accept_semantic_hud_command(batch, status);
    }
    if (geometry->outline_count != 0) {
	status = Renderer_stroke_path(
	    batch->renderer, geometry->outline, geometry->outline_count,
	    1.0f, geometry->color, 1);
	return Accept_semantic_hud_command(batch, status);
    }
    return batch->status;
}

static int Semantic_hud_radar_truncated_int(double value, int *result)
{
    if (result == NULL || !isfinite(value)
	|| value < (double)INT_MIN || value > (double)INT_MAX) {
	return 0;
    }
    *result = (int)value;
    return 1;
}

static RendererStatus Build_semantic_hud_radar_pass(
    double scale, double xlimit, double ylimit, int size,
    SemanticHudRadarPass *pass)
{
    double scaled_width;
    double scaled_height;

    if (pass == NULL || Setup == NULL
	|| Setup->width <= 0 || Setup->height <= 0 || RadarHeight == 0
	|| !isfinite(scale) || scale <= 0.0
	|| !isfinite(xlimit) || xlimit < 0.0
	|| !isfinite(ylimit) || ylimit < 0.0) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    scaled_width = scale * 256.0;
    scaled_height = scale * (double)RadarHeight;
    if (!Semantic_hud_radar_truncated_int(scaled_width, &pass->width)
	|| !Semantic_hud_radar_truncated_int(
	    scaled_height, &pass->height)
	|| pass->width <= 0 || pass->height <= 0) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    pass->scale = scale;
    pass->xlimit = xlimit;
    pass->ylimit = ylimit;
    pass->xfactor = (double)pass->width / (double)Setup->width;
    pass->yfactor = (double)pass->height / (double)Setup->height;
    pass->size = size;
    if (!isfinite(pass->xfactor) || !isfinite(pass->yfactor))
	return RENDERER_STATUS_INVALID_ARGUMENT;
    return RENDERER_STATUS_OK;
}

static int Semantic_hud_radar_half_size(int size)
{
    int half = size / 2;

    /* Match GCC's arithmetic right shift used by the legacy object marker. */
    if (size < 0 && size % 2 != 0)
	half--;
    return half;
}

static RendererStatus Build_semantic_hud_radar_entry(
    const SemanticHudRadarPass *pass, const radar_t *radar,
    SemanticHudRadarDotGeometry *geometry)
{
    double relative_x;
    double relative_y;
    int integer_x;
    int integer_y;
    int64_t x;
    int64_t y;
    int64_t center_x;
    int64_t center_y;
    Uint32 color;
    int shape;
    int size;

    if (pass == NULL || radar == NULL || geometry == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    memset(geometry, 0, sizeof(*geometry));
    relative_x = (double)radar->x * pass->scale
	- (double)((int64_t)world.x + ext_view_width / 2)
	    * pass->xfactor;
    relative_y = (double)radar->y * pass->scale
	- (double)((int64_t)world.y + ext_view_height / 2)
	    * pass->yfactor;
    if (!Semantic_hud_radar_truncated_int(relative_x, &integer_x)
	|| !Semantic_hud_radar_truncated_int(relative_y, &integer_y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    x = integer_x;
    y = integer_y;
    /* Preserve the legacy one-period wrap and its strict half-map bounds. */
    if (x < -(int64_t)pass->width / 2)
	x += pass->width;
    else if (x > (int64_t)pass->width / 2)
	x -= pass->width;
    if (y < -(int64_t)pass->height / 2)
	y += pass->height;
    else if (y > (int64_t)pass->height / 2)
	y -= pass->height;

    if ((double)x <= pass->xlimit && (double)x >= -pass->xlimit
	&& (double)y <= pass->ylimit && (double)y >= -pass->ylimit) {
	return RENDERER_STATUS_OK;
    }

    if (radar->type == RadarEnemy) {
	color = hudRadarEnemyColorRGBA;
	shape = hudRadarEnemyShape;
    } else {
	color = hudRadarOtherColorRGBA;
	shape = hudRadarOtherShape;
    }
    size = pass->size;
    if (radar->size == 0) {
	size = Semantic_hud_radar_half_size(size);
	if (hudRadarObjectColorRGBA)
	    color = hudRadarObjectColorRGBA;
	if (hudRadarObjectShape)
	    shape = hudRadarObjectShape;
    }

    center_x = (int64_t)(draw_width / 2);
    center_y = (int64_t)(draw_height / 2);
    return Build_semantic_hud_radar_dot_geometry(
	x + center_x, -y + center_y, color, shape, size, geometry);
}

static RendererStatus Validate_semantic_hud_radar_pass(
    const SemanticHudRadarPass *pass)
{
    SemanticHudRadarDotGeometry geometry;
    int radar_index;

    if (pass == NULL || num_radar < 0
	|| (num_radar > 0 && radar_ptr == NULL)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    /* Validate every derived dot before the batch accepts its first command. */
    for (radar_index = 0; radar_index < num_radar; radar_index++) {
	RendererStatus status = Build_semantic_hud_radar_entry(
	    pass, &radar_ptr[radar_index], &geometry);

	if (status != RENDERER_STATUS_OK)
	    return status;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Submit_semantic_hud_radar_pass(
    SemanticHudBatch *batch, const SemanticHudRadarPass *pass)
{
    SemanticHudRadarDotGeometry geometry;
    int radar_index;

    if (batch == NULL || pass == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    for (radar_index = 0;
	 radar_index < num_radar && batch->status == RENDERER_STATUS_OK;
	 radar_index++) {
	RendererStatus status = Build_semantic_hud_radar_entry(
	    pass, &radar_ptr[radar_index], &geometry);

	if (status != RENDERER_STATUS_OK) {
	    Track_semantic_hud_batch(batch, status);
	    break;
	}
	Submit_semantic_hud_radar_dot(batch, &geometry);
    }
    return batch->status;
}

static RendererStatus Build_semantic_hud_radar_scene(
    int radar_enabled, int draw_ball_scan, int draw_cover_scan,
    SemanticHudRadarScene *scene)
{
    double map_scale;
    double first_xlimit_value;
    double first_ylimit_value;
    double second_scale;
    double second_xlimit;
    double second_ylimit;
    int first_xlimit;
    int first_ylimit;
    int ball_radius = 0;
    int cover_radius = 0;
    RendererStatus status;

    if (scene == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;
    memset(scene, 0, sizeof(*scene));
    scene->radar_enabled = radar_enabled;
    scene->draw_ball_scan = draw_ball_scan;
    scene->draw_cover_scan = draw_cover_scan;

    if (radar_enabled) {
	if (Setup == NULL || Setup->width <= 0 || Setup->height <= 0
	    || ext_view_width < 0 || ext_view_height < 0
	    || active_view_width < 0 || active_view_height < 0
	    || !isfinite(hudRadarScale) || hudRadarScale <= 0.0
	    || !isfinite(hudRadarLimit) || hudRadarLimit < 0.0
	    || !isfinite(clData.scale) || clData.scale <= 0.0
	    || num_radar < 0 || (num_radar > 0 && radar_ptr == NULL)) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
	map_scale = (double)Setup->width / 256.0;
	if (!isfinite(map_scale) || map_scale <= 0.0
	    || map_scale > (double)FLT_MAX) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
	hudRadarMapScale = (float)map_scale;
	if (!isfinite(hudRadarMapScale) || hudRadarMapScale <= 0.0f)
	    return RENDERER_STATUS_INVALID_ARGUMENT;

	first_xlimit_value = hudRadarLimit * (ext_view_width / 2)
	    * hudRadarScale / (double)hudRadarMapScale;
	first_ylimit_value = hudRadarLimit * (ext_view_height / 2)
	    * hudRadarScale / (double)hudRadarMapScale;
	if (!Semantic_hud_radar_truncated_int(
		first_xlimit_value, &first_xlimit)
	    || !Semantic_hud_radar_truncated_int(
		first_ylimit_value, &first_ylimit)) {
	    return RENDERER_STATUS_INVALID_ARGUMENT;
	}
	second_scale = (double)hudRadarMapScale * clData.scale;
	second_xlimit = (active_view_width / 2) * clData.scale;
	second_ylimit = (active_view_height / 2) * clData.scale;
	status = Build_semantic_hud_radar_pass(
	    hudRadarScale, first_xlimit, first_ylimit,
	    hudRadarDotSize, &scene->passes[0]);
	if (status == RENDERER_STATUS_OK) {
	    status = Build_semantic_hud_radar_pass(
		second_scale, second_xlimit, second_ylimit,
		SHIP_SZ, &scene->passes[1]);
	}
	if (status != RENDERER_STATUS_OK)
	    return status;
	status = Validate_semantic_hud_radar_pass(&scene->passes[0]);
	if (status == RENDERER_STATUS_OK)
	    status = Validate_semantic_hud_radar_pass(&scene->passes[1]);
	if (status != RENDERER_STATUS_OK)
	    return status;
    }

    if (draw_ball_scan || draw_cover_scan) {
	if (!isfinite(clData.scale) || clData.scale <= 0.0)
	    return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (draw_ball_scan
	&& !Semantic_hud_radar_truncated_int(
	    8.0 * clData.scale, &ball_radius)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (draw_cover_scan
	&& !Semantic_hud_radar_truncated_int(
	    6.0 * clData.scale, &cover_radius)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (draw_ball_scan && ball_radius != 0) {
	status = Build_semantic_hud_radar_ring(
	    (int64_t)(draw_width / 2), (int64_t)(draw_height / 2),
	    ball_radius, scene->ball_scan);
	if (status != RENDERER_STATUS_OK)
	    return status;
    } else {
	scene->draw_ball_scan = 0;
    }
    if (draw_cover_scan && cover_radius != 0) {
	status = Build_semantic_hud_radar_ring(
	    (int64_t)(draw_width / 2), (int64_t)(draw_height / 2),
	    cover_radius, scene->cover_scan);
	if (status != RENDERER_STATUS_OK)
	    return status;
    } else {
	scene->draw_cover_scan = 0;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Paint_semantic_hud_radar_and_scans(void)
{
    SemanticHudBatch batch;
    SemanticHudRadarScene scene;
    RendererStatus operation_status;
    int radar_enabled;
    int draw_ball_scan;
    int draw_cover_scan;

    radar_enabled = hudRadarEnemyColorRGBA
	|| hudRadarOtherColorRGBA || hudRadarObjectColorRGBA;
    draw_ball_scan = Bms_test_state(BmsBall) && msgScanBallColorRGBA;
    draw_cover_scan = Bms_test_state(BmsCover) && msgScanCoverColorRGBA;
    if (!radar_enabled && !draw_ball_scan && !draw_cover_scan)
	return RENDERER_STATUS_OK;

    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_radar_scene(
	radar_enabled, draw_ball_scan, draw_cover_scan, &scene);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);

    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ALPHA);
    Track_semantic_hud_batch(&batch, operation_status);
    if (batch.status == RENDERER_STATUS_OK && scene.radar_enabled) {
	Submit_semantic_hud_radar_pass(&batch, &scene.passes[0]);
	if (batch.status == RENDERER_STATUS_OK)
	    Submit_semantic_hud_radar_pass(&batch, &scene.passes[1]);
    }
    if (batch.status == RENDERER_STATUS_OK) {
	operation_status = Renderer_set_blend(
	    batch.renderer, RENDERER_BLEND_OPAQUE);
	Track_semantic_hud_batch(&batch, operation_status);
    }
    if (batch.status == RENDERER_STATUS_OK && scene.draw_ball_scan) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, scene.ball_scan,
	    SEMANTIC_HUD_RADAR_RING_POINTS, 1.0f,
	    Renderer_color_from_rgba32(msgScanBallColorRGBA), 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    if (batch.status == RENDERER_STATUS_OK && scene.draw_cover_scan) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, scene.cover_scan,
	    SEMANTIC_HUD_RADAR_RING_POINTS, 1.0f,
	    Renderer_color_from_rgba32(msgScanCoverColorRGBA), 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    return Finish_semantic_hud_batch(&batch);
}

#ifdef XPILOT_SDLGUI_TEST_HOOKS
RendererStatus Sdlgui_test_paint_hud_radar_dot(
    int x, int y, Uint32 color, int shape, int size)
{
    SemanticHudBatch batch;
    SemanticHudRadarDotGeometry geometry;
    RendererStatus operation_status;

    if (!Semantic_hud_radar_dot_visible(color, shape, size))
	return RENDERER_STATUS_OK;
    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_radar_dot_geometry(
	x, y, color, shape, size, &geometry);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);
    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ALPHA);
    if (Track_semantic_hud_batch(&batch, operation_status)
	== RENDERER_STATUS_OK) {
	Submit_semantic_hud_radar_dot(&batch, &geometry);
    }
    return Finish_semantic_hud_batch(&batch);
}

RendererStatus Sdlgui_test_paint_hud_radar_and_scans(void)
{
    return Paint_semantic_hud_radar_and_scans();
}
#endif

static RendererStatus Build_semantic_hud_item_outline(
    int horiz_pos, int vert_pos, RendererPoint2D points[4])
{
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;

    if (points == NULL)
	return RENDERER_STATUS_INVALID_ARGUMENT;

    left = (int64_t)horiz_pos - (int64_t)ITEM_SIZE - INT64_C(2);
    top = (int64_t)vert_pos - INT64_C(2);
    right = (int64_t)horiz_pos;
    bottom = (int64_t)vert_pos + (int64_t)ITEM_SIZE;
    if (!Semantic_hud_integer_float(left, &points[0].x)
	|| !Semantic_hud_integer_float(top, &points[0].y)
	|| !Semantic_hud_integer_float(right, &points[1].x)
	|| !Semantic_hud_integer_float(top, &points[1].y)
	|| !Semantic_hud_integer_float(right, &points[2].x)
	|| !Semantic_hud_integer_float(bottom, &points[2].y)
	|| !Semantic_hud_integer_float(left, &points[3].x)
	|| !Semantic_hud_integer_float(bottom, &points[3].y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Paint_semantic_hud_item_outline(
    int horiz_pos, int vert_pos)
{
    SemanticHudBatch batch;
    RendererPoint2D points[4];
    RendererStatus operation_status;

    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_item_outline(
	horiz_pos, vert_pos, points);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);
    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ADDITIVE);
    if (Track_semantic_hud_batch(&batch, operation_status)
	== RENDERER_STATUS_OK) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, points, 4, 1.0f,
	    Renderer_color_from_rgba32(hudItemsColorRGBA), 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    return Finish_semantic_hud_batch(&batch);
}

static RendererStatus Paint_HUD_items(int hud_pos_x, int hud_pos_y)
{
    const int		BORDER = 3;
    char		str[50];
    int     	    	vert_pos, horiz_pos;
    int     	    	i, maxWidth = -1;
    RendererStatus	status;
    static int		vertSpacing = -1;
    static fontbounds	fb;


    /* Special itemtypes */
    if (vertSpacing < 0)
	vertSpacing = MAX(ITEM_SIZE, gamefont.requested_height) + 1;
    /* find the scaled location, then work in pixels */
    vert_pos = hud_pos_y - hudSize+HUD_OFFSET + BORDER;
    horiz_pos = hud_pos_x - hudSize+HUD_OFFSET - BORDER;

    for (i = 0; i < NUM_ITEMS; i++) {
	int num = numItems[i];

	if (i == ITEM_FUEL)
	    continue;

	if (instruments.showItems) {
	    lastNumItems[i] = num;
	    if (num <= 0)
		num = -1;
	} else {
	    if (num != lastNumItems[i]) {
		numItemsTime[i] = (int)(showItemsTime * (double)FPS);
		lastNumItems[i] = num;
	    }
	    if (numItemsTime[i]-- <= 0) {
		numItemsTime[i] = 0;
		num = -1;
	    }
	}

	if (num >= 0) {

	    Image_paint_hud(IMG_HUD_ITEMS,
			horiz_pos - ITEM_SIZE, 
			vert_pos, (u_byte)i, 
			hudItemsColorRGBA);

	    if (i == lose_item) {
		if (lose_item_active != 0) {
		    if (lose_item_active < 0)
			lose_item_active++;
		    status = Paint_semantic_hud_item_outline(
			horiz_pos, vert_pos);
		    if (status != RENDERER_STATUS_OK)
			return status;
		}
	    }

	    /* Paint item count */
	    sprintf(str, "%d", num);
	    status = printsize(&gamefont, &fb, "%s", str);
	    if (status != RENDERER_STATUS_OK)
		return status;

	    maxWidth = MAX(maxWidth, fb.width + BORDER + ITEM_SIZE);
	    
	    status = HUDprint(
		&gamefont, hudItemsColorRGBA, RIGHT, UP,
		horiz_pos - ITEM_SIZE - BORDER,
		draw_height - vert_pos - ITEM_SIZE, "%s", str);
	    if (status != RENDERER_STATUS_OK)
		return status;

	    vert_pos += vertSpacing;

	    if (vert_pos+vertSpacing
		> hud_pos_y+hudSize-HUD_OFFSET-BORDER) {
			horiz_pos -= maxWidth + 2*BORDER;
			vert_pos = hud_pos_y - hudSize+HUD_OFFSET + BORDER;
			maxWidth = -1;
	    }
	}
    }
    return RENDERER_STATUS_OK;
}

static int Semantic_hud_fuel_gauge_visible(void)
{
    return fuelGaugeColorRGBA
	&& (fuelTime > 0.0
	    || (fuelSum < fuelNotify
		&& ((fuelSum < fuelCritical && (loopsSlow % 4) < 2)
		    || (fuelSum < fuelWarning
			&& fuelSum > fuelCritical
			&& (loopsSlow % 8) < 4)
		    || fuelSum > fuelWarning)));
}

static RendererStatus Build_semantic_hud_fuel_gauge_geometry(
    int hud_pos_x, int hud_pos_y, SemanticHudFuelGaugeGeometry *geometry)
{
    double fill_size_value;
    int fill_size;
    int64_t outline_left;
    int64_t outline_top;
    int64_t outline_right;
    int64_t outline_bottom;
    int64_t fill_left;
    int64_t fill_top;
    int64_t fill_width;
    int64_t fill_height;
    int64_t fill_bottom;

    if (geometry == NULL || !isfinite(fuelSum) || !isfinite(fuelMax)
	|| fuelMax == 0.0) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    fill_size_value = (double)HUD_FUEL_GAUGE_SIZE * fuelSum / fuelMax;
    if (!isfinite(fill_size_value)
	|| fill_size_value < (double)INT_MIN
	|| fill_size_value > (double)INT_MAX) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    fill_size = (int)fill_size_value;

    outline_left = (int64_t)hud_pos_x + (int64_t)hudSize
	- (int64_t)HUD_OFFSET + (int64_t)FUEL_GAUGE_OFFSET - INT64_C(1);
    outline_top = (int64_t)hud_pos_y - (int64_t)hudSize
	+ (int64_t)HUD_OFFSET + (int64_t)FUEL_GAUGE_OFFSET - INT64_C(1);
    outline_right = outline_left
	+ (int64_t)(HUD_OFFSET - 2 * FUEL_GAUGE_OFFSET + 3);
    outline_bottom = outline_top + (int64_t)(HUD_FUEL_GAUGE_SIZE + 3);
    if (!Semantic_hud_integer_float(
	    outline_left, &geometry->outline[0].x)
	|| !Semantic_hud_integer_float(
	    outline_top, &geometry->outline[0].y)
	|| !Semantic_hud_integer_float(
	    outline_left, &geometry->outline[1].x)
	|| !Semantic_hud_integer_float(
	    outline_bottom, &geometry->outline[1].y)
	|| !Semantic_hud_integer_float(
	    outline_right, &geometry->outline[2].x)
	|| !Semantic_hud_integer_float(
	    outline_bottom, &geometry->outline[2].y)
	|| !Semantic_hud_integer_float(
	    outline_right, &geometry->outline[3].x)
	|| !Semantic_hud_integer_float(
	    outline_top, &geometry->outline[3].y)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    geometry->draw_fill = fill_size != 0;
    if (!geometry->draw_fill)
	return RENDERER_STATUS_OK;

    fill_left = (int64_t)hud_pos_x + (int64_t)hudSize
	- (int64_t)HUD_OFFSET + (int64_t)FUEL_GAUGE_OFFSET + INT64_C(1);
    fill_bottom = (int64_t)hud_pos_y - (int64_t)hudSize
	+ (int64_t)HUD_OFFSET + (int64_t)FUEL_GAUGE_OFFSET
	+ (int64_t)HUD_FUEL_GAUGE_SIZE + INT64_C(1);
    fill_top = fill_bottom - (int64_t)fill_size;
    fill_width = (int64_t)(HUD_OFFSET - 2 * FUEL_GAUGE_OFFSET);
    fill_height = (int64_t)fill_size;
    if (fill_height < 0) {
	fill_top += fill_height;
	fill_height = -fill_height;
    }
    if (!Semantic_hud_integer_float(fill_left, &geometry->fill_x)
	|| !Semantic_hud_integer_float(fill_top, &geometry->fill_y)
	|| !Semantic_hud_integer_float(fill_width, &geometry->fill_width)
	|| !Semantic_hud_integer_float(fill_height, &geometry->fill_height)) {
	return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Paint_semantic_hud_fuel_gauge(
    int hud_pos_x, int hud_pos_y)
{
    SemanticHudBatch batch;
    SemanticHudFuelGaugeGeometry geometry;
    RendererColor color;
    RendererStatus operation_status;

    if (!Semantic_hud_fuel_gauge_visible())
	return RENDERER_STATUS_OK;
    if (Begin_semantic_hud_batch(&batch) != RENDERER_STATUS_OK)
	return batch.status;
    operation_status = Build_semantic_hud_fuel_gauge_geometry(
	hud_pos_x, hud_pos_y, &geometry);
    if (operation_status != RENDERER_STATUS_OK)
	return Track_semantic_hud_batch(&batch, operation_status);

    operation_status = Renderer_set_blend(
	batch.renderer, RENDERER_BLEND_ADDITIVE);
    Track_semantic_hud_batch(&batch, operation_status);
    color = Renderer_color_from_rgba32(fuelGaugeColorRGBA);
    if (batch.status == RENDERER_STATUS_OK) {
	operation_status = Renderer_stroke_path(
	    batch.renderer, geometry.outline, 4, 1.0f, color, 1);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    if (batch.status == RENDERER_STATUS_OK && geometry.draw_fill) {
	operation_status = Renderer_fill_rect(
	    batch.renderer, geometry.fill_x, geometry.fill_y,
	    geometry.fill_width, geometry.fill_height, color);
	Accept_semantic_hud_command(&batch, operation_status);
    }
    return Finish_semantic_hud_batch(&batch);
}

#ifdef XPILOT_SDLGUI_TEST_HOOKS
RendererStatus Sdlgui_test_paint_hud_items(int hud_pos_x, int hud_pos_y)
{
    return Paint_HUD_items(hud_pos_x, hud_pos_y);
}

RendererStatus Sdlgui_test_paint_hud_fuel_gauge(
    int hud_pos_x, int hud_pos_y)
{
    return Paint_semantic_hud_fuel_gauge(hud_pos_x, hud_pos_y);
}

RendererStatus Sdlgui_test_paint_hud_frame(
    int hud_pos_x, int hud_pos_y)
{
    return Paint_semantic_hud_frame(hud_pos_x, hud_pos_y);
}
#endif

RendererStatus Paint_HUD_checked(void)
{
    const int		BORDER = 3;
    char		str[50];
    int			hud_pos_x, hud_pos_y;
    int			did_fuel = 0;
    int			i, j, tex_index;
    static char		autopilot[] = "Autopilot";
    fontbounds dummy;
    RendererStatus status;

    status = Paint_semantic_hud_pointers();
    if (status != RENDERER_STATUS_OK)
	return status;
    status = Paint_semantic_hud_radar_and_scans();
    if (status != RENDERER_STATUS_OK)
	return status;
    tex_index = 0;

    /*
     * Display the HUD
     */
    hud_pos_x = (int)(draw_width / 2 - hud_move_fact * selfVel.x);
    hud_pos_y = (int)(draw_height / 2 + hud_move_fact * selfVel.y);

    status = Paint_semantic_hud_frame(hud_pos_x, hud_pos_y);
    if (status != RENDERER_STATUS_OK)
	goto finish_hud;

    if (hudItemsColorRGBA) {
	status = Paint_HUD_items(hud_pos_x, hud_pos_y);
	if (status != RENDERER_STATUS_OK)
	    goto finish_hud;
    }

    /* Fuel notify, HUD meter on */
    if (hudColorRGBA && (fuelTime > 0.0 || fuelSum < fuelNotify)) {
	did_fuel = 1;
	/* TODO fix this */
	sprintf(str, "%04d", (int)fuelSum);
	tex_index=0;
	if (Ensure_cached_text(&gamefont, str, &HUD_texs[tex_index])) {
	    status = disp_text(
		&HUD_texs[tex_index], hudColorRGBA, LEFT, DOWN,
		hud_pos_x + hudSize - HUD_OFFSET + BORDER,
		hud_pos_y - (hudSize - HUD_OFFSET + BORDER), true);
	    if (status != RENDERER_STATUS_OK)
		goto finish_hud;
	}

	if (numItems[ITEM_TANK]) {
	    if (fuelCurrent == 0)
		strcpy(str,"M ");
	    else
		sprintf(str, "T%d", fuelCurrent);

	    tex_index=1;
	    if (Ensure_cached_text(&gamefont, str, &HUD_texs[tex_index])) {
		status = disp_text(
		    &HUD_texs[tex_index], hudColorRGBA, LEFT, DOWN,
		    hud_pos_x + hudSize - HUD_OFFSET + BORDER,
		    hud_pos_y - hudSize - HUD_OFFSET + BORDER, true);
		if (status != RENDERER_STATUS_OK)
		    goto finish_hud;
	    }

	}
    }

    /* Update the lock display */
    status = Paint_lock(hud_pos_x, hud_pos_y);
    if (status != RENDERER_STATUS_OK)
	goto finish_hud;

    /* Draw last score on hud if it is an message attached to it */
    if (hudColorRGBA) {
	for (i = 0, j = 0; i < MAX_SCORE_OBJECTS; i++) {
	    score_object_t*	sobj = &score_objects[(i+score_object)%MAX_SCORE_OBJECTS];
	    if (sobj->hud_msg_len > 0) {
		status = printsize(&gamefont, &dummy, "%s", sobj->hud_msg);
		if (status != RENDERER_STATUS_OK)
		    goto finish_hud;
		if (sobj->hud_msg_width == -1)
		    sobj->hud_msg_width = (int)dummy.width;
		if (j == 0 &&
		    sobj->hud_msg_width > 2*hudSize-HUD_OFFSET*2 &&
		    (did_fuel || hudVLineColorRGBA))
		    ++j;

		tex_index=MAX_HUD_TEXS+i;
		if (Ensure_cached_text(&gamefont, sobj->hud_msg,
				       &HUD_texs[tex_index])) {
		    status = disp_text(
			&HUD_texs[tex_index], hudColorRGBA,
			CENTER, DOWN, hud_pos_x,
			hud_pos_y - (hudSize - HUD_OFFSET + BORDER
				+ j * HUD_texs[tex_index].height), true);
		    if (status != RENDERER_STATUS_OK)
			goto finish_hud;
		}
		j++;
	    }
	}

	if (time_left > 0) {
	    sprintf(str, "%3d:%02d", (int)(time_left / 60), (int)(time_left % 60));
	    tex_index=3;
	    if (Ensure_cached_text(&gamefont, str, &HUD_texs[tex_index])) {
		status = disp_text(
		    &HUD_texs[tex_index], hudColorRGBA, RIGHT, DOWN,
		    hud_pos_x - hudSize + HUD_OFFSET - BORDER,
		    hud_pos_y + hudSize + HUD_OFFSET + BORDER, true);
		if (status != RENDERER_STATUS_OK)
		    goto finish_hud;
	    }
	}

	/* Update the modifiers */
	tex_index=4;
	if(strlen(mods)) {
	    if (Ensure_cached_text(&gamefont, mods, &HUD_texs[tex_index])) {
		status = disp_text(
		    &HUD_texs[tex_index], hudColorRGBA, RIGHT, UP,
		    hud_pos_x - hudSize + HUD_OFFSET - BORDER,
		    hud_pos_y - hudSize + HUD_OFFSET - BORDER, true);
		if (status != RENDERER_STATUS_OK)
		    goto finish_hud;
	    }
    	}

	if (autopilotLight) {
	    tex_index=5;
	    if (Ensure_cached_text(&gamefont, autopilot,
				   &HUD_texs[tex_index])) {
		status = disp_text(
		    &HUD_texs[tex_index], hudColorRGBA, RIGHT, DOWN,
		    hud_pos_x,
		    hud_pos_y + hudSize + HUD_OFFSET + BORDER
			+ HUD_texs[tex_index].height * 2, true);
		if (status != RENDERER_STATUS_OK)
		    goto finish_hud;
	    }
	}
    }

    if (fuelTime > 0.0) {
	fuelTime -= timePerFrame;
	if (fuelTime <= 0.0)
	    fuelTime = 0.0;
    }

    status = Paint_semantic_hud_fuel_gauge(hud_pos_x, hud_pos_y);

finish_hud:
    return status;
}

void Paint_HUD(void)
{
    (void)Paint_HUD_checked();
}

typedef struct alert_timeout_struct alert_timeout;
struct alert_timeout_struct {
    GLWidget	    *msg;   /* use to build widget lists */
    double     	    timeout;
    alert_timeout   *next;
};
static alert_timeout *alert_timeout_list = NULL;

void Add_alert_message(const char *message, double timeout)
{
    GLWidget *tmp = NULL;
    alert_timeout *tol;
    
    tmp = Init_LabelWidget(message,&whiteRGBA,&nullRGBA,CENTER,CENTER);
    if (tmp) {
    	ListWidget_Prepend(((WrapperWidget *)(MainWidget->wid_info))->alert_msgs,tmp);
    } else {
    	error("Add_alert_message: Failed to create LabelWidget");
	return;
    }
    
    tol = alert_timeout_list;
    alert_timeout_list = (alert_timeout *)malloc(sizeof(alert_timeout));
    alert_timeout_list->next = tol;
    alert_timeout_list->timeout = timeout;
    alert_timeout_list->msg = tmp;
}

void Clear_alert_messages(void)
{
    GLWidget *tmp,*list;
    bool dummy;
    alert_timeout *tol;
    
    while ((tol = alert_timeout_list)) {
    	alert_timeout_list = alert_timeout_list->next;
	free(tol);
    }
    
    list = ((WrapperWidget *)(MainWidget->wid_info))->alert_msgs;
    dummy = true;
    while (dummy) {
    	tmp = ListWidget_GetItemByIndex( list, 0 );
	if (tmp == NULL) break;
    	dummy = ListWidget_Remove( list, tmp );
    }
}

void Paint_messages(void)
{
    static int old_maxMessages = 0;
    static message_t **msgs[2];
    static GLWidget *msg_list[2] = {NULL,NULL};
    static bool showMessages = true;

    int j, i = 0;
    Uint32 *msg_color;
    /*const int BORDER = 10;*/
    GLWidget *tmp = NULL,*tmp2 = NULL;
    LabelWidget *wi;
    message_t	*msg;

    alert_timeout *garbage, **tol = &alert_timeout_list;
    static int lastloops;
    
    msgs[0] = TalkMsg;
    msgs[1] = GameMsg;
    
    msg_list[0] = ((WrapperWidget *)(MainWidget->wid_info))->chat_msgs;
    msg_list[1] = ((WrapperWidget *)(MainWidget->wid_info))->game_msgs;

    if ( showMessages != instruments.showMessages ) {
    	if (!instruments.showMessages)
	    DelGLWidgetListItem( &(MainWidget->children), ((WrapperWidget *)(MainWidget->wid_info))->game_msgs );
	else
	    AppendGLWidgetList( &(MainWidget->children), ((WrapperWidget *)(MainWidget->wid_info))->game_msgs );
	        
	showMessages = instruments.showMessages;
    }
    
    if ( maxMessages < old_maxMessages ) {
    	for (i=0;i<2;++i)
	    while ((tmp = ListWidget_GetItemByIndex(msg_list[i],maxMessages)) != NULL) {
    	    	ListWidget_Remove(msg_list[i],tmp);
	    	Close_Widget(&tmp);
    	    }
    }
    
    /* Check if any alert message has timed out, if so remove it */
    while ((*tol)) {
	if ((*tol)->timeout != 0.0) {
	    (*tol)->timeout -= (loops - lastloops)/(double)FPS;
	    if ((*tol)->timeout <= 0.0) {
	    	garbage = (*tol);
		*tol = (*tol)->next;
    	    	ListWidget_Remove( ((WrapperWidget *)(MainWidget->wid_info))->alert_msgs, garbage->msg );
	    	free(garbage);
		continue;
	    }
	}
    	tol = &((*tol)->next);
    }
    lastloops = loops;
    
    /* TODO: check whether there is a more efficient way to do this!
     * i.e. add labelwidgets as messages are added/removed
     * For now this will have to do...
     */
    for ( i=0 ; i < 2 ; ++i)
    for ( j=0 ; j <= maxMessages-1 ; ++j) {
    	msg = (msgs[i])[j];
	tmp = tmp2 = NULL;
	
    	if ((msg->lifeTime -= timePerFrame) <= 0.0) {
    	    msg->txt[0] = '\0';
    	    msg->len = 0;
    	    msg->lifeTime = 0.0;
    	}
	
	if ((tmp = ListWidget_GetItemByIndex(msg_list[i],j) ) != NULL) {
	    if ( !(wi = (LabelWidget *)tmp->wid_info) ) {
	    	error("Paint_messages: ListWidget lacks a wid_info ptr!");
		continue;
	    }
	    if (strlen(msg->txt)) {
	    	if ( strcmp(msg->txt, wi->tex.text) ) {
		    tmp2 = Init_LabelWidget(msg->txt,&messagesColorRGBA,&nullRGBA,LEFT,CENTER);
		    ListWidget_Insert(msg_list[i],tmp,tmp2);
		    if (ListWidget_NELEM(msg_list[i])>maxMessages) {
		    	tmp = ListWidget_GetItemByIndex(msg_list[i],maxMessages);
			ListWidget_Remove(msg_list[i],tmp);
			Close_Widget(&tmp);
		    }
		} else tmp2 = tmp;
	    } else {
	    	ListWidget_Remove(msg_list[i],tmp);
		Close_Widget(&tmp);
	    }
	} else {
	    if (strlen(msg->txt)) {
		tmp2 = Init_LabelWidget(msg->txt,&messagesColorRGBA,&nullRGBA,LEFT,CENTER);
		ListWidget_Append(msg_list[i],tmp2);
	    }
	}
		
	if (msg->lifeTime <= MSG_FLASH_TIME)
	    msg_color = &oldmessagesColorRGBA;
	else {
	    /* If paused, don't bother to paint messages in mscScan* colors. */
	    if (self && strchr("P", self->mychar))
		msg_color = &messagesColorRGBA;
	    else {
		switch (msg->bmsinfo) {
		case BmsBall:	msg_color = &msgScanBallColorRGBA;	break;
		case BmsSafe:	msg_color = &msgScanSafeColorRGBA;	break;
		case BmsCover:	msg_color = &msgScanCoverColorRGBA;	break;
		case BmsPop:	msg_color = &msgScanPopColorRGBA;	break;
		default:	msg_color = &messagesColorRGBA;	    	break;
		}
	    }
	}

	/* kps ugly hack */
	if (newbie && msg->txt) {
	    const char *help = "[*Newbie help*]";
	    size_t sz = strlen(msg->txt);
	    size_t hsz = strlen(help);

	    if (sz > hsz
		&& !strcmp(help, &msg->txt[sz - hsz]))
		msg_color = &whiteRGBA;
	}

	
	if (tmp2) LabelWidget_SetColor(tmp2, msg_color, &nullRGBA);
    }
    	
    old_maxMessages = maxMessages;
}

static bool set_rgba_color_option(xp_option_t *opt, const char *val)
{
    int c = 0;
    assert(val);
    if (*val != '#') return false;
    c = strtoul(val + 1, NULL, 16) & 0xffffffff;
    *((int*)Option_get_private_data(opt)) = c;
    return true;
}

static const char *get_rgba_color_option(xp_option_t *opt)
{
    static char buf[10];
    sprintf(buf, "#%08X", *((int*)Option_get_private_data(opt)));
    return buf;
}

#define COLOR(variable, defval, description) \
    XP_STRING_OPTION(#variable, \
		     defval, \
                     NULL, \
		     0, \
		     set_rgba_color_option, \
		     &variable, \
		     get_rgba_color_option, \
		     XP_OPTFLAG_CONFIG_COLORS, \
		     "The color of " description ".\n")


static xp_option_t sdlgui_options[] = {

    COLOR(messagesColorRGBA, "#00aaaaff", "messages"),
    COLOR(oldmessagesColorRGBA, "#008888ff", "old messages"),
    COLOR(msgScanBallColorRGBA, "#ff0000ff", "ball warning"),
    COLOR(msgScanSafeColorRGBA, "#00ff00ff", "ball safe announcement"),
    COLOR(msgScanCoverColorRGBA, "#4e7cffff", "cover request"),
    COLOR(msgScanPopColorRGBA, "#ffbb11ff", "ball pop announcement"),

    COLOR(meterBorderColorRGBA, "#0000ffaa", "meter borders"),
    COLOR(fuelMeterColorRGBA, "#ff0000aa", "fuel meter"),
    COLOR(fuelGaugeColorRGBA, "#0000ff44", "fuel gauge"),
    COLOR(powerMeterColorRGBA, "#ff0000aa", "power meter"),
    COLOR(turnSpeedMeterColorRGBA, "#ff0000aa", "turn speed meter"),
    COLOR(packetSizeMeterColorRGBA, "#ff0000aa", "packet size meter"),
    COLOR(packetLossMeterColorRGBA, "#ff0000aa", "packet loss meter"),
    COLOR(packetDropMeterColorRGBA, "#ff0000aa", "drop meter"),
    COLOR(packetLagMeterColorRGBA, "#ff0000aa", "lag meter"),
    COLOR(temporaryMeterColorRGBA, "#ff0000aa", "time meter"),

    COLOR(ballColorRGBA, "#00ff00ff", "balls"),
    COLOR(connColorRGBA, "#00ff0088", "the ball connector"),
    COLOR(fuelColorRGBA, "#ffffff7f", "fuel cells"),
    COLOR(wallColorRGBA, "#0000ffff", "walls on blockmaps"),
    COLOR(decorColorRGBA, "#bb7700ff", "decorations on blockmaps"),
    COLOR(baseNameColorRGBA, "#77bbffff", "base name"),
    COLOR(shipNameColorRGBA, "#77bbffff", "ship name"),
    COLOR(scoreObjectColorRGBA, "#00ff0088", "score objects"),

    COLOR(hudColorRGBA, "#ff000088", "the HUD"),
    COLOR(hudHLineColorRGBA, "#0000ff44", "horizontal HUD line"),
    COLOR(hudVLineColorRGBA, "#0000ff44", "vertical HUD line"),
    COLOR(hudItemsColorRGBA, "#0080ffaa", "hud items"),
    COLOR(hudRadarEnemyColorRGBA, "#ff000088", "enemy on HUD radar"),
    COLOR(hudRadarOtherColorRGBA, "#0000ff88", "friend on HUD radar"),
    COLOR(hudRadarObjectColorRGBA, "#00000000", "small object on HUD radar"),

    COLOR(dirPtrColorRGBA, "#0000ff22", "direction pointer"),
    COLOR(selectionColorRGBA, "#ff0000ff", "selection"),

    COLOR(scoreInactiveSelfColorRGBA, "#88008888", "my score when inactive"),
    COLOR(scoreInactiveColorRGBA, "#8800aa88", "score when inactive"),
    COLOR(scoreSelfColorRGBA, "#ffff00ff", "my score"),
    COLOR(scoreColorRGBA, "#888800ff", "score"),
    COLOR(scoreOwnTeamColorRGBA, "#0000ffff", "my team score"),
    COLOR(scoreEnemyTeamColorRGBA, "#ff0000ff", "enemy team score"),

    COLOR(teamShipColorRGBA, "#0000ffff", "teammate color"),
    COLOR(team0ColorRGBA, "#00000000", "team 0"),
    COLOR(team1ColorRGBA, "#00000000", "team 1"),
    COLOR(team2ColorRGBA, "#00000000", "team 2"),
    COLOR(team3ColorRGBA, "#00000000", "team 3"),
    COLOR(team4ColorRGBA, "#00000000", "team 4"),
    COLOR(team5ColorRGBA, "#00000000", "team 5"),
    COLOR(team6ColorRGBA, "#00000000", "team 6"),
    COLOR(team7ColorRGBA, "#00000000", "team 7"),
    COLOR(team8ColorRGBA, "#00000000", "team 8"),
    COLOR(team9ColorRGBA, "#00000000", "team 9"),
    
    COLOR(selfLWColorRGBA, "#ff0000ff", "my ship on last life"),
    COLOR(teamLWColorRGBA, "#ff00ffff", "team ship on last life"),
    COLOR(enemyLWColorRGBA, "#ffff00ff", "enemy ship on last life"),
    COLOR(manyLivesColorRGBA, "#666666aa", "name of ship with many lives"),
    COLOR(twoLivesColorRGBA, "#008800aa", "name of ship with two lives"),
    COLOR(oneLifeColorRGBA, "#aaaa00aa", "name of ship with one life"),
    COLOR(zeroLivesColorRGBA, "#ff0000aa", "name of ship with no lives"),
    
    XP_INT_OPTION(
        "meterWidth",
	60, 0, 600,
	&meterWidth,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set the width of the meters.\n"),

    XP_INT_OPTION(
        "meterHeight",
	10, 0, 100,
	&meterHeight,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set the height of a meter.\n"),

    XP_DOUBLE_OPTION(
        "shipLineWidth",
	1.0, 1.0, 10.0,
	&shipLineWidth,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Set the line width of ships.\n"),

    XP_BOOL_OPTION(
        "smoothLines",
        true,
	&smoothLines,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Use antialized smooth lines.\n"),

    XP_BOOL_OPTION(
        "texturedBalls",
        true,
	&texturedBalls,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Draw balls with textures.\n"),

    XP_BOOL_OPTION(
        "texturedShips",
        true,
	&texturedShips,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"Draw ships with textures.\n"),

    XP_INT_OPTION(
        "hudRadarEnemyShape",
	2, 1, 7,
	&hudRadarEnemyShape,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"The shape of enemy ships on hud radar.\n"),

    XP_INT_OPTION(
        "hudRadarOtherShape",
	2, 1, 7,
	&hudRadarOtherShape,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"The shape of friendly ships on hud radar.\n"),

    XP_INT_OPTION(
        "hudRadarObjectShape",
	0, 0, 7,
	&hudRadarObjectShape,
	NULL,
	XP_OPTFLAG_CONFIG_DEFAULT,
	"The shape of small objects on hud radar.\n")
};

void Store_sdlgui_options(void)
{
    STORE_OPTIONS(sdlgui_options);
}
