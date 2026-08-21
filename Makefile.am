if COND_MINGW_VENDORED
SUBDIRS = vendor/mingw src lib doc contrib
else
SUBDIRS = src lib doc contrib
endif

if COND_SDL_CLIENT
SUBDIRS += tests
endif
dist_pkgdata_DATA = mapconvert.py
EXTRA_DIST = build.sh config/package-windows.sh

if COND_WINDOWS
MINGW_TEST_JOBS =

.PHONY: windows-package windows-check mingw-deps-clean
windows-package: all
	$(SHELL) $(top_srcdir)/config/package-windows.sh \
		--source-dir '$(abs_top_srcdir)' \
		--build-dir '$(abs_top_builddir)' \
		--output '@MINGW_PACKAGE_DIR@'

windows-check: windows-package
	$(SHELL) $(top_srcdir)/tests/run-wine-suite.sh \
		--build-dir '$(abs_top_builddir)' \
		--package-dir '@MINGW_PACKAGE_DIR@' \
		--wine-prefix '@MINGW_WINE_PREFIX@' \
		--arch '@MINGW_ARCH@' \
		--jobs '$(MINGW_TEST_JOBS)'

check-local: windows-check

mingw-deps-clean:
	$(MAKE) -C vendor/mingw mingw-deps-clean
endif
