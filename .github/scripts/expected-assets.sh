#!/bin/bash
#
# expected-assets.sh <version> - every asset a complete TSC release carries.
#
# Prints one line per PRIMARY asset:
#
#   <kind> <selector> <asset-name> <sums|nosums>
#
# `kind` is the workflow that builds it (deb, appimage, flatpak, windows, mac);
# `selector` is the value that workflow's `only` input filters its matrix on, so
# release-all-missing.yml can say "build these three and nothing else".
#
# The CHECKSUMS are not listed, they are DERIVED. Every one of these workflows
# writes `<asset>.md5sum` and `<asset>.sha256sum` beside the file, KEEPING the
# asset's full name - TSC-2.2.0-beta2-x86_64.AppImage is checksummed as
# TSC-2.2.0-beta2-x86_64.AppImage.md5sum - so `sums` means "and those two as
# well". Keeping the extension is what stops two assets that share a stem (the
# x86_64 AppImage and the x86_64 Flatpak, the win64 .exe and the win64 .7z) from
# writing to the same checksum file and clobbering each other - which is exactly
# why the earlier extension-dropped naming left some assets with no checksum. Such
# an asset counts as present only when all three are there: a package whose
# checksum upload failed is a half-published release, and treating the package
# alone as present would leave it that way.
#
# `nosums` is for the build-script zips, which are published without checksums.
# Requiring them would leave those two permanently "missing".
#
# THE LISTS BELOW MUST MATCH THE WORKFLOWS' MATRICES. Each is named beside it.
# If a distro or an architecture is added there and not here, this will not
# notice it is missing - which is the one way this script can be wrong without
# anything failing.
#
# NOTE on v2.2.0-beta2: its assets were checksummed under the OLD extension-
# dropped naming, so the x86_64 and aarch64 AppImages and the win64 .exe went
# without a checksum of their own - the same-stem Flatpak and .7z overwrote the
# shared checksum file. Re-running "Release all" or "Release all missing" over
# that release with this workflow rebuilds them and writes the per-asset checksum
# files (<asset>.md5sum / <asset>.sha256sum), filling those gaps in.

set -euo pipefail

v="${1:?usage: expected-assets.sh <version>   e.g. 2.2.0-beta2}"

# Deb.yml: two distributions times six architectures, plus the one
# architecture-independent data package built once for all of them.
for distro in resolute forky; do
    for arch in amd64 arm64 armhf ppc64el riscv64 s390x; do
        echo "deb ${distro}-${arch} TSC-${v}-${distro}-${arch}.deb sums"
    done
done
echo "deb data TSC-${v}-data-all.deb sums"

# The build scripts beta1 published, one zip per distribution: the record of how
# that distribution's packages were made. They are assembled by Deb.yml's attach
# job out of every architecture's artifacts, not by one matrix entry, so they
# have no architecture of their own - which is why a MISSING one asks for the
# whole Deb workflow rather than a filtered run (see release-all-missing.yml).
# Their names carry no version, so they are the one pair here that does not
# start with TSC-${v}.
for distro in resolute forky; do
    echo "deb scripts-${distro} ${distro}-build-deb.zip nosums"
done

# AppImage.yml: the AppImage of each CPU (built on Ubuntu 22.04, runs on glibc 2.35 and newer),
# and the same file under the Raspberry Pi OS name for the two ARM ones. The selector is the matrix entry's `selector`, which the workflow's `only` accepts.
for arch in x86_64 aarch64 armhf; do
    echo "appimage ${arch} TSC-${v}-${arch}.AppImage sums"
done
for arch in aarch64 armhf; do
    echo "appimage raspberrypios-${arch} TSC-${v}-raspberrypios-${arch}.AppImage sums"
done

# Flatpak.yml
for arch in x86_64 aarch64; do
    echo "flatpak ${arch} TSC-${v}-${arch}.flatpak sums"
done

# Windows.yml: the two builds, and the combined installer that carries both and
# is only made when both exist.
for name in win64 win32; do
    echo "windows ${name} TSC-${v}-${name}.exe sums"
done
echo "windows combined TSC-${v}-windows.exe sums"

# Mac.yml: macOS 15 and 26 on arm64, macOS 15 on Intel, and the universal image
# lipo welds from the two arm64/x86_64 builds.
echo "mac 15-arm64 TSC-${v}-macos15-arm64.dmg sums"
echo "mac 26-arm64 TSC-${v}-macos26-arm64.dmg sums"
echo "mac 15-x86_64 TSC-${v}-macos15-x86_64.dmg sums"
echo "mac universal TSC-${v}-macos-universal.dmg sums"
