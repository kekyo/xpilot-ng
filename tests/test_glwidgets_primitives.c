#include "test_helpers.h"

#include "glwidgets.h"
#include "renderer.h"
#include "sdlinit.h"
#include "sdlrenderer.h"

#include <GL/gl.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 8
#define MAX_DRAWS 8
#define MAX_FILLS 8
#define MAX_TEXT_DRAWS 8

struct Renderer {
    int frame_active;
};

struct SdlRenderer {
    Renderer *frontend;
    RendererStatus frame_result;
};

struct TextRenderer {
    int identity;
};

struct TextRendererCache {
    int identity;
};

typedef enum DrawEvent {
    DRAW_EVENT_BLEND,
    DRAW_EVENT_TRIANGLES,
    DRAW_EVENT_FILL_RECT,
    DRAW_EVENT_FLUSH,
    DRAW_EVENT_TEXT
} DrawEvent;

typedef struct FakeDraw {
    RendererTexture *texture;
    RendererVertex2D vertices[6];
    size_t vertex_count;
} FakeDraw;

typedef struct FakeFill {
    float x;
    float y;
    float width;
    float height;
    RendererColor color;
} FakeFill;

typedef struct FakeTextDraw {
    string_tex_t *texture;
    uint32_t color;
    int horizontal_alignment;
    int vertical_alignment;
    int x;
    int y;
    bool on_hud;
} FakeTextDraw;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {
    &fake_renderer,
    RENDERER_STATUS_OK
};
static DrawEvent events[MAX_EVENTS];
static FakeDraw draws[MAX_DRAWS];
static FakeFill fills[MAX_FILLS];
static FakeTextDraw text_draws[MAX_TEXT_DRAWS];
static int event_count;
static int call_order;
static int preflight_attempts;
static int preflight_order;
static int blend_order;
static int draw_order;
static int fill_order;
static int flush_order;
static int text_order;
static int operation_result_pending;
static int blend_attempts;
static int draw_attempts;
static int successful_draws;
static int fill_attempts;
static int successful_fills;
static int flush_attempts;
static int text_attempts;
static int successful_text_draws;
static int legacy_begin_calls;
static int legacy_vertex_calls;
static int legacy_color_calls;
static RendererStatus blend_result;
static RendererStatus draw_result;
static RendererStatus fill_result;
static RendererStatus flush_result;
static RendererStatus text_result;
static int clipboard_attempts;
static char clipboard_text[64];

static TextRenderer fake_text_renderer = {1};
static TextRendererCache fake_text_cache = {1};

long loopsSlow;
unsigned draw_width = 160;
unsigned draw_height = 101;
double clientFPS = 60.0;
double tbl_sin[TABLE_SIZE];
Uint32 redRGBA = 0xff0000ff;
Uint32 greenRGBA = 0x00ff00ff;
Uint32 whiteRGBA = 0xffffffff;
font_data gamefont;

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int vertex_equal(const RendererVertex2D *vertex,
                        float x, float y, RendererColor color)
{
    return vertex->x == x && vertex->y == y
        && vertex->u == 0.0f && vertex->v == 0.0f
        && color_equal(vertex->color, color);
}

static void record_event(DrawEvent event)
{
    if (event_count < MAX_EVENTS)
        events[event_count++] = event;
}

static void reset_frame(void)
{
    memset(events, 0, sizeof(events));
    memset(draws, 0, sizeof(draws));
    memset(fills, 0, sizeof(fills));
    memset(text_draws, 0, sizeof(text_draws));
    event_count = 0;
    call_order = 0;
    preflight_attempts = 0;
    preflight_order = 0;
    blend_order = 0;
    draw_order = 0;
    fill_order = 0;
    flush_order = 0;
    text_order = 0;
    operation_result_pending = 0;
    blend_attempts = 0;
    draw_attempts = 0;
    successful_draws = 0;
    fill_attempts = 0;
    successful_fills = 0;
    flush_attempts = 0;
    text_attempts = 0;
    successful_text_draws = 0;
    legacy_begin_calls = 0;
    legacy_vertex_calls = 0;
    legacy_color_calls = 0;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    fill_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    text_result = RENDERER_STATUS_OK;
    clipboard_attempts = 0;
    memset(clipboard_text, 0, sizeof(clipboard_text));
    fake_renderer.frame_active = 1;
    fake_sdl_renderer.frame_result = RENDERER_STATUS_OK;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color = {
        (uint8_t)(rgba >> 24),
        (uint8_t)(rgba >> 16),
        (uint8_t)(rgba >> 8),
        (uint8_t)rgba
    };

    return color;
}

SdlRenderer *Get_sdl_renderer(void)
{
    return &fake_sdl_renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer == &fake_sdl_renderer ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status)
{
    if (operation_result_pending) {
        operation_result_pending = 0;
    } else {
        preflight_attempts++;
        if (preflight_order == 0)
            preflight_order = ++call_order;
    }
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frontend == NULL || !renderer->frontend->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (renderer->frame_result == RENDERER_STATUS_OK
        && status != RENDERER_STATUS_OK) {
        renderer->frame_result = status;
    }
    return renderer->frame_result;
}

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    return renderer == NULL ? RENDERER_STATUS_INVALID_ARGUMENT
                            : renderer->frame_result;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    blend_attempts++;
    if (blend_order == 0)
        blend_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_BLEND);
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return blend_result;
}

RendererStatus Renderer_draw_triangles(Renderer *renderer,
                                       RendererTexture *texture,
                                       const RendererVertex2D *vertices,
                                       size_t vertex_count)
{
    FakeDraw *draw;

    draw_attempts++;
    if (draw_order == 0)
        draw_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_TRIANGLES);
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture != NULL || vertices == NULL
        || (vertex_count != 3 && vertex_count != 6)) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_result != RENDERER_STATUS_OK)
        return draw_result;
    if (successful_draws >= MAX_DRAWS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    draw = &draws[successful_draws++];
    draw->texture = texture;
    draw->vertex_count = vertex_count;
    memcpy(draw->vertices, vertices, vertex_count * sizeof(*vertices));
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_fill_rect(Renderer *renderer,
                                  float x, float y,
                                  float width, float height,
                                  RendererColor color)
{
    FakeFill *fill;

    fill_attempts++;
    if (fill_order == 0)
        fill_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_FILL_RECT);
    if (renderer != &fake_renderer || !renderer->frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    if (fill_result != RENDERER_STATUS_OK)
        return fill_result;
    if (successful_fills >= MAX_FILLS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    fill = &fills[successful_fills++];
    fill->x = x;
    fill->y = y;
    fill->width = width;
    fill->height = height;
    fill->color = color;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
{
    flush_attempts++;
    if (flush_order == 0)
        flush_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_FLUSH);
    if (renderer != &fake_sdl_renderer
        || renderer->frontend == NULL
        || !renderer->frontend->frame_active) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    return flush_result;
}

bool render_text(font_data *font, const char *text,
                 string_tex_t *string_texture)
{
    size_t length;
    char *copy;

    if (font != &gamefont || text == NULL || string_texture == NULL)
        return false;
    length = strlen(text);
    copy = malloc(length + 1);
    if (copy == NULL)
        return false;
    memcpy(copy, text, length + 1);
    memset(string_texture, 0, sizeof(*string_texture));
    string_texture->text_renderer = &fake_text_renderer;
    string_texture->cache = &fake_text_cache;
    string_texture->text = copy;
    string_texture->width = 13;
    string_texture->height = 7;
    string_texture->font_height = 7;
    return true;
}

RendererStatus disp_text(string_tex_t *string_texture, int color,
                         int horizontal_alignment,
                         int vertical_alignment,
                         int x, int y, bool on_hud)
{
    FakeTextDraw *draw;

    text_attempts++;
    if (text_order == 0)
        text_order = ++call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_TEXT);
    if (text_result != RENDERER_STATUS_OK)
        return text_result;
    if (successful_text_draws >= MAX_TEXT_DRAWS)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    draw = &text_draws[successful_text_draws++];
    draw->texture = string_texture;
    draw->color = (uint32_t)color;
    draw->horizontal_alignment = horizontal_alignment;
    draw->vertical_alignment = vertical_alignment;
    draw->x = x;
    draw->y = y;
    draw->on_hud = on_hud;
    return RENDERER_STATUS_OK;
}

void free_string_texture(string_tex_t *string_texture)
{
    if (string_texture == NULL)
        return;
    free(string_texture->text);
    memset(string_texture, 0, sizeof(*string_texture));
}

int Sdl_clipboard_set_text(const char *text)
{
    size_t length;

    clipboard_attempts++;
    if (text == NULL)
        return -1;
    length = strlen(text);
    if (length >= sizeof(clipboard_text))
        length = sizeof(clipboard_text) - 1;
    memcpy(clipboard_text, text, length);
    clipboard_text[length] = '\0';
    return 0;
}

void set_alphacolor(Uint32 color)
{
    (void)color;
    legacy_color_calls++;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green,
                          GLubyte blue, GLubyte alpha)
{
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    legacy_color_calls++;
}

void GLAPIENTRY glBegin(GLenum mode)
{
    (void)mode;
    legacy_begin_calls++;
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
{
    (void)x;
    (void)y;
    legacy_vertex_calls++;
}

void GLAPIENTRY glEnd(void)
{
}

static void destroy_widget(GLWidget *widget)
{
    GLWidget *child;
    GLWidget *next;

    if (widget == NULL)
        return;
    child = widget->children;
    while (child != NULL) {
        next = child->next;
        child->next = NULL;
        destroy_widget(child);
        child = next;
    }
    if (widget->Close != NULL)
        widget->Close(widget);
    free(widget->wid_info);
    free(widget);
}

static int check_successful_draw(RendererColor expected_color,
                                 const float expected_points[3][2])
{
    int index;

    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].vertex_count == 3);
    for (index = 0; index < 3; index++) {
        TEST_CHECK(vertex_equal(
            &draws[0].vertices[index],
            expected_points[index][0], expected_points[index][1],
            expected_color));
    }
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_successful_gradient_quad(
    float x, float y, float width, float height,
    RendererColor top_color, RendererColor bottom_color)
{
    const float points[6][2] = {
        {x, y},
        {x + width, y},
        {x + width, y + height},
        {x, y},
        {x + width, y + height},
        {x, y + height}
    };
    const RendererColor colors[6] = {
        top_color, top_color, bottom_color,
        top_color, bottom_color, bottom_color
    };
    int index;

    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].vertex_count == 6);
    for (index = 0; index < 6; index++) {
        TEST_CHECK(vertex_equal(
            &draws[0].vertices[index],
            points[index][0], points[index][1], colors[index]));
    }
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_successful_fill(float expected_x, float expected_y,
                                 float expected_width,
                                 float expected_height,
                                 RendererColor expected_color)
{
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(fills[0].x == expected_x);
    TEST_CHECK(fills[0].y == expected_y);
    TEST_CHECK(fills[0].width == expected_width);
    TEST_CHECK(fills[0].height == expected_height);
    TEST_CHECK(color_equal(fills[0].color, expected_color));
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_text_draw(string_tex_t *expected_texture,
                           uint32_t expected_color,
                           int expected_horizontal_alignment,
                           int expected_vertical_alignment,
                           int expected_x, int expected_y)
{
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(successful_text_draws == 1);
    TEST_CHECK(text_draws[0].texture == expected_texture);
    TEST_CHECK(text_draws[0].color == expected_color);
    TEST_CHECK(text_draws[0].horizontal_alignment
               == expected_horizontal_alignment);
    TEST_CHECK(text_draws[0].vertical_alignment
               == expected_vertical_alignment);
    TEST_CHECK(text_draws[0].x == expected_x);
    TEST_CHECK(text_draws[0].y == expected_y);
    TEST_CHECK(text_draws[0].on_hud);
    return 0;
}

static int check_successful_background_text(
    float x, float y, float width, float height,
    RendererColor background_color,
    string_tex_t *expected_texture, uint32_t expected_text_color,
    int expected_horizontal_alignment, int expected_vertical_alignment,
    int expected_text_x, int expected_text_y)
{
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(events[3] == DRAW_EVENT_TEXT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(text_order == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fills[0].x == x);
    TEST_CHECK(fills[0].y == y);
    TEST_CHECK(fills[0].width == width);
    TEST_CHECK(fills[0].height == height);
    TEST_CHECK(color_equal(fills[0].color, background_color));
    TEST_CHECK(check_text_draw(
        expected_texture, expected_text_color,
        expected_horizontal_alignment, expected_vertical_alignment,
        expected_text_x, expected_text_y) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_successful_text_without_background(
    string_tex_t *expected_texture, uint32_t expected_text_color,
    int expected_horizontal_alignment, int expected_vertical_alignment,
    int expected_text_x, int expected_text_y)
{
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == DRAW_EVENT_TEXT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(text_order == 2);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(check_text_draw(
        expected_texture, expected_text_color,
        expected_horizontal_alignment, expected_vertical_alignment,
        expected_text_x, expected_text_y) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_direction_geometry(void)
{
    static const ArrowWidget_dir_t directions[4] = {
        RIGHTARROW, UPARROW, LEFTARROW, DOWNARROW
    };
    static const float points[4][3][2] = {
        {{11.0f, 17.0f}, {11.0f, 24.0f}, {20.0f, 20.0f}},
        {{15.0f, 17.0f}, {11.0f, 24.0f}, {20.0f, 24.0f}},
        {{20.0f, 17.0f}, {11.0f, 20.0f}, {20.0f, 24.0f}},
        {{11.0f, 17.0f}, {15.0f, 24.0f}, {20.0f, 17.0f}}
    };
    const RendererColor normal_color = {0xff, 0x00, 0x00, 0xff};
    SDL_Rect bounds = {11, 17, 9, 7};
    int index;

    for (index = 0; index < 4; index++) {
        GLWidget *widget = Init_ArrowWidget(
            directions[index], 2, 3, NULL, NULL);

        TEST_CHECK(widget != NULL);
        SetBounds_GLWidget(widget, &bounds);
        reset_frame();
        widget->Draw(widget);
        TEST_CHECK(check_successful_draw(normal_color, points[index]) == 0);
        destroy_widget(widget);
    }
    return 0;
}

static void count_action(void *data)
{
    int *count = data;

    (*count)++;
}

static int check_slide_geometry_colors_and_state(void)
{
    const RendererColor normal_top = {0xff, 0x00, 0x00, 0xff};
    const RendererColor normal_bottom = {0xff, 0x00, 0x00, 0x77};
    const RendererColor sliding_top = {0x00, 0xff, 0x00, 0xff};
    const RendererColor sliding_bottom = {0x00, 0xff, 0x00, 0x77};
    const RendererColor locked_top = {0x33, 0x33, 0x33, 0xff};
    const RendererColor locked_bottom = {0x33, 0x33, 0x33, 0x77};
    SDL_Rect bounds = {11, 17, 9, 7};
    GLWidget *widget;
    SlideWidget *info;
    int release_count = 0;

    widget = Init_SlideWidget(
        false, NULL, NULL, count_action, &release_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(!info->locked);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        normal_top, normal_bottom) == 0);

    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(release_count == 0);

    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(info->sliding);
    TEST_CHECK(release_count == 0);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        sliding_top, sliding_bottom) == 0);

    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(info->sliding);
    TEST_CHECK(release_count == 0);
    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->sliding);
    TEST_CHECK(release_count == 1);
    destroy_widget(widget);

    widget = Init_SlideWidget(
        true, NULL, NULL, count_action, &release_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(info->locked);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_gradient_quad(
        11.0f, 17.0f, 9.0f, 7.0f,
        locked_top, locked_bottom) == 0);
    TEST_CHECK(release_count == 1);

    destroy_widget(widget);
    return 0;
}

static int check_label_background_and_alignment(void)
{
    Uint32 foreground = 0x12345678;
    Uint32 background = 0x90abcdef;
    Uint32 zero_background = 0x00000000;
    Uint32 transparent_background = 0x23456700;
    const RendererColor background_color = {0x90, 0xab, 0xcd, 0xef};
    const RendererColor transparent_background_color = {
        0x23, 0x45, 0x67, 0x00
    };
    SDL_Rect bounds = {11, 17, 9, 7};
    GLWidget *widget;
    LabelWidget *info;

    widget = Init_LabelWidget(
        "left", &foreground, &background, LEFT, DOWN);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        11.0f, 17.0f, 9.0f, 7.0f, background_color,
        &info->tex, 0x12345678, LEFT, DOWN, 11, 84) == 0);
    destroy_widget(widget);

    widget = Init_LabelWidget(
        "center", NULL, &zero_background, CENTER, CENTER);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_text_without_background(
        &info->tex, 0xffffffff, CENTER, CENTER, 15, 81) == 0);
    destroy_widget(widget);

    widget = Init_LabelWidget(
        "right", &foreground, &transparent_background, RIGHT, UP);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        11.0f, 17.0f, 9.0f, 7.0f,
        transparent_background_color,
        &info->tex, 0x12345678, RIGHT, UP, 20, 77) == 0);
    destroy_widget(widget);
    return 0;
}

typedef struct RadioActionCapture {
    int calls;
    bool state;
} RadioActionCapture;

static void capture_radio_action(bool state, void *data)
{
    RadioActionCapture *capture = data;

    capture->calls++;
    capture->state = state;
}

static void initialize_external_text(string_tex_t *texture,
                                     TextRendererCache *cache,
                                     int width, int height)
{
    memset(texture, 0, sizeof(*texture));
    texture->text_renderer = &fake_text_renderer;
    texture->cache = cache;
    texture->width = width;
    texture->height = height;
}

static int check_radio_background_text_and_toggle(void)
{
    const RendererColor background_color = {0x00, 0x00, 0x00, 0x44};
    TextRendererCache on_cache = {2};
    TextRendererCache off_cache = {3};
    string_tex_t on_texture;
    string_tex_t off_texture;
    RadioActionCapture capture = {0, false};
    SDL_Rect bounds = {21, 31, 11, 9};
    GLWidget *widget;
    LabeledRadiobuttonWidget *info;

    initialize_external_text(&on_texture, &on_cache, 14, 7);
    initialize_external_text(&off_texture, &off_cache, 10, 9);
    widget = Init_LabeledRadiobuttonWidget(
        &on_texture, &off_texture,
        capture_radio_action, &capture, false);
    TEST_CHECK(widget != NULL);
    TEST_CHECK(widget->bounds.w == 19);
    TEST_CHECK(widget->bounds.h == 9);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(!info->state);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        21.0f, 31.0f, 11.0f, 9.0f, background_color,
        &off_texture, 0xff0000ff, CENTER, CENTER, 26, 66) == 0);

    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);
    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->state);
    TEST_CHECK(capture.calls == 0);

    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(info->state);
    TEST_CHECK(capture.calls == 1);
    TEST_CHECK(capture.state);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        21.0f, 31.0f, 11.0f, 9.0f, background_color,
        &on_texture, 0x00ff00ff, CENTER, CENTER, 26, 66) == 0);

    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(info->state);
    TEST_CHECK(capture.calls == 1);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(!info->state);
    TEST_CHECK(capture.calls == 2);
    TEST_CHECK(!capture.state);

    destroy_widget(widget);
    return 0;
}

static int check_text_failure_short_circuit_for_widget(
    GLWidget *widget, int has_background)
{
    int expected_event_count = has_background ? 4 : 1;
    int blend_attempts_after_failure;
    int fill_attempts_after_failure;
    int flush_attempts_after_failure;

    TEST_CHECK(widget != NULL);
    reset_frame();
    text_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(events[expected_event_count - 1] == DRAW_EVENT_TEXT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(text_order == (has_background ? 5 : 2));
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(successful_text_draws == 0);
    if (has_background) {
        TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
        TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
        TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
        TEST_CHECK(blend_attempts == 1);
        TEST_CHECK(fill_attempts == 1);
        TEST_CHECK(successful_fills == 1);
        TEST_CHECK(flush_attempts == 1);
    } else {
        TEST_CHECK(blend_attempts == 0);
        TEST_CHECK(fill_attempts == 0);
        TEST_CHECK(flush_attempts == 0);
    }
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    blend_attempts_after_failure = blend_attempts;
    fill_attempts_after_failure = fill_attempts;
    flush_attempts_after_failure = flush_attempts;
    text_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(blend_attempts == blend_attempts_after_failure);
    TEST_CHECK(fill_attempts == fill_attempts_after_failure);
    TEST_CHECK(flush_attempts == flush_attempts_after_failure);
    TEST_CHECK(text_attempts == 1);
    TEST_CHECK(successful_text_draws == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    return 0;
}

static int check_text_failure_short_circuit(void)
{
    Uint32 zero_background = 0;
    TextRendererCache on_cache = {4};
    TextRendererCache off_cache = {5};
    string_tex_t on_texture;
    string_tex_t off_texture;
    GLWidget *label;
    GLWidget *radio;

    initialize_external_text(&on_texture, &on_cache, 12, 7);
    initialize_external_text(&off_texture, &off_cache, 11, 7);
    label = Init_LabelWidget(
        "failure", NULL, &zero_background, CENTER, CENTER);
    radio = Init_LabeledRadiobuttonWidget(
        &on_texture, &off_texture, NULL, NULL, false);
    TEST_CHECK(label != NULL);
    TEST_CHECK(radio != NULL);
    TEST_CHECK(check_text_failure_short_circuit_for_widget(label, 0) == 0);
    TEST_CHECK(check_text_failure_short_circuit_for_widget(radio, 1) == 0);

    destroy_widget(label);
    destroy_widget(radio);
    return 0;
}

static int check_button_geometry_colors_and_state(void)
{
    Uint32 normal_rgba = 0x12345678;
    Uint32 pressed_rgba = 0x90abcdef;
    const RendererColor normal_color = {0x12, 0x34, 0x56, 0x78};
    const RendererColor pressed_color = {0x90, 0xab, 0xcd, 0xef};
    const RendererColor default_normal_color = {0x00, 0xff, 0x00, 0xff};
    const RendererColor default_pressed_color = {0xff, 0x00, 0x00, 0xff};
    SDL_Rect bounds = {11, 17, 9, 7};
    GLWidget *widget;
    ButtonWidget *info;
    int action_count = 0;

    widget = Init_ButtonWidget(&normal_rgba, &pressed_rgba, 3,
                               count_action, &action_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    loopsSlow = 100;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, normal_color) == 0);
    TEST_CHECK(!info->pressed);
    TEST_CHECK(action_count == 0);

    loopsSlow = 200;
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(info->pressed);
    TEST_CHECK(info->press_time == 200);
    TEST_CHECK(action_count == 1);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 1);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, pressed_color) == 0);
    TEST_CHECK(info->pressed);
    TEST_CHECK(action_count == 1);

    loopsSlow = 203;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, pressed_color) == 0);
    TEST_CHECK(!info->pressed);
    TEST_CHECK(action_count == 1);

    loopsSlow = 204;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, normal_color) == 0);
    TEST_CHECK(action_count == 1);
    destroy_widget(widget);

    widget = Init_ButtonWidget(NULL, NULL, 2,
                               count_action, &action_count);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    loopsSlow = 300;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, default_normal_color) == 0);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 2);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        11.0f, 17.0f, 9.0f, 7.0f, default_pressed_color) == 0);

    destroy_widget(widget);
    return 0;
}

static int check_scrollbar_geometry_and_color(void)
{
    const RendererColor background_color = {0x00, 0x00, 0x00, 0x44};
    SDL_Rect bounds = {13, 19, 21, 27};
    GLWidget *widget = Init_ScrollbarWidget(
        false, 0.25f, 0.5f, SB_VERTICAL, NULL, NULL);

    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        13.0f, 19.0f, 21.0f, 27.0f, background_color) == 0);

    destroy_widget(widget);
    return 0;
}

static int check_state_colors(void)
{
    const float points[3][2] = {
        {0.0f, 0.0f}, {0.0f, 8.0f}, {10.0f, 4.0f}
    };
    const RendererColor normal_color = {0xff, 0x00, 0x00, 0xff};
    const RendererColor press_color = {0x00, 0xff, 0x00, 0xff};
    const RendererColor tap_color = {0xff, 0xff, 0xff, 0xff};
    const RendererColor lock_color = {0x88, 0x00, 0x00, 0x88};
    GLWidget *widget;
    ArrowWidget *info;
    int action_count = 0;

    widget = Init_ArrowWidget(RIGHTARROW, 10, 8,
                              count_action, &action_count);
    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(normal_color, points) == 0);
    TEST_CHECK(action_count == 0);

    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(press_color, points) == 0);
    TEST_CHECK(action_count == 1);
    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);

    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(action_count == 2);
    TEST_CHECK(info->tap);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(tap_color, points) == 0);
    TEST_CHECK(!info->tap);
    TEST_CHECK(action_count == 2);

    info->press = true;
    info->tap = true;
    info->locked = true;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_draw(lock_color, points) == 0);
    TEST_CHECK(action_count == 2);

    destroy_widget(widget);
    return 0;
}

static int check_triangle_failure_short_circuit_for_widget(GLWidget *widget)
{
    int event_count_after_failure;
    int blend_attempts_after_failure;
    int draw_attempts_after_failure;
    int successful_draws_after_failure;
    int flush_attempts_after_failure;

    TEST_CHECK(widget != NULL);

    reset_frame();
    blend_result = RENDERER_STATUS_INVALID_ARGUMENT;
    widget->Draw(widget);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    draw_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    event_count_after_failure = event_count;
    blend_attempts_after_failure = blend_attempts;
    draw_attempts_after_failure = draw_attempts;
    successful_draws_after_failure = successful_draws;
    flush_attempts_after_failure = flush_attempts;
    flush_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == event_count_after_failure);
    TEST_CHECK(blend_attempts == blend_attempts_after_failure);
    TEST_CHECK(draw_attempts == draw_attempts_after_failure);
    TEST_CHECK(successful_draws == successful_draws_after_failure);
    TEST_CHECK(flush_attempts == flush_attempts_after_failure);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    return 0;
}

static int check_failure_short_circuit(void)
{
    GLWidget *widget = Init_ArrowWidget(
        RIGHTARROW, 10, 8, NULL, NULL);

    TEST_CHECK(widget != NULL);
    TEST_CHECK(check_triangle_failure_short_circuit_for_widget(widget) == 0);
    destroy_widget(widget);
    return 0;
}

static int check_slide_failure_short_circuit(void)
{
    GLWidget *widget = Init_SlideWidget(
        false, NULL, NULL, NULL, NULL);

    TEST_CHECK(widget != NULL);
    TEST_CHECK(check_triangle_failure_short_circuit_for_widget(widget) == 0);
    destroy_widget(widget);
    return 0;
}

static int check_fill_failure_short_circuit_for_widget(GLWidget *widget)
{
    TEST_CHECK(widget != NULL);

    reset_frame();
    blend_result = RENDERER_STATUS_INVALID_ARGUMENT;
    widget->Draw(widget);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    reset_frame();
    fill_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(flush_order == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);

    return 0;
}

static int check_fill_failure_short_circuit(void)
{
    Uint32 normal_rgba = 0x12345678;
    Uint32 pressed_rgba = 0x90abcdef;
    SDL_Rect bounds = {7, 9, 11, 13};
    GLWidget *button = Init_ButtonWidget(
        &normal_rgba, &pressed_rgba, 3, NULL, NULL);
    GLWidget *scrollbar = Init_ScrollbarWidget(
        false, 0.25f, 0.5f, SB_HORISONTAL, NULL, NULL);

    TEST_CHECK(button != NULL);
    TEST_CHECK(scrollbar != NULL);
    SetBounds_GLWidget(button, &bounds);
    SetBounds_GLWidget(scrollbar, &bounds);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(button) == 0);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(scrollbar) == 0);

    destroy_widget(button);
    destroy_widget(scrollbar);
    return 0;
}

static int check_label_radio_background_failure_short_circuit(void)
{
    Uint32 background = 0x12345678;
    TextRendererCache on_cache = {6};
    TextRendererCache off_cache = {7};
    string_tex_t on_texture;
    string_tex_t off_texture;
    GLWidget *label;
    GLWidget *radio;

    initialize_external_text(&on_texture, &on_cache, 12, 7);
    initialize_external_text(&off_texture, &off_cache, 11, 7);
    label = Init_LabelWidget(
        "background failure", NULL, &background, CENTER, CENTER);
    radio = Init_LabeledRadiobuttonWidget(
        &on_texture, &off_texture, NULL, NULL, false);
    TEST_CHECK(label != NULL);
    TEST_CHECK(radio != NULL);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(label) == 0);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(radio) == 0);

    destroy_widget(label);
    destroy_widget(radio);
    return 0;
}

static int check_tap_is_consumed_when_flush_fails(void)
{
    GLWidget *widget = Init_ArrowWidget(
        RIGHTARROW, 10, 8, NULL, NULL);
    ArrowWidget *info;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    info->tap = true;
    reset_frame();
    flush_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(!info->tap);

    destroy_widget(widget);
    return 0;
}

static int check_label_scrap_button(void)
{
    Uint32 zero_background = 0;
    GLWidget *widget = Init_LabelWidget(
        "copy-me", NULL, &zero_background, LEFT, DOWN);

    TEST_CHECK(widget != NULL);
    reset_frame();
    widget->button(1, SDL_RELEASED, 0, 0, widget->buttondata);
    widget->button(2, SDL_PRESSED, 0, 0, widget->buttondata);
    widget->button(2, SDL_RELEASED, 0, 0, widget->buttondata);
    TEST_CHECK(clipboard_attempts == 0);
    widget->button(1, SDL_PRESSED, 0, 0, widget->buttondata);
    TEST_CHECK(clipboard_attempts == 1);
    TEST_CHECK(strcmp(clipboard_text, "copy-me") == 0);

    destroy_widget(widget);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_direction_geometry() == 0);
    TEST_CHECK(check_state_colors() == 0);
    TEST_CHECK(check_failure_short_circuit() == 0);
    TEST_CHECK(check_tap_is_consumed_when_flush_fails() == 0);
    TEST_CHECK(check_slide_geometry_colors_and_state() == 0);
    TEST_CHECK(check_slide_failure_short_circuit() == 0);
    TEST_CHECK(check_button_geometry_colors_and_state() == 0);
    TEST_CHECK(check_scrollbar_geometry_and_color() == 0);
    TEST_CHECK(check_fill_failure_short_circuit() == 0);
    TEST_CHECK(check_label_background_and_alignment() == 0);
    TEST_CHECK(check_radio_background_text_and_toggle() == 0);
    TEST_CHECK(check_text_failure_short_circuit() == 0);
    TEST_CHECK(check_label_radio_background_failure_short_circuit() == 0);
    TEST_CHECK(check_label_scrap_button() == 0);
    return 0;
}
