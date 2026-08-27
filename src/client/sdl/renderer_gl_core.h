/* OpenGL 3.3 core renderer backend. */

#ifndef RENDERER_GL_CORE_H
#define RENDERER_GL_CORE_H

#include "renderer_gl.h"

/**
 * Create a shader/VBO backend borrowing the current OpenGL 3.3 context.
 *
 * @param loader Resolver associated with the current context.
 * @param userdata Value passed to loader.
 * @param renderer Receives the renderer on success.
 * @return Operation status.
 *
 * @remarks The caller must keep the active context current for every
 * renderer operation. Before deleting a context, call
 * Renderer_notify_context_lost(); after making a compatible replacement
 * current, call Renderer_restore_context(). The backend never destroys or
 * switches a context.
 */
RendererStatus Renderer_gl_core_create(RendererGLProcLoader loader,
                                       void *userdata,
                                       Renderer **renderer);

#endif /* RENDERER_GL_CORE_H */
