/**********************************************************************/
/*                                                                    */
/*  File:          maps2image.c                                       */
/*  Author:        Andrew W. Scherpbier                               */
/*  Version:       1.00                                               */
/*  Created:       27 Feb 1994                     		      */
/*                                                                    */
/*  Copyright (c) 1994 Andrew Scherpbier                              */
/*                All Rights Reserved.                                */
/*                                                                    */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  Description:  Convert a series of maps to an image                */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "font.h"

#define	STRING_HEIGHT	(FONTY)

#define	PUBLIC
#define	PRIVATE		static


/* Private routines
 * ================
 */
PRIVATE void one2eight(char data, char *image);
PRIVATE int build_char(char ch, char *image, int x, int y, int pos);
PRIVATE void add_title(char *image, int x, int y, char *filename);
PRIVATE char *convert_map(int width, int height, FILE *fl);
PRIVATE void build_image(char *image, int x, int y, FILE *fl);


/* Private variables
 * =================
 */


/* Public routines
 * ===============
 */


/* Public variables
 * ================
 */
int	mapsize = 64;
int	xcount = 0, ycount = 0;
int	count;
int	xsize, ysize;
int	label = 1;		/* Produce a map label by default */
int	invert = 0;
int	verbose = 0;
int	bitmap = 0;


/**************************************************************************
 * PUBLIC int main(int ac, char **av)
 *
 */
PUBLIC int main(int ac, char **av)
{
	int		c;
	extern char	*optarg;
	extern int	optind;
	char		*image;
	int		x, y;
	int		i;
	int		value;
	char		*p;

	/*
	 * Deal with arguments
	 */
	while ((c = getopt(ac, av, "s:x:ivbl")) != EOF)
	{
		switch (c)
		{
			case 's':
				mapsize = atoi(optarg);
				break;
			case 'x':
				xcount = atoi(optarg);
				break;
			case 'i':
				invert = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'b':
				bitmap = 1;
				break;
			case 'l':
				label = 0;
				break;
			default:
				fprintf(stderr, "usage: %s [-s size][-x nmaps][-i][-v][-l] xpmap ...\n", av[0]);
				exit(1);
		}
	}

	/*
	 *   compute the actual size of the resultant image. We do this by
	 *   taking the square root of the total number of maps and using
	 *   the truncated value as the y count. The x count is then
	 *   caluculated from this. Knowing the number of maps on a side,
	 *   we can then compute the total image size.
	 */
	count = ac - optind;

	if (count < 1)
	{
		fprintf(stderr, "%s: you need to specify at least one map\n", av[0]);
		exit(0);
	}

	if (xcount)
	{
		/*
		 * The user specified an X dimension
		 */
		ycount = (count + xcount - 1) / xcount;
	}
	else
	{
		ycount = sqrt((double) count);
		xcount = (count + ycount - 1) / ycount;
	}

	if (verbose)
		fprintf(stderr, "%d images.  Using %d by %d\n", count, xcount, ycount);

	xsize = (mapsize + 2) * xcount;
	if (label)
		ysize = (mapsize + 2 + STRING_HEIGHT) * ycount;
	else
	    	ysize = (mapsize + 2) * ycount;

	if (verbose)
		fprintf(stderr, "Image size: %d x %d\n", xsize, ysize);

	/*
	 *   Reserve enough space for the image.
	 */
	image = malloc(xsize * ysize);
	memset(image, invert, xsize * ysize);

	/*
	 *   Build the image
	 */
	x = 0; y = 0;
	for (i = optind; i < ac; i++)
	{
		char	*p = strrchr(av[i], '.');
		char	command[100];
		FILE	*fl;

		command[0] = '\0';
		if (p && strcmp(p, ".gz") == 0)
			sprintf(command, "gunzip<%s", av[i]);
		else if (p && strcmp(p, ".Z") == 0)
			sprintf(command, "uncompress<%s", av[i]);
		if (*command)
			fl = popen(command, "r");
		else
			fl = fopen(av[i], "r");

		if (!fl)
		{
			perror("open");
			exit(1);
		}

		build_image(image, x, y, fl);
		if (label)
			add_title(image, x, y, av[i]);

		if (*command)
			pclose(fl);
		else
			fclose(fl);

		x++;
		if (x >= xcount)
		{
			x = 0;
			y++;
		}
	}

	/*
	 *   Output the image.  The output image can be either pbm or bmp
	 */
	if (bitmap)
	{
		int	count = 0;
		int	Xsize = (xsize + 7) & 0xfff8;

		printf("#define maps_width %d\n#define maps_height %d\n", Xsize, ysize);
		printf("static unsigned char maps_bits[] = {\n   ");
		p = image;
		value = 0;
		i = 0x80;
		for (y = 0; y < ysize; y++)
		{
			for (x = 0; x < Xsize; x++)
			{
				if (*p)
					value |= 0x80;
				i >>= 1;
				if (i == 0)
				{
					printf("0x%02x, ", value & 0xff);
					count++;
					if (count >= 12)
					{
						count = 0;
						printf("\n   ");
					}
					i = 0x80;
					value = 0;
				}
				value >>= 1;
				p++;
			}
			if (i != 0x80)
			{
				printf("0x%02x, ", value & 0xff);
				count++;
				if (count >= 12)
				{
					count = 0;
					printf("\n   ");
				}
				i = 0x80;
				value = 0;
			}
#if 0
			value = 0;
#endif
			p -= Xsize - xsize;
		}
		printf("\n};\n");
	}
	else
	{
		printf("P4\n%d %d\n", xsize, ysize);
		p = image;
		for (y = 0; y < ysize; y++)
		{
			value = 0;
			i = 0x80;
			for (x = 0; x < xsize; x++)
			{
				if (*p)
					value |= i;
				i >>= 1;
				if (i == 0)
				{
					putchar(value);
					i = 0x80;
					value = 0;
				}
				p++;
			}
			if (i != 0x80)
				putchar(value);
			value = 0;
		}
	}
	if (verbose)
		fprintf(stderr, "\ndone\n");
	return 0;
}


/**************************************************************************
 * PRIVATE void build_image(char *image, int x, int y, FILE *fl)
 *   Read a map and create the image for it. The image is placed
 *   at location (x, y)
 */
PRIVATE void build_image(char *image, int x, int y, FILE *fl)
{
	int	width = 0;
	int	height = 0;
	char	buffer[1024];
	char	*token;

	if (!fl)
		return;

	if (verbose)
		fprintf(stderr, "\rbuilding %d x %d   ", x, y);

	while (fgets(buffer, sizeof(buffer), fl))
	{
		if (strncasecmp(buffer, "mapwidth", 8) == 0)
		{
			token = strtok(buffer, " :\r\t\n");
			width = atoi(strtok(NULL, " :\t\r\n"));
		}
		else if (strncasecmp(buffer, "mapheight", 9) == 0)
		{
			token = strtok(buffer, " :\r\t\n");
			height = atoi(strtok(NULL, " :\t\r\n"));
		}
		else if (strncasecmp(buffer, "mapdata", 7) == 0)
		{
			char	*p, *data, *topleft;
			int	j;

			if (!width || !height)
			{
				printf("Implementation fault: mapData while not mapWidth or mapHeight\n");
				exit(1);
			}
			p = data = convert_map(width, height, fl);

			/*
			 *   Compute the top left location of this image in the big output
			 *   image
			 */
			if (label)
				topleft = image + xsize * (mapsize + 2 + STRING_HEIGHT) * y + 1 + xsize + (mapsize + 2) * x;
			else
				topleft = image + xsize * (mapsize + 2) * y + 1 + xsize + (mapsize + 2) * x;
			for (j = 0; j < mapsize; j++)
			{
				memcpy(topleft, p, mapsize);
				topleft += xsize;
				p += mapsize;
			}
			free(data);
		}
	}
}


/**************************************************************************
 * PRIVATE void add_title(char *image, int x, int y, char *filename)
 *   Create the title for the map below the map image. The title
 *   is just the file name without the .map at the end.
 */
PRIVATE void add_title(char *image, int x, int y, char *filename)
{
	char	*p = strrchr(filename, '/');
	int	pos;
	char	*s;
	char	*botleft;
	int	j;

	if (!p)
		p = filename;
	else
		p++;
	s = strrchr(p, '.');
	if (s)
		*s = '\0';

	/*
	 *   Make the string image
	 */
	s = malloc(mapsize * STRING_HEIGHT);
	memset(s, 0, mapsize * STRING_HEIGHT);
	memset(s, invert, mapsize);
	pos = 0;
	while (*p)
	{
		pos = build_char(*p, s, mapsize, STRING_HEIGHT, pos);
		p++;
	}

	/*
	 *   Find the bottom of the image so we can put the string there.
	 */
	botleft = image + xsize * (mapsize + 2 + STRING_HEIGHT) * y + 1 + xsize + (mapsize + 2) * x;
	botleft += xsize * mapsize;
	p = s;
	for (j = 0; j < STRING_HEIGHT; j++)
	{
		memcpy(botleft, p, mapsize);
		botleft += xsize;
		p += mapsize;
	}
	free(s);
}


/**************************************************************************
 * PRIVATE int build_char(char ch, char *image, int x, int y, int pos)
 *
 */
PRIVATE int build_char(char ch, char *image, int x, int y, int pos)
{
	int	i, j, n;
	char	*p;
	char	*data;

	/*
	 *   Locate the character
	 */
	for (i = 0; font[i].ch; i++)
	{
		if (font[i].ch == ch)
			break;
	}
	if (font[i].ch == '\0')
		return pos;

	if (pos + font[i].width > x)
		return pos;		/* Don't overwrite the bounding box */

	p = image + pos;
	data = (char *) font[i].data;
	for (n = 0; n <= FONTY; n++)
	{
		for (j = 0; j < FONTX; j++)
		{
			one2eight(*data++, p + j * 8);
		}
		p += x;
	}
	return pos + font[i].width + 1;
}


/**************************************************************************
 * PRIVATE void one2eight(char data, char *image)
 *   Convert one byte (8 bits) to a sequence of 8 bytes.
 */
PRIVATE void one2eight(char data, char *image)
{
	int	mask = 0x80;
	int	i;

	for (i = 0; i < 8; i++)
	{
		if (data & mask)
			*image = 1;
		image++;
		mask >>= 1;
	}
}


/**************************************************************************
 * PRIVATE char *convert_map(int width, int height, FILE *fl)
 *   Create an image from map data.
 */
PRIVATE char *convert_map(int width, int height, FILE *fl)
{
	char	buffer[10240];
	char	*p;
	float	div;
	int	x, y;
	int	lx, ly, px, py;
	int	value = 1;
	int	i, j;
	char	*output = malloc(mapsize * mapsize);

	if (width >= height)
		div = ((float) mapsize) / width;
	else
		div = ((float) mapsize) / height;

	value ^= invert;

	y = 0;
	memset((char *) output, invert, mapsize * mapsize);

	/*
	 *   If the map is smaller than the image we are producing, we
	 *   need to scale the map, otherwise we need to scale the image.
	 */
	if (div > 1)
	{
		ly = 0;
		px = 0;
		py = 0;
		while (fgets(buffer, sizeof(buffer), fl) && y < height)
		{
			x = 0;
			px = 0;
			p = buffer;
			while (*p && x < width)
			{
				lx = (x + 1) * div;
				if (strchr("xswqa#", *p))
				{
					for (i = px; i < lx; i++)
						output[ly * mapsize + i] = value;
				}
				else
				{
					for (i = px; i <= lx; i++)
						output[ly * mapsize + i] = !value;
				}
				px = lx;
				p++;
				x++;
			}
			y++;
			px = 0;
			ly = y * div;
			/*
			 *   Copy the previous line as many times as needed...
			 */
			for (j = py; j < ly; j++)
			{
				memcpy(&output[j * mapsize], &output[py * mapsize], mapsize);
			}
			py = ly;
		}
	}
	else
	{
		while (fgets(buffer, sizeof(buffer), fl) && y < height)
		{
			x = 0;
			p = buffer;
			while (*p && x < width)
			{
				if (strchr("xswqa#", *p))
				{
					output[(int)(y * div + 0.5) * mapsize + (int)(x * div + 0.5)] = value;
				}
				p++;
				x++;
			}
			y++;
		}
	}
	return output;
}



