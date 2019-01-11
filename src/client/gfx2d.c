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

#include "xpclient.h"

char	*texturePath = NULL;    /* Configured list of texture directories */
char    *realTexturePath = NULL; /* Real texture lookup path */

/*
 *   Purpose: initialize xp_picture structure and load it from file.
 *   Error handling is incomplete.
 *
 *   return 0 on success.
 *   return -1 on error.
 */

int Picture_init (xp_picture_t *picture, const char *filename, int count)
{
    picture->count = count;
    picture->data = XMALLOC(RGB_COLOR *, ABS(count));
    if (!picture->data) {
	error("Not enough memory.");
	return -1;
    }

    if (Picture_load(picture, filename) == -1)
	return -1;

    if (count > 1)
        if (Picture_rotate(picture) == -1)
	    return -1;

    picture->bbox = XMALLOC(bbox_t, ABS(count));
    if (!picture->bbox) {
	error("Not enough memory.");
	return -1;
    }
    Picture_get_bounding_box(picture);

    return 0;
}


/*
 * Find full path for a picture filename.
 */
static int Picture_find_path(const char *filename, char *path,
			     size_t path_size)
{
    char		*dir, *colon;
    size_t		len;

    if (!filename || !*filename)
	return false;

    /*
     * If filename doesn't contain a slash
     * then we also try the realTexturePath, if it exists.
     */
    if (!strchr(filename, PATHNAME_SEP) && realTexturePath != NULL) {
	for (dir = realTexturePath; *dir; dir = colon) {
	    if (!(colon = strchr(dir, ':'))) {
		len = strlen(dir);
		colon = &dir[len];
	    } else {
		len = colon - dir;
		colon++;
	    }
	    if (len > 0 && len + strlen(filename) + 1 < path_size) {
		memcpy(path, dir, len);
		if (path[len - 1] != PATHNAME_SEP)
		    path[len++] = PATHNAME_SEP;
		strlcpy(&path[len], filename, path_size - len);
		/* kps - #ifndef R_OK #define R_OK 4 #endif */
		if (access(path, R_OK) == 0)
		    return true;
	    }
	}
    }

    /*error("Can't find PPM file \"%s\"", filename);*/
    return(false);
}

/*
 * Fetch next ASCII character from PPM file.
 * Strip comments starting with "#" from the input.
 * On error return EOF.
 */
static int Picture_getc(FILE *f)
{
    int			c;

    c = getc(f);
    if (c == '#') {
	do {
	    c = getc(f);
	} while (c != '\n' && c != EOF);
    }

    return c;
}

/*
 * Verify last input character is a whitespace character
 * and skip all following whitespace.
 * On error return EOF.
 */
static int Picture_skip_whitespace(FILE *f, int c)
{
    if (!isspace(c))
	return EOF;
    do {
	c = Picture_getc(f);
    } while (isspace(c));

    return c;
}

/*
 * Verify last input character is a digit
 * and extract a decimal value from the input stream.
 * On error return EOF.
 */
static int Picture_get_decimal(FILE *f, int c, int *dec)
{
    *dec = 0;
    for (; isdigit(c); c = Picture_getc(f))
	*dec = (10 * *dec) + (c - '0');
    return c;
}

/*
 * Purpose: load images in to the xp_picture structure.
 * format is only binary PPM's at the moment.
 * More error handling and a better understanding of the PPM standard
 * would be good. But suffices for a proof of concept.
 *
 * return 0 on success.
 * return -1 on error.
 */
int Picture_load(xp_picture_t *picture, const char *filename)
{
    FILE		*f;
    int			c, c1, c2;
    int			x, y;
    int			r, g, b;
    int			p;
    int			width, height, maxval, count;
    char		path[PATH_MAX + 1];


    if (!Picture_find_path(filename, path, sizeof(path))) {
	error("Cannot find picture file \"%s\"", filename);
	return -1;
    }

    if ((f = fopen(path, "rb")) == NULL) {
	error("Cannot open \"%s\"", path);
	return -1;
    }

    errno = 0;
    if ((c1 = Picture_getc(f)) != 'P' ||
	(c2 = Picture_getc(f)) != '6') {
	error("\"%s\" does not contain a valid binary PPM file.\n",
	       path);
	fclose(f);
	return -1;
    }
    c = Picture_getc(f);
    c = Picture_skip_whitespace(f, c);
    c = Picture_get_decimal(f, c, &width);
    c = Picture_skip_whitespace(f, c);
    c = Picture_get_decimal(f, c, &height);
    c = Picture_skip_whitespace(f, c);
    c = Picture_get_decimal(f, c, &maxval);

    if (!isspace(c) || maxval != 255) {
	error("\"%s\" does not contain a valid binary PPM file.\n",
	       path);
	fclose(f);
	return -1;
    }

    picture->height = height;
    if (picture->count > 0) {
	count = 1;
	picture->width = width;
    } else  {
	count = -picture->count;
	picture->width = width / count;
    }

    for (p = 0; p < count; p++) {
	if (!(picture->data[p] =
	      XMALLOC(RGB_COLOR, picture->width * picture->height))) {
	    error("Not enough memory.");
	    return -1;
	}
    }

    for (y = 0 ; y < (int)picture->height ; y++) {
	for (p = 0; p < count ; p++) {
	    for (x = 0; x < (int)picture->width ; x++) {
		r = getc(f);
		g = getc(f);
		b = getc(f);
		Picture_set_pixel(picture, p, x, y, RGB24(r, g, b));
	    }
	}
	/* skip the rest */
	for (p = width % count * 3; p > 0; p--)
	    getc(f);
    }

    fclose(f);

    return 0;
}

/*
 * Purpose: We want to provide rotation, a picture which is rotated has
 * just 1 image with index=0 as source, which is rotated 360 degrees,
 * more pictures = higher resolution.
 *
 * Note that this is done by traversing the target image, and selecting
 * the corresponding source colorvalue, this assures there will be no
 * gaps in the image.
 */
int Picture_rotate(xp_picture_t *picture)
{
    int size, x, y, image;
    RGB_COLOR color;
    
    size = picture->height;
    for (image = 1; image < picture->count; image++) {
        if (!(picture->data[image] =
              XMALLOC(RGB_COLOR, picture->width * picture->height))) {
            error("Not enough memory.");
            return -1;
        }
	for (y = 0; y < size; y++) {
	    for (x = 0; x < size; x++) {
		color = Picture_get_rotated_pixel(picture, x, y, image);
		Picture_set_pixel(picture, image, x, y, color);
	    }
	}
    }
    return 0;
}

/*
 * Purpose: set the color value of a 1x1 pixel,
 * This is a convenient wrapper for the data array.
 */
void Picture_set_pixel(xp_picture_t *picture, int image, int x, int y,
		       RGB_COLOR color)
{
    if (x < 0 || y < 0
	|| x >= (int)picture->width || y >= (int)picture->height) {
	;
	/*
	 * this might be an error, but it can be a convenience to allow the
	 * function to be called with indexes out of range, so i won't
	 * introduce error handling here
	 */
    } else
	picture->data[image][x + y * picture->width] = color;
}

/*
 * Purpose: get the color value of a 1x1 pixel,
 * This is a wrapper for looking up in the data array.
 */
RGB_COLOR Picture_get_pixel(const xp_picture_t *picture, int image,
			    int x, int y)
{
    if (x < 0 || y < 0
	|| x >= (int)picture->width || y >= (int)picture->height) {
	return RGB24(0, 0, 0);
	/*
	 * this might be an error, but it can be a convenience to allow the
	 * function to be called with indexes out of range, so i won't
	 * introduce error handling here. Return value is defaulted to black.
	 * There is already code that relies on this behavior
	 */
    } else
	return picture->data[image][x + y * picture->width];
}

/*
 * Purpose: Find the color value of the 1x1 pixel with upperleft corner x,y.
 * Note that x and y is doubles.
 */
static RGB_COLOR Picture_get_pixel_avg(const xp_picture_t *picture,
				       int image, double x, double y)
{
    int		r_x, r_y;
    double	frac_x, frac_y;
    int		i;
    RGB_COLOR	c[4];
    double	p[4];
    double	r, g, b;

    frac_x = x - (int)(x);
    frac_y = y - (int)(y);

    r_x = (int)x;
    r_y = (int)y;

    c[0] = Picture_get_pixel(picture, image, r_x, r_y);
    c[1] = Picture_get_pixel(picture, image, r_x + 1, r_y);
    c[2] = Picture_get_pixel(picture, image, r_x, r_y + 1);
    c[3] = Picture_get_pixel(picture, image, r_x + 1, r_y + 1);

    p[0] = (1 - frac_x) * (1 - frac_y);
    p[1] = (frac_x) * (1 - frac_y);
    p[2] = (1 - frac_x) * frac_y;
    p[3] = frac_x * frac_y;

    r = 0;
    g = 0;
    b = 0;

    for (i = 0; i < 4; i++) {
	r += RED_VALUE(c[i]) * p[i];
	g += GREEN_VALUE(c[i]) * p[i];
	b += BLUE_VALUE(c[i]) * p[i];
    }
    return RGB24((unsigned char)r, (unsigned char)g, (unsigned char)b);
}

/*
 * Purpose: Rotate a point around the center of an image
 * and return the matching color in the base image.
 * A picture that contains a rotated image uses all it images to make
 * a full 360 degree rotation, which is reflected in the angle calculation.
 * (first image is ang=0 and is used to index the texture for the color value)
 * Note: this function is used by the rotation code,
 * and that is why the it's rotating the "wrong" direction.
 */
RGB_COLOR Picture_get_rotated_pixel(const xp_picture_t *picture,
				    int x, int y, int image)
{
    int		angle;
    double	rot_x, rot_y;

    angle = ((image  * RES) / picture->count) % 128;

    x -= picture->width / 2;
    y -= picture->height / 2;

    rot_x = (tcos(angle) * x - tsin(angle) * y) + picture->width / 2;
    rot_y = (tsin(angle) * x + tcos(angle) * y) + picture->height / 2;

    return (Picture_get_pixel_avg(picture, 0, rot_x, rot_y));
}



#if 0
/*notused*/
/*
 *    Purpose: A helper function for adding up color values.
 */
static void color_Add_weight(int *r, int *g, int *b, RGB_COLOR col,
			     double weight)
{
    if (weight == 1) {
	*r += RED_VALUE(col);
	*g += GREEN_VALUE(col);
	*b += BLUE_VALUE(col);
    } else {
	*r += (int)(RED_VALUE(col) * weight);
        *g += (int)(GREEN_VALUE(col) * weight);
	*b += (int)(BLUE_VALUE(col) * weight);
    }
}
#endif


/*
 * Purpose: find color values from x + xfrac to x + xfrac + scale.
 * This is the most called function in the scaling routine,
 * so i address the picture data directly.
 */
static void Picture_scale_x_slice(const xp_picture_t * picture, int image,
				  int *r, int *g, int *b, int x, int y,
				  double xscale, double xfrac, double yfrac)

{
    double weight;
    RGB_COLOR col;
    RGB_COLOR *image_data = picture->data[image] + x + y * picture->width ;

    if (xscale > xfrac) {
	col = *image_data;
	weight = xfrac * yfrac;
	*r += (int)(RED_VALUE(col) * weight);
        *g += (int)(GREEN_VALUE(col) * weight);
	*b += (int)(BLUE_VALUE(col) * weight);

	xscale -= xfrac;
	image_data++;

        weight = yfrac;
	if (yfrac == 1) {
    	    while(xscale >= 1.0) {
		col = *image_data;
		*r += (int)(RED_VALUE(col));
		*g += (int)(GREEN_VALUE(col));
		*b += (int)(BLUE_VALUE(col));
		image_data++;
		xscale -=1.0;
	    }
	} else {
	    while(xscale >= 1.0) {
		col = *image_data;
		*r += (int)(RED_VALUE(col) * weight);
		*g += (int)(GREEN_VALUE(col) * weight);
		*b += (int)(BLUE_VALUE(col) * weight);
		image_data++;
		xscale -=1.0;
	    }
	}
    }
    if (xscale > .00001) {
	col = *image_data;
	weight = yfrac * xscale;
	*r += (int)(RED_VALUE(col) * weight);
        *g += (int)(GREEN_VALUE(col) * weight);
	*b += (int)(BLUE_VALUE(col) * weight);
    }
}

/*
 * Purpose: Calculate the average color of a rectangle in an image,
 * This is used by the scaling algorithm.
 */
RGB_COLOR Picture_get_pixel_area(const xp_picture_t *picture, int image,
				 double x_1, double y_1, double dx, double dy)
{
    int r, g, b;
    double area;

    int x, y;
    double xfrac, yfrac;

    r = 0;
    g = 0;
    b = 0;

    x = (int)x_1;
    y = (int)y_1;

    xfrac = (x + 1) - x_1;
    yfrac = (y + 1) - y_1;

    area = dx * dy;

    if (dy > yfrac) {
	Picture_scale_x_slice(picture, image, &r, &g, &b, x, y, dx,
			      xfrac, yfrac);
	dy -= yfrac;
	y++;
	while (dy >= 1.0) {
	    Picture_scale_x_slice(picture, image, &r, &g, &b, x, y, dx,
				  xfrac, 1.0);
	    y++;
	    dy -=1.0;
	}
    }
    if (dy > .00001)
	Picture_scale_x_slice(picture, image, &r, &g, &b, x, y, dx, xfrac, dy);

    return RGB24((unsigned char)(r/area), (unsigned char)(g/area),
		 (unsigned char)(b/area));
}

/*
 * Purpose: We want to know the bounding box of a picture,
 * so that we can reduce the number of operations done on
 * a picture.
 */
void Picture_get_bounding_box(xp_picture_t *picture)
{
    int		x, y, i;
    bbox_t	*box;

    for (i = 0; i < ABS(picture->count); i++) {
	box = &picture->bbox[i];
	box->xmin = picture->width - 1;
	box->xmax = 0;
	box->ymin = picture->height - 1;
	box->ymax = 0;

	for (y = 0 ; y < (int)picture->height ; y++) {
	    for (x = 0; x < (int)picture->width ; x++) {
		RGB_COLOR color = Picture_get_pixel(picture, i, x, y);
		if (color) {
		    if (box->xmin > x)
			box->xmin = x;
		    if (box->xmax < x)
			box->xmax = x;
		    if (box->ymin > y)
			box->ymin = y;
		    if (box->ymax < y)
			box->ymax = y;
		}
	    }
	}
    }
}
