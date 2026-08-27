#include "test_helpers.h"

#include "surface_primitives.h"

#include <SDL3/SDL.h>

static int pixel_equals(SDL_Surface *surface, int x, int y,
                        Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    Uint8 actual_red;
    Uint8 actual_green;
    Uint8 actual_blue;
    Uint8 actual_alpha;

    if (!SDL_ReadSurfacePixel(surface, x, y,
                              &actual_red, &actual_green,
                              &actual_blue, &actual_alpha)) {
        return 0;
    }
    return actual_red == red && actual_green == green
        && actual_blue == blue && actual_alpha == alpha;
}

static int check_line_endpoints_and_clipping(void)
{
    SDL_Surface *surface;
    SDL_Rect clip = {2, 1, 4, 5};
    Uint32 background;
    int x;

    surface = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK(surface != NULL);
    background = SDL_MapSurfaceRGBA(surface, 1, 2, 3, 255);
    TEST_CHECK(SDL_FillSurfaceRect(surface, NULL, background));
    TEST_CHECK(SDL_SetSurfaceClipRect(surface, &clip));

    TEST_CHECK(Sdl_surface_draw_line_rgba(
        surface, -2, 3, 7, 3, 200, 10, 20, 255));
    for (x = 0; x < 8; x++) {
        if (x >= 2 && x <= 5) {
            TEST_CHECK(pixel_equals(surface, x, 3, 200, 10, 20, 255));
        } else {
            TEST_CHECK(pixel_equals(surface, x, 3, 1, 2, 3, 255));
        }
    }

    TEST_CHECK(SDL_SetSurfaceClipRect(surface, NULL));
    TEST_CHECK(SDL_FillSurfaceRect(surface, NULL, background));
    TEST_CHECK(Sdl_surface_draw_line_rgba(
        surface, 0, 0, 3, 3, 90, 80, 70, 255));
    for (x = 0; x < 4; x++)
        TEST_CHECK(pixel_equals(surface, x, x, 90, 80, 70, 255));
    TEST_CHECK(pixel_equals(surface, 1, 2, 1, 2, 3, 255));

    SDL_DestroySurface(surface);
    return 0;
}

static int check_line_alpha_blending(void)
{
    SDL_Surface *surface;
    Uint32 background;

    surface = SDL_CreateSurface(5, 5, SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK(surface != NULL);
    background = SDL_MapSurfaceRGBA(surface, 20, 40, 60, 100);
    TEST_CHECK(SDL_FillSurfaceRect(surface, NULL, background));

    TEST_CHECK(Sdl_surface_draw_line_rgba(
        surface, 2, 2, 2, 2, 220, 140, 40, 128));
    TEST_CHECK(pixel_equals(surface, 2, 2, 120, 90, 50, 178));
    TEST_CHECK(pixel_equals(surface, 1, 2, 20, 40, 60, 100));

    SDL_DestroySurface(surface);
    return 0;
}

static int check_concave_polygon_and_clipping(void)
{
    const Sint16 x_coordinates[] = {1, 7, 7, 3, 3, 1};
    const Sint16 y_coordinates[] = {1, 1, 3, 3, 7, 7};
    SDL_Surface *surface;
    SDL_Rect clip = {2, 2, 4, 4};
    Uint32 background;
    int filled_count = 0;
    int x;
    int y;

    surface = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK(surface != NULL);
    background = SDL_MapSurfaceRGBA(surface, 3, 4, 5, 255);
    TEST_CHECK(SDL_FillSurfaceRect(surface, NULL, background));
    TEST_CHECK(SDL_SetSurfaceClipRect(surface, &clip));

    TEST_CHECK(Sdl_surface_fill_polygon_rgba(
        surface, x_coordinates, y_coordinates, 6,
        10, 200, 30, 255));
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            if (pixel_equals(surface, x, y, 10, 200, 30, 255))
                filled_count++;
        }
    }
    TEST_CHECK(filled_count == 7);
    TEST_CHECK(pixel_equals(surface, 2, 2, 10, 200, 30, 255));
    TEST_CHECK(pixel_equals(surface, 5, 2, 10, 200, 30, 255));
    TEST_CHECK(pixel_equals(surface, 2, 5, 10, 200, 30, 255));
    TEST_CHECK(pixel_equals(surface, 3, 3, 3, 4, 5, 255));
    TEST_CHECK(pixel_equals(surface, 1, 2, 3, 4, 5, 255));
    TEST_CHECK(pixel_equals(surface, 6, 2, 3, 4, 5, 255));

    SDL_DestroySurface(surface);
    return 0;
}

static int check_rgb565_surface(void)
{
    SDL_Surface *surface;
    Uint32 background;

    surface = SDL_CreateSurface(4, 4, SDL_PIXELFORMAT_RGB565);
    TEST_CHECK(surface != NULL);
    background = SDL_MapSurfaceRGB(surface, 0, 0, 0);
    TEST_CHECK(SDL_FillSurfaceRect(surface, NULL, background));
    TEST_CHECK(Sdl_surface_draw_line_rgba(
        surface, 0, 1, 3, 1, 255, 255, 255, 255));
    TEST_CHECK(pixel_equals(surface, 0, 1, 255, 255, 255, 255));
    TEST_CHECK(pixel_equals(surface, 3, 1, 255, 255, 255, 255));
    TEST_CHECK(pixel_equals(surface, 2, 2, 0, 0, 0, 255));

    SDL_DestroySurface(surface);
    return 0;
}

int main(void)
{
    TEST_CHECK(SDL_Init(0));
    TEST_CHECK(check_line_endpoints_and_clipping() == 0);
    TEST_CHECK(check_line_alpha_blending() == 0);
    TEST_CHECK(check_concave_polygon_and_clipping() == 0);
    TEST_CHECK(check_rgb565_surface() == 0);
    SDL_Quit();
    return 0;
}
