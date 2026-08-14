/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef GL_DIAGNOSTICS_H
#define GL_DIAGNOSTICS_H

/** Log identifying information for the current OpenGL context. */
void Gl_diagnostics_log_context(void);

/**
 * Drain and report every error recorded by the current OpenGL context.
 *
 * @param boundary Non-NULL name of the rendering boundary being checked.
 * @return Number of errors drained by this call.
 */
int Gl_diagnostics_check(const char *boundary);

/**
 * Return the number of OpenGL errors observed since the last reset.
 *
 * @return Cumulative number of drained errors.
 */
unsigned int Gl_diagnostics_total_errors(void);

/** Reset the cumulative error count without changing OpenGL error state. */
void Gl_diagnostics_reset(void);

#endif /* GL_DIAGNOSTICS_H */
