#include "test_helpers.h"

#include "renderer.h"
#include "renderer_gl.h"
#include "renderer_gl_core.h"
#include "renderer_gl_legacy.h"
#include "sdlrenderer.h"
#include "text_geometry.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_WIDTH 64
#define FRAME_HEIGHT 56
#define PIXEL_COMPONENTS 4
#define EXPECTED_PIXEL_TOLERANCE 2
#define STRICT_PIXEL_TOLERANCE 0
#define LINE_PIXEL_TOLERANCE 2
#define TEXT_PIXEL_TOLERANCE 2
#define COMPARISON_TEXT_ATLAS_WIDTH 8
#define COMPARISON_TEXT_ATLAS_HEIGHT 4

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

typedef struct LegacyInteropApi {
    PFNGLCREATESHADERPROC create_shader;
    PFNGLSHADERSOURCEPROC shader_source;
    PFNGLCOMPILESHADERPROC compile_shader;
    PFNGLGETSHADERIVPROC get_shader_integer;
    PFNGLDELETESHADERPROC delete_shader;
    PFNGLCREATEPROGRAMPROC create_program;
    PFNGLATTACHSHADERPROC attach_shader;
    PFNGLLINKPROGRAMPROC link_program;
    PFNGLGETPROGRAMIVPROC get_program_integer;
    PFNGLDELETEPROGRAMPROC delete_program;
    PFNGLUSEPROGRAMPROC use_program;
    PFNGLBLENDEQUATIONPROC blend_equation;
} LegacyInteropApi;

typedef struct LegacyTextureUnitState {
    GLint binding_2d;
    GLint texture_environment_mode;
    GLboolean texture_2d_enabled;
    GLboolean texture_gen_s_enabled;
    GLboolean texture_gen_t_enabled;
    GLfloat matrix[16];
} LegacyTextureUnitState;

#define LEGACY_DRAW_CAPABILITY_COUNT 9

typedef struct LegacyDrawState {
    GLfloat projection_matrix[16];
    GLfloat modelview_matrix[16];
    LegacyTextureUnitState texture_units[2];
    GLint matrix_mode;
    GLint viewport[4];
    GLint active_texture;
    GLint blend_source;
    GLint blend_destination;
    GLint blend_equation;
    GLint scissor_box[4];
    GLfloat current_color[4];
    GLint current_program;
    GLfloat line_width;
    GLint polygon_mode[2];
    GLboolean color_mask[4];
    GLint shade_model;
    GLboolean capabilities[LEGACY_DRAW_CAPABILITY_COUNT];
} LegacyDrawState;

typedef enum LegacyUnrestorableProgramState {
    LEGACY_PROGRAM_DELETE_PENDING,
    LEGACY_PROGRAM_RELINK_FAILED
} LegacyUnrestorableProgramState;

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

typedef struct PixelComparisonRegion {
    const char *label;
    RendererRect bounds;
    int tolerance;
} PixelComparisonRegion;

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
    {26, 17,   0, 255,   0, 255}
};

static const PixelComparisonRegion strict_feature_regions[] = {
    {"triangle interior", {35, 4, 3, 4}, STRICT_PIXEL_TOLERANCE},
    {"sprite interior", {49, 3, 6, 6}, STRICT_PIXEL_TOLERANCE},
    {"blend opaque interior", {35, 17, 2, 6}, STRICT_PIXEL_TOLERANCE},
    {"blend alpha interior", {39, 17, 2, 6}, STRICT_PIXEL_TOLERANCE},
    {"blend additive interior", {43, 17, 2, 6}, STRICT_PIXEL_TOLERANCE},
    {"scissor inside", {51, 17, 2, 4}, STRICT_PIXEL_TOLERANCE},
    {"scissor outside left", {49, 17, 1, 4}, STRICT_PIXEL_TOLERANCE},
    {"scissor outside right", {55, 17, 2, 4}, STRICT_PIXEL_TOLERANCE},
    {"scissor outside top", {51, 15, 2, 1}, STRICT_PIXEL_TOLERANCE},
    {"scissor outside bottom", {51, 23, 2, 1}, STRICT_PIXEL_TOLERANCE}
};

static const PixelComparisonRegion tolerant_feature_regions[] = {
    {"line", {32, 27, 28, 12}, LINE_PIXEL_TOLERANCE},
    {"text", {32, 43, 14, 10}, TEXT_PIXEL_TOLERANCE}
};

static const uint8_t comparison_text_atlas_pixels[] = {
    /* A row 0, gap, B row 0, gap. */
      0,   0,   0,   0, 255, 255, 255, 128,
      0,   0,   0,   0,   0,   0,   0,   0,
    255, 255, 255, 255, 255, 255, 255, 128,
      0,   0,   0,   0,   0,   0,   0,   0,
    /* A row 1, gap, B row 1, gap. */
    255, 255, 255, 128,   0,   0,   0,   0,
    255, 255, 255, 128,   0,   0,   0,   0,
    255, 255, 255, 255,   0,   0,   0,   0,
    255, 255, 255, 128,   0,   0,   0,   0,
    /* A row 2, gap, B row 2, gap. */
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255,   0,   0,   0,   0,
    255, 255, 255, 255, 255, 255, 255, 128,
      0,   0,   0,   0,   0,   0,   0,   0,
    /* A row 3, gap, B row 3, gap. */
    255, 255, 255, 255,   0,   0,   0,   0,
    255, 255, 255, 255,   0,   0,   0,   0,
    255, 255, 255, 255,   0,   0,   0,   0,
    255, 255, 255, 128,   0,   0,   0,   0
};

static TextGeometryFont make_comparison_text_font(void)
{
    TextGeometryFont font;

    memset(&font, 0, sizeof(font));
    font.atlas_width = COMPARISON_TEXT_ATLAS_WIDTH;
    font.atlas_height = COMPARISON_TEXT_ATLAS_HEIGHT;
    font.ascent = 4.0f;
    font.line_height = 4.0f;
    font.line_spacing = 5.0f;
    font.fallback_byte = (unsigned char)'?';

    font.glyphs[(unsigned char)'A'].atlas_bounds
        = (RendererRect){0, 0, 3, 4};
    font.glyphs[(unsigned char)'A'].bearing_top = 4.0f;
    font.glyphs[(unsigned char)'A'].advance = 4.0f;
    font.glyphs[(unsigned char)'A'].available = 1;

    font.glyphs[(unsigned char)'B'].atlas_bounds
        = (RendererRect){4, 0, 3, 4};
    font.glyphs[(unsigned char)'B'].bearing_top = 4.0f;
    font.glyphs[(unsigned char)'B'].advance = 4.0f;
    font.glyphs[(unsigned char)'B'].available = 1;

    font.glyphs[(unsigned char)'?'] = font.glyphs[(unsigned char)'A'];
    return font;
}

static const GLenum legacy_draw_capabilities[
    LEGACY_DRAW_CAPABILITY_COUNT] = {
    GL_BLEND,
    GL_SCISSOR_TEST,
    GL_DEPTH_TEST,
    GL_CULL_FACE,
    GL_STENCIL_TEST,
    GL_ALPHA_TEST,
    GL_LIGHTING,
    GL_FOG,
    GL_COLOR_LOGIC_OP
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
    static const RendererVertex2D strict_triangle_vertices[] = {
        {34.0f, 2.0f, 0.0f, 0.0f, {255, 32, 16, 255}},
        {46.0f, 2.0f, 0.0f, 0.0f, {255, 32, 16, 255}},
        {34.0f, 12.0f, 0.0f, 0.0f, {255, 32, 16, 255}}
    };
    static const RendererPoint2D line_points[] = {
        {34.0f, 30.0f}, {58.0f, 34.0f}
    };
    static const unsigned char comparison_text[] = {'A', 'B'};
    const RendererTextureDesc texture_desc = {
        .width = 2,
        .height = 2,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererTextureDesc comparison_text_texture_desc = {
        .width = COMPARISON_TEXT_ATLAS_WIDTH,
        .height = COMPARISON_TEXT_ATLAS_HEIGHT,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const TextGeometryFont comparison_text_font
        = make_comparison_text_font();
    const RendererColor clear = {8, 16, 24, 255};
    const RendererRect scissor = {18, 2, 4, 6};
    const RendererRect strict_scissor = {50, 16, 4, 6};
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
    RendererTexture *comparison_text_texture = NULL;
    RendererMesh *mesh = NULL;
    RendererVertex2D comparison_text_vertices[12];
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
    TEST_CHECK_CLEANUP(sizeof(comparison_text_atlas_pixels)
                       == COMPARISON_TEXT_ATLAS_WIDTH
                          * COMPARISON_TEXT_ATLAS_HEIGHT
                          * PIXEL_COMPONENTS);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &comparison_text_texture_desc,
                           comparison_text_atlas_pixels,
                           COMPARISON_TEXT_ATLAS_WIDTH * PIXEL_COMPONENTS,
                           &comparison_text_texture)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(check_borrowed_upload_state(
                           test_context, unpack_buffer) == 0);
    end_borrowed_upload_state(test_context, &unpack_buffer);
    TEST_CHECK_CLEANUP(Renderer_mesh_create(renderer, mesh_vertices, 3,
                                             &mesh)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Text_geometry_build(
                           &comparison_text_font,
                           comparison_text, sizeof(comparison_text),
                           (RendererPoint2D){34.0f, 46.0f},
                           TEXT_GEOMETRY_ALIGN_LEFT,
                           TEXT_GEOMETRY_ALIGN_TOP,
                           (RendererColor){160, 224, 96, 255},
                           comparison_text_vertices,
                           sizeof(comparison_text_vertices)
                               / sizeof(comparison_text_vertices[0]))
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
    TEST_CHECK_CLEANUP(Renderer_draw_mesh(renderer, texture, mesh)
                       == RENDERER_STATUS_OK);

    /* These isolated regions make strict backend parity a property of one
     * semantic feature at a time. Comparisons use stable interior coverage;
     * implementation-dependent rasterization edges are excluded. */
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_OPAQUE)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_triangles(
                           renderer, NULL, strict_triangle_vertices, 3)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           renderer, texture,
                           48.0f, 2.0f, 56.0f, 10.0f,
                           0.0f, 0.0f, 1.0f, 1.0f,
                           (RendererColor){255, 255, 255, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 34.0f, 16.0f, 12.0f, 8.0f,
                           (RendererColor){0, 0, 0, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 38.0f, 16.0f, 4.0f, 8.0f,
                           (RendererColor){255, 255, 255, 128})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(
                           renderer, RENDERER_BLEND_ADDITIVE)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 42.0f, 16.0f, 4.0f, 8.0f,
                           (RendererColor){64, 32, 16, 128})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_OPAQUE)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_scissor(renderer, &strict_scissor)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           renderer, 48.0f, 14.0f, 10.0f, 10.0f,
                           (RendererColor){32, 255, 64, 255})
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_set_scissor(renderer, NULL)
                       == RENDERER_STATUS_OK);

    TEST_CHECK_CLEANUP(Renderer_set_blend(renderer, RENDERER_BLEND_ALPHA)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_stroke_path(
                           renderer, line_points,
                           sizeof(line_points) / sizeof(line_points[0]),
                           3.0f, (RendererColor){220, 40, 80, 255}, 0)
                       == RENDERER_STATUS_OK);
    /* Text_geometry_build plus the immutable atlas follows the same
     * semantic triangle path used by Text_renderer_draw without depending
     * on host font rasterization. */
    TEST_CHECK_CLEANUP(Renderer_draw_triangles(
                           renderer, comparison_text_texture,
                           comparison_text_vertices,
                           sizeof(comparison_text_vertices)
                               / sizeof(comparison_text_vertices[0]))
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
        255,   0,   0, 255,    0,   0, 255, 255,    0, 255,   0, 255,
        255,   0,   0, 255,    0,   0, 255, 255,    0, 255,   0, 255,
        255,   0,   0, 255,    0,   0, 255, 255,    0, 255,   0, 255
    };
    const RendererTextureDesc nearest_clamp_desc = {
        .width = 3,
        .height = 3,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererTextureDesc linear_clamp_desc = {
        .width = 3,
        .height = 3,
        .filter = RENDERER_TEXTURE_FILTER_LINEAR,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererTextureDesc nearest_repeat_desc = {
        .width = 3,
        .height = 3,
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
                           renderer, &nearest_clamp_desc, texture_pixels, 12,
                           &nearest_clamp) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &linear_clamp_desc, texture_pixels, 12,
                           &linear_clamp) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           renderer, &nearest_repeat_desc, texture_pixels, 12,
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
                           1.0f / 3.0f, 0.25f,
                           1.0f / 3.0f, 0.25f, white)
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

static int components_within_tolerance(GLubyte first, GLubyte second,
                                       int tolerance)
{
    int difference = (int)first - (int)second;

    if (difference < 0)
        difference = -difference;
    return difference <= tolerance;
}

static int component_near(GLubyte actual, int expected)
{
    return components_within_tolerance(
        actual, (GLubyte)expected, EXPECTED_PIXEL_TOLERANCE);
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
    /* Nearest chooses the blue texel instead of blending adjacent colors. */
    TEST_CHECK(check_rgba(pixels, 5, 5, 0, 0, 255, 255) == 0);
    /* Clamp keeps an out-of-range horizontal coordinate at the green edge. */
    TEST_CHECK(check_rgba(pixels, 13, 5, 0, 255, 0, 255) == 0);
    /* Linear sampling halfway between red and blue mixes them evenly. */
    TEST_CHECK(check_rgba(pixels, 21, 5, 128, 0, 128, 255) == 0);
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

static int load_legacy_interop_api(LegacyInteropApi *api)
{
    memset(api, 0, sizeof(*api));
    api->create_shader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress(
        "glCreateShader");
    api->shader_source = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress(
        "glShaderSource");
    api->compile_shader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress(
        "glCompileShader");
    api->get_shader_integer = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress(
        "glGetShaderiv");
    api->delete_shader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress(
        "glDeleteShader");
    api->create_program = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress(
        "glCreateProgram");
    api->attach_shader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress(
        "glAttachShader");
    api->link_program = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress(
        "glLinkProgram");
    api->get_program_integer = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress(
        "glGetProgramiv");
    api->delete_program = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress(
        "glDeleteProgram");
    api->use_program = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress(
        "glUseProgram");
    api->blend_equation = (PFNGLBLENDEQUATIONPROC)SDL_GL_GetProcAddress(
        "glBlendEquation");
    if (api->blend_equation == NULL) {
        api->blend_equation = (PFNGLBLENDEQUATIONPROC)
            SDL_GL_GetProcAddress("glBlendEquationEXT");
    }

    return api->create_shader != NULL && api->shader_source != NULL
        && api->compile_shader != NULL && api->get_shader_integer != NULL
        && api->delete_shader != NULL && api->create_program != NULL
        && api->attach_shader != NULL && api->link_program != NULL
        && api->get_program_integer != NULL
        && api->delete_program != NULL && api->use_program != NULL
        && api->blend_equation != NULL;
}

static GLuint compile_legacy_shader(LegacyInteropApi *api, GLenum type,
                                    const GLchar *source)
{
    GLuint shader = api->create_shader(type);
    GLint compiled = GL_FALSE;

    if (shader == 0)
        return 0;
    api->shader_source(shader, 1, &source, NULL);
    api->compile_shader(shader);
    api->get_shader_integer(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        api->delete_shader(shader);
        return 0;
    }
    return shader;
}

static int create_legacy_state_program(LegacyInteropApi *api,
                                       GLuint *program)
{
    static const GLchar vertex_source[] =
        "#version 120\n"
        "void main(void) { gl_Position = gl_Vertex; }\n";
    static const GLchar fragment_source[] =
        "#version 120\n"
        "void main(void) { gl_FragColor = vec4(1.0); }\n";
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    GLuint created = 0;
    GLint linked = GL_FALSE;
    int failed = 1;

    *program = 0;
    discard_gl_errors();
    vertex_shader = compile_legacy_shader(api, GL_VERTEX_SHADER,
                                          vertex_source);
    fragment_shader = compile_legacy_shader(api, GL_FRAGMENT_SHADER,
                                            fragment_source);
    if (vertex_shader == 0 || fragment_shader == 0)
        goto cleanup;
    created = api->create_program();
    if (created == 0)
        goto cleanup;
    api->attach_shader(created, vertex_shader);
    api->attach_shader(created, fragment_shader);
    api->link_program(created);
    api->get_program_integer(created, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE || glGetError() != GL_NO_ERROR)
        goto cleanup;

    *program = created;
    created = 0;
    failed = 0;

cleanup:
    if (created != 0)
        api->delete_program(created);
    if (fragment_shader != 0)
        api->delete_shader(fragment_shader);
    if (vertex_shader != 0)
        api->delete_shader(vertex_shader);
    if (failed)
        fprintf(stderr, "Could not create legacy state test program\n");
    discard_gl_errors();
    return failed;
}

static int establish_legacy_draw_state(TestContext *test_context,
                                       LegacyInteropApi *api,
                                       const GLuint textures[2],
                                       GLuint program)
{
    static const GLfloat projection_matrix[16] = {
        1.25f, 0.0f,  0.0f, 0.0f,
        0.0f,  0.75f, 0.0f, 0.0f,
        0.0f,  0.0f, -1.0f, 0.0f,
        0.25f, -0.5f, 0.0f, 1.0f
    };
    static const GLfloat modelview_matrix[16] = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        2.0f, 3.0f, 0.0f, 1.0f
    };
    static const GLfloat texture_matrices[2][16] = {
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.125f, 0.25f, 0.0f, 1.0f
        },
        {
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.375f, 0.625f, 0.0f, 1.0f
        }
    };
    size_t capability_index;

    discard_gl_errors();
    glViewport(3, 2, FRAME_WIDTH - 7, FRAME_HEIGHT - 5);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection_matrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(modelview_matrix);

    for (capability_index = 0;
         capability_index < LEGACY_DRAW_CAPABILITY_COUNT;
         capability_index++) {
        glEnable(legacy_draw_capabilities[capability_index]);
    }
    api->blend_equation(GL_FUNC_REVERSE_SUBTRACT);
    glBlendFunc(GL_ONE, GL_ZERO);
    glScissor(4, 3, FRAME_WIDTH - 9, FRAME_HEIGHT - 8);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glShadeModel(GL_FLAT);
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_POINT);
    glLineWidth(2.0f);
    glColor4f(0.25f, 0.5f, 0.75f, 0.625f);

    test_context->buffer_api.active_texture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(texture_matrices[0]);

    test_context->buffer_api.active_texture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(texture_matrices[1]);
    api->use_program(program);

    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not establish borrowed legacy draw state\n");
        return 1;
    }
    return 0;
}

static int capture_legacy_draw_state(TestContext *test_context,
                                     LegacyDrawState *state)
{
    size_t capability_index;
    size_t texture_unit;

    memset(state, 0, sizeof(*state));
    glGetFloatv(GL_PROJECTION_MATRIX, state->projection_matrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, state->modelview_matrix);
    glGetIntegerv(GL_MATRIX_MODE, &state->matrix_mode);
    glGetIntegerv(GL_VIEWPORT, state->viewport);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state->active_texture);
    glGetIntegerv(GL_BLEND_SRC, &state->blend_source);
    glGetIntegerv(GL_BLEND_DST, &state->blend_destination);
    glGetIntegerv(GL_BLEND_EQUATION, &state->blend_equation);
    glGetIntegerv(GL_SCISSOR_BOX, state->scissor_box);
    glGetFloatv(GL_CURRENT_COLOR, state->current_color);
    glGetIntegerv(GL_CURRENT_PROGRAM, &state->current_program);
    glGetFloatv(GL_LINE_WIDTH, &state->line_width);
    glGetIntegerv(GL_POLYGON_MODE, state->polygon_mode);
    glGetBooleanv(GL_COLOR_WRITEMASK, state->color_mask);
    glGetIntegerv(GL_SHADE_MODEL, &state->shade_model);
    for (capability_index = 0;
         capability_index < LEGACY_DRAW_CAPABILITY_COUNT;
         capability_index++) {
        state->capabilities[capability_index] = glIsEnabled(
            legacy_draw_capabilities[capability_index]);
    }

    for (texture_unit = 0; texture_unit < 2; texture_unit++) {
        LegacyTextureUnitState *unit = &state->texture_units[texture_unit];

        test_context->buffer_api.active_texture(
            (GLenum)(GL_TEXTURE0 + texture_unit));
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &unit->binding_2d);
        glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                     &unit->texture_environment_mode);
        unit->texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
        unit->texture_gen_s_enabled = glIsEnabled(GL_TEXTURE_GEN_S);
        unit->texture_gen_t_enabled = glIsEnabled(GL_TEXTURE_GEN_T);
        glGetFloatv(GL_TEXTURE_MATRIX, unit->matrix);
    }
    test_context->buffer_api.active_texture((GLenum)state->active_texture);

    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not capture borrowed legacy draw state\n");
        return 1;
    }
    return 0;
}

static int integer_values_equal(const char *label, const GLint *expected,
                                const GLint *actual, size_t count)
{
    size_t index;

    for (index = 0; index < count; index++) {
        if (actual[index] != expected[index]) {
            fprintf(stderr, "%s[%lu] changed: %d != %d\n", label,
                    (unsigned long)index, actual[index], expected[index]);
            return 0;
        }
    }
    return 1;
}

static int boolean_values_equal(const char *label,
                                const GLboolean *expected,
                                const GLboolean *actual, size_t count)
{
    size_t index;

    for (index = 0; index < count; index++) {
        if (actual[index] != expected[index]) {
            fprintf(stderr, "%s[%lu] changed: %u != %u\n", label,
                    (unsigned long)index, (unsigned int)actual[index],
                    (unsigned int)expected[index]);
            return 0;
        }
    }
    return 1;
}

static int float_values_near(const char *label, const GLfloat *expected,
                             const GLfloat *actual, size_t count)
{
    size_t index;

    for (index = 0; index < count; index++) {
        GLfloat difference = actual[index] - expected[index];

        if (difference < 0.0f)
            difference = -difference;
        if (difference > 0.00001f) {
            fprintf(stderr, "%s[%lu] changed: %.7g != %.7g\n", label,
                    (unsigned long)index, (double)actual[index],
                    (double)expected[index]);
            return 0;
        }
    }
    return 1;
}

static int check_legacy_draw_state_preserved(
    const LegacyDrawState *expected, const LegacyDrawState *actual)
{
    static const char *texture_matrix_labels[2] = {
        "texture unit 0 matrix", "texture unit 1 matrix"
    };
    size_t texture_unit;

    TEST_CHECK(float_values_near("projection matrix",
                                 expected->projection_matrix,
                                 actual->projection_matrix, 16));
    TEST_CHECK(float_values_near("modelview matrix",
                                 expected->modelview_matrix,
                                 actual->modelview_matrix, 16));
    TEST_CHECK(integer_values_equal("matrix mode", &expected->matrix_mode,
                                    &actual->matrix_mode, 1));
    TEST_CHECK(integer_values_equal("viewport", expected->viewport,
                                    actual->viewport, 4));
    TEST_CHECK(integer_values_equal("active texture",
                                    &expected->active_texture,
                                    &actual->active_texture, 1));
    TEST_CHECK(integer_values_equal("blend source",
                                    &expected->blend_source,
                                    &actual->blend_source, 1));
    TEST_CHECK(integer_values_equal("blend destination",
                                    &expected->blend_destination,
                                    &actual->blend_destination, 1));
    TEST_CHECK(integer_values_equal("blend equation",
                                    &expected->blend_equation,
                                    &actual->blend_equation, 1));
    TEST_CHECK(integer_values_equal("scissor box", expected->scissor_box,
                                    actual->scissor_box, 4));
    TEST_CHECK(float_values_near("current color", expected->current_color,
                                 actual->current_color, 4));
    TEST_CHECK(integer_values_equal("current program",
                                    &expected->current_program,
                                    &actual->current_program, 1));
    TEST_CHECK(float_values_near("line width", &expected->line_width,
                                 &actual->line_width, 1));
    TEST_CHECK(integer_values_equal("polygon mode", expected->polygon_mode,
                                    actual->polygon_mode, 2));
    TEST_CHECK(boolean_values_equal("color write mask", expected->color_mask,
                                    actual->color_mask, 4));
    TEST_CHECK(integer_values_equal("shade model", &expected->shade_model,
                                    &actual->shade_model, 1));
    TEST_CHECK(boolean_values_equal("capability enables",
                                    expected->capabilities,
                                    actual->capabilities,
                                    LEGACY_DRAW_CAPABILITY_COUNT));

    for (texture_unit = 0; texture_unit < 2; texture_unit++) {
        const LegacyTextureUnitState *expected_unit =
            &expected->texture_units[texture_unit];
        const LegacyTextureUnitState *actual_unit =
            &actual->texture_units[texture_unit];

        TEST_CHECK(integer_values_equal(
                       "texture binding", &expected_unit->binding_2d,
                       &actual_unit->binding_2d, 1));
        TEST_CHECK(integer_values_equal(
                       "texture environment mode",
                       &expected_unit->texture_environment_mode,
                       &actual_unit->texture_environment_mode, 1));
        TEST_CHECK(boolean_values_equal(
                       "texture 2D enable",
                       &expected_unit->texture_2d_enabled,
                       &actual_unit->texture_2d_enabled, 1));
        TEST_CHECK(boolean_values_equal(
                       "texture coordinate generation S enable",
                       &expected_unit->texture_gen_s_enabled,
                       &actual_unit->texture_gen_s_enabled, 1));
        TEST_CHECK(boolean_values_equal(
                       "texture coordinate generation T enable",
                       &expected_unit->texture_gen_t_enabled,
                       &actual_unit->texture_gen_t_enabled, 1));
        TEST_CHECK(float_values_near(texture_matrix_labels[texture_unit],
                                     expected_unit->matrix,
                                     actual_unit->matrix, 16));
    }
    return 0;
}

static int establish_top_left_direct_state(TestContext *test_context,
                                           LegacyInteropApi *api)
{
    size_t capability_index;
    size_t texture_unit;

    discard_gl_errors();
    api->use_program(0);
    for (texture_unit = 0; texture_unit < 2; texture_unit++) {
        test_context->buffer_api.active_texture(
            (GLenum)(GL_TEXTURE0 + texture_unit));
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    test_context->buffer_api.active_texture(GL_TEXTURE0);
    for (capability_index = 0;
         capability_index < LEGACY_DRAW_CAPABILITY_COUNT;
         capability_index++) {
        glDisable(legacy_draw_capabilities[capability_index]);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    api->blend_equation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ZERO);
    glViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)FRAME_WIDTH, (GLdouble)FRAME_HEIGHT,
            0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not establish top-left direct draw state\n");
        return 1;
    }
    return 0;
}

static int read_legacy_framebuffer(GLubyte *pixels)
{
    glFinish();
    glReadPixels(0, 0, FRAME_WIDTH, FRAME_HEIGHT,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "Could not read legacy framebuffer\n");
        return 1;
    }
    return 0;
}

static int check_sdl_renderer_rejects_unrestorable_program(
    TestContext *test_context, LegacyUnrestorableProgramState program_state)
{
    const RendererColor clear = {13, 27, 41, 255};
    const RendererColor queued_color = {193, 71, 29, 255};
    const char *state_name = program_state == LEGACY_PROGRAM_DELETE_PENDING
        ? "delete-pending current program"
        : "failed-relink current program";
    GLubyte pixels[FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    LegacyInteropApi interop_api;
    LegacyDrawState state_before_flush;
    LegacyDrawState state_after_flush;
    SdlRenderer *sdl_renderer = NULL;
    Renderer *frontend = NULL;
    GLuint borrowed_textures[2] = {0, 0};
    GLuint borrowed_program = 0;
    GLuint failed_link_shader = 0;
    GLint current_program = 0;
    GLint delete_status = GL_FALSE;
    GLint link_status = GL_TRUE;
    RendererStatus flush_status;
    int delete_requested = 0;
    int failed = 0;
    int result = 1;

    memset(&interop_api, 0, sizeof(interop_api));
    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    TEST_CHECK_CLEANUP(load_legacy_interop_api(&interop_api));
    TEST_CHECK_CLEANUP(Sdl_renderer_create(test_context->window,
                                           &sdl_renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(sdl_renderer != NULL);
    frontend = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK_CLEANUP(frontend != NULL);
    TEST_CHECK_CLEANUP(create_legacy_state_program(
                           &interop_api, &borrowed_program) == 0);
    glGenTextures(2, borrowed_textures);
    TEST_CHECK_CLEANUP(borrowed_textures[0] != 0
                       && borrowed_textures[1] != 0);

    test_context->framebuffer_api.bind(GL_FRAMEBUFFER,
                                       test_context->framebuffer);
    TEST_CHECK_CLEANUP(Sdl_renderer_begin_frame(
                           sdl_renderer, FRAME_WIDTH, FRAME_HEIGHT, clear)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_fill_rect(
                           frontend, 6.0f, 5.0f, 12.0f, 9.0f,
                           queued_color) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(establish_legacy_draw_state(
                           test_context, &interop_api, borrowed_textures,
                           borrowed_program) == 0);

    discard_gl_errors();
    if (program_state == LEGACY_PROGRAM_DELETE_PENDING) {
        interop_api.delete_program(borrowed_program);
        delete_requested = 1;
        interop_api.get_program_integer(
            borrowed_program, GL_DELETE_STATUS, &delete_status);
        TEST_CHECK_CLEANUP(delete_status == GL_TRUE);
    } else {
        /* An attached shader that was never compiled makes this relink fail.
         * OpenGL 2.1 keeps the old executable current until glUseProgram
         * removes it, even though the program's LINK_STATUS becomes false. */
        failed_link_shader = interop_api.create_shader(GL_FRAGMENT_SHADER);
        TEST_CHECK_CLEANUP(failed_link_shader != 0);
        interop_api.attach_shader(borrowed_program, failed_link_shader);
        interop_api.link_program(borrowed_program);
        interop_api.get_program_integer(
            borrowed_program, GL_LINK_STATUS, &link_status);
        TEST_CHECK_CLEANUP(link_status == GL_FALSE);
    }
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    TEST_CHECK_CLEANUP(current_program == (GLint)borrowed_program);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);
    TEST_CHECK_CLEANUP(capture_legacy_draw_state(
                           test_context, &state_before_flush) == 0);

    flush_status = Sdl_renderer_flush_preserving_legacy(sdl_renderer);
    if (flush_status != RENDERER_STATUS_BACKEND_ERROR) {
        fprintf(stderr, "%s flush returned %d; expected backend error\n",
                state_name, (int)flush_status);
        failed = 1;
    }
    if (capture_legacy_draw_state(test_context, &state_after_flush) != 0) {
        failed = 1;
    } else if (check_legacy_draw_state_preserved(
                   &state_before_flush, &state_after_flush) != 0) {
        fprintf(stderr, "%s flush changed borrowed legacy GL state\n",
                state_name);
        failed = 1;
    }
    if (read_legacy_framebuffer(pixels) != 0) {
        failed = 1;
    } else if (check_rgba(pixels, 10, 8,
                          clear.red, clear.green, clear.blue,
                          clear.alpha) != 0) {
        fprintf(stderr, "%s flush executed a queued command\n",
                state_name);
        failed = 1;
    }

    /* Retire the exceptional program according to its OpenGL lifetime, then
     * erase any accidental draw. A successful end_frame must still submit
     * the command that the rejected bridge left untouched in the queue. */
    interop_api.use_program(0);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);
    if (delete_requested) {
        borrowed_program = 0;
    } else {
        interop_api.delete_program(borrowed_program);
        borrowed_program = 0;
    }
    if (failed_link_shader != 0) {
        interop_api.delete_shader(failed_link_shader);
        failed_link_shader = 0;
    }
    TEST_CHECK_CLEANUP(establish_top_left_direct_state(
                           test_context, &interop_api) == 0);
    glClearColor((GLclampf)clear.red / 255.0f,
                 (GLclampf)clear.green / 255.0f,
                 (GLclampf)clear.blue / 255.0f,
                 (GLclampf)clear.alpha / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    TEST_CHECK_CLEANUP(glGetError() == GL_NO_ERROR);
    TEST_CHECK_CLEANUP(Sdl_renderer_end_frame(sdl_renderer)
                       == RENDERER_STATUS_OK);
    if (read_legacy_framebuffer(pixels) != 0) {
        failed = 1;
    } else {
        if (check_rgba(pixels, 10, 8,
                       queued_color.red, queued_color.green,
                       queued_color.blue, queued_color.alpha) != 0) {
            fprintf(stderr, "%s flush did not retain its queued command\n",
                    state_name);
            failed = 1;
        }
        if (check_rgba(pixels, 2, 2,
                       clear.red, clear.green, clear.blue,
                       clear.alpha) != 0) {
            fprintf(stderr, "%s retry changed pixels outside the queue\n",
                    state_name);
            failed = 1;
        }
    }
    result = failed;

cleanup:
    if (SDL_GL_GetCurrentContext() == test_context->context
        || SDL_GL_MakeCurrent(test_context->window,
                              test_context->context) == 0) {
        if (interop_api.use_program != NULL)
            interop_api.use_program(0);
        if (borrowed_program != 0
            && !delete_requested
            && interop_api.delete_program != NULL) {
            interop_api.delete_program(borrowed_program);
        }
        if (failed_link_shader != 0
            && interop_api.delete_shader != NULL) {
            interop_api.delete_shader(failed_link_shader);
        }
        if (test_context->buffer_api.active_texture != NULL) {
            test_context->buffer_api.active_texture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            test_context->buffer_api.active_texture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            test_context->buffer_api.active_texture(GL_TEXTURE0);
        }
        if (borrowed_textures[0] != 0 || borrowed_textures[1] != 0)
            glDeleteTextures(2, borrowed_textures);
        discard_gl_errors();
    }
    if (sdl_renderer != NULL) {
        Sdl_renderer_destroy(sdl_renderer);
        if (SDL_GL_GetCurrentContext() != test_context->context
            || glGetString(GL_VERSION) == NULL) {
            fprintf(stderr, "SDL renderer destruction after %s changed or "
                    "invalidated its borrowed context\n", state_name);
            result = 1;
        }
    }
    return result;
}

static int check_sdl_renderer_mixed_legacy_scene(TestContext *test_context)
{
    static const uint8_t green_pixel[] = {0, 255, 0, 255};
    const RendererTextureDesc green_texture_desc = {
        .width = 1,
        .height = 1,
        .filter = RENDERER_TEXTURE_FILTER_NEAREST,
        .wrap = RENDERER_TEXTURE_WRAP_CLAMP
    };
    const RendererColor clear = {8, 16, 24, 255};
    GLubyte pixels[FRAME_WIDTH * FRAME_HEIGHT * PIXEL_COMPONENTS];
    LegacyInteropApi interop_api;
    LegacyDrawState state_before_flush;
    LegacyDrawState state_after_flush;
    SdlRenderer *sdl_renderer = NULL;
    Renderer *frontend = NULL;
    RendererTexture *green_texture = NULL;
    GLuint borrowed_textures[2] = {0, 0};
    GLuint borrowed_program = 0;
    int drawable_width = 0;
    int drawable_height = 0;
    int result = 1;

    memset(&interop_api, 0, sizeof(interop_api));
    TEST_CHECK_CLEANUP(SDL_GL_MakeCurrent(test_context->window,
                                          test_context->context) == 0);
    TEST_CHECK_CLEANUP(load_legacy_interop_api(&interop_api));
    TEST_CHECK_CLEANUP(Sdl_renderer_create(test_context->window,
                                           &sdl_renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(sdl_renderer != NULL);
    frontend = Sdl_renderer_frontend(sdl_renderer);
    TEST_CHECK_CLEANUP(frontend != NULL);
    TEST_CHECK_CLEANUP(Sdl_renderer_flush_preserving_legacy(NULL)
                       == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK_CLEANUP(Sdl_renderer_flush_preserving_legacy(sdl_renderer)
                       == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK_CLEANUP(Renderer_texture_create_with_desc(
                           frontend, &green_texture_desc, green_pixel, 4,
                           &green_texture) == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(green_texture != NULL);
    TEST_CHECK_CLEANUP(create_legacy_state_program(
                           &interop_api, &borrowed_program) == 0);
    glGenTextures(2, borrowed_textures);
    TEST_CHECK_CLEANUP(borrowed_textures[0] != 0
                       && borrowed_textures[1] != 0);
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
                           sdl_renderer, SDL_RENDERER_LEGACY_TOP_LEFT)
                       == RENDERER_STATUS_OK);
    draw_legacy_quad(2.0f, 2.0f, 22.0f, 18.0f, 255, 0, 0);

    TEST_CHECK_CLEANUP(Renderer_set_blend(frontend, RENDERER_BLEND_ALPHA)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(Renderer_draw_sprite(
                           frontend, green_texture,
                           8.0f, 6.0f, 26.0f, 20.0f,
                           0.0f, 0.0f, 1.0f, 1.0f,
                           (RendererColor){255, 255, 255, 255})
                       == RENDERER_STATUS_OK);

    /* The bridge is an ordering barrier, but it borrows every direct-GL
     * state value rather than imposing the semantic backend's state. */
    TEST_CHECK_CLEANUP(establish_legacy_draw_state(
                           test_context, &interop_api, borrowed_textures,
                           borrowed_program) == 0);
    TEST_CHECK_CLEANUP(capture_legacy_draw_state(
                           test_context, &state_before_flush) == 0);
    TEST_CHECK_CLEANUP(Sdl_renderer_flush_preserving_legacy(sdl_renderer)
                       == RENDERER_STATUS_OK);
    TEST_CHECK_CLEANUP(capture_legacy_draw_state(
                           test_context, &state_after_flush) == 0);
    TEST_CHECK_CLEANUP(check_legacy_draw_state_preserved(
                           &state_before_flush, &state_after_flush) == 0);

    /* Normalize with direct GL only. If the bridge did not submit the queued
     * green sprite, end_frame would draw it over this blue quad. */
    TEST_CHECK_CLEANUP(establish_top_left_direct_state(
                           test_context, &interop_api) == 0);
    draw_legacy_quad(14.0f, 10.0f, 30.0f, 22.0f, 0, 0, 255);

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

    TEST_CHECK_CLEANUP(check_rgba(pixels, 31, 2, 8, 16, 24, 255) == 0);
    TEST_CHECK_CLEANUP(check_rgba(pixels, 4, 4, 255, 0, 0, 255) == 0);
    /* The semantic sprite was submitted after the first direct quad. */
    TEST_CHECK_CLEANUP(check_rgba(pixels, 10, 8, 0, 255, 0, 255) == 0);
    /* The bridge submitted semantic work before the second direct quad. */
    TEST_CHECK_CLEANUP(check_rgba(pixels, 18, 14, 0, 0, 255, 255) == 0);
    result = 0;

cleanup:
    if (SDL_GL_GetCurrentContext() == test_context->context
        || SDL_GL_MakeCurrent(test_context->window,
                              test_context->context) == 0) {
        if (interop_api.use_program != NULL)
            interop_api.use_program(0);
        if (test_context->buffer_api.active_texture != NULL) {
            test_context->buffer_api.active_texture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            test_context->buffer_api.active_texture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            test_context->buffer_api.active_texture(GL_TEXTURE0);
        }
        if (borrowed_textures[0] != 0 || borrowed_textures[1] != 0)
            glDeleteTextures(2, borrowed_textures);
        if (borrowed_program != 0
            && interop_api.delete_program != NULL) {
            interop_api.delete_program(borrowed_program);
        }
        discard_gl_errors();
    }
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

static int check_comparison_feature_pixels(const GLubyte *pixels)
{
    TEST_CHECK(check_rgba(pixels, 36, 4, 255, 32, 16, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 49, 3, 255, 255, 0, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 35, 18, 0, 0, 0, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 39, 18, 128, 128, 128, 191) == 0);
    TEST_CHECK(check_rgba(pixels, 43, 18, 32, 16, 8, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 49, 18, 8, 16, 24, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 51, 18, 32, 255, 64, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 55, 18, 8, 16, 24, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 51, 15, 8, 16, 24, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 51, 23, 8, 16, 24, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 46, 32, 220, 40, 80, 255) == 0);
    TEST_CHECK(check_rgba(pixels, 34, 48, 160, 224, 96, 255) == 0);
    return 0;
}

static int compare_pixel_region(const GLubyte *legacy,
                                const GLubyte *core,
                                const PixelComparisonRegion *region)
{
    int x;
    int y;

    TEST_CHECK(region != NULL);
    TEST_CHECK(region->bounds.x >= 0 && region->bounds.y >= 0);
    TEST_CHECK(region->bounds.width > 0 && region->bounds.height > 0);
    TEST_CHECK(region->bounds.x + region->bounds.width <= FRAME_WIDTH);
    TEST_CHECK(region->bounds.y + region->bounds.height <= FRAME_HEIGHT);
    TEST_CHECK(region->tolerance >= 0);

    for (y = region->bounds.y;
         y < region->bounds.y + region->bounds.height; y++) {
        for (x = region->bounds.x;
             x < region->bounds.x + region->bounds.width; x++) {
            const GLubyte *legacy_pixel = pixel_at(legacy, x, y);
            const GLubyte *core_pixel = pixel_at(core, x, y);
            size_t component;

            for (component = 0; component < PIXEL_COMPONENTS; component++) {
                if (!components_within_tolerance(
                        legacy_pixel[component], core_pixel[component],
                        region->tolerance)) {
                    fprintf(stderr,
                            "%s legacy/core mismatch at (%d,%d), "
                            "component %lu with tolerance %d: %u != %u\n",
                            region->label, x, y, (unsigned long)component,
                            region->tolerance,
                            (unsigned int)legacy_pixel[component],
                            (unsigned int)core_pixel[component]);
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int compare_feature_regions(
    const GLubyte *legacy, const GLubyte *core,
    const PixelComparisonRegion *regions, size_t region_count)
{
    size_t region_index;

    for (region_index = 0; region_index < region_count; region_index++) {
        TEST_CHECK(compare_pixel_region(
                       legacy, core, &regions[region_index]) == 0);
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
            if (difference > EXPECTED_PIXEL_TOLERANCE) {
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
    int unrestorable_program_failures;
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
    unrestorable_program_failures =
        check_sdl_renderer_rejects_unrestorable_program(
            &legacy_context, LEGACY_PROGRAM_DELETE_PENDING);
    unrestorable_program_failures +=
        check_sdl_renderer_rejects_unrestorable_program(
            &legacy_context, LEGACY_PROGRAM_RELINK_FAILED);
    if (unrestorable_program_failures != 0)
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
    TEST_CHECK_CLEANUP(check_comparison_feature_pixels(legacy_pixels) == 0);
    TEST_CHECK_CLEANUP(check_comparison_feature_pixels(core_pixels) == 0);
    TEST_CHECK_CLEANUP(compare_feature_regions(
                           legacy_pixels, core_pixels,
                           strict_feature_regions,
                           sizeof(strict_feature_regions)
                               / sizeof(strict_feature_regions[0])) == 0);
    TEST_CHECK_CLEANUP(compare_feature_regions(
                           legacy_pixels, core_pixels,
                           tolerant_feature_regions,
                           sizeof(tolerant_feature_regions)
                               / sizeof(tolerant_feature_regions[0])) == 0);
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
