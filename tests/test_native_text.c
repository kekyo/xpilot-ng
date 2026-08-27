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

static int Bitmaps_are_equal(const XpTextBitmap *left,
			     const XpTextBitmap *right)
{
    int row;

    if (left->width != right->width || left->height != right->height)
	return 0;
    for (row = 0; row < left->height; row++) {
	if (memcmp(left->pixels + (size_t)row * left->pitch,
		   right->pixels + (size_t)row * right->pitch,
		   (size_t)left->width * 4) != 0) {
	    return 0;
	}
    }
    return 1;
}

static int Font_resolved_family_index(const XpTextFont *font,
				      const char *expected_family)
{
    size_t index;

    for (index = 0; index < Xp_text_font_resolved_count(font); index++) {
	const char *family = Xp_text_font_resolved_family(font, index);

	if (family != NULL && strcmp(family, expected_family) == 0)
	    return (int)index;
    }
    return -1;
}

static int Check_noto_sans_mono_fallback(void)
{
    XpTextSystem *system = NULL;
    XpTextFont *noto_mono_probe = NULL;
    XpTextFont *noto_sans_mono_probe = NULL;
    XpTextFont *fallback_font = NULL;
    XpTextFontRequest request;
    const char *family;
    int noto_mono_index;
    int noto_sans_mono_index;
    int noto_mono_available;
    int noto_sans_mono_available;
    int result = 1;

    TEST_CHECK_CLEANUP(Xp_text_system_create(
	XPILOT_TEST_FONT_DIR, &system) == XP_TEXT_STATUS_OK);
    request.pixel_height = 16.0f;
    request.weight = XP_TEXT_WEIGHT_NORMAL;
    request.slant = XP_TEXT_SLANT_NORMAL;
    request.spacing = XP_TEXT_SPACING_MONOSPACE;

    request.family_list = "Noto Mono";
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &request, &noto_mono_probe) == XP_TEXT_STATUS_OK);
    family = Xp_text_font_resolved_family(noto_mono_probe, 0);
    noto_mono_available = family != NULL
	&& strcmp(family, "Noto Mono") == 0;

    request.family_list = "Noto Sans Mono";
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &request, &noto_sans_mono_probe) == XP_TEXT_STATUS_OK);
    family = Xp_text_font_resolved_family(noto_sans_mono_probe, 0);
    noto_sans_mono_available = family != NULL
	&& strcmp(family, "Noto Sans Mono") == 0;

    request.family_list = "XPilot Missing Test Font";
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &request, &fallback_font) == XP_TEXT_STATUS_OK);
    /* Missing system fonts are valid. When both names exist as distinct
     * installed faces, verify the intended family precedes generic results. */
    if (noto_mono_available && noto_sans_mono_available) {
	noto_sans_mono_index = Font_resolved_family_index(
	    fallback_font, "Noto Sans Mono");
	noto_mono_index = Font_resolved_family_index(
	    fallback_font, "Noto Mono");
	TEST_CHECK_CLEANUP(noto_sans_mono_index >= 0);
	TEST_CHECK_CLEANUP(noto_mono_index < 0
	    || noto_sans_mono_index < noto_mono_index);
    }
    result = 0;

cleanup:
    Xp_text_font_close(&fallback_font);
    Xp_text_font_close(&noto_sans_mono_probe);
    Xp_text_font_close(&noto_mono_probe);
    Xp_text_system_destroy(&system);
    return result;
}

static int Check_sdl_style_cjk_fallback(void)
{
    static const char first_character[] = "\xe3\x81\x91";
    static const char second_character[] = "\xe3\x81\x8d";
    XpTextSystem *system = NULL;
    XpTextFont *cjk_probe = NULL;
    XpTextFont *font = NULL;
    XpTextLayout *first_layout = NULL;
    XpTextLayout *second_layout = NULL;
    XpTextBitmap first_bitmap = { NULL, 0, 0, 0 };
    XpTextBitmap second_bitmap = { NULL, 0, 0, 0 };
    XpTextFontRequest font_request;
    XpTextLayoutRequest layout_request;
    const char *family;
    int cjk_available;
    int result = 1;

    TEST_CHECK_CLEANUP(Xp_text_system_create(
	XPILOT_TEST_FONT_DIR, &system) == XP_TEXT_STATUS_OK);
    font_request.family_list = "Noto Sans Mono CJK JP";
    font_request.pixel_height = 18.0f;
    font_request.weight = XP_TEXT_WEIGHT_NORMAL;
    font_request.slant = XP_TEXT_SLANT_NORMAL;
    font_request.spacing = XP_TEXT_SPACING_MONOSPACE;
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &font_request, &cjk_probe) == XP_TEXT_STATUS_OK);
    family = Xp_text_font_resolved_family(cjk_probe, 0);
    cjk_available = family != NULL
	&& strcmp(family, "Noto Sans Mono CJK JP") == 0;

    font_request.family_list = "FreeSans";
    font_request.weight = XP_TEXT_WEIGHT_BOLD;
    font_request.slant = XP_TEXT_SLANT_ITALIC;
    font_request.spacing = XP_TEXT_SPACING_PROPORTIONAL;
    TEST_CHECK_CLEANUP(Xp_text_font_open(
	system, &font_request, &font) == XP_TEXT_STATUS_OK);
    if (cjk_available) {
	TEST_CHECK_CLEANUP(Xp_text_font_has_glyph(font, 0x3051));
	TEST_CHECK_CLEANUP(Xp_text_font_has_glyph(font, 0x304d));
	layout_request.utf8 = first_character;
	layout_request.byte_length = sizeof(first_character) - 1;
	layout_request.direction = XP_TEXT_DIRECTION_LTR;
	layout_request.language_bcp47 = "ja";
	TEST_CHECK_CLEANUP(Xp_text_layout_create(
	    font, &layout_request, &first_layout) == XP_TEXT_STATUS_OK);
	TEST_CHECK_CLEANUP(Xp_text_layout_rasterize(
	    first_layout, &first_bitmap) == XP_TEXT_STATUS_OK);
	layout_request.utf8 = second_character;
	layout_request.byte_length = sizeof(second_character) - 1;
	TEST_CHECK_CLEANUP(Xp_text_layout_create(
	    font, &layout_request, &second_layout) == XP_TEXT_STATUS_OK);
	TEST_CHECK_CLEANUP(Xp_text_layout_rasterize(
	    second_layout, &second_bitmap) == XP_TEXT_STATUS_OK);
	TEST_CHECK_CLEANUP(!Bitmaps_are_equal(
	    &first_bitmap, &second_bitmap));
    }
    result = 0;

cleanup:
    Xp_text_bitmap_destroy(&second_bitmap);
    Xp_text_bitmap_destroy(&first_bitmap);
    Xp_text_layout_destroy(&second_layout);
    Xp_text_layout_destroy(&first_layout);
    Xp_text_font_close(&font);
    Xp_text_font_close(&cjk_probe);
    Xp_text_system_destroy(&system);
    return result;
}

static int Check_family_resolution_and_layout(void)
{
    static const char families[] =
	"XPilot Missing Test Font, Bitstream Vera Sans Mono, "
	"Noto Sans Mono, monospace";
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
    if (Check_family_resolution_and_layout() != 0)
	return 1;
    if (Check_noto_sans_mono_fallback() != 0)
	return 1;
    return Check_sdl_style_cjk_fallback();
}
