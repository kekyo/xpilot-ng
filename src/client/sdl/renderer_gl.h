/* Backend-specific OpenGL renderer construction contracts. */

#ifndef RENDERER_GL_H
#define RENDERER_GL_H

#include "renderer.h"

/**
 * Resolve an OpenGL entry point for the current context.
 *
 * @param userdata Caller-owned loader context.
 * @param name NUL-terminated OpenGL symbol name.
 * @return Entry point address, or NULL when unavailable.
 *
 * @remarks The callback is invoked only while the context borrowed by the
 * renderer is current. Function addresses are retained only until renderer
 * destruction.
 */
typedef void *(*RendererGLProcLoader)(void *userdata, const char *name);

#endif /* RENDERER_GL_H */
