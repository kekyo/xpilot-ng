/* Test seams for deterministic image preparation. */

#ifndef IMAGES_TEST_SUPPORT_H
#define IMAGES_TEST_SUPPORT_H

#include "gfx2d.h"

/** Picture operations injected into image preparation tests. */
typedef struct ImagesTestHooks {
    /** Load or generate a picture. */
    int (*picture_init)(xp_picture_t *picture, const char *filename,
                        int count, void *context);
    /** Read one top-left-origin source pixel. */
    RGB_COLOR (*picture_get_pixel)(const xp_picture_t *picture, int frame,
                                   int x, int y, void *context);
    /** Release a successfully initialized picture. */
    void (*picture_cleanup)(xp_picture_t *picture, void *context);
    /** Opaque value supplied to every hook. */
    void *context;
} ImagesTestHooks;

/**
 * Replace picture operations used by image preparation tests.
 *
 * @param hooks Complete hook table copied by value.
 */
void Images_test_set_hooks(const ImagesTestHooks *hooks);

/** Reset image preparation test hooks to an unavailable state. */
void Images_test_reset_hooks(void);

#endif /* IMAGES_TEST_SUPPORT_H */
