## Build: success

Release build completed without errors. Pre-existing warnings only (Widget operator=, dylib versions).

## Files changed

### Headers
- `include/DataTypes.h` — added `HOUSE_REBELS = 7`, NUM_HOUSES now 8
- `include/Colors.h` — added `PALCOLOR_REBELS 192`
- `include/globals.h` — extended `houseToPaletteIndex[]` (8 entries) and `houseChar[]` (+`'R'`)
- `include/structures/Palace.h` — added `spawnRebelsUnits()` declaration

### Source
- `src/sand.cpp` — added "Rebels" to `getHouseByName()` and `getHouseNameByNumber()` arrays
- `src/FileClasses/SFXManager.cpp` — added `HOUSE_REBELS` case in `loadEnglishVoice()` switch (uses Harkonnen voice prefix "H")
- `src/FileClasses/GFXManager.cpp` — Tornie Custom_IBM.pal palette override (entries 192-199); HeraldRebels.png loading with fallback to Neutral remap; Rebels mentat background (reuses Neutral style); Herald_Grey for Rebels
- `src/FileClasses/TextManager.cpp` — added `HOUSE_REBELS` fallthrough to `HOUSE_NEUTRAL` in mission text switch; added to Ordos/Mercenary/Neutral mentat group
- `src/structures/Palace.cpp` — added `HOUSE_REBELS` case in `doSpecialWeapon()` + `spawnRebelsUnits()` (spawns 1-2 Sonic Tanks in HUNT mode); added `#include <units/SonicTank.h>`
- `src/ObjectBase.cpp` — added `HOUSE_REBELS` case for Unit_Special: creates SonicTank (matches palace ability)
- `src/Menu/MapChoice.cpp` — added `HOUSE_REBELS` case ("REB") + default to campaign region switch
- `src/MapEditor/MapEditor.cpp` — added HOUSE_REBELS to default player lists (v1/v2 and loadMap)
- `src/Menu/CustomGamePlayers.cpp` — skip HOUSE_REBELS in house dropdown loop (AI-only, not player-selectable)

### Data
- `mods/Tornie/ObjectData.ini` — added Rebels-specific tech entries:
  - `Builder(R) = Invalid` for Deviator (disabled)
  - `Builder(R) = Heavy Factory` + `TechLevel(R) = 6` for Sonic Tank (enabled)
  - `Builder(R) = Heavy Factory` + `TechLevel(R) = 9` for Elite Siege Tank (enabled)
  - `TechLevel(R) = 8` for Flame Tank

### Auto-applied by linter
- `src/sand.cpp` — `HOUSE_REBELS` case added to `getHouseSpiceFraction()` switch
- `src/Menu/CustomGamePlayers.cpp` — "Rebels" section added to `boundHousesOnMap` parsing; Player8 support added

## What works
- Compiles cleanly (Release, no new warnings)
- HOUSE_REBELS (index 7) is fully wired into house name/number resolution, palette mapping, voice loading, object data parsing, herald graphics, mentat background, palace special weapon, map editor placement, campaign text, mentat entries, and Unit_Special creation
- Rebels excluded from human player house picker (AI-only)
- Tornie mod gates: Custom_IBM.pal palette override and HeraldRebels.png loading are conditional on active mod being "Tornie"
- All switch statements on HOUSETYPE audited and HOUSE_REBELS cases added where needed

## What is untested
- In-game Rebels AI behavior (requires a map with Rebels house placed)
- HeraldRebels.png actual rendering (file exists in data/, palette-indexed)
- Custom_IBM.pal grey palette range appearance in-game
- Palace sonic tank spawning behavior
- Map editor Rebels player placement and saving/loading
- Network/multiplayer with Rebels house on map

## PUSH_REQUIRED: next version
