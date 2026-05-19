#!/usr/bin/env bash
# bump-version.sh — single source of truth for DuneCity version bumps.
# Updates CMakeLists.txt, include/config.h, and vcpkg.json consistently.
#
# Usage:
#   scripts/bump-version.sh 1.0.8          # apply version
#   scripts/bump-version.sh --check        # print current versions and flag mismatches
#   scripts/bump-version.sh 1.0.8 --dry-run  # show what would change without writing

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

CMAKE="$REPO_ROOT/CMakeLists.txt"
CONFIG_H="$REPO_ROOT/include/config.h"
VCPKG="$REPO_ROOT/vcpkg.json"

# ── helpers ──────────────────────────────────────────────────────────

read_cmake_version() {
  sed -n 's/^project(DuneCity VERSION \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/p' "$CMAKE"
}

read_config_h_version() {
  sed -n 's/.*#define VERSION "\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)".*/\1/p' "$CONFIG_H"
}

read_vcpkg_version() {
  python3 -c "import json; print(json.load(open('$VCPKG'))['version'])"
}

# ── --check mode ─────────────────────────────────────────────────────

if [[ "${1:-}" == "--check" ]]; then
  v_cmake=$(read_cmake_version)
  v_config=$(read_config_h_version)
  v_vcpkg=$(read_vcpkg_version)

  echo "CMakeLists.txt:   $v_cmake"
  echo "include/config.h: $v_config"
  echo "vcpkg.json:       $v_vcpkg"

  if [[ "$v_cmake" == "$v_config" && "$v_config" == "$v_vcpkg" ]]; then
    echo "OK — all files agree on $v_cmake"
    exit 0
  else
    echo "MISMATCH — versions differ across files" >&2
    exit 1
  fi
fi

# ── parse args ───────────────────────────────────────────────────────

VERSION="${1:-}"
DRY_RUN=0

if [[ -z "$VERSION" ]]; then
  echo "Usage: $0 <X.Y.Z> [--dry-run]" >&2
  echo "       $0 --check" >&2
  exit 1
fi

if [[ "${2:-}" == "--dry-run" ]]; then
  DRY_RUN=1
fi

# validate semver format
if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
  echo "Error: version must be X.Y.Z (got '$VERSION')" >&2
  exit 1
fi

# ── apply ────────────────────────────────────────────────────────────

bump_file() {
  local file="$1" pattern="$2" replacement="$3"
  if (( DRY_RUN )); then
    echo "[dry-run] $file:"
    diff <(cat "$file") <(sed "$pattern" "$file") || true
  else
    # macOS sed needs '' after -i; Linux sed does not.
    if sed --version 2>/dev/null | grep -q GNU; then
      sed -i "$pattern" "$file"
    else
      sed -i '' "$pattern" "$file"
    fi
  fi
}

bump_file "$CMAKE" \
  "s/project(DuneCity VERSION [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*/project(DuneCity VERSION ${VERSION}/" \
  ""

bump_file "$CONFIG_H" \
  "s/#define VERSION \"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\"/#define VERSION \"${VERSION}\"/" \
  ""

# vcpkg.json — use python for proper JSON handling
if (( DRY_RUN )); then
  echo "[dry-run] vcpkg.json:"
  python3 -c "
import json, sys
d = json.load(open('$VCPKG'))
old = d['version']
d['version'] = '$VERSION'
if old != '$VERSION':
    print(f'  version: {old} -> $VERSION')
else:
    print('  (no change)')
"
else
  python3 -c "
import json
d = json.load(open('$VCPKG'))
d['version'] = '$VERSION'
open('$VCPKG', 'w').write(json.dumps(d, indent=2) + '\n')
"
fi

if (( DRY_RUN )); then
  echo ""
  echo "Dry run complete — no files were modified."
else
  echo "Version bumped to $VERSION in:"
  echo "  $CMAKE"
  echo "  $CONFIG_H"
  echo "  $VCPKG"
fi
