/* UTF-8 player metadata validation and display helpers. */

#include "xpcommon.h"

#include "utf8_names.h"

static int Utf8_decode(const unsigned char *text, unsigned *codepoint,
		       size_t *length)
{
    unsigned first = text[0];
    unsigned second;
    unsigned third;
    unsigned fourth;

    if (first <= 0x7f) {
	*codepoint = first;
	*length = 1;
	return first != 0;
    }
    second = text[1];
    if (first >= 0xc2 && first <= 0xdf
	&& second >= 0x80 && second <= 0xbf) {
	*codepoint = ((first & 0x1f) << 6) | (second & 0x3f);
	*length = 2;
	return 1;
    }
    third = second != 0 ? text[2] : 0;
    if (first >= 0xe0 && first <= 0xef
	&& second >= 0x80 && second <= 0xbf
	&& third >= 0x80 && third <= 0xbf
	&& !(first == 0xe0 && second < 0xa0)
	&& !(first == 0xed && second > 0x9f)) {
	*codepoint = ((first & 0x0f) << 12)
	    | ((second & 0x3f) << 6) | (third & 0x3f);
	*length = 3;
	return 1;
    }
    fourth = third != 0 ? text[3] : 0;
    if (first >= 0xf0 && first <= 0xf4
	&& second >= 0x80 && second <= 0xbf
	&& third >= 0x80 && third <= 0xbf
	&& fourth >= 0x80 && fourth <= 0xbf
	&& !(first == 0xf0 && second < 0x90)
	&& !(first == 0xf4 && second > 0x8f)) {
	*codepoint = ((first & 0x07) << 18)
	    | ((second & 0x3f) << 12)
	    | ((third & 0x3f) << 6) | (fourth & 0x3f);
	*length = 4;
	return 1;
    }
    return 0;
}

/* Names must not contain spacing or invisible direction-changing code points.
 * Combining marks and variation selectors remain valid because they can be
 * visible parts of a grapheme rendered by SDL_ttf. */
static int Utf8_codepoint_is_name_graphic(unsigned codepoint)
{
    if (codepoint <= 0x20
	|| (codepoint >= 0x7f && codepoint <= 0x9f)
	|| codepoint == 0x00ad || codepoint == 0x061c
	|| codepoint == 0x1680 || codepoint == 0x180e
	|| (codepoint >= 0x2000 && codepoint <= 0x200f)
	|| (codepoint >= 0x2028 && codepoint <= 0x202f)
	|| (codepoint >= 0x205f && codepoint <= 0x206f)
	|| codepoint == 0x3000 || codepoint == 0xfeff
	|| (codepoint >= 0xfdd0 && codepoint <= 0xfdef)
	|| (codepoint & 0xffff) == 0xfffe
	|| (codepoint & 0xffff) == 0xffff) {
	return 0;
    }
    return 1;
}

static int Check_utf8_name(const char *name, size_t maximum_length,
			   int allow_empty)
{
    const unsigned char *text = (const unsigned char *)name;
    size_t offset = 0;

    if (name == NULL || (!allow_empty && name[0] == '\0'))
	return NAME_ERROR;
    while (text[offset] != '\0') {
	unsigned codepoint;
	size_t length;

	if (!Utf8_decode(&text[offset], &codepoint, &length)
	    || !Utf8_codepoint_is_name_graphic(codepoint)
	    || offset > maximum_length - length) {
	    return NAME_ERROR;
	}
	offset += length;
    }
    return NAME_OK;
}

static void Fix_utf8_name(char *name, size_t maximum_length,
			  int allow_empty, const char *fallback)
{
    unsigned char *text = (unsigned char *)name;
    size_t offset = 0;

    if (name == NULL)
	return;
    while (text[offset] != '\0') {
	unsigned codepoint;
	size_t length;

	if (!Utf8_decode(&text[offset], &codepoint, &length)
	    || !Utf8_codepoint_is_name_graphic(codepoint)) {
	    strlcpy(name, fallback, maximum_length + 1);
	    return;
	}
	if (offset > maximum_length - length)
	    break;
	offset += length;
    }
    name[offset] = '\0';
    if (!allow_empty && offset == 0)
	strlcpy(name, fallback, maximum_length + 1);
}

int Check_utf8_user_name(const char *name)
{
    return Check_utf8_name(name, MAX_NAME_LEN - 1, 0);
}

void Fix_utf8_user_name(char *name)
{
    Fix_utf8_name(name, MAX_NAME_LEN - 1, 0, "X");
}

int Check_utf8_disp_name(const char *name)
{
    return Check_utf8_name(name, MAX_DISP_LEN - 1, 1);
}

void Fix_utf8_disp_name(char *name)
{
    Fix_utf8_name(name, MAX_DISP_LEN - 1, 1, "");
}

int Format_player_identity(char *destination, size_t destination_size,
			   const char *nick_name, const char *user_name)
{
    size_t nick_length;
    size_t user_length;
    size_t required_size;
    char *cursor;

    if (destination == NULL || destination_size == 0
	|| nick_name == NULL || user_name == NULL) {
	return 0;
    }
    destination[0] = '\0';
    nick_length = strlen(nick_name);
    if (user_name[0] == '\0' || strcmp(nick_name, user_name) == 0) {
	if (nick_length >= destination_size)
	    return 0;
	memcpy(destination, nick_name, nick_length + 1);
	return 1;
    }
    user_length = strlen(user_name);
    if (nick_length > SIZE_MAX - user_length
	|| nick_length + user_length > SIZE_MAX - 4) {
	return 0;
    }
    required_size = nick_length + user_length + 4;
    if (required_size > destination_size)
	return 0;
    cursor = destination;
    memcpy(cursor, nick_name, nick_length);
    cursor += nick_length;
    *cursor++ = ' ';
    *cursor++ = '(';
    memcpy(cursor, user_name, user_length);
    cursor += user_length;
    *cursor++ = ')';
    *cursor = '\0';
    return 1;
}
