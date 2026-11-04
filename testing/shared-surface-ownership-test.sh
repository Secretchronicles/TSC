#!/bin/bash
#
# shared-surface-ownership-test.sh - a sprite never takes ownership of a surface the image manager
# owns (#725).
#
# THE FAULT. cVideo::Get_Surface() looks the image up in the image manager and returns THAT surface to
# every caller; it is the manager's, and stays alive for the whole run. cSprite::Set_Image() has a
# third argument, `del_img`, that makes the sprite delete the surface when it dies. The level loader
# passed it for the "image not found" placeholder it puts on every sprite whose image is missing:
#
#   p_sprite->Set_Image(pVideo->Get_Surface("game/image_not_found.png"), true, true);
#
# So one placeholder surface was owned by every such sprite at once. Mountain Trials has many sprites
# with a missing image; leaving the level deleted the placeholder with the first of them, and the
# next ~cSprite deleted it again:
#
#   AddressSanitizer: heap-use-after-free ... in TSC::cGL_Surface::~cGL_Surface()
#   freed by ... TSC::cSprite::~cSprite() ... allocated by ... cVideo::Get_Surface()
#   ... cLevelLoader::Create_Sprites_From_XML_Tag() level_loader.cpp:493
#
# That is the crash on dying in Mountain Trials. The surfaces a sprite may delete are the ones it made
# itself, never one that came out of Get_Surface().
#
# Run: testing/shared-surface-ownership-test.sh

set -uo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$here/../tsc/src"

passed=0; failed=0
ok()   { echo "  ok   - $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL - $1"; failed=$((failed + 1)); }

echo "===== surface ownership"

# calls: every Set_Image( whose first argument is a Get_Surface(...) result, with its argument list
scan() {
    python3 - "$@" <<'PY'
import re, sys
for path in sys.argv[1:]:
    text = open(path, encoding='utf-8', errors='replace').read()
    for m in re.finditer(r'Set_Image\(\s*(?:\w+\s*->\s*)?(?:p\w+\s*->\s*)?(?:pVideo\s*->\s*)?Get_Surface\(', text):
        # the argument list of this Set_Image call: from the opening ( to its matching )
        i = text.index('Set_Image(', m.start()) + len('Set_Image(')
        depth, j = 1, i
        while j < len(text) and depth:
            depth += {'(': 1, ')': -1}.get(text[j], 0)
            j += 1
        args = text[i:j - 1]
        # split on top-level commas
        parts, d, cur = [], 0, ''
        for ch in args:
            if ch in '({[': d += 1
            if ch in ')}]': d -= 1
            if ch == ',' and d == 0:
                parts.append(cur.strip()); cur = ''
            else:
                cur += ch
        parts.append(cur.strip())
        if len(parts) >= 3 and parts[2] not in ('0', 'false'):
            line = text.count('\n', 0, m.start()) + 1
            print('%s:%d: Set_Image(Get_Surface(...), ..., %s) hands a managed surface to the sprite to delete' % (path, line, parts[2]))
PY
}

bad="$(scan $(grep -rl 'Set_Image(' "$src" --include='*.cpp' 2>/dev/null))"
if [ -z "$bad" ]; then ok "no sprite is told to delete a surface that came from Get_Surface()"; else echo "$bad"; fail "a sprite takes ownership of a managed surface"; fi

grep -q 'Set_Image(pVideo->Get_Surface(utf8_to_path("game/image_not_found.png")), true);' "$src/level/level_loader.cpp" \
    && ok "the placeholder for a missing image is set without ownership" || fail "the placeholder is not set without ownership"

# negative: the checker does catch the old line
tmp="$(mktemp -d)"
printf '    p_sprite->Set_Image(pVideo->Get_Surface(utf8_to_path("game/image_not_found.png")), true, true);\n' > "$tmp/old.cpp"
caught="$(scan "$tmp/old.cpp")"; rm -rf "$tmp"
echo "$caught" | grep -q 'hands a managed surface' && ok "negative: the old line (del_img true on a shared surface) is caught" || fail "the checker missed the old line"

# the ownership rule is still what Get_Surface() does
grep -q 'pImage_Manager->Get_Pointer' "$src/video/video.cpp" && grep -q 'pImage_Manager->Add(image)' "$src/video/video.cpp" \
    && ok "Get_Surface() still returns the image manager's surface, so the rule above still holds" || fail "Get_Surface() no longer caches in the image manager"

echo
if [ "$failed" -eq 0 ]; then echo "===== surface ownership: $passed passed, 0 failed"; else echo "===== surface ownership: $passed passed, $failed failed"; fi
exit "$failed"
