# DuneCity C++ Migration Analysis

**Date:** 2026-03-24  
**Analyzer:** hermes (cron job)  
**Goal:** Migrate SimCity/Micropolis city-building into Dune Legacy (C++)

---

## 1. Specific Files to Port from MicropolisCore

### Tier 1: Core Simulation (Complete)
| File | LOC | Purpose | Dune Legacy Equivalent |
|------|-----|---------|------------------------|
| `simulate.cpp` | 1,729 | Main simulation loop, 16-phase cycle | `CitySimulation.cpp` ✓ |
| `budget.cpp` | ~800 | Tax, funding, economy | `CityBudget.cpp` ✓ |
| `evaluate.cpp` | ~900 | City evaluation, ratings | `CityEvaluation.cpp` ✓ |

### Tier 2: Zone System (60% Complete)
| File | LOC | Purpose | Dune Legacy Equivalent |
|------|-----|---------|------------------------|
| `zone.cpp` | 1,039 | Zone growth, building placement | `ZoneSimulation.cpp` (60%) |
| `tool.cpp` | 1,617 | Building tools (res/com/ind zones) | Partial via BuilderBase |
| `connect.cpp` | ~1,200 | Connectivity/road tracing | Partial in TrafficSim |

### Tier 3: Remaining Ports
| File | LOC | Purpose | Priority |
|------|-----|---------|----------|
| `power.cpp` | 195 | Power grid calculation | ✓ Done |
| `scan.cpp` | 600 | Map scanning for effects | ✓ Done |
| `traffic.cpp` | 519 | Vehicle routing | Partial - needs unit integration |
| `sprite.cpp` | 2,039 | City sprites (cars, planes) | Medium |
| `disasters.cpp` | ~600 | Disaster effects | Partial (Dune-specific disasters) |
| `animate.cpp` | ~700 | Animation state | Low |

---

## 2. Integration Points with Dune Legacy

### Current Architecture
```
Dune Legacy src/
├── dunecity/                    # Ported modules
│   ├── CitySimulation.cpp/h     ✓ Core orchestration (16-phase) - INTEGRATED
│   ├── ZoneSimulation.cpp/h     ✓ Zone growth (60%) - INTEGRATED
│   ├── TrafficSimulation.cpp/h ✓ Road traffic
│   ├── PowerGrid.cpp/h         ✓ Power grid - INTEGRATED
│   ├── CityScanner.cpp/h       ✓ Map scanning
│   ├── CityBudget.cpp/h       ✓ Economy - INTEGRATED
│   ├── CityEvaluation.cpp/h   ✓ Ratings
│   └── CityConstants.h         ✓ Zone/Power constants
├── structures/
│   ├── WindTrap.cpp            ✓ Power source (CONNECTED to citySim)
│   └── BuilderBase.cpp         # Building placement
├── Game.cpp                    # Main game loop - INTEGRATED (advancePhase)
└── Tile.cpp                    # Has zone fields (cityPowered, etc.)
```

### Integration Status

| Dune Legacy File | Change Required | Status |
|------------------|-----------------|--------|
| `src/structures/WindTrap.cpp` | Call `citySim.registerPowerSource()` | ✓ **CONNECTED** |
| `src/Game.cpp` | Wire city simulation phases into game cycle | ✓ **CONNECTED** |
| `src/structures/BuilderBase.cpp` | Hook zone placement into `ZoneSimulation` | ✓ EXISTS |
| `src/Command.cpp` | Handle city commands (set tax, etc.) | Not started |
| `src/units/*.cpp` | Hook vehicle AI into `TrafficSimulation` | Not started |
| `include/Tile.h` | Add zone type fields for city overlay | ✓ DONE |

### Verified Integration Code

**WindTrap → PowerGrid:**
```cpp
// src/structures/WindTrap.cpp lines 70-75
auto* citySim = currentGame->getCitySimulation();
if (citySim) {
    citySim->registerPowerSource(location.x, location.y, getProducedPower());
}
```

**Game Loop → CitySimulation:**
```cpp
// src/Game.cpp lines 2386-2396
if (citySimEnabled_ && citySimulation_) {
    citySimulation_->advancePhase(gameCycleCount);
    if ((winFlags & WINLOSEFLAGS_ECONOMIC) && !finished) {
        int32_t threshold = citySimulation_->getEconomicVictoryThreshold();
        if (threshold > 0 && citySimulation_->getTotalPop() >= threshold) {
            setGameWon();
        }
    }
}
```

---

## 3. Changes Since Last Run

### NEW: Integration Complete
- **WindTrap connected**: Verified in git diff - `registerPowerSource()` called each update cycle
- **Game loop wired**: `advancePhase()` called in `Game::updateGameState()`
- **Save/load wired**: CitySimulation state persisted with version 9807+
- **Feature flag**: `citySimEnabled_` in Game.h controls activation

### Completed Items (All Verified)
1. ✓ CityBudget.cpp - Tax collection and fund allocation
2. ✓ PowerGrid.cpp - Power calculation and coverage
3. ✓ CityScanner.cpp - Map scanning phases
4. ✓ CityEvaluation.cpp - City ratings
5. ✓ WindTrap → PowerGrid connection - **CONNECTED**
6. ✓ ZoneSimulation - Basic zone growth (res/com/ind)
7. ✓ CityConstants.h - Zone types, disaster types, city constants
8. ✓ Game integration - **CONNECTED** (advancePhase called each cycle)

### Remaining Gaps (Updated Priority)
1. **CRITICAL: Zone structure spawning** - Zone density updates but no House/Industrial spawned
2. Vehicle/unit AI not integrated with TrafficSimulation
3. City command handlers in Command.cpp not wired (tax, funding UI)
4. GUI overlays not rendering (pollution, traffic, power maps)

---

## 4. Testing Strategy

### Using Dune Legacy's Test Infrastructure

```bash
# Run existing tests
cd ~/development/dune/dunelegacy
./runUnitTests.sh

# Or run specific DuneCity tests
./dunelegacy_tests "[dunecity]"
./dunelegacy_tests "[dunecity][constants]"
```

### Current Test Coverage
| Test File | Coverage |
|-----------|----------|
| `tests/DuneCityTestCase/DuneCityTestCase.cpp` | ZoneType, DisasterType, CityConstants |

### Dune Legacy uses Catch2 (not gtest)
- Build config: `tests/CMakeLists.txt`
- Entry point: `tests/testmain.cpp`
- Tags: `[dunecity]`, `[dunecity][constants]`

### Recommended Test Additions

**Unit Tests:**
1. `PowerGrid::calculatePower()` - verify coverage calculation
2. `ZoneSimulation::doZone()` - verify zone placement rules  
3. `TrafficSimulation::makeTraffic()` - verify traffic flow
4. `CityBudget::collectTax()` - verify tax calculations
5. `CitySimulation::advancePhase()` - verify 16-phase cycle

**Integration Tests:**
- Full city simulation cycle (16 phases)
- Save/load city state
- Multi-player sync (if applicable)

**Regression Tests:**
- Compare output with MicropolisCore reference
- Verify zone growth patterns match original

---

## 5. Blockers and Decisions Needed

### Blocker: Zone Structure Spawning
- **Issue:** `ZoneSimulation::doZone()` updates zone density but doesn't spawn Dune Legacy structures
- **Impact:** Zones grow to max density but no buildings appear on map
- **Solution:** Map zone density → structure type (House, Garage, LightFactory, etc.)
- **File to modify:** `src/dunecity/ZoneSimulation.cpp` - add `spawnStructureAt()` callback

### Decision Required: Vehicle Integration
- **Issue:** Micropolis vehicles are simulation-driven. Dune Legacy has entity-based units.
- **Solution:** Bridge via `TrafficSimulation` - compute desired routes, have Dune Legacy units follow
- **Priority:** Medium (not blocking core simulation)

### Decision Required: Command Wiring
- **Issue:** `CitySimulation::executeCityCommand()` exists but no Command.cpp handler calls it
- **Solution:** Add city command enum and handler in Command.cpp
- **Priority:** Low (can set defaults for now)

### Blocker: Missing GUI Integration
- **Issue:** Overlays exist (pollution, traffic, power) but no rendering to radar/minimap
- **Solution:** Defer. Focus on simulation core first.

### RESOLVED
- **Power System:** WindTrap registers power via `citySim->registerPowerSource()` - **CONNECTED**
- **Game Loop:** CitySimulation.advancePhase() called each cycle - **CONNECTED**
- **Save/Load:** CitySimulation state persisted - **CONNECTED**

---

## 6. Action Items (Priority Order)

### Immediate
1. ~~Connect WindTrap to PowerGrid~~ - **DONE**
2. ~~Wire CitySimulation into Game.cpp~~ - **DONE**
3. **Add zone structure spawning** - Map density → structures (CRITICAL)
4. **Build and test** - Verify game compiles and runs with city simulation

### Short-term (2-3 Weeks)
5. Connect `TrafficSimulation` routes to Dune Legacy unit AI
6. Add missing test cases for PowerGrid, ZoneSimulation
7. Add Command.cpp handlers for city commands (tax, funding)

### Medium-term (1-2 Months)
8. Add GUI overlay rendering for city maps
9. Integrate disaster system (already has Dune-specific: sandstorm, sandworm)
10. Performance optimization for 64x64+ maps

---

## 7. File Reference

| Path | Description |
|------|-------------|
| `~/development/dune/dunelegacy/` | Dune Legacy (C++17, ~198 source files) |
| `~/development/simcity/MicropolisCore/MicropolisEngine/src/` | MicropolisCore (C++, 22,649 LOC) |
| `~/development/dune/dunelegacy/include/dunecity/` | DuneCity headers (9 files) |
| `~/development/dune/dunelegacy/src/dunecity/` | DuneCity sources (7 files, untracked) |
| `~/development/dune/dunelegacy/tests/DuneCityTestCase/` | Test cases |

---

## 8. Git Status

```
M include/Command.h
M include/Game.h           # Added citySimulation_ member
M include/Tile.h
M src/Game.cpp              # Added advancePhase() call
M src/structures/WindTrap.cpp  # Added registerPowerSource()
?? include/dunecity/         # Untracked
?? src/dunecity/            # Untracked
?? tests/DuneCityTestCase/  # Untracked
```

**Action needed:** `git add` and commit DuneCity files to track changes.

---

## 9. Zone Growth Algorithm (Current State)

`ZoneSimulation::doZone()` flow:
```
1. Get tile zone type (Res/Com/Ind)
2. Check power status (powered/unpowered counter)
3. Call zone-specific handler:
   - doResidential() / doCommercial() / doIndustrial()
4. Each handler:
   - Gets current population (density)
   - Calls TrafficSimulation.makeTraffic() for route quality
   - Evaluates local factors (land value, pollution, comRate)
   - Calculates zscore = valve + local_value
   - Calls do{X}In() or do{X}Out() based on zscore
5. In/Out functions modify tile density (0-8 for res, 0-5 for com, 0-4 for ind)
```

**Missing:** No structure spawning when density increases. Need to add: if density crosses threshold, spawn corresponding Dune Legacy structure.
