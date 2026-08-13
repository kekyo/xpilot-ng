/* Coordinate transforms for semantic SDL rendering. */

#include "paint_transform.h"

#include <math.h>
#include <string.h>

static int Paint_dimensions_valid(int logical_width, int logical_height,
                                  int drawable_width, int drawable_height)
{
    return logical_width > 0 && logical_height > 0
        && drawable_width > 0 && drawable_height > 0;
}

static int Paint_transform_values_finite(const RendererTransform2D *transform)
{
    size_t index;

    for (index = 0; index < sizeof(transform->m) / sizeof(transform->m[0]);
         index++) {
        if (!isfinite(transform->m[index]))
            return 0;
    }
    return 1;
}

int Paint_transform_hud(int logical_width, int logical_height,
                        int drawable_width, int drawable_height,
                        RendererTransform2D *transform)
{
    double scale_x;
    double scale_y;

    if (transform == NULL
        || !Paint_dimensions_valid(logical_width, logical_height,
                                   drawable_width, drawable_height)) {
        return -1;
    }

    scale_x = (double)drawable_width / logical_width;
    scale_y = (double)drawable_height / logical_height;
    memset(transform, 0, sizeof(*transform));
    transform->m[0] = (float)scale_x;
    transform->m[4] = (float)scale_y;
    transform->m[8] = 1.0f;
    return Paint_transform_values_finite(transform) ? 0 : -1;
}

int Paint_transform_world(double scale, double world_x, double world_y,
                          int logical_width, int logical_height,
                          int drawable_width, int drawable_height,
                          int rounded_translation,
                          RendererTransform2D *transform)
{
    double scale_x;
    double scale_y;
    double translation_x;
    double translation_y;

    if (transform == NULL || !isfinite(scale) || scale <= 0.0
        || !isfinite(world_x) || !isfinite(world_y)
        || (rounded_translation != 0 && rounded_translation != 1)
        || !Paint_dimensions_valid(logical_width, logical_height,
                                   drawable_width, drawable_height)) {
        return -1;
    }

    scale_x = (double)drawable_width / logical_width;
    scale_y = (double)drawable_height / logical_height;
    translation_x = -world_x * scale;
    translation_y = -world_y * scale;
    if (rounded_translation) {
        translation_x = rint(translation_x);
        translation_y = rint(translation_y);
    }

    memset(transform, 0, sizeof(*transform));
    transform->m[0] = (float)(scale_x * scale);
    transform->m[4] = (float)(-scale_y * scale);
    transform->m[6] = (float)(scale_x * translation_x);
    transform->m[7] = (float)(drawable_height - scale_y * translation_y);
    transform->m[8] = 1.0f;
    return Paint_transform_values_finite(transform) ? 0 : -1;
}
