
bin_PROGRAMS = xpilot-ng-sdl
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client @SDL_CFLAGS@
xpilot_ng_sdl_SOURCES = \
	asteroid_data.h DT_drawtext.c DT_drawtext.h \
	SDL_console.c SDL_console.h SDL_gfxPrimitives.c SDL_gfxPrimitives.h \
	SDL_gfxPrimitives_font.h console.c console.h glwidgets.c glwidgets.h \
	images.c images.h main.c scrap.c scrap.h radar.c radar.h \
	sdlevent.c sdlgui.c sdlinit.c sdlkeys.c sdlkeys.h sdlpaint.c \
	sdlpaint.h sdlwindow.c sdlwindow.h text.c text.h todo.c \
	sdlmeta.h sdlmeta.c sdlinit.h ../xhacks.h xpclient_sdl.h
xpilot_ng_sdl_LDADD = $(top_builddir)/src/client/libxpclient.a  $(top_builddir)/src/common/libxpcommon.a @GL_LIBS@ @SDL_EXTRA_LIBS@ @X_LIBS@ @X_PRE_LIBS@ @X_EXTENSIONS_LIB@ @X_EXTRA_LIBS@ @SOUND_LIBS@ @W32_LIBS@

if COND_SDL_GAMELOOP
  xpilot_ng_sdl_SOURCES += gameloop.c
else
  xpilot_ng_sdl_SOURCES += gameloop_x.c
endif

if COND_WINDOWS
  xpilot_ng_sdl_SOURCES += win32hacks.c
else
  xpilot_ng_sdl_LDADD += -lX11
endif

EXTRA_DIST = xpilot.rc xpilot_sdl.dsp xpilot_sdl.dsw
