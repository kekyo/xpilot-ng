#include "test_helpers.h"

#include "console.h"
#include "glwidgets.h"
#include "sdlevent.h"

#include <stdarg.h>
#include <string.h>

client_data_t clData;
GLWidget *MainWidget;

static int console_handles_event;
static int console_visible;
static int key_press_count;
static int key_release_count;
static xp_keysym_t last_pressed_key;
static xp_keysym_t last_released_key;
static int widget_lookup_count;
static int drag_motion_count;
static Sint16 last_drag_xrel;
static Sint16 last_drag_yrel;
static Uint16 last_drag_x;
static Uint16 last_drag_y;
static void *last_drag_data;
static int pointer_move_count;
static int last_pointer_move;
static int net_flush_count;
static int resize_count;
static int last_width;
static int last_height;
static int client_exit_count;
static int last_exit_status;
static int key_clear_count;
static int pointer_release_count[MAX_POINTER_BUTTONS];
static int invalid_pointer_release_count;
static int widget_release_count[NUM_MOUSE_BUTTONS];
static Uint8 widget_release_button[NUM_MOUSE_BUTTONS];
static Uint8 widget_release_state[NUM_MOUSE_BUTTONS];
static Uint16 widget_release_x[NUM_MOUSE_BUTTONS];
static Uint16 widget_release_y[NUM_MOUSE_BUTTONS];
static int invalid_widget_release_count;

static const int focus_mouse_x = 137;
static const int focus_mouse_y = 211;

void Add_message(const char *message)
{
    (void)message;
}

void Client_exit(int status)
{
    client_exit_count++;
    last_exit_status = status;
}

int Client_pointer_move(int movement)
{
    pointer_move_count++;
    last_pointer_move = movement;
    return 0;
}

int Console_isVisible(void)
{
    return console_visible;
}

int Console_process(SDL_Event *event)
{
    (void)event;
    return console_handles_event;
}

void Console_show(void)
{
}

void Console_hide(void)
{
}

GLWidget *FindGLWidget(GLWidget *list, Uint16 x, Uint16 y)
{
    (void)list;
    (void)x;
    (void)y;
    widget_lookup_count++;
    return NULL;
}

void Keyboard_button_pressed(xp_keysym_t key)
{
    key_press_count++;
    last_pressed_key = key;
}

void Keyboard_button_released(xp_keysym_t key)
{
    key_release_count++;
    last_released_key = key;
}

void Key_clear_counts(void)
{
    key_clear_count++;
}

void MainWidget_ShowMenu(GLWidget *widget, bool show)
{
    (void)widget;
    (void)show;
}

int Net_flush(void)
{
    net_flush_count++;
    return 0;
}

void Pointer_button_pressed(int button)
{
    (void)button;
}

void Pointer_button_released(int button)
{
    if (button < 1 || button > MAX_POINTER_BUTTONS) {
        invalid_pointer_release_count++;
        return;
    }
    pointer_release_count[button - 1]++;
}

SDL_MouseButtonFlags SDL_GetMouseState(float *x, float *y)
{
    if (x != NULL) {
        *x = focus_mouse_x;
    }
    if (y != NULL) {
        *y = focus_mouse_y;
    }
    return 0;
}

void Set_window_grab(bool on)
{
    (void)on;
}

bool Set_relative_mouse_mode(bool on)
{
    (void)on;
    return true;
}

void Window_size_changed(int width, int height)
{
    resize_count++;
    last_width = width;
    last_height = height;
}

void warn(const char *format, ...)
{
    (void)format;
}

static void record_drag_motion(Sint16 xrel, Sint16 yrel, Uint16 x, Uint16 y,
                               void *data)
{
    drag_motion_count++;
    last_drag_xrel = xrel;
    last_drag_yrel = yrel;
    last_drag_x = x;
    last_drag_y = y;
    last_drag_data = data;
}

static void record_widget_release(Uint8 button, Uint8 state, Uint16 x,
                                  Uint16 y, void *data)
{
    int widget_index;

    if (data == NULL) {
        invalid_widget_release_count++;
        return;
    }
    widget_index = *(int *)data;
    if (widget_index < 0 || widget_index >= NUM_MOUSE_BUTTONS) {
        invalid_widget_release_count++;
        return;
    }
    widget_release_count[widget_index]++;
    widget_release_button[widget_index] = button;
    widget_release_state[widget_index] = state;
    widget_release_x[widget_index] = x;
    widget_release_y[widget_index] = y;
}

static void reset_observations(void)
{
    memset(&clData, 0, sizeof(clData));
    memset(clicktarget, 0, sizeof(GLWidget *) * NUM_MOUSE_BUTTONS);
    hovertarget = NULL;
    console_handles_event = 0;
    console_visible = 0;
    key_press_count = 0;
    key_release_count = 0;
    widget_lookup_count = 0;
    drag_motion_count = 0;
    pointer_move_count = 0;
    net_flush_count = 0;
    resize_count = 0;
    client_exit_count = 0;
    key_clear_count = 0;
    memset(pointer_release_count, 0, sizeof(pointer_release_count));
    invalid_pointer_release_count = 0;
    memset(widget_release_count, 0, sizeof(widget_release_count));
    memset(widget_release_button, 0, sizeof(widget_release_button));
    memset(widget_release_state, 0, sizeof(widget_release_state));
    memset(widget_release_x, 0, sizeof(widget_release_x));
    memset(widget_release_y, 0, sizeof(widget_release_y));
    invalid_widget_release_count = 0;
}

int main(void)
{
    SDL_Event event;
    GLWidget drag_widget;
    GLWidget focus_widgets[NUM_MOUSE_BUTTONS];
    GLWidget hover_widget;
    int drag_marker;
    int focus_markers[NUM_MOUSE_BUTTONS];
    int button_index;

    reset_observations();

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_QUIT;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(client_exit_count == 1);
    TEST_CHECK(last_exit_status == 0);

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_press_count == 1);
    TEST_CHECK(last_pressed_key == (xp_keysym_t)SDLK_A);

    event.key.repeat = 1;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_press_count == 1);

    console_visible = 1;
    event.key.repeat = 0;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_press_count == 1);

    event.type = SDL_EVENT_KEY_UP;
    event.key.key = SDLK_A;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_release_count == 1);
    TEST_CHECK(last_released_key == (xp_keysym_t)SDLK_A);

    console_handles_event = 1;
    event.key.key = SDLK_B;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_release_count == 1);
    console_handles_event = 0;
    console_visible = 0;

    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.down = true;
    event.button.button = 0;
    TEST_CHECK(Process_event(&event) == 1);
    event.button.button = NUM_MOUSE_BUTTONS + 1;
    TEST_CHECK(Process_event(&event) == 1);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(widget_lookup_count == 0);

    /* SDL3 assigns 4 and 5 to the physical X1/X2 buttons.  XPilot keeps
     * those legacy button numbers reserved for synthesized wheel events. */
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.down = true;
    event.button.button = SDL_BUTTON_X1;
    TEST_CHECK(Process_event(&event) == 1);
    event.button.button = SDL_BUTTON_X2;
    TEST_CHECK(Process_event(&event) == 1);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    event.button.button = SDL_BUTTON_X1;
    TEST_CHECK(Process_event(&event) == 1);
    event.button.button = SDL_BUTTON_X2;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(widget_lookup_count == 0);

    /* SDL3 exposes high-resolution wheel deltas as floats.  Fractional
     * movement must accumulate until it represents a legacy wheel tick. */
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    event.wheel.y = 0.5f;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(widget_lookup_count == 0);
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(widget_lookup_count == 1);

    memset(&drag_widget, 0, sizeof(drag_widget));
    drag_widget.motion = record_drag_motion;
    drag_widget.motiondata = &drag_marker;
    clicktarget[0] = &drag_widget;
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = -3;
    event.motion.yrel = 4;
    event.motion.x = 120;
    event.motion.y = 80;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(drag_motion_count == 1);
    TEST_CHECK(last_drag_xrel == -3);
    TEST_CHECK(last_drag_yrel == 4);
    TEST_CHECK(last_drag_x == 120);
    TEST_CHECK(last_drag_y == 80);
    TEST_CHECK(last_drag_data == &drag_marker);

    clicktarget[0] = NULL;
    clData.pointerControl = true;
    event.motion.xrel = 7;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(pointer_move_count == 1);
    TEST_CHECK(last_pointer_move == 7);
    TEST_CHECK(net_flush_count == 1);

    clData.pointerControl = false;
    memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    event.window.data1 = 1024;
    event.window.data2 = 768;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(resize_count == 1);
    TEST_CHECK(last_width == 1024);
    TEST_CHECK(last_height == 768);

    event.type = SDL_EVENT_WINDOW_MOVED;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(resize_count == 1);

    for (button_index = 0; button_index < NUM_MOUSE_BUTTONS;
         button_index++) {
        memset(&focus_widgets[button_index], 0,
               sizeof(focus_widgets[button_index]));
        focus_markers[button_index] = button_index;
        focus_widgets[button_index].button = record_widget_release;
        focus_widgets[button_index].buttondata = &focus_markers[button_index];
        clicktarget[button_index] = &focus_widgets[button_index];
    }
    memset(&hover_widget, 0, sizeof(hover_widget));
    hovertarget = &hover_widget;

    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    TEST_CHECK(Process_event(&event) == 1);
    TEST_CHECK(key_clear_count == 1);
    TEST_CHECK(invalid_pointer_release_count == 0);
    for (button_index = 0; button_index < MAX_POINTER_BUTTONS;
         button_index++) {
        TEST_CHECK(pointer_release_count[button_index] == 1);
    }
    TEST_CHECK(invalid_widget_release_count == 0);
    for (button_index = 0; button_index < NUM_MOUSE_BUTTONS;
         button_index++) {
        TEST_CHECK(widget_release_count[button_index] == 1);
        TEST_CHECK(widget_release_button[button_index] == button_index + 1);
        TEST_CHECK(widget_release_state[button_index] == false);
        TEST_CHECK(widget_release_x[button_index] == focus_mouse_x);
        TEST_CHECK(widget_release_y[button_index] == focus_mouse_y);
        TEST_CHECK(clicktarget[button_index] == NULL);
    }
    TEST_CHECK(hovertarget == NULL);

    return 0;
}
