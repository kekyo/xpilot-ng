/* Compatibility-profile OpenGL renderer backend. */

#ifndef RENDERER_GL_LEGACY_H
#define RENDERER_GL_LEGACY_H

#include "renderer_gl.h"

/**
 * Create a fixed-function backend borrowing the current OpenGL context.
 *
 * @param loader Resolver associated with the current context.
 * @param userdata Value passed to loader.
 * @param renderer Receives the renderer on success.
 * @return Operation status.
 *
 * @remarks The caller must keep the same context current for every renderer
 * operation and must destroy the renderer before deleting the context. The
 * backend never destroys or switches the context.
 */
RendererStatus Renderer_gl_legacy_create(RendererGLProcLoader loader,
                                         void *userdata,
                                         Renderer **renderer);

#endif /* RENDERER_GL_LEGACY_H */
