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

#include "console.h"
#include "SDL_console.h"
#include "sdlwindow.h"

static sdl_window_t console_window;
static ConsoleInformation *console;
static SDL_Window *text_input_window;

void command_handler(ConsoleInformation *, char *);

static void Console_refresh(void)
{
    SDL_FillSurfaceRect(console_window.surface, NULL, 0);
    CON_UpdateConsole(console);
    CON_DrawConsole(console);
    sdl_window_refresh(&console_window);
}

int Console_init(SDL_Window *window)
{
    SDL_Rect cr;

    if (window == NULL)
	return -1;
    text_input_window = window;
    cr.w = 500;
    cr.h = 100;
    if (cr.w > draw_width) cr.w = draw_width;
    if (cr.h > draw_height) cr.h = draw_height;
    cr.x = (draw_width - cr.w) / 2;
    cr.y = (draw_height - cr.h) / 2;

    if (sdl_window_init(&console_window, cr.x, cr.y, cr.w, cr.h)) {
	error("failed to init console window");
	return -1;
    }
    
    console = CON_Init(CONF_FONTDIR "ConsoleFont.bmp", console_window.surface, 100, cr);
    if (console == NULL) {
	error("failed to init SDL_console");
	sdl_window_destroy(&console_window);
	return -1;
    }
    CON_SetExecuteFunction(console, command_handler);
    CON_Topmost(console);
    CON_Alpha(console, 200);
    CON_SetPrompt(console, "xp> ");
    Console_print("XPilot console ready");
    /*Console_print("Type /help to get help on commands.");
    Console_show();*/
    return 0;
}

int Console_prepare(Renderer *renderer)
{
    if (console == NULL || renderer == NULL)
	return -1;

    if (console->Visible != CON_CLOSED && console->Visible != CON_OPEN)
	Console_refresh();

    return sdl_window_prepare(&console_window, renderer);
}

RendererStatus Console_paint(void)
{
    if (!Console_isVisible())
	return RENDERER_STATUS_OK;
    console_window.x = (draw_width - console_window.w) / 2;
    console_window.y = (draw_height - console_window.h) / 2;
    return sdl_window_paint(&console_window, NULL);
}

void Console_show(void)
{
    CON_Show(console);
    SDL_StartTextInput(text_input_window);
    Console_refresh();
}

void Console_hide(void)
{
    SDL_StopTextInput(text_input_window);
    CON_Hide(console);
}

int Console_isVisible(void)
{
    return (console->Visible != CON_CLOSED);
}

int Console_process(SDL_Event *e)
{
    if (CON_Events(e) == NULL) {
	Console_refresh();
	return 1;
    }
    return 0;
}

void Paste_String_to_Console(const char *text)
{
    Add_String_to_Console(text);
    Console_refresh();
}

void Console_cleanup(void)
{
    CON_Destroy(console);
    console = NULL;
    sdl_window_destroy(&console_window);
    text_input_window = NULL;
}

void Console_print(const char *str, ...)
{
    va_list marker;
    va_start(marker, str);
    CON_Out(console, str, marker);
    va_end(marker);
    Console_refresh();
}

void command_handler(ConsoleInformation *con, char *command)
{
    Net_talk(command);
    Talk_set_state(false);
}
