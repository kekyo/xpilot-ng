/**********************************************************************/
/*                                                                    */
/*  File:          bdf2c.c                                            */
/*  Author:        Andrew W. Scherpbier                               */
/*  Version:       1.00                                               */
/*  Created:       27 Feb 1994                     		      */
/*                                                                    */
/*  Copyright (c) 1991, 1992 Andrew Scherpbier                        */
/*                All Rights Reserved.                                */
/*                                                                    */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  convert a BDF font to a C include file              */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define	PUBLIC
#define	PRIVATE		static


/* Private routines
 * ================
 */
PRIVATE void bounds(void);
PRIVATE void start_char(char *name, FILE *fl);


/* Private variables
 * =================
 */


/* Public routines
 * ===============
 */


/* Public variables
 * ================
 */
int	fontheight, fontbase;
int	bpl;

struct
{
	char	*name;
	char	*chr;
} charmap[] =
{
	{ "space",	"' '" },
	{ "exclam",	"'!'" },
	{ "quotedbl",	"'\"'" },
	{ "numbersign",	"'#'" },
	{ "dollar",	"'$'" },
	{ "percent",	"'%'" },
	{ "ampersand",	"'&'" },
	{ "quoteright",	"'\\''" },
	{ "parenleft",	"'('" },
	{ "parenright",	"')'" },
	{ "asterisk",	"'*'" },
	{ "plus",	"'+'" },
	{ "comma",	"','" },
	{ "minus",	"'-'" },
	{ "period",	"'.'" },
	{ "slash",	"'/'" },
	{ "zero",	"'0'" },
	{ "one",	"'1'" },
	{ "two",	"'2'" },
	{ "three",	"'3'" },
	{ "four",	"'4'" },
	{ "five",	"'5'" },
	{ "six",	"'6'" },
	{ "seven",	"'7'" },
	{ "eight",	"'8'" },
	{ "nine",	"'9'" },
	{ "colon",	"':'" },
	{ "semicolon",	"';'" },
	{ "less",	"'<'" },
	{ "equal",	"'='" },
	{ "greater",	"'>'" },
	{ "question",	"'?'" },
	{ "at",		"'@'" },
	{ "bracketleft",	"'['" },
	{ "backslash",		"'\\\\'" },
	{ "bracketright",	"']'" },
	{ "asciicircum",	"'^'" },
	{ "underscore",		"'_'" },
	{ "quoteleft",		"'`'" },
	{ "braceleft",		"'{'" },
	{ "bar",		"'|'" },
	{ "braceright",		"'}'" },
	{ "asciitilde",		"'~'" },
	{ "A",		"'A'" },
	{ "B",		"'B'" },
	{ "C",		"'C'" },
	{ "D",		"'D'" },
	{ "E",		"'E'" },
	{ "F",		"'F'" },
	{ "G",		"'G'" },
	{ "H",		"'H'" },
	{ "I",		"'I'" },
	{ "J",		"'J'" },
	{ "K",		"'K'" },
	{ "L",		"'L'" },
	{ "M",		"'M'" },
	{ "N",		"'N'" },
	{ "O",		"'O'" },
	{ "P",		"'P'" },
	{ "Q",		"'Q'" },
	{ "R",		"'R'" },
	{ "S",		"'S'" },
	{ "T",		"'T'" },
	{ "U",		"'U'" },
	{ "V",		"'V'" },
	{ "W",		"'W'" },
	{ "X",		"'X'" },
	{ "Y",		"'Y'" },
	{ "Z",		"'Z'" },
	{ "a",		"'a'" },
	{ "b",		"'b'" },
	{ "c",		"'c'" },
	{ "d",		"'d'" },
	{ "e",		"'e'" },
	{ "f",		"'f'" },
	{ "g",		"'g'" },
	{ "h",		"'h'" },
	{ "i",		"'i'" },
	{ "j",		"'j'" },
	{ "k",		"'k'" },
	{ "l",		"'l'" },
	{ "m",		"'m'" },
	{ "n",		"'n'" },
	{ "o",		"'o'" },
	{ "p",		"'p'" },
	{ "q",		"'q'" },
	{ "r",		"'r'" },
	{ "s",		"'s'" },
	{ "t",		"'t'" },
	{ "u",		"'u'" },
	{ "v",		"'v'" },
	{ "w",		"'w'" },
	{ "x",		"'x'" },
	{ "y",		"'y'" },
	{ "z",		"'z'" },
	{ NULL,		NULL },
};

/**************************************************************************
 * PUBLIC int main(int ac, char **av)
 *
 */
PUBLIC int main(int ac, char **av)
{
	FILE	*fl = fopen(av[1], "r");
	char	buffer[1024];
	char	*token;

	if (!fl)
		exit(1);

	while (fgets(buffer, 1024, fl))
	{
		token = strtok(buffer, " \r\n");
		if (strcmp(token, "STARTCHAR") == 0)
			start_char(strtok(NULL, " \r\n"), fl);
		else if (strcmp(token, "FONTBOUNDINGBOX") == 0)
			bounds();
	}
	printf("{'\\0'}};\n");
	return 0;
}


/**************************************************************************
 * PRIVATE void start_char(char *name, FILE *fl)
 */
PRIVATE void start_char(char *name, FILE *fl)
{
	int	i;
	char	buffer[1024];
	int	read_data = 0;
	int	width, height, dummy, start;
	int	line = 0;

	/*
	 * Lookup the character
	 */
	for (i = 0; charmap[i].name; i++)
	{
		if (strcmp(name, charmap[i].name) == 0)
			break;
	}
	if (!charmap[i].name)
		return;		/* Not found */

	/*
	 * Output the C stuff
	 */
	printf("{%s,", charmap[i].chr);

	/*
	 * Search for the BITMAP line
	 */
	while (fgets(buffer, 1024, fl))
	{
		if (strncmp(buffer, "BITMAP", 6) == 0)
			read_data = 1;
		else if (strncmp(buffer, "BBX", 3) == 0)
		{
			int	i;
			sscanf(buffer + 4, "%d %d %d %d", &width, &height, &dummy, &start);
			line = fontbase - start - height;
			printf("{");
			for (i = 0; i < line * bpl; i++)
				printf("0,");
		}
		else if (read_data)
		{
			char	*p = buffer;
			int	count = 0;
			if (strncmp(buffer, "ENDCHAR", 7) == 0)
				break;

			while (isxdigit(*p))
			{
				int	value = 0;
				if (isdigit(*p))
					value |= *p - '0';
				else
					value |= *p - 'A' + 10;
				p++;
				value <<= 4;
				if (isdigit(*p))
					value |= *p - '0';
				else
					value |= *p - 'A' + 10;
				printf("%d,", value);
				p++;
				count++;
			}
			for (; count < bpl; count++)
				printf("0,");
			line++;
		}
	}
	printf("}, %d},\n", width);
}


/**************************************************************************
 * PRIVATE void bounds(void)
 *
 */
PRIVATE void bounds(void)
{
	char	*token = strtok(NULL, "\r\n");
	int	x, y;
	int	dummy, base;
	int	n;

	sscanf(token, "%d %d %d %d", &x, &y, &dummy, &base);
	n = y * ((x + 7) / 8);
	printf("#define FONTX %d\n#define FONTY %d\n", (x + 7) / 8, y);
	printf("struct{char ch; unsigned char data[%d]; int width;}font[]={\n", n);
	fontheight = y;
	fontbase = y + base;
	bpl = (x + 7) / 8;
}



