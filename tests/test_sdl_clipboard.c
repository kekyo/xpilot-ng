#include "test_helpers.h"

#include "sdlclipboard.h"

#include <string.h>

int main(void)
{
    char *clipboard_text;

    TEST_CHECK(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    TEST_CHECK(SDL_Init(SDL_INIT_VIDEO) == 0);

    TEST_CHECK(Sdl_clipboard_set_text("first\r\nsecond\rthird\nfourth") == 0);
    clipboard_text = Sdl_clipboard_get_text();
    TEST_CHECK(clipboard_text != NULL);
    TEST_CHECK(strcmp(clipboard_text, "first\nsecond\nthird\nfourth") == 0);
    Sdl_clipboard_free_text(clipboard_text);

    TEST_CHECK(Sdl_clipboard_set_text("") == 0);
    TEST_CHECK(Sdl_clipboard_get_text() == NULL);
    TEST_CHECK(Sdl_clipboard_set_text(NULL) == -1);
    Sdl_clipboard_free_text(NULL);

    SDL_Quit();
    return 0;
}
