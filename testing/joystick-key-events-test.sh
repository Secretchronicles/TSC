#!/bin/bash
#
# joystick-key-events-test.sh - the key events the joystick code makes for the keyboard code are
# of the type the keyboard code reads (#734).
#
# THE FAULT. TSC turns joystick input into keyboard events (see #499) and hands them to
# cKeyboard::Key_Down() and cKeyboard::Key_Up(). Key_Up() reads its argument with
# evt.getIf<sf::Event::KeyReleased>() and asserts it got one. cJoystick::Handle_Motion() built a
# sf::Event::KeyPressed for BOTH directions - a press and a release - so the first time a
# direction was let go after pressing a gamepad button on the title screen,
#
#   tsc: src/input/keyboard.cpp:65: bool TSC::cKeyboard::Key_Up(const sf::Event&):
#   Assertion `keyReleased' failed.
#
# aborted the game (and in a build without assertions the pointer is null and is dereferenced).
# Handle_Button_Up_Event() always built the right type, which is why only axes and the
# directional pad were hit.
#
# THE CHECK. Wherever the code builds a key event itself, the event a Key_Up call is handed is a
# KeyReleased one and the event a Key_Down call is handed a KeyPressed one, in all of src/. A new caller that gets it wrong
# fails here, not on somebody's gamepad.
#
# Run: testing/joystick-key-events-test.sh

set -uo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$here/../tsc/src"

passed=0; failed=0
ok()   { echo "  ok   - $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL - $1"; failed=$((failed + 1)); }

echo "===== joystick key events"

# check FILE...  - prints one line per call whose event is of the wrong type
check() {
    python3 - "$@" <<'PY'
import re, sys
for path in sys.argv[1:]:
    lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    for i, line in enumerate(lines):
        m = re.search(r'pKeyboard->(Key_Up|Key_Down)\((\w+)\)', line)
        if not m:
            continue
        call, var = m.groups()
        want = 'KeyReleased' if call == 'Key_Up' else 'KeyPressed'
        # Only an event this code BUILDS itself can be of the wrong type; one it was handed and
        # tests with is<>() / getIf<>() (main.cpp, text_box.cpp, mouse.cpp) is the right one.
        decl = None
        for j in range(i - 1, max(i - 12, -1), -1):
            if re.search(r'sf::Event\s+%s\(' % var, lines[j]):
                decl = lines[j]
                break
        if decl is None:
            continue
        if ('sf::Event::%s{' % want) not in decl:
            print('%s:%d: %s is not preceded by a %s event' % (path, i + 1, call, want))
PY
}

[ -f "$src/input/joystick.cpp" ] && ok "joystick.cpp is there" || { fail "joystick.cpp is missing"; exit 1; }

bad="$(check $(find "$src" -name '*.cpp' -print0 | xargs -0 grep -l 'pKeyboard->Key_' ))"
if [ -z "$bad" ]; then ok "every key event built here has the type its call reads, in all of src/"; else echo "$bad"; fail "a key event has the wrong type"; fi

# the four directions of Handle_Motion are the ones that were wrong
n_rel="$(grep -c 'sf::Event::KeyReleased{pPreferences->m_key_\(right\|left\|up\|down\),' "$src/input/joystick.cpp")"
[ "$n_rel" -eq 4 ] && ok "right, left, up and down each release with a KeyReleased" || fail "expected 4 direction releases, found $n_rel"

# negative: the checker does catch the old shape, so the check above can fail
tmp="$(mktemp -d)"
cat > "$tmp/old.cpp" <<'CPP'
    if (m_right != wasRight) {
        sf::Event newevt(sf::Event::KeyPressed{pPreferences->m_key_right, sf::Keyboard::Scancode::Unknown, false, false, false, false});
        if (m_right) {
            pKeyboard->Key_Down(newevt);
        }
        else {
            pKeyboard->Key_Up(newevt);
        }
    }
CPP
caught="$(check "$tmp/old.cpp")"
rm -rf "$tmp"
if echo "$caught" | grep -q 'Key_Up is not preceded by a KeyReleased'; then ok "negative: the old shape (a KeyPressed handed to Key_Up) is caught"; else fail "the checker missed the old shape"; fi

# the assertion in Key_Up still documents the contract
grep -q 'assert(keyReleased)' "$src/input/keyboard.cpp" && ok "Key_Up still asserts it got a KeyReleased (the callers are fixed, the contract is not loosened)" || fail "the assertion in Key_Up is gone"

echo
if [ "$failed" -eq 0 ]; then echo "===== joystick key events: $passed passed, 0 failed"; else echo "===== joystick key events: $passed passed, $failed failed"; fi
exit "$failed"
