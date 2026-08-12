#include "test_helpers.h"

#include "sdlcompat.h"

#include <string.h>

static int get_pixel(SDL_Surface *surface, Uint8 *red, Uint8 *green,
                     Uint8 *blue, Uint8 *alpha)
{
    Uint32 pixel;

    if (SDL_LockSurface(surface) != 0) {
        return -1;
    }
    memcpy(&pixel, surface->pixels, sizeof(pixel));
    SDL_UnlockSurface(surface);
    SDL_GetRGBA(pixel, surface->format, red, green, blue, alpha);
    return 0;
}

int main(void)
{
    SDL_Surface *source;
    SDL_Surface *destination;
    SDL_BlendMode blend_mode;
    Uint8 alpha_mod;
    Uint8 red_mod;
    Uint8 green_mod;
    Uint8 blue_mod;
    Uint8 red;
    Uint8 green;
    Uint8 blue;
    Uint8 alpha;
    Uint32 source_pixel;

    source = SDL_CreateRGBSurfaceWithFormat(
        0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
    destination = SDL_CreateRGBSurfaceWithFormat(
        0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK(source != NULL);
    TEST_CHECK(destination != NULL);

    source_pixel = SDL_MapRGBA(source->format, 200, 100, 50, 80);
    memcpy(source->pixels, &source_pixel, sizeof(source_pixel));
    TEST_CHECK(SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_BLEND) == 0);
    TEST_CHECK(SDL_SetSurfaceAlphaMod(source, 123) == 0);
    TEST_CHECK(SDL_SetSurfaceColorMod(source, 17, 31, 47) == 0);

    TEST_CHECK(Sdl_blit_surface_unblended(source, NULL, destination, NULL) == 0);
    TEST_CHECK(get_pixel(destination, &red, &green, &blue, &alpha) == 0);
    TEST_CHECK(red == 200);
    TEST_CHECK(green == 100);
    TEST_CHECK(blue == 50);
    TEST_CHECK(alpha == 80);

    TEST_CHECK(SDL_GetSurfaceBlendMode(source, &blend_mode) == 0);
    TEST_CHECK(blend_mode == SDL_BLENDMODE_BLEND);
    TEST_CHECK(SDL_GetSurfaceAlphaMod(source, &alpha_mod) == 0);
    TEST_CHECK(alpha_mod == 123);
    TEST_CHECK(SDL_GetSurfaceColorMod(
                   source, &red_mod, &green_mod, &blue_mod) == 0);
    TEST_CHECK(red_mod == 17);
    TEST_CHECK(green_mod == 31);
    TEST_CHECK(blue_mod == 47);

    SDL_FreeSurface(destination);
    SDL_FreeSurface(source);
    return 0;
}
