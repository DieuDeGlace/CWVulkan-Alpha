#include "Loot.h"
#include "worldgen/Items.h"
#include <cmath>
#include <cstring>

const uint8_t TAMEABLE[45] = {35, 87, 60, 55, 34, 23, 22, 30, 33,  98, 25,  53, 67, 102, 104,
                              105, 19, 40, 37, 38, 39, 92, 93, 56, 90, 91,  50, 27, 75,  26,
                              86, 99, 106, 63, 66, 64, 65, 74, 36, 57, 58,  59, 61, 62,  88};

int32_t rarityRoll(worldgen::Random &random, int32_t m, bool force) {
    int32_t q = random.draw() % (m + 1);
    if (random.draw() % 100 == 0)
        q++;
    if (random.draw() % 1000 == 0)
        q++;
    if (random.draw() % 10000 == 0)
        q++;
    if (force)
        q = m + 1;
    return std::min(q, 4);
}

int16_t rerollLevel(worldgen::Random &random, int16_t level, bool widen) {
    if (level < 2)
        return level;
    const auto curve = [](float x) { return 1.f - 1.f / ((x - 1.f) * 0.05f + 1.f); };
    const auto inverse = [](float y) { return (1.f / (1.f - y) - 1.f) * 20.f + 1.f; };
    const float k = curve((float)level);
    if (k - 0.05f <= 0)
        return 1;
    const int32_t lo = std::max(1, (int32_t)inverse(k - 0.05f));
    const int32_t hi = widen ? (int32_t)inverse(std::min(k + 0.05f, 0.9999f)) : level;
    return (int16_t)(lo + random.draw() % (hi - lo + 1));
}

namespace {

creatures::ItemRecord fresh(uint8_t type, uint8_t subtype, uint8_t material = 0, uint8_t rarity = 0) {
    creatures::ItemRecord r{};
    r[0x00] = type, r[0x01] = subtype, r[0x0c] = rarity, r[0x0d] = material;
    creatures::setItemLevel(r, 1);
    return r;
}

} // namespace

int32_t identificationFee(const creatures::ItemRecord &item) {
    if (creatures::itemType(item) != 14)
        return 0;
    return std::max(1, creatures::price(item) / 2);
}

creatures::ItemRecord identify(const creatures::ItemRecord &leftovers, worldgen::Random &random) {
    const int16_t level = creatures::itemLevel(leftovers);
    const int32_t rarity = creatures::itemRarity(leftovers);
    const bool weapon = random.draw() % 2 == 0;
    return creatures::toRecord(weapon ? worldgen::weaponItem(random, level, rarity, -1)
                                      : worldgen::armourItem(random, level, rarity, -1));
}

int32_t adaptionCost(int16_t level, uint8_t rarity, int32_t playerLevel) {
    const int32_t n = playerLevel - level;
    if (n <= 0)
        return 0;
    const float q = (float)std::pow(2.0, (double)((float)(level + rarity - 1) * 0.25f));
    const float t = 2.f * q;
    int32_t cost = 0;
    for (int32_t i = 0; i < n; i++)
        cost = (int32_t)((float)cost + t);
    return cost;
}

int32_t rebalancedAdaptionCost(int16_t level, uint8_t rarity, int32_t playerLevel) {
    if (level >= playerLevel)
        return 0;
    const float d = (float)(playerLevel - ((level - 5) + rarity));
    if (d <= 0)
        return 0; // (the mod leaves this case open; nothing to pay here)
    const float P = (float)playerLevel;
    return (int32_t)((P + 1.f) / (P + 10.f) * (d / (d + 1500.f)) * 10000.f);
}

namespace {
LootVictim collectLootVictim(const creatures::Combatant &c, uint16_t appearanceFlags,
                             const int64_t position[3], std::span<const creatures::ItemRecord> inventory) {
    LootVictim v;
    v.controller = c.controller;
    v.type = c.type;
    v.appearanceFlags = appearanceFlags;
    v.level = c.level;
    v.tier = c.powerTier;
    std::copy_n(position, 3, v.position);
    for (const creatures::ItemRecord &item : inventory)
        if (creatures::itemType(item) != 0)
            v.inventory.push_back({1, item}); // (one unit per entry)
    for (size_t k = 0; k < c.equipment.size(); k++)
        v.equipment[k] = c.equipmentRecord(k);
    return v;
}
} // namespace

LootVictim lootVictim(const creatures::NpcFrame::Death &death) {
    return collectLootVictim(death.fighter, death.appearanceFlags, death.position, death.inventory);
}

LootVictim lootVictim(const creatures::Combatant &fighter, const creatures::Body &body,
                     std::span<const creatures::ItemRecord> inventory) {
    return collectLootVictim(fighter, fighter.appearanceFlags, body.position, inventory);
}

void dropLoot(const LootVictim &v, WorldItems &items, int32_t day, const creatures::BlockType &block,
              worldgen::Random &random) {
    const auto draw = [&] { return random.draw(); };
    // §3.1: the offset round the victim (two draws) and the angle argument the drop routine
    // throws away (one draw); `between` runs between the offset and the angle draw.
    const auto place = [&](const creatures::ItemRecord &item, float factor, const std::function<void()> &between = {}) {
        const float dx = 1.f - (float)(draw() * 2) / 32767.f;
        const float dy = 1.f - (float)(draw() * 2) / 32767.f;
        int64_t at[3] = {v.position[0] + (int64_t)(dx * 65536.f), v.position[1] + (int64_t)(dy * 65536.f), v.position[2]};
        if (between)
            between();
        draw(); // the angle argument
        items.drop(item, at, factor, day, block, draw);
    };
    const bool boss = (v.appearanceFlags & 0x200) != 0;
    // §3.2: no loot from players, merchants or "no loot" creatures.
    if (v.controller == 0 || v.controller == 3 || (v.appearanceFlags & 0x800))
        return;
    draw(); // discarded
    std::vector<int> slots;
    for (int k : {6, 7, 2, 5, 4, 3, 1})
        if (v.equipment[k][0] != 0)
            slots.push_back(k);
    if (!slots.empty() && (draw() % 4 == 0 || boss)) {
        creatures::ItemRecord chosen{};
        // The slot pick sits between the offset and the angle draw; the item is dropped as it is.
        place(chosen, 1, [&] { chosen = v.equipment[slots[(uint32_t)draw() % slots.size()]]; });
    }
    const bool hostile = v.controller == 1 || v.type == 143;
    // §3.3: a random armour piece or weapon, then perhaps pet food.
    if (hostile && draw() % 10 == 0) {
        const int16_t level = (int16_t)std::max(1, v.level - 1 + draw() % 2);
        const bool armour = draw() % 2 == 0;
        const int32_t rarity = rarityRoll(random, v.tier);
        creatures::ItemRecord item = creatures::toRecord(armour ? worldgen::armourItem(random, level, rarity, -1)
                                                                : worldgen::weaponItem(random, level, rarity, -1));
        creatures::setItemLevel(item, rerollLevel(random, creatures::itemLevel(item)));
        place(item, 1);
        if (draw() % 5 == 0) {
            rarityRoll(random, v.tier); // (spent; pet food takes no rarity)
            creatures::ItemRecord food = fresh(20, TAMEABLE[(uint32_t)draw() % 45]);
            place(food, 1);
        }
    }
    // §3.4: what the creature's kind drops.
    const int32_t count = 1 + draw() % 3;
    for (int32_t i = 0; i < count; i++) {
        creatures::ItemRecord item;
        float factor = 0.8f;
        switch (v.type) {
        case 58: item = fresh(11, 2, 19); break;
        case 70: item = fresh(11, 18); break;
        case 71: item = fresh(11, 21); break;
        case 120: case 121: case 124: case 126:
            item = draw() % 2 == 0 ? fresh(11, 5, 21) : fresh(11, 1, 2);
            break;
        case 122: item = fresh(11, 27); break;
        case 123: item = fresh(11, 11, 27); break;
        case 125: item = fresh(11, 6); break;
        case 127: item = fresh(11, 20); break;
        case 128: item = fresh(11, 23); break;
        case 131: item = fresh(11, 0, 11), factor = 3; break;
        case 132: item = fresh(11, 0, 1), factor = 3; break;
        case 133: item = fresh(11, 0, 12), factor = 3; break;
        case 134: item = fresh(11, 0, 17); break;
        case 135: item = fresh(11, 0, 13, 1), factor = 3; break;
        case 136: item = fresh(11, 0, 14, 2), factor = 3; break;
        case 137: item = fresh(11, 0, 15, 3), factor = 3; break;
        case 138: item = fresh(11, 0, 16, 4), factor = 3; break;
        case 139: item = fresh(11, 8, 22); break;
        default: i = count; continue; // no type drop: the step ends
        }
        place(item, factor);
    }
    // §3.5: a recipe; then coins, or leftovers for animals.
    if (hostile) {
        if (draw() % 50 == 0) {
            const float dx = 1.f - (float)(draw() * 2) / 32767.f;
            const float dy = 1.f - (float)(draw() * 2) / 32767.f;
            draw(); // the angle argument
            const int32_t rarity = rarityRoll(random, v.tier + 1);
            const creatures::ItemRecord recipe = creatures::toRecord(worldgen::recipeItem(random, (int16_t)v.level, rarity));
            int64_t at[3] = {v.position[0] + (int64_t)(dx * 65536.f), v.position[1] + (int64_t)(dy * 65536.f), v.position[2]};
            items.drop(recipe, at, 0.75f, day, block, draw);
        }
        if (!(v.appearanceFlags & 0x18)) {
            const float c = 1.f - 1.f / ((float)(v.level - 1) * 0.05f + 1.f);
            const float V = (float)std::pow(2.0, (double)(3.f * c)) * (float)std::pow(2.0, (double)(0.25f * (float)v.tier));
            float value = ((float)(draw() * 2) / 32767.f + 1.f) * (V * 10.f);
            if (boss)
                value *= 10;
            const int32_t total = (int32_t)value;
            const std::pair<int32_t, uint8_t> parts[3] = {{total % 100, 10}, {total / 100 % 100, 12}, {total / 10000, 11}};
            for (const auto &[amount, kind] : parts)
                if (amount != 0) {
                    creatures::ItemRecord coins = fresh(12, 0, kind);
                    creatures::setItemLevel(coins, (int16_t)amount);
                    place(coins, 0.75f);
                }
        } else {
            const int32_t u = draw() % 1000 + 20 * v.tier;
            int32_t rarity = u < 700 ? 0 : u < 950 ? 1 : u <= 998 ? 2 : 3;
            if (boss)
                rarity = std::min(rarity + 1, 3);
            if (draw() % 20 == 0 && rarity != 0) {
                creatures::ItemRecord leftovers = fresh(14, 0, 0, (uint8_t)rarity);
                const uint32_t seed = (uint32_t)draw();
                std::memcpy(leftovers.data() + 0x04, &seed, 4);
                creatures::setItemLevel(leftovers, (int16_t)v.level);
                place(leftovers, 0.75f);
            }
        }
    }
    // §3.6: a hostile's inventory, one world item per unit, healing potions kept back.
    if (v.controller == 1)
        for (const creatures::Stack &s : v.inventory)
            if (s.item[0] != 0 && s.count > 0)
                for (int32_t n = 0; n < s.count; n++)
                    if (!(s.item[0] == 1 && s.item[1] == 1))
                        place(s.item, 0.75f);
}
