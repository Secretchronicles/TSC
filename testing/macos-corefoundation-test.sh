#!/bin/bash
#
# macos-corefoundation-test.sh - macOS links CoreFoundation, which main() uses.
#
# WHY THIS EXISTS. Both macOS jobs of the v2.2.0-beta2 release run compiled
# every file and then stopped at the last link:
#
#   Undefined symbols for architecture arm64:
#     "_CFBundleCopyBundleURL", referenced from: _main in main.cpp.o
#     "_CFBundleGetMainBundle", referenced from: _main in main.cpp.o
#   ld: symbol(s) not found for architecture arm64
#
# main() finds the game data inside the .app bundle with CoreFoundation's
# CFBundle functions, and CMakeLists.txt never named the framework: it used to
# come along with another library. Checked here from the source, and - on a
# Mac with cmake - by linking a program that calls the same functions through
# the same find_library.
#
# Run: testing/macos-corefoundation-test.sh

set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cmakelists="$here/../tsc/CMakeLists.txt"
main="$here/../tsc/src/core/main.cpp"

passed=0
failed=0
ok()   { echo "  ok   - $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL - $1"; failed=$((failed + 1)); }

echo "===== macOS CoreFoundation: linked where main() uses it"

for f in "$cmakelists" "$main"; do
    [ -f "$f" ] || { echo "  FAIL - missing $f" >&2; exit 1; }
done

if grep -q 'CFBundleGetMainBundle' "$main"; then
    ok "main() still uses CFBundle on __APPLE__, so the framework is needed"
else
    echo "  --   main() no longer uses CFBundle; the link below is harmless"
fi

if grep -q 'find_library(TSC_COREFOUNDATION CoreFoundation REQUIRED)' "$cmakelists" \
   && grep -q 'target_link_libraries(tsc ${TSC_COREFOUNDATION})' "$cmakelists"; then
    ok "CMakeLists finds CoreFoundation and links it"
else
    fail "CoreFoundation is not linked; the macOS link ends in _CFBundleCopyBundleURL"
fi

# NEGATIVE: the link must be inside if (APPLE). A CoreFoundation link on Linux
# or Windows is a REQUIRED find that cannot succeed there.
if awk '/find_library\(TSC_COREFOUNDATION/ { print prev } { prev = $0 }' "$cmakelists" \
   | grep -q 'if (APPLE)'; then
    ok "negative: only macOS asks for CoreFoundation"
else
    fail "the CoreFoundation find is not guarded by if (APPLE)"
fi

if [ "$(uname -s)" = "Darwin" ] && command -v cmake >/dev/null 2>&1; then
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    cat > "$tmp/main.cpp" <<'CPP'
#include <CoreFoundation/CoreFoundation.h>
int main() { CFBundleRef b = CFBundleGetMainBundle(); CFURLRef u = CFBundleCopyBundleURL(b); if (u) CFRelease(u); return 0; }
CPP
    cat > "$tmp/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.5)
project(t CXX)
add_executable(tsc main.cpp)
set_target_properties(tsc PROPERTIES LINK_FLAGS "-Wl,-dead_strip_dylibs")
find_library(TSC_COREFOUNDATION CoreFoundation REQUIRED)
target_link_libraries(tsc ${TSC_COREFOUNDATION})
CMAKE
    if cmake -S "$tmp" -B "$tmp/b" >/dev/null 2>&1 && cmake --build "$tmp/b" >/dev/null 2>&1; then
        ok "live: a program calling CFBundle links with that find_library"
    else
        fail "live: the CoreFoundation link does not work here"
    fi
else
    echo "  --   not macOS (or no cmake); skipping the live link"
fi

echo "===== macOS CoreFoundation: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
