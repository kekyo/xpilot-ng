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

/***************************************************************************\
*  winXKey.h - X11 to Windoze converter										*
*																			*
*  This file contains X11 style kb definitions for Winodoze.				*
*  These all come from Xutil.h												*
*																			*
*  							*
\***************************************************************************/
#ifndef	_WINXKEY_H_
#define	_WINXKEY_H_

#ifdef	_WINDOWS


/*
 * Compose sequence status structure, used in calling XLookupString.
 */
typedef struct _XComposeStatus {
    XPointer compose_ptr;	/* state table pointer */
    int chars_matched;		/* match state */
} XComposeStatus;

/* lifted from keysymdef.h */
#define XK_BackSpace            0xFF08	/* back space, back char */
#define XK_Tab                  0xFF09
#define XK_Linefeed             0xFF0A	/* Linefeed, LF */
#define XK_Clear                0xFF0B
#define XK_Return               0xFF0D	/* Return, enter */
#define XK_Pause                0xFF13	/* Pause, hold */
#define XK_Scroll_Lock          0xFF14
#define XK_Sys_Req              0xFF15
#define XK_Escape               0xFF1B
#define XK_Delete               0xFFFF	/* Delete, rubout */

#define XK_Home                 0xFF50
#define XK_Left                 0xFF51	/* Move left, left arrow */
#define XK_Up                   0xFF52	/* Move up, up arrow */
#define XK_Right                0xFF53	/* Move right, right arrow */
#define XK_Down                 0xFF54	/* Move down, down arrow */
#define XK_Prior                0xFF55	/* Prior, previous */
#define XK_Page_Up              0xFF55
#define XK_Next                 0xFF56	/* Next */
#define XK_Page_Down            0xFF56
#define XK_End                  0xFF57	/* EOL */
#define XK_Begin                0xFF58	/* BOL */



#define XK_KP_Space             0xFF80	/* space */
#define XK_KP_Tab               0xFF89
#define XK_KP_Enter             0xFF8D	/* enter */
#define XK_KP_F1                0xFF91	/* PF1, KP_A, ... */
#define XK_KP_F2                0xFF92
#define XK_KP_F3                0xFF93
#define XK_KP_F4                0xFF94
#define XK_KP_Home              0xFF95
#define XK_KP_Left              0xFF96
#define XK_KP_Up                0xFF97
#define XK_KP_Right             0xFF98
#define XK_KP_Down              0xFF99
#define XK_KP_Prior             0xFF9A
#define XK_KP_Page_Up           0xFF9A
#define XK_KP_Next              0xFF9B
#define XK_KP_Page_Down         0xFF9B
#define XK_KP_End               0xFF9C
#define XK_KP_Begin             0xFF9D
#define XK_KP_Insert            0xFF9E
#define XK_KP_Delete            0xFF9F
#define XK_KP_Equal             0xFFBD	/* equals */
#define XK_KP_Multiply          0xFFAA
#define XK_KP_Add               0xFFAB
#define XK_KP_Separator         0xFFAC	/* separator, often comma */
#define XK_KP_Subtract          0xFFAD
#define XK_KP_Decimal           0xFFAE
#define XK_KP_Divide            0xFFAF

#define XK_KP_0                 0xFFB0
#define XK_KP_1                 0xFFB1
#define XK_KP_2                 0xFFB2
#define XK_KP_3                 0xFFB3
#define XK_KP_4                 0xFFB4
#define XK_KP_5                 0xFFB5
#define XK_KP_6                 0xFFB6
#define XK_KP_7                 0xFFB7
#define XK_KP_8                 0xFFB8
#define XK_KP_9                 0xFFB9

#define XK_space               0x020
#define XK_exclam              0x021
#define XK_quotedbl            0x022
#define XK_numbersign          0x023
#define XK_dollar              0x024
#define XK_percent             0x025
#define XK_ampersand           0x026
#define XK_apostrophe          0x027
#define XK_quoteright          0x027	/* deprecated */
#define XK_parenleft           0x028
#define XK_parenright          0x029
#define XK_asterisk            0x02a
#define XK_plus                0x02b
#define XK_comma               0x02c
#define XK_minus               0x02d
#define XK_period              0x02e
#define XK_slash               0x02f
#define XK_0                   0x030
#define XK_1                   0x031
#define XK_2                   0x032
#define XK_3                   0x033
#define XK_4                   0x034
#define XK_5                   0x035
#define XK_6                   0x036
#define XK_7                   0x037
#define XK_8                   0x038
#define XK_9                   0x039
#define XK_colon               0x03a
#define XK_semicolon           0x03b
#define XK_less                0x03c
#define XK_equal               0x03d
#define XK_greater             0x03e
#define XK_question            0x03f
#define XK_at                  0x040
#define XK_A                   0x041
#define XK_B                   0x042
#define XK_C                   0x043
#define XK_D                   0x044
#define XK_E                   0x045
#define XK_F                   0x046
#define XK_G                   0x047
#define XK_H                   0x048
#define XK_I                   0x049
#define XK_J                   0x04a
#define XK_K                   0x04b
#define XK_L                   0x04c
#define XK_M                   0x04d
#define XK_N                   0x04e
#define XK_O                   0x04f
#define XK_P                   0x050
#define XK_Q                   0x051
#define XK_R                   0x052
#define XK_S                   0x053
#define XK_T                   0x054
#define XK_U                   0x055
#define XK_V                   0x056
#define XK_W                   0x057
#define XK_X                   0x058
#define XK_Y                   0x059
#define XK_Z                   0x05a
#define XK_bracketleft         0x05b
#define XK_backslash           0x05c
#define XK_bracketright        0x05d
#define XK_asciicircum         0x05e
#define XK_underscore          0x05f
#define XK_grave               0x060
#define XK_quoteleft           0x060	/* deprecated */
#define XK_a                   0x061
#define XK_b                   0x062
#define XK_c                   0x063
#define XK_d                   0x064
#define XK_e                   0x065
#define XK_f                   0x066
#define XK_g                   0x067
#define XK_h                   0x068
#define XK_i                   0x069
#define XK_j                   0x06a
#define XK_k                   0x06b
#define XK_l                   0x06c
#define XK_m                   0x06d
#define XK_n                   0x06e
#define XK_o                   0x06f
#define XK_p                   0x070
#define XK_q                   0x071
#define XK_r                   0x072
#define XK_s                   0x073
#define XK_t                   0x074
#define XK_u                   0x075
#define XK_v                   0x076
#define XK_w                   0x077
#define XK_x                   0x078
#define XK_y                   0x079
#define XK_z                   0x07a
#define XK_braceleft           0x07b
#define XK_bar                 0x07c
#define XK_braceright          0x07d
#define XK_asciitilde          0x07e

extern XLookupKeysym(XKeyEvent * key_event, int index);
extern XLookupString(XKeyEvent * key_event, char *buffer_return,
		     int bytes_buffer, KeySym * keysym_return,
		     XComposeStatus * status_in_out);


#endif				/* _WINDOWS */

#endif				/* _WINXKEY_H_ */
