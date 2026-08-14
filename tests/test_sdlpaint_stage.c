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

static int check_logical_frames_reset_setup_mode(void)
{
    paintSetupMode = HUD_MODE;
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);

    paintSetupMode = STATIONARY_MODE | MOVING_MODE;
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);
    return 0;
}

static int check_successful_setup_commits_mode(void)
{
    Sdlpaint_test_begin_logical_frame();

    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_STATIONARY,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == STATIONARY_MODE);

    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_MOVING,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == MOVING_MODE);

    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(paintSetupMode == HUD_MODE);
    return 0;
}

static int check_failed_setup_leaves_mode_unchanged(void)
{
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_STATIONARY,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OUT_OF_MEMORY)
               == RENDERER_STATUS_OUT_OF_MEMORY);
    TEST_CHECK(paintSetupMode == STATIONARY_MODE);
    return 0;
}

static int check_invalid_setup_target_leaves_mode_unchanged(void)
{
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_HUD,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdlpaint_test_commit_setup(
                   (SdlPaintTestSetupTarget)99,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(paintSetupMode == HUD_MODE);
    return 0;
}

static int check_end_frame_resets_mode_and_preserves_result(void)
{
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(Sdlpaint_test_commit_setup(
                   SDLPAINT_TEST_SETUP_MOVING,
                   RENDERER_STATUS_OK) == RENDERER_STATUS_OK);
    TEST_CHECK(Sdlpaint_test_end_logical_frame(
                   RENDERER_STATUS_RESOURCE_MISMATCH)
               == RENDERER_STATUS_RESOURCE_MISMATCH);
    TEST_CHECK(paintSetupMode == 0);

    paintSetupMode = HUD_MODE;
    Sdlpaint_test_begin_logical_frame();
    TEST_CHECK(paintSetupMode == 0);
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
    if (check_logical_frames_reset_setup_mode() != 0)
        return 1;
    if (check_successful_setup_commits_mode() != 0)
        return 1;
    if (check_failed_setup_leaves_mode_unchanged() != 0)
        return 1;
    if (check_invalid_setup_target_leaves_mode_unchanged() != 0)
        return 1;
    if (check_end_frame_resets_mode_and_preserves_result() != 0)
        return 1;
    return 0;
}
