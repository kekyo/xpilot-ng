/* Fixed-function OpenGL compatibility renderer backend. */

#include "renderer_gl_legacy.h"
#include "renderer_test_support.h"

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_CONTEXT_PROFILE_MASK
# define GL_CONTEXT_PROFILE_MASK 0x9126
#endif

#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
# define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

#ifndef GL_CONTEXT_FLAGS
# define GL_CONTEXT_FLAGS 0x821E
#endif

#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
# define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#endif

#ifndef GL_NUM_EXTENSIONS
# define GL_NUM_EXTENSIONS 0x821D
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
# define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER_BINDING
# define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#endif

typedef const GLubyte *(APIENTRYP LegacyGetString)(GLenum name);
typedef const GLubyte *(APIENTRYP LegacyGetStringi)(GLenum name,
                                                   GLuint index);
typedef void (APIENTRYP LegacyGetIntegerv)(GLenum name, GLint *value);
typedef GLenum (APIENTRYP LegacyGetError)(void);
typedef void (APIENTRYP LegacyActiveTexture)(GLenum texture);
typedef void (APIENTRYP LegacyBindBuffer)(GLenum target, GLuint buffer);
typedef void (APIENTRYP LegacyUseProgram)(GLuint program);
typedef void (APIENTRYP LegacyViewport)(GLint x, GLint y,
                                       GLsizei width, GLsizei height);
typedef void (APIENTRYP LegacyMatrixMode)(GLenum mode);
typedef void (APIENTRYP LegacyLoadIdentity)(void);
typedef void (APIENTRYP LegacyOrtho)(GLdouble left, GLdouble right,
                                    GLdouble bottom, GLdouble top,
                                    GLdouble near_value,
                                    GLdouble far_value);
typedef void (APIENTRYP LegacyLoadMatrixf)(const GLfloat *matrix);
typedef void (APIENTRYP LegacyEnable)(GLenum capability);
typedef void (APIENTRYP LegacyDisable)(GLenum capability);
typedef void (APIENTRYP LegacyClearColor)(GLclampf red, GLclampf green,
                                         GLclampf blue, GLclampf alpha);
typedef void (APIENTRYP LegacyClear)(GLbitfield mask);
typedef void (APIENTRYP LegacyColorMask)(GLboolean red, GLboolean green,
                                        GLboolean blue, GLboolean alpha);
typedef void (APIENTRYP LegacyShadeModel)(GLenum mode);
typedef void (APIENTRYP LegacyPolygonMode)(GLenum face, GLenum mode);
typedef void (APIENTRYP LegacyBlendEquation)(GLenum equation);
typedef void (APIENTRYP LegacyBlendFunc)(GLenum source, GLenum destination);
typedef void (APIENTRYP LegacyScissor)(GLint x, GLint y,
                                      GLsizei width, GLsizei height);
typedef void (APIENTRYP LegacyBegin)(GLenum mode);
typedef void (APIENTRYP LegacyEnd)(void);
typedef void (APIENTRYP LegacyColor4ub)(GLubyte red, GLubyte green,
                                       GLubyte blue, GLubyte alpha);
typedef void (APIENTRYP LegacyTexCoord2f)(GLfloat u, GLfloat v);
typedef void (APIENTRYP LegacyVertex2f)(GLfloat x, GLfloat y);
typedef void (APIENTRYP LegacyFlush)(void);
typedef void (APIENTRYP LegacyGenTextures)(GLsizei count, GLuint *textures);
typedef void (APIENTRYP LegacyBindTexture)(GLenum target, GLuint texture);
typedef void (APIENTRYP LegacyTexParameteri)(GLenum target, GLenum name,
                                            GLint value);
typedef void (APIENTRYP LegacyTexEnvi)(GLenum target, GLenum name,
                                      GLint value);
typedef void (APIENTRYP LegacyPixelStorei)(GLenum name, GLint value);
typedef void (APIENTRYP LegacyTexImage2D)(GLenum target, GLint level,
                                         GLint internal_format,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLenum format,
                                         GLenum type, const GLvoid *pixels);
typedef void (APIENTRYP LegacyTexSubImage2D)(GLenum target, GLint level,
                                            GLint x, GLint y,
                                            GLsizei width, GLsizei height,
                                            GLenum format, GLenum type,
                                            const GLvoid *pixels);
typedef void (APIENTRYP LegacyDeleteTextures)(GLsizei count,
                                             const GLuint *textures);

typedef struct LegacyFunctions {
    LegacyGetString get_string;
    LegacyGetStringi get_string_indexed;
    LegacyGetIntegerv get_integer;
    LegacyGetError get_error;
    LegacyActiveTexture active_texture;
    LegacyBindBuffer bind_buffer;
    LegacyUseProgram use_program;
    LegacyViewport viewport;
    LegacyMatrixMode matrix_mode;
    LegacyLoadIdentity load_identity;
    LegacyOrtho ortho;
    LegacyLoadMatrixf load_matrix;
    LegacyEnable enable;
    LegacyDisable disable;
    LegacyClearColor clear_color;
    LegacyClear clear;
    LegacyColorMask color_mask;
    LegacyShadeModel shade_model;
    LegacyPolygonMode polygon_mode;
    LegacyBlendEquation blend_equation;
    LegacyBlendFunc blend_func;
    LegacyScissor scissor;
    LegacyBegin begin;
    LegacyEnd end;
    LegacyColor4ub color;
    LegacyTexCoord2f tex_coord;
    LegacyVertex2f vertex;
    LegacyFlush flush;
    LegacyGenTextures gen_textures;
    LegacyBindTexture bind_texture;
    LegacyTexParameteri texture_parameter;
    LegacyTexEnvi texture_environment;
    LegacyPixelStorei pixel_store;
    LegacyTexImage2D texture_image;
    LegacyTexSubImage2D texture_sub_image;
    LegacyDeleteTextures delete_textures;
} LegacyFunctions;

typedef struct LegacyContext {
    LegacyFunctions gl;
    int frame_height;
    int pixel_unpack_buffer_supported;
} LegacyContext;

typedef struct LegacyTexture {
    GLuint name;
    int width;
    int height;
} LegacyTexture;

typedef struct LegacyUploadState {
    GLint active_texture;
    GLint texture;
    GLint unpack_buffer;
    GLint unpack_alignment;
    GLint unpack_row_length;
    GLint unpack_skip_rows;
    GLint unpack_skip_pixels;
} LegacyUploadState;

typedef struct LegacyMesh {
    RendererVertex2D *vertices;
    size_t vertex_count;
} LegacyMesh;

static int Legacy_load_functions(LegacyContext *context,
                                 RendererGLProcLoader loader,
                                 void *userdata)
{
#define LOAD(member, type, symbol)                                          \
    do {                                                                    \
        context->gl.member = (type)loader(userdata, symbol);                 \
        if (context->gl.member == NULL)                                     \
            return 0;                                                       \
    } while (0)

    LOAD(get_string, LegacyGetString, "glGetString");
    context->gl.get_string_indexed = (LegacyGetStringi)loader(
        userdata, "glGetStringi");
    LOAD(get_integer, LegacyGetIntegerv, "glGetIntegerv");
    LOAD(get_error, LegacyGetError, "glGetError");
    context->gl.active_texture = (LegacyActiveTexture)loader(
        userdata, "glActiveTexture");
    if (context->gl.active_texture == NULL) {
        context->gl.active_texture = (LegacyActiveTexture)loader(
            userdata, "glActiveTextureARB");
    }
    if (context->gl.active_texture == NULL)
        return 0;
    context->gl.bind_buffer = (LegacyBindBuffer)loader(
        userdata, "glBindBuffer");
    if (context->gl.bind_buffer == NULL) {
        context->gl.bind_buffer = (LegacyBindBuffer)loader(
            userdata, "glBindBufferARB");
    }
    context->gl.use_program = (LegacyUseProgram)loader(
        userdata, "glUseProgram");
    LOAD(viewport, LegacyViewport, "glViewport");
    LOAD(matrix_mode, LegacyMatrixMode, "glMatrixMode");
    LOAD(load_identity, LegacyLoadIdentity, "glLoadIdentity");
    LOAD(ortho, LegacyOrtho, "glOrtho");
    LOAD(load_matrix, LegacyLoadMatrixf, "glLoadMatrixf");
    LOAD(enable, LegacyEnable, "glEnable");
    LOAD(disable, LegacyDisable, "glDisable");
    LOAD(clear_color, LegacyClearColor, "glClearColor");
    LOAD(clear, LegacyClear, "glClear");
    LOAD(color_mask, LegacyColorMask, "glColorMask");
    LOAD(shade_model, LegacyShadeModel, "glShadeModel");
    LOAD(polygon_mode, LegacyPolygonMode, "glPolygonMode");
    context->gl.blend_equation = (LegacyBlendEquation)loader(
        userdata, "glBlendEquation");
    if (context->gl.blend_equation == NULL) {
        context->gl.blend_equation = (LegacyBlendEquation)loader(
            userdata, "glBlendEquationEXT");
    }
    if (context->gl.blend_equation == NULL)
        return 0;
    LOAD(blend_func, LegacyBlendFunc, "glBlendFunc");
    LOAD(scissor, LegacyScissor, "glScissor");
    LOAD(begin, LegacyBegin, "glBegin");
    LOAD(end, LegacyEnd, "glEnd");
    LOAD(color, LegacyColor4ub, "glColor4ub");
    LOAD(tex_coord, LegacyTexCoord2f, "glTexCoord2f");
    LOAD(vertex, LegacyVertex2f, "glVertex2f");
    LOAD(flush, LegacyFlush, "glFlush");
    LOAD(gen_textures, LegacyGenTextures, "glGenTextures");
    LOAD(bind_texture, LegacyBindTexture, "glBindTexture");
    LOAD(texture_parameter, LegacyTexParameteri, "glTexParameteri");
    LOAD(texture_environment, LegacyTexEnvi, "glTexEnvi");
    LOAD(pixel_store, LegacyPixelStorei, "glPixelStorei");
    LOAD(texture_image, LegacyTexImage2D, "glTexImage2D");
    LOAD(texture_sub_image, LegacyTexSubImage2D, "glTexSubImage2D");
    LOAD(delete_textures, LegacyDeleteTextures, "glDeleteTextures");

#undef LOAD
    return 1;
}

static void Legacy_discard_errors(LegacyContext *context)
{
    while (context->gl.get_error() != GL_NO_ERROR)
        ;
}

static RendererStatus Legacy_result(LegacyContext *context)
{
    int failed = 0;

    while (context->gl.get_error() != GL_NO_ERROR)
        failed = 1;
    return failed ? RENDERER_STATUS_BACKEND_ERROR : RENDERER_STATUS_OK;
}

static int Legacy_version_at_least(int major, int minor,
                                   int required_major,
                                   int required_minor)
{
    return major > required_major
        || (major == required_major && minor >= required_minor);
}

static int Legacy_extension_string_contains(const char *extensions,
                                            const char *name)
{
    size_t name_length = strlen(name);
    const char *match = extensions;

    while ((match = strstr(match, name)) != NULL) {
        if ((match == extensions || match[-1] == ' ')
            && (match[name_length] == '\0'
                || match[name_length] == ' ')) {
            return 1;
        }
        match += name_length;
    }
    return 0;
}

static int Legacy_extension_supported(LegacyContext *context, int major,
                                      const char *name)
{
    LegacyFunctions *gl = &context->gl;

    if (major >= 3) {
        GLint extension_count = 0;
        GLint index;

        if (gl->get_string_indexed == NULL)
            return 0;
        Legacy_discard_errors(context);
        gl->get_integer(GL_NUM_EXTENSIONS, &extension_count);
        if (Legacy_result(context) != RENDERER_STATUS_OK
            || extension_count < 0) {
            return 0;
        }
        for (index = 0; index < extension_count; index++) {
            const GLubyte *extension = gl->get_string_indexed(
                GL_EXTENSIONS, (GLuint)index);

            if (extension != NULL
                && strcmp((const char *)extension, name) == 0) {
                return Legacy_result(context) == RENDERER_STATUS_OK;
            }
        }
        if (Legacy_result(context) != RENDERER_STATUS_OK)
            return 0;
        return 0;
    }

    {
        const GLubyte *extensions;

        Legacy_discard_errors(context);
        extensions = gl->get_string(GL_EXTENSIONS);
        if (Legacy_result(context) != RENDERER_STATUS_OK
            || extensions == NULL) {
            return 0;
        }
        return Legacy_extension_string_contains(
            (const char *)extensions, name);
    }
}

static int Legacy_context_is_compatible(LegacyContext *context)
{
    const GLubyte *version;
    int major;
    int minor;

    Legacy_discard_errors(context);
    version = context->gl.get_string(GL_VERSION);
    if (version == NULL) {
        Legacy_discard_errors(context);
        return 0;
    }
    if (Legacy_result(context) != RENDERER_STATUS_OK
        || sscanf((const char *)version, "%d.%d", &major, &minor) != 2) {
        return 0;
    }
    if (!Legacy_version_at_least(major, minor, 1, 4))
        return 0;

    /* Fixed-function entry points are unavailable in a 3.0 forward context,
     * in 3.1 without ARB_compatibility, and in 3.2+ core profiles. */
    if (major >= 3) {
        GLint flags = 0;

        Legacy_discard_errors(context);
        context->gl.get_integer(GL_CONTEXT_FLAGS, &flags);
        if (Legacy_result(context) != RENDERER_STATUS_OK
            || (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0) {
            return 0;
        }
    }
    if (major == 3 && minor == 1
        && !Legacy_extension_supported(context, major,
                                       "GL_ARB_compatibility")) {
        return 0;
    }
    if (major > 3 || (major == 3 && minor >= 2)) {
        GLint profile = 0;

        Legacy_discard_errors(context);
        context->gl.get_integer(GL_CONTEXT_PROFILE_MASK, &profile);
        if (Legacy_result(context) != RENDERER_STATUS_OK
            || (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0) {
            return 0;
        }
    }

    if (Legacy_version_at_least(major, minor, 2, 0)
        && context->gl.use_program == NULL) {
        return 0;
    }
    context->pixel_unpack_buffer_supported =
        Legacy_version_at_least(major, minor, 2, 1)
        || Legacy_extension_supported(context, major,
                                      "GL_ARB_pixel_buffer_object")
        || Legacy_extension_supported(context, major,
                                      "GL_EXT_pixel_buffer_object");
    if (context->pixel_unpack_buffer_supported
        && context->gl.bind_buffer == NULL) {
        return 0;
    }
    return 1;
}

static RendererStatus Legacy_begin_frame(void *backend_context,
                                         int width, int height,
                                         RendererColor clear_color)
{
    LegacyContext *context = backend_context;
    LegacyFunctions *gl = &context->gl;
    GLint texture_unit_count = 0;
    GLint texture_unit;

    Legacy_discard_errors(context);
    gl->get_integer(GL_MAX_TEXTURE_UNITS, &texture_unit_count);
    if (Legacy_result(context) != RENDERER_STATUS_OK
        || texture_unit_count < 1) {
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    /* A fixed-function fragment combines every enabled texture unit. */
    for (texture_unit = 0; texture_unit < texture_unit_count;
         texture_unit++) {
        gl->active_texture((GLenum)(GL_TEXTURE0 + texture_unit));
        gl->disable(GL_TEXTURE_2D);
    }
    gl->active_texture(GL_TEXTURE0);
    context->frame_height = height;
    gl->viewport(0, 0, width, height);
    gl->disable(GL_DEPTH_TEST);
    gl->disable(GL_CULL_FACE);
    gl->disable(GL_STENCIL_TEST);
    gl->disable(GL_ALPHA_TEST);
    gl->disable(GL_LIGHTING);
    gl->disable(GL_FOG);
    gl->disable(GL_BLEND);
    gl->disable(GL_SCISSOR_TEST);
    gl->color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl->shade_model(GL_SMOOTH);
    gl->polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
    gl->matrix_mode(GL_PROJECTION);
    gl->load_identity();
    gl->ortho(0.0, (GLdouble)width, (GLdouble)height, 0.0, -1.0, 1.0);
    gl->matrix_mode(GL_MODELVIEW);
    gl->load_identity();
    gl->clear_color((GLclampf)clear_color.red / 255.0f,
                    (GLclampf)clear_color.green / 255.0f,
                    (GLclampf)clear_color.blue / 255.0f,
                    (GLclampf)clear_color.alpha / 255.0f);
    gl->clear(GL_COLOR_BUFFER_BIT);
    return Legacy_result(context);
}

static void Legacy_set_blend(LegacyContext *context,
                             RendererBlendMode blend)
{
    LegacyFunctions *gl = &context->gl;

    if (blend == RENDERER_BLEND_OPAQUE) {
        gl->disable(GL_BLEND);
        return;
    }
    gl->enable(GL_BLEND);
    gl->blend_equation(GL_FUNC_ADD);
    if (blend == RENDERER_BLEND_ALPHA)
        gl->blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else
        gl->blend_func(GL_SRC_ALPHA, GL_ONE);
}

static void Legacy_set_scissor(LegacyContext *context,
                               const RendererBackendDraw *draw)
{
    if (!draw->scissor_enabled) {
        context->gl.disable(GL_SCISSOR_TEST);
        return;
    }
    context->gl.enable(GL_SCISSOR_TEST);
    context->gl.scissor(draw->scissor.x,
                        context->frame_height - draw->scissor.y
                            - draw->scissor.height,
                        draw->scissor.width, draw->scissor.height);
}

static void Legacy_set_transform(LegacyContext *context,
                                 RendererTransform2D transform)
{
    const GLfloat matrix[16] = {
        transform.m[0], transform.m[1], 0.0f, 0.0f,
        transform.m[3], transform.m[4], 0.0f, 0.0f,
        0.0f,           0.0f,           1.0f, 0.0f,
        transform.m[6], transform.m[7], 0.0f, 1.0f
    };

    context->gl.matrix_mode(GL_MODELVIEW);
    context->gl.load_matrix(matrix);
}

static RendererStatus Legacy_draw(void *backend_context,
                                  const RendererBackendDraw *draw)
{
    LegacyContext *context = backend_context;
    LegacyFunctions *gl = &context->gl;
    const RendererVertex2D *vertices = draw->vertices;
    size_t vertex_count = draw->vertex_count;
    size_t index;

    if (draw->mesh != NULL) {
        const LegacyMesh *mesh = draw->mesh;

        vertices = mesh->vertices;
        vertex_count = mesh->vertex_count;
    }
    if (vertices == NULL || vertex_count == 0)
        return RENDERER_STATUS_BACKEND_ERROR;

    Legacy_discard_errors(context);
    if (gl->use_program != NULL)
        gl->use_program(0);
    gl->disable(GL_COLOR_LOGIC_OP);
    gl->active_texture(GL_TEXTURE0);
    gl->matrix_mode(GL_TEXTURE);
    gl->load_identity();
    gl->matrix_mode(GL_MODELVIEW);
    gl->disable(GL_TEXTURE_GEN_S);
    gl->disable(GL_TEXTURE_GEN_T);
    Legacy_set_blend(context, draw->blend);
    Legacy_set_scissor(context, draw);
    Legacy_set_transform(context, draw->transform);
    if (draw->texture != NULL) {
        const LegacyTexture *texture = draw->texture;

        gl->enable(GL_TEXTURE_2D);
        gl->bind_texture(GL_TEXTURE_2D, texture->name);
        gl->texture_environment(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                                GL_MODULATE);
    } else {
        gl->disable(GL_TEXTURE_2D);
    }

    gl->begin(GL_TRIANGLES);
    for (index = 0; index < vertex_count; index++) {
        const RendererVertex2D *vertex = &vertices[index];

        gl->color(vertex->color.red, vertex->color.green,
                  vertex->color.blue, vertex->color.alpha);
        gl->tex_coord(vertex->u, vertex->v);
        gl->vertex(vertex->x, vertex->y);
    }
    gl->end();
    return Legacy_result(context);
}

static RendererStatus Legacy_flush(void *backend_context)
{
    LegacyContext *context = backend_context;

    Legacy_discard_errors(context);
    context->gl.flush();
    return Legacy_result(context);
}

static RendererStatus Legacy_end_frame(void *backend_context)
{
    LegacyContext *context = backend_context;
    RendererStatus status = Legacy_result(context);

    if (status == RENDERER_STATUS_OK)
        context->frame_height = 0;
    return status;
}

static const uint8_t *Legacy_contiguous_pixels(const uint8_t *pixels,
                                               size_t pitch,
                                               int width, int height,
                                               uint8_t **allocation)
{
    size_t row_size = (size_t)width * 4;
    size_t row;

    *allocation = NULL;
    if (pitch == row_size)
        return pixels;
    if ((size_t)height > SIZE_MAX / row_size)
        return NULL;
    *allocation = malloc((size_t)height * row_size);
    if (*allocation == NULL)
        return NULL;
    for (row = 0; row < (size_t)height; row++) {
        memcpy(*allocation + row * row_size,
               pixels + row * pitch, row_size);
    }
    return *allocation;
}

static int Legacy_pixel_data_valid(int width, int height, size_t pitch)
{
    size_t row_size;

    if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / 4)
        return 0;
    row_size = (size_t)width * 4;
    if (pitch < row_size)
        return 0;
    return height == 1
        || pitch <= (SIZE_MAX - row_size) / (size_t)(height - 1);
}

static RendererStatus Legacy_restore_upload_state(
    LegacyContext *context, const LegacyUploadState *state,
    RendererStatus upload_status)
{
    LegacyFunctions *gl = &context->gl;

    Legacy_discard_errors(context);
    if (context->pixel_unpack_buffer_supported) {
        gl->bind_buffer(GL_PIXEL_UNPACK_BUFFER,
                        (GLuint)state->unpack_buffer);
    }
    gl->pixel_store(GL_UNPACK_ALIGNMENT, state->unpack_alignment);
    gl->pixel_store(GL_UNPACK_ROW_LENGTH, state->unpack_row_length);
    gl->pixel_store(GL_UNPACK_SKIP_ROWS, state->unpack_skip_rows);
    gl->pixel_store(GL_UNPACK_SKIP_PIXELS, state->unpack_skip_pixels);
    gl->bind_texture(GL_TEXTURE_2D, (GLuint)state->texture);
    gl->active_texture((GLenum)state->active_texture);
    if (Legacy_result(context) != RENDERER_STATUS_OK)
        return RENDERER_STATUS_BACKEND_ERROR;
    return upload_status;
}

static RendererStatus Legacy_begin_texture_upload(
    LegacyContext *context, GLuint texture, LegacyUploadState *state)
{
    LegacyFunctions *gl = &context->gl;

    memset(state, 0, sizeof(*state));
    Legacy_discard_errors(context);
    gl->get_integer(GL_ACTIVE_TEXTURE, &state->active_texture);
    if (Legacy_result(context) != RENDERER_STATUS_OK)
        return RENDERER_STATUS_BACKEND_ERROR;

    /* Texture binding is per unit; pixel-store and PBO bindings are shared. */
    gl->active_texture(GL_TEXTURE0);
    gl->get_integer(GL_TEXTURE_BINDING_2D, &state->texture);
    gl->get_integer(GL_UNPACK_ALIGNMENT, &state->unpack_alignment);
    gl->get_integer(GL_UNPACK_ROW_LENGTH, &state->unpack_row_length);
    gl->get_integer(GL_UNPACK_SKIP_ROWS, &state->unpack_skip_rows);
    gl->get_integer(GL_UNPACK_SKIP_PIXELS, &state->unpack_skip_pixels);
    if (context->pixel_unpack_buffer_supported) {
        gl->get_integer(GL_PIXEL_UNPACK_BUFFER_BINDING,
                        &state->unpack_buffer);
    }
    if (Legacy_result(context) != RENDERER_STATUS_OK) {
        Legacy_discard_errors(context);
        gl->active_texture((GLenum)state->active_texture);
        Legacy_discard_errors(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    if (context->pixel_unpack_buffer_supported)
        gl->bind_buffer(GL_PIXEL_UNPACK_BUFFER, 0);
    gl->bind_texture(GL_TEXTURE_2D, texture);
    gl->pixel_store(GL_UNPACK_ALIGNMENT, 1);
    gl->pixel_store(GL_UNPACK_ROW_LENGTH, 0);
    gl->pixel_store(GL_UNPACK_SKIP_ROWS, 0);
    gl->pixel_store(GL_UNPACK_SKIP_PIXELS, 0);
    if (Legacy_result(context) != RENDERER_STATUS_OK) {
        return Legacy_restore_upload_state(
            context, state, RENDERER_STATUS_BACKEND_ERROR);
    }
    return RENDERER_STATUS_OK;
}

static RendererStatus Legacy_texture_create(void *backend_context,
                                            int width, int height,
                                            const uint8_t *rgba_pixels,
                                            size_t pitch, void **handle)
{
    LegacyContext *context = backend_context;
    LegacyFunctions *gl = &context->gl;
    LegacyTexture *texture;
    uint8_t *allocation;
    const uint8_t *pixels;
    LegacyUploadState upload_state;
    RendererStatus status;

    if (handle == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *handle = NULL;
    if (rgba_pixels == NULL
        || !Legacy_pixel_data_valid(width, height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    pixels = Legacy_contiguous_pixels(rgba_pixels, pitch, width, height,
                                      &allocation);
    if (pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    texture = calloc(1, sizeof(*texture));
    if (texture == NULL) {
        free(allocation);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }

    Legacy_discard_errors(context);
    gl->gen_textures(1, &texture->name);
    status = Legacy_result(context);
    if (status == RENDERER_STATUS_OK && texture->name != 0) {
        status = Legacy_begin_texture_upload(context, texture->name,
                                             &upload_state);
    }
    if (status == RENDERER_STATUS_OK) {
        gl->texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                              GL_NEAREST);
        gl->texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                              GL_NEAREST);
        gl->texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE);
        gl->texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE);
        gl->texture_image(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        status = Legacy_result(context);
        status = Legacy_restore_upload_state(context, &upload_state, status);
    }
    free(allocation);
    if (status != RENDERER_STATUS_OK || texture->name == 0) {
        if (texture->name != 0)
            gl->delete_textures(1, &texture->name);
        free(texture);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    texture->width = width;
    texture->height = height;
    *handle = texture;
    return RENDERER_STATUS_OK;
}

static RendererStatus Legacy_texture_update(void *backend_context,
                                            void *handle,
                                            RendererRect region,
                                            const uint8_t *rgba_pixels,
                                            size_t pitch)
{
    LegacyContext *context = backend_context;
    LegacyFunctions *gl = &context->gl;
    LegacyTexture *texture = handle;
    uint8_t *allocation;
    const uint8_t *pixels;
    LegacyUploadState upload_state;
    RendererStatus status;

    if (texture == NULL || rgba_pixels == NULL
        || region.x < 0 || region.y < 0
        || region.width <= 0 || region.height <= 0
        || region.x > texture->width - region.width
        || region.y > texture->height - region.height
        || !Legacy_pixel_data_valid(region.width, region.height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    pixels = Legacy_contiguous_pixels(
        rgba_pixels, pitch, region.width, region.height, &allocation);
    if (pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    status = Legacy_begin_texture_upload(context, texture->name,
                                         &upload_state);
    if (status == RENDERER_STATUS_OK) {
        gl->texture_sub_image(GL_TEXTURE_2D, 0, region.x, region.y,
                              region.width, region.height,
                              GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        status = Legacy_result(context);
        status = Legacy_restore_upload_state(context, &upload_state, status);
    }
    free(allocation);
    return status;
}

static void Legacy_texture_destroy(void *backend_context, void *handle)
{
    LegacyContext *context = backend_context;
    LegacyTexture *texture = handle;

    if (texture == NULL)
        return;
    context->gl.delete_textures(1, &texture->name);
    free(texture);
}

static RendererStatus Legacy_mesh_create(void *backend_context,
                                         const RendererVertex2D *vertices,
                                         size_t vertex_count, void **handle)
{
    LegacyMesh *mesh;

    (void)backend_context;
    *handle = NULL;
    mesh = calloc(1, sizeof(*mesh));
    if (mesh == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    mesh->vertices = malloc(vertex_count * sizeof(*mesh->vertices));
    if (mesh->vertices == NULL) {
        free(mesh);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    memcpy(mesh->vertices, vertices,
           vertex_count * sizeof(*mesh->vertices));
    mesh->vertex_count = vertex_count;
    *handle = mesh;
    return RENDERER_STATUS_OK;
}

static RendererStatus Legacy_mesh_update(void *backend_context, void *handle,
                                         const RendererVertex2D *vertices,
                                         size_t vertex_count)
{
    LegacyMesh *mesh = handle;
    RendererVertex2D *replacement;

    (void)backend_context;
    replacement = malloc(vertex_count * sizeof(*replacement));
    if (replacement == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    memcpy(replacement, vertices, vertex_count * sizeof(*replacement));
    free(mesh->vertices);
    mesh->vertices = replacement;
    mesh->vertex_count = vertex_count;
    return RENDERER_STATUS_OK;
}

static void Legacy_mesh_destroy(void *backend_context, void *handle)
{
    LegacyMesh *mesh = handle;

    (void)backend_context;
    if (mesh == NULL)
        return;
    free(mesh->vertices);
    free(mesh);
}

static void Legacy_destroy(void *backend_context)
{
    free(backend_context);
}

static const RendererBackendInterface legacy_interface = {
    .begin_frame = Legacy_begin_frame,
    .draw = Legacy_draw,
    .flush = Legacy_flush,
    .end_frame = Legacy_end_frame,
    .texture_create = Legacy_texture_create,
    .texture_update = Legacy_texture_update,
    .texture_destroy = Legacy_texture_destroy,
    .mesh_create = Legacy_mesh_create,
    .mesh_update = Legacy_mesh_update,
    .mesh_destroy = Legacy_mesh_destroy,
    .destroy = Legacy_destroy
};

RendererStatus Renderer_gl_legacy_create(RendererGLProcLoader loader,
                                         void *userdata,
                                         Renderer **renderer)
{
    LegacyContext *context;
    Renderer *created;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *renderer = NULL;
    if (loader == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    context = calloc(1, sizeof(*context));
    if (context == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (!Legacy_load_functions(context, loader, userdata)
        || !Legacy_context_is_compatible(context)) {
        free(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    created = Renderer_test_create(&legacy_interface, context);
    if (created == NULL) {
        free(context);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    *renderer = created;
    return RENDERER_STATUS_OK;
}
