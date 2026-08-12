#include "test_helpers.h"

#include "xpclient_sdl.h"
#include "guimap.h"
#include "guiobjects.h"
#include "gl_diagnostics.h"
#include "images.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define BASELINE_SIZE 12
#define PIXEL_COMPONENTS 4

typedef void (APIENTRYP gen_framebuffers_fn)(GLsizei, GLuint *);
typedef void (APIENTRYP bind_framebuffer_fn)(GLenum, GLuint);
typedef void (APIENTRYP framebuffer_texture_2d_fn)(GLenum, GLenum, GLenum,
                                                   GLuint, GLint);
typedef GLenum (APIENTRYP check_framebuffer_status_fn)(GLenum);
typedef void (APIENTRYP delete_framebuffers_fn)(GLsizei, const GLuint *);

typedef struct framebuffer_api {
    gen_framebuffers_fn gen;
    bind_framebuffer_fn bind;
    framebuffer_texture_2d_fn attach_texture;
    check_framebuffer_status_fn check_status;
    delete_framebuffers_fn destroy;
} framebuffer_api_t;

xp_polygon_t *polygons;
polygon_style_t *polygon_styles;
int num_polygons;

static int failure_count;

extern int Gui_init(void);
extern void Gui_cleanup(void);
#ifdef XPILOT_GL_TEST_HOOKS
extern void Gui_test_get_display_lists(GLuint *asteroid_list,
                                       GLuint *polygon_fill_list_base,
                                       GLuint *polygon_edge_list_base);
#endif

#define CHECK_CONTINUE(condition)                                           \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: check failed: %s\n",                 \
                    __FILE__, __LINE__, #condition);                        \
            failure_count++;                                                \
        }                                                                   \
    } while (0)

void error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

void warn(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

int xpprintf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = vfprintf(stdout, format, args);
    va_end(args);
    return result;
}

image_t *Image_get_texture(int index)
{
    (void)index;
    return NULL;
}

static void *load_gl_function(const char *core_name, const char *ext_name)
{
    void *function = SDL_GL_GetProcAddress(core_name);

    if (function == NULL)
        function = SDL_GL_GetProcAddress(ext_name);
    return function;
}

static int load_framebuffer_api(framebuffer_api_t *api)
{
    memset(api, 0, sizeof(*api));
    api->gen = (gen_framebuffers_fn)load_gl_function(
        "glGenFramebuffers", "glGenFramebuffersEXT");
    api->bind = (bind_framebuffer_fn)load_gl_function(
        "glBindFramebuffer", "glBindFramebufferEXT");
    api->attach_texture = (framebuffer_texture_2d_fn)load_gl_function(
        "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    api->check_status = (check_framebuffer_status_fn)load_gl_function(
        "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    api->destroy = (delete_framebuffers_fn)load_gl_function(
        "glDeleteFramebuffers", "glDeleteFramebuffersEXT");

    return api->gen != NULL && api->bind != NULL
        && api->attach_texture != NULL && api->check_status != NULL
        && api->destroy != NULL;
}

static unsigned int drain_gl_errors(void)
{
    unsigned int count = 0;

    while (glGetError() != GL_NO_ERROR)
        count++;
    return count;
}

static int component_near(GLubyte actual, int expected, int tolerance)
{
    int difference = (int)actual - expected;

    if (difference < 0)
        difference = -difference;
    return difference <= tolerance;
}

static void check_pixel(const GLubyte *pixels, int x, int y,
                        int red, int green, int blue, int tolerance)
{
    const GLubyte *pixel = pixels
        + (y * BASELINE_SIZE + x) * PIXEL_COMPONENTS;

    CHECK_CONTINUE(component_near(pixel[0], red, tolerance));
    CHECK_CONTINUE(component_near(pixel[1], green, tolerance));
    CHECK_CONTINUE(component_near(pixel[2], blue, tolerance));
}

static void draw_quad(GLfloat left, GLfloat bottom,
                      GLfloat right, GLfloat top)
{
    glBegin(GL_QUADS);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
    glEnd();
}

static void check_procedural_baseline(void)
{
    framebuffer_api_t api;
    GLuint framebuffer = 0;
    GLuint color_texture = 0;
    GLuint sample_texture = 0;
    GLubyte sample_color[] = {255, 255, 0, 255};
    GLubyte pixels[BASELINE_SIZE * BASELINE_SIZE * PIXEL_COMPONENTS];
    int framebuffer_api_loaded;

    framebuffer_api_loaded = load_framebuffer_api(&api);
    CHECK_CONTINUE(framebuffer_api_loaded);
    if (!framebuffer_api_loaded)
        return;

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BASELINE_SIZE, BASELINE_SIZE,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    api.gen(1, &framebuffer);
    api.bind(GL_FRAMEBUFFER, framebuffer);
    api.attach_texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                       color_texture, 0);
    CHECK_CONTINUE(api.check_status(GL_FRAMEBUFFER)
                   == GL_FRAMEBUFFER_COMPLETE);

    glViewport(0, 0, BASELINE_SIZE, BASELINE_SIZE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, BASELINE_SIZE, 0.0, BASELINE_SIZE, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4ub(255, 0, 0, 255);
    draw_quad(0.0f, 0.0f, 4.0f, 4.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(0, 0, 255, 128);
    draw_quad(2.0f, 0.0f, 6.0f, 4.0f);
    glDisable(GL_BLEND);

    glEnable(GL_SCISSOR_TEST);
    glScissor(8, 0, 2, 2);
    glColor4ub(0, 255, 0, 255);
    draw_quad(0.0f, 0.0f, BASELINE_SIZE, BASELINE_SIZE);
    glDisable(GL_SCISSOR_TEST);

    glGenTextures(1, &sample_texture);
    glBindTexture(GL_TEXTURE_2D, sample_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, sample_color);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 8.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(4.0f, 8.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(4.0f, 12.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 12.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glColor4ub(255, 255, 255, 255);
    glBegin(GL_TRIANGLES);
    glVertex2f(6.0f, 8.0f);
    glVertex2f(12.0f, 8.0f);
    glVertex2f(9.0f, 12.0f);
    glEnd();

    glFinish();
    memset(pixels, 0, sizeof(pixels));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, BASELINE_SIZE, BASELINE_SIZE,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    check_pixel(pixels, 1, 1, 255, 0, 0, 0);
    check_pixel(pixels, 3, 1, 127, 0, 128, 2);
    check_pixel(pixels, 5, 1, 0, 0, 128, 2);
    check_pixel(pixels, 8, 1, 0, 255, 0, 0);
    check_pixel(pixels, 7, 1, 0, 0, 0, 0);
    check_pixel(pixels, 8, 3, 0, 0, 0, 0);
    check_pixel(pixels, 1, 9, 255, 255, 0, 0);
    check_pixel(pixels, 9, 9, 255, 255, 255, 0);
    check_pixel(pixels, 11, 6, 0, 0, 0, 0);
    CHECK_CONTINUE(drain_gl_errors() == 0);

    api.bind(GL_FRAMEBUFFER, 0);
    api.destroy(1, &framebuffer);
    glDeleteTextures(1, &sample_texture);
    glDeleteTextures(1, &color_texture);
}

static void check_known_legacy_draw_paths(void)
{
    drain_gl_errors();
    Gui_paint_border(0, 0, 1, 1);
    CHECK_CONTINUE(drain_gl_errors() == 0);
}

static void check_gui_display_list_lifecycle(void)
{
    static ipos_t points[] = {
        {0, 0},
        {8, 0},
        {0, 8},
        {-8, 0},
    };
    static xp_polygon_t test_polygons[] = {
        {
            points,
            (int)(sizeof(points) / sizeof(points[0])),
            {0, 0, 8, 8},
            NULL,
            0,
        },
    };
    static polygon_style_t test_polygon_styles[] = {
        {0, 0, 0, 0, 0},
    };
    GLuint asteroid_list = 0;
    GLuint polygon_fill_list_base = 0;
    GLuint polygon_edge_list_base = 0;

    polygons = test_polygons;
    polygon_styles = test_polygon_styles;
    num_polygons = (int)(sizeof(test_polygons) / sizeof(test_polygons[0]));

    drain_gl_errors();
    CHECK_CONTINUE(Gui_init() == 0);
    CHECK_CONTINUE(drain_gl_errors() == 0);
#ifdef XPILOT_GL_TEST_HOOKS
    Gui_test_get_display_lists(&asteroid_list, &polygon_fill_list_base,
                               &polygon_edge_list_base);
#endif
    CHECK_CONTINUE(asteroid_list != 0);
    CHECK_CONTINUE(polygon_fill_list_base != 0);
    CHECK_CONTINUE(polygon_edge_list_base != 0);
    CHECK_CONTINUE(glIsList(asteroid_list) == GL_TRUE);
    CHECK_CONTINUE(glIsList(polygon_fill_list_base) == GL_TRUE);
    CHECK_CONTINUE(glIsList(polygon_edge_list_base) == GL_TRUE);

    Gui_paint_asteroid(6, 6, 0, 0, 1);
    CHECK_CONTINUE(drain_gl_errors() == 0);
    Gui_cleanup();
    CHECK_CONTINUE(glIsList(asteroid_list) == GL_FALSE);
    CHECK_CONTINUE(glIsList(polygon_fill_list_base) == GL_FALSE);
    CHECK_CONTINUE(glIsList(polygon_edge_list_base) == GL_FALSE);
    Gui_cleanup();
    CHECK_CONTINUE(drain_gl_errors() == 0);
}

static void check_diagnostics(void)
{
    Gl_diagnostics_reset();
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 0);

    glEnable(GL_TEXTURE_WIDTH);
    CHECK_CONTINUE(Gl_diagnostics_check("unit-injected") == 1);
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 1);
    CHECK_CONTINUE(glGetError() == GL_NO_ERROR);
    CHECK_CONTINUE(Gl_diagnostics_check("unit-empty") == 0);
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 1);

    Gl_diagnostics_reset();
    CHECK_CONTINUE(Gl_diagnostics_total_errors() == 0);
}

int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL video initialization failed: %s\n",
                SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("XPilot legacy OpenGL test",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == NULL) {
        fprintf(stderr, "SDL OpenGL window creation failed: %s\n",
                SDL_GetError());
        SDL_Quit();
        return 1;
    }
    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SDL OpenGL context creation failed: %s\n",
                SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    CHECK_CONTINUE(glGetString(GL_VERSION) != NULL);
    check_procedural_baseline();
    check_known_legacy_draw_paths();
    check_gui_display_list_lifecycle();
    check_diagnostics();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (failure_count != 0) {
        fprintf(stderr, "%d legacy OpenGL checks failed\n", failure_count);
        return 1;
    }
    return 0;
}
