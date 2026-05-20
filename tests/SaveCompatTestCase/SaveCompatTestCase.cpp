/*
 *  SaveCompatTestCase.cpp
 *
 *  Regression tests for savegame backward compatibility after the
 *  Num_ItemID expansion from 48 (v1.0.7) to 52 (v1.0.8+).
 *
 *  The actual ObjectData::load round-trip cannot be tested here without
 *  pulling in heavy deps (FileManager, INIFile, etc.), so these tests
 *  verify the constants and stream-format assumptions that the fix relies on.
 */

#include <catch2/catch_all.hpp>
#include <data.h>
#include <Definitions.h>

TEST_CASE("Save compat: LEGACY_NUM_ITEM_ID_9810 matches v1.0.7 value",
          "[save-compat][regression]") {
    // v1.0.0-v1.0.7 all shipped with Num_ItemID=48.
    // This constant must stay at 48 forever — it defines how many items
    // those saves contain on disk.
    REQUIRE(LEGACY_NUM_ITEM_ID_9810 == 48);
}

TEST_CASE("Save compat: current Num_ItemID >= legacy",
          "[save-compat][regression]") {
    // We only add items, never remove. If this fails, the backward-compat
    // logic in ObjectData::load needs revisiting.
    REQUIRE(Num_ItemID >= LEGACY_NUM_ITEM_ID_9810);
}

TEST_CASE("Save compat: SAVEGAMEVERSION is 9811 or higher",
          "[save-compat][regression]") {
    // 9811 introduced the self-describing item count in ObjectData::save.
    // If this is ever lowered back to 9810, the fix is broken.
    REQUIRE(SAVEGAMEVERSION >= 9811);
}

TEST_CASE("Save compat: new items are at indices >= LEGACY_NUM_ITEM_ID_9810",
          "[save-compat][regression]") {
    // The four items added in v1.0.8 must be at indices 48-51 (after the
    // legacy boundary). If they were inserted in the middle, the fix
    // would read the wrong data for old saves.
    REQUIRE(Structure_Stadium == 48);
    REQUIRE(Structure_Airport == 49);
    REQUIRE(Unit_AmbientAirplane == 50);
    REQUIRE(Unit_AmbientHelicopter == 51);
}

TEST_CASE("Save compat: Unit_Troopers is last legacy item at index 47",
          "[save-compat][regression]") {
    // The boundary item — the last item in a v1.0.7 save.
    REQUIRE(Unit_Troopers == 47);
    REQUIRE(Unit_Troopers == LEGACY_NUM_ITEM_ID_9810 - 1);
}

TEST_CASE("Save compat: extended items classified correctly",
          "[save-compat][regression]") {
    // Extended structures
    REQUIRE(isStructure(Structure_Stadium));
    REQUIRE(isStructure(Structure_Airport));
    REQUIRE_FALSE(isUnit(Structure_Stadium));

    // Extended units
    REQUIRE(isUnit(Unit_AmbientAirplane));
    REQUIRE(isUnit(Unit_AmbientHelicopter));
    REQUIRE_FALSE(isStructure(Unit_AmbientAirplane));
}
