#!/bin/bash
#
# windows-libxmlpp-test.sh - win64 gets libxml++ 2.6 although ucrt64 delisted it.
#
# WHY THIS EXISTS. The win64 job of the v2.2.0-beta2 release run stopped while
# setting up MSYS2:
#
#   error: target not found: mingw-w64-ucrt-x86_64-libxml++2.6
#
# MSYS2 dropped the package from ucrt64.db; the file is still served from
# repo.msys2.org, and it imports libxml2-16.dll and libglibmm-2.4-1.dll, which
# the listed libxml2 and glibmm still provide. So the workflow installs those
# from the db and that one package by URL with `pacman -U`.
#
# Run: testing/windows-libxmlpp-test.sh

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
yml="$here/../.github/workflows/Windows.yml"

passed=0
failed=0
ok()   { echo "  ok   - $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL - $1"; failed=$((failed + 1)); }

echo "===== win64 libxml++ 2.6: installed by URL, not from the db"

[ -f "$yml" ] || { echo "  FAIL - missing $yml" >&2; exit 1; }

# The win64 matrix entry, from "- name: win64" to the next "- name:".
win64="$(awk '/^          - name: win64/ { on = 1; next } on && /^          - name:/ { exit } on' "$yml")"

# NEGATIVE: asking pacman -S for it is the line that failed.
if echo "$win64" | grep -q 'mingw-w64-ucrt-x86_64-libxml++2.6'; then
    fail "win64 still asks setup-msys2 for libxml++2.6 (target not found)"
else
    ok "negative: setup-msys2 is not asked for the delisted package"
fi

for dep in libxml2 glibmm; do
    if echo "$win64" | grep -q "mingw-w64-ucrt-x86_64-$dep\$"; then
        ok "its dependency $dep comes from the db"
    else
        fail "win64 does not install $dep, which libxml++ 2.6 links"
    fi
done

if grep -qE "pacman -U .*--noconfirm" "$yml" \
   && grep -q "https://repo.msys2.org/mingw/ucrt64/mingw-w64-ucrt-x86_64-libxml++2.6-.*-any.pkg.tar.zst" "$yml"; then
    ok "pacman -U installs it from repo.msys2.org"
else
    fail "nothing installs libxml++ 2.6 on win64"
fi

if grep -B8 'pacman -U' "$yml" | grep -q "matrix.msystem == 'UCRT64'"; then
    ok "and only on win64 (win32 has its own mirror)"
else
    fail "the pacman -U step is not limited to UCRT64"
fi

echo "===== win64 libxml++ 2.6: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
