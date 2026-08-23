/* UTF-8 validation for player metadata used by session transports. */

#ifndef UTF8_NAMES_H
#define UTF8_NAMES_H

/**
 * Validate a UTF-8 operating-system user name.
 *
 * @param name NUL-terminated name to validate.
 * @return NAME_OK when the name is nonempty, fits MAX_NAME_LEN, contains
 *         valid UTF-8, and contains no whitespace, control, or invisible
 *         formatting code points; otherwise NAME_ERROR.
 */
int Check_utf8_user_name(const char *name);

/**
 * Make a user name valid UTF-8 within MAX_NAME_LEN.
 *
 * @param name Writable NUL-terminated name. A valid overlong value is
 *        truncated at a UTF-8 boundary; an invalid value becomes "X".
 */
void Fix_utf8_user_name(char *name);

/**
 * Validate a UTF-8 client display identifier.
 *
 * @param name NUL-terminated identifier to validate.
 * @return NAME_OK when the identifier fits MAX_DISP_LEN, contains valid
 *         UTF-8, and contains no whitespace, control, or invisible formatting
 *         code points; otherwise NAME_ERROR. An empty identifier is valid.
 */
int Check_utf8_disp_name(const char *name);

/**
 * Make a client display identifier valid UTF-8 within MAX_DISP_LEN.
 *
 * @param name Writable NUL-terminated identifier. A valid overlong value is
 *        truncated at a UTF-8 boundary; an invalid value becomes empty.
 */
void Fix_utf8_disp_name(char *name);

#endif
