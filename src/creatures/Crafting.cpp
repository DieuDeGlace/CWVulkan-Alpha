#include "creatures/Crafting.h"
#include <algorithm>
#include <cstring>

namespace creatures {

int32_t stationKind(int32_t station) {
    switch (station) {
    case FURNACE: return 71;
    case ANVIL: return 72;
    case SPINNING_WHEEL_STATION: return 73;
    case LOOM_STATION: return 74;
    case SAW: return 75;
    case WORKBENCH_STATION: return 76;
    case CAMPFIRE_STATION: return 65;
    default: return 0;
    }
}

const char *stationText(int32_t station) {
    switch (station) {
    case WATER: return "Requires water";
    case FURNACE: return "Requires Furnace";
    case ANVIL: return "Requires Anvil";
    case SPINNING_WHEEL_STATION: return "Requires Spinning Wheel";
    case LOOM_STATION: return "Requires Loom";
    case SAW: return "Requires Saw";
    case WORKBENCH_STATION: return "Requires Workbench";
    case CAMPFIRE_STATION: return "Requires Campfire";
    default: return "";
    }
}

namespace {

ItemRecord ingredient(uint8_t subtype, uint8_t material, uint8_t rarity = 0) {
    ItemRecord r{};
    r[0x00] = 11, r[0x01] = subtype, r[0x0c] = rarity, r[0x0d] = material;
    setItemLevel(r, 1);
    return r;
}

ItemRecord key(uint8_t type, uint8_t subtype, uint8_t material, int16_t level, uint32_t seed = 0) {
    ItemRecord r{};
    r[0x00] = type, r[0x01] = subtype, r[0x0d] = material;
    std::memcpy(r.data() + 0x04, &seed, 4);
    setItemLevel(r, level);
    return r;
}

} // namespace

Recipe buildRecipe(const ItemRecord &k) {
    Recipe r;
    r.product = k;
    const uint8_t type = itemType(k), sub = itemSubtype(k), m = itemMaterial(k);
    const auto need = [&](ItemRecord item, int32_t count) { r.ingredients.push_back({item, count}); };
    const ItemRecord iron = ingredient(10, 1), wood = ingredient(10, 2);
    switch (type) {
    case 1:
        switch (sub) {
        case 1: need(ingredient(22, 0), 1), need(ingredient(26, 0), 1); break;
        case 2: need(ingredient(23, 0), 1), need(ingredient(12, 24), 1); break;
        case 4: need(ingredient(20, 0), 1), r.station = CAMPFIRE_STATION; break;
        case 5: need(ingredient(27, 0), 1); break;
        case 6: need(ingredient(15, 0), 1), need(ingredient(21, 0), 1), r.station = CAMPFIRE_STATION; break;
        case 8: need(ingredient(17, 0), 1); break;
        case 9: need(ingredient(16, 0), 1), r.station = CAMPFIRE_STATION; break;
        default: break;
        }
        break;
    case 11:
        if (sub == 9 && m == 25)
            need(ingredient(6, 0), 1), r.station = SPINNING_WHEEL_STATION;
        else if (sub == 9 && m == 26)
            need(ingredient(5, 21), 1), r.station = SPINNING_WHEEL_STATION;
        else if (sub == 9 && m == 27)
            need(ingredient(11, 27), 1), r.station = SPINNING_WHEEL_STATION;
        else if (sub == 10 && m == 2)
            need(ingredient(1, 2), 1), r.station = SAW;
        else if (sub == 10)
            need(ingredient(0, m), 1), r.station = FURNACE;
        else if (sub == 22)
            need(ingredient(24, 0), 1), r.station = CAMPFIRE_STATION;
        else if (sub == 26)
            need(ingredient(12, 24), 1), r.station = WATER;
        break;
    case 3:
        switch (sub) {
        case 0: case 1: case 2: case 3: case 13: need(iron, 8), r.station = ANVIL; break;
        case 4: need(iron, 6), need(ingredient(9, 27), 2), r.station = ANVIL; break;
        case 5: case 15: case 16: case 17: need(iron, 16), r.station = ANVIL; break;
        case 6: need(wood, 15), need(ingredient(9, 26), 1), r.station = WORKBENCH_STATION; break;
        case 7: need(iron, 6), need(wood, 9), need(ingredient(9, 26), 1), r.station = WORKBENCH_STATION; break;
        case 8: case 10: case 11: need(wood, 16), r.station = WORKBENCH_STATION; break;
        case 12: need(ingredient(10, m), 8), r.station = ANVIL; break;
        default: break;
        }
        break;
    case 4: case 5: case 6: case 7: {
        const int32_t count = type == 4 ? 20 : type == 5 ? 6 : 8;
        if (m == 1)
            need(iron, count), r.station = ANVIL;
        else
            need(ingredient(9, m), count), r.station = LOOM_STATION;
        break;
    }
    case 8: need(ingredient(10, m), 6); break;
    case 9: need(ingredient(10, m), 3); break;
    default: break;
    }
    // The rarity surcharge: two gem nuggets of the key's rarity.
    if (const uint8_t rarity = itemRarity(k); rarity >= 1 && rarity <= 4)
        need(ingredient(0, (uint8_t)(12 + rarity), rarity), 2);
    return r;
}

int recipeTab(const ItemRecord &k) {
    switch (itemType(k)) {
    case 3: return 0;
    case 4: case 5: case 6: case 7: return 1;
    case 8: case 9: return 2;
    case 1: return itemSubtype(k) == 1 || itemSubtype(k) == 2 ? 4 : 3;
    case 11: return 5;
    default: return -1;
    }
}

std::array<std::vector<Recipe>, 6> recipeTabs(const std::vector<ItemRecord> &known) {
    std::array<std::vector<Recipe>, 6> tabs;
    for (const ItemRecord &k : known)
        if (const int t = recipeTab(k); t >= 0)
            tabs[t].push_back(buildRecipe(k));
    for (auto &list : tabs)
        std::stable_sort(list.begin(), list.end(), [](const Recipe &a, const Recipe &b) {
            const auto weight = [](const Recipe &r) { return 6 * itemLevel(r.product) + itemRarity(r.product); };
            return weight(a) > weight(b);
        });
    return tabs;
}

int32_t holding(const Inventory &inventory, const ItemRecord &item) {
    int32_t n = 0;
    for (const auto &bag : inventory.bags)
        for (const Stack &s : bag)
            if (!s.empty() && sameItem(s.item, item))
                n += s.count;
    return n;
}

int32_t timesCraftable(const Inventory &inventory, const Recipe &recipe) {
    if (recipe.ingredients.empty())
        return 0;
    int32_t times = INT32_MAX;
    for (const auto &[item, count] : recipe.ingredients)
        times = std::min(times, holding(inventory, item) / std::max(count, 1));
    return times;
}

void grantLevel(std::vector<ItemRecord> &known, int32_t L, int32_t cls, const std::function<int()> &draw) {
    const auto grant = [&](const ItemRecord &k) {
        if (std::none_of(known.begin(), known.end(), [&](const ItemRecord &o) { return sameItem(o, k); }))
            known.push_back(k);
    };
    const int16_t level = (int16_t)L;
    if (L % 4 == 1)
        grant(key(1, 1, 0, level));
    if (L % 4 == 3)
        grant(key(1, 2, 0, level));
    switch (L % 5) {
    case 1: grant(key(1, 4, 0, level)), grant(key(1, 8, 0, level)); break;
    case 2: grant(key(1, 9, 0, level)); break;
    case 3: grant(key(1, 6, 0, level)); break;
    case 4: grant(key(1, 5, 0, level)); break;
    default: break;
    }
    if (L == 1) {
        grant(key(11, 26, 0, 1));
        grant(key(11, 22, 0, 1));
        if (cls == 4)
            grant(key(11, 9, 27, 1));
        else if (cls == 2)
            grant(key(11, 9, 26, 1));
        else if (cls == 3)
            grant(key(11, 9, 25, 1));
        for (uint8_t m : {1, 2, 11, 12})
            grant(key(11, 10, m, 1));
    }
    // Every level: an armour piece and a weapon of the class, each with a seed of rand() mod 11.
    static const uint8_t ARMOUR_TYPE[4] = {7, 4, 6, 5};
    const uint8_t armourMaterial = cls == 1 ? 1 : cls == 2 ? 26 : cls == 3 ? 25 : cls == 4 ? 27 : 12;
    grant(key(ARMOUR_TYPE[L % 4], 0, armourMaterial, level, (uint32_t)(draw() % 11)));
    const uint32_t seed = (uint32_t)(draw() % 11);
    uint8_t sub = 0, material = 1;
    switch (cls) {
    case 1:
        sub = L % 3 == 0 ? (uint8_t)(15 + draw() % 3) : L % 3 == 1 ? (uint8_t)(draw() % 3) : 13;
        break;
    case 2:
        material = 2, sub = L % 3 == 0 ? 6 : L % 3 == 1 ? 7 : 8;
        break;
    case 3:
        material = 2, sub = L % 3 == 0 ? 10 : L % 3 == 1 ? 11 : 12;
        if (sub == 12)
            material = draw() % 2 ? 12 : 11;
        break;
    case 4:
        sub = L % 3 == 0 ? 3 : L % 3 == 1 ? 5 : 4;
        break;
    default:
        return;
    }
    grant(key(3, sub, material, level, seed));
}

CraftOutcome craft(Inventory &inventory, const Recipe &recipe, const std::function<int()> &draw) {
    CraftOutcome out;
    ItemRecord product = recipe.product;
    if (!inventory.canCarry(product)) {
        out.refused = true;
        return out;
    }
    // Step 2, as the original does it: a stack of at least the need loses the need and the
    // walk moves to the next bag; a smaller stack is emptied without lowering the need.
    for (const auto &[item, count] : recipe.ingredients)
        for (auto &bag : inventory.bags)
            for (Stack &s : bag) {
                if (s.empty() || !sameItem(s.item, item))
                    continue;
                if (count < s.count) {
                    s.count -= count;
                    break; // the next bag
                }
                s.count = 0;
                s.item[0x00] = s.item[0x01] = 0;
            }
    if (itemType(product) >= 3 && itemType(product) <= 9)
        product[0x0e] |= 1; // crafted equipment counts as adapted
    out.sounds.push_back(0x59);
    if (itemType(product) == 11 && itemSubtype(product) != 26 && draw() % 5 == 0) {
        int16_t extra = 1;
        if (draw() % 5 == 0)
            extra = draw() % 5 == 0 ? 3 : 2;
        setItemLevel(product, (int16_t)(itemLevel(product) + extra));
        out.sounds.push_back(0x5a);
    }
    inventory.add(product);
    out.made = product;
    return out;
}

} // namespace creatures
