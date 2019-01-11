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

#ifndef	XINIT_H
#define	XINIT_H

#define MAX_VISUAL_NAME	12

#define MIN_TOP_WIDTH	(640 + 2)
#define MAX_TOP_WIDTH	(1920 + 2)
#define DEF_TOP_WIDTH	(1280 + 2)
#define MIN_TOP_HEIGHT	480
#define MAX_TOP_HEIGHT	1440
#define DEF_TOP_HEIGHT	1024

#ifdef _WINDOWS
# ifndef O_BINARY
#  define O_BINARY		0 /* compability with MSDOS */
# endif
#endif

extern Atom		ProtocolAtom, KillAtom;
extern int		buttonColor;
extern int		windowColor;
extern int		borderColor;
extern int		wallColor;	/* Color index for wall drawing */
extern int		decorColor;	/* Color index for decor drawing */
extern char		sparkColors[MSG_LEN];
extern int		spark_color[MAX_COLORS];
extern int		ButtonHeight;
extern char		visualName[MAX_VISUAL_NAME];
extern Visual		*visual;
extern unsigned		dispDepth;
extern bool		texturedObjects;
extern bool		fullColor;
extern bool		colorSwitch;
extern bool		multibuffer;
extern int		button_form;
extern unsigned		top_width, top_height;
extern unsigned		players_width, players_height;
extern bool		ignoreWindowManager;
extern bool		quitting;

/* XPilot Mouse settings */
extern bool mouseAccelInClient;
extern int new_acc_num, new_acc_denom, new_threshold;
extern Cursor	pointerControlCursor;
/* For restoring the mouse back to normal */
extern bool pre_exists;
extern int pre_acc_num, pre_acc_denom, pre_threshold;

/*
 * xdefault.c
 */
extern void Store_X_options(void);
extern void Handle_X_options(void);

/*
 * Prototypes for xinit.c
 */
extern const char* Item_get_text(int i);
extern int Init_top(void);
extern void Expose_info_window(void);
extern void Expose_button_window(int color, Window w);
extern void Info(Window w);
extern void Talk_resize(void);
extern void Talk_cursor(bool visible);
extern void Talk_map_window(bool map);
extern int Talk_do_event(XEvent *event);
extern int Talk_paste(char* data, size_t len, bool overwrite);
extern int Talk_place_cursor(XButtonEvent *xbutton, bool pending);
extern void Talk_window_cut(XButtonEvent *xbutton);
extern void Talk_cut_from_messages(XButtonEvent *xbutton);
extern void Clear_selection(void);
extern int FatalError(Display *);
extern void Draw_score_table(void);
extern void Resize(Window w, unsigned width, unsigned height);

extern int DrawShadowText(Display*, Window, GC,
			  int x_border, int start_y, const char *str,
			  unsigned long fg, unsigned long bg);
extern void ShadowDrawString(Display*, Window, GC,
			     int x, int start_y, const char *str,
			     unsigned long fg, unsigned long bg);
/*
 * about.c
 */
extern int About_callback(int, void *, const char **);
extern int Keys_callback(int, void *, const char **);
extern void Keys_destroy(void);
extern void About(Window w);
extern int Motd_callback(int, void *, const char **);
extern void Motd_destroy(void);
extern void Expose_about_window(void);
extern void Scale_dashes(void);
#ifdef _WINDOWS
extern int Credits_callback(int, void *, const char **);
#endif

#endif

