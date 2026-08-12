#include "test_helpers.h"

#include "renderer.h"
#include "renderer_gl.h"
#include "renderer_gl_core.h"
#include "renderer_gl_legacy.h"
#include "sdlrenderer.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_WIDTH 32
#define FRAME_HEIGHT 24
#define PIXEL_COMPONENTS 4
#define PIXEL_TOLERANCE 2

#define TEST_CHECK_CLEANUP(condition)                                       \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: check failed: %s\n",                  \
                    __FILE__, __LINE__, #condition);                        \
            goto cleanup;                                                   \
        }                                                                   \
    } while (0)

typedef void (APIENTRYP gen_framebuffers_fn)(GLsizei, GLuint *);
typedef void (APIENTRYP bind_framebuffer_fn)(GLenum, GLuint);
typedef void (APIENTRYP framebuffer_texture_2d_fn)(GLenum, GLenum, GLenum,
                                                   GLuint, GLint);
typedef GLenum (APIENTRYP check_framebuffer_status_fn)(GLenum);
typedef void (APIENTRYP delete_framebuffers_fn)(GLsizei, const GLuint *);

typedef struct BufferApi {
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLGENBUFFERSPROC gen;
    PFNGLBINDBUFFERPROC bind;
    PFNGLBUFFERDATAPROC data;
    PFNGLDELETEBUFFERSPROC destroy;
} BufferApi;

typedef void (APIENTRYP enable_fn)(GLenum);
typedef void (APIENTRYP disable_fn)(GLenum);
typedef void (APIENTRYP polygon_mode_fn)(GLenum, GLenum);
typedef void (APIENTRYP logic_operation_fn)(GLenum);
typedef void (APIENTRYP get_integer_fn)(GLenum, GLint *);

typedef struct CoreBorrowedState {
    PFNGLGENSAMPLERSPROC gen_samplers;
    PFNGLBINDSAMPLERPROC bind_sampler;
    PFNGLSAMPLERPARAMETERIPROC sampler_parameter;
    PFNGLDELETESAMPLERSPROC delete_samplers;
    PFNGLUSEPROGRAMPROC use_program;
    enable_fn enable;
    disable_fn disable;
    polygon_mode_fn polygon_mode;
    logic_operation_fn logic_operation;
    get_integer_fn get_integer;
    GLuint sampler;
} CoreBorrowedState;

typedef struct FramebufferApi {
    gen_framebuffers_fn gen;
    bind_framebuffer_fn bind;
    framebuffer_texture_2d_fn attach_texture;
    check_framebuffer_status_fn check_status;
    delete_framebuffers_fn destroy;
} FramebufferApi;

typedef struct TestContext {
    SDL_Window *window;
    SDL_GLContext context;
    FramebufferApi framebuffer_api;
    BufferApi buffer_api;
    GLuint framebuffer;
    GLuint color_texture;
} TestContext;

typedef RendererStatus (*RendererFactory)(RendererGLProcLoader loader,
                                          void *userdata,
                                          Renderer **renderer);

typedef struct MissingProcLoader {
    const char *missing_name;
    int requested;
} MissingProcLoader;

typedef struct PixelExpectation {
    int x;
    int y;
    int red;
    int green;
    int blue;
    int alpha;
} PixelExpectation;

static const PixelExpectation semantic_pixels[] = {
    {13, 10,   8,  16,  24, 255},
    { 4,  4, 255,   0,   0, 255},
    { 8,  4, 127,   0, 128, 191},
    {12,  4,   4,   8, 140, 191},
    {19,  4,   0, 255,   0, 255},
    {16,  4,   8,  16,  24, 255},
    { 4, 10,  18,  36,  54, 255},
    {25,  4, 128,  64,   0, 128},
    { 9, 13, 255, 128,   0, 255},
    {15, 13, 255, 255,   0, 255},
    {20, 13,   0, 255,   0, 255},
    {15, 18, 255,   0, 255, 255},
    {20, 18,   0,   0, 255, 255},
    {26, 17,   0, 255, 255, 255}
};

static void *load_gl_proc(void *userdata, const char *name)
{
    (void)userdata;
    return SDL_GL_GetProcAddress(name);
}

static void *load_gl_proc_except(void *userdata, const char *name)
{
    MissingProcLoader *loader = userdata;

    if (strcmp(name, loader->missing_name) == 0) {
        loader->requested = 1;
        return NULL;
    }
    return SDL_GL_GetProcAddress(name);
}

static void *load_framebuffer_proc(const char *core_name,
                                   const char *extension_name)
{
    void *address = SDL_GL_GetProcAddress(core_name);

    if (address == NULL)
        address = SDL_GL_GetProcAddress(extension_name);
    return address;
}

static int load_framebuffer_api(FramebufferApi *api)
{
    memset(api, 0, sizeof(*api));
    api->gen = (gen_framebuffers_fn)load_framebuffer_proc(
        "glGenFramebuffers", "glGenFramebuffersEXT");
    api->bind = (bind_framebuffer_fn)load_framebuffer_proc(
        "glBindFramebuffer", "glBindFramebufferEXT");
    api->attach_texture = (framebuffer_texture_2d_fn)load_framebuffer_proc(
        "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    api->check_status = (check_framebuffer_status_fn)load_framebuffer_proc(
        "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    api->destroy = (delete_framebuffers_fn)load_framebuffer_proc(
        "glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    return api->gen != NULL && api->bind != NULL
        && api->attach_texture != NULL && api->check_status != NULL
        && api->destroy != NULL;
}

static int load_buffer_api(BufferApi *api)
{
    memset(api, 0, sizeof(*api));
    api->active_texture = (PFNGLACTIVETEXTUREPROC)load_framebuffer_proc(
        "glActiveTexture", "glActiveTextureARB");
    api->gen = (PFNGLGENBUFFERSPROC)load_framebuffer_proc(
        "glGenBuffers", "glGenBuffersARB");
    api->bind = (PFNGLBINDBUFFERPROC)load_framebuffer_proc(
        "glBindBuffer", "glBindBufferARB");
    api->data = (PFNGLBUFFERDATAPROC)load_framebuffer_proc(
        "glBufferData", "glBufferDataARB");
    api->destroy = (PFNGLDELETEBUFFERSPROC)load_framebuffer_proc(
        "glDeleteBuffers", "glDeleteBuffersARB");
    return api->active_texture != NULL && api->gen != NULL
        && api->bind != NULL && api->data != NULL && api->destroy != NULL;
}

static int load_core_borrowed_state_api(CoreBorrowedState *state)
{
    int loaded;

    memset(state, 0, sizeof(*state));
    state->gen_samplers = (PFNGLGENSAMPLERSPROC)SDL_GL_GetProcAddress(
        "glGenSamplers");
    state->bind_sampler = (PFNGLBINDSAMPLERPROC)SDL_GL_GetProcAddress(
        "glBindSampler");
    state->sampler_parameter = (PFNGLSAMPLERPARAMETERIPROC)
        SDL_GL_GetProcAddress("glSamplerParameteri");
    state->delete_samplers = (PFNGLDELETESAMPLERSPROC)SDL_GL_GetProcAddress(
        "glDeleteSamplers");
    state->use_program = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress(
        "glUseProgram");
    state->enable = (enable_fn)SDL_GL_GetProcAddress("glEnable");
    state->disable = (disable_fn)SDL_GL_GetProcAddress("glDisable");
    state->polygon_mode = (polygon_mode_fn)SDL_GL_GetProcAddress(
        "glPolygonMode");
    state->logic_operation = (logic_operation_fn)SDL_GL_GetProcAddress(
        "glLogicOp");
    state->get_integer = (get_integer_fn)SDL_GL_GetProcAddress(
        "glGetIntegerv");
    loaded = state->gen_samplers != NULL && state->bind_sampler != NULL
        && state->sampler_parameter != NULL
        && state->delete_samplers != NULL && state->use_program != NULL
        && state->enable != NULL && state->disable != NULL
        && state->polygon_mode != NULL && state->logic_operation != NULL
        && state->get_integer != NULL;
    if (!loaded)
        memset(state, 0, sizeof(*state));
    return loaded;
}

static int verify_context_attributes(int major, int minor, int profile,
                                     const char *title)
{
    int actual_major = 0;
    int actual_minor = 0;
    int actual_profile = 0;

    if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,
                            &actual_major) != 0
        || SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,
                               &actual_minor) != 0
        || SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                               &actual_profile) != 0) {
        fprintf(stderr, "Could not query %s context attributes: %s\n",
                title, SDL_GetError());
        return 1;
    }
    if (actual_major < major
        || (actual_major == major && actual_minor < minor)) {
        fprintf(stderr, "%s context is OpenGL %d.%d; expected at least "
                "%d.%d\n", title, actual_major, actual_minor,
                major, minor);
        return 1;
    }
    if (profile == SDL_GL_CONTEXT_PROFILE_CORE
        && actual_profile != SDL_GL_CONTEXT_PROFILE_CORE) {
        fprintf(stderr, "%s context profile is 0x%x; expected core\n",
                title, actual_profile);
        return 1;
    }
    if (profile == 0
        && (actual_profile & (SDL_GL_CONTEXT_PROFILE_CORE
                              | SDL_GL_CONTEXT_PROFILE_ES)) != 0) {
        fprintf(stderr, "%s context profile is 0x%x; expected a "
                "compatibility context\n", title, actual_profile);
        return 1;
    }
    if (glGetString(GL_VERSION) == NULL) {
        fprintf(stderr, "%s context has no GL_VERSION string\n", title);
        return 1;
    }
    return 0;
}

static int create_context(TestContext *test_context, int major, int minor,
                          int profile, const char *title)
{
    memset(test_context, 0, sizeof(*test_context));
    SDL_GL_ResetAttributes();
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile) != 0
        || SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) != 0
        || SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) != 0
        || SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) != 0
        || SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8) != 0) {
        fprintf(stderr, "Could not request %s context: %s\n",
                title, SDL_GetError());
        return 1;
    }

    test_context->window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        FRAME_WIDTH, FRAME_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (test_context->window == NULL) {
        fprintf(stderr, "Could not create %s window: %s\n",
                title, SDL_GetError());
        return 1;
    }
    test_context->context = SDL_GL_CreateContext(test_context->window);
    if (test_context->context == NULL) {
        fprintf(stderr, "Could not create %s context: %s\n",
                title, SDL_GetError());
        return 1;
    }
    if (SDL_GL_MakeCurrent(test_context->window,
                           test_context->context) != 0) {
        fprintf(stderr, "Could not make %s context current: %s\n",
                title, SDL_GetError());
        return 1;
    }
    if (verify_context_attributes(major, minor, profile, title) != 0)
        return 1;
    if (!load_framebuffer_api(&test_context->framebuffer_api)) {
        fprintf(stderr, "%s context lacks framebuffer entry points\n",
                title);
        return 1;
    }
    if (!load_buffer_api(&test_context->buffer_api)) {
        fprintf(stderr, "%s context lacks buffer entry points\n", title);
        return 1;
    }

    glGenTextures(1, &test_context->color_texture);
    glBindTexture(GL_TEXTURE_2D, test_context->color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FRAME_WIDTH, FRAME_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    test_context->framebuffer_api.gen(1, &test_context->framebuffer);
    test_context->framebuffer_api.bind(GL_FRAMEBUFFER,
                                       test_context->framebuffer);
    test_context->framebuffer_api.attach_texture(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        test_context->color_texture, 0);
    if (test_context->framebuffer_api.check_status(GL_FRAMEBUFFER)
            != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "%s RGBA8 framebuffer is incomplete\n", title);
        return 1;
    }
    return 0;
}

static void destroy_context(TestContext *test_context)
{
    if (test_context->context != NULL
        && SDL_GL_MakeCurrent(test_context->window,
                              test_context->context) == 0) {
        if (test_context->framebuffer != 0)
            test_context->framebuffer_api.destroy(
                1, &test_context->framebuffer);
        if (test_context->color_texture != 0)
            glDeleteTextures(1, &test_context->color_texture);
    }
    if (test_context->context != NULL)
        SDL_GL_DeleteContext(test_context->context);
    if (test_context->window != NULL)
        SDL_DestroyWindow(test_context->window);
    memset(test_context, 0, sizeof(*test_context));
}

static int check_factory_failure_contract(RendererFactory factory,
                                          TestContext *test_context,
                                          const char *missing_name)
{
    MissingProcLoader loader = {missing_name, 0};
    Renderer *renderer = NULL;
    int result = 1;

    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    TEST_CHECK_CLEANUP(factory(load_gl_proc, NULL, NULL)
                       == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK_CLEANUP(factory(NULL, NULL, &renderer)
                       == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK_CLEANUP(renderer == NULL);
    TEST_CHECK_CLEANUP(factory(load_gl_proc_except, &loader, &renderer)
                       == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK_CLEANUP(loader.requested);
    TEST_CHECK_CLEANUP(renderer == NULL);
    TEST_CHECK_CLEANUP(SDL_GL_GetCurrentContext()
                       == test_context->context);
    TEST_CHECK_CLEANUP(glGetString(GL_VERSION) != NULL);
    result = 0;

cleanup:
    if (renderer != NULL) {
        if (SDL_GL_GetCurrentContext() != test_context->context
            && SDL_GL_MakeCurrent(test_context->window,
                                  test_context->context) != 0) {
            fprintf(stderr, "Could not restore context for renderer "
                    "cleanup: %s\n", SDL_GetError());
        } else {
            Renderer_destroy(renderer);
        }
    }
    return result;
}

static void discard_gl_errors(void)
{
    while (glGetError() != GL_NO_ERROR)
        ;
}

static int begin_core_borrowed_draw_state(TestContext *test_context,
                                          CoreBorrowedState *state)
{
    if (!load_core_borrowed_state_api(state)) {
        fprintf(stderr, "OpenGL 3.3 core context lacks borrowed-state "
                "entry points\n");
        return 1;
    }
    discard_gl_errors();
    state->gen_samplers(1, &state->sampler);
    if (state->sampler == 0) {
        fprintf(stderr, "Could not create inherited sampler object\n");
        return 1;
    }
    test_context->buffer_api.active_texture(GL_TEXTURE0);
    state->bind_sampler(0, state->sampler);
    state->sampler_parameter(state->sampler, GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST_MIPMAP_NEAREST);
    state->enable(GL_RASTERIZER_DISCARD);
    state->polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
    state->logic_operation(GL_XOR);
    state->enable(GL_COLOR_LOGIC_OP);
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not establish inherited core draw state\n");
        return 1;
    }
    return 0;
}

static int check_core_program_released(CoreBorrowedState *state)
{
    GLint current_program = 0;

    state->get_integer(GL_CURRENT_PROGRAM, &current_program);
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not query current program after renderer "
                "destruction\n");
        return 1;
    }
    if (current_program != 0) {
        fprintf(stderr, "renderer-owned program %d remains current after "
                "renderer destruction\n", current_program);
        return 1;
    }
    return 0;
}

static void end_core_borrowed_draw_state(CoreBorrowedState *state)
{
    if (state->disable == NULL)
        return;

    discard_gl_errors();
    state->use_program(0);
    state->bind_sampler(0, 0);
    if (state->sampler != 0)
        state->delete_samplers(1, &state->sampler);
    state->disable(GL_RASTERIZER_DISCARD);
    state->polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
    state->disable(GL_COLOR_LOGIC_OP);
    memset(state, 0, sizeof(*state));
    discard_gl_errors();
}

static int check_borrowed_upload_state(TestContext *test_context,
                                       GLuint unpack_buffer)
{
    GLint active_texture = 0;
    GLint actual_unpack_buffer = 0;

    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &actual_unpack_buffer);
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not query borrowed upload state\n");
        return 1;
    }
    if (active_texture != GL_TEXTURE1
        || actual_unpack_buffer != (GLint)unpack_buffer) {
        fprintf(stderr, "borrowed upload state changed: active texture "
                "0x%x (expected 0x%x), unpack buffer %d (expected %u)\n",
                active_texture, GL_TEXTURE1, actual_unpack_buffer,
                (unsigned int)unpack_buffer);
        return 1;
    }
    return 0;
}

static int begin_borrowed_upload_state(TestContext *test_context,
                                       GLuint *unpack_buffer)
{
    static const GLubyte buffer_contents[64] = {0};
    BufferApi *api = &test_context->buffer_api;

    *unpack_buffer = 0;
    discard_gl_errors();
    api->gen(1, unpack_buffer);
    if (*unpack_buffer == 0) {
        fprintf(stderr, "Could not create borrowed pixel unpack buffer\n");
        return 1;
    }
    api->bind(GL_PIXEL_UNPACK_BUFFER, *unpack_buffer);
    api->data(GL_PIXEL_UNPACK_BUFFER, (GLsizeiptr)sizeof(buffer_contents),
              buffer_contents, GL_STATIC_DRAW);
    api->active_texture(GL_TEXTURE1);
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not establish borrowed upload state\n");
        return 1;
    }
    return check_borrowed_upload_state(test_context, *unpack_buffer);
}

static void end_borrowed_upload_state(TestContext *test_context,
                                      GLuint *unpack_buffer)
{
    BufferApi *api = &test_context->buffer_api;

    if (*unpack_buffer == 0)
        return;
    discard_gl_errors();
    api->active_texture(GL_TEXTURE0);
    api->bind(GL_PIXEL_UNPACK_BUFFER, 0);
    api->destroy(1, unpack_buffer);
    *unpack_buffer = 0;
    discard_gl_errors();
}

static int render_scene(RendererFactory factory, TestContext *test_context,
                        GLubyte *pixels, int exercise_core_borrowed_state)
{
    static const uint8_t texture_pixels[] = {
        255, 255,   0, 255,    0, 255, 255, 255,
         11,  12,  13,  14,
        255,   0, 255, 255,  255, 255, 255, 255,
         21,  22,  23,  24
    };
    static const uint8_t texture_update_pixels[] = {
          0, 255,   0, 255,  31, 32, 33, 34,
          0,   0, 255, 255,  41, 42, 43, 44
    };
    static const RendererVertex2D mesh_vertices[] = {
        {24.0f, 12.0f, 0.0f, 0.0f, {0, 255, 255, 255}},
        {30.0f, 20.0f, 0.0f, 0.0f, {0, 255, 255, 255}},
        {22.0f, 20.0f, 0.0f, 0.0f, {0, 255, 255, 255}}
    };
    static const RendererVertex2D textured_triangle_vertices[] = {
        {24.0f, 2.0f, 0.0f, 0.0f, {128, 64, 192, 128}},
        {30.0f, 2.0f, 0.0f, 0.0f, {128, 64, 192, 128}},
        {24.0f, 10.0f, 0.0f, 0.0f, {128, 64, 192, 128}}
    };
    const RendererTextureDesc texture_desc = {
        .width = 2,
        .height = 2,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererColor clear = {8, 16, 24, 255};
    const RendererRect scissor = {18, 2, 4, 6};
    const RendererTransform2D translation = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         6.0f, 0.0f, 1.0f}
    };
    const RendererTransform2D identity = {
        {1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f}
    };
    Renderer *renderer = NULL;
    RendererTexture *texture = NULL;
    RendererMesh *mesh = NULL;
    CoreBorrowedState core_borrowed_state;
    GLuint unpack_buffer = 0;
    int result = 1;

    memset(&core_borrowed_state, 0, sizeof(core_borrowed_state));
    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    test_context->framebuffer_api.bind(GL_FRAMEBUFFER,
                                       test_context->framebuffer);
    if (exercise_core_borrowed_state) {
        TEST_CHECK_CLEANUP(begin_core_borrowed_draw_state(
                               test_context, &core_borrowed_state) == 0);
    }
    TEST_CHECK_CLEANUP(begin_borrowed_upload_state(
                           test_context, &unpack_buffer) == 0);
    TEST_CHECK_CLEANUP(factory(load_gl_proc, NULL, &renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(renderer != NULL);
    TEST_CHECK_CLEANUP(check_borrowed_upload_state(
                           test_context, unpack_buffer) == 0);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &texture_desc, texture_pixels, 12,
                           &texture)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(check_borrowed_upload_state(
                           test_context, unpack_buffer) == 0);
    TEST_CHECK_CLEANUP(Renderer_texture_update(
                           renderer, texture, (RendererRect){1, 0, 1, 2},
                           texture_update_pixels, 8)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(check_borrowed_upload_state(
                           test_context, unpack_buffer) == 0);
    end_borrowed_upload_state(test_context, &unpack_buffer);
    TEST_CHECK_CLEANUP(Renderer_mesh_create(renderer, mesh_vertices, 3,
                                             &mesh)
                       == RENDERER_STATUS_OK);

    TEST_CHECK_CLEANUP(Renderer_begin_frame(renderer, FRAME_WIDTH,
                                             FRAME_HEIGHT, clear)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 2.0f, 2.0f, 8.0f, 6.0f,
                           (RendererColor){255, 0, 0, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 6.0f, 2.0f, 8.0f, 6.0f,
                           (RendererColor){0, 0, 255, 128})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer,
                                           RENDERER_BLEND_ADDITIVE)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 2.0f, 9.0f, 6.0f, 3.0f,
                           (RendererColor){20, 40, 60, 128})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_OPAQUE)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_scissor(renderer, &scissor)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 16.0f, 0.0f, 10.0f, 10.0f,
                           (RendererColor){0, 255, 0, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_scissor(renderer, NULL)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_transform_2d(renderer, translation)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 2.0f, 12.0f, 4.0f, 4.0f,
                           (RendererColor){255, 128, 0, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_transform_2d(renderer, identity)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_triangles(
                           renderer, texture, textured_triangle_vertices, 3)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, texture,
                           14.0f, 12.0f, 22.0f, 20.0f,
                           0.0f, 0.0f, 1.0f, 1.0f,
                           (RendererColor){255, 255, 255, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_mesh(renderer, mesh)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);

    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, FRAME_WIDTH, FRAME_HEIGHT,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);
    result = 0;

cleanup:
    end_borrowed_upload_state(test_context, &unpack_buffer);
    if (renderer != NULL) {
        if (SDL_GL_GetCurrentContext() != test_context->context
            && SDL_GL_MakeCurrent(test_context->window,
                                  test_context->context) != 0) {
            fprintf(stderr, "Could not restore context for renderer "
                    "cleanup: %s\n", SDL_GetError());
            result = 1;
        } else {
            Renderer_destroy(renderer);
            renderer = NULL;
            /* Destroying a backend must retain its borrowed context. */
            if (SDL_GL_GetCurrentContext() != test_context->context
                || glGetString(GL_VERSION) == NULL) {
                fprintf(stderr, "Renderer destruction changed or invalidated "
                        "its borrowed context\n");
                result = 1;
            }
            if (exercise_core_borrowed_state
                && check_core_program_released(
                       &core_borrowed_state) != 0) {
                result = 1;
            }
        }
    }
    /* Draw correctness takes precedence; inherited draw state need not be
     * restored by the backend after the frame. */
    end_core_borrowed_draw_state(&core_borrowed_state);
    return result;
}

static int render_texture_sampling_scene(RendererFactory factory,
                                         TestContext *test_context,
                                         GLubyte *pixels)
{
    static const uint8_t texture_pixels[] = {
        255,   0,   0, 255,    0, 255,   0, 255,
          0,   0, 255, 255,  255, 255, 255, 255
    };
    const RendererTextureDesc nearest_clamp_desc = {
        .width = 2,
        .height = 2,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererTextureDesc linear_clamp_desc = {
        .width = 2,
        .height = 2,
        .filter = RENDERER_TEXTURE_FILTER_LINEAR,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererTextureDesc nearest_repeat_desc = {
        .width = 2,
        .height = 2,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_REPEAT
    };
    const RendererColor black = {0, 0, 0, 255};
    const RendererColor white = {255, 255, 255, 255};
    Renderer *renderer = NULL;
    RendererTexture *nearest_clamp = NULL;
    RendererTexture *linear_clamp = NULL;
    RendererTexture *nearest_repeat = NULL;
    int result = 1;

    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    test_context->framebuffer_api.bind(GL_FRAMEBUFFER,
                                       test_context->framebuffer);
    TEST_CHECK_CLEANUP(factory(load_gl_proc, NULL, &renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &nearest_clamp_desc, texture_pixels, 8,
                           &nearest_clamp) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &linear_clamp_desc, texture_pixels, 8,
                           &linear_clamp) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &nearest_repeat_desc, texture_pixels, 8,
                           &nearest_repeat) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_begin_frame(renderer, FRAME_WIDTH,
                                             FRAME_HEIGHT, black)
                       == RENDERER_STATUS_OK);

    /* All checks read the center of a six-pixel quad.  Constant UVs keep the
     * result independent of triangle edges and framebuffer interpolation. */
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, nearest_clamp,
                           2.0f, 2.0f, 8.0f, 8.0f,
                           0.375f, 0.25f, 0.375f, 0.25f, white)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, nearest_clamp,
                           10.0f, 2.0f, 16.0f, 8.0f,
                           1.25f, 0.25f, 1.25f, 0.25f, white)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, linear_clamp,
                           18.0f, 2.0f, 24.0f, 8.0f,
                           0.5f, 0.25f, 0.5f, 0.25f, white)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, nearest_repeat,
                           2.0f, 10.0f, 8.0f, 16.0f,
                           1.25f, 0.25f, 1.25f, 0.25f, white)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_end_frame(renderer) == RENDERER_STATUS_OK);

    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, FRAME_WIDTH, FRAME_HEIGHT,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);
    result = 0;

cleanup:
    if (renderer != NULL)
        Renderer_destroy(renderer);
    return result;
}

static const GLubyte *pixel_at(const GLubyte *pixels, int x, int y)
{
    return pixels + (((FRAME_HEIGHT - 1 - y) * FRAME_WIDTH + x)
                     * PIXEL_COMPONENTS);
}

static int component_near(GLubyte actual, int expected)
{
    int difference = (int)actual - expected;

    if (difference < 0)
        difference = -difference;
    return difference <= PIXEL_TOLERANCE;
}

static int check_rgb(const GLubyte *pixels, int x, int y,
                     int red, int green, int blue)
{
    const GLubyte *pixel = pixel_at(pixels, x, y);

    if (!component_near(pixel[0], red)
        || !component_near(pixel[1], green)
        || !component_near(pixel[2], blue)) {
        fprintf(stderr, "pixel (%d,%d): got %u,%u,%u,%u; "
                "expected %d,%d,%d\n", x, y,
                (unsigned int)pixel[0], (unsigned int)pixel[1],
                (unsigned int)pixel[2], (unsigned int)pixel[3],
                red, green, blue);
        return 1;
    }
    return 0;
}

static int check_rgba(const GLubyte *pixels, int x, int y,
                      int red, int green, int blue, int alpha)
{
    const GLubyte *pixel = pixel_at(pixels, x, y);

    if (check_rgb(pixels, x, y, red, green, blue) != 0)
        return 1;
    if (!component_near(pixel[3], alpha)) {
        fprintf(stderr, "pixel (%d,%d): got alpha %u; expected %d\n",
                x, y, (unsigned int)pixel[3], alpha);
        return 1;
    }
    return 0;
}

static int check_texture_sampling_pixels(const GLubyte *pixels)
{
    /* Nearest chooses the red texel instead of blending toward green. */
    TEST_CHECK(check_rgba(pixels, 5, 5, 255, 0, 0, 255) == 0);
    /* Clamp keeps an out-of-range horizontal coordinate at the green edge. */
    TEST_CHECK(check_rgba(pixels, 13, 5, 0, 255, 0, 255) == 0);
    /* Linear sampling halfway between the top two texels mixes them evenly. */
    TEST_CHECK(check_rgba(pixels, 21, 5, 128, 128, 0, 255) == 0);
    /* Repeat maps the same out-of-range coordinate back to the red texel. */
    TEST_CHECK(check_rgba(pixels, 5, 13, 255, 0, 0, 255) == 0);
    return 0;
}

static void draw_legacy_quad(float left, float bottom,
                             float right, float top,
                             GLubyte red, GLubyte green, GLubyte blue)
{
    glColor4ub(red, green, blue, 255);
    glBegin(GL_QUADS);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glEnd();
}

static int check_sdl_renderer_mixed_legacy_scene(TestContext *test_context)
{
    const RendererColor clear = {8, 16, 24, 255};
    const RendererColor green = {0, 255, 0, 255};
    GLubyte pixels[FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    SdlRenderer *sdl_renderer = NULL;
    Renderer *frontend;
    int drawable_width = 0;
    int drawable_height = 0;
    int result = 1;

    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    TEST_CHECK_CLEANUP(Sdl_renderer_create(test_context->window,
                                           &sdl_renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(sdl_renderer != NULL);
    frontend = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK_CLEANUP(frontend != NULL);
    TEST_CHECK_CLEANUP(Sdl_renderer_get_drawable_size(
                           sdl_renderer, &drawable_width, &drawable_height)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(drawable_width == FRAME_WIDTH);
    TEST_CHECK_CLEANUP(drawable_height == FRAME_HEIGHT);

    test_context->framebuffer_api.bind(GL_FRAMEBUFFER,
                                       test_context->framebuffer);
    TEST_CHECK_CLEANUP(Sdl_renderer_begin_frame(
                           sdl_renderer, FRAME_WIDTH, FRAME_HEIGHT, clear)
                       == RENDERER_STATUS_OK);

    TEST_CHECK_CLEANUP(Sdl_renderer_prepare_legacy(
                           sdl_renderer, SDL_RENDERER_LEGACY_BOTTOM_LEFT)
                       == RENDERER_STATUS_OK);
    draw_legacy_quad(2.0f, 2.0f, 14.0f, 12.0f, 255, 0, 0);

    /* A semantic draw owns its complete rendering state even when a direct
     * compatibility draw leaves hostile fixed-function state behind. */
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    test_context->buffer_api.active_texture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);

    TEST_CHECK_CLEANUP(Renderer_fill_rect(frontend,
                                          8.0f, 8.0f, 14.0f, 12.0f,
                                          green) == RENDERER_STATUS_OK);

    /* Preparing another direct draw is also the semantic/direct order barrier. */
    glEnable(0);
    TEST_CHECK_CLEANUP(Sdl_renderer_prepare_legacy(
                           sdl_renderer, SDL_RENDERER_LEGACY_TOP_LEFT)
                       == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK_CLEANUP(Sdl_renderer_prepare_legacy(
                           sdl_renderer, SDL_RENDERER_LEGACY_TOP_LEFT)
                       == RENDERER_STATUS_OK);
    draw_legacy_quad(16.0f, 14.0f, 28.0f, 22.0f, 0, 0, 255);

    /* End failure leaves the frame active and retryable without replaying
     * the already flushed semantic command. */
    glEnable(0);
    TEST_CHECK_CLEANUP(Sdl_renderer_end_frame(sdl_renderer)
                       == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK_CLEANUP(Sdl_renderer_end_frame(sdl_renderer)
                       == RENDERER_STATUS_OK);
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, FRAME_WIDTH, FRAME_HEIGHT,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);

    TEST_CHECK_CLEANUP(check_rgba(pixels, 30, 2, 8, 16, 24, 255) == 0);
    TEST_CHECK_CLEANUP(check_rgba(pixels, 4, 18, 255, 0, 0, 255) == 0);
    TEST_CHECK_CLEANUP(check_rgba(pixels, 18, 10, 0, 255, 0, 255) == 0);
    /* The semantic draw was submitted after the bottom-left direct draw. */
    TEST_CHECK_CLEANUP(check_rgba(pixels, 10, 16, 0, 255, 0, 255) == 0);
    TEST_CHECK_CLEANUP(check_rgba(pixels, 26, 16, 0, 0, 255, 255) == 0);
    /* prepare_legacy flushed the semantic draw before this top-left quad. */
    TEST_CHECK_CLEANUP(check_rgba(pixels, 18, 16, 0, 0, 255, 255) == 0);
    result = 0;

cleanup:
    if (sdl_renderer != NULL) {
        Sdl_renderer_destroy(sdl_renderer);
        if (SDL_GL_GetCurrentContext() != test_context->context
            || glGetString(GL_VERSION) == NULL) {
            fprintf(stderr, "SDL renderer destruction changed or invalidated "
                    "its borrowed context\n");
            result = 1;
        }
    }
    return result;
}

static int check_sdl_renderer_rejects_core_context(
    TestContext *test_context)
{
    SdlRenderer *sdl_renderer = NULL;

    TEST_CHECK(SDL_GL_MakeCurrent(test_context->window,
                                  test_context->context) == 0);
    TEST_CHECK(Sdl_renderer_create(test_context->window, &sdl_renderer)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(sdl_renderer == NULL);
    TEST_CHECK(SDL_GL_GetCurrentContext() == test_context->context);
    TEST_CHECK(glGetString(GL_VERSION) != NULL);
    return 0;
}

static int check_semantic_pixels(const GLubyte *pixels)
{
    size_t index;

    for (index = 0;
         index < sizeof(semantic_pixels) / sizeof(semantic_pixels[0]);
         index++) {
        const PixelExpectation *expected = &semantic_pixels[index];

        TEST_CHECK(check_rgba(pixels, expected->x, expected->y,
                              expected->red, expected->green,
                              expected->blue, expected->alpha) == 0);
    }
    return 0;
}

static int compare_interior_samples(const GLubyte *legacy,
                                    const GLubyte *core)
{
    size_t sample_index;

    for (sample_index = 0;
         sample_index < sizeof(semantic_pixels) / sizeof(semantic_pixels[0]);
         sample_index++) {
        const PixelExpectation *sample = &semantic_pixels[sample_index];
        const GLubyte *legacy_pixel = pixel_at(legacy, sample->x, sample->y);
        const GLubyte *core_pixel = pixel_at(core, sample->x, sample->y);
        size_t component;

        for (component = 0; component < PIXEL_COMPONENTS; component++) {
            int difference = (int)legacy_pixel[component]
                - (int)core_pixel[component];

            if (difference < 0)
                difference = -difference;
            if (difference > PIXEL_TOLERANCE) {
                fprintf(stderr, "legacy/core mismatch at (%d,%d), "
                        "component %lu: %u != %u\n",
                        sample->x, sample->y, (unsigned long)component,
                        (unsigned int)legacy_pixel[component],
                        (unsigned int)core_pixel[component]);
                return 1;
            }
        }
    }
    return 0;
}

static int check_forward_compatible_legacy_rejection(void)
{
    SDL_Window *window = NULL;
    SDL_GLContext context = NULL;
    Renderer *renderer = NULL;
    GLint context_flags = 0;
    GLint major = 0;
    GLint minor = 0;
    RendererStatus status;
    int result = 0;

    SDL_GL_ResetAttributes();
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                               SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG) != 0) {
        fprintf(stderr, "SKIP: OpenGL 3.0 forward-compatible context "
                "attributes are unsupported: %s\n", SDL_GetError());
        goto cleanup;
    }
    window = SDL_CreateWindow(
        "OpenGL 3.0 forward-compatible legacy rejection",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        FRAME_WIDTH, FRAME_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        fprintf(stderr, "SKIP: could not create a window for the OpenGL "
                "3.0 forward-compatible context: %s\n", SDL_GetError());
        goto cleanup;
    }
    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SKIP: OpenGL 3.0 forward-compatible context is "
                "unavailable: %s\n", SDL_GetError());
        goto cleanup;
    }
    if (SDL_GL_MakeCurrent(window, context) != 0) {
        fprintf(stderr, "SKIP: could not make the OpenGL 3.0 "
                "forward-compatible context current: %s\n", SDL_GetError());
        goto cleanup;
    }

    discard_gl_errors();
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (glGetError() != GL_NO_ERROR || major < 3
        || (context_flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) == 0) {
        fprintf(stderr, "SKIP: driver did not provide the requested OpenGL "
                "3.0 forward-compatible context (actual %d.%d, flags "
                "0x%x)\n", major, minor, context_flags);
        goto cleanup;
    }

    status = Renderer_gl_legacy_create(load_gl_proc, NULL, &renderer);
    if (status != RENDERER_STATUS_BACKEND_ERROR || renderer != NULL) {
        fprintf(stderr, "legacy factory accepted OpenGL %d.%d "
                "forward-compatible context (status %d, renderer %p)\n",
                major, minor, (int)status, (void *)renderer);
        result = 1;
        goto cleanup;
    }
    if (SDL_GL_GetCurrentContext() != context
        || glGetString(GL_VERSION) == NULL) {
        fprintf(stderr, "legacy factory rejection changed or invalidated "
                "the borrowed forward-compatible context\n");
        result = 1;
    }

cleanup:
    if (renderer != NULL) {
        if (SDL_GL_GetCurrentContext() == context
            || SDL_GL_MakeCurrent(window, context) == 0) {
            Renderer_destroy(renderer);
        }
    }
    if (context != NULL)
        SDL_GL_DeleteContext(context);
    if (window != NULL)
        SDL_DestroyWindow(window);
    return result;
}

int main(void)
{
    TestContext legacy_context;
    TestContext core_context;
    GLubyte legacy_pixels[FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    GLubyte core_pixels[FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    GLubyte legacy_sampling_pixels[
        FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    GLubyte core_sampling_pixels[
        FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    int sdl_initialized = 0;
    int result = 1;

    memset(&legacy_context, 0, sizeof(legacy_context));
    memset(&core_context, 0, sizeof(core_context));
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video initialization failed: %s\n",
                SDL_GetError());
        goto cleanup;
    }
    sdl_initialized = 1;
    if (check_forward_compatible_legacy_rejection() != 0)
        goto cleanup;
    if (create_context(&legacy_context, 2, 1, 0, "legacy renderer") != 0)
        goto cleanup;
    if (check_factory_failure_contract(Renderer_gl_legacy_create,
                                       &legacy_context,
                                       "glDeleteTextures") != 0)
        goto cleanup;
    if (render_scene(Renderer_gl_legacy_create, &legacy_context,
                     legacy_pixels, 0) != 0)
        goto cleanup;
    if (render_texture_sampling_scene(Renderer_gl_legacy_create,
                                      &legacy_context,
                                      legacy_sampling_pixels) != 0)
        goto cleanup;
    TEST_CHECK_CLEANUP(check_texture_sampling_pixels(
                           legacy_sampling_pixels) == 0);
    if (check_sdl_renderer_mixed_legacy_scene(&legacy_context) != 0)
        goto cleanup;
    destroy_context(&legacy_context);

    if (create_context(&core_context, 3, 3,
                       SDL_GL_CONTEXT_PROFILE_CORE,
                       "OpenGL 3.3 core renderer") != 0)
        goto cleanup;
    if (check_sdl_renderer_rejects_core_context(&core_context) != 0)
        goto cleanup;
    if (check_factory_failure_contract(Renderer_gl_core_create,
                                       &core_context,
                                       "glDrawArrays") != 0)
        goto cleanup;
    if (render_scene(Renderer_gl_core_create, &core_context,
                     core_pixels, 1) != 0)
        goto cleanup;
    if (render_texture_sampling_scene(Renderer_gl_core_create,
                                      &core_context,
                                      core_sampling_pixels) != 0)
        goto cleanup;
    TEST_CHECK_CLEANUP(check_texture_sampling_pixels(core_sampling_pixels)
                       == 0);
    TEST_CHECK_CLEANUP(check_semantic_pixels(legacy_pixels) == 0);
    TEST_CHECK_CLEANUP(check_semantic_pixels(core_pixels) == 0);
    TEST_CHECK_CLEANUP(compare_interior_samples(legacy_pixels,
                                                core_pixels) == 0);
    result = 0;

cleanup:
    destroy_context(&core_context);
    destroy_context(&legacy_context);
    if (sdl_initialized)
        SDL_Quit();
    return result;
}
