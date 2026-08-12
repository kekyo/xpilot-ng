
bin_PROGRAMS = xpilot-ng-sdl
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client \
	@SDL2_CFLAGS@ @SDL2_IMAGE_CFLAGS@ @SDL2_TTF_CFLAGS@ \
	@GL_CFLAGS@ @GLU_CFLAGS@
xpilot_ng_sdl_SOURCES = \
	asteroid_data.h DT_drawtext.c DT_drawtext.h \
	SDL_console.c SDL_console.h SDL_gfxPrimitives.c SDL_gfxPrimitives.h \
	SDL_gfxPrimitives_font.h console.c console.h glwidgets.c glwidgets.h \
	gameloop.c images.c images.h main.c radar.c radar.h \
	sdlclipboard.c sdlclipboard.h \
	sdlcompat.c sdlcompat.h \
	sdlevent.c sdlevent.h sdlgui.c sdlinit.c sdlkeys.c sdlkeys.h sdlpaint.c \
	sdlpaint.h sdlwindow.c sdlwindow.h text.c text.h todo.c \
	sdlmeta.h sdlmeta.c sdlinit.h xpclient_sdl.h
xpilot_ng_sdl_LDADD = $(top_builddir)/src/client/libxpclient.a \
	$(top_builddir)/src/common/libxpcommon.a \
	@SDL2_IMAGE_LIBS@ @SDL2_TTF_LIBS@ @SDL2_LIBS@ \
	@GLU_LIBS@ @GL_LIBS@ @SOUND_LIBS@ @W32_LIBS@

if COND_WINDOWS
  xpilot_ng_sdl_SOURCES += win32hacks.c
endif

EXTRA_DIST = xpilot.rc xpilot_sdl.dsp xpilot_sdl.dsw
