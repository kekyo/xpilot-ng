#include "test_helpers.h"

#include "xpclient_sdl.h"
#include "gl_diagnostics.h"
#include "renderer.h"
#include "renderer_gl_core.h"
#include "sdlrenderer.h"
#include "text.h"
#include "text_atlas.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef XPILOT_TEST_FONT_PATH
#error "XPILOT_TEST_FONT_PATH must name the bundled test font"
#endif

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
static int capture_font_resources;
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

extern TTF_Font *__real_TTF_OpenFont(const char *file, int ptsize);
extern void __real_TTF_CloseFont(TTF_Font *font);

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

static void APIENTRY capture_gl_gen_textures(GLsizei count, GLuint *textures)
{
    glGenTextures(count, textures);
    if (capture_font_resources && count > 0)
        captured_font_texture_count += (size_t)count;
}

static void *load_font_gl_proc(void *userdata, const char *name)
{
    (void)userdata;
    if (strcmp(name, "glGenTextures") == 0)
        return (void *)capture_gl_gen_textures;
    return SDL_GL_GetProcAddress(name);
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

int main(void)
{
    SDL_Window *window = NULL;
    SDL_GLContext context = NULL;
    Renderer *font_renderer = NULL;
    int actual_major = 0;
    int actual_minor = 0;
    int actual_profile = 0;
    int ttf_initialized = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video initialization failed: %s\n",
                SDL_GetError());
        return 1;
    }
    SDL_GL_ResetAttributes();
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0
        || SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                               SDL_GL_CONTEXT_PROFILE_CORE) != 0) {
        fprintf(stderr, "OpenGL core attribute request failed: %s\n",
                SDL_GetError());
        failure_count++;
        goto cleanup;
    }
    window = SDL_CreateWindow("XPilot OpenGL core resource test",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        fprintf(stderr, "SDL OpenGL window creation failed: %s\n",
                SDL_GetError());
        failure_count++;
        goto cleanup;
    }
    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SDL OpenGL context creation failed: %s\n",
                SDL_GetError());
        failure_count++;
        goto cleanup;
    }
    CHECK_CONTINUE(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,
                                       &actual_major) == 0);
    CHECK_CONTINUE(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,
                                       &actual_minor) == 0);
    CHECK_CONTINUE(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                       &actual_profile) == 0);
    CHECK_CONTINUE(actual_major > 3
                   || (actual_major == 3 && actual_minor >= 3));
    CHECK_CONTINUE(actual_profile == SDL_GL_CONTEXT_PROFILE_CORE);
    CHECK_CONTINUE(glGetString(GL_VERSION) != NULL);

    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf initialization failed: %s\n",
                TTF_GetError());
        failure_count++;
        goto cleanup;
    }
    ttf_initialized = 1;
    CHECK_CONTINUE(Renderer_gl_core_create(load_font_gl_proc, NULL,
                                           &font_renderer)
                   == RENDERER_STATUS_OK);
    if (font_renderer != NULL) {
        check_font_renderer_lifecycle(font_renderer);
        check_active_frame_font_init_rolls_back(font_renderer);
        Renderer_destroy(font_renderer);
        font_renderer = NULL;
    }
    check_diagnostics();

cleanup:
    if (font_renderer != NULL)
        Renderer_destroy(font_renderer);
    if (ttf_initialized)
        TTF_Quit();
    if (context != NULL)
        SDL_GL_DeleteContext(context);
    if (window != NULL)
        SDL_DestroyWindow(window);
    SDL_Quit();

    if (failure_count != 0) {
        fprintf(stderr, "%d OpenGL core resource checks failed\n",
                failure_count);
        return 1;
    }
    return 0;
}
