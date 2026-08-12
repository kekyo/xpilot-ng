/* SDL window and compatibility OpenGL renderer coordination. */

#include "sdlrenderer.h"

#include "renderer_gl_legacy.h"

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct SdlRenderer {
    SDL_Window *window;
    Renderer *frontend;
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLGETPROGRAMIVPROC get_program_iv;
    PFNGLUSEPROGRAMPROC use_program;
    int logical_width;
    int logical_height;
    int drawable_width;
    int drawable_height;
    int frame_active;
    RendererStatus frame_result;
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
    Sdl_renderer_copy_proc(&created->get_program_iv,
                           sizeof(created->get_program_iv),
                           "glGetProgramiv");
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

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    return renderer->frame_result;
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

RendererStatus Sdl_renderer_set_transform_2d(
    SdlRenderer *renderer, RendererTransform2D transform)
{
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    status = Sdl_renderer_track_frame_result(renderer, RENDERER_STATUS_OK);
    if (status != RENDERER_STATUS_OK)
        return status;
    status = Renderer_set_transform_2d(renderer->frontend, transform);
    return Sdl_renderer_track_frame_result(renderer, status);
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
    renderer->frame_result = RENDERER_STATUS_OK;
    return RENDERER_STATUS_OK;
}

static int64_t Sdl_renderer_clamp_logical_edge(int64_t edge, int limit)
{
    if (edge < 0)
        return 0;
    if (edge > limit)
        return limit;
    return edge;
}

static int Sdl_renderer_scale_edge_floor(int64_t logical_edge,
                                         int logical_size,
                                         int drawable_size)
{
    return (int)((logical_edge * (int64_t)drawable_size)
                 / logical_size);
}

static int Sdl_renderer_scale_edge_ceil(int64_t logical_edge,
                                        int logical_size,
                                        int drawable_size)
{
    int64_t numerator = logical_edge * (int64_t)drawable_size;

    return (int)((numerator + logical_size - 1) / logical_size);
}

RendererStatus Sdl_renderer_set_logical_scissor(
    SdlRenderer *renderer, const RendererRect *scissor)
{
    RendererRect drawable_scissor;
    int64_t logical_left;
    int64_t logical_top;
    int64_t logical_right;
    int64_t logical_bottom;
    int drawable_left;
    int drawable_top;
    int drawable_right;
    int drawable_bottom;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (scissor == NULL)
        return Renderer_set_scissor(renderer->frontend, NULL);
    if (scissor->width <= 0 || scissor->height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;

    logical_left = Sdl_renderer_clamp_logical_edge(
        scissor->x, renderer->logical_width);
    logical_top = Sdl_renderer_clamp_logical_edge(
        scissor->y, renderer->logical_height);
    logical_right = Sdl_renderer_clamp_logical_edge(
        (int64_t)scissor->x + scissor->width,
        renderer->logical_width);
    logical_bottom = Sdl_renderer_clamp_logical_edge(
        (int64_t)scissor->y + scissor->height,
        renderer->logical_height);

    if (logical_right <= logical_left || logical_bottom <= logical_top) {
        drawable_scissor.x = renderer->drawable_width;
        drawable_scissor.y = renderer->drawable_height;
        drawable_scissor.width = 1;
        drawable_scissor.height = 1;
        return Renderer_set_scissor(renderer->frontend,
                                    &drawable_scissor);
    }

    /* Clamping bounds every factor by INT_MAX, so the int64_t products and
     * the ceil adjustment cannot overflow. */
    drawable_left = Sdl_renderer_scale_edge_floor(
        logical_left, renderer->logical_width,
        renderer->drawable_width);
    drawable_top = Sdl_renderer_scale_edge_floor(
        logical_top, renderer->logical_height,
        renderer->drawable_height);
    drawable_right = Sdl_renderer_scale_edge_ceil(
        logical_right, renderer->logical_width,
        renderer->drawable_width);
    drawable_bottom = Sdl_renderer_scale_edge_ceil(
        logical_bottom, renderer->logical_height,
        renderer->drawable_height);
    drawable_scissor.x = drawable_left;
    drawable_scissor.y = drawable_top;
    drawable_scissor.width = drawable_right - drawable_left;
    drawable_scissor.height = drawable_bottom - drawable_top;
    return Renderer_set_scissor(renderer->frontend, &drawable_scissor);
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

RendererStatus Sdl_renderer_flush_preserving_legacy(
    SdlRenderer *renderer)
{
    RendererStatus flush_status = RENDERER_STATUS_BACKEND_ERROR;
    RendererStatus restore_status;
    GLint texture_unit_count = 0;
    GLint current_program = 0;
    GLint current_program_delete_pending = GL_FALSE;
    GLint current_program_linked = GL_TRUE;
    GLint current_active_texture = GL_TEXTURE0;
    GLint current_matrix_mode = GL_MODELVIEW;
    GLint texture_unit;
    GLint texture_matrices_pushed = 0;
    int projection_pushed = 0;
    int modelview_pushed = 0;
    int attribute_pushed = 0;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (!renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;

    /* Renderer backends intentionally drain GL errors at command boundaries.
     * Reject an error owned by preceding direct drawing before saving state. */
    if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK)
        return RENDERER_STATUS_BACKEND_ERROR;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_unit_count);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &current_active_texture);
    glGetIntegerv(GL_MATRIX_MODE, &current_matrix_mode);
    if (renderer->use_program != NULL)
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    if (texture_unit_count <= 0
        || Sdl_renderer_gl_result() != RENDERER_STATUS_OK) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    /* A delete-pending program ceases to exist when it is unbound, and a
     * failed relink leaves its old executable usable only while it remains
     * current.  The semantic backend temporarily binds program zero, so
     * reject either state before changing GL state or delivering commands. */
    if (current_program != 0) {
        if (renderer->get_program_iv == NULL)
            return RENDERER_STATUS_BACKEND_ERROR;
        renderer->get_program_iv((GLuint)current_program, GL_DELETE_STATUS,
                                 &current_program_delete_pending);
        renderer->get_program_iv((GLuint)current_program, GL_LINK_STATUS,
                                 &current_program_linked);
        if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK
            || current_program_delete_pending == GL_TRUE
            || current_program_linked != GL_TRUE) {
            return RENDERER_STATUS_BACKEND_ERROR;
        }
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK)
        return RENDERER_STATUS_BACKEND_ERROR;
    attribute_pushed = 1;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK)
        goto restore_state;
    projection_pushed = 1;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK)
        goto restore_state;
    modelview_pushed = 1;
    for (texture_unit = 0; texture_unit < texture_unit_count;
         texture_unit++) {
        renderer->active_texture((GLenum)(GL_TEXTURE0 + texture_unit));
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        if (Sdl_renderer_gl_result() != RENDERER_STATUS_OK)
            goto restore_state;
        texture_matrices_pushed++;
    }

    flush_status = Renderer_flush(renderer->frontend);

restore_state:
    for (texture_unit = texture_matrices_pushed - 1; texture_unit >= 0;
         texture_unit--) {
        renderer->active_texture((GLenum)(GL_TEXTURE0 + texture_unit));
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
    }
    if (modelview_pushed) {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    if (projection_pushed) {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    if (attribute_pushed)
        glPopAttrib();
    if (renderer->use_program != NULL)
        renderer->use_program((GLuint)current_program);
    renderer->active_texture((GLenum)current_active_texture);
    glMatrixMode((GLenum)current_matrix_mode);
    restore_status = Sdl_renderer_gl_result();
    if (restore_status != RENDERER_STATUS_OK)
        return restore_status;
    return flush_status;
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
