#!/usr/bin/env bash
# scripts/build.sh — cross-platform local build helper for DuneCity.
#
# Builds the canonical target declared in CLAUDE.md (`dunelegacy`) using
# the project's canonical `build/` directory. Replaces the prior Mac-only
# version that hardcoded `arm64-osx-dynamic` and a non-existent `dmg`
# target (commit db93c76 removed the macOS CI job from
# .github/workflows/build.yml because no Apple-Silicon runner was
# registered). This script works on Linux, macOS, and Windows (MSYS).
#
# Usage:
#   scripts/build.sh                # configure + build (Release, no tests)
#   scripts/build.sh clean          # wipe build/ first
#   scripts/build.sh tests          # build + run dunelegacy_tests
#   scripts/build.sh debug          # Debug build + run tests
#   scripts/build.sh clean tests    # wipe, build Debug, run tests
#
# Tests require Catch2 v3 (apt: libcatch2-dev / brew: catch2). Use
# `apt-get install -y libcatch2-dev` (or the `setup-python` vcpkg action
# in CI) before requesting `tests`.
#
# Exit codes:
#   0  success
#   1  configure / build / test failure
#   2  missing required tool (cmake)

set -euo pipefail

cd "$(dirname "$0")/.."
REPO_ROOT="$(pwd)"

ARGS=("$@")
DO_CLEAN=0
DO_TESTS=0
BUILD_TYPE="Release"

# Tests require Catch2 v3 (libcatch2-dev) which is not always installed
# on a fresh clone. Default OFF; require explicit `tests` (or `debug`)
# to enable. Otherwise a clean checkout on a stock Ubuntu box dies
# at cmake configure with: "Could not find a package configuration
# file provided by Catch2".
BUILD_TESTS_FLAG=("-DDUNECITY_BUILD_TESTS=OFF")

for a in "${ARGS[@]}"; do
  case "$a" in
    clean)   DO_CLEAN=1 ;;
    tests)   DO_TESTS=1 ; BUILD_TESTS_FLAG=("-DDUNECITY_BUILD_TESTS=ON") ;;
    debug)   BUILD_TYPE="Debug" ; DO_TESTS=1 ; BUILD_TESTS_FLAG=("-DDUNECITY_BUILD_TESTS=ON") ;;
    release) BUILD_TYPE="Release" ;;
    *)       echo "Unknown arg: $a" >&2 ; echo "Usage: $0 [clean] [tests] [debug|release]" >&2 ; exit 2 ;;
  esac
done

BUILD_DIR="$REPO_ROOT/build"

command -v cmake >/dev/null 2>&1 || { echo "cmake not found in PATH" >&2 ; exit 2 ; }
case "$(uname -s 2>/dev/null || echo Windows)" in
  Linux|FreeBSD|OpenBSD|Darwin)
    GENERATOR="Unix Makefiles"
    if command -v ninja >/dev/null 2>&1; then
      GENERATOR="Ninja"
    fi
    ;;
  MINGW*|MSYS*|CYGWIN*|Windows*|Windows)
    GENERATOR="Visual Studio 17 2022"
    if command -v ninja >/dev/null 2>&1; then
      GENERATOR="Ninja"
    fi
    ;;
  *)
    GENERATOR="Unix Makefiles"
    ;;
esac

# Honor an explicit VCPKG_ROOT, otherwise fall back to the vendored
# checkout if one exists. Mirrors what the CI workflow does, minus the
# caching that CI gets for free.
VCPKG_ROOT="${VCPKG_ROOT:-}"
TOOLCHAIN_FLAG=()
if [[ -z "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
  if [[ -d "$REPO_ROOT/vcpkg" ]]; then
    VCPKG_ROOT="$REPO_ROOT/vcpkg"
  fi
fi
if [[ -n "$VCPKG_ROOT" && -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]]; then
  TOOLCHAIN_FLAG=("-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")
fi

if (( DO_CLEAN )); then
  echo "Removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "Configuring with $GENERATOR ($BUILD_TYPE)"
  if [[ "$GENERATOR" == "Ninja" ]]; then
    cmake -B "$BUILD_DIR" -G Ninja "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" \
      "${BUILD_TESTS_FLAG[@]}" "${TOOLCHAIN_FLAG[@]}"
  else
    cmake -B "$BUILD_DIR" -G "$GENERATOR" "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" \
      "${BUILD_TESTS_FLAG[@]}" "${TOOLCHAIN_FLAG[@]}"
  fi
fi

echo "Building dunelegacy ($BUILD_TYPE)"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel

if (( DO_TESTS )); then
  echo "Running dunelegacy_tests"
  TEST_BIN="$BUILD_DIR/bin/dunelegacy_tests"
  if [[ ! -x "$TEST_BIN" ]]; then
    echo "Test binary not built: $TEST_BIN" >&2
    echo "  (DUNECITY_BUILD_TESTS may be OFF; install libcatch2-dev and rebuild)" >&2
    exit 1
  fi
  DUNE_CITY_SOURCE_DIR="$REPO_ROOT" \
    DUNECITY_DATADIR="$REPO_ROOT/data" \
    "$TEST_BIN"
fi

echo ""
echo "Build complete"
if [[ -x "$BUILD_DIR/bin/dunecity" ]]; then
  echo "Binary: $BUILD_DIR/bin/dunecity"
  ls -lh "$BUILD_DIR/bin/dunecity" 2>/dev/null | awk '{print "  (" $5 ")"}'
fi
if (( DO_TESTS )); then
  echo "  (tests passed)"
fi
echo ""
echo "To run:      $BUILD_DIR/bin/dunecity"
echo "To package:  cmake --install $BUILD_DIR --prefix install --config $BUILD_TYPE && cpack -G ZIP"
