#include "test_helpers.h"

#include "xpcommon.h"

#include <limits.h>
#include <string.h>

#define SAMPLE_COUNT 12

static int check_legacy_sequence_and_default_seed(void)
{
    const unsigned int expected[SAMPLE_COUNT] = {
        3510405877U, 4290933890U, 2191955339U, 564929546U,
        152112058U, 4262624192U, 2687398418U, 268830360U,
        1763988213U, 578848526U, 4212814465U, 3596577449U
    };
    unsigned int index;

    TEST_CHECK(UINT_MAX >= 0xffffffffU);
    TEST_CHECK(randomMT() == expected[0]);

    seedMT(4357U);
    for (index = 0; index < SAMPLE_COUNT; index++)
        TEST_CHECK(randomMT() == expected[index]);
    return 0;
}

static int check_reseeding_is_deterministic(void)
{
    unsigned int first[SAMPLE_COUNT];
    unsigned int second[SAMPLE_COUNT];
    unsigned int index;

    seedMT(1U);
    for (index = 0; index < SAMPLE_COUNT; index++)
        first[index] = randomMT();
    seedMT(1U);
    for (index = 0; index < SAMPLE_COUNT; index++)
        second[index] = randomMT();

    TEST_CHECK(memcmp(first, second, sizeof(first)) == 0);
    TEST_CHECK(first[0] == 3796174982U);
    TEST_CHECK(first[3] == 3809380472U);
    TEST_CHECK(first[7] == 2094500863U);
    return 0;
}

static int check_even_seeds_are_normalized_to_odd(void)
{
    unsigned int even_seeded[SAMPLE_COUNT];
    unsigned int odd_seeded[SAMPLE_COUNT];
    unsigned int index;

    seedMT(2U);
    for (index = 0; index < SAMPLE_COUNT; index++)
        even_seeded[index] = randomMT();
    seedMT(3U);
    for (index = 0; index < SAMPLE_COUNT; index++)
        odd_seeded[index] = randomMT();

    TEST_CHECK(memcmp(even_seeded, odd_seeded, sizeof(even_seeded)) == 0);
    TEST_CHECK(even_seeded[0] == 2618579386U);
    TEST_CHECK(even_seeded[5] == 184702899U);
    TEST_CHECK(even_seeded[11] == 4091188469U);
    return 0;
}

int main(void)
{
    TEST_CHECK(check_legacy_sequence_and_default_seed() == 0);
    TEST_CHECK(check_reseeding_is_deterministic() == 0);
    TEST_CHECK(check_even_seeds_are_normalized_to_odd() == 0);
    return 0;
}
