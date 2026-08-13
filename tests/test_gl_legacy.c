#include "test_helpers.h"

#include "xpclient_sdl.h"
#include "guiobjects.h"
#include "gl_diagnostics.h"
#include "images.h"
#include "renderer.h"
#include "renderer_gl_legacy.h"
#include "sdlrenderer.h"
#include "text.h"
#include "text_atlas.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define BASELINE_SIZE 12
#define PIXEL_COMPONENTS 4
#define TEST_SKIP 77
#define CONTEXT_LOG_GL_1_5_ARGUMENT "--context-log-gl-1.5"

#ifndef XPILOT_TEST_FONT_PATH
#error "XPILOT_TEST_FONT_PATH must name the bundled test font"
#endif

typedef void (APIENTRYP gen_framebuffers_fn)(GLsizei, GLuint *);
typedef void (APIENTRYP bind_framebuffer_fn)(GLenum, GLuint);
typedef void (APIENTRYP framebuffer_texture_2d_fn)(GLenum, GLenum, GLenum,
                                                   GLuint, GLint);
typedef GLenum (APIENTRYP check_framebuffer_status_fn)(GLenum);
typedef void (APIENTRYP delete_framebuffers_fn)(GLsizei, const GLuint *);

typedef struct framebuffer_api {
    gen_framebuffers_fn gen;
    bind_framebuffer_fn bind;
    framebuffer_texture_2d_fn attach_texture;
    check_framebuffer_status_fn check_status;
    delete_framebuffers_fn destroy;
} framebuffer_api_t;

struct SdlRenderer {
    unsigned int identity;
    int frame_active;
    RendererStatus frame_result;
};

struct TextRenderer {
    SdlRenderer *sdl_renderer;
    TextAtlas *atlas;
};

static int failure_count;
static int reject_pre_2_glsl_query;
static unsigned int rejected_pre_2_glsl_queries;
static int capture_font_resources;
static size_t captured_font_list_generations;
static size_t captured_font_texture_count;
static size_t ttf_open_count;
static size_t ttf_close_count;
static RendererStatus text_renderer_create_result = RENDERER_STATUS_OK;
static int text_renderer_create_calls;
static int text_renderer_destroy_calls;
static SdlRenderer paint_sdl_renderer = {
    0,
    1,
    RENDERER_STATUS_OK
};

SdlRenderer *Get_sdl_renderer(void)
{
    return &paint_sdl_renderer;
}

RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->frame_result == RENDERER_STATUS_OK
        && status != RENDERER_STATUS_OK) {
        renderer->frame_result = status;
    }
    return renderer->frame_result;
}

RendererStatus Text_renderer_create(SdlRenderer *sdl_renderer,
                                    TextAtlas *atlas,
                                    TextRenderer **text_renderer)
{
    TextRenderer *candidate;

    if (sdl_renderer == NULL || atlas == NULL || text_renderer == NULL
        || *text_renderer != NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    text_renderer_create_calls++;
    if (text_renderer_create_result != RENDERER_STATUS_OK)
        return text_renderer_create_result;
    candidate = malloc(sizeof(*candidate));
    if (candidate == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    candidate->sdl_renderer = sdl_renderer;
    candidate->atlas = atlas;
    *text_renderer = candidate;
    return RENDERER_STATUS_OK;
}

void Text_renderer_destroy(TextRenderer **text_renderer)
{
    if (text_renderer == NULL || *text_renderer == NULL)
        return;
    text_renderer_destroy_calls++;
    free(*text_renderer);
    *text_renderer = NULL;
}

int Text_renderer_matches(const TextRenderer *text_renderer,
                          const SdlRenderer *sdl_renderer,
                          const TextAtlas *atlas)
{
    return text_renderer != NULL && sdl_renderer != NULL && atlas != NULL
        && text_renderer->sdl_renderer == sdl_renderer
        && text_renderer->atlas == atlas;
}

extern const GLubyte *APIENTRY __real_glGetString(GLenum name);
extern GLuint APIENTRY __real_glGenLists(GLsizei range);
extern void APIENTRY __real_glGenTextures(GLsizei count, GLuint *textures);
extern TTF_Font *__real_TTF_OpenFont(const char *file, int ptsize);
extern void __real_TTF_CloseFont(TTF_Font *font);

const GLubyte *APIENTRY __wrap_glGetString(GLenum name)
{
    if (reject_pre_2_glsl_query
        && name == GL_SHADING_LANGUAGE_VERSION) {
        rejected_pre_2_glsl_queries++;
        return __real_glGetString(0);
    }
    return __real_glGetString(name);
}

GLuint APIENTRY __wrap_glGenLists(GLsizei range)
{
    GLuint list_base = __real_glGenLists(range);

    if (capture_font_resources)
        captured_font_list_generations++;
    return list_base;
}

void APIENTRY __wrap_glGenTextures(GLsizei count, GLuint *textures)
{
    __real_glGenTextures(count, textures);
    if (capture_font_resources && count > 0)
        captured_font_texture_count += (size_t)count;
}

TTF_Font *__wrap_TTF_OpenFont(const char *file, int ptsize)
{
    TTF_Font *font = __real_TTF_OpenFont(file, ptsize);

    if (font != NULL)
        ttf_open_count++;
    return font;
}

void __wrap_TTF_CloseFont(TTF_Font *font)
{
    if (font != NULL)
        ttf_close_count++;
    __real_TTF_CloseFont(font);
}

#define CHECK_CONTINUE(condition)                                           \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: check failed: %s\n",                 \
                    __FILE__, __LINE__, #condition);                        \
            failure_count++;                                                \
        }                                                                   \
    } while (0)

void error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

void warn(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

int xpprintf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = vfprintf(stdout, format, args);
    va_end(args);
    return result;
}

image_t *Image_get_texture(int index)
{
    (void)index;
    return NULL;
}

image_t *Image_get(int index)
{
    (void)index;
    return NULL;
}

static void APIENTRY capture_gl_gen_textures(GLsizei count, GLuint *textures)
{
    __real_glGenTextures(count, textures);
    if (capture_font_resources && count > 0)
        captured_font_texture_count += (size_t)count;
}

static void *load_current_gl_proc(void *userdata, const char *name)
{
    (void)userdata;
    if (strcmp(name, "glGenTextures") == 0)
        return (void *)capture_gl_gen_textures;
    return SDL_GL_GetProcAddress(name);
}

static void *load_gl_function(const char *core_name, const char *ext_name)
{
    void *function = SDL_GL_GetProcAddress(core_name);

    if (function == NULL)
        function = SDL_GL_GetProcAddress(ext_name);
    return function;
}

static int load_framebuffer_api(framebuffer_api_t *api)
{
    memset(api, 0, sizeof(*api));
    api->gen = (gen_framebuffers_fn)load_gl_function(
        "glGenFramebuffers", "glGenFramebuffersEXT");
    api->bind = (bind_framebuffer_fn)load_gl_function(
        "glBindFramebuffer", "glBindFramebufferEXT");
    api->attach_texture = (framebuffer_texture_2d_fn)load_gl_function(
        "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    api->check_status = (check_framebuffer_status_fn)load_gl_function(
        "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    api->destroy = (delete_framebuffers_fn)load_gl_function(
        "glDeleteFramebuffers", "glDeleteFramebuffersEXT");

    return api->gen != NULL && api->bind != NULL
        && api->attach_texture != NULL && api->check_status != NULL
        && api->destroy != NULL;
}

static unsigned int drain_gl_errors(void)
{
    unsigned int count = 0;

    while (glGetError() != GL_NO_ERROR)
        count++;
    return count;
}

static int check_context_logging_on_gl_1_5(void)
{
    SDL_Window *window = NULL;
    SDL_GLContext context = NULL;
    Renderer *renderer = NULL;
    const char *version;
    int major = 0;
    int minor = 0;
    int result = 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SKIP: SDL video initialization failed for the "
                "OpenGL 1.5 context: %s\n", SDL_GetError());
        return TEST_SKIP;
    }
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0) != 0) {
        fprintf(stderr, "SKIP: SDL cannot request an OpenGL 1.5 context: %s\n",
                SDL_GetError());
        result = TEST_SKIP;
        goto cleanup;
    }

    window = SDL_CreateWindow("XPilot OpenGL 1.5 diagnostics test",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        fprintf(stderr, "SKIP: OpenGL 1.5 window creation failed: %s\n",
                SDL_GetError());
        result = TEST_SKIP;
        goto cleanup;
    }
    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SKIP: OpenGL 1.5 context creation failed: %s\n",
                SDL_GetError());
        result = TEST_SKIP;
        goto cleanup;
    }

    version = (const char *)glGetString(GL_VERSION);
    if (version == NULL || sscanf(version, "%d.%d", &major, &minor) != 2) {
        fprintf(stderr, "Could not read the OpenGL 1.5 context version\n");
        goto cleanup;
    }
    if (major != 1 || minor != 5) {
        fprintf(stderr, "SKIP: MESA_GL_VERSION_OVERRIDE=1.5 was not "
                "honored (actual version: %s)\n", version);
        result = TEST_SKIP;
        goto cleanup;
    }

    drain_gl_errors();
    Gl_diagnostics_reset();
    rejected_pre_2_glsl_queries = 0;
    reject_pre_2_glsl_query = 1;
    Gl_diagnostics_log_context();
    reject_pre_2_glsl_query = 0;
    CHECK_CONTINUE(rejected_pre_2_glsl_queries == 0);
    CHECK_CONTINUE(Gl_diagnostics_check("OpenGL 1.5 context logging") == 0);
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 0);
    CHECK_CONTINUE(glGetError() == GL_NO_ERROR);
    CHECK_CONTINUE(Renderer_gl_legacy_create(load_current_gl_proc, NULL,
                                             &renderer)
                   == RENDERER_STATUS_BACKEND_ERROR);
    CHECK_CONTINUE(renderer == NULL);
    CHECK_CONTINUE(SDL_GL_GetCurrentContext() == context);
    CHECK_CONTINUE(glGetString(GL_VERSION) != NULL);
    result = failure_count == 0 ? 0 : 1;

cleanup:
    if (renderer != NULL)
        Renderer_destroy(renderer);
    if (context != NULL)
        SDL_GL_DeleteContext(context);
    if (window != NULL)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

static int component_near(GLubyte actual, int expected, int tolerance)
{
    int difference = (int)actual - expected;

    if (difference < 0)
        difference = -difference;
    return difference <= tolerance;
}

static void check_pixel(const GLubyte *pixels, int x, int y,
                        int red, int green, int blue, int tolerance)
{
    const GLubyte *pixel = pixels
        + (y * BASELINE_SIZE + x) * PIXEL_COMPONENTS;

    CHECK_CONTINUE(component_near(pixel[0], red, tolerance));
    CHECK_CONTINUE(component_near(pixel[1], green, tolerance));
    CHECK_CONTINUE(component_near(pixel[2], blue, tolerance));
}

static void draw_quad(GLfloat left, GLfloat bottom,
                      GLfloat right, GLfloat top)
{
    glBegin(GL_QUADS);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glEnd();
}

static void check_procedural_baseline(void)
{
    framebuffer_api_t api;
    GLuint framebuffer = 0;
    GLuint color_texture = 0;
    GLuint sample_texture = 0;
    GLubyte sample_color[] = {255, 255, 0, 255};
    GLubyte pixels[BASELINE_SIZE * BASELINE_SIZE * PIXEL_COMPONENTS];
    int framebuffer_api_loaded;

    framebuffer_api_loaded = load_framebuffer_api(&api);
    CHECK_CONTINUE(framebuffer_api_loaded);
    if (!framebuffer_api_loaded)
        return;

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BASELINE_SIZE, BASELINE_SIZE,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    api.gen(1, &framebuffer);
    api.bind(GL_FRAMEBUFFER, framebuffer);
    api.attach_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                       color_texture, 0);
    CHECK_CONTINUE(api.check_status(GL_FRAMEBUFFER)
                   == GL_FRAMEBUFFER_COMPLETE);

    glViewport(0, 0, BASELINE_SIZE, BASELINE_SIZE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, BASELINE_SIZE, 0.0, BASELINE_SIZE, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4ub(255, 0, 0, 255);
    draw_quad(0.0f, 0.0f, 4.0f, 4.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(0, 0, 255, 128);
    draw_quad(2.0f, 0.0f, 6.0f, 4.0f);
    glDisable(GL_BLEND);

    glEnable(GL_SCISSOR_TEST);
    glScissor(8, 0, 2, 2);
    glColor4ub(0, 255, 0, 255);
    draw_quad(0.0f, 0.0f, BASELINE_SIZE, BASELINE_SIZE);
    glDisable(GL_SCISSOR_TEST);

    glGenTextures(1, &sample_texture);
    glBindTexture(GL_TEXTURE_2D, sample_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, sample_color);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(4.0f, 8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(4.0f, 12.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 12.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glColor4ub(255, 255, 255, 255);
    glBegin(GL_TRIANGLES);
    glVertex2f(6.0f, 8.0f);
    glVertex2f(12.0f, 8.0f);
    glVertex2f(9.0f, 12.0f);
    glEnd();

    glFinish();
    memset(pixels, 0, sizeof(pixels));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, BASELINE_SIZE, BASELINE_SIZE,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    check_pixel(pixels, 1, 1, 255, 0, 0, 0);
    check_pixel(pixels, 3, 1, 127, 0, 128, 2);
    check_pixel(pixels, 5, 1, 0, 0, 128, 2);
    check_pixel(pixels, 8, 1, 0, 255, 0, 0);
    check_pixel(pixels, 7, 1, 0, 0, 0, 0);
    check_pixel(pixels, 8, 3, 0, 0, 0, 0);
    check_pixel(pixels, 1, 9, 255, 255, 0, 0);
    check_pixel(pixels, 9, 9, 255, 255, 255, 0);
    check_pixel(pixels, 11, 6, 0, 0, 0, 0);
    CHECK_CONTINUE(drain_gl_errors() == 0);

    api.bind(GL_FRAMEBUFFER, 0);
    api.destroy(1, &framebuffer);
    glDeleteTextures(1, &sample_texture);
    glDeleteTextures(1, &color_texture);
}

static int font_state_is_empty(const font_data *font)
{
    return font->requested_height == 0 && font->atlas == NULL
        && font->text_renderer == NULL;
}

static void check_font_renderer_lifecycle(Renderer *renderer)
{
    const RendererColor clear = {0, 0, 0, 255};
    const TextGeometryFont *geometry;
    SdlRenderer sdl_renderer = {1};
    SdlRenderer other_sdl_renderer = {2};
    font_data font;
    TextAtlas *retained_atlas;
    TextRenderer *retained_text_renderer;
    RendererStatus status;
    int frame_active = 0;

    memset(&font, 0, sizeof(font));
    captured_font_list_generations = 0;
    captured_font_texture_count = 0;
    ttf_open_count = 0;
    ttf_close_count = 0;
    capture_font_resources = 1;
    status = fontinit(&font, renderer, XPILOT_TEST_FONT_PATH, 17);
    capture_font_resources = 0;
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK) {
        CHECK_CONTINUE(font_state_is_empty(&font));
        return;
    }

    CHECK_CONTINUE(font.atlas != NULL);
    CHECK_CONTINUE(font.requested_height == 17);
    CHECK_CONTINUE(captured_font_list_generations == 0);
    CHECK_CONTINUE(captured_font_texture_count == 1);
    CHECK_CONTINUE(ttf_open_count == 1);
    CHECK_CONTINUE(ttf_close_count == 1);
    if (font.atlas != NULL) {
        geometry = Text_atlas_geometry_font(font.atlas);
        CHECK_CONTINUE(geometry != NULL);
        CHECK_CONTINUE(Text_atlas_texture(font.atlas) != NULL);
        if (geometry != NULL) {
            CHECK_CONTINUE(geometry->glyphs[(unsigned char)'A'].available);
            CHECK_CONTINUE(geometry->line_spacing > 0.0f);
        }
    }

    text_renderer_create_calls = 0;
    text_renderer_destroy_calls = 0;
    text_renderer_create_result = RENDERER_STATUS_OUT_OF_MEMORY;
    retained_atlas = font.atlas;
    status = font_text_renderer_attach(&font, &sdl_renderer);
    CHECK_CONTINUE(status == RENDERER_STATUS_OUT_OF_MEMORY);
    CHECK_CONTINUE(font.text_renderer == NULL);
    CHECK_CONTINUE(font.atlas == retained_atlas);
    CHECK_CONTINUE(font.requested_height == 17);
    CHECK_CONTINUE(text_renderer_create_calls == 1);
    CHECK_CONTINUE(text_renderer_destroy_calls == 0);

    text_renderer_create_result = RENDERER_STATUS_OK;
    status = font_text_renderer_attach(&font, &sdl_renderer);
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    CHECK_CONTINUE(font.text_renderer != NULL);
    if (font.text_renderer != NULL) {
        CHECK_CONTINUE(font.text_renderer->sdl_renderer == &sdl_renderer);
        CHECK_CONTINUE(font.text_renderer->atlas == font.atlas);
    }
    CHECK_CONTINUE(text_renderer_create_calls == 2);
    CHECK_CONTINUE(text_renderer_destroy_calls == 0);
    retained_text_renderer = font.text_renderer;

    status = font_text_renderer_attach(&font, &sdl_renderer);
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    CHECK_CONTINUE(font.text_renderer == retained_text_renderer);
    CHECK_CONTINUE(text_renderer_create_calls == 2);
    CHECK_CONTINUE(text_renderer_destroy_calls == 0);

    status = font_text_renderer_attach(&font, &other_sdl_renderer);
    CHECK_CONTINUE(status == RENDERER_STATUS_RESOURCE_MISMATCH);
    CHECK_CONTINUE(font.text_renderer == retained_text_renderer);
    CHECK_CONTINUE(font.atlas == retained_atlas);
    CHECK_CONTINUE(font.requested_height == 17);
    CHECK_CONTINUE(text_renderer_create_calls == 2);
    CHECK_CONTINUE(text_renderer_destroy_calls == 0);

    retained_atlas = font.atlas;
    status = Renderer_begin_frame(renderer, 64, 64, clear);
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    if (status == RENDERER_STATUS_OK)
        frame_active = 1;

    if (frame_active) {
        status = fontclean(&font);
        CHECK_CONTINUE(status == RENDERER_STATUS_INVALID_STATE);
        CHECK_CONTINUE(font.atlas == retained_atlas);
        CHECK_CONTINUE(font.text_renderer == retained_text_renderer);
        CHECK_CONTINUE(text_renderer_destroy_calls == 0);
        CHECK_CONTINUE(font.requested_height == 17);

        status = Renderer_end_frame(renderer);
        CHECK_CONTINUE(status == RENDERER_STATUS_OK);
        if (status == RENDERER_STATUS_OK)
            frame_active = 0;
    }

    if (!frame_active) {
        status = fontclean(&font);
        CHECK_CONTINUE(status == RENDERER_STATUS_OK);
        CHECK_CONTINUE(font_state_is_empty(&font));
        CHECK_CONTINUE(text_renderer_destroy_calls == 1);
        CHECK_CONTINUE(fontclean(&font) == RENDERER_STATUS_OK);
    }
}

static void check_active_frame_font_init_rolls_back(Renderer *renderer)
{
    const RendererColor clear = {0, 0, 0, 255};
    font_data candidate;
    RendererStatus status;
    int frame_active = 0;

    memset(&candidate, 0, sizeof(candidate));
    captured_font_list_generations = 0;
    captured_font_texture_count = 0;
    ttf_open_count = 0;
    ttf_close_count = 0;

    status = Renderer_begin_frame(renderer, 64, 64, clear);
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
        return;
    frame_active = 1;

    capture_font_resources = 1;
    status = fontinit(&candidate, renderer, XPILOT_TEST_FONT_PATH, 17);
    capture_font_resources = 0;
    CHECK_CONTINUE(status == RENDERER_STATUS_INVALID_STATE);
    CHECK_CONTINUE(font_state_is_empty(&candidate));
    CHECK_CONTINUE(captured_font_list_generations == 0);
    CHECK_CONTINUE(captured_font_texture_count == 0);
    CHECK_CONTINUE(ttf_open_count == 1);
    CHECK_CONTINUE(ttf_close_count == 1);

    status = Renderer_end_frame(renderer);
    CHECK_CONTINUE(status == RENDERER_STATUS_OK);
    if (status == RENDERER_STATUS_OK)
        frame_active = 0;
    if (!frame_active)
        CHECK_CONTINUE(fontclean(&candidate) == RENDERER_STATUS_OK);
}

static void check_diagnostics(void)
{
    Gl_diagnostics_reset();
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 0);

    glEnable(GL_TEXTURE_WIDTH);
    CHECK_CONTINUE(Gl_diagnostics_check("unit-injected") == 1);
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 1);
    CHECK_CONTINUE(glGetError() == GL_NO_ERROR);
    CHECK_CONTINUE(Gl_diagnostics_check("unit-empty") == 0);
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 1);

    Gl_diagnostics_reset();
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 0);
}

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_GLContext context;
    Renderer *font_renderer = NULL;

    if (argc == 2 && strcmp(argv[1], CONTEXT_LOG_GL_1_5_ARGUMENT) == 0)
        return check_context_logging_on_gl_1_5();
    if (argc != 1) {
        fprintf(stderr, "Unexpected test argument\n");
        return 2;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video initialization failed: %s\n",
                SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("XPilot legacy OpenGL test",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        fprintf(stderr, "SDL OpenGL window creation failed: %s\n",
                SDL_GetError());
        SDL_Quit();
        return 1;
    }
    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SDL OpenGL context creation failed: %s\n",
                SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf initialization failed: %s\n",
                TTF_GetError());
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    CHECK_CONTINUE(glGetString(GL_VERSION) != NULL);
    check_procedural_baseline();
    CHECK_CONTINUE(Renderer_gl_legacy_create(load_current_gl_proc, NULL,
                                             &font_renderer)
                   == RENDERER_STATUS_OK);
    if (font_renderer != NULL) {
        check_font_renderer_lifecycle(font_renderer);
        check_active_frame_font_init_rolls_back(font_renderer);
        Renderer_destroy(font_renderer);
    }
    check_diagnostics();

    TTF_Quit();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failure_count != 0) {
        fprintf(stderr, "%d legacy OpenGL checks failed\n", failure_count);
        return 1;
    }
    return 0;
}
