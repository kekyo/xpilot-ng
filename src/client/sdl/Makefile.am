
bin_PROGRAMS = xpilot-ng-sdl
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client \
	@SDL2_CFLAGS@ @SDL2_IMAGE_CFLAGS@ @SDL2_TTF_CFLAGS@ \
	@GL_CFLAGS@ @GLU_CFLAGS@
xpilot_ng_sdl_SOURCES = \
	asteroid_data.h DT_drawtext.c DT_drawtext.h \
	SDL_console.c SDL_console.h SDL_gfxPrimitives.c SDL_gfxPrimitives.h \
	SDL_gfxPrimitives_font.h console.c console.h glwidgets.c glwidgets.h \
	gameloop.c gl_diagnostics.c gl_diagnostics.h \
	image_geometry.c image_geometry.h images.c images.h images_test_support.h \
	main.c radar.c radar.h \
	paint_transform.c paint_transform.h \
	renderer.c renderer.h renderer_backend.h \
	renderer_gl.h renderer_gl_legacy.c renderer_gl_legacy.h \
	renderer_gl_core.c renderer_gl_core.h \
	sdlclipboard.c sdlclipboard.h \
	sdlcompat.c sdlcompat.h \
	sdlevent.c sdlevent.h sdlgui.c sdlinit.c sdlkeys.c sdlkeys.h \
	sdlmetaframe.c sdlmetaframe.h sdlrenderer.c sdlrenderer.h \
	sdluiimage.c sdluiimage.h \
	sdlpaint.c surface_rgba8.c surface_rgba8.h \
	sdlpaint.h sdlwindow.c sdlwindow.h text.c text.h \
	text_atlas.c text_atlas.h text_geometry.c text_geometry.h \
	text_renderer.c text_renderer.h \
	text_ttf_atlas.c text_ttf_atlas.h todo.c \
	sdlmeta.h sdlmeta.c sdlinit.h xpclient_sdl.h
xpilot_ng_sdl_LDADD = $(top_builddir)/src/client/libxpclient.a \
	$(top_builddir)/src/common/libxpcommon.a \
	@SDL2_IMAGE_LIBS@ @SDL2_TTF_LIBS@ @SDL2_LIBS@ \
	@GLU_LIBS@ @GL_LIBS@ @SOUND_LIBS@ @W32_LIBS@

if COND_WINDOWS
  xpilot_ng_sdl_SOURCES += win32hacks.c
endif

EXTRA_DIST = xpilot.rc xpilot_sdl.dsp xpilot_sdl.dsw
