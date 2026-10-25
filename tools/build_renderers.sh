#!/bin/sh
# Build vv_render, the program tools/vv_pack.py runs to convert the game's
# files (PNG pictures, Ogg Vorbis songs, WAV sounds), for Linux, Windows and
# macOS, with "zig cc" as cross compiler:
#   pip install ziglang
#   tools/build_renderers.sh            -> tools/bin/vv_render-<os>-<arch>
# TARGETS="triple:name ..." selects the targets.
set -e
cd "$(dirname "$0")/.."
OUT=${OUT:-tools/bin}
mkdir -p "$OUT"
ZIG="${ZIG:-python3 -m ziglang}"
TARGETS="${TARGETS:-x86_64-linux-musl:linux-x86_64 aarch64-linux-musl:linux-arm64 x86_64-windows-gnu:windows-x86_64.exe x86_64-macos:macos-x86_64 aarch64-macos:macos-arm64}"
for target in $TARGETS; do
    triple=${target%%:*}
    name=vv_render-${target#*:}
    # no FMA contraction: the same output on every computer
    $ZIG cc -target "$triple" -O2 -ffp-contract=off -w -s tools/render/vv_render.c -lm -o "$OUT/$name" &
done
wait
rm -f "$OUT"/*.pdb "$OUT"/*.lib
ls -la "$OUT"
