/* 
 * Adapted from 'The UNIX Programming Environment' by Kernighan & Pike
 * and an example from the manualpage for vprintf by
 * Gaute Nessan, University of Tromsoe (gaute@staff.cs.uit.no).
 *
 * Modified by Bjoern Stabell <bjoern@xpilot.org>.
 * Windows mods and memory leak detection by Dick Balaska <dick@xpilot.org>.
 */
#include "xpcommon.h"

/*
 * This file defines several entry points:
 *
 * init_error()		- Initialize the error routine, accepts program name
 *			  as input.
 * error()		- perror() with printf functionality.
 * warn(), ...
 */

/*
 * File local static data.
 */
#define	MAX_PROG_LENGTH	32
static char progname[MAX_PROG_LENGTH];

static const char *prog_basename(const char *prog)
{
#ifndef _WINDOWS
    char *p;

    p = strrchr(prog, '/');

    return (p != NULL) ? (p + 1) : prog;
#else
    return "XPilot NG";
#endif
}


/*
 * Functions.
 */
void init_error(const char *prog)
{
    const char *p = prog_basename(prog);   /* Beautify argv[0] */

    strlcpy(progname, p, MAX_PROG_LENGTH);
}

#ifndef _WINDOWS

/*
 * Ok, let's do it the ANSI C way.
 */
void xpinfo(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
	fprintf(stderr, "%s: INFO: ", progname);

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
	fprintf(stderr, "\n");

    va_end(ap);
}

void warn(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
	fprintf(stderr, "%s: WARNING: ", progname);

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
	fprintf(stderr, "\n");

    va_end(ap);
}

void error(const char *fmt, ...)
{
    size_t len;
    va_list ap;
    int e = errno;

    va_start(ap, fmt);

    if (progname[0] != '\0')
	fprintf(stderr, "%s: ERROR: ", progname);

    vfprintf(stderr, fmt, ap);

    if (e != 0)
	fprintf(stderr, ": (%s)", strerror(e));

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
	fprintf(stderr, "\n");

    va_end(ap);
}

void fatal(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
	fprintf(stderr, "%s: FATAL: ", progname);

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
	fprintf(stderr, "\n");

    va_end(ap);

    exit(1);
}

void dumpcore(const char *fmt, ...)
{
    size_t len;
    va_list ap;

    va_start(ap, fmt);

    if (progname[0] != '\0')
	fprintf(stderr, "%s: ABORT: ", progname);

    vfprintf(stderr, fmt, ap);

    len = strlen(fmt);
    if (len == 0 || fmt[len - 1] != '\n')
	fprintf(stderr, "\n");

    va_end(ap);

    abort();
}
#endif /* _WINDOWS */

#ifdef _WINDOWS
static void Win_show_error(char *s)
{
    static int inerror = FALSE;
    Trace("Error: %s\n", s);
    if (inerror) return;
    inerror = TRUE;
    {
#ifdef   _XPILOTNTSERVER_
	/* putting up a message box on the server is a bad thing.
	   It kinda halts the server, which is a bad thing to do for
	   the simple info messages (nick in use) that call this routine
	*/
	xpprintf("%s %s\n", showtime(), s);
#else
	/*
	if (MessageBox(NULL, s, "Error", MB_OKCANCEL | MB_TASKMODAL)
	    == IDCANCEL) {
# ifdef   _XPMON_
	    xpmemShutdown();
# endif
	    ExitProcess(1);
	}
	*/
#endif
    }
    /* kps - moved out from ifdef block, seems to be a better idea. */
    inerror = FALSE;
}

void xpinfo(const char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);

    vsprintf(s, fmt, ap);

    Win_show_error(s);

    va_end(ap);
}

void error(const char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);

    vsprintf(s, fmt, ap);

    Win_show_error(s);

    va_end(ap);
}

void warn(const char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);

    vsprintf(s, fmt, ap);

    Win_show_error(s);

    va_end(ap);
}

void fatal(const char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);

    vsprintf(s, fmt, ap);

    Win_show_error(s);

    va_end(ap);

    exit(1);
}

void dumpcore(const char *fmt, ...)
{
    va_list ap;
    char s[512];

    va_start(ap, fmt);

    vsprintf(s, fmt, ap);

    Win_show_error(s);

    va_end(ap);

    exit(1);
}

#endif
