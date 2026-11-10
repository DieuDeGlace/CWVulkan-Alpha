#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>

namespace creatures::activation {

// Borrow a cached context record, retaining separate reads at its use sites.
// The caller owns stable context/descriptor storage for this operation.
struct Context {
    const uint32_t &seed;
    const int32_t &selector;
    const int32_t &level;
    const uint8_t &tier;
};
struct Descriptor {
    uint64_t identifier;
    int32_t role;
    int32_t &level;
    uint8_t &tier;
    uint16_t &flags;
    uint8_t marker;
};
struct InventoryEntry {
    int32_t count;
    uint8_t type, subtype;
    int16_t &level;
    uint8_t &rarity;
};
using InventoryVisitor = std::function<void(InventoryEntry)>;
struct Services {
    std::function<void(uint32_t)> seed;
    std::function<void(int32_t)> setup;
    std::function<int16_t &(size_t)> equipmentLevel;
    // Visit outer groups and their entries in storage order, including zero
    // counts. The engine's flat one-unit inventory adapter is separately bounded.
    std::function<void(const InventoryVisitor &)> inventory;
    std::function<int32_t()> draw;
};

// Held alpha-descriptor-activation-refresh.md. Convert the containing cell's
// integer pair, not descriptor position. This lookup never loads a parent.
std::optional<std::pair<int32_t, int32_t>> contextCoordinates(int32_t x, int32_t y);
bool inWindow(int32_t lower, int32_t upper, const std::function<int32_t()> &readClock);
bool inRange(std::span<const int64_t, 3> actor, std::span<const int64_t, 3> descriptor, float radius);

// Optional refresh, before actor construction. False retains the descriptor and
// permits activation to continue. Setup is the supplied nonzero-argument builder;
// its unclosed equipment/stat branches are not certified by this caller.
bool refresh(Descriptor descriptor, const Context *context, const Services &services);

} // namespace creatures::activation
