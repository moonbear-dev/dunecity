#!/usr/bin/env python3
"""Generate mods/Tornie/campaign/scenr001-022.ini (Rebels campaign).

Per Tornie mod user request: add a 8th campaign section (Rebels) to
campaign and skirmish modes, after the existing 7 houses
(Atreides, Ordos, Harkonnen/Sardaukar, Mercenary, Fremen, Sardaukar, Neutral).

This script clones the existing Sardaukar scenarios (scens001-022.ini) to
scenr001-022.ini, adjusting the house key from `s` to `r` and rebalancing
starting credits / enemy composition to Rebels-themed content. For now the
clone is verbatim — the user can edit the per-mission specifics later.

Run from the repo root:
    python3 scripts/make-rebels-scenarios.py
"""

import os
import shutil
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = os.path.join(REPO, "mods", "Tornie", "campaign")
SRC_HOUSE = "s"   # Sardaukar — same files used for Harkonnen campaign
DST_HOUSE = "r"   # Rebels — new 8th house

def main():
    if not os.path.isdir(SRC_DIR):
        print(f"ERROR: {SRC_DIR} not found", file=sys.stderr)
        sys.exit(1)

    for mission in range(1, 23):
        src_name = f"scen{SRC_HOUSE}{mission:03d}.ini"
        dst_name = f"scen{DST_HOUSE}{mission:03d}.ini"
        src_path = os.path.join(SRC_DIR, src_name)
        dst_path = os.path.join(SRC_DIR, dst_name)

        if not os.path.isfile(src_path):
            print(f"WARN: missing {src_path}, skipping {dst_name}", file=sys.stderr)
            continue

        # Read source, write target with house swap
        with open(src_path, "r") as f:
            content = f.read()

        # The scenario files use [Sardaukar] as a section header. Replace it
        # with [Rebels] and adjust the Sardaukar house to Rebels in unit /
        # structure listings. The rest of the scenario is identical.
        content = content.replace("[Sardaukar]", "[Rebels]")
        content = content.replace(",Sardaukar,", ",Rebels,")
        # DuneCity 1.0.251: also rewrite leading-identifier Sardaukar
        # values (e.g. 'ID024=Sardaukar,Trike,...' or '1=Sardaukar,Soldier,Homebase,1'
        # in the [UNITS] and [REINFORCEMENTS] sections). The simple
        # substring replace above caught embedded 'Sardaukar,' but missed
        # the leading value. Without this fix, the Rebels campaign
        # starts with Sardaukar-owned units still on the map, which
        # produces the 'green grid / no place to build' symptom.
        import re
        content = re.sub(r'(^|[\s,;=])Sardaukar(,|$)', r'\1Rebels\2', content, flags=re.MULTILINE)
        # Also update the 'Scenario N control for house Sardaukar.' comment.
        content = re.sub(
            r'^; Scenario \d+ control for house Sardaukar\.$',
            f'; Scenario {mission} control for house Rebels (Tornie mod, 8th career).',
            content,
            count=1,
            flags=re.MULTILINE,
        )

        with open(dst_path, "w") as f:
            f.write(content)
        print(f"wrote {dst_path}")

if __name__ == "__main__":
    main()
