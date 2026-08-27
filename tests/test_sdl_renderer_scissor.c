#include "test_helpers.h"

#include "renderer_backend.h"
#include "renderer_gl_core.h"
#include "sdlrenderer.h"

#include <SDL3/SDL.h>

#include <stddef.h>
#include <string.h>

#define MAX_DRAWS 4

typedef struct CapturedDraw {
    int scissor_enabled;
    RendererRect scissor;
    RendererTransform2D transform;
} CapturedDraw;

typedef struct FakeBackend {
    RendererStatus begin_result;
    int begin_count;
    int end_count;
    int destroy_count;
    int frame_width;
    int frame_height;
    int draw_count;
    int flush_count;
    CapturedDraw draws[MAX_DRAWS];
} FakeBackend;

static FakeBackend backend;
static int fake_window_storage;
static int fake_context_storage;
static int drawable_width = 250;
static int drawable_height = 120;
static bool drawable_size_result = true;

static void fake_proc(void)
{
}

static int rect_equal(RendererRect left, RendererRect right)
{
    return left.x == right.x && left.y == right.y
        && left.width == right.width && left.height == right.height;
}

static int transform_equal(RendererTransform2D left,
                           RendererTransform2D right)
{
    return memcmp(left.m, right.m, sizeof(left.m)) == 0;
}

static RendererStatus fake_begin_frame(void *context, int width, int height,
                                       RendererColor clear_color)
{
    FakeBackend *fake = context;

    (void)clear_color;
    fake->begin_count++;
    fake->frame_width = width;
    fake->frame_height = height;
    return fake->begin_result;
}

static RendererStatus fake_draw(void *context,
                                const RendererBackendDraw *draw)
{
    FakeBackend *fake = context;
    CapturedDraw *captured;

    if (fake->draw_count >= MAX_DRAWS)
        return RENDERER_STATUS_BACKEND_ERROR;
    captured = &fake->draws[fake->draw_count++];
    captured->scissor_enabled = draw->scissor_enabled;
    captured->scissor = draw->scissor;
    captured->transform = draw->transform;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_flush(void *context)
{
    FakeBackend *fake = context;

    fake->flush_count++;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_end_frame(void *context)
{
    FakeBackend *fake = context;

    fake->end_count++;
    return RENDERER_STATUS_OK;
}

static RendererStatus fake_texture_create(
    void *context, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, void **handle)
{
    (void)context;
    (void)desc;
    (void)rgba_pixels;
    (void)pitch;
    (void)handle;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus fake_texture_update(
    void *context, void *handle, RendererRect region,
    const uint8_t *rgba_pixels, size_t pitch)
{
    (void)context;
    (void)handle;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static void fake_texture_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static RendererStatus fake_mesh_create(
    void *context, const RendererVertex2D *vertices,
    size_t vertex_count, void **handle)
{
    (void)context;
    (void)vertices;
    (void)vertex_count;
    (void)handle;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus fake_mesh_update(
    void *context, void *handle, const RendererVertex2D *vertices,
    size_t vertex_count)
{
    (void)context;
    (void)handle;
    (void)vertices;
    (void)vertex_count;
    return RENDERER_STATUS_BACKEND_ERROR;
}

static void fake_mesh_destroy(void *context, void *handle)
{
    (void)context;
    (void)handle;
}

static void fake_destroy(void *context)
{
    FakeBackend *fake = context;

    fake->destroy_count++;
}

static const RendererBackendInterface fake_backend_interface = {
    .begin_frame = fake_begin_frame,
    .draw = fake_draw,
    .flush = fake_flush,
    .end_frame = fake_end_frame,
    .texture_create = fake_texture_create,
    .texture_update = fake_texture_update,
    .texture_destroy = fake_texture_destroy,
    .mesh_create = fake_mesh_create,
    .mesh_update = fake_mesh_update,
    .mesh_destroy = fake_mesh_destroy,
    .destroy = fake_destroy
};

RendererStatus Renderer_gl_core_create(RendererGLProcLoader loader,
                                       void *userdata,
                                       Renderer **renderer)
{
    (void)loader;
    (void)userdata;
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *renderer = Renderer_backend_create(&fake_backend_interface, &backend);
    return *renderer != NULL
        ? RENDERER_STATUS_OK : RENDERER_STATUS_OUT_OF_MEMORY;
}

SDL_Window *SDLCALL __wrap_SDL_GL_GetCurrentWindow(void)
{
    return (SDL_Window *)&fake_window_storage;
}

SDL_GLContext SDLCALL __wrap_SDL_GL_GetCurrentContext(void)
{
    return (SDL_GLContext)&fake_context_storage;
}

SDL_FunctionPointer SDLCALL __wrap_SDL_GL_GetProcAddress(const char *name)
{
    (void)name;
    return fake_proc;
}

bool SDLCALL __wrap_SDL_GetWindowSizeInPixels(SDL_Window *window, int *width,
                                              int *height)
{
    (void)window;
    *width = drawable_width;
    *height = drawable_height;
    return drawable_size_result;
}

static int check_drawable_size_failure_is_reported(void)
{
    const RendererColor clear = {0, 0, 0, 255};
    SDL_Window *window = (SDL_Window *)&fake_window_storage;
    SdlRenderer *sdl_renderer = NULL;
    RendererStatus size_status;
    RendererStatus begin_status;
    int width = 0;
    int height = 0;

    memset(&backend, 0, sizeof(backend));
    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_create(window, &sdl_renderer)
               == RENDERER_STATUS_OK);

    drawable_size_result = false;
    size_status = Sdl_renderer_get_drawable_size(sdl_renderer, &width,
                                                 &height);
    begin_status = Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear);
    drawable_size_result = true;

    TEST_CHECK(size_status == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(begin_status == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(backend.begin_count == 0);
    Sdl_renderer_destroy(sdl_renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_logical_scissor_uses_drawable_pixels(void)
{
    const RendererColor clear = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    const RendererTransform2D transform = {
        {4.0f, 0.0f, 0.0f,
         0.0f, 6.0f, 0.0f,
         31.0f, 37.0f, 1.0f}
    };
    const RendererRect logical_scissor = {3, 5, 6, 8};
    const RendererRect expected_drawable_scissor = {7, 7, 16, 13};
    const RendererRect framebuffer_scissor = {11, 13, 17, 19};
    SDL_Window *window = (SDL_Window *)&fake_window_storage;
    SdlRenderer *sdl_renderer = NULL;
    Renderer *renderer;

    memset(&backend, 0, sizeof(backend));
    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_create(window, &sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(sdl_renderer != NULL);
    TEST_CHECK(Sdl_renderer_set_logical_scissor(
                   sdl_renderer, &logical_scissor)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(backend.begin_count == 1);
    TEST_CHECK(backend.frame_width == drawable_width);
    TEST_CHECK(backend.frame_height == drawable_height);

    renderer = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_set_transform_2d(renderer, transform)
               == RENDERER_STATUS_OK);

    /* The scaled edges enclose the complete logical rectangle:
     * left/top round down while right/bottom round up. */
    TEST_CHECK(Sdl_renderer_set_logical_scissor(
                   sdl_renderer, &logical_scissor)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 0.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);

    /* Renderer rectangles are already framebuffer coordinates and therefore
     * remain independent of the active geometry transform. */
    TEST_CHECK(Renderer_set_scissor(renderer, &framebuffer_scissor)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 1.0f, 1.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);

    TEST_CHECK(Sdl_renderer_set_logical_scissor(sdl_renderer, NULL)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 2.0f, 2.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);

    TEST_CHECK(backend.draw_count == 3);
    TEST_CHECK(backend.draws[0].scissor_enabled);
    TEST_CHECK(rect_equal(backend.draws[0].scissor,
                          expected_drawable_scissor));
    TEST_CHECK(backend.draws[1].scissor_enabled);
    TEST_CHECK(rect_equal(backend.draws[1].scissor,
                          framebuffer_scissor));
    TEST_CHECK(!backend.draws[2].scissor_enabled);
    TEST_CHECK(backend.end_count == 1);

    Sdl_renderer_destroy(sdl_renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_frame_result_is_sticky_until_successful_begin(void)
{
    const RendererColor clear = {0, 0, 0, 255};
    SDL_Window *window = (SDL_Window *)&fake_window_storage;
    SdlRenderer *sdl_renderer = NULL;

    memset(&backend, 0, sizeof(backend));
    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_create(window, &sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(sdl_renderer != NULL);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   sdl_renderer, RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OK);

    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   sdl_renderer, RENDERER_STATUS_OK)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   sdl_renderer, RENDERER_STATUS_OUT_OF_MEMORY)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   sdl_renderer, RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   sdl_renderer, RENDERER_STATUS_OK)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);

    /* Presentation code can inspect the completed frame until the next
     * frame has actually begun. A failed begin must not erase that result. */
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    backend.begin_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OUT_OF_MEMORY);

    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);

    Sdl_renderer_destroy(sdl_renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_transform_failure_is_sticky_until_next_frame(void)
{
    const RendererColor clear = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    const RendererTransform2D invalid = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f}
    };
    const RendererTransform2D replacement = {
        {2.0f, 0.0f, 0.0f,
         0.0f, 3.0f, 0.0f,
         11.0f, 13.0f, 1.0f}
    };
    SDL_Window *window = (SDL_Window *)&fake_window_storage;
    SdlRenderer *sdl_renderer = NULL;
    Renderer *renderer;

    memset(&backend, 0, sizeof(backend));
    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_create(window, &sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(sdl_renderer != NULL);
    TEST_CHECK(Sdl_renderer_set_transform_2d(sdl_renderer, identity)
               == RENDERER_STATUS_INVALID_STATE);

    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_set_transform_2d(sdl_renderer, invalid)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_INVALID_ARGUMENT);

    /* A later valid transform cannot make the incomplete frame presentable
     * or mutate the transform after its first semantic failure. */
    TEST_CHECK(Sdl_renderer_set_transform_2d(sdl_renderer, replacement)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    renderer = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK(Renderer_fill_rect(renderer, 0.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(transform_equal(backend.draws[0].transform, identity));

    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_set_transform_2d(sdl_renderer, replacement)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_frame_result(sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(Renderer_fill_rect(renderer, 0.0f, 0.0f, 1.0f, 1.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 2);
    TEST_CHECK(transform_equal(backend.draws[1].transform, replacement));

    Sdl_renderer_destroy(sdl_renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

static int check_flush_delivers_each_command_once(void)
{
    const RendererColor clear = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    SDL_Window *window = (SDL_Window *)&fake_window_storage;
    SdlRenderer *sdl_renderer = NULL;
    Renderer *renderer;

    memset(&backend, 0, sizeof(backend));
    backend.begin_result = RENDERER_STATUS_OK;
    TEST_CHECK(Sdl_renderer_create(window, &sdl_renderer)
               == RENDERER_STATUS_OK);
    TEST_CHECK(sdl_renderer != NULL);
    TEST_CHECK(Sdl_renderer_flush(NULL) == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_renderer_flush(sdl_renderer)
               == RENDERER_STATUS_INVALID_STATE);

    TEST_CHECK(Sdl_renderer_begin_frame(sdl_renderer, 100, 80, clear)
               == RENDERER_STATUS_OK);
    renderer = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK(renderer != NULL);
    TEST_CHECK(Renderer_fill_rect(renderer, 2.0f, 3.0f, 5.0f, 7.0f,
                                  white) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 0);
    TEST_CHECK(Sdl_renderer_flush(sdl_renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.flush_count == 1);
    TEST_CHECK(Sdl_renderer_end_frame(sdl_renderer) == RENDERER_STATUS_OK);
    TEST_CHECK(backend.draw_count == 1);
    TEST_CHECK(backend.flush_count == 1);
    TEST_CHECK(backend.end_count == 1);

    Sdl_renderer_destroy(sdl_renderer);
    TEST_CHECK(backend.destroy_count == 1);
    return 0;
}

int main(void)
{
    TEST_CHECK(Sdl_renderer_frame_result(NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_renderer_track_frame_result(
                   NULL, RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Sdl_renderer_set_logical_scissor(NULL, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(check_drawable_size_failure_is_reported() == 0);
    TEST_CHECK(check_logical_scissor_uses_drawable_pixels() == 0);
    TEST_CHECK(check_frame_result_is_sticky_until_successful_begin() == 0);
    TEST_CHECK(check_transform_failure_is_sticky_until_next_frame() == 0);
    TEST_CHECK(check_flush_delivers_each_command_once() == 0);
    return 0;
}
