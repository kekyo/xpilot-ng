#include "test_helpers.h"

#define XP_X11_TEXT_NO_REDIRECT
#include "x11_text.h"
#include "text_config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include <stddef.h>

Display *dpy;
Colormap colormap;

const char *Xp_text_config_families(XpTextSpacing spacing)
{
    return spacing == XP_TEXT_SPACING_MONOSPACE
        ? "Bitstream Vera Sans Mono" : "FreeSans";
}

static int count_non_background_pixels(
    Display *display, Drawable drawable, unsigned long background,
    unsigned int x_offset, unsigned int width, unsigned int height)
{
    XImage *image;
    unsigned int x;
    unsigned int y;
    int count = 0;

    image = XGetImage(display, drawable, (int)x_offset, 0, width, height,
                      AllPlanes, ZPixmap);
    TEST_CHECK(image != NULL);
    if (image == NULL)
        return 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (XGetPixel(image, (int)x, (int)y) != background)
                count++;
        }
    }
    XDestroyImage(image);
    return count;
}

static int horizontal_regions_are_equal(
    Display *display, Drawable drawable,
    unsigned int first_y, unsigned int second_y,
    unsigned int width, unsigned int height)
{
    XImage *first;
    XImage *second;
    unsigned int x;
    unsigned int y;
    int equal = 1;

    first = XGetImage(display, drawable, 0, (int)first_y, width, height,
                      AllPlanes, ZPixmap);
    second = XGetImage(display, drawable, 0, (int)second_y, width, height,
                       AllPlanes, ZPixmap);
    if (first == NULL || second == NULL) {
        if (first != NULL)
            XDestroyImage(first);
        if (second != NULL)
            XDestroyImage(second);
        return 0;
    }
    for (y = 0; y < height && equal; y++) {
        for (x = 0; x < width; x++) {
            if (XGetPixel(first, (int)x, (int)y)
                != XGetPixel(second, (int)x, (int)y)) {
                equal = 0;
                break;
            }
        }
    }
    XDestroyImage(first);
    XDestroyImage(second);
    return equal;
}

int main(void)
{
    static const char japanese[] = "\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87";
    static const char score_prefix[] = "R      0.0";
    static const char ascii_score_row[] = "R      0.0  Terminator (robot)";
    static const char utf8_score_row[] =
        "R      0.0  Kouji (\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87)";
    static const char repeated_japanese[] =
        "\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87"
        "\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87"
        "\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87"
        "\xe3\x81\x91\xe3\x81\x8d\xe3\x82\x87";
    const unsigned int width = 400;
    const unsigned int height = 60;
    XFontStruct *font;
    Pixmap pixmap;
    Window root;
    GC gc;
    int render_event;
    int render_error;
    int utf8_width;
    int first_character_width;
    int partial_width;
    int score_prefix_width;
    int repeated_width;
    int ink_pixels;
    int overflow_pixels;

    dpy = XOpenDisplay(NULL);
    TEST_CHECK(dpy != NULL);
    if (dpy == NULL)
        return 1;
    TEST_CHECK(XRenderQueryExtension(dpy, &render_event, &render_error));
    colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    root = RootWindow(dpy, DefaultScreen(dpy));
    pixmap = XCreatePixmap(dpy, root, width, height,
                           (unsigned int)DefaultDepth(
                               dpy, DefaultScreen(dpy)));
    TEST_CHECK(pixmap != None);
    gc = XCreateGC(dpy, pixmap, 0, NULL);
    TEST_CHECK(gc != NULL);
    font = XLoadQueryFont(dpy, "fixed");
    TEST_CHECK(font != NULL);
    if (pixmap == None || gc == NULL || font == NULL)
        return 1;
    XSetFont(dpy, gc, font->fid);

    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);
    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    TEST_CHECK(Xp_x11_draw_native_string(
                   dpy, pixmap, gc, 10, 20, ascii_score_row,
                   (int)(sizeof(ascii_score_row) - 1)) == 0);
    TEST_CHECK(Xp_x11_draw_native_string(
                   dpy, pixmap, gc, 10, 50, utf8_score_row,
                   (int)(sizeof(utf8_score_row) - 1)) == 0);
    score_prefix_width = Xp_x11_native_text_width(
        font, score_prefix, (int)(sizeof(score_prefix) - 1));
    TEST_CHECK(score_prefix_width > 0);
    XSync(dpy, False);
    TEST_CHECK(horizontal_regions_are_equal(
        dpy, pixmap, 0, 30,
        10 + (unsigned int)score_prefix_width, 30));

    TEST_CHECK(Xp_x11_text_width(font, "ASCII", 5)
               == XTextWidth(font, "ASCII", 5));
    utf8_width = Xp_x11_text_width(
        font, japanese, (int)(sizeof(japanese) - 1));
    first_character_width = Xp_x11_text_width(font, japanese, 3);
    partial_width = Xp_x11_text_width(font, japanese, 5);
    TEST_CHECK(utf8_width > 0);
    TEST_CHECK(first_character_width > 0);
    TEST_CHECK(utf8_width > first_character_width);
    TEST_CHECK(partial_width == first_character_width);
    repeated_width = Xp_x11_text_width(
        font, repeated_japanese,
        (int)(sizeof(repeated_japanese) - 1));
    TEST_CHECK(repeated_width > utf8_width);

    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);
    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    TEST_CHECK(Xp_x11_draw_string(
                   dpy, pixmap, gc, 10, 35, repeated_japanese,
                   (int)(sizeof(repeated_japanese) - 1)) == 0);
    XSync(dpy, False);
    ink_pixels = count_non_background_pixels(
        dpy, pixmap, BlackPixel(dpy, DefaultScreen(dpy)), 0, width, height);
    TEST_CHECK(ink_pixels > 20);
    overflow_pixels = count_non_background_pixels(
        dpy, pixmap, BlackPixel(dpy, DefaultScreen(dpy)),
        10 + (unsigned int)repeated_width,
        width - 10 - (unsigned int)repeated_width, height);
    TEST_CHECK(overflow_pixels == 0);

    Xp_x11_text_shutdown(dpy);
    XFreeFont(dpy, font);
    XFreeGC(dpy, gc);
    XFreePixmap(dpy, pixmap);
    XCloseDisplay(dpy);
    dpy = NULL;
    return 0;
}
