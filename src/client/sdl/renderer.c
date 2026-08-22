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
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    RendererTexture *next;
};

struct RendererMesh {
    Renderer *owner;
    void *handle;
    RendererVertex2D *vertices;
    size_t vertex_count;
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
    int backend_context_available;
    int context_available;
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
    int recovery_callbacks_absent;
    int recovery_callbacks_present;

    if (interface == NULL)
        return 0;
    recovery_callbacks_absent = interface->context_lost == NULL
        && interface->context_restore == NULL
        && interface->texture_restore == NULL
        && interface->mesh_restore == NULL;
    recovery_callbacks_present = interface->context_lost != NULL
        && interface->context_restore != NULL
        && interface->texture_restore != NULL
        && interface->mesh_restore != NULL;
    return interface->begin_frame != NULL
        && interface->draw != NULL
        && interface->flush != NULL
        && interface->end_frame != NULL
        && interface->texture_create != NULL
        && interface->texture_update != NULL
        && interface->texture_destroy != NULL
        && interface->mesh_create != NULL
        && interface->mesh_update != NULL
        && interface->mesh_destroy != NULL
        && (recovery_callbacks_absent || recovery_callbacks_present)
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

static uint8_t *Copy_pixels(int width, int height,
                            const uint8_t *pixels, size_t pitch)
{
    size_t row_bytes = (size_t)width * 4;
    uint8_t *copy = malloc(row_bytes * (size_t)height);
    int row;

    if (copy == NULL)
        return NULL;
    for (row = 0; row < height; row++) {
        memcpy(copy + (size_t)row * row_bytes,
               pixels + (size_t)row * pitch, row_bytes);
    }
    return copy;
}

static void Copy_texture_region(RendererTexture *texture,
                                RendererRect region,
                                const uint8_t *pixels, size_t pitch)
{
    size_t row_bytes = (size_t)region.width * 4;
    int row;

    for (row = 0; row < region.height; row++) {
        memcpy(texture->pixels
                   + (size_t)(region.y + row) * texture->pitch
                   + (size_t)region.x * 4,
               pixels + (size_t)row * pitch, row_bytes);
    }
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
    renderer->backend_context_available = 1;
    renderer->context_available = 1;
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
        free(texture->pixels);
        free(texture);
        texture = next;
    }
    mesh = renderer->meshes;
    while (mesh != NULL) {
        RendererMesh *next = mesh->next;

        renderer->backend->mesh_destroy(renderer->backend_context,
                                        mesh->handle);
        free(mesh->vertices);
        free(mesh);
        mesh = next;
    }
    renderer->backend->destroy(renderer->backend_context);
    free(renderer);
}

RendererStatus Renderer_notify_context_lost(Renderer *renderer)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available
        && !renderer->backend_context_available) {
        return RENDERER_STATUS_OK;
    }
    if (renderer->backend->context_lost == NULL)
        return RENDERER_STATUS_BACKEND_ERROR;

    if (renderer->context_available) {
        Free_commands(renderer);
        renderer->frame_active = 0;
        renderer->frame_width = 0;
        renderer->frame_height = 0;
    }
    renderer->backend->context_lost(renderer->backend_context);
    renderer->backend_context_available = 0;
    renderer->context_available = 0;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_restore_context(Renderer *renderer)
{
    RendererTexture *texture;
    RendererMesh *mesh;
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->context_available)
        return RENDERER_STATUS_INVALID_STATE;

    status = renderer->backend->context_restore(renderer->backend_context);
    if (status != RENDERER_STATUS_OK) {
        renderer->backend_context_available = 0;
        return status;
    }
    renderer->backend_context_available = 1;
    for (texture = renderer->textures; texture != NULL;
         texture = texture->next) {
        status = renderer->backend->texture_restore(
            renderer->backend_context, texture->handle, &texture->desc,
            texture->pixels, texture->pitch);
        if (status != RENDERER_STATUS_OK)
            return status;
    }
    for (mesh = renderer->meshes; mesh != NULL; mesh = mesh->next) {
        status = renderer->backend->mesh_restore(
            renderer->backend_context, mesh->handle,
            mesh->vertices, mesh->vertex_count);
        if (status != RENDERER_STATUS_OK)
            return status;
    }
    renderer->context_available = 1;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_begin_frame(Renderer *renderer, int width, int height,
                                    RendererColor clear_color)
{
    RendererStatus status;

    if (renderer == NULL || width <= 0 || height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available || renderer->frame_active)
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
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *copy;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (texture != NULL && !Texture_is_owned(renderer, texture))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    copy = Copy_vertices(vertices, vertex_count);
    if (copy == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    return Queue_command(renderer, copy, vertex_count,
                         texture != NULL ? texture->handle : NULL, NULL,
                         renderer->transform);
}

static RendererStatus Queue_stroke_path(
    Renderer *renderer, const RendererPoint2D *points,
    const RendererColor *point_colors, size_t point_count, float width,
    const RendererColor *constant_color, int closed)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *vertices;
    size_t maximum_segments;
    size_t segment;
    size_t vertex_count = 0;
    float half_width;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (points == NULL || point_count < 2 || !isfinite(width)
        || width <= 0.0f
        || ((point_colors == NULL) == (constant_color == NULL))) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    maximum_segments = point_count - 1 + (closed != 0);
    if (maximum_segments > SIZE_MAX / 6
        || maximum_segments * 6 > SIZE_MAX / sizeof(*vertices)) {
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    for (segment = 0; segment < point_count; segment++) {
        if (!isfinite(points[segment].x) || !isfinite(points[segment].y))
            return RENDERER_STATUS_INVALID_ARGUMENT;
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
        float first_minus_x;
        float first_minus_y;
        float first_plus_x;
        float first_plus_y;
        float second_minus_x;
        float second_minus_y;
        float second_plus_x;
        float second_plus_y;
        RendererColor first_color;
        RendererColor second_color;

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
        normal_x = -(dy / length) * half_width;
        normal_y = (dx / length) * half_width;
        first_minus_x = first_point.x - normal_x;
        first_minus_y = first_point.y - normal_y;
        first_plus_x = first_point.x + normal_x;
        first_plus_y = first_point.y + normal_y;
        second_minus_x = second_point.x - normal_x;
        second_minus_y = second_point.y - normal_y;
        second_plus_x = second_point.x + normal_x;
        second_plus_y = second_point.y + normal_y;
        if (!isfinite(first_minus_x) || !isfinite(first_minus_y)
            || !isfinite(first_plus_x) || !isfinite(first_plus_y)
            || !isfinite(second_minus_x) || !isfinite(second_minus_y)
            || !isfinite(second_plus_x) || !isfinite(second_plus_y)) {
            free(vertices);
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        first_color = point_colors != NULL
            ? point_colors[first] : *constant_color;
        second_color = point_colors != NULL
            ? point_colors[second] : *constant_color;
        Set_vertex(&vertices[vertex_count++],
                   first_minus_x, first_minus_y, 0.0f, 0.0f, first_color);
        Set_vertex(&vertices[vertex_count++],
                   second_minus_x, second_minus_y, 0.0f, 0.0f, second_color);
        Set_vertex(&vertices[vertex_count++],
                   second_plus_x, second_plus_y, 0.0f, 0.0f, second_color);
        Set_vertex(&vertices[vertex_count++],
                   first_minus_x, first_minus_y, 0.0f, 0.0f, first_color);
        Set_vertex(&vertices[vertex_count++],
                   second_plus_x, second_plus_y, 0.0f, 0.0f, second_color);
        Set_vertex(&vertices[vertex_count++],
                   first_plus_x, first_plus_y, 0.0f, 0.0f, first_color);
    }
    if (vertex_count == 0) {
        free(vertices);
        return RENDERER_STATUS_OK;
    }
    return Queue_command(renderer, vertices, vertex_count, NULL, NULL,
                         identity_transform);
}

RendererStatus Renderer_stroke_path(Renderer *renderer,
                                    const RendererPoint2D *points,
                                    size_t point_count, float width,
                                    RendererColor color, int closed)
{
    return Queue_stroke_path(renderer, points, NULL, point_count, width,
                             &color, closed);
}

RendererStatus Renderer_stroke_colored_path(
    Renderer *renderer, const RendererPoint2D *points,
    const RendererColor *colors, size_t point_count, float width, int closed)
{
    return Queue_stroke_path(renderer, points, colors, point_count, width,
                             NULL, closed);
}

typedef struct StippleSegment {
    RendererPoint2D first;
    RendererPoint2D second;
    float dx;
    float dy;
    float length;
} StippleSegment;

static int Stipple_bit_enabled(uint16_t pattern, unsigned int bit)
{
    return (pattern & (uint16_t)(UINT16_C(1) << bit)) != 0;
}

static double Stipple_advance_phase(double phase, float length,
                                    double cycle_length)
{
    double length_phase = fmod((double)length, cycle_length);

    return fmod(phase + length_phase, cycle_length);
}

static RendererStatus Stipple_prepare_segment(
    RendererTransform2D transform, RendererPoint2D first,
    RendererPoint2D second, StippleSegment *segment)
{
    if (!isfinite(first.x) || !isfinite(first.y)
        || !isfinite(second.x) || !isfinite(second.y)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    segment->first = Transform_point(transform, first);
    segment->second = Transform_point(transform, second);
    if (!isfinite(segment->first.x) || !isfinite(segment->first.y)
        || !isfinite(segment->second.x) || !isfinite(segment->second.y)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    segment->dx = segment->second.x - segment->first.x;
    segment->dy = segment->second.y - segment->first.y;
    if (segment->dx == 0.0f && segment->dy == 0.0f) {
        segment->length = 0.0f;
        return RENDERER_STATUS_OK;
    }
    segment->length = hypotf(segment->dx, segment->dy);
    if (!isfinite(segment->length) || segment->length <= 0.0f)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return RENDERER_STATUS_OK;
}

static RendererStatus Stipple_count_segment_runs(
    uint16_t pattern, unsigned int factor, double phase, float length,
    size_t maximum_runs, size_t *run_count)
{
    double cycle_length = (double)factor * 16.0;
    unsigned int first_bit = (unsigned int)(phase / (double)factor);
    unsigned int bit;

    if (Stipple_bit_enabled(pattern, first_bit)) {
        if (*run_count == maximum_runs)
            return RENDERER_STATUS_OUT_OF_MEMORY;
        (*run_count)++;
    }

    for (bit = 0; bit < 16; bit++) {
        unsigned int previous_bit = (bit + 15) % 16;
        double boundary;
        double distance;
        double occurrences;

        if (!Stipple_bit_enabled(pattern, bit)
            || Stipple_bit_enabled(pattern, previous_bit)) {
            continue;
        }
        boundary = (double)bit * (double)factor;
        distance = boundary - phase;
        if (distance <= 0.0)
            distance += cycle_length;
        if (distance >= (double)length)
            continue;

        occurrences = ceil(((double)length - distance) / cycle_length);
        if (!isfinite(occurrences)
            || occurrences > (double)(maximum_runs - *run_count)) {
            return RENDERER_STATUS_OUT_OF_MEMORY;
        }
        *run_count += (size_t)occurrences;
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Stipple_count_path_runs(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, size_t segment_count, int closed,
    unsigned int factor, uint16_t pattern, size_t maximum_runs,
    size_t *run_count)
{
    double cycle_length = (double)factor * 16.0;
    double phase = 0.0;
    size_t segment_index;

    *run_count = 0;
    for (segment_index = 0; segment_index < segment_count;
         segment_index++) {
        size_t second_index = segment_index + 1;
        StippleSegment segment;
        RendererStatus status;

        if (second_index == point_count) {
            if (!closed)
                return RENDERER_STATUS_INVALID_ARGUMENT;
            second_index = 0;
        }
        status = Stipple_prepare_segment(renderer->transform,
                                         points[segment_index],
                                         points[second_index], &segment);
        if (status != RENDERER_STATUS_OK)
            return status;
        if (segment.length == 0.0f)
            continue;
        status = Stipple_count_segment_runs(
            pattern, factor, phase, segment.length,
            maximum_runs, run_count);
        if (status != RENDERER_STATUS_OK)
            return status;
        phase = Stipple_advance_phase(phase, segment.length, cycle_length);
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Stipple_append_run(
    const StippleSegment *segment, double start_distance,
    double end_distance, float half_width, RendererColor color,
    RendererVertex2D *vertices, size_t maximum_runs, size_t *run_count)
{
    RendererPoint2D start;
    RendererPoint2D end;
    float direction_x = segment->dx / segment->length;
    float direction_y = segment->dy / segment->length;
    float normal_x = -direction_y * half_width;
    float normal_y = direction_x * half_width;
    float start_minus_x;
    float start_minus_y;
    float start_plus_x;
    float start_plus_y;
    float end_minus_x;
    float end_minus_y;
    float end_plus_x;
    float end_plus_y;
    size_t vertex_index;

    if (*run_count >= maximum_runs
        || !isfinite(direction_x) || !isfinite(direction_y)
        || !isfinite(normal_x) || !isfinite(normal_y)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (start_distance == 0.0) {
        start = segment->first;
    } else {
        start.x = segment->first.x
            + direction_x * (float)start_distance;
        start.y = segment->first.y
            + direction_y * (float)start_distance;
    }
    if (end_distance == (double)segment->length) {
        end = segment->second;
    } else {
        end.x = segment->first.x + direction_x * (float)end_distance;
        end.y = segment->first.y + direction_y * (float)end_distance;
    }
    start_minus_x = start.x - normal_x;
    start_minus_y = start.y - normal_y;
    start_plus_x = start.x + normal_x;
    start_plus_y = start.y + normal_y;
    end_minus_x = end.x - normal_x;
    end_minus_y = end.y - normal_y;
    end_plus_x = end.x + normal_x;
    end_plus_y = end.y + normal_y;
    if (!isfinite(start.x) || !isfinite(start.y)
        || !isfinite(end.x) || !isfinite(end.y)
        || !isfinite(start_minus_x) || !isfinite(start_minus_y)
        || !isfinite(start_plus_x) || !isfinite(start_plus_y)
        || !isfinite(end_minus_x) || !isfinite(end_minus_y)
        || !isfinite(end_plus_x) || !isfinite(end_plus_y)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    if (vertices != NULL) {
        vertex_index = *run_count * 6;
        Set_vertex(&vertices[vertex_index++], start_minus_x, start_minus_y,
                   0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_index++], end_minus_x, end_minus_y,
                   0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_index++], end_plus_x, end_plus_y,
                   0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_index++], start_minus_x, start_minus_y,
                   0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_index++], end_plus_x, end_plus_y,
                   0.0f, 0.0f, color);
        Set_vertex(&vertices[vertex_index], start_plus_x, start_plus_y,
                   0.0f, 0.0f, color);
    }
    (*run_count)++;
    return RENDERER_STATUS_OK;
}

static RendererStatus Stipple_process_segment(
    const StippleSegment *segment, double phase, unsigned int factor,
    uint16_t pattern, float half_width, RendererColor color,
    RendererVertex2D *vertices, size_t maximum_runs, size_t *run_count)
{
    double cycle_length = (double)factor * 16.0;
    double cursor = 0.0;
    double segment_phase = phase;
    double run_start = 0.0;
    int run_active = 0;

    if (pattern == 0)
        return RENDERER_STATUS_OK;
    if (pattern == UINT16_MAX) {
        return Stipple_append_run(
            segment, 0.0, (double)segment->length, half_width, color,
            vertices, maximum_runs, run_count);
    }

    while (cursor < (double)segment->length) {
        unsigned int bit =
            (unsigned int)(segment_phase / (double)factor);
        double bit_end = (double)(bit + 1) * (double)factor;
        double to_boundary = bit_end - segment_phase;
        double remaining = (double)segment->length - cursor;
        int enabled = Stipple_bit_enabled(pattern, bit);

        if (!isfinite(to_boundary) || to_boundary <= 0.0)
            return RENDERER_STATUS_INVALID_ARGUMENT;
        if (enabled && !run_active) {
            run_start = cursor;
            run_active = 1;
        }

        if (to_boundary >= remaining) {
            cursor = (double)segment->length;
        } else {
            double next_cursor = cursor + to_boundary;

            if (next_cursor <= cursor)
                return RENDERER_STATUS_INVALID_ARGUMENT;
            cursor = next_cursor;
            segment_phase = bit_end;
            if (segment_phase >= cycle_length)
                segment_phase = 0.0;
        }

        if (run_active) {
            int run_finished = cursor == (double)segment->length;

            if (!run_finished) {
                unsigned int next_bit =
                    (unsigned int)(segment_phase / (double)factor);

                run_finished = !Stipple_bit_enabled(pattern, next_bit);
            }
            if (run_finished) {
                RendererStatus status = Stipple_append_run(
                    segment, run_start, cursor, half_width, color,
                    vertices, maximum_runs, run_count);

                if (status != RENDERER_STATUS_OK)
                    return status;
                run_active = 0;
            }
        }
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Stipple_process_path(
    Renderer *renderer, const RendererPoint2D *points,
    size_t point_count, size_t segment_count, int closed,
    float half_width, RendererColor color, unsigned int factor,
    uint16_t pattern, RendererVertex2D *vertices, size_t expected_runs)
{
    double cycle_length = (double)factor * 16.0;
    double phase = 0.0;
    size_t run_count = 0;
    size_t segment_index;

    for (segment_index = 0; segment_index < segment_count;
         segment_index++) {
        size_t second_index = segment_index + 1;
        StippleSegment segment;
        RendererStatus status;

        if (second_index == point_count) {
            if (!closed)
                return RENDERER_STATUS_INVALID_ARGUMENT;
            second_index = 0;
        }
        status = Stipple_prepare_segment(renderer->transform,
                                         points[segment_index],
                                         points[second_index], &segment);
        if (status != RENDERER_STATUS_OK)
            return status;
        if (segment.length == 0.0f)
            continue;
        status = Stipple_process_segment(
            &segment, phase, factor, pattern, half_width, color,
            vertices, expected_runs, &run_count);
        if (status != RENDERER_STATUS_OK)
            return status;
        phase = Stipple_advance_phase(phase, segment.length, cycle_length);
    }
    return run_count == expected_runs ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_ARGUMENT;
}

RendererStatus Renderer_stroke_stippled_path(
    Renderer *renderer, const RendererPoint2D *points, size_t point_count,
    float width, RendererColor color, int closed, unsigned int factor,
    uint16_t pattern)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *vertices;
    size_t maximum_vertex_count;
    size_t maximum_runs;
    size_t segment_count;
    size_t run_count;
    size_t vertex_count;
    float half_width;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (points == NULL || point_count < 2 || !isfinite(width)
        || width <= 0.0f || factor == 0 || factor > 256) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    maximum_vertex_count = SIZE_MAX / sizeof(*vertices);
    if (maximum_vertex_count > (size_t)INT_MAX)
        maximum_vertex_count = (size_t)INT_MAX;
    maximum_runs = maximum_vertex_count / 6;
    segment_count = point_count - 1 + (closed != 0);
    if (segment_count > maximum_runs)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    status = Stipple_count_path_runs(
        renderer, points, point_count, segment_count, closed != 0,
        factor, pattern, maximum_runs, &run_count);
    if (status != RENDERER_STATUS_OK)
        return status;
    if (run_count == 0)
        return RENDERER_STATUS_OK;

    half_width = width * 0.5f;
    status = Stipple_process_path(
        renderer, points, point_count, segment_count, closed != 0,
        half_width, color, factor, pattern, NULL, run_count);
    if (status != RENDERER_STATUS_OK)
        return status;

    vertex_count = run_count * 6;
    vertices = malloc(vertex_count * sizeof(*vertices));
    if (vertices == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    status = Stipple_process_path(
        renderer, points, point_count, segment_count, closed != 0,
        half_width, color, factor, pattern, vertices, run_count);
    if (status != RENDERER_STATUS_OK) {
        free(vertices);
        return status;
    }
    return Queue_command(renderer, vertices, vertex_count, NULL, NULL,
                         identity_transform);
}

RendererStatus Renderer_draw_colored_points(
    Renderer *renderer, const RendererColoredPoint2D *points,
    size_t point_count, float size)
{
    RendererStatus status = Check_draw_state(renderer);
    RendererVertex2D *vertices;
    size_t maximum_vertex_count;
    size_t vertex_count;
    size_t point_index;
    float half_size;

    if (status != RENDERER_STATUS_OK)
        return status;
    if (points == NULL || point_count == 0
        || !isfinite(size) || size <= 0.0f) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    half_size = size * 0.5f;
    if (!isfinite(half_size))
        return RENDERER_STATUS_INVALID_ARGUMENT;

    maximum_vertex_count = SIZE_MAX / sizeof(*vertices);
    if (maximum_vertex_count
        > (size_t)PTRDIFF_MAX / sizeof(*vertices)) {
        maximum_vertex_count = (size_t)PTRDIFF_MAX / sizeof(*vertices);
    }
    if (maximum_vertex_count > (size_t)INT_MAX)
        maximum_vertex_count = (size_t)INT_MAX;
    if (point_count > maximum_vertex_count / 6)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    /* Validate every transformed square before accepting any batch prefix. */
    for (point_index = 0; point_index < point_count; point_index++) {
        RendererPoint2D center = points[point_index].position;

        if (!isfinite(center.x) || !isfinite(center.y))
            return RENDERER_STATUS_INVALID_ARGUMENT;
        center = Transform_point(renderer->transform, center);
        if (!Float_rect_valid(
                center.x - half_size, center.y - half_size,
                center.x + half_size, center.y + half_size)) {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
    }

    vertex_count = point_count * 6;
    vertices = malloc(vertex_count * sizeof(*vertices));
    if (vertices == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    for (point_index = 0; point_index < point_count; point_index++) {
        const RendererColoredPoint2D *point = &points[point_index];
        RendererPoint2D center = Transform_point(
            renderer->transform, point->position);
        float left = center.x - half_size;
        float top = center.y - half_size;
        float right = center.x + half_size;
        float bottom = center.y + half_size;
        RendererVertex2D *quad = &vertices[point_index * 6];

        Set_vertex(&quad[0], left, top, 0.0f, 0.0f, point->color);
        Set_vertex(&quad[1], right, top, 0.0f, 0.0f, point->color);
        Set_vertex(&quad[2], right, bottom, 0.0f, 0.0f, point->color);
        Set_vertex(&quad[3], left, top, 0.0f, 0.0f, point->color);
        Set_vertex(&quad[4], right, bottom, 0.0f, 0.0f, point->color);
        Set_vertex(&quad[5], left, bottom, 0.0f, 0.0f, point->color);
    }
    return Queue_command(renderer, vertices, vertex_count, NULL, NULL,
                         identity_transform);
}

RendererStatus Renderer_draw_point(Renderer *renderer, float x, float y,
                                   float size, RendererColor color)
{
    RendererColoredPoint2D point;

    point.position.x = x;
    point.position.y = y;
    point.color = color;
    return Renderer_draw_colored_points(renderer, &point, 1, size);
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

RendererStatus Renderer_draw_mesh(Renderer *renderer,
                                  RendererTexture *texture,
                                  RendererMesh *mesh)
{
    RendererStatus status = Check_draw_state(renderer);

    if (status != RENDERER_STATUS_OK)
        return status;
    if (texture != NULL && !Texture_is_owned(renderer, texture))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (!Mesh_is_owned(renderer, mesh))
        return mesh == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                            : RENDERER_STATUS_RESOURCE_MISMATCH;
    return Queue_command(renderer, NULL, 0,
                         texture != NULL ? texture->handle : NULL,
                         mesh->handle,
                         renderer->transform);
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    RendererTexture *created;
    RendererStatus status;
    size_t retained_pitch;
    void *handle = NULL;

    if (texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *texture = NULL;
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available || renderer->frame_active)
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
    retained_pitch = (size_t)desc->width * 4;
    created->pixels = Copy_pixels(desc->width, desc->height,
                                  rgba_pixels, pitch);
    if (created->pixels == NULL) {
        free(created);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    status = renderer->backend->texture_create(renderer->backend_context,
                                               desc, created->pixels,
                                               retained_pitch,
                                               &handle);
    if (status != RENDERER_STATUS_OK || handle == NULL) {
        free(created->pixels);
        free(created);
        return status != RENDERER_STATUS_OK ? status
                                           : RENDERER_STATUS_BACKEND_ERROR;
    }
    created->owner = renderer;
    created->handle = handle;
    created->desc = *desc;
    created->pitch = retained_pitch;
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
    RendererStatus status;

    if (renderer == NULL || texture == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available || renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Texture_is_owned(renderer, texture))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    right = (int64_t)region.x + region.width;
    bottom = (int64_t)region.y + region.height;
    if (region.x < 0 || region.y < 0
        || region.width <= 0 || region.height <= 0
        || right > texture->desc.width || bottom > texture->desc.height
        || rgba_pixels == NULL
        || !Pixel_data_valid(region.width, region.height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    status = renderer->backend->texture_update(renderer->backend_context,
                                               texture->handle, region,
                                               rgba_pixels, pitch);
    if (status == RENDERER_STATUS_OK)
        Copy_texture_region(texture, region, rgba_pixels, pitch);
    return status;
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
    free(texture->pixels);
    free(texture);
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_mesh_create(Renderer *renderer,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count,
                                    RendererMesh **mesh)
{
    RendererMesh *created;
    RendererVertex2D *retained_vertices;
    RendererStatus status;
    void *handle = NULL;

    if (mesh == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *mesh = NULL;
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available || renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    created = calloc(1, sizeof(*created));
    if (created == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    retained_vertices = Copy_vertices(vertices, vertex_count);
    if (retained_vertices == NULL) {
        free(created);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    status = renderer->backend->mesh_create(renderer->backend_context,
                                            retained_vertices, vertex_count,
                                            &handle);
    if (status != RENDERER_STATUS_OK || handle == NULL) {
        free(retained_vertices);
        free(created);
        return status != RENDERER_STATUS_OK ? status
                                           : RENDERER_STATUS_BACKEND_ERROR;
    }
    created->owner = renderer;
    created->handle = handle;
    created->vertices = retained_vertices;
    created->vertex_count = vertex_count;
    created->next = renderer->meshes;
    renderer->meshes = created;
    *mesh = created;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_mesh_update(Renderer *renderer, RendererMesh *mesh,
                                    const RendererVertex2D *vertices,
                                    size_t vertex_count)
{
    RendererVertex2D *replacement;
    RendererStatus status;

    if (renderer == NULL || mesh == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->context_available || renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (!Mesh_is_owned(renderer, mesh))
        return RENDERER_STATUS_RESOURCE_MISMATCH;
    if (vertices == NULL || !Vertex_count_valid(vertex_count))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    replacement = Copy_vertices(vertices, vertex_count);
    if (replacement == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    status = renderer->backend->mesh_update(renderer->backend_context,
                                            mesh->handle, replacement,
                                            vertex_count);
    if (status != RENDERER_STATUS_OK) {
        free(replacement);
        return status;
    }
    free(mesh->vertices);
    mesh->vertices = replacement;
    mesh->vertex_count = vertex_count;
    return RENDERER_STATUS_OK;
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
    free(mesh->vertices);
    free(mesh);
    return RENDERER_STATUS_OK;
}
