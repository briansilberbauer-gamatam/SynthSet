#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Reuse the cached JUCE checkout and Linux development sysroot from the
# neighbouring plugin_poc workspace when available. Both directories are
# gitignored and never shipped in source or release archives.
POC="${SYNTHSET_SIBLING_POC:-../plugin_poc}"

if [ ! -f external/JUCE/CMakeLists.txt ] && [ -d "${POC}/external/JUCE" ]; then
    mkdir -p external
    ln -sfn "$(cd "${POC}/external/JUCE" && pwd)" external/JUCE
fi

if [ ! -d deps/sysroot/usr ] && [ -d "${POC}/deps/sysroot" ]; then
    mkdir -p deps
    ln -sfn "$(cd "${POC}/deps/sysroot" && pwd)" deps/sysroot
fi

if [ ! -f external/JUCE/CMakeLists.txt ]; then
    echo "JUCE was not found at external/JUCE." >&2
    echo "Place JUCE 8.0.8 there, or let CMake's FetchContent fallback retrieve it." >&2
fi

printf 'JUCE:    %s\n' "$ROOT/external/JUCE"
printf 'Sysroot: %s\n' "$ROOT/deps/sysroot"
