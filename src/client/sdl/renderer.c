/*
 * Backend-neutral renderer command frontend.
 *
 * This file deliberately contains no SDL or OpenGL dependency. It owns frame
 * state, command lifetime, geometry lowering, and resource ownership while a
 * selected backend owns graphics API objects.
 */

#include "renderer.h"
#include "renderer_backend.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct RendererCommand RendererCommand;

struct RendererTexture {
    Renderer *owner;
    void *handle;
    int width;
    int height;
    RendererTexture *next;
};

struct RendererMesh {
    Renderer *owner;
    void *handle;
    RendererMesh *next;
};

struct RendererCommand {
    RendererBackendDraw draw;
    RendererVertex2D *vertices;
    RendererCommand *next;
};

struct Renderer {
    const RendererBackendInterface *backend;
    void *backend_context;
    int frame_active;
    int frame_width;
    int frame_height;
    RendererTransform2D transform;
    RendererBlendMode blend;
    int scissor_enabled;
    int scissor_empty;
    RendererRect scissor;
    RendererCommand *commands;
    RendererCommand *last_command;
    RendererCommand *next_undelivered;
    int flush_pending;
    RendererTexture *textures;
    RendererMesh *meshes;
};

static const RendererTransform2D identity_transform = {
    {1.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 1.0f}
};

static int Backend_interface_valid(const RendererBackendInterface *interface)
{
    return interface != NULL
        && interface->begin_frame != NULL
        && interface->draw != NULL
        && interface->flush != NULL
        && interface->end_frame != NULL
        && interface->texture_create != NULL
        && interface->texture_update != NULL
        && interface->texture_destroy != NULL
        && interface->mesh_create != NULL
        && interface->mesh_update != NULL
        && interface->mesh_destroy != NULL
        && interface->destroy != NULL;
}

static int Transform_valid(RendererTransform2D transform)
{
    size_t index;

    for (index = 0; index < sizeof(transform.m) / sizeof(transform.m[0]);
         index++) {
        if (!isfinite(transform.m[index]))
            return 0;
    }
    return transform.m[2] == 0.0f
        && transform.m[5] == 0.0f
        && transform.m[8] == 1.0f;
}

static RendererPoint2D Transform_point(RendererTransform2D transform,
                                       RendererPoint2D point)
{
    RendererPoint2D transformed;

    transformed.x = transform.m[0] * point.x
        + transform.m[3] * point.y + transform.m[6];
    transformed.y = transform.m[1] * point.x
        + transform.m[4] * point.y + transform.m[7];
    return transformed;
}

static int Float_rect_valid(float left, float top, float right, float bottom)
{
    return isfinite(left) && isfinite(top)
        && isfinite(right) && isfinite(bottom)
        && right > left && bottom > top;
}

static int Vertex_count_valid(size_t vertex_count)
{
    return vertex_count != 0 && vertex_count % 3 == 0
        && vertex_count <= SIZE_MAX / sizeof(RendererVertex2D);
}

static int Pixel_data_valid(int width, int height, size_t pitch)
{
    size_t row_bytes;

    if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / 4)
        return 0;
    row_bytes = (size_t)width * 4;
    return pitch >= row_bytes
        && (height == 1
            || pitch <= (SIZE_MAX - row_bytes) / (size_t)(height - 1));
}

static void Free_commands(Renderer *renderer)
{
    RendererCommand *command = renderer->commands;

    while (command != NULL) {
        RendererCommand *next = command->next;

        free(command->vertices);
        free(command);
        command = next;
    }
    renderer->commands = NULL;
    renderer->last_command = NULL;
    renderer->next_undelivered = NULL;
    renderer->flush_pending = 0;
}

static RendererStatus Check_draw_state(const Renderer *renderer)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active || renderer->flush_pending)
        return RENDERER_STATUS_INVALID_STATE;
    return RENDERER_STATUS_OK;
}

static RendererStatus Queue_command(Renderer *renderer,
                                    RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    void *texture, void *mesh,
                                    RendererTransform2D transform)
{
    RendererCommand *command;

    if (renderer->scissor_enabled && renderer->scissor_empty) {
        free(vertices);
        return RENDERER_STATUS_OK;
    }

    command = calloc(1, sizeof(*command));
    if (command == NULL) {
        free(vertices);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    command->vertices = vertices;
    command->draw.transform = transform;
    command->draw.blend = renderer->blend;
    command->draw.scissor_enabled = renderer->scissor_enabled;
    command->draw.scissor = renderer->scissor;
    command->draw.texture = texture;
    command->draw.mesh = mesh;
    command->draw.vertices = vertices;
    command->draw.vertex_count = vertex_count;

    if (renderer->last_command != NULL)
        renderer->last_command->next = command;
    else
        renderer->commands = command;
    renderer->last_command = command;
    if (renderer->next_undelivered == NULL)
        renderer->next_undelivered = command;
    return RENDERER_STATUS_OK;
}

static RendererVertex2D *Copy_vertices(const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    RendererVertex2D *copy;

    if (!Vertex_count_valid(vertex_count) || vertices == NULL)
        return NULL;
    copy = malloc(vertex_count * sizeof(*copy));
    if (copy != NULL)
        memcpy(copy, vertices, vertex_count * sizeof(*copy));
    return copy;
}

static void Set_vertex(RendererVertex2D *vertex, float x, float y,
                       float u, float v, RendererColor color)
{
    vertex->x = x;
    vertex->y = y;
    vertex->u = u;
    vertex->v = v;
    vertex->color = color;
}

static RendererStatus Queue_quad(Renderer *renderer,
                                 float left, float top,
                                 float right, float bottom,
                                 float u0, float v0, float u1, float v1,
                                 RendererColor color, void *texture,
                                 RendererTransform2D transform)
{
    RendererVertex2D *vertices = malloc(6 * sizeof(*vertices));

    if (vertices == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    Set_vertex(&vertices[0], left, top, u0, v0, color);
    Set_vertex(&vertices[1], right, top, u1, v0, color);
    Set_vertex(&vertices[2], right, bottom, u1, v1, color);
    Set_vertex(&vertices[3], left, top, u0, v0, color);
    Set_vertex(&vertices[4], right, bottom, u1, v1, color);
    Set_vertex(&vertices[5], left, bottom, u0, v1, color);
    return Queue_command(renderer, vertices, 6, texture, NULL, transform);
}

static int Texture_is_owned(const Renderer *renderer,
                            const RendererTexture *texture)
{
    const RendererTexture *candidate;

    if (texture == NULL || texture->owner != renderer)
        return 0;
    for (candidate = renderer->textures; candidate != NULL;
         candidate = candidate->next) {
        if (candidate == texture)
            return 1;
    }
    return 0;
}

static int Mesh_is_owned(const Renderer *renderer, const RendererMesh *mesh)
{
    const RendererMesh *candidate;

    if (mesh == NULL || mesh->owner != renderer)
        return 0;
    for (candidate = renderer->meshes; candidate != NULL;
         candidate = candidate->next) {
        if (candidate == mesh)
            return 1;
    }
    return 0;
}

Renderer *Renderer_backend_create(const RendererBackendInterface *interface,
                                  void *context)
{
    Renderer *renderer;

    if (!Backend_interface_valid(interface))
        return NULL;
    renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL)
        return NULL;
    renderer->backend = interface;
    renderer->backend_context = context;
    renderer->transform = identity_transform;
    renderer->blend = RENDERER_BLEND_OPAQUE;
    return renderer;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color;

    color.red = (uint8_t)(rgba >> 24);
    color.green = (uint8_t)(rgba >> 16);
    color.blue = (uint8_t)(rgba >> 8);
    color.alpha = (uint8_t)rgba;
    return color;
}

uint32_t Renderer_color_to_rgba32(RendererColor color)
{
    return ((uint32_t)color.red << 24)
        | ((uint32_t)color.green << 16)
        | ((uint32_t)color.blue << 8)
        | (uint32_t)color.alpha;
}

void Renderer_destroy(Renderer *renderer)
{
    RendererTexture *texture;
    RendererMesh *mesh;

    if (renderer == NULL)
        return;
    Free_commands(renderer);
    texture = renderer->textures;
    while (texture != NULL) {
        RendererTexture *next = texture->next;

        renderer->backend->texture_destroy(renderer->backend_context,
                                           texture->handle);
        free(texture);
        texture = next;
    }
    mesh = renderer->meshes;
    while (mesh != NULL) {
        RendererMesh *next = mesh->next;

        renderer->backend->mesh_destroy(renderer->backend_context,
                                        mesh->handle);
        free(mesh);
        mesh = next;
    }
    renderer->backend->destroy(renderer->backend_context);
    free(renderer);
}

RendererStatus Renderer_begin_frame(Renderer *renderer, int width, int height,
                                    RendererColor clear_color)
{
    RendererStatus status;

    if (renderer == NULL || width <= 0 || height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    status = renderer->backend->begin_frame(renderer->backend_context,
                                            width, height, clear_color);
    if (status != RENDERER_STATUS_OK)
        return status;
    renderer->frame_active = 1;
    renderer->frame_width = width;
    renderer->frame_height = height;
    renderer->transform = identity_transform;
    renderer->blend = RENDERER_BLEND_OPAQUE;
    renderer->scissor_enabled = 0;
    renderer->scissor_empty = 0;
    memset(&renderer->scissor, 0, sizeof(renderer->scissor));
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_flush(Renderer *renderer)
{
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->commands == NULL && !renderer->flush_pending)
        return RENDERER_STATUS_OK;

    while (renderer->next_undelivered != NULL) {
        RendererCommand *command = renderer->next_undelivered;

        status = renderer->backend->draw(renderer->backend_context,
                                         &command->draw);
        if (status != RENDERER_STATUS_OK) {
            renderer->flush_pending = 1;
            return status;
        }
        renderer->next_undelivered = command->next;
    }

    status = renderer->backend->flush(renderer->backend_context);
    if (status != RENDERER_STATUS_OK) {
        renderer->flush_pending = 1;
        return status;
    }
    Free_commands(renderer);
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_end_frame(Renderer *renderer)
{
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->commands != NULL || renderer->flush_pending) {
        status = Renderer_flush(renderer);
        if (status != RENDERER_STATUS_OK)
            return status;
    }
    status = renderer->backend->end_frame(renderer->backend_context);
    if (status == RENDERER_STATUS_OK) {
        renderer->frame_active = 0;
        renderer->frame_width = 0;
        renderer->frame_height = 0;
    }
    return status;
}

RendererStatus Renderer_set_transform_2d(Renderer *renderer,
                                         RendererTransform2D transform)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (!Transform_valid(transform))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    renderer->transform = transform;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (blend != RENDERER_BLEND_OPAQUE
        && blend != RENDERER_BLEND_ALPHA
        && blend != RENDERER_BLEND_ADDITIVE) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    renderer->blend = blend;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_set_scissor(Renderer *renderer,
                                    const RendererRect *scissor)
{
    RendererStatus status = Check_draw_state(renderer);
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (scissor == NULL) {
        renderer->scissor_enabled = 0;
        renderer->scissor_empty = 0;
        memset(&renderer->scissor, 0, sizeof(renderer->scissor));
        return RENDERER_STATUS_OK;
    }
    if (scissor->width <= 0 || scissor->height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;

    left = scissor->x;
    top = scissor->y;
    right = left + (int64_t)scissor->width;
    bottom = top + (int64_t)scissor->height;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    if (right > renderer->frame_width)
        right = renderer->frame_width;
    if (bottom > renderer->frame_height)
        bottom = renderer->frame_height;

    renderer->scissor_enabled = 1;
    renderer->scissor_empty = right <= left || bottom <= top;
    if (renderer->scissor_empty) {
        memset(&renderer->scissor, 0, sizeof(renderer->scissor));
    } else {
        renderer->scissor.x = (int)left;
        renderer->scissor.y = (int)top;
        renderer->scissor.width = (int)(right - left);
        renderer->scissor.height = (int)(bottom - top);
    }
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_fill_rect(Renderer *renderer, float x, float y,
                                  float width, float height,
                                  RendererColor color)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (!Float_rect_valid(x, y, x + width, y + height))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return Queue_quad(renderer, x, y, x + width, y + height,
                      0.0f, 0.0f, 0.0f, 0.0f, color, NULL,
                      renderer->transform);
}

RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *copy;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    copy = Copy_vertices(vertices, vertex_count);
    if (copy == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    return Queue_command(renderer, copy, vertex_count, NULL, NULL,
                         renderer->transform);
}

RendererStatus Renderer_stroke_path(Renderer *renderer,
                                    const RendererPoint2D *points,
                                    size_t point_count, float width,
                                    RendererColor color, int closed)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *vertices;
    size_t maximum_segments;
    size_t segment;
    size_t vertex_count = 0;
    float half_width;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (points == NULL || point_count < 2 || !isfinite(width) || width <= 0.0f)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    for (segment = 0; segment < point_count; segment++) {
        if (!isfinite(points[segment].x) || !isfinite(points[segment].y))
            return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    maximum_segments = point_count - 1 + (closed != 0);
    if (maximum_segments > SIZE_MAX / 6
        || maximum_segments * 6 > SIZE_MAX / sizeof(*vertices)) {
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    vertices = malloc(maximum_segments * 6 * sizeof(*vertices));
    if (vertices == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    half_width = width * 0.5f;

    for (segment = 0; segment < maximum_segments; segment++) {
        size_t first = segment;
        size_t second = segment + 1;
        RendererPoint2D first_point;
        RendererPoint2D second_point;
        float dx;
        float dy;
        float length;
        float normal_x;
        float normal_y;

        if (second == point_count)
            second = 0;
        first_point = Transform_point(renderer->transform, points[first]);
        second_point = Transform_point(renderer->transform, points[second]);
        if (!isfinite(first_point.x) || !isfinite(first_point.y)
            || !isfinite(second_point.x) || !isfinite(second_point.y)) {
            free(vertices);
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        dx = second_point.x - first_point.x;
        dy = second_point.y - first_point.y;
        if (dx == 0.0f && dy == 0.0f)
            continue;
        length = hypotf(dx, dy);
        if (!isfinite(length) || length <= 0.0f) {
            free(vertices);
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        normal_x = -dy * half_width / length;
        normal_y = dx * half_width / length;
        Set_vertex(&vertices[vertex_count++],
                   first_point.x - normal_x,
                   first_point.y - normal_y, 0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_count++],
                   second_point.x - normal_x,
                   second_point.y - normal_y, 0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_count++],
                   second_point.x + normal_x,
                   second_point.y + normal_y, 0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_count++],
                   first_point.x - normal_x,
                   first_point.y - normal_y, 0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_count++],
                   second_point.x + normal_x,
                   second_point.y + normal_y, 0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_count++],
                   first_point.x + normal_x,
                   first_point.y + normal_y, 0.0f, 0.0f, color);
    }
    if (vertex_count == 0) {
        free(vertices);
        return RENDERER_STATUS_OK;
    }
    return Queue_command(renderer, vertices, vertex_count, NULL, NULL,
                         identity_transform);
}

RendererStatus Renderer_draw_point(Renderer *renderer, float x, float y,
                                   float size, RendererColor color)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererPoint2D center;
    float half_size;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (!isfinite(x) || !isfinite(y) || !isfinite(size) || size <= 0.0f)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    half_size = size * 0.5f;
    if (!isfinite(half_size))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    center.x = x;
    center.y = y;
    center = Transform_point(renderer->transform, center);
    if (!isfinite(center.x) || !isfinite(center.y))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return Queue_quad(renderer,
                      center.x - half_size, center.y - half_size,
                      center.x + half_size, center.y + half_size,
                      0.0f, 0.0f, 0.0f, 0.0f, color, NULL,
                      identity_transform);
}

RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (!Texture_is_owned(renderer, texture))
        return texture == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                               : RENDERER_STATUS_RESOURCE_MISMATCH;
    if (!Float_rect_valid(left, top, right, bottom)
        || !isfinite(u0) || !isfinite(v0)
        || !isfinite(u1) || !isfinite(v1)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return Queue_quad(renderer, left, top, right, bottom,
                      u0, v0, u1, v1, tint, texture->handle,
                      renderer->transform);
}

RendererStatus Renderer_draw_mesh(Renderer *renderer, RendererMesh *mesh)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (!Mesh_is_owned(renderer, mesh))
        return mesh == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                            : RENDERER_STATUS_RESOURCE_MISMATCH;
    return Queue_command(renderer, NULL, 0, NULL, mesh->handle,
                         renderer->transform);
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    RendererTexture *created;
    RendererStatus status;
    void *handle = NULL;

    if (texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *texture = NULL;
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (desc == NULL || desc->width <= 0 || desc->height <= 0
        || (desc->filter != RENDERER_TEXTURE_FILTER_NEAREST
            && desc->filter != RENDERER_TEXTURE_FILTER_LINEAR)
        || (desc->wrap != RENDERER_TEXTURE_WRAP_CLAMP
            && desc->wrap != RENDERER_TEXTURE_WRAP_REPEAT)
        || rgba_pixels == NULL
        || !Pixel_data_valid(desc->width, desc->height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    created = calloc(1, sizeof(*created));
    if (created == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    status = renderer->backend->texture_create(renderer->backend_context,
                                               desc, rgba_pixels, pitch,
                                               &handle);
    if (status != RENDERER_STATUS_OK || handle == NULL) {
        free(created);
        return status != RENDERER_STATUS_OK ? status
                                           : RENDERER_STATUS_BACKEND_ERROR;
    }
    created->owner = renderer;
    created->handle = handle;
    created->width = desc->width;
    created->height = desc->height;
    created->next = renderer->textures;
    renderer->textures = created;
    *texture = created;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *rgba_pixels,
                                       size_t pitch)
{
    int64_t right;
    int64_t bottom;

    if (renderer == NULL || texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Texture_is_owned(renderer, texture))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    right = (int64_t)region.x + region.width;
    bottom = (int64_t)region.y + region.height;
    if (region.x < 0 || region.y < 0
        || region.width <= 0 || region.height <= 0
        || right > texture->width || bottom > texture->height
        || rgba_pixels == NULL
        || !Pixel_data_valid(region.width, region.height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    return renderer->backend->texture_update(renderer->backend_context,
                                             texture->handle, region,
                                             rgba_pixels, pitch);
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    RendererTexture **link;

    if (renderer == NULL || texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Texture_is_owned(renderer, texture))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    for (link = &renderer->textures; *link != texture; link = &(*link)->next)
        ;
    *link = texture->next;
    renderer->backend->texture_destroy(renderer->backend_context,
                                       texture->handle);
    texture->owner = NULL;
    free(texture);
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_mesh_create(Renderer *renderer,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    RendererMesh **mesh)
{
    RendererMesh *created;
    RendererStatus status;
    void *handle = NULL;

    if (mesh == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *mesh = NULL;
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    created = calloc(1, sizeof(*created));
    if (created == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    status = renderer->backend->mesh_create(renderer->backend_context,
                                            vertices, vertex_count, &handle);
    if (status != RENDERER_STATUS_OK || handle == NULL) {
        free(created);
        return status != RENDERER_STATUS_OK ? status
                                           : RENDERER_STATUS_BACKEND_ERROR;
    }
    created->owner = renderer;
    created->handle = handle;
    created->next = renderer->meshes;
    renderer->meshes = created;
    *mesh = created;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_mesh_update(Renderer *renderer, RendererMesh *mesh,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count)
{
    if (renderer == NULL || mesh == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Mesh_is_owned(renderer, mesh))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return renderer->backend->mesh_update(renderer->backend_context,
                                          mesh->handle, vertices,
                                          vertex_count);
}

RendererStatus Renderer_mesh_destroy(Renderer *renderer, RendererMesh *mesh)
{
    RendererMesh **link;

    if (renderer == NULL || mesh == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Mesh_is_owned(renderer, mesh))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    for (link = &renderer->meshes; *link != mesh; link = &(*link)->next)
        ;
    *link = mesh->next;
    renderer->backend->mesh_destroy(renderer->backend_context, mesh->handle);
    mesh->owner = NULL;
    free(mesh);
    return RENDERER_STATUS_OK;
}
