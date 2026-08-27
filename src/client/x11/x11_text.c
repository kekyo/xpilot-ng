/* UTF-8 text compatibility adapter for the X11 client. */

#define XP_X11_TEXT_NO_REDIRECT
#include "x11_text.h"

#include "native_text.h"
#include "text_config.h"
#include "xpclient.h"

#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define X11_TEXT_CACHE_LIMIT 128

#ifdef XPILOT_TEST_FONT_DIR
#define X11_TEXT_FONT_DIRECTORY XPILOT_TEST_FONT_DIR
#else
#define X11_TEXT_FONT_DIRECTORY CONF_FONTDIR
#endif

typedef struct X11TextFont {
    Font core_font;
    int ascent;
    int descent;
    XpTextFont *native_font;
    struct X11TextFont *next;
} X11TextFont;

typedef struct X11TextLayout {
    X11TextFont *font;
    char *utf8;
    size_t byte_length;
    XpTextLayout *layout;
    XpTextMetrics metrics;
    Pixmap mask_pixmap;
    Picture mask_picture;
    uint64_t last_use;
    struct X11TextLayout *next;
} X11TextLayout;

extern Display *dpy;
extern Colormap colormap;

static Display *text_display;
static XpTextSystem *text_system;
static X11TextFont *text_fonts;
static X11TextLayout *text_layouts;
static size_t text_layout_count;
static uint64_t text_use_clock;

static int Text_has_non_ascii(const char *text, size_t byte_length)
{
    size_t index;

    for (index = 0; index < byte_length; index++) {
        if ((unsigned char)text[index] >= 0x80)
            return 1;
    }
    return 0;
}

static int Text_complete_length(const char *text, int byte_length,
                                size_t *complete_length)
{
    if (text == NULL || byte_length < 0 || complete_length == NULL)
        return 0;
    return Xp_text_complete_utf8_prefix(
               text, (size_t)byte_length, complete_length)
        == XP_TEXT_STATUS_OK;
}

static int Text_state_initialize(Display *display)
{
    int event_base;
    int error_base;

    if (display == NULL)
        return 0;
    if (text_system != NULL)
        return text_display == display;
    if (!XRenderQueryExtension(display, &event_base, &error_base))
        return 0;
    if (Xp_text_system_create(X11_TEXT_FONT_DIRECTORY, &text_system)
        != XP_TEXT_STATUS_OK) {
        return 0;
    }
    text_display = display;
    return 1;
}

static int Text_font_is_monospace(const XFontStruct *font_struct)
{
    return font_struct->min_bounds.width > 0
        && font_struct->min_bounds.width == font_struct->max_bounds.width;
}

static X11TextFont *Text_font_find(Font core_font)
{
    X11TextFont *font;

    for (font = text_fonts; font != NULL; font = font->next) {
        if (font->core_font == core_font)
            return font;
    }
    return NULL;
}

static X11TextFont *Text_font_acquire(Display *display,
                                      const XFontStruct *font_struct)
{
    XpTextFontRequest request;
    X11TextFont *font;
    const char *families;
    int height;

    if (font_struct == NULL || !Text_state_initialize(display))
        return NULL;
    font = Text_font_find(font_struct->fid);
    if (font != NULL)
        return font;
    height = font_struct->ascent + font_struct->descent;
    if (height <= 0)
        return NULL;
    request.spacing = Text_font_is_monospace(font_struct)
        ? XP_TEXT_SPACING_MONOSPACE : XP_TEXT_SPACING_PROPORTIONAL;
    families = Xp_text_config_families(request.spacing);
    if (families == NULL || families[0] == '\0') {
        families = request.spacing == XP_TEXT_SPACING_MONOSPACE
            ? "Bitstream Vera Sans Mono" : "FreeSans";
    }
    request.family_list = families;
    request.pixel_height = (float)height;
    request.weight = XP_TEXT_WEIGHT_NORMAL;
    request.slant = XP_TEXT_SLANT_NORMAL;
    font = calloc(1, sizeof(*font));
    if (font == NULL)
        return NULL;
    if (Xp_text_font_open(text_system, &request, &font->native_font)
        != XP_TEXT_STATUS_OK) {
        free(font);
        return NULL;
    }
    font->core_font = font_struct->fid;
    font->ascent = font_struct->ascent;
    font->descent = font_struct->descent;
    font->next = text_fonts;
    text_fonts = font;
    return font;
}

static int Text_layout_matches(const X11TextLayout *layout,
                               const X11TextFont *font,
                               const char *text, size_t byte_length)
{
    return layout->font == font && layout->byte_length == byte_length
        && (byte_length == 0
            || memcmp(layout->utf8, text, byte_length) == 0);
}

static void Text_layout_touch(X11TextLayout *layout)
{
    X11TextLayout *cursor;

    if (text_use_clock == UINT64_MAX) {
        uint64_t stamp = 1;

        for (cursor = text_layouts; cursor != NULL; cursor = cursor->next)
            cursor->last_use = stamp++;
        text_use_clock = stamp;
    } else {
        text_use_clock++;
    }
    layout->last_use = text_use_clock;
}

static void Text_layout_destroy(Display *display, X11TextLayout *layout)
{
    if (layout == NULL)
        return;
    if (layout->mask_picture != None)
        XRenderFreePicture(display, layout->mask_picture);
    if (layout->mask_pixmap != None)
        XFreePixmap(display, layout->mask_pixmap);
    Xp_text_layout_destroy(&layout->layout);
    free(layout->utf8);
    free(layout);
}

static void Text_layout_evict_oldest(Display *display)
{
    X11TextLayout **link;
    X11TextLayout **oldest_link = NULL;
    X11TextLayout *oldest = NULL;

    for (link = &text_layouts; *link != NULL; link = &(*link)->next) {
        if (oldest == NULL || (*link)->last_use < oldest->last_use) {
            oldest = *link;
            oldest_link = link;
        }
    }
    if (oldest == NULL || oldest_link == NULL)
        return;
    *oldest_link = oldest->next;
    text_layout_count--;
    Text_layout_destroy(display, oldest);
}

static X11TextLayout *Text_layout_acquire(
    Display *display, X11TextFont *font,
    const char *text, size_t byte_length)
{
    XpTextLayoutRequest request;
    X11TextLayout *layout;

    for (layout = text_layouts; layout != NULL; layout = layout->next) {
        if (Text_layout_matches(layout, font, text, byte_length)) {
            Text_layout_touch(layout);
            return layout;
        }
    }
    if (byte_length == SIZE_MAX)
        return NULL;
    layout = calloc(1, sizeof(*layout));
    if (layout == NULL)
        return NULL;
    layout->utf8 = malloc(byte_length + 1);
    if (layout->utf8 == NULL) {
        free(layout);
        return NULL;
    }
    if (byte_length > 0)
        memcpy(layout->utf8, text, byte_length);
    layout->utf8[byte_length] = '\0';
    layout->font = font;
    layout->byte_length = byte_length;
    request.utf8 = layout->utf8;
    request.byte_length = byte_length;
    request.direction = XP_TEXT_DIRECTION_LTR;
    request.language_bcp47 = "";
    if (Xp_text_layout_create(font->native_font, &request, &layout->layout)
            != XP_TEXT_STATUS_OK
        || Xp_text_layout_measure(layout->layout, &layout->metrics)
            != XP_TEXT_STATUS_OK) {
        Text_layout_destroy(display, layout);
        return NULL;
    }
    if (text_layout_count >= X11_TEXT_CACHE_LIMIT)
        Text_layout_evict_oldest(display);
    Text_layout_touch(layout);
    layout->next = text_layouts;
    text_layouts = layout;
    text_layout_count++;
    return layout;
}

static int Text_layout_create_mask(Display *display, X11TextLayout *layout)
{
    XRenderPictFormat *format;
    XpTextBitmap bitmap;
    XImage *image = NULL;
    GC gc = NULL;
    char *pixels = NULL;
    int x;
    int y;

    if (layout->mask_picture != None || layout->metrics.width == 0)
        return 1;
    memset(&bitmap, 0, sizeof(bitmap));
    if (Xp_text_layout_rasterize(layout->layout, &bitmap)
        != XP_TEXT_STATUS_OK) {
        return 0;
    }
    if (bitmap.pixels == NULL || bitmap.width != layout->metrics.width
        || bitmap.height != layout->metrics.height) {
        Xp_text_bitmap_destroy(&bitmap);
        return 0;
    }
    image = XCreateImage(display, DefaultVisual(display,
                                                DefaultScreen(display)),
                         8, ZPixmap, 0, NULL,
                         (unsigned int)bitmap.width,
                         (unsigned int)bitmap.height, 8, 0);
    if (image == NULL || image->bytes_per_line <= 0
        || (size_t)image->bytes_per_line
               > SIZE_MAX / (size_t)bitmap.height) {
        if (image != NULL)
            XDestroyImage(image);
        Xp_text_bitmap_destroy(&bitmap);
        return 0;
    }
    pixels = calloc((size_t)image->bytes_per_line,
                    (size_t)bitmap.height);
    if (pixels == NULL) {
        XDestroyImage(image);
        Xp_text_bitmap_destroy(&bitmap);
        return 0;
    }
    image->data = pixels;
    for (y = 0; y < bitmap.height; y++) {
        for (x = 0; x < bitmap.width; x++) {
            const unsigned char alpha = bitmap.pixels[
                (size_t)y * bitmap.pitch + (size_t)x * 4 + 3];

            XPutPixel(image, x, y, alpha);
        }
    }
    layout->mask_pixmap = XCreatePixmap(
        display, RootWindow(display, DefaultScreen(display)),
        (unsigned int)bitmap.width, (unsigned int)bitmap.height, 8);
    if (layout->mask_pixmap == None)
        goto cleanup;
    gc = XCreateGC(display, layout->mask_pixmap, 0, NULL);
    if (gc == NULL)
        goto cleanup;
    XPutImage(display, layout->mask_pixmap, gc, image, 0, 0, 0, 0,
              (unsigned int)bitmap.width, (unsigned int)bitmap.height);
    format = XRenderFindStandardFormat(display, PictStandardA8);
    if (format == NULL)
        goto cleanup;
    layout->mask_picture = XRenderCreatePicture(
        display, layout->mask_pixmap, format, 0, NULL);

cleanup:
    if (gc != NULL)
        XFreeGC(display, gc);
    XDestroyImage(image);
    Xp_text_bitmap_destroy(&bitmap);
    if (layout->mask_picture == None) {
        if (layout->mask_pixmap != None) {
            XFreePixmap(display, layout->mask_pixmap);
            layout->mask_pixmap = None;
        }
        return 0;
    }
    return 1;
}

static X11TextLayout *Text_layout_for_core_font(
    Display *display, const XFontStruct *font_struct,
    const char *text, size_t byte_length)
{
    X11TextFont *font = Text_font_acquire(display, font_struct);

    if (font == NULL)
        return NULL;
    return Text_layout_acquire(display, font, text, byte_length);
}

int Xp_x11_native_text_width(XFontStruct *font_struct,
                             const char *string, int count)
{
    X11TextLayout *layout;
    size_t complete_length;

    if (font_struct == NULL || string == NULL || count <= 0)
        return XTextWidth(font_struct, string, count);
    if (!Text_complete_length(string, count, &complete_length))
        return XTextWidth(font_struct, string, count);
    if (complete_length == 0)
        return 0;
    layout = Text_layout_for_core_font(
        dpy, font_struct, string, complete_length);
    return layout != NULL ? layout->metrics.width
                          : XTextWidth(font_struct, string, count);
}

int Xp_x11_text_width(XFontStruct *font_struct,
                      const char *string, int count)
{
    if (font_struct == NULL || string == NULL || count <= 0
        || !Text_has_non_ascii(string, (size_t)(count > 0 ? count : 0))) {
        return XTextWidth(font_struct, string, count);
    }
    return Xp_x11_native_text_width(font_struct, string, count);
}

static int Text_draw_layout(Display *display, Drawable drawable,
                            int x, int y, X11TextLayout *layout,
                            const XGCValues *gc_values)
{
    XRenderPictureAttributes attributes;
    XRenderPictFormat *destination_format;
    XRenderColor render_color;
    XColor xcolor;
    Picture destination;
    Picture source;
    unsigned long attribute_mask;

    if (layout->metrics.width == 0)
        return 1;
    if (!Text_layout_create_mask(display, layout))
        return 0;
    destination_format = XRenderFindVisualFormat(
        display, DefaultVisual(display, DefaultScreen(display)));
    if (destination_format == NULL)
        return 0;
    memset(&attributes, 0, sizeof(attributes));
    attributes.graphics_exposures = gc_values->graphics_exposures;
    attributes.subwindow_mode = gc_values->subwindow_mode;
    attribute_mask = CPGraphicsExposure | CPSubwindowMode;
    destination = XRenderCreatePicture(
        display, drawable, destination_format,
        attribute_mask, &attributes);
    if (destination == None)
        return 0;
    xcolor.pixel = gc_values->foreground;
    XQueryColor(display,
                colormap != None
                    ? colormap
                    : DefaultColormap(display, DefaultScreen(display)),
                &xcolor);
    render_color.red = xcolor.red;
    render_color.green = xcolor.green;
    render_color.blue = xcolor.blue;
    render_color.alpha = 0xffff;
    source = XRenderCreateSolidFill(display, &render_color);
    if (source == None) {
        XRenderFreePicture(display, destination);
        return 0;
    }
    XRenderComposite(
        display, PictOpOver, source, layout->mask_picture, destination,
        0, 0, 0, 0, x, y - layout->metrics.ascent,
        (unsigned int)layout->metrics.width,
        (unsigned int)layout->metrics.height);
    XRenderFreePicture(display, source);
    XRenderFreePicture(display, destination);
    return 1;
}

int Xp_x11_draw_native_string(Display *display, Drawable drawable, GC gc,
                              int x, int y, const char *string, int length)
{
    /* XGetGCValues rejects GCClipMask because Xlib may represent clipping as
     * a client-side region rather than a pixmap. The X11 client does not clip
     * text GCs, so only request attributes the API can return. */
    const unsigned long gc_mask = GCForeground | GCFont
        | GCSubwindowMode | GCGraphicsExposures;
    XFontStruct *font_struct;
    X11TextLayout *layout;
    XGCValues gc_values;
    size_t complete_length;
    int drawn;

    if (display == NULL || gc == NULL || string == NULL || length <= 0) {
        return XDrawString(display, drawable, gc, x, y, string, length);
    }
    if (!Text_complete_length(string, length, &complete_length))
        return XDrawString(display, drawable, gc, x, y, string, length);
    if (complete_length == 0)
        return 0;
    if (!XGetGCValues(display, gc, gc_mask, &gc_values))
        return XDrawString(display, drawable, gc, x, y, string, length);
    font_struct = XQueryFont(display, gc_values.font);
    if (font_struct == NULL)
        return XDrawString(display, drawable, gc, x, y, string, length);
    layout = Text_layout_for_core_font(
        display, font_struct, string, complete_length);
    XFreeFontInfo(NULL, font_struct, 1);
    if (layout == NULL)
        return XDrawString(display, drawable, gc, x, y, string, length);
    drawn = Text_draw_layout(
        display, drawable, x, y, layout, &gc_values);
    return drawn ? 0
                 : XDrawString(display, drawable, gc, x, y,
                               string, length);
}

int Xp_x11_draw_string(Display *display, Drawable drawable, GC gc,
                       int x, int y, const char *string, int length)
{
    if (display == NULL || gc == NULL || string == NULL || length <= 0
        || !Text_has_non_ascii(
            string, (size_t)(length > 0 ? length : 0))) {
        return XDrawString(display, drawable, gc, x, y, string, length);
    }
    return Xp_x11_draw_native_string(
        display, drawable, gc, x, y, string, length);
}

void Xp_x11_text_shutdown(Display *display)
{
    X11TextLayout *layout;
    X11TextFont *font;

    if (text_display != NULL && display != text_display)
        return;
    while (text_layouts != NULL) {
        layout = text_layouts;
        text_layouts = layout->next;
        Text_layout_destroy(display, layout);
    }
    while (text_fonts != NULL) {
        font = text_fonts;
        text_fonts = font->next;
        Xp_text_font_close(&font->native_font);
        free(font);
    }
    Xp_text_system_destroy(&text_system);
    text_display = NULL;
    text_layout_count = 0;
    text_use_clock = 0;
}
