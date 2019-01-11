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

dnl  Checks for options.
dnl  We should have some general macro for these.
AC_ARG_ENABLE(dbe,
	AC_HELP_STRING([--enable-dbe],
			[support for X Doublebuffer Extension in client]),
[if test x$enable_dbe = xyes; then
	AC_DEFINE([DBE], 1, [support for X Doublebuffer Extension in client])
fi])

AC_ARG_ENABLE(mbx,
	AC_HELP_STRING([--enable-mbx],
			[support for X Multibuffer Extension in client]),
[if test x$enable_mbx = xyes; then
	AC_DEFINE([MBX], 1, [support for X Multibuffer Extension in client])
fi])

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

AC_ARG_ENABLE(replay,
	AC_HELP_STRING([--disable-replay],
			[disable building of replay program]),
[], [enable_replay=yes; need_x=yes])

AC_ARG_ENABLE(xp_mapedit,
	AC_HELP_STRING([--disable-xp-mapedit],
			[disable building of xp-mapedit program]),
[], [enable_xp_mapedit=yes; need_x=yes])

AC_ARG_ENABLE(x11_client,
	AC_HELP_STRING([--disable-x11-client],
			[disable X11 client]),
[], [enable_x11_client=yes; need_x=yes; need_libxpclient=yes])

AC_ARG_ENABLE(sdl_client,
	AC_HELP_STRING([--disable-sdl-client],
			[disable SDL/OpenGL client]),
[], [enable_sdl_client=yes; need_x=yes; need_libxpclient=yes])
if test x$enable_sdl_client = xyes; then
	AC_DEFINE([SDL_CLIENT], 1, [enable sdl client])
fi

dnl  Do this if you want to use the gameloop without the X11 hack.
AC_ARG_ENABLE(sdl_gameloop,
	AC_HELP_STRING([--enable-sdl-gameloop],
			[enable sdl gameloop in sdl client
			 (otherwise use gameloop optimised for X11)]))

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

dnl Figure out which math library to use
dnl (borrowed from from http://www.libsdl.org/opengl/SDLgears-1.0.2.tar.gz)
case "$target" in
    *-*-mingw32*)
	MATHLIB=""
	SYS_GL_LIBS="-lopengl32 -lglu32"
	windows_target=yes
	;;
    *-*-beos*)
	MATHLIB=""
	SYS_GL_LIBS="-lGL"
	;;
    *-*-linux*)
	MATHLIB="-lm"
	SYS_GL_LIBS="-L/usr/X11R6/lib -lGL -lGLU"
	;;
    *-*-freebsd*)
	MATHLIB="-lm"
	SYS_GL_LIBS="-L/usr/X11R6/lib -lGL -lGLU"
	CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include -I/usr/local/include"
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
	;;
    *)
	MATHLIB="-lm"
	SYS_GL_LIBS="-lGL -lGLU"
	;;
esac

if test x$windows_target = xyes; then
	W32_LIBS="-lmingw32 -lwsock32"
	AC_SUBST(W32_LIBS)
	AC_DEFINE([_WINDOWS], 1, [compiling for windows target])

	enable_select_sched=yes
	enable_sdl_gameloop=yes
	enable_replay=no
	enable_xp_mapedit=no
	enable_x11_client=no
	need_x=no
fi

AC_PATH_X
if test x$need_x = xyes; then
	if test x$no_x == xyes; then
		AC_MSG_ERROR([*** Couldn't find X headers or libraries!])
	fi
	
	AC_PATH_XTRA

	AC_MSG_CHECKING(for xf86misc extensions)
	AC_COMPILE_IFELSE([
	 #include <X11/X.h>
	 #include <X11/Xlib.h>
	 #include <X11/extensions/xf86misc.h>
	 int main(int argc, char** argv) { return 0; }
	],[
	have_xf86misc=yes
	],[
	have_xf86misc=no])
	AC_MSG_RESULT($have_xf86misc)

	dnl kps - Hack to link with libXext if DBE or MBX is configured
	if test x$enable_dbe = xyes || test x$enable_mbx = xyes || test x$have_xf86misc = xyes; then
		X_EXTENSIONS_LIB="-lXext"
	else
		X_EXTENSIONS_LIB=
	fi
	
	if test x$have_xf86misc = xyes; then
		X_EXTENSIONS_LIB="-lXxf86misc $X_EXTENSIONS_LIB"
		AC_DEFINE([HAVE_XF86MISC], 1, [Define to 1 if you have the Xf86Misc library])
	fi
	
	AC_SUBST(X_EXTENSIONS_LIB)
fi

if test x$enable_sdl_gameloop = xyes; then
  AC_DEFINE([SDL_GAMELOOP], 1, [enable sdl gameloop in sdl client])
fi

if test x$enable_x11_client = xyes; then
	AC_DEFINE([X11_CLIENT], 1, [enable x11 client])
fi

if test x$enable_select_sched = xyes; then
	AC_DEFINE([SELECT_SCHED], 1,
		[enable new server scheduling most useful on Linux 2.6])
fi

if test x$enable_replay = xyes; then
	AC_DEFINE([REPLAY], 1, [enable replay program])
fi

if test x$enable_xp_mapedit = xyes; then
	AC_DEFINE([XP_MAPEDIT], 1, [enable xp-mapedit program])
fi

dnl Stuff needed by SDL client.
if test x$enable_sdl_client = xyes; then

	dnl Check for SDL,
	dnl from http://www.libsdl.org/opengl/SDLgears-1.0.2.tar.gz
	SDL_VERSION=1.2.0
	AM_PATH_SDL($SDL_VERSION,
		    :,
		    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
	)
	
	CFLAGS_BAK="$CFLAGS"
	CXXFLAGS_BAK="$CXXFLAGS"
	CFLAGS="$CFLAGS $SDL_CFLAGS"
	CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"

	if test x$no_sdl = xyes; then
	    AC_MSG_ERROR(SDL client will not be built.)
	fi

	dnl check for SDL_ttf and OpenGL only if SDL client is built.

	dnl kps - hack to check for SDL_ttf
	AC_MSG_CHECKING(for SDL_ttf)
	AC_COMPILE_IFELSE([
	 #include "SDL.h"
	 #include "SDL/SDL_ttf.h"
	 int main(int argc, char** argv) { return 0; }
	],[
	have_libsdl_ttf=yes
	],[
	have_libsdl_ttf=no])
	AC_MSG_RESULT($have_libsdl_ttf)
	if test x$have_libsdl_ttf = xyes; then
	    SDL_EXTRA_LIBS="$SDL_EXTRA_LIBS -lSDL_ttf"
	else
	    AC_MSG_ERROR([*** Unable to find SDL_ttf headers and libraries!])
	fi
	AC_SUBST(SDL_EXTRA_LIBS)

	dnl kps - hack to check for SDL_image
	AC_MSG_CHECKING(for SDL_image)
	AC_COMPILE_IFELSE([
	 #include "SDL.h"
	 #include "SDL/SDL_image.h"
	 int main(int argc, char** argv) { return 0; }
	],[
	have_libsdl_image=yes
	],[
	have_libsdl_image=no])
	AC_MSG_RESULT($have_libsdl_image)
	if test x$have_libsdl_image = xyes; then
	    SDL_EXTRA_LIBS="$SDL_EXTRA_LIBS -lSDL_image"
	    AC_DEFINE([HAVE_SDL_IMAGE], 1,
			[Define to 1 if you have the SDL_image library.])
	else
	    AC_MSG_ERROR([*** Unable to find SDL_image headers and libraries!])
	fi
	AC_SUBST(SDL_EXTRA_LIBS)

	dnl Check for OpenGL
	AC_MSG_CHECKING(for OpenGL support)
	AC_COMPILE_IFELSE([
	 #include <GL/gl.h>
	 #include <GL/glu.h>
	 int main(int argc, char** argv) { return 0; } 
	],[
	have_opengl=yes
	],[
	have_opengl=no])
	AC_MSG_RESULT($have_opengl)
	if test x$have_opengl = xyes; then
	    GL_LIBS="$SDL_LIBS $SYS_GL_LIBS"
	else
	    AC_MSG_ERROR([*** Unable to find OpenGL headers and libraries!])
	fi
	AC_SUBST(GL_LIBS)
	AC_SUBST(SDL_CFLAGS)
	
	CFLAGS="$CFLAGS_BAK"
	CXXFLAGS="$CXXFLAGS_BAK"
fi

AM_CONDITIONAL([COND_CLIENT], [test x$need_libxpclient = xyes])
AM_CONDITIONAL([COND_WINDOWS], [test x$windows_target = xyes])
AM_CONDITIONAL([COND_REPLAY], [test x$enable_replay = xyes])
AM_CONDITIONAL([COND_XP_MAPEDIT], [test x$enable_xp_mapedit = xyes])
AM_CONDITIONAL([COND_X11_CLIENT], [test x$enable_x11_client = xyes])
AM_CONDITIONAL([COND_SDL_CLIENT], [test x$enable_sdl_client = xyes])
AM_CONDITIONAL([COND_SDL_GAMELOOP], [test x$enable_sdl_gameloop = xyes])

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
	X11/X.h \
	X11/Xlib.h \
	X11/Xos.h \
	X11/Xutil.h \
	X11/keysym.h \
	X11/Xatom.h \
	X11/Xmd.h \
	])

dnl 	X11/extensions/Xdbe.h
dnl	X11/extensions/multibuf.h

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
	src/common/NT/bindist/Makefile \
	src/client/Makefile \
	src/client/items/Makefile \
	src/client/sdl/Makefile \
	src/client/x11/Makefile \
	src/client/NT/Makefile \
	src/client/NT/res/Makefile \
	src/server/Makefile \
	src/replay/Makefile \
	src/replay/tools/Makefile \
	src/mapedit/Makefile \
	])
AC_OUTPUT

