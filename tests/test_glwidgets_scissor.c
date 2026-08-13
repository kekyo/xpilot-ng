#include "test_helpers.h"

#include "glwidgets.h"

#include <string.h>

#define MAX_EVENTS 16
#define MAX_ORDERED_EVENTS 32

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

typedef enum OrderedEventKind {
    ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
    ORDERED_EVENT_DISABLE_SEMANTIC_SCISSOR,
    ORDERED_EVENT_DRAW
} OrderedEventKind;

typedef struct OrderedEvent {
    OrderedEventKind kind;
    RendererRect scissor;
    int widget_id;
} OrderedEvent;

static TraversalEvent events[MAX_EVENTS];
static OrderedEvent ordered_events[MAX_ORDERED_EVENTS];
static int event_count;
static int ordered_event_count;
static int semantic_scissor_call_count;
static int failing_semantic_scissor_call;
static RendererStatus semantic_scissor_failure;
static RendererStatus fake_frame_result;
static RendererStatus tracked_frame_statuses[MAX_EVENTS];
static int frame_result_track_count;
static SdlUiDrawState *active_draw_state;
static GLWidget *failing_widget;
static GLWidget *frame_failing_widget;
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
    memset(ordered_events, 0, sizeof(ordered_events));
    event_count = 0;
    ordered_event_count = 0;
    semantic_scissor_call_count = 0;
    failing_semantic_scissor_call = 0;
    semantic_scissor_failure = RENDERER_STATUS_OK;
    fake_frame_result = RENDERER_STATUS_OK;
    memset(tracked_frame_statuses, 0, sizeof(tracked_frame_statuses));
    frame_result_track_count = 0;
    active_draw_state = NULL;
    failing_widget = NULL;
    frame_failing_widget = NULL;
}

static void record_ordered_event(OrderedEventKind kind,
                                 const RendererRect *scissor,
                                 int widget_id)
{
    OrderedEvent *event;

    if (ordered_event_count >= MAX_ORDERED_EVENTS)
        return;
    event = &ordered_events[ordered_event_count++];
    event->kind = kind;
    if (scissor != NULL)
        event->scissor = *scissor;
    event->widget_id = widget_id;
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
    record_ordered_event(
        scissor != NULL
            ? ORDERED_EVENT_SET_SEMANTIC_SCISSOR
            : ORDERED_EVENT_DISABLE_SEMANTIC_SCISSOR,
        scissor, 0);
    semantic_scissor_call_count++;
    if (semantic_scissor_call_count == failing_semantic_scissor_call)
        return semantic_scissor_failure;
    return RENDERER_STATUS_OK;
}

RendererStatus Sdl_renderer_track_frame_result(
    SdlRenderer *renderer, RendererStatus status)
{
    if (renderer == NULL)
        return RENDERER_STATUS_INVALID_ARGUMENT;
    if (frame_result_track_count < MAX_EVENTS) {
        tracked_frame_statuses[frame_result_track_count] = status;
    }
    frame_result_track_count++;
    if (fake_frame_result == RENDERER_STATUS_OK
        && status != RENDERER_STATUS_OK) {
        fake_frame_result = status;
    }
    return fake_frame_result;
}

RendererStatus Sdl_renderer_frame_result(const SdlRenderer *renderer)
{
    return renderer != NULL ? fake_frame_result
                            : RENDERER_STATUS_INVALID_ARGUMENT;
}

RendererStatus Sdl_ui_draw_state_status(const SdlUiDrawState *state)
{
    return state != NULL
        ? state->status : RENDERER_STATUS_INVALID_ARGUMENT;
}

static void draw_widget(GLWidget *widget)
{
    int id = *(int *)widget->wid_info;

    record_event(TRAVERSAL_EVENT_DRAW, NULL, id);
    record_ordered_event(ORDERED_EVENT_DRAW, NULL, id);
    if (widget == failing_widget && active_draw_state != NULL)
        active_draw_state->status = RENDERER_STATUS_BACKEND_ERROR;
    if (widget == frame_failing_widget)
        fake_frame_result = RENDERER_STATUS_INVALID_STATE;
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

static int check_ordered_event(int index, OrderedEventKind expected_kind,
                               RendererRect expected_scissor,
                               int expected_widget_id)
{
    const OrderedEvent *event;

    TEST_CHECK(index >= 0 && index < ordered_event_count);
    event = &ordered_events[index];
    TEST_CHECK(event->kind == expected_kind);
    if (expected_kind == ORDERED_EVENT_SET_SEMANTIC_SCISSOR)
        TEST_CHECK(rect_equal(event->scissor, expected_scissor));
    if (expected_kind == ORDERED_EVENT_DRAW)
        TEST_CHECK(event->widget_id == expected_widget_id);
    return 0;
}

static int check_tracked_frame_status(int index,
                                      RendererStatus expected)
{
    TEST_CHECK(index >= 0 && index < frame_result_track_count);
    TEST_CHECK(index < MAX_EVENTS);
    TEST_CHECK(tracked_frame_statuses[index] == expected);
    return 0;
}

static int check_ok_scissors_were_tracked(int expected_count)
{
    int index;

    TEST_CHECK(semantic_scissor_call_count == expected_count);
    TEST_CHECK(frame_result_track_count == expected_count);
    for (index = 0; index < expected_count; index++) {
        TEST_CHECK(check_tracked_frame_status(
                       index, RENDERER_STATUS_OK)
                   == 0);
    }
    return 0;
}

static int draw_event_count(void)
{
    int count = 0;
    int index;

    for (index = 0; index < event_count; index++) {
        if (events[index].kind == TRAVERSAL_EVENT_DRAW)
            count++;
    }
    return count;
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
    TEST_CHECK(check_ok_scissors_were_tracked(5) == 0);

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

static int check_sticky_failure_stops_unwind_commands(void)
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
    /* A failed renderer operation locks semantic state until end-frame retry,
     * so no further state command is submitted while unwinding. */
    TEST_CHECK(event_count == 4);
    TEST_CHECK(check_set_event(0, (RendererRect){5, 7, 21, 16}) == 0);
    TEST_CHECK(check_draw_event(1, parent_id) == 0);
    TEST_CHECK(check_set_event(2, (RendererRect){8, 10, 10, 7}) == 0);
    TEST_CHECK(check_draw_event(3, child_id) == 0);
    TEST_CHECK(draw_event_count() == 2);
    TEST_CHECK(check_ok_scissors_were_tracked(2) == 0);
    return 0;
}

static int check_checked_requires_renderer(void)
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
    TEST_CHECK(frame_result_track_count == 0);
    return 0;
}

static int check_fullscreen_checked_draw_orders_clipping(void)
{
    GLWidget widget;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&widget, &parent_id, 5, 7, 20, 15);
    reset_capture();
    active_draw_state = &state;

    TEST_CHECK(DrawGLWidgets_checked(&widget, renderer, &state)
               == RENDERER_STATUS_OK);
    TEST_CHECK(ordered_event_count == 4);
    TEST_CHECK(check_ordered_event(
                   0, ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
                   (RendererRect){5, 7, 21, 16}, 0)
               == 0);
    TEST_CHECK(check_ordered_event(
                   1, ORDERED_EVENT_DRAW,
                   (RendererRect){0}, parent_id)
               == 0);
    TEST_CHECK(check_ordered_event(
                   2, ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
                   (RendererRect){0, 0, 100, 80}, 0)
               == 0);
    TEST_CHECK(check_ordered_event(
                   3, ORDERED_EVENT_DISABLE_SEMANTIC_SCISSOR,
                   (RendererRect){0}, 0)
               == 0);
    TEST_CHECK(check_ok_scissors_were_tracked(3) == 0);
    return 0;
}

static int check_fullscreen_checked_arguments_and_empty_list(void)
{
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    reset_capture();
    TEST_CHECK(DrawGLWidgets_checked(NULL, NULL, &state)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(DrawGLWidgets_checked(NULL, renderer, NULL)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(ordered_event_count == 0);
    TEST_CHECK(frame_result_track_count == 0);
    reset_capture();
    TEST_CHECK(DrawGLWidgets_checked(NULL, renderer, &state)
               == RENDERER_STATUS_OK);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 1);
    TEST_CHECK(event_count == 1);
    TEST_CHECK(events[0].kind == TRAVERSAL_EVENT_DISABLE_SCISSOR);
    TEST_CHECK(check_ok_scissors_were_tracked(1) == 0);
    state.status = RENDERER_STATUS_BACKEND_ERROR;
    reset_capture();
    TEST_CHECK(DrawGLWidgets_checked(NULL, renderer, &state)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 0);
    TEST_CHECK(frame_result_track_count == 0);
    state.status = RENDERER_STATUS_OK;
    reset_capture();
    fake_frame_result = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(DrawGLWidgets_checked(NULL, renderer, &state)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 0);
    TEST_CHECK(frame_result_track_count == 0);
    return 0;
}

static int check_preexisting_failure_skips_fullscreen_widgets(void)
{
    GLWidget first;
    GLWidget second;
    SdlUiDrawState state = {RENDERER_STATUS_RESOURCE_MISMATCH, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&first, &parent_id, 5, 7, 20, 15);
    init_widget(&second, &skipped_id, 30, 7, 20, 15);
    first.next = &second;
    reset_capture();
    active_draw_state = &state;

    TEST_CHECK(DrawGLWidgets_checked(&first, renderer, &state)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 0);
    TEST_CHECK(frame_result_track_count == 0);
    state.status = RENDERER_STATUS_OK;
    reset_capture();
    active_draw_state = &state;
    fake_frame_result = RENDERER_STATUS_BACKEND_ERROR;
    TEST_CHECK(DrawGLWidgets_checked(&first, renderer, &state)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 0);
    TEST_CHECK(frame_result_track_count == 0);
    return 0;
}

static int check_fullscreen_widget_failure_skips_siblings(void)
{
    GLWidget first;
    GLWidget second;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&first, &parent_id, 5, 7, 20, 15);
    init_widget(&second, &skipped_id, 30, 7, 20, 15);
    first.next = &second;
    reset_capture();
    active_draw_state = &state;
    failing_widget = &first;

    TEST_CHECK(DrawGLWidgets_checked(&first, renderer, &state)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(draw_event_count() == 1);
    TEST_CHECK(check_draw_event(1, parent_id) == 0);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(check_ok_scissors_were_tracked(1) == 0);
    state.status = RENDERER_STATUS_OK;
    reset_capture();
    active_draw_state = &state;
    frame_failing_widget = &first;
    TEST_CHECK(DrawGLWidgets_checked(&first, renderer, &state)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_event_count() == 1);
    TEST_CHECK(check_draw_event(1, parent_id) == 0);
    TEST_CHECK(event_count == 2);
    TEST_CHECK(check_ok_scissors_were_tracked(1) == 0);
    return 0;
}

static int check_semantic_scissor_failures_are_sticky(void)
{
    GLWidget widget;
    SdlUiDrawState state = {RENDERER_STATUS_OK, 0};
    SdlRenderer *renderer = (SdlRenderer *)&fake_renderer_storage;

    init_widget(&widget, &parent_id, 5, 7, 20, 15);
    reset_capture();
    active_draw_state = &state;
    failing_semantic_scissor_call = 1;
    semantic_scissor_failure = RENDERER_STATUS_OUT_OF_MEMORY;

    TEST_CHECK(DrawGLWidgets_checked(&widget, renderer, &state)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 1);
    TEST_CHECK(ordered_event_count == 1);
    TEST_CHECK(check_ordered_event(
                   0, ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
                   (RendererRect){5, 7, 21, 16}, 0)
               == 0);
    TEST_CHECK(frame_result_track_count == 1);
    TEST_CHECK(check_tracked_frame_status(
                   0, RENDERER_STATUS_OUT_OF_MEMORY)
               == 0);
    TEST_CHECK(fake_frame_result == RENDERER_STATUS_OUT_OF_MEMORY);
    /* The retained first failure blocks a same-frame retry before any new
     * semantic state, draw, or tracking operation is attempted. */
    TEST_CHECK(DrawGLWidgets_checked(&widget, renderer, &state)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(draw_event_count() == 0);
    TEST_CHECK(semantic_scissor_call_count == 1);
    TEST_CHECK(frame_result_track_count == 1);
    TEST_CHECK(ordered_event_count == 1);
    TEST_CHECK(fake_frame_result == RENDERER_STATUS_OUT_OF_MEMORY);
    reset_capture();
    active_draw_state = &state;
    failing_semantic_scissor_call = 2;
    semantic_scissor_failure = RENDERER_STATUS_RESOURCE_MISMATCH;
    TEST_CHECK(DrawGLWidgets_checked(&widget, renderer, &state)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(draw_event_count() == 1);
    TEST_CHECK(semantic_scissor_call_count == 2);
    TEST_CHECK(ordered_event_count == 3);
    TEST_CHECK(check_ordered_event(
                   0, ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
                   (RendererRect){5, 7, 21, 16}, 0)
               == 0);
    TEST_CHECK(check_ordered_event(
                   1, ORDERED_EVENT_DRAW,
                   (RendererRect){0}, parent_id)
               == 0);
    TEST_CHECK(check_ordered_event(
                   2, ORDERED_EVENT_SET_SEMANTIC_SCISSOR,
                   (RendererRect){0, 0, 100, 80}, 0)
               == 0);
    TEST_CHECK(frame_result_track_count == 2);
    TEST_CHECK(check_tracked_frame_status(
                   0, RENDERER_STATUS_OK)
               == 0);
    TEST_CHECK(check_tracked_frame_status(
                   1, RENDERER_STATUS_RESOURCE_MISMATCH)
               == 0);
    TEST_CHECK(fake_frame_result
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    reset_capture();
    active_draw_state = &state;
    failing_semantic_scissor_call = 3;
    semantic_scissor_failure = RENDERER_STATUS_INVALID_STATE;
    TEST_CHECK(DrawGLWidgets_checked(&widget, renderer, &state)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(draw_event_count() == 1);
    TEST_CHECK(semantic_scissor_call_count == 3);
    TEST_CHECK(ordered_event_count == 4);
    TEST_CHECK(check_ordered_event(
                   3, ORDERED_EVENT_DISABLE_SEMANTIC_SCISSOR,
                   (RendererRect){0}, 0)
               == 0);
    TEST_CHECK(frame_result_track_count == 3);
    TEST_CHECK(check_tracked_frame_status(
                   0, RENDERER_STATUS_OK)
               == 0);
    TEST_CHECK(check_tracked_frame_status(
                   1, RENDERER_STATUS_OK)
               == 0);
    TEST_CHECK(check_tracked_frame_status(
                   2, RENDERER_STATUS_INVALID_STATE)
               == 0);
    TEST_CHECK(fake_frame_result == RENDERER_STATUS_INVALID_STATE);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_success_restores_nested_parents() == 0);
    TEST_CHECK(check_sticky_failure_stops_unwind_commands() == 0);
    TEST_CHECK(check_checked_requires_renderer() == 0);
    TEST_CHECK(check_fullscreen_checked_draw_orders_clipping() == 0);
    TEST_CHECK(check_fullscreen_checked_arguments_and_empty_list() == 0);
    TEST_CHECK(check_preexisting_failure_skips_fullscreen_widgets() == 0);
    TEST_CHECK(check_fullscreen_widget_failure_skips_siblings() == 0);
    TEST_CHECK(check_semantic_scissor_failures_are_sticky()
               == 0);
    return 0;
}
