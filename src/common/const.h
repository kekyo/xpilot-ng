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

#ifndef CONST_H
#define CONST_H

#ifndef XPCONFIG_H
#include "xpconfig.h"
#endif

#ifndef TYPES_H
#include "types.h"
#endif

/*
 * FLT_MAX is ANSI C standard, but some systems (BSD) use
 * MAXFLOAT instead.
 */
#ifndef	FLT_MAX
#   if defined(MAXFLOAT)
#      define FLT_MAX	MAXFLOAT
#   else
#      define FLT_MAX	1e30f	/* should suffice :-) */
#   endif
#endif

/* Not everyone has PI (or M_PI defined). */
#ifndef	M_PI
#   define PI		3.14159265358979323846
#else
#   define PI		M_PI
#endif

/* Not everyone has LINE_MAX either, *sigh* */
#ifndef LINE_MAX
#   define LINE_MAX	2048
#endif

/* No comment. */
#ifndef PATH_MAX
#   define PATH_MAX	1023
#endif

#define RES		128

#define BLOCK_SZ	35

#define TABLE_SIZE	RES

extern double		tbl_sin[];
extern double		tbl_cos[];

#if 0
  /* The way it was: one table, and always range checking. */
# define tsin(x)	(tbl_sin[MOD2(x, TABLE_SIZE)])
# define tcos(x)	(tbl_sin[MOD2((x)+TABLE_SIZE/4, TABLE_SIZE)])
#else
# if 0
   /* Range checking: find out where the table size is exceeded. */
#  define CHK2(x, m)	((MOD2(x, m) != x) ? (printf("MOD %s:%d:%s\n", __FILE__, __LINE__, #x), MOD2(x, m)) : (x))
# else
   /* No range checking. */
#  define CHK2(x, m)	(x)
# endif
  /* New table lookup with optional range checking and no extra calculations. */
# define tsin(x)	(tbl_sin[CHK2(x, TABLE_SIZE)])
# define tcos(x)	(tbl_cos[CHK2(x, TABLE_SIZE)])
#endif

#define NELEM(a)	((int)(sizeof(a) / sizeof((a)[0])))

#undef ABS
#define ABS(x)			( (x)<0 ? -(x) : (x) )
#ifndef MAX
#   define MIN(x, y)		( (x)>(y) ? (y) : (x) )
#   define MAX(x, y)		( (x)>(y) ? (x) : (y) )
#endif
#define sqr(x)			( (x)*(x) )
#define DELTA(a, b)		(((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))
#define LENGTH(x, y)		( hypot( (double) (x), (double) (y) ) )
#define VECTOR_LENGTH(v)	( hypot( (double) (v).x, (double) (v).y ) )
#define QUICK_LENGTH(x,y)	( ABS(x)+ABS(y) ) /*-BA Only approx, but v. quick */
#define LIMIT(val, lo, hi)	( val=(val)>(hi)?(hi):((val)<(lo)?(lo):(val)) )


#ifndef MOD2
#  define MOD2(x, m)		( (x) & ((m) - 1) )
#endif	/* MOD2 */

/* borrowed from autobook */
#define XCALLOC(type, num) \
        ((type *) calloc ((num), sizeof(type)))
#define XMALLOC(type, num) \
        ((type *) malloc ((num) * sizeof(type)))
#define XREALLOC(type, p, num) \
        ((type *) realloc ((p), (num) * sizeof(type)))
#define XFREE(ptr) \
do { \
    if (ptr) { free(ptr);  ptr = NULL; } \
} while (0)

/* Use this to remove unused parameter warning. */
#define UNUSED_PARAM(x) x = x

/* Do NOT change these! */
#define OLD_MAX_CHECKS		26
#define MAX_TEAMS		10

#define EXPIRED_MINE_ID		4096   /* assume no player has this id */

#define MAX_CHARS		80
#define MSG_LEN			256

#define NUM_MODBANKS		4

#define SPEED_LIMIT		65.0
#define MAX_PLAYER_TURNSPEED	64.0
#define MIN_PLAYER_TURNSPEED	0.0
#define MAX_PLAYER_POWER	55.0
#define MIN_PLAYER_POWER	5.0
#define MAX_PLAYER_TURNRESISTANCE	1.0
#define MIN_PLAYER_TURNRESISTANCE	0.0

#define MAX_STATION_FUEL	500.0
#define TARGET_DAMAGE		250.0
#define SELF_DESTRUCT_DELAY	150.0

/*
 * Size (pixels) of radius for legal HIT!
 * Was 14 until 4.2. Increased due to 'analytical collision detection'
 * which inspects a real circle and not just a square anymore.
 */
#define SHIP_SZ		        16

#define VISIBILITY_DISTANCE	1000.0

#define BALL_RADIUS		10

#define MISSILE_LEN		15

#define TEAM_NOT_SET		0xffff

#define DEBRIS_TYPES		(8 * 4 * 4)

#undef rand
#define rand()	please dont use rand.

/*
 * The server supports only 4 colors, except for spark/debris, which
 * may have 8 different colors.
 */
#define NUM_COLORS	    4

#define BLACK		    0
#define WHITE		    1
#define BLUE		    2
#define RED		    3

/*
 * Windows deals in Pens, not Colors.  So each pen has to have all of its
 * attributes defined.
 */
#if defined(_WINDOWS) && !defined(PENS_OF_PLENTY)
#define	CLOAKCOLOROFS	15	/* colors 16 and 17 are dashed white/blue */
#define	MISSILECOLOR	18	/* wide white pen */
#define	LASERCOLOR	19	/* wide red pen */
#define	LASERTEAMCOLOR	20	/* wide blue pen */
#define	FUNKCOLORS	6	/* 6 funky colors here (15-20) */
#endif

/*
 * The minimum and maximum playing window sizes supported by the server.
 */
#define MIN_VIEW_SIZE	    384
#define MAX_VIEW_SIZE	    1024
#define DEF_VIEW_SIZE	    768

/*
 * Spark rand limits.
 */
#define MIN_SPARK_RAND	    0		/* Not display spark */
#define MAX_SPARK_RAND	    0x80	/* Always display spark */
#define DEF_SPARK_RAND	    0x55	/* 66% */

#define	UPDATE_SCORE_DELAY	(FPS)

/*
 * Polygon style flags
 */
#define STYLE_FILLED          (1U << 0)
#define STYLE_TEXTURED        (1U << 1)
#define STYLE_INVISIBLE       (1U << 2)
#define STYLE_INVISIBLE_RADAR (1U << 3)

#endif
