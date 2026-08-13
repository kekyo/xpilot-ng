#include "polygon_geometry.h"

#include "tesselator.h"

#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define POLYGON_VERTEX_SIZE 2
#define POLYGON_TRIANGLE_SIZE 3
#define POLYGON_ALLOCATION_BUDGET ((size_t)64 * 1024 * 1024)

typedef char Polygon_renderer_point_matches_tess_real[
    sizeof(((RendererPoint2D *)0)->x) == sizeof(TESSreal)
        && offsetof(RendererPoint2D, x) == 0
        && offsetof(RendererPoint2D, y) == sizeof(TESSreal)
        && sizeof(RendererPoint2D)
               == POLYGON_VERTEX_SIZE * sizeof(TESSreal)
        ? 1 : -1];

typedef struct PolygonTrackedAllocation {
    void *pointer;
    size_t size;
    struct PolygonTrackedAllocation *next;
} PolygonTrackedAllocation;

typedef struct PolygonAllocatorContext {
    jmp_buf environment;
    PolygonTrackedAllocation *allocations;
    RendererPoint2D *output;
    size_t output_count;
    size_t outstanding_bytes;
    RendererStatus failure_status;
} PolygonAllocatorContext;

static void Polygon_allocator_release_all(PolygonAllocatorContext *context)
{
    PolygonTrackedAllocation *allocation = context->allocations;

    while (allocation != NULL) {
        PolygonTrackedAllocation *next = allocation->next;

        free(allocation->pointer);
        free(allocation);
        allocation = next;
    }
    context->allocations = NULL;
    context->outstanding_bytes = 0;
}

static void Polygon_allocator_fail(
    PolygonAllocatorContext *context, RendererStatus status)
{
    context->failure_status = status;
    longjmp(context->environment, 1);
}

static PolygonTrackedAllocation *Polygon_allocator_find(
    PolygonAllocatorContext *context, void *pointer)
{
    PolygonTrackedAllocation *allocation;

    for (allocation = context->allocations;
         allocation != NULL;
         allocation = allocation->next) {
        if (allocation->pointer == pointer)
            return allocation;
    }
    return NULL;
}

static void *Polygon_allocator_allocate(void *user_data, unsigned int size)
{
    PolygonAllocatorContext *context = user_data;
    PolygonTrackedAllocation *tracked;
    void *pointer;
    size_t allocation_size = size == 0 ? 1 : (size_t)size;
    size_t allocation_charge;

    if (allocation_size > SIZE_MAX - sizeof(*tracked))
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);
    allocation_charge = allocation_size + sizeof(*tracked);
    if (context->outstanding_bytes > POLYGON_ALLOCATION_BUDGET
        || allocation_charge
               > POLYGON_ALLOCATION_BUDGET
                     - context->outstanding_bytes) {
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);
    }

    pointer = malloc(allocation_size);
    if (pointer == NULL)
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);

    tracked = malloc(sizeof(*tracked));
    if (tracked == NULL) {
        free(pointer);
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);
    }
    tracked->pointer = pointer;
    tracked->size = allocation_size;
    tracked->next = context->allocations;
    context->allocations = tracked;
    context->outstanding_bytes += allocation_charge;
    return pointer;
}

static void *Polygon_allocator_reallocate(
    void *user_data, void *pointer, unsigned int size)
{
    PolygonAllocatorContext *context = user_data;
    PolygonTrackedAllocation *tracked;
    void *replacement;
    size_t allocation_size = size == 0 ? 1 : (size_t)size;
    size_t retained_bytes;

    if (pointer == NULL)
        return Polygon_allocator_allocate(user_data, size);

    tracked = Polygon_allocator_find(context, pointer);
    if (tracked == NULL)
        Polygon_allocator_fail(context, RENDERER_STATUS_BACKEND_ERROR);
    if (tracked->size > context->outstanding_bytes)
        Polygon_allocator_fail(context, RENDERER_STATUS_BACKEND_ERROR);

    retained_bytes = context->outstanding_bytes - tracked->size;
    if (retained_bytes > POLYGON_ALLOCATION_BUDGET
        || allocation_size > POLYGON_ALLOCATION_BUDGET - retained_bytes) {
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);
    }

    replacement = realloc(pointer, allocation_size);
    if (replacement == NULL)
        Polygon_allocator_fail(context, RENDERER_STATUS_OUT_OF_MEMORY);

    tracked->pointer = replacement;
    tracked->size = allocation_size;
    context->outstanding_bytes = retained_bytes + allocation_size;
    return replacement;
}

static void Polygon_allocator_free(void *user_data, void *pointer)
{
    PolygonAllocatorContext *context = user_data;
    PolygonTrackedAllocation **link;
    PolygonTrackedAllocation *tracked;
    size_t allocation_charge;

    if (pointer == NULL)
        return;

    for (link = &context->allocations;
         *link != NULL && (*link)->pointer != pointer;
         link = &(*link)->next) {
    }
    tracked = *link;
    if (tracked == NULL)
        Polygon_allocator_fail(context, RENDERER_STATUS_BACKEND_ERROR);
    if (tracked->size > SIZE_MAX - sizeof(*tracked))
        Polygon_allocator_fail(context, RENDERER_STATUS_BACKEND_ERROR);
    allocation_charge = tracked->size + sizeof(*tracked);
    if (allocation_charge > context->outstanding_bytes)
        Polygon_allocator_fail(context, RENDERER_STATUS_BACKEND_ERROR);

    *link = tracked->next;
    context->outstanding_bytes -= allocation_charge;
    free(tracked->pointer);
    free(tracked);
}

static RendererStatus Polygon_status_from_tess(
    TESSstatus status, RendererStatus ok_status)
{
    switch (status) {
    case TESS_STATUS_OK:
        return ok_status;
    case TESS_STATUS_OUT_OF_MEMORY:
        return RENDERER_STATUS_OUT_OF_MEMORY;
    case TESS_STATUS_INVALID_INPUT:
        return RENDERER_STATUS_INVALID_ARGUMENT;
    default:
        return RENDERER_STATUS_BACKEND_ERROR;
    }
}

static int Polygon_size_multiply_fits(size_t count, size_t item_size)
{
    return item_size == 0
        || (count <= SIZE_MAX / item_size
            && count <= (size_t)PTRDIFF_MAX / item_size);
}

static double Polygon_triangle_twice_area(
    RendererPoint2D first, RendererPoint2D second, RendererPoint2D third)
{
    return ((double)second.x - first.x) * ((double)third.y - first.y)
        - ((double)second.y - first.y) * ((double)third.x - first.x);
}

static RendererStatus Polygon_validate_input(
    const RendererPoint2D *points, size_t point_count)
{
    size_t index;

    if (point_count > (size_t)INT_MAX)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (!Polygon_size_multiply_fits(point_count, sizeof(*points)))
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (points == NULL || point_count < POLYGON_TRIANGLE_SIZE)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (sizeof(*points) > (size_t)INT_MAX)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    for (index = 0; index < point_count; index++) {
        float x = points[index].x;
        float y = points[index].y;

        if (!isfinite(x) || !isfinite(y)
            || x < TESS_MIN_VALID_INPUT_VALUE
            || x > TESS_MAX_VALID_INPUT_VALUE
            || y < TESS_MIN_VALID_INPUT_VALUE
            || y > TESS_MAX_VALID_INPUT_VALUE) {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Polygon_copy_tessellation(
    TESStesselator *tess, PolygonAllocatorContext *context)
{
    const TESSindex *elements;
    const TESSreal *vertices;
    int element_count = tessGetElementCount(tess);
    int vertex_count = tessGetVertexCount(tess);
    size_t element_index;
    size_t element_value_count;
    size_t output_count = 0;
    size_t vertex_coordinate_count;

    context->output_count = 0;

    if (element_count < 0 || vertex_count < 0)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (element_count == 0)
        return RENDERER_STATUS_OK;
    if (vertex_count == 0)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (!Polygon_size_multiply_fits(
            (size_t)element_count, POLYGON_TRIANGLE_SIZE)
        || !Polygon_size_multiply_fits(
            (size_t)vertex_count, POLYGON_VERTEX_SIZE)) {
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }

    element_value_count
        = (size_t)element_count * POLYGON_TRIANGLE_SIZE;
    vertex_coordinate_count = (size_t)vertex_count * POLYGON_VERTEX_SIZE;
    if (!Polygon_size_multiply_fits(
            element_value_count, sizeof(*elements))
        || !Polygon_size_multiply_fits(
            vertex_coordinate_count, sizeof(*vertices))) {
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }

    elements = tessGetElements(tess);
    vertices = tessGetVertices(tess);
    if (elements == NULL || vertices == NULL)
        return RENDERER_STATUS_BACKEND_ERROR;

    for (element_index = 0;
         element_index < (size_t)vertex_count;
         element_index++) {
        size_t coordinate_index = element_index * POLYGON_VERTEX_SIZE;

        if (!isfinite(vertices[coordinate_index])
            || !isfinite(vertices[coordinate_index + 1])
            || vertices[coordinate_index] < TESS_MIN_VALID_INPUT_VALUE
            || vertices[coordinate_index] > TESS_MAX_VALID_INPUT_VALUE
            || vertices[coordinate_index + 1] < TESS_MIN_VALID_INPUT_VALUE
            || vertices[coordinate_index + 1] > TESS_MAX_VALID_INPUT_VALUE) {
            return RENDERER_STATUS_BACKEND_ERROR;
        }
    }

    for (element_index = 0;
         element_index < (size_t)element_count;
         element_index++) {
        RendererPoint2D triangle[POLYGON_TRIANGLE_SIZE];
        size_t triangle_index;
        double twice_area;

        for (triangle_index = 0;
             triangle_index < POLYGON_TRIANGLE_SIZE;
             triangle_index++) {
            TESSindex vertex_index = elements[
                element_index * POLYGON_TRIANGLE_SIZE + triangle_index];
            size_t coordinate_index;

            if (vertex_index < 0 || vertex_index >= vertex_count)
                return RENDERER_STATUS_BACKEND_ERROR;
            coordinate_index = (size_t)vertex_index * POLYGON_VERTEX_SIZE;
            triangle[triangle_index].x = vertices[coordinate_index];
            triangle[triangle_index].y = vertices[coordinate_index + 1];
        }

        twice_area = Polygon_triangle_twice_area(
            triangle[0], triangle[1], triangle[2]);
        if (!isfinite(twice_area))
            return RENDERER_STATUS_BACKEND_ERROR;
        if (twice_area != 0.0) {
            if (output_count > SIZE_MAX - POLYGON_TRIANGLE_SIZE)
                return RENDERER_STATUS_OUT_OF_MEMORY;
            output_count += POLYGON_TRIANGLE_SIZE;
        }
    }

    if (output_count == 0)
        return RENDERER_STATUS_OK;
    if (!Polygon_size_multiply_fits(output_count, sizeof(*context->output))
        || output_count * sizeof(*context->output)
               > POLYGON_ALLOCATION_BUDGET) {
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }

    context->output = malloc(output_count * sizeof(*context->output));
    if (context->output == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    output_count = 0;
    for (element_index = 0;
         element_index < (size_t)element_count;
         element_index++) {
        RendererPoint2D triangle[POLYGON_TRIANGLE_SIZE];
        size_t triangle_index;

        for (triangle_index = 0;
             triangle_index < POLYGON_TRIANGLE_SIZE;
             triangle_index++) {
            TESSindex vertex_index = elements[
                element_index * POLYGON_TRIANGLE_SIZE + triangle_index];
            size_t coordinate_index
                = (size_t)vertex_index * POLYGON_VERTEX_SIZE;

            triangle[triangle_index].x = vertices[coordinate_index];
            triangle[triangle_index].y = vertices[coordinate_index + 1];
        }
        if (Polygon_triangle_twice_area(
                triangle[0], triangle[1], triangle[2]) == 0.0) {
            continue;
        }
        memcpy(&context->output[output_count], triangle, sizeof(triangle));
        output_count += POLYGON_TRIANGLE_SIZE;
    }
    context->output_count = output_count;
    return RENDERER_STATUS_OK;
}

static RendererStatus Polygon_tessellate_with_context(
    const RendererPoint2D *points, size_t point_count,
    PolygonGeometry *geometry, PolygonAllocatorContext *context)
{
    TESSalloc allocator;
    TESStesselator *tess;
    RendererStatus status;

    memset(&allocator, 0, sizeof(allocator));
    allocator.memalloc = Polygon_allocator_allocate;
    allocator.memrealloc = Polygon_allocator_reallocate;
    allocator.memfree = Polygon_allocator_free;
    allocator.userData = context;

    tess = tessNewTess(&allocator);
    if (tess == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    tessAddContour(tess, POLYGON_VERTEX_SIZE, points,
                   (int)sizeof(*points), (int)point_count);
    status = Polygon_status_from_tess(
        tessGetStatus(tess), RENDERER_STATUS_OK);
    if (status == RENDERER_STATUS_OK
        && !tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS,
                          POLYGON_TRIANGLE_SIZE, POLYGON_VERTEX_SIZE, NULL)) {
        status = Polygon_status_from_tess(
            tessGetStatus(tess), RENDERER_STATUS_BACKEND_ERROR);
    }
    if (status == RENDERER_STATUS_OK) {
        status = Polygon_status_from_tess(
            tessGetStatus(tess), RENDERER_STATUS_OK);
    }
    if (status == RENDERER_STATUS_OK) {
        status = Polygon_copy_tessellation(tess, context);
    }

    tessDeleteTess(tess);
    Polygon_allocator_release_all(context);

    if (status != RENDERER_STATUS_OK) {
        free(context->output);
        context->output = NULL;
        return status;
    }
    if (context->output != NULL) {
        geometry->triangle_points = context->output;
        geometry->triangle_point_count = context->output_count;
        context->output = NULL;
        context->output_count = 0;
    }
    return RENDERER_STATUS_OK;
}

void Polygon_geometry_cleanup(PolygonGeometry *geometry)
{
    if (geometry == NULL)
        return;

    free(geometry->triangle_points);
    geometry->triangle_points = NULL;
    geometry->triangle_point_count = 0;
}

RendererStatus Polygon_geometry_tessellate_odd(
    const RendererPoint2D *points, size_t point_count,
    PolygonGeometry *geometry)
{
    PolygonAllocatorContext *context;
    RendererStatus status;

    if (geometry == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    Polygon_geometry_cleanup(geometry);

    status = Polygon_validate_input(points, point_count);
    if (status != RENDERER_STATUS_OK)
        return status;

    context = malloc(sizeof(*context));
    if (context == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    memset(context, 0, sizeof(*context));
    context->failure_status = RENDERER_STATUS_OUT_OF_MEMORY;

    /* The mutable allocator state lives on the heap so it remains defined
     * after an allocator callback leaves libtess2 through longjmp. */
    if (setjmp(context->environment) != 0) {
        status = context->failure_status;
        Polygon_allocator_release_all(context);
        free(context->output);
        free(context);
        return status;
    }

    status = Polygon_tessellate_with_context(
        points, point_count, geometry, context);
    Polygon_allocator_release_all(context);
    free(context->output);
    free(context);
    return status;
}
