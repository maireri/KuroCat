#!/usr/bin/env bash
# Linux skin regression (never launches the app):
#  1. the SkinSpec renderer draws the 4 built-in coats byte-identically to the pre-SkinSpec renderer of
#     commit e30e0c1 (v1.3.5): 19 poses each, plus the petting hit maps;
#  2. the strict .kurocat parser checks pass (tests/skin-format/parse_test.cpp).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BASE_COMMIT="${KUROCAT_BASELINE:-e30e0c1}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/base/linux/src" "$WORK/base/shared" "$WORK/before" "$WORK/after"
for f in CatRenderer.hpp CatPhysics.hpp KeyMap.hpp InputEngine.hpp; do
    git -C "$ROOT" show "$BASE_COMMIT:linux/src/$f" > "$WORK/base/linux/src/$f"
done
git -C "$ROOT" show "$BASE_COMMIT:shared/CoatPatterns.hpp" > "$WORK/base/shared/CoatPatterns.hpp"

# Same optimisation level as linux/build.sh so floating-point contraction matches the shipped binary.
read -r -a GTK <<< "$(pkg-config --cflags --libs gtk+-3.0 cairo)"
g++ -std=c++17 -O2 -I "$WORK/base/linux/src" "$ROOT/tests/linux-render/render_poses.cpp" "${GTK[@]}" -o "$WORK/render_before"
g++ -std=c++17 -O2 -DKUROCAT_SKINSPEC -I "$ROOT/linux/src" "$ROOT/tests/linux-render/render_poses.cpp" "${GTK[@]}" -o "$WORK/render_after"
"$WORK/render_before" "$WORK/before" > /dev/null
"$WORK/render_after" "$WORK/after" > /dev/null

n=0
for f in "$WORK/before"/*; do
    name="$(basename "$f")"
    if ! cmp -s "$f" "$WORK/after/$name"; then
        echo "FAIL: $name differs from $BASE_COMMIT"
        exit 1
    fi
    n=$((n + 1))
done
echo "PASS: $n built-in frames + petting hit maps are byte-identical to $BASE_COMMIT"

g++ -std=c++17 "$ROOT/tests/skin-format/parse_test.cpp" -o "$WORK/parse_test"
(cd "$ROOT" && "$WORK/parse_test")
