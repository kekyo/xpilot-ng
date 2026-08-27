MINGW_DEPS_JOBS =

mingw_deps_stamp = $(abs_builddir)/mingw-deps.stamp
mingw_deps_makefile = @MINGW_DEPS_MAKEFILE@

all-local: $(mingw_deps_makefile)

$(mingw_deps_stamp): Makefile \
	$(top_srcdir)/vendor/mingw/build.sh \
	$(top_srcdir)/vendor/sdl3/build.sh \
	$(top_srcdir)/vendor/zlib/zlib.h \
	$(top_srcdir)/vendor/expat/expat/CMakeLists.txt \
	$(top_srcdir)/vendor/openal-soft/CMakeLists.txt \
	$(top_srcdir)/vendor/freealut/CMakeLists.txt \
	$(top_srcdir)/vendor/sdl3/SDL/CMakeLists.txt \
	$(top_srcdir)/vendor/sdl3/SDL_image/CMakeLists.txt \
	$(top_srcdir)/vendor/sdl3/SDL_ttf/CMakeLists.txt \
	@MINGW_TOOLCHAIN_FILE@
	@jobs_option=; \
	if test -n "$(MINGW_DEPS_JOBS)"; then \
		case "$(MINGW_DEPS_JOBS)" in \
			''|0|*[!0-9]*) \
				echo "MINGW_DEPS_JOBS must be a positive integer" >&2; \
				exit 1 ;; \
		esac; \
		jobs_option="--jobs $(MINGW_DEPS_JOBS)"; \
	fi; \
	$(SHELL) $(top_srcdir)/vendor/mingw/build.sh \
		--build-dir '@MINGW_DEPS_BUILD_DIR@' \
		--prefix '@MINGW_DEPS_PREFIX@' \
		--triplet '@host@' \
		--toolchain-file '@MINGW_TOOLCHAIN_FILE@' \
		--build-type '@MINGW_DEPS_BUILD_TYPE@' \
		$$jobs_option
	@touch "$@.tmp"
	@mv -f "$@.tmp" "$@"

$(mingw_deps_makefile): $(mingw_deps_stamp) \
	$(top_srcdir)/vendor/mingw/generate-deps-makefile.sh
	$(SHELL) $(top_srcdir)/vendor/mingw/generate-deps-makefile.sh \
		--prefix '@MINGW_DEPS_PREFIX@' \
		--output '$@' \
		--pkg-config '@PKG_CONFIG@'

.PHONY: mingw-deps-clean
mingw-deps-clean:
	@for output_path in \
		'@MINGW_DEPS_BUILD_DIR@' \
		'@MINGW_DEPS_PREFIX@'; do \
		case "$$output_path" in \
			'$(abs_top_builddir)'/*) rm -rf -- "$$output_path" ;; \
			*) echo "refusing to remove unexpected MinGW path: $$output_path" >&2; \
				exit 1 ;; \
		esac; \
	done
	@rm -f -- '$(mingw_deps_stamp)' '$(mingw_deps_makefile)'

distclean-local: mingw-deps-clean

EXTRA_DIST = build.sh generate-deps-makefile.sh
