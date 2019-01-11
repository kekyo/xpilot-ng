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

#ifndef	COMMONPROTO_H
#define	COMMONPROTO_H

/* randommt.c */
extern void seedMT(unsigned int seed);
extern unsigned int reloadMT(void);
extern unsigned int randomMT(void);

/* math.c */
extern double rfrac(void);
extern int mod(int x, int y);
extern void Make_table(void);
extern int ON(const char *optval);
extern int OFF(const char *optval);
extern double findDir(double x, double y);

/* strdup.c */
extern char *xp_strdup(const char *);
extern char *xp_safe_strdup(const char *old_string);

/* default.c */
unsigned String_hash(const char *s);

/* strlcpy.c */
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat(char *dest, const char *src, size_t size);
#endif

/* strcasecmp.c */
#ifndef HAVE_STRCASECMP
int strcasecmp(const char *str1, const char *str2);
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp(const char *str1, const char *str2, size_t n);
#endif

/* xpmemory.c */
void *xp_safe_malloc(size_t size);
void *xp_safe_realloc(void *oldptr, size_t size);
void *xp_safe_calloc(size_t nmemb, size_t size);
void xp_safe_free(void *p);

#endif
