/* Backend-neutral atlas sprite geometry. */

#include "image_geometry.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static int Image_geometry_atlas_valid(const ImageGeometryAtlas *atlas)
{
    size_t content_width;

    if (atlas == NULL || atlas->texture_width <= 0
        || atlas->texture_height <= 0 || atlas->frame_width <= 0
        || atlas->frame_height <= 0 || atlas->frame_count == 0
        || atlas->frame_count > SIZE_MAX / (size_t)atlas->frame_width) {
        return 0;
    }
    content_width = atlas->frame_count * (size_t)atlas->frame_width;
    return content_width <= (size_t)atlas->texture_width
        && atlas->frame_height <= atlas->texture_height;
}

static int Image_geometry_area_valid(const ImageGeometryAtlas *atlas,
                                     RendererRect area)
{
    int64_t right;
    int64_t bottom;

    if (area.x < 0 || area.y < 0 || area.width <= 0 || area.height <= 0)
        return 0;
    right = (int64_t)area.x + area.width;
    bottom = (int64_t)area.y + area.height;
    return right <= atlas->frame_width && bottom <= atlas->frame_height;
}

static RendererPoint2D Image_geometry_rotate(RendererPoint2D point,
                                             RendererPoint2D center,
                                             float sine, float cosine,
                                             ImageGeometrySpace space)
{
    RendererPoint2D result;
    float x = point.x - center.x;
    float y = point.y - center.y;

    if (space == IMAGE_GEOMETRY_SPACE_WORLD) {
        result.x = center.x + cosine * x - sine * y;
        result.y = center.y + sine * x + cosine * y;
    } else {
        /* HUD Y grows down, so invert the algebraic angle to retain a
         * visually counterclockwise public rotation contract. */
        result.x = center.x + cosine * x + sine * y;
        result.y = center.y - sine * x + cosine * y;
    }
    return result;
}

RendererStatus Image_geometry_sprite(const ImageGeometryAtlas *atlas,
                                     size_t frame_index,
                                     RendererRect area,
                                     RendererPoint2D position,
                                     float radians,
                                     ImageGeometrySpace space,
                                     RendererColor tint,
                                     RendererVertex2D vertices[6])
{
    static const size_t corner_indices[6] = {0, 1, 2, 0, 2, 3};
    RendererPoint2D corners[4];
    RendererPoint2D center;
    RendererVertex2D candidate[6];
    float u_left;
    float u_right;
    float v_top;
    float v_bottom;
    float sine;
    float cosine;
    size_t index;

    if (vertices == NULL || !Image_geometry_atlas_valid(atlas)
        || frame_index >= atlas->frame_count
        || !Image_geometry_area_valid(atlas, area)
        || !isfinite(position.x) || !isfinite(position.y)
        || !isfinite(radians)
        || (space != IMAGE_GEOMETRY_SPACE_WORLD
            && space != IMAGE_GEOMETRY_SPACE_HUD)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (!isfinite(position.x + area.width)
        || !isfinite(position.y + area.height)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    u_left = ((float)frame_index * atlas->frame_width + area.x)
        / atlas->texture_width;
    u_right = ((float)frame_index * atlas->frame_width
               + area.x + area.width) / atlas->texture_width;
    v_top = (float)area.y / atlas->texture_height;
    v_bottom = (float)(area.y + area.height) / atlas->texture_height;

    if (space == IMAGE_GEOMETRY_SPACE_WORLD) {
        corners[0] = (RendererPoint2D){position.x,
                                       position.y + area.height};
        corners[1] = (RendererPoint2D){position.x + area.width,
                                       position.y + area.height};
        corners[2] = (RendererPoint2D){position.x + area.width, position.y};
        corners[3] = position;
    } else {
        corners[0] = position;
        corners[1] = (RendererPoint2D){position.x + area.width, position.y};
        corners[2] = (RendererPoint2D){position.x + area.width,
                                       position.y + area.height};
        corners[3] = (RendererPoint2D){position.x,
                                       position.y + area.height};
    }
    center.x = position.x + area.width * 0.5f;
    center.y = position.y + area.height * 0.5f;
    sine = sinf(radians);
    cosine = cosf(radians);
    for (index = 0; index < 4; index++) {
        corners[index] = Image_geometry_rotate(corners[index], center,
                                               sine, cosine, space);
        if (!isfinite(corners[index].x) || !isfinite(corners[index].y))
            return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    for (index = 0; index < 6; index++) {
        size_t corner = corner_indices[index];

        candidate[index].x = corners[corner].x;
        candidate[index].y = corners[corner].y;
        candidate[index].u = corner == 0 || corner == 3 ? u_left : u_right;
        candidate[index].v = corner == 0 || corner == 1 ? v_top : v_bottom;
        candidate[index].color = tint;
    }
    memcpy(vertices, candidate, sizeof(candidate));
    return RENDERER_STATUS_OK;
}
