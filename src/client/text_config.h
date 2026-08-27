/* Shared logical font-family configuration. */

#ifndef TEXT_CONFIG_H
#define TEXT_CONFIG_H

#include "native_text.h"

/**
 * Register shared native/browser-compatible font-family options.
 *
 * @remarks The options are named `fontFamilies` and `monoFontFamilies` and
 * accept comma-separated family names in priority order. Native providers
 * append Noto and generic system fallbacks automatically.
 */
void Store_text_options(void);

/**
 * Return the configured preferred family list for a spacing category.
 *
 * @param spacing Proportional or monospace font category.
 * @return Borrowed comma-separated family list.
 */
const char *Xp_text_config_families(XpTextSpacing spacing);

#endif /* TEXT_CONFIG_H */
