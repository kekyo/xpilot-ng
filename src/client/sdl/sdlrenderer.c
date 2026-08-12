/* SDL window and compatibility OpenGL renderer coordination. */

#include "sdlrenderer.h"

#include "renderer_gl_legacy.h"

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <stdlib.h>
#include <string.h>

struct SdlRenderer {
    SDL_Window *window;
    Renderer *frontend;
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLUSEPROGRAMPROC use_program;
    int logical_width;
    int logical_height;
    int drawable_width;
    int drawable_height;
    int frame_active;
};

static void *Sdl_renderer_load_proc(void *userdata, const char *name)
{
    (void)userdata;
    return SDL_GL_GetProcAddress(name);
}

static void Sdl_renderer_copy_proc(void *destination, size_t size,
                                   const char *name)
{
    void *address = SDL_GL_GetProcAddress(name);

    memset(destination, 0, size);
    if (address != NULL) {
        size_t copy_size = size < sizeof(address) ? size : sizeof(address);

        memcpy(destination, &address, copy_size);
    }
}

static RendererStatus Sdl_renderer_gl_result(void)
{
    int failed = 0;

    while (glGetError() != GL_NO_ERROR)
        failed = 1;
    return failed ? RENDERER_STATUS_BACKEND_ERROR : RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_create(SDL_Window *window,
                                   SdlRenderer **renderer)
{
    SdlRenderer *created;
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *renderer = NULL;
    if (window == NULL || SDL_GL_GetCurrentContext() == NULL
        || SDL_GL_GetCurrentWindow() != window) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    created = calloc(1, sizeof(*created));
    if (created == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    created->window = window;
    status = Renderer_gl_legacy_create(Sdl_renderer_load_proc, NULL,
                                       &created->frontend);
    if (status != RENDERER_STATUS_OK) {
        free(created);
        return status;
    }

    Sdl_renderer_copy_proc(&created->active_texture,
                           sizeof(created->active_texture),
                           "glActiveTexture");
    if (created->active_texture == NULL) {
        Sdl_renderer_copy_proc(&created->active_texture,
                               sizeof(created->active_texture),
                               "glActiveTextureARB");
    }
    Sdl_renderer_copy_proc(&created->use_program,
                           sizeof(created->use_program), "glUseProgram");
    if (created->active_texture == NULL) {
        Renderer_destroy(created->frontend);
        free(created);
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    *renderer = created;
    return RENDERER_STATUS_OK;
}

void Sdl_renderer_destroy(SdlRenderer *renderer)
{
    if (renderer == NULL)
        return;
    Renderer_destroy(renderer->frontend);
    free(renderer);
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer != NULL ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_get_drawable_size(SdlRenderer *renderer,
                                              int *width, int *height)
{
    int drawable_width;
    int drawable_height;

    if (renderer == NULL || width == NULL || height == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    SDL_GL_GetDrawableSize(renderer->window, &drawable_width,
                           &drawable_height);
    if (drawable_width <= 0 || drawable_height <= 0)
        return RENDERER_STATUS_BACKEND_ERROR;
    *width = drawable_width;
    *height = drawable_height;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_begin_frame(SdlRenderer *renderer,
                                        int logical_width,
                                        int logical_height,
                                        RendererColor clear_color)
{
    RendererStatus status;
    int drawable_width;
    int drawable_height;

    if (renderer == NULL || logical_width <= 0 || logical_height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    status = Sdl_renderer_get_drawable_size(renderer, &drawable_width,
                                            &drawable_height);
    if (status != RENDERER_STATUS_OK)
        return status;
    status = Renderer_begin_frame(renderer->frontend, drawable_width,
                                  drawable_height, clear_color);
    if (status != RENDERER_STATUS_OK)
        return status;

    renderer->logical_width = logical_width;
    renderer->logical_height = logical_height;
    renderer->drawable_width = drawable_width;
    renderer->drawable_height = drawable_height;
    renderer->frame_active = 1;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_prepare_legacy(
    SdlRenderer *renderer, SdlRendererLegacyOrigin origin)
{
    RendererStatus status;
    GLint texture_unit_count = 0;
    GLint texture_unit;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (origin != SDL_RENDERER_LEGACY_BOTTOM_LEFT
        && origin != SDL_RENDERER_LEGACY_TOP_LEFT) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }

    /* Do not let the semantic backend silently drain an error left by the
     * preceding direct-OpenGL range.  The queued command remains retryable. */
    status = Sdl_renderer_gl_result();
    if (status != RENDERER_STATUS_OK)
        return status;
    status = Renderer_flush(renderer->frontend);
    if (status != RENDERER_STATUS_OK)
        return status;

    if (renderer->use_program != NULL)
        renderer->use_program(0);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_unit_count);
    for (texture_unit = 0; texture_unit < texture_unit_count;
         texture_unit++) {
        renderer->active_texture((GLenum)(GL_TEXTURE0 + texture_unit));
        glDisable(GL_TEXTURE_2D);
    }
    renderer->active_texture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor4ub(255, 255, 255, 255);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glViewport(0, 0, renderer->drawable_width, renderer->drawable_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (origin == SDL_RENDERER_LEGACY_BOTTOM_LEFT) {
        glOrtho(0.0, (GLdouble)renderer->logical_width,
                0.0, (GLdouble)renderer->logical_height, -1.0, 1.0);
    } else {
        glOrtho(0.0, (GLdouble)renderer->logical_width,
                (GLdouble)renderer->logical_height, 0.0, -1.0, 1.0);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    return Sdl_renderer_gl_result();
}

RendererStatus Sdl_renderer_end_frame(SdlRenderer *renderer)
{
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    status = Renderer_end_frame(renderer->frontend);
    if (status == RENDERER_STATUS_OK) {
        renderer->logical_width = 0;
        renderer->logical_height = 0;
        renderer->drawable_width = 0;
        renderer->drawable_height = 0;
        renderer->frame_active = 0;
    }
    return status;
}
