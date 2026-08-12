AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client \
	-I$(top_srcdir)/src/client/sdl \
	@SDL2_CFLAGS@ @SDL2_IMAGE_CFLAGS@ @SDL2_TTF_CFLAGS@ \
	@GL_CFLAGS@ @GLU_CFLAGS@

check_PROGRAMS = test-sdl-keys test-sdl-console test-sdl-surface \
	test-sdl-clipboard test-sdl-event test-gl-legacy \
	test-renderer-commands test-renderer-geometry \
	test-renderer-gl-backends test-paint-transform
TESTS = test-sdl-keys test-sdl-console test-sdl-surface \
	test-sdl-clipboard test-sdl-event test-renderer-commands \
	test-renderer-geometry test-paint-transform run-gl-legacy.sh \
	run-renderer-gl-backends.sh run-e2e.sh

test_sdl_keys_SOURCES = test_sdl_keys.c \
	$(top_srcdir)/src/client/sdl/sdlkeys.c
test_sdl_keys_LDADD = @SDL2_LIBS@

test_sdl_console_SOURCES = test_sdl_console.c \
	$(top_srcdir)/src/client/sdl/SDL_console.c
test_sdl_console_LDADD = @SDL2_IMAGE_LIBS@ @SDL2_LIBS@

test_sdl_surface_SOURCES = test_sdl_surface.c \
	$(top_srcdir)/src/client/sdl/sdlcompat.c
test_sdl_surface_LDADD = @SDL2_LIBS@

test_sdl_clipboard_SOURCES = test_sdl_clipboard.c \
	$(top_srcdir)/src/client/sdl/sdlclipboard.c
test_sdl_clipboard_LDADD = @SDL2_LIBS@

test_sdl_event_SOURCES = test_sdl_event.c \
	$(top_srcdir)/src/client/sdl/sdlevent.c
test_sdl_event_LDADD = @SDL2_LIBS@

test_gl_legacy_SOURCES = test_gl_legacy.c \
	$(top_srcdir)/src/client/sdl/gl_diagnostics.c \
	$(top_srcdir)/src/client/sdl/sdlgui.c
test_gl_legacy_CFLAGS = $(AM_CFLAGS) -DXPILOT_GL_TEST_HOOKS \
	-ffunction-sections -fdata-sections
test_gl_legacy_LDFLAGS = $(AM_LDFLAGS) -Wl,--gc-sections \
	-Wl,--wrap=glGetString
test_gl_legacy_LDADD = @SDL2_LIBS@ @GLU_LIBS@ @GL_LIBS@

test_renderer_commands_SOURCES = test_renderer_commands.c \
	$(top_srcdir)/src/client/sdl/renderer.c

test_renderer_geometry_SOURCES = test_renderer_geometry.c \
	$(top_srcdir)/src/client/sdl/renderer.c

test_renderer_gl_backends_SOURCES = test_renderer_gl_backends.c \
	$(top_srcdir)/src/client/sdl/renderer.c \
	$(top_srcdir)/src/client/sdl/renderer_gl_legacy.c \
	$(top_srcdir)/src/client/sdl/renderer_gl_core.c
test_renderer_gl_backends_LDADD = @SDL2_LIBS@ @GL_LIBS@

test_paint_transform_SOURCES = test_paint_transform.c \
	$(top_srcdir)/src/client/sdl/paint_transform.c

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
