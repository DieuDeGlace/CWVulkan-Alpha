#include "creatures/InventoryValue.h"
#include "creatures/Inventory.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace creatures {

ItemRecord freshInventoryItem() {
    ItemRecord result{};
    setItemLevel(result, 1);
    return result;
}

void InventoryValue::copyFrom(const InventoryValue &source) {
    if (&source == this)
        return;
    groups.resize(source.groups.size());
    for (size_t group = 0; group < source.groups.size(); group++) {
        const auto &from = source.groups[group];
        auto &to = groups[group];
        to.resize(from.size());
        for (size_t index = 0; index < from.size(); index++) {
            to[index].count = from[index].count;
            copyItemFields(from[index].item, to[index].item);
        }
    }
    auxiliary = source.auxiliary;
    copyItemFields(source.separate, separate);
    accumulators[0] = source.accumulators[0];
    accumulators[1] = source.accumulators[1];
}

void InventoryValue::reset(size_t emptyGroups) {
    copyFrom(InventoryValue{});
    groups.resize(emptyGroups);
}

void InventoryValue::insert(const ItemRecord &source, int32_t group) {
    const uint8_t type = itemType(source);
    if (type == 0)
        return;
    if (group == -1)
        group = bagOf(source);
    if (group < 0)
        throw std::invalid_argument("Inventory group must be nonnegative or -1");
    if (size_t(group) >= groups.size())
        groups.resize(size_t(group) + 1);
    ItemRecord item = freshInventoryItem();
    copyItemFields(source, item);
    int32_t increment = 1;
    if (quantityType(item)) {
        increment = itemLevel(item);
        setItemLevel(item, 1);
    }
    if (type == 12) {
        uint32_t scale = 0;
        switch (source[0x0d]) {
        case 10: scale = 1; break;
        case 11: scale = 10000; break;
        case 12: scale = 100; break;
        default: break;
        }
        if (scale) {
            accumulators[0] += uint32_t(increment) * scale;
            return;
        }
    } else if (type == 13) {
        accumulators[1] += uint32_t(increment);
        return;
    }
    auto &entries = groups[size_t(group)];
    size_t vacant = entries.size();
    for (size_t index = 0; index < entries.size(); index++) {
        auto &entry = entries[index];
        if (entry.count == 0 && vacant == entries.size())
            vacant = index;
        if (stackable(type) && sameItem(entry.item, item)) {
            entry.count = std::bit_cast<int32_t>(uint32_t(entry.count) + uint32_t(increment));
            return;
        }
    }
    if (vacant == entries.size())
        entries.emplace_back();
    auto &entry = entries[vacant];
    copyItemFields(item, entry.item);
    entry.count = increment;
}

void InventoryValue::replaceTen(const std::function<ItemRecord(size_t)> &generate) {
    groups.clear();
    groups.resize(1);
    groups[0].resize(10);
    for (size_t index = 0; index < 10; index++) {
        auto &entry = groups[0][index];
        entry.count = 1;
        const ItemRecord item = generate(index);
        copyItemFields(item, entry.item);
    }
}

void InventoryValue::appendUnit(const ItemRecord &source, size_t group) {
    CountedItem entry;
    entry.count = 1;
    copyItemFields(source, entry.item);
    if (group >= groups.size())
        groups.resize(group + 1);
    groups[group].push_back(std::move(entry));
}

CountedItem *InventoryValue::firstHealthItem() {
    for (auto &group : groups)
        for (auto &entry : group)
            if (entry.count != 0 && entry.item[0] == 1 && entry.item[1] == 1)
                return &entry;
    return nullptr;
}

std::vector<CountedItem> InventoryValue::entries() const {
    size_t count = 0;
    for (const auto &group : groups) count += group.size();
    std::vector<CountedItem> result;
    result.reserve(count);
    for (const auto &group : groups)
        result.insert(result.end(), group.begin(), group.end());
    return result;
}

std::vector<ItemRecord> InventoryValue::itemValues() const {
    size_t count = 0;
    for (const auto &group : groups) count += group.size();
    std::vector<ItemRecord> result;
    result.reserve(count);
    for (const auto &group : groups)
        for (const auto &entry : group)
            result.push_back(entry.item);
    return result;
}

} // namespace creatures
