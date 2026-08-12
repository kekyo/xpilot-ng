#include "test_helpers.h"

#include "image_geometry.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FLOAT_TOLERANCE 0.0001f

static int float_equal(float actual, float expected)
{
    return fabsf(actual - expected) <= FLOAT_TOLERANCE;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, float u, float v,
                        RendererColor color)
{
    return float_equal(vertex->x, x)
        && float_equal(vertex->y, y)
        && float_equal(vertex->u, u)
        && float_equal(vertex->v, v)
        && vertex->color.red == color.red
        && vertex->color.green == color.green
        && vertex->color.blue == color.blue
        && vertex->color.alpha == color.alpha;
}

static int check_world_atlas_area_uses_bottom_left_position(void)
{
    const ImageGeometryAtlas atlas = {16, 8, 5, 6, 3};
    const RendererRect area = {1, 2, 3, 4};
    const RendererPoint2D position = {10.0f, 20.0f};
    const RendererColor tint = {17, 33, 65, 129};
    RendererVertex2D vertices[6];

    TEST_CHECK(Image_geometry_sprite(&atlas, 1, area, position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, tint,
                                     vertices) == RENDERER_STATUS_OK);
    TEST_CHECK(vertex_equal(&vertices[0], 10.0f, 24.0f,
                            6.0f / 16.0f, 2.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[1], 13.0f, 24.0f,
                            9.0f / 16.0f, 2.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[2], 13.0f, 20.0f,
                            9.0f / 16.0f, 6.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[3], 10.0f, 24.0f,
                            6.0f / 16.0f, 2.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[4], 13.0f, 20.0f,
                            9.0f / 16.0f, 6.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[5], 10.0f, 20.0f,
                            6.0f / 16.0f, 6.0f / 8.0f, tint));
    return 0;
}

static int check_hud_atlas_area_uses_top_left_position(void)
{
    const ImageGeometryAtlas atlas = {16, 8, 5, 6, 3};
    const RendererRect area = {1, 2, 3, 4};
    const RendererPoint2D position = {10.0f, 20.0f};
    const RendererColor tint = {255, 127, 63, 31};
    RendererVertex2D vertices[6];

    TEST_CHECK(Image_geometry_sprite(&atlas, 1, area, position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_HUD, tint,
                                     vertices) == RENDERER_STATUS_OK);
    TEST_CHECK(vertex_equal(&vertices[0], 10.0f, 20.0f,
                            6.0f / 16.0f, 2.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[1], 13.0f, 20.0f,
                            9.0f / 16.0f, 2.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[2], 13.0f, 24.0f,
                            9.0f / 16.0f, 6.0f / 8.0f, tint));
    TEST_CHECK(vertex_equal(&vertices[5], 10.0f, 24.0f,
                            6.0f / 16.0f, 6.0f / 8.0f, tint));
    return 0;
}

static int check_rotation_is_visually_counterclockwise(void)
{
    const ImageGeometryAtlas atlas = {4, 2, 4, 2, 1};
    const RendererRect area = {0, 0, 4, 2};
    const RendererPoint2D world_position = {10.0f, 20.0f};
    const RendererPoint2D hud_position = {10.0f, 20.0f};
    const RendererColor white = {255, 255, 255, 255};
    RendererVertex2D world[6];
    RendererVertex2D hud[6];
    const float half_pi = 1.57079632679489661923f;

    TEST_CHECK(Image_geometry_sprite(&atlas, 0, area, world_position,
                                     half_pi, IMAGE_GEOMETRY_SPACE_WORLD,
                                     white, world) == RENDERER_STATUS_OK);
    TEST_CHECK(vertex_equal(&world[0], 11.0f, 19.0f,
                            0.0f, 0.0f, white));
    TEST_CHECK(vertex_equal(&world[1], 11.0f, 23.0f,
                            1.0f, 0.0f, white));
    TEST_CHECK(vertex_equal(&world[2], 13.0f, 23.0f,
                            1.0f, 1.0f, white));
    TEST_CHECK(vertex_equal(&world[5], 13.0f, 19.0f,
                            0.0f, 1.0f, white));

    TEST_CHECK(Image_geometry_sprite(&atlas, 0, area, hud_position,
                                     half_pi, IMAGE_GEOMETRY_SPACE_HUD,
                                     white, hud) == RENDERER_STATUS_OK);
    TEST_CHECK(vertex_equal(&hud[0], 11.0f, 23.0f,
                            0.0f, 0.0f, white));
    TEST_CHECK(vertex_equal(&hud[1], 11.0f, 19.0f,
                            1.0f, 0.0f, white));
    TEST_CHECK(vertex_equal(&hud[2], 13.0f, 19.0f,
                            1.0f, 1.0f, white));
    TEST_CHECK(vertex_equal(&hud[5], 13.0f, 23.0f,
                            0.0f, 1.0f, white));
    return 0;
}

static int check_invalid_input_is_rejected_atomically(void)
{
    const ImageGeometryAtlas valid_atlas = {16, 8, 5, 6, 3};
    const RendererRect valid_area = {0, 0, 5, 6};
    const RendererPoint2D valid_position = {2.0f, 3.0f};
    const RendererColor white = {255, 255, 255, 255};
    ImageGeometryAtlas invalid_atlas;
    RendererVertex2D vertices[6];
    RendererVertex2D original[6];

    memset(vertices, 0xa5, sizeof(vertices));
    memcpy(original, vertices, sizeof(vertices));
    TEST_CHECK(Image_geometry_sprite(NULL, 0, valid_area, valid_position,
                                     0.0f, IMAGE_GEOMETRY_SPACE_WORLD,
                                     white, vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(vertices, original, sizeof(vertices)) == 0);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0, valid_area,
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);

    invalid_atlas = valid_atlas;
    invalid_atlas.texture_width = 0;
    TEST_CHECK(Image_geometry_sprite(&invalid_atlas, 0, valid_area,
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    invalid_atlas = valid_atlas;
    invalid_atlas.frame_count = SIZE_MAX;
    TEST_CHECK(Image_geometry_sprite(&invalid_atlas, 0, valid_area,
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 3, valid_area,
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0,
                                     (RendererRect){0, 0, 0, 6},
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0,
                                     (RendererRect){4, 0, 2, 6},
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0,
                                     (RendererRect){INT_MAX, 0, 1, 1},
                                     valid_position, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0, valid_area,
                                     (RendererPoint2D){NAN, 0.0f}, 0.0f,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0, valid_area,
                                     valid_position, INFINITY,
                                     IMAGE_GEOMETRY_SPACE_WORLD, white,
                                     vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Image_geometry_sprite(&valid_atlas, 0, valid_area,
                                     valid_position, 0.0f,
                                     (ImageGeometrySpace)99, white, vertices)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(memcmp(vertices, original, sizeof(vertices)) == 0);
    return 0;
}

int main(void)
{
    if (check_world_atlas_area_uses_bottom_left_position() != 0)
        return 1;
    if (check_hud_atlas_area_uses_top_left_position() != 0)
        return 1;
    if (check_rotation_is_visually_counterclockwise() != 0)
        return 1;
    if (check_invalid_input_is_rejected_atomically() != 0)
        return 1;
    return 0;
}
