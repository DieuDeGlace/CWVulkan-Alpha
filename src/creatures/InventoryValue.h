#pragma once
#include "creatures/ItemRecord.h"
#include <functional>
#include <vector>

namespace creatures {

// Constructor-defined semantic fields, with deterministic engine-zero gaps.
// Native constructor storage does not establish values for those five gaps.
ItemRecord freshInventoryItem();
struct CountedItem {
    int32_t count = 0;
    ItemRecord item = freshInventoryItem();
};

// Full grouped value, independently owned. The auxiliary word and standalone
// item are retained without assigning unverified UI meanings to them.
struct InventoryValue {
    std::vector<std::vector<CountedItem>> groups;
    uint32_t auxiliary = 0;
    ItemRecord separate = freshInventoryItem();
    std::array<uint32_t, 2> accumulators{};

    // Native semantic assignment preserves destination item gaps and all counts.
    void copyFrom(const InventoryValue &source);
    // Explicit caller policy; fresh construction itself has no groups.
    void reset(size_t emptyGroups = 0);
    // -1 selects the held type classifier. Other negative values are outside
    // the native ordinary domain and are rejected by this engine API.
    void insert(const ItemRecord &item, int32_t group = -1);
    // Bounded collection-only producer: count1 before each of ten callbacks.
    void replaceTen(const std::function<ItemRecord(size_t index)> &generate);
};

} // namespace creatures
