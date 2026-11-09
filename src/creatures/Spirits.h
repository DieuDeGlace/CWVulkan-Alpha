#pragma once
// Cubes attached to a weapon at the Weapon Customization bench (docs/specs/alpha-item-services.md
// §3): the item's 32 spirit entries (x, y, z, material, level), how many a weapon takes, placing
// and removing them, what the bench offers from the bags, and their colours.
#include "creatures/Combat.h"
#include "creatures/Inventory.h"
#include <glm/glm.hpp>

namespace creatures {

struct SpiritCube {
    glm::ivec3 cell{0}; // voxel cell in the weapon model's grid
    uint8_t material = 0;
    int32_t level = 1;
};

int32_t spiritCount(const ItemRecord &item);
SpiritCube spiritAt(const ItemRecord &item, int32_t k);
// §3.3: 32 for two-handers and ranged weapons, 16 for the others.
int32_t spiritCapacity(const ItemRecord &item);
// §3.6 place rule: replaces a cube at that cell, else appends while below the capacity.
void placeSpirit(ItemRecord &item, glm::ivec3 cell, uint8_t material, int32_t level);
// §3.6 remove rule: the first cube at that cell goes, the later ones move down.
void removeSpirit(ItemRecord &item, glm::ivec3 cell);
// Item services §0: the level of a power, lvl(p).
int32_t levelOfPower(int32_t power);
// §3.4: what the bench offers for a weapon: its own material's cubes, then spirits from 10
// below its power up to it.
std::vector<Stack> spiritPalette(const Inventory &inventory, const ItemRecord &weapon);
// §3.6: takes one item of a palette entry from the first matching bag stack.
void takeSpiritItem(Inventory &inventory, const ItemRecord &entry);
// §3.9, materials §1.1: an attached cube's colour at strength 0 (the spirits' own colours,
// ordinary materials' factors on the white cube).
glm::vec3 spiritColour(uint8_t material);
// Materials §1.5: the average colour of an item's spirit cubes at strength 1 (f = 1.5), and
// whether it has any.
bool averageSpiritColour(const Item &item, glm::vec3 &colour);

} // namespace creatures
