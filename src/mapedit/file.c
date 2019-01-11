/*
 * XPilot NG XP-MapEdit, a map editor for xp maps.  Copyright (C) 1993 by
 *
 *      Aaron Averill           <averila@oes.orst.edu>
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
 *
 * Modifications:
 * 1996:
 *      Robert Templeman        <mbcaprt@mphhpd.ph.man.ac.uk>
 * 1997:
 *      William Docter          <wad2@lehigh.edu>
 */

#include "xpmapedit.h"

Window filepromptwin;
max_str_t filepromptname = "continent";

char oldmap[90] = {
    ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ',
    '_', '+', ' ', '-', ' ',
    ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', '<', ' ',
    '>', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', '#', ' ',
    ' ', '(', ' ', ' ', ' ',
    ' ', ' ', ')', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ',
    '@', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ',
    'a', ' ', 'c', 'd', ' ',
    'f', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ',
    ' ', 'q', 'r', 's', ' ',
    ' ', ' ', 'w', 'x', ' '
};

/***************************************************************************/
/* GetMapDir                                                               */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
static char *GetMapDir(void)
{
    static char *mapdir;
    char *src, *dst;

    if (!mapdir) {
#if defined(MAPDIR)
	mapdir = (char *) malloc(sizeof(MAPDIR) + 1);
	strcpy(mapdir, MAPDIR);
#elif defined(LIBDIR)
	mapdir = (char *) malloc(sizeof(LIBDIR) + 10);
	strcpy(mapdir, LIBDIR);
	strcat(mapdir, "/maps");
#else
	static char default_mapdir[] = "/usr/local/games/lib/xpilot/maps";
	mapdir = (char *) malloc(sizeof(default_mapdir) + 1);
	strcpy(mapdir, default_mapdir);
#endif
	/* remove duplicate slashes. */
	for (dst = src = mapdir; (*dst = *src) != '\0'; src++) {
	    if (*dst != '/' || dst == mapdir || dst[-1] != '/') {
		dst++;
	    }
	}
	/* remove trailing slash. */
	if (dst > mapdir && dst[-1] == '/') {
	    *--dst = '\0';
	}
    }
    return mapdir;
}

/***************************************************************************/
/* SavePrompt                                                              */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int SavePrompt(HandlerInfo_t info)
{

    ClearSelectArea();
    T_PopupClose(changedwin);
    T_PopupClose(filepromptwin);

    strcpy(filepromptname, map.mapFileName);
    filepromptwin = T_PopupPrompt(-1, -1, 300, 150, "Save Map",
				  "Enter file name to save:", "Save", NULL,
				  filepromptname, sizeof(max_str_t),
				  SaveOk);
    return 0;
}

/***************************************************************************/
/* SaveOk                                                                  */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int SaveOk(HandlerInfo_t info)
{
    int len;

    len = strlen(filepromptname);

    if (len > 3) {
	if (strcmp(&filepromptname[len - 3], ".xp")) {
	    if (len > 4) {
		if (strcmp(&filepromptname[len - 4], ".map")) {
		    strcat(filepromptname, ".xp");
		}
	    } else {
		strcat(filepromptname, ".xp");
	    }
	}
    } else {
	strcat(filepromptname, ".xp");
    }

    SaveMap(filepromptname);
    T_PopupClose(info.form->window);
    map.changed = 0;
    return 0;
}

/***************************************************************************/
/* SaveMap                                                                 */
/* Arguments :                                                             */
/*   file                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int SaveMap(char *file)
{
    FILE *ofile = NULL;
    int n, i, j;
    char *tmpstr;
    time_t tim;

    if (strlen(file) == 0) {
	return 1;
    }
    ofile = fopen(file, "w");
    if (ofile == NULL) {
	tmpstr = (char *) malloc(strlen(file) + 20);
	sprintf(tmpstr, "Error saving file: %s", file);
	T_PopupAlert(1, tmpstr, NULL, NULL, NULL, NULL);
	free(tmpstr);
	return 1;
    }
    time(&tim);
    fprintf(ofile, "# Created by %s on %s\n", progname, ctime(&tim));
    if (map.comments != NULL) {
	fprintf(ofile, "%s\n", map.comments);
    }

    if ((map.worldLives != NULL) && (atoi(map.worldLives) != 0))
	fprintf(ofile, "limitedlives : yes\n");

    for (n = 0; n < numprefs; n++) {
	switch (prefs[n].type) {

	case MAPWIDTH:
	case MAPHEIGHT:
	case STRING:
	case INT:
	case POSINT:
	case FLOAT:
	case POSFLOAT:
	case COORD:

	    if (strlen(prefs[n].charvar) != (int) NULL)
		fprintf(ofile, "%s : %s\n", prefs[n].name,
			prefs[n].charvar);
	    break;

	case MAPDATA:
	    fprintf(ofile, "\nmapData: \\multiline: EndOfMapdata\n");
	    for (i = 0; i < map.height; i++) {
		for (j = 0; j < map.width; j++) {
		    fprintf(ofile, "%c", map.data[j][i]);
		}
		fprintf(ofile, "\n");
	    }
	    fprintf(ofile, "EndOfMapdata\n");
	    break;

	case YESNO:
	    if ((*prefs[n].intvar) == 0)
		break;
	    if ((*prefs[n].intvar) == 1)
		fprintf(ofile, "%s : no\n", prefs[n].name);
	    else
		fprintf(ofile, "%s : yes\n", prefs[n].name);
	    break;
	}
    }

    fclose(ofile);
    strcpy(map.mapFileName, file);
    return 0;
}

/***************************************************************************/
/* LoadPrompt                                                              */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int LoadPrompt(HandlerInfo_t info)
{

    ClearSelectArea();
    if (ChangedPrompt(LoadPrompt)) {
	return 1;
    }
    if (T_IsPopupOpen(changedwin)) {
	T_PopupClose(changedwin);
	changedwin = (Window) NULL;
    }
    if (T_IsPopupOpen(filepromptwin)) {
	T_PopupClose(filepromptwin);
    }
    filepromptname[0] = '\0';
    filepromptwin = T_PopupPrompt(-1, -1, 300, 150, "Load Map",
				  "Enter file name to load:", "Load", NULL,
				  filepromptname, sizeof(max_str_t),
				  LoadOk);
    return 0;
}

/***************************************************************************/
/* LoadOk                                                                  */
/* Arguments :                                                             */
/*   win                                                                   */
/*   name                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int LoadOk(HandlerInfo_t info)
{
    if (LoadMap(filepromptname)) {
	return 1;
    }
    T_PopupClose(info.form->window);
    map.view_x = map.view_y = 0;
    map.view_zoom = DEFAULT_MAP_ZOOM;
    map.changed = 0;
    ResetMap();
    ClearUndo();
    return 0;
}

/***************************************************************************/
/* LoadMap                                                                 */
/* Arguments :                                                             */
/*   file                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
int LoadMap(char *file)
{
    FILE *ifile = NULL;
    int ich;
    int corrupted = 0;
    char *filename, *tmpstr;
    int i, j;
    char *mapdir = GetMapDir();


    if (strlen(file) == 0)
	return 1;

    filename = (char *) malloc(strlen(file) + 1);
    strcpy(filename, file);
    ifile = fopen(filename, "r");	/* "FILE" */

    if (ifile == NULL) {
	free(filename);
	filename = (char *) malloc(strlen(file) + 4);
	sprintf(filename, "%s.xp", file);
	ifile = fopen(filename, "r");	/* "FILE.xp" */

	if (ifile == NULL) {
	    free(filename);
	    filename = (char *) malloc(strlen(file) + 5);
	    sprintf(filename, "%s.map", file);
	    ifile = fopen(filename, "r");	/* "FILE.map" */

	    if (ifile == NULL) {
		free(filename);
		filename =
		    (char *) malloc(strlen(file) + strlen(mapdir) + 2);
		sprintf(filename, "%s/%s", mapdir, file);
		ifile = fopen(filename, "r");	/* "MAPDIR/FILE" */

		if (ifile == NULL) {
		    free(filename);
		    filename =
			(char *) malloc(strlen(file) + strlen(mapdir) + 5);
		    sprintf(filename, "%s/%s.xp", mapdir, file);
		    ifile = fopen(filename, "r");	/* "MAPDIR/FILE.xp" */

		    if (ifile == NULL) {
			free(filename);
			filename =
			    (char *) malloc(strlen(file) + strlen(mapdir) +
					    6);
			sprintf(filename, "%s/%s.map", mapdir, file);
			ifile = fopen(filename, "r");	/* "MAPDIR/FILE.map" */

			if (ifile == NULL) {
			    tmpstr = (char *) malloc(strlen(file) + 21);
			    sprintf(tmpstr, "Couldn't find file: %s",
				    file);
			    T_PopupAlert(1, tmpstr, NULL, NULL, NULL,
					 NULL);
			    free(tmpstr);
			    return 1;
			}
		    }
		}
	    }
	}
    }
    if (map.comments)
	free(map.comments);
    map.comments = (char *) NULL;
    map.mapName[0] = map.mapAuthor[0] = map.gravity[0] = map.shipMass[0] =
	'\0';
    map.maxRobots[0] = map.worldLives[0] = '\0';
    map.view_zoom = DEFAULT_MAP_ZOOM;
    map.changed = map.edgeWrap = map.edgeBounce = map.teamPlay = 0;
    map.timing = 0;
    map.limitedVisibility = map.allowShields = 0;
    for (i = 0; i < MAX_MAP_SIZE; i++)
	for (j = 0; j < MAX_MAP_SIZE; j++)
	    map.data[i][j] = ' ';

    strcpy(map.mapFileName, filename);
    tmpstr = strrchr(filename, (int) '.');
    if (tmpstr != NULL) {
	if (strcmp(tmpstr, ".xbm") == 0) {
	    fclose(ifile);
	    return LoadXbmFile(filename);
	}
    }
    ich = getc(ifile);
    if (ich != EOF)
	ungetc(ich, ifile);
    if (isdigit(ich)) {
	fclose(ifile);
	return LoadOldMap(filename);
    } else {
	while (!feof(ifile)) {
	    if (ParseLine(ifile))
		corrupted = 1;
	}
    }
    if (ifile)
	fclose(ifile);
    if (corrupted) {
	T_PopupAlert(1, "Corrupted map file.", NULL, NULL, NULL, NULL);
    }
    return 0;
}

/***************************************************************************/
/* LoadXbmFile                                                             */
/* Arguments :                                                             */
/*   file                                                                  */
/* Purpose : Load a version 2 map                                          */
/***************************************************************************/
int LoadXbmFile(char *file)
{
    FILE *fp;
    max_str_t line;
    char *tmpstr, *tmp;
    int bits, x = 0, y = 0;

    if ((fp = fopen(file, "r")) == NULL) {
	tmpstr = (char *) malloc(strlen(file) + 21);
	sprintf(tmpstr, "Couldn't find file: %s", file);
	T_PopupAlert(1, tmpstr, NULL, NULL, NULL, NULL);
	free(tmpstr);
	return 1;
    }
    fgets(line, sizeof(max_str_t), fp);
    tmp = strrchr(line, (int) ' ');
    if (tmp == NULL)
	return 1;
    tmp++;
    map.width = atoi(tmp);
    sprintf(map.width_str, "%d", map.width);

    fgets(line, sizeof(max_str_t), fp);
    tmp = strrchr(line, (int) ' ');
    if (tmp == NULL)
	return 1;
    tmp++;
    map.height = atoi(tmp);
    sprintf(map.height_str, "%d", map.height);

    while ((fgets(line, sizeof(max_str_t), fp)) != 0) {
	tmp = strstr(line, "0x");
	while (tmp != NULL) {
	    tmp += 2;
	    if ((int) tmp[0] > 96) {
		bits = ((int) (tmp[0]) - 87) * 16;
	    } else {
		bits = ((int) (tmp[0]) - 48) * 16;
	    }
	    if ((int) tmp[1] > 96) {
		bits += (int) (tmp[1]) - 87;
	    } else {
		bits += (int) (tmp[1]) - 48;
	    }
	    if ((bits & 128) == 128)
		map.data[x + 7][y] = 'x';
	    if ((bits & 64) == 64)
		map.data[x + 6][y] = 'x';
	    if ((bits & 32) == 32)
		map.data[x + 5][y] = 'x';
	    if ((bits & 16) == 16)
		map.data[x + 4][y] = 'x';
	    if ((bits & 8) == 8)
		map.data[x + 3][y] = 'x';
	    if ((bits & 4) == 4)
		map.data[x + 2][y] = 'x';
	    if ((bits & 2) == 2)
		map.data[x + 1][y] = 'x';
	    if ((bits & 1) == 1)
		map.data[x][y] = 'x';
	    x += 8;
	    if (x >= map.width) {
		y++;
		x = 0;
	    }
	    tmp = strstr(tmp, "0x");
	}
    }
    fclose(fp);
    return 0;
}

/***************************************************************************/
/* LoadOldMap                                                              */
/* Arguments :                                                             */
/*   file                                                                  */
/* Purpose : Load a version 2 map                                          */
/***************************************************************************/
int LoadOldMap(char *file)
{
    FILE *fp;
    max_str_t line, filenm;
    int x, y, shortline, corrupted;
    int fchr = 32, rule;
    char *tmpstr;

    strcpy(filenm, file);
    if ((fp = fopen(filenm, "r")) == NULL) {
	tmpstr = (char *) malloc(strlen(file) + 21);
	sprintf(tmpstr, "Couldn't find file: %s", file);
	T_PopupAlert(1, tmpstr, NULL, NULL, NULL, NULL);
	free(tmpstr);
	return 1;
    }
    /* read in map x and y size */
    fgets(line, sizeof(max_str_t), fp);
    tmpstr = (char *) strstr(line, "x");
    tmpstr++;
    map.height = atoi(tmpstr);
    strncpy(map.height_str, tmpstr, strlen(tmpstr) - 1);
    tmpstr = (char *) strstr(line, "x");
    (*tmpstr) = '\0';
    map.width = atoi(line);
    strcpy(map.width_str, line);
/* read in map rule */
    fgets(line, sizeof(max_str_t), fp);
    rule = atoi(line);
    switch (rule) {
    case 6:
	map.edgeWrap = 2;
	break;
    }
/* get map name and author */
    fgets(line, sizeof(max_str_t), fp);
    strncpy(map.mapName, line, strlen(line) - 1);
    map.mapName[strlen(line) - 1] = '\0';
    fgets(line, sizeof(max_str_t), fp);
    strncpy(map.mapAuthor, line, strlen(line) - 1);
    map.mapAuthor[strlen(line) - 1] = '\0';

/* read in map */
    shortline = corrupted = 0;
    for (y = 0; y < (map.height); y++) {
	for (x = 0; x < (map.width); x++) {
	    if (shortline == 0)
		fchr = fgetc(fp);
	    if (fchr == '.')
		fchr = ' ';
	    else if ((fchr == '\n') || (fchr == EOF))
		shortline = corrupted = 1;
	    if (shortline == 0)
		map.data[x][y] = oldmap[fchr - 32];
	    else
		map.data[x][y] = ' ';
	}
	if (shortline == 0)
	    fgetc(fp);
	shortline = 0;
    }

    if (corrupted == 1) {
	T_PopupAlert(1, "Corrupted map file.", NULL, NULL, NULL, NULL);
    }
    fclose(fp);
    return 0;
}

/***************************************************************************/
/* toeol                                                                   */
/* Arguments :                                                             */
/*   ifile                                                                 */
/* Purpose :                                                               */
/***************************************************************************/
void toeol(FILE * ifile)
{
    int ich;

    while (!feof(ifile))
	if ((ich = getc(ifile)) == '\n') {
	    return;
	}
}

/***************************************************************************/
/* skipspace                                                               */
/* Arguments :                                                             */
/*   ifile                                                                 */
/* Purpose :                                                               */
/***************************************************************************/
char skipspace(FILE * ifile)
{
    int ich;

    while (!feof(ifile)) {
	ich = getc(ifile);
	if (ich == '\n') {
	    return ich;
	}
	if (!isascii(ich) || !isspace(ich))
	    return ich;
    }
    return EOF;
}

/***************************************************************************/
/* char *getMultilineValue                                                 */
/* Arguments :                                                             */
/*   delimiter                                                             */
/*    ifile                                                                */
/* Purpose :                                                               */
/***************************************************************************/
char *getMultilineValue(char *delimiter, FILE * ifile)
{
    char *s = (char *) malloc(32768);
    int i = 0;
    int slen = 32768;
    char *bol;
    int ich;

    bol = s;
    while (1) {
	ich = getc(ifile);
	if (ich == EOF) {
	    s = (char *) realloc(s, i + 1);
	    s[i] = '\0';
	    return s;
	}
	if (i == slen) {
	    char *t = s;

	    s = (char *) realloc(s, slen += 32768);
	    bol += s - t;
	}
	if (ich == '\n') {
	    s[i] = 0;
	    if (delimiter && !strcmp(bol, delimiter)) {
		char *t = s;

		s = (char *) realloc(s, bol - s + 1);
		s[bol - t] = '\0';
		return s;
	    }
	    bol = &s[i + 1];
	}
	s[i++] = ich;
    }
}

#define                  EXPAND                        \
if (i == slen) {                   \
   s = (char *) realloc(s, slen *= 2);      \
}
/***************************************************************************/
/* ParseLine                                                               */
/* Arguments :                                                             */
/*   ifile                                                                 */
/* Purpose :                                                               */
/***************************************************************************/
int ParseLine(FILE * ifile)
{
    int ich;
    char *value, *head, *name, *s = (char *) malloc(128);
    char *tmp, *commentline;
    int slen = 128;
    int i = 0;
    int override = 0;
    int multiline = 0;

    ich = getc(ifile);

    /* Skip blank lines... */
    if (ich == '\n') {
	free(s);
	return 0;
    }
    /* Skip leading space... */
    if (isascii(ich) && isspace(ich)) {
	ich = skipspace(ifile);
	if (ich == '\n') {
	    free(s);
	    return 0;
	}
    }
    /* Skip lines that start with comment character... */
    if (ich == '#') {
	commentline = malloc(2);
	sprintf(commentline, "#");
	ich = getc(ifile);
	while ((ich != EOF) && (ich != '\n')) {
	    tmp = malloc(strlen(commentline) + 2);
	    sprintf(tmp, "%s%c", commentline, ich);
	    free(commentline);
	    commentline = tmp;
	    ich = getc(ifile);
	}
	if (ich == '\n') {
	    tmp = malloc(strlen(commentline) + 2);
	    sprintf(tmp, "%s\n", commentline);
	    free(commentline);
	    commentline = tmp;
	}

	/* only add comment lines not created by xmapedit */
	if (strstr(commentline, "Created by") == NULL) {
	    if (map.comments == NULL) {
		map.comments = malloc(strlen(commentline) + 1);
		map.comments = commentline;
	    } else {
		tmp =
		    malloc(strlen(map.comments) + strlen(commentline) + 1);
		sprintf(tmp, "%s%s", map.comments, commentline);
		free(map.comments);
		free(commentline);
		map.comments = tmp;
	    }
	}
	free(s);
	return 0;
    }
    /* Skip lines that start with the end of the file... :') */
    if (ich == EOF) {
	free(s);
	return 0;
    }
    /* Start with ascii? */
    if (!isascii(ich) || !isalpha(ich)) {
	toeol(ifile);
	free(s);
	return 1;
    }
    s[i++] = ich;
    do {
	ich = getc(ifile);
	if (ich == '\n' || ich == '#' || ich == EOF) {
	    if (ich == '#')
		toeol(ifile);
	    free(s);
	    return 1;
	}
	if (isascii(ich) && isspace(ich))
	    continue;
	if (ich == ':')
	    break;
	EXPAND;
	s[i++] = ich;
    } while (1);

    ich = skipspace(ifile);

    EXPAND;
    s[i++] = '\0';
    name = s;

    s = (char *) malloc(slen = 128);
    i = 0;
    do {
	EXPAND;
	s[i++] = ich;
	ich = getc(ifile);
    } while (ich != EOF && ich != '#' && ich != '\n');

    if (ich == '#')
	toeol(ifile);

    EXPAND;
    s[i++] = 0;
    head = value = s;
    s = value + strlen(value) - 1;
    while (s >= value && isascii(*s) && isspace(*s))
	--s;
    *++s = 0;
    if (!strncmp(value, "\\override:", 10)) {
	override = 1;
	value += 10;
    }
    while (*value && isascii(*value) && isspace(*value))
	++value;
    if (!strncmp(value, "\\multiline:", 11)) {
	multiline = 1;
	value += 11;
    }
    while (*value && isascii(*value) && isspace(*value))
	++value;
    if (!*value) {
	free(name);
	free(head);
	return 1;
    }
    if (multiline)
	value = (char *) getMultilineValue(value, ifile);
    i = AddOption(name, value);

    free(name);
    free(head);
    return i;
}

/***************************************************************************/
/* AddOption                                                               */
/* Arguments :                                                             */
/*   name                                                                  */
/*    value                                                                */
/* Purpose :                                                               */
/***************************************************************************/
int AddOption(char *name, char *value)
{
    int option, i;
    char *tmp;

    for (i = 0; i < strlen(name); i++) {
	if (isupper(name[i]))
	    name[i] = tolower(name[i]);
    }
    for (option = 0; option < numprefs; option++) {
	if (!strcmp(name, prefs[option].name))
	    break;
	if (!strcmp(name, prefs[option].altname))
	    break;
    }
    if (option >= numprefs) {
	if (map.comments == NULL) {
	    map.comments = malloc(strlen(name) + strlen(value) + 3);
	    sprintf(map.comments, "%s:%s\n", name, value);
	} else {
	    tmp =
		malloc(strlen(map.comments) + strlen(name) +
		       strlen(value) + 3);
	    sprintf(tmp, "%s%s:%s\n", map.comments, name, value);
	    free(map.comments);
	    map.comments = tmp;
	}
	return 0;
    }

    switch (prefs[option].type) {

    case MAPDATA:
	return (LoadMapData(value));

    case MAPWIDTH:
	map.width = atoi(value);
	strncpy(map.width_str, value, 3);
	return 0;

    case MAPHEIGHT:
	map.height = atoi(value);
	strncpy(map.height_str, value, 3);
	return 0;

    case STRING:
    case COORD:
	strncpy(prefs[option].charvar, value, prefs[option].length);
	return 0;

    case YESNO:
	(*(prefs[option].intvar)) = YesNo(value) + 1;
	return 0;

    case INT:
    case POSINT:
    case FLOAT:
    case POSFLOAT:
	strcpy(prefs[option].charvar, StrToNum(value, prefs[option].length,
					       prefs[option].type));
	return 0;
    }
    return 1;
}

/***************************************************************************/
/* YesNo                                                                   */
/* Arguments :                                                             */
/*   val                                                                   */
/* Purpose :                                                               */
/***************************************************************************/
int YesNo(char *val)
{
    if ((tolower(val[0]) == 'y') || (tolower(val[0]) == 't'))
	return 1;
    return 0;
}

/***************************************************************************/
/* char *StrToNum                                                          */
/* Arguments :                                                             */
/*   string                                                                */
/*   len                                                                   */
/*   type                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
char *StrToNum(char *string, int len, int type)
{
    char *returnval;

    returnval = (char *) malloc(len + 1);
    returnval[0] = '\0';

    if (type == FLOAT || type == INT) {

	if ((string[0] == '-')
	    || ((string[0] >= '0') && (string[0] <= '9')))
	    sprintf(returnval, "%s%c", returnval, string[0]);

    } else if ((string[0] == '-') || ((string[0] >= '0') &&
				      (string[0] <= '9'))
	       || (string[0] == '.'))
	sprintf(returnval, "%s%c", returnval, string[0]);

    string++;
    while ((string[0] != '\0') && (strlen(returnval) <= (len - 1))) {

	if (type == FLOAT || type == POSFLOAT) {
	    /*         if ( ((string[0] >= '0') && (string[0] <= '9')) || (string[0] == '.')) */
	    sprintf(returnval, "%s%c", returnval, string[0]);

	} else if ((string[0] >= '0') && (string[0] <= '9'))
	    sprintf(returnval, "%s%c", returnval, string[0]);

	string++;
    }
    return (char *) returnval;
}

/***************************************************************************/
/* LoadMapData                                                             */
/* Arguments :                                                             */
/*   value                                                                 */
/* Purpose :                                                               */
/***************************************************************************/
int LoadMapData(char *value)
{
    int x = 0, y = 0;

    while (*value != '\0') {
	if (*value == '\n') {
	    x = 0;
	    y++;
	} else
	    map.data[x++][y] = *value;
	value++;
    }
    return 0;
}
