#include "test_helpers.h"

#include "ppm_image.h"

#include <stdio.h>
#include <string.h>

#define OUTPUT_CAPACITY 128

static size_t read_output(FILE *stream, unsigned char *output)
{
    long length;

    TEST_CHECK(fflush(stream) == 0);
    TEST_CHECK(fseek(stream, 0, SEEK_END) == 0);
    length = ftell(stream);
    TEST_CHECK(length >= 0);
    TEST_CHECK((size_t)length <= OUTPUT_CAPACITY);
    TEST_CHECK(fseek(stream, 0, SEEK_SET) == 0);
    TEST_CHECK(fread(output, 1, (size_t)length, stream) == (size_t)length);
    return (size_t)length;
}

static int check_area_average_downscale(void)
{
    static const unsigned char input[] = {
        0, 0, 0, 100, 20, 40, 10, 200, 30, 30, 40, 250,
        200, 100, 60, 100, 80, 100, 50, 0, 90, 110, 160, 10
    };
    static const unsigned char expected[] = {
        'P', '6', '\n', '2', ' ', '1', '\n', '2', '5', '5', '\n',
        100, 50, 50, 50, 100, 95
    };
    unsigned char output[OUTPUT_CAPACITY];
    FILE *stream = tmpfile();
    size_t length;

    TEST_CHECK(stream != NULL);
    TEST_CHECK(Ppm_write_scaled_rgb(stream, input, 4, 2, 0.5, 0.0) == 0);
    length = read_output(stream, output);
    TEST_CHECK(fclose(stream) == 0);
    TEST_CHECK(length == sizeof(expected));
    TEST_CHECK(memcmp(output, expected, sizeof(expected)) == 0);
    return 0;
}

static int check_fractional_dimensions_round_up(void)
{
    static const unsigned char input[] = {
        12, 34, 56, 12, 34, 56, 12, 34, 56,
        12, 34, 56, 12, 34, 56, 12, 34, 56,
        12, 34, 56, 12, 34, 56, 12, 34, 56
    };
    static const unsigned char expected[] = {
        'P', '6', '\n', '2', ' ', '2', '\n', '2', '5', '5', '\n',
        12, 34, 56, 12, 34, 56, 12, 34, 56, 12, 34, 56
    };
    unsigned char output[OUTPUT_CAPACITY];
    FILE *stream = tmpfile();
    size_t length;

    TEST_CHECK(stream != NULL);
    TEST_CHECK(Ppm_write_scaled_rgb(stream, input, 3, 3, 0.5, 0.0) == 0);
    length = read_output(stream, output);
    TEST_CHECK(fclose(stream) == 0);
    TEST_CHECK(length == sizeof(expected));
    TEST_CHECK(memcmp(output, expected, sizeof(expected)) == 0);
    return 0;
}

static int check_gamma_correction(void)
{
    static const unsigned char input[] = { 0, 64, 255, 0, 64, 255 };
    static const unsigned char expected[] = {
        'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n',
        0, 128, 255
    };
    unsigned char output[OUTPUT_CAPACITY];
    FILE *stream = tmpfile();
    size_t length;

    TEST_CHECK(stream != NULL);
    TEST_CHECK(Ppm_write_scaled_rgb(stream, input, 2, 1, 0.5, 2.0) == 0);
    length = read_output(stream, output);
    TEST_CHECK(fclose(stream) == 0);
    TEST_CHECK(length == sizeof(expected));
    TEST_CHECK(memcmp(output, expected, sizeof(expected)) == 0);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_area_average_downscale() == 0);
    TEST_CHECK(check_fractional_dimensions_round_up() == 0);
    TEST_CHECK(check_gamma_correction() == 0);
    return 0;
}
