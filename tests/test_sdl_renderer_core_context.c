#include "test_helpers.h"

#include "renderer_gl.h"
#include "sdlrenderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <stdio.h>
#include <string.h>

#define FRAME_WIDTH 64
#define FRAME_HEIGHT 48

typedef struct TestContext {
    SDL_Window *window;
    SDL_GLContext context;
} TestContext;

static void destroy_context(TestContext *test_context)
{
    if (test_context->context != NULL)
        SDL_GL_DestroyContext(test_context->context);
    if (test_context->window != NULL)
        SDL_DestroyWindow(test_context->window);
    memset(test_context, 0, sizeof(*test_context));
}

static int create_context(TestContext *test_context, int profile,
                          const char *title, int required)
{
    int major = 0;
    int minor = 0;
    int actual_profile = 0;
    const char *failure_prefix = required ? "ERROR" : "SKIP";

    memset(test_context, 0, sizeof(*test_context));
    SDL_GL_ResetAttributes();
    if (!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)
        || !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
        || !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
        || !SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile)) {
        fprintf(stderr, "%s: could not request %s context: %s\n",
                failure_prefix, title, SDL_GetError());
        return 0;
    }
    test_context->window = SDL_CreateWindow(
        title, FRAME_WIDTH, FRAME_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (test_context->window == NULL) {
        fprintf(stderr, "%s: could not create %s window: %s\n",
                failure_prefix, title, SDL_GetError());
        return 0;
    }
    test_context->context = SDL_GL_CreateContext(test_context->window);
    if (test_context->context == NULL) {
        fprintf(stderr, "%s: could not create %s context: %s\n",
                failure_prefix, title, SDL_GetError());
        return 0;
    }
    if (!SDL_GL_MakeCurrent(test_context->window,
                            test_context->context)
        || !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major)
        || !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor)
        || !SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                &actual_profile)
        || major < 3 || (major == 3 && minor < 3)
        || (actual_profile & profile) == 0) {
        fprintf(stderr, "%s: requested %s context is unavailable "
                "(actual %d.%d, profile 0x%x): %s\n",
                failure_prefix, title, major, minor, actual_profile,
                SDL_GetError());
        return 0;
    }
    return 1;
}

static int check_compatibility_context_is_rejected(TestContext *test_context)
{
    SdlRenderer *renderer = NULL;

    TEST_CHECK(SDL_GL_MakeCurrent(test_context->window,
                                  test_context->context));
    TEST_CHECK(Sdl_renderer_create(test_context->window, &renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(renderer == NULL);
    TEST_CHECK(SDL_GL_GetCurrentWindow() == test_context->window);
    TEST_CHECK(SDL_GL_GetCurrentContext() == test_context->context);
    TEST_CHECK(glGetString(GL_VERSION) != NULL);
    return 0;
}

static int check_core_context_is_accepted(TestContext *test_context)
{
    const RendererColor clear = {7, 11, 19, 255};
    const RendererColor color = {211, 73, 37, 255};
    GLubyte clear_pixel[4] = {0, 0, 0, 0};
    GLubyte filled_pixel[4] = {0, 0, 0, 0};
    SdlRenderer *renderer = NULL;
    Renderer *frontend;
    int drawable_width = 0;
    int drawable_height = 0;

    TEST_CHECK(SDL_GL_MakeCurrent(test_context->window,
                                  test_context->context));
    TEST_CHECK(Sdl_renderer_create(test_context->window, &renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(renderer != NULL);
    frontend = Sdl_renderer_frontend(renderer);
    TEST_CHECK(frontend != NULL);
    TEST_CHECK(Sdl_renderer_get_drawable_size(
                   renderer, &drawable_width, &drawable_height)
               == RENDERER_STATUS_OK);
    TEST_CHECK(drawable_width > 31);
    TEST_CHECK(drawable_height > 19);
    TEST_CHECK(Sdl_renderer_begin_frame(renderer, drawable_width,
                                        drawable_height, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(frontend, 3.0f, 5.0f, 17.0f, 13.0f,
                                  color) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_end_frame(renderer) == RENDERER_STATUS_OK);
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(10, drawable_height - 11, 1, 1,
                 GL_RGBA, GL_UNSIGNED_BYTE, filled_pixel);
    glReadPixels(30, drawable_height - 11, 1, 1,
                 GL_RGBA, GL_UNSIGNED_BYTE, clear_pixel);
    TEST_CHECK(glGetError() == GL_NO_ERROR);
    TEST_CHECK(memcmp(filled_pixel,
                      (const GLubyte[]){211, 73, 37, 255},
                      sizeof(filled_pixel)) == 0);
    TEST_CHECK(memcmp(clear_pixel,
                      (const GLubyte[]){7, 11, 19, 255},
                      sizeof(clear_pixel)) == 0);
    Sdl_renderer_destroy(renderer);
    TEST_CHECK(SDL_GL_GetCurrentWindow() == test_context->window);
    TEST_CHECK(SDL_GL_GetCurrentContext() == test_context->context);
    TEST_CHECK(glGetString(GL_VERSION) != NULL);
    return 0;
}

int main(void)
{
    TestContext compatibility_context;
    TestContext core_context;
    int sdl_initialized = 0;
    int result = 1;

    memset(&compatibility_context, 0, sizeof(compatibility_context));
    memset(&core_context, 0, sizeof(core_context));
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL video initialization failed: %s\n",
                SDL_GetError());
        goto cleanup;
    }
    sdl_initialized = 1;
    if (!create_context(&core_context, SDL_GL_CONTEXT_PROFILE_CORE,
                        "OpenGL 3.3 core acceptance", 1))
        goto cleanup;
    if (check_core_context_is_accepted(&core_context) != 0)
        goto cleanup;
    destroy_context(&core_context);

    if (create_context(&compatibility_context,
                       SDL_GL_CONTEXT_PROFILE_COMPATIBILITY,
                       "OpenGL 3.3 compatibility rejection", 0)) {
        if (check_compatibility_context_is_rejected(
                &compatibility_context) != 0)
            goto cleanup;
    }
    result = 0;

cleanup:
    destroy_context(&core_context);
    destroy_context(&compatibility_context);
    if (sdl_initialized)
        SDL_Quit();
    return result;
}
