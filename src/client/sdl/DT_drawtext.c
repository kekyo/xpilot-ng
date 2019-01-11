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

/*
	SDL_console: An easy to use drop-down console based on the SDL library
	Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Clemens Wacha
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WHITOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library Generla Public
	License along with this library; if not, write to the Free
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
	Clemens Wacha
	reflex-2000@gmx.net
*/


/*  DT_drawtext.c
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 *  Modified for xpilot: Juha Lindström <juhal@users.sourceforge.net>
 */

#include "xpclient_sdl.h"

#include "DT_drawtext.h"

#ifdef _WINDOWS
#define __FUNCTION__ ""
#endif
#define PRINT_ERROR(X) fprintf(stderr, "ERROR in %s:%s(): %s\n", __FILE__, __FUNCTION__, X)


static BitFont *BitFonts = NULL;	/* Linked list of fonts */

/* Loads the font into a new struct
 * returns -1 as an error else it returns the number
 * of the font for the user to use
 */
int DT_LoadFont(const char *BitmapName, int flags) {
	int FontNumber = 0;
	BitFont **CurrentFont = &BitFonts;
	SDL_Surface *Temp;


	while(*CurrentFont) {
		CurrentFont = &((*CurrentFont)->NextFont);
		FontNumber++;
	}

	/* load the font bitmap */

#ifdef HAVE_SDLIMAGE
	Temp = IMG_Load(BitmapName);
#else
	Temp = SDL_LoadBMP(BitmapName);
#endif

    if(Temp == NULL) {
		PRINT_ERROR("Cannot load file ");
		printf("%s: %s\n", BitmapName, SDL_GetError());
		return -1;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont *) malloc(sizeof(BitFont));

	(*CurrentFont)->FontSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);

	(*CurrentFont)->CharWidth = (*CurrentFont)->FontSurface->w / 256;
	(*CurrentFont)->CharHeight = (*CurrentFont)->FontSurface->h;
	(*CurrentFont)->FontNumber = FontNumber;
	(*CurrentFont)->NextFont = NULL;


	/* Set font as transparent if the flag is set.  The assumption we'll go on
	 * is that the first pixel of the font image will be the color we should treat
	 * as transparent.
	 */
	if(flags & TRANS_FONT) {
	    SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB((*CurrentFont)->FontSurface->format, 255, 0, 255));
	}
	return FontNumber;
}

/* Takes the font type, coords, and text to draw to the surface*/
void DT_DrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y) {
	int loop;
	int characters;
	int current;
	SDL_Rect SourceRect, DestRect;
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontType);

	/* see how many characters can fit on the screen */
	if(x > surface->w || y > surface->h)
		return;

	if((int)strlen(string) < (surface->w - x) / CurrentFont->CharWidth)
		characters = strlen(string);
	else
		characters = (surface->w - x) / CurrentFont->CharWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = (const unsigned char)string[loop];
		if (current<0 || current > 255)
			current = 0;
		/* SourceRect.x = string[loop] * CurrentFont->CharWidth; */
		SourceRect.x = current * CurrentFont->CharWidth;
		SDL_BlitSurface(CurrentFont->FontSurface, &SourceRect, surface, &DestRect);
		DestRect.x += CurrentFont->CharWidth;
	}
}


/* Returns the height of the font numbers character
 * returns 0 if the fontnumber was invalid */
int DT_FontHeight(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharHeight;
	else
		return 0;
}

/* Returns the width of the font numbers charcter */
int DT_FontWidth(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharWidth;
	else
		return 0;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if theres an error
 */
BitFont *DT_FontPointer(int FontNumber) {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont)
		if(CurrentFont->FontNumber == FontNumber)
			return CurrentFont;
		else {
			temp = CurrentFont;
			CurrentFont = CurrentFont->NextFont;
		}

	return NULL;

}

/* removes all the fonts currently loaded */
void DT_DestroyDrawText(void) {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont) {
		temp = CurrentFont;
		CurrentFont = CurrentFont->NextFont;

		SDL_FreeSurface(temp->FontSurface);
		free(temp);
	}

	BitFonts = NULL;
}


