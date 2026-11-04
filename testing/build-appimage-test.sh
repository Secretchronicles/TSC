#!/bin/bash
#
# build-appimage-test.sh - what .github/scripts/build-appimage.sh must and must
# not fail a release for.
#
# The script packs the AppImage and then checks it: it unpacks what it just
# built and runs `tsc --print-paths`, which resolves the game data directory
# exactly as the game does and exits nonzero when the data is not where it will
# look. That check exists because the 2.2.0-beta2 AppImage shipped carrying all
# 240 MB of its data and still aborted on startup with
#
#   CEGUI::FileIOException ... /usr/share/tsc/gui/schemes/TSCLook256.scheme
#   does not exist
#
# A check that runs on every release build has to be careful about WHEN it
# fails. Nothing environmental - a missing unsquashfs, an AppImage that would
# not unpack here - may withhold a finished AppImage; those warn. Only tsc
# saying the data is missing fails the build. This pins both halves, because
# getting it the other way round would either block releases for tooling
# reasons or wave the original bug through.

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/.." && pwd)"
script="$repo/.github/scripts/build-appimage.sh"

failures=0
checks=0

ok() { checks=$((checks + 1)); echo "  ok   - $1"; }
bad() { checks=$((checks + 1)); failures=$((failures + 1)); echo "  FAIL - $1"; }

has() {
    if grep -q -- "$2" "$script"; then ok "$1"; else bad "$1"; fi
}
hasnt() {
    if grep -q -- "$2" "$script"; then bad "$1"; else ok "$1"; fi
}

echo "===== build-appimage.sh"

if [ -f "$script" ]; then ok "the script is there"; else bad "the script is missing"; exit 1; fi

bash -n "$script" && ok "it parses" || bad "it does not parse"

has 'it checks the packed AppImage with --print-paths' -- '--print-paths'
has 'it unpacks the AppImage rather than executing it (qemu cannot load the runtime)' 'extract_appimage "\$out/\${name}.AppImage"'
has 'it gives the check a HOME, which TSC needs for its XDG directories' 'HOME="\${HOME:-/tmp}"'

# The warn-only paths: each is a reason the check could not RUN.
has 'a missing unsquashfs warns' 'command -v unsquashfs'
has 'an AppImage that will not unpack warns' 'could not unpack the AppImage just built'
has 'no tsc inside the unpacked tree warns' 'no usr/bin/tsc in the unpacked AppImage'

# ...and none of them may exit.
# From the comment that opens the check down to the `fi` that closes it. The
# `rm -rf` lines around it are not landmarks: there is one before the block too.
warn_block="$(awk '/A CHECK THAT CANNOT RUN IS NOT A FAILED CHECK/ {inside=1} inside {print} inside && /^fi$/ {exit}' "$script")"
if [ -z "$warn_block" ]; then
    bad "the check block was not found - this test needs updating with the script"
else
    exits="$(printf '%s\n' "$warn_block" | grep -c 'exit 1')"
    if [ "$exits" -eq 1 ]; then
        ok "exactly one exit in the check block: the real failure"
    else
        bad "expected exactly one 'exit 1' in the check block, found $exits"
    fi

    if printf '%s\n' "$warn_block" | grep -q 'elif ! env HOME=.* --print-paths; then'; then
        ok "the one exit is behind tsc's own verdict"
    else
        bad "the exit is not behind the --print-paths result"
    fi

    warns="$(printf '%s\n' "$warn_block" | grep -c 'WARNING')"
    if [ "$warns" -eq 3 ]; then
        ok "three environmental cases warn instead of failing"
    else
        bad "expected 3 WARNING cases, found $warns"
    fi
fi

# The AppImage must be checked BEFORE its checksums are written, so a rejected
# one leaves nothing behind that looks publishable.
check_line="$(grep -n 'A CHECK THAT CANNOT RUN' "$script" | cut -d: -f1)"
sum_line="$(grep -n 'md5sum    "\${each}.AppImage"' "$script" | cut -d: -f1)"
if [ -n "$check_line" ] && [ -n "$sum_line" ] && [ "$check_line" -lt "$sum_line" ]; then
    ok "the check runs before the checksums are written"
else
    bad "the check must run before the checksums (check=$check_line sums=$sum_line)"
fi

hasnt 'negative: the check does not run the AppImage runtime directly' '"\$out/\${name}.AppImage" --print-paths'

echo
if [ "$failures" -ne 0 ]; then
    echo "===== $checks checks, $failures FAILED"
    exit 1
fi
echo "===== $checks checks, all passed"
