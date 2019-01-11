/* 
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/***************************************************************************\
*  winAudio.c - XPilotNT Windoze audio interface module						*
\***************************************************************************/
#ifdef SOUND

#include "windows.h"

#include "winAudio.h"
#include <io.h>
#include <fcntl.h>
#include <mmsystem.h>

#define	SNDQSIZE	32
static char *SndQ = NULL;
static long SndSize = 0;
static BOOL SndFlag = FALSE;
static HANDLE hPlayEvent = 0;

static DWORD Win32PlaySounds(LPVOID Arg)
{
    while (1) {
	if (WAIT_OBJECT_0 == WaitForSingleObject(hPlayEvent, 100000)) {
	    char *pFileName = SndQ;
	    ResetEvent(hPlayEvent);
	    SndQ = NULL;
	    PlaySound(pFileName, NULL,
		      SND_SYNC | SND_FILENAME | SND_NODEFAULT);
	}
    }
    return 0;
}

static long Win32GetFileSize(const char *filename)
{
    int fd = _open(filename, O_BINARY | O_RDONLY);
    long fileSize = -1L;
    if (fd >= 0) {
	fileSize = _filelength(fd);
	close(fd);
    }
    return fileSize;
}

int audioDeviceInit(char *display)
{
    DWORD ThreadId;
    HANDLE hThread;

    hPlayEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hPlayEvent == 0)
	return -1;

    if ((hThread =
	 CreateThread(NULL, 1000, (LPTHREAD_START_ROUTINE) Win32PlaySounds,
		      NULL, 0, &ThreadId)) == NULL) {
	CloseHandle(hPlayEvent);
	return -1;
    }
    SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
    return 0;
}

void audioDeviceEvents()
{
}

void audioDevicePlay(char *filename, int type, int volume, void **private)
{
    if (SndQ == NULL) {
	SndQ = filename;
	SndSize = Win32GetFileSize(filename);
    } else {
	long size = Win32GetFileSize(filename);
	if (size > SndSize) {
	    SndQ = filename;
	    SndSize = size;
	}
    }
    SetEvent(hPlayEvent);
}


#endif
