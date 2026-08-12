AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client \
	-I$(top_srcdir)/src/client/sdl \
	@SDL2_CFLAGS@ @SDL2_IMAGE_CFLAGS@ @SDL2_TTF_CFLAGS@ \
	@GL_CFLAGS@ @GLU_CFLAGS@

check_PROGRAMS = test-meta-environment \
	test-sdl-keys test-sdl-console test-sdl-surface \
	test-surface-rgba8 test-sdl-window \
	test-sdl-clipboard test-sdl-event test-gl-legacy \
	test-renderer-commands test-renderer-geometry \
	test-renderer-gl-backends test-paint-transform test-image-geometry \
	test-gfx2d-picture test-images test-radar-renderer test-sdl-ui-image \
	test-sdl-meta-frame test-sdl-renderer-scissor \
	test-glwidgets-scissor test-text-geometry test-text-atlas \
	test-text-ttf-atlas test-text-renderer
TESTS = test-meta-environment \
	test-sdl-keys test-sdl-console test-sdl-surface \
	test-surface-rgba8 test-sdl-window \
	test-sdl-clipboard test-sdl-event test-renderer-commands \
	test-renderer-geometry test-paint-transform run-gl-legacy.sh \
	test-image-geometry test-gfx2d-picture \
	test-images test-radar-renderer test-sdl-ui-image test-sdl-meta-frame \
	test-sdl-renderer-scissor test-glwidgets-scissor test-text-geometry \
	test-text-atlas test-text-ttf-atlas test-text-renderer \
	run-renderer-gl-backends.sh run-e2e.sh

test_meta_environment_SOURCES = test_meta_environment.c \
	$(top_srcdir)/src/client/meta.c \
	$(top_srcdir)/src/client/meta_test_support.h \
	$(top_srcdir)/src/common/strlcpy.c
test_meta_environment_CPPFLAGS = $(AM_CPPFLAGS) -DXPILOT_META_TEST_HOOKS
test_meta_environment_CFLAGS = $(AM_CFLAGS) \
	-ffunction-sections -fdata-sections
test_meta_environment_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections

test_sdl_keys_SOURCES = test_sdl_keys.c \
	$(top_srcdir)/src/client/sdl/sdlkeys.c
test_sdl_keys_LDADD = @SDL2_LIBS@

test_sdl_console_SOURCES = test_sdl_console.c \
	$(top_srcdir)/src/client/sdl/SDL_console.c
test_sdl_console_LDADD = @SDL2_IMAGE_LIBS@ @SDL2_LIBS@

test_sdl_surface_SOURCES = test_sdl_surface.c \
	$(top_srcdir)/src/client/sdl/sdlcompat.c
test_sdl_surface_LDADD = @SDL2_LIBS@

test_surface_rgba8_SOURCES = test_surface_rgba8.c \
	$(top_srcdir)/src/client/sdl/surface_rgba8.c \
	$(top_srcdir)/src/client/sdl/sdlcompat.c
test_surface_rgba8_LDADD = @SDL2_LIBS@

test_sdl_window_SOURCES = test_sdl_window.c \
	$(top_srcdir)/src/client/sdl/sdlwindow.c
test_sdl_window_LDADD = @SDL2_LIBS@

test_sdl_clipboard_SOURCES = test_sdl_clipboard.c \
	$(top_srcdir)/src/client/sdl/sdlclipboard.c
test_sdl_clipboard_LDADD = @SDL2_LIBS@

test_sdl_event_SOURCES = test_sdl_event.c \
	$(top_srcdir)/src/client/sdl/sdlevent.c
test_sdl_event_LDADD = @SDL2_LIBS@

test_gl_legacy_SOURCES = test_gl_legacy.c \
	$(top_srcdir)/src/client/sdl/gl_diagnostics.c \
	$(top_srcdir)/src/client/sdl/sdlgui.c \
	$(top_srcdir)/src/client/sdl/renderer.c \
	$(top_srcdir)/src/client/sdl/renderer_gl_legacy.c \
	$(top_srcdir)/src/client/sdl/sdlcompat.c \
	$(top_srcdir)/src/client/sdl/text.c \
	$(top_srcdir)/src/client/sdl/text_atlas.c \
	$(top_srcdir)/src/client/sdl/text_atlas.h \
	$(top_srcdir)/src/client/sdl/text_geometry.c \
	$(top_srcdir)/src/client/sdl/text_geometry.h \
	$(top_srcdir)/src/client/sdl/text_ttf_atlas.c \
	$(top_srcdir)/src/client/sdl/text_ttf_atlas.h
test_gl_legacy_CPPFLAGS = $(AM_CPPFLAGS) \
	-DXPILOT_TEST_FONT_PATH=\"$(top_srcdir)/lib/fonts/FreeSansBoldOblique.ttf\"
test_gl_legacy_CFLAGS = $(AM_CFLAGS) -DXPILOT_GL_TEST_HOOKS \
	-ffunction-sections -fdata-sections
test_gl_legacy_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections \
	-Wl,--wrap=glGetString -Wl,--wrap=glGenLists \
	-Wl,--wrap=glGenTextures
test_gl_legacy_LDADD = @SDL2_TTF_LIBS@ @SDL2_LIBS@ \
	@GLU_LIBS@ @GL_LIBS@

test_renderer_commands_SOURCES = test_renderer_commands.c \
	$(top_srcdir)/src/client/sdl/renderer.c

test_renderer_geometry_SOURCES = test_renderer_geometry.c \
	$(top_srcdir)/src/client/sdl/renderer.c

test_renderer_gl_backends_SOURCES = test_renderer_gl_backends.c \
	$(top_srcdir)/src/client/sdl/renderer.c \
	$(top_srcdir)/src/client/sdl/renderer_gl_legacy.c \
	$(top_srcdir)/src/client/sdl/renderer_gl_core.c \
	$(top_srcdir)/src/client/sdl/sdlrenderer.c
test_renderer_gl_backends_LDADD = @SDL2_LIBS@ @GL_LIBS@

test_paint_transform_SOURCES = test_paint_transform.c \
	$(top_srcdir)/src/client/sdl/paint_transform.c

test_image_geometry_SOURCES = test_image_geometry.c \
	$(top_srcdir)/src/client/sdl/image_geometry.c

test_gfx2d_picture_SOURCES = test_gfx2d_picture.c \
	$(top_srcdir)/src/client/gfx2d.c
test_gfx2d_picture_LDFLAGS = $(AM_LDFLAGS) -Wl,--wrap=malloc
test_gfx2d_picture_LDADD = $(top_builddir)/src/common/libxpcommon.a

test_images_SOURCES = test_images.c \
	$(top_srcdir)/src/client/sdl/images.c \
	$(top_srcdir)/src/client/sdl/image_geometry.c \
	$(top_srcdir)/src/common/strdup.c
test_images_CPPFLAGS = $(AM_CPPFLAGS) -DXPILOT_IMAGE_TEST_HOOKS
test_images_LDADD = @SDL2_LIBS@ @GL_LIBS@

test_radar_renderer_SOURCES = test_radar_renderer.c \
	$(top_srcdir)/src/client/sdl/radar.c
test_radar_renderer_CPPFLAGS = $(AM_CPPFLAGS) -DXPILOT_RADAR_TEST_HOOKS
test_radar_renderer_CFLAGS = $(AM_CFLAGS) \
	-ffunction-sections -fdata-sections
test_radar_renderer_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
test_radar_renderer_LDADD = @SDL2_LIBS@

test_sdl_ui_image_SOURCES = test_sdl_ui_image.c \
	$(top_srcdir)/src/client/sdl/sdluiimage.c \
	$(top_srcdir)/src/client/sdl/sdlmetaframe.c \
	$(top_srcdir)/src/client/sdl/glwidgets.c \
	$(top_srcdir)/src/client/sdl/surface_rgba8.c \
	$(top_srcdir)/src/client/sdl/sdlcompat.c
test_sdl_ui_image_CFLAGS = $(AM_CFLAGS) \
	-ffunction-sections -fdata-sections
test_sdl_ui_image_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
test_sdl_ui_image_LDADD = @SDL2_LIBS@

test_sdl_meta_frame_SOURCES = test_sdl_meta_frame.c \
	$(top_srcdir)/src/client/sdl/sdlmetaframe.c

test_sdl_renderer_scissor_SOURCES = test_sdl_renderer_scissor.c \
	$(top_srcdir)/src/client/sdl/renderer.c \
	$(top_srcdir)/src/client/sdl/sdlrenderer.c
test_sdl_renderer_scissor_CFLAGS = $(AM_CFLAGS) \
	-ffunction-sections -fdata-sections
test_sdl_renderer_scissor_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
test_sdl_renderer_scissor_LDADD = @SDL2_LIBS@ @GL_LIBS@

test_glwidgets_scissor_SOURCES = test_glwidgets_scissor.c \
	$(top_srcdir)/src/client/sdl/glwidgets.c
test_glwidgets_scissor_CFLAGS = $(AM_CFLAGS) \
	-ffunction-sections -fdata-sections
test_glwidgets_scissor_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections
test_glwidgets_scissor_LDADD = @SDL2_LIBS@

test_text_geometry_SOURCES = test_text_geometry.c \
	$(top_srcdir)/src/client/sdl/text_geometry.c \
	$(top_srcdir)/src/client/sdl/text_geometry.h

test_text_atlas_SOURCES = test_text_atlas.c \
	$(top_srcdir)/src/client/sdl/text_atlas.c \
	$(top_srcdir)/src/client/sdl/text_atlas.h
test_text_atlas_LDADD = @SDL2_LIBS@

test_text_ttf_atlas_SOURCES = test_text_ttf_atlas.c \
	$(top_srcdir)/src/client/sdl/text_ttf_atlas.c \
	$(top_srcdir)/src/client/sdl/text_ttf_atlas.h \
	$(top_srcdir)/src/client/sdl/text_atlas.c \
	$(top_srcdir)/src/client/sdl/text_atlas.h \
	$(top_srcdir)/src/client/sdl/text_geometry.c \
	$(top_srcdir)/src/client/sdl/text_geometry.h
test_text_ttf_atlas_CPPFLAGS = $(AM_CPPFLAGS) \
	-DXPILOT_TEST_FONT_PATH=\"$(top_srcdir)/lib/fonts/FreeSansBoldOblique.ttf\"
test_text_ttf_atlas_LDADD = @SDL2_TTF_LIBS@ @SDL2_LIBS@

test_text_renderer_SOURCES = test_text_renderer.c \
	$(top_srcdir)/src/client/sdl/text_renderer.c \
	$(top_srcdir)/src/client/sdl/text_renderer.h \
	$(top_srcdir)/src/client/sdl/text_geometry.c \
	$(top_srcdir)/src/client/sdl/text_geometry.h

TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = $(SHELL)
AM_TESTS_ENVIRONMENT = \
	XPILOT_TEST_PREFIX='$(prefix)'; export XPILOT_TEST_PREFIX; \
	XPILOT_TEST_BINDIR='$(bindir)'; export XPILOT_TEST_BINDIR; \
	XPILOT_TEST_PKGDATADIR='$(pkgdatadir)'; export XPILOT_TEST_PKGDATADIR; \
	XPILOT_GL_TEST_BINARY='$(abs_builddir)/test-gl-legacy'; \
	export XPILOT_GL_TEST_BINARY; \
	XPILOT_RENDERER_GL_TEST_BINARY='$(abs_builddir)/test-renderer-gl-backends'; \
	export XPILOT_RENDERER_GL_TEST_BINARY;

EXTRA_DIST = run-e2e.sh run-full-suite.sh run-gl-legacy.sh \
	run-renderer-gl-backends.sh test_helpers.h

.PHONY: e2e-install
run-e2e.log: e2e-install
e2e-install:
	@case '$(prefix)' in \
		/tmp/*|/var/tmp/*) $(MAKE) -C '$(abs_top_builddir)' install ;; \
		*) : ;; \
	esac
