#include "xpclient.h"

#include <stdio.h>
#include <string.h>

#define TEST_CHECK(condition)                                                \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "%s:%d: check failed: %s\n",                 \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct DecorCall {
    int x;
    int y;
    int xi;
    int yi;
    int type;
    bool last;
    bool more_y;
} DecorCall;

vdecor_t *vdecor_ptr;
int num_vdecor;
int max_vdecor;

static DecorCall calls[2];
static int call_count;

void Gui_paint_decor(int x, int y, int xi, int yi, int type,
                     bool last, bool more_y)
{
    DecorCall *call;

    if (call_count >= (int)NELEM(calls))
        return;
    call = &calls[call_count++];
    call->x = x;
    call->y = y;
    call->xi = xi;
    call->yi = yi;
    call->type = type;
    call->last = last;
    call->more_y = more_y;
}

static void reset_capture(void)
{
    memset(calls, 0, sizeof(calls));
    call_count = 0;
}

static int check_single_entry_does_not_consult_following_storage(void)
{
    vdecor_t entries[2] = {
        {11, 17, 23, 29, 31},
        {0, 0, 0, 999, 0}
    };

    reset_capture();
    vdecor_ptr = entries;
    num_vdecor = 1;
    max_vdecor = 2;

    Paint_vdecor();

    TEST_CHECK(call_count == 1);
    TEST_CHECK(calls[0].x == 11);
    TEST_CHECK(calls[0].y == 17);
    TEST_CHECK(calls[0].xi == 23);
    TEST_CHECK(calls[0].yi == 29);
    TEST_CHECK(calls[0].type == 31);
    TEST_CHECK(calls[0].last);
    TEST_CHECK(!calls[0].more_y);
    TEST_CHECK(num_vdecor == 0);
    TEST_CHECK(max_vdecor == 2);
    return 0;
}

static int check_only_nonfinal_entry_compares_the_next_row(void)
{
    vdecor_t entries[3] = {
        {1, 2, 3, 4, 5},
        {6, 7, 8, 9, 10},
        {0, 0, 0, 1234, 0}
    };

    reset_capture();
    vdecor_ptr = entries;
    num_vdecor = 2;
    max_vdecor = 3;

    Paint_vdecor();

    TEST_CHECK(call_count == 2);
    TEST_CHECK(!calls[0].last);
    TEST_CHECK(calls[0].more_y);
    TEST_CHECK(calls[1].last);
    TEST_CHECK(!calls[1].more_y);
    TEST_CHECK(num_vdecor == 0);
    TEST_CHECK(max_vdecor == 3);
    return 0;
}

int main(void)
{
    if (check_single_entry_does_not_consult_following_storage() != 0)
        return 1;
    return check_only_nonfinal_entry_compares_the_next_row();
}
