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
# define vsnprintf _vsnprintf
# ifndef GL_BGR
#  define GL_BGR 0x80E0 /* OpenGL 1.2, for which I did not have headers */
# endif
#endif

#include "xpclient_sdl.h"

#include "text.h"

#define BUFSIZE 1024

float modelview_matrix[16];
int renderstyle;
enum rendertype rendertype;

void pushScreenCoordinateMatrix(void);
void pop_projection_matrix(void);
int next_p2 ( int a );
void print(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, int length, const char *text, bool onHUD);
int FTinit(font_data *font, const char * fontname, int ptsize);

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
    Uint32 saved_flags;
    Uint8  saved_alpha;

    /* Use the surface width and height expanded to powers of 2 */
    w = next_p2(surface->w);
    h = next_p2(surface->h);
    texcoord->MinX = 0.0f;		 /* Min X */
    texcoord->MinY = 0.0f;		 /* Min Y */
    texcoord->MaxX = (GLfloat)surface->w / w;  /* Max X */
    texcoord->MaxY = (GLfloat)surface->h / h;  /* Max Y */

    image = SDL_CreateRGBSurface(
    		    SDL_SWSURFACE,
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

    /* Save the alpha blending attributes */
    saved_flags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
    saved_alpha = surface->format->alpha;
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
    	    SDL_SetAlpha(surface, 0, 0);
    }

    /* Copy the surface into the GL texture image */
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    SDL_BlitSurface(surface, &area, image, &area);

    /* Restore the alpha blending attributes */
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
    	    SDL_SetAlpha(surface, saved_flags, saved_alpha);
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

int FTinit(font_data *font, const char * fontname, int ptsize)
{
    int i;
    SDL_Color white = { 0xFF, 0xFF, 0xFF, 0x00 };
    SDL_Color black = { 0x00, 0x00, 0x00, 0 };
    SDL_Color *forecol;
    SDL_Color *backcol;
    GLenum gl_error;
    texcoord_t texcoords;
    int minx = 0,miny = 0,maxx = 0,maxy = 0;

    /* We might support changing theese later */
    /* Look for special rendering types */
    renderstyle = TTF_STYLE_NORMAL;
    rendertype = RENDER_LATIN1;
    /* Default is black and white */
    forecol = &white;
    backcol = &black;
    
    /* Initialize the TTF library */
    /*if ( TTF_Init() < 0 ) {
    	fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
    	return(2);
    }*/
    font->ttffont = TTF_OpenFont(fontname, ptsize);
    if ( font->ttffont == NULL ) {
    	fprintf(stderr, "Couldn't load %d pt font from %s: %s\n", ptsize, fontname, SDL_GetError());
    	return(2);
    }
    TTF_SetFontStyle(font->ttffont, renderstyle);
    font->list_base=glGenLists(next_p2(NUMCHARS));
    /* Get the recommended spacing between lines of text for this font */
    font->linespacing = TTF_FontLineSkip(font->ttffont);
    font->h = ptsize;

    for( i = 0; i < NUMCHARS; i++ ) {
	SDL_Surface *glyph = NULL;
	GLuint height = 0; /* kps - added default value */

	forecol = &white;
	
    	glyph = TTF_RenderGlyph_Blended( font->ttffont, i, *forecol );
    	if(glyph) {
	    glGetError();
    	    font->textures[i] = SDL_GL_LoadTexture(glyph, &texcoords);
    	    if ( (gl_error = glGetError()) != GL_NO_ERROR )
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
    return 0;
}

int fontinit(font_data *ft_font, const char * fname, unsigned int size)
{
    return FTinit(ft_font,fname,size);
}

void fontclean(font_data *ft_font)
{
    if (ft_font == NULL) return;
    glDeleteLists(ft_font->list_base,next_p2(NUMCHARS));
    if (ft_font->textures != NULL) {
    	glDeleteTextures(NUMCHARS,ft_font->textures);
    }
}

/* A fairly straight forward function that pushes
 * a projection matrix that will make object world 
 * coordinates identical to window coordinates.
 */
void pushScreenCoordinateMatrix(void)
{
	GLint	viewport[4];
	glPushAttrib(GL_TRANSFORM_BIT);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(viewport[0],viewport[2],viewport[1],viewport[3]);
	glPopAttrib();
}

/* Pops the projection matrix without changing the current
 * MatrixMode.
 */
void pop_projection_matrix(void)
{
	glPushAttrib(GL_TRANSFORM_BIT);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}


fontbounds nprintsize(font_data *ft_font, int length, const char *fmt, ...)
{
    int i=0,j,textlength;
    float len;
    fontbounds returnval;
    int start,end,toklen;
	char text[BUFSIZE];  /* Holds Our String */
	va_list ap;
    
    returnval.width=0.0;
    returnval.height=0.0;
    
    if (ft_font == NULL) return returnval;

    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    if (!(textlength = MIN((int)strlen(text),length))) {
    	return returnval;
    }

    start = 0;
    for (;;) {
	
	for (end=start;end<textlength;++end)
	    if (text[end] == '\n') {
	    	break;
	    }
	
	toklen = end - start;
	
	len = 0.0;
	for (j=start;j<=end-1;++j)
	    len = len + ft_font->W[(GLubyte)text[j]];
	
    	if (len > returnval.width)
	    returnval.width = len;	

    	++i;
	
	if (end >= textlength - 1) break;
	
	start = end + 1;
    }
    /* i should be atleast 1 if we get here...*/
    returnval.height = ft_font->h + (i-1)*ft_font->linespacing;
    
    return returnval;
}

fontbounds printsize(font_data *ft_font, const char *fmt, ...)
{
    fontbounds returnval;
    char text[BUFSIZE];  /* Holds Our String */
    va_list ap; 	    /* Pointer To List Of Arguments */
    
    returnval.width=0.0;
    returnval.height=0.0;
    
    if (ft_font == NULL) return returnval;


    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    return nprintsize(ft_font, BUFSIZE, "%s", text);
}

bool render_text(font_data *ft_font, const char *text, string_tex_t *string_tex)
{
    SDL_Color white = { 0xFF, 0xFF, 0xFF, 0x00 };
    SDL_Color *forecol;
    SDL_Surface *string_glyph = NULL;
    SDL_Surface *glyph = NULL;
    SDL_Rect src, dest;
    GLenum gl_error;

    if (!(ft_font)) return false;
    if (!(ft_font->ttffont)) return false;
    if (!(string_tex)) return false;
#if 0
    if (!strlen(text)) return false; /* something is printing an empty string each frame */
#else
    /* kps - fix for empty author field in cannon dodgers */
    if (!strlen(text))
	text = " ";
#endif

    forecol = &white;
	
    string_tex->font_height = ft_font->h;
    
    string_glyph = TTF_RenderText_Blended( ft_font->ttffont, text, *forecol );
    
    string_tex->tex_list = Arraylist_alloc(sizeof(tex_t));
	
    string_tex->width = 0;
    string_tex->height = string_glyph->h;
    
    if (string_glyph) {
    	int i, num = 1 + string_glyph->w / 254;
 	string_tex->text = (char *)malloc(sizeof(char)*(strlen(text)+1));
	sprintf(string_tex->text,"%s",text);
   	for( i=0 ; i<num ; ++i ) {
	    tex_t tex;
	    
	    tex.texture = 0;
	    tex.texcoords.MinX = 0.0;
	    tex.texcoords.MaxX = 0.0;
	    tex.texcoords.MinY = 0.0;
	    tex.texcoords.MaxY = 0.0;
	    tex.width = 0;
	    
    	    src.x   = i*254;
	    dest.x  = 0;
    	    src.y = dest.y = 0;
	    if (i==num-1)
	    	dest.w = src.w = string_glyph->w - i*254;
	    else
	    	dest.w = src.w = 254;
    	    src.h = dest.h = string_glyph->h;
	    
    	    glyph = SDL_CreateRGBSurface(0,dest.w,dest.h,32,0,0,0,0);
    	    SDL_SetColorKey(glyph, SDL_SRCCOLORKEY, 0x00000000);
    	    SDL_BlitSurface(string_glyph,&src,glyph,&dest);
    
  	    glGetError();
	    tex.texture = SDL_GL_LoadTexture(glyph,&(tex.texcoords));
    	    if ( (gl_error = glGetError()) != GL_NO_ERROR )
    	    	printf("Warning: Couldn't create texture: 0x%x\n", gl_error);

    	    tex.width = dest.w;
	    string_tex->width += dest.w;
	    
    	    SDL_FreeSurface(glyph);
	    
	    Arraylist_add(string_tex->tex_list,&tex);
	}
	SDL_FreeSurface(string_glyph);
    } else {
    	printf("TTF_RenderText_Blended failed for [%s]\n",text);
	return false;
    }
    return true;
}

bool draw_text(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, const char *text, bool savetex, string_tex_t *string_tex, bool onHUD)
{
    return draw_text_fraq(ft_font, color, XALIGN, YALIGN, x, y, text, 0.0f, 1.0f, 0.0f, 1.0f, savetex, string_tex, onHUD);
}

bool draw_text_fraq(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, const char *text
    	    	    , float xstart
    	    	    , float xstop
    	    	    , float ystart
    	    	    , float ystop
		    , bool savetex, string_tex_t *string_tex, bool onHUD)
{
    bool remove_tex = false;    	
    if (!(ft_font)) return false;
    if (!(ft_font->ttffont)) return false;
        
    if (!string_tex) {
    	remove_tex = true;
    	string_tex = XMALLOC(string_tex_t, 1);
    }
    
    if (render_text(ft_font,text,string_tex)) {
    	disp_text_fraq(string_tex, color, XALIGN, YALIGN, x, y, xstart, xstop, ystart, ystop, onHUD);
    }
    
    if (!savetex || remove_tex)
    	free_string_texture(string_tex);
    
    return true;
}

void disp_text(string_tex_t *string_tex, int color, int XALIGN, int YALIGN, int x, int y, bool onHUD)
{
    disp_text_fraq(string_tex, color, XALIGN, YALIGN, x, y, 0.0f, 1.0f, 0.0f, 1.0f, onHUD);
}

void disp_text_fraq(string_tex_t *string_tex, int color, int XALIGN, int YALIGN, int x, int y
    	    	    , float xstart
    	    	    , float xstop
    	    	    , float ystart
    	    	    , float ystop
    	    	    , bool onHUD)
{
    int i,num,xpos;
    
    if (!(string_tex)) return;
    set_alphacolor(color);
    
    x -= (int)(string_tex->width/2.0f*XALIGN);
    y += (int)(string_tex->height/2.0f*YALIGN - string_tex->height);
    
    if (onHUD) pushScreenCoordinateMatrix();
    
    xpos=x;
    num = Arraylist_get_num_elements(string_tex->tex_list);
    for (i=0;i<num;++i) {
    	tex_t tex = *((tex_t *)Arraylist_get(string_tex->tex_list,i));
    	
	glBindTexture(GL_TEXTURE_2D, tex.texture);
    	
	glEnable(GL_TEXTURE_2D);
    	glEnable(GL_BLEND);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    	glBegin(GL_TRIANGLE_STRIP);
    	    glTexCoord2f(   xstart*tex.texcoords.MaxX	, ystop*tex.texcoords.MaxY 	    );
	    glVertex2f(     xpos + xstart*tex.width	, y  + ystart*string_tex->height    );

     	    glTexCoord2f(   xstop*tex.texcoords.MaxX	, ystop*tex.texcoords.MaxY 	    );
	    glVertex2f(     xpos + xstop*tex.width 	, y  + ystart*string_tex->height    );

    	    glTexCoord2f(   xstart*tex.texcoords.MaxX	, ystart*tex.texcoords.MaxY	    );
	    glVertex2f(     xpos + xstart*tex.width	, y + ystop*string_tex->height	    ); 

   	    glTexCoord2f(   xstop*tex.texcoords.MaxX	, ystart*tex.texcoords.MaxY	    );
	    glVertex2f(     xpos + xstop*tex.width 	, y + ystop*string_tex->height	    ); 
    	glEnd();
	
    	glDisable(GL_TEXTURE_2D);
	
	xpos += tex.width;
    }
    
    if (onHUD) pop_projection_matrix();
}

void free_string_texture(string_tex_t *string_tex)
{
    if (string_tex) {
    	if (string_tex->tex_list) {
	    int i,num = Arraylist_get_num_elements(string_tex->tex_list);
	    for (i=0;i<num;++i) {
	    	tex_t tex;
	    	tex = *((tex_t *)Arraylist_get(string_tex->tex_list,i));
		glDeleteTextures(1,&(tex.texture));
	    }
	    Arraylist_free(string_tex->tex_list);
	    string_tex->tex_list = NULL;
	}
	XFREE(string_tex->text);
    }
}

void print(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, int length, const char *text, bool onHUD)
{
    int i=0,j,textlength;
    fontbounds returnval,dummy;
    float xoff = 0.0,yoff = 0.0;
    int start,end,toklen;
    int X,Y;
	GLuint font;
    
    returnval.width = 0.0;
    returnval.height = 0.0;

    if (!(textlength = MIN(strlen(text),length))) return;
    
    font=ft_font->list_base;

    returnval = nprintsize(ft_font,length,"%s",text);
    
    yoff = (returnval.height/2.0f)*((float)YALIGN) - ft_font->h;

    glListBase(font);

    if (onHUD) pushScreenCoordinateMatrix();					
    
    glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);	
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

    glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);

    /* This is where the text display actually happens.
     * For each line of text we reset the modelview matrix
     * so that the line's text will start in the correct position.
     * Notice that we need to reset the matrix, rather than just translating
     * down by h. This is because when each character is
     * draw it modifies the current matrix so that the next character
     * will be drawn immediatly after it. 
     */
    /* make sure not to use mytok until we are done!!! */
    start = 0;
    for (;;) {
	
	for (end=start;end<textlength;++end)
	    if (text[end] == '\n') {
	    	break;
	    }
	
	toklen = end - start;
	
	dummy.width = 0.0;
	for (j=start;j<=end-1;++j)
	    dummy.width = dummy.width + ft_font->W[(GLubyte)text[j]];
	
	xoff = - (dummy.width/2.0f)*((float)XALIGN);

    	glPushMatrix();
    	glLoadIdentity();
	
    	X = (int)(x + xoff);
    	Y = (int)(y - ft_font->linespacing*i + yoff);
	
    	if (color) set_alphacolor(color);
	if (onHUD) glTranslatef(X, Y, 0);
	else glTranslatef(X * clData.scale,Y * clData.scale, 0);
    	glMultMatrixf(modelview_matrix);

    	glCallLists(toklen, GL_UNSIGNED_BYTE, (GLubyte *) &text[start]);
		
    	glPopMatrix();
   	
	++i;
	
	if (end >= textlength - 1) break;
	
	start = end + 1;
    }
    glPopAttrib();		

    if (onHUD) pop_projection_matrix();

}

void mapnprint(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, int length, const char *fmt,...)
{
    int textlength;
    
    char		text[BUFSIZE];  /* Holds Our String */
    va_list		ap; 	    /* Pointer To List Of Arguments */
    
    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    if (!(textlength = MIN((int)strlen(text),length))) {
    	return;
    }

    if (ft_font == NULL) return;

    print(ft_font, color, XALIGN, YALIGN, x, y, length, text, false);
}

void HUDnprint(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, int length, const char *fmt, ...)
{
    int textlength;
    
    char		text[BUFSIZE];  /* Holds Our String */
    va_list		ap; 	    /* Pointer To List Of Arguments */
    
    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    if (!(textlength = MIN((int)strlen(text),length))) {
    	return;
    }
    
    if (ft_font == NULL) return;

    print( ft_font, color, XALIGN, YALIGN, x, y, length, text, true);
}

void mapprint(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, const char *fmt,...)
{
    unsigned int textlength;
    
    char		text[BUFSIZE];  /* Holds Our String */
    va_list		ap; 	    /* Pointer To List Of Arguments */
    
    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    if (!(textlength = strlen(text))) {
    	return;
    }

    if (ft_font == NULL) return;

    print(ft_font, color, XALIGN, YALIGN, x, y, BUFSIZE, text, false);
}

void HUDprint(font_data *ft_font, int color, int XALIGN, int YALIGN, int x, int y, const char *fmt, ...)
{
    unsigned int textlength;
    
    char		text[BUFSIZE];  /* Holds Our String */
    va_list		ap; 	    /* Pointer To List Of Arguments */
    
    if (fmt == NULL)	    	    /* If There's No Text */
    	*text=0;    	    	    /* Do Nothing */
    else {
    	va_start(ap, fmt);  	    /* Parses The String For Variables */
    	vsnprintf(text, BUFSIZE, fmt, ap);    /* And Converts Symbols To Actual Numbers */
    	va_end(ap); 	    	    /* Results Are Stored In Text */
    }
    if (!(textlength = strlen(text))) {
    	return;
    }
    
    if (ft_font == NULL) return;

    print( ft_font, color, XALIGN, YALIGN, x, y, BUFSIZE, text, true);
}
