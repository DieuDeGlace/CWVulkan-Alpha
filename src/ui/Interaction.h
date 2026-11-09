#pragma once
// What the crosshair offers to do (docs/specs/alpha-interaction.md): the prompts of §3 for
// the creature and the static the player is aiming at.
#include <cstdint>
#include <string>

namespace ui {

// §3, the static kinds that matter here (alpha-worldgen-structures.md §0 table B).
enum StaticKind : int32_t {
    DOOR = 1, BIG_DOOR = 2, WINDOW = 3, LEVER = 9, CHEST = 10, STOOL = 16, BENCH = 18, BED = 19,
    RUNESTONE = 45, CAMPFIRE = 65, BEACH_TOWEL = 68, WOOD_MAT = 69, FURNACE = 71, ANVIL = 72,
    SPINNING_WHEEL = 73, LOOM = 74, SAWBENCH = 75, WORKBENCH = 76, CUSTOMIZATION_BENCH = 77,
};

// §3 step 3: the prompt for a static of `kind` in `state` (+0x30: 1 closed, 0 open, 2 a
// closed chest with loot); `occupiedByPlayer` hides the seat, bed and station prompts.
std::u16string staticPrompt(int32_t kind, uint8_t state, bool occupiedByPlayer);

// §3 step 2: 0x444760, the creature types a player can ride.
bool rideable(int32_t type);

} // namespace ui
