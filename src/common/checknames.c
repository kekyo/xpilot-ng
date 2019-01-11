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

#include "xpcommon.h"

int Check_user_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_NAME_LEN - 1)
	return NAME_ERROR;
    if (!*name)
	return NAME_ERROR;

    str = (unsigned char *) name;
    for (; *str; str++) {
	if (!isgraph(*str))
	    return NAME_ERROR;
    }

    return NAME_OK;
}

void Fix_user_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_NAME_LEN - 1)
	name[MAX_NAME_LEN - 1] = 0;
    if (!*name) {
	strlcpy(name, "X", sizeof(name));
	return;
    }
    str = (unsigned char *) name;
    for (; *str; str++) {
	if (!isgraph(*str))
	    *str = 'x';
    }
}

int Check_nick_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_NAME_LEN - 1)
	return NAME_ERROR;
    if (!*name)
	return NAME_ERROR;

    str = (unsigned char *) name;
    if (!isupper(*str))
	return NAME_ERROR;

    for (; *str; str++) {
	if (!isprint(*str) || *str == PROT_EXT)
	    return NAME_ERROR;
    }
    --str;
    if (isspace(*str))
	return NAME_ERROR;

    return NAME_OK;
}

void Fix_nick_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_NAME_LEN - 1)
	name[MAX_NAME_LEN - 1] = 0;
    if (!*name) {
	static int n;
	sprintf(name, "X%d", n++);
	return;
    }
    str = (unsigned char *) name;
    if (!isupper(*str)) {
	if (islower(*str))
	    *str = toupper(*str);
	else
	    *str = 'X';
    }
    for (; *str; str++) {
	if (!isprint(*str) || *str == PROT_EXT)
	    *str = 'x';
    }
    --str;
    while (isspace(*str))
	*str-- = '\0';
}

/* isalnum() depends on locale. */
static bool is_alpha_numeric(int c)
{
    if (c >= 'A' && c <= 'Z')
	return true;
    if (c >= 'a' && c <= 'z')
	return true;
    if (c >= '0' && c <= '9')
	return true;
    return false;
}

int Check_host_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_HOST_LEN - 1)
	return NAME_ERROR;
    str = (unsigned char *) name;
    if (!is_alpha_numeric(*str))
	return NAME_ERROR;
    for (; *str; str++) {
	if (!is_alpha_numeric(*str)) {
	    if (*str == '.' || *str == '-') {
		if (str[1] == '.' || str[1] == '-' || !str[1])
		    return NAME_ERROR;
	    } else
		return NAME_ERROR;
	}
    }
    return NAME_OK;
}

void Fix_host_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_HOST_LEN - 1)
	name[MAX_HOST_LEN - 1] = 0;
    str = (unsigned char *) name;
    if (!is_alpha_numeric(*str)) {
	strlcpy(name, "xxx.xxx", sizeof(name));
	return;
    }
    for (; *str; str++) {
	if (!is_alpha_numeric(*str)) {
	    if (*str == '.' || *str == '-') {
		if (str[1] == '.' || str[1] == '-' || !str[1])
		    *str = 'x';
	    } else
		*str = 'x';
	}
    }
}

/*
 */
int Check_disp_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_DISP_LEN - 1)
	return NAME_ERROR;
    str = (unsigned char *) name;
    for (; *str; str++) {
	if (!isgraph(*str))
	    return NAME_ERROR;
    }
    return NAME_OK;
}

void Fix_disp_name(char *name)
{
    unsigned char *str;

    if (strlen(name) > MAX_DISP_LEN - 1)
	name[MAX_DISP_LEN - 1] = 0;
    str = (unsigned char *) name;
    for (; *str; str++) {
	if (!isgraph(*str))
	    *str = 'x';
    }
}
