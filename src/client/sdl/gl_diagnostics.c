/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "xpclient_sdl.h"

#include "gl_diagnostics.h"

#include <SDL3/SDL_opengl.h>

#include <ctype.h>

static unsigned int total_errors;

static const char *Gl_error_name(GLenum error_code)
{
    switch (error_code) {
    case GL_INVALID_ENUM:
	return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
	return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
	return "GL_INVALID_OPERATION";
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
    case GL_INVALID_FRAMEBUFFER_OPERATION:
	return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
    case GL_OUT_OF_MEMORY:
	return "GL_OUT_OF_MEMORY";
#ifdef GL_STACK_UNDERFLOW
    case GL_STACK_UNDERFLOW:
	return "GL_STACK_UNDERFLOW";
#endif
#ifdef GL_STACK_OVERFLOW
    case GL_STACK_OVERFLOW:
	return "GL_STACK_OVERFLOW";
#endif
#ifdef GL_CONTEXT_LOST
    case GL_CONTEXT_LOST:
	return "GL_CONTEXT_LOST";
#endif
    default:
	return "unknown error";
    }
}

static const char *Gl_context_string(GLenum name)
{
    const GLubyte *value = glGetString(name);

    return value != NULL ? (const char *)value : "unknown";
}

static const char *Gl_profile_name(int profile)
{
    switch (profile) {
    case SDL_GL_CONTEXT_PROFILE_CORE:
	return "core";
    case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY:
	return "compatibility";
    case SDL_GL_CONTEXT_PROFILE_ES:
	return "ES";
    default:
	return "unspecified";
    }
}

static int Gl_version_at_least_2(const char *version)
{
    unsigned long major = 0;

    if (version == NULL)
	return 0;
    while (*version != '\0' && !isdigit((unsigned char)*version))
	version++;
    if (*version == '\0')
	return 0;
    while (isdigit((unsigned char)*version)) {
	major = major * 10 + (unsigned long)(*version - '0');
	if (major >= 2)
	    return 1;
	version++;
    }
    return 0;
}

void Gl_diagnostics_log_context(void)
{
    int major = 0;
    int minor = 0;
    int profile = 0;
    const char *version;
    const char *shading_language = "not available";

    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);

    version = Gl_context_string(GL_VERSION);
    if (Gl_version_at_least_2(version))
	shading_language = Gl_context_string(GL_SHADING_LANGUAGE_VERSION);

    xpprintf("OpenGL context: version=%s, GLSL=%s, vendor=%s, "
	     "renderer=%s, profile=%s, attributes=%d.%d\n",
	     version, shading_language,
	     Gl_context_string(GL_VENDOR),
	     Gl_context_string(GL_RENDERER),
	     Gl_profile_name(profile), major, minor);
    fflush(stdout);
}

int Gl_diagnostics_check(const char *boundary)
{
    GLenum error_code;
    int errors = 0;

    while ((error_code = glGetError()) != GL_NO_ERROR) {
	warn("OpenGL error at %s: %s (0x%04x)",
	     boundary != NULL ? boundary : "unknown boundary",
	     Gl_error_name(error_code), (unsigned int)error_code);
	errors++;
	total_errors++;
#ifdef GL_CONTEXT_LOST
	if (error_code == GL_CONTEXT_LOST)
	    break;
#endif
    }

    return errors;
}

unsigned int Gl_diagnostics_total_errors(void)
{
    return total_errors;
}

void Gl_diagnostics_reset(void)
{
    total_errors = 0;
}
