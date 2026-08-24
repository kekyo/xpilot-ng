/* Shared logical font-family configuration. */

#include "xpclient.h"

#include "text_config.h"

#define TEXT_FAMILY_LIST_SIZE 256

static char font_families[TEXT_FAMILY_LIST_SIZE];
static char mono_font_families[TEXT_FAMILY_LIST_SIZE];

static xp_option_t text_options[] = {
    XP_STRING_OPTION(
	"fontFamilies",
	"FreeSans",
	font_families,
	sizeof(font_families),
	NULL, NULL, NULL,
	XP_OPTFLAG_DEFAULT,
	"Comma-separated preferred font family names. Installed Noto Sans Mono "
	"and other system fallbacks are appended automatically.\n"),

    XP_STRING_OPTION(
	"monoFontFamilies",
	"Bitstream Vera Sans Mono",
	mono_font_families,
	sizeof(mono_font_families),
	NULL, NULL, NULL,
	XP_OPTFLAG_DEFAULT,
	"Comma-separated preferred monospace font family names. Installed "
	"Noto Sans Mono and other system fallbacks are appended automatically.\n")
};

void Store_text_options(void)
{
    STORE_OPTIONS(text_options);
}

const char *Xp_text_config_families(XpTextSpacing spacing)
{
    if (spacing == XP_TEXT_SPACING_MONOSPACE)
	return mono_font_families;
    return font_families;
}
