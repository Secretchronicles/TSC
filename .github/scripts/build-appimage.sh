#!/bin/bash
#
# build-appimage.sh - turn a staged TSC into an AppImage, with its checksums.
#
# An AppImage is one file that runs on any Linux distribution without being
# installed, which is the thing a .deb cannot be: a .deb is built against one
# distribution's libraries and only installs there. That is also what decides
# how this is built - on an OLD distribution, with SFML compiled from source.
# glibc is backward compatible but not forward compatible, so an AppImage can
# only run on a system at least as new as the one it was built on; building it
# on Debian bookworm is what lets it run on everything from bookworm onwards,
# and SFML 3 has to come from source there because bookworm ships SFML 2.
#
# The libraries are bundled by linuxdeploy, which reads the ELF headers, copies
# every shared object the binary needs into the AppDir, and writes an AppRun
# that puts them on the search path.
#
# Environment:
#   TSC_VERSION   e.g. 2.2.0-beta2                        (required)
#   TSC_STAGE     the staging directory from build-tsc.sh (default: ./stage)
#   TSC_PREFIX    the prefix it was staged under          (default: /usr)
#   TSC_APPIMAGE_ARCH  x86_64 | aarch64 | armhf  (default: from uname -m)
#   TSC_OUT       where the AppImage is written           (default: ./dist)
#   TSC_APPIMAGE_DISTRO   the distribution it was built on and is for, e.g. ubuntu2604;
#                         it goes into the name: TSC-<version>-<distro>-<arch>
#                         (default: none - the generic bookworm build, TSC-<version>-<arch>)
#   TSC_APPIMAGE_ALIASES  space-separated further distro names the SAME file is also
#                         published under, e.g. raspberrypios: the same file under the
#                         name people look for it by, built and checked once
#
# Produces, in $TSC_OUT, named the way the .deb assets are named - the game,
# the version, what it runs on - and with the same two checksum files beside it.
# The checksum files KEEP the full asset name and append .md5sum/.sha256sum, so
# the AppImage and the Flatpak of the same architecture (both TSC-<version>-<arch>)
# do not write to the same checksum file and overwrite each other:
#   TSC-<version>-<arch>.AppImage
#   TSC-<version>-<arch>.AppImage.md5sum
#   TSC-<version>-<arch>.AppImage.sha256sum

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

version="${TSC_VERSION:?TSC_VERSION is required}"
stage="${TSC_STAGE:-$repo_root/stage}"
prefix="${TSC_PREFIX:-/usr}"
out="${TSC_OUT:-$repo_root/dist}"

case "${TSC_APPIMAGE_ARCH:-$(uname -m)}" in
    x86_64|amd64)          arch=x86_64  ;;
    aarch64|arm64)         arch=aarch64 ;;
    armv7l|armhf|arm)      arch=armhf   ;;
    *)
        echo "build-appimage.sh: no AppImage runtime exists for" \
             "${TSC_APPIMAGE_ARCH:-$(uname -m)}." >&2
        echo "  AppImage supports x86_64, aarch64 and armhf only; the other" >&2
        echo "  architectures are covered by the .deb packages." >&2
        exit 1
        ;;
esac

if [ ! -d "$stage" ]; then
    echo "build-appimage.sh: no staged build at $stage - run build-tsc.sh first" >&2
    exit 1
fi

appdir="$repo_root/AppDir"
rm -rf "$appdir"
mkdir -p "$appdir"
cp -a "$stage"/. "$appdir"/

# linuxdeploy wants the prefix at the AppDir root. TSC is compiled for /usr, so
# usually it already is; move it if a different prefix was staged.
if [ "$prefix" != "/usr" ]; then
    mkdir -p "$appdir/usr"
    cp -a "$appdir$prefix"/. "$appdir/usr"/
    rm -rf "${appdir:?}$prefix"
fi

desktop="$appdir/usr/share/applications/org.secretchronicles.TSC.desktop"
icon="$appdir/usr/share/icons/hicolor/128x128/apps/org.secretchronicles.TSC.png"
for f in "$desktop" "$icon"; do
    if [ ! -f "$f" ]; then
        echo "build-appimage.sh: $f is missing from the staged build" >&2
        exit 1
    fi
done

tools="$repo_root/.appimage-tools"
mkdir -p "$tools/bin"

# linuxdeploy and appimagetool are themselves AppImages, and a type-2 AppImage's
# runtime is a STATIC-PIE binary. On armhf this job runs under qemu-user (the
# x86_64 runner emulates the armhf container), and qemu-user CANNOT load a
# static-PIE executable: running `linuxdeploy-armhf.AppImage` dies with "cannot
# execute binary file: Exec format error" BEFORE it starts - so
# APPIMAGE_EXTRACT_AND_RUN, which the runtime reads only once it is running, can
# never take effect. The fix is to never execute the runtime: unpack the
# appended squashfs (whose inner linuxdeploy/appimagetool is a DYNAMIC binary
# qemu-user can run) and call that directly. It also drops the FUSE dependency
# the old APPIMAGE_EXTRACT_AND_RUN worked around, so it is done for every arch.
#
# extract_appimage IMAGE DEST - unpack a type-2 AppImage without running it, by
# finding the appended squashfs (its little-endian 'hsqs' magic) and unsquashing
# from that byte offset. Needs squashfs-tools (unsquashfs).
extract_appimage() {
    local img="$1" dest="$2" off offs
    # The squashfs is APPENDED after the ELF runtime, so the offset we want is the
    # 'hsqs' that BEGINS a valid superblock - which is not necessarily the FIRST
    # 'hsqs' byte sequence in the file. That four-byte sequence can also occur
    # inside the ELF runtime, and taking `-m1` (the first match) landed on such a
    # spurious one, so unsquashfs died with
    #   FATAL ERROR: Can't find a valid SQUASHFS superblock on linuxdeploy.AppImage
    # (this is exactly how the armhf AppImage job failed, after TSC had already
    # built). So collect EVERY candidate offset and keep the first that actually
    # unsquashes into a tree with an AppRun - a spurious offset does not.
    offs="$(LC_ALL=C grep -a -b -o 'hsqs' "$img" | cut -d: -f1)"
    [ -n "$offs" ] || { echo "extract_appimage: no squashfs magic in $img" >&2; return 1; }
    for off in $offs; do
        rm -rf "$dest"
        if unsquashfs -q -f -d "$dest" -o "$off" "$img" 2>/dev/null \
            && [ -x "$dest/AppRun" ]; then
            return 0
        fi
    done
    rm -rf "$dest"
    echo "extract_appimage: no 'hsqs' offset in $img held a valid squashfs with an AppRun" >&2
    return 1
}

if [ ! -x "$tools/linuxdeploy/AppRun" ]; then
    wget -q -O "$tools/linuxdeploy.AppImage" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${arch}.AppImage"
    extract_appimage "$tools/linuxdeploy.AppImage" "$tools/linuxdeploy"
fi
if [ ! -x "$tools/appimagetool/AppRun" ]; then
    wget -q -O "$tools/appimagetool.AppImage" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${arch}.AppImage"
    extract_appimage "$tools/appimagetool.AppImage" "$tools/appimagetool"
fi

# linuxdeploy's `--output appimage` plugin runs `appimagetool` from PATH; point
# it at the EXTRACTED one so appimagetool is never exec'd as a static-PIE
# AppImage either.
cat > "$tools/bin/appimagetool" <<EOF
#!/bin/sh
exec "$tools/appimagetool/AppRun" "\$@"
EOF
chmod +x "$tools/bin/appimagetool"
export PATH="$tools/bin:$PATH"

export ARCH="$arch"
# Written into the AppImage's own metadata, and into the file name below.
export VERSION="$version"
# Belt and braces for any nested tool that still tries to self-mount.
export APPIMAGE_EXTRACT_AND_RUN=1

mkdir -p "$out"
name="TSC-${version}-${TSC_APPIMAGE_DISTRO:+${TSC_APPIMAGE_DISTRO}-}${arch}"

# Deploy only; the AppImage is packed below, AFTER the libraries have been made portable.
"$tools/linuxdeploy/AppRun" \
    --appdir "$appdir" \
    --executable "$appdir/usr/bin/tsc" \
    --desktop-file "$desktop" \
    --icon-file "$icon"

# THE LIBRARIES THE AppImage CARRIES MUST NOT NEED WHAT THE HOST MAY NOT HAVE.
#
# linuxdeploy bundles libGLU.so.1, and on Debian and Ubuntu that library is linked against
# libOpenGL.so.0 - one of the GLVND dispatch libraries, in the package libopengl0. linuxdeploy
# does not bundle those (they must pair with the host's GL driver), and libopengl0 is not part of
# every desktop: on a clean Debian bookworm, Debian trixie, Ubuntu 22.04, 24.04, 26.04 and Fedora 42
# container the AppImage failed with
#   error while loading shared libraries: libOpenGL.so.0: cannot open shared object file
# before it drew a frame. tsc itself links libGL.so.1, which exists wherever there is any OpenGL at
# all and exports every function libOpenGL does, so the bundled libraries are pointed at that.
# (GLU only calls ordinary OpenGL functions.)
if ! command -v patchelf >/dev/null 2>&1; then
    echo "build-appimage.sh: patchelf is required (apt install patchelf): without it the AppImage" \
         "would need libOpenGL.so.0, which many systems do not have." >&2
    exit 1
fi
while IFS= read -r -d '' lib; do
    if patchelf --print-needed "$lib" 2>/dev/null | grep -qx 'libOpenGL.so.0'; then
        patchelf --replace-needed libOpenGL.so.0 libGL.so.1 "$lib"
        echo "build-appimage.sh: ${lib#$appdir/} needs libGL.so.1 instead of libOpenGL.so.0"
    fi
done < <(find "$appdir/usr" -type f \( -name '*.so' -o -name '*.so.*' -o -path '*/bin/*' \) -print0)

# The same, as a check that cannot be forgotten: nothing in the AppImage may need a GLVND-only
# library that is not inside it.
glvnd_only='libOpenGL.so.0 libGLdispatch.so.0 libGLX.so.0'
leaks=""
while IFS= read -r -d '' lib; do
    for want in $glvnd_only; do
        if patchelf --print-needed "$lib" 2>/dev/null | grep -qx "$want" && [ ! -e "$appdir/usr/lib/$want" ]; then
            leaks="$leaks ${lib#$appdir/}->$want"
        fi
    done
done < <(find "$appdir/usr" -type f \( -name '*.so' -o -name '*.so.*' -o -path '*/bin/*' \) -print0)
if [ -n "$leaks" ]; then
    echo "build-appimage.sh: these would not start on a system without the GLVND libraries:$leaks" >&2
    exit 1
fi

appimagetool --no-appstream "$appdir" "$out/${name}.AppImage"
chmod +x "$out/${name}.AppImage"

# Does the packed AppImage actually FIND its game data? This is what the
# 2.2.0-beta2 AppImage got wrong: it carried all 240 MB of it and then looked
# for it at the compiled-in /usr/share/tsc, which does not exist on the machine
# it runs on, so it aborted with
#   CEGUI::FileIOException ... /usr/share/tsc/gui/schemes/TSCLook256.scheme does
#   not exist
# before drawing a single frame. `tsc --print-paths` resolves the paths exactly
# as the game does and exits nonzero when the data is not there - and it opens
# no window, so it runs in this container.
#
# The AppImage is UNPACKED for the check instead of being executed: its runtime
# is a static-PIE binary, which qemu-user cannot load on the emulated armhf job
# (the same reason linuxdeploy is unpacked above). The tsc inside is dynamic and
# runs. Unpacked is also the harder case - no AppImage runtime means no $APPDIR
# is exported, so this proves the game finds its data from its own location.
# A CHECK THAT CANNOT RUN IS NOT A FAILED CHECK. The AppImage is finished and
# correct as far as anything here knows; if unsquashfs is missing, or the
# unpacking does not produce a tsc to run, that is this script's problem and not
# a reason to withhold a release. Those cases warn. Only tsc itself SAYING the
# data is not where it will look for it fails the build - that is the bug this
# is here for, and it is not environmental.
check="$repo_root/.appimage-check"
rm -rf "$check"
if ! command -v unsquashfs >/dev/null 2>&1; then
    echo "build-appimage.sh: WARNING - no unsquashfs, so the packed AppImage" \
         "was not checked for whether it finds its game data." >&2
elif ! extract_appimage "$out/${name}.AppImage" "$check"; then
    echo "build-appimage.sh: WARNING - could not unpack the AppImage just built," \
         "so it was not checked for whether it finds its game data." >&2
elif [ ! -x "$check/usr/bin/tsc" ]; then
    echo "build-appimage.sh: WARNING - no usr/bin/tsc in the unpacked AppImage," \
         "so it was not checked for whether it finds its game data." >&2
# TSC reads $HOME for the XDG user directories and refuses to start without
# one; a container that has none would fail this check for the wrong reason.
elif ! env HOME="${HOME:-/tmp}" "$check/usr/bin/tsc" --print-paths; then
    echo "build-appimage.sh: the packed AppImage cannot find its game data." \
         "It would abort on startup the way the 2.2.0-beta2 one did, so it is" \
         "not published." >&2
    exit 1
fi
rm -rf "$check"

# The same bytes under the other distributions' names (see TSC_APPIMAGE_ALIASES): built and
# checked once, named for each. Checksum files are per name, as everywhere else.
names="$name"
for alias in ${TSC_APPIMAGE_ALIASES:-}; do
    case "$alias" in *[!a-z0-9]*|"") echo "build-appimage.sh: bad alias '$alias'" >&2; exit 1 ;; esac
    alias_name="TSC-${version}-${alias}-${arch}"
    cp "$out/${name}.AppImage" "$out/${alias_name}.AppImage"
    chmod +x "$out/${alias_name}.AppImage"
    names="$names $alias_name"
done

(
    cd "$out"
    for each in $names; do
        md5sum    "${each}.AppImage" > "${each}.AppImage.md5sum"
        sha256sum "${each}.AppImage" > "${each}.AppImage.sha256sum"
    done
)

echo "Built $out/${name}.AppImage"
ls -l "$out/${name}.AppImage"
