/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Erik Andersson <maximan@users.sourceforge.net>
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
#include "text.h"
#include "glwidgets.h"
#include "scrap.h"

/****************************************************/
/* BEGIN: Main GLWidget stuff	    	    	    */
/****************************************************/

static void option_callback( void *opt, const char *value );
static void confmenu_callback( void );
static void hover_optionWidget( int over, Uint16 x , Uint16 y , void *data );
static void clear_eventTarget( GLWidget *widget );

static char *scrap = NULL;
static GLWidget *scraptarget = NULL;

GLWidget *Init_EmptyBaseGLWidget( void )
{
    GLWidget *tmp = XMALLOC(GLWidget, 1);
    if ( !tmp ) return NULL;
    tmp->WIDGET     	= -1;
    tmp->bounds.x   	= 0;
    tmp->bounds.y   	= 0;
    tmp->bounds.w   	= 0;
    tmp->bounds.h   	= 0;
    tmp->wid_info   	= NULL;
    tmp->Draw	    	= NULL;
    tmp->Close	    	= NULL;
    tmp->SetBounds  	= NULL;
    tmp->button     	= NULL;
    tmp->buttondata 	= NULL;
    tmp->motion     	= NULL;
    tmp->motiondata 	= NULL;
    tmp->hover	    	= NULL;
    tmp->hoverdata  	= NULL;
    tmp->list   	= NULL;
    tmp->children   	= NULL;
    tmp->next	    	= NULL;
    return tmp;
}

static void clear_eventTarget( GLWidget *widget )
{
    int i;
    
    if (!widget) {
    	error("NULL passed to clear_eventTarget!");
    	return;
    }
    
    for (i=0;i<NUM_MOUSE_BUTTONS;++i)
    	if (widget == clicktarget[i]) clicktarget[i]=NULL;
    if (widget == hovertarget) hovertarget=NULL;
    if (widget == scraptarget) scraptarget=NULL;
    
}

/* only supposed to take care of mallocs done on behalf of the
 * appropriate Init_<foo> function
 */
static void Close_WidgetTree ( GLWidget **widget )
{
    if (!widget) return;
    if (!(*widget)) return;
    
    Close_WidgetTree ( &((*widget)->next) );
    Close_WidgetTree ( &((*widget)->children) );
        
    if ((*widget)->Close) (*widget)->Close(*widget);

    clear_eventTarget(*widget);

    if ((*widget)->wid_info) free((*widget)->wid_info);
    free(*widget);
    *widget = NULL;
}

void Close_Widget ( GLWidget **widget )
{
    GLWidget *tmp;

    if (!widget) {
    	error("NULL passed to Close_Widget!");
	return;
    }
    if (!(*widget)) {
    	error("pointer passed to Close_Widget points to NULL !");
	return;
    }
    
    Close_WidgetTree( &((*widget)->children) );

    if ((*widget)->Close) (*widget)->Close(*widget);

    clear_eventTarget(*widget);

    tmp = *widget;
    *widget = (*widget)->next;
    free(tmp);
}

/* IMPORTANT: compound widgets need to edit this function */
void SetBounds_GLWidget( GLWidget *widget, SDL_Rect *b )
{
    if (!widget) {
    	error("NULL widget passed to SetBounds_GLWidget!");
    	return;
    }
    if (!b) {
    	error("NULL bounds passed to SetBounds_GLWidget!");
    	return;
    }
    
    if (widget->SetBounds) widget->SetBounds(widget,b);
    else {
    	widget->bounds.x = b->x;
    	widget->bounds.y = b->y;
    	widget->bounds.w = b->w;
    	widget->bounds.h = b->h;
    }
}

static void hover_optionWidget( int over, Uint16 x , Uint16 y , void *data )
{
    static GLWidget *hoverWidget, *labelWidget;
    xp_option_t *opt;
    static SDL_Rect b;
    const char *help;
    static char line[256];
    int start = 0, end = 0;
    bool eternalLoop = true;
    static Uint32 bgColor = 0x0000ff88;
    
    if (over) {
    	if (!data) {
    	    error("NULL option passed to hover_optionWidget\n");
	    return;
    	}
    
    	opt = (xp_option_t *)data;
    
    	if ((help = Option_get_help(opt))) {
    	    if (!(hoverWidget = Init_ListWidget( x, y, &nullRGBA, &nullRGBA, &nullRGBA, DOWN, LEFT, VERTICAL, false ))) {
	    	error("hover_optionWidget: Failed to create ListWidget\n");
		return;
	    }
	    
    	    if (!(labelWidget = Init_LabelWidget( Option_get_name(opt), &yellowRGBA, &bgColor, CENTER, CENTER ))) {
    	    	error("hover_optionWidget: Failed to create LabelWidget\n");
    	    	Close_Widget(&hoverWidget);
    	    	return;
    	    }
    	    ListWidget_Append( hoverWidget , labelWidget );
	    
	    for ( end=0 ; end < 1000000; ++end) {
	    	if ( help[end] != '\n' && help[end] != '\0') {
		    continue;
		}

    	    	if (start != end ) {
		    strncpy(&line[0],help+start,MIN(255,end-start));
		    line[MIN(255,end-start)]='\0';

    	    	    if (!(labelWidget = Init_LabelWidget( &line[0], &whiteRGBA, &bgColor, CENTER, CENTER ))) {
    	    	    	error("hover_optionWidget: Failed to create LabelWidget\n");
		    	Close_Widget(&hoverWidget);
    	    	    	return;
    	    	    }
    	    	    ListWidget_Append( hoverWidget , labelWidget );
		}
		
    	    	start = end+1;
		if (help[end] == '\0') {
		    eternalLoop = false;
		    break;
		}
	    }
	    
	    if (eternalLoop) error("hover_optionWidget: string parse never ends! (infinite loop prevented)\n");

	    AppendGLWidgetList( &(MainWidget->children), hoverWidget );

   	    b.h = hoverWidget->bounds.h;
    	    b.w = hoverWidget->bounds.w;
    	    b.x = x - hoverWidget->bounds.w - 10;
    	    b.y = y - hoverWidget->bounds.h/2;
	    
    	    SetBounds_GLWidget(hoverWidget,&b);
    	}
    } else {
    	if (!hoverWidget)
	    return;
    	DelGLWidgetListItem( &(MainWidget->children), hoverWidget );
    	Close_Widget(&hoverWidget);
    }
}

static void option_callback( void *tmp, const char *value )
{
    xp_option_t *opt;
    
    if (!(opt = (xp_option_t *)tmp)) {
    	error("Faulty parameter to option_callback: opt is a NULL pointer!");
	return;
    }
    
    switch ( Option_get_type(opt) ) {
    	case xp_bool_option:
    	    Set_bool_option( opt, *(opt->bool_ptr), xp_option_origin_config);
	    return;
    	case xp_int_option:
    	    Set_int_option( opt, *(opt->int_ptr), xp_option_origin_config);
	    return;
    	case xp_double_option:
    	    Set_double_option( opt, *(opt->dbl_ptr), xp_option_origin_config);
	    return;
    	case xp_string_option:
	    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS) {
	    	Set_string_option( opt, value, xp_option_origin_config);
	    	return;
	    }
    	default:
	    return;
    }
}

/* Eventually this will be the only visible initializer I guess */
GLWidget *Init_OptionWidget( xp_option_t *opt, Uint32 *fgcolor, Uint32 *bgcolor )
{
    if (!opt) {
    	error("Faulty parameter to Init_DoubleChooserWidget: opt is a NULL pointer!");
	return NULL;
    }
    
    switch ( Option_get_type(opt) ) {
    	case xp_bool_option:
	    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT)
	    	return Init_BoolChooserWidget(opt->name,opt->bool_ptr,fgcolor,bgcolor,option_callback,opt);
	    break;
    	case xp_int_option:
	    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT)
	    	return Init_IntChooserWidget(opt->name,opt->int_ptr,opt->int_minval,opt->int_maxval,fgcolor,bgcolor,option_callback,opt);
	    break;
    	case xp_double_option:
	    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT)
	    	return Init_DoubleChooserWidget(opt->name,opt->dbl_ptr,opt->dbl_minval,opt->dbl_maxval,fgcolor,bgcolor,option_callback,opt);
	    break;
    	case xp_string_option:
	    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS)
	    	return Init_ColorChooserWidget(opt->name,(Uint32 *)opt->private_data,fgcolor,bgcolor,option_callback,opt);
	    break;
    	default:
	    break;
    }
    return NULL;
}

bool AppendGLWidgetList( GLWidget **list, GLWidget *item )
{
    GLWidget **curr;

    if (!list) {
    	error("No list holder for AppendGLWidgetList %i");
    	return false;
    }
    if (!item) {
    	error("Null item sent to AppendGLWidgetList");
    }

    item->list = list;

    curr = list;
    while (*curr) {
    	/* Just a trivial check if item already is in the list
	 * doesn't check for item's trailers however!
	 * Make sure items aren't added twice!
	 */
	if (*curr == item) {
	    error("AppendGLWidgetList: item is already in the list!");
	    return false;
	}
    	curr = &((*curr)->next);
    }
    *curr = item;

    return true;
}

void PrependGLWidgetList( GLWidget **list, GLWidget *item )
{
    GLWidget **curr;

    if (!list) {
    	error("No list holder for PrependGLWidgetList");
    	return;
    }
    if (!item) {
    	error("Null item sent to PrependGLWidgetList");
    }
    
    item->list = list;

    curr = &item;
    while (*curr) {
    	curr = &((*curr)->next);
    }

    *curr = *list;
    *list = item;
}

bool DelGLWidgetListItem( GLWidget **list, GLWidget *widget )
{
    GLWidget **curr;

    if (!list) {
    	error("No list holder for DelGLWidgetListItem");
    	return false;
    }
    if (!widget) {
    	error("Null widget sent to DelGLWidgetListItem");
	return false;
    }
    
    /* We don't clear widget->list here, because it still 'belongs'
     * to list until we link it somewhere else
     */
    
    curr = list;
    while (*curr) {
    	if (*curr == widget) {
	    *curr = (*curr)->next;
	    widget->next = NULL;
	    return true;
	}
    	curr = &((*curr)->next);
    }
    
    return false;
}

/* 
 * Traverses a widget tree and calls Draw for each widget
 * the order is widget then its children (first to last)
 * then it moves onto the next widget in the list
 */
void DrawGLWidgetsi( GLWidget *list, int x, int y, int w, int h)
{
    int x2,y2,w2,h2;
    GLWidget *curr;
    
    curr = list;
    
    while (curr) {
    	x2 = MAX(x,curr->bounds.x);
    	y2 = MAX(y,curr->bounds.y);
    	w2 = MIN(x+w,curr->bounds.x+curr->bounds.w) - x2;
    	h2 = MIN(y+h,curr->bounds.y+curr->bounds.h) - y2;
	if ( (w2 > 0) && (h2 > 0) ) {
    	    glEnable(GL_BLEND);
    	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glScissor(x2, draw_height - y2 - h2, w2+1, h2+1);
	    if (curr->Draw) curr->Draw(curr);
	    glDisable(GL_BLEND);
	
	    DrawGLWidgetsi(curr->children,x2, y2, w2, h2);
	    glScissor(x, draw_height - y - h, w, h);
	}
	
	curr = curr->next;
    }
}
void DrawGLWidgets( GLWidget *list )
{
    glScissor(0, 0, draw_width, draw_height);
    glEnable(GL_SCISSOR_TEST);
    DrawGLWidgetsi( list , 0, 0, draw_width, draw_height );
    glDisable(GL_SCISSOR_TEST);
}

/*
 * Similar to DrawGLWidgets, but this one needs to traverse the
 * tree in reverse order! (since the things painted last will
 * be seen ontop, thus should get first pick of events
 * So it will descend to the last child in the list's last widget
 * then traverse back trying to find the target.
 */
/*
 * Possibly this function will be hidden, and
 * GLWidget *FindGLWidget( Uint16 x, Uint16 y );
 * visible only. (so that nobody passes a non-visible
 * widget list)
 */
GLWidget *FindGLWidgeti( GLWidget *widget, Uint16 x, Uint16 y )
{
    GLWidget *tmp;
    
    if ( !widget ) return NULL;
    
    if ( (tmp = FindGLWidgeti( widget->next, x, y )) ) {
    	return tmp;
    }
    
    if(     (x >= widget->bounds.x) && (x <= (widget->bounds.x + widget->bounds.w))
    	&&  (y >= widget->bounds.y) && (y <= (widget->bounds.y + widget->bounds.h)) ) {
    	if ( (tmp = FindGLWidgeti( widget->children, x, y )) ) {
    	    return tmp;
    	} else return widget;
    } else {
    	return NULL;
    }
}
GLWidget *FindGLWidget( GLWidget *list, Uint16 x, Uint16 y )
{
    return FindGLWidgeti( list, x, y );
}

void load_textscrap(char *text)
{
    char *cp;
    int   i;
    
    if (!text) return;
    
    scraptarget = NULL;
    scrap = realloc(scrap, strlen(text)+1);
    strcpy(scrap, text);
    for ( cp=scrap, i=0; i<(int)strlen(scrap); ++cp, ++i ) {
    	if ( *cp == '\n' )
    	    *cp = '\r';
    }
    put_scrap(TextScrap('T','E','X','T'), strlen(scrap), scrap);
}
/****************************************************/
/* END: Main GLWidget stuff 	    	    	    */
/****************************************************/

/**********************/
/* Begin:  ArrowWidget*/
/**********************/
static void button_ArrowWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data );
static void Paint_ArrowWidget( GLWidget *widget );

static void button_ArrowWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data )
{
    ArrowWidget *tmp;
    
    if (!data) return;
    tmp = (ArrowWidget *)(((GLWidget *)data)->wid_info);
    if (state == SDL_PRESSED && !(tmp->locked)) {
	if (button == 1) {
    	    tmp->press = true;
	}
	if (button == 2) {
	    tmp->tap = true;
	    if (tmp->action) tmp->action(tmp->actiondata);
	}
    }
    if (state == SDL_RELEASED) {
	if (button == 1) {
    	    tmp->press = false;
	}
    }
}

static void Paint_ArrowWidget( GLWidget *widget )
{
    GLWidget *tmp;
    SDL_Rect *b;
    ArrowWidget *wid_info;
    ArrowWidget_dir_t dir;
    static Uint32 normalcolor  = 0xff0000ff;
    static Uint32 presscolor   = 0x00ff00ff;
    static Uint32 tapcolor     = 0xffffffff;
    static Uint32 lockcolor    = 0x88000088;
    
    if (!widget) return;
    	
    tmp = widget;
    b = &(tmp->bounds);
    wid_info = (ArrowWidget *)(tmp->wid_info);
    
    if (wid_info->locked) {
    	set_alphacolor( lockcolor );
    } else if (wid_info->press) {
    	if (wid_info->action) {
	    wid_info->action(wid_info->actiondata);
	}
	set_alphacolor( presscolor );
    } else if (wid_info->tap) {
    	set_alphacolor( tapcolor );
    	wid_info->tap = false;
    } else {
    	set_alphacolor( normalcolor );
    }
    
    dir = wid_info->direction;
    glBegin(GL_POLYGON);
    switch ( dir ) {
    	case RIGHTARROW:
	    glVertex2i(b->x 	    ,b->y   	);
	    glVertex2i(b->x 	    ,b->y+b->h	);
	    glVertex2i(b->x + b->w  ,b->y+b->h/2);
	    break;
    	case UPARROW:
	    glVertex2i(b->x + b->w/2,b->y   	);
	    glVertex2i(b->x 	    ,b->y+b->h	);
	    glVertex2i(b->x + b->w  ,b->y+b->h	);
	    break;
    	case LEFTARROW:
	    glVertex2i(b->x + b->w  ,b->y   	);
	    glVertex2i(b->x 	    ,b->y+b->h/2);
	    glVertex2i(b->x + b->w  ,b->y+b->h	);
	    break;
    	case DOWNARROW:
	    glVertex2i(b->x 	    ,b->y   	);
	    glVertex2i(b->x + b->w/2,b->y+b->h	);
	    glVertex2i(b->x + b->w  ,b->y   	);
	    break;
	default:
	    error("Weird direction for ArrowWidget! (direction:%i)\n",dir);
    }
    glEnd();
}

GLWidget *Init_ArrowWidget( ArrowWidget_dir_t direction,int width, int height,
    	     void (*action)( void *data), void *actiondata )
{
    ArrowWidget *wid_info;
    GLWidget	*tmp	= Init_EmptyBaseGLWidget();
    
    if ( !tmp ) {
        error("Failed to malloc in Init_ArrowWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ArrowWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ArrowWidget");
	return NULL;
    }
    wid_info = (ArrowWidget *)tmp->wid_info;
    
    tmp->WIDGET     	= ARROWWIDGET;
    wid_info->direction  = direction;
    tmp->bounds.w   	= width;
    tmp->bounds.h   	= height;
    wid_info->press 	= false;
    wid_info->tap   	= false;
    wid_info->locked	= false;
    wid_info->action	= action;
    wid_info->actiondata= actiondata;
    tmp->Draw	    	= Paint_ArrowWidget;
    tmp->button     	= button_ArrowWidget;
    tmp->buttondata 	= tmp;
    
    return tmp;
}

/********************/
/* End:  ArrowWidget*/
/********************/

/**********************/
/* Begin:  ButtonWidget*/
/**********************/
static void button_ButtonWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data );
static void Paint_ButtonWidget( GLWidget *widget );

static void button_ButtonWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data )
{
    ButtonWidget *tmp;
    
    if (!data) return;
    tmp = (ButtonWidget *)(((GLWidget *)data)->wid_info);
    if (state == SDL_PRESSED) {
    	if (tmp->pressed) return;
	if (button == 1) {
    	    tmp->pressed = true;
	    tmp->press_time = loopsSlow;
	    if (tmp->action) tmp->action(tmp->actiondata);
	}
    }
}

static void Paint_ButtonWidget( GLWidget *widget )
{
    ButtonWidget *wid_info;
    int color;
    
    if (!widget) return;	
    if (!(wid_info = (ButtonWidget *)(widget->wid_info))) return;
    
    if (wid_info->pressed) {
    	if (wid_info->pressed_color)
	    color = *(wid_info->pressed_color);
	else color = redRGBA;
	if (loopsSlow >= (int)(wid_info->press_time + wid_info->depress_time))
	    wid_info->pressed = false;
    } else {
    	if (wid_info->normal_color)
	    color = *(wid_info->normal_color);
	else color = greenRGBA;
    }
    
    set_alphacolor(color);
    glBegin(GL_QUADS);
    	glVertex2i( widget->bounds.x 	    	    	, widget->bounds.y	    	    	);
    	glVertex2i( widget->bounds.x	    	    	, widget->bounds.y+widget->bounds.h	);
    	glVertex2i( widget->bounds.x+widget->bounds.w	, widget->bounds.y+widget->bounds.h	);
    	glVertex2i( widget->bounds.x+widget->bounds.w	, widget->bounds.y	    	    	);
    glEnd();
}

GLWidget *Init_ButtonWidget( Uint32 *normal_color, Uint32 *pressed_color, Uint8 depress_time, void (*action)(void *data), void *actiondata)
{
    ButtonWidget    *wid_info;
    GLWidget	    *tmp    = Init_EmptyBaseGLWidget();
   
    if ( !tmp ) {
        error("Failed to malloc in Init_ButtonWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ButtonWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ButtonWidget");
	return NULL;
    }
    wid_info = (ButtonWidget *)tmp->wid_info;
    
    tmp->WIDGET     	= BUTTONWIDGET;
    wid_info->pressed	    = false;
    wid_info->normal_color  = normal_color;
    wid_info->pressed_color = pressed_color;
    wid_info->depress_time  = depress_time;
    wid_info->action	    = action;
    wid_info->actiondata    = actiondata;
    tmp->Draw	    	= Paint_ButtonWidget;
    tmp->button     	= button_ButtonWidget;
    tmp->buttondata 	= tmp;
    
    return tmp;
}

/********************/
/* End:  ButtonWidget*/
/********************/

/**********************/
/* Begin: SlideWidget*/
/**********************/
static void button_SlideWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data );
static void Paint_SlideWidget( GLWidget *widget );

static void button_SlideWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data )
{
    SlideWidget *tmp;
    
    if (!data) return;

    tmp = (SlideWidget *)(((GLWidget *)data)->wid_info);
    if (state == SDL_PRESSED && !(tmp->sliding)) {
	if (button == 1) {
    	    tmp->sliding = true;
	}
    }
    if (state == SDL_RELEASED) {
	if (button == 1) {
    	    tmp->sliding = false;
	    if (tmp->release) tmp->release(tmp->releasedata);
	}
    }
}

static void Paint_SlideWidget( GLWidget *widget )
{
    GLWidget *tmp;
    SDL_Rect *b;
    SlideWidget *wid_info;

   
    static Uint32 normalcolor	= 0xff0000ff;
    static Uint32 presscolor	= 0x00ff00ff;
    static Uint32 lockcolor 	= 0x333333ff;
    Uint32 color;

    if (!widget) return;
    tmp = widget;
    b = &(tmp->bounds);
    wid_info = (SlideWidget *)(tmp->wid_info);
    
    if (wid_info->locked) {
    	color = lockcolor;
    } else if (wid_info->sliding) {
    	color = presscolor;
    } else {
    	color = normalcolor;
    }

    glBegin(GL_QUADS);
    	set_alphacolor(color	    	    	);
	glVertex2i(b->x     	, b->y	    	);
    	set_alphacolor(color	    	    	);
    	glVertex2i(b->x + b->w	, b->y	    	);
    	set_alphacolor(color & 0xffffff77   	);
    	glVertex2i(b->x + b->w	, b->y + b->h	);
    	set_alphacolor(color & 0xffffff77   	);
    	glVertex2i(b->x     	, b->y + b->h	);
    glEnd();
}

GLWidget *Init_SlideWidget( bool locked,
    	     void (*motion)( Sint16 xrel, Sint16 yrel, Uint16 x, Uint16 y, void *data ), void *motiondata,
	     void (*release)(void *releasedata),void *releasedata )
{
    SlideWidget *wid_info;
    GLWidget *tmp	= Init_EmptyBaseGLWidget();
    
    if ( !tmp ) {
        error("Failed to malloc in Init_SlideWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(SlideWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_SlideWidget");
	return NULL;
    }
    wid_info = (SlideWidget *)tmp->wid_info;
    
    tmp->WIDGET     	= SLIDEWIDGET;
    tmp->bounds.x   	= 0;
    tmp->bounds.y   	= 0;
    tmp->bounds.w   	= 10;
    tmp->bounds.h   	= 10;
    wid_info->sliding	    = false;
    wid_info->locked	    = locked;
    wid_info->release	    = release;
    wid_info->releasedata   = releasedata;
    tmp->Draw	    	= Paint_SlideWidget;
    tmp->button     	= button_SlideWidget;
    tmp->buttondata 	= tmp;
    tmp->motion     	= motion;
    tmp->motiondata 	= motiondata;
    
    return tmp;
}

/********************/
/* End: SlideWidget*/
/********************/

/*************************/
/* Begin: ScrollbarWidget*/
/*************************/
static void motion_ScrollbarWidget( Sint16 xrel, Sint16 yrel, Uint16 x, Uint16 y, void *data );
static void release_ScrollbarWidget( void *releasedata );
static void Paint_ScrollbarWidget( GLWidget *widget );
static void SetBounds_ScrollbarWidget( GLWidget *widget, SDL_Rect *b );
static void Close_ScrollbarWidget ( GLWidget *widget );

static void Close_ScrollbarWidget ( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=SCROLLBARWIDGET) {
    	error("Wrong widget type for Close_ScrollbarWidget [%i]",widget->WIDGET);
	return;
    }
}

static void SetBounds_ScrollbarWidget( GLWidget *widget, SDL_Rect *b )
{
    ScrollbarWidget *tmp;
    SDL_Rect sb;

    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=SCROLLBARWIDGET) {
    	error("Wrong widget type for SetBounds_ScrollbarWidget [%i]",widget->WIDGET);
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    tmp = (ScrollbarWidget *)(widget->wid_info);

    switch (tmp->dir) {
    	case SB_VERTICAL:
    	    sb.x = b->x;
    	    sb.w = b->w;
	    sb.y = b->y + (int)(b->h*tmp->pos);
	    sb.h = (int)(b->h*tmp->size);
	    break;
	case SB_HORISONTAL:
    	    sb.y = b->y;
    	    sb.h = b->h;
	    sb.x = b->x + (int)(b->w*tmp->pos);
	    sb.w = (int)(b->w*tmp->size);
	    break; 
    	default :
	    error("bad direction for Scrollbar in SetBounds_ScrollbarWidget!");
	    return;
    }
    	    
    SetBounds_GLWidget(tmp->slide,&sb);
}

static void Paint_ScrollbarWidget( GLWidget *widget )
{
    static Uint32 bgcolor  = 0x00000044;
    SDL_Rect *b = &(widget->bounds);
    
    set_alphacolor( bgcolor );
    
    glBegin(GL_QUADS);
    	glVertex2i(b->x     	, b->y);
    	glVertex2i(b->x + b->w	, b->y);
    	glVertex2i(b->x + b->w	, b->y + b->h);
    	glVertex2i(b->x     	, b->y + b->h);
    glEnd();
}

static void motion_ScrollbarWidget( Sint16 xrel, Sint16 yrel, Uint16 x, Uint16 y, void *data )
{
    GLWidget *tmp;
    ScrollbarWidget *wid_info;
    GLWidget *slide;
    Sint16 *coord1, coord2 = 0, min, max, size, move;
    GLfloat oldpos;

    if (!data) return;

    tmp = (GLWidget *)data;
    wid_info = (ScrollbarWidget *)tmp->wid_info;
    slide = wid_info->slide;
    
   
    switch (wid_info->dir) {
    	case SB_VERTICAL:
	    coord1 = &(slide->bounds.y);
	    min = tmp->bounds.y;
	    size = slide->bounds.h;
	    max = min + tmp->bounds.h;
	    move = yrel;
	    break;
	case SB_HORISONTAL:
	    coord1 = &(slide->bounds.x);
	    min = tmp->bounds.x;
	    size = slide->bounds.w;
	    max = min + tmp->bounds.w;
	    move = xrel;
	    break;
    	default :
	    error("bad direction for Scrollbar in motion_ScrollbarWidget!");
	    return;
    }
    
    wid_info->oldmoves += move;
    
    if (!(wid_info->oldmoves)) return;
    
    if (wid_info->oldmoves > 0) {
    	coord2 = MIN(max-size,*coord1+wid_info->oldmoves);
    } else if (wid_info->oldmoves < 0) {
    	coord2 = MAX(min,*coord1+wid_info->oldmoves);
    }
    wid_info->oldmoves -= coord2 - *coord1;
    *coord1 = coord2;

    oldpos = wid_info->pos;
    wid_info->pos = ((GLfloat)(*coord1 - min))/((GLfloat)(max - min));
    
    if ( (oldpos != wid_info->pos) && wid_info->poschange )
    	wid_info->poschange(wid_info->pos,wid_info->poschangedata);
}

static void release_ScrollbarWidget( void *releasedata )
{
    GLWidget *tmp;
    ScrollbarWidget *wid_info;
    if (!releasedata) return;

    tmp = (GLWidget *)releasedata;
    wid_info = (ScrollbarWidget *)tmp->wid_info;
    
    wid_info->oldmoves = 0;
}

void ScrollbarWidget_SetSlideSize( GLWidget *widget, GLfloat size )
{
    ScrollbarWidget *sb;
    
    if (!widget) return;
    if (widget->WIDGET !=SCROLLBARWIDGET) {
    	error("Wrong widget type for SetBounds_ScrollbarWidget [%i]",widget->WIDGET);
	return;
    }
    if (!(sb = (ScrollbarWidget *)(widget->wid_info))) {
    	error("ScrollbarWidget_SetSlideSize: wid_info missing!");
	return;
    }
    
    sb->size = MIN(1.0f,MAX(0.0f,size));
    
    SetBounds_ScrollbarWidget(widget,&(widget->bounds));
}

GLWidget *Init_ScrollbarWidget( bool locked, GLfloat pos, GLfloat size, ScrollWidget_dir_t dir,
    	    	    	    	void (*poschange)( GLfloat pos , void *poschangedata),
				void *poschangedata )
{
    ScrollbarWidget *wid_info;
    GLWidget *tmp	= Init_EmptyBaseGLWidget();
    
    if ( !tmp ) {
        error("Failed to malloc in Init_ScrollbarWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ScrollbarWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ScrollbarWidget");
	return NULL;
    }
    wid_info = (ScrollbarWidget *)tmp->wid_info;
    
    tmp->WIDGET     	= SCROLLBARWIDGET;
    tmp->bounds.w   	= 10;
    tmp->bounds.h   	= 10;
    tmp->Draw	    	= Paint_ScrollbarWidget;
    tmp->Close	    	= Close_ScrollbarWidget;
    tmp->SetBounds  	= SetBounds_ScrollbarWidget;
    /*add pgUp, pgDown here later with button*/
    wid_info->pos   	= MAX(0.0f,MIN(1.0f,pos));
    wid_info->size  	= MAX(0.0f,MIN(1.0f,size));
    wid_info->dir   	= dir;
    wid_info->oldmoves	= 0;
    wid_info->poschange = poschange;
    wid_info->poschangedata = poschangedata;
    wid_info->slide 	= Init_SlideWidget(locked,motion_ScrollbarWidget, tmp, release_ScrollbarWidget, tmp);
    
    if ( !(((ScrollbarWidget *)tmp->wid_info)->slide) ) {
    	error("Failed to make a SlideWidget for Init_ScrollbarWidget");
	free(tmp->wid_info);
	free(tmp);
	return NULL;
    }
    
    AppendGLWidgetList(&(tmp->children), ((ScrollbarWidget *)tmp->wid_info)->slide);
    return tmp;
}
/*************************/
/*   End: ScrollbarWidget*/
/*************************/

/***********************/
/* Begin:  LabelWidget*/
/***********************/
static void Paint_LabelWidget( GLWidget *widget );
static void Close_LabelWidget ( GLWidget *widget );

static void button_LabelWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data )
{
    LabelWidget *tmp;
    
    if (!data) return;
    tmp = (LabelWidget *)(((GLWidget *)data)->wid_info);
    if (state == SDL_PRESSED) {
	if (button == 1) {
	    if ((tmp->tex).text) {
	    	load_textscrap((tmp->tex).text);
		scraptarget = (GLWidget *)data;
	    }
	}
    }
}

static void Close_LabelWidget( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=LABELWIDGET) {
    	error("Wrong widget type for Close_LabelWidget [%i]",widget->WIDGET);
	return;
    }
    free_string_texture(&(((LabelWidget *)widget->wid_info)->tex));
}

bool LabelWidget_SetColor( GLWidget *widget , Uint32 *fgcolor, Uint32 *bgcolor )
{
    LabelWidget *wi;
    
    if (!widget) return false;
    if (widget->WIDGET !=LABELWIDGET) {
    	error("Wrong widget type for LabelWidget_SetColor [%i]",widget->WIDGET);
	return false;
    }
    
    if ( !(wi = (LabelWidget *)widget->wid_info) ) {
    	error("LabelWidget_SetColor: widget->wid_info missing!");
	return false;
    }
    wi->bgcolor = bgcolor;
    wi->fgcolor = fgcolor;
    
    return true;
}
static void Paint_LabelWidget( GLWidget *widget )
{
    GLWidget *tmp;
    SDL_Rect *b;
    LabelWidget *wid_info;
    int x, y, alpha;
    Uint32 color;
    static int flasher = 0;

    if (!widget) return;
     
    tmp = widget;
    b = &(tmp->bounds);
    wid_info = (LabelWidget *)(tmp->wid_info);
    
    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	set_alphacolor(*(wid_info->bgcolor));
    
    	glBegin(GL_QUADS);
    	    glVertex2i(b->x     	,b->y);
   	    glVertex2i(b->x + b->w  ,b->y);
    	    glVertex2i(b->x + b->w  ,b->y+b->h);
    	    glVertex2i(b->x     	,b->y+b->h);
     	glEnd();
    }
    
    x = wid_info->align == LEFT   ? tmp->bounds.x :
	wid_info->align == CENTER ? tmp->bounds.x + tmp->bounds.w / 2 :
	tmp->bounds.x + tmp->bounds.w;
    y = wid_info->valign == DOWN   ? tmp->bounds.y :
	wid_info->valign == CENTER ? tmp->bounds.y + tmp->bounds.h / 2 :
	tmp->bounds.y + tmp->bounds.h;

    
    if ( wid_info->fgcolor )
    	color = *(wid_info->fgcolor);
    else
    	color = whiteRGBA;
	
    if (scraptarget == tmp) {
	alpha = MAX(0,MIN(255,(color & 255) + tsin(flasher)*64));
	flasher += TABLE_SIZE/clientFPS;
    	if (flasher >= TABLE_SIZE) flasher -= TABLE_SIZE;
	
	color = (color&0xFFFFFF00) + alpha;
    }
	
    disp_text(&(wid_info->tex), 
    	    	color, 
    	    	wid_info->align, 
    	    	wid_info->valign, 
    	    	x, 
    	    	draw_height - y, 
    	    	true);
}

GLWidget *Init_LabelWidget( const char *text , Uint32 *fgcolor, Uint32 *bgcolor, int align, int valign  )
{
    GLWidget *tmp;
    LabelWidget *wid_info;

    if (!text) {
    	error("text missing for Init_LabelWidget.");
	return NULL;
    }
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_LabelWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(LabelWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_LabelWidget");
	return NULL;
    }
    wid_info = (LabelWidget *)tmp->wid_info;

    if ( !render_text(&gamefont, text, &(((LabelWidget *)tmp->wid_info)->tex)) ) {
    	free(tmp->wid_info);
    	free(tmp);
        error("Failed to render text in Init_LabelWidget");
	return NULL;
    } 
    
    tmp->WIDGET     	= LABELWIDGET;
    tmp->bounds.w   	= (((LabelWidget *)tmp->wid_info)->tex).width;
    tmp->bounds.h   	= (((LabelWidget *)tmp->wid_info)->tex).height;
    wid_info->fgcolor	= fgcolor;
    wid_info->bgcolor	= bgcolor;
    wid_info->align 	= align;
    wid_info->valign	= valign;
    tmp->Draw	    	= Paint_LabelWidget;
    tmp->Close     	= Close_LabelWidget;
    tmp->button     	= button_LabelWidget;
    tmp->buttondata 	= tmp; 
    
    return tmp;
}
/********************/
/* End:  LabelWidget*/
/********************/

/***********************************/
/* Begin:  LabeledRadiobuttonWidget*/
/***********************************/
static void button_LabeledRadiobuttonWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data );
static void Paint_LabeledRadiobuttonWidget( GLWidget *widget );

static void button_LabeledRadiobuttonWidget( Uint8 button, Uint8 state, Uint16 x, Uint16 y, void *data )
{
    LabeledRadiobuttonWidget *tmp;
    if (!data) return;
    tmp = (LabeledRadiobuttonWidget *)(((GLWidget *)data)->wid_info);
    if (state == SDL_PRESSED) {
	if (button == 1) {
	    /* Toggle state, and call (*action)*/
	    tmp->state = !(tmp->state);
	    if (tmp->action)  {
		tmp->action(tmp->state,tmp->actiondata);
	    }
	}
    }
}

static void Paint_LabeledRadiobuttonWidget( GLWidget *widget )
{
    GLWidget *tmp;
    SDL_Rect *b;
    LabeledRadiobuttonWidget *wid_info;
    static Uint32 false_bg_color	= 0x00000044;
    static Uint32 true_bg_color	    	= 0x00000044;
    static Uint32 false_text_color	= 0xff0000ff;
    static Uint32 true_text_color	= 0x00ff00ff;

    if (!widget) return;
     
    tmp = widget;
    b = &(tmp->bounds);
    wid_info = (LabeledRadiobuttonWidget *)(tmp->wid_info);
    
    if (wid_info->state)
    	set_alphacolor(true_bg_color);
    else
    	set_alphacolor(false_bg_color);
    
    glBegin(GL_QUADS);
    	glVertex2i(b->x     	,b->y);
   	glVertex2i(b->x + b->w  ,b->y);
    	glVertex2i(b->x + b->w  ,b->y+b->h);
    	glVertex2i(b->x     	,b->y+b->h);
     glEnd();
    
    if (wid_info->state) {
    	disp_text(wid_info->ontex, true_text_color, CENTER, CENTER,tmp->bounds.x+tmp->bounds.w/2, draw_height - tmp->bounds.y-tmp->bounds.h/2, true);
    } else {
    	disp_text(wid_info->offtex, false_text_color, CENTER, CENTER, tmp->bounds.x+tmp->bounds.w/2, draw_height - tmp->bounds.y-tmp->bounds.h/2, true);
    }
}

GLWidget *Init_LabeledRadiobuttonWidget( string_tex_t *ontex, string_tex_t *offtex,
    	    	    	    	    	void (*action)(bool state, void *actiondata),
					void *actiondata, bool start_state )
{
    GLWidget	    	    	*tmp;
    LabeledRadiobuttonWidget	*wid_info;

    if (!ontex || !(ontex->tex_list) || !offtex || !(offtex->tex_list) ) {
    	error("texure(s) missing for Init_LabeledRadiobuttonWidget.");
	return NULL;
    }
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_LabeledRadiobuttonWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(LabeledRadiobuttonWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_LabeledRadiobuttonWidget");
	return NULL;
    }
    wid_info = (LabeledRadiobuttonWidget *)tmp->wid_info;
    
    tmp->WIDGET     	= LABELEDRADIOBUTTONWIDGET;
    tmp->bounds.w   	= MAX(ontex->width,offtex->width)+5;
    tmp->bounds.h   	= MAX(ontex->height,offtex->height);
    wid_info->state 	= start_state;
    wid_info->ontex 	= ontex;
    wid_info->offtex	= offtex;
    wid_info->action	= action;
    wid_info->actiondata= actiondata;
    tmp->Draw	    	= Paint_LabeledRadiobuttonWidget;
    tmp->button     	= button_LabeledRadiobuttonWidget;
    tmp->buttondata 	= tmp;
    
    return tmp;
}
/*********************************/
/* End:  LabeledRadiobuttonWidget*/
/*********************************/

/*****************************/
/* Begin:  BoolChooserWidget */
/*****************************/
static void Paint_BoolChooserWidget( GLWidget *widget );
static void BoolChooserWidget_SetValue( bool state, void *data );
static void Close_BoolChooserWidget ( GLWidget *widget );
static void SetBounds_BoolChooserWidget( GLWidget *widget, SDL_Rect *b );

static int num_BoolChooserWidget = 0;
static string_tex_t *BoolChooserWidget_ontex = NULL;
static string_tex_t *BoolChooserWidget_offtex = NULL;

static void Close_BoolChooserWidget( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=BOOLCHOOSERWIDGET) {
    	error("Wrong widget type for Close_BoolChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    --num_BoolChooserWidget;
    if (!num_BoolChooserWidget) {
    	free_string_texture(BoolChooserWidget_ontex);
	free(BoolChooserWidget_ontex);
	BoolChooserWidget_ontex = NULL;
    	free_string_texture(BoolChooserWidget_offtex);
	free(BoolChooserWidget_offtex);
	BoolChooserWidget_offtex = NULL;
    }
}

static void SetBounds_BoolChooserWidget( GLWidget *widget, SDL_Rect *b )
{
    GLWidget *tmp;
    SDL_Rect b2;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=BOOLCHOOSERWIDGET) {
    	error("Wrong widget type for SetBounds_BoolChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    tmp = ((BoolChooserWidget *)(widget->wid_info))->buttonwidget;
    
    b2.h = tmp->bounds.h;
    b2.w = tmp->bounds.w;
    b2.x = widget->bounds.x + widget->bounds.w - 2 - tmp->bounds.w;
    b2.y = widget->bounds.y + 1;
	    
    SetBounds_GLWidget(tmp,&b2);

    tmp = ((BoolChooserWidget *)(widget->wid_info))->name;
    
    b2.h = widget->bounds.h;
    b2.w = tmp->bounds.w;
    b2.x = widget->bounds.x + 2;
    b2.y = widget->bounds.y;
	    
    SetBounds_GLWidget(tmp,&b2);
}

static void BoolChooserWidget_SetValue( bool state, void *data )
{
    GLWidget *wid;
    BoolChooserWidget *wi;
    
    if ( !(wid = (GLWidget *)data) ) {
    	error("BoolChooserWidget_SetValue: data missing!");
	return;
    }
    
    if ( wid->WIDGET != BOOLCHOOSERWIDGET ) {
    	error("BoolChooserWidget_SetValue: wrong type of widget!");
	return;
    }
    
    if ( !(wi = (BoolChooserWidget *)(wid->wid_info)) ) {
    	error("BoolChooserWidget_SetValue: wid_info missing!");
	return;
    }
    
    *(wi->value) = state;
    
    if (wi->callback)
    	wi->callback( wi->data, NULL );
}

static void Paint_BoolChooserWidget( GLWidget *widget )
{
    /*static int name_color   = 0xffff66ff;*/
    BoolChooserWidget *wid_info;

    if (!widget) {
    	error("Paint_BoolChooserWidget: widget missing!");
	return;
    }
    
    if ( widget->WIDGET != BOOLCHOOSERWIDGET ) {
    	error("Paint_BoolChooserWidget: wrong type of widget!");
	return;
    }
    
    if ( !(wid_info = (BoolChooserWidget *)(widget->wid_info)) ) {
    	error("Paint_BoolChooserWidget: wid_info missing!");
	return;
    }

    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	set_alphacolor( *(wid_info->bgcolor) );
    	glBegin(GL_QUADS);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y+widget->bounds.h	);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    	glEnd();
    }
}

GLWidget *Init_BoolChooserWidget( const char *name, bool *value, Uint32 *fgcolor, Uint32 *bgcolor,
    	    	    	    	 void (*callback)(void *tmp, const char *value), void *data )
{
    GLWidget *tmp;
    BoolChooserWidget *wid_info;
    
    if (!value) {
    	error("Faulty parameter to Init_BoolChooserWidget: value is a NULL pointer!");
	return NULL;
    }
    if (!name || !strlen(name) ) {
    	error("name misssing for Init_BoolChooserWidget.");
	return NULL;
    }

    
    if (!BoolChooserWidget_ontex) {
    	if ((BoolChooserWidget_ontex = XMALLOC(string_tex_t, 1))) {
	    if (!(BoolChooserWidget_offtex = XMALLOC(string_tex_t, 1))) {
	    	XFREE(BoolChooserWidget_ontex);
	    	error("Failed to malloc BoolChooserWidget_offtex in Init_BoolChooserWidget");
	    	return NULL;
	    }
	} else {
	    error("Failed to malloc BoolChooserWidget_ontex in Init_BoolChooserWidget");
	    return NULL;
	}
	if (render_text(&gamefont,"True",BoolChooserWidget_ontex)) {
    	    if (!render_text(&gamefont,"False",BoolChooserWidget_offtex)) {
	    	error("Failed to render 'False' in Init_BoolChooserWidget");
		free_string_texture(BoolChooserWidget_ontex);
		XFREE(BoolChooserWidget_ontex);
		XFREE(BoolChooserWidget_offtex);
		return NULL;
	    }
    	} else {
	    error("Failed to render 'True' in Init_BoolChooserWidget");
    	    XFREE(BoolChooserWidget_ontex);
    	    XFREE(BoolChooserWidget_offtex);
    	    return NULL;
	}
    }
    
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc tmp in Init_BoolChooserWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(BoolChooserWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc tmp->wid_info in Init_BoolChooserWidget");
	return NULL;
    }
    
    wid_info = (BoolChooserWidget *)(tmp->wid_info);
    
    if ( !(wid_info->name = Init_LabelWidget(name,fgcolor,&nullRGBA,LEFT,CENTER)) ) {
    	error("Failed to make a LabelWidget for Init_BoolChooserWidget");
	Close_Widget(&tmp);
	return NULL;
    }
    AppendGLWidgetList(&(tmp->children),wid_info->name);
    
    wid_info->name->hover   	= hover_optionWidget;
    wid_info->name->hoverdata	= data;
    
    if ( !(wid_info->buttonwidget = Init_LabeledRadiobuttonWidget(BoolChooserWidget_ontex,
    	    	    	    	    	BoolChooserWidget_offtex, BoolChooserWidget_SetValue,
					tmp, *(value))) ) {
    	error("Failed to make a LabeledRadiobuttonWidget for Init_BoolChooserWidget");
	Close_Widget(&tmp);
    	return NULL;
    }
    AppendGLWidgetList(&(tmp->children),wid_info->buttonwidget);
        
    tmp->WIDGET     	= BOOLCHOOSERWIDGET;
    tmp->bounds.w   	= 2 + wid_info->name->bounds.w+5+wid_info->buttonwidget->bounds.w + 2;
    tmp->bounds.h   	= 1 + MAX( wid_info->name->bounds.h,wid_info->buttonwidget->bounds.h) + 1 ;
    
    wid_info->value 	= value;
    wid_info->fgcolor 	= fgcolor;
    wid_info->bgcolor 	= bgcolor;
    wid_info->callback 	= callback;
    wid_info->data 	= data;

    tmp->Draw	    	= Paint_BoolChooserWidget;
    tmp->Close  	= Close_BoolChooserWidget;
    tmp->SetBounds  	= SetBounds_BoolChooserWidget;

    ++num_BoolChooserWidget;
    return tmp;
}
/***************************/
/* End:  BoolChooserWidget */
/***************************/

/***************************/
/* Begin: IntChooserWidget */
/***************************/
static void IntChooserWidget_Add( void *data );
static void IntChooserWidget_Subtract( void *data );
static void Paint_IntChooserWidget( GLWidget *widget );
static void Close_IntChooserWidget ( GLWidget *widget );
static void SetBounds_IntChooserWidget( GLWidget *widget, SDL_Rect *b );

static void Close_IntChooserWidget ( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=INTCHOOSERWIDGET) {
    	error("Wrong widget type for Close_IntChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    free_string_texture( &(((IntChooserWidget *)widget->wid_info)->valuetex) );
}

static void SetBounds_IntChooserWidget( GLWidget *widget, SDL_Rect *b )
{
    IntChooserWidget *tmp;
    GLWidget *tmp2;
    SDL_Rect rab,lab,b2;

    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=INTCHOOSERWIDGET) {
    	error("Wrong widget type for SetBounds_IntChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    tmp = (IntChooserWidget *)(widget->wid_info);
    
    lab.h = rab.h = tmp->valuetex.height-4;
    lab.w = rab.w = rab.h;
    lab.y = rab.y = widget->bounds.y + (widget->bounds.h-rab.h)/2;
    rab.x = widget->bounds.x + widget->bounds.w - rab.w -2/*>_|*/;
    lab.x = rab.x - tmp->valuespace/*_value*/ -2/*<_value_>*/ - lab.w;
	    
    SetBounds_GLWidget(tmp->leftarrow,&lab);
    SetBounds_GLWidget(tmp->rightarrow,&rab);

    tmp2 = ((IntChooserWidget *)(widget->wid_info))->name;
    
    b2.h = widget->bounds.h;
    b2.w = tmp2->bounds.w;
    b2.x = widget->bounds.x + 2;
    b2.y = widget->bounds.y;
	    
    SetBounds_GLWidget(tmp2,&b2);
}

static void IntChooserWidget_Add( void *data )
{
    IntChooserWidget *tmp;
    char valuetext[16];
    int step;
    
    if (!data) return;
    tmp = ((IntChooserWidget *)((GLWidget *)data)->wid_info);

    if (tmp->direction > 0) {
     	step = (++tmp->duration)*(tmp->maxval-tmp->minval)/(MAX(1,MIN(maxFPS,FPS))*3);
    } else {
    	step = 1;
    }

    if (*(tmp->value) > tmp->maxval) {
    	((ArrowWidget *)tmp->rightarrow->wid_info)->locked = true;
    } else if (step) {
    	tmp->duration = 0;
    	*(tmp->value) = MIN( *(tmp->value) + step, tmp->maxval);
    	
	if (tmp->callback) tmp->callback( tmp->data, NULL );
	
    	if ( (*(tmp->value)) > tmp->minval)
	    ((ArrowWidget *)tmp->leftarrow->wid_info)->locked = false;
    	if ( (*(tmp->value)) == tmp->maxval)
	    ((ArrowWidget *)tmp->rightarrow->wid_info)->locked = true;
	tmp->direction = 2;
	snprintf(valuetext,15,"%i",*(tmp->value));
	free_string_texture(&(tmp->valuetex));
	if(!render_text(&gamefont,valuetext,&(tmp->valuetex)))
	    error("Failed to make value (%s=%i) texture for IntChooserWidget!\n",
	    	((LabelWidget *)(tmp->name->wid_info))->tex.text,*(tmp->value));
    } else {
    	++tmp->direction;
    }
}

static void IntChooserWidget_Subtract( void *data )
{
    IntChooserWidget *tmp;
    char valuetext[16];
    int step;
    
    if (!data) return;
    tmp = ((IntChooserWidget *)((GLWidget *)data)->wid_info);

    if (tmp->direction < 0) {
    	step = (++tmp->duration)*(tmp->maxval-tmp->minval)/(MAX(1,MIN(maxFPS,FPS))*3);
    } else {
    	step = 1;
    }

    if (*(tmp->value) < tmp->minval) {
    	((ArrowWidget *)tmp->leftarrow->wid_info)->locked = true;
    } else if (step) {
    	tmp->duration = 0;
    	*(tmp->value) = MAX( (*(tmp->value)) - step, tmp->minval);

	if (tmp->callback) tmp->callback( tmp->data, NULL );

    	if ( (*(tmp->value)) < tmp->maxval)
	    ((ArrowWidget *)tmp->rightarrow->wid_info)->locked = false;
    	if ( (*(tmp->value)) == tmp->minval)
	    ((ArrowWidget *)tmp->leftarrow->wid_info)->locked = true;
	tmp->direction = -2;
	snprintf(valuetext,15,"%i",*(tmp->value));
	free_string_texture(&(tmp->valuetex));
	if(!render_text(&gamefont,valuetext,&(tmp->valuetex)))
	    error("Failed to make value (%s=%i) texture for IntChooserWidget!\n",
	    ((LabelWidget *)(tmp->name->wid_info))->tex.text,*(tmp->value));
    } else {
    	--tmp->direction;
    }
}

static void Paint_IntChooserWidget( GLWidget *widget )
{
    IntChooserWidget *wid_info;

    if (!widget) {
    	error("Paint_IntChooserWidget: argument is NULL!");
	return;
    }
    
    wid_info = (IntChooserWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("Paint_IntChooserWidget: wid_info missing");
	return;
    }
    
    if (wid_info->direction > 0) --(wid_info->direction);
    else if (wid_info->direction < 0) ++(wid_info->direction);

    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	set_alphacolor(*(wid_info->bgcolor));
    	glBegin(GL_QUADS);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y+widget->bounds.h	);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    	glEnd();
    }
    if ( wid_info->fgcolor )
    	disp_text(&(wid_info->valuetex), *(wid_info->fgcolor), RIGHT, CENTER, wid_info->rightarrow->bounds.x-1/*value_>*/-2/*>_|*/, draw_height - widget->bounds.y - widget->bounds.h/2, true );
    else
    	disp_text(&(wid_info->valuetex), whiteRGBA, RIGHT, CENTER, wid_info->rightarrow->bounds.x-1/*value_>*/-2/*>_|*/, draw_height - widget->bounds.y - widget->bounds.h/2, true );
}

GLWidget *Init_IntChooserWidget( const char *name, int *value, int minval, int maxval, Uint32 *fgcolor,
    	    	    	    	Uint32 *bgcolor, void (*callback)(void *tmp, const char *value), void *data )
{
    int valuespace;
    GLWidget *tmp;
    IntChooserWidget *wid_info;
    char valuetext[16];
    string_tex_t tmp_tex;
    int buttonsize;

    if (!value) {
    	error("Faulty parameter to Init_IntChooserWidget: value is a NULL pointer!");
	return NULL;
    }
    if (!(name) || !strlen(name) ) {
    	error("name misssing for Init_IntChooserWidget.");
	return NULL;
    }

    tmp = Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_IntChooserWidget");
	return NULL;
    }
    tmp->wid_info   = malloc(sizeof(IntChooserWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_IntChooserWidget");
	return NULL;
    }

    /* hehe ugly hack to guess max size of value strings
     * monospace font is preferred
     */
    if (render_text(&gamefont,"555.55",&tmp_tex)) {
    	free_string_texture(&tmp_tex);
	valuespace = tmp_tex.width+4;
    } else {
    	valuespace = 50;
    }
    
    wid_info = (IntChooserWidget *)tmp->wid_info;

    snprintf(valuetext,15,"%i",*(value));
    if(!render_text(&gamefont,valuetext,&(wid_info->valuetex))) {
    	Close_Widget(&tmp);
	error("Init_IntChooserWidget: Failed to render value string");
	return NULL;
    }
    buttonsize = wid_info->valuetex.height-4;
    
    tmp->WIDGET     = INTCHOOSERWIDGET;
    tmp->Draw	    	= Paint_IntChooserWidget;
    tmp->Close  	= Close_IntChooserWidget;
    tmp->SetBounds  	= SetBounds_IntChooserWidget;
    wid_info->value   	= value;
    wid_info->minval   	= minval;
    wid_info->maxval   	= maxval;
    wid_info->valuespace= valuespace;
    wid_info->direction = 0;
    wid_info->fgcolor 	= fgcolor;
    wid_info->bgcolor 	= bgcolor;
    wid_info->callback 	= callback;
    wid_info->data 	= data;
    
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->name = Init_LabelWidget(name,fgcolor,&nullRGBA,LEFT,CENTER))) ) {
    	Close_Widget(&tmp);
    	error("Init_IntChooserWidget: Failed to initialize label [%s]",name);
	return NULL;
    }
    
    wid_info->name->hover   	= hover_optionWidget;
    wid_info->name->hoverdata	= data;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->leftarrow  = Init_ArrowWidget(LEFTARROW,buttonsize,buttonsize,IntChooserWidget_Subtract,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_IntChooserWidget couldn't init leftarrow!");
    	return NULL;
    } 	
    
    if (*(wid_info->value) <= wid_info->minval) ((ArrowWidget *)(wid_info->leftarrow->wid_info))->locked = true;

    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->rightarrow = Init_ArrowWidget(RIGHTARROW,buttonsize,buttonsize,IntChooserWidget_Add,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_IntChooserWidget couldn't init rightarrow!");
    	return NULL;
    }
    
    if (*(wid_info->value) >= wid_info->maxval) ((ArrowWidget *)(wid_info->rightarrow->wid_info))->locked = true;

    tmp->bounds.w   = 2/*|_text*/+ wid_info->name->bounds.w +5/*text___<*/ + valuespace/*__value*/ + 2/*<_value_>*/
    	    	    + wid_info->leftarrow->bounds.w + wid_info->rightarrow->bounds.w +2/*>_|*/;
    tmp->bounds.h   = wid_info->name->bounds.h;

    return tmp;
}
/*************************/
/* End: IntChooserWidget */
/*************************/

/******************************/
/* Begin: DoubleChooserWidget */
/******************************/
static void DoubleChooserWidget_Add( void *data );
static void DoubleChooserWidget_Subtract( void *data );
static void Paint_DoubleChooserWidget( GLWidget *widget );
static void Close_DoubleChooserWidget ( GLWidget *widget );
static void SetBounds_DoubleChooserWidget( GLWidget *widget, SDL_Rect *b );

static void Close_DoubleChooserWidget ( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=DOUBLECHOOSERWIDGET) {
    	error("Wrong widget type for Close_DoubleChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    free_string_texture( &(((DoubleChooserWidget *)widget->wid_info)->valuetex) );
}

static void SetBounds_DoubleChooserWidget( GLWidget *widget, SDL_Rect *b )
{
    DoubleChooserWidget *tmp;
    GLWidget *tmp2;
    SDL_Rect rab,lab,b2;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=DOUBLECHOOSERWIDGET) {
    	error("Wrong widget type for SetBounds_DoubleChooserWidget [%i]",widget->WIDGET);
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    tmp = (DoubleChooserWidget *)(widget->wid_info);
    
    lab.h = rab.h = tmp->valuetex.height-4;
    lab.w = rab.w = rab.h;
    lab.y = rab.y = widget->bounds.y + (widget->bounds.h-rab.h)/2;
    rab.x = widget->bounds.x + widget->bounds.w - rab.w -2/*>_|*/;
    lab.x = rab.x - tmp->valuespace/*_value*/ -2/*<_value_>*/ - lab.w;
	    
    SetBounds_GLWidget(tmp->leftarrow,&lab);
    SetBounds_GLWidget(tmp->rightarrow,&rab);

    tmp2 = ((DoubleChooserWidget *)(widget->wid_info))->name;
    
    b2.h = widget->bounds.h;
    b2.w = tmp2->bounds.w;
    b2.x = widget->bounds.x + 2;
    b2.y = widget->bounds.y;
	    
    SetBounds_GLWidget(tmp2,&b2);
}

static void DoubleChooserWidget_Add( void *data )
{
    DoubleChooserWidget *tmp;
    double step;
    char valuetext[16];
    
    if (!data) return;
    tmp = ((DoubleChooserWidget *)((GLWidget *)data)->wid_info);

    if (tmp->direction > 0)
    	step = (tmp->maxval-tmp->minval)/(clientFPS*10.0);
    else
    	step = 0.01;
    
    if ( *(tmp->value) < tmp->maxval ) {
    	*(tmp->value) = MIN( (*(tmp->value))+step,tmp->maxval );
	
	if ( tmp->callback ) tmp->callback( tmp->data, NULL );

    	if ( (*(tmp->value)) > tmp->minval )
	    ((ArrowWidget *)tmp->leftarrow->wid_info)->locked = false;
    	if ( (*(tmp->value)) >= tmp->maxval )
	    ((ArrowWidget *)tmp->rightarrow->wid_info)->locked = true;
	tmp->direction = 2;
	snprintf(valuetext,15,"%1.2f",*(tmp->value));
	free_string_texture(&(tmp->valuetex));
	if(!render_text(&gamefont,valuetext,&(tmp->valuetex)))
	    error("Failed to make value (%s=%1.2f) texture for doubleChooserWidget!\n",
	    	    ((LabelWidget *)(tmp->name->wid_info))->tex.text,*(tmp->value));
    } else {
    	((ArrowWidget *)tmp->rightarrow->wid_info)->locked = true;
    }
}

static void DoubleChooserWidget_Subtract( void *data )
{
    DoubleChooserWidget *tmp;
    double step;
    char valuetext[16];
    
    if (!data) return;
    tmp = ((DoubleChooserWidget *)((GLWidget *)data)->wid_info);

    if (tmp->direction < 0)
    	step = (tmp->maxval-tmp->minval)/(clientFPS*10.0);
    else
    	step = 0.01;

    if ( *(tmp->value) > tmp->minval ) {
    	*(tmp->value) = MAX( (*(tmp->value))-step,tmp->minval );
	
    	if ( tmp->callback ) tmp->callback( tmp->data, NULL );

    	if ( (*(tmp->value)) < tmp->maxval )
	    ((ArrowWidget *)tmp->rightarrow->wid_info)->locked = false;
    	if ( (*(tmp->value)) <= tmp->minval )
	    ((ArrowWidget *)tmp->leftarrow->wid_info)->locked = true;
	tmp->direction = -2;
	snprintf(valuetext,15,"%1.2f",*(tmp->value));
	free_string_texture(&(tmp->valuetex));
	if(!render_text(&gamefont,valuetext,&(tmp->valuetex)))
	    error("Failed to make value (%s=%1.2f) texture for doubleChooserWidget!\n",
	    	    ((LabelWidget *)(tmp->name->wid_info))->tex.text,*(tmp->value));
    } else {
    	((ArrowWidget *)tmp->leftarrow->wid_info)->locked = true;
    }
}

static void Paint_DoubleChooserWidget( GLWidget *widget )
{
    DoubleChooserWidget *wid_info;

    if (!widget) {
    	error("Paint_DoubleChooserWidget: argument is NULL!");
	return;
    }
    
    wid_info = (DoubleChooserWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("Paint_DoubleChooserWidget: wid_info missing");
	return;
    }

    if (wid_info->direction > 0) --(wid_info->direction);
    else if (wid_info->direction < 0) ++(wid_info->direction);

    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	set_alphacolor(*(wid_info->bgcolor));
    	glBegin(GL_QUADS);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y+widget->bounds.h	);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    	glEnd();
    }

    if ( wid_info->fgcolor )
    	disp_text(&(wid_info->valuetex), *(wid_info->fgcolor), RIGHT, CENTER, wid_info->rightarrow->bounds.x-1/*value_>*/-2/*>_|*/, draw_height - widget->bounds.y - widget->bounds.h/2, true );
    else
    	disp_text(&(wid_info->valuetex), whiteRGBA, RIGHT, CENTER, wid_info->rightarrow->bounds.x-1/*value_>*/-2/*>_|*/, draw_height - widget->bounds.y - widget->bounds.h/2, true );
}

GLWidget *Init_DoubleChooserWidget( const char *name, double *value, double minval, double maxval,
    	    	    	    	    Uint32 *fgcolor, Uint32 *bgcolor,
    	    	    	    	    void (*callback)(void *tmp, const char *value), void *data )
{
    int valuespace;
    GLWidget *tmp;
    string_tex_t tmp_tex;
    DoubleChooserWidget *wid_info;
    char valuetext[16];
    int buttonsize;
    
    if (!value) {
    	error("Faulty parameter to Init_DoubleChooserWidget: value is a NULL pointer!");
	return NULL;
    }
    if (!(name) || !strlen(name) ) {
    	error("name misssing for Init_DoubleChooserWidget.");
	return NULL;
    }

    tmp = Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_DoubleChooserWidget");
	return NULL;
    }
    tmp->wid_info   = XMALLOC(DoubleChooserWidget, 1);
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_DoubleChooserWidget");
	return NULL;
    }
    
    /* hehe ugly hack to guess max size of value strings
     * monospace font is preferred
     */
    if (render_text(&gamefont,"555.55",&tmp_tex)) {
    	free_string_texture(&tmp_tex);
	valuespace = tmp_tex.width+4;
    } else {
    	valuespace = 50;
    }
    
    wid_info = (DoubleChooserWidget *)tmp->wid_info;

    snprintf(valuetext,15,"%1.2f",*(value));
    if(!render_text(&gamefont,valuetext,&(wid_info->valuetex))) {
    	Close_Widget(&tmp);
	error("Init_DoubleChooserWidget: Failed to render value string");
	return NULL;
    }
    buttonsize = wid_info->valuetex.height-4;

    tmp->WIDGET     = DOUBLECHOOSERWIDGET;
    tmp->Draw	    	= Paint_DoubleChooserWidget;
    tmp->Close  	= Close_DoubleChooserWidget;
    tmp->SetBounds  	= SetBounds_DoubleChooserWidget;
    wid_info->value   	= value;
    wid_info->minval   	= minval;
    wid_info->maxval   	= maxval;
    wid_info->valuespace = valuespace;
    wid_info->direction = 0;
    wid_info->fgcolor 	= fgcolor;
    wid_info->bgcolor 	= bgcolor;
    wid_info->callback 	= callback;
    wid_info->data 	= data;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->name = Init_LabelWidget(name,fgcolor,&nullRGBA,LEFT,CENTER))) ) {
    	Close_Widget(&tmp);
    	error("Init_DoubleChooserWidget: Failed to initialize label [%s]",name);
	return NULL;
    }
    
    wid_info->name->hover   	= hover_optionWidget;
    wid_info->name->hoverdata	= data;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->leftarrow  = Init_ArrowWidget(LEFTARROW,buttonsize,buttonsize,DoubleChooserWidget_Subtract,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_DoubleChooserWidget: couldn't init leftarrow!");
    	return NULL;
    } 	
    
    if (*(wid_info->value) <= wid_info->minval) ((ArrowWidget *)(wid_info->leftarrow->wid_info))->locked = true;

    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->rightarrow = Init_ArrowWidget(RIGHTARROW,buttonsize,buttonsize,DoubleChooserWidget_Add,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_DoubleChooserWidget: couldn't init rightarrow!");
    	return NULL;
    }
    
    if (*(wid_info->value) >= wid_info->maxval) ((ArrowWidget *)(wid_info->rightarrow->wid_info))->locked = true;

    tmp->bounds.w   = 2/*|_text*/+ wid_info->name->bounds.w +5/*text___<*/ + valuespace/*__value*/ + 2/*<_value_>*/
    	    	    + wid_info->leftarrow->bounds.w + wid_info->rightarrow->bounds.w +2/*>_|*/;
    tmp->bounds.h   = wid_info->name->bounds.h;

    return tmp;
}
/****************************/
/* End: DoubleChooserWidget */
/****************************/

/*****************************/
/* Begin: ColorChooserWidget */
/*****************************/
static void Paint_ColorChooserWidget( GLWidget *widget );
static void SetBounds_ColorChooserWidget( GLWidget *widget, SDL_Rect *b );
static void action_ColorChooserWidget(void *data);

static void SetBounds_ColorChooserWidget( GLWidget *widget, SDL_Rect *b )
{
    ColorChooserWidget *wid_info;
    GLWidget *name,*button,*m;
    SDL_Rect b2;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=COLORCHOOSERWIDGET) {
    	error("Wrong widget type for SetBounds_ColorChooserWidget [%i]",widget->WIDGET);
	return;
    }
    if (!(wid_info = (ColorChooserWidget *)(widget->wid_info) )) {
    	error("SetBounds_ColorChooserWidget: wid_info missing");
	return;
    }
    
    name = ((ColorChooserWidget *)(widget->wid_info))->name;
    button = ((ColorChooserWidget *)(widget->wid_info))->button;
    m = ((ColorChooserWidget *)(widget->wid_info))->mod;
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    b2.h = name->bounds.h;
    b2.w = name->bounds.w;
    b2.x = widget->bounds.x + 2;
    b2.y = widget->bounds.y;
	    
    SetBounds_GLWidget(name,&b2);

    b2.h = name->bounds.h - 4;
    b2.w = b2.h;
    b2.x = widget->bounds.x + widget->bounds.w - 2 - b2.w;
    b2.y = widget->bounds.y + 2;
	    
    SetBounds_GLWidget(button,&b2);

    if (wid_info->expanded && m) {
    	
    	b2.h = m->bounds.h;
    	b2.w = widget->bounds.w;
    	b2.x = widget->bounds.x;
    	b2.y = widget->bounds.y + name->bounds.h;
	    
    	SetBounds_GLWidget(m,&b2);
   }
}

static void Paint_ColorChooserWidget( GLWidget *widget )
{
    ColorChooserWidget *wid_info;

    if (!widget) {
    	error("Paint_ColorChooserWidget: argument is NULL!");
	return;
    }
    
    wid_info = (ColorChooserWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("Paint_ColorChooserWidget: wid_info missing");
	return;
    }

    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	set_alphacolor(*(wid_info->bgcolor));
    	glBegin(GL_QUADS);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w    ,widget->bounds.y	    	    	);
    	    glVertex2i(widget->bounds.x+widget->bounds.w    ,widget->bounds.y+widget->bounds.h	);
    	    glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    	glEnd();
    }
    set_alphacolor(blackRGBA);
    glBegin(GL_TRIANGLES);
    	glVertex2i(wid_info->button->bounds.x ,wid_info->button->bounds.y);
    	glVertex2i(wid_info->button->bounds.x ,wid_info->button->bounds.y + wid_info->button->bounds.h);
    	glVertex2i(wid_info->button->bounds.x +wid_info->button->bounds.w ,wid_info->button->bounds.y + wid_info->button->bounds.h);
    glEnd();
}

static void action_ColorChooserWidget(void *data)
{
    ColorChooserWidget *wid_info;
    GLWidget *widget;

    if (!(widget = (GLWidget *)data)) {
    	error("action_ColorChooserWidget: argument is NULL!");
	return;
    }
    
    if (widget->WIDGET != COLORCHOOSERWIDGET) {
    	error("action_ColorChooserWidget: wrong type widget!");
	return;
    }
    
    wid_info = (ColorChooserWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("action_ColorChooserWidget: wid_info missing");
	return;
    }
    
    if (wid_info->expanded) {
    	if (wid_info->mod) {
    	    DelGLWidgetListItem(&(widget->children),wid_info->mod);
	    widget->bounds.h -= wid_info->mod->bounds.h;
    	    Close_Widget(&(wid_info->mod));
    	} else error("action_ColorChooserWidget: Color mod widget mysteriously missing!");
	wid_info->expanded = false;
    } else {
    	if (!(wid_info->mod)) {
	    wid_info->mod = Init_ColorModWidget(wid_info->value,wid_info->fgcolor,&nullRGBA,wid_info->callback,wid_info->data);
    	    if (!(wid_info->mod)) {
	    	error("action_ColorChooserWidget: Failed to Init_ColorModWidget!");
	    }
	    widget->bounds.h += wid_info->mod->bounds.h;
	    AppendGLWidgetList(&(widget->children),wid_info->mod);
    	} else error("action_ColorChooserWidget: Color mod widget mysteriously present!");
	wid_info->expanded = true;
    }
    
    confmenu_callback();
}

GLWidget *Init_ColorChooserWidget( const char *name, Uint32 *value, Uint32 *fgcolor, Uint32 *bgcolor,
    	    	    	    	    void (*callback)(void *tmp, const char *value), void *data )
{
    GLWidget *tmp;
    ColorChooserWidget *wid_info;
    
    if (!value) {
    	error("Faulty parameter to Init_ColorChooserWidget: value is a NULL pointer!");
	return NULL;
    }
    if (!(name) || !strlen(name) ) {
    	error("name misssing for Init_ColorChooserWidget.");
	return NULL;
    }

    tmp = Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_ColorChooserWidget.");
	return NULL;
    }
    tmp->wid_info = XMALLOC(ColorChooserWidget, 1);
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ColorChooserWidget.");
	return NULL;
    }
    
    wid_info = (ColorChooserWidget *)tmp->wid_info;

    tmp->WIDGET     	= COLORCHOOSERWIDGET;
    tmp->Draw	    	= Paint_ColorChooserWidget;
    tmp->SetBounds  	= SetBounds_ColorChooserWidget;
    wid_info->value   	= value;
    wid_info->fgcolor 	= fgcolor;
    wid_info->bgcolor 	= bgcolor;
    wid_info->callback 	= callback;
    wid_info->data 	= data;
    wid_info->expanded 	= false;
    wid_info->mod 	= NULL;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->name = Init_LabelWidget(name,fgcolor,&nullRGBA,LEFT,CENTER))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorChooserWidget: Failed to initialize label [%s]",name);
	return NULL;
    }

    wid_info->name->hover   	= hover_optionWidget;
    wid_info->name->hoverdata	= data;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->button = Init_ButtonWidget( wid_info->value, &whiteRGBA, 1, action_ColorChooserWidget, tmp ))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorChooserWidget: Failed to initialize button");
	return NULL;
    }

    tmp->bounds.w   = 2 + wid_info->name->bounds.w + 5 + wid_info->button->bounds.w + 2;
    tmp->bounds.h   = wid_info->name->bounds.h;

    return tmp;
}
/***************************/
/* End: ColorChooserWidget */
/***************************/

/*****************************/
/* Begin: ColorModWidget */
/*****************************/
static void Paint_ColorModWidget( GLWidget *widget );
static void SetBounds_ColorModWidget( GLWidget *widget, SDL_Rect *b );
static void Callback_ColorModWidget(void *tmp, const char *value);

static void SetBounds_ColorModWidget( GLWidget *widget, SDL_Rect *b )
{
    ColorModWidget *wi;
    GLWidget *tmp2;
    SDL_Rect b2;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=COLORMODWIDGET) {
    	error("Wrong widget type for SetBounds_ColorModWidget [%i]",widget->WIDGET);
	return;
    }
    
    if ( !(wi=((ColorModWidget *)(widget->wid_info))) ) {
    	error("SetBounds_ColorModWidget: wid_info missing!");
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    tmp2 = wi->redpick;
    
    b2.x = widget->bounds.x + 2;
    b2.y = widget->bounds.y;
    b2.h = tmp2->bounds.h;
    b2.w = tmp2->bounds.w;
	    
    SetBounds_GLWidget(tmp2,&b2);
    
    tmp2 = wi->greenpick;
    
    b2.x = widget->bounds.x + 2;
    b2.y += b2.h;
    b2.h = tmp2->bounds.h;
    b2.w = tmp2->bounds.w;
	    
    SetBounds_GLWidget(tmp2,&b2);
    
    tmp2 = wi->bluepick;
    
    b2.x = widget->bounds.x + 2;
    b2.y += b2.h;
    b2.h = tmp2->bounds.h;
    b2.w = tmp2->bounds.w;
	    
    SetBounds_GLWidget(tmp2,&b2);
    
    tmp2 = wi->alphapick;
    
    b2.x = widget->bounds.x + 2;
    b2.y += b2.h;
    b2.h = tmp2->bounds.h;
    b2.w = tmp2->bounds.w;
	    
    SetBounds_GLWidget(tmp2,&b2);
}

static void Paint_ColorModWidget( GLWidget *widget )
{
    ColorModWidget *wid_info;
    SDL_Rect b;

    if (!widget) {
    	error("Paint_ColorModWidget: argument is NULL!");
	return;
    }
    
    if ( widget->WIDGET != COLORMODWIDGET ) {
    	error("Paint_ColorModWidget: widget is not a ColorModWidget!");
	return;
    }
    
    wid_info = (ColorModWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("Paint_ColorModWidget: wid_info missing");
	return;
    }

    if ( (wid_info->bgcolor) && *(wid_info->bgcolor) ) {
    	b.x = widget->bounds.x;
    	b.y = widget->bounds.y;
    	b.w = widget->bounds.w;
    	b.h = widget->bounds.h;
    	set_alphacolor(*(wid_info->bgcolor));
    	glBegin(GL_QUADS);
    	    glVertex2i(b.x  	,b.y	    );
    	    glVertex2i(b.x+b.w	,b.y	    );
    	    glVertex2i(b.x+b.w	,b.y+b.h    );
    	    glVertex2i(b.x  	,b.y+b.h    );
    	glEnd();
    }
    
    b.x = wid_info->redpick->bounds.x + wid_info->redpick->bounds.w + 16;
    b.y = widget->bounds.y + 2;
    b.w = widget->bounds.x + widget->bounds.w - 2 - b.x;
    b.h = widget->bounds.y + widget->bounds.h - 2 - b.y;
    
    glBegin(GL_TRIANGLES);
    	set_alphacolor(blackRGBA);
    	glVertex2i(b.x	    ,b.y    	);
    	glVertex2i(b.x	    ,b.y+b.h    );
    	glVertex2i(b.x+b.w  ,b.y+b.h    );
    	set_alphacolor(whiteRGBA);
    	glVertex2i(b.x	    ,b.y    	);
    	glVertex2i(b.x+b.w  ,b.y+b.h    );
    	glVertex2i(b.x+b.w  ,b.y    	);
    glEnd();
    
    glBegin(GL_POLYGON);
    	set_alphacolor(whiteRGBA);
    	glVertex2i(b.x + (GLint)(0.9*b.w)   ,b.y + b.h	    	    );
    	glVertex2i(b.x + b.w	    	    ,b.y + b.h	    	    );
    	glVertex2i(b.x + b.w	    	    ,b.y + (GLint)(0.9*b.h) );
    	set_alphacolor(blackRGBA);
    	glVertex2i(b.x + (GLint)(0.1*b.w)   ,b.y    	    	    );
    	glVertex2i(b.x	    	    	    ,b.y    	    	    );
    	glVertex2i(b.x	    	    	    ,b.y + (GLint)(0.1*b.h) );
    glEnd();
    
    glBegin(GL_QUADS);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	set_alphacolor(*(wid_info->value));
    	glVertex2i(b.x + (GLint)(0.1*b.w)   ,b.y + (GLint)(0.1*b.h) );
    	glVertex2i(b.x + (GLint)(0.1*b.w)   ,b.y + (GLint)(0.9*b.h) );
    	glVertex2i(b.x + (GLint)(0.9*b.w)   ,b.y + (GLint)(0.9*b.h) );
    	glVertex2i(b.x + (GLint)(0.9*b.w)   ,b.y + (GLint)(0.1*b.h) );
    glEnd();
}

static void Callback_ColorModWidget(void *tmp, const char *value)
{
    GLWidget *widget;
    ColorModWidget *wid_info;
    char str[10];

    if (!(widget = (GLWidget *)tmp)) {
    	error("Callback_ColorModWidget: argument is NULL!");
	return;
    }

    if ( widget->WIDGET != COLORMODWIDGET ) {
    	error("Callback_ColorModWidget: widget is not a ColorModWidget!");
	return;
    }
    
    wid_info = (ColorModWidget *)(widget->wid_info);

    if (!wid_info) {
    	error("Callback_ColorModWidget: wid_info missing");
	return;
    }

    *(wid_info->value) = ((wid_info->red)<<24) | ((wid_info->green)<<16)
    	    	    	 | ((wid_info->blue)<<8) | (wid_info->alpha);
			 
    snprintf(str,10,"#%02X%02X%02X%02X",wid_info->red,wid_info->green,wid_info->blue,wid_info->alpha);
    str[9] = '\0';
    
    if (wid_info->callback) wid_info->callback(wid_info->data,str);
}

GLWidget *Init_ColorModWidget( Uint32 *value, Uint32 *fgcolor, Uint32 *bgcolor,
    	    	    	    	    void (*callback)(void *tmp, const char *value), void *data )
{
    GLWidget *tmp;
    ColorModWidget *wid_info;
    int maxwidth = 0;
    
    if (!value) {
    	error("Faulty parameter to Init_ColorModWidget: value is a NULL pointer!");
	return NULL;
    }
    tmp = Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_ColorModWidget.");
	return NULL;
    }
    tmp->wid_info   = XMALLOC(ColorModWidget, 1);
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ColorModWidget.");
	return NULL;
    }
    
    wid_info = (ColorModWidget *)tmp->wid_info;

    tmp->WIDGET     	= COLORMODWIDGET;
    tmp->Draw	    	= Paint_ColorModWidget;
    tmp->SetBounds  	= SetBounds_ColorModWidget;
    wid_info->value   	= value;
    wid_info->red   	= ((*value) >> 24) & 255;
    wid_info->green   	= ((*value) >> 16) & 255;
    wid_info->blue   	= ((*value) >> 8) & 255;
    wid_info->alpha   	= (*value) & 255;
    wid_info->fgcolor 	= fgcolor;
    wid_info->bgcolor 	= bgcolor;
    wid_info->callback 	= callback;
    wid_info->data 	= data;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->redpick = Init_IntChooserWidget("Red",&(wid_info->red),0,255,fgcolor,&nullRGBA,Callback_ColorModWidget,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorModWidget: Failed to initialize label [%s]","Red");
	return NULL;
    }
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->greenpick = Init_IntChooserWidget("Green",&(wid_info->green),0,255,fgcolor,&nullRGBA,Callback_ColorModWidget,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorModWidget: Failed to initialize label [%s]","Green");
	return NULL;
    }
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->bluepick = Init_IntChooserWidget("Blue",&(wid_info->blue),0,255,fgcolor,&nullRGBA,Callback_ColorModWidget,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorModWidget: Failed to initialize label [%s]","Blue");
	return NULL;
    }
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->alphapick = Init_IntChooserWidget("Alpha",&(wid_info->alpha),0,255,fgcolor,&nullRGBA,Callback_ColorModWidget,tmp))) ) {
    	Close_Widget(&tmp);
    	error("Init_ColorModWidget: Failed to initialize label [%s]","Alpha");
	return NULL;
    }

    maxwidth = MAX(maxwidth,wid_info->redpick->bounds.w);
    maxwidth = MAX(maxwidth,wid_info->greenpick->bounds.w);
    maxwidth = MAX(maxwidth,wid_info->bluepick->bounds.w);
    maxwidth = MAX(maxwidth,wid_info->alphapick->bounds.w);
    
    wid_info->redpick->bounds.w = maxwidth;
    wid_info->greenpick->bounds.w = maxwidth;
    wid_info->bluepick->bounds.w = maxwidth;
    wid_info->alphapick->bounds.w = maxwidth;

    tmp->bounds.h   = wid_info->redpick->bounds.h
    	    	    + wid_info->greenpick->bounds.h
    	    	    + wid_info->bluepick->bounds.h
		    + wid_info->alphapick->bounds.h;
    tmp->bounds.w   = 2 + maxwidth + 16 + tmp->bounds.h + 2;

    return tmp;
}
/***********************/
/* End: ColorModWidget */
/***********************/

/**********************/
/* Begin: ListWidget  */
/**********************/
static void SetBounds_ListWidget( GLWidget *widget, SDL_Rect *b );
static void Paint_ListWidget( GLWidget *widget );

bool ListWidget_Append( GLWidget *list, GLWidget *item )
{
    GLWidget *curr1, **curr2;
    ListWidget *wid_info;
    
    if (!list) {
    	error("ListWidget_Append: *list is NULL!");
	return false;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_Append: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return false;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Append: list->wid_info missing!");
	return false;
    }
    if (!item) {
    	error("ListWidget_Append: *item is NULL");
	return false;
    }

    curr1 = item;
    while (curr1) {   	
	curr2 = &(list->children);
    	while (*curr2) {
	    if (*curr2 == curr1) break;
	    curr2 = &((*curr2)->next);
    	}

    	if (*curr2) {
	    error("ListWidget_Append: Attempt to append an existing item!");
	    break;
	}
	
	*curr2 = curr1;
	curr1 = curr1->next;
	/* disengage added item from the item list t be added */
	(*curr2)->next = NULL;

     	++wid_info->num_elements;
    }
    
    /* This works since SetBounds_ListWidget copies the content
     * if the SDL_Rect *
     */
    SetBounds_ListWidget( list, &(list->bounds));

    return true;
}

bool ListWidget_Prepend( GLWidget *list, GLWidget *item )
{
    GLWidget *curr1, *curr2, **entry_pt, *first;
    ListWidget *wid_info;
    /*int y_rel;*/
    
    if (!list) {
    	error("ListWidget_Prepend: *list is NULL!");
	return false;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_Prepend: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return false;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Prepend: list->wid_info missing!");
	return false;
    }
    if (!item) {
    	error("ListWidget_Prepend: *item is NULL");
	return false;
    }
    
    entry_pt = &(list->children);
    first = *entry_pt;
    
    curr1 = item;
    while (curr1) {
    	curr2 = list->children;
    	while (curr2) {
	    if (curr2 == item) break;
	    curr2 = curr2->next;
    	}
	
	if (curr2) {
	    error("ListWidget_Append: Attempt to append an existing item!");
	    break;
	}
    	
	*entry_pt = curr1;
	curr1 = curr1->next;
	(*entry_pt)->next = first;
	entry_pt = &((*entry_pt)->next);

    	++wid_info->num_elements;
    }
    
    /* This works since SetBounds_ListWidget copies the content
     * if the SDL_Rect *
     */
    SetBounds_ListWidget( list, &(list->bounds));
    
    return true;
}

bool ListWidget_Insert( GLWidget *list, GLWidget *target, GLWidget *item )
{
    GLWidget **curr, *curr1, *curr2;
    ListWidget *wid_info;
    
    if (!list) {
    	error("ListWidget_Insert: *list is NULL!");
	return false;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_Insert: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return false;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Insert: list->wid_info missing!");
	return false;
    }
    if (!item) {
    	error("ListWidget_Insert: *item is NULL");
	return false;
    }

    curr = &(list->children);
    while (*curr) {
	if (*curr == target) break;
	curr = &((*curr)->next);
    }
    
    if (!(*curr)) {
	    error("ListWidget_Insert: target is not in the list!");
	    return false;
    }
    
    curr1 = item;
    while (curr1) {
    	curr2 = list->children;
    	while (curr2) {
	    if (curr2 == item) break;
	    curr2 = curr2->next;
    	}
	
	if (curr2) {
	    error("ListWidget_Append: Attempt to append an existing item!");
	    break;
	}
    	
	*curr = curr1;
	curr1 = curr1->next;
	(*curr)->next = target;
	curr = &((*curr)->next);

    	++wid_info->num_elements;
    }
    
    /* This works since SetBounds_ListWidget copies the content
     * if the SDL_Rect *
     */
    SetBounds_ListWidget( list, &(list->bounds));
    
    return true;
}

bool ListWidget_Remove( GLWidget *list, GLWidget *item )
{
    GLWidget **curr;
    ListWidget *wid_info;
    
    if (!list) {
    	error("ListWidget_Remove: *list is NULL!");
	return false;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_Remove: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return false;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Remove: list->wid_info missing!");
	return false;
    }
    if (!item) {
    	error("ListWidget_Remove: *item is NULL");
	return false;
    }
    
    curr = &(list->children);
    while (*curr) {
	if (*curr == item) {
	    break;
	}
	curr = &((*curr)->next);
    }
    
    if (!(*curr)) {
	    error("ListWidget_Remove: item is not in the list!");
	    return false;
    }
    
    *curr = (*curr)->next;
    item->next = NULL;
    
    --wid_info->num_elements;
    
    /* This works since SetBounds_ListWidget copies the content
     * if the SDL_Rect *
     */
    SetBounds_ListWidget( list, &(list->bounds));
    
    return true;
}

bool ListWidget_SetScrollorder( GLWidget *list, bool order )
{
    /*GLWidget **curr;*/
    ListWidget *wid_info;
    
    if (!list) {
    	error("ListWidget_SetScrollorder: *list is NULL!");
	return false;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_SetScrollorder: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return false;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_SetScrollorder: list->wid_info missing!");
	return false;
    }

    wid_info->reverse_scroll = order;
        
    /* This works since SetBounds_ListWidget copies the content
     * if the SDL_Rect *
     */
    SetBounds_ListWidget( list, &(list->bounds));
    
    return true;
}

int ListWidget_NELEM( GLWidget *list )
{
    ListWidget *wid_info;
    
    if (!list) {
    	error("ListWidget_NELEM: *list is NULL!");
	return -1;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_NELEM: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return -1;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Remove: list->wid_info missing!");
	return -1;
    }
    
    return wid_info->num_elements;
}

GLWidget *ListWidget_GetItemByIndex( GLWidget *list, int i )
{
    GLWidget *curr;
    ListWidget *wid_info;
    int j;
    
    if (!list) {
    	error("ListWidget_NELEM: *list is NULL!");
	return NULL;
    }
    if (list->WIDGET != LISTWIDGET) {
    	error("ListWidget_NELEM: list is not a LISTWIDGET! [%i]",list->WIDGET);
	return NULL;
    }
    if (!(wid_info = (ListWidget *)list->wid_info)) {
    	error("ListWidget_Remove: list->wid_info missing!");
	return NULL;
    }
    
    if ( i > ( wid_info->num_elements - 1) ) return NULL;
    
    curr = list->children;
    j = 0;
    while (curr && (j<i)) {
	curr = curr->next;
    	++j;
    }
    
    if ( j != i ) return NULL;
    
    return curr;
}


static void Paint_ListWidget( GLWidget *widget )
{
    ListWidget *wid_info;
    GLWidget *curr;
    int count = 0;
    Uint32  *col;

    if (!widget) return;
    
    wid_info = (ListWidget *)(widget->wid_info);

    glBegin(GL_QUADS);

    curr = widget->children;
    while (curr) {
    	if (count % 2) col = wid_info->bg2;
    	else col = wid_info->bg1;
	
	++count;
    
    	if (*col) {
    	    set_alphacolor(*col);

    	    glVertex2i(curr->bounds.x	    	    	,curr->bounds.y	    	    	);
    	    glVertex2i(curr->bounds.x+widget->bounds.w	,curr->bounds.y	    	    	);
    	    glVertex2i(curr->bounds.x+widget->bounds.w	,curr->bounds.y+curr->bounds.h	);
    	    glVertex2i(curr->bounds.x 	    	    	,curr->bounds.y+curr->bounds.h	);
    	}
    	curr = curr->next;
    }

    glEnd();
}

/* This setbounds method has very special behaviour; basically
 * only one corner of the bounding box is regarded, this is 
 * because the list is an expanding/contracting entity, and
 * thus the others might soon get overridden anyway.
 * The only way to properly bound a ListWidget is to make a
 * bounding widget adopt it. (i.e. a scrollpane)
 */
static void SetBounds_ListWidget( GLWidget *widget, SDL_Rect *b )
{
    ListWidget *tmp;
    SDL_Rect bounds,b2;
    GLWidget *curr;    
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET !=LISTWIDGET) {
    	error("Wrong widget type for SetBounds_ListWidget [%i]",widget->WIDGET);
	return;
    }

    if (!(tmp = (ListWidget *)(widget->wid_info))) {
    	error("SetBounds_ListWidget: wid_info missing!");
	return;
    }
    
    bounds.y = b->y;
    bounds.x = b->x;
    bounds.w = 0;
    bounds.h = 0;
    curr = widget->children;
    while(curr) {
    	if (tmp->direction == VERTICAL) {
    	    bounds.w = MAX(bounds.w,curr->bounds.w);
	    bounds.h += curr->bounds.h;
	} else {
    	    bounds.w += curr->bounds.w;
	    bounds.h = MAX(bounds.h,curr->bounds.h);
	}
	curr = curr->next;
    }

    if (tmp->v_dir == LW_UP) {
    	bounds.y += b->h - bounds.h;
    }
    if (tmp->v_dir == LW_VCENTER) {
    	bounds.y += (b->h - bounds.h)/2;
    }
    if (tmp->h_dir == LW_LEFT) {
    	bounds.x += b->w - bounds.w;
    }
    if (tmp->h_dir == LW_HCENTER) {
    	bounds.x += (b->w - bounds.w)/2;
    }
    
    widget->bounds.y = bounds.y;
    widget->bounds.h = bounds.h;
    widget->bounds.x = bounds.x;
    widget->bounds.w = bounds.w;
    
    curr = widget->children;
    while(curr) {
    	if (tmp->direction == VERTICAL) {
    	    bounds.h -= curr->bounds.h;

	    b2.y = bounds.y;
	    if (tmp->reverse_scroll) {
    	    	b2.y += bounds.h;
    	    } else {
	    	bounds.y += curr->bounds.h;
	    }

    	    b2.x = bounds.x;
	    if (tmp->h_dir == LW_LEFT) {
	    	b2.x += bounds.w - curr->bounds.w;
	    }
	
	    b2.h = curr->bounds.h;
	    b2.w = widget->bounds.w;
	    /*b2.w = curr->bounds.w;*/ /*TODO: make this optional*/
	} else {
    	    bounds.w -= curr->bounds.w;

	    b2.x = bounds.x;
	    if (tmp->reverse_scroll) {
    	    	b2.x += bounds.w;
    	    } else {
	    	bounds.x += curr->bounds.w;
	    }

    	    b2.y = bounds.y;
	    if (tmp->v_dir == LW_UP) {
	    	b2.y += bounds.h - curr->bounds.h;
	    }
	
	    b2.w = curr->bounds.w;
	    b2.h = widget->bounds.h;
	    /*b2.w = curr->bounds.w;*/ /*TODO: make this optional*/
	}
	
	SetBounds_GLWidget( curr, &b2 );

    	curr = curr->next;
    }
}

GLWidget *Init_ListWidget( Uint16 x, Uint16 y, Uint32 *bg1, Uint32 *bg2, Uint32 *highlight_color
    	    	    	    ,ListWidget_ver_dir_t v_dir, ListWidget_hor_dir_t h_dir
			    ,ListWidget_direction direction, bool reverse_scroll )
{
    GLWidget *tmp;
    ListWidget *wid_info;
    
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc GLWidget in Init_ListWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ListWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc MainWidget in Init_ListWidget");
	return NULL;
    }
    wid_info = ((ListWidget *)tmp->wid_info);
    wid_info->num_elements = 0;
    wid_info->bg1	= bg1;
    wid_info->bg2	= bg2;
    wid_info->highlight_color	= highlight_color;
    wid_info->reverse_scroll	= reverse_scroll;
    wid_info->direction	= direction;
    wid_info->v_dir	= v_dir;
    wid_info->h_dir	= h_dir;
    
    tmp->WIDGET     	= LISTWIDGET;
    tmp->bounds.x   	= x;
    tmp->bounds.y   	= y;
    tmp->bounds.w   	= 0;
    tmp->bounds.h   	= 0;
    tmp->SetBounds  	= SetBounds_ListWidget;
    tmp->Draw	    	= Paint_ListWidget;

    return tmp;
}

/*******************/
/* End: ListWidget */
/*******************/

/****************************/
/* Begin: ScrollPaneWidget  */
/****************************/
static void ScrollPaneWidget_poschange( GLfloat pos , void *data );
static void SetBounds_ScrollPaneWidget( GLWidget *widget, SDL_Rect *b );

static void ScrollPaneWidget_poschange( GLfloat pos , void *data )
{
    GLWidget *widget;
    GLWidget *masque;
    ScrollPaneWidget *wid_info;
    SDL_Rect bounds;
    GLWidget *vert_scroller;
    GLWidget *hori_scroller;
    /*GLWidget *curr;*/
    
    if ( !data ) {
        error("NULL data to ScrollPaneWidget_poschange!");
	return;
    }
    widget = (GLWidget *)data;
    wid_info = ((ScrollPaneWidget *)(widget->wid_info));
    masque = wid_info->masque;
    if (!masque) {
    	error("ScrollPaneWidget_poschange: masque missing!");
	return;
    }
    vert_scroller = wid_info->vert_scroller;
    hori_scroller = wid_info->hori_scroller;

    if (wid_info->content) {
    	bounds.x = masque->bounds.x;
    	if (hori_scroller) {
	    if (!hori_scroller->wid_info) {
    	    	error("ScrollPaneWidget_poschange: hori_scroller wid_info missing!");
	    	return;
	    }
    	    bounds.x -= (Sint16)((((ScrollbarWidget *)(hori_scroller->wid_info))->pos)*(wid_info->content->bounds.w));
	}
    	bounds.w = wid_info->content->bounds.w;
    	bounds.y = masque->bounds.y;
    	if (vert_scroller) {
	    if (!vert_scroller->wid_info) {
    	    	error("ScrollPaneWidget_poschange: vert_scroller wid_info missing!");
	    	return;
	    }
    	    bounds.y -= (Sint16)((((ScrollbarWidget *)(vert_scroller->wid_info))->pos)*(wid_info->content->bounds.h));
	}
    	bounds.h = wid_info->content->bounds.h;
    	SetBounds_GLWidget(wid_info->content,&bounds);
    }    
}

static void SetBounds_ScrollPaneWidget(GLWidget *widget, SDL_Rect *b )
{
    ScrollPaneWidget *wid_info;
    SDL_Rect bounds;
    GLfloat pos;
    int i;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET != SCROLLPANEWIDGET) {
    	error("Wrong widget type for SetBounds_ScrollPaneWidget [%i]",widget->WIDGET);
	return;
    }

    if (!(wid_info = (ScrollPaneWidget *)(widget->wid_info))) {
    	error("SetBounds_ScrollPaneWidget: wid_info missing!");
	return;
    }
    
    widget->bounds.x = b->x;
    widget->bounds.w = b->w;
    widget->bounds.y = b->y;
    widget->bounds.h = b->h;

    if (!wid_info->masque) {
    	error("SetBounds_ScrollPaneWidget: masque missing!");
	return;
    }

    if (!wid_info->content) {
    	if (wid_info->vert_scroller) {
    	    Close_Widget(&(wid_info->vert_scroller));
    	}
    	if (wid_info->hori_scroller) {
    	    Close_Widget(&(wid_info->hori_scroller));
    	}
    	return;
    }
    
    for ( i = 0 ; i < 2 ; ++i ) {
    	if  (
	    (	(wid_info->vert_scroller) &&
	    	(widget->bounds.w - wid_info->vert_scroller->bounds.w < wid_info->content->bounds.w))
	    || (widget->bounds.w < wid_info->content->bounds.w)
	    ) {
    	    if (!wid_info->hori_scroller) {
    	    	if  ( !AppendGLWidgetList(&(widget->children),
    	    	    	(wid_info->hori_scroller = Init_ScrollbarWidget(false,0.0f,1.0f,SB_HORISONTAL
	    	    	    	    	    	    ,ScrollPaneWidget_poschange,widget)))
		    ) {
	    	    error("SetBounds_ScrollPaneWidget: Failed to init horisontal scroller!");
    	    	    return;
    	    	}
	    }
    	} else {
    	    if (wid_info->hori_scroller) {
	    	Close_Widget(&(wid_info->hori_scroller));
	    }
    	}

    	if  (
	    (	(wid_info->hori_scroller) &&
	    	(widget->bounds.h - wid_info->hori_scroller->bounds.h < wid_info->content->bounds.h))
	    || (widget->bounds.h < wid_info->content->bounds.h)
	    ) {
    	    if (!wid_info->vert_scroller) {
    	    	if  ( !AppendGLWidgetList(&(widget->children),
    	    	    	(wid_info->vert_scroller = Init_ScrollbarWidget(false,0.0f,1.0f,SB_VERTICAL
	    	    	    	    	    	    ,ScrollPaneWidget_poschange,widget)))
		    ) {
	    	    error("SetBounds_ScrollPaneWidget: Failed to init vertical scroller!");
    	    	    return;
    	    	}
	    }
    	} else {
    	    if (wid_info->vert_scroller) {
	    	Close_Widget(&(wid_info->vert_scroller));
	    }
    	}
    }
    
    if (wid_info->vert_scroller) {
    	bounds.x = widget->bounds.x + widget->bounds.w - wid_info->vert_scroller->bounds.w;
    	bounds.w = wid_info->vert_scroller->bounds.w;
    	bounds.y = widget->bounds.y;
    	bounds.h = widget->bounds.h;
    	if (wid_info->hori_scroller)
    	    bounds.h -= wid_info->hori_scroller->bounds.h;
    
    	SetBounds_GLWidget(wid_info->vert_scroller,&bounds);
    }

    if (wid_info->hori_scroller) {
     	bounds.x = widget->bounds.x;
    	bounds.w = widget->bounds.w;
    	if (wid_info->vert_scroller)
    	    bounds.w -= wid_info->vert_scroller->bounds.w;
    	bounds.y = widget->bounds.y + widget->bounds.h - wid_info->hori_scroller->bounds.h;
    	bounds.h = wid_info->hori_scroller->bounds.h;
    
    	SetBounds_GLWidget(wid_info->hori_scroller,&bounds);
    }
    
    /* we already made sure masque exists! */
    bounds.x = widget->bounds.x;
    bounds.w = widget->bounds.w;
    if (wid_info->vert_scroller)
    	bounds.w -= wid_info->vert_scroller->bounds.w;
    bounds.y = widget->bounds.y;
    bounds.h = widget->bounds.h;
    if (wid_info->hori_scroller)
    	bounds.h -= wid_info->hori_scroller->bounds.h;
    
    SetBounds_GLWidget(wid_info->masque,&bounds);
    
    if (wid_info->content) {
    	if (wid_info->hori_scroller) {
	    bounds.w = wid_info->content->bounds.w;
	} else {
	    bounds.w = wid_info->masque->bounds.w;
	}
    	if (wid_info->vert_scroller) {
	    bounds.h = wid_info->content->bounds.h;
	} else {
	    bounds.h = wid_info->masque->bounds.h;
	}
    
	if (wid_info->hori_scroller) {
	    ScrollbarWidget_SetSlideSize(wid_info->hori_scroller,MIN(((GLfloat)wid_info->masque->bounds.w)/((GLfloat)bounds.w),1.0f));
    	    pos = MIN( ((ScrollbarWidget *)(wid_info->hori_scroller->wid_info))->pos, 1.0f - ((ScrollbarWidget *)(wid_info->hori_scroller->wid_info))->size);
    	    bounds.x = (Sint16)(wid_info->masque->bounds.x - pos*(wid_info->content->bounds.x));
	} else {
	    bounds.x = wid_info->masque->bounds.x;
	}
	
	if (wid_info->vert_scroller) {
	    ScrollbarWidget_SetSlideSize(wid_info->vert_scroller,MIN(((GLfloat)wid_info->masque->bounds.h)/((GLfloat)bounds.h),1.0f));
    	    pos = MIN( ((ScrollbarWidget *)(wid_info->vert_scroller->wid_info))->pos, 1.0f - ((ScrollbarWidget *)(wid_info->vert_scroller->wid_info))->size);
    	    bounds.y = (Sint16)(wid_info->masque->bounds.y - pos*(wid_info->content->bounds.h));
	} else {
	    bounds.y = wid_info->masque->bounds.y;
	}
    
    	SetBounds_GLWidget(wid_info->content,&bounds);
    }
}

GLWidget *Init_ScrollPaneWidget( GLWidget *content )
{
    GLWidget *tmp;
    ScrollPaneWidget *wid_info;
    /*SDL_Rect b;*/
    
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc GLWidget in Init_ScrollPaneWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ScrollPaneWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc MainWidget in Init_ScrollPaneWidget");
	return NULL;
    }
    wid_info = ((ScrollPaneWidget *)tmp->wid_info);
    wid_info->content	= content;
    wid_info->hori_scroller = NULL;
    wid_info->vert_scroller = NULL;
    
    tmp->WIDGET     	= SCROLLPANEWIDGET;
    tmp->bounds.x   	= 0;
    tmp->bounds.y   	= 0;
    tmp->bounds.w   	= 0;
    tmp->bounds.h   	= 0;
    tmp->SetBounds  	= SetBounds_ScrollPaneWidget;
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->masque = Init_EmptyBaseGLWidget()))
    	) {
	error("Init_ScrollPaneWidget: Failed to init masque!");
	Close_Widget(&tmp);
    	return NULL;
    }
    
    if (wid_info->content) {
    	if ( !AppendGLWidgetList(&(wid_info->masque->children),wid_info->content) ) {
	    error("Init_ScrollPaneWidget: Failed to adopt the content to the masque!");
    	    /*Close_Widget(&(wid_info->scroller));*/
	    Close_Widget(&tmp);
    	    return NULL;
    	}
    
        tmp->bounds.w = wid_info->content->bounds.w;
        tmp->bounds.h = wid_info->content->bounds.h;
    }
    
    /*tmp->bounds.w += wid_info->scroller->bounds.w;*/
    
    return tmp;
}

/**************************/
/* End: ScrollPaneWidget  */
/**************************/

/**********************/
/* Begin: MainWidget  */
/**********************/
static void SetBounds_MainWidget( GLWidget *widget, SDL_Rect *b );
static void button_MainWidget( Uint8 button, Uint8 state , Uint16 x , Uint16 y, void *data );
static void Close_MainWidget( GLWidget *widget );

void MainWidget_ShowMenu( GLWidget *widget, bool show )
{
    WrapperWidget *wid_info;
    
    if (!widget) {
    	error("MainWidget_ShowMenu: widget missing!");
	return;
    }
    if (widget->WIDGET != MAINWIDGET) {
    	error("Wrong widget type for MainWidget_ShowMenu [%i]",widget->WIDGET);
	return;
    }
    if (!(wid_info = (WrapperWidget *)(widget->wid_info)) ) {
    	error("MainWidget_ShowMenu: wid_info missing!");
	return;
    }
    
    if (wid_info->showconf == show) return;
    
    wid_info->showconf = show;
    if (show) {
    	AppendGLWidgetList(&(widget->children), wid_info->confmenu);
    } else {
    	DelGLWidgetListItem(&(widget->children), wid_info->confmenu);
	if (hovertarget)
	    hover_optionWidget( 0, 0 , 0 , NULL );
    }
}

static void SetBounds_MainWidget( GLWidget *widget, SDL_Rect *b )
{
    WrapperWidget *wid_info;
    SDL_Rect bs = {0,0,0,0},*bw;
    GLWidget *subs[5];
    int i,diff;
    bool change;
    
    if (!widget) return;
    if (!b) return;
    if (widget->WIDGET != MAINWIDGET) {
    	error("Wrong widget type for SetBounds_MainWidget [%i]",widget->WIDGET);
	return;
    }

    if (!(wid_info = (WrapperWidget *)(widget->wid_info))) {
    	error("SetBounds_MainWidget: wid_info missing!");
	return;
    }
    
    subs[0] = wid_info->radar;
    subs[1] = wid_info->scorelist;
    subs[2] = wid_info->chat_msgs;
    subs[3] = wid_info->game_msgs;
    subs[4] = wid_info->confmenu;
    
    bw = &(widget->bounds);
    
    for ( i=0; i < 5 ; ++i) {
    	if (subs[i]) {
	    change = false;
	    
    	    bs.x = subs[i]->bounds.x;
    	    bs.w = subs[i]->bounds.w;
    	    bs.y = subs[i]->bounds.y;
    	    bs.h = subs[i]->bounds.h;
	    
	    if ( ABS(bs.x - bw->x) > ABS( diff = (bw->x+bw->w) - (bs.x+bs.w) ) ) {
	    	bs.x = b->x + b->w - diff - bs.w;
		change = true;
	    }

	    if ( ABS(bs.y - bw->y) > ABS( diff = (bw->y+bw->h) - (bs.y+bs.h) ) ) {
	    	bs.y = b->y + b->h - diff - bs.h;
		change = true;
	    }

	    if ( change )    
    	    	SetBounds_GLWidget(subs[i],&bs);
    	}
    }
    
    bs.w = wid_info->alert_msgs->bounds.w;
    bs.x = (b->w - bs.w)/2;
    bs.h = wid_info->alert_msgs->bounds.h;
    bs.y = b->h/2 - bs.h - 50;
    
    SetBounds_GLWidget(wid_info->alert_msgs,&bs);
    
    
    widget->bounds.x = b->x;
    widget->bounds.w = b->w;
    widget->bounds.y = b->y;
    widget->bounds.h = b->h;
    
}

extern int Console_isVisible(void);
extern void Paste_String_to_Console(char *text);
static void button_MainWidget( Uint8 button, Uint8 state , Uint16 x , Uint16 y, void *data )
{
    int scraplen;
    
    if (!data) return;

    if (state == SDL_PRESSED) {
	if (button == 1) {
	    Key_press(KEY_POINTER_CONTROL);
	}
	if (button == 2) {
    	    if (Console_isVisible()) {
	    	scraptarget = NULL;
	    	get_scrap(TextScrap('T','E','X','T'), &scraplen, &scrap);
		if ( scraplen == 0 ) return;
		Paste_String_to_Console(scrap);
	    }
	}
    }
}

static void Close_MainWidget ( GLWidget *widget )
{
    if (!widget) return;
    if (widget->WIDGET !=MAINWIDGET) {
    	error("Wrong widget type for Close_MainWidget [%i]",widget->WIDGET);
	return;
    }
    
    if ( scrap ) free(scrap);
}

GLWidget *Init_MainWidget( font_data *font )
{
    GLWidget *tmp;
    WrapperWidget *wid_info;
    SDL_Rect b;
    
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc GLWidget in Init_MainWidget");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(WrapperWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc MainWidget in Init_MainWidget");
	return NULL;
    }
    wid_info = ((WrapperWidget *)tmp->wid_info);
    wid_info->font	= font;
    wid_info->BORDER	= 10;
    wid_info->showconf	= true;
    
    tmp->WIDGET     	= MAINWIDGET;
    tmp->bounds.w   	= draw_width;
    tmp->bounds.h   	= draw_height;
    tmp->SetBounds 	= SetBounds_MainWidget;
    tmp->button     	= button_MainWidget;
    tmp->buttondata 	= tmp;
    tmp->Close	    	= Close_MainWidget;
    
    if ( !AppendGLWidgetList(&(tmp->children),
    	    (wid_info->game_msgs = Init_ListWidget(wid_info->BORDER,tmp->bounds.h-wid_info->BORDER,
	    &nullRGBA,&nullRGBA,&greenRGBA,LW_UP,LW_RIGHT,VERTICAL,true)))
	) {
	error("Failed to initialize game msg list");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !AppendGLWidgetList(&(tmp->children),
    	    (wid_info->alert_msgs = Init_ListWidget(tmp->bounds.w/2,tmp->bounds.h/2-50,
	    &nullRGBA,&nullRGBA,&greenRGBA,LW_UP,LW_HCENTER,VERTICAL,false)))
	) {
	error("Failed to initialize alert msg list");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->radar = Init_RadarWidget())) ) {
	error("radar initialization failed");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->scorelist = Init_ScorelistWidget())) ) {
	error("scorelist initialization failed");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !AppendGLWidgetList(&(tmp->children),(wid_info->confmenu = Init_ConfMenuWidget(0,0))) ) {
	error("confmenu initialization failed");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !AppendGLWidgetList(&(tmp->children),
    	    	(wid_info->chat_msgs = Init_ListWidget(wid_info->radar->bounds.w + 2*wid_info->BORDER,wid_info->BORDER,
		&nullRGBA,&nullRGBA,&greenRGBA,LW_DOWN,LW_RIGHT,VERTICAL,false)))
    	) {
	error("Failed to initialize chat msg list");
	Close_Widget(&tmp);
	return NULL;
    }
    
    b.w = wid_info->confmenu->bounds.w;
    b.h = wid_info->confmenu->bounds.h;
    b.x = tmp->bounds.w - b.w - 16;
    b.y = tmp->bounds.h - b.h - 16;
    SetBounds_GLWidget(wid_info->confmenu,&b);
    
    return tmp;
}
/*******************/
/* End: MainWidget */
/*******************/

/**************************/
/* Begin: ConfMenuWidget  */
/**************************/
static Uint32 cm_name_color    = 0xffff66ff;
static Uint32 cm_bg1_color     = 0x00000022;
static Uint32 cm_bg2_color     = 0xffffff22;
static Uint32 cm_but1_color    = 0xff000088;

static void Paint_ConfMenuWidget( GLWidget *widget );
static void ConfMenuWidget_Quit( void *data );
static void ConfMenuWidget_Save( void *data );
static void ConfMenuWidget_Join( void *data );
static void ConfMenuWidget_Pause( void *data );
static void ConfMenuWidget_Join_Team( void *data );
static void ConfMenuWidget_Config( void *data );

static void confmenu_callback( void )
{
    GLWidget *list;
    WrapperWidget *mw;
    ConfMenuWidget *cm;
    ScrollPaneWidget *sp;
    
    if ( !(mw = (WrapperWidget *)(MainWidget->wid_info)) ) {
    	error("confmenu_callback: MainWidget missing!");
	return;
    }
    if ( !(cm = (ConfMenuWidget*)(mw->confmenu->wid_info)) ) {
    	error("confmenu_callback: confmenu missing!");
	return;
    }
    if ( !(sp = (ScrollPaneWidget *)(cm->scrollpane->wid_info)) ) {
    	error("confmenu_callback: scrollpane missing!");
	return;
    }
    list = sp->content;
    if ( !list ) {
    	error("confmenu_callback: list missing!");
	return;
    }

    SetBounds_GLWidget(list,&(list->bounds));
    SetBounds_GLWidget(mw->confmenu,&(mw->confmenu->bounds));
}

static void ConfMenuWidget_Quit( void *data )
{
    SDL_Event quit;
    quit.type = SDL_QUIT;
    SDL_PushEvent(&quit);
}

static void ConfMenuWidget_Save( void *data )
{
    char path[PATH_MAX + 1];

    Xpilotrc_get_filename(path, sizeof(path));
    Xpilotrc_write(path);
}

typedef struct {
    GLWidget	*widget;
    int	    	team;
} Join_Team_Data;

static void ConfMenuWidget_Join_Team( void *data )
{
    char msg[16];
    GLWidget *widget;
    ConfMenuWidget *wid_info;
    
    if (!(widget = ((Join_Team_Data *)data)->widget)) {
	error("ConfMenuWidget_Join_Team: widget missing!");
	return;
    }
    if ( widget->WIDGET != CONFMENUWIDGET ) {
	error("ConfMenuWidget_Join_Team: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if ( !(wid_info = (ConfMenuWidget *)(widget->wid_info)) ) {
	error("ConfMenuWidget_Join_Team: wid_info missing!");
	return;
    }
    
    if (wid_info->join_list) {
    	ListWidget_Remove(wid_info->main_list,wid_info->join_list);
    	Close_Widget(&(wid_info->join_list));
    	SetBounds_GLWidget(wid_info->main_list,&(wid_info->main_list->bounds));
    	widget->bounds.x = wid_info->main_list->bounds.x - 1;
    	widget->bounds.y = wid_info->main_list->bounds.y - 1;
    	widget->bounds.w = wid_info->main_list->bounds.w + 2;
    	widget->bounds.h = wid_info->main_list->bounds.h + 2;
    	SetBounds_GLWidget(widget,&(widget->bounds));
    }
    
    snprintf(msg, sizeof(msg), "/team %d", ((Join_Team_Data *)data)->team);
    Net_talk(msg);
    
    Pointer_control_set_state(true);
}

static void ConfMenuWidget_Pause( void *data )
{
    GLWidget *widget;
    ConfMenuWidget *wid_info;
    bool change;
    
    if (!(widget = (GLWidget *)data)) {
	error("ConfMenuWidget_Pause: widget missing!");
	return;
    }
    if ( widget->WIDGET != CONFMENUWIDGET ) {
	error("ConfMenuWidget_Pause: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if ( !(wid_info = (ConfMenuWidget *)(widget->wid_info)) ) {
	error("ConfMenuWidget_Pause: wid_info missing!");
	return;
    }
    
    if (wid_info->join_list) {
    	ListWidget_Remove(wid_info->main_list,wid_info->join_list);
    	Close_Widget(&(wid_info->join_list));
    	SetBounds_GLWidget(wid_info->main_list,&(wid_info->main_list->bounds));
    	widget->bounds.x = wid_info->main_list->bounds.x - 1;
    	widget->bounds.y = wid_info->main_list->bounds.y - 1;
    	widget->bounds.w = wid_info->main_list->bounds.w + 2;
    	widget->bounds.h = wid_info->main_list->bounds.h + 2;
    	SetBounds_GLWidget(widget,&(widget->bounds));
    }

    change = false;
    change |= Key_press(KEY_PAUSE);
    if (change) Net_key_change();
    change = false;
    change |= Key_release(KEY_PAUSE);
    if (change) Net_key_change();
}
static void ConfMenuWidget_Join( void *data )
{
    GLWidget *widget;
    ConfMenuWidget *wid_info;
    static Join_Team_Data jtd[MAX_TEAMS];
    
    if (!(widget = (GLWidget *)data)) {
	error("ConfMenuWidget_Join: widget missing!");
	return;
    }
    if ( widget->WIDGET != CONFMENUWIDGET ) {
	error("ConfMenuWidget_Join: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if ( !(wid_info = (ConfMenuWidget *)(widget->wid_info)) ) {
	error("ConfMenuWidget_Join: wid_info missing!");
	return;
    }
    
    if ((Setup->mode & TEAM_PLAY) == 0) {
    	bool change = false;
	change |= Key_press(KEY_PAUSE);
	if (change) Net_key_change();
	change = false;
	change |= Key_release(KEY_PAUSE);
	if (change) Net_key_change();
    } else {
    	int i, t;
    	char tstr[12];
    	bool has_base[MAX_TEAMS];
	GLWidget *tmp;
	
	if (wid_info->join_list) {
	    ListWidget_Remove(wid_info->main_list,wid_info->join_list);
	    Close_Widget(&(wid_info->join_list));

    	    widget->bounds.x = wid_info->main_list->bounds.x - 1;
    	    widget->bounds.y = wid_info->main_list->bounds.y - 1;
    	    widget->bounds.w = wid_info->main_list->bounds.w + 2;
    	    widget->bounds.h = wid_info->main_list->bounds.h + 2;
    	    SetBounds_GLWidget(widget,&(widget->bounds));

	    return;
	}
    	
    	memset(has_base, 0, sizeof(has_base));
    	if ((Setup->mode & TEAM_PLAY) != 0) {
    	    for (i = 0; i < num_bases; i++) {
    	    	t = bases[i].team;
	    	    if (t >= 0 && t < MAX_TEAMS)
	            	has_base[t] = true;
            }
    	}
    
    	if (!(wid_info->join_list = Init_ListWidget(0,0,&cm_bg1_color,&cm_bg2_color,&nullRGBA,LW_DOWN,LW_RIGHT,VERTICAL,false))) {
    	    error("ConfMenuWidget_Join: Couldn't make the join teams list widget!");
    	    return;
    	}
    	
	for (i = 0; i < MAX_TEAMS; i++) {
    	    if (has_base[i]) {
	    	snprintf(tstr, sizeof(tstr), "Join Team %i", i);
		jtd[i].widget = widget;
		jtd[i].team = i;
    	    	if ( !(tmp = Init_LabelButtonWidget(tstr,&greenRGBA,&cm_bg2_color,&cm_but1_color,1,ConfMenuWidget_Join_Team,&(jtd[i]))) ) {
    	    	    error("ConfMenuWidget_Join: Couldn't make the join team labelButton!");
	    	    return;
    	    	}
		
		tmp->bounds.w = widget->bounds.w - 2;
		
		SetBounds_GLWidget(tmp,&(tmp->bounds));
		
		ListWidget_Append(wid_info->join_list,tmp);
	    }
    	}
	
    	if (self && strchr("P", self->mychar)) {
	    snprintf(tstr, sizeof(tstr), "Unpause");
	} else {
	    snprintf(tstr, sizeof(tstr), "Pause");
	}
	
    	if ( !(tmp = Init_LabelButtonWidget(tstr,&redRGBA,&cm_bg2_color,&cm_but1_color,1,ConfMenuWidget_Pause,widget)) ) {
    	    error("ConfMenuWidget_Join: Couldn't make the Pause labelButton!");
    	    return;
    	}
		
    	tmp->bounds.w = widget->bounds.w - 2;
		
    	SetBounds_GLWidget(tmp,&(tmp->bounds));
		
    	ListWidget_Append(wid_info->join_list,tmp);
	
	
	ListWidget_Append(wid_info->main_list,wid_info->join_list);
	
    	widget->bounds.x = wid_info->main_list->bounds.x - 1;
    	widget->bounds.y = wid_info->main_list->bounds.y - 1;
    	widget->bounds.w = wid_info->main_list->bounds.w + 2;
    	widget->bounds.h = wid_info->main_list->bounds.h + 2;
    	SetBounds_GLWidget(widget,&(widget->bounds));
    }
}

static void ConfMenuWidget_Config( void *data )
{
    GLWidget *widget;
    ConfMenuWidget *wid_info;
    
    if (!(widget = (GLWidget *)data)) {
	error("ConfMenuWidget_Close: widget missing!");
	return;
    }
    if ( widget->WIDGET != CONFMENUWIDGET ) {
	error("ConfMenuWidget_Close: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if ( !(wid_info = (ConfMenuWidget *)(widget->wid_info)) ) {
	error("ConfMenuWidget_Close: wid_info missing!");
	return;
    }
    
    if ((wid_info->showconf = !wid_info->showconf)) {
    	if (wid_info->join_list) {
	    ListWidget_Remove(wid_info->main_list,wid_info->join_list);
	    Close_Widget(&(wid_info->join_list));
	}
    	ListWidget_Remove(wid_info->button_list,wid_info->jlb);
    	ListWidget_Append(wid_info->button_list,wid_info->slb);
    	ListWidget_Append(wid_info->main_list,wid_info->scrollpane);
    } else {
     	ListWidget_Append(wid_info->button_list,wid_info->jlb);
    	ListWidget_Remove(wid_info->button_list,wid_info->slb);
   	ListWidget_Remove(wid_info->main_list,wid_info->scrollpane);
    }
    
    SetBounds_GLWidget(wid_info->main_list,&(wid_info->main_list->bounds));
    
    widget->bounds.x = wid_info->main_list->bounds.x - 1;
    widget->bounds.y = wid_info->main_list->bounds.y - 1;
    widget->bounds.w = wid_info->main_list->bounds.w + 2;
    widget->bounds.h = wid_info->main_list->bounds.h + 2;
    SetBounds_GLWidget(widget,&(widget->bounds));
}

static void SetBounds_ConfMenuWidget( GLWidget *widget, SDL_Rect *b )
{
    ConfMenuWidget *wid_info;
    SDL_Rect bounds = {0,0,0,0};
    
    if (!widget ) {
	error("SetBounds_ConfMenuWidget: tried to change bounds on NULL ConfMenuWidget!");
	return;
    }
    if ( widget->WIDGET != CONFMENUWIDGET ) {
	error("SetBounds_ConfMenuWidget: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if (!(wid_info = (ConfMenuWidget *)(widget->wid_info))) {
	error("SetBounds_ConfMenuWidget: wid_info missing!");
	return;
    }
    if (!b ) {
	error("SetBounds_ConfMenuWidget: tried to set NULL bounds on ConfMenuWidget!");
	return;
    }
        
    widget->bounds.x = b->x;
    widget->bounds.y = b->y;
    widget->bounds.w = b->w;
    widget->bounds.h = b->h;

    bounds.x += b->x + 1;
    bounds.y += b->y + 1;
    
    if (!wid_info->main_list) {
	error("SetBounds_ConfMenuWidget: main_list missing!");
	return;
    }
    bounds.w = wid_info->main_list->bounds.w;
    bounds.h = wid_info->main_list->bounds.h;
    
    SetBounds_GLWidget(wid_info->main_list,&bounds);
}

static void Paint_ConfMenuWidget( GLWidget *widget )
{
    Uint32 edgeColor = 0xff0000ff;
    Uint32 bgColor = 0x0000ff88;
    ConfMenuWidget *wid_info;

    if (!widget ) {
    	error("Paint_ConfMenuWidget: tried to paint NULL ConfMenuWidget!");
	return;
    }
    
    if (!(wid_info = (ConfMenuWidget *)(widget->wid_info))) {
	error("Paint_ConfMenuWidget: wid_info missing!");
	return;
    }
        
    if ((Setup->mode & TEAM_PLAY) == 0) {
    	if (self && strchr("P", self->mychar)) {
    	    if ( !wid_info->paused ) {
    	    	ListWidget_Remove(wid_info->button_list,wid_info->jlb);
	    	Close_Widget(&(wid_info->jlb));
    	    	if ( !(wid_info->jlb = Init_LabelButtonWidget("Join",&greenRGBA,&nullRGBA,&cm_but1_color,1,ConfMenuWidget_Join,widget)) ) {
    	    	    error("Paint_ConfMenuWidget: Couldn't make the join labelButton!");
	    	    return;
	    	}
    	    	SetBounds_GLWidget(wid_info->jlb, &(wid_info->clb->bounds));
    	    	ListWidget_Append(wid_info->button_list,wid_info->jlb);
	    	wid_info->paused = true;
    	    }
    	} else {
    	    if ( wid_info->paused ) {
    	    	ListWidget_Remove(wid_info->button_list,wid_info->jlb);
	    	Close_Widget(&(wid_info->jlb));
    	    	if ( !(wid_info->jlb = Init_LabelButtonWidget("Pause",&greenRGBA,&nullRGBA,&cm_but1_color,50,ConfMenuWidget_Pause,widget)) ) {
    	    	    error("Paint_ConfMenuWidget: Couldn't make the Pause labelButton!");
	    	    return;
    	    	}
    	    	SetBounds_GLWidget(wid_info->jlb, &(wid_info->clb->bounds));
    	    	ListWidget_Append(wid_info->button_list,wid_info->jlb);
	    	wid_info->paused = false;
	    }
    	}
    }
    
    set_alphacolor(bgColor);
    glBegin(GL_QUADS);
    	glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y	    	    	);
    	glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y+widget->bounds.h	);
    	glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    glEnd();
    glBegin(GL_LINE_LOOP);
    	set_alphacolor(edgeColor);
    	glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y	    	    	);
    	set_alphacolor(bgColor | 0x000000ff);
    	glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y	    	    	);
    	set_alphacolor(edgeColor);
    	glVertex2i(widget->bounds.x+widget->bounds.w,widget->bounds.y+widget->bounds.h	);
    	set_alphacolor(bgColor | 0x000000ff);
    	glVertex2i(widget->bounds.x 	    	    ,widget->bounds.y+widget->bounds.h	);
    glEnd();
}

GLWidget *Init_ConfMenuWidget( Uint16 x, Uint16 y )
{
    GLWidget *tmp, *item, *dummy, *list;
    ConfMenuWidget *wid_info;
    int i;
    xp_option_t *opt;
    
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_ConfMenu");
	return NULL;
    }
    tmp->wid_info   	= malloc(sizeof(ConfMenuWidget));
    if ( !(tmp->wid_info) ) {
    	free(tmp);
        error("Failed to malloc in Init_ConfMenu");
	return NULL;
    }
    wid_info = ((ConfMenuWidget *)(tmp->wid_info));
    
    tmp->WIDGET     	= CONFMENUWIDGET;
    tmp->Draw	    	= Paint_ConfMenuWidget;
    tmp->SetBounds  	= SetBounds_ConfMenuWidget;
    wid_info->showconf	= false;
    wid_info->join_list	= NULL;
        
    dummy = Init_EmptyBaseGLWidget();
    if ( !dummy ) {
        error("Failed to malloc in Init_ConfMenu");
	return NULL;
    }
    
    for ( i=0 ; i < num_options; ++i ) {
    	opt = Option_by_index(i);
	item = Init_OptionWidget(opt,&cm_name_color, &nullRGBA);
	if (item) {
	    AppendGLWidgetList( &(dummy->next), item );
	}
    }

    if (!(list = Init_ListWidget(0,0,&cm_bg1_color,&cm_bg2_color,&nullRGBA,LW_DOWN,LW_RIGHT,VERTICAL,false))) {
    	error("Init_ConfMenuWidget: Couldn't make the list widget!");
	Close_WidgetTree(&dummy);
	Close_Widget(&tmp);
	return NULL;
    }
    
    ListWidget_Append(list,dummy->next);
    Close_Widget(&dummy);
    
    if ( !(wid_info->scrollpane = Init_ScrollPaneWidget(list)) ) {
    	error("Init_ConfMenuWidget: Couldn't make the scrollpane!");
	Close_Widget(&list);
	Close_Widget(&tmp);
	return NULL;
    }
    
    wid_info->scrollpane->bounds.h = 512;
    
    if ( !(wid_info->slb = Init_LabelButtonWidget("Save",&greenRGBA,&nullRGBA,&cm_but1_color,1,ConfMenuWidget_Save,tmp)) ) {
    	error("Init_ConfMenuWidget: Couldn't make the save labelButton!");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if (self && strchr("P", self->mychar))
    	wid_info->paused = true;
    else
    	wid_info->paused = false;

    if ((Setup->mode & TEAM_PLAY) || wid_info->paused) {
    	if ( !(wid_info->jlb = Init_LabelButtonWidget("Join",&greenRGBA,&nullRGBA,&cm_but1_color,1,ConfMenuWidget_Join,tmp)) ) {
    	    error("Init_ConfMenuWidget: Couldn't make the join labelButton!");
	    Close_Widget(&tmp);
	    return NULL;
    	}
    } else {
    	if ( !(wid_info->jlb = Init_LabelButtonWidget("Pause",&greenRGBA,&nullRGBA,&cm_but1_color,50,ConfMenuWidget_Pause,tmp)) ) {
    	    error("Init_ConfMenuWidget: Couldn't make the Pause labelButton!");
	    Close_Widget(&tmp);
	    return NULL;
    	}
    }
    
    if ( !(wid_info->clb = Init_LabelButtonWidget("Config",&yellowRGBA,&nullRGBA,&cm_but1_color,1,ConfMenuWidget_Config,tmp)) ) {
    	error("Init_ConfMenuWidget: Couldn't make the config labelButton!");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if ( !(wid_info->qlb = Init_LabelButtonWidget("Quit",&redRGBA,&nullRGBA,&cm_but1_color,1,ConfMenuWidget_Quit,tmp)) ) {
    	error("Init_ConfMenuWidget: Couldn't make the quit label!");
	Close_Widget(&tmp);
	return NULL;
    }
    
    if (!(wid_info->button_list = Init_ListWidget(0,0,&nullRGBA,&nullRGBA,&nullRGBA,LW_DOWN,LW_RIGHT,HORISONTAL,true))) {
    	error("Init_ConfMenuWidget: Couldn't make the button_list widget!");
	Close_Widget(&tmp);
	return NULL;
    }
    
    wid_info->clb->bounds.w = ( wid_info->scrollpane->bounds.w + 10 )/3 + 1;
    SetBounds_GLWidget(wid_info->clb, &(wid_info->clb->bounds));
    SetBounds_GLWidget(wid_info->slb, &(wid_info->clb->bounds));
    SetBounds_GLWidget(wid_info->jlb, &(wid_info->clb->bounds));
    SetBounds_GLWidget(wid_info->qlb, &(wid_info->clb->bounds));
    
    
    ListWidget_Append(wid_info->button_list,wid_info->qlb);
    ListWidget_Append(wid_info->button_list,wid_info->clb);
    ListWidget_Append(wid_info->button_list,wid_info->jlb);
    
    if (!(wid_info->main_list = Init_ListWidget(0,0,&nullRGBA,&nullRGBA,&nullRGBA,LW_UP,LW_LEFT,VERTICAL,true))) {
    	error("Init_ConfMenuWidget: Couldn't make the main_list widget!");
	Close_Widget(&tmp);
	return NULL;
    }
    
    ListWidget_Append(wid_info->main_list,wid_info->button_list);

    if (!AppendGLWidgetList( &(tmp->children), wid_info->main_list )) {
    	error("Init_ConfMenuWidget: failed to append main_list to children");
	Close_Widget(&tmp);
	return NULL;
    }
    
    tmp->bounds.x = x;
    tmp->bounds.y = y;
    tmp->bounds.w = wid_info->main_list->bounds.w + 2;
    tmp->bounds.h = wid_info->main_list->bounds.h + 2;
    
    SetBounds_GLWidget(tmp,&(tmp->bounds));
       
    return tmp;
}
/***********************/
/* End: ConfMenuWidget */
/***********************/

/****************************/
/* Begin: ImageButtonWidget */
/****************************/

static void Button_ImageButtonWidget(Uint8 button, Uint8 state, Uint16 x, 
			      Uint16 y, void *data)
{
    GLWidget *widget;
    ImageButtonWidget *info;

    widget = (GLWidget*)data;
    if (widget->WIDGET != IMAGEBUTTONWIDGET) {
	error("expected IMAGEBUTTONWIDGET got [%d]", widget->WIDGET);
	return;
    }
    info = (ImageButtonWidget*)widget->wid_info;
    if (info->state == state) return;
    info->state = state;

    if (state != SDL_PRESSED && info->onClick) {
	if (x >= widget->bounds.x
	    && x <= widget->bounds.x + widget->bounds.w
	    && y >= widget->bounds.y
	    && y <= widget->bounds.y + widget->bounds.h)
	    info->onClick(widget);
    }
}

static void Close_ImageButtonWidget(GLWidget *widget)
{
    ImageButtonWidget *info;
    if (!widget) return;
    if (widget->WIDGET != IMAGEBUTTONWIDGET) {
    	error("Wrong widget type for Close_ImageButtonWidget [%i]",
	      widget->WIDGET);
	return;
    }
    info = (ImageButtonWidget*)widget->wid_info;
    free_string_texture(&(info->tex));
    if (info->imageUp) glDeleteTextures(1, &(info->imageUp));
    if (info->imageDown) glDeleteTextures(1, &(info->imageDown));
}

static void Paint_ImageButtonWidget(GLWidget *widget)
{
    SDL_Rect *b;
    ImageButtonWidget *info;
    int x, y, c;

    if (!widget) return;
     
    b = &(widget->bounds);
    info = (ImageButtonWidget*)(widget->wid_info);

    if (info->state != SDL_PRESSED) {
	if (info->imageUp) {
	    set_alphacolor(info->bg);
	    glBindTexture(GL_TEXTURE_2D, info->imageUp);
	    glEnable(GL_TEXTURE_2D);
	    glBegin(GL_QUADS);
	    glTexCoord2f(info->txcUp.MinX, info->txcUp.MinY); 
	    glVertex2i(b->x, b->y);
	    glTexCoord2f(info->txcUp.MaxX, info->txcUp.MinY); 
	    glVertex2i(b->x + b->w , b->y);
	    glTexCoord2f(info->txcUp.MaxX, info->txcUp.MaxY); 
	    glVertex2i(b->x + b->w , b->y + b->h);
	    glTexCoord2f(info->txcUp.MinY, info->txcUp.MaxY); 
	    glVertex2i(b->x, b->y + b->h);
	    glEnd();
	}
    } else {
	if (info->imageDown) {
	    set_alphacolor(info->bg);
	    glBindTexture(GL_TEXTURE_2D, info->imageDown);
	    glEnable(GL_TEXTURE_2D);
	    glBegin(GL_QUADS);
	    glTexCoord2f(info->txcDown.MinX, info->txcDown.MinY); 
	    glVertex2i(b->x, b->y);
	    glTexCoord2f(info->txcDown.MaxX, info->txcDown.MinY); 
	    glVertex2i(b->x + b->w , b->y);
	    glTexCoord2f(info->txcDown.MaxX, info->txcDown.MaxY); 
	    glVertex2i(b->x + b->w , b->y + b->h);
	    glTexCoord2f(info->txcDown.MinY, info->txcDown.MaxY); 
	    glVertex2i(b->x, b->y + b->h);
	    glEnd();
	}
    }
    
    x = widget->bounds.x + widget->bounds.w / 2;
    y = widget->bounds.y + widget->bounds.h / 2;
    c = (int)(info->fg ? info->fg : whiteRGBA);
    if (info->state == SDL_PRESSED) {
	x += 1;
	y += 1;
    }
    disp_text(&(info->tex), c,
	      CENTER, CENTER, 
	      x, draw_height - y, 
	      true);
}

GLWidget *Init_ImageButtonWidget(const char *text,
				 const char *upImage,
				 const char *downImage,
				 Uint32 bg, 
				 Uint32 fg,
				 void (*onClick)(GLWidget *widget))
{
    GLWidget *tmp;
    ImageButtonWidget *info;
    SDL_Surface *surface;
    char imagePath[256];
    int width, height;
    
    if (!text) {
    	error("text missing for Init_ImageButtonWidget.");
	return NULL;
    }
    tmp	= Init_EmptyBaseGLWidget();
    if ( !tmp ) {
        error("Failed to malloc in Init_ImageButtonWidget");
	return NULL;
    }
    info = XMALLOC(ImageButtonWidget, 1);
    if (!info) {
    	free(tmp);
        error("Failed to malloc in Init_ImageButtonWidget");
	return NULL;
    }

    info->onClick = onClick;
    info->fg = fg;
    info->bg = bg;
    info->state = SDL_RELEASED;
    info->imageUp = 0;
    info->imageDown = 0;

    if (!render_text(&gamefont, text, &(info->tex))) {
    	free(info);
    	free(tmp);
        error("Failed to render text in Init_ImageButtonWidget");
	return NULL;
    }
    width = info->tex.width + 1;
    height = info->tex.height + 1;

#ifdef HAVE_SDL_IMAGE
    sprintf(imagePath, "%s%s", CONF_TEXTUREDIR, upImage);
    surface = IMG_Load(imagePath);
    if (surface) {
	info->imageUp = SDL_GL_LoadTexture(surface, &(info->txcUp));
	if (width < surface->w) width = surface->w;
	if (height < surface->h) height = surface->h;
	SDL_FreeSurface(surface);
    } else {
	error("Failed to load button image %s", imagePath);
    }
    sprintf(imagePath, "%s%s", CONF_TEXTUREDIR, downImage);
    surface = IMG_Load(imagePath);
    if (surface) {
	info->imageDown = SDL_GL_LoadTexture(surface, &(info->txcDown));
	if (width < surface->w) width = surface->w;
	if (height < surface->h) height = surface->h;
	SDL_FreeSurface(surface);
    } else {
	error("Failed to load button image %s", imagePath);
    }
#endif

    tmp->WIDGET     	= IMAGEBUTTONWIDGET;
    tmp->wid_info       = info;
    tmp->bounds.w   	= width;
    tmp->bounds.h   	= height;
    tmp->Draw	    	= Paint_ImageButtonWidget;
    tmp->Close     	= Close_ImageButtonWidget;
    tmp->button         = Button_ImageButtonWidget;
    tmp->buttondata     = tmp;
    return tmp;
}
/**************************/
/* End: ImageButtonWidget */
/**************************/

/*****************************/
/* Begin: LabelButtonWidget  */
/*****************************/
static void SetBounds_LabelButtonWidget( GLWidget *widget, SDL_Rect *b );

static void SetBounds_LabelButtonWidget( GLWidget *widget, SDL_Rect *b )
{
    LabelButtonWidget *wid_info;
    
    if (!widget ) {
	error("SetBounds_LabelButtonWidget: tried to change bounds on NULL ImageButtonWidget!");
	return;
    }
    if ( widget->WIDGET != LABELBUTTONWIDGET ) {
	error("SetBounds_LabelButtonWidget: Wrong widget type! [%i]",widget->WIDGET);
	return;
    }
    if (!(wid_info = (LabelButtonWidget *)(widget->wid_info))) {
	error("SetBounds_LabelButtonWidget: wid_info missing!");
	return;
    }
    if (!b ) {
	error("SetBounds_LabelButtonWidget: tried to set NULL bounds on LabelButtonWidget!");
	return;
    }
        
    widget->bounds.x = b->x;
    widget->bounds.w = b->w;
    widget->bounds.y = b->y;
    widget->bounds.h = b->h;
    
    SetBounds_GLWidget(wid_info->label,&(widget->bounds));
    SetBounds_GLWidget(wid_info->button,&(widget->bounds));
}

GLWidget *Init_LabelButtonWidget(   const char *text,
				    Uint32 *text_color,
    	    	    	    	    Uint32 *bg_color,
    	    	    	    	    Uint32 *active_color,
    	    	    	    	    Uint8 depress_time,
    	    	    	    	    void (*action)(void *data),
				    void *actiondata)
{
    GLWidget *widget;
    LabelButtonWidget *wid_info;
    
    widget	= Init_EmptyBaseGLWidget();
    if ( !widget ) {
        error("Failed to malloc in Init_LabelButtonWidget");
	return NULL;
    }
    widget->wid_info   	= malloc(sizeof(LabelButtonWidget));
    if ( !(widget->wid_info) ) {
    	free(widget);
        error("Failed to malloc in Init_LabelButtonWidget");
	return NULL;
    }
    wid_info = ((LabelButtonWidget *)(widget->wid_info));

    widget->WIDGET     	= LABELBUTTONWIDGET;
    widget->SetBounds  	= SetBounds_LabelButtonWidget;
    
    wid_info->label = Init_LabelWidget( text , text_color, &nullRGBA, CENTER, CENTER );
    
    if (!AppendGLWidgetList(&(widget->children),wid_info->label)) {
    	error("Init_LabelButtonWidget: Could not initialize label widget!");
	Close_Widget(&widget);
	return NULL;
    } 

    wid_info->button = Init_ButtonWidget( bg_color, active_color, depress_time, action, actiondata);
    
    if (!AppendGLWidgetList(&(widget->children),wid_info->button)) {
    	error("Init_LabelButtonWidget: Could not initialize button widget!");
	Close_Widget(&widget);
	return NULL;
    }

    SetBounds_GLWidget(widget,&(wid_info->label->bounds));
    
    return widget;
}
/**************************/
/* End: LabelButtonWidget */
/**************************/
