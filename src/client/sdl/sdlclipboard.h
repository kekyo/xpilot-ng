/*
 * XPilot Infinity/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SDLCLIPBOARD_H
#define SDLCLIPBOARD_H

#include "xpclient_sdl.h"

/**
 * Copy text to the SDL3 clipboard with normalized Unix line endings.
 *
 * @param text NUL-terminated text to copy.
 * @return 0 on success, or -1 on allocation or SDL clipboard failure.
 */
int Sdl_clipboard_set_text(const char *text);

/**
 * Retrieve text from the SDL3 clipboard.
 *
 * @return SDL-allocated text, or NULL when the clipboard is empty or fails.
 *         The caller must release the result with Sdl_clipboard_free_text().
 */
char *Sdl_clipboard_get_text(void);

/**
 * Release text returned by Sdl_clipboard_get_text().
 *
 * @param text Clipboard text, which may be NULL.
 */
void Sdl_clipboard_free_text(char *text);

#endif
