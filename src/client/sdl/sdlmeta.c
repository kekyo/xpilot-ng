/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 by
 *
 *      Juha Lindström       <juhal@users.sourceforge.net>
 *      Darel Cullen         <darelcullen@users.sourceforge.net>
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

#include "sdlmeta.h"
#include "sdlwindow.h"
#include "text.h"
#include "glwidgets.h"

#define EVENT_JOIN 0
#define EVENT_REFRESH 1

#define SELECTED_BG 0x009000ff
#define ROW_FG 0xffff00ff
#define ROW_BG1 0x0000a0ff
#define ROW_BG2 0x000070ff
#define HEADER_FG 0xffff00ff
#define HEADER_BG 0xff0000ff
#define STATUS_ROWS 7
#define STATUS_COLS 4
#define STATUS_FIELD_FG 0xffff00ff
#define STATUS_FIELD_BG 0xff0000ff
#define STATUS_VALUE_FG 0xffff00ff
#define STATUS_VALUE_BG 0x000070ff
#define PLIST_HEADER_FG 0xffff00ff
#define PLIST_HEADER_BG 0xff0000ff
#define PLIST_ITEM_FG 0xffff00ff
#define PLIST_ITEM_BG 0x000070ff
#define ROW_HEIGHT 19
#define VERSION_WIDTH 100
#define COUNT_WIDTH 20
#define META_WIDTH 837
#define META_HEIGHT 768
#define BUTTON_BG 0xff0000ff
#define BUTTON_FG 0xffff00ff

#define METAWIDGET        100
#define METATABLEWIDGET   101
#define METAROWWIDGET     102
#define METAHEADERWIDGET  103
#define STATUSWIDGET      104
#define PLAYERLISTWIDGET  105

static int status_column_widths[] = { 100, 0, 100, 70 };

typedef struct {
    GLWidget   *table;
    GLWidget   *status;
    GLWidget   *players;
    GLuint     texture;
    texcoord_t txc;
} MetaWidget;

typedef struct {
    list_t                server_list;
    GLWidget              *meta;
    GLWidget	          *scrollbar;
    GLWidget              *header;
    struct _MetaRowWidget *selected;
    struct _MetaRowWidget *first_row;
} MetaTableWidget;

typedef struct _MetaRowWidget {
    Uint32          fg;
    Uint32          bg;
    MetaTableWidget *table;
    server_info_t   *sip;
    bool            is_selected;
} MetaRowWidget;

typedef struct {
    Uint32 fg;
    Uint32 bg;
} MetaHeaderWidget;

typedef struct {
    Uint32         name_fg, name_bg, value_fg, value_bg;
    char           address_str[22];
    char           fps_str[4];
    server_info_t  *sip;
} StatusWidget;

typedef struct {
    Uint32   header_fg, header_bg, item_fg, item_bg;
    GLWidget *header;
    GLWidget *scrollbar;
    list_t   players;
    char     *players_str;
} PlayerListWidget;


static void Scroll_PlayerListWidget(GLfloat pos, void *data)
{
    PlayerListWidget *info;
    GLWidget *widget, *row;
    SDL_Rect b;
    int y;
    
    widget = (GLWidget*)data;
    if (widget->WIDGET != PLAYERLISTWIDGET) {
	error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
	return;
    }

    info = (PlayerListWidget*)widget->wid_info;
    y = widget->bounds.y + ROW_HEIGHT
	- (int)(List_size(info->players) * ROW_HEIGHT * pos);

    for (row = widget->children; row; row = row->next) {
	if (row->WIDGET == LABELWIDGET && row != info->header) {
	    b = row->bounds;
	    b.y = y;
	    SetBounds_GLWidget(row, &b);
	    y += ROW_HEIGHT;
	}
    }
}

static void SetBounds_PlayerListWidget(GLWidget *widget, SDL_Rect *b)
{
    int y;
    GLWidget *row;
    PlayerListWidget *info;
    GLfloat list_height;
    SDL_Rect *wb, sb, rb, hb;

    if (widget->WIDGET != PLAYERLISTWIDGET) {
	error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
	return;
    }

    widget->bounds = *b;
    info = (PlayerListWidget*)widget->wid_info;
    list_height = List_size(info->players) * ROW_HEIGHT + ROW_HEIGHT;

    if (info->scrollbar != NULL) {
	DelGLWidgetListItem(&(widget->children), info->scrollbar);
	Close_Widget(&(info->scrollbar));
	info->scrollbar = NULL;
    }

    y = b->y + ROW_HEIGHT;
    for (row = widget->children; row; row = row->next) {
	if (row->WIDGET == LABELWIDGET && row != info->header) {
	    rb.x = b->x + 1;
	    rb.y = y;
	    rb.w = b->w - 11;
	    rb.h = ROW_HEIGHT;
	    SetBounds_GLWidget(row, &rb);
	    y += ROW_HEIGHT;
	}
    }

    if (list_height > b->h) {
	info->scrollbar = 
	    Init_ScrollbarWidget(false, 0.0f, ((GLfloat)b->h) / list_height, 
				 SB_VERTICAL, Scroll_PlayerListWidget, widget);
	if (info->scrollbar != NULL) {
	    wb = &(widget->bounds);
	    sb.x = wb->x + wb->w - 10;
	    sb.y = wb->y + ROW_HEIGHT; 
	    sb.w = 10;
	    sb.h = wb->h - ROW_HEIGHT;
	    SetBounds_GLWidget(info->scrollbar, &sb);
	    AppendGLWidgetList(&(widget->children), info->scrollbar);
	} else {
	    error("failed to create a scroll bar for player list");
	    return;
	}
    }
    if (info->header != NULL) {
	hb.x = widget->bounds.x + 1;
	hb.y = widget->bounds.y - 1;
	hb.w = widget->bounds.w - 10;
	hb.h = ROW_HEIGHT;
	SetBounds_GLWidget(info->header, &hb);
    }
}

static list_t create_player_list(char *players_str)
{
    list_t players;
    char *t;

    if (!(players = List_new())) return NULL;
    for (t = strtok(players_str, ","); t; t = strtok(NULL, ","))
	if (!(List_push_back(players, t))) 
	    break;
    
    return players;
}

static void Close_PlayerListWidget(GLWidget *widget) 
{
    PlayerListWidget *info;

    if (widget->WIDGET != PLAYERLISTWIDGET) {
	error("expected PLAYERLISTWIDGET got [%d]", widget->WIDGET);
	return;
    }
    
    info = (PlayerListWidget*)widget->wid_info;
    if (info->players_str) free(info->players_str);
    if (info->players) List_delete(info->players);
    info->players_str = NULL;
    info->players = NULL;
}

static void Paint_PlayerListWidget(GLWidget *widget)
{
    SDL_Rect *b = &(widget->bounds);
    set_alphacolor(PLIST_ITEM_BG);
    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w - 10, b->y);
    glVertex2i(b->x + b->w - 10, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
}

static GLWidget *Init_PlayerListWidget(server_info_t *sip)
{
    char *player, *players_str;
    list_t players;
    list_iter_t iter;
    GLWidget *tmp, *header, *row;
    PlayerListWidget *info;

    if (!(players_str = strdup(sip->playlist))) {
	error("out of memory");
	return NULL;
    }
    if (!(players = create_player_list(players_str))) {
	error("failed to create players list");
	free(players_str);
	return NULL;
    }
    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	free(players_str);
	List_delete(players);
	return NULL;
    }
    if (!(info = (PlayerListWidget*)malloc(sizeof(PlayerListWidget)))) {
        error("out of memory");
	free(players_str);
	List_delete(players);
	free(tmp);
	return NULL;
    }
    if (!(header = 
	  Init_LabelWidget("Players", 
			   &(info->header_fg),
			   &(info->header_bg), 
			   CENTER,CENTER))) {
	error("failed to create header for player list");
	free(players_str);
	List_delete(players);
	free(tmp);
	free(info);
	return NULL;
    }

    info->players_str   = players_str;
    info->players       = players;
    info->scrollbar     = NULL;
    info->header        = header;
    info->header_fg     = PLIST_HEADER_FG;
    info->header_bg     = PLIST_HEADER_BG;
    info->item_fg       = PLIST_ITEM_FG;
    info->item_bg       = PLIST_ITEM_BG;

    tmp->wid_info       = info;
    tmp->WIDGET     	= PLAYERLISTWIDGET;
    tmp->SetBounds      = SetBounds_PlayerListWidget;
    tmp->Draw           = Paint_PlayerListWidget;
    tmp->Close          = Close_PlayerListWidget;

    for (iter = List_begin(players); 
	 iter != List_end(players); 
	 LI_FORWARD(iter)) {
	player = (char*)SI_DATA(iter);
	row = Init_LabelWidget(player,
			       &(info->item_fg),
			       &(info->item_bg),
			       LEFT,CENTER);
	if (!row) break;
	AppendGLWidgetList(&(tmp->children), row);
    }
    AppendGLWidgetList(&(tmp->children), header);

    return tmp;
}

static void compute_layout(SDL_Rect *wb, int *width, int *xoff)
{
    int i, x, w, free_space, num_dynamic, w_dynamic;

    num_dynamic = 0;
    free_space = wb->w;
    for (i = 0; i < STATUS_COLS; i++) {
	w = status_column_widths[i];
	if (w > 5) {
	    free_space -= w;
	} else {
	    num_dynamic++;
	}
    }
    w_dynamic = free_space / num_dynamic - 5;
    x = 0;
    for (i = 0; i < STATUS_COLS; i++) {
	w = status_column_widths[i];
	xoff[i] = x;
	width[i] = w > 5 ? w - 5 : w_dynamic;
	x += width[i] + 5;
    }
}

static void SetBounds_StatusWidget(GLWidget *widget, SDL_Rect *wb)
{
    int i, c, rowc, rowi, colc, coli;
    int col_width[4];
    int col_xoff[4];
    SDL_Rect b;
    GLWidget *w;
    
    widget->bounds = *wb;
    compute_layout(wb, col_width, col_xoff);
    
    for (c = 0, w = widget->children; w; w = w->next) c++;

    rowc = STATUS_ROWS;
    colc = STATUS_COLS;
    b.h = ROW_HEIGHT;

    for (i = 0, w = widget->children; w && i < colc * rowc; w = w->next) {
	coli = (i % 2) + (i / 2 / rowc) * 2;
	rowi = (i / 2) % rowc;
	b.w = col_width[coli];
	b.x = wb->x + col_xoff[coli];
	b.y = wb->y + rowi * ROW_HEIGHT;
	SetBounds_GLWidget(w, &b);
	i++;
    }
}

static void add_status_entry(const char *name, char *value, GLWidget *parent)
{
    GLWidget *name_label, *value_label;
    StatusWidget *info;

    info = (StatusWidget*)parent->wid_info;
    
    if ((name_label = 
	 Init_LabelWidget(name, 
			  &(info->name_fg),
			  &(info->name_bg), 
			  LEFT,CENTER))) {
	AppendGLWidgetList(&(parent->children), name_label);
    }
    if ((value_label = 
	 Init_LabelWidget(value, 
			  &(info->value_fg),
			  &(info->value_bg), 
			  LEFT,CENTER))) {
	AppendGLWidgetList(&(parent->children), value_label);
    }
}

static GLWidget *Init_StatusWidget(server_info_t *sip)
{
    GLWidget *tmp;
    StatusWidget *info;

    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return NULL;
    }
    if (!(info = (StatusWidget*)malloc(sizeof(StatusWidget)))) {
        error("out of memory");
	free(tmp);
	return NULL;
    }
    sprintf(info->address_str, "%s:%u", sip->ip_str, sip->port);
    sprintf(info->fps_str, "%u", sip->fps);
    info->sip           = sip;
    info->name_fg       = STATUS_FIELD_FG;
    info->name_bg       = STATUS_FIELD_BG;
    info->value_fg      = STATUS_VALUE_FG;
    info->value_bg      = STATUS_VALUE_BG;
    tmp->wid_info       = info;
    tmp->WIDGET     	= STATUSWIDGET;
    tmp->SetBounds      = SetBounds_StatusWidget;

    add_status_entry(" Server", sip->hostname, tmp);
    add_status_entry(" Address", info->address_str, tmp);
    add_status_entry(" Version", sip->version, tmp);
    add_status_entry(" Map name", sip->mapname, tmp);
    add_status_entry(" Map size", sip->mapsize, tmp);
    add_status_entry(" Map author", sip->author, tmp);
    add_status_entry(" Status", sip->status, tmp);
    add_status_entry(" Bases", sip->bases_str, tmp);
    add_status_entry(" Teams", sip->teambases_str, tmp);
    add_status_entry(" Free bases", sip->freebases, tmp);
    add_status_entry(" Queue", sip->queue_str, tmp);
    add_status_entry(" FPS", info->fps_str, tmp);
    add_status_entry(" Sound", sip->sound, tmp);
    add_status_entry(" Timing", sip->timing, tmp);

    return tmp;
}

static void SelectRow_MetaWidget(GLWidget *widget, MetaRowWidget *row)
{
    MetaWidget *meta;
    MetaTableWidget *table;
    SDL_Rect status_bounds, plist_bounds;

    if (widget->WIDGET != METAWIDGET) {
	error("expected METAWIDGET got [%d]", widget->WIDGET);
	return;
    }
    meta = (MetaWidget*)widget->wid_info;
    if (meta->status != NULL) {
	DelGLWidgetListItem(&(widget->children), meta->status);
	Close_Widget(&(meta->status));
	meta->status = NULL;
    }
    status_bounds.x = widget->bounds.x + 22;
    status_bounds.y = widget->bounds.y + 598;
    status_bounds.w = 792 * 3 / 5;
    status_bounds.h = ROW_HEIGHT * STATUS_ROWS;
    if ((meta->status = Init_StatusWidget(row->sip))) {
	SetBounds_GLWidget(meta->status, &status_bounds);
	AppendGLWidgetList(&(widget->children), meta->status);
    } else {
	error("failed to create a status widget");
    }
    if (meta->players != NULL) {
	DelGLWidgetListItem(&(widget->children), meta->players);
	Close_Widget(&(meta->players));
	meta->players = NULL;
    }
    plist_bounds.x = status_bounds.x + status_bounds.w;
    plist_bounds.y = status_bounds.y;
    plist_bounds.h = status_bounds.h;
    plist_bounds.w = 792 - status_bounds.w;
    if ((meta->players = Init_PlayerListWidget(row->sip))) {
	SetBounds_GLWidget(meta->players, &plist_bounds);
	AppendGLWidgetList(&(widget->children), meta->players);
    } else {
	error("failed to create a player list widget");
    }
    row->is_selected = true;
    table = (MetaTableWidget*)meta->table->wid_info;
    if (table->selected)
	table->selected->is_selected = false;
    table->selected = row;
}

static server_info_t *GetSelectedServer_MetaWidget(GLWidget *widget)
{
    MetaWidget *meta;

    if (widget->WIDGET != METAWIDGET) {
	error("expected METAWIDGET got [%d]", widget->WIDGET);
	return NULL;
    }
    meta = (MetaWidget*)widget->wid_info;
    return ((MetaTableWidget*)meta->table->wid_info)->selected->sip;
}

static void Paint_MetaRowWidget(GLWidget *widget)
{
    SDL_Rect *b;
    MetaRowWidget *row;

    if (widget->WIDGET != METAROWWIDGET) {
	error("expected METAROWWIDGET got [%d]", widget->WIDGET);
	return;
    }

    b = &(widget->bounds);
    row = (MetaRowWidget*)widget->wid_info;
    set_alphacolor(row->is_selected ? SELECTED_BG : row->bg);

    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();
}

static void SetBounds_MetaRowWidget(GLWidget *row, SDL_Rect *rb)
{
    int free_width;
    SDL_Rect cb;
    GLWidget *col;

    row->bounds = *rb;
    free_width = MAX(rb->w - (VERSION_WIDTH + COUNT_WIDTH), 0);

    if (!(col = row->children)) return;
    cb.x = rb->x; 
    cb.w = free_width / 2;
    cb.y = rb->y; cb.h = rb->h;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next)) return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y; cb.h = rb->h;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next)) return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y; cb.h = rb->h;
    cb.w = VERSION_WIDTH;
    SetBounds_GLWidget(col, &cb);

    if (!(col = col->next)) return;
    cb.x = cb.x + cb.w;
    cb.y = rb->y; cb.h = rb->h;
    cb.w = COUNT_WIDTH;
    SetBounds_GLWidget(col, &cb);
}

static void Button_MetaRowWidget(Uint8 button, Uint8 state, Uint16 x, 
				 Uint16 y, void *data)
{
    GLWidget *widget;
    MetaRowWidget *row;
    SDL_Event evt;

    if (state != SDL_PRESSED) return;
    if (button != 1) return;

    widget = (GLWidget*)data;
    if (widget->WIDGET != METAROWWIDGET) {
	error("expected METAROWWIDGET got [%d]", widget->WIDGET);
	return;
    }

    row = (MetaRowWidget*)widget->wid_info;
    if (row->is_selected) {
	evt.type = SDL_USEREVENT;
	evt.user.code = EVENT_JOIN;
	evt.user.data1 = row->sip;
	SDL_PushEvent(&evt);
    } else {
	SelectRow_MetaWidget(row->table->meta, row);
#if 0
	printf("version: %s\n", row->sip->version);
	printf("hostname: %s\n", row->sip->hostname);
	printf("users_str: %s\n", row->sip->users_str);
	printf("mapname: %s\n", row->sip->mapname);
	printf("mapsize: %s\n", row->sip->mapsize);
	printf("author: %s\n", row->sip->author);
	printf("status: %s\n", row->sip->status);
	printf("bases_str: %s\n", row->sip->bases_str);
	printf("fps_str: %s\n", row->sip->fps_str);
	printf("playlist: %s\n", row->sip->playlist);
	printf("sound: %s\n", row->sip->sound);
	printf("teambases_str: %s\n", row->sip->teambases_str);
	printf("timing: %s\n", row->sip->timing);
	printf("ip_str: %s\n", row->sip->ip_str);
	printf("freebases: %s\n", row->sip->freebases);
	printf("queue_str: %s\n", row->sip->queue_str);
	printf("domain: %s\n", row->sip->domain);
	printf("pingtime_str: %s\n", row->sip->pingtime_str);
	printf("port: %u\n", row->sip->port);
	printf("ip: %u\n", row->sip->ip);
	printf("users: %u\n", row->sip->users);
	printf("bases: %u\n", row->sip->bases);
	printf("fps: %u\n", row->sip->fps);
	printf("uptime: %u\n", row->sip->uptime);
	printf("queue: %u\n", row->sip->queue);
	printf("pingtime: %u\n", row->sip->pingtime);
	printf("serial: %c\n", row->sip->serial);
#endif
    }
}

static GLWidget *Init_MetaRowWidget(server_info_t *sip, 
				    MetaTableWidget *table, 
				    bool is_selected,
				    unsigned int bg)
{
    GLWidget *tmp, *col;
    MetaRowWidget *row;

    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return NULL;
    }
    if (!(row = (MetaRowWidget*)malloc(sizeof(MetaRowWidget)))) {
        error("out of memory");
	free(tmp);
	return NULL;
    }
    row->fg             = ROW_FG;
    row->bg             = bg;
    row->sip            = sip;
    row->table          = table;
    row->is_selected    = is_selected;
    tmp->wid_info       = row;
    tmp->WIDGET     	= METAROWWIDGET;
    tmp->Draw	    	= Paint_MetaRowWidget;
    tmp->SetBounds      = SetBounds_MetaRowWidget;
    tmp->button         = Button_MetaRowWidget;
    tmp->buttondata     = tmp;

#define COLUMN(TEXT) \
    if ((col = Init_LabelWidget((TEXT), &(row->fg), NULL, LEFT, CENTER))) { \
        col->button = Button_MetaRowWidget; \
        col->buttondata = tmp; \
	AppendGLWidgetList(&(tmp->children), col); \
    }
    COLUMN(sip->hostname);
    COLUMN(sip->mapname);
    COLUMN(sip->version);
    COLUMN(sip->users_str);
#undef COLUMN

    return tmp;
}

static GLWidget *Init_MetaHeaderWidget(void)
{
    GLWidget *tmp, *col;
    MetaHeaderWidget *info;

    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return NULL;
    }
    if (!(info = (MetaHeaderWidget*)malloc(sizeof(MetaHeaderWidget)))) {
	error("out of memory");
	free(tmp);
	return NULL;
    }
    info->fg       = HEADER_FG;
    info->bg       = HEADER_BG;
    tmp->wid_info  = info;
    tmp->WIDGET    = METAHEADERWIDGET;
    tmp->SetBounds = SetBounds_MetaRowWidget;

#define HEADER(TEXT) \
    if ((col = Init_LabelWidget((TEXT), &(info->fg), &(info->bg), LEFT, CENTER))) { \
	AppendGLWidgetList(&(tmp->children), col); \
    }
    HEADER("Server");
    HEADER("Map");
    HEADER("Version");
    HEADER("Pl");
#undef COLUMN

    return tmp;    
}

static void Scroll_MetaTableWidget(GLfloat pos, void *data)
{
    MetaTableWidget *info;
    GLWidget *widget, *row;
    SDL_Rect b;
    int y;
    
    widget = (GLWidget*)data;
    if (widget->WIDGET != METATABLEWIDGET) {
	error("expected METATABLEWIDGET got [%d]", widget->WIDGET);
	return;
    }

    info = (MetaTableWidget*)widget->wid_info;
    y = widget->bounds.y + ROW_HEIGHT
	- (int)(List_size(info->server_list) * ROW_HEIGHT * pos);

    for (row = widget->children; row; row = row->next) {
	if (row->WIDGET == METAROWWIDGET) {
	    b = row->bounds;
	    b.y = y;
	    SetBounds_GLWidget(row, &b);
	    y += ROW_HEIGHT;
	}
    }
}

static void SetBounds_MetaTableWidget(GLWidget *widget, SDL_Rect *b)
{
    int y;
    GLWidget *row;
    MetaTableWidget *info;
    GLfloat table_height;
    SDL_Rect *wb, sb, rb, hb;

    if (widget->WIDGET != METATABLEWIDGET) {
	error("expected METATABLEWIDGET got [%d]", widget->WIDGET);
	return;
    }

    widget->bounds = *b;
    info = (MetaTableWidget*)widget->wid_info;
    table_height = List_size(info->server_list) * ROW_HEIGHT;

    if (info->scrollbar != NULL) {
	DelGLWidgetListItem(&(widget->children), info->scrollbar);
	Close_Widget(&(info->scrollbar));
	info->scrollbar = NULL;
    }

    y = b->y + ROW_HEIGHT;
    for (row = widget->children; row; row = row->next) {
	if (row->WIDGET == METAROWWIDGET) {
	    rb.x = b->x;
	    rb.y = y;
	    rb.w = b->w - 10;
	    rb.h = ROW_HEIGHT;
	    SetBounds_GLWidget(row, &rb);
	    y += ROW_HEIGHT;
	}
    }

    if (table_height > b->h) {
	info->scrollbar = 
	    Init_ScrollbarWidget(false, 0.0f, ((GLfloat)b->h) / table_height, 
				 SB_VERTICAL, Scroll_MetaTableWidget, widget);
	if (info->scrollbar != NULL) {
	    wb = &(widget->bounds);
	    sb.x = wb->x + wb->w - 10;
	    sb.y = wb->y + ROW_HEIGHT; 
	    sb.w = 10;
	    sb.h = wb->h - ROW_HEIGHT;
	    SetBounds_GLWidget(info->scrollbar, &sb);
	    AppendGLWidgetList(&(widget->children), info->scrollbar);
	} else {
	    error("failed to create a scroll bar for meta table");
	    return;
	}
    }
    if (info->header != NULL) {
	hb.x = widget->bounds.x;
	hb.y = widget->bounds.y - 1; /* don't ask why */
	hb.w = widget->bounds.w - 10;
	hb.h = ROW_HEIGHT;
	SetBounds_GLWidget(info->header, &hb);
    }
}

static GLWidget *Init_MetaTableWidget(GLWidget *meta, list_t servers)
{
    GLWidget *tmp, *row;
    list_iter_t iter;
    server_info_t *sip;
    MetaTableWidget *info;
    bool bg = true;

    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return NULL;
    }
    if (!(info = (MetaTableWidget*)malloc(sizeof(MetaTableWidget)))) {
	error("out of memory");
	free(tmp);
	return NULL;
    }
    info->meta          = meta;
    info->server_list   = servers;
    info->scrollbar     = NULL;
    info->header        = NULL;
    info->selected      = NULL;
    info->first_row     = NULL;

    tmp->wid_info       = info;
    tmp->WIDGET     	= METATABLEWIDGET;
    tmp->SetBounds      = SetBounds_MetaTableWidget;
    
    for (iter = List_begin(servers); 
	 iter != List_end(servers); 
	 LI_FORWARD(iter)) {
	sip = SI_DATA(iter);
	row = Init_MetaRowWidget(sip, info, false, bg ? ROW_BG1 : ROW_BG2);
	if (!row) break;
	if (info->first_row == NULL) 
	    info->first_row = (MetaRowWidget*)row->wid_info;
	AppendGLWidgetList(&(tmp->children), row);
	bg = !bg;
    }
    if ((info->header = Init_MetaHeaderWidget())) {
	AppendGLWidgetList(&(tmp->children), info->header);
    } else {
	error("failed to create a header row for meta table");
    }

    return tmp;
}

static void Paint_MetaWidget(GLWidget *widget)
{
    MetaWidget *info;
    SDL_Rect *b;

    if (widget->WIDGET != METAWIDGET) {
	error("expected METAWIDGET got [%d]", widget->WIDGET);
	return;
    }
    info = (MetaWidget*)widget->wid_info;
    if (info->texture == 0) return;
    
    b = &(widget->bounds);
    glColor4ub(255, 255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, info->texture);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(info->txc.MinX, info->txc.MinY); 
    glVertex2i(b->x, b->y);
    glTexCoord2f(info->txc.MaxX, info->txc.MinY); 
    glVertex2i(b->x + b->w , b->y);
    glTexCoord2f(info->txc.MaxX, info->txc.MaxY); 
    glVertex2i(b->x + b->w , b->y + b->h);
    glTexCoord2f(info->txc.MinY, info->txc.MaxY); 
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

static void Close_MetaWidget(GLWidget *widget)
{
    MetaWidget *info;

    if (widget->WIDGET != METAWIDGET) {
	error("expected METAWIDGET got [%d]", widget->WIDGET);
	return;
    }
    info = (MetaWidget*)widget->wid_info;
    if (info->texture) glDeleteTextures(1, &(info->texture));
}

static void OnClick_Join(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_USEREVENT;
    evt.user.code = EVENT_JOIN;
    evt.user.data1 = NULL;
    SDL_PushEvent(&evt);
}

static void OnClick_Refresh(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_USEREVENT;
    evt.user.code = EVENT_REFRESH;
    evt.user.data1 = NULL;
    SDL_PushEvent(&evt);
}

static void OnClick_Quit(GLWidget *widget)
{
    SDL_Event evt;
    evt.type = SDL_QUIT;
    SDL_PushEvent(&evt);
}

static GLWidget *Init_MetaWidget(list_t servers)
{
    GLWidget *tmp;
    MetaWidget *info;
    MetaTableWidget *table;
    SDL_Rect table_bounds;
    SDL_Surface *surface;

    if (!(tmp = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return NULL;
    }
    if (!(info = (MetaWidget*)malloc(sizeof(MetaWidget)))) {
	error("out of memory");
	free(tmp);
	return NULL;
    }
    info->table         = NULL;
    info->status        = NULL;
    info->players       = NULL;
    info->texture       = 0;
    tmp->WIDGET     	= METAWIDGET;
    tmp->bounds.x       = (draw_width - META_WIDTH) / 2;
    tmp->bounds.y       = (draw_height - META_HEIGHT) / 2;
    tmp->bounds.w       = META_WIDTH;
    tmp->bounds.h       = META_HEIGHT;
    tmp->wid_info       = info;
    tmp->Draw           = Paint_MetaWidget;
    tmp->Close          = Close_MetaWidget;

    if (!(info->table = Init_MetaTableWidget(tmp, servers))) {
	free(tmp);
	return NULL;
    }
    table = (MetaTableWidget*)info->table->wid_info;
    table_bounds.x = tmp->bounds.x + 20;
    table_bounds.y = tmp->bounds.y + 64;
    table_bounds.w = 796;
    table_bounds.h = 514;
    SetBounds_GLWidget(info->table, &table_bounds);
    AppendGLWidgetList(&(tmp->children), info->table);

#ifdef HAVE_SDL_IMAGE
    surface = IMG_Load(CONF_TEXTUREDIR "sdlmetabg.png");
    if (surface) {
	info->texture = SDL_GL_LoadTexture(surface, &(info->txc));
	SDL_FreeSurface(surface);
    }
#endif

    if (table->first_row != NULL)
	SelectRow_MetaWidget(tmp, table->first_row);

    return tmp;
}

static bool join_server(Connect_param_t *conpar, server_info_t *sip)
{
    char *server_addr_ptr = conpar->server_addr;
    strlcpy(conpar->server_name, sip->hostname,
            sizeof(conpar->server_name));
    strlcpy(conpar->server_addr, sip->ip_str, sizeof(conpar->server_addr));
    conpar->contact_port = sip->port;
    if (Contact_servers(1, &server_addr_ptr, 1, 0, 0, NULL,
			0, NULL, NULL, NULL, NULL, conpar)) {
	return true;
    }
    printf("Server %s (%s) didn't respond on port %d\n",
	   conpar->server_name, conpar->server_addr,
	   conpar->contact_port);
    return false;
}

static void handleKeyPress(GLWidget *meta, SDL_keysym *keysym )
{
    /*static unsigned int row = 1;*/
    SDL_Event evt;
    
    switch ( keysym->sym )
    {
    case SDLK_ESCAPE:
	/* ESC key was pressed */
	evt.type = SDL_QUIT;
	SDL_PushEvent(&evt);
	break;
    case SDLK_F11:
	/* F11 key was pressed
	 * this toggles fullscreen mode
	 */
#ifndef _WINDOWS
		/* This segfaults */
		/* SDL_WM_ToggleFullScreen(MainSDLSurface); */
#endif
	break;
    case SDLK_UP: 
	/* move the cursor up */
	break;
    case SDLK_DOWN: 
	/* move the curor down */
	break;
    default:
	break;
    }
    
    return;
}

int Meta_window(Connect_param_t *conpar)
{
    static char err[MSG_LEN] = {0};
    int num_serv = 0;
    int btn_x, btn_y, btn_h;
    GLWidget *btn, *root, *meta, *target = NULL;
    SDL_Event evt;
    server_info_t *server = NULL;
    
    if (!server_list ||
	List_size(server_list) < 10 ||
	server_list_creation_time + 5 < time(NULL)) {
	
	Delete_server_list();
	if ((num_serv = Get_meta_data(err)) <= 0) {
	    error("Couldn't get meta list.");
	    return -1;
	} else
	    warn("Got %d servers.", num_serv);
    }
    
    if (Welcome_sort_server_list() == -1) {
	Delete_server_list();
	error("out of memory");
	return -1;
    }

    if (!(root = Init_EmptyBaseGLWidget())) {
        error("Widget init failed");
	return -1;
    }
    root->bounds.x = root->bounds.y = 0;
    root->bounds.w = draw_width;
    root->bounds.w = draw_height;

    if (!(meta = Init_MetaWidget(server_list))) {
	free(root);
	return -1;
    }
    AppendGLWidgetList(&(root->children), meta);

    btn_x = (draw_width - META_WIDTH) / 2 - 80;
    btn_y = (draw_height - META_HEIGHT) / 2 + 60;
    btn = Init_ImageButtonWidget("Join", 
				 "metabtnup.png", 
				 "metabtndown.png",
				 BUTTON_BG,
				 BUTTON_FG,
				 OnClick_Join);
    if (btn) {
	btn->bounds.x = btn_x;
	btn->bounds.y = btn_y;
	btn_h = btn->bounds.h;
	btn_y += btn_h + 5;
	AppendGLWidgetList(&(root->children), btn);
    }
    btn = Init_ImageButtonWidget("Refresh", 
				 "metabtnup.png", 
				 "metabtndown.png",
				 BUTTON_BG,
				 BUTTON_FG,
				 OnClick_Refresh);
    if (btn) {
	btn->bounds.x = btn_x;
	btn->bounds.y = btn_y;
	btn_h = btn->bounds.h;
	btn_y += btn_h + 5;
	AppendGLWidgetList(&(root->children), btn);
    }
    btn = Init_ImageButtonWidget("Quit", 
				 "metabtnup.png", 
				 "metabtndown.png",
				 BUTTON_BG,
				 BUTTON_FG,
				 OnClick_Quit);
    if (btn) {
	btn->bounds.x = btn_x;
	btn->bounds.y = btn_y;
	btn_h = btn->bounds.h;
	btn_y += btn_h + 5;
	AppendGLWidgetList(&(root->children), btn);
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, draw_width, draw_height, 0);
    glDisable(GL_BLEND);
    
    while(1) {

	set_alphacolor(blackRGBA);
	glBegin(GL_QUADS);
	glVertex2i(0,0);
	glVertex2i(draw_width,0);
	glVertex2i(draw_width,draw_height);
	glVertex2i(0,draw_height);
	glEnd();
	glEnable(GL_SCISSOR_TEST);
	DrawGLWidgetsi(meta, 0, 0, draw_width, draw_height);
	glDisable(GL_SCISSOR_TEST);
	SDL_GL_SwapBuffers();
	
	SDL_WaitEvent(&evt);
	do {
	    
	    switch(evt.type) {
	    case SDL_QUIT: 
		return -1;
		
	    case SDL_USEREVENT:
		if (evt.user.code == EVENT_JOIN) {
		    server = (server_info_t*)evt.user.data1;
		    if (server == NULL)
			server = GetSelectedServer_MetaWidget(meta);
		    if (!server) break;
		    if (join_server(conpar, server)) {
			Close_Widget(&root);
			glEnable(GL_BLEND);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, draw_width, 0, draw_height);
			return 0;
		    }
		} else if (evt.user.code == EVENT_REFRESH) {
		    Close_Widget(&root);
		    return 1;
		}
		break;

	    case SDL_KEYDOWN:
	        /* handle key presses */
	        handleKeyPress( meta, &evt.key.keysym );
	        break;

	    case SDL_MOUSEBUTTONDOWN:
		target = FindGLWidgeti(meta, evt.button.x, evt.button.y);
		if (target && target->button)
		    target->button(evt.button.button, 
				   evt.button.state,
				   evt.button.x,
				   evt.button.y,
				   target->buttondata);
		break;
		
	    case SDL_MOUSEBUTTONUP:
		if (target && target->button) {
		    target->button(evt.button.button, 
				   evt.button.state,
				   evt.button.x,
				   evt.button.y,
				   target->buttondata);
		    target = NULL;
		}
		break;
		
	    case SDL_MOUSEMOTION:
		if (target && target->motion)
		    target->motion(evt.motion.xrel,
				   evt.motion.yrel,
				   evt.button.x,
				   evt.button.y,
				   target->motiondata);
		break;

	    case SDL_VIDEOEXPOSE:
		glDisable(GL_SCISSOR_TEST);
		set_alphacolor(blackRGBA);
		glBegin(GL_QUADS);
		glVertex2i(0,0);
		glVertex2i(draw_width,0);
		glVertex2i(draw_width,draw_height);
		glVertex2i(0,draw_height);
		glEnd();
		glEnable(GL_SCISSOR_TEST);
		break;
	    }
	} while (SDL_PollEvent(&evt));
    }	
}
