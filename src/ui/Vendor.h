#pragma once
#include "creatures/InventoryValue.h"
#include <array>
#include <functional>
#include <list>
#include <utility>

namespace ui::vendor {

using creatures::ItemRecord;
using Buyback = std::list<creatures::CountedItem>; // oldest first, stable live entries

struct Index {
    int32_t group = -1, entry = -1;
    bool operator==(const Index &) const = default;
};

// Logical rows retain zero/negative quantities. Geometry/rendering is supplied
// by the engine; these are the held widget's group, index and scroll rules.
struct Grid {
    int32_t currentGroup = 0;
    int32_t columns = 0, rows = 0, range = 1;
    std::vector<int32_t> scroll;
    Index hover, selected;

    void update(const creatures::InventoryValue *buffer, float width, float height,
                int32_t cellWidth, int32_t cellHeight);
    std::pair<size_t, size_t> visible(const creatures::InventoryValue &buffer) const;
    bool choose(int32_t group, const creatures::InventoryValue &buffer,
                float width, float height, int32_t cellWidth, int32_t cellHeight);
    bool select();
};

// Caller-owned callbacks keep UI, player storage and subsequent interface work
// distinct from the transaction's specified order. Valid storage must remain
// alive through each call; this is not an atomic or rollback interface.
struct Services {
    std::function<int32_t(const ItemRecord &)> price;
    std::function<bool(const ItemRecord &)> canCarry;
    std::function<void(const ItemRecord &)> add;
    std::function<void(int32_t)> feedback;
    std::function<void()> carryLimit, refresh, updatePlayer, continueInterface;
};

const ItemRecord *lookup(const creatures::InventoryValue &buffer, bool active, Index hover);
bool quantityText(int32_t count, uint8_t type);
void refresh(creatures::InventoryValue &buffer, const creatures::InventoryValue *source,
             int32_t currentGroup, const Buyback &buyback);
// Returns whether vendor lookup and, for group 1, live-node mapping succeeded.
bool purchase(const creatures::InventoryValue &buffer, bool active, Index hover,
              int32_t currentGroup, Buyback &buyback, uint32_t &payment, const Services &services);
void sellEntry(int32_t &count, ItemRecord &item, Buyback &buyback,
               uint32_t &payment, const Services &services);
void sellSlot(ItemRecord &item, Buyback &buyback, uint32_t &payment, const Services &services);

// Talk contract §3.6: whether the talk partner at fixed-point position `partner` is cleared,
// its squared distance from `player` in blocks being 25 or more (5 blocks or further).
bool partnerOutOfReach(const std::array<int64_t, 3> &partner, const int64_t (&player)[3]);

} // namespace ui::vendor
