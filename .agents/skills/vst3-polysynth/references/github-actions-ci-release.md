# SynthSet — GitHub Actions CI & Versioned Release Reference

This reference provides the complete cross-platform pipeline for SynthSet:
Windows x64, Linux x64, and macOS Universal VST3 + Standalone builds, tests,
versioned ZIP distributions, SHA-256 checksums, and GitHub Releases.

Read this before writing `.github/workflows/build.yml`, `scripts/build.sh`, or
release documentation.

---

## 1. Goals and Non-Negotiables

- Build on every push to `main` and every pull request.
- Run the DSP/preset tests and the VST3 host smoke test on every platform.
- Package **versioned** artifacts:
  - `SynthSet-Windows-x64-v<version>.zip`
  - `SynthSet-Linux-x64-v<version>.zip`
  - `SynthSet-macOS-Universal-v<version>.zip`
  - each with a `.sha256` checksum.
- Publish a GitHub Release when a `v*` tag is pushed, or when a manual dispatch
  supplies a non-empty `release_tag`.
- The version comes from `CMakeLists.txt`:
  `project(SynthSet VERSION x.y.z LANGUAGES C CXX)`. Do not hard-code a second
  version in the workflow or packaging script.
- Keep top-level `permissions: contents: read`; only the release job gets
  `contents: write`.
- Builds must work from a clean checkout. Do not commit `external/JUCE`, the
  Linux sysroot, `build*/`, `dist/`, or `stage*/`.

---

## 2. CMake Prerequisites

### 2.1 Version

```cmake
cmake_minimum_required(VERSION 3.22)
project(SynthSet VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_RPATH TRUE)

option(SYNTHSET_BUILD_TESTS "Build offline DSP/preset tests" ON)
option(SYNTHSET_BUILD_SCREENSHOT "Build headless UI renderer" ON)
option(SYNTHSET_BUILD_HOST_TEST "Build VST3 host smoke test" ON)
```

`CMAKE_SKIP_RPATH TRUE` is mandatory: local offline builds must not bake the
sibling sysroot's absolute paths into distributed binaries.

### 2.2 Hybrid JUCE resolution

Use a local checkout when present; fall back to FetchContent in CI.

```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
```

Do **not** define `JUCE_DISPLAY_SPLASH_SCREEN=0`; it was removed in JUCE 8.0.8
and triggers a compile error.

### 2.3 Plugin target shape

```cmake
set(SYNTHSET_DEFINITIONS
    SYNTHSET_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_add_binary_data(SynthSetAssets
    SOURCES
        Resources/LatoRegular.ttf
        Resources/LatoSemibold.ttf
        Resources/LatoBlack.ttf
        Resources/Lato-OFL.txt)

juce_add_plugin(SynthSet
    COMPANY_NAME "BLS Audio"
    BUNDLE_ID "com.blsaudio.synthset"
    PLUGIN_MANUFACTURER_CODE Blsa
    PLUGIN_CODE Sset
    FORMATS VST3 Standalone
    PRODUCT_NAME "SynthSet"
    DESCRIPTION "Preset-only 16-voice polyphonic synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument|Synth")

target_sources(SynthSet PRIVATE ${SYNTHSET_SOURCES})
target_include_directories(SynthSet PRIVATE Source)
target_compile_definitions(SynthSet PUBLIC ${SYNTHSET_DEFINITIONS})
target_link_libraries(SynthSet
    PRIVATE
        SynthSetAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
```

Test targets:

```cmake
juce_add_console_app(SynthSetTests PRODUCT_NAME "SynthSetTests")
juce_add_console_app(SynthSetHostSmokeTest PRODUCT_NAME "SynthSetHostSmokeTest")
juce_add_gui_app(SynthSetScreenshot PRODUCT_NAME "SynthSetScreenshot")
```

For the host test:

```cmake
target_compile_definitions(SynthSetHostSmokeTest PRIVATE
    ${SYNTHSET_DEFINITIONS}
    JUCE_PLUGINHOST_VST3=1)
add_dependencies(SynthSetHostSmokeTest SynthSet_VST3)
```

### 2.4 `.gitignore`

```gitignore
build/
build-*/
dist/
external/
deps/
stage/
stage-*/
*.zip
*.sha256
.DS_Store
```

Keep `docs/*.png` out of the ignore list only if those previews are intentional
documentation.

---

## 3. Platform Nuances

### Windows x64 (`windows-2022`)

- Pin `windows-2022`; newer `windows-latest` images may not include the VS 2022
  toolchain expected by JUCE 8.0.8.
- Configure with `-A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` so the VST3
  and Standalone `.exe` do not require a separately installed VC++ runtime.
- Build Release; run `SynthSetTests.exe` and `SynthSetHostSmokeTest.exe`.
- Copy the `.vst3` bundle and Standalone `.exe` into a staging folder, add
  `README.md`, `CHANGELOG.md`, and the OFL license, then `Compress-Archive`.
- Compute SHA-256 with `Get-FileHash`.

### Linux x64 (`ubuntu-latest`)

- Install `build-essential`, `cmake`, `ninja-build`, `pkg-config`, `zip`, ALSA,
  X11, FreeType, and Fontconfig development packages.
- Use `-G Ninja -DCMAKE_BUILD_TYPE=Release`.
- Run tests and the host smoke test.
- Run the screenshot target under `xvfb-run -a` and upload the PNG as the
  `SynthSet-UI-Preview` artifact.
- `strip --strip-unneeded` the `.so` and Standalone binary before zipping, but
  never strip the test binaries before running them.

### macOS Universal (`macos-15-intel`)

- Use a supported Intel runner for a Universal build. `macos-14` may not be
  available for this repository; `macos-15-intel` is the stable choice.
- Configure with:
  ```bash
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
  ```
- Ad-hoc sign both the `.vst3` and the Standalone `.app` **before** running the
  host smoke test:
  ```bash
  codesign --force --deep --sign - <bundle>
  ```
  Apple Silicon refuses to load unsigned arm64 bundles; ad-hoc signing is enough
  for CI and local testing, not for notarized distribution.
- Package with `ditto -c -k --sequesterRsrc` to preserve bundle metadata and
  symlinks.
- Build Universal, but note that the Intel runner can only execute the x86_64
  slice. The arm64 slice is compile-verified; add a separate arm64 runner later
  if runtime coverage on both slices is required.

---

## 4. Complete `.github/workflows/build.yml`

```yaml
name: Build SynthSet

on:
  push:
    branches: [ main ]
    tags: [ "v*" ]
  pull_request:
    branches: [ main ]
  workflow_dispatch:
    inputs:
      release_tag:
        description: "Optional release tag (e.g. v1.0.0). Leave empty for CI artifacts only."
        required: false
        type: string

permissions:
  contents: read

concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: ${{ github.event_name == 'pull_request' }}

jobs:
  windows-x64:
    name: Windows x64
    runs-on: windows-2022
    timeout-minutes: 40

    steps:
      - name: Check out source
        uses: actions/checkout@v6

      - name: Configure
        shell: pwsh
        run: |
          cmake -S . -B build-windows -A x64 `
            -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
            -DSYNTHSET_BUILD_SCREENSHOT=OFF `
            -DSYNTHSET_BUILD_HOST_TEST=ON
          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

      - name: Build Release
        shell: pwsh
        run: |
          cmake --build build-windows --config Release --parallel
          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

      - name: Run DSP and preset tests
        shell: pwsh
        run: |
          & "build-windows\SynthSetTests_artefacts\Release\SynthSetTests.exe"
          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

      - name: Run VST3 host smoke test
        shell: pwsh
        run: |
          & "build-windows\SynthSetHostSmokeTest_artefacts\Release\SynthSetHostSmokeTest.exe" `
            "build-windows\SynthSet_artefacts\Release\VST3\SynthSet.vst3"
          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

      - name: Package Windows distribution
        shell: pwsh
        run: |
          $ErrorActionPreference = "Stop"
          $stage = "stage-windows"
          Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
          New-Item -ItemType Directory -Force $stage | Out-Null

          Copy-Item -Recurse -Force "build-windows\SynthSet_artefacts\Release\VST3\*.vst3" $stage
          Copy-Item -Force "build-windows\SynthSet_artefacts\Release\Standalone\*.exe" $stage
          if (Test-Path "README.md") { Copy-Item -Force "README.md" $stage }
          if (Test-Path "CHANGELOG.md") { Copy-Item -Force "CHANGELOG.md" $stage }
          if (Test-Path "Resources\Lato-OFL.txt") { Copy-Item -Force "Resources\Lato-OFL.txt" $stage }

          $match = Select-String -Path "CMakeLists.txt" `
            -Pattern 'project\(\s*SynthSet\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)'
          if (-not $match -or $match.Matches.Count -eq 0) {
            throw "Could not read the SynthSet version from CMakeLists.txt"
          }
          $version = $match.Matches[0].Groups[1].Value

          $zip = "SynthSet-Windows-x64-v$version.zip"
          Compress-Archive -Path "$stage\*" -DestinationPath $zip -Force

          $hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
          "$hash  $zip" | Out-File -Encoding ascii `
            "SynthSet-Windows-x64-v$version.sha256"
          Get-Item $zip | Format-List Name, Length

      - name: Upload Windows distribution
        uses: actions/upload-artifact@v7
        with:
          name: SynthSet-Windows-x64
          path: |
            SynthSet-Windows-x64-v*.zip
            SynthSet-Windows-x64-v*.sha256
          if-no-files-found: error

  linux-x64:
    name: Linux x64
    runs-on: ubuntu-latest
    timeout-minutes: 40

    steps:
      - name: Check out source
        uses: actions/checkout@v6

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends \
            build-essential cmake ninja-build pkg-config zip xvfb \
            libasound2-dev \
            libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
            libxcomposite-dev libxrender-dev libxfixes-dev libxi-dev \
            libsm-dev libice-dev libx11-xcb-dev libxcb1-dev \
            libexpat1-dev zlib1g-dev libpng-dev libbrotli-dev libbz2-dev

          if apt-cache show libfreetype-dev >/dev/null 2>&1; then
            FREETYPE_PACKAGE=libfreetype-dev
          else
            FREETYPE_PACKAGE=libfreetype6-dev
          fi

          if apt-cache show libfontconfig-dev >/dev/null 2>&1; then
            FONTCONFIG_PACKAGE=libfontconfig-dev
          else
            FONTCONFIG_PACKAGE=libfontconfig1-dev
          fi

          sudo apt-get install -y --no-install-recommends \
            "$FREETYPE_PACKAGE" "$FONTCONFIG_PACKAGE"

      - name: Configure
        run: |
          cmake -S . -B build-linux -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DSYNTHSET_BUILD_SCREENSHOT=ON \
            -DSYNTHSET_BUILD_HOST_TEST=ON

      - name: Build Release
        run: cmake --build build-linux --parallel

      - name: Run DSP and preset tests
        run: ./build-linux/SynthSetTests_artefacts/Release/SynthSetTests

      - name: Run VST3 host smoke test
        run: |
          ./build-linux/SynthSetHostSmokeTest_artefacts/Release/SynthSetHostSmokeTest \
            build-linux/SynthSet_artefacts/Release/VST3/SynthSet.vst3

      - name: Render UI preview
        run: |
          mkdir -p ui-preview
          xvfb-run -a \
            ./build-linux/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot \
            --preset classic.pad.106-dream \
            --out ui-preview/synthset-106-dream.png \
            --scale 2

      - name: Package Linux distribution
        run: |
          set -euo pipefail

          VERSION="$(sed -nE 's/^project\(SynthSet[[:space:]]+VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt)"
          if [ -z "$VERSION" ]; then
            echo "Could not read the SynthSet version from CMakeLists.txt" >&2
            exit 1
          fi

          STAGE="stage-linux"
          rm -rf "$STAGE"
          mkdir -p "$STAGE"

          cp -a build-linux/SynthSet_artefacts/Release/VST3/SynthSet.vst3 "$STAGE/"
          cp build-linux/SynthSet_artefacts/Release/Standalone/SynthSet "$STAGE/"
          [ -f README.md ] && cp README.md "$STAGE/"
          [ -f CHANGELOG.md ] && cp CHANGELOG.md "$STAGE/"
          [ -f Resources/Lato-OFL.txt ] && cp Resources/Lato-OFL.txt "$STAGE/"

          find "$STAGE" -type f \( -name "*.so" -o -perm -111 \) \
            -exec strip --strip-unneeded {} + 2>/dev/null || true

          ZIP="SynthSet-Linux-x64-v${VERSION}.zip"
          (cd "$STAGE" && zip -q -r "../${ZIP}" .)
          sha256sum "$ZIP" > "SynthSet-Linux-x64-v${VERSION}.sha256"

          ls -lh "$ZIP"
          cat "SynthSet-Linux-x64-v${VERSION}.sha256"

      - name: Upload Linux distribution
        uses: actions/upload-artifact@v7
        with:
          name: SynthSet-Linux-x64
          path: |
            SynthSet-Linux-x64-v*.zip
            SynthSet-Linux-x64-v*.sha256
          if-no-files-found: error

      - name: Upload UI preview
        uses: actions/upload-artifact@v7
        with:
          name: SynthSet-UI-Preview
          path: ui-preview/*.png
          if-no-files-found: error

  macos-universal:
    name: macOS Universal
    runs-on: macos-15-intel
    timeout-minutes: 45

    steps:
      - name: Check out source
        uses: actions/checkout@v6

      - name: Install Ninja
        run: brew install ninja

      - name: Configure Universal Binary
        run: |
          cmake -S . -B build-macos -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
            -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
            -DSYNTHSET_BUILD_SCREENSHOT=OFF \
            -DSYNTHSET_BUILD_HOST_TEST=ON

      - name: Build Release
        run: cmake --build build-macos --parallel

      - name: Ad-hoc sign bundles
        run: |
          for bundle in build-macos/SynthSet_artefacts/Release/VST3/SynthSet.vst3 \
                        build-macos/SynthSet_artefacts/Release/Standalone/SynthSet.app; do
            if [ -e "$bundle" ]; then
              codesign --force --deep --sign - "$bundle"
            fi
          done

      - name: Run DSP and preset tests
        run: ./build-macos/SynthSetTests_artefacts/Release/SynthSetTests

      - name: Run VST3 host smoke test
        run: |
          ./build-macos/SynthSetHostSmokeTest_artefacts/Release/SynthSetHostSmokeTest \
            build-macos/SynthSet_artefacts/Release/VST3/SynthSet.vst3

      - name: Package macOS distribution
        run: |
          set -euo pipefail

          VERSION="$(sed -nE 's/^project\(SynthSet[[:space:]]+VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt)"
          if [ -z "$VERSION" ]; then
            echo "Could not read the SynthSet version from CMakeLists.txt" >&2
            exit 1
          fi

          STAGE="stage-macos"
          rm -rf "$STAGE"
          mkdir -p "$STAGE"

          cp -R build-macos/SynthSet_artefacts/Release/VST3/SynthSet.vst3 "$STAGE/"
          cp -R build-macos/SynthSet_artefacts/Release/Standalone/SynthSet.app "$STAGE/"
          [ -f README.md ] && cp README.md "$STAGE/"
          [ -f CHANGELOG.md ] && cp CHANGELOG.md "$STAGE/"
          [ -f Resources/Lato-OFL.txt ] && cp Resources/Lato-OFL.txt "$STAGE/"

          ZIP="SynthSet-macOS-Universal-v${VERSION}.zip"
          ditto -c -k --sequesterRsrc "$STAGE" "$ZIP"
          shasum -a 256 "$ZIP" > "SynthSet-macOS-Universal-v${VERSION}.sha256"

          ls -lh "$ZIP"
          cat "SynthSet-macOS-Universal-v${VERSION}.sha256"

      - name: Upload macOS distribution
        uses: actions/upload-artifact@v7
        with:
          name: SynthSet-macOS-Universal
          path: |
            SynthSet-macOS-Universal-v*.zip
            SynthSet-macOS-Universal-v*.sha256
          if-no-files-found: error

  publish-release:
    name: Publish GitHub Release
    needs: [ windows-x64, linux-x64, macos-universal ]
    if: ${{ github.ref_type == 'tag' || (github.event_name == 'workflow_dispatch' && inputs.release_tag != '') }}
    runs-on: ubuntu-latest
    permissions:
      contents: write

    steps:
      - name: Check out source for version check
        uses: actions/checkout@v6

      - name: Verify tag matches CMake version
        shell: bash
        run: |
          set -euo pipefail
          VERSION="$(sed -nE 's/^project\(SynthSet[[:space:]]+VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt)"
          TAG="${{ inputs.release_tag || github.ref_name }}"
          case "$TAG" in
            "v${VERSION}"|"v${VERSION}"-*) ;;
            *)
              echo "Tag '$TAG' does not match CMake version 'v${VERSION}'" >&2
              exit 1
              ;;
          esac

      - name: Download release packages
        uses: actions/download-artifact@v8
        with:
          pattern: SynthSet-*
          path: release-assets
          merge-multiple: true

      - name: Publish release
        uses: softprops/action-gh-release@v3
        with:
          tag_name: ${{ inputs.release_tag || github.ref_name }}
          target_commitish: ${{ github.sha }}
          name: SynthSet ${{ inputs.release_tag || github.ref_name }}
          files: |
            release-assets/*.zip
            release-assets/*.sha256
          fail_on_unmatched_files: true
          overwrite_files: true
          generate_release_notes: true
          prerelease: ${{ contains(inputs.release_tag || github.ref_name, '-') }}
```

---

## 5. Local Build Scripts

### `scripts/setup-linux.sh`

Reuse the cached JUCE and sysroot from the sibling `plugin_poc` project. Do not
download hundreds of megabytes during normal development.

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

POC="../plugin_poc"
if [ ! -d external/JUCE ] && [ -d "$POC/external/JUCE" ]; then
  mkdir -p external
  ln -sfn "$(cd "$POC/external/JUCE" && pwd)" external/JUCE
fi
if [ ! -d deps/sysroot ] && [ -d "$POC/deps/sysroot" ]; then
  mkdir -p deps
  ln -sfn "$(cd "$POC/deps/sysroot" && pwd)" deps/sysroot
fi

echo "JUCE:  ${PWD}/external/JUCE"
echo "Sysroot: ${PWD}/deps/sysroot"
```

### `scripts/build.sh`

Use the same sysroot flags as the sibling projects. A representative shape:

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/setup-linux.sh

SYSROOT="$PWD/deps/sysroot"
export PKG_CONFIG_PATH="$SYSROOT/usr/lib/x86_64-linux-gnu/pkgconfig:$SYSROOT/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export CMAKE_PREFIX_PATH="$SYSROOT/usr"

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="--sysroot=$SYSROOT" \
  -DCMAKE_EXE_LINKER_FLAGS="--sysroot=$SYSROOT -Wl,-rpath-link,$SYSROOT/usr/lib/x86_64-linux-gnu" \
  -DCMAKE_SHARED_LINKER_FLAGS="--sysroot=$SYSROOT -Wl,-rpath-link,$SYSROOT/usr/lib/x86_64-linux-gnu" \
  -DSYNTHSET_BUILD_TESTS=ON \
  -DSYNTHSET_BUILD_SCREENSHOT=ON \
  -DSYNTHSET_BUILD_HOST_TEST=ON

cmake --build build --parallel

mkdir -p dist
rm -rf dist/SynthSet.vst3
cp -a build/SynthSet_artefacts/Release/VST3/SynthSet.vst3 dist/
cp build/SynthSet_artefacts/Release/Standalone/SynthSet dist/
```

If the sysroot setup differs, mirror the working flags from
`../plugin_poc/scripts/build.sh`; do not invent RPATHs that point outside the
workspace.

### `scripts/render-ui.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

OUT="${1:-dist/synthset-ui.png}"
BIN="build/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot"
if [ ! -x "$BIN" ]; then
  echo "Build the screenshot target first: ./scripts/build.sh" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"
xvfb-run -a "$BIN" \
  --preset "${SYNTHSET_PRESET:-classic.pad.106-dream}" \
  --out "$OUT" --scale 2
```

---

## 6. Release Procedure

1. Update `project(SynthSet VERSION x.y.z ...)` in `CMakeLists.txt`.
2. Update `CHANGELOG.md` with the same version and a short summary.
3. Verify locally:
   ```bash
   ./scripts/build.sh
   ./build/SynthSetTests_artefacts/Release/SynthSetTests
   ./build/SynthSetHostSmokeTest_artefacts/Release/SynthSetHostSmokeTest dist/SynthSet.vst3
   ./scripts/render-ui.sh dist/synthset-106-dream.png
   ```
4. Commit and push to `main`; wait for the three CI jobs to pass.
5. Tag and push:
   ```bash
   git tag v1.0.0
   git push origin main --tags
   ```
6. The `publish-release` job verifies the tag matches the CMake version, then
   publishes all three ZIPs and SHA-256 files.

Manual dispatch:

- GitHub → **Actions** → **Build SynthSet** → **Run workflow**.
- Enter `v1.0.0` (or `v1.0.0-rc1`) in `release_tag`; leave it blank for a CI-only
  run.

If the tag and CMake version do not match, the release job fails intentionally.
Do not bypass the check; bump the version and retag.

---

## 7. Distribution Contents and Install Notes

Each ZIP should contain:

- `SynthSet.vst3` (Windows/Linux bundle directory; macOS bundle)
- Standalone executable/app (where applicable)
- `README.md`
- `CHANGELOG.md`
- `Lato-OFL.txt`
- A short `INSTALL.txt` if the README does not cover install paths.

Recommended install locations (document in README):

| Platform | VST3 location |
| :--- | :--- |
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` (or `/Library/Audio/Plug-Ins/VST3/`) |
| Linux | `~/.vst3/` |

Standalone apps can run directly from the extracted folder; macOS users may
need to right-click → Open the first time because the build is ad-hoc signed,
not notarized. Document that honestly.

---

## 8. CI Acceptance Checklist

- [ ] Workflow YAML parses (no tabs, valid expressions).
- [ ] All three jobs configure from a clean checkout using FetchContent.
- [ ] DSP/preset tests pass on Windows, Linux, and macOS.
- [ ] Host smoke test loads the built VST3 and sees 48 programs / 0 parameters.
- [ ] Linux screenshot job produces `ui-preview/synthset-106-dream.png`.
- [ ] Every ZIP name contains `v<version>` and a matching `.sha256`.
- [ ] Release job downloads exactly three ZIPs + three checksums.
- [ ] Tag/version mismatch fails the release.
- [ ] `contents: write` exists only on the release job.
- [ ] No `external/`, `deps/`, build trees, or ZIPs are committed.
- [ ] No code signing secrets are required for the default ad-hoc build.
