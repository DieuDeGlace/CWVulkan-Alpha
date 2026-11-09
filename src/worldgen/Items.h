#pragma once
// Alpha's random equipment generators (docs/specs/alpha-items-generator.md): dungeon chests,
// boss loot, table loot and recipes. Each builds a list of candidates, drawing as it goes, and
// then picks one with a last draw.
#include "worldgen/Random.h"
#include "worldgen/Zone.h"

namespace worldgen {

// §2.1: armour of class c (1 warrior, 2 ranger, 3 mage, 4 rogue; below 0: any), plus an
// amulet and a ring. The rarity is stored as given.
ItemRecord armourItem(Random &random, int32_t level, int32_t rarity, int32_t c);
// §2.2: a weapon of class c. Class 0 or above 4 has no candidates (the original divides by
// zero); an empty record comes back after the pick draw.
ItemRecord weaponItem(Random &random, int32_t level, int32_t rarity, int32_t c);
// §3: a recipe (type 2) for a random armour piece or weapon; its product type is `product`.
ItemRecord recipeItem(Random &random, int32_t level, int32_t rarity);
// §1: dungeon loot, 128 or 136 draws. `rarityCap` must be 0 or more.
ItemRecord lootItem(Random &random, int32_t level, int32_t rarityCap);

} // namespace worldgen
