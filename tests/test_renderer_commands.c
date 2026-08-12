#include "test_helpers.h"

#include "renderer.h"
#include "renderer_backend.h"

#include <stdint.h>
#include <string.h>

#define MAX_DRAWS 16
#define MAX_VERTICES 32

typedef struct captured_draw {
    RendererTransform2D transform;
    RendererBlendMode blend;
    int scissor_enabled;
    RendererRect scissor;
    void *texture;
    void *mesh;
    size_t vertex_count;
    RendererVertex2D vertices[MAX_VERTICES];
} captured_draw_t;

typedef struct fake_backend {
    int begin_count;
    int draw_attempt_count;
    int flush_count;
    int end_count;
    int destroy_count;
    int texture_create_count;
    int texture_update_count;
    int texture_destroy_count;
    int mesh_create_count;
    int mesh_update_count;
    int mesh_destroy_count;
    int draw_count;
    int fail_draw_attempt;
    int fail_next_flush;
    int frame_width;
    int frame_height;
    RendererColor clear_color;
    RendererTextureDesc texture_desc;
    unsigned char texture_token;
    unsigned char mesh_token;
    captured_draw_t draws[MAX_DRAWS];
} fake_backend_t;

static const RendererTextureDesc single_pixel_texture_desc = {
    .width = 1,
    .height = 1,
    .filter = RENDERER_TEXTURE_FILTER_NEAREST,
    .wrap = RENDERER_TEXTURE_WRAP_CLAMP
};

static int color_equal(RendererColor left, RendererColor right)
{
    return left.red == right.red && left.green == right.green
        && left.blue == right.blue && left.alpha == right.alpha;
}

static int rect_equal(RendererRect left, RendererRect right)
{
    return left.x == right.x && left.y == right.y
        && left.width == right.width && left.height == right.height;
}

static int transform_equal(RendererTransform2D left,
                           RendererTransform2D right)
{
    return memcmp(left.m, right.m, sizeof(left.m)) == 0;
}

static RendererStatus fake_begin_frame(void *context, int width, int height,
                                       RendererColor clear_color)
{
    fake_backend_t *backend = context;

    backend->begin_count++;
    backend->frame_width = width;
    backend->frame_height = height;
    backend->clear_color = clear_color;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_draw(void *context,
                                const RendererBackendDraw *draw)
{
    fake_backend_t *backend = context;
    captured_draw_t *captured;

    backend->draw_attempt_count++;
    if (backend->draw_attempt_count == backend->fail_draw_attempt) {
        backend->fail_draw_attempt = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (backend->draw_count >= MAX_DRAWS
            || draw->vertex_count > MAX_VERTICES) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    captured = &backend->draws[backend->draw_count++];
    captured->transform = draw->transform;
    captured->blend = draw->blend;
    captured->scissor_enabled = draw->scissor_enabled;
    captured->scissor = draw->scissor;
    captured->texture = draw->texture;
    captured->mesh = draw->mesh;
    captured->vertex_count = draw->vertex_count;
    if (draw->vertex_count != 0) {
        memcpy(captured->vertices, draw->vertices,
               draw->vertex_count * sizeof(*draw->vertices));
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_flush(void *context)
{
    fake_backend_t *backend = context;

    backend->flush_count++;
    if (backend->fail_next_flush) {
        backend->fail_next_flush = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_end_frame(void *context)
{
    fake_backend_t *backend = context;

    backend->end_count++;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_texture_create(void *context,
                                          const RendererTextureDesc *desc,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch, void **handle)
{
    fake_backend_t *backend = context;

    if (desc == NULL || desc->width <= 0 || desc->height <= 0
            || rgba_pixels == NULL
            || pitch < (size_t)desc->width * 4 || handle == NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    backend->texture_create_count++;
    backend->texture_desc = *desc;
    *handle = &backend->texture_token;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_texture_update(void *context, void *handle,
                                          RendererRect region,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch)
{
    fake_backend_t *backend = context;

    if (handle != &backend->texture_token || region.width <= 0
            || region.height <= 0 || rgba_pixels == NULL
            || pitch < (size_t)region.width * 4) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    backend->texture_update_count++;
    return RENDERER_STATUS_OK;
}

static void fake_texture_destroy(void *context, void *handle)
{
    fake_backend_t *backend = context;

    if (handle == &backend->texture_token)
        backend->texture_destroy_count++;
}

static RendererStatus fake_mesh_create(void *context,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count, void **handle)
{
    fake_backend_t *backend = context;

    if (vertices == NULL || vertex_count == 0 || handle == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    backend->mesh_create_count++;
    *handle = &backend->mesh_token;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_mesh_update(void *context, void *handle,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    fake_backend_t *backend = context;

    if (handle != &backend->mesh_token || vertices == NULL
            || vertex_count == 0) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    backend->mesh_update_count++;
    return RENDERER_STATUS_OK;
}

static void fake_mesh_destroy(void *context, void *handle)
{
    fake_backend_t *backend = context;

    if (handle == &backend->mesh_token)
        backend->mesh_destroy_count++;
}

static void fake_destroy(void *context)
{
    fake_backend_t *backend = context;

    backend->destroy_count++;
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

static int check_color_conversion(void)
{
    RendererColor color = Renderer_color_from_rgba32(UINT32_C(0x12345678));

    TEST_CHECK(color.red == 0x12);
    TEST_CHECK(color.green == 0x34);
    TEST_CHECK(color.blue == 0x56);
    TEST_CHECK(color.alpha == 0x78);
    TEST_CHECK(Renderer_color_to_rgba32(color) == UINT32_C(0x12345678));
    return 0;
}

static int check_frame_lifecycle_and_default_state(void)
{
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    const RendererColor clear = {1, 2, 3, 4};
    const RendererColor white = {255, 255, 255, 255};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_end_frame(renderer)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_begin_frame(renderer, 320, 200, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 320, 200, clear)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_fill_rect(renderer, 1.0f, 2.0f, 3.0f, 4.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);

    TEST_CHECK(backend.begin_count == 1);
    TEST_CHECK(backend.frame_width == 320);
    TEST_CHECK(backend.frame_height == 200);
    TEST_CHECK(color_equal(backend.clear_color, clear));
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(transform_equal(backend.draws[0].transform, identity));
    TEST_CHECK(backend.draws[0].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(!backend.draws[0].scissor_enabled);
    TEST_CHECK(backend.flush_count == 1);
    TEST_CHECK(backend.end_count == 1);

    Renderer_destroy(renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_state_snapshot_and_scissor(void)
{
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    const RendererTransform2D first_transform = {
        {2.0f, 0.0f, 0.0f,
         0.0f, 3.0f, 0.0f,
         4.0f, 5.0f, 1.0f}
    };
    const RendererTransform2D second_transform = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         8.0f, 9.0f, 1.0f}
    };
    const RendererRect partially_visible = {-10, 5, 30, 100};
    const RendererRect first_clamped = {0, 5, 20, 75};
    const RendererRect edge_visible = {95, 75, 10, 10};
    const RendererRect second_clamped = {95, 75, 5, 5};
    const RendererRect empty = {120, 10, 5, 5};
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor red = {255, 0, 0, 255};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 100, 80, black)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_set_transform_2d(renderer, first_transform)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, &partially_visible)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 0.0f, 0.0f, 1.0f, 1.0f, red)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_set_transform_2d(renderer, second_transform)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ADDITIVE)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, &edge_visible)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 2.0f, 2.0f, 1.0f, 1.0f, red)
               == RENDERER_STATUS_OK);

    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(transform_equal(backend.draws[0].transform, first_transform));
    TEST_CHECK(backend.draws[0].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(backend.draws[0].scissor_enabled);
    TEST_CHECK(rect_equal(backend.draws[0].scissor, first_clamped));
    TEST_CHECK(transform_equal(backend.draws[1].transform, second_transform));
    TEST_CHECK(backend.draws[1].blend == RENDERER_BLEND_ADDITIVE);
    TEST_CHECK(backend.draws[1].scissor_enabled);
    TEST_CHECK(rect_equal(backend.draws[1].scissor, second_clamped));

    TEST_CHECK(Renderer_set_scissor(renderer, &empty)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 3.0f, 3.0f, 1.0f, 1.0f, red)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);

    TEST_CHECK(Renderer_set_scissor(renderer, NULL)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 4.0f, 4.0f, 1.0f, 1.0f, red)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 3);
    TEST_CHECK(!backend.draws[2].scissor_enabled);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_begin_frame(renderer, 100, 80, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 5.0f, 5.0f, 1.0f, 1.0f, red)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 3);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 4);
    TEST_CHECK(transform_equal(backend.draws[3].transform, identity));
    TEST_CHECK(backend.draws[3].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(!backend.draws[3].scissor_enabled);

    Renderer_destroy(renderer);
    return 0;
}

static int check_texture_sampling_descriptor_contract(void)
{
    const uint8_t pixels[] = {
        10, 20, 30, 40,
        50, 60, 70, 80
    };
    const RendererTextureDesc desc = {
        .width = 2,
        .height = 1,
        .filter = RENDERER_TEXTURE_FILTER_LINEAR,
        .wrap = RENDERER_TEXTURE_WRAP_REPEAT
    };
    RendererTextureDesc invalid_desc = desc;
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = (RendererTexture *)(uintptr_t)1;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, NULL, pixels, sizeof(pixels), &texture)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture == NULL);
    TEST_CHECK(backend.texture_create_count == 0);

    invalid_desc.filter = (RendererTextureFilter)-1;
    texture = (RendererTexture *)(uintptr_t)1;
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &invalid_desc, pixels, sizeof(pixels), &texture)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture == NULL);
    TEST_CHECK(backend.texture_create_count == 0);

    invalid_desc = desc;
    invalid_desc.wrap = (RendererTextureWrap)-1;
    texture = (RendererTexture *)(uintptr_t)1;
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &invalid_desc, pixels, sizeof(pixels), &texture)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture == NULL);
    TEST_CHECK(backend.texture_create_count == 0);

    invalid_desc = desc;
    invalid_desc.width = 1;
    invalid_desc.height = 2;
    texture = (RendererTexture *)(uintptr_t)1;
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &invalid_desc, pixels, SIZE_MAX, &texture)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(texture == NULL);
    TEST_CHECK(backend.texture_create_count == 0);

    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &desc, pixels, sizeof(pixels), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(texture != NULL);
    TEST_CHECK(backend.texture_create_count == 1);
    TEST_CHECK(backend.texture_desc.width == desc.width);
    TEST_CHECK(backend.texture_desc.height == desc.height);
    TEST_CHECK(backend.texture_desc.filter == desc.filter);
    TEST_CHECK(backend.texture_desc.wrap == desc.wrap);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);

    Renderer_destroy(renderer);
    return 0;
}

static int check_resource_ownership(void)
{
    const RendererColor white = {255, 255, 255, 255};
    const RendererColor black = {0, 0, 0, 255};
    const RendererRect pixel_region = {0, 0, 1, 1};
    const uint8_t pixel[] = {10, 20, 30, 40};
    const RendererVertex2D triangle[] = {
        {0.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {1.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {0.0f, 1.0f, 0.0f, 0.0f, {255, 255, 255, 255}}
    };
    fake_backend_t first_backend;
    fake_backend_t second_backend;
    Renderer *first = create_renderer(&first_backend);
    Renderer *second = create_renderer(&second_backend);
    RendererTexture *texture = NULL;
    RendererMesh *mesh = NULL;
    RendererMesh *second_mesh = NULL;

    TEST_CHECK(first != NULL);
    TEST_CHECK(second != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   first, &single_pixel_texture_desc, pixel, sizeof(pixel),
                   &texture) == RENDERER_STATUS_OK);
    TEST_CHECK(texture != NULL);
    TEST_CHECK(Renderer_mesh_create(first, triangle, 3, &mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(mesh != NULL);
    TEST_CHECK(Renderer_mesh_create(second, triangle, 3, &second_mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(second_mesh != NULL);

    TEST_CHECK(Renderer_begin_frame(second, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_sprite(second, texture,
                                    0.0f, 0.0f, 1.0f, 1.0f,
                                    0.0f, 0.0f, 1.0f, 1.0f, white)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_draw_triangles(second, texture, triangle, 3)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_draw_mesh(second, texture, second_mesh)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_draw_mesh(second, NULL, mesh)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(second_backend.draw_count == 0);
    TEST_CHECK(Renderer_end_frame(second) == RENDERER_STATUS_OK);
    TEST_CHECK(second_backend.draw_attempt_count == 0);
    TEST_CHECK(second_backend.draw_count == 0);

    TEST_CHECK(Renderer_texture_update(second, texture, pixel_region,
                                       pixel, sizeof(pixel))
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_mesh_update(second, mesh, triangle, 3)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_texture_destroy(second, texture)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(Renderer_mesh_destroy(second, mesh)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(second_backend.texture_update_count == 0);
    TEST_CHECK(second_backend.mesh_update_count == 0);
    TEST_CHECK(second_backend.texture_destroy_count == 0);
    TEST_CHECK(second_backend.mesh_destroy_count == 0);

    TEST_CHECK(Renderer_texture_update(first, texture, pixel_region,
                                       pixel, sizeof(pixel))
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_update(first, mesh, triangle, 3)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_begin_frame(first, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_sprite(first, texture,
                                    0.0f, 0.0f, 1.0f, 1.0f,
                                    0.0f, 0.0f, 1.0f, 1.0f, white)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_mesh(first, NULL, mesh) == RENDERER_STATUS_OK);
    TEST_CHECK(first_backend.draw_count == 0);
    TEST_CHECK(Renderer_end_frame(first) == RENDERER_STATUS_OK);
    TEST_CHECK(first_backend.draw_count == 2);
    TEST_CHECK(first_backend.draws[0].texture
               == &first_backend.texture_token);
    TEST_CHECK(first_backend.draws[1].texture == NULL);
    TEST_CHECK(first_backend.draws[1].mesh == &first_backend.mesh_token);

    TEST_CHECK(Renderer_texture_destroy(first, texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_destroy(first, mesh) == RENDERER_STATUS_OK);
    TEST_CHECK(first_backend.texture_create_count == 1);
    TEST_CHECK(first_backend.texture_update_count == 1);
    TEST_CHECK(first_backend.texture_destroy_count == 1);
    TEST_CHECK(first_backend.mesh_create_count == 1);
    TEST_CHECK(first_backend.mesh_update_count == 1);
    TEST_CHECK(first_backend.mesh_destroy_count == 1);
    TEST_CHECK(Renderer_mesh_destroy(second, second_mesh)
               == RENDERER_STATUS_OK);

    Renderer_destroy(second);
    Renderer_destroy(first);
    return 0;
}

static int check_active_frame_rejects_resource_changes(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererRect pixel_region = {0, 0, 1, 1};
    const uint8_t pixel[] = {10, 20, 30, 40};
    const RendererVertex2D triangle[] = {
        {0.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {1.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {0.0f, 1.0f, 0.0f, 0.0f, {255, 255, 255, 255}}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = NULL;
    RendererTexture *new_texture = NULL;
    RendererMesh *mesh = NULL;
    RendererMesh *new_mesh = NULL;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_create(renderer, triangle, 3, &mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &new_texture)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_texture_update(renderer, texture, pixel_region,
                                       pixel, sizeof(pixel))
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_mesh_create(renderer, triangle, 3, &new_mesh)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_mesh_update(renderer, mesh, triangle, 3)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_mesh_destroy(renderer, mesh)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(backend.texture_create_count == 1);
    TEST_CHECK(backend.texture_update_count == 0);
    TEST_CHECK(backend.texture_destroy_count == 0);
    TEST_CHECK(backend.mesh_create_count == 1);
    TEST_CHECK(backend.mesh_update_count == 0);
    TEST_CHECK(backend.mesh_destroy_count == 0);

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_destroy(renderer, mesh) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_destroy_discards_active_frame(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    const uint8_t pixel[] = {10, 20, 30, 40};
    const RendererVertex2D triangle[] = {
        {0.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {1.0f, 0.0f, 0.0f, 0.0f, {255, 255, 255, 255}},
        {0.0f, 1.0f, 0.0f, 0.0f, {255, 255, 255, 255}}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = NULL;
    RendererMesh *mesh = NULL;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_create(renderer, triangle, 3, &mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_sprite(renderer, texture,
                                    0.0f, 0.0f, 1.0f, 1.0f,
                                    0.0f, 0.0f, 1.0f, 1.0f, white)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_mesh(renderer, texture, mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);

    Renderer_destroy(renderer);

    TEST_CHECK(backend.draw_attempt_count == 0);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(backend.texture_destroy_count == 1);
    TEST_CHECK(backend.mesh_destroy_count == 1);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_draw_failure_retries_from_undelivered_command(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 1.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 2.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 3.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);

    backend.fail_draw_attempt = 2;
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.draws[0].vertices[0].x == 1.0f);
    TEST_CHECK(backend.flush_count == 0);

    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_fill_rect(renderer, 4.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.flush_count == 0);

    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 4);
    TEST_CHECK(backend.draw_count == 3);
    TEST_CHECK(backend.draws[0].vertices[0].x == 1.0f);
    TEST_CHECK(backend.draws[1].vertices[0].x == 2.0f);
    TEST_CHECK(backend.draws[2].vertices[0].x == 3.0f);
    TEST_CHECK(backend.draws[0].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(backend.draws[1].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(backend.draws[2].blend == RENDERER_BLEND_OPAQUE);
    TEST_CHECK(backend.flush_count == 1);

    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 4.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 4);
    TEST_CHECK(backend.draw_count == 3);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 5);
    TEST_CHECK(backend.draw_count == 4);
    TEST_CHECK(backend.draws[3].vertices[0].x == 4.0f);
    TEST_CHECK(backend.draws[3].blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(backend.flush_count == 2);

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 5);
    TEST_CHECK(backend.draw_count == 4);

    Renderer_destroy(renderer);
    return 0;
}

static int check_textured_triangle_state_and_retry(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererTransform2D transform = {
        {2.0f, 0.0f, 0.0f,
         0.0f, 3.0f, 0.0f,
         4.0f, 5.0f, 1.0f}
    };
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    const RendererRect scissor = {2, 3, 14, 12};
    const uint8_t pixel[] = {20, 40, 60, 80};
    RendererVertex2D vertices[] = {
        {1.0f, 2.0f, 0.125f, 0.25f, {10, 20, 30, 40}},
        {8.0f, 2.0f, 0.75f, 0.25f, {50, 60, 70, 80}},
        {1.0f, 9.0f, 0.125f, 0.875f, {90, 100, 110, 120}}
    };
    RendererVertex2D expected_vertices[3];
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = NULL;
    const captured_draw_t *draw;

    memcpy(expected_vertices, vertices, sizeof(expected_vertices));
    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_transform_2d(renderer, transform)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, &scissor)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_triangles(renderer, texture, vertices, 3)
               == RENDERER_STATUS_OK);

    vertices[0].x = 99.0f;
    vertices[0].u = 0.0f;
    vertices[0].color.red = 255;
    TEST_CHECK(Renderer_set_transform_2d(renderer, identity)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ADDITIVE)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, NULL)
               == RENDERER_STATUS_OK);

    backend.fail_draw_attempt = 1;
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(backend.draw_attempt_count == 1);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(backend.flush_count == 0);

    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.flush_count == 1);
    draw = &backend.draws[0];
    TEST_CHECK(draw->texture == &backend.texture_token);
    TEST_CHECK(draw->mesh == NULL);
    TEST_CHECK(draw->vertex_count == 3);
    TEST_CHECK(memcmp(draw->vertices, expected_vertices,
                      sizeof(expected_vertices)) == 0);
    TEST_CHECK(transform_equal(draw->transform, transform));
    TEST_CHECK(draw->blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(draw->scissor_enabled);
    TEST_CHECK(rect_equal(draw->scissor, scissor));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_textured_mesh_state_and_retry(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererTransform2D transform = {
        {2.0f, 0.0f, 0.0f,
         0.0f, 3.0f, 0.0f,
         4.0f, 5.0f, 1.0f}
    };
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    const RendererRect scissor = {2, 3, 14, 12};
    const uint8_t pixel[] = {20, 40, 60, 80};
    const RendererVertex2D vertices[] = {
        {1.0f, 2.0f, 0.125f, 0.25f, {10, 20, 30, 40}},
        {8.0f, 2.0f, 0.75f, 0.25f, {50, 60, 70, 80}},
        {1.0f, 9.0f, 0.125f, 0.875f, {90, 100, 110, 120}}
    };
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);
    RendererTexture *texture = NULL;
    RendererMesh *mesh = NULL;
    const captured_draw_t *draw;

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &single_pixel_texture_desc, pixel,
                   sizeof(pixel), &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_create(renderer, vertices, 3, &mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_transform_2d(renderer, transform)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, &scissor)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_mesh(renderer, texture, mesh)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Renderer_set_transform_2d(renderer, identity)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_blend(renderer, RENDERER_BLEND_ADDITIVE)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_set_scissor(renderer, NULL)
               == RENDERER_STATUS_OK);

    backend.fail_draw_attempt = 1;
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(backend.draw_attempt_count == 1);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(backend.flush_count == 0);

    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.flush_count == 1);
    draw = &backend.draws[0];
    TEST_CHECK(draw->texture == &backend.texture_token);
    TEST_CHECK(draw->mesh == &backend.mesh_token);
    TEST_CHECK(draw->vertex_count == 0);
    TEST_CHECK(transform_equal(draw->transform, transform));
    TEST_CHECK(draw->blend == RENDERER_BLEND_ALPHA);
    TEST_CHECK(draw->scissor_enabled);
    TEST_CHECK(rect_equal(draw->scissor, scissor));

    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_destroy(renderer, mesh) == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

static int check_flush_failure_does_not_redeliver_draws(void)
{
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    fake_backend_t backend;
    Renderer *renderer = create_renderer(&backend);

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_begin_frame(renderer, 32, 32, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 4.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 5.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);

    backend.fail_next_flush = 1;
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(backend.flush_count == 1);

    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(backend.flush_count == 2);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_attempt_count == 2);
    TEST_CHECK(backend.draw_count == 2);

    Renderer_destroy(renderer);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_color_conversion() == 0);
    TEST_CHECK(check_frame_lifecycle_and_default_state() == 0);
    TEST_CHECK(check_state_snapshot_and_scissor() == 0);
    TEST_CHECK(check_texture_sampling_descriptor_contract() == 0);
    TEST_CHECK(check_resource_ownership() == 0);
    TEST_CHECK(check_active_frame_rejects_resource_changes() == 0);
    TEST_CHECK(check_destroy_discards_active_frame() == 0);
    TEST_CHECK(check_draw_failure_retries_from_undelivered_command() == 0);
    TEST_CHECK(check_textured_triangle_state_and_retry() == 0);
    TEST_CHECK(check_textured_mesh_state_and_retry() == 0);
    TEST_CHECK(check_flush_failure_does_not_redeliver_draws() == 0);
    return 0;
}
