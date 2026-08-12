#include "test_helpers.h"

#include "glwidgets.h"

#include <GL/gl.h>

#include <string.h>

#define MAX_EVENTS 16
#define MAX_GL_SCISSORS 16

typedef enum TraversalEventKind {
    TRAVERSAL_EVENT_SET_SCISSOR,
    TRAVERSAL_EVENT_DISABLE_SCISSOR,
    TRAVERSAL_EVENT_DRAW
} TraversalEventKind;

typedef struct TraversalEvent {
    TraversalEventKind kind;
    RendererRect scissor;
    int widget_id;
} TraversalEvent;

typedef struct GlScissorCall {
    int x;
    int y;
    int width;
    int height;
} GlScissorCall;

static TraversalEvent events[MAX_EVENTS];
static GlScissorCall gl_scissors[MAX_GL_SCISSORS];
static int event_count;
static int gl_scissor_count;
static SdlUiDrawState *active_draw_state;
static GLWidget *failing_widget;
static int fake_renderer_storage;
static int parent_id = 1;
static int child_id = 2;
static int skipped_id = 3;

unsigned draw_width = 100;
unsigned draw_height = 80;

static int rect_equal(RendererRect left, RendererRect right)
{
    return left.x == right.x && left.y == right.y
        && left.width == right.width && left.height == right.height;
}

static void reset_capture(void)
{
    memset(events, 0, sizeof(events));
    memset(gl_scissors, 0, sizeof(gl_scissors));
    event_count = 0;
    gl_scissor_count = 0;
    active_draw_state = NULL;
    failing_widget = NULL;
}

static void record_event(TraversalEventKind kind,
                         const RendererRect *scissor, int widget_id)
{
    TraversalEvent *event;

    if (event_count >= MAX_EVENTS)
        return;
    event = &events[event_count++];
    event->kind = kind;
    if (scissor != NULL)
        event->scissor = *scissor;
    event->widget_id = widget_id;
}

RendererStatus Sdl_renderer_set_logical_scissor(
    SdlRenderer *renderer, const RendererRect *scissor)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    record_event(scissor != NULL
                     ? TRAVERSAL_EVENT_SET_SCISSOR
                     : TRAVERSAL_EVENT_DISABLE_SCISSOR,
                 scissor, 0);
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    return renderer != NULL ? RENDERER_STATUS_OK
                            : RENDERER_STATUS_INVALID_ARGUMENT;
}

RendererStatus Sdl_ui_draw_state_status(const SdlUiDrawState *state)
{
    return state != NULL
        ? state->status : RENDERER_STATUS_INVALID_ARGUMENT;
}

void GLAPIENTRY glEnable(GLenum capability)
{
    (void)capability;
}

void GLAPIENTRY glDisable(GLenum capability)
{
    (void)capability;
}

void GLAPIENTRY glBlendFunc(GLenum source, GLenum destination)
{
    (void)source;
    (void)destination;
}

void GLAPIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GlScissorCall *call;

    if (gl_scissor_count >= MAX_GL_SCISSORS)
        return;
    call = &gl_scissors[gl_scissor_count++];
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
}

static void draw_widget(GLWidget *widget)
{
    int id = *(int *)widget->wid_info;

    record_event(TRAVERSAL_EVENT_DRAW, NULL, id);
    if (widget == failing_widget && active_draw_state != NULL)
        active_draw_state->status = RENDERER_STATUS_BACKEND_ERROR;
}

static void init_widget(GLWidget *widget, int *id,
                        int x, int y, int width, int height)
{
    memset(widget, 0, sizeof(*widget));
    widget->wid_info = id;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.w = width;
    widget->bounds.h = height;
    widget->Draw = draw_widget;
}

static int check_set_event(int index, RendererRect expected)
{
    TEST_CHECK(index >= 0 && index < event_count);
    TEST_CHECK(events[index].kind == TRAVERSAL_EVENT_SET_SCISSOR);
    TEST_CHECK(rect_equal(events[index].scissor, expected));
    return 0;
}

static int check_draw_event(int index, int expected_id)
{
    TEST_CHECK(index >= 0 && index < event_count);
    TEST_CHECK(events[index].kind == TRAVERSAL_EVENT_DRAW);
    TEST_CHECK(events[index].widget_id == expected_id);
    return 0;
}

static int check_gl_scissor(int index, int x, int y,
                            int width, int height)
{
    const GlScissorCall *call;

    TEST_CHECK(index >= 0 && index < gl_scissor_count);
    call = &gl_scissors[index];
    TEST_CHECK(call->x == x);
    TEST_CHECK(call->y == y);
    TEST_CHECK(call->width == width);
    TEST_CHECK(call->height == height);
    return 0;
}

static int check_nested_clip_sequence(void)
{
    const RendererRect parent_draw_clip = {5, 7, 21, 16};
    const RendererRect child_draw_clip = {8, 10, 10, 7};
    const RendererRect parent_restore_clip = {5, 7, 20, 15};
    const RendererRect root_restore_clip = {2, 3, 40, 30};

    TEST_CHECK(event_count == 7);
    TEST_CHECK(check_set_event(0, parent_draw_clip) == 0);
    TEST_CHECK(check_draw_event(1, parent_id) == 0);
    TEST_CHECK(check_set_event(2, child_draw_clip) == 0);
    TEST_CHECK(check_draw_event(3, child_id) == 0);
    TEST_CHECK(check_set_event(4, parent_restore_clip) == 0);
    TEST_CHECK(check_set_event(5, root_restore_clip) == 0);
    TEST_CHECK(events[6].kind == TRAVERSAL_EVENT_DISABLE_SCISSOR);

    /* Direct legacy GL keeps its historical one-pixel expansion for a
     * widget draw, but restores an inherited parent without expansion. */
    TEST_CHECK(gl_scissor_count == 4);
    TEST_CHECK(check_gl_scissor(0, 5, 58, 21, 16) == 0);
    TEST_CHECK(check_gl_scissor(1, 8, 64, 10, 7) == 0);
    TEST_CHECK(check_gl_scissor(2, 5, 58, 20, 15) == 0);
    TEST_CHECK(check_gl_scissor(3, 2, 47, 40, 30) == 0);
    return 0;
}

static int check_success_restores_nested_parents(void)
{
    GLWidget parent;
    GLWidget child;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&parent, &parent_id, 5, 7, 20, 15);
    init_widget(&child, &child_id, 8, 10, 9, 6);
    parent.children = &child;
    reset_capture();
    active_draw_state = &state;

    TEST_CHECK(DrawGLWidgetsi_checked(
                   &parent, 2, 3, 40, 30, renderer, &state)
               == RENDERER_STATUS_OK);
    TEST_CHECK(check_nested_clip_sequence() == 0);
    return 0;
}

static int check_failure_restores_before_unwinding(void)
{
    GLWidget parent;
    GLWidget child;
    GLWidget skipped;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&parent, &parent_id, 5, 7, 20, 15);
    init_widget(&child, &child_id, 8, 10, 9, 6);
    init_widget(&skipped, &skipped_id, 9, 11, 5, 4);
    parent.children = &child;
    child.next = &skipped;
    reset_capture();
    active_draw_state = &state;
    failing_widget = &child;

    TEST_CHECK(DrawGLWidgetsi_checked(
                   &parent, 2, 3, 40, 30, renderer, &state)
               == RENDERER_STATUS_BACKEND_ERROR);
    /* A failed renderer flush locks semantic state until end-frame retry, so
     * no further semantic state command is submitted while unwinding. The
     * direct legacy clip is still restored for the caller. */
    TEST_CHECK(event_count == 4);
    TEST_CHECK(check_set_event(0, (RendererRect){5, 7, 21, 16}) == 0);
    TEST_CHECK(check_draw_event(1, parent_id) == 0);
    TEST_CHECK(check_set_event(2, (RendererRect){8, 10, 10, 7}) == 0);
    TEST_CHECK(check_draw_event(3, child_id) == 0);
    TEST_CHECK(gl_scissor_count == 4);
    TEST_CHECK(check_gl_scissor(0, 5, 58, 21, 16) == 0);
    TEST_CHECK(check_gl_scissor(1, 8, 64, 10, 7) == 0);
    TEST_CHECK(check_gl_scissor(2, 5, 58, 20, 15) == 0);
    TEST_CHECK(check_gl_scissor(3, 2, 47, 40, 30) == 0);
    return 0;
}

static int check_checked_requires_renderer_but_legacy_does_not(void)
{
    GLWidget parent;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};

    init_widget(&parent, &parent_id, 5, 7, 20, 15);
    reset_capture();
    active_draw_state = &state;
    TEST_CHECK(DrawGLWidgetsi_checked(
                   &parent, 2, 3, 40, 30, NULL, &state)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(event_count == 0);
    TEST_CHECK(gl_scissor_count == 0);

    reset_capture();
    DrawGLWidgetsi(&parent, 2, 3, 40, 30);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(check_draw_event(0, parent_id) == 0);
    TEST_CHECK(gl_scissor_count == 2);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_success_restores_nested_parents() == 0);
    TEST_CHECK(check_failure_restores_before_unwinding() == 0);
    TEST_CHECK(check_checked_requires_renderer_but_legacy_does_not() == 0);
    return 0;
}
