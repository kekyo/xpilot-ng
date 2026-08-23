#include "xpcommon.h"

#include "mtwister.h"

#include <stdint.h>

#define XPILOT_DEFAULT_MT_SEED UINT32_C(4357)
#define XPILOT_MT_SEED_MULTIPLIER UINT32_C(69069)

static MTRand random_state;
static int random_state_initialized;

void seedMT(unsigned int seed)
{
    uint32_t value = (uint32_t)seed | UINT32_C(1);
    int index;

    /* Preserve XPilot's historic odd-seed expansion while delegating the
     * MT19937 twist and temper operations to the upstream implementation. */
    for (index = 0; index < STATE_VECTOR_LENGTH; index++) {
        random_state.mt[index] = value;
        value *= XPILOT_MT_SEED_MULTIPLIER;
    }
    random_state.index = STATE_VECTOR_LENGTH;
    random_state_initialized = 1;
}

unsigned int randomMT(void)
{
    if (!random_state_initialized)
        seedMT(XPILOT_DEFAULT_MT_SEED);
    return (unsigned int)genRandLong(&random_state);
}
