#include "creatures/VendorStock.h"
#include "creatures/Inventory.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace creatures::vendor_stock {
namespace {
struct Candidate {
    uint8_t type, subtype, material, tier;
    int32_t seed;
};
struct Pool {
    std::array<Candidate, 21> values{};
    size_t size = 0;
    void append(Candidate item) { values[size++] = item; }
    void remove(size_t index) {
        for (size_t next = index + 1; next < size; ++next) values[next - 1] = values[next];
        --size;
    }
};
struct Header { uint8_t type, material; };
constexpr Header ARMOUR[] = {
    {7, 1}, {4, 1}, {5, 1}, {6, 1},
    {7, 25}, {7, 25}, {4, 25}, {5, 25}, {6, 25},
    {7, 26}, {7, 26}, {4, 26}, {5, 26}, {6, 26},
    {7, 27}, {7, 27}, {4, 27}, {5, 27}, {6, 27}
};

int32_t truncate(float value) {
    if (!(value >= -2147483648.f && value < 2147483648.f)) return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
int32_t endpoint(float value) {
    const float difference = 1.f - value;
    const float reciprocal = 1.f / difference;
    const float growth = reciprocal - 1.f;
    const float scaled = growth * 20.f;
    const float shifted = scaled + 1.f;
    return truncate(shifted);
}
int32_t bandLevel(int32_t band, const Draw &draw) {
    const float index = float(band);
    const float lowerInput = index / 30.f;
    const int32_t lower = endpoint(lowerInput);
    const float upperIndex = index + std::bit_cast<float>(0x3f7fff58u);
    const float upperInput = upperIndex / 30.f;
    const int32_t upper = endpoint(upperInput);
    const int32_t divisor = std::bit_cast<int32_t>(uint32_t(upper) - uint32_t(lower) + 1u);
    const int32_t remainder = draw() % divisor;
    const int32_t level = std::max(1, std::bit_cast<int32_t>(uint32_t(lower) + uint32_t(remainder)));
    return band == 0 ? 1 : level;
}

ItemRecord materialize(const Candidate &candidate, int32_t level, bool specialized) {
    ItemRecord item = freshInventoryItem();
    item[0] = candidate.type; item[1] = candidate.subtype;
    std::memcpy(item.data() + 4, &candidate.seed, 4);
    item[0x0c] = candidate.tier;
    item[0x0d] = candidate.material;
    item[0x0e] = specialized ? 1 : 0;
    setItemLevel(item, std::bit_cast<int16_t>(uint16_t(level)));
    return item;
}

Candidate candidate(uint8_t type, uint8_t subtype, uint8_t material,
                    uint8_t suppliedTier, bool specialized, const Draw &draw) {
    if (!specialized) return {type, subtype, material, suppliedTier, draw() % 100};
    draw();
    int32_t tier = draw() % 100 == 0 ? 1 : 0;
    if (draw() % 1000 == 0) ++tier;
    if (draw() % 10000 == 0) ++tier;
    return {type, subtype, material, uint8_t(std::min(tier, 4)), draw()};
}

Pool armourPool(uint8_t tier, bool specialized, const Draw &draw) {
    Pool pool;
    for (const auto header : ARMOUR) pool.append(candidate(header.type, 0, header.material, tier, specialized, draw));
    for (uint8_t type : {uint8_t(8), uint8_t(9)}) {
        const bool odd = draw() % 2 != 0;
        // General selector and specialized body have opposite jewel parity.
        const uint8_t material = specialized ? (odd ? 11 : 12) : (odd ? 12 : 11);
        pool.append(candidate(type, 0, material, tier, specialized, draw));
    }
    return pool;
}

Pool weaponPool(uint8_t tier, bool specialized, const Draw &draw) {
    Pool pool;
    const auto append = [&](uint8_t subtype, uint8_t material) {
        pool.append(candidate(3, subtype, material, tier, specialized, draw));
    };
    append(uint8_t(draw() % 3), 1);
    append(uint8_t(15 + draw() % 3), 1);
    for (uint8_t subtype : {13, 3, 5, 4}) append(subtype, 1);
    append(6, 2);
    for (int discard = 0; discard < (specialized ? 5 : 1); ++discard) draw();
    for (uint8_t subtype : {8, 10, 11}) append(subtype, 2);
    const uint8_t material = draw() % 2 == 0 ? 12 : 11;
    append(12, material);
    return pool;
}

void fixedStock(InventoryValue &inventory, const Draw &draw) {
    const auto insert = [&](uint8_t type, uint8_t subtype, uint8_t material, uint8_t tier = 0) {
        inventory.insert(materialize({type, subtype, material, tier, 0}, 1, false), 0);
    };
    insert(11, 12, 24);
    insert(24, 0, 1);
    for (int tier = 1; tier <= 4; ++tier)
        if (draw() % (tier * 10) == 0) insert(24, 0, 1, uint8_t(tier));
    insert(23, 0, 2);
    insert(23, 1, 2);
    constexpr uint8_t PETS[] = {0x22, 0x23, 0x13, 0x1a, 0x1e, 0x57};
    const int32_t choice = draw() % 6;
    if (choice < 0) throw std::domain_error("Fixed stock pet draw is outside the held ordinary domain");
    insert(20, PETS[choice], 0);
}

void generalBand(InventoryValue &inventory, int32_t level, const Draw &draw) {
    const int32_t count = draw() % 3;
    for (int32_t index = 0; index < count; ++index) {
        int32_t tier = draw() % 2;
        if (draw() % 100 == 0) ++tier;
        if (draw() % 1000 == 0) ++tier;
        if (draw() % 10000 == 0) ++tier;
        const uint8_t suppliedTier = uint8_t(std::min(tier, 4));
        Pool pool = draw() % 2 == 0 ? armourPool(suppliedTier, false, draw) : weaponPool(suppliedTier, false, draw);
        const size_t selected = uint32_t(draw()) % pool.size;
        ItemRecord item = materialize(pool.values[selected], level, false);
        const uint32_t category = item[0];
        std::memcpy(item.data() + 8, &category, 4);
        item[0] = 2;
        inventory.insert(item, 0);
    }
    if (draw() % 2 != 0) inventory.insert(materialize({1, 7, 0, 0, 0}, level, false), 0);
}
} // namespace

void generate(InventoryValue &inventory, Route route, const Draw &draw) {
    for (int32_t band = 0; band < 30; ++band) {
        const int32_t level = bandLevel(band, draw);
        if (route == Route::General) {
            if (band == 0) fixedStock(inventory, draw);
            generalBand(inventory, level, draw);
            continue;
        }
        Pool pool = route == Route::Armour ? armourPool(0, true, draw) : weaponPool(0, true, draw);
        const int32_t count = draw() % 3 + 1;
        for (int32_t index = 0; index < count && pool.size; ++index) {
            const size_t selected = uint32_t(draw()) % pool.size;
            inventory.insert(materialize(pool.values[selected], level, true));
            pool.remove(selected);
        }
    }
}
} // namespace creatures::vendor_stock
