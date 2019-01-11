/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) TODO Erik Andersson
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

#ifndef CLIENTRANK_H
#define CLIENTRANK_H 1

extern char clientname[16];	/*assigned in xpilot.c :( */
extern char clientRankFile[PATH_MAX];
extern char clientRankHTMLFile[PATH_MAX];
extern char clientRankHTMLNOJSFile[PATH_MAX];

typedef struct ScoreNode {
    char nick[16];
    int timestamp;
    unsigned short kills;
    unsigned short deaths;
} ScoreNode;

void Init_saved_scores(void);

void Print_saved_scores(void);
void Paint_baseInfoOnMap(int x, int y);

void Add_rank_Kill(char *nick);
void Add_rank_Death(char *nick);
int Get_kills(char *nick);
int Get_deaths(char *nick);

#endif
