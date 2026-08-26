/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by the XPilot Authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "test_helpers.h"
#include "xpclient.h"

#include <AL/al.h>
#include <AL/alut.h>

long loops;

static int alut_init_calls;
static int openal_calls_after_failed_init;
static int cleanup_calls;
static ALenum error_string_code;

ALboolean ALUT_APIENTRY __wrap_alutInit(int *argcp, char **argv)
{
    UNUSED_PARAM(argcp);
    UNUSED_PARAM(argv);
    alut_init_calls++;
    return AL_FALSE;
}

ALenum ALUT_APIENTRY __wrap_alutGetError(void)
{
    return ALUT_ERROR_OPEN_DEVICE;
}

const char *ALUT_APIENTRY __wrap_alutGetErrorString(ALenum error_code)
{
    error_string_code = error_code;
    return "no audio device";
}

ALboolean ALUT_APIENTRY __wrap_alutExit(void)
{
    cleanup_calls++;
    return AL_TRUE;
}

void AL_APIENTRY __wrap_alListenerf(ALenum parameter, ALfloat value)
{
    UNUSED_PARAM(parameter);
    UNUSED_PARAM(value);
    openal_calls_after_failed_init++;
}

void AL_APIENTRY __wrap_alDopplerFactor(ALfloat value)
{
    UNUSED_PARAM(value);
    openal_calls_after_failed_init++;
}

void AL_APIENTRY __wrap_alSpeedOfSound(ALfloat value)
{
    UNUSED_PARAM(value);
    openal_calls_after_failed_init++;
}

ALenum AL_APIENTRY __wrap_alGetError(void)
{
    openal_calls_after_failed_init++;
    return AL_NO_ERROR;
}

void AL_APIENTRY __wrap_alGenSources(ALsizei count, ALuint *sources)
{
    UNUSED_PARAM(count);
    UNUSED_PARAM(sources);
    openal_calls_after_failed_init++;
}

void AL_APIENTRY __wrap_alDeleteSources(ALsizei count, const ALuint *sources)
{
    UNUSED_PARAM(count);
    UNUSED_PARAM(sources);
    cleanup_calls++;
}

int main(void)
{
    init_error("test-audio-device");

    TEST_CHECK(audioDeviceInit(NULL) == -1);
    TEST_CHECK(alut_init_calls == 1);
    TEST_CHECK(error_string_code == ALUT_ERROR_OPEN_DEVICE);
    TEST_CHECK(openal_calls_after_failed_init == 0);

    audioDeviceClose();
    TEST_CHECK(cleanup_calls == 0);

    return 0;
}
