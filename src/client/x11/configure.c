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
 * Copyright (C) 2003-2004 Kristian Söderblom <kps@users.sourceforge.net>
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
 * Configure.c: real-time option control.
 */

#include "xpclient_x11.h"

static int Config_creator(xp_option_t *opt, int widget_desc, int *height);
static int Config_create_save(int widget_desc, int *height);
static int Config_close(int widget_desc, void *data, const char **strptr);
static int Config_next(int widget_desc, void *data, const char **strptr);
static int Config_prev(int widget_desc, void *data, const char **strptr);
static int Config_save(int widget_desc, void *data, const char **strptr);
static int Config_save_confirm_callback(int widget_desc, void *popup_desc,
					const char **strptr);

static int num_default_options = 0;
static int max_default_options = 0;
static int *default_option_indices = NULL;
static int num_color_options = 0;
static int max_color_options = 0;
static int *color_option_indices = NULL;

static bool		config_created = false,
			config_mapped = false;
static int		config_page,
			config_x,
			config_y,
			config_width,
			config_height,
			config_space,
			config_max,
			config_button_space,
			config_text_space,
			config_text_height,
			config_button_height,
			config_entry_height,
			config_bool_width,
			config_bool_height,
			config_int_width,
			config_double_width,
			config_arrow_width,
			config_arrow_height;
static int		*config_widget_desc,
			config_save_confirm_desc = NO_WIDGET;

static int *config_widget_ids = NULL;
static int config_what = CONFIG_NONE;

/* this must be updated if new config menu items are added */
static int Nelem_config_creator(void)
{
    if (config_what == CONFIG_DEFAULT)
	return num_default_options + 1;
    if (config_what == CONFIG_COLORS)
	return num_color_options + 1;
    return 0;
}

static xp_option_t *Config_creator_option(int i)
{
    int ind = -1;

    if (config_what == CONFIG_DEFAULT
	&& i >= 0 && i < num_default_options)
	ind = default_option_indices[i];
    if (config_what == CONFIG_COLORS
	&& i >= 0 && i < num_color_options)
	ind = color_option_indices[i];
    return Option_by_index(ind);
}

static int Update_bool_option(int widget_desc, void *data, bool *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    UNUSED_PARAM(widget_desc);
    Set_bool_option(opt, *val, xp_option_origin_config);

    return 0;
}

static int Update_int_option(int widget_desc, void *data, int *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    UNUSED_PARAM(widget_desc);
    Set_int_option(opt, *val, xp_option_origin_config);

    return 0;
}

static int Update_double_option(int widget_desc, void *data, double *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    UNUSED_PARAM(widget_desc);
    Set_double_option(opt, *val, xp_option_origin_config);

    return 0;
}

static void Create_config(void)
{
    int			i,
			num,
			height,
			offset,
			width,
			widget_desc;
    bool		full;

    /*
     * Window dimensions relative to the top window.
     */
    config_x = 0;
    config_y = RadarHeight + ButtonHeight + 2;
    config_width = 256;
    config_height = top_height - config_y;

    /*
     * Space between label-text and label-border.
     */
    config_text_space = 3;
    /*
     * Height of a label window.
     */
    config_text_height = 2 * 1 + textFont->ascent + textFont->descent;

    /*
     * Space between button-text and button-border.
     */
    config_button_space = 3;
    /*
     * Height of a button window.
     */
    config_button_height = buttonFont->ascent + buttonFont->descent + 2 * 1;

    config_entry_height = MAX(config_text_height, config_button_height);

    /*
     * Space between entries and between an entry and the border.
     */
    config_space = 6;

    /*
     * Sizes of the different widget types.
     */
    config_bool_width = XTextWidth(buttonFont, "Yes", 3)
			+ 2 * config_button_space;
    config_bool_height = config_button_height;
    config_arrow_height = config_text_height;
    config_arrow_width = config_text_height;
    config_int_width = 4 + XTextWidth(buttonFont, "10000", 5);
    config_double_width = 4 + XTextWidth(buttonFont, "0.222", 5);

    config_max = Nelem_config_creator();
    config_widget_desc = XMALLOC(int, config_max);
    if (config_widget_desc == NULL) {
	error("No memory for config");
	return;
    }

    num = -1;
    full = true;
    for (i = 0; i < Nelem_config_creator(); i++) {
	xp_option_t *opt = Config_creator_option(i);

	if (full) {
	    full = false;
	    num++;
	    config_widget_desc[num]
		= Widget_create_form(NO_WIDGET, topWindow,
				     config_x, config_y,
				     config_width, config_height,
				     0);
	    if (config_widget_desc[num] == 0)
		break;

	    height = config_height - config_space - config_button_height;
	    width = 2 * config_button_space + XTextWidth(buttonFont,
							  "PREV", 4);
	    offset = (config_width - width) / 2;
	    widget_desc =
		Widget_create_activate(config_widget_desc[num],
				       offset, height,
				       width, config_button_height,
				       0, "PREV", Config_prev,
				       (void *)(long)num);
	    if (widget_desc == 0)
		break;

	    width = 2 * config_button_space + XTextWidth(buttonFont,
							  "NEXT", 4);
	    offset = config_width - width - config_space;
	    widget_desc =
		Widget_create_activate(config_widget_desc[num],
				       offset, height,
				       width, config_button_height,
				       0, "NEXT", Config_next,
				       (void *)(long)num);
	    if (widget_desc == 0)
		break;

	    width = 2 * config_button_space + XTextWidth(buttonFont,
							  "CLOSE", 5);
	    offset = config_space;
	    widget_desc =
		Widget_create_activate(config_widget_desc[num],
				       offset, height,
				       width, config_button_height,
				       0, "CLOSE", Config_close,
				       (void *)(long)num);
	    if (widget_desc == 0)
		break;

	    height = config_space;
	}

	if (opt == NULL && i != Nelem_config_creator()-1)
	    assert(0);

	if ((config_widget_ids[i] =
	     Config_creator(opt, config_widget_desc[num], &height)) == 0) {
	    i--;
	    full = true;
	    if (height == config_space)
		break;
	    continue;
	}

    }
    if (i < Nelem_config_creator()) {
	for (; num >= 0; num--) {
	    if (config_widget_desc[num] != 0)
		Widget_destroy(config_widget_desc[num]);
	}
	config_created = false;
	config_mapped = false;
    } else {
	config_max = num + 1;
	config_widget_desc = XREALLOC(int, config_widget_desc, config_max);
	config_page = 0;
	for (i = 0; i < config_max; i++)
	    Widget_map_sub(config_widget_desc[i]);
	config_created = true;
	config_mapped = false;
    }
}

static int Config_close(int widget_desc, void *data, const char **strptr)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(strptr);
    Widget_unmap(config_widget_desc[config_page]);
    config_mapped = false;
    return 0;
}

static int Config_next(int widget_desc, void *data, const char **strptr)
{
    int			prev_page = config_page;

    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(strptr);
    if (config_max > 1) {
	config_page = (config_page + 1) % config_max;
	Widget_raise(config_widget_desc[config_page]);
	Widget_unmap(config_widget_desc[prev_page]);
	config_mapped = true;
    }
    return 0;
}

static int Config_prev(int widget_desc, void *data, const char **strptr)
{
    int			prev_page = config_page;

    UNUSED_PARAM(widget_desc); UNUSED_PARAM(data); UNUSED_PARAM(strptr);
    if (config_max > 1) {
	config_page = (config_page - 1 + config_max) % config_max;
	Widget_raise(config_widget_desc[config_page]);
	Widget_unmap(config_widget_desc[prev_page]);
	config_mapped = true;
    }
    return 0;
}

static int Config_create_bool(int widget_desc, int *height,
			      const char *str, bool val,
			      int (*callback)(int, void *, bool *),
			      void *data)
{
    int			offset,
			label_width,
			boolw;

    if (*height + 2*config_entry_height + 2*config_space >= config_height)
	return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str))
		  + 2 * config_text_space;
    offset = config_width - (config_space + config_bool_width);
    if (config_space + label_width > offset) {
	if (*height + 3*config_entry_height + 2*config_space >= config_height)
	    return 0;
    }

    Widget_create_label(widget_desc, config_space, *height
			    + (config_entry_height - config_text_height) / 2,
			label_width, config_text_height, true,
			0, str);
    if (config_space + label_width > offset)
	*height += config_entry_height;
    boolw = Widget_create_bool(widget_desc,
		       offset, *height
			   + (config_entry_height - config_bool_height) / 2,
		       config_bool_width,
		       config_bool_height,
		       0, val, callback, data);
    *height += config_entry_height + config_space;

    return boolw;
}

static int Config_create_int(int widget_desc, int *height,
			     const char *str, int *val, int min, int max,
			     int (*callback)(int, void *, int *), void *data)
{
    int			offset,
			label_width,
			intw;

    if (*height + 2*config_entry_height + 2*config_space >= config_height)
	return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str))
		  + 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width
	    + config_int_width);
    if (config_space + label_width > offset) {
	if (*height + 3*config_entry_height + 2*config_space >= config_height)
	    return 0;
    }
    Widget_create_label(widget_desc, config_space, *height
			+ (config_entry_height - config_text_height) / 2,
			label_width, config_text_height, true,
			0, str);
    if (config_space + label_width > offset)
	*height += config_entry_height;
    intw = Widget_create_int(widget_desc, offset, *height
			      + (config_entry_height - config_text_height) / 2,
			     config_int_width, config_text_height,
			     0, val, min, max, callback, data);
    offset += config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height
			     + (config_entry_height - config_arrow_height) / 2,
			     config_arrow_width, config_arrow_height,
			     0, intw);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height
			      + (config_entry_height-config_arrow_height) / 2,
			      config_arrow_width, config_arrow_height,
			      0, intw);
    *height += config_entry_height + config_space;

    return intw;
}

static int Config_create_color(int widget_desc, int *height, int color,
			       const char *str, int *val, int min, int max,
			       int (*callback)(int, void *, int *), void *data)
{
    int			offset,	label_width, colw;
 
    if (*height + 2*config_entry_height + 2*config_space >= config_height)
 	return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str))
	+ 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width
			     + config_int_width);
    if (config_space + label_width > offset) {
 	if (*height + 3*config_entry_height + 2*config_space >= config_height)
 	    return 0;
    }
    Widget_create_label(widget_desc, config_space, *height
 			+ (config_entry_height - config_text_height) / 2,
 			label_width, config_text_height, true,
 			0, str);
    if (config_space + label_width > offset)
 	*height += config_entry_height;
    colw = Widget_create_color(widget_desc, color, offset, *height
			       + (config_entry_height - config_text_height)/2,
			       config_int_width, config_text_height,
			       0, val, min, max, callback, data);
    offset += config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height
			     + (config_entry_height - config_arrow_height)/2,
 			     config_arrow_width, config_arrow_height,
 			     0, colw);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height
			      + (config_entry_height - config_arrow_height)/2,
 			      config_arrow_width, config_arrow_height,
 			      0, colw);
    *height += config_entry_height + config_space;

    return colw;
}
 
static int Config_create_double(int widget_desc, int *height,
				const char *str, double *val,
				double min, double max,
				int (*callback)(int, void *, double *),
				void *data)
{
    int			offset,
			label_width,
			doublew;

    if (*height + 2*config_entry_height + 2*config_space >= config_height)
	return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str))
		  + 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width
	    + config_double_width);
    if (config_space + label_width > offset) {
	if (*height + 3*config_entry_height + 2*config_space >= config_height)
	    return 0;
    }
    Widget_create_label(widget_desc, config_space, *height
			+ (config_entry_height - config_text_height) / 2,
			label_width, config_text_height, true,
			0, str);
    if (config_space + label_width > offset)
	*height += config_entry_height;
    doublew = Widget_create_double(widget_desc, offset, *height
				  + (config_entry_height
				     - config_text_height) / 2,
				  config_double_width, config_text_height,
				  0, val, min, max, callback, data);
    offset += config_double_width;
    Widget_create_arrow_left(widget_desc, offset, *height
			     + (config_entry_height - config_arrow_height) / 2,
			     config_arrow_width, config_arrow_height,
			     0, doublew);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height
			      + (config_entry_height-config_arrow_height) / 2,
			      config_arrow_width, config_arrow_height,
			      0, doublew);
    *height += config_entry_height + config_space;

    return doublew;
}


static int Config_create_save(int widget_desc, int *height)
{
    static char		save_str[] = "Save Configuration";
    int			space,
			button_desc,
			width = 2 * config_button_space
				+ XTextWidth(buttonFont, save_str,
					     (int)strlen(save_str));

    space = config_height - (*height + 2*config_entry_height + 2*config_space);
    if (space < 0)
	return 0;
    button_desc =
	Widget_create_activate(widget_desc,
			       (config_width - width) / 2,
			       *height + space / 2,
			       width, config_button_height,
			       0, save_str,
			       Config_save, (void *)save_str);
    if (button_desc == NO_WIDGET)
	return 0;
    *height += config_entry_height + config_space + space;

    return 1;
}

static int Config_creator(xp_option_t *opt, int widget_desc, int *height)
{
    if (opt == NULL)
	return Config_create_save(widget_desc, height);

    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS) {
	assert (Option_get_type(opt) == xp_int_option);
	return Config_create_color(widget_desc, height, *opt->int_ptr,
				   Option_get_name(opt), opt->int_ptr,
				   opt->int_minval, opt->int_maxval,
				   Update_int_option, opt);
    }

    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT) {
	switch (Option_get_type(opt)) {
	case xp_bool_option:
	    return Config_create_bool(widget_desc, height,
				      Option_get_name(opt),
				      *opt->bool_ptr,
				      Update_bool_option, opt);
	case xp_int_option:
	    return Config_create_int(widget_desc, height,
				     Option_get_name(opt), opt->int_ptr,
				     opt->int_minval, opt->int_maxval,
				     Update_int_option, opt);

	case xp_double_option:
	    return Config_create_double(widget_desc, height,
					Option_get_name(opt), opt->dbl_ptr,
					opt->dbl_minval, opt->dbl_maxval,
					Update_double_option, opt);
	default:
	    return 0;
	}
    }

    return 0;
}

static void Config_save_failed(const char *reason, const char **strptr)
{
    if (config_save_confirm_desc != NO_WIDGET)
	Widget_destroy(config_save_confirm_desc);

    config_save_confirm_desc
	= Widget_create_confirm(reason, Config_save_confirm_callback);

    if (config_save_confirm_desc != NO_WIDGET)
	Widget_raise(config_save_confirm_desc);

    *strptr = "Saving failed...";
}

static int Config_save(int widget_desc, void *button_str, const char **strptr)
{
    int retval;
    char path[PATH_MAX + 1];

    *strptr = "Saving...";
    Widget_draw(widget_desc);
    XFlush(dpy);

    Xpilotrc_get_filename(path, sizeof(path));
    retval = Xpilotrc_write(path);

    if (retval == -1) {
	Config_save_failed("Can't find .xpilotrc file", strptr);
	return 1;
    }

    if (retval == -2) {
	Config_save_failed("Can't open file to save to.", strptr);
	return 1;
    }

    if (config_save_confirm_desc != NO_WIDGET) {
	Widget_destroy(config_save_confirm_desc);
	config_save_confirm_desc = NO_WIDGET;
    }

    *strptr = (char *) button_str;
 
    return 1;
}

static int Config_save_confirm_callback(int widget_desc, void *popup_desc,
					const char **strptr)
{
    UNUSED_PARAM(widget_desc); UNUSED_PARAM(strptr);
    if (config_save_confirm_desc != NO_WIDGET) {
	Widget_destroy((int)(long int)popup_desc);
	config_save_confirm_desc = NO_WIDGET;
    }
    return 0;
}

int Config(bool doit, int what)
{
    /* get rid of the old widgets, it's the most easy way */
    Config_destroy();
    if (doit == false)
	return false;

    config_what = what;

    Create_config();
    if (config_created == false)
	return false;

    Widget_raise(config_widget_desc[config_page]);
    config_mapped = true;
    return true;
}

void Config_destroy(void)
{
    int			i;

    if (config_created) {
	if (config_mapped) {
	    Widget_unmap(config_widget_desc[config_page]);
	    config_mapped = false;
	}
	for (i = 0; i < config_max; i++)
	    Widget_destroy(config_widget_desc[i]);
	config_created = false;
	free(config_widget_desc);
	config_widget_desc = NULL;
	config_max = 0;
	config_page = 0;
    }
}

void Config_resize(void)
{
    bool		mapped = config_mapped;

    if (config_created) {
	Config_destroy();
	if (mapped)
	    Config(mapped, config_what);
    }
}

void Config_redraw(void)
{
    int i;

    if (!config_mapped)
	return;

    for (i = 0; i < Nelem_config_creator(); i++)
	Widget_draw(config_widget_ids[i]);
}

void Config_init(void)
{
    int i, max_ids;

    for (i = 0; i < num_options; i++) {
	xp_option_t *opt = Option_by_index(i);

	if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS) {
	    STORE(int, color_option_indices,
		  num_color_options, max_color_options, i);
	} else if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT) {
	    STORE(int, default_option_indices,
		  num_default_options, max_default_options, i);
	}
    }

    /* +1 is for the save widget */
    max_ids = MAX(num_color_options, num_default_options) + 1;
    config_widget_ids = XMALLOC(int, max_ids);
    if (config_widget_ids == NULL) {
	error("Config_init: not enough memory.");
	exit(1);
    }
}
