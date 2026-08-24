/* UTF-8 text compatibility adapter for the X11 client. */

#ifndef X11_TEXT_H
#define X11_TEXT_H

#include <X11/Xlib.h>

/**
 * Draw a byte range, using installed-font UTF-8 rendering when needed.
 *
 * @param display X display owning the drawable and GC.
 * @param drawable Destination drawable.
 * @param gc Source GC providing font, foreground, and clipping state.
 * @param x Baseline origin X coordinate.
 * @param y Baseline origin Y coordinate.
 * @param string UTF-8 bytes. Printable ASCII retains XDrawString behavior.
 * @param length Available byte count; an incomplete final UTF-8 sequence is
 * omitted.
 * @return Zero after UTF-8 drawing, otherwise the XDrawString result.
 */
int Xp_x11_draw_string(Display *display, Drawable drawable, GC gc,
                       int x, int y, const char *string, int length);

/**
 * Measure a byte range, using installed-font UTF-8 layout when needed.
 *
 * @param font_struct X core font defining the compatible logical size.
 * @param string UTF-8 bytes. Printable ASCII retains XTextWidth behavior.
 * @param count Available byte count; an incomplete final UTF-8 sequence is
 * omitted.
 * @return Logical width in pixels, or the XTextWidth fallback on failure.
 */
int Xp_x11_text_width(XFontStruct *font_struct,
                      const char *string, int count);

/**
 * Release cached native layouts, fonts, and XRender masks.
 *
 * @param display Open display used by the adapter.
 *
 * @remarks This must be called before XCloseDisplay().
 */
void Xp_x11_text_shutdown(Display *display);

#ifndef XP_X11_TEXT_NO_REDIRECT
#define XDrawString Xp_x11_draw_string
#define XTextWidth Xp_x11_text_width
#endif

#endif /* X11_TEXT_H */
