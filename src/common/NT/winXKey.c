/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/***************************************************************\
*  winXKey.c - X11 to Windoze converter							*
\***************************************************************/
#include "winX.h"

struct wks {
	char*	s;
	KeySym	k;
} winkeysym[] = {
"BackSpace",		0x000E,	/* back space, back char */
"Tab",				0x000F,
"Return",			0x001C,	/* Return, enter */
"Pause",			0x0045,	/* Pause, hold */
"Scroll_Lock",		0x0046,
"Escape",			0x0001,
"Delete",			0x0153,	/* Delete, rubout */
"Home",				0x0147,
"Left",				0x014B,	/* Move left, left arrow */
"Up",				0x0148,	/* Move up, up arrow */
"Right",			0x014D,	/* Move right, right arrow */
"Down",				0x0150,	/* Move down, down arrow */
"Page_Up",			0x0149,
"Page_Down",		0x0151,
"End",				0x014F,	/* EOL */
"Insert",			0x0152,	/* Insert, insert here */
"Num_Lock",			0x0145,
"KP_Enter",			0x011C,	/* enter */
"KP_Multiply",		0x0037,
"KP_Add",			0x004E,
"KP_Subtract",		0x004A,
"KP_Decimal",		0x0053,
"KP_Divide",		0x0135,
"KP_0",				0x0052,
"KP_1",				0x004F,
"KP_2",				0x0050,
"KP_3",				0x0051,
"KP_4",				0x004B,
"KP_5",				0x004C,
"KP_6",				0x004D,
"KP_7",				0x0047,
"KP_8",				0x0048,
"KP_9",				0x0049,
"F1",				0x003B,
"F2",				0x003C,
"F3",				0x003D,
"F4",				0x003E,
"F5",				0x003F,
"F6",				0x0040,
"F7",				0x0041,
"F8",				0x0042,
"F9",				0x0043,
"F10",				0x0044,
"F11",				0x0057,
"F12",				0x0058,
"Shift_L",			0x002A,	/* Left shift */
"Shift_R",			0x0036,	/* Right shift */
"Control_L",		0x001D,	/* Left control */
"Control_R",		0x011D,	/* Right control */
"Caps_Lock",		0x003A,	/* Caps lock */
"space",			0x0039,
"apostrophe",		0x0028,
"quoteright",		0x0028,
"comma",			0x0033,
"minus",			0x000C,
"period",			0x0034,
"slash",			0x0035,
"0",				0x000B,
"1",				0x0002,
"2",				0x0003,
"3",				0x0004,
"4",				0x0005,
"5",				0x0006,
"6",				0x0007,
"7",				0x0008,
"8",				0x0009,
"9",				0x000A,
"semicolon",		0x0027,
"equal",			0x000D,
"A",				0x001E,
"B",				0x0030,
"C",				0x002E,
"D",				0x0020,
"E",				0x0012,
"F",				0x0021,
"G",				0x0022,
"H",				0x0023,
"I",				0x0017,
"J",				0x0024,
"K",				0x0025,
"L",				0x0026,
"M",				0x0032,
"N",				0x0031,
"O",				0x0018,
"P",				0x0019,
"Q",				0x0010,
"R",				0x0013,
"S",				0x001F,
"T",				0x0014,
"U",				0x0016,
"V",				0x002F,
"W",				0x0011,
"X",				0x002D,
"Y",				0x0015,
"Z",				0x002C,
"a",				0x001E,
"b",				0x0030,
"c",				0x002E,
"d",				0x0020,
"e",				0x0012,
"f",				0x0021,
"g",				0x0022,
"h",				0x0023,
"i",				0x0017,
"j",				0x0024,
"k",				0x0025,
"l",				0x0026,
"m",				0x0032,
"n",				0x0031,
"o",				0x0018,
"p",				0x0019,
"q",				0x0010,
"r",				0x0013,
"s",				0x001F,
"t",				0x0014,
"u",				0x0016,
"v",				0x002F,
"w",				0x0011,
"x",				0x002D,
"y",				0x0015,
"z",				0x002C,
"bracketleft",		0x001A,
"backslash",		0x002B,
"bracketright",		0x001B,
"grave",			0x0029,
"quoteleft",		0x0029,	/* deprecated */
NULL,				0
};

KeySym XStringToKeysym(char* s)
{
	struct wks* wks;
	for (wks = &winkeysym[0]; wks->s; wks++)
	{
		if (!stricmp(s, wks->s))
			return(wks->k);
	}
	return(NoSymbol);
}

char *XKeysymToString(KeySym keysym)
{

	struct wks* wks;
	for (wks = &winkeysym[0]; wks->s; wks++)
	{
		if (keysym == wks->k)
			return(wks->s);
	}
	return(NULL);
}
