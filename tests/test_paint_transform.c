#include "test_helpers.h"

#include "paint_transform.h"

#include <math.h>
#include <stddef.h>

#define FLOAT_TOLERANCE 0.0001f

typedef struct transformed_point {
    float x;
    float y;
} transformed_point_t;

static transformed_point_t transform_point(RendererTransform2D transform,
                                           float x, float y)
{
    transformed_point_t result;

    result.x = transform.m[0] * x + transform.m[3] * y + transform.m[6];
    result.y = transform.m[1] * x + transform.m[4] * y + transform.m[7];
    return result;
}

static int float_equal(float actual, float expected)
{
    return fabsf(actual - expected) <= FLOAT_TOLERANCE;
}

static int point_equal(transformed_point_t actual, float expected_x,
                       float expected_y)
{
    return float_equal(actual.x, expected_x)
        && float_equal(actual.y, expected_y);
}

static int check_hud_scales_logical_coordinates_to_drawable_pixels(void)
{
    RendererTransform2D transform;

    TEST_CHECK(Paint_transform_hud(640, 480, 1280, 720, &transform) == 0);
    TEST_CHECK(point_equal(transform_point(transform, 0.0f, 0.0f),
                           0.0f, 0.0f));
    TEST_CHECK(point_equal(transform_point(transform, 640.0f, 480.0f),
                           1280.0f, 720.0f));
    TEST_CHECK(point_equal(transform_point(transform, 123.0f, 200.0f),
                           246.0f, 300.0f));
    return 0;
}

static int check_moving_world_uses_top_left_drawable_coordinates(void)
{
    RendererTransform2D transform;

    TEST_CHECK(Paint_transform_world(0.5, 100.0, 40.0,
                                     320, 200, 640, 300, 0,
                                     &transform) == 0);

    /* world_y is the bottom of the visible world; renderer Y grows down. */
    TEST_CHECK(point_equal(transform_point(transform, 100.0f, 40.0f),
                           0.0f, 300.0f));
    TEST_CHECK(point_equal(transform_point(transform, 100.0f, 440.0f),
                           0.0f, 0.0f));
    TEST_CHECK(point_equal(transform_point(transform, 220.0f, 120.0f),
                           120.0f, 240.0f));
    return 0;
}

static int check_stationary_world_rounds_only_camera_translation(void)
{
    RendererTransform2D transform;

    TEST_CHECK(Paint_transform_world(0.6, 10.75, -3.25,
                                     100, 80, 250, 120, 1,
                                     &transform) == 0);

    /*
     * The legacy stationary path computes T(rint(-world * scale)) * S.
     * Drawable scaling and the top-left Y conversion happen afterwards.
     */
    TEST_CHECK(point_equal(transform_point(transform, 0.0f, 0.0f),
                           -15.0f, 117.0f));
    TEST_CHECK(point_equal(transform_point(transform, 20.0f, 10.0f),
                           15.0f, 108.0f));
    return 0;
}

static int check_world_without_rounding_preserves_fractional_camera_offset(void)
{
    RendererTransform2D transform;

    TEST_CHECK(Paint_transform_world(0.6, 10.75, -3.25,
                                     100, 80, 250, 120, 0,
                                     &transform) == 0);
    TEST_CHECK(point_equal(transform_point(transform, 20.0f, 10.0f),
                           13.875f, 108.075f));
    return 0;
}

static int check_invalid_arguments_are_rejected(void)
{
    RendererTransform2D transform;

    TEST_CHECK(Paint_transform_hud(0, 100, 200, 200, &transform) == -1);
    TEST_CHECK(Paint_transform_hud(100, -1, 200, 200, &transform) == -1);
    TEST_CHECK(Paint_transform_hud(100, 100, 0, 200, &transform) == -1);
    TEST_CHECK(Paint_transform_hud(100, 100, 200, -1, &transform) == -1);
    TEST_CHECK(Paint_transform_hud(100, 100, 200, 200, NULL) == -1);

    TEST_CHECK(Paint_transform_world(0.0, 0.0, 0.0,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(-1.0, 0.0, 0.0,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(NAN, 0.0, 0.0,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(INFINITY, 0.0, 0.0,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(1.0, NAN, 0.0,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(1.0, 0.0, -INFINITY,
                                     100, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(1.0, 0.0, 0.0,
                                     0, 100, 200, 200, 0,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(1.0, 0.0, 0.0,
                                     100, 100, 200, 200, 2,
                                     &transform) == -1);
    TEST_CHECK(Paint_transform_world(1.0, 0.0, 0.0,
                                     100, 100, 200, 200, 0,
                                     NULL) == -1);
    return 0;
}

int main(void)
{
    if (check_hud_scales_logical_coordinates_to_drawable_pixels() != 0)
        return 1;
    if (check_moving_world_uses_top_left_drawable_coordinates() != 0)
        return 1;
    if (check_stationary_world_rounds_only_camera_translation() != 0)
        return 1;
    if (check_world_without_rounding_preserves_fractional_camera_offset() != 0)
        return 1;
    if (check_invalid_arguments_are_rejected() != 0)
        return 1;
    return 0;
}
