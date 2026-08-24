#include "test_helpers.h"

#include "native_text.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef XPILOT_TEST_FONT_DIR
#error "XPILOT_TEST_FONT_DIR must name the application font directory"
#endif

#define TEST_CHECK_CLEANUP(condition)                                      \
    do {                                                                    \
	if (!(condition)) {                                                   \
	    fprintf(stderr, "%s:%d: check failed: %s\n",                    \
		    __FILE__, __LINE__, #condition);                         \
	    goto cleanup;                                                     \
	}                                                                     \
    } while (0)

static int Bitmap_has_ink(const XpTextBitmap *bitmap)
{
    int x;
    int y;

    for (y = 0; y < bitmap->height; y++) {
	const unsigned char *row = bitmap->pixels
	    + (size_t)y * bitmap->pitch;

	for (x = 0; x < bitmap->width; x++) {
	    if (row[(size_t)x * 4 + 3] != 0)
		return 1;
	}
    }
    return 0;
}

static int Check_family_resolution_and_layout(void)
{
    static const char families[] =
	"XPilot Missing Test Font, Bitstream Vera Sans Mono, "
	"Noto Mono, monospace";
    static const char multilingual_text[] =
	"Pilot \xe6\x97\xa5\xe6\x9c\xac";
    static const char ascii_text[] = "Pilot";
    XpTextSystem *system = NULL;
    XpTextFont *font = NULL;
    XpTextLayout *layout = NULL;
    XpTextLayout *unsupported = NULL;
    XpTextBitmap bitmap = { NULL, 0, 0, 0 };
    XpTextMetrics metrics;
    XpTextFontRequest font_request;
    XpTextLayoutRequest layout_request;
    const char *text;
    const char *family;
    size_t text_length;
    int result = 1;

    TEST_CHECK_CLEANUP(Xp_text_system_create(
	XPILOT_TEST_FONT_DIR, &system) == XP_TEXT_STATUS_OK);
    font_request.family_list = families;
    font_request.pixel_height = 16.0f;
    font_request.weight = XP_TEXT_WEIGHT_BOLD;
    font_request.slant = XP_TEXT_SLANT_NORMAL;
    font_request.spacing = XP_TEXT_SPACING_MONOSPACE;
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &font_request, &font) == XP_TEXT_STATUS_OK);

    family = Xp_text_font_resolved_family(font, 0);
    TEST_CHECK_CLEANUP(family != NULL);
    TEST_CHECK_CLEANUP(strstr(family, "Bitstream Vera Sans Mono") != NULL);
    TEST_CHECK_CLEANUP(Xp_text_font_resolved_count(font) >= 1);
    TEST_CHECK_CLEANUP(Xp_text_font_has_glyph(font, '?'));

    if (Xp_text_font_has_glyph(font, 0x65e5)) {
	text = multilingual_text;
	text_length = sizeof(multilingual_text) - 1;
    } else {
	/* The generic fallback is required to work even on installations that
	 * do not provide a CJK system font. */
	text = ascii_text;
	text_length = sizeof(ascii_text) - 1;
    }
    layout_request.utf8 = text;
    layout_request.byte_length = text_length;
    layout_request.direction = XP_TEXT_DIRECTION_LTR;
    layout_request.language_bcp47 = "ja";
    TEST_CHECK_CLEANUP(Xp_text_layout_create(
	font, &layout_request, &layout) == XP_TEXT_STATUS_OK);
    TEST_CHECK_CLEANUP(Xp_text_layout_direction(layout)
	== XP_TEXT_DIRECTION_LTR);
    TEST_CHECK_CLEANUP(strcmp(Xp_text_layout_language(layout), "ja") == 0);
    TEST_CHECK_CLEANUP(Xp_text_layout_measure(layout, &metrics)
	== XP_TEXT_STATUS_OK);
    TEST_CHECK_CLEANUP(metrics.width > 0 && metrics.height > 0
	&& metrics.ascent > 0);
    TEST_CHECK_CLEANUP(Xp_text_layout_rasterize(layout, &bitmap)
	== XP_TEXT_STATUS_OK);
    TEST_CHECK_CLEANUP(bitmap.width == metrics.width
	&& bitmap.height == metrics.height && bitmap.pitch >= bitmap.width * 4
	&& bitmap.pixels != NULL && Bitmap_has_ink(&bitmap));

    layout_request.direction = XP_TEXT_DIRECTION_RTL;
    TEST_CHECK_CLEANUP(Xp_text_layout_create(
	font, &layout_request, &unsupported)
	== XP_TEXT_STATUS_UNSUPPORTED_DIRECTION);
    TEST_CHECK_CLEANUP(unsupported == NULL);

    layout_request.utf8 = "\xe6\x97";
    layout_request.byte_length = 2;
    layout_request.direction = XP_TEXT_DIRECTION_AUTO;
    layout_request.language_bcp47 = NULL;
    TEST_CHECK_CLEANUP(Xp_text_layout_create(
	font, &layout_request, &unsupported) == XP_TEXT_STATUS_INVALID_UTF8);
    TEST_CHECK_CLEANUP(unsupported == NULL);
    result = 0;

cleanup:
    Xp_text_bitmap_destroy(&bitmap);
    Xp_text_layout_destroy(&unsupported);
    Xp_text_layout_destroy(&layout);
    Xp_text_font_close(&font);
    Xp_text_system_destroy(&system);
    return result;
}

int main(void)
{
    return Check_family_resolution_and_layout();
}
