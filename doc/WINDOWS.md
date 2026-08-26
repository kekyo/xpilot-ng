# Linux-hosted Windows builds

XPilot NG's supported Windows build is a Linux cross-build.  It targets
32-bit x86 and 64-bit x86_64 Windows with MinGW-w64; building with Visual
Studio or on a Windows host is not supported.

## Prerequisites

Install the following host tools before building:

- the `i686-w64-mingw32` and `x86_64-w64-mingw32` GCC toolchains;
- CMake, Make, pkg-config, Git, and Node.js;
- Wine with both 32-bit and 64-bit support;
- Xvfb and xdotool for the SDL integration tests;
- the standard `file` and `timeout` utilities.

Initialize every pinned dependency, including the recursive SDL_ttf
dependencies:

```sh
git submodule update --init --recursive
```

The build does not download dependencies.  zlib, Expat, SDL3, SDL3_image,
SDL3_ttf, FreeType, and HarfBuzz are cross-compiled from the pinned submodules
and linked statically.

## Build and test

Use a separate configured build directory for each architecture.  For
64-bit Windows, run:

```sh
mkdir -p build/windows/x86_64
cd build/windows/x86_64
../../../configure --host=x86_64-w64-mingw32
make -j4
make windows-package
make check
```

For 32-bit Windows, use another build directory and host triplet:

```sh
mkdir -p build/windows/x86
cd build/windows/x86
../../../configure --host=i686-w64-mingw32
make -j4
make windows-package
make check
```

The first `make` cross-builds the pinned dependencies before building XPilot
NG.  `make windows-package` assembles the executable and game data, and
`make check` builds the supported Windows tests and runs the complete suite
through Wine.  Repeating `make` reuses the dependency build until one of its
inputs or configuration changes.  Do not reconfigure one build directory for
a different host architecture.

The dependency CMake configuration defaults to `Release`.  Override it at
configure time with `--with-mingw-deps-build-type=TYPE`.  A custom absolute
CMake toolchain file can be selected with
`--with-mingw-toolchain-file=PATH`.  `make mingw-deps-clean` removes the
configured tree's dependency build and prefix when a complete dependency
rebuild is required.

As a convenience, the top-level wrapper performs the same configure and make
sequence for both architectures and creates versioned distribution archives:

```sh
./build.sh --target windows --arch all --test
```

Running `./build.sh` without `--target` builds these two Windows architectures
after the native target.  Use `--target windows` to omit the native build.
Use `--arch x86` or `--arch x86_64` to select one architecture.  The wrapper
does not build dependencies, package executables, or invoke Wine itself;
those operations remain Makefile targets in each configured tree.  `--jobs`,
`--build-root`, and `--build-type` select their corresponding configure and
make settings.  `--artifact-root` selects the ZIP destination, and
`--package-version` overrides the version normally derived by `screw-up`.
Run `./build.sh --help` for the complete interface.

Each architecture has an isolated dependency prefix, build tree, Wine
prefix, and package directory:

```text
build/windows/x86/package/
build/windows/x86_64/package/
artifacts/windows/xpilot-ng-<version>-windows-x86.zip
artifacts/windows/xpilot-ng-<version>-windows-x86_64.zip
```

The packages contain `xpilot-ng-server.exe`, `xpilot-ng-sdl.exe`, the game
data, and the license.  ZIP entries are sorted and use a fixed timestamp, so
unchanged package trees produce byte-identical archives.  The Wine suite
verifies the PE architecture, runs the portable networking and SDL dependency
tests, validates a TCP-contact server's metaserver advertisement, and starts
the packaged server and SDL client for all four UDP/TCP contact and gameplay
combinations.  Every session must join, create an OpenGL context, initialize
its text renderers, and quit cleanly.  A software OpenGL renderer is selected
so the graphical checks do not depend on host GPU drivers.

The Wine prefixes are persistent in their configured build directories.
Build and test output never needs to be written into the source tree.
