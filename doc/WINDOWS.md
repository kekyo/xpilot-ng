# Linux-hosted Windows builds

XPilot Infinity's supported Windows build is a Linux cross-build.  It targets
32-bit x86 and 64-bit x86_64 Windows with MinGW-w64; building with Visual
Studio or on a Windows host is not supported.

## Prerequisites

Install the following host tools before building:

- the `i686-w64-mingw32` and `x86_64-w64-mingw32` GCC toolchains;
- CMake, Make, pkg-config, Git, Node.js, and Info-ZIP `zip`;
- NSIS 3 (`makensis`) for the installer;
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
and linked statically.  OpenAL Soft and freealut are also cross-compiled from
pinned submodules; they remain shared libraries so the required runtime DLLs
can be distributed with the client.

## Build and test

Use a separate configured build directory for each architecture.  For
64-bit Windows, run:

```sh
mkdir -p build/windows/x86_64
cd build/windows/x86_64
../../../configure --host=x86_64-w64-mingw32
make -j4
make windows-installer
make check
```

For 32-bit Windows, use another build directory and host triplet:

```sh
mkdir -p build/windows/x86
cd build/windows/x86
../../../configure --host=i686-w64-mingw32
make -j4
make windows-installer
make check
```

The first `make` cross-builds the pinned dependencies before building XPilot
Infinity. `make windows-package` assembles the portable executable and game
data tree. `make windows-installer` depends on that tree and compiles
`xpilot-infinity-setup.exe` with NSIS. `make check` builds the supported
Windows tests and runs the complete installer and application suite through
Wine. Repeating `make` reuses the dependency build until one of its inputs or
configuration changes. Do not reconfigure one build directory for a different
host architecture.

The dependency CMake configuration defaults to `Release`.  Override it at
configure time with `--with-mingw-deps-build-type=TYPE`.  A custom absolute
CMake toolchain file can be selected with
`--with-mingw-toolchain-file=PATH`.  `make mingw-deps-clean` removes the
configured tree's dependency build and prefix when a complete dependency
rebuild is required.

As a convenience, the top-level wrapper performs the same configure and make
sequence for both architectures and creates versioned ZIP archives and NSIS
installers:

```sh
./build.sh --target windows --arch all --test
```

Running `./build.sh` without `--target` builds these two Windows architectures
after the native target.  Use `--target windows` to omit the native build.
Use `--arch x86` or `--arch x86_64` to select one architecture. The wrapper
delegates dependency builds, portable packaging, and Wine tests to each
configured tree, then creates the release ZIP and installer. `--jobs`,
`--build-root`, and `--build-type` select their corresponding configure and
make settings. `--artifact-root` selects the artifact destination, and
`--package-version` overrides the version normally derived by `screw-up`.
Run `./build.sh --help` for the complete interface.

Each architecture has an isolated dependency prefix, build tree, Wine
prefix, and package directory:

```text
build/windows/x86/package/
build/windows/x86_64/package/
artifacts/windows/xpilot-infinity-<version>-windows-x86.zip
artifacts/windows/xpilot-infinity-<version>-windows-x86_64.zip
artifacts/windows/xpilot-infinity-<version>-windows-x86-setup.exe
artifacts/windows/xpilot-infinity-<version>-windows-x86_64-setup.exe
```

The portable package trees contain `xpilot-infinity-server.exe`,
`xpilot-infinity-sdl.exe`, `OpenAL32.dll`, `libalut.dll`, game data including
sound samples, and the XPilot Infinity, OpenAL Soft, and freealut licenses.
Info-ZIP `zip` uses its maximum compression setting. Entries are sorted, use a
fixed timestamp and permissions, and omit host-specific extra fields, so
unchanged package trees produce byte-identical archives. The NSIS installers
add an icon-bearing Start menu shortcut and an optional manual-start
`XPilotInfinityServer` service; the ZIP files remain portable and do not
require installation. The Wine suite
installs and uninstalls both component selections, launches the shortcut
without arguments, starts and contacts the registered service, verifies the
PE architecture, runs the portable networking and SDL dependency tests,
validates a TCP-contact server's metaserver advertisement, and starts the
packaged server and SDL client for all four UDP/TCP contact and gameplay
combinations. Every session must join, create an OpenGL context, initialize its
text renderers, and quit cleanly. A software OpenGL renderer is selected so the
graphical checks do not depend on host GPU drivers. If Wine or a real Windows
system has no usable sound device, OpenAL initialization is disabled for that
process and gameplay continues silently.

The Wine prefixes are persistent in their configured build directories.
Build and test output never needs to be written into the source tree.
