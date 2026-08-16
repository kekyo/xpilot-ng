#include "test_helpers.h"

#include "polygon_geometry.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define COORDINATE_LIMIT 8388608.0f
#define GEOMETRY_EPSILON 0.0001

static double twice_signed_area(
    RendererPoint2D first, RendererPoint2D second, RendererPoint2D third)
{
    return ((double)second.x - first.x) * ((double)third.y - first.y)
        - ((double)second.y - first.y) * ((double)third.x - first.x);
}

static int double_near(double actual, double expected)
{
    return fabs(actual - expected) <= GEOMETRY_EPSILON;
}

static int geometry_is_empty(const PolygonGeometry *geometry)
{
    return geometry != NULL
        && geometry->triangle_points == NULL
        && geometry->triangle_point_count == 0;
}

static int geometry_is_well_formed(const PolygonGeometry *geometry)
{
    size_t triangle_start;

    if (geometry == NULL || geometry->triangle_points == NULL
        || geometry->triangle_point_count == 0
        || geometry->triangle_point_count % 3 != 0) {
        return 0;
    }
    for (triangle_start = 0;
         triangle_start < geometry->triangle_point_count;
         triangle_start += 3) {
        const RendererPoint2D first
            = geometry->triangle_points[triangle_start];
        const RendererPoint2D second
            = geometry->triangle_points[triangle_start + 1];
        const RendererPoint2D third
            = geometry->triangle_points[triangle_start + 2];

        if (!isfinite(first.x) || !isfinite(first.y)
            || !isfinite(second.x) || !isfinite(second.y)
            || !isfinite(third.x) || !isfinite(third.y)
            || fabs(twice_signed_area(first, second, third))
                   <= GEOMETRY_EPSILON) {
            return 0;
        }
    }
    return 1;
}

static double geometry_area(const PolygonGeometry *geometry)
{
    double area = 0.0;
    size_t triangle_start;

    for (triangle_start = 0;
         triangle_start < geometry->triangle_point_count;
         triangle_start += 3) {
        area += 0.5 * fabs(twice_signed_area(
            geometry->triangle_points[triangle_start],
            geometry->triangle_points[triangle_start + 1],
            geometry->triangle_points[triangle_start + 2]));
    }
    return area;
}

static int point_is_on_segment(
    RendererPoint2D point, RendererPoint2D first, RendererPoint2D second)
{
    double dx = (double)second.x - first.x;
    double dy = (double)second.y - first.y;
    double point_dx = (double)point.x - first.x;
    double point_dy = (double)point.y - first.y;
    double cross = dx * point_dy - dy * point_dx;
    double scale = fabs(dx) + fabs(dy) + 1.0;

    if (fabs(cross) > GEOMETRY_EPSILON * scale)
        return 0;
    return (double)point.x >= fmin(first.x, second.x) - GEOMETRY_EPSILON
        && (double)point.x <= fmax(first.x, second.x) + GEOMETRY_EPSILON
        && (double)point.y >= fmin(first.y, second.y) - GEOMETRY_EPSILON
        && (double)point.y <= fmax(first.y, second.y) + GEOMETRY_EPSILON;
}

static int point_is_on_contour(
    const RendererPoint2D *points, size_t point_count, RendererPoint2D point)
{
    size_t index;

    for (index = 0; index < point_count; index++) {
        size_t next = (index + 1) % point_count;

        if (point_is_on_segment(point, points[index], points[next]))
            return 1;
    }
    return 0;
}

static int contour_contains_odd(
    const RendererPoint2D *points, size_t point_count, RendererPoint2D point)
{
    int inside = 0;
    size_t current;
    size_t previous = point_count - 1;

    for (current = 0; current < point_count; current++) {
        double current_y = points[current].y;
        double previous_y = points[previous].y;

        if ((current_y > point.y) != (previous_y > point.y)) {
            double crossing_x = points[current].x
                + ((double)point.y - current_y)
                    * ((double)points[previous].x - points[current].x)
                    / (previous_y - current_y);

            if ((double)point.x < crossing_x)
                inside = !inside;
        }
        previous = current;
    }
    return inside;
}

static int triangle_contains(
    RendererPoint2D first, RendererPoint2D second, RendererPoint2D third,
    RendererPoint2D point)
{
    double first_edge = twice_signed_area(first, second, point);
    double second_edge = twice_signed_area(second, third, point);
    double third_edge = twice_signed_area(third, first, point);
    int nonnegative = first_edge >= -GEOMETRY_EPSILON
        && second_edge >= -GEOMETRY_EPSILON
        && third_edge >= -GEOMETRY_EPSILON;
    int nonpositive = first_edge <= GEOMETRY_EPSILON
        && second_edge <= GEOMETRY_EPSILON
        && third_edge <= GEOMETRY_EPSILON;

    return nonnegative || nonpositive;
}

static int geometry_contains(
    const PolygonGeometry *geometry, RendererPoint2D point)
{
    size_t triangle_start;

    for (triangle_start = 0;
         triangle_start < geometry->triangle_point_count;
         triangle_start += 3) {
        if (triangle_contains(
                geometry->triangle_points[triangle_start],
                geometry->triangle_points[triangle_start + 1],
                geometry->triangle_points[triangle_start + 2], point)) {
            return 1;
        }
    }
    return 0;
}

static int check_odd_coverage(
    const RendererPoint2D *contour, size_t contour_count,
    const PolygonGeometry *geometry,
    int minimum_x, int maximum_x, int minimum_y, int maximum_y)
{
    int x_index;
    int y_index;

    for (y_index = 0;
         y_index < (maximum_y - minimum_y) * 2;
         y_index++) {
        for (x_index = 0;
             x_index < (maximum_x - minimum_x) * 2;
             x_index++) {
            RendererPoint2D sample = {
                (float)minimum_x + 0.25f + 0.5f * (float)x_index,
                (float)minimum_y + 0.25f + 0.5f * (float)y_index
            };
            int expected;
            int actual;

            if (point_is_on_contour(contour, contour_count, sample))
                continue;
            expected = contour_contains_odd(contour, contour_count, sample);
            actual = geometry_contains(geometry, sample);
            if (actual != expected) {
                fprintf(stderr,
                        "coverage mismatch at (%g, %g): %d != %d\n",
                        (double)sample.x, (double)sample.y,
                        actual, expected);
                return 1;
            }
        }
    }
    return 0;
}

static int check_convex_winding_and_owned_output(void)
{
    const RendererPoint2D counterclockwise[] = {
        {0.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 4.0f}, {0.0f, 4.0f}
    };
    const RendererPoint2D clockwise[] = {
        {0.0f, 4.0f}, {6.0f, 4.0f}, {6.0f, 0.0f}, {0.0f, 0.0f}
    };
    RendererPoint2D mutable_points[6] = {
        {0.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 4.0f}, {0.0f, 4.0f},
        {101.0f, 103.0f}, {107.0f, 109.0f}
    };
    RendererPoint2D snapshot[6];
    PolygonGeometry geometry = {NULL, 0};
    size_t index;

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   counterclockwise, 4, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(geometry.triangle_point_count == 6);
    TEST_CHECK(double_near(geometry_area(&geometry), 24.0));
    TEST_CHECK(check_odd_coverage(
                   counterclockwise, 4, &geometry, -1, 7, -1, 5) == 0);
    Polygon_geometry_cleanup(&geometry);

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   clockwise, 4, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(geometry.triangle_point_count == 6);
    TEST_CHECK(double_near(geometry_area(&geometry), 24.0));
    TEST_CHECK(check_odd_coverage(
                   clockwise, 4, &geometry, -1, 7, -1, 5) == 0);
    Polygon_geometry_cleanup(&geometry);

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   mutable_points, 4, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry.triangle_point_count == 6);
    memcpy(snapshot, geometry.triangle_points, sizeof(snapshot));
    for (index = 0; index < 4; index++) {
        mutable_points[index].x += 1000.0f;
        mutable_points[index].y -= 1000.0f;
    }
    TEST_CHECK(memcmp(snapshot, geometry.triangle_points,
                      sizeof(snapshot)) == 0);

    Polygon_geometry_cleanup(&geometry);
    TEST_CHECK(geometry_is_empty(&geometry));
    Polygon_geometry_cleanup(&geometry);
    TEST_CHECK(geometry_is_empty(&geometry));
    Polygon_geometry_cleanup(NULL);
    return 0;
}

static int check_concave_l_shape(void)
{
    const RendererPoint2D contour[] = {
        {0.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 2.0f},
        {2.0f, 2.0f}, {2.0f, 6.0f}, {0.0f, 6.0f}
    };
    PolygonGeometry geometry = {NULL, 0};

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   contour, 6, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(double_near(geometry_area(&geometry), 20.0));
    TEST_CHECK(check_odd_coverage(
                   contour, 6, &geometry, -1, 7, -1, 7) == 0);
    TEST_CHECK(geometry_contains(
        &geometry, (RendererPoint2D){1.0f, 5.0f}));
    TEST_CHECK(geometry_contains(
        &geometry, (RendererPoint2D){5.0f, 1.0f}));
    TEST_CHECK(!geometry_contains(
        &geometry, (RendererPoint2D){5.0f, 5.0f}));
    Polygon_geometry_cleanup(&geometry);
    return 0;
}

static int check_self_intersections_use_odd_winding(void)
{
    const RendererPoint2D bowtie[] = {
        {0.0f, 0.0f}, {6.0f, 6.0f}, {0.0f, 6.0f}, {6.0f, 0.0f}
    };
    const RendererPoint2D pentagram[] = {
        {0.0f, 6.0f}, {-4.0f, -5.0f}, {6.0f, 2.0f},
        {-6.0f, 2.0f}, {4.0f, -5.0f}
    };
    const RendererPoint2D center = {0.0f, 0.0f};
    const RendererPoint2D upper_arm = {0.0f, 4.0f};
    PolygonGeometry geometry = {NULL, 0};

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   bowtie, 4, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(double_near(geometry_area(&geometry), 18.0));
    TEST_CHECK(check_odd_coverage(
                   bowtie, 4, &geometry, -1, 7, -1, 7) == 0);
    Polygon_geometry_cleanup(&geometry);

    TEST_CHECK(!contour_contains_odd(pentagram, 5, center));
    TEST_CHECK(contour_contains_odd(pentagram, 5, upper_arm));
    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   pentagram, 5, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(check_odd_coverage(
                   pentagram, 5, &geometry, -7, 7, -6, 7) == 0);
    TEST_CHECK(!geometry_contains(&geometry, center));
    TEST_CHECK(geometry_contains(&geometry, upper_arm));
    Polygon_geometry_cleanup(&geometry);
    return 0;
}

static int check_duplicate_collinear_and_zero_area_contours(void)
{
    const RendererPoint2D noisy_square[] = {
        {0.0f, 0.0f}, {3.0f, 0.0f}, {6.0f, 0.0f}, {6.0f, 0.0f},
        {6.0f, 4.0f}, {3.0f, 4.0f}, {0.0f, 4.0f}, {0.0f, 0.0f}
    };
    const RendererPoint2D all_collinear[] = {
        {0.0f, 0.0f}, {2.0f, 0.0f}, {4.0f, 0.0f},
        {4.0f, 0.0f}, {6.0f, 0.0f}
    };
    PolygonGeometry geometry = {NULL, 0};

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   noisy_square, 8, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(double_near(geometry_area(&geometry), 24.0));
    TEST_CHECK(check_odd_coverage(
                   noisy_square, 8, &geometry, -1, 7, -1, 5) == 0);
    Polygon_geometry_cleanup(&geometry);

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   all_collinear, 5, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_empty(&geometry));
    Polygon_geometry_cleanup(&geometry);
    return 0;
}

static int expect_empty_result(
    const RendererPoint2D *points, size_t point_count,
    RendererStatus expected_status)
{
    PolygonGeometry geometry = {NULL, 0};

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   points, point_count, &geometry) == expected_status);
    TEST_CHECK(geometry_is_empty(&geometry));
    Polygon_geometry_cleanup(&geometry);
    return 0;
}

static int check_validation_status_and_empty_output(void)
{
    const RendererPoint2D valid_triangle[] = {
        {0.0f, 0.0f}, {4.0f, 0.0f}, {0.0f, 4.0f}
    };
    const RendererPoint2D valid_limits[] = {
        {COORDINATE_LIMIT - 2.0f, -COORDINATE_LIMIT},
        {COORDINATE_LIMIT, -COORDINATE_LIMIT},
        {COORDINATE_LIMIT, -COORDINATE_LIMIT + 2.0f}
    };
    RendererPoint2D invalid[3];
    PolygonGeometry geometry = {NULL, 0};
    size_t count;

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   valid_limits, 3, &geometry) == RENDERER_STATUS_OK);
    TEST_CHECK(geometry_is_well_formed(&geometry));
    TEST_CHECK(double_near(geometry_area(&geometry), 2.0));

    memcpy(invalid, valid_triangle, sizeof(invalid));
    invalid[1].x = NAN;
    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   invalid, 3, &geometry)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(geometry_is_empty(&geometry));

    TEST_CHECK(Polygon_geometry_tessellate_odd(
                   valid_triangle, 3, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(expect_empty_result(
                   NULL, 3, RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    for (count = 0; count < 3; count++) {
        TEST_CHECK(expect_empty_result(
                       valid_triangle, count,
                       RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    }

    memcpy(invalid, valid_triangle, sizeof(invalid));
    invalid[1].y = INFINITY;
    TEST_CHECK(expect_empty_result(
                   invalid, 3, RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    memcpy(invalid, valid_triangle, sizeof(invalid));
    invalid[2].x = -INFINITY;
    TEST_CHECK(expect_empty_result(
                   invalid, 3, RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    memcpy(invalid, valid_triangle, sizeof(invalid));
    invalid[1].x = COORDINATE_LIMIT + 1.0f;
    TEST_CHECK(expect_empty_result(
                   invalid, 3, RENDERER_STATUS_INVALID_ARGUMENT) == 0);
    memcpy(invalid, valid_triangle, sizeof(invalid));
    invalid[2].y = -COORDINATE_LIMIT - 1.0f;
    TEST_CHECK(expect_empty_result(
                   invalid, 3, RENDERER_STATUS_INVALID_ARGUMENT) == 0);

    TEST_CHECK(expect_empty_result(
                   (const RendererPoint2D *)(uintptr_t)1,
                   (size_t)INT_MAX + 1,
                   RENDERER_STATUS_OUT_OF_MEMORY) == 0);
    return 0;
}

int main(void)
{
    if (check_convex_winding_and_owned_output() != 0)
        return 1;
    if (check_concave_l_shape() != 0)
        return 1;
    if (check_self_intersections_use_odd_winding() != 0)
        return 1;
    if (check_duplicate_collinear_and_zero_area_contours() != 0)
        return 1;
    if (check_validation_status_and_empty_output() != 0)
        return 1;
    return 0;
}
