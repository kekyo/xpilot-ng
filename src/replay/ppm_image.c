#include "ppm_image.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#define RGB_CHANNEL_COUNT 3
#define RGB_CHANNEL_MAX 255.0

static size_t Scaled_dimension(size_t source_dimension, double scale)
{
    double scaled = (double)source_dimension * scale;
    size_t result = (size_t)scaled;

    if ((double)result < scaled)
        result++;
    if (result == 0)
        result = 1;
    return result;
}

static double Interval_overlap(double first_start, double first_end,
                               double second_start, double second_end)
{
    double start = fmax(first_start, second_start);
    double end = fmin(first_end, second_end);

    return end > start ? end - start : 0.0;
}

static unsigned char Correct_gamma(unsigned char value, double gamma)
{
    double normalized;
    double corrected;

    if (gamma == 0.0)
        return value;
    normalized = (double)value / RGB_CHANNEL_MAX;
    corrected = RGB_CHANNEL_MAX * pow(normalized, 1.0 / gamma) + 0.5;
    if (corrected > RGB_CHANNEL_MAX)
        corrected = RGB_CHANNEL_MAX;
    return (unsigned char)corrected;
}

static unsigned char Average_channel(const unsigned char *rgb,
                                     size_t source_width,
                                     size_t source_height,
                                     double source_x_start,
                                     double source_x_end,
                                     double source_y_start,
                                     double source_y_end,
                                     size_t channel)
{
    size_t first_x = (size_t)source_x_start;
    size_t final_x = (size_t)ceil(source_x_end);
    size_t first_y = (size_t)source_y_start;
    size_t final_y = (size_t)ceil(source_y_end);
    double weighted_sum = 0.0;
    double total_weight = 0.0;
    size_t source_y;

    if (final_x > source_width)
        final_x = source_width;
    if (final_y > source_height)
        final_y = source_height;

    for (source_y = first_y; source_y < final_y; source_y++) {
        double y_weight = Interval_overlap(
            source_y_start, source_y_end,
            (double)source_y, (double)source_y + 1.0);
        size_t source_x;

        for (source_x = first_x; source_x < final_x; source_x++) {
            double x_weight = Interval_overlap(
                source_x_start, source_x_end,
                (double)source_x, (double)source_x + 1.0);
            double weight = x_weight * y_weight;
            size_t source_offset =
                (source_y * source_width + source_x) * RGB_CHANNEL_COUNT;

            weighted_sum += weight * rgb[source_offset + channel];
            total_weight += weight;
        }
    }
    return (unsigned char)(weighted_sum / total_weight + 0.5);
}

int Ppm_write_scaled_rgb(FILE *stream, const unsigned char *rgb,
                         size_t width, size_t height, double scale,
                         double gamma)
{
    const size_t maximum_size = (size_t)-1;
    size_t output_width;
    size_t output_height;
    size_t output_row_size;
    unsigned char *output_row;
    size_t output_y;

    if (stream == NULL || rgb == NULL || width == 0 || height == 0
        || !isfinite(scale) || scale <= 0.0 || scale > 1.0
        || !isfinite(gamma) || gamma < 0.0
        || width > maximum_size / height
        || width * height > maximum_size / RGB_CHANNEL_COUNT) {
        errno = EINVAL;
        return -1;
    }

    output_width = Scaled_dimension(width, scale);
    output_height = Scaled_dimension(height, scale);
    output_row_size = output_width * RGB_CHANNEL_COUNT;
    output_row = malloc(output_row_size);
    if (output_row == NULL)
        return -1;

    if (fprintf(stream, "P6\n%zu %zu\n255\n", output_width, output_height)
        < 0) {
        free(output_row);
        return -1;
    }

    for (output_y = 0; output_y < output_height; output_y++) {
        double source_y_start =
            (double)output_y * height / output_height;
        double source_y_end =
            (double)(output_y + 1) * height / output_height;
        size_t output_x;

        for (output_x = 0; output_x < output_width; output_x++) {
            double source_x_start =
                (double)output_x * width / output_width;
            double source_x_end =
                (double)(output_x + 1) * width / output_width;
            size_t channel;

            for (channel = 0; channel < RGB_CHANNEL_COUNT; channel++) {
                unsigned char average = Average_channel(
                    rgb, width, height, source_x_start, source_x_end,
                    source_y_start, source_y_end, channel);

                output_row[output_x * RGB_CHANNEL_COUNT + channel] =
                    Correct_gamma(average, gamma);
            }
        }
        if (fwrite(output_row, 1, output_row_size, stream)
            != output_row_size) {
            free(output_row);
            return -1;
        }
    }

    free(output_row);
    return fflush(stream) == 0 ? 0 : -1;
}
