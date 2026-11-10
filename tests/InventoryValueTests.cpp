#include "creatures/InventoryValue.h"
#include "creatures/Inventory.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
constexpr std::array<size_t, 5> gaps{2, 3, 15, 18, 19};
ItemRecord item(uint8_t type, uint8_t subtype, int16_t level) {
    auto result = freshInventoryItem();
    result[0] = type; result[1] = subtype;
    setItemLevel(result, level);
    return result;
}
ItemRecord patterned(uint8_t value) {
    ItemRecord result;
    for (size_t index = 0; index < result.size(); index++) result[index] = uint8_t(value + index * 17);
    return result;
}
bool assigned(const ItemRecord &source, const ItemRecord &destination, const ItemRecord &before) {
    ItemRecord expected = source;
    for (size_t gap : gaps) expected[gap] = before[gap];
    return destination == expected;
}
void constructionAndCopy() {
    InventoryValue fresh;
    CountedItem empty;
    check(fresh.groups.empty() && fresh.auxiliary == 0 && fresh.accumulators == std::array<uint32_t, 2>{} &&
          itemLevel(fresh.separate) == 1 && empty.count == 0 && itemLevel(empty.item) == 1 &&
          std::all_of(fresh.separate.begin() + 0x14, fresh.separate.end(), [](uint8_t b) { return b == 0; }),
          "fresh inventory has no implicit groups, zero scalar/accumulators and constructor level1 items");
    InventoryValue source;
    source.groups.resize(3);
    source.groups[0] = {{0, patterned(1)}, {-1, patterned(2)}};
    source.groups[2] = {{std::numeric_limits<int32_t>::min(), patterned(3)}};
    source.auxiliary = 0xfedcba98;
    source.separate = patterned(4);
    source.accumulators = {0x80000001, 0xffffffff};
    InventoryValue destination;
    destination.groups.resize(5);
    for (auto &group : destination.groups) group = {{17, patterned(55)}, {18, patterned(66)}, {19, patterned(77)}};
    destination.separate = patterned(88);
    const auto before = destination;
    destination.copyFrom(source);
    check(destination.groups.size() == 3 && destination.groups[0].size() == 2 && destination.groups[1].empty() &&
          destination.groups[2].size() == 1 && destination.groups[0][0].count == 0 && destination.groups[0][1].count == -1 &&
          destination.groups[2][0].count == std::numeric_limits<int32_t>::min() && destination.auxiliary == source.auxiliary &&
          destination.accumulators == source.accumulators,
          "value assignment preserves empty groups, all signed counts and separate scalars while removing excess extents");
    check(assigned(source.groups[0][0].item, destination.groups[0][0].item, before.groups[0][0].item) &&
          assigned(source.groups[0][1].item, destination.groups[0][1].item, before.groups[0][1].item) &&
          assigned(source.separate, destination.separate, before.separate),
          "assignment copies the entire item extent including inactive payloads while retaining each destination's five gaps");
    const auto *outer = destination.groups.data();
    const auto *inner = destination.groups[0].data();
    destination.copyFrom(destination);
    check(destination.groups.data() == outer && destination.groups[0].data() == inner,
          "self assignment does not rebuild nested collections");
    destination.groups[0][1].item[0] ^= 1;
    check(destination.groups[0].data() != source.groups[0].data() && destination.groups[0][1].item[0] != source.groups[0][1].item[0],
          "actor and descriptor nested storage are independently owned");
    const auto separate = destination.separate;
    destination.copyFrom(fresh);
    check(destination.groups.empty() && destination.auxiliary == 0 && destination.accumulators == fresh.accumulators &&
          assigned(fresh.separate, destination.separate, separate),
          "empty-source assignment resets logical extents and semantic members without zeroing destination gaps");
    destination.reset(4);
    check(destination.groups.size() == 4 && std::all_of(destination.groups.begin(), destination.groups.end(), [](const auto &g) { return g.empty(); }),
          "four empty groups require an explicit reset policy rather than activation normalization");
}
void routingAndCounts() {
    const std::array<std::pair<uint8_t, size_t>, 13> routes{{{3, 0}, {9, 0}, {23, 0}, {24, 0}, {11, 2}, {19, 3}, {20, 3},
                                                        {1, 1}, {10, 1}, {12, 1}, {21, 1}, {25, 1}, {255, 1}}};
    for (const auto [type, group] : routes) {
        InventoryValue value;
        value.insert(item(type, 0, 1));
        check(value.groups.size() == group + 1 && value.groups[group].size() == 1,
              "automatic routing grows only the selected group and empty intervening headers");
    }
    InventoryValue value;
    const auto input = item(11, 12, 5);
    value.insert(input, 0);
    check(value.groups.size() == 1 && value.groups[0][0].count == 5 && itemLevel(value.groups[0][0].item) == 1 && itemLevel(input) == 5,
          "explicit group0 bypasses classification and counted normalization changes only the temporary item");
    value.insert(item(3, 0, 5), 0);
    value.insert(item(11, 14, 5), 0);
    check(value.groups[0][1].count == 1 && itemLevel(value.groups[0][1].item) == 5 &&
          value.groups[0][2].count == 1 && itemLevel(value.groups[0][2].item) == 5,
          "ordinary equipment and type11/subtype14 retain internal word5 while contributing one entry unit");
    InventoryValue zero;
    zero.insert(item(0, 0, 7));
    check(zero.groups.empty(), "type0 returns before group growth");
    zero.insert(item(21, 0, 0));
    check(zero.groups[1].size() == 1 && zero.groups[1][0].count == 0 && zero.groups[1][0].item[0] == 21 &&
          itemLevel(zero.groups[1][0].item) == 1, "zero quantity retains a populated zero-count item");
    InventoryValue negative;
    negative.insert(item(12, 0, -1));
    check(negative.groups[1][0].count == -1 && itemLevel(negative.groups[1][0].item) == 1,
          "signed quantity minus1 is retained rather than rejected or clamped");
}
void identityAndVacancies() {
    InventoryValue value;
    value.groups.resize(2);
    auto matching = item(11, 12, 1);
    const int32_t count = 1;
    std::memcpy(matching.data() + 0x114, &count, 4);
    matching[0x14] = 1; matching[0x15] = 2; matching[0x16] = 3; matching[0x17] = 4;
    matching[0x18] = 0xaa;
    const auto vacant = patterned(55);
    value.groups[1] = {{0, vacant}, {-3, matching}};
    auto input = matching;
    input[0x18] = 0xbb;
    setItemLevel(input, 2);
    value.insert(input, 1);
    check(value.groups[1][0].count == 0 && value.groups[1][0].item == vacant && value.groups[1][1].count == -1 &&
          value.groups[1][1].item[0x18] == 0xaa,
          "negative matching entry takes priority over an earlier zero slot and identity ignores payload metadata words");
    InventoryValue holes;
    holes.groups.resize(1);
    holes.groups[0] = {{-1, item(9, 0, 1)}, {0, patterned(11)}, {0, patterned(22)}};
    const auto before = holes.groups[0][1].item;
    const auto incoming = item(3, 7, 5);
    holes.insert(incoming, 0);
    check(holes.groups[0].size() == 3 && holes.groups[0][0].count == -1 && holes.groups[0][1].count == 1 &&
          assigned(incoming, holes.groups[0][1].item, before) && holes.groups[0][2].count == 0,
          "first exact-zero vacancy is reused with semantic copy, preserving gaps and skipping negative entries as vacancies");
    InventoryValue wrap;
    wrap.groups.resize(1);
    wrap.groups[0] = {{std::numeric_limits<int32_t>::max(), item(1, 1, 5)}};
    wrap.insert(item(1, 1, 5), 0);
    check(wrap.groups[0][0].count == std::numeric_limits<int32_t>::min() && wrap.groups.size() == 1,
          "stack-count addition wraps32 bits without saturation or automatic removal");
}
void accumulatorsAndReplacement() {
    InventoryValue value;
    auto input = item(12, 0, -1); input[0x0d] = 10; value.insert(input);
    check(value.groups.size() == 2 && value.groups[0].empty() && value.groups[1].empty() && value.accumulators[0] == 0xffffffff,
          "accumulator-only insertion still grows its selected group before returning");
    setItemLevel(input, 2); input[0x0d] = 11; value.insert(input);
    setItemLevel(input, 3); input[0x0d] = 12; value.insert(input);
    value.insert(item(13, 0, std::numeric_limits<int16_t>::min()));
    check(value.accumulators == std::array<uint32_t, 2>{20299, 0xffff8000} && value.groups[1].empty(),
          "concrete scales1/10000/100 and the second accumulator use full wrapping additions without entries");
    value.auxiliary = 0x12345678;
    value.separate = patterned(37);
    const auto separate = value.separate;
    value.groups[0] = {{-7, patterned(1)}};
    int calls = 0;
    value.replaceTen([&](size_t index) {
        check(index == size_t(calls++) && value.groups.size() == 1 && value.groups[0].size() == 10 && value.groups[0][index].count == 1 &&
              (index == 9 || value.groups[0][index + 1].count == 0),
              "replacement stores count1 before each ordered generator call, without preassigning later counts");
        return item(11, uint8_t(index), 5);
    });
    check(calls == 10 && value.auxiliary == 0x12345678 && value.separate == separate &&
          value.accumulators == std::array<uint32_t, 2>{20299, 0xffff8000} && value.groups[0][9].count == 1 &&
          itemLevel(value.groups[0][9].item) == 5,
          "ten-entry collection replacement retains other members and bypasses counted normalization");
}
void consumerProjections() {
    InventoryValue value;
    value.groups.resize(3);
    value.groups[0] = {{0, item(1, 1, 1)}, {-1, item(3, 0, 1)}};
    value.groups[2] = {{-7, item(1, 1, 2)}, {5, item(1, 1, 3)}};
    check(value.firstHealthItem() == &value.groups[2][0],
          "health lookup skips zero counts and searches every group in order, including negative quantities");
    const auto entries = value.entries();
    const auto items = value.itemValues();
    check(entries.size() == 4 && entries[0].count == 0 && entries[1].count == -1 && entries[2].count == -7 && entries[3].count == 5 &&
          items.size() == 4 && itemLevel(items[2]) == 2 && entries[2].item == value.groups[2][0].item,
          "event and widget projections retain ordered rows without imposing a shared quantity filter");
}
struct WorldFixture {
    std::shared_ptr<worldgen::Zone> zone = std::make_shared<worldgen::Zone>(1, 0);
    worldgen::WorldMap map;
    Combatant player;
    Body body;
    WorldFixture() {
        player.id = 1; player.health = maxHealth(player);
        body.place({276.5, 150.5, 6});
    }
    worldgen::Spawn source() const {
        worldgen::Spawn spawn;
        spawn.x = 276 * 65536; spawn.y = 20 * 65536; spawn.z = 5 * 65536;
        spawn.type = 37; spawn.controller = 3; spawn.finished = true; spawn.appearanceFlags = 0x40;
        for (auto &record : spawn.equipment) record.level = 1;
        return spawn;
    }
    void start(Npcs &world) {
        map.add(zone);
        world.step(0, map, {{player, body, 0, 0}}, 1000, 0, [](int32_t, int32_t, int32_t z) { return uint8_t(z < 5 ? 1 : 0); });
    }
};
void activationTransfer() {
    WorldFixture f;
    auto source = std::make_shared<InventoryValue>();
    source->groups.resize(3);
    source->groups[0] = {{0, patterned(1)}, {-1, patterned(2)}};
    source->groups[2] = {{7, patterned(3)}};
    source->auxiliary = 0xabcdef01;
    source->accumulators = {5, 0xffffffff};
    source->separate = patterned(4);
    auto spawn = f.source();
    spawn.inventoryValue = source;
    spawn.creationChoice = 17; spawn.creationKinds = {96, 17};
    spawn.equipment[2].type = 4;
    f.zone->spawns.push_back(spawn);
    Npcs world;
    Npc *creating = nullptr;
    bool sawInventory = false;
    world.onActivate = [&](Npc &npc) { creating = &npc; };
    auto &bonuses = itemBonuses();
    const auto saved = bonuses.health;
    bonuses.health = [&](const Item &) {
        if (creating && !sawInventory) {
            const auto &value = creating->inventory;
            check(value.groups.size() == 3 && value.groups[0][1].count == -1 && value.auxiliary == 0xabcdef01 &&
                  value.accumulators == source->accumulators && creating->creationConfig.choice == 17 &&
                  creating->creationConfig.kinds == std::vector<int32_t>{96, 17},
                  "actual amount query sees full inventory after creation configuration and equipment setup");
            sawInventory = true;
        }
        return 0.f;
    };
    f.start(world);
    bonuses.health = saved;
    check(world.all().size() == 1 && sawInventory, "actual activation reaches its post-transfer amount query");
    if (world.all().size() != 1) return;
    const auto &value = world.all()[0].inventory;
    check(value.groups[1].empty() && value.groups[2][0].count == 7 && value.groups[0].data() != source->groups[0].data() &&
          assigned(source->groups[0][1].item, value.groups[0][1].item, freshInventoryItem()) &&
          assigned(source->separate, value.separate, freshInventoryItem()),
          "real actor retains raw group order/counts and complete semantic items with independently owned storage");
    WorldFixture legacy;
    auto compact = legacy.source();
    compact.inventory.push_back({11, 12, 0, 0, 0, 0, 0, 7});
    legacy.zone->spawns.push_back(compact);
    Npcs older;
    legacy.start(older);
    check(older.all().size() == 1 && older.all()[0].inventory.groups.size() == 1 &&
          older.all()[0].inventory.groups[0][0].count == 1 && itemLevel(older.all()[0].inventory.groups[0][0].item) == 7,
          "legacy flat generator bridge retains direct one-unit internal word7 instead of applying automatic insertion");
}
void refreshBacking() {
    WorldFixture f;
    auto source = std::make_shared<InventoryValue>();
    source->groups.resize(3);
    source->groups[0] = {{0, item(3, 0, 4)}};
    source->groups[2] = {{-1, item(3, 0, 4)}, {5, item(11, 14, 5)}, {7, item(12, 0, 5)}};
    source->separate = item(3, 0, 99);
    auto spawn = f.source();
    spawn.appearanceFlags = 0x1040;
    spawn.inventoryValue = source;
    f.zone->spawns.push_back(spawn);
    const uint32_t seed = 123;
    const int32_t selector = 5, level = 42;
    const uint8_t tier = 2;
    const activation::Context context{seed, selector, level, tier};
    worldgen::Random random{999};
    Npcs world{random};
    world.descriptorContext = [&](int32_t, int32_t) { return std::optional(context); };
    worldgen::Random expected{123 + 256};
    uint8_t rarity = 3;
    if (expected.draw() % 20 == 0) rarity++;
    if (expected.draw() % 100 == 0) rarity++;
    rarity = std::min(rarity, uint8_t(4));
    f.start(world);
    check(world.all().size() == 1 && random.count() == 2, "full grouped refresh preserves its two reached item draws without compact-row work");
    if (world.all().size() != 1) return;
    const auto &value = world.all()[0].inventory;
    check(value.groups[0][0].count == 0 && itemLevel(value.groups[0][0].item) == 4 &&
          value.groups[2][0].count == -1 && itemLevel(value.groups[2][0].item) == 42 && value.groups[2][0].item[0x0c] == rarity &&
          itemLevel(value.groups[2][1].item) == 42 && itemLevel(value.groups[2][2].item) == 5 && itemLevel(value.separate) == 99 &&
          itemLevel(source->groups[2][0].item) == 4 && f.zone->spawns[0].inventoryValue == source,
          "refresh admits negative counts, skips zero/rejected entries and standalone item, then copies without mutating the immutable source");
}
}

int main() {
    constructionAndCopy(); routingAndCounts(); identityAndVacancies(); accumulatorsAndReplacement(); activationTransfer(); refreshBacking();
    consumerProjections();
    if (failures) std::printf("%d inventory-value failures\n", failures);
    else std::puts("InventoryValueTests PASS");
    return failures ? 1 : 0;
}
