/* Backend-neutral polygon tessellation geometry. */

#ifndef POLYGON_GEOMETRY_H
#define POLYGON_GEOMETRY_H

#include "renderer.h"

#include <stddef.h>

/** Owned triangle-list geometry produced from one polygon contour. */
typedef struct PolygonGeometry {
    /** Owned points grouped as consecutive triples, or NULL when empty. */
    RendererPoint2D *triangle_points;
    /** Number of points in @c triangle_points; always divisible by three. */
    size_t triangle_point_count;
} PolygonGeometry;

/**
 * Release owned polygon geometry and reset it to the empty state.
 *
 * @param geometry Geometry to clean up, or NULL for no operation.
 *
 * @remarks Calling this function repeatedly is safe.
 */
void Polygon_geometry_cleanup(PolygonGeometry *geometry);

/**
 * Tessellate one closed contour using the odd winding rule.
 *
 * @param points Borrowed contour points; the final point is connected to the
 *        first without requiring a repeated closing point.
 * @param point_count Number of contour points; must be at least three and no
 *        greater than @c INT_MAX.
 * @param geometry Initialized output whose previous owned result is released
 *        before this request is processed.
 * @return Operation status. A count greater than @c INT_MAX reports
 *         RENDERER_STATUS_OUT_OF_MEMORY without reading @p points.
 *
 * @remarks Coordinates must be finite and lie in the inclusive range
 *          [-2^23, 2^23]. On success, @p geometry owns a non-overlapping
 *          triangle list independent of @p points. Duplicate and collinear
 *          contour points are accepted; a contour with no filled area
 *          succeeds with empty geometry. Every failure leaves @p geometry in
 *          the empty state.
 */
RendererStatus Polygon_geometry_tessellate_odd(
    const RendererPoint2D *points, size_t point_count,
    PolygonGeometry *geometry);

#endif /* POLYGON_GEOMETRY_H */
