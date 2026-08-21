#include "test_helpers.h"

#include "session_token.h"

int main(void)
{
    session_token_t generated;
    session_token_t copy;

    TEST_CHECK(Session_token_generate(&generated));
    copy = generated;
    TEST_CHECK(Session_token_equal(&generated, &copy));
    copy.words[SESSION_TOKEN_WORDS - 1] ^= UINT32_C(1);
    TEST_CHECK(!Session_token_equal(&generated, &copy));
    TEST_CHECK(!Session_token_generate(NULL));
    TEST_CHECK(!Session_token_equal(NULL, &generated));
    TEST_CHECK(!Session_token_equal(&generated, NULL));
    return 0;
}
