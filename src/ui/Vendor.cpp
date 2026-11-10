#include "ui/Vendor.h"
#include "creatures/Inventory.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace ui::vendor {
namespace {
int32_t signedWord(uint32_t word) { return std::bit_cast<int32_t>(word); }

void finish(const Services &s) {
    if (s.updatePlayer) s.updatePlayer();
    if (s.continueInterface) s.continueInterface();
}

void refuse(const Services &s) {
    if (s.carryLimit) s.carryLimit();
    if (s.feedback) s.feedback(0x31);
}

void publish(const ItemRecord &item, Buyback &buyback, const Services &s) {
    creatures::CountedItem entry;
    entry.count = 1;
    creatures::copyItemFields(item, entry.item);
    buyback.push_back(std::move(entry));
    if (buyback.size() > 10) buyback.pop_front();
    if (s.refresh) s.refresh();
}
} // namespace

void Grid::update(const creatures::InventoryValue *buffer, float width, float height,
                  int32_t cellWidth, int32_t cellHeight) {
    if (!buffer) return;
    // Invalid dimensions have no native normalized fallback. Reject them at
    // this engine API rather than supplying a different native layout.
    if (cellWidth <= -5 || cellHeight <= -5 || cellWidth > std::numeric_limits<int32_t>::max() - 5 ||
        cellHeight > std::numeric_limits<int32_t>::max() - 5)
        throw std::invalid_argument("Invalid inventory cell dimensions");
    const float c = (width - 10.f) / float(cellWidth + 5);
    const float r = (height - 40.f) / float(cellHeight + 5);
    if (!(c >= 1.f && c < 2147483648.f && r >= 1.f && r < 2147483648.f))
        throw std::invalid_argument("Inventory layout requires positive columns and rows");
    columns = int32_t(c);
    rows = int32_t(r);
    scroll.resize(buffer->groups.size(), 0);
    if (currentGroup < 0) throw std::invalid_argument("Negative inventory group");
    if (size_t(currentGroup) >= buffer->groups.size()) {
        range = 1;
        return;
    }
    const size_t n = buffer->groups[size_t(currentGroup)].size();
    auto &position = scroll[size_t(currentGroup)];
    if (!n) {
        range = position = 0;
        return;
    }
    if (n > std::numeric_limits<uint32_t>::max())
        throw std::length_error("Inventory row count exceeds the held width");
    const uint32_t value = (uint32_t(n) - 1u) / uint32_t(columns) - uint32_t(rows) + 2u;
    range = std::max(1, signedWord(value));
    if (position > range - 1) position = range - 1;
}

std::pair<size_t, size_t> Grid::visible(const creatures::InventoryValue &buffer) const {
    if (currentGroup < 0 || size_t(currentGroup) >= buffer.groups.size() ||
        size_t(currentGroup) >= scroll.size() || scroll[size_t(currentGroup)] < 0 || columns <= 0 || rows <= 0)
        return {}; // invalid storage/negative scroll is outside the native ordinary row pass
    const size_t n = buffer.groups[size_t(currentGroup)].size();
    const size_t first = std::min(n, size_t(scroll[size_t(currentGroup)]) * size_t(columns));
    return {first, std::min(n, first + size_t(rows) * size_t(columns))};
}

bool Grid::choose(int32_t group, const creatures::InventoryValue &buffer,
                  float width, float height, int32_t cellWidth, int32_t cellHeight) {
    if (group < 0 || group == currentGroup) return false;
    currentGroup = group;
    update(&buffer, width, height, cellWidth, cellHeight);
    const auto [first, end] = visible(buffer);
    selected = first == end ? Index{} : Index{group, int32_t(first)};
    return true;
}

bool Grid::select() {
    if (hover.entry < 0) return false;
    selected = hover;
    return true;
}

const ItemRecord *lookup(const creatures::InventoryValue &buffer, bool active, Index hover) {
    if (!active || hover.group < 0 || hover.entry < 0 || size_t(hover.group) >= buffer.groups.size()) return nullptr;
    const auto &group = buffer.groups[size_t(hover.group)];
    return size_t(hover.entry) < group.size() ? &group[size_t(hover.entry)].item : nullptr;
}

bool quantityText(int32_t count, uint8_t type) {
    if (count <= 0) return false;
    switch (type) {
    case 1: case 10: case 12: case 13: case 11: case 21: return true;
    default: return false;
    }
}

void refresh(creatures::InventoryValue &buffer, const creatures::InventoryValue *source,
             int32_t currentGroup, const Buyback &buyback) {
    if (currentGroup != 1) {
        if (source) buffer.copyFrom(*source);
        return;
    }
    buffer.groups.resize(2);
    auto &rows = buffer.groups[1];
    rows.clear();
    rows.reserve(buyback.size());
    for (auto it = buyback.rbegin(); it != buyback.rend(); ++it) {
        rows.emplace_back();
        rows.back().count = it->count;
        creatures::copyItemFields(it->item, rows.back().item);
    }
}

bool purchase(const creatures::InventoryValue &buffer, bool active, Index hover,
              int32_t currentGroup, Buyback &buyback, uint32_t &payment, const Services &s) {
    const ItemRecord *displayed = lookup(buffer, active, hover);
    if (!displayed) return false;
    if (currentGroup != 1) {
        if (s.price(*displayed) > signedWord(payment)) return true;
        if (!s.canCarry(*displayed)) {
            refuse(s);
            return true;
        }
        s.add(*displayed);
        payment -= uint32_t(s.price(*displayed));
        if (s.feedback) s.feedback(0x39);
        finish(s);
        return true;
    }
    auto live = buyback.begin();
    int32_t index = signedWord(uint32_t(buyback.size()) - 1u);
    while (live != buyback.end() && index != hover.entry) { ++live; --index; }
    if (live == buyback.end()) return false;
    int32_t attempted = 0, successful = 0;
    while (attempted < live->count) {
        if (!s.canCarry(live->item)) { refuse(s); break; }
        if (s.price(*displayed) <= signedWord(payment)) {
            s.add(live->item);
            payment -= uint32_t(s.price(live->item));
            successful = signedWord(uint32_t(successful) + 1u);
        }
        attempted = signedWord(uint32_t(attempted) + 1u);
    }
    live->count = signedWord(uint32_t(live->count) - uint32_t(successful));
    if (live->count == 0) buyback.erase(live);
    if (s.feedback) s.feedback(0x39);
    if (s.refresh) s.refresh();
    finish(s);
    return true;
}

void sellEntry(int32_t &count, ItemRecord &item, Buyback &buyback,
               uint32_t &payment, const Services &s) {
    if (count == 0) return;
    ItemRecord saved = creatures::freshInventoryItem();
    creatures::copyItemFields(item, saved);
    count = signedWord(uint32_t(count) - 1u);
    if (count <= 0) { count = 0; item[0] = item[1] = 0; }
    payment += uint32_t(s.price(saved));
    if (s.feedback) s.feedback(0x3a);
    publish(saved, buyback, s);
}

void sellSlot(ItemRecord &item, Buyback &buyback, uint32_t &payment, const Services &s) {
    if (!item[0]) return;
    payment += uint32_t(s.price(item));
    if (s.feedback) s.feedback(0x3a);
    publish(item, buyback, s);
    item[0] = item[1] = 0;
}
} // namespace ui::vendor
