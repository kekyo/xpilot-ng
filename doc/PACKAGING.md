# Distribution packages

XPilot NG builds release artifacts from a Linux host.  Debian packages are
built inside target-specific Podman containers so that their runtime
dependencies are resolved against the selected Debian or Ubuntu release.

## Host prerequisites

Install Podman, `qemu-user-static`, `dpkg-dev`, binutils, Node.js, and
[`screw-up`](https://github.com/kekyo/screw-up).  Initialize the pinned source
dependencies before packaging:

```sh
git submodule update --init --recursive
```

Prepare the reusable package build images, then build the complete matrix:

```sh
./prereq.sh
./build_package_all.sh
```

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
game data, documentation, and manual pages.  The build validates package
metadata, installed paths, and the ELF architecture of every executable.
Artifacts are written as:

```text
artifacts/deb/xpilot-ng-<version>-<distro>-<release>-<deb-arch>.deb
```

## Windows archives

The Linux-hosted MinGW build creates separate 32-bit and 64-bit Windows ZIP
archives because both packages use the same executable names:

```sh
./build.sh --target windows --arch all --test
```

The ZIP files contain the SDL client, dedicated server, game data, and license
from the corresponding `windows-package` directory.  Entries are sorted and
assigned a fixed timestamp so the result is reproducible for identical input.
Artifacts are written as:

```text
artifacts/windows/xpilot-ng-<version>-windows-x86.zip
artifacts/windows/xpilot-ng-<version>-windows-x86_64.zip
```

Use `--artifact-root` to select another output directory and
`--package-version` to override the version derived by `screw-up`.  The full
cross-build and Wine prerequisites are documented in `doc/WINDOWS.md`.
