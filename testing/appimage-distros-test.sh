#!/bin/bash
#
# appimage-distros-test.sh - that the AppImage runs on many Linux distributions, and that the
# workflow, the list of release assets, the naming and the download pages agree.
#
# THE FAULT THIS PINS. The AppImage built on Debian bookworm (glibc 2.36) did not start on Ubuntu
# 22.04 (2.35), and - on every distribution tried, bookworm included - stopped with
#   error while loading shared libraries: libOpenGL.so.0
# because linuxdeploy bundled libGLU.so.1, which needs libOpenGL.so.0 (package libopengl0), and
# libopengl0 is not part of every desktop. So:
#   * it is built on Ubuntu 22.04, the oldest supported glibc (2.35) with a new enough CMake;
#   * build-appimage.sh points bundled libraries at libGL.so.1 and refuses to pack an AppImage
#     that still needs a GLVND-only library;
#   * expected-assets.sh is wrong without anything failing if a matrix entry is added to the
#     workflow and not to it - the matrix and that list are compared here.
# The real proof is the container run recorded in AppImage.yml's header (Debian bookworm and
# trixie, Ubuntu 22.04, 24.04, 26.04, Fedora 42); it needs a built AppImage, so it is not here.

set -uo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/.." && pwd)"
wf="$repo/.github/workflows/AppImage.yml"
script="$repo/.github/scripts/build-appimage.sh"

failures=0; checks=0
ok() { checks=$((checks + 1)); echo "  ok   - $1"; }
bad() { checks=$((checks + 1)); failures=$((failures + 1)); echo "  FAIL - $1"; }

echo "===== AppImage on many distributions"

selectors="$(sed -n 's/^ *selector: \(.*\)$/\1/p' "$wf" | sort)"
alsos="$(sed -n 's/^ *also: \(.*\)$/\1/p' "$wf" | sort)"
expected="$(bash "$repo/.github/scripts/expected-assets.sh" 9.9.9 | awk '$1 == "appimage" { print $2 }' | sort)"
want="$(printf '%s\n%s\n' "$selectors" "$alsos" | sed '/^$/d' | sort)"
[ "$want" = "$expected" ] && ok "the matrix and expected-assets.sh list the same AppImages" \
    || bad "matrix selectors+also ($(echo $want)) differ from expected-assets.sh ($(echo $expected))"
for target in x86_64 aarch64 armhf raspberrypios-armhf raspberrypios-aarch64; do
    echo "$expected" | grep -qx "$target" && ok "$target is in the release" || bad "$target is missing from the release"
done

# AppImage has a runtime for these three CPUs only
others="$(sed -n 's/^ *- arch: \(.*\)$/\1/p' "$wf" | grep -vxE 'x86_64|aarch64|armhf' || true)"
[ -z "$others" ] && ok "negative: no CPU without an AppImage runtime is built" || bad "AppImage cannot exist for: $others"

# the oldest supported glibc, not a newer one
images="$(sed -n 's/^ *image: \(.*\)$/\1/p' "$wf" | sort -u)"
[ "$images" = "ubuntu:22.04" ] && ok "every AppImage is built on ubuntu:22.04 (glibc 2.35)" || bad "the build images are: $images"
! grep -qE 'image: (debian:bookworm|ubuntu:2[4-9])' "$wf" && ok "negative: nothing is built on a newer glibc (bookworm 2.36 excluded Ubuntu 22.04)" || bad "a newer image is used"
[ "$(grep -c "selector:" "$wf")" -eq 3 ] && ok "negative: no per-distribution builds - one file per CPU runs everywhere" || bad "per-distribution builds are back"

# Raspberry Pi OS: the same file under its name, built once
grep -B1 -A3 'also: raspberrypios-aarch64' "$wf" | grep -q 'aliases: raspberrypios' && ok "raspberrypios-aarch64 is a copy of the aarch64 file" || bad "raspberrypios-aarch64 is not an alias"
grep -B1 -A3 'also: raspberrypios-armhf' "$wf" | grep -q 'aliases: raspberrypios' && ok "raspberrypios-armhf is a copy of the armhf file" || bad "raspberrypios-armhf is not an alias"

# the portability fix
grep -q 'patchelf' "$wf" && ok "patchelf is installed for the build" || bad "the build does not install patchelf"
grep -q "replace-needed libOpenGL.so.0 libGL.so.1" "$script" && ok "bundled libraries are pointed at libGL.so.1" || bad "libOpenGL.so.0 is not replaced"
grep -q "glvnd_only='libOpenGL.so.0 libGLdispatch.so.0 libGLX.so.0'" "$script" && grep -q "would not start on a system without the GLVND libraries" "$script" \
    && ok "an AppImage that still needs a GLVND-only library is refused" || bad "no guard against GLVND-only libraries"
dep_line="$(grep -n 'replace-needed' "$script" | head -1 | cut -d: -f1)"
pack_line="$(grep -n '^appimagetool' "$script" | head -1 | cut -d: -f1)"
[ -n "$dep_line" ] && [ -n "$pack_line" ] && [ "$dep_line" -lt "$pack_line" ] && ok "the libraries are fixed BEFORE the AppImage is packed" || bad "the AppImage is packed before the libraries are fixed"
! grep -A2 -- '--icon-file "$icon"' "$script" | grep -q -- '--output' && ok "negative: linuxdeploy no longer packs it before the fix" || bad "linuxdeploy still packs the AppImage"

# the name
grep -q 'name="TSC-${version}-${TSC_APPIMAGE_DISTRO:+${TSC_APPIMAGE_DISTRO}-}${arch}"' "$script" && ok "the file name can carry a distribution" || bad "the file name cannot carry a distribution"
grep -q 'TSC_APPIMAGE_ALIASES' "$script" && grep -q 'bad alias' "$script" && ok "an alias is copied from the one build, and a bad alias is refused" || bad "alias handling is missing"
grep -q 'md5sum    "${each}.AppImage"' "$script" && ok "every name gets its own checksum files" || bad "checksums are not written per name"

# the download page
python3 - "$repo" <<'PY' && ok "the download page labels and orders the names" || bad "the download page does not handle the names"
import importlib.util, sys
sp = importlib.util.spec_from_file_location('u', sys.argv[1] + '/.github/scripts/update-download-pages.py')
u = importlib.util.module_from_spec(sp); sp.loader.exec_module(u)
keys = ['raspberrypios-armhf', 'x86_64', 'raspberrypios-aarch64', 'armhf', 'aarch64']
assert sorted(keys, key=u.appimage_sort) == ['x86_64', 'aarch64', 'armhf', 'raspberrypios-aarch64', 'raspberrypios-armhf']
assert u.cpu_label('raspberrypios-aarch64') == 'Raspberry Pi OS - 64-bit ARM'
assert u.cpu_label('x86_64') == '64-bit x86', 'the plain label is unchanged'
assert u.classify('TSC-2.2.0-beta3-raspberrypios-armhf.AppImage', '2.2.0-beta3') == ('appimage', 'raspberrypios-armhf')
PY

echo
if [ "$failures" -eq 0 ]; then echo "===== $checks checks, all passed"; else echo "===== $checks checks, $failures FAILED"; fi
exit "$failures"
