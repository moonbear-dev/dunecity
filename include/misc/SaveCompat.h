#ifndef SAVECOMPAT_H
#define SAVECOMPAT_H

#include <Definitions.h>
#include <string>
#include <cstdio>

/// Return the ObjectData item count for a pre-9811 save, or 0 for self-describing (9811+).
///
/// Mapping:
///   "dunelegacy*"            → LEGACY_NUM_ITEM_ID_DUNELEGACY (41)
///   "dunecity1.0.0"–"1.0.7" → LEGACY_NUM_ITEM_ID_9810 (48)
///   "dunecity1.0.8"–"1.0.10"→ 52
///   savegameVersion > 9810  → 0 (count is in the stream)
inline int determineLegacySavedItemCount(int savegameVersion, const std::string& duneVersion) {
    if(savegameVersion > 9810) {
        return 0;  // 9811+ writes the count into the stream
    }

    // Original Dune Legacy saves: "dunelegacy0.99.5" etc.
    if(duneVersion.find("dunelegacy") != std::string::npos) {
        return LEGACY_NUM_ITEM_ID_DUNELEGACY;  // 41
    }

    // DuneCity saves — parse version to distinguish 48-item vs 52-item era
    auto pos = duneVersion.find("dunecity");
    if(pos != std::string::npos) {
        std::string verPart = duneVersion.substr(pos + 8);
        int major = 0, minor = 0, patch = 0;
        if(std::sscanf(verPart.c_str(), "%d.%d.%d", &major, &minor, &patch) >= 2) {
            if(major > 1 || (major == 1 && minor > 0) || (major == 1 && minor == 0 && patch >= 8)) {
                return 52;
            }
        }
    }

    return LEGACY_NUM_ITEM_ID_9810;  // 48 — default for dunecity 1.0.0–1.0.7
}

#endif // SAVECOMPAT_H
