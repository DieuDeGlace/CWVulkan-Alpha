#pragma once
// What a creature leaves when it dies (docs/specs/alpha-loot-rolls.md, server simulation
// §6.4): its equipment, a random armour or weapon and pet food, the drops of its kind, a
// recipe, coins or leftovers, and its inventory, each dropped as a world item round it. The
// draws follow the original's order exactly, so one rand() stream gives the same loot.
#include "WorldItems.h"
#include "creatures/Npcs.h"
#include "worldgen/Random.h"

// §1: the rarity roll ρ(m, force): four draws; forced, the result is m + 1 (at most 4).
int32_t rarityRoll(worldgen::Random &random, int32_t m, bool force = false);
// §2: the level re-roll with spread 0.05: none or one draw. `widen` (the loot's way) lets the
// level rise as well; without it the level only stays or goes down.
int16_t rerollLevel(worldgen::Random &random, int16_t level, bool widen = true);
// §4: the 45 creature types pet food may be for.
extern const uint8_t TAMEABLE[45];

struct LootVictim {
    uint8_t controller = 1; // 0 player, 1 hostile NPC, 3 merchant …
    int32_t type = 0;
    uint16_t appearanceFlags = 0; // 0x08 / 0x10 animal, 0x200 boss, 0x800 no loot
    int32_t level = 1;
    int32_t tier = 0;
    std::array<creatures::ItemRecord, 13> equipment{};
    std::vector<creatures::Stack> inventory; // all bags, in order
    int64_t position[3] = {0, 0, 0};          // 1/65536 blocks
};

// The victim of an NPC death (its equipment rebuilt as item records).
LootVictim lootVictim(const creatures::NpcFrame::Death &death);

// Item services §1.5: the identifier's fee, in copper (0 without Leftovers, type 14).
int32_t identificationFee(const creatures::ItemRecord &leftovers);
// Item services §1.6 step 3-4: what Leftovers turn into: one draw, then the weapon (even) or
// armour (odd) generator at their level and rarity, for any class.
creatures::ItemRecord identify(const creatures::ItemRecord &leftovers, worldgen::Random &random);
// Item services §2.4: the platinum an adaption to `playerLevel` costs, as Alpha has it. It grows
// with the level gap times an exponential of the item's level, so late items cost tens of
// thousands.
int32_t adaptionCost(int16_t level, uint8_t rarity, int32_t playerLevel);
// The required engine adaptation cost: the rule of the
// "Cube World Adaption Rebalance" mod by coremaze (Chris Miuchiz),
// https://github.com/coremaze/Cube-World-Adaption-Rebalance, written here from its
// description. With d = playerLevel - (level - 5 + rarity) the cost is
// (P + 1) / (P + 10) * d / (d + 1500) * 10000, truncated: a curve that levels off below
// 10 000 platinum. Nothing is paid when the item is not below the player or d <= 0.
int32_t rebalancedAdaptionCost(int16_t level, uint8_t rarity, int32_t playerLevel);

// §3: drops the victim's loot into `items` (a drop over a zone that is not loaded is lost).
void dropLoot(const LootVictim &victim, WorldItems &items, int32_t day, const creatures::BlockType &block,
              worldgen::Random &random);
