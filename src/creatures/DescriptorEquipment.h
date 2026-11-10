#pragma once
#include "creatures/Inventory.h"
#include <functional>
#include <span>

namespace creatures::equipment {

// Borrowed descriptor values, independently arranged. Items retain their full
// payloads and gaps; only explicitly reached header/scalar fields are written.
struct Descriptor {
    int32_t kind, role;
    uint16_t level;
    const uint16_t &flags;
    uint8_t &classification, &companion;
    std::span<ItemRecord, 13> items;
    std::span<float, 5> multipliers;
    std::vector<int32_t> &actions;
    int32_t &choice;
    std::vector<int32_t> &candidates;
};

// Held supplied-input setup. Only the option's low byte admits numeric setup.
// Successful allocations, stable inputs and ordinary finite arithmetic assumed.
void setup(Descriptor descriptor, int32_t option, const std::function<int32_t()> &draw);

} // namespace creatures::equipment
