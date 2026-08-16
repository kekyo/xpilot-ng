#include "test_helpers.h"

#include "surface_rgba8.h"

#include <SDL.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define TEST_CHECK_CLEANUP(condition)                                       \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: check failed: %s\n",                 \
                    __FILE__, __LINE__, #condition);                        \
            result = 1;                                                     \
            goto cleanup;                                                   \
        }                                                                   \
    } while (0)

static const uint8_t *image_pixel(const SdlRgba8Image *image, int x, int y)
{
    return image->pixels + (size_t)y * image->pitch + (size_t)x * 4;
}

static int pixel_equals(const SdlRgba8Image *image, int x, int y,
                        uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t alpha)
{
    const uint8_t *pixel = image_pixel(image, x, y);

    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
}

static int check_bgra_pitch_and_top_left_order(void)
{
    uint8_t pixels[32] = {
         3,  2,  1,  4,   7,  6,  5,  8,  11, 10,  9, 12,
        91, 92, 93, 94,
        15, 14, 13, 16,  19, 18, 17, 20,  23, 22, 21, 24,
        95, 96, 97, 98
    };
    SDL_Surface *source = NULL;
    SdlRgba8Image *image = NULL;
    int result = 0;

    source = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels, 3, 2, 32, 16, SDL_PIXELFORMAT_BGRA32);
    TEST_CHECK_CLEANUP(source != NULL);
    image = Sdl_rgba8_image_create(source);
    TEST_CHECK_CLEANUP(image != NULL);
    TEST_CHECK_CLEANUP(image->content_width == 3);
    TEST_CHECK_CLEANUP(image->content_height == 2);
    TEST_CHECK_CLEANUP(image->texture_width == 4);
    TEST_CHECK_CLEANUP(image->texture_height == 2);
    TEST_CHECK_CLEANUP(image->pitch == 16);
    TEST_CHECK_CLEANUP(pixel_equals(image, 0, 0, 1, 2, 3, 4));
    TEST_CHECK_CLEANUP(pixel_equals(image, 2, 0, 9, 10, 11, 12));
    TEST_CHECK_CLEANUP(pixel_equals(image, 0, 1, 13, 14, 15, 16));
    TEST_CHECK_CLEANUP(pixel_equals(image, 2, 1, 21, 22, 23, 24));
    TEST_CHECK_CLEANUP(pixel_equals(image, 3, 0, 0, 0, 0, 0));
    TEST_CHECK_CLEANUP(pixel_equals(image, 3, 1, 0, 0, 0, 0));

cleanup:
    Sdl_rgba8_image_destroy(image);
    SDL_FreeSurface(source);
    return result;
}

static int check_power_of_two_padding(void)
{
    SDL_Surface *source = NULL;
    SdlRgba8Image *image = NULL;
    SDL_Rect content = {0, 0, 3, 3};
    Uint32 color;
    int x;
    int y;
    int result = 0;

    source = SDL_CreateRGBSurfaceWithFormat(
        0, 3, 3, 32, SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK_CLEANUP(source != NULL);
    color = SDL_MapRGBA(source->format, 17, 34, 51, 68);
    TEST_CHECK_CLEANUP(SDL_FillRect(source, &content, color) == 0);
    image = Sdl_rgba8_image_create(source);
    TEST_CHECK_CLEANUP(image != NULL);
    TEST_CHECK_CLEANUP(image->texture_width == 4);
    TEST_CHECK_CLEANUP(image->texture_height == 4);
    TEST_CHECK_CLEANUP(pixel_equals(image, 0, 0, 17, 34, 51, 68));
    TEST_CHECK_CLEANUP(pixel_equals(image, 2, 2, 17, 34, 51, 68));
    for (y = 0; y < image->texture_height; y++) {
        for (x = 0; x < image->texture_width; x++) {
            if (x >= image->content_width || y >= image->content_height) {
                TEST_CHECK_CLEANUP(
                    pixel_equals(image, x, y, 0, 0, 0, 0));
            }
        }
    }

cleanup:
    Sdl_rgba8_image_destroy(image);
    SDL_FreeSurface(source);
    return result;
}

static int check_color_key_and_source_state(void)
{
    SDL_Surface *source = NULL;
    SdlRgba8Image *image = NULL;
    SDL_Rect left = {0, 0, 1, 1};
    SDL_Rect right = {1, 0, 1, 1};
    SDL_BlendMode blend_mode = SDL_BLENDMODE_NONE;
    Uint8 alpha = 0;
    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;
    Uint32 key = 0;
    Uint32 red_pixel;
    Uint32 green_pixel;
    int result = 0;

    source = SDL_CreateRGBSurfaceWithFormat(
        0, 2, 1, 24, SDL_PIXELFORMAT_RGB24);
    TEST_CHECK_CLEANUP(source != NULL);
    red_pixel = SDL_MapRGB(source->format, 255, 0, 0);
    green_pixel = SDL_MapRGB(source->format, 0, 255, 0);
    TEST_CHECK_CLEANUP(SDL_FillRect(source, &left, red_pixel) == 0);
    TEST_CHECK_CLEANUP(SDL_FillRect(source, &right, green_pixel) == 0);
    TEST_CHECK_CLEANUP(SDL_SetColorKey(source, SDL_TRUE, red_pixel) == 0);
    TEST_CHECK_CLEANUP(SDL_SetSurfaceRLE(source, 1) == 0);
    TEST_CHECK_CLEANUP(SDL_HasSurfaceRLE(source) == SDL_TRUE);
    TEST_CHECK_CLEANUP(
        SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_BLEND) == 0);
    TEST_CHECK_CLEANUP(SDL_SetSurfaceAlphaMod(source, 73) == 0);
    TEST_CHECK_CLEANUP(SDL_SetSurfaceColorMod(source, 11, 22, 33) == 0);

    image = Sdl_rgba8_image_create(source);
    TEST_CHECK_CLEANUP(image != NULL);
    TEST_CHECK_CLEANUP(pixel_equals(image, 0, 0, 0, 0, 0, 0));
    TEST_CHECK_CLEANUP(pixel_equals(image, 1, 0, 0, 255, 0, 255));
    TEST_CHECK_CLEANUP(SDL_GetSurfaceBlendMode(source, &blend_mode) == 0);
    TEST_CHECK_CLEANUP(blend_mode == SDL_BLENDMODE_BLEND);
    TEST_CHECK_CLEANUP(SDL_GetSurfaceAlphaMod(source, &alpha) == 0);
    TEST_CHECK_CLEANUP(alpha == 73);
    TEST_CHECK_CLEANUP(
        SDL_GetSurfaceColorMod(source, &red, &green, &blue) == 0);
    TEST_CHECK_CLEANUP(red == 11 && green == 22 && blue == 33);
    TEST_CHECK_CLEANUP(SDL_GetColorKey(source, &key) == 0);
    TEST_CHECK_CLEANUP(key == red_pixel);
    TEST_CHECK_CLEANUP(SDL_HasSurfaceRLE(source) == SDL_TRUE);
    TEST_CHECK_CLEANUP(source->locked == 0);

cleanup:
    Sdl_rgba8_image_destroy(image);
    SDL_FreeSurface(source);
    return result;
}

static int check_invalid_dimensions(void)
{
    SDL_Surface invalid;

    TEST_CHECK(Sdl_rgba8_image_create(NULL) == NULL);
    Sdl_rgba8_image_destroy(NULL);

    memset(&invalid, 0, sizeof(invalid));
    invalid.w = 0;
    invalid.h = 1;
    TEST_CHECK(Sdl_rgba8_image_create(&invalid) == NULL);

    invalid.w = INT_MAX;
    invalid.h = 1;
    TEST_CHECK(Sdl_rgba8_image_create(&invalid) == NULL);

    invalid.w = 1;
    invalid.h = INT_MAX;
    TEST_CHECK(Sdl_rgba8_image_create(&invalid) == NULL);
    return 0;
}

int main(void)
{
    TEST_CHECK(SDL_Init(0) == 0);
    TEST_CHECK(check_bgra_pitch_and_top_left_order() == 0);
    TEST_CHECK(check_power_of_two_padding() == 0);
    TEST_CHECK(check_color_key_and_source_state() == 0);
    TEST_CHECK(check_invalid_dimensions() == 0);
    SDL_Quit();
    return 0;
}
