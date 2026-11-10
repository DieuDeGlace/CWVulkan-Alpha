#pragma once
#include "creatures/Inventory.h"

namespace creatures {

struct LinkedItemSnapshot {
    ItemRecord item{};
    uint32_t word = 0;
    LinkedItemSnapshot() { setItemLevel(item, 1); }
};

// Only the source belongs to copied core state. Retained comparison state stays
// with its actor across core assignment. Fresh padding is engine zero.
struct LinkedItemState {
    ItemRecord source{};
    LinkedItemSnapshot retained;
    LinkedItemState() { setItemLevel(source, 1); }
    bool matches(uint32_t word) const {
        return retained.word == word && sameItem(source, retained.item);
    }
    void publish(uint32_t word) {
        retained.word = word;
        copyItemFields(source, retained.item);
    }
};

} // namespace creatures
