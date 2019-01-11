/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
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

#ifndef WIDGET_H
#define WIDGET_H

#define NO_WIDGET		0	/* Not a widget descriptor */

void Widget_destroy_children(int widget_desc);
void Widget_destroy(int widget_desc);
Window Widget_window(int widget_desc);
void Widget_draw(int widget_desc);
int Widget_event(XEvent *event);
int Widget_create_form(int parent_desc, Window parent_window,
		       int x, int y, int width, int height,
		       int border);
int Widget_create_activate(int parent_desc,
			   int x, int y, int width, int height,
			   int border,
			   const char *str,
			   int (*callback)(int, void *, const char **),
			   void *user_data);
int Widget_create_bool(int parent_desc,
		       int x, int y, int width, int height,
		       int border,
		       bool val, int (*callback)(int, void *, bool *),
		       void *user_data);
int Widget_add_pulldown_entry(int menu_desc, const char *str,
			      int (*callback)(int, void *, const char **),
			      void *user_data);
int Widget_create_menu(int parent_desc,
		       int x, int y, int width, int height,
		       int border, const char *str);
int Widget_create_int(int parent_desc,
		      int x, int y, int width, int height,
		      int border, int *val, int min, int max,
		      int (*callback)(int, void *, int *),
		      void *user_data);
int Widget_create_color(int parent_desc, int color,
 		        int x, int y, int width, int height,
 		        int border, int *val, int min, int max,
 		        int (*callback)(int, void *, int *),
 		        void *user_data);
int Widget_create_double(int parent_desc,
			 int x, int y, int width, int height,
			 int border, double *val, double min, double max,
			 int (*callback)(int, void *, double *),
			 void *user_data);
int Widget_create_label(int parent_desc,
			int x, int y,
			int width, int height, bool centered,
			int border, const char *str);
int Widget_create_colored_label(int parent_desc,
				int x, int y,
				int width, int height, bool centered,
				int border, int bg, int bord,
				const char *str);
int Widget_create_arrow_right(int parent_desc, int x, int y,
			      int width, int height,
			      int border,
			      int related_desc);
int Widget_create_arrow_left(int parent_desc, int x, int y,
			     int width, int height,
			     int border, int related_desc);
int Widget_create_popup(int width, int height, int border,
			const char *window_name, const char *icon_name);
int Widget_create_confirm(const char *confirm_str,
			  int (*callback)(int, void *, const char **));
int Widget_backing_store(int widget_desc, int mode);
int Widget_set_background(int widget_desc, int bgcolor);
int Widget_map_sub(int widget_desc);
int Widget_map(int widget_desc);
int Widget_raise(int widget_desc);
int Widget_get_dimensions(int widget_desc, int *width, int *height);
int Widget_unmap(int widget_desc);
int Widget_create_viewer(const char *buf, int len,
			 int width, int height, int border,
			 const char *window_name, const char *icon_name,
			 XFontStruct *font);
int Widget_update_viewer(int popup_desc, const char *buf, int len);

#endif
