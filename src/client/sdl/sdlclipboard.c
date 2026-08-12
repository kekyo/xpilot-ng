/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sdlclipboard.h"

int Sdl_clipboard_set_text(const char *text)
{
    char *normalized;
    size_t input_index, output_index, length;
    int result;

    if (text == NULL)
	return -1;

    length = strlen(text);
    normalized = SDL_malloc(length + 1);
    if (normalized == NULL)
	return -1;

    output_index = 0;
    for (input_index = 0; input_index < length; input_index++) {
	if (text[input_index] == '\r') {
	    normalized[output_index++] = '\n';
	    if (text[input_index + 1] == '\n')
		input_index++;
	} else {
	    normalized[output_index++] = text[input_index];
	}
    }
    normalized[output_index] = '\0';

    result = SDL_SetClipboardText(normalized);
    SDL_free(normalized);
    return result;
}

char *Sdl_clipboard_get_text(void)
{
    if (SDL_HasClipboardText() != SDL_TRUE)
	return NULL;
    return SDL_GetClipboardText();
}

void Sdl_clipboard_free_text(char *text)
{
    SDL_free(text);
}
