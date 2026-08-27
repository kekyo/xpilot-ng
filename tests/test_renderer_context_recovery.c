#include "test_helpers.h"

#include "renderer.h"
#include "renderer_backend.h"

#include <stdint.h>
#include <string.h>

#define MAX_CAPTURED_PIXELS 32
#define MAX_CAPTURED_VERTICES 6

typedef struct FakeResource {
    unsigned int generation;
} FakeResource;

typedef struct FakeBackend {
    unsigned int generation;
    int context_lost_count;
    int context_restore_count;
    int texture_restore_count;
    int mesh_restore_count;
    int draw_count;
    int fail_next_texture_restore;
    RendererTextureDesc restored_texture_desc;
    uint8_t restored_pixels[MAX_CAPTURED_PIXELS];
    size_t restored_pitch;
    RendererVertex2D restored_vertices[MAX_CAPTURED_VERTICES];
    size_t restored_vertex_count;
    void *draw_texture;
    void *draw_mesh;
    FakeResource texture;
    FakeResource mesh;
} FakeBackend;

static RendererStatus Fake_begin_frame(void *context, int width, int height,
                                       RendererColor clear_color)
{
    (void)context;
    (void)width;
    (void)height;
    (void)clear_color;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_draw(void *context,
                                const RendererBackendDraw *draw)
{
    FakeBackend *backend = context;
    FakeResource *texture = draw->texture;
    FakeResource *mesh = draw->mesh;

    if (texture == NULL || mesh == NULL
        || texture->generation != backend->generation
        || mesh->generation != backend->generation) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    backend->draw_count++;
    backend->draw_texture = draw->texture;
    backend->draw_mesh = draw->mesh;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_flush(void *context)
{
    (void)context;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_end_frame(void *context)
{
    (void)context;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_texture_create(void *context,
                                          const RendererTextureDesc *desc,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch, void **handle)
{
    FakeBackend *backend = context;

    (void)desc;
    (void)rgba_pixels;
    (void)pitch;
    backend->texture.generation = backend->generation;
    *handle = &backend->texture;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_texture_update(void *context, void *handle,
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

static void Fake_texture_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static RendererStatus Fake_mesh_create(void *context,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count, void **handle)
{
    FakeBackend *backend = context;

    (void)vertices;
    (void)vertex_count;
    backend->mesh.generation = backend->generation;
    *handle = &backend->mesh;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_mesh_update(void *context, void *handle,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    (void)context;
    (void)handle;
    (void)vertices;
    (void)vertex_count;
    return RENDERER_STATUS_OK;
}

static void Fake_mesh_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static void Fake_context_lost(void *context)
{
    FakeBackend *backend = context;

    backend->context_lost_count++;
}

static RendererStatus Fake_context_restore(void *context)
{
    FakeBackend *backend = context;

    backend->context_restore_count++;
    backend->generation++;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_texture_restore(
    void *context, void *handle, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch)
{
    FakeBackend *backend = context;
    FakeResource *texture = handle;
    size_t byte_count = (size_t)desc->height * pitch;

    backend->texture_restore_count++;
    if (backend->fail_next_texture_restore) {
        backend->fail_next_texture_restore = 0;
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (byte_count > sizeof(backend->restored_pixels))
        return RENDERER_STATUS_BACKEND_ERROR;
    backend->restored_texture_desc = *desc;
    backend->restored_pitch = pitch;
    memcpy(backend->restored_pixels, rgba_pixels, byte_count);
    texture->generation = backend->generation;
    return RENDERER_STATUS_OK;
}

static RendererStatus Fake_mesh_restore(void *context, void *handle,
                                        const RendererVertex2D *vertices,
                                        size_t vertex_count)
{
    FakeBackend *backend = context;
    FakeResource *mesh = handle;

    backend->mesh_restore_count++;
    if (vertex_count > MAX_CAPTURED_VERTICES)
        return RENDERER_STATUS_BACKEND_ERROR;
    memcpy(backend->restored_vertices, vertices,
           vertex_count * sizeof(*vertices));
    backend->restored_vertex_count = vertex_count;
    mesh->generation = backend->generation;
    return RENDERER_STATUS_OK;
}

static void Fake_destroy(void *context)
{
    (void)context;
}

static const RendererBackendInterface fake_backend_interface = {
    .begin_frame = Fake_begin_frame,
    .draw = Fake_draw,
    .flush = Fake_flush,
    .end_frame = Fake_end_frame,
    .texture_create = Fake_texture_create,
    .texture_update = Fake_texture_update,
    .texture_destroy = Fake_texture_destroy,
    .mesh_create = Fake_mesh_create,
    .mesh_update = Fake_mesh_update,
    .mesh_destroy = Fake_mesh_destroy,
    .context_lost = Fake_context_lost,
    .context_restore = Fake_context_restore,
    .texture_restore = Fake_texture_restore,
    .mesh_restore = Fake_mesh_restore,
    .destroy = Fake_destroy
};

static int Check_context_recovery_preserves_resources(void)
{
    const RendererTextureDesc texture_desc = {
        2, 2, RENDERER_TEXTURE_FILTER_LINEAR, RENDERER_TEXTURE_WRAP_REPEAT
    };
    const RendererRect right_column = {1, 0, 1, 2};
    const RendererColor black = {0, 0, 0, 255};
    uint8_t pixels[] = {
         1,  2,  3,  4,  5,  6,  7,  8, 99, 99, 99, 99,
         9, 10, 11, 12, 13, 14, 15, 16, 99, 99, 99, 99
    };
    uint8_t update[] = {
        21, 22, 23, 24, 99, 99, 99, 99,
        25, 26, 27, 28, 99, 99, 99, 99
    };
    const uint8_t expected_pixels[] = {
         1,  2,  3,  4, 21, 22, 23, 24,
         9, 10, 11, 12, 25, 26, 27, 28
    };
    RendererVertex2D initial_vertices[] = {
        {0.0f, 0.0f, 0.0f, 0.0f, {255, 0, 0, 255}},
        {2.0f, 0.0f, 1.0f, 0.0f, {0, 255, 0, 255}},
        {0.0f, 2.0f, 0.0f, 1.0f, {0, 0, 255, 255}}
    };
    RendererVertex2D replacement_vertices[] = {
        {1.0f, 1.0f, 0.0f, 0.0f, {10, 20, 30, 255}},
        {3.0f, 1.0f, 1.0f, 0.0f, {40, 50, 60, 255}},
        {1.0f, 3.0f, 0.0f, 1.0f, {70, 80, 90, 255}}
    };
    RendererVertex2D expected_vertices[3];
    FakeBackend backend;
    Renderer *renderer;
    RendererTexture *texture = NULL;
    RendererMesh *mesh = NULL;

    memset(&backend, 0, sizeof(backend));
    backend.generation = 1;
    renderer = Renderer_backend_create(&fake_backend_interface, &backend);
    memcpy(expected_vertices, replacement_vertices, sizeof(expected_vertices));

    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_restore_context(renderer)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_texture_create_with_desc(
                   renderer, &texture_desc, pixels, 12, &texture)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_update(renderer, texture, right_column,
                                       update, 8) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_create(renderer, initial_vertices, 3, &mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_mesh_update(renderer, mesh, replacement_vertices, 3)
               == RENDERER_STATUS_OK);

    memset(pixels, 0, sizeof(pixels));
    memset(update, 0, sizeof(update));
    memset(initial_vertices, 0, sizeof(initial_vertices));
    memset(replacement_vertices, 0, sizeof(replacement_vertices));

    TEST_CHECK(Renderer_begin_frame(renderer, 8, 8, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_mesh(renderer, texture, mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_notify_context_lost(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_notify_context_lost(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.context_lost_count == 1);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Renderer_flush(renderer) == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_begin_frame(renderer, 8, 8, black)
               == RENDERER_STATUS_INVALID_STATE);

    backend.fail_next_texture_restore = 1;
    TEST_CHECK(Renderer_restore_context(renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(Renderer_begin_frame(renderer, 8, 8, black)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Renderer_restore_context(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.context_restore_count == 2);
    TEST_CHECK(backend.texture_restore_count == 2);
    TEST_CHECK(backend.mesh_restore_count == 1);
    TEST_CHECK(backend.restored_texture_desc.width == texture_desc.width);
    TEST_CHECK(backend.restored_texture_desc.height == texture_desc.height);
    TEST_CHECK(backend.restored_texture_desc.filter == texture_desc.filter);
    TEST_CHECK(backend.restored_texture_desc.wrap == texture_desc.wrap);
    TEST_CHECK(backend.restored_pitch == 8);
    TEST_CHECK(memcmp(backend.restored_pixels, expected_pixels,
                      sizeof(expected_pixels)) == 0);
    TEST_CHECK(backend.restored_vertex_count == 3);
    TEST_CHECK(memcmp(backend.restored_vertices, expected_vertices,
                      sizeof(expected_vertices)) == 0);

    TEST_CHECK(Renderer_begin_frame(renderer, 8, 8, black)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_draw_mesh(renderer, texture, mesh)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.draw_texture == &backend.texture);
    TEST_CHECK(backend.draw_mesh == &backend.mesh);

    TEST_CHECK(Renderer_mesh_destroy(renderer, mesh) == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_texture_destroy(renderer, texture)
               == RENDERER_STATUS_OK);
    Renderer_destroy(renderer);
    return 0;
}

int main(void)
{
    TEST_CHECK(Renderer_notify_context_lost(NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Renderer_restore_context(NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Check_context_recovery_preserves_resources() == 0);
    return 0;
}
