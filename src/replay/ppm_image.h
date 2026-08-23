#ifndef PPM_IMAGE_H
#define PPM_IMAGE_H

#include <stddef.h>
#include <stdio.h>

/**
 * Write a scaled RGB image in binary PPM format.
 *
 * Each output pixel is the area-weighted average of the source pixels it
 * covers.  Output dimensions are rounded up so a positive scale always
 * produces at least one pixel in each direction.
 *
 * @param stream Destination stream.
 * @param rgb Source pixels in row-major RGB byte order.
 * @param width Source width in pixels.
 * @param height Source height in pixels.
 * @param scale Reduction factor in the range (0, 1].
 * @param gamma Gamma correction factor, or zero to disable correction.
 * @return Zero on success, or -1 for invalid input, allocation failure, or
 *         an output error.
 */
int Ppm_write_scaled_rgb(FILE *stream, const unsigned char *rgb,
                         size_t width, size_t height, double scale,
                         double gamma);

#endif
