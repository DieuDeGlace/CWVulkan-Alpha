#pragma once
// Crafting (docs/specs/alpha-crafting.md): the recipes a character knows (keys: the items
// they make), what each needs and where, the six tab lists, the level grants, and making an
// item. All of it is client-side, as in Alpha; the tables are plain data so mods can add to
// them.
#include "creatures/Inventory.h"
#include <functional>
#include <string>

namespace creatures {

// §3.1: station ids.
enum Station : int32_t {
    NO_STATION = 0, WATER = 1, FURNACE = 3, ANVIL = 4, SPINNING_WHEEL_STATION = 5, LOOM_STATION = 6, SAW = 7,
    WORKBENCH_STATION = 8, CAMPFIRE_STATION = 9,
};
// The static kind a station is (0 for none and water), and its "Requires …" text.
int32_t stationKind(int32_t station);
const char *stationText(int32_t station);

// §1.2: a recipe record.
struct Recipe {
    ItemRecord product{};
    int32_t station = NO_STATION;
    std::vector<std::pair<ItemRecord, int32_t>> ingredients; // item, count needed
};

// §2.1 (0x59cff0): the recipe of a key, with the rarity surcharge.
Recipe buildRecipe(const ItemRecord &key);
// §2.4: the tab of a key (0 Weapons, 1 Armor, 2 Amulets, 3 Cooking, 4 Alchemy, 5 the
// ingredients), or -1 when it is not listed.
int recipeTab(const ItemRecord &key);
// §2.4: the six tab lists from the known keys, each sorted by 6·level + rarity, highest first.
std::array<std::vector<Recipe>, 6> recipeTabs(const std::vector<ItemRecord> &known);
// §2.4 step 3: how many of an item the bags hold (the item in hand not counted), and how many
// times a recipe can be made (0 without ingredients).
int32_t holding(const Inventory &inventory, const ItemRecord &item);
int32_t timesCraftable(const Inventory &inventory, const Recipe &recipe);

// §2.2 (0x47fae0): the keys a character is given on reaching `level` (class 1-4).
void grantLevel(std::vector<ItemRecord> &known, int32_t level, int32_t characterClass, const std::function<int()> &draw);

// §5.3: makes a recipe once. Not carried (refused) → nothing changes.
struct CraftOutcome {
    bool refused = false;          // "You can't carry more of these items."
    std::optional<ItemRecord> made; // added to the bags
    std::vector<int32_t> sounds;    // 0x59, and 0x5a for a bonus
};
CraftOutcome craft(Inventory &inventory, const Recipe &recipe, const std::function<int()> &draw);

} // namespace creatures
