#include "creatures/VendorStock.h"
#include "creatures/Inventory.h"
#include <cstdio>
#include <cstring>
#include <map>

using namespace creatures;
using namespace creatures::vendor_stock;
static int failures = 0;
static void check(bool ok, const char *message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static uint32_t word(const ItemRecord &item, size_t offset) {
    uint32_t result; std::memcpy(&result, item.data() + offset, 4); return result;
}
static bool zeroPayload(const ItemRecord &item) {
    for (size_t offset = 0x14; offset < item.size(); ++offset) if (item[offset]) return false;
    return true;
}
struct Script {
    size_t calls = 0;
    int32_t fallback = 0;
    std::map<size_t, int32_t> values;
    int32_t draw() {
        const auto found = values.find(calls++);
        return found == values.end() ? fallback : found->second;
    }
};

static void fixedAndRetention() {
    InventoryValue value;
    value.auxiliary = 0x12345678;
    value.accumulators = {0x80000001, 0xffffffff};
    value.separate[2] = 0x66;
    Script script;
    generate(value, Route::General, [&] { return script.draw(); });
    const auto &rows = value.groups[0];
    check(script.calls == 95 && value.groups.size() == 1 && rows.size() == 9,
          "general zero stream retains all four optional checks, pet draw and three draws per band");
    check(rows[0].item[0] == 11 && rows[0].item[1] == 12 && rows[0].item[0x0d] == 24 &&
          rows[6].item[0] == 23 && rows[6].item[1] == 0 && rows[7].item[1] == 1 && rows[8].item[0] == 20 && rows[8].item[1] == 0x22,
          "band-zero fixed items retain insertion order and numeric selectors");
    for (size_t i = 1; i < 6; ++i)
        check(rows[i].item[0] == 24 && rows[i].item[0x0c] == i - 1 && rows[i].item[0x0d] == 1,
              "optional fixed tiers advance in order without leaking into later items");
    for (const auto &entry : rows)
        check(entry.count == 1 && itemLevel(entry.item) == 1 && word(entry.item, 4) == 0 && word(entry.item, 8) == 0 &&
              entry.item[0x0e] == 0 && zeroPayload(entry.item), "fixed entries have semantic zero payload and direct unit contribution");
    generate(value, Route::General, [&] { return script.draw(); });
    check(script.calls == 190 && value.groups[0].size() == 17 && value.groups[0][0].count == 2 &&
          value.auxiliary == 0x12345678 && value.accumulators[0] == 0x80000001 && value.accumulators[1] == 0xffffffff && value.separate[2] == 0x66,
          "repeated generation inserts without clearing, merges only eligible items and preserves other inventory members");

    InventoryValue existing;
    existing.groups.resize(3);
    CountedItem vacancy; vacancy.item.fill(0xa5);
    existing.groups[0].push_back(vacancy);
    existing.groups[2].push_back({-7, freshInventoryItem()});
    generate(existing, Route::General, [] { return 0; });
    check(existing.groups.size() == 3 && existing.groups[2][0].count == -7 && existing.groups[0][0].item[2] == 0xa5 &&
          existing.groups[0][0].item[3] == 0xa5 && zeroPayload(existing.groups[0][0].item),
          "first-zero reuse preserves destination gaps while copying every cleared payload cell");
}

static void generalSelectors() {
    InventoryValue weapons;
    Script ones; ones.fallback = 1;
    generate(weapons, Route::General, [&] { return ones.draw(); });
    size_t recipes = 0, potions = 0;
    for (const auto &entry : weapons.groups[0]) {
        if (entry.item[0] == 2) {
            ++recipes;
            check(word(entry.item, 8) == 3 && entry.item[1] == 16 && entry.item[0x0c] == 1 &&
                  entry.item[0x0d] == 1 && word(entry.item, 4) == 1 && entry.item[0x0e] == 0 && entry.count == 1 && zeroPayload(entry.item),
                  "general weapon selector preserves subtype/seed/tier/material and records original category before recategorizing");
        } else if (entry.item[0] == 1) { potions += size_t(entry.count); check(entry.item[1] == 7, "final odd draw inserts category-one subtype seven"); }
    }
    check(ones.calls == 725 && recipes == 30 && potions == 30,
          "general weapon path consumes its discarded candidate draw and complete sixteen-draw selector per band");

    // One first-band general armour selection; the remaining stream is zero.
    Script armour;
    armour.values = {{6, 1}, {32, 77}, {35, 19}};
    InventoryValue jewellery;
    generate(jewellery, Route::General, [&] { return armour.draw(); });
    check(armour.calls == 124 && jewellery.groups[0].size() == 10 && jewellery.groups[0][9].item[0] == 2 &&
          word(jewellery.groups[0][9].item, 8) == 8 && jewellery.groups[0][9].item[0x0d] == 11 &&
          word(jewellery.groups[0][9].item, 4) == 77 && jewellery.groups[0][9].item[0x0c] == 3,
          "general armour selector spends all twenty-four draws and uses even-material eleven for its selected jewel");
    armour = {};
    armour.values = {{6, 1}, {33, 1}, {35, 20}};
    jewellery = {};
    generate(jewellery, Route::General, [&] { return armour.draw(); });
    check(word(jewellery.groups[0][9].item, 8) == 9 && jewellery.groups[0][9].item[0x0d] == 12,
          "general second jewel material follows its own parity draw");
}

static void specializedPools() {
    for (const auto route : {Route::Armour, Route::Weapons}) {
        InventoryValue value;
        Script zero;
        generate(value, route, [&] { return zero.draw(); });
        check(zero.calls == (route == Route::Armour ? 3300u : 1980u) && value.groups.size() == 1 && value.groups[0].size() == 30,
              "specialized body constructs every candidate before one selection in each of thirty bands");
        for (const auto &entry : value.groups[0])
            check(entry.count == 1 && entry.item[0] == (route == Route::Armour ? 7 : 3) && entry.item[0x0c] == 3 &&
                  entry.item[0x0e] == 1 && word(entry.item, 8) == 0 && zeroPayload(entry.item),
                  "specialized stock retains category, unit quantity, three tier successes, byte0e and zero auxiliary/payload");
        Script ones; ones.fallback = 1;
        InventoryValue picks;
        generate(picks, route, [&] { return ones.draw(); });
        check(ones.calls == (route == Route::Armour ? 3330u : 2010u) && picks.groups[0].size() == 60,
              "two specialized picks use a fresh reduced pool without changing candidate construction draws");
        check(picks.groups[0][0].item[route == Route::Armour ? 0 : 1] == (route == Route::Armour ? 4 : 16) &&
              picks.groups[0][1].item[route == Route::Armour ? 0 : 1] == (route == Route::Armour ? 5 : 13),
              "stable removal shifts later candidates before the next identical ordinal choice");
    }
    Script jewellery; jewellery.values = {{109, 19}};
    InventoryValue value;
    generate(value, Route::Armour, [&] { return jewellery.draw(); });
    check(value.groups[0][0].item[0] == 8 && value.groups[0][0].item[0x0d] == 12,
          "specialized armour uses even-material twelve, independently of the general selector's opposite parity");
    Script discarded;
    discarded.values = {{38, 91}, {39, 92}, {40, 93}, {41, 94}, {42, 95}, {43, -5}, {47, 0x12345678}, {65, 7}};
    value = {};
    generate(value, Route::Weapons, [&] { return discarded.draw(); });
    check(discarded.calls == 1980 && value.groups[0][0].item[1] == 8 && value.groups[0][0].item[0x0d] == 2 &&
          word(value.groups[0][0].item, 4) == 0x12345678,
          "specialized weapon sequence retains five extra discards plus the candidate's own discard and stores the full random word");
    Script unsignedChoice; unsignedChoice.values = {{65, -1}};
    value = {};
    generate(value, Route::Weapons, [&] { return unsignedChoice.draw(); });
    check(value.groups[0][0].item[1] == 3, "selection divides the complete unsigned draw by the current pool count");
    Script high; high.fallback = 32767;
    value = {};
    generate(value, Route::Weapons, [&] { return high.draw(); });
    check(value.groups[0].size() == 60 && uint16_t(itemLevel(value.groups[0].back().item)) == 0x8243 &&
          itemLevel(value.groups[0].back().item) < 0 && itemLevel(value.groups[0].front().item) == 1,
          "band zero consumes its overridden draw and the highest band keeps rounded endpoints and the raw unsaturated low word");
}

int main() {
    fixedAndRetention(); generalSelectors(); specializedPools();
    if (!failures) std::puts("PASS: general and specialized stock, exact draw streams, semantic items, raw levels and stable selection");
    return failures ? 1 : 0;
}
