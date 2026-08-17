#include "test_helpers.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

static int linked_sdl_major(void)
{
    return SDL_VERSIONNUM_MAJOR(SDL_GetVersion());
}

static int linked_image_major(void)
{
    return SDL_VERSIONNUM_MAJOR(IMG_Version());
}

static int linked_ttf_major(void)
{
    return SDL_VERSIONNUM_MAJOR(TTF_Version());
}

int main(void)
{
    TEST_CHECK(SDL_MAJOR_VERSION == 3);
    TEST_CHECK(SDL_IMAGE_MAJOR_VERSION == 3);
    TEST_CHECK(SDL_TTF_MAJOR_VERSION == 3);
    TEST_CHECK(linked_sdl_major() == 3);
    TEST_CHECK(linked_image_major() == 3);
    TEST_CHECK(linked_ttf_major() == 3);
    return 0;
}
