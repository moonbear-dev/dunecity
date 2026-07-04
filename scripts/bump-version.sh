#!/usr/bin/env bash
# bump-version.sh — single source of truth for DuneCity version bumps.
# Updates CMakeLists.txt, include/config.h, and vcpkg.json consistently.
#
# Usage:
#   scripts/bump-version.sh 1.0.8          # apply 3-segment version
#   scripts/bump-version.sh 1.0.263.4      # apply 4-segment hotfix version
#   scripts/bump-version.sh --check        # print current versions and flag mismatches
#   scripts/bump-version.sh 1.0.8 --dry-run  # show what would change without writing
#
# Version format:
#   X.Y.Z     - release tag. Matches git tag vX.Y.Z.
#   X.Y.Z.W   - intermediate / hotfix bump during a release batch (e.g.
#               1.0.263.1/.2/.3 before the final 1.0.264 lands). The W
#               component is source-controlled metadata only; the git tag
#               and produced artifact always use X.Y.Z. See commits e28bca5
#               and ef8e5eb for prior 4-segment uses.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

CMAKE="$REPO_ROOT/CMakeLists.txt"
CONFIG_H="$REPO_ROOT/include/config.h"
VCPKG="$REPO_ROOT/vcpkg.json"

pypath() { command -v cygpath >/dev/null 2>&1 && cygpath -w "$1" || echo "$1"; }

read_cmake_version() {
  sed -n 's/^project(DuneCity VERSION \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\(\.[0-9][0-9]*\)*\).*/\1/p' "$CMAKE"
}

read_config_h_version() {
  sed -n 's/.*#define VERSION "\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\(\.[0-9][0-9]*\)*\)".*/\1/p' "$CONFIG_H"
}

read_vcpkg_version() {
  python3 -c "import json; print(json.load(open(r'$(pypath "$VCPKG")'))['version'])"
}

if [[ "${1:-}" == "--check" ]]; then
  v_cmake=$(read_cmake_version)
  v_config=$(read_config_h_version)
  v_vcpkg=$(read_vcpkg_version)

  echo "CMakeLists.txt:   $v_cmake"
  echo "include/config.h: $v_config"
  echo "vcpkg.json:       $v_vcpkg"

  if [[ "$v_cmake" == "$v_config" && "$v_config" == "$v_vcpkg" ]]; then
    echo "OK - all files agree on $v_cmake"
    exit 0
  else
    echo "MISMATCH - versions differ across files" >&2
    exit 1
  fi
fi

VERSION="${1:-}"
DRY_RUN=0

if [[ -z "$VERSION" ]]; then
  echo "Usage: $0 <X.Y.Z[.W]> [--dry-run]" >&2
  echo "       $0 --check" >&2
  exit 1
fi

if [[ "${2:-}" == "--dry-run" ]]; then
  DRY_RUN=1
fi

# Accept either X.Y.Z (release) or X.Y.Z.W (hotfix). 4-segment was the
# de-facto convention for hotfix batches in 1.0.263.x and previously
# required manual sed bypass (commit e28bca5 had to edit 3 files by
# hand because this script rejected the value).
if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$'; then
  echo "Error: version must be X.Y.Z or X.Y.Z.W (got '$VERSION')" >&2
  exit 1
fi

bump_file() {
  local file="$1" pattern="$2"
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

# Substitutes "X.Y.Z" (or existing "X.Y.Z.W") with the new value. Anchors
# matter: the old regex tried to match exactly "X.Y.Z" and left the ".W"
# dangling when the file already had a 4-segment value, so 4->3 segment
# fallbacks also need to match.
bump_file "$CMAKE" \
  "s/project(DuneCity VERSION [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\(\.[0-9][0-9]*\)*/project(DuneCity VERSION ${VERSION}/"

bump_file "$CONFIG_H" \
  "s/#define VERSION \"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\(\.[0-9][0-9]*\)*\"/#define VERSION \"${VERSION}\"/"

VCPKG_PY="$(pypath "$VCPKG")"
if (( DRY_RUN )); then
  echo "[dry-run] vcpkg.json:"
  python3 -c "
import json
d = json.load(open(r'$VCPKG_PY'))
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
d = json.load(open(r'$VCPKG_PY'))
d['version'] = '$VERSION'
open(r'$VCPKG_PY', 'w').write(json.dumps(d, indent=2) + '\n')
"
fi

if (( DRY_RUN )); then
  echo ""
  echo "Dry run complete - no files were modified."
else
  echo "Version bumped to $VERSION in:"
  echo "  $CMAKE"
  echo "  $CONFIG_H"
  echo "  $VCPKG"
  # Friendly hint when user bumps to a 4-segment value: tags are X.Y.Z.
  # Match exactly 4 dot-separated segments, not any version containing
  # a dot (the previous bug printed this hint for 3-segment bumps too).
  if [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    TAG="${VERSION%.*}"
    echo ""
    echo "Source-controlled version is 4-segment. Git tags stay 3-segment,"
    echo "so tag the release as 'v${TAG}' (not 'v${VERSION}')."
  fi
fi
