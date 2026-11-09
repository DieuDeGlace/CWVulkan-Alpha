// Alpha's random equipment generators (docs/specs/alpha-items-generator.md).
#include "worldgen/Items.h"
#include <algorithm>
#include <vector>

namespace worldgen {
namespace {

// §0: the pick, one draw among n candidates.
ItemRecord pick(Random &random, const std::vector<ItemRecord> &candidates) {
    const uint32_t d = (uint32_t)random.draw();
    return candidates.empty() ? ItemRecord{} : candidates[d % candidates.size()];
}

// §0: the rarity roll, always 4 draws.
uint8_t rollRarity(Random &random, int32_t m) {
    const int32_t d = random.draw();
    int32_t q = m > 0 ? d % m : 0;
    if (random.draw() % 100 == 0)
        q++;
    if (random.draw() % 1000 == 0)
        q++;
    if (random.draw() % 10000 == 0)
        q++;
    return (uint8_t)std::min(q, 4);
}

// The building blocks' push: a fresh seed draw, then a copy of the working record.
void pushSeeded(Random &random, ItemRecord &work, std::vector<ItemRecord> &list) {
    work.seed = (uint32_t)(random.draw() % 100);
    list.push_back(work);
}

} // namespace

ItemRecord armourItem(Random &random, int32_t level, int32_t rarity, int32_t c) {
    ItemRecord work;
    work.level = (int16_t)level;
    work.rarity = (uint8_t)rarity;
    std::vector<ItemRecord> list;
    auto set = [&](uint8_t material, std::initializer_list<uint8_t> types) {
        work.material = material;
        for (uint8_t t : types) {
            work.type = t;
            pushSeeded(random, work, list);
        }
    };
    if (c == 1 || c < 0)
        set(1, {7, 4, 5, 6});
    if (c == 3 || c < 0)
        set(25, {7, 7, 4, 5, 6});
    if (c == 2 || c < 0)
        set(26, {7, 7, 4, 5, 6});
    if (c == 4 || c < 0)
        set(27, {7, 7, 4, 5, 6});
    for (uint8_t t : {8, 9}) { // amulet, ring
        work.material = (random.draw() & 1) ? 12 : 11;
        work.type = t;
        pushSeeded(random, work, list);
    }
    return pick(random, list);
}

ItemRecord weaponItem(Random &random, int32_t level, int32_t rarity, int32_t c) {
    ItemRecord work;
    work.type = 3;
    work.material = 1;
    work.level = (int16_t)level;
    work.rarity = (uint8_t)rarity;
    std::vector<ItemRecord> list;
    auto push = [&](uint8_t subtype) {
        work.subtype = subtype;
        pushSeeded(random, work, list);
    };
    if (c == 1 || c < 0) {
        push((uint8_t)(random.draw() % 3));
        push((uint8_t)(random.draw() % 3 + 15));
        push(13);
    }
    if (c == 4 || c < 0)
        for (uint8_t s : {3, 5, 4})
            push(s);
    work.material = 2;
    if (c == 2 || c < 0) {
        push(6);
        random.draw(); // (its value is not used)
        push(8);
    }
    if (c == 3 || c < 0) {
        push(10);
        push(11);
        work.material = (random.draw() & 1) ? 11 : 12;
        push(12);
    }
    return pick(random, list);
}

ItemRecord recipeItem(Random &random, int32_t level, int32_t rarity) {
    ItemRecord item = (random.draw() & 1) ? weaponItem(random, level, rarity, -1) : armourItem(random, level, rarity, -1);
    item.product = item.type;
    item.type = 2;
    return item;
}

ItemRecord lootItem(Random &random, int32_t level, int32_t rarityCap) {
    const int32_t m = rarityCap + 1;
    ItemRecord work;
    work.level = (int16_t)level;
    std::vector<ItemRecord> list;
    for (int i = 0; i < 4; i++)
        random.draw(); // (their values are not used)
    work.type = 3;
    work.material = 1;
    work.subtype = (uint8_t)(random.draw() % 3);
    work.rarity = rollRarity(random, m);
    list.push_back(work);
    for (int i = 0; i < 5; i++)
        random.draw(); // (their values are not used)
    struct Fixed {
        uint8_t type, subtype, material;
    };
    static const Fixed FIXED[] = {
        {3, 13, 1}, {3, 6, 2}, {3, 7, 2}, {3, 10, 2}, {3, 11, 2}, {7, 0, 1}, {4, 0, 1}, {5, 0, 1}, {6, 0, 1},
        {7, 0, 25}, {7, 0, 25}, {4, 0, 25}, {5, 0, 25}, {6, 0, 25},
        {7, 0, 26}, {7, 0, 26}, {4, 0, 26}, {5, 0, 26}, {6, 0, 26},
        {7, 0, 27}, {7, 0, 27}, {4, 0, 27}, {5, 0, 27}, {6, 0, 27},
    };
    for (const Fixed &f : FIXED) {
        work.type = f.type, work.subtype = f.subtype, work.material = f.material;
        work.rarity = rollRarity(random, m);
        list.push_back(work);
    }
    list.push_back(recipeItem(random, level, rarityCap));
    return pick(random, list);
}

} // namespace worldgen
