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

#include "xpclient_x11.h"

/* Kludge for visuals under C++ */
#if defined(__cplusplus)
#define class c_class
#endif


/*
 * The number of X11 visuals.
 */
#define MAX_VISUAL_CLASS	4


/*
 * Default colors.
 */
#define XP_COLOR0		"#000000"		/* black */
#define XP_COLOR1		"#FFFFFF"		/* white */
#define XP_COLOR2		"#4E7CFF"		/* "xpblue" */
#define XP_COLOR3		"#FF3A27"		/* "xpred" */
#define XP_COLOR4		"#33BB44"		/* "xpgreen" */
#define XP_COLOR5		"#992200"
#define XP_COLOR6		"#BB7700"
#define XP_COLOR7		"#EE9900"
#define XP_COLOR8		"#002299"
#define XP_COLOR9		"#CC4400"
#define XP_COLOR10		"#DD8800"
#define XP_COLOR11		"#FFBB11"		/* "xpyellow" */
#define XP_COLOR12		"#9F9F9F"
#define XP_COLOR13		"#5F5F5F"
#define XP_COLOR14		"#DFDFDF"
#define XP_COLOR15		"#202020"

static char		color_names[MAX_COLORS][MAX_COLOR_LEN];
static const char	*color_defaults[MAX_COLORS] = {
    XP_COLOR0,  XP_COLOR1,  XP_COLOR2,  XP_COLOR3,
    XP_COLOR4,  XP_COLOR5,  XP_COLOR6,  XP_COLOR7,
    XP_COLOR8,  XP_COLOR9,  XP_COLOR10, XP_COLOR11,
    XP_COLOR12, XP_COLOR13, XP_COLOR14, XP_COLOR15
};

char		visualName[MAX_VISUAL_NAME];
Visual		*visual;
unsigned	dispDepth;
bool		colorSwitch;
bool		multibuffer;
bool		fullColor;	/* Whether to try using colors as close to
				 * the specified ones as possible, or just
				 * use a few standard colors for everything. */
bool		texturedObjects; /* Whether to draw bitmaps for some objects.
				  * Previously this variable determined
				  * fullColor too. */
int		maxColors;	/* Max. number of colors to use */
XColor		colors[MAX_COLORS];
Colormap	colormap;	/* Private colormap */

char	sparkColors[MSG_LEN];
int	spark_color[MAX_COLORS];

int	buttonColor;		/* Color index for button drawing */
int	windowColor;		/* Color index for window drawing */
int	borderColor;		/* Color index for border drawing */
int	wallColor;		/* Color index for wall drawing */
int	decorColor;		/* Color index for decoration drawing */


/*
 * Dimensions of color cubes in decreasing
 * total number of colors used.
 */
static struct rgb_cube_size {
    unsigned char	r, g, b;
} rgb_cube_sizes[] = {
    { 6, 6, 5 },	/* 180 */
    { 5, 6, 5 },	/* 150 */
    { 6, 6, 4 },	/* 144 */
    { 5, 5, 5 },	/* 125 */
    { 5, 6, 4 },	/* 120 */
    { 6, 6, 3 },	/* 108 */
    { 5, 5, 4 },	/* 100 */
    { 5, 6, 3 },	/* 90 */
    { 4, 5, 4 },	/* 80 */
    { 5, 5, 3 },	/* 75 */
    { 4, 4, 4 },	/* 64 */
    { 4, 5, 3 },	/* 60 */
    { 4, 4, 3 },	/* 48 */
};

/* This assumes that the RGB values are encoded in 24 bits
 * so that bits 0-7 encode the blue value, bits 8-15 encode
 * the green value and bits 16-24 encode the red value.
 */
#define RGB2COLOR(c) RGB(((c) >> 16) & 255, ((c) >> 8) & 255, ((c) & 255))

unsigned long		(*RGB)(int r, int g, int b) = NULL;
static unsigned long	RGB_PC(int r, int g, int b);
static unsigned long	RGB_TC(int r, int g, int b);

/*
 * Visual names.
 */
static struct Visual_class_name {
    int		visual_class;
    const char	*visual_name;
} visual_class_names[MAX_VISUAL_CLASS] = {
    { StaticColor,	"StaticColor" },
    { PseudoColor,	"PseudoColor" },
    { TrueColor,	"TrueColor"   },
    { DirectColor,	"DirectColor" }
};

/*
 * Structure to hold pixel information
 * for a color cube for PseudoColor visuals.
 */
struct Color_cube {
    int				reds;
    int				greens;
    int				blues;
    int				mustfree;
    unsigned long		pixels[256];
};
static struct Color_cube	*color_cube;

/*
 * Structure to hold pixel information
 * for a true color visual.
 */
struct True_color {
    unsigned long		red_bits[256];
    unsigned long		green_bits[256];
    unsigned long		blue_bits[256];
};
static struct True_color	*true_color;

static void Colors_init_radar_hack(void);
static int  Colors_init_color_cube(void);
static int  Colors_init_true_color(void);

/*
 * Create a private colormap.
 */
static void Get_colormap(void)
{
    printf("Creating a private colormap\n");
    colormap = XCreateColormap(dpy, DefaultRootWindow(dpy),
			       visual, AllocNone);
}


/*
 * Convert a visual class to its name.
 */
static const char *Visual_class_name(int visual_class)
{
    int			i;

    for (i = 0; i < MAX_VISUAL_CLASS; i++) {
	if (visual_class_names[i].visual_class == visual_class)
	    return visual_class_names[i].visual_name;
    }
    return "UnknownVisual";
}


/*
 * List the available visuals for the default screen.
 */
void List_visuals(void)
{
    int				i,
				num;
    XVisualInfo			*vinfo_ptr,
				my_vinfo;
    long			mask;

    num = 0;
    mask = 0;
    my_vinfo.screen = DefaultScreen(dpy);
    mask |= VisualScreenMask;
    vinfo_ptr = XGetVisualInfo(dpy, mask, &my_vinfo, &num);
    printf("Listing all visuals:\n");
    for (i = 0; i < num; i++) {
	printf("Visual class    %12s\n",
	       Visual_class_name(vinfo_ptr[i].class));
	printf("    id                  0x%02x\n", (unsigned)vinfo_ptr[i].visualid);
	printf("    screen          %8d\n", vinfo_ptr[i].screen);
	printf("    depth           %8d\n", vinfo_ptr[i].depth);
	printf("    red_mask        0x%06x\n", (unsigned)vinfo_ptr[i].red_mask);
	printf("    green_mask      0x%06x\n", (unsigned)vinfo_ptr[i].green_mask);
	printf("    blue_mask       0x%06x\n", (unsigned)vinfo_ptr[i].blue_mask);
	printf("    colormap_size   %8d\n", vinfo_ptr[i].colormap_size);
	printf("    bits_per_rgb    %8d\n", vinfo_ptr[i].bits_per_rgb);
    }
    XFree((void *) vinfo_ptr);

#ifdef DEVELOPMENT
    dbuff_list(dpy);
#endif
}


/*
 * Support all available visuals.
 */
static void Choose_visual(void)
{
    int				i,
				num,
				best_size,
				cmap_size,
				using_default,
				visual_id,
				visual_class;
    XVisualInfo			*vinfo_ptr,
				my_vinfo,
				*best_vinfo;
    long			mask;

    visual_id = -1;
    visual_class = -1;
    if (visualName[0] != '\0') {
	if (strncmp(visualName, "0x", 2) == 0) {
	    unsigned int vid;

	    if (sscanf(visualName, "%x", &vid) < 1) {
		warn("Bad visual id \"%s\", using default\n", visualName);
		visual_id = -1;
	    } else
		visual_id = vid;
	} else {
	    for (i = 0; i < MAX_VISUAL_CLASS; i++) {
		if (strncasecmp(visualName, visual_class_names[i].visual_name,
				strlen(visual_class_names[i].visual_name))
				== 0) {
		    visual_class = visual_class_names[i].visual_class;
		    break;
		}
	    }
	    if (visual_class == -1) {
		warn("Unknown visual class named \"%s\", using default\n",
		     visualName);
	    }
	}
    }
    if (visual_class < 0 && visual_id < 0) {
	visual = DefaultVisual(dpy, DefaultScreen(dpy));
	if (visual->class == TrueColor || visual->class == DirectColor) {
	    visual_class = PseudoColor;
	    strcpy(visualName, "PseudoColor");
	}
	using_default = true;
    } else
	using_default = false;

    if (visual_class >= 0 || visual_id >= 0) {
	mask = 0;
	my_vinfo.screen = DefaultScreen(dpy);
	mask |= VisualScreenMask;
	if (visual_class >= 0) {
	    my_vinfo.class = visual_class;
	    mask |= VisualClassMask;
	}
	if (visual_id >= 0) {
	    my_vinfo.visualid = visual_id;
	    mask |= VisualIDMask;
	}
	num = 0;
	if ((vinfo_ptr = XGetVisualInfo(dpy, mask, &my_vinfo, &num)) == NULL
	    || num <= 0) {
	    if (using_default == false)
		warn("No visuals available with class name \"%s\", "
		     "using default", visualName);
	    visual_class = -1;
	}
	else {
	    best_vinfo = vinfo_ptr;
	    for (i = 1; i < num; i++) {
		best_size = best_vinfo->colormap_size;
		cmap_size = vinfo_ptr[i].colormap_size;
		if (cmap_size > best_size) {
		    if (best_size < 256)
			best_vinfo = &vinfo_ptr[i];
		}
		else if (cmap_size >= 256)
		    best_vinfo = &vinfo_ptr[i];
	    }
	    visual = best_vinfo->visual;
	    visual_class = best_vinfo->class;
	    dispDepth = best_vinfo->depth;
	    XFree((void *) vinfo_ptr);
	    printf("Using visual %s with depth %d and %d colors\n",
		   Visual_class_name(visual->class), dispDepth,
		   visual->map_entries);
	    Get_colormap();
	}
    }
    if (visual_class < 0) {
	visual = DefaultVisual(dpy, DefaultScreen(dpy));
	dispDepth = DefaultDepth(dpy, DefaultScreen(dpy));
	colormap = 0;
    }
}


/*
 * Parse the user configurable color definitions.
 */
static int Parse_colors(Colormap cmap)
{
    int			i;
    const char		**def = &color_defaults[0];

    /*
     * Get the color definitions.
     */
    for (i = 0; i < maxColors; i++) {
	if (strlen(color_names[i]) > 0) {
	    char cname[MAX_COLOR_LEN], *tmp;

	    strlcpy(cname, color_names[i], sizeof(cname));
	    tmp = strtok(cname, " \t\r\n");

	    if (tmp && XParseColor(dpy, cmap, tmp, &colors[i]))
		continue;
	    warn("Can't parse color %d \"%s\".", i, color_names[i]);
	}
	if (def[i] != NULL && strlen(def[i]) > 0) {
	    if (XParseColor(dpy, cmap, def[i], &colors[i]))
		continue;
	    warn("Can't parse default color %d \"%s\".", i, def[i]);
	}
	if (i < NUM_COLORS)
	    return -1;
	else
	    colors[i] = colors[i % NUM_COLORS];
    }
    return 0;
}


/*
 * If we have a private colormap and color switching is on then
 * copy the first few colors from the default colormap into it
 * to prevent ugly color effects on the rest of the screen.
 */
static void Fill_colormap(void)
{
    int			i,
			cells_needed,
			max_fill;
    unsigned long	pixels[256];
    XColor		mycolors[256];

    if (colormap == 0 || colorSwitch != true)
	return;

    cells_needed = (maxColors == 16) ? 256
	: (maxColors == 8) ? 64
	: 16;
    max_fill = MAX(256, visual->map_entries) - cells_needed;
    if (max_fill <= 0)
	return;

    if (XAllocColorCells(dpy, colormap,
			 False, NULL,
			 0, pixels, (unsigned)max_fill) == False) {
	warn("Can't pre-alloc color cells");
	return;
    }

    /* Check for misunderstanding of X colormap stuff. */
    for (i = 0; i < max_fill; i++) {
	if (i != (int) pixels[i]) {
#ifdef DEVELOPMENT
	    warn("Can't pre-fill color map, got %d'th pixel %lu",
		 i, pixels[i]);
#endif
	    XFreeColors(dpy, colormap, pixels, max_fill, 0);
	    return;
	}
    }
    for (i = 0; i < max_fill; i++)
	mycolors[i].pixel = pixels[i];
    XQueryColors(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
		 mycolors, max_fill);
    XStoreColors(dpy, colormap, mycolors, max_fill);
}


/*
 * Setup color and double buffering resources.
 * It returns 0 if the initialization was successful,
 * or -1 if it couldn't initialize the double buffering routine.
 */
int Colors_init(void)
{
    int				i;
    unsigned			num_planes;

    colormap = 0;

    Choose_visual();

    /*
     * Get misc. display info.
     */
    if (visual->class == StaticColor ||
	visual->class == TrueColor)
	colorSwitch = false;

    if (visual->map_entries < 16)
	colorSwitch = false;

    if (colorSwitch) {
	maxColors = (maxColors >= 16 && visual->map_entries >= 256) ? 16
	    : (maxColors >= 8 && visual->map_entries >= 64) ? 8
	    : 4;
    }
    else {
	maxColors = (maxColors >= 16 && visual->map_entries >= 16) ? 16
	    : (maxColors >= 8 && visual->map_entries >= 8) ? 8
	    : 4;
    }
    num_planes = (maxColors == 16) ? 4
	: (maxColors == 8) ? 3
	: 2;

    if (Parse_colors(DefaultColormap(dpy, DefaultScreen(dpy))) == -1) {
	warn("Color parsing failed.");
	return -1;
    }

    if (colormap != 0)
	Fill_colormap();

    /*
     * Initialize the double buffering routine.
     */
    dbuf_state = NULL;

    if (multibuffer)
	dbuf_state = start_dbuff(dpy,
				 (colormap != 0)
				     ? colormap
				     : DefaultColormap(dpy,
						       DefaultScreen(dpy)),
				 MULTIBUFFER,
				 num_planes,
				 colors);
    if (dbuf_state == NULL)
	dbuf_state = start_dbuff(dpy,
				 (colormap != 0)
				     ? colormap
				     : DefaultColormap(dpy,
						       DefaultScreen(dpy)),
				 ((colorSwitch) ? COLOR_SWITCH : PIXMAP_COPY),
				 num_planes,
				 colors);
    if (dbuf_state == NULL && colormap == 0) {

	/*
	 * Create a private colormap if we can't allocate enough colors.
	 */
	Get_colormap();
	Fill_colormap();

	/*
	 * Try to initialize the double buffering again.
	 */

	if (multibuffer)
	    dbuf_state = start_dbuff(dpy, colormap, MULTIBUFFER, num_planes,
				     colors);

	if (dbuf_state == NULL)
	    dbuf_state = start_dbuff(dpy, colormap,
				     ((colorSwitch)
				      ? COLOR_SWITCH : PIXMAP_COPY),
				     num_planes,
				     colors);
    }

    if (dbuf_state == NULL) {
	/* Can't setup double buffering */
	warn("Can't setup colors with visual %s and %d colormap entries",
	     Visual_class_name(visual->class), visual->map_entries);
	return -1;
    }

    switch (dbuf_state->type) {
    case COLOR_SWITCH:
	printf("Using color switching\n");
	break;

    case PIXMAP_COPY:
	printf("Using pixmap copying\n");
	break;

    case MULTIBUFFER:
#ifdef	DBE
	printf("Using double-buffering\n");
	break;
#else
#ifdef	MBX
	printf("Using multi-buffering\n");
	break;
#endif
#endif

    default:
	fatal("Unknown dbuf state %d.", dbuf_state->type);
    }

    for (i = maxColors; i < MAX_COLORS; i++)
	colors[i] = colors[i % maxColors];

    Colors_init_radar_hack();

    Colors_init_bitmaps();

    return 0;
}


/*
 * A little hack that enables us to draw on both sets of double buffering
 * planes at once.
 */
static void Colors_init_radar_hack(void)
{
    int				i, p;

    for (p = 0; p < 2; p++) {
	int num = 0;

	dpl_1[p] = dpl_2[p] = 0;

	for (i = 0; i < 32; i++) {
	    if (!((1 << i) & dbuf_state->drawing_plane_masks[p])) {
	        num++;
		if (num == 1 || num == 3)
		    dpl_1[p] |= 1<<i;   /* planes with moving radar objects */
		else
		    dpl_2[p] |= 1<<i;   /* constant map part of radar */
	    }
	}
    }
}


/*
 * Setup color structures for use with drawing bitmaps.
 *
 * on error return -1,
 * on success return 0.
 */
static int Colors_init_bitmap_colors(void)
{
    int r = -1;

    switch (visual->class) {
    case PseudoColor:
	r = Colors_init_color_cube();
	break;

    case DirectColor:
	/*
	 * I don't really understand the difference between
	 * DirectColor and TrueColor.  Let's test if we can
	 * consider DirectColor to be similar to TrueColor.
	 */
	/*FALLTHROUGH*/

    case StaticColor:
    case TrueColor:
	r = Colors_init_true_color();
	break;

    default:
	warn("fullColor not implemented for visual \"%s\"",
	     Visual_class_name(visual->class));
	fullColor = false;
	texturedObjects = false;
	break;
    }

    return r;
}


/*
 * Converts the RGB colors used by polygon and edge styles
 * to device colors.
 */
void Colors_init_style_colors(void)
{
    int i;
    for (i = 0; i < num_polygon_styles; i++)
	polygon_styles[i].color = (fullColor && RGB) ?
	    RGB2COLOR(polygon_styles[i].rgb) : (unsigned long)wallColor;
    for (i = 0; i < num_edge_styles; i++)
	edge_styles[i].color = (fullColor && RGB) ?
	    RGB2COLOR(edge_styles[i].rgb) : (unsigned long)wallColor;
}


/*
 * See if we can use bitmaps.
 * If we can then setup the colors
 * and allocate the bitmaps.
 *
 * on error return -1,
 * on success return 0.
 */
int Colors_init_bitmaps(void)
{
    /* kps hack */
    if (dbuf_state == NULL)
	return 0;

    if (dbuf_state->type == COLOR_SWITCH) {
	if (fullColor) {
	    warn("Can't do texturedObjects if colorSwitch.");
	    fullColor = false;
	    texturedObjects = false;
	}
    }
    if (fullColor) {
	if (Colors_init_bitmap_colors() == -1) {
	    fullColor = false;
	    texturedObjects = false;
	}
    }

    Colors_init_style_colors();

    return (fullColor) ? 0 : -1;
}


/*
 * Calculate a pixel from a RGB triplet for a PseudoColor visual.
 */
static unsigned long RGB_PC(int r, int g, int b)
{
    int			i;

    r = (r * color_cube->reds) >> 8;
    g = (g * color_cube->greens) >> 8;
    b = (b * color_cube->blues) >> 8;
    i = (((r * color_cube->greens) + g) * color_cube->blues) + b;

    return color_cube->pixels[i];
}


/*
 * Calculate a pixel from a RGB triplet for a TrueColor visual.
 */
static unsigned long RGB_TC(int r, int g, int b)
{
    unsigned long	pixel = 0;

    pixel |= true_color->red_bits[r];
    pixel |= true_color->green_bits[g];
    pixel |= true_color->blue_bits[b];

    return pixel;
}


/*
 * Fill a color cube.
 *
 * Simple implementation for now.
 * Make it more ambitious wrt. read-only cells later.
 *
 * Two ways to allocate colors for a RGB cube.
 * One is to use the outer edges and sides for colors.
 * Another is to divide the cube in r*g*b sub-cubes and
 * choose the color in the centre of each sub-cube.
 * The latter option looks better because it will most
 * likely result in better color matches with on average
 * less color distance.
 */
static void Fill_color_cube(int reds, int greens, int blues,
			    XColor colorarray[256])
{
    int			i, r, g, b;

    i = 0;
    for (r = 0; r < reds; r++) {
	for (g = 0; g < greens; g++) {
	    for (b = 0; b < blues; b++, i++) {
		colorarray[i].pixel = color_cube->pixels[i];
		colorarray[i].flags = DoRed | DoGreen | DoBlue;
		colorarray[i].red   = (((r * 256) + 128) / reds) * 0x101;
		colorarray[i].green = (((g * 256) + 128) / greens) * 0x101;
		colorarray[i].blue  = (((b * 256) + 128) / blues) * 0x101;
	    }
	}
    }

    color_cube->reds = reds;
    color_cube->greens = greens;
    color_cube->blues = blues;
}


/*
 * Allocate a color cube.
 *
 * Simple implementation for now.
 * Make it more ambitious wrt. read-only cells later.
 */
static int Colors_init_color_cube(void)
{
    int			i, n, r, g, b;
    XColor		colorarray[256];

    if (color_cube != NULL) {
	error("Already a cube!\n");
	exit(1);
    }

    color_cube = (struct Color_cube *) calloc(1, sizeof(struct Color_cube));
    if (!color_cube) {
	error("Could not allocate memory for a color cube");
	return -1;
    }
    for (i = 0; i < NELEM(rgb_cube_sizes); i++) {

	r = rgb_cube_sizes[i].r;
	g = rgb_cube_sizes[i].g;
	b = rgb_cube_sizes[i].b;
	n = r * g * b;

	if (XAllocColorCells(dpy,
			     (colormap != 0)
				 ? colormap
				 : DefaultColormap(dpy,
						   DefaultScreen(dpy)),
			     False, NULL, 0,
			     &color_cube->pixels[0],
			     (unsigned)n) == False) {
	    /*printf("Could not alloc %d colors for RGB cube\n", n);*/
	    continue;
	}

	printf("Got %d colors for a %d*%d*%d RGB cube\n",
		n, r, g, b);

	color_cube->mustfree = 1;

	Fill_color_cube(r, g, b, &colorarray[0]);

	XStoreColors(dpy,
		     (colormap != 0)
			 ? colormap
			 : DefaultColormap(dpy,
					   DefaultScreen(dpy)),
		     colorarray,
		     n);

	RGB = RGB_PC;

	return 0;
    }

    warn("Could not alloc colors for RGB cube.");

    return -1;
}


/*
 * Free our color cube.
 */
static void Colors_free_color_cube(void)
{
    if (color_cube) {
	if (color_cube->mustfree) {
	    XFreeColors(
		dpy,
		(colormap != 0)
		? colormap
		: DefaultColormap(dpy,
				  DefaultScreen(dpy)),
		&color_cube->pixels[0],
		color_cube->reds * color_cube->greens * color_cube->blues,
		0);
	    color_cube->mustfree = 0;
	}
	free(color_cube);
	color_cube = NULL;
	RGB = NULL;
    }
}


/*
 * Allocate and initialize a true color structure.
 */
static int Colors_init_true_color(void)
{
    int			i, j, r, g, b;

    if ((visual->red_mask == 0) ||
	(visual->green_mask == 0) ||
	(visual->blue_mask == 0) ||
	((visual->red_mask &
	  visual->green_mask &
	  visual->blue_mask) != 0)) {

	printf("Your visual \"%s\" has weird characteristics:\n",
		Visual_class_name(visual->class));
	printf("\tred mask 0x%06lx, green mask 0x%06lx, blue mask 0x%06lx,\n",
		visual->red_mask, visual->green_mask, visual->blue_mask);
	printf("\toverlap mask 0x%06lx\n",
		visual->red_mask & visual->green_mask & visual->blue_mask);
	return -1;
    }

    if (true_color) {
	error("Already a True_color!");
	exit(1);
    }

    true_color = (struct True_color *) calloc(1, sizeof(struct True_color));
    if (!true_color) {
	error("Could not allocate memory for a true color structure");
	return -1;
    }

    r = 7;
    g = 7;
    b = 7;
    for (i = 31; i >= 0; --i) {
	if ((visual->red_mask & (1UL << i)) != 0) {
	    if (r >= 0) {
		for (j = 0; j < 256; j++) {
		    if (j & (1 << r))
			true_color->red_bits[j] |= (1UL << i);
		}
		r--;
	    }
	}
	if ((visual->green_mask & (1UL << i)) != 0) {
	    if (g >= 0) {
		for (j = 0; j < 256; j++) {
		    if (j & (1 << g))
			true_color->green_bits[j] |= (1UL << i);
		}
		g--;
	    }
	}
	if ((visual->blue_mask & (1UL << i)) != 0) {
	    if (b >= 0) {
		for (j = 0; j < 256; j++) {
		    if (j & (1 << b))
			true_color->blue_bits[j] |= (1UL << i);
		}
		b--;
	    }
	}
    }

    RGB = RGB_TC;

    return 0;
}


/*
 * Free a true color structure.
 */
static void Colors_free_true_color(void)
{
    if (true_color) {
	free(true_color);
	true_color = NULL;
	RGB = NULL;
    }
}


/*
 * Deallocate everything related to colors.
 */
void Colors_free_bitmaps(void)
{
    Colors_free_color_cube();
    Colors_free_true_color();

    fullColor = false;
    texturedObjects = false;
}


/*
 * Deallocate everything related to colors.
 */
void Colors_cleanup(void)
{
    Colors_free_bitmaps();

    if (dbuf_state) {
	end_dbuff(dbuf_state);
	dbuf_state = NULL;
    }
    if (colormap) {
	XFreeColormap(dpy, colormap);
	colormap = 0;
    }
}


#ifdef DEVELOPMENT
void Colors_debug(void)
{
    int			i, n, r, g, b;
    XColor		cols[256];
    FILE		*fp = fopen("rgb", "w");

    if (!color_cube) {
	static struct Color_cube cc;
	color_cube = &cc;
	for (i = 0; i < 256; i++)
	    cc.pixels[i] = i;
    }

    for (i = 0; i < NELEM(rgb_cube_sizes); i++) {

	r = rgb_cube_sizes[i].r;
	g = rgb_cube_sizes[i].g;
	b = rgb_cube_sizes[i].b;
	n = r * g * b;

	Fill_color_cube(r, g, b, cols);

	fprintf(fp, "\n\n  RGB  %d %d %d\n\n", r, g, b);
	i = 0;
	for (r = 0; r < color_cube->reds; r++) {
	    for (g = 0; g < color_cube->greens; g++) {
		for (b = 0; b < color_cube->blues; b++, i++)
		    fprintf(fp, "color %4d    %04X  %04X  %04X\n",
			    i, cols[i].red, cols[i].green, cols[i].blue);
	    }
	}
	fprintf(fp,
		"\nblack %3lu\nwhite %3lu\nred   %3lu"
		"\ngreen %3lu\nblue  %3lu\n",
		RGB_PC(0, 0, 0),
		RGB_PC(255, 255, 255),
		RGB_PC(255, 0, 0),
		RGB_PC(0, 255, 0),
		RGB_PC(0, 0, 255));
    }

    fclose(fp);

    exit(1);
}
#endif	/* DEVELOPMENT */

/*
 * Convert a string of color numbers into an array
 * of "colors[]" indices stored by "spark_color[]".
 * Initialize "num_spark_colors".
 */
void Init_spark_colors(void)
{
    char		buf[MSG_LEN];
    char		*src, *dst;
    unsigned		col;
    int			i;

    num_spark_colors = 0;
    /*
     * The sparkColors specification may contain
     * any possible separator.  Only look at numbers.
     */

     /* hack but protocol will allow max 9 (MM) */ 
    for (src = sparkColors; *src && (num_spark_colors < 9); src++) {
	if (isascii(*src) && isdigit(*src)) {
	    dst = &buf[0];
	    do {
		*dst++ = *src++;
	    } while (*src &&
		     isascii(*src) &&
		     isdigit(*src) &&
		     ((size_t)(dst - buf) < (sizeof(buf) - 1)));
	    *dst = '\0';
	    src--;
	    if (sscanf(buf, "%u", &col) == 1) {
		if (col < (unsigned)maxColors)
		    spark_color[num_spark_colors++] = col;
	    }
	}
    }
    if (num_spark_colors == 0) {
	if (maxColors <= 8) {
	    /* 3 colors ranging from 5 up to 7 */
	    for (i = 5; i < maxColors; i++)
		spark_color[num_spark_colors++] = i;
	}
	else {
	    /* 7 colors ranging from 5 till 11 */
	    for (i = 5; i < 12; i++)
		spark_color[num_spark_colors++] = i;
	}
	/* default spark colors always include RED. */
	spark_color[num_spark_colors++] = RED;
    }
    for (i = num_spark_colors; i < MAX_COLORS; i++)
	spark_color[i] = spark_color[num_spark_colors - 1];
}


static bool Set_sparkColors (xp_option_t *opt, const char *val)
{
    UNUSED_PARAM(opt);
    strlcpy(sparkColors, val, sizeof sparkColors);
    Init_spark_colors();
    /* might fail to set what we wanted, but return ok nonetheless */
    return true;
}

static bool Set_maxColors (xp_option_t *opt, int val)
{
    UNUSED_PARAM(opt);
    if (val == 4 || val == 8) {
	warn("Values 4 or 8 for maxColors are not actively "
	     "supported. Use at own risk.");
	maxColors = val;
    } else
	maxColors = MAX_COLORS;
    return true;
}

static bool Set_color(xp_option_t *opt, const char *val)
{
    char *buf = (char *)Option_get_private_data(opt);

    /*warn("Set_color: name=%s, val=\"%s\", buf=%p", opt->name, val, buf);*/
    assert(val != NULL);
    strlcpy(buf, val, MAX_COLOR_LEN);

    return true;
}


static xp_option_t color_options[] = {

    XP_INT_OPTION(
	"maxColors",
	MAX_COLORS,
	4,
	MAX_COLORS,
	&maxColors,
	Set_maxColors,
	XP_OPTFLAG_DEFAULT,
	"The number of colors to use.\n"
	"Use value 16. Other values are not actively supported.\n"),

    /* 16 user definable color values */
    XP_STRING_OPTION(
	"color0",
	XP_COLOR0,
	color_names[0],
	MAX_COLOR_LEN,
	Set_color, color_names[0], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the first color.\n"),

    XP_STRING_OPTION(
	"color1",
	XP_COLOR1,
	color_names[1],
	MAX_COLOR_LEN,
	Set_color, color_names[1], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the second color.\n"),

    XP_STRING_OPTION(
	"color2",
	XP_COLOR2,
	color_names[2],
	MAX_COLOR_LEN,
	Set_color, color_names[2], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the third color.\n"),

    XP_STRING_OPTION(
	"color3",
	XP_COLOR3,
	color_names[3],
	MAX_COLOR_LEN,
	Set_color, color_names[3], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the fourth color.\n"),

    XP_STRING_OPTION(
	"color4",
	XP_COLOR4,
	color_names[4],
	MAX_COLOR_LEN,
	Set_color, color_names[4], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the fifth color.\n"),

    XP_STRING_OPTION(
	"color5",
	XP_COLOR5,
	color_names[5],
	MAX_COLOR_LEN,
	Set_color, color_names[5], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the sixth color.\n"),

    XP_STRING_OPTION(
	"color6",
	XP_COLOR6,
	color_names[6],
	MAX_COLOR_LEN,
	Set_color, color_names[6], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the seventh color.\n"),

    XP_STRING_OPTION(
	"color7",
	XP_COLOR7,
	color_names[7],
	MAX_COLOR_LEN,
	Set_color, color_names[7], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the eighth color.\n"),

    XP_STRING_OPTION(
	"color8",
	XP_COLOR8,
	color_names[8],
	MAX_COLOR_LEN,
	Set_color, color_names[8], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the ninth color.\n"),

    XP_STRING_OPTION(
	"color9",
	XP_COLOR9,
	color_names[9],
	MAX_COLOR_LEN,
	Set_color, color_names[9], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the tenth color.\n"),

    XP_STRING_OPTION(
	"color10",
	XP_COLOR10,
	color_names[10],
	MAX_COLOR_LEN,
	Set_color, color_names[10], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the eleventh color.\n"),

    XP_STRING_OPTION(
	"color11",
	XP_COLOR11,
	color_names[11],
	MAX_COLOR_LEN,
	Set_color, color_names[11], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the twelfth color.\n"),

    XP_STRING_OPTION(
	"color12",
	XP_COLOR12,
	color_names[12],
	MAX_COLOR_LEN,
	Set_color, color_names[12], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the thirteenth color.\n"),

    XP_STRING_OPTION(
	"color13",
	XP_COLOR13,
	color_names[13],
	MAX_COLOR_LEN,
	Set_color, color_names[13], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the fourteenth color.\n"),

    XP_STRING_OPTION(
	"color14",
	XP_COLOR14,
	color_names[14],
	MAX_COLOR_LEN,
	Set_color, color_names[14], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the fifteenth color.\n"),

    XP_STRING_OPTION(
	"color15",
	XP_COLOR15,
	color_names[15],
	MAX_COLOR_LEN,
	Set_color, color_names[15], NULL,
	XP_OPTFLAG_DEFAULT,
	"The color value for the sixteenth color.\n"),

    XP_STRING_OPTION(
	"sparkColors",
	"5,6,7,3",
	sparkColors,
	sizeof sparkColors,
	Set_sparkColors, NULL, NULL,
	XP_OPTFLAG_DEFAULT,
	"Which color numbers to use for spark and debris particles.\n"),

    COLOR_INDEX_OPTION(
	"wallColor",
	2,
	&wallColor,
	"Which color number to use for drawing walls.\n"),

    COLOR_INDEX_OPTION(
	"decorColor",
	6,
	&decorColor,
	"Which color number to use for drawing decorations.\n"),

    COLOR_INDEX_OPTION(
	"windowColor",
	8,
	&windowColor,
	"Which color number to use for drawing windows.\n"),

    COLOR_INDEX_OPTION(
	"buttonColor",
	2,
	&buttonColor,
	"Which color number to use for drawing buttons.\n"),

    COLOR_INDEX_OPTION(
	"borderColor",
	1,
	&borderColor,
	"Which color number to use for drawing borders.\n"),
};


void Store_color_options(void)
{
    STORE_OPTIONS(color_options);
}


