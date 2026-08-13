/* Coordinate transforms for semantic SDL rendering. */

#ifndef PAINT_TRANSFORM_H
#define PAINT_TRANSFORM_H

#include "renderer.h"

/**
 * Build the HUD transform from logical window coordinates to drawable pixels.
 *
 * @param logical_width Logical SDL window width.
 * @param logical_height Logical SDL window height.
 * @param drawable_width OpenGL drawable width in pixels.
 * @param drawable_height OpenGL drawable height in pixels.
 * @param transform Receives a column-major local-to-framebuffer transform.
 * @return 0 on success, or -1 for invalid dimensions or output storage.
 */
int Paint_transform_hud(int logical_width, int logical_height,
                        int drawable_width, int drawable_height,
                        RendererTransform2D *transform);

/**
 * Build a bottom-left world to top-left framebuffer transform.
 *
 * @param scale World-to-logical-window scale.
 * @param world_x X coordinate at the left edge of the visible world.
 * @param world_y Y coordinate at the bottom edge of the visible world.
 * @param logical_width Logical SDL window width.
 * @param logical_height Logical SDL window height.
 * @param drawable_width OpenGL drawable width in pixels.
 * @param drawable_height OpenGL drawable height in pixels.
 * @param rounded_translation Nonzero to round the camera
 * translation, or zero to preserve the fractional translation.
 * @param transform Receives a column-major local-to-framebuffer transform.
 * @return 0 on success, or -1 for invalid or non-finite input.
 */
int Paint_transform_world(double scale, double world_x, double world_y,
                          int logical_width, int logical_height,
                          int drawable_width, int drawable_height,
                          int rounded_translation,
                          RendererTransform2D *transform);

#endif /* PAINT_TRANSFORM_H */
