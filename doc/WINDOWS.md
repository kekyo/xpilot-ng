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

Build both Windows architectures and run their complete supported Wine test
suites with:

```sh
./build.sh --target windows --arch all --test
```

Use `--arch x86` or `--arch x86_64` to select one architecture.  `--jobs`,
`--build-root`, and `--build-type` control the shared wrapper in the same way
as for a native build.  Run `./build.sh --help` for the complete interface.

Each architecture has an isolated dependency prefix, build tree, Wine
prefix, and package directory:

```text
build/windows/x86/package/
build/windows/x86_64/package/
```

The packages contain `xpilot-ng-server.exe`, `xpilot-ng-sdl.exe`, the game
data, and the license.  The Wine suite verifies the PE architecture, runs the
portable networking and SDL dependency tests, validates a TCP-contact server's
metaserver advertisement, and starts the packaged server and SDL client for
all four UDP/TCP contact and gameplay combinations.  Every session must join,
create an OpenGL context, initialize its text renderers, and quit cleanly.  A
software OpenGL renderer is selected so the graphical checks do not depend on
host GPU drivers.

The Wine prefixes are persistent under the selected build root.  Build and
test output never needs to be written into the source tree.
