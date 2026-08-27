#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "socklib.h"

/** Result of preparing or advancing the client game loop. */
typedef enum GameLoopResult {
    /** The caller may schedule another iteration. */
    GAME_LOOP_CONTINUE = 0,
    /** The session ended or a transport or input error occurred. */
    GAME_LOOP_STOP = 1
} GameLoopResult;

/** Persistent state shared by scheduled game-loop iterations. */
typedef struct GameLoopState {
    /** Current native socket handle used only by the blocking wrapper. */
    socket_handle_t network_fd;
    /** Most recent Net_input result, including buffered-work indication. */
    int previous_network_result;
} GameLoopState;

/**
 * Perform the initial non-blocking network and input processing.
 *
 * @param state Receives the initialized loop state.
 * @return GAME_LOOP_CONTINUE when iterations may be scheduled, otherwise
 * GAME_LOOP_STOP.
 */
GameLoopResult Game_loop_prepare(GameLoopState *state);

/**
 * Advance the client game loop once without waiting.
 *
 * @param state State initialized by Game_loop_prepare().
 * @param network_ready Nonzero when the transport can be read without waiting.
 * @return GAME_LOOP_CONTINUE when another iteration may be scheduled,
 * otherwise GAME_LOOP_STOP.
 *
 * @remarks A scheduler that cannot query socket readiness may pass nonzero;
 * Net_input is non-blocking and reports when no complete input is ready. The
 * native socket handle is refreshed after every successful step so a resumed
 * gameplay session can replace its transport safely.
 */
GameLoopResult Game_loop_step(GameLoopState *state, int network_ready);

/** Run the native blocking wrapper until the session ends. */
void Game_loop(void);

#endif /* GAMELOOP_H */
