#!/usr/bin/env bash
# notify-build.sh — Monitor GitHub Actions and post macOS build status to Discord #dev.
# Designed to run as a launchd periodic job on the Mac mini.
# Tracks the last-reported run ID to avoid duplicate notifications.

set -euo pipefail

REPO="svan058/dunecity"
CHANNEL_ID="1506221759437869166"
STATE_FILE="${HOME}/.openclaw/data/dunecity-build-notify-state"
OPENCLAW="/opt/homebrew/bin/openclaw"

# Ensure state directory exists
mkdir -p "$(dirname "$STATE_FILE")"

# Read last reported run ID
LAST_RUN_ID=""
if [ -f "$STATE_FILE" ]; then
  LAST_RUN_ID=$(cat "$STATE_FILE")
fi

# Get latest completed workflow run with macOS build
LATEST=$(gh run list --repo "$REPO" --limit 5 --json databaseId,status,headBranch,displayTitle,conclusion \
  --jq '[.[] | select(.status=="completed")] | .[0]')

if [ -z "$LATEST" ] || [ "$LATEST" = "null" ]; then
  exit 0
fi

RUN_ID=$(echo "$LATEST" | python3 -c "import sys,json; print(json.load(sys.stdin)['databaseId'])")
CONCLUSION=$(echo "$LATEST" | python3 -c "import sys,json; print(json.load(sys.stdin)['conclusion'])")
TITLE=$(echo "$LATEST" | python3 -c "import sys,json; print(json.load(sys.stdin)['displayTitle'])")
BRANCH=$(echo "$LATEST" | python3 -c "import sys,json; print(json.load(sys.stdin)['headBranch'])")

# Skip if already reported
if [ "$RUN_ID" = "$LAST_RUN_ID" ]; then
  exit 0
fi

# Get macOS job status specifically
MAC_STATUS=$(gh run view "$RUN_ID" --repo "$REPO" --json jobs \
  --jq '.jobs[] | select(.name=="build-macos") | .conclusion' 2>/dev/null || echo "unknown")

if [ "$MAC_STATUS" = "success" ]; then
  EMOJI="green_check"
  MSG="macOS build passed — ${TITLE} (${BRANCH}). DMG artifact ready for download."
elif [ "$MAC_STATUS" = "failure" ]; then
  EMOJI="x"
  MSG="macOS build failed — ${TITLE} (${BRANCH})"
elif [ "$MAC_STATUS" = "skipped" ] || [ "$MAC_STATUS" = "cancelled" ]; then
  MSG="macOS build ${MAC_STATUS} — ${TITLE} (${BRANCH})"
else
  # macOS job hasn't finished or doesn't exist in this run
  exit 0
fi

# Post to Discord #dev
"$OPENCLAW" message send --channel discord -t "$CHANNEL_ID" \
  -m "**Build notification:** ${MSG}" 2>/dev/null || true

# Save state
echo "$RUN_ID" > "$STATE_FILE"
