/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
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

#include "sdlkeys.h"

typedef struct {
	const char *name;
	SDLKey	key;
} sdlkey_t;

static sdlkey_t sdlkeys[] = {
   { "BackSpace",    SDLK_BACKSPACE },
   { "Tab",          SDLK_TAB },
   { "Return",       SDLK_RETURN },
   { "Pause",        SDLK_PAUSE },
   { "Scroll_Lock",  SDLK_SCROLLOCK },
   { "Print",        SDLK_PRINT },
   { "Escape",       SDLK_ESCAPE },
   { "Delete",       SDLK_DELETE },
   { "Home",         SDLK_HOME },
   { "Left",         SDLK_LEFT },
   { "Up",           SDLK_UP },
   { "Right",        SDLK_RIGHT },
   { "Down",         SDLK_DOWN },
   { "Page_Up",      SDLK_PAGEUP },
   { "Page_Down",    SDLK_PAGEDOWN },
   { "Prior",        SDLK_PAGEUP },
   { "Next",         SDLK_PAGEDOWN },
   { "End",          SDLK_END },
   { "Insert",       SDLK_INSERT },
   { "Num_Lock",     SDLK_NUMLOCK },
   { "KP_Enter",     SDLK_KP_ENTER },
   { "KP_Multiply",  SDLK_KP_MULTIPLY },
   { "KP_Add",       SDLK_KP_PLUS },
   { "KP_Subtract",  SDLK_KP_MINUS },
   { "KP_Decimal",   SDLK_KP_PERIOD },
   { "KP_Divide",    SDLK_KP_DIVIDE },
   { "KP_Insert",    SDLK_KP0 },
   { "KP_Delete",    SDLK_KP_PERIOD },
   { "KP_0",         SDLK_KP0 },
   { "KP_1",         SDLK_KP1 },
   { "KP_2",         SDLK_KP2 },
   { "KP_3",         SDLK_KP3 },
   { "KP_4",         SDLK_KP4 },
   { "KP_5",         SDLK_KP5 },
   { "KP_6",         SDLK_KP6 },
   { "KP_7",         SDLK_KP7 },
   { "KP_8",         SDLK_KP8 },
   { "KP_9",         SDLK_KP9 },
   { "F1",           SDLK_F1 },
   { "F2",           SDLK_F2 },
   { "F3",           SDLK_F3 },
   { "F4",           SDLK_F4 },
   { "F5",           SDLK_F5 },
   { "F6",           SDLK_F6 },
   { "F7",           SDLK_F7 },
   { "F8",           SDLK_F8 },
   { "F9",           SDLK_F9 },
   { "F10",          SDLK_F10 },
   { "F11",          SDLK_F11 },
   { "F12",          SDLK_F12 },
   { "Shift_L",      SDLK_LSHIFT },
   { "Shift_R",      SDLK_RSHIFT },
   { "Control_L",    SDLK_LCTRL },
   { "Control_R",    SDLK_RCTRL },
   { "Caps_Lock",    SDLK_CAPSLOCK },
   { "space",        SDLK_SPACE },
   { "apostrophe",   SDLK_QUOTE },
   { "quoteright",   SDLK_QUOTE },
   { "comma",        SDLK_COMMA },
   { "plus",         SDLK_PLUS },
   { "minus",        SDLK_MINUS },
   { "period",       SDLK_PERIOD },
   { "slash",        SDLK_SLASH },
   { "0",            SDLK_0 },
   { "1",            SDLK_1 },
   { "2",            SDLK_2 },
   { "3",            SDLK_3 },
   { "4",            SDLK_4 },
   { "5",            SDLK_5 },
   { "6",            SDLK_6 },
   { "7",            SDLK_7 },
   { "8",            SDLK_8 },
   { "9",            SDLK_9 },
   { "semicolon",    SDLK_SEMICOLON },
   { "equal",        SDLK_EQUALS },
   { "A",            SDLK_a },
   { "B",            SDLK_b },
   { "C",            SDLK_c },
   { "D",            SDLK_d },
   { "E",            SDLK_e },
   { "F",            SDLK_f },
   { "G",            SDLK_g },
   { "H",            SDLK_h },
   { "I",            SDLK_i },
   { "J",            SDLK_j },
   { "K",            SDLK_k },
   { "L",            SDLK_l },
   { "M",            SDLK_m },
   { "N",            SDLK_n },
   { "O",            SDLK_o },
   { "P",            SDLK_p },
   { "Q",            SDLK_q },
   { "R",            SDLK_r },
   { "S",            SDLK_s },
   { "T",            SDLK_t },
   { "U",            SDLK_u },
   { "V",            SDLK_v },
   { "W",            SDLK_w },
   { "X",            SDLK_x },
   { "Y",            SDLK_y },
   { "Z",            SDLK_z },
   { "a",            SDLK_a },
   { "b",            SDLK_b },
   { "c",            SDLK_c },
   { "d",            SDLK_d },
   { "e",            SDLK_e },
   { "f",            SDLK_f },
   { "g",            SDLK_g },
   { "h",            SDLK_h },
   { "i",            SDLK_i },
   { "j",            SDLK_j },
   { "k",            SDLK_k },
   { "l",            SDLK_l },
   { "m",            SDLK_m },
   { "n",            SDLK_n },
   { "o",            SDLK_o },
   { "p",            SDLK_p },
   { "q",            SDLK_q },
   { "r",            SDLK_r },
   { "s",            SDLK_s },
   { "t",            SDLK_t },
   { "u",            SDLK_u },
   { "v",            SDLK_v },
   { "w",            SDLK_w },
   { "x",            SDLK_x },
   { "y",            SDLK_y },
   { "z",            SDLK_z },
   { "bracketleft", 	SDLK_LEFTBRACKET },
   { "backslash",   	SDLK_BACKSLASH },
   { "bracketright",	SDLK_RIGHTBRACKET },
   { "grave",	    	SDLK_BACKQUOTE },
   { "quoteleft",   	SDLK_BACKQUOTE },
   { "quotedbl",   	SDLK_QUOTEDBL },
   { "section",   	SDLK_WORLD_7 },
   { NULL,  	    	SDLK_UNKNOWN },
};

SDLKey Get_key_by_name(const char* name)
{
    sdlkey_t *k;

    for (k = &sdlkeys[0]; k->name != NULL; k++)
        if (!strcmp(name, k->name)) {
	   return k->key;
	}

    return SDLK_UNKNOWN;
}

char *Get_name_by_key(SDLKey key)
{
    sdlkey_t *k;

    for (k = &sdlkeys[0]; k->name != NULL; k++)
        if (key == k->key)
            return (char *)(k->name);

    return NULL;
}

xp_keysym_t String_to_xp_keysym(/*const*/ char *name)
{
    SDLKey sdlk = Get_key_by_name(name);
    if (sdlk == SDLK_UNKNOWN) return XP_KS_UNKNOWN;
    return (xp_keysym_t)sdlk;
}
