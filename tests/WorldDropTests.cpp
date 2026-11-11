#include "Loot.h"
#include <bit>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

using namespace creatures;

static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static uint32_t bits(float value) { return std::bit_cast<uint32_t>(value); }
constexpr int64_t UNIT = 65536;

static const BlockType floorBlock = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };

static WorldItems heldOrigin() {
    WorldItems items;
    items.generated(*std::make_shared<worldgen::Zone>(0, 0));
    return items;
}
static size_t resident(const WorldItems &items) { return items.zone(0, 0) ? items.zone(0, 0)->size() : 0; }

static ItemRecord marked() {
    ItemRecord item = freshInventoryItem();
    item[0] = 11; item[1] = 2;
    const uint32_t seed = 0xdeadbeef;
    std::memcpy(item.data() + 4, &seed, 4);
    return item;
}

static void recordAndResidency() {
    WorldItems items = heldOrigin();
    const size_t before = resident(items);
    std::vector<int> script{599, 16384};
    size_t used = 0;
    const auto draw = [&] { return script[used++]; };
    const int64_t at[3] = {10 * UNIT, 20 * UNIT, 25 * UNIT + 7};
    check(items.drop(marked(), at, .75f, 9, floorBlock, draw) && used == 2 && resident(items) == before + 1,
          "a resident drop consumes the landing draw and the replacement-angle draw");
    const WorldItem &dropped = items.zone(0, 0)->back();
    check(bits(dropped.scale) == 0x3d5b6db8, "the constructor scalar takes one binary32 product with the supplied factor");
    check(dropped.landingMs == 799 && dropped.day == 9, "the landing value is the draw's remainder modulo 300 plus 500");
    check(bits(dropped.angle) == bits(16384.f * 360.f / 32767.f), "the stored angle is the nested draw times 360 divided by 32767");
    check(dropped.position[0] == at[0] && dropped.position[1] == at[1] && dropped.position[2] == UNIT,
          "x and y keep their raw values and z becomes one above the terminating integer");

    used = 0;
    const int64_t away[3] = {5 * 256 * UNIT, 20 * UNIT, 25 * UNIT};
    check(!items.drop(marked(), away, .75f, 9, floorBlock, draw) && used == 1 && resident(items) == before + 1,
          "missing first residency returns after only the landing draw and appends nothing");

    items.takeChanged();
    used = 0; script = {1, 2, 3, 4};
    items.drop(marked(), at, .75f, 9, floorBlock, draw);
    items.drop(marked(), at, .75f, 9, floorBlock, draw);
    const auto changed = items.takeChanged();
    check(resident(items) == before + 3 && changed.size() == 1 && changed.count({0, 0}) == 1,
          "equal resident keys coalesce without suppressing the appended records");
}

static void truncationAndScan() {
    WorldItems items = heldOrigin();
    const auto draw = [] { return 100; };
    std::vector<int32_t> heights;
    const BlockType recording = [&](int32_t, int32_t, int32_t z) { heights.push_back(z); return uint8_t(1); };

    const int64_t beforeOrigin[3] = {-5 * UNIT, 20 * UNIT, 3 * UNIT};
    check(items.drop(marked(), beforeOrigin, 1, 0, recording, draw) && heights.empty() &&
          items.zone(0, 0)->back().position[2] == 4 * UNIT,
          "a small negative coordinate truncates to zone zero, then its out-of-range integer ends the scan at once");
    const int64_t farBefore[3] = {-300 * UNIT, 20 * UNIT, 3 * UNIT};
    check(!items.drop(marked(), farBefore, 1, 0, recording, draw), "a negative zone is not resident");

    const auto firstHeight = [&](int64_t raw) {
        heights.clear();
        const int64_t at[3] = {10 * UNIT, 20 * UNIT, raw};
        items.drop(marked(), at, 1, 0, recording, draw);
        return heights.empty() ? 12345 : heights.front();
    };
    check(firstHeight(3 * UNIT + 5) == 3 && firstHeight(0) == 0 && firstHeight(-1) == -1 && firstHeight(-UNIT) == -2 &&
          firstHeight(-UNIT - 1) == -2,
          "the vertical start truncates and then steps down once for every negative height, including exact multiples");
    check(items.zone(0, 0)->back().position[2] == -UNIT, "the landed height is the terminating integer plus one");

    const BlockType layered = [](int32_t, int32_t, int32_t z) { return uint8_t(z > 5 ? 0x22 : z > 0 ? 0x40 : 0x21); };
    const int64_t high[3] = {10 * UNIT, 20 * UNIT, 30 * UNIT};
    items.drop(marked(), high, 1, 0, layered, draw);
    check(items.zone(0, 0)->back().position[2] == UNIT, "only the low five class bits decide; classes 0 and 2 are passed and others end the scan");
}

static void semanticItem() {
    WorldItems items = heldOrigin();
    ItemRecord source = marked();
    for (size_t i = 0x14; i < source.size(); ++i)
        source[i] = uint8_t(i * 7 + 1);
    for (int gap : {0x02, 0x03, 0x0f, 0x12, 0x13})
        source[gap] = 0xaa;
    source[0x0c] = 3; source[0x0d] = 21; source[0x0e] = 5;
    setItemLevel(source, 0x7123);
    const int64_t at[3] = {10 * UNIT, 20 * UNIT, 5 * UNIT};
    items.drop(source, at, 1, 0, floorBlock, [] { return 0; });
    const ItemRecord &dropped = items.zone(0, 0)->back().item;
    bool gaps = true, rest = true;
    for (size_t i = 0; i < source.size(); ++i) {
        const bool gap = i == 0x02 || i == 0x03 || i == 0x0f || i == 0x12 || i == 0x13;
        if (gap) gaps &= dropped[i] == 0;
        else rest &= dropped[i] == source[i];
    }
    check(gaps && rest, "the item is assigned field by field: every semantic byte and body entry, with fresh gaps left zero");
}

static uint32_t tailDraws(int32_t count, const ItemRecord &item, bool residentZone) {
    const auto run = [&](bool withInventory) {
        WorldItems items = heldOrigin();
        worldgen::Random random(123);
        LootVictim victim;
        victim.controller = 1;
        victim.type = 4;
        const int64_t zone = residentZone ? 0 : 7;
        victim.position[0] = (zone * 256 + 100) * UNIT;
        victim.position[1] = 100 * UNIT;
        victim.position[2] = 3 * UNIT;
        if (withInventory)
            victim.inventory = {{count, item}};
        dropLoot(victim, items, 0, floorBlock, random);
        return random.count();
    };
    return run(true) - run(false);
}

static void unitAccounting() {
    ItemRecord potion = marked();
    potion[0] = potion[1] = 1;
    ItemRecord empty = marked();
    empty[0] = 0;
    check(tailDraws(3, marked(), true) == 15, "a resident inventory unit consumes five direct draws");
    check(tailDraws(3, marked(), false) == 12, "a unit without first residency consumes four direct draws");
    check(tailDraws(4, potion, true) == 0, "category-1 subtype-1 units advance without draws");
    check(tailDraws(0, marked(), true) == 0 && tailDraws(-2, marked(), true) == 0 && tailDraws(2, empty, true) == 0,
          "zero or negative quantities and category zero make no attempts");

    WorldItems items = heldOrigin();
    const size_t before = resident(items);
    worldgen::Random random(123);
    LootVictim hostile;
    hostile.controller = 2;
    hostile.inventory = {{2, marked()}};
    hostile.position[0] = hostile.position[1] = 100 * UNIT;
    dropLoot(hostile, items, 0, floorBlock, random);
    size_t unitsDropped = 0;
    for (size_t i = before; i < resident(items); ++i) {
        uint32_t seed;
        std::memcpy(&seed, (*items.zone(0, 0))[i].item.data() + 4, 4);
        unitsDropped += seed == 0xdeadbeef;
    }
    check(unitsDropped == 0, "only actor byte value one reaches the inventory-unit branch");
}

int main() {
    recordAndResidency(); truncationAndScan(); semanticItem(); unitAccounting();
    if (!failures) std::puts("PASS: world-drop record, residency, truncation, vertical scan, semantic item copy and unit draw accounting");
    return failures ? 1 : 0;
}
