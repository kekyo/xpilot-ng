dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([XPilot NG], 4.7.3, xpilot-hacks@lists.sourceforge.net, xpilot-ng)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/client/xpclient.h)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

dnl  Copyright & stuff.
XP_AUTHORS="Bjørn Stabell, Ken Ronny Schouten, Bert Gijsbers, Dick Balaska, Uoti Urpala, Juha Lindström, Kristian Söderblom and Erik Andersson"
AC_SUBST([XP_AUTHORS])

XP_COPYRIGHTDATE="1991-2005"
AC_SUBST([XP_COPYRIGHTDATE])

XP_COPYRIGHT="Copyright © $XP_COPYRIGHTDATE by $XP_AUTHORS"
AC_SUBST([XP_COPYRIGHT])

AC_SUBST([XP_RELEASEDATE],["Sep 16, 2005"])
AC_SUBST([XP_DATADIR],"$prefix/share/xpilot-ng")

AC_ARG_ENABLE(plockserver,
	AC_HELP_STRING([--enable-plockserver],
			[lock the xpilot-ng server into memory]),
[if test x$enable_plockserver = xyes; then
	AC_DEFINE([PLOCKSERVER], 1, [lock the xpilot-ng server into memory])
fi])

AC_ARG_ENABLE(development,
	AC_HELP_STRING([--enable-development],
			[enable development code]),
[if test x$enable_development = xyes; then
	AC_DEFINE([DEVELOPMENT], 1, [enable development code])
fi])

AC_ARG_ENABLE(select_sched,
	AC_HELP_STRING([--enable-select-sched],
		[enable new server scheduling most useful on Linux 2.6]))

AC_ARG_ENABLE(sdl_client,
	AC_HELP_STRING([--disable-sdl-client],
			[disable SDL/OpenGL client]),
[], [enable_sdl_client=yes])

AC_ARG_ENABLE(sound,
	AC_HELP_STRING([--enable-sound],
			[enable OpenAL sound]), [])

dnl  Checks for programs.
AC_PROG_CC
dnl AC_PROG_GCC_TRADITIONAL
dnl AC_PROG_CPP

dnl Needed if you want to compile with g++ e.g.
dnl AC_PROG_CXX
dnl AC_PROG_CXXCPP

dnl AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

dnl  Checks for libraries.
dnl kps - these should be cleaned up and not use the autoconf internal
dnl shell variables.
AC_CHECK_LIB(m, cos)
if test x$ac_cv_lib_m_cos != xyes; then
	AC_MSG_ERROR([*** Math library not found!])
fi
AC_CHECK_LIB(z, gzopen)
if test x$ac_cv_lib_z_gzopen != xyes; then
	AC_MSG_ERROR([*** Required library zlib not found!])
fi
AC_CHECK_LIB(expat, XML_ParserCreate)
if test x$ac_cv_lib_expat_XML_ParserCreate != xyes; then
	AC_MSG_ERROR([*** Required library Expat not found!])
fi

case "$target" in
    *-*-mingw32*)
	windows_target=yes
	;;
esac

if test x$windows_target = xyes; then
	W32_LIBS="-lmingw32 -lwsock32"
	AC_SUBST(W32_LIBS)
	AC_DEFINE([_WINDOWS], 1, [compiling for windows target])

	enable_select_sched=yes
fi

if test x$enable_select_sched = xyes; then
	AC_DEFINE([SELECT_SCHED], 1,
		[enable new server scheduling most useful on Linux 2.6])
fi

dnl Libraries needed by the SDL client.
if test x$enable_sdl_client = xyes; then
	PKG_CHECK_MODULES([SDL2], [sdl2 >= 2.0.9])
	PKG_CHECK_MODULES([SDL2_IMAGE], [SDL2_image >= 2.0.0])
	PKG_CHECK_MODULES([SDL2_TTF], [SDL2_ttf >= 2.0.12])
	PKG_CHECK_MODULES([GL], [gl])
	AC_DEFINE([HAVE_SDL_IMAGE], 1,
		  [Define to 1 if you have the SDL_image library.])
fi

AM_CONDITIONAL([COND_CLIENT], [test x$enable_sdl_client = xyes])
AM_CONDITIONAL([COND_WINDOWS], [test x$windows_target = xyes])
AM_CONDITIONAL([COND_SDL_CLIENT], [test x$enable_sdl_client = xyes])

dnl OpenAL client sound support.
if test x$enable_sound = xyes; then
	have_openal=no
	have_alut=no
	have_sound=no
	AC_CHECK_LIB(
		openal, 
		main, 
		[have_openal=yes],
		[])
	AC_CHECK_LIB(
		alut, 
		main, 
		[have_alut=yes],
		[])
	if test x$have_openal = xyes; then
		if test x$have_alut = xyes; then
			have_sound=yes
			SOUND_LIBS="-lopenal -lalut"
			AC_DEFINE([SOUND], 1, [Define to 1 if you want OpenAL sound.])
		else
			echo "*** Client sound disabled. Check that you have OpenAL installed."
		fi
	fi
	AC_SUBST(SOUND_LIBS)
fi

AM_CONDITIONAL([COND_SOUND], [test x$have_sound = xyes])

dnl  Checks for header files.
dnl AC_HEADER_STDC
AC_HEADER_TIME
dnl AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
	arpa/inet.h \
	arpa/nameser.h \
	assert.h \
	bstring.h \
	ctype.h \
	errno.h \
	fcntl.h \
	float.h \
	limits.h \
	math.h \
	net/if.h \
	netdb.h \
	netinet/in.h \
	netinet/tcp.h \
	pwd.h \
	resolv.h \
	setjmp.h \
	signal.h \
	stdarg.h \
	stddef.h \
	sys/file.h \
	sys/filio.h \
	sys/fcntl.h \
	sys/inttypes.h \
	sys/ioctl.h \
	sys/lock.h \
	sys/mman.h \
	sys/param.h \
	sys/socket.h \
	sys/sockio.h \
	sys/time.h \
	values.h \
	])

dnl  Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_MODE_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Check for socklen_t (in Unix98)
AC_MSG_CHECKING(for socklen_t)
AC_TRY_COMPILE([
	#include <sys/types.h>
	#include <sys/socket.h>
	socklen_t x;
], [],
[
	AC_MSG_RESULT(yes)
], [
	AC_TRY_COMPILE([
		#include <sys/types.h>
		#include <sys/socket.h>
		int accept(int, struct sockaddr *, size_t *);
	], [], [
		AC_MSG_RESULT(size_t)
		AC_DEFINE(socklen_t, size_t, [socklen_t size])
	], [
		AC_MSG_RESULT(int)
		AC_DEFINE(socklen_t, int, [socklen_t size])
	])
])

dnl  Checks for library functions.

dnl We probably don't need GNU compatible malloc and realloc functions,
dnl so these are disabled for now.
dnl AC_FUNC_MALLOC
dnl AC_FUNC_REALLOC

AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ \
	alarm \
	floor \
	gethostbyaddr \
	gethostbyname \
	gethostname \
	gettimeofday \
	inet_ntoa \
	isascii \
	memchr \
	memmove \
	memset \
	mkdir \
	pow \
	rint \
	select \
	socket \
	sqrt \
	strcasecmp \
	strchr \
	strdup \
	strerror \
	strlcat \
	strlcpy \
	strncasecmp \
	strpbrk \
	strrchr \
	strspn \
	strstr \
	strtol \
	strtoul \
	])

AC_CONFIG_FILES([ \
	Makefile \
	README \
	contrib/Makefile \
	contrib/xpngcc/Makefile \
	doc/Makefile \
	doc/man/Makefile \
	lib/Makefile \
	lib/fonts/Makefile \
	lib/maps/Makefile \
	lib/textures/Makefile \
	lib/sound/Makefile \
	src/Makefile \
	src/common/Makefile \
	src/common/version.h \
	src/common/NT/Makefile \
	src/client/Makefile \
	src/client/sdl/Makefile \
	src/server/Makefile \
	tests/Makefile \
	])
AC_OUTPUT
