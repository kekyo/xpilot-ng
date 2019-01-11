/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * Copyright (C) 2003-2004 Juha Lindström <juhal@users.sourceforge.net>
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

#include "xpclient_sdl.h"

#include "images.h"
#include "sdlpaint.h"

static image_t *images = NULL;
static int num_images = 0, max_images = 0;
static int first_texture = 0;

static int pow2_ceil(int t) 
{
    int r = 1;
    while (r < t) r <<= 1;
    return r;   
}

static int Image_init(image_t *img)
{
    int i, x, y;
    xp_picture_t pic;
    RGB_COLOR c;
    
    if (img->state != IMG_STATE_UNINITIALIZED) 
	return -1;
	
    if (Picture_init(&pic,
		     img->filename,
		     img->num_frames * (img->rotate ? 1 : -1)) == -1) {
	img->state = IMG_STATE_ERROR;
	return -1;
    }
    img->name = 0;
    img->width = pic.width * img->num_frames;
    img->height = pic.height;
    img->frame_width = img->width / img->num_frames;
    img->data_width = pow2_ceil(img->width);
    img->data_height = pow2_ceil(img->height);
	
    warn("Loaded image %s: w=%d, h=%d, fw=%d, dw=%d, dh=%d",
	 img->filename, img->width, img->height, img->frame_width,
	 img->data_width, img->data_height);
	
    img->data = XCALLOC(unsigned int, img->data_width * img->data_height);
    if (img->data == NULL) {
        error("Failed to allocate memory for: %s size %dx%d",
              img->filename, img->data_width, img->data_height);
	img->state = IMG_STATE_ERROR;
	return -1;
    }

    for (i = 0; i < img->num_frames; i++) {
	for (y = 0; y < img->height; y++) {
	    for (x = 0; x < img->frame_width; x++) {
		/* the pixels needs to be mirrored over x-axis because
		 * of the used OpenGL projection */
		c = Picture_get_pixel(&pic, i, x, img->height - y - 1);
		if (c)
		    c |= 0xff000000; /* alpha */
		img->data[(x + img->frame_width * i) + (y * img->data_width)]
		    = c;
	    }
	}
    }

    glGenTextures(1, &img->name);
    glBindTexture(GL_TEXTURE_2D, img->name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->data_width, img->data_height, 
                 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                    GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                    GL_NEAREST);
	
    img->state = IMG_STATE_READY;
    return 0;
}

static void Image_free(image_t *img)
{
    XFREE(img->filename);
    if (img->state == IMG_STATE_READY) {
	glDeleteTextures(1, &img->name);
	/* this causes a Segmentation Fault for some reason
	free(img->data);
	*/
    }
    img->state = IMG_STATE_UNINITIALIZED;
}

image_t *Image_get(int ind) {

    image_t *img;

    if (ind >= num_images)
	return NULL;
    img = &images[ind];
    if (img == NULL)
	return NULL;
    if (img->state == IMG_STATE_UNINITIALIZED)
	Image_init(img);
    if (img->state != IMG_STATE_READY)
	return NULL;
    return img;
}

image_t *Image_get_texture(int ind) 
{
    return Image_get(first_texture + ind);
}

void Image_use_texture(int ind)
{
    image_t *img = Image_get_texture(ind);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    if (img == NULL) {
	warn("Texture %d is undefined.\n", ind);
	return;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, img->name);
    glColor4ub(255, 255, 255, 255);
}

void Image_no_texture(void)
{
    /*glDisable(GL_BLEND);*/
    glDisable(GL_TEXTURE_2D);
}

void Image_paint(int ind, int x, int y, int frame, int c)
{
    Image_paint_area(ind, x, y, frame, NULL, c);
}

void Image_paint_area(int ind, int x, int y, int frame, irec_t *r, int c)
{
    image_t *img;
    irec_t whole;
    float tx1, ty1, tx2, ty2;

    img = Image_get(ind);
    if (img == NULL)
	return;

    if (r == NULL) {
	whole.x = 0;
	whole.y = 0;
	whole.w = img->frame_width;
	whole.h = img->height;
	r = &whole;
    }

    tx1 = ((float)frame * img->frame_width + r->x) / img->data_width;
    ty1 = ((float)r->y) / img->data_height;
    tx2 = ((float)frame * img->frame_width + r->x + r->w) / img->data_width;
    ty2 = ((float)r->y + r->h) / (img->data_height);
    
    glBindTexture(GL_TEXTURE_2D, img->name);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    set_alphacolor(c);

    glBegin(GL_QUADS);
    glTexCoord2f(tx1, ty1); glVertex2i(x    	, y 	    );
    glTexCoord2f(tx2, ty1); glVertex2i(x + r->w , y 	    );
    glTexCoord2f(tx2, ty2); glVertex2i(x + r->w , y + r->h  );
    glTexCoord2f(tx1, ty2); glVertex2i(x    	, y + r->h  );
    glEnd();    

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void Image_paint_rotated(int ind, int x, int y, int dir, int color)
{
    image_t *img;
    irec_t whole;
    float tx1, ty1, tx2, ty2;

    img = Image_get(ind);
    if (img == NULL)
	return;

    whole.x = 0;
    whole.y = 0;
    whole.w = img->frame_width;
    whole.h = img->height;

    tx1 = 0.0;
    ty1 = 0.0;
    tx2 = img->frame_width / (double)img->data_width;
    ty2 = img->height / (double)img->data_height;

    glPushMatrix();
    glTranslatef((GLfloat)(x), (GLfloat)(y), 0.0);
    glRotatef(360.0 * dir / (double)TABLE_SIZE, 0.0, 0.0, 1.0);
    
    glBindTexture(GL_TEXTURE_2D, img->name);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* Linear Filtering */
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    set_alphacolor(color);

    glBegin(GL_QUADS);
    glTexCoord2f(tx1, ty1); glVertex2f( -whole.w / 2.0 ,    -whole.h / 2.0  );
    glTexCoord2f(tx2, ty1); glVertex2f( whole.w / 2.0  ,    -whole.h / 2.0  );
    glTexCoord2f(tx2, ty2); glVertex2f( whole.w / 2.0  ,    whole.h / 2.0   );
    glTexCoord2f(tx1, ty2); glVertex2f( -whole.w / 2.0 ,    whole.h / 2.0   );
    glEnd();    

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}


int Images_init(void)
{
#define DEF_IMG(name, count) Bitmap_add(name, count, false);

    DEF_IMG("holder1.ppm", 1);
    DEF_IMG("holder2.ppm", 1);
    DEF_IMG("ball_gray.ppm", 1);
    DEF_IMG("ship_friend.ppm", 1); /* 128 fails in some OpenGL drivers */
    DEF_IMG("ship_friend.ppm", 1); /* I guess texture gets too wide (4096) */
    DEF_IMG("ship_enemy.ppm", 1);
    DEF_IMG("bullet.ppm", -16);
    DEF_IMG("bullet_blue.ppm", -16);
    DEF_IMG("base_down.ppm", 1);
    DEF_IMG("base_left.ppm", 1);
    DEF_IMG("base_up.ppm", 1);
    DEF_IMG("base_right.ppm", 1);
    DEF_IMG("fuelcell.ppm", 1);
    DEF_IMG("fuel2.ppm", -16);
    DEF_IMG("allitems.ppm", -30);
    DEF_IMG("cannon_down.ppm", 1);
    DEF_IMG("cannon_left.ppm", 1);
    DEF_IMG("cannon_up.ppm", 1);
    DEF_IMG("cannon_right.ppm", 1);
    DEF_IMG("sparks.ppm", -8);
    DEF_IMG("paused.ppm", -2);
    DEF_IMG("refuel.ppm", -4);
    DEF_IMG("wormhole.ppm", 1);
    DEF_IMG("mine_team.ppm", 1);
    DEF_IMG("mine_other.ppm", 1);
    DEF_IMG("concentrator.ppm", 1);
    DEF_IMG("plus.ppm", 1);
    DEF_IMG("minus.ppm", 1);
    DEF_IMG("checkpoint.ppm", -2);
    DEF_IMG("meter.ppm", -2);
    DEF_IMG("asteroidconcentrator.ppm", 1);
    DEF_IMG("shield.ppm", 1);
    DEF_IMG("acwise_grav.ppm", -6);
    DEF_IMG("cwise_grav.ppm", -6);
    DEF_IMG("missile.ppm", 1);
    DEF_IMG("asteroid.ppm", 1);
    DEF_IMG("target.ppm", 1);
    DEF_IMG("huditems.ppm", -30);

    first_texture = num_images;

#undef DEF_IMG
    return 0;
}


void Images_cleanup(void)
{
    int i;

    if (images == NULL)
	return;

    for (i = 0; i < num_images; i++)
	Image_free(images + i);

    XFREE(images);
}


int Bitmap_add(const char *filename, int count, bool scalable)
{
    image_t img;

    img.filename   = xp_strdup(filename);
    img.num_frames = ABS(count);
    img.rotate     = count > 1;
    img.state      = IMG_STATE_UNINITIALIZED;
    STORE(image_t, images, num_images, max_images, img);

    return num_images - 1;
}


