# Task: Civic expansion structures and budget polish

## Context

The current DuneCity build already has working R/C/I zones, city budget UI, city-year budget ticks, roads, nuclear plant, police station, overlays, and city effects. The next pass should add higher-order civic/economy structures and clean up visible rough edges that are making the city layer feel unfinished.

This spec is intentionally scoped as a single implementation pass, but each item should be kept modular and compile-safe.

## Goals

### 1. Stadiums

Add stadiums back into the game as a city/civic structure.

Expected behaviour:
- Buildable from the relevant city/construction build flow.
- Occupies a sensible footprint using existing structure-size conventions.
- Provides a positive land-value or quality-of-life effect to nearby residential/commercial areas.
- Should not count as police, industrial, or ordinary housing.
- Use existing/placeholder art if a dedicated stadium sprite is not already available, but name and icon should make it recognizable.

### 2. Airports

Add airports as a city/economic structure.

Expected behaviour:
- Buildable from the relevant city/construction build flow.
- Provides a commercial/transport boost and a land-value or growth effect appropriate for nearby commercial/industrial areas.
- Should not break aircraft/combat systems or existing Ornithopter selection UI.
- Use existing/placeholder art if a dedicated airport sprite is not already available, but name and icon should make it recognizable.

### 3. Palace as Super Residential Zone

Add a palace-like structure that functions as a high-capacity residential civic building.

Expected behaviour:
- High-capacity residential civic building: 250/500/1000 people at levels 1/2/3.
- Consumes 100 power. Grows slowly via the same occupancy path as other city-role structures.
- Contributes residential population through the same city simulation totals used by budget and HUD.
- Provides a park-style land-value bonus (same as walls/turrets) in a 3-tile radius.
- Palace sidebar UI shows population count below the special weapon display.
- Must not alter normal R/C/I zone footprint rules.

### 4. Road visual fix

Roads are still rendering brown in some cases. They should render as proper road pavement/markings, consistent with the intended DuneCity road art.

Expected behaviour:
- Existing placed roads visibly look like roads, not brown terrain.
- Connectivity/edge markings should stay legible.
- Fix should apply to normal render and any relevant overlay interaction.
- Avoid changing road gameplay placement rules unless the visual bug is caused by missing tile state.

### 5. Pollution effects

Pollution should materially impact the city model beyond simply being an overlay.

Expected behaviour:
- Pollution should reduce land value where appropriate.
- Pollution should suppress or slow residential/commercial growth when high.
- Industrial pollution should remain a tradeoff rather than making industry impossible.
- Existing pollution overlay should still represent the same underlying data.
- Add or update tests for pollution-to-value/growth behaviour if practical.

### 6. Starport as shipyard

Make the Starport function like a shipyard within the DuneCity economy.

Expected behaviour:
- Starport should provide a meaningful industrial/commercial/economic role, not just vanilla RTS purchasing.
- It should interact with the city simulation in a way that fits DuneCity: likely jobs, industrial capacity, trade/shipyard boost, and/or tax base.
- Avoid breaking existing Starport purchasing/CHOAM behaviour.

### 7. Nuclear icon

Make the nuclear plant icon look nuclear.

Expected behaviour:
- Nuclear plant build/menu icon should be visually distinct from generic placeholder art.
- Prefer existing game art if suitable; otherwise add a small repo-native asset following current asset conventions.
- Do not invent a large new art pipeline for one icon.

## Existing issues to preserve/fix

The current working tree includes a budget window fix:
- Budget window refreshes while open.
- Budget "Treasury" label is now "Credits" to match the single player credit pool.
- Projected revenue/net use pending slider values.

Keep that work intact and compile it with the rest of this pass.

## Constraints

- Work in `/Users/stefanclaw/development/dunecity`.
- Treat `/Users/stefanclaw/development/simcity` as read-only Micropolis reference.
- Preserve the settled rule: normal R/C/I gameplay zones stay 2x2. The palace may represent 3x3 worth of residents, but it must not convert normal zone placement to 3x3.
- Keep edits scoped. Do not refactor unrelated systems.
- Preserve save/load compatibility unless a version bump is strictly required and documented.
- Must compile on macOS arm64 using the existing `build` directory/target.
- Do not restart OpenClaw or the gateway from inside the worker.

## Suggested code areas

Start by inspecting:
- `include/data.h`
- `config/ObjectData.ini.default`
- `src/dunecity/CityEffectsRuntime.cpp`
- `include/dunecity/CityEffects.h`
- `src/GUI/dune/CityBudgetWindow.cpp`
- road rendering paths in `src/Tile.cpp` / `src/Game.cpp`
- builder/build-list paths for city structures
- existing structure classes for NuclearPlant, PoliceStation, ZoneStructure, StarPort

## Verification

Minimum verification:
- Compile changed code, at least the `dunecity` target or the smallest target that proves changed objects compile.
- Run focused tests if existing city-effect/structure tests cover the changed helpers.
- Report any item that is only partially implemented or blocked by missing art/data.

## Completion report

Report:
- Built
- Not yet built
- Blocked on
- Verification commands and outcomes

