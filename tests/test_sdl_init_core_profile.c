#include "test_helpers.h"

#include "console.h"
#include "gl_diagnostics.h"
#include "glwidgets.h"
#include "sdlinit.h"
#include "text.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdarg.h>
#include <stddef.h>

unsigned draw_width = 800;
unsigned draw_height = 600;
int num_spark_colors;
GLWidget *MainWidget;

static int call_sequence;
static int create_window_sequence;
static int major_sequence;
static int minor_sequence;
static int profile_sequence;
static int requested_major;
static int requested_minor;
static int requested_profile;
static int create_window_calls;
static int create_window_had_opengl_flag;

void Conf_print(void)
{
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

int xpprintf(const char *format, ...)
{
    (void)format;
    return 0;
}

RendererStatus fontinit(font_data *font, Renderer *renderer,
                        const char *filename, unsigned int size)
{
    (void)font;
    (void)renderer;
    (void)filename;
    (void)size;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus font_text_renderer_attach(font_data *font,
                                         SdlRenderer *renderer)
{
    (void)font;
    (void)renderer;
    return RENDERER_STATUS_BACKEND_ERROR;
}

RendererStatus fontclean(font_data *font)
{
    (void)font;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_create(SDL_Window *window,
                                   SdlRenderer **renderer)
{
    (void)window;
    if (renderer != NULL)
        *renderer = NULL;
    return RENDERER_STATUS_BACKEND_ERROR;
}

void Sdl_renderer_destroy(SdlRenderer *renderer)
{
    (void)renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    (void)renderer;
    return NULL;
}

void Gl_diagnostics_log_context(void)
{
}

int Gl_diagnostics_check(const char *boundary)
{
    (void)boundary;
    return 0;
}

void Close_Widget(GLWidget **widget)
{
    if (widget != NULL)
        *widget = NULL;
}

void Gui_cleanup(void)
{
}

void Console_cleanup(void)
{
}

int SDLCALL __wrap_SDL_Init(Uint32 flags)
{
    TEST_CHECK((flags & SDL_INIT_VIDEO) != 0);
    return 0;
}

void SDLCALL __wrap_SDL_Quit(void)
{
}

int SDLCALL __wrap_IMG_Init(int flags)
{
    return flags;
}

void SDLCALL __wrap_IMG_Quit(void)
{
}

int SDLCALL __wrap_TTF_Init(void)
{
    return 0;
}

void SDLCALL __wrap_TTF_Quit(void)
{
}

int SDLCALL __wrap_SDL_GL_SetAttribute(SDL_GLattr attribute, int value)
{
    call_sequence++;
    switch (attribute) {
    case SDL_GL_CONTEXT_MAJOR_VERSION:
        requested_major = value;
        major_sequence = call_sequence;
        break;
    case SDL_GL_CONTEXT_MINOR_VERSION:
        requested_minor = value;
        minor_sequence = call_sequence;
        break;
    case SDL_GL_CONTEXT_PROFILE_MASK:
        requested_profile = value;
        profile_sequence = call_sequence;
        break;
    default:
        break;
    }
    return 0;
}

SDL_Window *SDLCALL __wrap_SDL_CreateWindow(
    const char *title, int x, int y, int width, int height, Uint32 flags)
{
    (void)title;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    call_sequence++;
    create_window_sequence = call_sequence;
    create_window_calls++;
    create_window_had_opengl_flag = (flags & SDL_WINDOW_OPENGL) != 0;

    /* Stop initialization after observing the complete pre-window contract. */
    return NULL;
}

int main(void)
{
    TEST_CHECK(Init_window() == -1);
    TEST_CHECK(create_window_calls == 1);
    TEST_CHECK(create_window_had_opengl_flag);
    TEST_CHECK(requested_major == 3);
    TEST_CHECK(requested_minor == 3);
    TEST_CHECK(requested_profile == SDL_GL_CONTEXT_PROFILE_CORE);
    TEST_CHECK(major_sequence > 0 && major_sequence < create_window_sequence);
    TEST_CHECK(minor_sequence > 0 && minor_sequence < create_window_sequence);
    TEST_CHECK(profile_sequence > 0
               && profile_sequence < create_window_sequence);
    return 0;
}
