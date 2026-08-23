#include "test_helpers.h"

#include "xpcommon.h"
#include "utf8_names.h"

static int test_utf8_names_are_valid(void)
{
    static char latin_name[] = "Jos\xc3\xa9";
    static char japanese_name[] = "\xe6\x97\xa5\xe6\x9c\xac";

    TEST_CHECK(Check_utf8_user_name(latin_name) == NAME_OK);
    TEST_CHECK(Check_utf8_user_name(japanese_name) == NAME_OK);
    TEST_CHECK(Check_utf8_disp_name(latin_name) == NAME_OK);
    TEST_CHECK(Check_utf8_disp_name(japanese_name) == NAME_OK);
    TEST_CHECK(Check_user_name(latin_name) == NAME_ERROR);
    TEST_CHECK(Check_user_name(japanese_name) == NAME_ERROR);
    return 0;
}

static int test_utf8_name_boundaries_and_invalid_input(void)
{
    static const char five_characters[] =
        "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe5\x90\x8d\xe5\x89\x8d";
    static const char six_characters[] =
        "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe5\x90\x8d\xe5\x89\x8d"
        "\xe8\xa1\xa8";
    static const char malformed[] = "bad\xe3\x81";
    static const char bidi_override[] = "bad\xe2\x80\xae";
    char fixed[MAX_CHARS];

    TEST_CHECK(Check_utf8_user_name(five_characters) == NAME_OK);
    TEST_CHECK(Check_utf8_user_name(six_characters) == NAME_ERROR);
    strlcpy(fixed, six_characters, sizeof(fixed));
    Fix_utf8_user_name(fixed);
    TEST_CHECK(strcmp(fixed, five_characters) == 0);
    TEST_CHECK(Check_utf8_user_name(malformed) == NAME_ERROR);
    TEST_CHECK(Check_utf8_user_name(bidi_override) == NAME_ERROR);
    TEST_CHECK(Check_utf8_user_name("two words") == NAME_ERROR);
    strlcpy(fixed, malformed, sizeof(fixed));
    Fix_utf8_user_name(fixed);
    TEST_CHECK(strcmp(fixed, "X") == 0);
    TEST_CHECK(Check_utf8_disp_name("") == NAME_OK);
    return 0;
}

static int test_legacy_names_remain_ascii(void)
{
    char user[MAX_CHARS] = "\xe6\x97\xa5\xe6\x9c\xac";
    char display[MAX_CHARS] = "Jos\xc3\xa9";

    Fix_user_name(user);
    Fix_disp_name(display);
    TEST_CHECK(strcmp(user, "xxxxxx") == 0);
    TEST_CHECK(strcmp(display, "Josxx") == 0);
    TEST_CHECK(Check_user_name(user) == NAME_OK);
    TEST_CHECK(Check_disp_name(display) == NAME_OK);
    return 0;
}

int main(void)
{
    TEST_CHECK(test_utf8_names_are_valid() == 0);
    TEST_CHECK(test_utf8_name_boundaries_and_invalid_input() == 0);
    TEST_CHECK(test_legacy_names_remain_ascii() == 0);
    return 0;
}
