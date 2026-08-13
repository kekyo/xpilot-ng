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

#include "image_geometry.h"
#include "images.h"
#include "images_test_support.h"
#include "sdlinit.h"
#include "sdlpaint.h"
#include "sdlrenderer.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct ImageCandidate {
    image_t *image;
    RendererTexture *texture;
    GLuint legacy_name;
    int width;
    int height;
    int data_width;
    int data_height;
    int frame_width;
} ImageCandidate;

static image_t *images = NULL;
static int num_images = 0;
static int max_images = 0;
static int first_texture = 0;
static Renderer *images_renderer = NULL;

#ifdef XPILOT_IMAGE_TEST_HOOKS
static ImagesTestHooks images_test_hooks;

void Images_test_set_hooks(const ImagesTestHooks *hooks)
{
    if (hooks == NULL) {
        memset(&images_test_hooks, 0, sizeof(images_test_hooks));
        return;
    }
    images_test_hooks = *hooks;
}

void Images_test_reset_hooks(void)
{
    memset(&images_test_hooks, 0, sizeof(images_test_hooks));
}

static int Image_picture_init(xp_picture_t *picture,
                              const char *filename, int count)
{
    if (images_test_hooks.picture_init == NULL)
        return -1;
    return images_test_hooks.picture_init(
        picture, filename, count, images_test_hooks.context);
}

static RGB_COLOR Image_picture_get_pixel(const xp_picture_t *picture,
                                         int frame, int x, int y)
{
    if (images_test_hooks.picture_get_pixel == NULL)
        return 0;
    return images_test_hooks.picture_get_pixel(
        picture, frame, x, y, images_test_hooks.context);
}

static void Image_picture_cleanup(xp_picture_t *picture)
{
    if (images_test_hooks.picture_cleanup != NULL) {
        images_test_hooks.picture_cleanup(
            picture, images_test_hooks.context);
    }
}
#else
static int Image_picture_init(xp_picture_t *picture,
                              const char *filename, int count)
{
    return Picture_init(picture, filename, count);
}

static RGB_COLOR Image_picture_get_pixel(const xp_picture_t *picture,
                                         int frame, int x, int y)
{
    return Picture_get_pixel(picture, frame, x, y);
}

static void Image_picture_cleanup(xp_picture_t *picture)
{
    Picture_cleanup(picture);
}
#endif

static int Image_gl_failed(void)
{
    GLenum error_code;
    int failed = 0;

    while ((error_code = glGetError()) != GL_NO_ERROR) {
        failed = 1;
#ifdef GL_CONTEXT_LOST
        if (error_code == GL_CONTEXT_LOST)
            break;
#endif
    }
    return failed;
}

static int Image_pow2_ceil(int value)
{
    int result = 1;

    if (value <= 0)
        return 0;
    while (result < value) {
        if (result > INT_MAX / 2)
            return 0;
        result <<= 1;
    }
    return result;
}

static int Image_picture_valid(const xp_picture_t *picture,
                               const image_t *image)
{
    int picture_frames;

    if (picture == NULL || image == NULL || image->num_frames <= 0
        || picture->width == 0 || picture->height == 0
        || picture->width > INT_MAX || picture->height > INT_MAX
        || picture->count == 0 || picture->count == INT_MIN) {
        return 0;
    }
    picture_frames = picture->count < 0 ? -picture->count : picture->count;
    return picture_frames == image->num_frames
        && picture->width <= (unsigned int)(INT_MAX / image->num_frames);
}

static int Image_pixel_buffer_size(int width, int height, size_t *size)
{
    size_t row_size;

    if (width <= 0 || height <= 0 || size == NULL
        || (size_t)width > SIZE_MAX / 4) {
        return -1;
    }
    row_size = (size_t)width * 4;
    if ((size_t)height > SIZE_MAX / row_size)
        return -1;
    *size = row_size * (size_t)height;
    return 0;
}

static void Image_store_rgba(uint8_t *destination, RGB_COLOR color)
{
    if (color == 0) {
        memset(destination, 0, 4);
        return;
    }
    destination[0] = (uint8_t)RED_VALUE(color);
    destination[1] = (uint8_t)GREEN_VALUE(color);
    destination[2] = (uint8_t)BLUE_VALUE(color);
    destination[3] = 255;
}

static void Image_flip_legacy_rows(uint8_t *pixels, int data_width,
                                   int content_height)
{
    size_t row_size = (size_t)data_width * 4;
    int top;

    /* Semantic textures retain top-left source rows. Fixed-function OpenGL
     * still expects the old bottom-up content inside the unflipped padding. */
    for (top = 0; top < content_height / 2; top++) {
        uint8_t *top_row = pixels + (size_t)top * row_size;
        uint8_t *bottom_row = pixels
            + (size_t)(content_height - top - 1) * row_size;
        size_t byte;

        for (byte = 0; byte < row_size; byte++) {
            uint8_t temporary = top_row[byte];

            top_row[byte] = bottom_row[byte];
            bottom_row[byte] = temporary;
        }
    }
}

static void Image_candidate_release(Renderer *renderer,
                                    ImageCandidate *candidate)
{
    int released_legacy = candidate->legacy_name != 0;

    if (candidate->texture != NULL) {
        if (Renderer_texture_destroy(renderer, candidate->texture)
            != RENDERER_STATUS_OK) {
            warn("Could not release a staged renderer texture");
        }
        candidate->texture = NULL;
    }
    if (candidate->legacy_name != 0) {
        glDeleteTextures(1, &candidate->legacy_name);
        candidate->legacy_name = 0;
    }
    if (released_legacy && Image_gl_failed())
        warn("OpenGL failed while releasing a staged image texture");
}

static void Image_candidates_release(Renderer *renderer,
                                     ImageCandidate *candidates,
                                     size_t count)
{
    while (count > 0) {
        count--;
        Image_candidate_release(renderer, &candidates[count]);
    }
}

static int Image_stage(Renderer *renderer, image_t *image,
                       int registration_index, ImageCandidate *candidate)
{
    xp_picture_t picture;
    RendererTextureDesc texture_desc;
    uint8_t *pixels = NULL;
    size_t pixel_size;
    int frame;
    int x;
    int y;
    int signed_count;
    int result = -1;

    memset(&picture, 0, sizeof(picture));
    memset(candidate, 0, sizeof(*candidate));
    candidate->image = image;
    signed_count = image->rotate ? image->num_frames : -image->num_frames;
    if (Image_picture_init(&picture, image->filename, signed_count) != 0)
        return -1;

    if (!Image_picture_valid(&picture, image)) {
        error("Invalid image dimensions or frame count: %s", image->filename);
        goto cleanup;
    }
    candidate->frame_width = (int)picture.width;
    candidate->width = candidate->frame_width * image->num_frames;
    candidate->height = (int)picture.height;
    candidate->data_width = Image_pow2_ceil(candidate->width);
    candidate->data_height = Image_pow2_ceil(candidate->height);
    if (candidate->data_width == 0 || candidate->data_height == 0
        || Image_pixel_buffer_size(candidate->data_width,
                                   candidate->data_height,
                                   &pixel_size) != 0) {
        error("Image is too large: %s", image->filename);
        goto cleanup;
    }

    pixels = calloc(1, pixel_size);
    if (pixels == NULL) {
        error("Failed to allocate image staging memory: %s", image->filename);
        goto cleanup;
    }
    for (frame = 0; frame < image->num_frames; frame++) {
        for (y = 0; y < candidate->height; y++) {
            for (x = 0; x < candidate->frame_width; x++) {
                size_t destination =
                    ((size_t)y * (size_t)candidate->data_width
                     + (size_t)frame * (size_t)candidate->frame_width
                     + (size_t)x) * 4;

                Image_store_rgba(
                    pixels + destination,
                    Image_picture_get_pixel(&picture, frame, x, y));
            }
        }
    }

    texture_desc.width = candidate->width;
    texture_desc.height = candidate->height;
    texture_desc.filter = image->filter;
    texture_desc.wrap = registration_index >= first_texture
        ? RENDERER_TEXTURE_WRAP_REPEAT : RENDERER_TEXTURE_WRAP_CLAMP;
    if (Renderer_texture_create_with_desc(
            renderer, &texture_desc, pixels,
            (size_t)candidate->data_width * 4,
            &candidate->texture) != RENDERER_STATUS_OK) {
        error("Failed to create renderer texture: %s", image->filename);
        goto cleanup;
    }

    Image_flip_legacy_rows(pixels, candidate->data_width, candidate->height);
    if (Image_gl_failed()) {
        error("OpenGL reported an error before compatibility upload: %s",
              image->filename);
        goto cleanup;
    }
    glGenTextures(1, &candidate->legacy_name);
    if (candidate->legacy_name == 0) {
        error("Failed to create compatibility texture: %s", image->filename);
        goto cleanup;
    }
    glBindTexture(GL_TEXTURE_2D, candidate->legacy_name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 candidate->data_width, candidate->data_height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    image->filter == RENDERER_TEXTURE_FILTER_LINEAR
                        ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    image->filter == RENDERER_TEXTURE_FILTER_LINEAR
                        ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    texture_desc.wrap == RENDERER_TEXTURE_WRAP_REPEAT
                        ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    texture_desc.wrap == RENDERER_TEXTURE_WRAP_REPEAT
                        ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    if (Image_gl_failed()) {
        error("Failed to upload compatibility texture: %s", image->filename);
        goto cleanup;
    }

    warn("Loaded image %s: w=%d, h=%d, fw=%d, dw=%d, dh=%d",
         image->filename, candidate->width, candidate->height,
         candidate->frame_width, candidate->data_width,
         candidate->data_height);
    result = 0;

cleanup:
    free(pixels);
    Image_picture_cleanup(&picture);
    if (result != 0)
        Image_candidate_release(renderer, candidate);
    return result;
}

static void Image_publish_candidate(Renderer *renderer,
                                    ImageCandidate *candidate)
{
    image_t *image = candidate->image;

    image->legacy_name = candidate->legacy_name;
    image->texture = candidate->texture;
    image->renderer = renderer;
    image->width = candidate->width;
    image->height = candidate->height;
    image->data_width = candidate->data_width;
    image->data_height = candidate->data_height;
    image->frame_width = candidate->frame_width;
    image->state = IMG_STATE_READY;
    candidate->legacy_name = 0;
    candidate->texture = NULL;
}

int Images_prepare(Renderer *renderer)
{
    ImageCandidate *candidates;
    size_t candidate_count = 0;
    int index;

    if (renderer == NULL)
        return -1;
    if (images_renderer != NULL && images_renderer != renderer) {
        error("Images are already owned by another renderer");
        return -1;
    }
    if (num_images == 0)
        return 0;
    if ((size_t)num_images > SIZE_MAX / sizeof(*candidates))
        return -1;

    candidates = calloc((size_t)num_images, sizeof(*candidates));
    if (candidates == NULL)
        return -1;
    for (index = 0; index < num_images; index++) {
        if (images[index].state == IMG_STATE_READY)
            continue;
        if (Image_stage(renderer, &images[index], index,
                        &candidates[candidate_count]) != 0) {
            Image_candidates_release(renderer, candidates, candidate_count);
            free(candidates);
            return -1;
        }
        candidate_count++;
    }
    for (index = 0; index < (int)candidate_count; index++)
        Image_publish_candidate(renderer, &candidates[index]);
    if (candidate_count > 0)
        images_renderer = renderer;
    free(candidates);
    return 0;
}

static void Image_free(image_t *image)
{
    int released_gpu_resource = 0;

    if (image->texture != NULL && image->renderer != NULL) {
        released_gpu_resource = 1;
        if (Renderer_texture_destroy(image->renderer, image->texture)
            != RENDERER_STATUS_OK) {
            warn("Could not release renderer texture for %s",
                 image->filename != NULL ? image->filename : "(unknown)");
        }
    }
    if (image->legacy_name != 0) {
        released_gpu_resource = 1;
        glDeleteTextures(1, &image->legacy_name);
    }
    if (released_gpu_resource && Image_gl_failed()) {
        warn("OpenGL failed while releasing image texture for %s",
             image->filename != NULL ? image->filename : "(unknown)");
    }
    free(image->filename);
    memset(image, 0, sizeof(*image));
    image->state = IMG_STATE_UNINITIALIZED;
}

image_t *Image_get(int ind)
{
    if (ind < 0 || ind >= num_images || images[ind].state != IMG_STATE_READY)
        return NULL;
    return &images[ind];
}

image_t *Image_get_texture(int ind)
{
    if (ind < 0 || first_texture > INT_MAX - ind)
        return NULL;
    return Image_get(first_texture + ind);
}

void Image_use_texture(int ind)
{
    image_t *image = Image_get_texture(ind);

    if (image == NULL) {
        warn("Texture %d is undefined.\n", ind);
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, image->legacy_name);
    glColor4ub(255, 255, 255, 255);
}

void Image_no_texture(void)
{
    /*glDisable(GL_BLEND);*/
    glDisable(GL_TEXTURE_2D);
}

static void Image_draw(image_t *image, int frame, RendererRect area,
                       RendererPoint2D position, float radians,
                       ImageGeometrySpace space, int packed_color)
{
    SdlRenderer *sdl_renderer;
    Renderer *renderer;
    ImageGeometryAtlas atlas;
    RendererVertex2D vertices[6];
    RendererStatus status;

    if (image == NULL)
        return;
    sdl_renderer = Get_sdl_renderer();
    if (sdl_renderer == NULL)
        return;
    status = Sdl_renderer_track_frame_result(
        sdl_renderer, RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
        return;
    renderer = Sdl_renderer_frontend(sdl_renderer);
    if (renderer == NULL) {
        status = Sdl_renderer_track_frame_result(
            sdl_renderer, RENDERER_STATUS_INVALID_STATE);
        goto failure;
    }
    if (renderer != image->renderer) {
        status = Sdl_renderer_track_frame_result(
            sdl_renderer, RENDERER_STATUS_RESOURCE_MISMATCH);
        goto failure;
    }
    atlas.texture_width = image->width;
    atlas.texture_height = image->height;
    atlas.frame_width = image->frame_width;
    atlas.frame_height = image->height;
    atlas.frame_count = (size_t)image->num_frames;
    status = Image_geometry_sprite(
        &atlas, (size_t)frame, area, position, radians, space,
        Renderer_color_from_rgba32((uint32_t)packed_color), vertices);
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status != RENDERER_STATUS_OK)
        goto failure;
    status = Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA);
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status != RENDERER_STATUS_OK)
        goto failure;
    status = Renderer_draw_triangles(
        renderer, image->texture, vertices, NELEM(vertices));
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status != RENDERER_STATUS_OK)
        goto failure;
    status = Sdl_renderer_flush_preserving_legacy(sdl_renderer);
    status = Sdl_renderer_track_frame_result(sdl_renderer, status);
    if (status == RENDERER_STATUS_OK)
        return;

failure:
    warn("Could not draw image %s (%d)", image->filename, (int)status);
}

void Image_paint(int ind, int x, int y, int frame, int c)
{
    Image_paint_area(ind, x, y, frame, NULL, c);
}

void Image_paint_area(int ind, int x, int y, int frame, irec_t *r, int c)
{
    image_t *image = Image_get(ind);
    RendererRect area;
    RendererPoint2D position = {(float)x, (float)y};

    if (image == NULL)
        return;
    if (r == NULL) {
        area = (RendererRect){0, 0, image->frame_width, image->height};
    } else {
        area = (RendererRect){r->x, r->y, r->w, r->h};
    }
    Image_draw(image, frame, area, position, 0.0f,
               IMAGE_GEOMETRY_SPACE_WORLD, c);
}

void Image_paint_hud(int ind, int x, int y, int frame, int c)
{
    image_t *image = Image_get(ind);
    RendererRect area;
    RendererPoint2D position = {(float)x, (float)y};

    if (image == NULL)
        return;
    area = (RendererRect){0, 0, image->frame_width, image->height};
    Image_draw(image, frame, area, position, 0.0f,
               IMAGE_GEOMETRY_SPACE_HUD, c);
}

void Image_paint_rotated(int ind, int center_x, int center_y,
                         int dir, int color)
{
    const float full_turn = 6.28318530717958647692f;
    image_t *image = Image_get(ind);
    RendererRect area;
    RendererPoint2D position;

    if (image == NULL)
        return;
    area = (RendererRect){0, 0, image->frame_width, image->height};
    position.x = (float)center_x - image->frame_width * 0.5f;
    position.y = (float)center_y - image->height * 0.5f;
    Image_draw(image, 0, area, position,
               full_turn * (float)dir / (float)TABLE_SIZE,
               IMAGE_GEOMETRY_SPACE_WORLD, color);
}

int Images_init(void)
{
#define DEF_IMG(name, count)                                                \
    do {                                                                    \
        if (Bitmap_add(name, count, false) < 0)                             \
            return -1;                                                      \
    } while (0)
#define DEF_ROTATED_IMG(name)                                               \
    do {                                                                    \
        int image_index = Bitmap_add(name, 1, false);                       \
                                                                            \
        if (image_index < 0)                                                \
            return -1;                                                      \
        images[image_index].filter = RENDERER_TEXTURE_FILTER_LINEAR;        \
    } while (0)

    DEF_IMG("holder1.ppm", 1);
    DEF_IMG("holder2.ppm", 1);
    DEF_IMG("ball_gray.ppm", 1);
    /* Rotated sprites use a single source frame and immutable linear
     * sampling rather than mutating texture state while painting. */
    DEF_ROTATED_IMG("ship_friend.ppm");
    DEF_ROTATED_IMG("ship_friend.ppm");
    DEF_ROTATED_IMG("ship_enemy.ppm");
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
    DEF_ROTATED_IMG("wormhole.ppm");
    DEF_IMG("mine_team.ppm", 1);
    DEF_IMG("mine_other.ppm", 1);
    DEF_ROTATED_IMG("concentrator.ppm");
    DEF_IMG("plus.ppm", 1);
    DEF_IMG("minus.ppm", 1);
    DEF_IMG("checkpoint.ppm", -2);
    DEF_IMG("meter.ppm", -2);
    DEF_ROTATED_IMG("asteroidconcentrator.ppm");
    DEF_IMG("shield.ppm", 1);
    DEF_IMG("acwise_grav.ppm", -6);
    DEF_IMG("cwise_grav.ppm", -6);
    DEF_ROTATED_IMG("missile.ppm");
    DEF_IMG("asteroid.ppm", 1);
    DEF_IMG("target.ppm", 1);
    DEF_IMG("huditems.ppm", -30);

    first_texture = num_images;

#undef DEF_IMG
#undef DEF_ROTATED_IMG
    return 0;
}

void Images_cleanup(void)
{
    int index;

    for (index = 0; index < num_images; index++)
        Image_free(&images[index]);
    free(images);
    images = NULL;
    num_images = 0;
    max_images = 0;
    first_texture = 0;
    images_renderer = NULL;
}

int Bitmap_add(const char *filename, int count, bool scalable)
{
    image_t image;
    image_t *resized;
    int new_max_images;

    if (filename == NULL || filename[0] == '\0'
        || count == 0 || count == INT_MIN) {
        return -1;
    }
    memset(&image, 0, sizeof(image));
    image.filename = xp_strdup(filename);
    if (image.filename == NULL)
        return -1;
    image.num_frames = count < 0 ? -count : count;
    image.rotate = count > 1;
    image.scale = scalable;
    image.filter = image.rotate ? RENDERER_TEXTURE_FILTER_LINEAR
                                : RENDERER_TEXTURE_FILTER_NEAREST;
    image.state = IMG_STATE_UNINITIALIZED;

    if (num_images >= max_images) {
        if (max_images > INT_MAX / 2) {
            free(image.filename);
            return -1;
        }
        new_max_images = max_images == 0 ? 1 : max_images * 2;
        if ((size_t)new_max_images > SIZE_MAX / sizeof(*images)) {
            free(image.filename);
            return -1;
        }
        resized = realloc(images, (size_t)new_max_images * sizeof(*images));
        if (resized == NULL) {
            free(image.filename);
            return -1;
        }
        images = resized;
        max_images = new_max_images;
    }
    images[num_images] = image;
    num_images++;
    return num_images - 1;
}
