# Distribution packages

XPilot Infinity builds release artifacts from a Linux host.  Debian packages are
built inside target-specific Podman containers so that their runtime
dependencies are resolved against the selected Debian or Ubuntu release.

## Host prerequisites

Install Podman, `qemu-user-static`, `dpkg-dev`, binutils, Node.js, and
[`screw-up`](https://github.com/kekyo/screw-up).  Initialize the pinned source
dependencies before packaging:

```sh
git submodule update --init --recursive
```

Prepare the reusable Linux package images, install the MinGW/Wine prerequisites
described in `doc/WINDOWS.md`, then build every package:

```sh
./prereq.sh
./build_package_all.sh
```

This generates the complete Debian/Ubuntu matrix and then the Windows x86
(32-bit) and x86_64 (64-bit) ZIP archives.  Distribution, release, and
architecture filters apply only to the Debian/Ubuntu matrix; both Windows
architectures are always built.  The `--version`, `--jobs`, and `--debug`
options apply to both package families.

The supported Debian package matrix is:

| Distribution | Release | Architectures |
| --- | --- | --- |
| Debian | bookworm | amd64, i386, arm64, armhf |
| Debian | trixie | amd64, i386, arm64, armhf, riscv64 |
| Ubuntu | 22.04 | amd64, arm64 |
| Ubuntu | 24.04 | amd64, arm64 |
| Ubuntu | 26.04 | amd64, arm64 |

Both image preparation and package generation accept `--distro`, `--release`,
and `--arch` comma-separated filters.  For example:

```sh
./prereq.sh --distro ubuntu --release 24.04 --arch amd64
./build_package.sh --target deb --distro ubuntu --release 24.04 --arch amd64
```

Architecture aliases include `x86_64|amd64`, `i686|i386`,
`arm64|aarch64`, and `armv7l|armv7|armhf`.  Ubuntu `jammy` and `noble` are
accepted as release aliases.  Add `--debug` for an unoptimized build, or use
`--version` to override the version derived by `screw-up`.

Each package contains both graphical clients, the dedicated server, utilities,
game data including the sound map and samples, documentation, and manual
pages.  Its generated `Depends` field includes the OpenAL and freealut runtime
packages (`libopenal1` and `libalut0`).  The build validates those dependencies,
installed paths, and the ELF architecture of every executable.  Artifacts are
written as:

```text
artifacts/deb/xpilot-infinity-<version>-<distro>-<release>-<deb-arch>.deb
```

## Windows archives

The Linux-hosted MinGW build creates separate 32-bit and 64-bit Windows ZIP
archives because both packages use the same executable names.
`build_package_all.sh` creates both archives after the Linux packages.  To
build and test only the Windows archives, run:

```sh
./build.sh --target windows --arch all --test
```

The ZIP files contain the SDL client, dedicated server, game data including
sound samples, `OpenAL32.dll`, `libalut.dll`, and the corresponding
project licenses from the `windows-package` directory.  Entries are sorted
and assigned a fixed timestamp so the result is reproducible for identical
input.  Artifacts are written as:

```text
artifacts/windows/xpilot-infinity-<version>-windows-x86.zip
artifacts/windows/xpilot-infinity-<version>-windows-x86_64.zip
```

Use `--artifact-root` to select another output directory and
`--package-version` to override the version derived by `screw-up`.  The full
cross-build and Wine prerequisites are documented in `doc/WINDOWS.md`.
