#pragma once
// The names items are shown by (docs/specs/alpha-item-names.md): the dictionary word of the
// item, its material and, for items with a seed, a rarity adjective that may carry a made-up
// name ("glorious Iron Sword of Grimika").
#include "Dictionary.h"
#include "creatures/Inventory.h"
#include <string>

// §1.1: the dictionary key of an item's word, "" when there is none.
std::string itemKey(uint8_t type, uint8_t subtype);
// §1.2: the dictionary key of a material, "Material" when it has no entry.
std::string materialKey(uint8_t material);
// §3.1: the name a rarity-3 or rarity-4 item carries for "#".
std::string seedName(uint32_t seed);
// §2-§6 (0x598a50): an item's name, UTF-8.
std::string itemName(const creatures::ItemRecord &item, const Dictionary &dictionary);
// §7 and ui spec §8.1.9: what the chat says after "You receive ": "37 Copper", "2 X Mushroom",
// "Iron Sword +17".
std::string receivedText(const creatures::ItemRecord &item, const Dictionary &dictionary);
