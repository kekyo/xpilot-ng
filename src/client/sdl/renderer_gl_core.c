/* OpenGL 3.3 core implementation of the renderer backend interface. */

#include "renderer_gl_core.h"
#include "renderer_backend.h"

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef GLenum (APIENTRYP CoreGetErrorProc)(void);
typedef void (APIENTRYP CoreGetIntegervProc)(GLenum name, GLint *value);
typedef void (APIENTRYP CoreViewportProc)(GLint x, GLint y,
                                         GLsizei width, GLsizei height);
typedef void (APIENTRYP CoreClearColorProc)(GLfloat red, GLfloat green,
                                           GLfloat blue, GLfloat alpha);
typedef void (APIENTRYP CoreClearProc)(GLbitfield mask);
typedef void (APIENTRYP CoreEnableProc)(GLenum capability);
typedef void (APIENTRYP CoreDisableProc)(GLenum capability);
typedef void (APIENTRYP CoreColorMaskProc)(GLboolean red, GLboolean green,
                                          GLboolean blue, GLboolean alpha);
typedef void (APIENTRYP CoreScissorProc)(GLint x, GLint y,
                                        GLsizei width, GLsizei height);
typedef void (APIENTRYP CorePolygonModeProc)(GLenum face, GLenum mode);
typedef void (APIENTRYP CoreBlendFuncProc)(GLenum source, GLenum destination);
typedef void (APIENTRYP CoreFlushProc)(void);
typedef void (APIENTRYP CoreGenTexturesProc)(GLsizei count, GLuint *textures);
typedef void (APIENTRYP CoreBindTextureProc)(GLenum target, GLuint texture);
typedef void (APIENTRYP CoreTexParameteriProc)(GLenum target, GLenum name,
                                              GLint value);
typedef void (APIENTRYP CorePixelStoreiProc)(GLenum name, GLint value);
typedef void (APIENTRYP CoreTexImage2DProc)(GLenum target, GLint level,
                                           GLint internal_format,
                                           GLsizei width, GLsizei height,
                                           GLint border, GLenum format,
                                           GLenum type, const void *pixels);
typedef void (APIENTRYP CoreTexSubImage2DProc)(GLenum target, GLint level,
                                              GLint x, GLint y,
                                              GLsizei width, GLsizei height,
                                              GLenum format, GLenum type,
                                              const void *pixels);
typedef void (APIENTRYP CoreDeleteTexturesProc)(GLsizei count,
                                               const GLuint *textures);
typedef void (APIENTRYP CoreDrawArraysProc)(GLenum mode, GLint first,
                                           GLsizei count);

typedef struct CoreFunctions {
    CoreGetErrorProc get_error;
    CoreGetIntegervProc get_integerv;
    CoreViewportProc viewport;
    CoreClearColorProc clear_color;
    CoreClearProc clear;
    CoreEnableProc enable;
    CoreDisableProc disable;
    CoreColorMaskProc color_mask;
    CoreScissorProc scissor;
    CorePolygonModeProc polygon_mode;
    PFNGLBLENDEQUATIONPROC blend_equation;
    CoreBlendFuncProc blend_func;
    CoreFlushProc flush;
    CoreGenTexturesProc gen_textures;
    CoreBindTextureProc bind_texture;
    CoreTexParameteriProc tex_parameter_i;
    CorePixelStoreiProc pixel_store_i;
    CoreTexImage2DProc tex_image_2d;
    CoreTexSubImage2DProc tex_sub_image_2d;
    CoreDeleteTexturesProc delete_textures;
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLGENVERTEXARRAYSPROC gen_vertex_arrays;
    PFNGLBINDVERTEXARRAYPROC bind_vertex_array;
    PFNGLDELETEVERTEXARRAYSPROC delete_vertex_arrays;
    PFNGLGENBUFFERSPROC gen_buffers;
    PFNGLBINDBUFFERPROC bind_buffer;
    PFNGLBUFFERDATAPROC buffer_data;
    PFNGLDELETEBUFFERSPROC delete_buffers;
    PFNGLCREATESHADERPROC create_shader;
    PFNGLSHADERSOURCEPROC shader_source;
    PFNGLCOMPILESHADERPROC compile_shader;
    PFNGLGETSHADERIVPROC get_shader_iv;
    PFNGLDELETESHADERPROC delete_shader;
    PFNGLCREATEPROGRAMPROC create_program;
    PFNGLATTACHSHADERPROC attach_shader;
    PFNGLBINDATTRIBLOCATIONPROC bind_attrib_location;
    PFNGLBINDFRAGDATALOCATIONPROC bind_frag_data_location;
    PFNGLLINKPROGRAMPROC link_program;
    PFNGLGETPROGRAMIVPROC get_program_iv;
    PFNGLDELETEPROGRAMPROC delete_program;
    PFNGLUSEPROGRAMPROC use_program;
    PFNGLGETUNIFORMLOCATIONPROC get_uniform_location;
    PFNGLUNIFORMMATRIX3FVPROC uniform_matrix_3fv;
    PFNGLUNIFORM2FPROC uniform_2f;
    PFNGLUNIFORM1IPROC uniform_1i;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attrib_array;
    PFNGLVERTEXATTRIBPOINTERPROC vertex_attrib_pointer;
    PFNGLBINDSAMPLERPROC bind_sampler;
    CoreDrawArraysProc draw_arrays;
} CoreFunctions;

typedef struct CoreContext {
    CoreFunctions gl;
    GLuint program;
    GLuint vertex_array;
    GLuint stream_buffer;
    GLuint white_texture;
    GLint transform_uniform;
    GLint framebuffer_size_uniform;
    GLint texture_uniform;
    int frame_width;
    int frame_height;
} CoreContext;

typedef struct CoreTexture {
    GLuint name;
    int width;
    int height;
} CoreTexture;

typedef struct CoreMesh {
    GLuint buffer;
    GLsizei vertex_count;
} CoreMesh;

enum {
    CORE_POSITION_ATTRIBUTE = 0,
    CORE_TEXCOORD_ATTRIBUTE = 1,
    CORE_COLOR_ATTRIBUTE = 2
};

static const char core_vertex_shader_source[] =
    "#version 330 core\n"
    "in vec2 a_position;\n"
    "in vec2 a_texcoord;\n"
    "in vec4 a_color;\n"
    "uniform mat3 u_transform;\n"
    "uniform vec2 u_framebuffer_size;\n"
    "out vec2 v_texcoord;\n"
    "out vec4 v_color;\n"
    "void main(void)\n"
    "{\n"
    "    vec2 position = (u_transform * vec3(a_position, 1.0)).xy;\n"
    "    vec2 clip_position = vec2(\n"
    "        position.x * 2.0 / u_framebuffer_size.x - 1.0,\n"
    "        1.0 - position.y * 2.0 / u_framebuffer_size.y);\n"
    "    gl_Position = vec4(clip_position, 0.0, 1.0);\n"
    "    v_texcoord = a_texcoord;\n"
    "    v_color = a_color;\n"
    "}\n";

static const char core_fragment_shader_source[] =
    "#version 330 core\n"
    "uniform sampler2D u_texture;\n"
    "in vec2 v_texcoord;\n"
    "in vec4 v_color;\n"
    "out vec4 fragment_color;\n"
    "void main(void)\n"
    "{\n"
    "    fragment_color = texture(u_texture, v_texcoord) * v_color;\n"
    "}\n";

static int Core_load_proc(RendererGLProcLoader loader, void *userdata,
                          const char *name, void *destination,
                          size_t destination_size)
{
    void *address = loader(userdata, name);

    if (address == NULL || destination_size != sizeof(address))
        return 0;
    memcpy(destination, &address, sizeof(address));
    return 1;
}

#define CORE_LOAD(functions, loader, userdata, member, symbol)                \
    Core_load_proc((loader), (userdata), (symbol),                            \
                   &(functions)->member, sizeof((functions)->member))

static int Core_load_functions(CoreFunctions *functions,
                               RendererGLProcLoader loader, void *userdata)
{
    memset(functions, 0, sizeof(*functions));
    return CORE_LOAD(functions, loader, userdata, get_error, "glGetError")
        && CORE_LOAD(functions, loader, userdata, get_integerv,
                     "glGetIntegerv")
        && CORE_LOAD(functions, loader, userdata, viewport, "glViewport")
        && CORE_LOAD(functions, loader, userdata, clear_color, "glClearColor")
        && CORE_LOAD(functions, loader, userdata, clear, "glClear")
        && CORE_LOAD(functions, loader, userdata, enable, "glEnable")
        && CORE_LOAD(functions, loader, userdata, disable, "glDisable")
        && CORE_LOAD(functions, loader, userdata, color_mask, "glColorMask")
        && CORE_LOAD(functions, loader, userdata, scissor, "glScissor")
        && CORE_LOAD(functions, loader, userdata, polygon_mode,
                     "glPolygonMode")
        && CORE_LOAD(functions, loader, userdata, blend_equation,
                     "glBlendEquation")
        && CORE_LOAD(functions, loader, userdata, blend_func, "glBlendFunc")
        && CORE_LOAD(functions, loader, userdata, flush, "glFlush")
        && CORE_LOAD(functions, loader, userdata, gen_textures,
                     "glGenTextures")
        && CORE_LOAD(functions, loader, userdata, bind_texture,
                     "glBindTexture")
        && CORE_LOAD(functions, loader, userdata, tex_parameter_i,
                     "glTexParameteri")
        && CORE_LOAD(functions, loader, userdata, pixel_store_i,
                     "glPixelStorei")
        && CORE_LOAD(functions, loader, userdata, tex_image_2d,
                     "glTexImage2D")
        && CORE_LOAD(functions, loader, userdata, tex_sub_image_2d,
                     "glTexSubImage2D")
        && CORE_LOAD(functions, loader, userdata, delete_textures,
                     "glDeleteTextures")
        && CORE_LOAD(functions, loader, userdata, active_texture,
                     "glActiveTexture")
        && CORE_LOAD(functions, loader, userdata, gen_vertex_arrays,
                     "glGenVertexArrays")
        && CORE_LOAD(functions, loader, userdata, bind_vertex_array,
                     "glBindVertexArray")
        && CORE_LOAD(functions, loader, userdata, delete_vertex_arrays,
                     "glDeleteVertexArrays")
        && CORE_LOAD(functions, loader, userdata, gen_buffers, "glGenBuffers")
        && CORE_LOAD(functions, loader, userdata, bind_buffer, "glBindBuffer")
        && CORE_LOAD(functions, loader, userdata, buffer_data, "glBufferData")
        && CORE_LOAD(functions, loader, userdata, delete_buffers,
                     "glDeleteBuffers")
        && CORE_LOAD(functions, loader, userdata, create_shader,
                     "glCreateShader")
        && CORE_LOAD(functions, loader, userdata, shader_source,
                     "glShaderSource")
        && CORE_LOAD(functions, loader, userdata, compile_shader,
                     "glCompileShader")
        && CORE_LOAD(functions, loader, userdata, get_shader_iv,
                     "glGetShaderiv")
        && CORE_LOAD(functions, loader, userdata, delete_shader,
                     "glDeleteShader")
        && CORE_LOAD(functions, loader, userdata, create_program,
                     "glCreateProgram")
        && CORE_LOAD(functions, loader, userdata, attach_shader,
                     "glAttachShader")
        && CORE_LOAD(functions, loader, userdata, bind_attrib_location,
                     "glBindAttribLocation")
        && CORE_LOAD(functions, loader, userdata, bind_frag_data_location,
                     "glBindFragDataLocation")
        && CORE_LOAD(functions, loader, userdata, link_program,
                     "glLinkProgram")
        && CORE_LOAD(functions, loader, userdata, get_program_iv,
                     "glGetProgramiv")
        && CORE_LOAD(functions, loader, userdata, delete_program,
                     "glDeleteProgram")
        && CORE_LOAD(functions, loader, userdata, use_program, "glUseProgram")
        && CORE_LOAD(functions, loader, userdata, get_uniform_location,
                     "glGetUniformLocation")
        && CORE_LOAD(functions, loader, userdata, uniform_matrix_3fv,
                     "glUniformMatrix3fv")
        && CORE_LOAD(functions, loader, userdata, uniform_2f, "glUniform2f")
        && CORE_LOAD(functions, loader, userdata, uniform_1i, "glUniform1i")
        && CORE_LOAD(functions, loader, userdata, enable_vertex_attrib_array,
                     "glEnableVertexAttribArray")
        && CORE_LOAD(functions, loader, userdata, vertex_attrib_pointer,
                     "glVertexAttribPointer")
        && CORE_LOAD(functions, loader, userdata, bind_sampler,
                     "glBindSampler")
        && CORE_LOAD(functions, loader, userdata, draw_arrays,
                     "glDrawArrays");
}

#undef CORE_LOAD

static void Core_clear_errors(CoreContext *context)
{
    unsigned int error_count = 0;

    while (context->gl.get_error() != GL_NO_ERROR && error_count < 64)
        error_count++;
}

static int Core_operation_succeeded(CoreContext *context)
{
    GLenum error = context->gl.get_error();

    if (error == GL_NO_ERROR)
        return 1;
    Core_clear_errors(context);
    return 0;
}

static int Core_vertex_data_valid(size_t vertex_count, GLsizeiptr *byte_count)
{
    if (vertex_count == 0 || vertex_count > (size_t)INT_MAX
        || vertex_count > (size_t)PTRDIFF_MAX / sizeof(RendererVertex2D)) {
        return 0;
    }
    *byte_count = (GLsizeiptr)(vertex_count * sizeof(RendererVertex2D));
    return 1;
}

static int Core_pixel_data_valid(int width, int height, size_t pitch)
{
    size_t row_bytes;

    if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / 4)
        return 0;
    row_bytes = (size_t)width * 4;
    if (pitch < row_bytes)
        return 0;
    return height == 1
        || pitch <= (SIZE_MAX - row_bytes) / (size_t)(height - 1);
}

static RendererStatus Core_upload_bound_texture(CoreContext *context,
                                                int x, int y,
                                                int width, int height,
                                                const uint8_t *pixels,
                                                size_t pitch)
{
    size_t row_bytes = (size_t)width * 4;
    int row;

    if (pitch == row_bytes) {
        context->gl.tex_sub_image_2d(GL_TEXTURE_2D, 0, x, y, width, height,
                                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    } else {
        for (row = 0; row < height; row++) {
            context->gl.tex_sub_image_2d(
                GL_TEXTURE_2D, 0, x, y + row, width, 1,
                GL_RGBA, GL_UNSIGNED_BYTE,
                pixels + (size_t)row * pitch);
        }
    }
    return Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                             : RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus Core_configure_and_upload_texture(
    CoreContext *context, GLuint texture, int width, int height,
    const uint8_t *pixels, size_t pitch)
{
    GLint previous_active_texture;
    GLint previous_texture;
    GLint previous_unpack_alignment;
    GLint previous_unpack_buffer;
    GLint previous_unpack_row_length;
    GLint previous_unpack_skip_rows;
    GLint previous_unpack_skip_pixels;
    RendererStatus status = RENDERER_STATUS_BACKEND_ERROR;

    Core_clear_errors(context);
    context->gl.get_integerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    context->gl.active_texture(GL_TEXTURE0);
    context->gl.get_integerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    context->gl.get_integerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
    context->gl.get_integerv(GL_PIXEL_UNPACK_BUFFER_BINDING,
                             &previous_unpack_buffer);
    context->gl.get_integerv(GL_UNPACK_ROW_LENGTH,
                             &previous_unpack_row_length);
    context->gl.get_integerv(GL_UNPACK_SKIP_ROWS,
                             &previous_unpack_skip_rows);
    context->gl.get_integerv(GL_UNPACK_SKIP_PIXELS,
                             &previous_unpack_skip_pixels);
    if (!Core_operation_succeeded(context))
        return RENDERER_STATUS_BACKEND_ERROR;

    Core_clear_errors(context);
    context->gl.bind_texture(GL_TEXTURE_2D, texture);
    context->gl.bind_buffer(GL_PIXEL_UNPACK_BUFFER, 0);
    context->gl.pixel_store_i(GL_UNPACK_ALIGNMENT, 1);
    context->gl.pixel_store_i(GL_UNPACK_ROW_LENGTH, 0);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_ROWS, 0);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_PIXELS, 0);
    context->gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_NEAREST);
    context->gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_NEAREST);
    context->gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
    context->gl.tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
    context->gl.tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    if (Core_operation_succeeded(context)) {
        Core_clear_errors(context);
        status = Core_upload_bound_texture(context, 0, 0, width, height,
                                           pixels, pitch);
    }

    Core_clear_errors(context);
    context->gl.bind_buffer(GL_PIXEL_UNPACK_BUFFER,
                            (GLuint)previous_unpack_buffer);
    context->gl.pixel_store_i(GL_UNPACK_ALIGNMENT,
                              previous_unpack_alignment);
    context->gl.pixel_store_i(GL_UNPACK_ROW_LENGTH,
                              previous_unpack_row_length);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_ROWS,
                              previous_unpack_skip_rows);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_PIXELS,
                              previous_unpack_skip_pixels);
    context->gl.bind_texture(GL_TEXTURE_2D, (GLuint)previous_texture);
    context->gl.active_texture((GLenum)previous_active_texture);
    if (!Core_operation_succeeded(context))
        return RENDERER_STATUS_BACKEND_ERROR;
    return status;
}

static GLuint Core_compile_shader(CoreContext *context, GLenum type,
                                  const char *source)
{
    const GLchar *shader_source = (const GLchar *)source;
    GLint compiled = GL_FALSE;
    GLuint shader;

    Core_clear_errors(context);
    shader = context->gl.create_shader(type);
    if (shader == 0 || !Core_operation_succeeded(context))
        return 0;
    context->gl.shader_source(shader, 1, &shader_source, NULL);
    context->gl.compile_shader(shader);
    context->gl.get_shader_iv(shader, GL_COMPILE_STATUS, &compiled);
    if (!Core_operation_succeeded(context) || compiled != GL_TRUE) {
        context->gl.delete_shader(shader);
        Core_clear_errors(context);
        return 0;
    }
    return shader;
}

static int Core_create_program(CoreContext *context)
{
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLint linked = GL_FALSE;

    vertex_shader = Core_compile_shader(context, GL_VERTEX_SHADER,
                                        core_vertex_shader_source);
    if (vertex_shader == 0)
        return 0;
    fragment_shader = Core_compile_shader(context, GL_FRAGMENT_SHADER,
                                          core_fragment_shader_source);
    if (fragment_shader == 0) {
        context->gl.delete_shader(vertex_shader);
        Core_clear_errors(context);
        return 0;
    }

    Core_clear_errors(context);
    context->program = context->gl.create_program();
    if (context->program != 0) {
        context->gl.attach_shader(context->program, vertex_shader);
        context->gl.attach_shader(context->program, fragment_shader);
        context->gl.bind_attrib_location(context->program,
                                         CORE_POSITION_ATTRIBUTE,
                                         "a_position");
        context->gl.bind_attrib_location(context->program,
                                         CORE_TEXCOORD_ATTRIBUTE,
                                         "a_texcoord");
        context->gl.bind_attrib_location(context->program,
                                         CORE_COLOR_ATTRIBUTE,
                                         "a_color");
        context->gl.bind_frag_data_location(context->program, 0,
                                            "fragment_color");
        context->gl.link_program(context->program);
        context->gl.get_program_iv(context->program, GL_LINK_STATUS, &linked);
    }
    context->gl.delete_shader(fragment_shader);
    context->gl.delete_shader(vertex_shader);
    if (context->program == 0 || linked != GL_TRUE
        || !Core_operation_succeeded(context)) {
        if (context->program != 0)
            context->gl.delete_program(context->program);
        context->program = 0;
        Core_clear_errors(context);
        return 0;
    }

    context->transform_uniform = context->gl.get_uniform_location(
        context->program, "u_transform");
    context->framebuffer_size_uniform = context->gl.get_uniform_location(
        context->program, "u_framebuffer_size");
    context->texture_uniform = context->gl.get_uniform_location(
        context->program, "u_texture");
    if (context->transform_uniform < 0
        || context->framebuffer_size_uniform < 0
        || context->texture_uniform < 0
        || !Core_operation_succeeded(context)) {
        context->gl.delete_program(context->program);
        context->program = 0;
        Core_clear_errors(context);
        return 0;
    }
    return 1;
}

static void Core_release_context_objects(CoreContext *context)
{
    GLint current_program = 0;

    if (context->white_texture != 0)
        context->gl.delete_textures(1, &context->white_texture);
    if (context->stream_buffer != 0)
        context->gl.delete_buffers(1, &context->stream_buffer);
    if (context->vertex_array != 0)
        context->gl.delete_vertex_arrays(1, &context->vertex_array);
    if (context->program != 0) {
        Core_clear_errors(context);
        context->gl.get_integerv(GL_CURRENT_PROGRAM, &current_program);
        if (Core_operation_succeeded(context)
            && (GLuint)current_program == context->program) {
            context->gl.use_program(0);
        }
        context->gl.delete_program(context->program);
    }
    context->white_texture = 0;
    context->stream_buffer = 0;
    context->vertex_array = 0;
    context->program = 0;
    Core_clear_errors(context);
}

static void Core_prepare_raster_state(CoreContext *context)
{
    context->gl.disable(GL_RASTERIZER_DISCARD);
    context->gl.disable(GL_COLOR_LOGIC_OP);
    context->gl.disable(GL_CULL_FACE);
    context->gl.disable(GL_DEPTH_TEST);
    context->gl.disable(GL_STENCIL_TEST);
    context->gl.disable(GL_DITHER);
    context->gl.disable(GL_FRAMEBUFFER_SRGB);
    context->gl.disable(GL_POLYGON_SMOOTH);
    context->gl.disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    context->gl.disable(GL_SAMPLE_ALPHA_TO_ONE);
    context->gl.disable(GL_SAMPLE_COVERAGE);
    context->gl.disable(GL_SAMPLE_MASK);
    context->gl.polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
    context->gl.color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static RendererStatus Core_begin_frame(void *opaque, int width, int height,
                                       RendererColor clear_color)
{
    CoreContext *context = opaque;

    Core_clear_errors(context);
    context->frame_width = width;
    context->frame_height = height;
    context->gl.viewport(0, 0, width, height);
    context->gl.disable(GL_SCISSOR_TEST);
    context->gl.disable(GL_BLEND);
    Core_prepare_raster_state(context);
    context->gl.clear_color((GLfloat)clear_color.red / 255.0f,
                            (GLfloat)clear_color.green / 255.0f,
                            (GLfloat)clear_color.blue / 255.0f,
                            (GLfloat)clear_color.alpha / 255.0f);
    context->gl.clear(GL_COLOR_BUFFER_BIT);
    return Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                             : RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus Core_set_draw_state(CoreContext *context,
                                          const RendererBackendDraw *draw)
{
    int64_t scissor_bottom;

    if (draw->blend == RENDERER_BLEND_OPAQUE) {
        context->gl.disable(GL_BLEND);
    } else {
        context->gl.enable(GL_BLEND);
        context->gl.blend_equation(GL_FUNC_ADD);
        if (draw->blend == RENDERER_BLEND_ALPHA) {
            context->gl.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else if (draw->blend == RENDERER_BLEND_ADDITIVE) {
            context->gl.blend_func(GL_SRC_ALPHA, GL_ONE);
        } else {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
    }

    if (draw->scissor_enabled) {
        if (draw->scissor.x < 0 || draw->scissor.y < 0
            || draw->scissor.width <= 0 || draw->scissor.height <= 0
            || draw->scissor.x > context->frame_width - draw->scissor.width
            || draw->scissor.y > context->frame_height - draw->scissor.height) {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        scissor_bottom = (int64_t)context->frame_height
            - draw->scissor.y - draw->scissor.height;
        context->gl.enable(GL_SCISSOR_TEST);
        context->gl.scissor(draw->scissor.x, (GLint)scissor_bottom,
                            draw->scissor.width, draw->scissor.height);
    } else {
        context->gl.disable(GL_SCISSOR_TEST);
    }
    return RENDERER_STATUS_OK;
}

static void Core_bind_vertex_layout(CoreContext *context, GLuint buffer)
{
    context->gl.bind_buffer(GL_ARRAY_BUFFER, buffer);
    context->gl.enable_vertex_attrib_array(CORE_POSITION_ATTRIBUTE);
    context->gl.vertex_attrib_pointer(
        CORE_POSITION_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE,
        (GLsizei)sizeof(RendererVertex2D),
        (const void *)offsetof(RendererVertex2D, x));
    context->gl.enable_vertex_attrib_array(CORE_TEXCOORD_ATTRIBUTE);
    context->gl.vertex_attrib_pointer(
        CORE_TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE,
        (GLsizei)sizeof(RendererVertex2D),
        (const void *)offsetof(RendererVertex2D, u));
    context->gl.enable_vertex_attrib_array(CORE_COLOR_ATTRIBUTE);
    context->gl.vertex_attrib_pointer(
        CORE_COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        (GLsizei)sizeof(RendererVertex2D),
        (const void *)offsetof(RendererVertex2D, color));
}

static RendererStatus Core_draw(void *opaque,
                                const RendererBackendDraw *draw)
{
    CoreContext *context = opaque;
    CoreTexture *texture;
    CoreMesh *mesh;
    GLsizeiptr byte_count;
    GLuint buffer;
    GLsizei vertex_count;
    RendererStatus status;

    if (draw == NULL || context->frame_width <= 0 || context->frame_height <= 0)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (draw->mesh != NULL) {
        if (draw->vertices != NULL || draw->vertex_count != 0)
            return RENDERER_STATUS_INVALID_ARGUMENT;
        mesh = draw->mesh;
        buffer = mesh->buffer;
        vertex_count = mesh->vertex_count;
    } else {
        if (draw->vertices == NULL
            || !Core_vertex_data_valid(draw->vertex_count, &byte_count)) {
            return RENDERER_STATUS_INVALID_ARGUMENT;
        }
        buffer = context->stream_buffer;
        vertex_count = (GLsizei)draw->vertex_count;
    }

    Core_clear_errors(context);
    status = Core_set_draw_state(context, draw);
    if (status != RENDERER_STATUS_OK)
        return status;
    Core_prepare_raster_state(context);
    context->gl.use_program(context->program);
    context->gl.bind_vertex_array(context->vertex_array);
    context->gl.uniform_matrix_3fv(context->transform_uniform, 1, GL_FALSE,
                                  draw->transform.m);
    context->gl.uniform_2f(context->framebuffer_size_uniform,
                          (GLfloat)context->frame_width,
                          (GLfloat)context->frame_height);
    context->gl.uniform_1i(context->texture_uniform, 0);
    context->gl.active_texture(GL_TEXTURE0);
    context->gl.bind_sampler(0, 0);
    texture = draw->texture;
    context->gl.bind_texture(GL_TEXTURE_2D,
                             texture != NULL ? texture->name
                                             : context->white_texture);
    Core_bind_vertex_layout(context, buffer);
    if (draw->mesh == NULL) {
        context->gl.buffer_data(GL_ARRAY_BUFFER, byte_count, draw->vertices,
                                GL_STREAM_DRAW);
    }
    if (!Core_operation_succeeded(context))
        return RENDERER_STATUS_BACKEND_ERROR;
    Core_clear_errors(context);
    context->gl.draw_arrays(GL_TRIANGLES, 0, vertex_count);
    return Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                             : RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus Core_flush(void *opaque)
{
    CoreContext *context = opaque;

    Core_clear_errors(context);
    context->gl.flush();
    return Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                             : RENDERER_STATUS_BACKEND_ERROR;
}

static RendererStatus Core_end_frame(void *opaque)
{
    CoreContext *context = opaque;
    RendererStatus status;

    status = Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                               : RENDERER_STATUS_BACKEND_ERROR;
    if (status == RENDERER_STATUS_OK) {
        context->frame_width = 0;
        context->frame_height = 0;
    }
    return status;
}

static RendererStatus Core_texture_create(void *opaque, int width, int height,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch, void **handle)
{
    CoreContext *context = opaque;
    CoreTexture *texture;
    RendererStatus status;

    if (handle == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *handle = NULL;
    if (rgba_pixels == NULL || !Core_pixel_data_valid(width, height, pitch))
        return RENDERER_STATUS_INVALID_ARGUMENT;
    texture = calloc(1, sizeof(*texture));
    if (texture == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    Core_clear_errors(context);
    context->gl.gen_textures(1, &texture->name);
    if (texture->name == 0 || !Core_operation_succeeded(context)) {
        free(texture);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    status = Core_configure_and_upload_texture(context, texture->name,
                                               width, height, rgba_pixels,
                                               pitch);
    if (status != RENDERER_STATUS_OK) {
        context->gl.delete_textures(1, &texture->name);
        Core_clear_errors(context);
        free(texture);
        return status;
    }
    texture->width = width;
    texture->height = height;
    *handle = texture;
    return RENDERER_STATUS_OK;
}

static RendererStatus Core_texture_update(void *opaque, void *handle,
                                          RendererRect region,
                                          const uint8_t *rgba_pixels,
                                          size_t pitch)
{
    CoreContext *context = opaque;
    CoreTexture *texture = handle;
    GLint previous_active_texture;
    GLint previous_texture;
    GLint previous_unpack_alignment;
    GLint previous_unpack_buffer;
    GLint previous_unpack_row_length;
    GLint previous_unpack_skip_rows;
    GLint previous_unpack_skip_pixels;
    RendererStatus status;

    if (texture == NULL || rgba_pixels == NULL
        || region.x < 0 || region.y < 0
        || region.width <= 0 || region.height <= 0
        || region.x > texture->width - region.width
        || region.y > texture->height - region.height
        || !Core_pixel_data_valid(region.width, region.height, pitch)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    Core_clear_errors(context);
    context->gl.get_integerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    context->gl.active_texture(GL_TEXTURE0);
    context->gl.get_integerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    context->gl.get_integerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
    context->gl.get_integerv(GL_PIXEL_UNPACK_BUFFER_BINDING,
                             &previous_unpack_buffer);
    context->gl.get_integerv(GL_UNPACK_ROW_LENGTH,
                             &previous_unpack_row_length);
    context->gl.get_integerv(GL_UNPACK_SKIP_ROWS,
                             &previous_unpack_skip_rows);
    context->gl.get_integerv(GL_UNPACK_SKIP_PIXELS,
                             &previous_unpack_skip_pixels);
    if (!Core_operation_succeeded(context))
        return RENDERER_STATUS_BACKEND_ERROR;

    Core_clear_errors(context);
    context->gl.bind_texture(GL_TEXTURE_2D, texture->name);
    context->gl.bind_buffer(GL_PIXEL_UNPACK_BUFFER, 0);
    context->gl.pixel_store_i(GL_UNPACK_ALIGNMENT, 1);
    context->gl.pixel_store_i(GL_UNPACK_ROW_LENGTH, 0);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_ROWS, 0);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_PIXELS, 0);
    status = Core_upload_bound_texture(context, region.x, region.y,
                                       region.width, region.height,
                                       rgba_pixels, pitch);

    Core_clear_errors(context);
    context->gl.bind_buffer(GL_PIXEL_UNPACK_BUFFER,
                            (GLuint)previous_unpack_buffer);
    context->gl.pixel_store_i(GL_UNPACK_ALIGNMENT,
                              previous_unpack_alignment);
    context->gl.pixel_store_i(GL_UNPACK_ROW_LENGTH,
                              previous_unpack_row_length);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_ROWS,
                              previous_unpack_skip_rows);
    context->gl.pixel_store_i(GL_UNPACK_SKIP_PIXELS,
                              previous_unpack_skip_pixels);
    context->gl.bind_texture(GL_TEXTURE_2D, (GLuint)previous_texture);
    context->gl.active_texture((GLenum)previous_active_texture);
    if (!Core_operation_succeeded(context))
        return RENDERER_STATUS_BACKEND_ERROR;
    return status;
}

static void Core_texture_destroy(void *opaque, void *handle)
{
    CoreContext *context = opaque;
    CoreTexture *texture = handle;

    if (texture == NULL)
        return;
    context->gl.delete_textures(1, &texture->name);
    Core_clear_errors(context);
    free(texture);
}

static RendererStatus Core_mesh_create(void *opaque,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count, void **handle)
{
    CoreContext *context = opaque;
    CoreMesh *mesh;
    GLsizeiptr byte_count;
    GLint previous_buffer;

    if (handle == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *handle = NULL;
    if (vertices == NULL
        || !Core_vertex_data_valid(vertex_count, &byte_count)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    mesh = calloc(1, sizeof(*mesh));
    if (mesh == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    Core_clear_errors(context);
    context->gl.get_integerv(GL_ARRAY_BUFFER_BINDING, &previous_buffer);
    context->gl.gen_buffers(1, &mesh->buffer);
    context->gl.bind_buffer(GL_ARRAY_BUFFER, mesh->buffer);
    context->gl.buffer_data(GL_ARRAY_BUFFER, byte_count, vertices,
                            GL_STATIC_DRAW);
    context->gl.bind_buffer(GL_ARRAY_BUFFER, (GLuint)previous_buffer);
    if (mesh->buffer == 0 || !Core_operation_succeeded(context)) {
        if (mesh->buffer != 0)
            context->gl.delete_buffers(1, &mesh->buffer);
        Core_clear_errors(context);
        free(mesh);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    mesh->vertex_count = (GLsizei)vertex_count;
    *handle = mesh;
    return RENDERER_STATUS_OK;
}

static RendererStatus Core_mesh_update(void *opaque, void *handle,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    CoreContext *context = opaque;
    CoreMesh *mesh = handle;
    GLsizeiptr byte_count;
    GLint previous_buffer;
    GLuint replacement = 0;

    if (mesh == NULL || vertices == NULL
        || !Core_vertex_data_valid(vertex_count, &byte_count)) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    Core_clear_errors(context);
    context->gl.get_integerv(GL_ARRAY_BUFFER_BINDING, &previous_buffer);
    context->gl.gen_buffers(1, &replacement);
    context->gl.bind_buffer(GL_ARRAY_BUFFER, replacement);
    context->gl.buffer_data(GL_ARRAY_BUFFER, byte_count, vertices,
                            GL_STATIC_DRAW);
    context->gl.bind_buffer(GL_ARRAY_BUFFER, (GLuint)previous_buffer);
    if (replacement == 0 || !Core_operation_succeeded(context)) {
        if (replacement != 0)
            context->gl.delete_buffers(1, &replacement);
        Core_clear_errors(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    context->gl.delete_buffers(1, &mesh->buffer);
    mesh->buffer = replacement;
    mesh->vertex_count = (GLsizei)vertex_count;
    return Core_operation_succeeded(context) ? RENDERER_STATUS_OK
                                             : RENDERER_STATUS_BACKEND_ERROR;
}

static void Core_mesh_destroy(void *opaque, void *handle)
{
    CoreContext *context = opaque;
    CoreMesh *mesh = handle;

    if (mesh == NULL)
        return;
    context->gl.delete_buffers(1, &mesh->buffer);
    Core_clear_errors(context);
    free(mesh);
}

static void Core_destroy(void *opaque)
{
    CoreContext *context = opaque;

    Core_release_context_objects(context);
    free(context);
}

static const RendererBackendInterface core_backend_interface = {
    .begin_frame = Core_begin_frame,
    .draw = Core_draw,
    .flush = Core_flush,
    .end_frame = Core_end_frame,
    .texture_create = Core_texture_create,
    .texture_update = Core_texture_update,
    .texture_destroy = Core_texture_destroy,
    .mesh_create = Core_mesh_create,
    .mesh_update = Core_mesh_update,
    .mesh_destroy = Core_mesh_destroy,
    .destroy = Core_destroy
};

RendererStatus Renderer_gl_core_create(RendererGLProcLoader loader,
                                       void *userdata,
                                       Renderer **renderer)
{
    static const uint8_t white_pixel[] = {255, 255, 255, 255};
    CoreContext *context;
    Renderer *created;
    GLint major = 0;
    GLint minor = 0;
    GLint profile = 0;
    RendererStatus status;

    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    *renderer = NULL;
    if (loader == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    context = calloc(1, sizeof(*context));
    if (context == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    if (!Core_load_functions(&context->gl, loader, userdata)) {
        free(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    Core_clear_errors(context);
    context->gl.get_integerv(GL_MAJOR_VERSION, &major);
    context->gl.get_integerv(GL_MINOR_VERSION, &minor);
    context->gl.get_integerv(GL_CONTEXT_PROFILE_MASK, &profile);
    if (!Core_operation_succeeded(context)
        || major < 3 || (major == 3 && minor < 3)
        || (profile & GL_CONTEXT_CORE_PROFILE_BIT) == 0) {
        free(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    if (!Core_create_program(context)) {
        Core_release_context_objects(context);
        free(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }

    Core_clear_errors(context);
    context->gl.gen_vertex_arrays(1, &context->vertex_array);
    context->gl.gen_buffers(1, &context->stream_buffer);
    context->gl.gen_textures(1, &context->white_texture);
    if (context->vertex_array == 0 || context->stream_buffer == 0
        || context->white_texture == 0
        || !Core_operation_succeeded(context)) {
        Core_release_context_objects(context);
        free(context);
        return RENDERER_STATUS_BACKEND_ERROR;
    }
    status = Core_configure_and_upload_texture(
        context, context->white_texture, 1, 1, white_pixel,
        sizeof(white_pixel));
    if (status != RENDERER_STATUS_OK) {
        Core_release_context_objects(context);
        free(context);
        return status;
    }

    created = Renderer_backend_create(&core_backend_interface, context);
    if (created == NULL) {
        Core_release_context_objects(context);
        free(context);
        return RENDERER_STATUS_OUT_OF_MEMORY;
    }
    *renderer = created;
    return RENDERER_STATUS_OK;
}
