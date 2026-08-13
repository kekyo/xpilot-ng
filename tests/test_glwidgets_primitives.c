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

#define MAX_EVENTS 16
#define MAX_DRAWS 12
#define MAX_FILLS 8
#define MAX_TEXT_DRAWS 8
#define MAX_DRAW_VERTICES 12

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
    RendererVertex2D vertices[MAX_DRAW_VERTICES];
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
static int legacy_blend_calls;
static RendererStatus blend_result;
static RendererStatus draw_result;
static int draw_failure_attempt;
static RendererStatus draw_failure_result;
static RendererStatus fill_result;
static RendererStatus flush_result;
static RendererStatus text_result;
static int clipboard_attempts;
static char clipboard_text[64];
static int live_text_allocations;

static TextRenderer fake_text_renderer = {1};
static TextRendererCache fake_text_cache = {1};
static setup_t fake_setup = {
    .frames_per_second = 60
};

long loopsSlow;
unsigned draw_width = 160;
unsigned draw_height = 101;
double clientFPS = 60.0;
double tbl_sin[TABLE_SIZE];
int maxFPS = 60;
Uint32 redRGBA = 0xff0000ff;
Uint32 greenRGBA = 0x00ff00ff;
Uint32 whiteRGBA = 0xffffffff;
Uint32 blackRGBA = 0x000000ff;
Uint32 yellowRGBA = 0xffff00ff;
Uint32 nullRGBA = 0x00000000;
font_data gamefont;
setup_t *Setup = &fake_setup;
GLWidget *MainWidget;
GLWidget *clicktarget[NUM_MOUSE_BUTTONS];
GLWidget *hovertarget;

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

static int check_recorded_draw(
    int draw_index, size_t expected_vertex_count,
    const float expected_points[][2],
    const RendererColor expected_colors[])
{
    size_t index;

    TEST_CHECK(draw_index >= 0);
    TEST_CHECK(draw_index < successful_draws);
    TEST_CHECK(draws[draw_index].texture == NULL);
    TEST_CHECK(draws[draw_index].vertex_count == expected_vertex_count);
    for (index = 0; index < expected_vertex_count; index++) {
        TEST_CHECK(vertex_equal(
            &draws[draw_index].vertices[index],
            expected_points[index][0], expected_points[index][1],
            expected_colors[index]));
    }
    return 0;
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
    legacy_blend_calls = 0;
    blend_result = RENDERER_STATUS_OK;
    draw_result = RENDERER_STATUS_OK;
    draw_failure_attempt = 0;
    draw_failure_result = RENDERER_STATUS_OK;
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
    call_order++;
    if (draw_order == 0)
        draw_order = call_order;
    operation_result_pending = 1;
    record_event(DRAW_EVENT_TRIANGLES);
    if (renderer != &fake_renderer || !renderer->frame_active
        || texture != NULL || vertices == NULL
        || (vertex_count != 3 && vertex_count != 6
            && vertex_count != 12)) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (draw_failure_attempt == draw_attempts)
        return draw_failure_result;
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
    string_tex_t replacement = {0};
    size_t length;
    char *copy;

    if (font != &gamefont || text == NULL || string_texture == NULL)
        return false;
    length = strlen(text);
    copy = malloc(length + 1);
    if (copy == NULL)
        return false;
    memcpy(copy, text, length + 1);
    replacement.text_renderer = &fake_text_renderer;
    replacement.cache = &fake_text_cache;
    replacement.text = copy;
    replacement.width = 13;
    replacement.height = 7;
    replacement.font_height = 7;
    if (string_texture->text != NULL) {
        free(string_texture->text);
        live_text_allocations--;
    }
    *string_texture = replacement;
    live_text_allocations++;
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
    if (string_texture->text != NULL) {
        free(string_texture->text);
        live_text_allocations--;
    }
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

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
    legacy_blend_calls++;
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

static int check_successful_preflight_only(void)
{
    TEST_CHECK(event_count == 0);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(call_order == 1);
    TEST_CHECK(blend_attempts == 0);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
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

static int decayed_direction(int direction)
{
    if (direction > 0)
        return direction - 1;
    if (direction < 0)
        return direction + 1;
    return 0;
}

static int check_text_failure_short_circuit_for_widget(
    GLWidget *widget, int has_background, int *direction)
{
    int expected_event_count = has_background ? 4 : 1;
    int expected_direction = direction == NULL
        ? 0 : decayed_direction(*direction);
    int blend_attempts_after_failure;
    int fill_attempts_after_failure;
    int flush_attempts_after_failure;

    TEST_CHECK(widget != NULL);
    reset_frame();
    text_result = RENDERER_STATUS_BACKEND_ERROR;
    widget->Draw(widget);
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    TEST_CHECK(check_text_failure_short_circuit_for_widget(
        label, 0, NULL) == 0);
    TEST_CHECK(check_text_failure_short_circuit_for_widget(
        radio, 1, NULL) == 0);

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

static int check_fill_failure_short_circuit_for_widget(
    GLWidget *widget, int *direction)
{
    int expected_direction;

    TEST_CHECK(widget != NULL);

    reset_frame();
    blend_result = RENDERER_STATUS_INVALID_ARGUMENT;
    expected_direction = direction == NULL
        ? 0 : decayed_direction(*direction);
    widget->Draw(widget);
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    expected_direction = direction == NULL
        ? 0 : decayed_direction(*direction);
    widget->Draw(widget);
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    expected_direction = direction == NULL
        ? 0 : decayed_direction(*direction);
    widget->Draw(widget);
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    if (direction != NULL)
        TEST_CHECK(*direction == expected_direction);
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
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        button, NULL) == 0);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        scrollbar, NULL) == 0);

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
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        label, NULL) == 0);
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        radio, NULL) == 0);

    destroy_widget(label);
    destroy_widget(radio);
    return 0;
}

typedef struct ChooserUpdateCapture {
    int calls;
    const char *value;
} ChooserUpdateCapture;

static void capture_chooser_update(void *data, const char *value)
{
    ChooserUpdateCapture *capture = data;

    capture->calls++;
    capture->value = value;
}

static int check_bool_chooser_parent_paint(void)
{
    const RendererColor background_color = {0x90, 0xab, 0xcd, 0xef};
    Uint32 foreground = 0x12345678;
    Uint32 background = 0x90abcdef;
    Uint32 zero_background = 0;
    SDL_Rect bounds = {7, 9, 41, 13};
    ChooserUpdateCapture capture = {0, NULL};
    bool value = false;
    int allocations_before = live_text_allocations;
    GLWidget *widget;
    BoolChooserWidget *info;
    LabeledRadiobuttonWidget *button_info;

    widget = Init_BoolChooserWidget(
        "Boolean", &value, &foreground, &background,
        capture_chooser_update, &capture);
    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(widget->children == info->name);
    TEST_CHECK(info->name->next == info->buttonwidget);
    TEST_CHECK(info->buttonwidget->next == NULL);
    button_info = info->buttonwidget->wid_info;
    TEST_CHECK(button_info != NULL);
    TEST_CHECK(!button_info->state);
    SetBounds_GLWidget(widget, &bounds);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_fill(
        7.0f, 9.0f, 41.0f, 13.0f, background_color) == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(!value);
    TEST_CHECK(!button_info->state);
    TEST_CHECK(capture.calls == 0);

    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        widget, NULL) == 0);
    TEST_CHECK(!value);
    TEST_CHECK(!button_info->state);
    TEST_CHECK(capture.calls == 0);
    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);

    widget = Init_BoolChooserWidget(
        "Null background", &value, &foreground, NULL,
        capture_chooser_update, &capture);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_preflight_only() == 0);
    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);

    widget = Init_BoolChooserWidget(
        "Zero background", &value, &foreground, &zero_background,
        capture_chooser_update, &capture);
    TEST_CHECK(widget != NULL);
    SetBounds_GLWidget(widget, &bounds);
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_preflight_only() == 0);
    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);
    return 0;
}

static int check_int_chooser_parent_paint(void)
{
    const RendererColor background_color = {0x23, 0x45, 0x67, 0x00};
    Uint32 foreground = 0x12345678;
    Uint32 background = 0x23456700;
    SDL_Rect bounds = {11, 17, 53, 9};
    ChooserUpdateCapture capture = {0, NULL};
    int value = 4;
    int allocations_before = live_text_allocations;
    int allocations_before_update;
    GLWidget *widget = Init_IntChooserWidget(
        "Integer", &value, 0, 10, &foreground, &background,
        capture_chooser_update, &capture);
    IntChooserWidget *info;
    ArrowWidget *right_arrow;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(widget->children == info->name);
    TEST_CHECK(info->name->next == info->leftarrow);
    TEST_CHECK(info->leftarrow->next == info->rightarrow);
    TEST_CHECK(info->rightarrow->next == NULL);
    SetBounds_GLWidget(widget, &bounds);
    TEST_CHECK(info->rightarrow->bounds.x == 59);
    TEST_CHECK(info->rightarrow->bounds.y == 20);
    TEST_CHECK(info->rightarrow->bounds.w == 3);
    TEST_CHECK(info->rightarrow->bounds.h == 3);

    right_arrow = info->rightarrow->wid_info;
    TEST_CHECK(right_arrow != NULL);
    TEST_CHECK(!right_arrow->locked);
    TEST_CHECK(!right_arrow->tap);
    allocations_before_update = live_text_allocations;
    info->rightarrow->button(
        2, SDL_PRESSED, 0, 0, info->rightarrow->buttondata);
    TEST_CHECK(value == 5);
    TEST_CHECK(info->direction == 2);
    TEST_CHECK(right_arrow->tap);
    TEST_CHECK(capture.calls == 1);
    TEST_CHECK(capture.value == NULL);
    TEST_CHECK(strcmp(info->valuetex.text, "5") == 0);
    TEST_CHECK(live_text_allocations == allocations_before_update);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        11.0f, 17.0f, 53.0f, 9.0f, background_color,
        &info->valuetex, 0x12345678, RIGHT, CENTER, 56, 80) == 0);
    TEST_CHECK(info->direction == 1);
    TEST_CHECK(right_arrow->tap);
    TEST_CHECK(value == 5);
    TEST_CHECK(capture.calls == 1);

    foreground = 0;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        11.0f, 17.0f, 53.0f, 9.0f, background_color,
        &info->valuetex, 0x00000000, RIGHT, CENTER, 56, 80) == 0);
    TEST_CHECK(info->direction == 0);
    TEST_CHECK(right_arrow->tap);

    info->direction = 2;
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        widget, &info->direction) == 0);
    TEST_CHECK(right_arrow->tap);
    TEST_CHECK(value == 5);
    TEST_CHECK(capture.calls == 1);

    info->direction = 2;
    TEST_CHECK(check_text_failure_short_circuit_for_widget(
        widget, 1, &info->direction) == 0);
    TEST_CHECK(right_arrow->tap);
    TEST_CHECK(value == 5);
    TEST_CHECK(capture.calls == 1);

    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);
    return 0;
}

static int check_double_chooser_parent_paint(void)
{
    const RendererColor background_color = {0x90, 0xab, 0xcd, 0xef};
    Uint32 background = 0x90abcdef;
    SDL_Rect bounds = {23, 29, 55, 11};
    ChooserUpdateCapture capture = {0, NULL};
    double value = 1.25;
    int allocations_before = live_text_allocations;
    int allocations_before_update;
    GLWidget *widget = Init_DoubleChooserWidget(
        "Double", &value, 0.0, 2.0, NULL, NULL,
        capture_chooser_update, &capture);
    DoubleChooserWidget *info;
    ArrowWidget *left_arrow;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(widget->children == info->name);
    TEST_CHECK(info->name->next == info->leftarrow);
    TEST_CHECK(info->leftarrow->next == info->rightarrow);
    TEST_CHECK(info->rightarrow->next == NULL);
    SetBounds_GLWidget(widget, &bounds);
    TEST_CHECK(info->rightarrow->bounds.x == 73);
    TEST_CHECK(info->rightarrow->bounds.y == 33);
    TEST_CHECK(info->rightarrow->bounds.w == 3);
    TEST_CHECK(info->rightarrow->bounds.h == 3);

    left_arrow = info->leftarrow->wid_info;
    TEST_CHECK(left_arrow != NULL);
    TEST_CHECK(!left_arrow->locked);
    TEST_CHECK(!left_arrow->tap);
    allocations_before_update = live_text_allocations;
    info->leftarrow->button(
        2, SDL_PRESSED, 0, 0, info->leftarrow->buttondata);
    TEST_CHECK(value > 1.239 && value < 1.241);
    TEST_CHECK(info->direction == -2);
    TEST_CHECK(left_arrow->tap);
    TEST_CHECK(capture.calls == 1);
    TEST_CHECK(capture.value == NULL);
    TEST_CHECK(strcmp(info->valuetex.text, "1.24") == 0);
    TEST_CHECK(live_text_allocations == allocations_before_update);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_text_without_background(
        &info->valuetex, 0xffffffff,
        RIGHT, CENTER, 70, 67) == 0);
    TEST_CHECK(info->direction == -1);
    TEST_CHECK(left_arrow->tap);
    TEST_CHECK(capture.calls == 1);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_text_without_background(
        &info->valuetex, 0xffffffff,
        RIGHT, CENTER, 70, 67) == 0);
    TEST_CHECK(info->direction == 0);
    TEST_CHECK(left_arrow->tap);

    info->direction = -2;
    TEST_CHECK(check_text_failure_short_circuit_for_widget(
        widget, 0, &info->direction) == 0);
    TEST_CHECK(left_arrow->tap);
    TEST_CHECK(capture.calls == 1);

    info->bgcolor = &background;
    info->direction = -2;
    TEST_CHECK(check_fill_failure_short_circuit_for_widget(
        widget, &info->direction) == 0);
    TEST_CHECK(left_arrow->tap);
    TEST_CHECK(capture.calls == 1);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_background_text(
        23.0f, 29.0f, 55.0f, 11.0f, background_color,
        &info->valuetex, 0xffffffff, RIGHT, CENTER, 70, 67) == 0);
    TEST_CHECK(info->direction == 0);
    TEST_CHECK(left_arrow->tap);

    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);
    return 0;
}

static int check_successful_color_chooser_parent_paint(
    int has_background, RendererColor expected_background)
{
    const RendererColor black = {0x00, 0x00, 0x00, 0xff};
    const float points[3][2] = {
        {35.0f, 19.0f}, {35.0f, 22.0f}, {38.0f, 22.0f}
    };
    int expected_event_count = has_background ? 4 : 3;
    int index;

    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    if (has_background) {
        TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
        TEST_CHECK(events[2] == DRAW_EVENT_TRIANGLES);
        TEST_CHECK(events[3] == DRAW_EVENT_FLUSH);
    } else {
        TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
        TEST_CHECK(events[2] == DRAW_EVENT_FLUSH);
    }
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == (has_background ? 3 : 0));
    TEST_CHECK(draw_order == (has_background ? 4 : 3));
    TEST_CHECK(flush_order == (has_background ? 5 : 4));
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == has_background);
    TEST_CHECK(successful_fills == has_background);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 1);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    if (has_background) {
        TEST_CHECK(fills[0].x == 11.0f);
        TEST_CHECK(fills[0].y == 17.0f);
        TEST_CHECK(fills[0].width == 29.0f);
        TEST_CHECK(fills[0].height == 13.0f);
        TEST_CHECK(color_equal(fills[0].color, expected_background));
    }
    TEST_CHECK(draws[0].texture == NULL);
    TEST_CHECK(draws[0].vertex_count == 3);
    for (index = 0; index < 3; index++) {
        TEST_CHECK(vertex_equal(
            &draws[0].vertices[index],
            points[index][0], points[index][1], black));
    }
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    return 0;
}

static int check_color_chooser_partial_failures(GLWidget *widget)
{
    TEST_CHECK(widget != NULL);

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
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    fill_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(draw_attempts == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);

    reset_frame();
    draw_result = RENDERER_STATUS_BACKEND_ERROR;
    flush_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[2] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[3] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(draw_order == 4);
    TEST_CHECK(flush_order == 5);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    draw_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    return 0;
}

static int check_color_chooser_parent_paint(void)
{
    const RendererColor transparent_background = {
        0x12, 0x34, 0x56, 0x00
    };
    Uint32 value = 0xabcdef88;
    Uint32 foreground = 0x102030ff;
    Uint32 background = 0x12345600;
    Uint32 zero_background = 0;
    SDL_Rect bounds = {11, 17, 29, 13};
    int allocations_before = live_text_allocations;
    GLWidget *widget = Init_ColorChooserWidget(
        "Color", &value, &foreground, &background, NULL, NULL);
    ColorChooserWidget *info;
    ButtonWidget *button_info;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(widget->children == info->name);
    TEST_CHECK(info->name->next == info->button);
    TEST_CHECK(info->button->next == NULL);
    TEST_CHECK(!info->expanded);
    TEST_CHECK(info->mod == NULL);
    SetBounds_GLWidget(widget, &bounds);
    TEST_CHECK(info->button->bounds.x == 35);
    TEST_CHECK(info->button->bounds.y == 19);
    TEST_CHECK(info->button->bounds.w == 3);
    TEST_CHECK(info->button->bounds.h == 3);
    button_info = info->button->wid_info;
    TEST_CHECK(button_info != NULL);
    TEST_CHECK(!button_info->pressed);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_chooser_parent_paint(
        1, transparent_background) == 0);
    TEST_CHECK(!info->expanded);
    TEST_CHECK(info->mod == NULL);
    TEST_CHECK(!button_info->pressed);
    TEST_CHECK(value == 0xabcdef88);

    info->bgcolor = NULL;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_chooser_parent_paint(
        0, transparent_background) == 0);

    info->bgcolor = &zero_background;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_chooser_parent_paint(
        0, transparent_background) == 0);

    info->bgcolor = &background;
    TEST_CHECK(check_color_chooser_partial_failures(widget) == 0);
    TEST_CHECK(!info->expanded);
    TEST_CHECK(info->mod == NULL);
    TEST_CHECK(!button_info->pressed);
    TEST_CHECK(value == 0xabcdef88);

    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);
    return 0;
}

static int check_successful_color_mod_parent_paint(
    int has_background, RendererColor expected_background,
    RendererColor value_color)
{
    const RendererColor black = {0x00, 0x00, 0x00, 0xff};
    const RendererColor white = {0xff, 0xff, 0xff, 0xff};
    const float diagonal_points[6][2] = {
        {76.0f, 19.0f}, {76.0f, 46.0f}, {110.0f, 46.0f},
        {76.0f, 19.0f}, {110.0f, 46.0f}, {110.0f, 19.0f}
    };
    const RendererColor diagonal_colors[6] = {
        black, black, black, white, white, white
    };
    const float fan_points[12][2] = {
        {106.0f, 46.0f}, {110.0f, 46.0f}, {110.0f, 43.0f},
        {106.0f, 46.0f}, {110.0f, 43.0f}, {79.0f, 19.0f},
        {106.0f, 46.0f}, {79.0f, 19.0f}, {76.0f, 19.0f},
        {106.0f, 46.0f}, {76.0f, 19.0f}, {76.0f, 21.0f}
    };
    const RendererColor fan_colors[12] = {
        white, white, white,
        white, white, black,
        white, black, black,
        white, black, black
    };
    const float value_points[6][2] = {
        {79.0f, 21.0f}, {79.0f, 43.0f}, {106.0f, 43.0f},
        {79.0f, 21.0f}, {106.0f, 43.0f}, {106.0f, 21.0f}
    };
    const RendererColor value_colors[6] = {
        value_color, value_color, value_color,
        value_color, value_color, value_color
    };
    int expected_event_count = has_background ? 6 : 5;
    int triangle_event = has_background ? 2 : 1;

    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    if (has_background)
        TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    TEST_CHECK(events[triangle_event] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[triangle_event + 1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[triangle_event + 2] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[triangle_event + 3] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == (has_background ? 3 : 0));
    TEST_CHECK(draw_order == (has_background ? 4 : 3));
    TEST_CHECK(flush_order == (has_background ? 7 : 6));
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == has_background);
    TEST_CHECK(successful_fills == has_background);
    TEST_CHECK(draw_attempts == 3);
    TEST_CHECK(successful_draws == 3);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    if (has_background) {
        TEST_CHECK(fills[0].x == 11.0f);
        TEST_CHECK(fills[0].y == 17.0f);
        TEST_CHECK(fills[0].width == 101.0f);
        TEST_CHECK(fills[0].height == 31.0f);
        TEST_CHECK(color_equal(fills[0].color, expected_background));
    }
    TEST_CHECK(check_recorded_draw(
        0, 6, diagonal_points, diagonal_colors) == 0);
    TEST_CHECK(check_recorded_draw(
        1, 12, fan_points, fan_colors) == 0);
    TEST_CHECK(check_recorded_draw(
        2, 6, value_points, value_colors) == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result == RENDERER_STATUS_OK);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    return 0;
}

static int check_color_mod_later_draw_failure(
    GLWidget *widget, int failure_attempt)
{
    int index;
    int expected_event_count = failure_attempt + 3;
    int successful_draws_after_failure = failure_attempt - 1;

    TEST_CHECK(widget != NULL);
    TEST_CHECK(failure_attempt == 2 || failure_attempt == 3);
    reset_frame();
    draw_failure_attempt = failure_attempt;
    draw_failure_result = RENDERER_STATUS_BACKEND_ERROR;
    flush_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    widget->Draw(widget);
    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_FILL_RECT);
    for (index = 0; index < failure_attempt; index++)
        TEST_CHECK(events[index + 2] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(events[expected_event_count - 1] == DRAW_EVENT_FLUSH);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 3);
    TEST_CHECK(draw_order == 4);
    TEST_CHECK(flush_order == failure_attempt + 4);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(draw_attempts == failure_attempt);
    TEST_CHECK(successful_draws == successful_draws_after_failure);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);

    draw_failure_attempt = 0;
    draw_failure_result = RENDERER_STATUS_OK;
    flush_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == expected_event_count);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 1);
    TEST_CHECK(successful_fills == 1);
    TEST_CHECK(draw_attempts == failure_attempt);
    TEST_CHECK(successful_draws == successful_draws_after_failure);
    TEST_CHECK(flush_attempts == 1);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    return 0;
}

static int check_color_mod_first_draw_failure(GLWidget *widget)
{
    TEST_CHECK(widget != NULL);
    reset_frame();
    draw_failure_attempt = 1;
    draw_failure_result = RENDERER_STATUS_INVALID_ARGUMENT;
    widget->Draw(widget);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(events[0] == DRAW_EVENT_BLEND);
    TEST_CHECK(events[1] == DRAW_EVENT_TRIANGLES);
    TEST_CHECK(preflight_attempts == 1);
    TEST_CHECK(preflight_order == 1);
    TEST_CHECK(blend_order == 2);
    TEST_CHECK(fill_order == 0);
    TEST_CHECK(draw_order == 3);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);

    draw_failure_attempt = 0;
    draw_failure_result = RENDERER_STATUS_OK;
    widget->Draw(widget);
    TEST_CHECK(preflight_attempts == 2);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(blend_attempts == 1);
    TEST_CHECK(fill_attempts == 0);
    TEST_CHECK(draw_attempts == 1);
    TEST_CHECK(successful_draws == 0);
    TEST_CHECK(flush_attempts == 0);
    TEST_CHECK(text_attempts == 0);
    TEST_CHECK(fake_sdl_renderer.frame_result
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(legacy_begin_calls == 0);
    TEST_CHECK(legacy_vertex_calls == 0);
    TEST_CHECK(legacy_color_calls == 0);
    TEST_CHECK(legacy_blend_calls == 0);
    return 0;
}

static int check_color_mod_parent_paint(void)
{
    const RendererColor transparent_background = {
        0x23, 0x45, 0x67, 0x00
    };
    const RendererColor value_color = {0x89, 0xab, 0xcd, 0xef};
    Uint32 value = 0x89abcdef;
    Uint32 foreground = 0x102030ff;
    Uint32 background = 0x23456700;
    Uint32 zero_background = 0;
    SDL_Rect bounds = {11, 17, 101, 31};
    SDL_Rect swatch;
    ChooserUpdateCapture capture = {0, NULL};
    int allocations_before = live_text_allocations;
    GLWidget *widget = Init_ColorModWidget(
        &value, &foreground, &background,
        capture_chooser_update, &capture);
    ColorModWidget *info;
    IntChooserWidget *red_info;
    IntChooserWidget *green_info;
    IntChooserWidget *blue_info;
    IntChooserWidget *alpha_info;

    TEST_CHECK(widget != NULL);
    info = widget->wid_info;
    TEST_CHECK(info != NULL);
    TEST_CHECK(widget->children == info->redpick);
    TEST_CHECK(info->redpick->next == info->greenpick);
    TEST_CHECK(info->greenpick->next == info->bluepick);
    TEST_CHECK(info->bluepick->next == info->alphapick);
    TEST_CHECK(info->alphapick->next == NULL);
    SetBounds_GLWidget(widget, &bounds);
    TEST_CHECK(info->redpick->bounds.x == 13);
    TEST_CHECK(info->redpick->bounds.y == 17);
    TEST_CHECK(info->redpick->bounds.w == 47);
    TEST_CHECK(info->redpick->bounds.h == 7);
    TEST_CHECK(info->greenpick->bounds.y == 24);
    TEST_CHECK(info->bluepick->bounds.y == 31);
    TEST_CHECK(info->alphapick->bounds.y == 38);

    swatch.x = info->redpick->bounds.x + info->redpick->bounds.w + 16;
    swatch.y = widget->bounds.y + 2;
    swatch.w = widget->bounds.x + widget->bounds.w - 2 - swatch.x;
    swatch.h = widget->bounds.y + widget->bounds.h - 2 - swatch.y;
    TEST_CHECK(swatch.x == 76);
    TEST_CHECK(swatch.y == 19);
    TEST_CHECK(swatch.w == 34);
    TEST_CHECK(swatch.h == 27);

    red_info = info->redpick->wid_info;
    green_info = info->greenpick->wid_info;
    blue_info = info->bluepick->wid_info;
    alpha_info = info->alphapick->wid_info;
    TEST_CHECK(red_info != NULL);
    TEST_CHECK(green_info != NULL);
    TEST_CHECK(blue_info != NULL);
    TEST_CHECK(alpha_info != NULL);
    TEST_CHECK(info->red == 0x89);
    TEST_CHECK(info->green == 0xab);
    TEST_CHECK(info->blue == 0xcd);
    TEST_CHECK(info->alpha == 0xef);

    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_mod_parent_paint(
        1, transparent_background, value_color) == 0);

    info->bgcolor = NULL;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_mod_parent_paint(
        0, transparent_background, value_color) == 0);

    info->bgcolor = &zero_background;
    reset_frame();
    widget->Draw(widget);
    TEST_CHECK(check_successful_color_mod_parent_paint(
        0, transparent_background, value_color) == 0);

    info->bgcolor = &background;
    TEST_CHECK(check_color_mod_later_draw_failure(widget, 2) == 0);
    TEST_CHECK(check_color_mod_later_draw_failure(widget, 3) == 0);
    info->bgcolor = NULL;
    TEST_CHECK(check_color_mod_first_draw_failure(widget) == 0);

    TEST_CHECK(red_info->direction == 0);
    TEST_CHECK(green_info->direction == 0);
    TEST_CHECK(blue_info->direction == 0);
    TEST_CHECK(alpha_info->direction == 0);
    TEST_CHECK(info->red == 0x89);
    TEST_CHECK(info->green == 0xab);
    TEST_CHECK(info->blue == 0xcd);
    TEST_CHECK(info->alpha == 0xef);
    TEST_CHECK(value == 0x89abcdef);
    TEST_CHECK(capture.calls == 0);

    destroy_widget(widget);
    TEST_CHECK(live_text_allocations == allocations_before);
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
    TEST_CHECK(check_bool_chooser_parent_paint() == 0);
    TEST_CHECK(check_int_chooser_parent_paint() == 0);
    TEST_CHECK(check_double_chooser_parent_paint() == 0);
    TEST_CHECK(check_color_chooser_parent_paint() == 0);
    TEST_CHECK(check_color_mod_parent_paint() == 0);
    TEST_CHECK(check_label_scrap_button() == 0);
    TEST_CHECK(live_text_allocations == 0);
    return 0;
}
