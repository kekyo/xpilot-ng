
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common

noinst_LIBRARIES = libxpclient.a
libxpclient_a_SOURCES = \
	client.c client.h \
	clientcommand.c clientcommand.h clientrank.c clientrank.h \
	configure.h connectparam.h \
	datagram.c datagram.h default.c \
	event.c \
	gfx2d.c gfx2d.h \
	guimap.h guiobjects.h \
	mapdata.c messages.c meta.c meta.h \
	netclient.c netclient.h \
	paint.c paint.h paintmap.c paintobjects.c \
	option.c option.h \
	query.c \
	recordfmt.h \
	talk.h talkmacros.c textinterface.c \
	usleep.c \
	xpclient.h

if COND_SOUND
libxpclient_a_SOURCES += caudio.c caudio.h oalaudio.c
endif

# From section Conditional subdirectories with AM_CONDITIONAL
# in GNU automake manual.
if COND_SDL_CLIENT
  MAYBE_SDL = sdl
else
  MAYBE_SDL =
endif

if COND_X11_CLIENT
  MAYBE_X11 = x11
else
  MAYBE_X11 =
endif

# kps - . in SUBDIRS is needed to make sure libxpclient.a is compiled before
# SDL client
SUBDIRS = . items $(MAYBE_X11) $(MAYBE_SDL) NT
