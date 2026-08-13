#include "sdlpaint.h"

#include <stdio.h>
#include <string.h>

#define TEST_CHECK(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "%s:%d: check failed: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

client_data_t clData;
ipos_t world;
unsigned draw_width;
unsigned draw_height;

typedef struct FakeMatrices {
    GLenum mode;
    int modelview_depth;
    int projection_depth;
    int calls;
    int invalid_operations;
    int load_identity_calls;
    int translate_calls;
    int scale_calls;
    int ortho_calls;
    GLfloat translate_x;
    GLfloat translate_y;
    GLfloat scale_x;
    GLfloat scale_y;
    GLdouble ortho_right;
    GLdouble ortho_bottom;
} FakeMatrices;

static FakeMatrices matrices;

void GLAPIENTRY glMatrixMode(GLenum mode)
{
    matrices.calls++;
    if (mode != GL_MODELVIEW && mode != GL_PROJECTION) {
        matrices.invalid_operations++;
        return;
    }
    matrices.mode = mode;
}

void GLAPIENTRY glPushMatrix(void)
{
    matrices.calls++;
    if (matrices.mode == GL_MODELVIEW)
        matrices.modelview_depth++;
    else if (matrices.mode == GL_PROJECTION)
        matrices.projection_depth++;
    else
        matrices.invalid_operations++;
}

void GLAPIENTRY glPopMatrix(void)
{
    int *depth = NULL;

    matrices.calls++;
    if (matrices.mode == GL_MODELVIEW)
        depth = &matrices.modelview_depth;
    else if (matrices.mode == GL_PROJECTION)
        depth = &matrices.projection_depth;
    if (depth == NULL || *depth == 0) {
        matrices.invalid_operations++;
        return;
    }
    (*depth)--;
}

void GLAPIENTRY glLoadIdentity(void)
{
    matrices.calls++;
    matrices.load_identity_calls++;
}

void GLAPIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    matrices.calls++;
    matrices.translate_calls++;
    matrices.translate_x = x;
    matrices.translate_y = y;
    if (z != 0.0f)
        matrices.invalid_operations++;
}

void GLAPIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    matrices.calls++;
    matrices.scale_calls++;
    matrices.scale_x = x;
    matrices.scale_y = y;
    if (z != 0.0f)
        matrices.invalid_operations++;
}

void GLAPIENTRY gluOrtho2D(GLdouble left, GLdouble right,
                           GLdouble bottom, GLdouble top)
{
    matrices.calls++;
    matrices.ortho_calls++;
    matrices.ortho_right = right;
    matrices.ortho_bottom = bottom;
    if (left != 0.0 || top != 0.0)
        matrices.invalid_operations++;
}

static void reset_matrices(void)
{
    memset(&matrices, 0, sizeof(matrices));
    matrices.mode = GL_MODELVIEW;
    world.x = 2;
    world.y = -3;
    clData.scale = 1.4;
    draw_width = 640;
    draw_height = 480;
    Sdlpaint_test_begin_logical_frame();
}

typedef enum StageEvent {
    STAGE_EVENT_WORLD,
    STAGE_EVENT_VFUEL,
    STAGE_EVENT_VDECOR,
    STAGE_EVENT_VCANNON,
    STAGE_EVENT_VBASE,
    STAGE_EVENT_OBJECTS,
    STAGE_EVENT_SCORE,
    STAGE_EVENT_SHOTS,
    STAGE_EVENT_SETUP_MOVING,
    STAGE_EVENT_SHIPS,
    STAGE_EVENT_COUNT
} StageEvent;

typedef struct FakeStages {
    StageEvent events[STAGE_EVENT_COUNT];
    int event_count;
    RendererStatus stage_results[STAGE_EVENT_COUNT];
    RendererStatus pending_result;
} FakeStages;

static void record_stage(FakeStages *stages, StageEvent event)
{
    if (stages->event_count < STAGE_EVENT_COUNT)
        stages->events[stages->event_count++] = event;
    stages->pending_result = stages->stage_results[event];
}

static void fake_paint_world(void *context)
{
    record_stage(context, STAGE_EVENT_WORLD);
}

static void fake_paint_vfuel(void *context)
{
    record_stage(context, STAGE_EVENT_VFUEL);
}

static void fake_paint_vdecor(void *context)
{
    record_stage(context, STAGE_EVENT_VDECOR);
}

static void fake_paint_vcannon(void *context)
{
    record_stage(context, STAGE_EVENT_VCANNON);
}

static void fake_paint_vbase(void *context)
{
    record_stage(context, STAGE_EVENT_VBASE);
}

static void fake_paint_objects(void *context)
{
    record_stage(context, STAGE_EVENT_OBJECTS);
}

static void fake_paint_score_objects(void *context)
{
    record_stage(context, STAGE_EVENT_SCORE);
}

static void fake_paint_shots(void *context)
{
    record_stage(context, STAGE_EVENT_SHOTS);
}

static void fake_setup_moving(void *context)
{
    record_stage(context, STAGE_EVENT_SETUP_MOVING);
}

static void fake_paint_ships(void *context)
{
    record_stage(context, STAGE_EVENT_SHIPS);
}

static RendererStatus fake_frame_result(void *context)
{
    FakeStages *stages = context;
    RendererStatus status = stages->pending_result;

    /* A non-sticky fake makes retention of the first observed failure an
     * externally visible responsibility of the stage gate. */
    stages->pending_result = RENDERER_STATUS_OK;
    return status;
}

static const SdlPaintStageOps stage_ops = {
    .paint_world = fake_paint_world,
    .paint_vfuel = fake_paint_vfuel,
    .paint_vdecor = fake_paint_vdecor,
    .paint_vcannon = fake_paint_vcannon,
    .paint_vbase = fake_paint_vbase,
    .paint_objects = fake_paint_objects,
    .paint_score_objects = fake_paint_score_objects,
    .paint_shots = fake_paint_shots,
    .setup_moving = fake_setup_moving,
    .paint_ships = fake_paint_ships,
    .frame_result = fake_frame_result
};

static void reset_stages(FakeStages *stages)
{
    memset(stages, 0, sizeof(*stages));
}

static int expect_run(bool old_server, FakeStages *stages,
                      RendererStatus expected_status,
                      const StageEvent *expected_events,
                      size_t expected_count)
{
    RendererStatus status = Sdlpaint_test_run_world_object_stages(
        old_server, &stage_ops, stages);

    TEST_CHECK(status == expected_status);
    TEST_CHECK(stages->event_count == (int)expected_count);
    TEST_CHECK(memcmp(stages->events, expected_events,
                      expected_count * sizeof(expected_events[0])) == 0);
    return 0;
}

static int check_server_sequences(void)
{
    static const StageEvent new_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_OBJECTS,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    static const StageEvent old_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_VFUEL,
        STAGE_EVENT_VDECOR,
        STAGE_EVENT_VCANNON,
        STAGE_EVENT_VBASE,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    FakeStages stages;

    reset_stages(&stages);
    TEST_CHECK(expect_run(false, &stages, RENDERER_STATUS_OK,
                          new_events,
                          sizeof(new_events) / sizeof(new_events[0])) == 0);

    reset_stages(&stages);
    TEST_CHECK(expect_run(true, &stages, RENDERER_STATUS_OK,
                          old_events,
                          sizeof(old_events) / sizeof(old_events[0])) == 0);
    return 0;
}

static int check_new_server_failures_keep_cleanup(void)
{
    static const StageEvent full_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_OBJECTS,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    static const StageEvent gated_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_OBJECTS,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SHIPS
    };
    static const StageEvent failure_stages[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_OBJECTS,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    FakeStages stages;
    size_t i;

    for (i = 0; i < sizeof(failure_stages) / sizeof(failure_stages[0]); ++i) {
        const bool before_moving =
            failure_stages[i] < STAGE_EVENT_SETUP_MOVING;

        reset_stages(&stages);
        stages.stage_results[failure_stages[i]] =
            RENDERER_STATUS_BACKEND_ERROR;
        TEST_CHECK(expect_run(
            false, &stages, RENDERER_STATUS_BACKEND_ERROR,
            before_moving ? gated_events : full_events,
            before_moving
                ? sizeof(gated_events) / sizeof(gated_events[0])
                : sizeof(full_events) / sizeof(full_events[0])) == 0);
    }
    return 0;
}

static int check_old_server_failures_keep_cleanup(void)
{
    static const StageEvent full_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_VFUEL,
        STAGE_EVENT_VDECOR,
        STAGE_EVENT_VCANNON,
        STAGE_EVENT_VBASE,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    static const StageEvent gated_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_VFUEL,
        STAGE_EVENT_VDECOR,
        STAGE_EVENT_VCANNON,
        STAGE_EVENT_VBASE,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SHIPS
    };
    static const StageEvent failure_stages[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_VFUEL,
        STAGE_EVENT_VDECOR,
        STAGE_EVENT_VCANNON,
        STAGE_EVENT_VBASE,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SETUP_MOVING,
        STAGE_EVENT_SHIPS
    };
    FakeStages stages;
    size_t i;

    for (i = 0; i < sizeof(failure_stages) / sizeof(failure_stages[0]); ++i) {
        const bool before_moving =
            failure_stages[i] < STAGE_EVENT_SETUP_MOVING;

        reset_stages(&stages);
        stages.stage_results[failure_stages[i]] =
            RENDERER_STATUS_RESOURCE_MISMATCH;
        TEST_CHECK(expect_run(
            true, &stages, RENDERER_STATUS_RESOURCE_MISMATCH,
            before_moving ? gated_events : full_events,
            before_moving
                ? sizeof(gated_events) / sizeof(gated_events[0])
                : sizeof(full_events) / sizeof(full_events[0])) == 0);
    }
    return 0;
}

static int check_first_failure_is_retained(void)
{
    static const StageEvent expected_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_VFUEL,
        STAGE_EVENT_VDECOR,
        STAGE_EVENT_VCANNON,
        STAGE_EVENT_VBASE,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SHIPS
    };
    FakeStages stages;

    reset_stages(&stages);
    stages.stage_results[STAGE_EVENT_VFUEL] =
        RENDERER_STATUS_BACKEND_ERROR;
    stages.stage_results[STAGE_EVENT_SHIPS] =
        RENDERER_STATUS_OUT_OF_MEMORY;
    TEST_CHECK(expect_run(
        true, &stages, RENDERER_STATUS_BACKEND_ERROR,
        expected_events,
        sizeof(expected_events) / sizeof(expected_events[0])) == 0);
    return 0;
}

static int check_existing_failure_still_traverses_cleanup(void)
{
    static const StageEvent expected_events[] = {
        STAGE_EVENT_WORLD,
        STAGE_EVENT_OBJECTS,
        STAGE_EVENT_SCORE,
        STAGE_EVENT_SHOTS,
        STAGE_EVENT_SHIPS
    };
    FakeStages stages;

    reset_stages(&stages);
    stages.pending_result = RENDERER_STATUS_INVALID_STATE;
    TEST_CHECK(expect_run(
        false, &stages, RENDERER_STATUS_INVALID_STATE,
        expected_events,
        sizeof(expected_events) / sizeof(expected_events[0])) == 0);
    return 0;
}

static int check_logical_frames_reset_matrix_phase(void)
{
    paintSetupMode = HUD_MODE;
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);

    paintSetupMode = STATIONARY_MODE | MOVING_MODE;
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);
    return 0;
}

static int check_matrix_phase_sequence_is_balanced(void)
{
    reset_matrices();
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_NONE);

    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_STATIONARY,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == STATIONARY_MODE);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_WORLD);
    TEST_CHECK(matrices.modelview_depth == 1);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.translate_x == -3.0f);
    TEST_CHECK(matrices.translate_y == 4.0f);

    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_MOVING,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == MOVING_MODE);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_WORLD);
    TEST_CHECK(matrices.modelview_depth == 1);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.translate_x == -2.8f);
    TEST_CHECK(matrices.translate_y == 4.2f);

    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == HUD_MODE);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_HUD);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 1);
    TEST_CHECK(matrices.ortho_right == 640.0);
    TEST_CHECK(matrices.ortho_bottom == 480.0);

    TEST_CHECK(Sdlpaint_test_end_logical_frame(
                   RENDERER_STATUS_BACKEND_ERROR)
               == RENDERER_STATUS_BACKEND_ERROR);
    TEST_CHECK(paintSetupMode == 0);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_NONE);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.mode == GL_MODELVIEW);
    TEST_CHECK(matrices.invalid_operations == 0);
    return 0;
}

static int check_failed_setup_is_atomic_and_world_unwinds(void)
{
    int calls_before_failure;

    reset_matrices();
    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_STATIONARY,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    calls_before_failure = matrices.calls;
    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OUT_OF_MEMORY)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(matrices.calls == calls_before_failure);
    TEST_CHECK(paintSetupMode == STATIONARY_MODE);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_WORLD);
    TEST_CHECK(matrices.modelview_depth == 1);
    TEST_CHECK(matrices.projection_depth == 0);

    TEST_CHECK(Sdlpaint_test_end_logical_frame(RENDERER_STATUS_OK)
               == RENDERER_STATUS_OK);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.invalid_operations == 0);
    return 0;
}

static int check_hud_to_world_transition_is_balanced(void)
{
    reset_matrices();
    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 1);

    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_MOVING,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == MOVING_MODE);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_WORLD);
    TEST_CHECK(matrices.modelview_depth == 1);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.mode == GL_MODELVIEW);

    TEST_CHECK(Sdlpaint_test_end_logical_frame(RENDERER_STATUS_OK)
               == RENDERER_STATUS_OK);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.invalid_operations == 0);
    return 0;
}

static int check_none_failure_invalid_target_and_next_frame(void)
{
    int calls_before_failure;

    reset_matrices();
    calls_before_failure = matrices.calls;
    TEST_CHECK(Sdlpaint_test_apply_setup(
                   SDLPAINT_TEST_SETUP_STATIONARY,
                   RENDERER_STATUS_INVALID_STATE)
               == RENDERER_STATUS_INVALID_STATE);
    TEST_CHECK(matrices.calls == calls_before_failure);
    TEST_CHECK(paintSetupMode == 0);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_NONE);

    TEST_CHECK(Sdlpaint_test_apply_setup(
                   (SdlPaintTestSetupTarget)99,
                   RENDERER_STATUS_OK)
               == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(matrices.calls == calls_before_failure);
    TEST_CHECK(paintSetupMode == 0);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_NONE);

    TEST_CHECK(Sdlpaint_test_end_logical_frame(
                   RENDERER_STATUS_RESOURCE_MISMATCH)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(matrices.calls == calls_before_failure);
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);
    TEST_CHECK(Sdlpaint_test_matrix_phase()
               == SDLPAINT_TEST_MATRIX_NONE);
    TEST_CHECK(matrices.modelview_depth == 0);
    TEST_CHECK(matrices.projection_depth == 0);
    TEST_CHECK(matrices.invalid_operations == 0);
    return 0;
}

int main(void)
{
    if (check_server_sequences() != 0)
        return 1;
    if (check_new_server_failures_keep_cleanup() != 0)
        return 1;
    if (check_old_server_failures_keep_cleanup() != 0)
        return 1;
    if (check_first_failure_is_retained() != 0)
        return 1;
    if (check_existing_failure_still_traverses_cleanup() != 0)
        return 1;
    if (check_logical_frames_reset_matrix_phase() != 0)
        return 1;
    if (check_matrix_phase_sequence_is_balanced() != 0)
        return 1;
    if (check_failed_setup_is_atomic_and_world_unwinds() != 0)
        return 1;
    if (check_hud_to_world_transition_is_balanced() != 0)
        return 1;
    if (check_none_failure_invalid_target_and_next_frame() != 0)
        return 1;
    return 0;
}
