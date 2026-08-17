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
	SDL_Keycode key;
} sdlkey_t;

static sdlkey_t sdlkeys[] = {
   { "BackSpace",    SDLK_BACKSPACE },
   { "Tab",          SDLK_TAB },
   { "Return",       SDLK_RETURN },
   { "Linefeed",     SDLK_RETURN },
   { "Pause",        SDLK_PAUSE },
   { "Scroll_Lock",  SDLK_SCROLLLOCK },
   { "Print",        SDLK_PRINTSCREEN },
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
   { "Num_Lock",     SDLK_NUMLOCKCLEAR },
   { "KP_Enter",     SDLK_KP_ENTER },
   { "KP_Multiply",  SDLK_KP_MULTIPLY },
   { "KP_Add",       SDLK_KP_PLUS },
   { "KP_Subtract",  SDLK_KP_MINUS },
   { "KP_Decimal",   SDLK_KP_PERIOD },
   { "KP_Divide",    SDLK_KP_DIVIDE },
   { "KP_Insert",    SDLK_KP_0 },
   { "KP_Delete",    SDLK_KP_PERIOD },
   { "KP_0",         SDLK_KP_0 },
   { "KP_1",         SDLK_KP_1 },
   { "KP_2",         SDLK_KP_2 },
   { "KP_3",         SDLK_KP_3 },
   { "KP_4",         SDLK_KP_4 },
   { "KP_5",         SDLK_KP_5 },
   { "KP_6",         SDLK_KP_6 },
   { "KP_7",         SDLK_KP_7 },
   { "KP_8",         SDLK_KP_8 },
   { "KP_9",         SDLK_KP_9 },
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
   { "apostrophe",   SDLK_APOSTROPHE },
   { "quoteright",   SDLK_APOSTROPHE },
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
   { "A",            SDLK_A },
   { "B",            SDLK_B },
   { "C",            SDLK_C },
   { "D",            SDLK_D },
   { "E",            SDLK_E },
   { "F",            SDLK_F },
   { "G",            SDLK_G },
   { "H",            SDLK_H },
   { "I",            SDLK_I },
   { "J",            SDLK_J },
   { "K",            SDLK_K },
   { "L",            SDLK_L },
   { "M",            SDLK_M },
   { "N",            SDLK_N },
   { "O",            SDLK_O },
   { "P",            SDLK_P },
   { "Q",            SDLK_Q },
   { "R",            SDLK_R },
   { "S",            SDLK_S },
   { "T",            SDLK_T },
   { "U",            SDLK_U },
   { "V",            SDLK_V },
   { "W",            SDLK_W },
   { "X",            SDLK_X },
   { "Y",            SDLK_Y },
   { "Z",            SDLK_Z },
   { "a",            SDLK_A },
   { "b",            SDLK_B },
   { "c",            SDLK_C },
   { "d",            SDLK_D },
   { "e",            SDLK_E },
   { "f",            SDLK_F },
   { "g",            SDLK_G },
   { "h",            SDLK_H },
   { "i",            SDLK_I },
   { "j",            SDLK_J },
   { "k",            SDLK_K },
   { "l",            SDLK_L },
   { "m",            SDLK_M },
   { "n",            SDLK_N },
   { "o",            SDLK_O },
   { "p",            SDLK_P },
   { "q",            SDLK_Q },
   { "r",            SDLK_R },
   { "s",            SDLK_S },
   { "t",            SDLK_T },
   { "u",            SDLK_U },
   { "v",            SDLK_V },
   { "w",            SDLK_W },
   { "x",            SDLK_X },
   { "y",            SDLK_Y },
   { "z",            SDLK_Z },
   { "bracketleft", 	SDLK_LEFTBRACKET },
   { "backslash",   	SDLK_BACKSLASH },
   { "bracketright",	SDLK_RIGHTBRACKET },
   { "grave",	    	SDLK_GRAVE },
   { "quoteleft",   	SDLK_GRAVE },
   { "quotedbl",   	SDLK_DBLAPOSTROPHE },
   { "section",   	(SDL_Keycode)0x00a7 },
   { NULL,  	    	SDLK_UNKNOWN },
};

SDL_Keycode Get_key_by_name(const char *name)
{
    sdlkey_t *k;

    for (k = &sdlkeys[0]; k->name != NULL; k++)
        if (!strcmp(name, k->name)) {
	   return k->key;
	}

    return SDLK_UNKNOWN;
}

const char *Get_name_by_key(SDL_Keycode key)
{
    sdlkey_t *k;

    for (k = &sdlkeys[0]; k->name != NULL; k++)
        if (key == k->key)
            return k->name;

    return NULL;
}

xp_keysym_t String_to_xp_keysym(/*const*/ char *name)
{
    SDL_Keycode sdlk = Get_key_by_name(name);
    if (sdlk == SDLK_UNKNOWN) return XP_KS_UNKNOWN;
    return (xp_keysym_t)sdlk;
}
