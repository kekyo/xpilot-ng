/* SDL3 surface drawing primitives used by the XPilot client. */

#ifndef SURFACE_PRIMITIVES_H
#define SURFACE_PRIMITIVES_H

#include <SDL3/SDL.h>

/**
 * Draw a clipped, one-pixel-wide RGBA line on an SDL surface.
 *
 * @param surface Surface whose pixels are updated.
 * @param x1 Horizontal coordinate of the first endpoint.
 * @param y1 Vertical coordinate of the first endpoint.
 * @param x2 Horizontal coordinate of the second endpoint.
 * @param y2 Vertical coordinate of the second endpoint.
 * @param red Red source component.
 * @param green Green source component.
 * @param blue Blue source component.
 * @param alpha Source opacity.
 * @return @c true on success, including a completely clipped line; otherwise
 *         @c false with an SDL error set.
 *
 * @remarks Both endpoints are included. Non-opaque colors use straight-alpha
 *          source-over blending. The surface clip rectangle is honored.
 */
bool Sdl_surface_draw_line_rgba(
    SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2,
    Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);

/**
 * Fill one clipped polygon contour using the odd-even rule.
 *
 * @param surface Surface whose pixels are updated.
 * @param x_coordinates Borrowed array of horizontal vertex coordinates.
 * @param y_coordinates Borrowed array of vertical vertex coordinates.
 * @param point_count Number of vertices in both coordinate arrays; must be at
 *        least three.
 * @param red Red source component.
 * @param green Green source component.
 * @param blue Blue source component.
 * @param alpha Source opacity.
 * @return @c true on success, including a completely clipped polygon;
 *         otherwise @c false with an SDL error set.
 *
 * @remarks The final vertex is connected to the first. Concave and
 *          self-intersecting contours are supported. Non-opaque colors use
 *          straight-alpha source-over blending. The surface clip rectangle
 *          is honored.
 */
bool Sdl_surface_fill_polygon_rgba(
    SDL_Surface *surface,
    const Sint16 *x_coordinates, const Sint16 *y_coordinates,
    int point_count,
    Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);

#endif /* SURFACE_PRIMITIVES_H */
