#include "test_helpers.h"

#include "renderer.h"
#include "renderer_backend.h"

#include <stdint.h>
#include <string.h>

#define MAX_DRAWS 12
#define MAX_VERTICES 32

typedef struct captured_draw {
    RendererTransform2D transform;
    void *texture;
    size_t vertex_count;
    RendererVertex2D vertices[MAX_VERTICES];
} captured_draw_t;

typedef struct fake_backend {
    int draw_count;
    int texture_token;
    int mesh_token;
    captured_draw_t draws[MAX_DRAWS];
} fake_backend_t;

static const RendererTextureDesc single_pixel_texture_desc = {
    .width = 1,
    .height = 1,
    .filter = RENDERER_TEXTURE_FILTER_NEAREST,
    .wrap = RENDERER_TEXTURE_WRAP_CLAMP
};

static RendererStatus fake_begin_frame(void *context, int width, int height,
                                       RendererColor clear_color)
{
    (void)context;
    (void)width;
    (void)height;
    (void)clear_color;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_draw(void *context,
                                const RendererBackendDraw *draw)
{
    fake_backend_t *backend = context;
    captured_draw_t *captured;

    if (backend->draw_count >= MAX_DRAWS
            || draw->vertex_count > MAX_VERTICES) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    captured = &backend->draws[backend->draw_count++];
    captured->transform = draw->transform;
    captured->texture = draw->texture;
    captured->vertex_count = draw->vertex_count;
    if (draw->vertex_count != 0) {
        memcpy(captured->vertices, draw->vertices,
               draw->vertex_count * sizeof(*draw->vertices));
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_flush(void *context)
{
    (void)context;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_end_frame(void *context)
{
    (void)context;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_texture_create(void *context,
                                          const RendererTextureDesc *desc,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch, void **handle)
{
    fake_backend_t *backend = context;

    (void)desc;
    (void)rgba_pixels;
    (void)pitch;
    *handle = &backend->texture_token;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_texture_update(void *context, void *handle,
                                          RendererRect region,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch)
{
    (void)context;
    (void)handle;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    return RENDERER_STATUS_OK;
}

static void fake_texture_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static RendererStatus fake_mesh_create(void *context,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count, void **handle)
{
    fake_backend_t *backend = context;

    (void)vertices;
    (void)vertex_count;
    *handle = &backend->mesh_token;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_mesh_update(void *context, void *handle,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    (void)context;
    (void)handle;
    (void)vertices;
    (void)vertex_count;
    return RENDERER_STATUS_OK;
}

static void fake_mesh_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static void fake_destroy(void *context)
{
    (void)context;
}

static const RendererBackendInterface fake_backend_interface = {
    .begin_frame = fake_begin_frame,
    .draw = fake_draw,
    .flush = fake_flush,
    .end_frame = fake_end_frame,
    .texture_create = fake_texture_create,
    .texture_update = fake_texture_update,
    .texture_destroy = fake_texture_destroy,
    .mesh_create = fake_mesh_create,
    .mesh_update = fake_mesh_update,
    .mesh_destroy = fake_mesh_destroy,
    .destroy = fake_destroy
};

static Renderer *create_renderer(fake_backend_t *backend)
{
    memset(backend, 0, sizeof(*backend));
    return Renderer_backend_create(&fake_backend_interface, backend);
}

static int color_equal(RendererColor left, RendererColor right)
{
    return left.red == right.red && left.green == right.green
        && left.blue == right.blue && left.alpha == right.alpha;
}

static int vertex_equal(RendererVertex2D actual, float x, float y,
                        float u, float v, RendererColor color)
{
    return actual.x == x && actual.y == y
        && actual.u == u && actual.v == v
        && color_equal(actual.color, color);
}

static float triangle_double_area(const RendererVertex2D *vertices)
{
    return (vertices[1].x - vertices[0].x)
            * (vertices[2].y - vertices[0].y)
        - (vertices[1].y - vertices[0].y)
            * (vertices[2].x - vertices[0].x);
}

static RendererVertex2D transform_vertex(RendererVertex2D vertex,
                                         RendererTransform2D transform)
{
    float x = vertex.x;
    float y = vertex.y;

    vertex.x = transform.m[0] * x + transform.m[3] * y + transform.m[6];
    vertex.y = transform.m[1] * x + transform.m[4] * y + transform.m[7];
    return vertex;
}

static int axis_aligned_quad_equal(const RendererVertex2D *vertices,
                                   float min_x, float min_y,
                                   float max_x, float max_y,
                                   RendererColor color)
{
    unsigned int corner_mask = 0;
    float first_area;
    float second_area;
    size_t index;

    for (index = 0; index < 6; index++) {
        int x_side;
        int y_side;

        if (vertices[index].x == min_x)
            x_side = 0;
        else if (vertices[index].x == max_x)
            x_side = 1;
        else
            return 0;
        if (vertices[index].y == min_y)
            y_side = 0;
        else if (vertices[index].y == max_y)
            y_side = 1;
        else
            return 0;
        if (vertices[index].u != 0.0f || vertices[index].v != 0.0f
                || !color_equal(vertices[index].color, color)) {
            return 0;
        }
        corner_mask |= 1U << (y_side * 2 + x_side);
    }

    first_area = triangle_double_area(vertices);
    second_area = triangle_double_area(vertices + 3);
    if (first_area * second_area <= 0.0f)
        return 0;
    if (first_area < 0.0f)
        first_area = -first_area;
    if (second_area < 0.0f)
        second_area = -second_area;
    return corner_mask == 0x0fU
        && first_area + second_area
            == 2.0f * (max_x - min_x) * (max_y - min_y);
}

static int check_quad_triangulation(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor color = {10, 20, 30, 40};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    const captured_draw_t *draw;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 100, 80, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 10.0f, 20.0f,
                                  30.0f, 40.0f, color)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);

    draw = &backend.draws[0];
    TEST_CHECK(draw->vertex_count == 6);
    TEST_CHECK(vertex_equal(draw->vertices[0], 10.0f, 20.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[1], 40.0f, 20.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[2], 40.0f, 60.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[3], 10.0f, 20.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[4], 40.0f, 60.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[5], 10.0f, 60.0f,
                            0.0f, 0.0f, color));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_triangle_command_order(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor first_color = {255, 0, 0, 255};
    const RendererColor second_color = {0, 255, 0, 255};
    RendererVertex2D first[] = {
        {1.0f, 2.0f, 0.1f, 0.2f, {255, 0, 0, 255}},
        {3.0f, 4.0f, 0.3f, 0.4f, {255, 0, 0, 255}},
        {5.0f, 6.0f, 0.5f, 0.6f, {255, 0, 0, 255}}
    };
    const RendererVertex2D second[] = {
        {11.0f, 12.0f, 0.0f, 0.0f, {0, 255, 0, 255}},
        {13.0f, 14.0f, 0.0f, 0.0f, {0, 255, 0, 255}},
        {15.0f, 16.0f, 0.0f, 0.0f, {0, 255, 0, 255}}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 100, 80, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_triangles(renderer, first, 3)
               == RENDERER_STATUS_OK);
    first[0].x = 99.0f;
    first[0].color.red = 0;
    TEST_CHECK(Renderer_draw_triangles(renderer, second, 3)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_triangles(renderer, first, 2)
               == RENDERER_STATUS_INVALID_ARGUMENT);

    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(backend.draws[0].vertex_count == 3);
    TEST_CHECK(backend.draws[1].vertex_count == 3);
    TEST_CHECK(vertex_equal(backend.draws[0].vertices[0],
                            1.0f, 2.0f, 0.1f, 0.2f, first_color));
    TEST_CHECK(vertex_equal(backend.draws[0].vertices[2],
                            5.0f, 6.0f, 0.5f, 0.6f, first_color));
    TEST_CHECK(vertex_equal(backend.draws[1].vertices[0],
                            11.0f, 12.0f, 0.0f, 0.0f, second_color));
    TEST_CHECK(vertex_equal(backend.draws[1].vertices[2],
                            15.0f, 16.0f, 0.0f, 0.0f, second_color));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_sprite_geometry(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor tint = {40, 80, 120, 160};
    const uint8_t pixel[] = {255, 255, 255, 255};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = NULL;
    const captured_draw_t *draw;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 100, 80, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_sprite(renderer, texture,
                                    10.0f, 20.0f, 40.0f, 60.0f,
                                    0.125f, 0.25f, 0.75f, 0.875f, tint)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);

    draw = &backend.draws[0];
    TEST_CHECK(draw->texture == &backend.texture_token);
    TEST_CHECK(draw->vertex_count == 6);
    TEST_CHECK(vertex_equal(draw->vertices[0], 10.0f, 20.0f,
                            0.125f, 0.25f, tint));
    TEST_CHECK(vertex_equal(draw->vertices[1], 40.0f, 20.0f,
                            0.75f, 0.25f, tint));
    TEST_CHECK(vertex_equal(draw->vertices[2], 40.0f, 60.0f,
                            0.75f, 0.875f, tint));
    TEST_CHECK(vertex_equal(draw->vertices[3], 10.0f, 20.0f,
                            0.125f, 0.25f, tint));
    TEST_CHECK(vertex_equal(draw->vertices[4], 40.0f, 60.0f,
                            0.75f, 0.875f, tint));
    TEST_CHECK(vertex_equal(draw->vertices[5], 10.0f, 60.0f,
                            0.125f, 0.875f, tint));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_stroke_geometry(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor color = {80, 90, 100, 110};
    const RendererPoint2D points[] = {
        {2.0f, 3.0f},
        {8.0f, 3.0f},
        {8.0f, 9.0f}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    const captured_draw_t *draw;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 20, 20, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_stroke_path(renderer, points, 3, 2.0f,
                                    color, 0) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);
    draw = &backend.draws[0];
    TEST_CHECK(draw->vertex_count == 12);
    TEST_CHECK(vertex_equal(draw->vertices[0], 2.0f, 2.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[1], 8.0f, 2.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[2], 8.0f, 4.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[3], 2.0f, 2.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[4], 8.0f, 4.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[5], 2.0f, 4.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(axis_aligned_quad_equal(draw->vertices + 6,
                                       7.0f, 3.0f, 9.0f, 9.0f, color));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_closed_and_zero_length_strokes(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor color = {80, 90, 100, 110};
    const RendererPoint2D closed_points[] = {
        {2.0f, 2.0f},
        {8.0f, 2.0f},
        {8.0f, 8.0f},
        {2.0f, 8.0f}
    };
    const RendererPoint2D duplicate_points[] = {
        {2.0f, 12.0f},
        {2.0f, 12.0f},
        {8.0f, 12.0f}
    };
    const RendererPoint2D all_zero_length[] = {
        {15.0f, 15.0f},
        {15.0f, 15.0f}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 20, 20, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_stroke_path(renderer, closed_points, 4, 2.0f,
                                    color, 1) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_stroke_path(renderer, duplicate_points, 3, 2.0f,
                                    color, 0) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_stroke_path(renderer, all_zero_length, 2, 2.0f,
                                    color, 0) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);

    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(backend.draws[0].vertex_count == 24);
    TEST_CHECK(axis_aligned_quad_equal(backend.draws[0].vertices + 18,
                                       1.0f, 2.0f, 3.0f, 8.0f, color));
    TEST_CHECK(backend.draws[1].vertex_count == 6);
    TEST_CHECK(axis_aligned_quad_equal(backend.draws[1].vertices,
                                       2.0f, 11.0f, 8.0f, 13.0f, color));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_point_geometry(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor color = {120, 130, 140, 150};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    const captured_draw_t *draw;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 20, 20, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_point(renderer, 5.0f, 7.0f, 4.0f, color)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);
    draw = &backend.draws[0];
    TEST_CHECK(draw->vertex_count == 6);
    TEST_CHECK(vertex_equal(draw->vertices[0], 3.0f, 5.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[1], 7.0f, 5.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[2], 7.0f, 9.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[3], 3.0f, 5.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[4], 7.0f, 9.0f,
                            0.0f, 0.0f, color));
    TEST_CHECK(vertex_equal(draw->vertices[5], 3.0f, 9.0f,
                            0.0f, 0.0f, color));

    Renderer_destroy(renderer);
    return 0;
}

static int check_screen_space_stroke_and_point_sizes(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor color = {120, 130, 140, 150};
    const RendererTransform2D scaled = {
        {2.0f, 0.0f, 0.0f,
         0.0f, 3.0f, 0.0f,
         4.0f, 5.0f, 1.0f}
    };
    const RendererPoint2D points[] = {
        {1.0f, 2.0f},
        {4.0f, 2.0f}
    };
    RendererVertex2D screen_vertices[6];
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    size_t index;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 40, 40, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_transform_2d(renderer, scaled)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_stroke_path(renderer, points, 2, 2.0f,
                                    color, 0) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_point(renderer, 3.0f, 4.0f, 4.0f, color)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);

    for (index = 0; index < 6; index++) {
        screen_vertices[index] = transform_vertex(
            backend.draws[0].vertices[index], backend.draws[0].transform);
    }
    TEST_CHECK(axis_aligned_quad_equal(screen_vertices,
                                       6.0f, 10.0f, 12.0f, 12.0f, color));

    for (index = 0; index < 6; index++) {
        screen_vertices[index] = transform_vertex(
            backend.draws[1].vertices[index], backend.draws[1].transform);
    }
    TEST_CHECK(axis_aligned_quad_equal(screen_vertices,
                                       8.0f, 15.0f, 12.0f, 19.0f, color));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_quad_triangulation() == 0);
    TEST_CHECK(check_triangle_command_order() == 0);
    TEST_CHECK(check_sprite_geometry() == 0);
    TEST_CHECK(check_stroke_geometry() == 0);
    TEST_CHECK(check_closed_and_zero_length_strokes() == 0);
    TEST_CHECK(check_point_geometry() == 0);
    TEST_CHECK(check_screen_space_stroke_and_point_sizes() == 0);
    return 0;
}
