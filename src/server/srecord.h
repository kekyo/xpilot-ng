/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2001 Uoti Urpala <uau@users.sourceforge.net>
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

#ifndef SRECORD_H
#define SRECORD_H

extern int   playback;
extern int   record;
extern int   rrecord;
extern int   rplayback;
extern int   *playback_ints;
extern int   *playback_errnos;
extern int   *playback_errnos_start;
extern short *playback_shorts;
extern short *playback_shorts_start;
extern char  *playback_strings;
extern char  *playback_data;
extern char  *playback_sched;
extern int   *playback_ints_start;
extern char  *playback_strings_start;
extern char  *playback_data_start;
extern char  *playback_sched_start;
extern int   *playback_ei;
extern char  *playback_es;
extern int   *playback_ei_start;
extern char  *playback_es_start;
extern int   *playback_opttout;
extern int   recOpt;
#endif  /* SRECORD_H */
