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
#include "images.h"
#include "glwidgets.h"
#include "text.h"
#include "asteroid_data.h"

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
static GLuint asteroid = 0;

irec_t *select_bounds;

string_tex_t score_object_texs[MAX_SCORE_OBJECTS];
string_tex_t meter_texs[MAX_METERS];
string_tex_t message_texs[2*MAX_MSGS];
string_tex_t HUD_texs[MAX_HUD_TEXS+MAX_SCORE_OBJECTS];

int Gui_init(void);
void Gui_cleanup(void);

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

int GL_X(int x)
{
    return (int)((x - world.x) * clData.scale);
}

int GL_Y(int y)
{
    return (int)((y - world.y) * clData.scale);
}


/* remove this later maybe? to tedious for me to edit them all away now */
void Segment_add(Uint32 color, int x_1, int y_1, int x_2, int y_2)
{
    if (smoothLines) glEnable(GL_LINE_SMOOTH);
    set_alphacolor(color);
    glBegin( GL_LINE_LOOP );
    	glVertex2i(x_1,y_1);
	glVertex2i(x_2,y_2);
    glEnd();
    if (smoothLines) glDisable(GL_LINE_SMOOTH);
}

void Circle(Uint32 color,
	    int x, int y,
	    int radius, int filled)
{
    float i,resolution = 16;
    set_alphacolor(color);
    if (filled)
    	glBegin( GL_POLYGON );
    else
    	glBegin( GL_LINE_LOOP );
    	/* Silly resolution */
    	for (i = 0.0f; i < TABLE_SIZE; i=i+((float)TABLE_SIZE)/resolution)
    	    glVertex2f((x + tcos((int)i)*radius),(y + tsin((int)i)*radius));
    glEnd();
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
    if (asteroid) 
	glDeleteLists(asteroid, 1);
}

int Gui_init(void)
{
    int i;
    GLUtriangulatorObj *tess;

    if (asteroid_init() == -1) {
	error("failed to initialize asteroids");
	return -1;
    }
    
    if (num_polygons == 0) return 0;

    polyListBase = glGenLists(num_polygons);
    polyEdgeListBase = glGenLists(num_polygons);
    if ((!polyListBase)||(!polyEdgeListBase)) {
	error("failed to generate display lists");
	return -1;
    }

    tess = gluNewTess();
    if (tess == NULL) {
	error("failed to create tessellation object");
	return -1;
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
}

void Gui_cleanup(void)
{
    if (polyListBase)
	glDeleteLists(polyListBase, num_polygons);
    asteroid_cleanup();
}

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
    glBegin(GL_LINE);
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
	glBindTexture(GL_TEXTURE_2D, img->name);
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

/* this displays the asteroid hit area */
#if 0
    int i, x, y, size;
    for (i = 0; i < num_asteroids; i++) {
	x = asteroid_ptr[i].x;
	y = asteroid_ptr[i].y;
	if (wrap(&x, &y)) {
	    size = asteroid_ptr[i].size;
	    Circle(whiteRGBA, x, y, (int)(0.8 * SHIP_SZ * size), 0);
	}
    }
#endif
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
    glEnd();
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
		    if (!score_object_texs[i].tex_list || strcmp(sobj->msg,score_object_texs[i].text)) {
		    	free_string_texture(&score_object_texs[i]);
		    	draw_text(&mapfont, scoreObjectColorRGBA
			    	    ,CENTER,CENTER, x, y, sobj->msg, true
				    , &score_object_texs[i],false);
		    } else disp_text(&score_object_texs[i],scoreObjectColorRGBA,CENTER,CENTER,x,y,false);
		}
	    }
	    sobj->life_time -= timePerFrame;
	    if (sobj->life_time <= 0.0) {
		sobj->life_time = 0.0;
		sobj->hud_msg_len = 0;
	    }
	} else {
	    if (score_object_texs[i].tex_list) free_string_texture(&score_object_texs[i]);
	}
    }
}

void Paint_select(void)
{
    if(!select_bounds) return;
    set_alphacolor(selectionColorRGBA);
    glBegin(GL_LINE_LOOP);
    	glVertex2i(select_bounds->x 	    	    	,select_bounds->y   	    	    );
    	glVertex2i(select_bounds->x + select_bounds->w	,select_bounds->y   	    	    );
    	glVertex2i(select_bounds->x + select_bounds->w	,select_bounds->y + select_bounds->h);
    	glVertex2i(select_bounds->x 	    	    	,select_bounds->y + select_bounds->h);
    glEnd();
}

void Paint_HUD_values(void)
{
    int x, y;

    if (!hudColorRGBA)
	return;

    x = draw_width - 20;
    /* Better make sure it's below the meters */
    y = draw_height - 9*(MAX((GLuint)meterHeight,gamefont.h) + 6);

    HUDprint(&gamefont,hudColorRGBA,RIGHT,DOWN,x,y,"FPS: %.3f",clientFPS);
    HUDprint(&gamefont,hudColorRGBA,RIGHT,DOWN,x,y-20,"CL.LAG : %.1f ms", clData.clientLag);
}

static void Paint_meter(int xoff, int y, string_tex_t *tex, int val, int max,
			int meter_color)
{
    int	mw1_4 = meterWidth/4,
    	mw2_4 = meterWidth/2,
    	mw3_4 = 3*meterWidth/4,
    	mw4_4 = meterWidth,
    	BORDER = 5;
    int		x, xstr;
    int x_alignment;
    int color;

    if (xoff >= 0) {
	x = xoff;
        xstr = x + meterWidth + BORDER;
	x_alignment = LEFT;
    } else {
	x = draw_width - (meterWidth - xoff);
        xstr = x - BORDER;
	x_alignment = RIGHT;
    }

    set_alphacolor(meter_color);
    glBegin( GL_POLYGON );
    	glVertex2i(x,y);
    	glVertex2i(x,y+2+meterHeight-3);
    	glVertex2i(x+(int)(((meterWidth)*val)/(max?max:1)),y+2+meterHeight-3);
    	glVertex2i(x+(int)(((meterWidth)*val)/(max?max:1)),y);
    glEnd();



    /* meterBorderColorRGBA = 0 obviously means no meter borders are drawn */
    if (meterBorderColorRGBA) {
    	color = meterBorderColorRGBA;

	set_alphacolor(color);
	glBegin( GL_LINE_LOOP );
	    glVertex2i(x,y);
	    glVertex2i(x,y + meterHeight);
	    glVertex2i(x + meterWidth,y + meterHeight);
	    glVertex2i(x + meterWidth,y);
	glEnd();

	glBegin( GL_LINES );
	    glVertex2i(x,       y-4);glVertex2i(x,       y+meterHeight+4);
	    glVertex2i(x+mw4_4, y-4);glVertex2i(x+mw4_4, y+meterHeight+4);
	    glVertex2i(x+mw2_4, y-3);glVertex2i(x+mw2_4, y+meterHeight+3);
	    glVertex2i(x+mw1_4, y-1);glVertex2i(x+mw1_4, y+meterHeight+1);
	    glVertex2i(x+mw3_4, y-1);glVertex2i(x+mw3_4, y+meterHeight+1);
	glEnd();
    }

    if (!meterBorderColorRGBA)
	color = meter_color;

    disp_text(tex,color,x_alignment,CENTER,xstr,draw_height - y - meterHeight/2,true);
}

void Paint_meters(void)
{
    int spacing = MAX((GLuint)meterHeight,gamefont.h) + 6;
    int y = spacing, color;
    static bool setup_texs = true;

    if (setup_texs) {
    	render_text(&gamefont,"Fuel"	    	, &meter_texs[0]);
    	render_text(&gamefont,"Power"	    	, &meter_texs[1]);
     	render_text(&gamefont,"Turnspeed"   	, &meter_texs[2]);
   	render_text(&gamefont,"Packet"	   	, &meter_texs[3]);
    	render_text(&gamefont,"Loss"	    	, &meter_texs[4]);
    	render_text(&gamefont,"Drop"	    	, &meter_texs[5]);
    	render_text(&gamefont,"Lag" 	    	, &meter_texs[6]);
    	render_text(&gamefont,"Thrust Left" 	, &meter_texs[7]);
    	render_text(&gamefont,"Shields Left"	, &meter_texs[8]);
    	render_text(&gamefont,"Phasing left"	, &meter_texs[9]);
    	render_text(&gamefont,"Self destructing", &meter_texs[10]);
    	render_text(&gamefont,"SHUTDOWN"    	, &meter_texs[11]);
	setup_texs = false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    glDisable(GL_BLEND);
}

static void Paint_lock(int hud_pos_x, int hud_pos_y)
{
    other_t *target;
    const int BORDER = 2;

    if ((target = Other_by_id(lock_id)) == NULL)
	return;

    if (hudColorRGBA) {
	int color = Life_color(target);

	if (!color)
	    color = hudColorRGBA;

	HUDnprint(&gamefont,
		  color,CENTER,CENTER,
		  hud_pos_x,
		  hud_pos_y -(- hudSize + HUD_OFFSET - BORDER),
		  strlen(target->id_string),"%s",target->id_string);

    }


}

static void Paint_hudradar_dot(int x, int y, Uint32 col, int shape, int sz)
{
    if (col == 0 || shape < 2 || sz == 0) return;
    set_alphacolor(col);

    switch(shape) {
    case 2:
    case 3:
	Circle(col, x, y, sz, shape == 2 ? 1 : 0);
	break;
    case 4:
    case 5:
	glBegin(shape == 4 ? GL_QUADS : GL_LINE_LOOP);
	glVertex2i(x - sz, y - sz);
	glVertex2i(x - sz, y + sz);
	glVertex2i(x + sz, y + sz);
	glVertex2i(x + sz, y - sz);
	glEnd();
	break;
    case 6:
    case 7:
	glBegin(shape == 6 ? GL_TRIANGLES : GL_LINE_LOOP);
	glVertex2i(x - sz, y + sz);
	glVertex2i(x, y - sz);
	glVertex2i(x + sz, y + sz);
	glEnd();
	break;
    }
}

static void Paint_hudradar(double hrscale, double xlimit, double ylimit, int sz)
{
    Uint32 c;
    int i, x, y, shape, size;
    int hrw = (int)(hrscale * 256);
    int hrh = (int)(hrscale * RadarHeight);
    double xf = (double) hrw / (double) Setup->width;
    double yf = (double) hrh / (double) Setup->height;

    for (i = 0; i < num_radar; i++) {
	x = (int)(radar_ptr[i].x * hrscale
		  - (world.x + ext_view_width / 2) * xf);
	y = (int)(radar_ptr[i].y * hrscale
		  - (world.y + ext_view_height / 2) * yf);

	if (x < -hrw / 2)
	    x += hrw;
	else if (x > hrw / 2)
	    x -= hrw;

	if (y < -hrh / 2)
	    y += hrh;
	else if (y > hrh / 2)
	    y -= hrh;

	if (!((x <= xlimit) && (x >= -xlimit)
	      && (y <= ylimit) && (y >= -ylimit))) {

 	    x = x + draw_width / 2;
 	    y = -y + draw_height / 2;

	    if (radar_ptr[i].type == RadarEnemy) {
		c = hudRadarEnemyColorRGBA;
		shape = hudRadarEnemyShape;
	    } else {
		c = hudRadarOtherColorRGBA;
		shape = hudRadarOtherShape;
	    }
	    size = sz;
	    if (radar_ptr[i].size == 0) {
		size >>= 1;
		if (hudRadarObjectColorRGBA)
		    c = hudRadarObjectColorRGBA;
		if (hudRadarObjectShape)
		    shape = hudRadarObjectShape;
	    }
	    Paint_hudradar_dot(x, y, c, shape, size);
	}
    }
}

static void Paint_HUD_items(int hud_pos_x, int hud_pos_y)
{
    const int		BORDER = 3;
    char		str[50];
    int     	    	vert_pos, horiz_pos, minx, miny, maxx, maxy;
    int     	    	i, maxWidth = -1,
			rect_x, rect_y, rect_width = 0, rect_height = 0;
    static int		vertSpacing = -1;
    static fontbounds	fb;


    /* Special itemtypes */
    if (vertSpacing < 0)
	vertSpacing = MAX(ITEM_SIZE, gamefont.h) + 1;
    /* find the scaled location, then work in pixels */
    vert_pos = hud_pos_y - hudSize+HUD_OFFSET + BORDER;
    horiz_pos = hud_pos_x - hudSize+HUD_OFFSET - BORDER;
    rect_width = 0;
    rect_height = 0;
    rect_x = horiz_pos;
    rect_y = vert_pos;

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

    	    Image_paint(IMG_HUD_ITEMS, 
			horiz_pos - ITEM_SIZE, 
			vert_pos, (u_byte)i, 
			hudItemsColorRGBA);

	    if (i == lose_item) {
		if (lose_item_active != 0) {
		    if (lose_item_active < 0)
			lose_item_active++;
			minx = horiz_pos-ITEM_SIZE-2;
			maxx = horiz_pos;
			miny = vert_pos-2;
			maxy = vert_pos + ITEM_SIZE;
			
    	    	    	glBegin(GL_LINE_LOOP);
    	    	    	    glVertex2i(minx , miny);
    	    	    	    glVertex2i(maxx , miny);
    	    	    	    glVertex2i(maxx , maxy);
    	    	    	    glVertex2i(minx , maxy);
    	    	    	glEnd();
		}
	    }

	    /* Paint item count */
	    sprintf(str, "%d", num);
	    fb = printsize(&gamefont,"%s",str);

	    maxWidth = MAX(maxWidth, fb.width + BORDER + ITEM_SIZE);
	    
	    HUDprint(&gamefont,hudItemsColorRGBA,RIGHT,UP,horiz_pos - ITEM_SIZE - BORDER
	    	    ,draw_height - vert_pos - ITEM_SIZE,"%s",str);

	    vert_pos += vertSpacing;

	    if (vert_pos+vertSpacing
		> hud_pos_y+hudSize-HUD_OFFSET-BORDER) {
		rect_width += maxWidth + 2*BORDER;
		rect_height = MAX(rect_height, vert_pos - rect_y);
		horiz_pos -= maxWidth + 2*BORDER;
		vert_pos = hud_pos_y - hudSize+HUD_OFFSET + BORDER;
		maxWidth = -1;
	    }
	}
    }
    if (maxWidth != -1)
	rect_width += maxWidth + BORDER;

    if (rect_width > 0) {
	if (rect_height == 0)
	    rect_height = vert_pos - rect_y;
	rect_x -= rect_width;
    }

}

typedef char hud_text_t[50];

void Paint_HUD(void)
{
    const int		BORDER = 3;
    char		str[50];
    int			hud_pos_x, hud_pos_y, size;
    int			did_fuel = 0;
    int			i, j, tex_index, modlen = 0;
    static char		autopilot[] = "Autopilot";
    int tempx,tempy,tempw,temph;
    static hud_text_t 	hud_texts[MAX_HUD_TEXS+MAX_SCORE_OBJECTS];

    fontbounds dummy;
    tex_index = 0;
    glEnable(GL_BLEND);
    /*
     * Show speed pointer
     */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    if (ptr_move_fact != 0.0
	&& selfVisible
	&& (selfVel.x != 0 || selfVel.y != 0))
    	Segment_add(hudColorRGBA,
		    draw_width / 2,
		    draw_height / 2,
		    (int)(draw_width / 2 - ptr_move_fact * selfVel.x),
		    (int)(draw_height / 2 + ptr_move_fact * selfVel.y));

    if (selfVisible && dirPtrColorRGBA) {
	Segment_add(dirPtrColorRGBA,
		    (int) (draw_width / 2 +
			   (100 - 15) * tcos(heading)),
		    (int) (draw_height / 2 -
			   (100 - 15) * tsin(heading)),
		    (int) (draw_width / 2 + 100 * tcos(heading)),
		    (int) (draw_height / 2 - 100 * tsin(heading)));
    }

    /* TODO */
    /* This should be done in a nicer way now (using radar.c maybe) */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (hudRadarEnemyColorRGBA 
	|| hudRadarOtherColorRGBA 
	|| hudRadarObjectColorRGBA) {
	hudRadarMapScale = (double) Setup->width / (double) 256;
	Paint_hudradar(
	    hudRadarScale,
	    (int)(hudRadarLimit * (ext_view_width / 2)
		  * hudRadarScale / hudRadarMapScale),
	    (int)(hudRadarLimit * (ext_view_height / 2)
		  * hudRadarScale / hudRadarMapScale),
	    hudRadarDotSize);

	Paint_hudradar(hudRadarMapScale*clData.scale,
		       (active_view_width / 2)*clData.scale,
		       (active_view_height / 2)*clData.scale,
		       SHIP_SZ);
    }


    glDisable(GL_BLEND);
    /* message scan hack by mara and jpv */
    if (Bms_test_state(BmsBall) && msgScanBallColorRGBA)
	Circle(msgScanBallColorRGBA, draw_width / 2,
	       draw_height / 2, (int)(8*clData.scale),0);
    if (Bms_test_state(BmsCover) && msgScanCoverColorRGBA)
	Circle(msgScanCoverColorRGBA, draw_width / 2,
	       draw_height / 2, (int)(6*clData.scale),0);

    glEnable(GL_BLEND);

    /*
     * Display the HUD
     */
    hud_pos_x = (int)(draw_width / 2 - hud_move_fact * selfVel.x);
    hud_pos_y = (int)(draw_height / 2 + hud_move_fact * selfVel.y);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    /* HUD frame */
    glLineStipple(4, 0xAAAA);
    glEnable(GL_LINE_STIPPLE);
    if (hudHLineColorRGBA) {
    	set_alphacolor(hudHLineColorRGBA);
    	glBegin(GL_LINES);
    	    glVertex2i(hud_pos_x - hudSize,hud_pos_y - hudSize + HUD_OFFSET);
	    glVertex2i(hud_pos_x + hudSize,hud_pos_y - hudSize + HUD_OFFSET);

	    glVertex2i(hud_pos_x - hudSize,hud_pos_y + hudSize - HUD_OFFSET);
	    glVertex2i(hud_pos_x + hudSize,hud_pos_y + hudSize - HUD_OFFSET);
    	glEnd();
    }
    if (hudVLineColorRGBA) {
    	set_alphacolor(hudVLineColorRGBA);
    	glBegin(GL_LINES);
    	    glVertex2i(hud_pos_x - hudSize + HUD_OFFSET,hud_pos_y - hudSize);
	    glVertex2i(hud_pos_x - hudSize + HUD_OFFSET,hud_pos_y + hudSize);

	    glVertex2i(hud_pos_x + hudSize - HUD_OFFSET,hud_pos_y - hudSize);
	    glVertex2i(hud_pos_x + hudSize - HUD_OFFSET,hud_pos_y + hudSize);
    	glEnd();
    }
    glDisable(GL_LINE_STIPPLE);

    if (hudItemsColorRGBA)
	Paint_HUD_items(hud_pos_x, hud_pos_y);

    /* Fuel notify, HUD meter on */
    if (hudColorRGBA && (fuelTime > 0.0 || fuelSum < fuelNotify)) {
	did_fuel = 1;
	/* TODO fix this */
	sprintf(str, "%04d", (int)fuelSum);
	tex_index=0;
	if (strcmp(str,hud_texts[tex_index])!=0) {
    	    if (HUD_texs[tex_index].tex_list)
	    	free_string_texture(&HUD_texs[tex_index]);
	    strlcpy(hud_texts[tex_index],str,50);
	}
	if (!HUD_texs[tex_index].tex_list)
	    render_text(&gamefont, str, &HUD_texs[tex_index]);
	disp_text(  &HUD_texs[tex_index],hudColorRGBA,LEFT,DOWN
	    	    ,hud_pos_x + hudSize-HUD_OFFSET+BORDER
		    ,hud_pos_y - (hudSize-HUD_OFFSET+BORDER)
		    ,true   );

	if (numItems[ITEM_TANK]) {
	    if (fuelCurrent == 0)
		strcpy(str,"M ");
	    else
		sprintf(str, "T%d", fuelCurrent);

	    tex_index=1;
	    if (strcmp(str,hud_texts[tex_index])!=0) {
    	    	if (HUD_texs[tex_index].tex_list)
		    free_string_texture(&HUD_texs[tex_index]);
    	    	strlcpy(hud_texts[tex_index],str,50);
	    }
	    if (!HUD_texs[tex_index].tex_list)
	    	render_text(&gamefont, str, &HUD_texs[tex_index]);
	    disp_text(  &HUD_texs[tex_index],hudColorRGBA,LEFT,DOWN
	    	    ,hud_pos_x + hudSize-HUD_OFFSET + BORDER
		    ,hud_pos_y - hudSize-HUD_OFFSET + BORDER
		    ,true   );

	}
    }

    /* Update the lock display */
    Paint_lock(hud_pos_x, hud_pos_y);

    /* Draw last score on hud if it is an message attached to it */
    if (hudColorRGBA) {
	for (i = 0, j = 0; i < MAX_SCORE_OBJECTS; i++) {
	    score_object_t*	sobj = &score_objects[(i+score_object)%MAX_SCORE_OBJECTS];
	    if (sobj->hud_msg_len > 0) {
	    	dummy = printsize(&gamefont,"%s",sobj->hud_msg);
		if (sobj->hud_msg_width == -1)
		    sobj->hud_msg_width = (int)dummy.width;
		if (j == 0 &&
		    sobj->hud_msg_width > 2*hudSize-HUD_OFFSET*2 &&
		    (did_fuel || hudVLineColorRGBA))
		    ++j;

		tex_index=MAX_HUD_TEXS+i;
		if (strcmp(sobj->hud_msg,hud_texts[tex_index])!=0) {
    	    	    if (HUD_texs[tex_index].tex_list)
		    	free_string_texture(&HUD_texs[tex_index]);
    	    	    strlcpy(hud_texts[tex_index],sobj->hud_msg,50);
	    	}
	    	if (!HUD_texs[tex_index].tex_list)
	    	    render_text(&gamefont, sobj->hud_msg, &HUD_texs[tex_index]);

		disp_text(  &HUD_texs[tex_index],hudColorRGBA,CENTER,DOWN
	    	    ,hud_pos_x
		    ,hud_pos_y - (hudSize-HUD_OFFSET + BORDER + j * HUD_texs[tex_index].height)
		    ,true   );
		j++;
	    }
	}

	if (time_left > 0) {
	    sprintf(str, "%3d:%02d", (int)(time_left / 60), (int)(time_left % 60));
	    tex_index=3;
	    if (strcmp(str,hud_texts[tex_index])!=0) {
    	    	if (HUD_texs[tex_index].tex_list)
		    free_string_texture(&HUD_texs[tex_index]);
    	    	strlcpy(hud_texts[tex_index],str,50);
	    }
	    if (!HUD_texs[tex_index].tex_list)
	    	render_text(&gamefont, str, &HUD_texs[tex_index]);
	    disp_text(  &HUD_texs[tex_index],hudColorRGBA,RIGHT,DOWN
	    	    ,hud_pos_x - hudSize+HUD_OFFSET - BORDER
		    ,hud_pos_y + hudSize+HUD_OFFSET + BORDER
		    ,true   );
	}

	/* Update the modifiers */
	modlen = strlen(mods);
	tex_index=4;
	if (strcmp(mods,hud_texts[tex_index])!=0) {
    	    if (HUD_texs[tex_index].tex_list)
	    	free_string_texture(&HUD_texs[tex_index]);
    	    strlcpy(hud_texts[tex_index],mods,50);
	}
	if(strlen(mods)) {
	    if (!HUD_texs[tex_index].tex_list)
	    	render_text(&gamefont, mods, &HUD_texs[tex_index]);
	    disp_text(  &HUD_texs[tex_index],hudColorRGBA,RIGHT,UP
		    	,hud_pos_x - hudSize+HUD_OFFSET-BORDER
	    	    	,hud_pos_y - hudSize+HUD_OFFSET-BORDER
	    	    	,true	);    
    	}

	if (autopilotLight) {
	    tex_index=5;
	    if (strcmp(autopilot,hud_texts[tex_index])!=0) {
    	    	if (HUD_texs[tex_index].tex_list)
		    free_string_texture(&HUD_texs[tex_index]);
    	    	strlcpy(hud_texts[tex_index],autopilot,50);
	    }
	    if (!HUD_texs[tex_index].tex_list)
	    	render_text(&gamefont, autopilot, &HUD_texs[tex_index]);
	    disp_text(  &HUD_texs[tex_index],hudColorRGBA,RIGHT,DOWN
	    	    ,hud_pos_x
		    ,hud_pos_y + hudSize+HUD_OFFSET + BORDER + HUD_texs[tex_index].height*2
		    ,true   );
	}
    }

    if (fuelTime > 0.0) {
	fuelTime -= timePerFrame;
	if (fuelTime <= 0.0)
	    fuelTime = 0.0;
    }

    /* draw fuel gauge */
    if (fuelGaugeColorRGBA &&
	((fuelTime > 0.0)
	 || (fuelSum < fuelNotify
	     && ((fuelSum < fuelCritical && (loopsSlow % 4) < 2)
		 || (fuelSum < fuelWarning
		     && fuelSum > fuelCritical
		     && (loopsSlow % 8) < 4)
		 || (fuelSum > fuelWarning))))) {

	set_alphacolor(fuelGaugeColorRGBA);
	tempx = hud_pos_x + hudSize - HUD_OFFSET + FUEL_GAUGE_OFFSET - 1;
	tempy = hud_pos_y - hudSize + HUD_OFFSET + FUEL_GAUGE_OFFSET - 1;
	tempw = HUD_OFFSET - (2*FUEL_GAUGE_OFFSET) + 3;
	temph = HUD_FUEL_GAUGE_SIZE + 3;
	glBegin(GL_LINE_LOOP);
	    glVertex2i(tempx,tempy);
	    glVertex2i(tempx,tempy+temph);
	    glVertex2i(tempx+tempw,tempy+temph);
	    glVertex2i(tempx+tempw,tempy);
	glEnd();

	size = (int)((HUD_FUEL_GAUGE_SIZE * fuelSum) / fuelMax);
	tempx = hud_pos_x + hudSize - HUD_OFFSET + FUEL_GAUGE_OFFSET + 1;
    	tempy = hud_pos_y - hudSize + HUD_OFFSET + FUEL_GAUGE_OFFSET + HUD_FUEL_GAUGE_SIZE - size + 1;
    	tempw = HUD_OFFSET - (2*FUEL_GAUGE_OFFSET);
    	temph = size;
	glBegin(GL_POLYGON);
	    glVertex2i(tempx,tempy);
	    glVertex2i(tempx,tempy+temph);
	    glVertex2i(tempx+tempw,tempy+temph);
	    glVertex2i(tempx+tempw,tempy);
	glEnd();
    }
    glDisable(GL_BLEND);
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
