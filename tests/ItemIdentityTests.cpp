#include "creatures/Inventory.h"
#include <bit>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace creatures;
static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static constexpr size_t gaps[] = {2, 3, 0x0f, 0x12, 0x13};
static bool isGap(size_t offset) {
    for (size_t gap : gaps)
        if (offset == gap)
            return true;
    return false;
}
static void countBody(ItemRecord &item, int32_t count) {
    std::memcpy(item.data() + 0x114, &count, 4);
}
static ItemRecord item(uint8_t type, uint8_t subtype = 0, int16_t level = 1, uint8_t padding = 0) {
    ItemRecord result{};
    result[0] = type;
    result[1] = subtype;
    setItemLevel(result, level);
    for (size_t gap : gaps)
        result[gap] = padding;
    return result;
}
static bool hasPadding(const ItemRecord &record, uint8_t padding) {
    for (size_t gap : gaps)
        if (record[gap] != padding)
            return false;
    return true;
}

static void equalityFields() {
    ItemRecord original;
    for (size_t i = 0; i < original.size(); i++)
        original[i] = uint8_t(i * 73 + 11);
    countBody(original, 2);
    check(sameItem(original, original), "a valid item equals itself");
    // Exercise each byte of the wire record, including inactive entries and gaps.
    for (size_t offset = 0; offset < original.size(); offset++) {
        ItemRecord changed = original;
        changed[offset] ^= 0x80;
        const bool header = offset < 0x14 && !isGap(offset);
        const bool activePrefix = offset >= 0x14 && offset < 0x24 && (offset - 0x14) % 8 < 4;
        const bool compared = header || activePrefix || offset >= 0x114;
        if (sameItem(original, changed) != !compared || sameItem(changed, original) != !compared) {
            std::fprintf(stderr, "FAIL: equality byte boundary at %zu\n", offset);
            failures++;
        }
    }
    for (int32_t count : {0, -1, -2, std::numeric_limits<int32_t>::min()}) {
        ItemRecord first = item(19), second = first;
        countBody(first, count);
        countBody(second, count);
        for (size_t i = 0x14; i < 0x114; i++) second[i] = 0xff;
        check(sameItem(first, second), "equal nonpositive raw counts skip the body");
        countBody(second, count == 0 ? -1 : 0);
        check(!sameItem(first, second), "different raw counts cannot become equal through clamping");
    }
    ItemRecord first = item(11), second = first;
    countBody(first, 32);
    countBody(second, 32);
    second[0x113] = 0xff;
    check(sameItem(first, second), "the last entry's trailing bytes are ignored at full capacity");
    second[0x10f] = 1;
    check(!sameItem(first, second), "the last active entry's fourth byte still participates");
    for (int32_t count : {33, std::numeric_limits<int32_t>::max()}) {
        countBody(first, count);
        check(!sameItem(first, first), "malformed positive counts compare unequal under explicit engine policy");
    }
}

static void assignmentFields() {
    ItemRecord source, destination;
    for (size_t i = 0; i < source.size(); i++) source[i] = uint8_t(i * 31 + 7);
    countBody(source, 0);
    destination.fill(0xa6);
    const ItemRecord before = source;
    copyItemFields(source, destination);
    for (size_t i = 0; i < source.size(); i++)
        check(destination[i] == (isGap(i) ? 0xa6 : source[i]),
              "assignment copies all selected fields, including inactive entries, and retains five destination bytes");
    check(source == before, "assignment does not mutate its source");
    const ItemRecord copied = destination;
    copyItemFields(destination, destination);
    check(destination == copied, "selected-field assignment supports self-copy");
}

static void ordinaryReceipt() {
    Inventory inv;
    inv.bags.resize(4);
    ItemRecord herb = item(11, 3, 1, 0x11);
    herb[0x80] = 0xab; // inactive data is copied, but does not affect equality
    inv.bags[2] = {{0, herb}, {10, herb}};
    ItemRecord incoming = herb;
    incoming[0x80] = 0xcd;
    inv.add(incoming);
    check(inv.bags[2][0].count == 1 && inv.bags[2][1].count == 10 && inv.bags[2][0].item == herb,
          "a matching zero-count row wins immediately without replacing ignored bytes");

    const ItemRecord empty = item(0, 0, 1, 0x22);
    inv.bags[2] = {{0, empty}, {-3, herb}};
    incoming = item(11, 3, 5);
    const auto supplied = incoming;
    inv.add(incoming);
    check(inv.bags[2][0].count == 0 && inv.bags[2][0].item == empty && inv.bags[2][1].count == 2,
          "a later equal negative-count row wins over an earlier unmatched empty row");
    check(incoming == supplied && itemLevel(inv.bags[2][1].item) == 1,
          "quantity normalization changes only the local incoming copy");
    inv.bags[2] = {{std::numeric_limits<int32_t>::max(), herb}};
    inv.add(herb);
    check(inv.bags[2][0].count == std::numeric_limits<int32_t>::min(), "ordinary merge retains low32 count wrap");
    inv.add(item(11, 3, -1));
    check(inv.bags[2][0].count == std::numeric_limits<int32_t>::max(), "negative incoming quantities are not clamped");

    inv.bags[2] = {{0, empty}};
    incoming = herb;
    setItemLevel(incoming, 0);
    inv.add(incoming);
    ItemRecord normalized = incoming;
    setItemLevel(normalized, 1);
    check(inv.bags[2][0].count == 0 && sameItem(inv.bags[2][0].item, normalized) &&
          inv.bags[2][0].item[0x80] == 0xab && hasPadding(inv.bags[2][0].item, 0x22),
          "zero quantity still assigns the item, all inactive data and destination padding correctly");
    inv.bags[2].clear();
    inv.add(item(11, 14, 5));
    inv.add(item(11, 14, 5));
    inv.add(item(11, 14, 6));
    check(inv.bags[2].size() == 2 && inv.bags[2][0].count == 2 && itemLevel(inv.bags[2][0].item) == 5 &&
          inv.bags[2][1].count == 1, "ingredient subtype fourteen retains its level and amount one");

    inv.bags[3].clear();
    inv.add(item(20, 2, 2));
    inv.add(item(20, 2, 2));
    check(inv.bags[3].size() == 2 && inv.bags[3][0].count == 2 && inv.bags[3][1].count == 2 &&
          itemLevel(inv.bags[3][0].item) == 1, "quantity normalization does not grant category twenty stacking permission");

    Inventory currency;
    ItemRecord unknown = item(12, 0, -3);
    unknown[0x0d] = 9;
    currency.add(unknown);
    setItemLevel(unknown, 5);
    currency.add(unknown);
    check(currency.money == 0 && currency.bags[1].size() == 1 && currency.bags[1][0].count == 2 &&
          itemLevel(currency.bags[1][0].item) == 1, "unknown category-twelve materials enter ordinary storage and stack");
    ItemRecord gold = item(12, 0, -1);
    gold[0x0d] = 11;
    currency.add(gold);
    ItemRecord silver = item(12, 0, 2);
    silver[0x0d] = 12;
    currency.add(silver);
    currency.add(item(13, 0, -2));
    check(currency.money == uint32_t(-9800) && currency.platinum == uint32_t(-2) && currency.bags[1].size() == 1,
          "known materials and Platinum retain separate wrapped counters without ordinary entries");
}

static void handTransfers() {
    Inventory inv;
    inv.bags.resize(4);
    ItemRecord herb = item(11, 3, 1, 0x11), carried = item(11, 3, 1, 0x22);
    inv.bags[2] = {{0, herb}};
    inv.hand = {-2, carried};
    inv.pressSlot(2, 0);
    carried[0] = carried[1] = 0;
    check(inv.bags[2][0].count == -2 && inv.bags[2][0].item == herb &&
          inv.hand.count == 0 && inv.hand.item == carried,
          "a negative hand merges into a matching zero row and clears only count/category/subtype");
    inv.bags[2][0] = {std::numeric_limits<int32_t>::max(), herb};
    inv.hand = {1, herb};
    inv.pressSlot(2, 0);
    check(inv.bags[2][0].count == std::numeric_limits<int32_t>::min(), "hand merge retains low32 count wrap");

    carried = item(11, 3, 5, 0x22);
    inv.bags[2][0] = {2, herb};
    inv.hand = {3, carried};
    inv.pressSlot(2, 0);
    check(inv.bags[2][0].count == 3 && inv.hand.count == 2 && itemLevel(inv.bags[2][0].item) == 5 &&
          itemLevel(inv.hand.item) == 1 && hasPadding(inv.bags[2][0].item, 0x11) && hasPadding(inv.hand.item, 0x22),
          "hand comparison does not normalize quantity fields; fallback swap retains each destination's padding");
}

static void linkedEquipmentTransfers() {
    Inventory inv;
    inv.bags.resize(4);
    ItemRecord food = item(20, 2, 1, 0x11), handFood = item(20, 2, 1, 0x22);
    inv.equipment[12] = food;
    inv.hand = {2, handFood};
    inv.pressBox(11, 1, 1);
    check(inv.hand.count == 2 && inv.hand.item == handFood && inv.equipment[12] == food && inv.bags[3].empty(),
          "matching equipped food skips one-item replacement and does not consume the hand stack");
    handFood[1] = 3;
    inv.hand = {2, handFood};
    inv.pressBox(11, 1, 1);
    check(inv.hand.count == 1 && itemSubtype(inv.equipment[12]) == 3 && hasPadding(inv.equipment[12], 0x11) &&
          inv.bags[3].size() == 1 && itemSubtype(inv.bags[3][0].item) == 2,
          "unequal food stores the old item, field-copies the new one and takes one hand unit");
    inv.hand = {-2, food};
    inv.pressBox(11, 1, 1);
    check(inv.hand.count == 1 && itemSubtype(inv.hand.item) == 3 && itemSubtype(inv.equipment[12]) == 2 &&
          hasPadding(inv.hand.item, 0x11), "signed hand counts at most one take the equipment swap route");

    inv = Inventory{};
    inv.bags.resize(4);
    inv.bags[3] = {{1, item(19, 4, 1, 0x22)}};
    inv.equipment[12] = item(19, 4, 1, 0x11);
    inv.useSlot(3, 0, false, 1, 1, true);
    check(inv.bags[3].size() == 2 && inv.bags[3][0].count == 0 && itemType(inv.bags[3][0].item) == 0 &&
          inv.bags[3][1].count == 1 && itemType(inv.bags[3][1].item) == 19 &&
          itemType(inv.equipment[12]) == 19 && hasPadding(inv.equipment[12], 0x11),
          "category nineteen bypasses equality and stores the old item before decrementing the selected row");

    inv.bags[3] = {{-1, food}};
    inv.equipment[12] = food;
    inv.useSlot(3, 0, false, 1, 1, true);
    check(inv.bags[3].size() == 1 && inv.bags[3][0].count == -1 && inv.equipment[12] == food,
          "matching category twenty skips replacement even for a negative selected count");
    inv.equipment[12] = handFood;
    inv.bags[3][0].count = 2;
    inv.useSlot(3, 0, false, 1, 1, true);
    check(inv.bags[3].size() == 2 && inv.bags[3][0].count == 1 && itemSubtype(inv.equipment[12]) == 2 &&
          itemSubtype(inv.bags[3][1].item) == 3, "replacement remains correct when storing the old food grows the selected bag");
    inv.bags[3] = {{std::numeric_limits<int32_t>::min(), handFood}};
    inv.equipment[12] = {};
    inv.useSlot(3, 0, false, 1, 1, true);
    check(inv.bags[3][0].count == std::numeric_limits<int32_t>::max() && itemType(inv.bags[3][0].item) == 20 &&
          itemSubtype(inv.equipment[12]) == 3, "a nonzero negative row is admitted and decrement wraps before the signed clear test");
}

int main() {
    equalityFields();
    assignmentFields();
    ordinaryReceipt();
    handTransfers();
    linkedEquipmentTransfers();
    if (!failures) std::puts("ItemIdentityTests passed");
    return failures ? 1 : 0;
}
