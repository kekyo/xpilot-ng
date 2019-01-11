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

#include "xpclient.h"

#define MAX_SCORES 500

char clientRankFile[PATH_MAX];
char clientRankHTMLFile[PATH_MAX];
char clientRankHTMLNOJSFile[PATH_MAX];

/*
 * Defining one/both of CLIENTRANKINGPAGE/CLIENTNOJSRANKINGPAGE while leaving
 * CLIENTSCOREFILE undefined grants a rank of last/(current maybe aswell)
 * xpilot session.
 */
static ScoreNode scores[MAX_SCORES];
static int recent[10];
static int oldest_cache = 0;
static int timesort[MAX_SCORES];
static double kd[MAX_SCORES];
static int kdsort[MAX_SCORES];
static bool client_Scoring = false;

static void swapd(double *d1, double *d2)
{
    double d = *d1;
    *d1 = *d2;
    *d2 = d;
}

static void swapi(int *i1, int *i2)
{
    int i = *i1;
    *i1 = *i2;
    *i2 = i;
}


static void Time_Sort(void)
{
    int i;

    for (i = 0; i < MAX_SCORES; i++) {
	int j;

	for (j = i + 1; j < MAX_SCORES; j++) {
	    if (scores[timesort[i]].timestamp <
		scores[timesort[j]].timestamp)
		swapi(&timesort[i], &timesort[j]);
	}
    }
    for (i = 0; i < 10; i++)
	recent[i] = timesort[i];

    oldest_cache = 5;
}

/* This function checks wether the strings contains certain characters
   that might be hazardous to include on a webpage (ie they screw it up). */
static void LegalizeName(char string[])
{
    const int length = strlen(string);
    int i;

    for (i = 0; i < length; i++)
	switch (string[i]) {
	case '<':
	case '>':
	case '\t':
	    string[i] = '?';
	default:
	    break;
	}
}

/* Sort the ranks and save them to the webpage. */
static void Rank_score(void)
{
    static const char header[] =
	"<html><head><title>Xpilot Clientrank - Evolved by Mara</title>\n"
	/* In order to save space/bandwidth, the table is saved as one */
	/* giant javascript file, instead of writing all the <TR>, <TD>, etc */
	"<SCRIPT language=\"Javascript\">\n<!-- Hide script\n"
	"function g(nick, kills, deaths, ratio) {\n"
	"document.write('<tr><td align=left><tt>', i, '</tt></td>');\n"
	"document.write('<td align=left><b>', nick, '</b></td>');\n"
	"document.write('<td align=right>', kills, '</td>');\n"
	"document.write('<td align=right>', deaths, '</td>');\n"
	"document.write('</td>');\n"
	"document.write('<td align=right>', ratio, '</td>');\n"
	"document.write('</tr>\\n');\n"
	"i = i + 1\n" "}\n// Hide script --></SCRIPT>\n" "</head><body>\n"
	/* Head of page */
	"<h1>XPilot Clientrank - Evolved by Mara</h1>"
	"<noscript>"
	"<blink><h1>YOU MUST HAVE JAVASCRIPT FOR THIS PAGE</h1></blink>"
	"Please go <A href=\"index_nojs.html\">here</A> for the non-js page"
	"</noscript>\n"
	"<table><tr><td></td>"
	"<td align=left><h1><u><b>Player</b></u></h1></td>"
	"<td align=right><h1><u><b>Kills</b></u></h1></td>"
	"<td align=right><h1><u><b>Deaths</b></u></h1></td>"
	"<td align=right><h1><u><b>Ratio</b></u></h1></td>"
	"</tr>\n" "<SCRIPT language=\"Javascript\">\n" "var i = 1\n";

    static const char headernojs[] =
	"<html><head><title>XPilot Clientrank - Evolved by Mara</title>\n"
	"</head><body>\n"
	/* Head of page */
	"<h1>XPilot Clientrank</h1>"
	"<table><tr><td></td>"
	"<td align=left><h1><u><b>Player</b></u></h1></td>"
	"<td align=right><h1><u><b>Kills</b></u></h1></td>"
	"<td align=right><h1><u><b>Deaths</b></u></h1></td>"
	"<td align=right><h1><u><b>Ratio</b></u></h1></td>" "</tr>\n";

    static const char footer[] =
	"</table>"
	"<i>Explanation for rank</i>:<br>"
	"The numbers are k/d/r, where<br>"
	"k = The number of times he has shot me<br>"
	"d = The number of time I have shot him<br>"
	"r = the quota between k and d<br>" "</body></html>";

    int i;

    for (i = 0; i < MAX_SCORES; i++) {
	kdsort[i] = i;
	kd[i] =
	    (scores[i].deaths !=
	     0.0) ? ((double) (scores[i].kills) /
		     (double) (scores[i].deaths)) : 0.0;
    }

    for (i = 0; i < MAX_SCORES; i++) {
	int j;

	for (j = i + 1; j < MAX_SCORES; j++) {
	    if (kd[i] < kd[j]) {
		swapi(&kdsort[i], &kdsort[j]);
		swapd(&kd[i], &kd[j]);
	    }
	}
    }

    if (strlen(clientRankHTMLFile) > 0) {
	FILE *const file = fopen(clientRankHTMLFile, "w");

	if (file != NULL && fseek(file, 2000, SEEK_SET) == 0) {
	    fprintf(file, "%s", header);
	    for (i = 0; i < MAX_SCORES; i++) {
		if (scores[kdsort[i]].nick[0] != '\0') {
		    LegalizeName(scores[kdsort[i]].nick);
		    fprintf(file, "g(\"%s\", %u, %u, %.3f);\n",
			    scores[kdsort[i]].nick,
			    scores[kdsort[i]].kills,
			    scores[kdsort[i]].deaths, kd[i]);
		}
	    }
	    fprintf(file, "</script>");
	    fprintf(file, footer);
	    fclose(file);
	}
    }

    if (strlen(clientRankHTMLNOJSFile) > 0) {
	FILE *const file = fopen(clientRankHTMLNOJSFile, "w");

	if (file != NULL && fseek(file, 2000, SEEK_SET) == 0) {
	    fprintf(file, "%s", headernojs);
	    for (i = 0; i < MAX_SCORES; i++) {
		if (scores[kdsort[i]].nick[0] != '\0') {
		    LegalizeName(scores[kdsort[i]].nick);
		    fprintf(file,
			    "<tr><td align=left><tt>%d</tt>"
			    "<td align=left><b>%s</b>"
			    "<td align=right>%u"
			    "<td align=right>%u"
			    "<td align=right>%.3f"
			    "</tr>\n",
			    i + 1,
			    scores[kdsort[i]].nick,
			    scores[kdsort[i]].kills,
			    scores[kdsort[i]].deaths, kd[i]);
		}
	    }
	    fprintf(file, footer);
	    fclose(file);
	}
    }

    if (strlen(clientRankHTMLFile) == 0
	&& strlen(clientRankHTMLNOJSFile) == 0)
	warn("You have not specified clientRankHTMLFile or "
	     "clientRankHTMLNOJSFile.");
}

static void Init_scorenode(ScoreNode * node, const char nick[])
{
    strcpy(node->nick, nick);
    node->kills = 0;
    node->deaths = 0;
}

/*
 * Read scores from disk, and zero-initialize the ones that are not used.
 * Call this on startup.
 */
void Init_saved_scores(void)
{
    int i = 0;

    if (strlen(clientRankFile) > 0) {
	FILE *file = fopen(clientRankFile, "r");

	if (file != NULL) {
	    const int actual = fread(scores, sizeof(ScoreNode),
				     MAX_SCORES, file);
	    if (actual != MAX_SCORES)
		warn("Error when reading score file!\n");

	    i += actual;

	    fclose(file);
	}
	client_Scoring = true;
    }

    while (i < MAX_SCORES) {
	Init_scorenode(&scores[i], "");
	scores[i].timestamp = 0;
	timesort[i] = i;
	i++;
    }
    if (client_Scoring)
	Time_Sort();
}

static int Get_saved_score(char *nick)
{
    int oldest = 0;
    int i;

    for (i = 0; i < MAX_SCORES; i++) {
	if (strcmp(nick, scores[i].nick) == 0)
	    return i;
	if (scores[i].timestamp < scores[oldest].timestamp)
	    oldest = i;
    }

    Init_scorenode(&scores[oldest], nick);
    scores[oldest].timestamp = time(0);
    return oldest;
}


static int Find_player(char *nick)
{
    int i;
    for (i = 0; i < 10; i++) {
	/*if (scores[recent[i]].timestamp > 0) { */
	if (strcmp(nick, scores[recent[i]].nick) == 0)
	    return i;
    }
    i = Get_saved_score(nick);
    recent[oldest_cache] = i;
    i = oldest_cache;
    oldest_cache = (oldest_cache + 1) / 10;
    return i;
}


void Add_rank_Kill(char *nick)
{
    int i = Find_player(nick);

    scores[recent[i]].kills = scores[recent[i]].kills + 1;
    scores[recent[i]].timestamp = time(0);
}

void Add_rank_Death(char *nick)
{
    int i = Find_player(nick);

    scores[recent[i]].deaths = scores[recent[i]].deaths + 1;
    scores[recent[i]].timestamp = time(0);
}

int Get_kills(char *nick)
{
    int i = Find_player(nick);

    return scores[recent[i]].kills;
}

int Get_deaths(char *nick)
{
    int i = Find_player(nick);

    return scores[recent[i]].deaths;
}


/* Save the scores to disk (not the webpage). */
void Print_saved_scores(void)
{
    FILE *file = NULL;

    Rank_score();
    if (strlen(clientRankFile) > 0 &&
	(file = fopen(clientRankFile, "w")) != NULL) {
	const int actual = fwrite(scores, sizeof(ScoreNode),
				  MAX_SCORES, file);
	if (actual != MAX_SCORES)
	    warn("Error when writing score file!\n");

	fclose(file);
    }
}
