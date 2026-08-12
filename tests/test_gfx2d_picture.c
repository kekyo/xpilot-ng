#include "test_helpers.h"

#include "xpcommon.h"
#include "gfx2d.h"

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIXTURE_WIDTH 4
#define FIXTURE_HEIGHT 4

typedef struct {
    char path[PATH_MAX + 1];
    char directory[PATH_MAX + 1];
    char filename[PATH_MAX + 1];
} picture_fixture_t;

static int fail_next_malloc;

void *__real_malloc(size_t size);

void *__wrap_malloc(size_t size)
{
    if (fail_next_malloc) {
        fail_next_malloc = 0;
        return NULL;
    }
    return __real_malloc(size);
}

static int count_open_descriptors(void)
{
    DIR *directory = opendir("/proc/self/fd");
    struct dirent *entry;
    int count = 0;

    if (directory == NULL)
        return -1;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0
            && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    closedir(directory);
    return count;
}

static int write_all(int fd, const void *data, size_t size)
{
    const unsigned char *cursor = data;

    while (size > 0) {
        ssize_t written = write(fd, cursor, size);

        if (written < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (written == 0)
            return -1;
        cursor += written;
        size -= (size_t)written;
    }
    return 0;
}

static int create_picture_fixture(picture_fixture_t *fixture)
{
    static const char header[] = "P6\n4 4\n255\n";
    unsigned char pixels[FIXTURE_WIDTH * FIXTURE_HEIGHT * 3];
    const char *temporary_directory = getenv("TMPDIR");
    const char *slash;
    size_t directory_length;
    int fd;
    int length;
    size_t i;

    if (temporary_directory == NULL || temporary_directory[0] == '\0')
        temporary_directory = "/tmp";
    length = snprintf(fixture->path, sizeof(fixture->path),
                      "%s/xpilot-gfx2d-picture.XXXXXX",
                      temporary_directory);
    if (length < 0 || (size_t)length >= sizeof(fixture->path))
        return -1;

    fd = mkstemp(fixture->path);
    if (fd < 0)
        return -1;

    slash = strrchr(fixture->path, PATHNAME_SEP);
    if (slash == NULL)
        goto failure;
    directory_length = (size_t)(slash - fixture->path);
    if (directory_length == 0
        || directory_length >= sizeof(fixture->directory)
        || strlen(slash + 1) >= sizeof(fixture->filename))
        goto failure;
    memcpy(fixture->directory, fixture->path, directory_length);
    fixture->directory[directory_length] = '\0';
    memcpy(fixture->filename, slash + 1, strlen(slash + 1) + 1);

    for (i = 0; i < sizeof(pixels); i++)
        pixels[i] = (unsigned char)(i + 1);
    if (write_all(fd, header, sizeof(header) - 1) != 0
        || write_all(fd, pixels, sizeof(pixels)) != 0) {
        close(fd);
        goto failure_closed;
    }
    if (close(fd) != 0)
        goto failure_closed;
    return 0;

failure:
    close(fd);
failure_closed:
    unlink(fixture->path);
    fixture->path[0] = '\0';
    return -1;
}

static int check_picture_is_empty(const xp_picture_t *picture)
{
    TEST_CHECK(picture->width == 0);
    TEST_CHECK(picture->height == 0);
    TEST_CHECK(picture->count == 0);
    TEST_CHECK(picture->data == NULL);
    TEST_CHECK(picture->bbox == NULL);
    return 0;
}

static int cleanup_twice_and_check(xp_picture_t *picture)
{
    Picture_cleanup(picture);
    if (check_picture_is_empty(picture) != 0)
        return 1;
    Picture_cleanup(picture);
    return check_picture_is_empty(picture);
}

static int check_horizontal_frames_are_owned(const char *filename)
{
    xp_picture_t picture = {0};

    TEST_CHECK(Picture_init(&picture, filename, -2) == 0);
    TEST_CHECK(picture.width == FIXTURE_WIDTH / 2);
    TEST_CHECK(picture.height == FIXTURE_HEIGHT);
    TEST_CHECK(picture.count == -2);
    TEST_CHECK(picture.data != NULL);
    TEST_CHECK(picture.data[0] != NULL);
    TEST_CHECK(picture.data[1] != NULL);
    TEST_CHECK(picture.bbox != NULL);
    return cleanup_twice_and_check(&picture);
}

static int check_rotated_frames_are_owned(const char *filename)
{
    xp_picture_t picture = {0};
    int image;

    TEST_CHECK(Picture_init(&picture, filename, 4) == 0);
    TEST_CHECK(picture.width == FIXTURE_WIDTH);
    TEST_CHECK(picture.height == FIXTURE_HEIGHT);
    TEST_CHECK(picture.count == 4);
    TEST_CHECK(picture.data != NULL);
    for (image = 0; image < picture.count; image++)
        TEST_CHECK(picture.data[image] != NULL);
    TEST_CHECK(picture.bbox != NULL);
    return cleanup_twice_and_check(&picture);
}

static int check_missing_picture_fails_atomically(const char *filename)
{
    xp_picture_t picture = {0};

    TEST_CHECK(Picture_init(&picture, filename, 4) == -1);
    if (check_picture_is_empty(&picture) != 0)
        return 1;
    return cleanup_twice_and_check(&picture);
}

static int check_pixel_allocation_failure_closes_file(const char *filename)
{
    xp_picture_t picture = {0};
    int descriptors_before = count_open_descriptors();

    TEST_CHECK(descriptors_before >= 0);
    fail_next_malloc = 1;
    TEST_CHECK(Picture_init(&picture, filename, -2) == -1);
    TEST_CHECK(fail_next_malloc == 0);
    TEST_CHECK(check_picture_is_empty(&picture) == 0);
    TEST_CHECK(count_open_descriptors() == descriptors_before);
    return cleanup_twice_and_check(&picture);
}

int main(void)
{
    picture_fixture_t fixture = {{0}, {0}, {0}};
    int result = 1;

    if (create_picture_fixture(&fixture) != 0) {
        perror("create_picture_fixture");
        return 1;
    }

    realTexturePath = fixture.directory;
    Make_table();
    if (check_horizontal_frames_are_owned(fixture.filename) != 0)
        goto cleanup;
    if (check_rotated_frames_are_owned(fixture.filename) != 0)
        goto cleanup;
    if (check_pixel_allocation_failure_closes_file(fixture.filename) != 0)
        goto cleanup;
    if (unlink(fixture.path) != 0) {
        perror("unlink");
        goto cleanup;
    }
    fixture.path[0] = '\0';
    if (check_missing_picture_fails_atomically(fixture.filename) != 0)
        goto cleanup;
    result = 0;

cleanup:
    realTexturePath = NULL;
    if (fixture.path[0] != '\0')
        unlink(fixture.path);
    return result;
}
