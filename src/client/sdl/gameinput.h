#ifndef GAMEINPUT_H
#define GAMEINPUT_H

/**
 * Processes a bounded batch of pending SDL input events.
 *
 * The batch limit ensures that callers regain control even when SDL events
 * are produced continuously.
 *
 * @return Nonzero when event processing requested that the game loop stop,
 * otherwise zero.
 */
int Game_input_process_batch(void);

#endif /* GAMEINPUT_H */
