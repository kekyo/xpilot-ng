#include "test_helpers.h"

#include "sdlkeys.h"

#include <string.h>

typedef struct {
    const char *name;
    SDL_Keycode key;
} key_case_t;

int main(void)
{
    static const key_case_t cases[] = {
        { "a", SDLK_a },
        { "A", SDLK_a },
        { "Return", SDLK_RETURN },
        { "Linefeed", SDLK_RETURN },
        { "Shift_L", SDLK_LSHIFT },
        { "F11", SDLK_F11 },
        { "Left", SDLK_LEFT },
        { "Print", SDLK_PRINTSCREEN },
        { "Scroll_Lock", SDLK_SCROLLLOCK },
        { "Num_Lock", SDLK_NUMLOCKCLEAR },
        { "KP_0", SDLK_KP_0 },
        { "KP_Insert", SDLK_KP_0 },
        { "section", (SDL_Keycode)0x00a7 }
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        SDL_Keycode key = Get_key_by_name(cases[index].name);
        const char *canonical_name;

        TEST_CHECK(key == cases[index].key);
        canonical_name = Get_name_by_key(key);
        TEST_CHECK(canonical_name != NULL);
        TEST_CHECK(Get_key_by_name(canonical_name) == key);
    }

    TEST_CHECK(Get_key_by_name("not-a-real-key") == SDLK_UNKNOWN);
    TEST_CHECK(Get_name_by_key(SDLK_UNKNOWN) == NULL);
    TEST_CHECK(String_to_xp_keysym("not-a-real-key") == XP_KS_UNKNOWN);
    TEST_CHECK(String_to_xp_keysym("Return") == (xp_keysym_t)SDLK_RETURN);

    return 0;
}
