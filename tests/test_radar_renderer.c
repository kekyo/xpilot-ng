#include "test_helpers.h"

#include "glwidgets.h"
#include "radar.h"
#include "renderer.h"
#include "sdlrenderer.h"

#include <SDL.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURES 8
#define MAX_SPRITES 8
#define MAX_EVENTS 32
#define FLOAT_TOLERANCE 0.0001f

struct Renderer {
    int frame_active;
};

struct RendererTexture {
    unsigned int identifier;
};

struct SdlRenderer {
    Renderer *frontend;
};

typedef struct FakeTexture {
    RendererTexture handle;
    RendererTextureDesc desc;
    uint8_t *pixels;
    size_t pitch;
    int destroy_count;
} FakeTexture;

typedef struct FakeSprite {
    RendererTexture *texture;
    float left;
    float top;
    float right;
    float bottom;
    float u0;
    float v0;
    float u1;
    float v1;
    RendererColor tint;
} FakeSprite;

typedef enum PaintEvent {
    PAINT_EVENT_SPRITE,
    PAINT_EVENT_FLUSH,
    PAINT_EVENT_LEGACY
} PaintEvent;

static Renderer fake_renderer;
static SdlRenderer fake_sdl_renderer = {&fake_renderer};
static FakeTexture fake_textures[MAX_TEXTURES];
static FakeSprite fake_sprites[MAX_SPRITES];
static PaintEvent paint_events[MAX_EVENTS];
static int texture_create_attempts;
static int texture_create_successes;
static int fail_texture_create_attempt;
static int texture_update_calls;
static int texture_destroy_calls;
static int resource_calls_during_frame;
static int sprite_calls;
static int blend_calls;
static int preserving_flush_calls;
static int paint_event_count;
static int forbidden_legacy_texture_calls;
static xp_option_t *radar_geometry_option;

setup_t *Setup;
int oldServer;
ipos_t selfPos;
short heading;
short nextCheckPoint;
short selfVisible;
instruments_t instruments;
unsigned RadarWidth;
unsigned RadarHeight;
radar_t *radar_ptr;
int num_radar;
int max_radar;
checkpoint_t *checks;
xp_polygon_t *polygons;
int num_polygons;
int max_polygons;
polygon_style_t *polygon_styles;
int num_polygon_styles;
int max_polygon_styles;
double tbl_sin[TABLE_SIZE];
double tbl_cos[TABLE_SIZE];

extern color_t wallRadarColorValue;
extern color_t targetRadarColorValue;
extern color_t decorRadarColorValue;
extern color_t bgRadarColorValue;

/* Test-only read access to atomically published radar resources. */
extern SDL_Surface *Radar_test_surface(void);
extern RendererTexture *Radar_test_texture(void);
extern SDL_Rect Radar_test_bounds(void);

static int float_equal(float actual, float expected)
{
    float difference = actual - expected;

    return difference >= -FLOAT_TOLERANCE
        && difference <= FLOAT_TOLERANCE;
}

static int color_equal(RendererColor actual, RendererColor expected)
{
    return actual.red == expected.red && actual.green == expected.green
        && actual.blue == expected.blue && actual.alpha == expected.alpha;
}

static int rect_equal(SDL_Rect actual, SDL_Rect expected)
{
    return actual.x == expected.x && actual.y == expected.y
        && actual.w == expected.w && actual.h == expected.h;
}

static void record_paint_event(PaintEvent event)
{
    if (paint_event_count < MAX_EVENTS)
        paint_events[paint_event_count++] = event;
}

static FakeTexture *fake_texture_from_handle(RendererTexture *texture)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        if (&fake_textures[index].handle == texture)
            return &fake_textures[index];
    }
    return NULL;
}

static int pixel_equals(const FakeTexture *texture, int x, int y,
                        uint8_t red, uint8_t green, uint8_t blue,
                        uint8_t alpha)
{
    const uint8_t *pixel;

    if (texture == NULL || x < 0 || x >= texture->desc.width
        || y < 0 || y >= texture->desc.height) {
        return 0;
    }
    pixel = texture->pixels + (size_t)y * texture->pitch + (size_t)x * 4;
    return pixel[0] == red && pixel[1] == green
        && pixel[2] == blue && pixel[3] == alpha;
}

RendererColor Renderer_color_from_rgba32(uint32_t rgba)
{
    RendererColor color = {
        (uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16),
        (uint8_t)(rgba >> 8), (uint8_t)rgba
    };

    return color;
}

RendererStatus Renderer_texture_create_with_desc(
    Renderer *renderer, const RendererTextureDesc *desc,
    const uint8_t *rgba_pixels, size_t pitch, RendererTexture **texture)
{
    FakeTexture *record;
    int y;

    texture_create_attempts++;
    if (renderer == NULL || desc == NULL || rgba_pixels == NULL
        || texture == NULL) {
        return RENDERER_STATUS_INVALID_ARGUMENT;
    }
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    if (texture_create_attempts == fail_texture_create_attempt)
        return RENDERER_STATUS_BACKEND_ERROR;
    if (texture_create_successes >= MAX_TEXTURES)
        return RENDERER_STATUS_OUT_OF_MEMORY;

    record = &fake_textures[texture_create_successes];
    memset(record, 0, sizeof(*record));
    record->handle.identifier = (unsigned int)texture_create_successes + 1;
    record->desc = *desc;
    record->pitch = (size_t)desc->width * 4;
    record->pixels = malloc(record->pitch * (size_t)desc->height);
    if (record->pixels == NULL)
        return RENDERER_STATUS_OUT_OF_MEMORY;
    for (y = 0; y < desc->height; y++) {
        memcpy(record->pixels + (size_t)y * record->pitch,
               rgba_pixels + (size_t)y * pitch, record->pitch);
    }
    texture_create_successes++;
    *texture = &record->handle;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_update(Renderer *renderer,
                                       RendererTexture *texture,
                                       RendererRect region,
                                       const uint8_t *rgba_pixels,
                                       size_t pitch)
{
    (void)texture;
    (void)region;
    (void)rgba_pixels;
    (void)pitch;
    texture_update_calls++;
    if (renderer != NULL && renderer->frame_active)
        resource_calls_during_frame++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_texture_destroy(Renderer *renderer,
                                        RendererTexture *texture)
{
    FakeTexture *record = fake_texture_from_handle(texture);

    if (renderer == NULL || record == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (renderer->frame_active) {
        resource_calls_during_frame++;
        return RENDERER_STATUS_INVALID_STATE;
    }
    record->destroy_count++;
    texture_destroy_calls++;
    return record->destroy_count == 1 ? RENDERER_STATUS_OK
                                      : RENDERER_STATUS_INVALID_STATE;
}

RendererStatus Renderer_set_blend(Renderer *renderer,
                                  RendererBlendMode blend)
{
    if (renderer != &fake_renderer || !renderer->frame_active
        || blend != RENDERER_BLEND_ALPHA) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    blend_calls++;
    return RENDERER_STATUS_OK;
}

RendererStatus Renderer_draw_sprite(Renderer *renderer,
                                    RendererTexture *texture,
                                    float left, float top,
                                    float right, float bottom,
                                    float u0, float v0, float u1, float v1,
                                    RendererColor tint)
{
    FakeTexture *record = fake_texture_from_handle(texture);
    FakeSprite *sprite;

    if (renderer != &fake_renderer || !renderer->frame_active
        || record == NULL || record->destroy_count != 0
        || sprite_calls >= MAX_SPRITES) {
        return RENDERER_STATUS_INVALID_STATE;
    }
    sprite = &fake_sprites[sprite_calls++];
    sprite->texture = texture;
    sprite->left = left;
    sprite->top = top;
    sprite->right = right;
    sprite->bottom = bottom;
    sprite->u0 = u0;
    sprite->v0 = v0;
    sprite->u1 = u1;
    sprite->v1 = v1;
    sprite->tint = tint;
    record_paint_event(PAINT_EVENT_SPRITE);
    return RENDERER_STATUS_OK;
}

SdlRenderer *Get_sdl_renderer(void)
{
    return &fake_sdl_renderer;
}

Renderer *Sdl_renderer_frontend(SdlRenderer *renderer)
{
    return renderer == &fake_sdl_renderer ? renderer->frontend : NULL;
}

RendererStatus Sdl_renderer_flush_preserving_legacy(SdlRenderer *renderer)
{
    if (renderer != &fake_sdl_renderer || !fake_renderer.frame_active)
        return RENDERER_STATUS_INVALID_STATE;
    preserving_flush_calls++;
    record_paint_event(PAINT_EVENT_FLUSH);
    return RENDERER_STATUS_OK;
}

GLWidget *Init_EmptyBaseGLWidget(void)
{
    return calloc(1, sizeof(GLWidget));
}

bool AppendGLWidgetList(GLWidget **list, GLWidget *widget)
{
    (void)list;
    (void)widget;
    return true;
}

void PrependGLWidgetList(GLWidget **list, GLWidget *widget)
{
    (void)list;
    (void)widget;
}

bool DelGLWidgetListItem(GLWidget **list, GLWidget *widget)
{
    (void)list;
    (void)widget;
    return true;
}

void Store_option(xp_option_t *option)
{
    if (option != NULL && strcmp(option->name, "radarGeometry") == 0)
        radar_geometry_option = option;
}

xp_option_t *Find_option(const char *name)
{
    return name != NULL && strcmp(name, "radarGeometry") == 0
        ? radar_geometry_option : NULL;
}

bool Set_string_option(xp_option_t *option, const char *value,
                       xp_option_origin_t origin)
{
    (void)origin;
    if (option == NULL || option->str_setfunc == NULL)
        return false;
    return option->str_setfunc(option, value);
}

int Target_alive(int x, int y, double *damage)
{
    (void)x;
    (void)y;
    if (damage != NULL)
        *damage = 0.0;
    return 1;
}

int Check_pos_by_index(int index, int *x, int *y)
{
    (void)index;
    if (x != NULL)
        *x = 0;
    if (y != NULL)
        *y = 0;
    return 0;
}

int filledPolygonRGBA(SDL_Surface *surface, Sint16 *vx, Sint16 *vy,
                      int count, Uint8 red, Uint8 green,
                      Uint8 blue, Uint8 alpha)
{
    (void)surface;
    (void)vx;
    (void)vy;
    (void)count;
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    return 0;
}

void warn(const char *format, ...)
{
    (void)format;
}

void error(const char *format, ...)
{
    (void)format;
}

GLenum GLAPIENTRY glGetError(void)
{
    return GL_NO_ERROR;
}

void GLAPIENTRY glGenTextures(GLsizei count, GLuint *textures)
{
    GLsizei index;

    forbidden_legacy_texture_calls++;
    for (index = 0; index < count; index++)
        textures[index] = (GLuint)(index + 1);
}

void GLAPIENTRY glDeleteTextures(GLsizei count, const GLuint *textures)
{
    (void)count;
    (void)textures;
    forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    (void)texture;
    forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glTexImage2D(GLenum target, GLint level,
                            GLint internal_format, GLsizei width,
                            GLsizei height, GLint border, GLenum format,
                            GLenum type, const GLvoid *pixels)
{
    (void)target;
    (void)level;
    (void)internal_format;
    (void)width;
    (void)height;
    (void)border;
    (void)format;
    (void)type;
    (void)pixels;
    forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glTexParameterf(GLenum target, GLenum name, GLfloat value)
{
    (void)target;
    (void)name;
    (void)value;
    forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glEnable(GLenum capability)
{
    if (capability == GL_TEXTURE_2D)
        forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    if (capability == GL_TEXTURE_2D)
        forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
}

void GLAPIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    (void)red;
    (void)green;
    (void)blue;
}

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green,
                          GLubyte blue, GLubyte alpha)
{
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
}

void GLAPIENTRY glBegin(GLenum mode)
{
    (void)mode;
    record_paint_event(PAINT_EVENT_LEGACY);
}

void GLAPIENTRY glTexCoord2f(GLfloat u, GLfloat v)
{
    (void)u;
    (void)v;
    forbidden_legacy_texture_calls++;
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
{
    (void)x;
    (void)y;
}

void GLAPIENTRY glEnd(void)
{
}

static void reset_paint_records(void)
{
    memset(fake_sprites, 0, sizeof(fake_sprites));
    memset(paint_events, 0, sizeof(paint_events));
    sprite_calls = 0;
    blend_calls = 0;
    preserving_flush_calls = 0;
    paint_event_count = 0;
}

static int set_radar_geometry(const char *geometry)
{
    if (radar_geometry_option == NULL
        || radar_geometry_option->str_setfunc == NULL) {
        return 0;
    }
    return radar_geometry_option->str_setfunc(
        radar_geometry_option, geometry) ? 1 : 0;
}

static int check_sprite(const FakeSprite *sprite,
                        RendererTexture *texture,
                        float left, float top, float right, float bottom,
                        float u0, float v0, float u1, float v1)
{
    const RendererColor white = {255, 255, 255, 255};

    TEST_CHECK(sprite->texture == texture);
    TEST_CHECK(float_equal(sprite->left, left));
    TEST_CHECK(float_equal(sprite->top, top));
    TEST_CHECK(float_equal(sprite->right, right));
    TEST_CHECK(float_equal(sprite->bottom, bottom));
    TEST_CHECK(float_equal(sprite->u0, u0));
    TEST_CHECK(float_equal(sprite->v0, v0));
    TEST_CHECK(float_equal(sprite->u1, u1));
    TEST_CHECK(float_equal(sprite->v1, v1));
    TEST_CHECK(color_equal(sprite->tint, white));
    return 0;
}

static int check_initial_prepare(GLWidget **widget_out,
                                 RendererTexture **texture_out)
{
    SDL_Surface *surface;
    RendererTexture *texture;
    FakeTexture *record;
    SDL_Rect expected_bounds = {10, 20, 8, 5};

    Store_radar_options();
    TEST_CHECK(radar_geometry_option != NULL);
    TEST_CHECK(set_radar_geometry("8x8+10+20"));

    *widget_out = Init_RadarWidget();
    TEST_CHECK(*widget_out != NULL);
    surface = Radar_test_surface();
    TEST_CHECK(surface != NULL);
    TEST_CHECK(surface->format->format == SDL_PIXELFORMAT_RGBA32);
    TEST_CHECK(Radar_test_texture() == NULL);
    TEST_CHECK(rect_equal(Radar_test_bounds(), expected_bounds));
    TEST_CHECK(texture_create_attempts == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);

    TEST_CHECK(Radar_prepare(&fake_renderer) == 0);
    texture = Radar_test_texture();
    TEST_CHECK(texture != NULL);
    TEST_CHECK(Radar_test_surface() == surface);
    TEST_CHECK(texture_create_successes == 1);
    TEST_CHECK(texture_destroy_calls == 0);
    record = fake_texture_from_handle(texture);
    TEST_CHECK(record != NULL);
    TEST_CHECK(record->desc.width == surface->w);
    TEST_CHECK(record->desc.height == surface->h);
    TEST_CHECK(record->desc.filter == RENDERER_TEXTURE_FILTER_NEAREST);
    TEST_CHECK(record->desc.wrap == RENDERER_TEXTURE_WRAP_CLAMP);
    TEST_CHECK(surface->pitch >= surface->w * 4);
    TEST_CHECK(pixel_equals(record, 0, 0, 0x00, 0x00, 0x00, 0xa0));
    TEST_CHECK(pixel_equals(record, 0, 1, 0x00, 0x00, 0x00, 0xa0));
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);
    *texture_out = texture;
    return 0;
}

static int check_atomic_update_and_resize(GLWidget *widget,
                                          RendererTexture *old_texture,
                                          RendererTexture **texture_out)
{
    SDL_Surface *old_surface = Radar_test_surface();
    SDL_Rect old_bounds = Radar_test_bounds();
    SDL_Rect expected_bounds = {30, 40, 12, 5};
    int attempts_before = texture_create_attempts;
    int destroys_before = texture_destroy_calls;
    RendererTexture *texture;
    FakeTexture *record;

    fake_renderer.frame_active = 1;
    bgRadarColorValue = 0x00112233;
    Radar_update();
    TEST_CHECK(set_radar_geometry("12x8+30+40"));
    TEST_CHECK(texture_create_attempts == attempts_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);
    TEST_CHECK(Radar_test_surface() == old_surface);
    TEST_CHECK(Radar_test_texture() == old_texture);
    TEST_CHECK(rect_equal(Radar_test_bounds(), old_bounds));
    fake_renderer.frame_active = 0;

    fail_texture_create_attempt = texture_create_attempts + 1;
    TEST_CHECK(Radar_prepare(&fake_renderer) == -1);
    TEST_CHECK(Radar_test_surface() == old_surface);
    TEST_CHECK(Radar_test_texture() == old_texture);
    TEST_CHECK(rect_equal(Radar_test_bounds(), old_bounds));
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 0);
    TEST_CHECK(texture_destroy_calls == destroys_before);

    fail_texture_create_attempt = 0;
    TEST_CHECK(Radar_prepare(&fake_renderer) == 0);
    texture = Radar_test_texture();
    TEST_CHECK(texture != NULL && texture != old_texture);
    TEST_CHECK(Radar_test_surface() != old_surface);
    TEST_CHECK(rect_equal(Radar_test_bounds(), expected_bounds));
    TEST_CHECK(widget->bounds.x == expected_bounds.x);
    TEST_CHECK(widget->bounds.y == expected_bounds.y);
    TEST_CHECK(widget->bounds.w == expected_bounds.w + 1);
    TEST_CHECK(widget->bounds.h == expected_bounds.h + 1);
    TEST_CHECK(fake_texture_from_handle(old_texture)->destroy_count == 1);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    record = fake_texture_from_handle(texture);
    TEST_CHECK(pixel_equals(record, 0, 0, 0x11, 0x22, 0x33, 0xff));
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);
    *texture_out = texture;
    return 0;
}

static int check_non_sliding_paint(GLWidget *widget,
                                    RendererTexture *texture)
{
    SDL_Surface *surface = Radar_test_surface();
    SDL_Rect bounds = Radar_test_bounds();
    int resource_calls_before = resource_calls_during_frame;

    instruments.slidingRadar = false;
    reset_paint_records();
    fake_renderer.frame_active = 1;
    widget->Draw(widget);
    fake_renderer.frame_active = 0;

    TEST_CHECK(sprite_calls == 1);
    TEST_CHECK(check_sprite(
        &fake_sprites[0], texture,
        (float)bounds.x, (float)bounds.y,
        (float)(bounds.x + bounds.w), (float)(bounds.y + bounds.h),
        0.0f, 0.0f,
        (float)bounds.w / surface->w,
        (float)bounds.h / surface->h) == 0);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(preserving_flush_calls == 1);
    TEST_CHECK(paint_event_count >= 3);
    TEST_CHECK(paint_events[0] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[1] == PAINT_EVENT_FLUSH);
    TEST_CHECK(paint_events[2] == PAINT_EVENT_LEGACY);
    TEST_CHECK(resource_calls_during_frame == resource_calls_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);
    return 0;
}

static int check_sliding_paint_order_and_uv(GLWidget *widget,
                                            RendererTexture *texture)
{
    SDL_Surface *surface = Radar_test_surface();
    SDL_Rect bounds = Radar_test_bounds();
    float u_split = 9.0f / surface->w;
    float v_split = 1.0f / surface->h;
    float u_end = 12.0f / surface->w;
    float v_end = 5.0f / surface->h;
    int index;
    int resource_calls_before = resource_calls_during_frame;

    instruments.slidingRadar = true;
    selfPos.x = 30;
    selfPos.y = 20;
    reset_paint_records();
    fake_renderer.frame_active = 1;
    widget->Draw(widget);
    fake_renderer.frame_active = 0;

    TEST_CHECK(sprite_calls == 4);
    TEST_CHECK(check_sprite(&fake_sprites[0], texture,
                            33.0f, 44.0f, 42.0f, 45.0f,
                            0.0f, 0.0f, u_split, v_split) == 0);
    TEST_CHECK(check_sprite(&fake_sprites[1], texture,
                            30.0f, 44.0f, 33.0f, 45.0f,
                            u_split, 0.0f, u_end, v_split) == 0);
    TEST_CHECK(check_sprite(&fake_sprites[2], texture,
                            33.0f, 40.0f, 42.0f, 44.0f,
                            0.0f, v_split, u_split, v_end) == 0);
    TEST_CHECK(check_sprite(&fake_sprites[3], texture,
                            30.0f, 40.0f, 33.0f, 44.0f,
                            u_split, v_split, u_end, v_end) == 0);
    TEST_CHECK(blend_calls == 1);
    TEST_CHECK(preserving_flush_calls == 1);
    TEST_CHECK(paint_event_count >= 6);
    for (index = 0; index < 4; index++)
        TEST_CHECK(paint_events[index] == PAINT_EVENT_SPRITE);
    TEST_CHECK(paint_events[4] == PAINT_EVENT_FLUSH);
    TEST_CHECK(paint_events[5] == PAINT_EVENT_LEGACY);
    TEST_CHECK(resource_calls_during_frame == resource_calls_before);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);

    selfPos.x = 60;
    selfPos.y = 40;
    reset_paint_records();
    fake_renderer.frame_active = 1;
    widget->Draw(widget);
    fake_renderer.frame_active = 0;
    TEST_CHECK(sprite_calls == 1);
    TEST_CHECK(check_sprite(&fake_sprites[0], texture,
                            (float)bounds.x, (float)bounds.y,
                            (float)(bounds.x + bounds.w),
                            (float)(bounds.y + bounds.h),
                            0.0f, 0.0f, u_end, v_end) == 0);
    TEST_CHECK(preserving_flush_calls == 1);
    TEST_CHECK(resource_calls_during_frame == resource_calls_before);
    return 0;
}

static int check_cleanup_is_exact_and_idempotent(GLWidget *widget,
                                                  RendererTexture *texture)
{
    int destroys_before = texture_destroy_calls;

    widget->Close(widget);
    TEST_CHECK(fake_texture_from_handle(texture)->destroy_count == 1);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(Radar_test_surface() == NULL);
    TEST_CHECK(Radar_test_texture() == NULL);

    widget->Close(widget);
    TEST_CHECK(texture_destroy_calls == destroys_before + 1);
    TEST_CHECK(resource_calls_during_frame == 0);
    TEST_CHECK(texture_update_calls == 0);
    TEST_CHECK(forbidden_legacy_texture_calls == 0);
    return 0;
}

static void release_fake_pixels(void)
{
    int index;

    for (index = 0; index < texture_create_successes; index++) {
        free(fake_textures[index].pixels);
        fake_textures[index].pixels = NULL;
    }
}

int main(void)
{
    setup_t fixture_setup;
    GLWidget *widget = NULL;
    RendererTexture *initial_texture = NULL;
    RendererTexture *resized_texture = NULL;
    int result = 1;

    memset(&fixture_setup, 0, sizeof(fixture_setup));
    fixture_setup.x = 2;
    fixture_setup.y = 2;
    fixture_setup.width = 120;
    fixture_setup.height = 80;
    memset(fixture_setup.map_data, SETUP_SPACE,
           sizeof(fixture_setup.map_data));
    Setup = &fixture_setup;
    oldServer = 1;
    RadarWidth = 120;
    RadarHeight = 80;
    bgRadarColorValue = 0xa00000ff;
    wallRadarColorValue = 0x00445566;
    targetRadarColorValue = 0x00778899;
    decorRadarColorValue = 0x00aabbcc;

    if (check_initial_prepare(&widget, &initial_texture) != 0)
        goto cleanup;
    if (check_atomic_update_and_resize(
            widget, initial_texture, &resized_texture) != 0) {
        goto cleanup;
    }
    if (check_non_sliding_paint(widget, resized_texture) != 0)
        goto cleanup;
    if (check_sliding_paint_order_and_uv(widget, resized_texture) != 0)
        goto cleanup;
    if (check_cleanup_is_exact_and_idempotent(
            widget, resized_texture) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    fake_renderer.frame_active = 0;
    if (result != 0 && widget != NULL && Radar_test_surface() != NULL)
        widget->Close(widget);
    free(widget);
    release_fake_pixels();
    return result;
}
