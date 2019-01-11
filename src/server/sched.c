/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2004 by
 *
 *      Uoti Urpala          <uau@users.sourceforge.net>
 *      Erik Andersson
 *      Kristian Söderblom
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "xpserver.h"

/* Windows incorrectly uses u_int in FD_CLR */
#ifdef _WINDOWS
typedef	u_int	FDTYPE;
#else
typedef	int	FDTYPE;
#endif

#ifndef _WINDOWS
#define NUM_SELECT_FD		((int)sizeof(int) * 8)
#else
/*
    Windoze:
    The first call to socket() returns 560ish.  Successive calls keep bumping
    up the SOCKET returned until about 880 when it wraps back to 8.
    (It seems to increment by 8 with each connect - but that's not important)
    I can't find a manifest constant to tell me what the upper limit will be
    *sigh*

    --- Now, the Windoze gurus tell me that SOCKET is an opaque data type.
    So i need to make a lookup array for the lookup array :(
*/
#define	NUM_SELECT_FD		2000
#endif

struct io_handler {
    int			fd;
    void		(*func)(int, void *);
    void		*arg;
};

static struct io_handler	input_handlers[NUM_SELECT_FD];
static struct io_handler	record_handlers[NUM_SELECT_FD];
static fd_set			input_mask;
int				max_fd, min_fd;
static int			input_inited = false;

#if !defined(_WINDOWS)
static volatile bool sched_running = false;

void stop_sched(void)
{
	sched_running = false;
}
#endif

static void io_dummy(int fd, void *arg)
{
    xpprintf("io_dummy called!  (%d, %p)\n", fd, arg);
}

void install_input(void (*func)(int, void *), int fd, void *arg)
{
    int i;
    static struct io_handler *handlers;

    if (playback) {
	handlers = record_handlers;
	fd += min_fd;
    }
    else
	handlers = input_handlers;

    if (input_inited == false) {
	input_inited = true;
	FD_ZERO(&input_mask);
#ifndef _WINDOWS
	min_fd = fd;
#else
	min_fd = 0;
#endif
	max_fd = fd;
	for (i = 0; i < NELEM(input_handlers); i++) {
	    input_handlers[i].fd = -1;
	    input_handlers[i].func = io_dummy;
	    input_handlers[i].arg = 0;
	}
    }
    /* IFWINDOWS(xpprintf("install_input: fd %d min_fd=%d\n", fd, min_fd)); */
    if (!playback && (fd < min_fd || fd >= min_fd + NUM_SELECT_FD)) {
	error("install illegal input handler fd %d (%d)", fd, min_fd);
	exit(1);
    }
    if (!playback && FD_ISSET(fd, &input_mask)) {
	error("input handler %d busy", fd);
	exit(1);
    }
    handlers[fd - min_fd].fd = fd;
    handlers[fd - min_fd].func = func;
    handlers[fd - min_fd].arg = arg;
    if (playback)
	return;
    FD_SET(fd, &input_mask);
    if (fd > max_fd) {
	max_fd = fd;
    }
}

void remove_input(int fd)
{
    if (!playback) {
	if (fd < min_fd || fd >= min_fd + NUM_SELECT_FD) {
	    error("remove illegal input handler fd %d (%d)", fd, min_fd);
	    exit(1);
	}
	if (FD_ISSET(fd, &input_mask) || playback) {
	    input_handlers[fd - min_fd].fd = -1;
	    input_handlers[fd - min_fd].func = io_dummy;
	    input_handlers[fd - min_fd].arg = 0;
	    FD_CLR((FDTYPE)fd, &input_mask);
	    if (fd == max_fd) {
		int i = fd;
		max_fd = -1;
		while (--i >= min_fd) {
		    if (FD_ISSET(i, &input_mask)) {
			max_fd = i;
			break;
		    }
		}
	    }
	}
    }
    else {
	record_handlers[fd].fd = -1;
	record_handlers[fd].func = io_dummy;
	record_handlers[fd].arg = 0;
    }
}

static void sched_select_error(void)
{
    error("sched select error");

    End_game();
}

#ifdef SELECT_SCHED

static long	timer_freq;
static void	(*timer_handler)(void);
static double	frametime;	/* time between 2 frames in seconds */
static double	t_nextframe;

static void setup_timer(void)
{
    struct timeval tv;
    double t;

    if (timer_freq <= 0 || timer_freq > MAX_SERVER_FPS) {
	error("illegal timer frequency: %ld", timer_freq);
	exit(1);
    }

    frametime = 1.0 / (double)timer_freq;

    gettimeofday(&tv, NULL);
    t = timeval_to_seconds(&tv);
    t_nextframe = t + frametime;
}

/*
 * Configure timer tick callback.
 */
void install_timer_tick(void (*func)(void), int freq)
{
    if (func != NULL) /* NULL to change freq, keep same handler */
	timer_handler = func;
    timer_freq = freq;
    setup_timer();
}


/*
 * If you set skip_to the server calculates frames
 * until that value of main_loops as fast as it can.
 * If you manage to record a bug, you can got to the
 * frame where it happens quickly.
 */
unsigned long skip_to = 0;

/*
 * I/O + timer dispatcher.
 */
void sched(void)
{
    int i, n;
    double t_now, t_wait;
    struct timeval tv, wait_tv;

    playback = rplayback;

    if (sched_running)
	dumpcore("sched already running");
    else
	sched_running = true;

    gettimeofday(&tv, NULL);
    t_now = timeval_to_seconds(&tv);
    t_nextframe = t_now + frametime;

    while (sched_running) {
	fd_set readmask = input_mask;

	gettimeofday(&tv, NULL);
	t_now = timeval_to_seconds(&tv);
	t_wait = t_nextframe - t_now;

	/* heuristics for different cases */
	if (t_wait < 0) {
	    if (t_wait < -2 * frametime) {
		/* long freeze, schedule frame now */
		t_nextframe = t_now;
		t_wait = 0;
	    } else
		t_wait = 0;
	} else {
	    if (t_wait > 2 * frametime) {
		/* nasty, someone changed the time! might aswell start over */
		t_nextframe = t_now + frametime;
		t_wait = frametime;
	    }
	}

	if (main_loops < skip_to) {
	    t_nextframe = t_now;
	    t_wait = 0;
	}
	
	/* RECORDING STUFF */
	Handle_recording_buffers();
	/* RECORDING STUFF END */

	wait_tv = seconds_to_timeval(t_wait);
	n = select(max_fd + 1, &readmask, NULL, NULL, &wait_tv);

	if (n <= 0) {
	    if (n == -1 && errno != EINTR)
		sched_select_error();

	    /* RECORDING STUFF */
	    if (playback) {
		while (*playback_sched) {
		    if (*playback_sched == 127) {
			playback_sched++;
			Get_recording_data();
		    }
		    else {
			struct io_handler *ioh;
			ioh = &record_handlers[*playback_sched++ - 1];
			(*(ioh->func))(ioh->fd, ioh->arg);
		    }
		}
		playback_sched++;
	    }
	    else if (record)
		*playback_sched++ = 0;
	    /* RECORDING STUFF END */

	    if (timer_handler)
		(*timer_handler)();

#if 1
	    /* stable 50 fps as deity (2.6.x kernel) */
	    t_nextframe += frametime - 0.0000028571;
#else
	    t_nextframe += frametime;
#endif
	}
	else {
	    for (i = max_fd; i >= min_fd; i--) {
		if (FD_ISSET(i, &readmask)) {
		    struct io_handler *ioh;

		    /* RECORDING STUFF */
		    record = playback = 0;
		    if (rrecord && (i - min_fd > 0)) {
			if (i - min_fd + 1 > 126) { /* 127 reserved */
			    warn("recording: this shouldn't happen");
			    exit(1);
			}
			*playback_sched++ = i - min_fd + 1;
			record = 1;
		    }
		    /* RECORDING STUFF END */

		    ioh = &input_handlers[i - min_fd];
		    (*(ioh->func))(ioh->fd, ioh->arg);

		    /* RECORDING STUFF */
		    record = rrecord;
		    playback = rplayback;
		    /* RECORDING STUFF END */

		    if (--n == 0)
			break;
		}
	    }
	}
    }
}

#else /* SELECT_SCHED */

static volatile long	timer_ticks;	/* SIGALRMs that have occurred */
static long		timers_used;	/* SIGALRMs that have been used */
static long		timer_freq;	/* rate at which timer ticks. (in FPS) */
#ifndef _WINDOWS
static void		(*timer_handler)(void);
#else
static	TIMERPROC	timer_handler;
#endif
static time_t		current_time;
static int		ticks_till_second;

/*
 * Block or unblock a single signal.
 */
static void sig_ok(int signum, int flag)
{
#if !defined(_WINDOWS)
    sigset_t    sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, signum);
    if (sigprocmask((flag) ? SIG_UNBLOCK : SIG_BLOCK, &sigset, NULL) == -1) {
	error("sigprocmask(%d,%d)", signum, flag);
	exit(1);
    }
#endif
}

/*
 * Prevent the real-time timer from interrupting system calls.
 * Globally accessible.
 */
void block_timer(void)
{
#ifndef _WINDOWS
    sig_ok(SIGALRM, 0);
#endif
}

/*
 * Unblock the real-time timer.
 * Globally accessible.
 */
void allow_timer(void)
{
#ifndef _WINDOWS
    sig_ok(SIGALRM, 1);
#endif
}


/*
 * Catch SIGALRM.
 */
static void catch_timer(int signum)
{
    static unsigned int		timer_count = 0;

    UNUSED_PARAM(signum);
    timer_count += FPS;
    if (timer_count >= (unsigned)options.timerResolution) {
	timer_count -= options.timerResolution;
	timer_ticks++;
	if (timer_count >= (unsigned)options.timerResolution)
	    /*
	     * Don't let timer_count grow boundlessly with timer resolution 0
	     * now that timer resolution can be changed at runtime.
	     */
	    timer_count = 0;
    }
}


/*
 * Setup the handling of the SIGALRM signal
 * and setup the real-time interval timer.
 */
static void setup_timer(void)
{
#ifndef _WINDOWS

    struct itimerval itv;
    struct sigaction act;

    /*
     * Prevent SIGALRMs from disturbing the initialization.
     */
    block_timer();

    /*
     * Install a signal handler for the alarm signal.
     */
    act.sa_handler = catch_timer;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);
    if (sigaction(SIGALRM, &act, (struct sigaction *)NULL) == -1) {
	error("sigaction SIGALRM");
	exit(1);
    }

    /*
     * Install a real-time timer.
     */
    if (timer_freq <= 0 || timer_freq > MAX_SERVER_FPS) {
	error("illegal timer frequency: %ld", timer_freq);
	exit(1);
    }

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 1000000 / timer_freq;
    itv.it_value = itv.it_interval;
    if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
	error("setitimer");
	exit(1);
    }

    timers_used = timer_ticks;
    time(&current_time);
    ticks_till_second = timer_freq;
#else
/*
    UINT cr = SetTimer(NULL, 0, 1000/timer_freq, timer_handler);
    UINT cr = SetTimer(NULL, 0, 20, (TIMERPROC)ServerThreadTimerProc);
    if (!cr)
	error("Can't create timer");
*/
#endif
    /*
     * Allow the real-time timer to generate SIGALRM signals.
     */
    allow_timer();
}

/*
 * Configure timer tick callback.
 */
#ifndef _WINDOWS
void install_timer_tick(void (*func)(void), int freq)
{
    if (func != NULL) /* NULL to change freq, keep same handler */
	timer_handler = func;
    timer_freq = freq;
    setup_timer();
}
#else

typedef void (__stdcall *windows_timer_t)(void *, unsigned int, unsigned int, unsigned long);

void install_timer_tick(windows_timer_t func, int freq)
{
    if (func != NULL)
	timer_handler = (TIMERPROC)func;
    timer_freq = freq;
    setup_timer();
}
#endif


/*
 * Linked list of timeout callbacks.
 */
struct to_handler {
    struct to_handler	*next;
    time_t		when;
    void		(*func)(void *);
    void		*arg;
};
static struct to_handler *to_busy_list = NULL;
static struct to_handler *to_free_list = NULL;
static int		to_min_free = 3;
static int		to_max_free = 5;
static int		to_cur_free = 0;

static void to_fill(void)
{
    if (to_cur_free < to_min_free) {
	do {
	    struct to_handler *top =
		(struct to_handler *)malloc(sizeof(struct to_handler));
	    if (!top) {
		break;
	    }
	    top->next = to_free_list;
	    to_free_list = top;
	    to_cur_free++;
	} while (to_cur_free < to_max_free);
    }
}

static struct to_handler *to_alloc(void)
{
    struct to_handler *top;

    to_fill();
    if (!to_free_list) {
	error("Not enough memory for timeouts");
	exit(1);
    }

    top = to_free_list;
    to_free_list = top->next;
    to_cur_free--;
    top->next = 0;

    return top;
}

static void to_free(struct to_handler *top)
{
    if (to_cur_free < to_max_free) {
	top->next = to_free_list;
	to_free_list = top;
	to_cur_free++;
    }
    else {
	free(top);
    }
}

/*
 * Configure timout callback.
 */
void install_timeout(void (*func)(void *), int offset, void *arg)
{
    struct to_handler *top = to_alloc();
    top->func = func;
    top->when = current_time + offset;
    top->arg = arg;
    if (!to_busy_list || to_busy_list->when >= top->when) {
	top->next = NULL;
	to_busy_list = top;
    }
    else {
	struct to_handler *prev = to_busy_list;
	struct to_handler *lp = prev->next;
	while (lp && lp->when < top->when) {
	    prev = lp;
	    lp = lp->next;
	}
	top->next = lp;
	prev->next = top;
    }
}

void remove_timeout(void (*func)(void *), void *arg)
{
    struct to_handler *prev = 0;
    struct to_handler *lp = to_busy_list;
    while (lp) {
	if (lp->func == func && lp->arg == arg) {
	    struct to_handler *top = lp;
	    lp = lp->next;
	    if (prev) {
		prev->next = lp;
	    } else {
		to_busy_list = lp;
	    }
	    to_free(top);
	}
	else {
	    prev = lp;
	    lp = lp->next;
	}
    }
}

static void timeout_chime(void)
{
    while (to_busy_list && to_busy_list->when <= current_time) {
	struct to_handler *top = to_busy_list;
	void (*func)(void *) = top->func;
	void *arg = top->arg;
	to_busy_list = top->next;
	to_free(top);
	(*func)(arg);
    }
}

/*
 * I/O + timer dispatcher.
 * Windows pumps this one time
 */

long skip_to = 0;

#ifndef _WINDOWS
void sched(void)
{
    int			i, n, io_todo = 3;
    struct timeval	tv, *tvp = &tv;

    playback = rplayback;

    if (sched_running)
	dumpcore("sched already running");
    else
	sched_running = true;

    while (sched_running) {

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (main_loops < skip_to && timers_used >= timer_ticks)
	    timer_ticks++;
	if (io_todo == 0 && timers_used < timer_ticks) {
	    io_todo = 1 + (timer_ticks - timers_used);
	    tvp = &tv;
	    if (playback) {
		while (*playback_sched) {
		    if (*playback_sched == 127) {
			playback_sched++;
			Get_recording_data();
		    }
		    else {
			struct io_handler *ioh;
			ioh = &record_handlers[*playback_sched++ - 1];
			(*(ioh->func))(ioh->fd, ioh->arg);
		    }
		}
		playback_sched++;
	    }
	    else if (record)
		*playback_sched++ = 0;

	    if (timer_handler)
		(*timer_handler)();

	    do {
		++timers_used;
		if (--ticks_till_second <= 0) {
		    ticks_till_second += timer_freq;
		    current_time++;
		    timeout_chime();
		}
	    } while (timers_used + 1 < timer_ticks);
	}
	else {
	    fd_set readmask;
	    readmask = input_mask;
	    Handle_recording_buffers();
	    n = select(max_fd + 1, &readmask, 0, 0, tvp);
	    if (n <= 0) {
		if (n == -1 && errno != EINTR)
		    sched_select_error();
		io_todo = 0;
	    }
	    else {
		for (i = max_fd; i >= min_fd; i--) {
		    if (FD_ISSET(i, &readmask)) {
			struct io_handler *ioh;

			record = playback = 0;
			if (rrecord && (i - min_fd > 0)) {
			    if (i - min_fd + 1 > 126) { /* 127 reserved */
				warn("recording: this shouldn't happen");
				exit(1);
			    }
			    *playback_sched++ = i - min_fd + 1;
			    record = 1;
			}
			ioh = &input_handlers[i - min_fd];
			(*(ioh->func))(ioh->fd, ioh->arg);
			record = rrecord;
			playback = rplayback;
			if (--n == 0)
			    break;
		    }
		}
		if (io_todo > 0)
		    io_todo--;
	    }
	    if (io_todo == 0)
		tvp = NULL;
	}
    }
}

#else /* _WINDOWS */
void sched(void)
{
    int			i, n, io_todo = 3;
    struct timeval	tv, *tvp = &tv;

    if (NumPlayers > NumRobots + NumPseudoPlayers
	|| login_in_progress != 0
	|| NumQueuedPlayers > 0) {

	/* need fast I/O checks now! (2 or 3 times per frames) */
	tv.tv_sec = 0;
	/* KOERBER */
	/*	tv.tv_usec = 1000000 / (3 * timer_freq + 1); */
	tv.tv_usec = 1000000 / (10 * timer_freq + 1);
    }
    else {
	/* slow I/O checks are possible here... (2 times per second) */
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
    }


    if (io_todo == 0 && timers_used < timer_ticks) {
	io_todo = 1 + (timer_ticks - timers_used);
	tvp = &tv;

	do {
	    ++timers_used;
	    if (--ticks_till_second <= 0) {
		ticks_till_second += timer_freq;
		current_time++;
		timeout_chime();
	    }
	} while (timers_used + 1 < timer_ticks);
    }
    else {
	fd_set readmask;
	readmask = input_mask;
	n = select(max_fd + 1, &readmask, 0, 0, tvp);
	if (n <= 0) {
	    if (n == -1 && errno != EINTR) {
		sched_select_error();
	    }
	    io_todo = 0;
	}
	else {
	    for (i = max_fd; i >= min_fd; i--) {
		if (FD_ISSET(i, &readmask)) {
		    struct io_handler *ioh;
		    ioh = &input_handlers[i - min_fd];
		    (*(ioh->func))(ioh->fd, ioh->arg);
		    if (--n == 0) {
			break;
		    }
		}
	    }
	    if (io_todo > 0) {
		io_todo--;
	    }
	}
	if (io_todo == 0) {
	    tvp = NULL;
	}
    }
}

#endif /* _WINDOWS */

#endif /* SELECT_SCHED */
