#include "test_helpers.h"

#include "xpcommon.h"
#include "netclient.h"

int main(void)
{
    TEST_CHECK(sizeof(Net_fd()) == sizeof(socket_handle_t));
    return 0;
}
