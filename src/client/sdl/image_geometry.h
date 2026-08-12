/* Backend-neutral atlas sprite geometry. */

#ifndef IMAGE_GEOMETRY_H
#define IMAGE_GEOMETRY_H

#include "renderer.h"

#include <stddef.h>

/** Logical coordinate space used to place an image sprite. */
typedef enum ImageGeometrySpace {
    /** Position is the bottom-left corner in a world whose Y axis points up. */
    IMAGE_GEOMETRY_SPACE_WORLD,
    /** Position is the top-left corner in a HUD whose Y axis points down. */
    IMAGE_GEOMETRY_SPACE_HUD
} ImageGeometrySpace;

/** Dimensions describing a horizontal frame atlas in one RGBA8 texture. */
typedef struct ImageGeometryAtlas {
    /** Full texture width, including right-side padding. */
    int texture_width;
    /** Full texture height, including bottom padding. */
    int texture_height;
    /** Width of one frame before texture padding. */
    int frame_width;
    /** Height of one frame before texture padding. */
    int frame_height;
    /** Number of equally sized frames laid out from left to right. */
    size_t frame_count;
} ImageGeometryAtlas;

/**
 * Build two textured triangles for part of one atlas frame.
 *
 * @param atlas Top-left-origin horizontal frame atlas.
 * @param frame_index Zero-based frame to sample.
 * @param area Top-left-origin rectangle within the selected frame.
 * @param position Destination corner selected by @p space.
 * @param radians Visual counterclockwise rotation about the area center.
 * @param space Logical coordinate space for the destination.
 * @param tint Unpremultiplied color copied to every vertex.
 * @param vertices Receives six vertices in source-corner order
 *        TL, TR, BR, TL, BR, BL.
 * @return Operation status.
 *
 * @remarks The output remains unchanged when validation fails. Texture
 * coordinates use the renderer's top-left origin regardless of destination
 * coordinate space.
 */
RendererStatus Image_geometry_sprite(const ImageGeometryAtlas *atlas,
                                     size_t frame_index,
                                     RendererRect area,
                                     RendererPoint2D position,
                                     float radians,
                                     ImageGeometrySpace space,
                                     RendererColor tint,
                                     RendererVertex2D vertices[6]);

#endif /* IMAGE_GEOMETRY_H */
