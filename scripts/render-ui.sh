#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

OUT="${1:-dist/synthset-ui.png}"
PRESET="${SYNTHSET_PRESET:-classic.pad.106-dream}"
SCALE="${SYNTHSET_SCALE:-2}"
BIN="$ROOT/build/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot"

if [ ! -x "$BIN" ]; then
    echo "Build the screenshot target first: ./scripts/build.sh" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUT")"

if command -v xvfb-run >/dev/null 2>&1 && [ -z "${DISPLAY:-}" ]; then
    xvfb-run -a "$BIN" --preset "$PRESET" --out "$OUT" --scale "$SCALE"
else
    "$BIN" --preset "$PRESET" --out "$OUT" --scale "$SCALE"
fi
