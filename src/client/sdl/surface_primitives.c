#include "surface_primitives.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct SurfacePixelAccess {
    SDL_Surface *surface;
    const SDL_PixelFormatDetails *format;
    SDL_Palette *palette;
    SDL_Rect clip;
    Uint32 source_pixel;
    Uint8 red;
    Uint8 green;
    Uint8 blue;
    Uint8 alpha;
    bool locked_here;
} SurfacePixelAccess;

static int Surface_max_int(int first, int second)
{
    return first > second ? first : second;
}

static int Surface_min_int(int first, int second)
{
    return first < second ? first : second;
}

static bool Surface_pixel_access_begin(
    SDL_Surface *surface, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha,
    SurfacePixelAccess *access)
{
    SDL_Rect surface_clip;
    int64_t clip_right;
    int64_t clip_bottom;
    int right;
    int bottom;

    memset(access, 0, sizeof(*access));
    if (surface == NULL)
        return SDL_InvalidParamError("surface");
    if (surface->pixels == NULL || surface->w <= 0
        || surface->h <= 0 || surface->pitch <= 0) {
        return SDL_SetError("Surface has no writable pixel storage");
    }

    access->format = SDL_GetPixelFormatDetails(surface->format);
    if (access->format == NULL)
        return false;
    if (access->format->bytes_per_pixel < 1
        || access->format->bytes_per_pixel > 4) {
        return SDL_SetError("Surface pixel format is not byte-addressable");
    }
    if (!SDL_GetSurfaceClipRect(surface, &surface_clip))
        return false;

    clip_right = (int64_t)surface_clip.x + surface_clip.w;
    clip_bottom = (int64_t)surface_clip.y + surface_clip.h;
    right = Surface_min_int(
        surface->w,
        clip_right > INT32_MAX ? INT32_MAX : (int)clip_right);
    bottom = Surface_min_int(
        surface->h,
        clip_bottom > INT32_MAX ? INT32_MAX : (int)clip_bottom);
    access->clip.x = Surface_max_int(0, surface_clip.x);
    access->clip.y = Surface_max_int(0, surface_clip.y);
    access->clip.w = Surface_max_int(0, right - access->clip.x);
    access->clip.h = Surface_max_int(0, bottom - access->clip.y);

    if (SDL_MUSTLOCK(surface)
        && (surface->flags & SDL_SURFACE_LOCKED) == 0) {
        if (!SDL_LockSurface(surface))
            return false;
        access->locked_here = true;
    }

    access->surface = surface;
    access->palette = SDL_GetSurfacePalette(surface);
    access->red = red;
    access->green = green;
    access->blue = blue;
    access->alpha = alpha;
    access->source_pixel = SDL_MapRGBA(
        access->format, access->palette, red, green, blue, alpha);
    return true;
}

static void Surface_pixel_access_end(SurfacePixelAccess *access)
{
    if (access->locked_here)
        SDL_UnlockSurface(access->surface);
}

static bool Surface_point_is_clipped(
    const SurfacePixelAccess *access, int x, int y)
{
    return x < access->clip.x || y < access->clip.y
        || x >= access->clip.x + access->clip.w
        || y >= access->clip.y + access->clip.h;
}

static Uint32 Surface_load_pixel(
    const SurfacePixelAccess *access, int x, int y)
{
    const Uint8 *pixel = (const Uint8 *)access->surface->pixels
        + (size_t)y * (size_t)access->surface->pitch
        + (size_t)x * access->format->bytes_per_pixel;
    Uint16 value16;
    Uint32 value32;

    switch (access->format->bytes_per_pixel) {
    case 1:
        return pixel[0];
    case 2:
        memcpy(&value16, pixel, sizeof(value16));
        return value16;
    case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        return ((Uint32)pixel[0] << 16)
            | ((Uint32)pixel[1] << 8) | pixel[2];
#else
        return pixel[0] | ((Uint32)pixel[1] << 8)
            | ((Uint32)pixel[2] << 16);
#endif
    default:
        memcpy(&value32, pixel, sizeof(value32));
        return value32;
    }
}

static void Surface_store_pixel(
    const SurfacePixelAccess *access, int x, int y, Uint32 value)
{
    Uint8 *pixel = (Uint8 *)access->surface->pixels
        + (size_t)y * (size_t)access->surface->pitch
        + (size_t)x * access->format->bytes_per_pixel;
    Uint16 value16;

    switch (access->format->bytes_per_pixel) {
    case 1:
        pixel[0] = (Uint8)value;
        break;
    case 2:
        value16 = (Uint16)value;
        memcpy(pixel, &value16, sizeof(value16));
        break;
    case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        pixel[0] = (Uint8)(value >> 16);
        pixel[1] = (Uint8)(value >> 8);
        pixel[2] = (Uint8)value;
#else
        pixel[0] = (Uint8)value;
        pixel[1] = (Uint8)(value >> 8);
        pixel[2] = (Uint8)(value >> 16);
#endif
        break;
    default:
        memcpy(pixel, &value, sizeof(value));
        break;
    }
}

static Uint8 Surface_blend_component(
    Uint8 source, Uint8 destination, Uint8 alpha)
{
    unsigned int blended = (unsigned int)source * alpha
        + (unsigned int)destination * (255u - alpha) + 127u;

    return (Uint8)(blended / 255u);
}

static void Surface_blend_pixel(
    const SurfacePixelAccess *access, int x, int y)
{
    Uint8 destination_red;
    Uint8 destination_green;
    Uint8 destination_blue;
    Uint8 destination_alpha;
    Uint8 output_alpha;
    Uint32 destination_pixel;
    Uint32 output_pixel;

    if (access->alpha == 0 || Surface_point_is_clipped(access, x, y))
        return;
    if (access->alpha == 255) {
        Surface_store_pixel(access, x, y, access->source_pixel);
        return;
    }

    destination_pixel = Surface_load_pixel(access, x, y);
    SDL_GetRGBA(destination_pixel, access->format, access->palette,
                &destination_red, &destination_green,
                &destination_blue, &destination_alpha);
    output_alpha = (Uint8)(access->alpha
        + ((unsigned int)destination_alpha * (255u - access->alpha)
           + 127u) / 255u);
    output_pixel = SDL_MapRGBA(
        access->format, access->palette,
        Surface_blend_component(
            access->red, destination_red, access->alpha),
        Surface_blend_component(
            access->green, destination_green, access->alpha),
        Surface_blend_component(
            access->blue, destination_blue, access->alpha),
        output_alpha);
    Surface_store_pixel(access, x, y, output_pixel);
}

bool Sdl_surface_draw_line_rgba(
    SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2,
    Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    SurfacePixelAccess access;
    int64_t x = x1;
    int64_t y = y1;
    int64_t horizontal = x2 >= x1
        ? (int64_t)x2 - x1 : (int64_t)x1 - x2;
    int64_t vertical = y2 >= y1
        ? (int64_t)y1 - y2 : (int64_t)y2 - y1;
    int64_t x_step = x1 < x2 ? 1 : -1;
    int64_t y_step = y1 < y2 ? 1 : -1;
    int64_t error = horizontal + vertical;

    if (!Surface_pixel_access_begin(
            surface, red, green, blue, alpha, &access)) {
        return false;
    }

    for (;;) {
        Surface_blend_pixel(&access, (int)x, (int)y);
        if (x == x2 && y == y2)
            break;
        if (2 * error >= vertical) {
            error += vertical;
            x += x_step;
        }
        if (2 * error <= horizontal) {
            error += horizontal;
            y += y_step;
        }
    }

    Surface_pixel_access_end(&access);
    return true;
}

static int Surface_compare_intersections(
    const void *first_pointer, const void *second_pointer)
{
    double first = *(const double *)first_pointer;
    double second = *(const double *)second_pointer;

    if (first < second)
        return -1;
    if (first > second)
        return 1;
    return 0;
}

static int Surface_collect_intersections(
    const Sint16 *x_coordinates, const Sint16 *y_coordinates,
    int point_count, int scanline, double *intersections)
{
    double scan_y = (double)scanline + 0.5;
    int intersection_count = 0;
    int current;

    for (current = 0; current < point_count; current++) {
        int previous = current == 0 ? point_count - 1 : current - 1;
        double first_y = y_coordinates[previous];
        double second_y = y_coordinates[current];

        if ((first_y <= scan_y && scan_y < second_y)
            || (second_y <= scan_y && scan_y < first_y)) {
            double first_x = x_coordinates[previous];
            double second_x = x_coordinates[current];

            intersections[intersection_count++] = first_x
                + (scan_y - first_y) * (second_x - first_x)
                    / (second_y - first_y);
        }
    }
    qsort(intersections, (size_t)intersection_count,
          sizeof(*intersections), Surface_compare_intersections);
    return intersection_count;
}

bool Sdl_surface_fill_polygon_rgba(
    SDL_Surface *surface,
    const Sint16 *x_coordinates, const Sint16 *y_coordinates,
    int point_count,
    Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    SurfacePixelAccess access;
    double *intersections;
    int y;

    if (x_coordinates == NULL)
        return SDL_InvalidParamError("x_coordinates");
    if (y_coordinates == NULL)
        return SDL_InvalidParamError("y_coordinates");
    if (point_count < 3)
        return SDL_InvalidParamError("point_count");
    if ((size_t)point_count > SIZE_MAX / sizeof(*intersections))
        return SDL_SetError("Polygon intersection storage is too large");

    intersections = malloc((size_t)point_count * sizeof(*intersections));
    if (intersections == NULL)
        return SDL_SetError("Could not allocate polygon intersections");
    if (!Surface_pixel_access_begin(
            surface, red, green, blue, alpha, &access)) {
        free(intersections);
        return false;
    }

    for (y = access.clip.y; y < access.clip.y + access.clip.h; y++) {
        int intersection_count = Surface_collect_intersections(
            x_coordinates, y_coordinates, point_count, y, intersections);
        int intersection_index;

        for (intersection_index = 0;
             intersection_index + 1 < intersection_count;
             intersection_index += 2) {
            int first_x = (int)ceil(
                intersections[intersection_index] - 0.5);
            int end_x = (int)ceil(
                intersections[intersection_index + 1] - 0.5);
            int x;

            first_x = Surface_max_int(first_x, access.clip.x);
            end_x = Surface_min_int(
                end_x, access.clip.x + access.clip.w);
            for (x = first_x; x < end_x; x++)
                Surface_blend_pixel(&access, x, y);
        }
    }

    Surface_pixel_access_end(&access);
    free(intersections);
    return true;
}
