#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CONFIG="${CONFIG:-Release}"

./scripts/setup-linux.sh

CMAKE_EXTRA_ARGS=()

if [ -d "$ROOT/deps/sysroot/usr" ]; then
    export PKG_CONFIG_PATH="$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$ROOT/deps/sysroot/usr/share/pkgconfig:${PKG_CONFIG_PATH:-}"
    export PKG_CONFIG_SYSROOT_DIR="$ROOT/deps/sysroot"
    CMAKE_EXTRA_ARGS+=(
        "-DCMAKE_INCLUDE_PATH=$ROOT/deps/sysroot/usr/include"
        "-DCMAKE_LIBRARY_PATH=$ROOT/deps/sysroot/usr/lib/x86_64-linux-gnu"
    )
fi

cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DSYNTHSET_BUILD_TESTS=ON \
    -DSYNTHSET_BUILD_SCREENSHOT=ON \
    -DSYNTHSET_BUILD_HOST_TEST=ON \
    "${CMAKE_EXTRA_ARGS[@]}"

if [ -z "${JOBS:-}" ]; then
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel "$JOBS"

VST3="$BUILD_DIR/SynthSet_artefacts/$CONFIG/VST3/SynthSet.vst3"
STANDALONE="$BUILD_DIR/SynthSet_artefacts/$CONFIG/Standalone/SynthSet"

mkdir -p "$ROOT/dist"

if [ -d "$VST3" ]; then
    rm -rf "$ROOT/dist/SynthSet.vst3"
    cp -a "$VST3" "$ROOT/dist/"
fi

if [ -x "$STANDALONE" ]; then
    cp -a "$STANDALONE" "$ROOT/dist/SynthSet"
fi

echo
echo "Built artefacts:"
[ -d "$ROOT/dist/SynthSet.vst3" ] && echo "  VST3        $ROOT/dist/SynthSet.vst3"
[ -x "$ROOT/dist/SynthSet" ] && echo "  Standalone  $ROOT/dist/SynthSet"
echo "  Tests       $BUILD_DIR/SynthSetTests_artefacts/$CONFIG/SynthSetTests"
echo "  Host test   $BUILD_DIR/SynthSetHostSmokeTest_artefacts/$CONFIG/SynthSetHostSmokeTest"
echo "  Screenshot  $BUILD_DIR/SynthSetScreenshot_artefacts/$CONFIG/SynthSetScreenshot"
