#include "creatures/DescriptorRefresh.h"
#include "creatures/Npcs.h"
#include "Missions.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
struct Entry {
    int32_t count = 1;
    uint8_t type = 3, subtype = 0;
    int16_t level = 77;
    uint8_t rarity = 99;
};
struct Fixture {
    uint32_t seed = 0xfffffff8;
    int32_t selector = 5, contextLevel = 70000;
    uint8_t contextTier = 255;
    activation::Context context{seed, selector, contextLevel, contextTier};
    uint64_t identifier = 0x81234567ffffffffull;
    int32_t role = 1, level = 17;
    uint8_t tier = 3, marker = 0;
    uint16_t flags = 0x1000;
    std::array<int16_t, 13> equipment{};
    std::vector<std::vector<Entry>> groups;
    std::vector<std::string> events;
    std::vector<size_t> slots;
    std::vector<int32_t> random;
    size_t draws = 0;
    uint32_t appliedSeed = 0;
    std::function<void()> afterSetup, beforeDraw;
    activation::Services services;
    Fixture() {
        equipment.fill(1);
        services.seed = [&](uint32_t value) { events.push_back("seed"); appliedSeed = value; };
        services.setup = [&](int32_t argument) {
            check(argument == 1 && events == std::vector<std::string>{"seed"},
                  "setup receives full argument1 after seeding");
            events.push_back("setup");
            if (afterSetup) afterSetup();
        };
        services.equipmentLevel = [&](size_t slot) -> int16_t & { slots.push_back(slot); return equipment[slot]; };
        services.inventory = [&](const activation::InventoryVisitor &visit) {
            events.push_back("inventory");
            for (auto &group : groups)
                for (auto &entry : group)
                    visit({entry.count, entry.type, entry.subtype, entry.level, entry.rarity});
        };
        services.draw = [&] {
            if (beforeDraw) beforeDraw();
            const int32_t value = draws < random.size() ? random[draws] : 1;
            draws++;
            return value;
        };
    }
    bool run(bool present = true) {
        return activation::refresh({identifier, role, level, tier, flags, marker}, present ? &context : nullptr, services);
    }
};
void admissionAndFlags() {
    for (int rejected = 0; rejected < 3; rejected++) {
        Fixture f;
        if (rejected == 1) f.selector = 0x105;
        if (rejected == 2) f.flags = 0x2200;
        const auto flags = f.flags;
        check(!f.run(rejected != 0) && f.events.empty() && f.slots.empty() && f.draws == 0 &&
              f.level == 17 && f.tier == 3 && f.flags == flags,
              "missing context, full-width selector mismatch and absent refresh bit retain all descriptor state");
    }
    for (const int32_t role : {6, 0x106}) {
        Fixture f;
        f.role = role;
        f.groups = {{{1, 3}}};
        f.afterSetup = [&] {
            check(f.level == (role == 6 ? 17 : 70000) && f.tier == (role == 6 ? 3 : 255),
                  "only full role6 skips descriptor level/tier copies before setup");
        };
        check(f.run() && f.appliedSeed == 0xfffffff7 && f.events == std::vector<std::string>{"seed", "setup", "inventory"} &&
              f.groups[0][0].level == 4464 && f.groups[0][0].rarity == 0 && f.draws == 2,
              "role6 still reseeds from the low identifier half and refreshes nested inventory");
    }
    {
        Fixture f;
        f.marker = 1;
        f.afterSetup = [&] { f.flags |= 0x0200; };
        check(f.run() && f.flags == 0x3200 && f.slots.empty(),
              "the caller rereads flags after setup and ORs the target mask without equipment adjustments");
    }
    {
        Fixture f;
        f.flags = 0x3200;
        check(f.run() && f.flags == 0x3200 && f.slots.empty(),
              "a zero marker never clears an existing target mask");
    }
}
void equipmentAndInventory() {
    const std::vector<size_t> order{7, 6, 5, 2, 4, 3, 1, 8, 9};
    {
        Fixture f;
        f.equipment.fill(5);
        f.random = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        check(f.run() && f.slots == order && f.draws == 9, "all nine level adjustments retain exact record/draw order");
        for (size_t i = 0; i < order.size(); i++)
            check(f.equipment[order[i]] == 3 + int(i % 3), "signed level5 retains inclusive lower3 through original5");
        for (const size_t untouched : {0u, 10u, 11u, 12u})
            check(f.equipment[untouched] == 5, "unlisted equipment records remain untouched");
    }
    for (const auto [input, expected, calls] : std::array<std::array<int16_t, 3>, 8>{{
            {-32768, -32768, 0}, {-1, -1, 0}, {0, 0, 0}, {1, 1, 0},
            {2, 1, 0}, {3, 1, 1}, {5, 3, 1}, {10, 8, 1}}}) {
        Fixture f;
        f.equipment[7] = input;
        f.random = {0};
        check(f.run() && f.equipment[7] == expected && f.draws == size_t(calls),
              "signed level admission and independent curve anchors retain zero/one draw boundaries");
    }
    {
        Fixture f;
        f.flags |= 0x200;
        f.groups = {{}, {{0, 3}, {-7, 3}, {1, 2}}, {{1, 11, 14}, {1, 11, 13}}};
        for (const uint8_t rejected : {0, 12, 13, 20, 21, 23, 24, 25})
            f.groups.back().push_back({1, rejected});
        f.random = {0, 0};
        f.beforeDraw = [&] {
            check(f.groups[1][1].level == 4464 && f.groups[1][1].rarity == f.draws,
                  "context level and wrapped base rarity are stored before each ordered rarity draw");
        };
        check(f.run() && f.draws == 2 && f.groups[1][0].level == 77 && f.groups[1][1].rarity == 2 &&
              f.groups[1][2].level == 4464 && f.groups[1][2].rarity == 99 && f.groups[2][0].level == 4464,
              "negative nonzero counts are admitted; non-equipment accepted kinds receive only their level copy");
        for (size_t i = 1; i < f.groups[2].size(); i++)
            check(f.groups[2][i].level == 77 && f.groups[2][i].rarity == 99,
                  "rejected item predicates do not mutate records or spend rarity draws");
    }
    for (const auto [tier, first, second, expected] : std::array<std::array<int32_t, 4>, 4>{{
            {255, 1, 1, 0}, {255, 0, 0, 2}, {254, 0, 1, 0}, {3, 0, 0, 4}}}) {
        Fixture f;
        f.contextTier = uint8_t(tier);
        f.groups = {{{1, 9}}};
        f.random = {first, second};
        check(f.run() && f.draws == 2 && f.groups[0][0].rarity == expected,
              "byte wrapping precedes both rarity draws and the final unsigned clamp");
    }
}
void contextLookup() {
    Missions missions;
    missions.edit(0, 0).mission.kind = 5;
    missions.edit(1023 * 8 + 7, 1023 * 8 + 7).mission.level = 77;
    int parents = 0;
    std::pair<int32_t, int32_t> parent{};
    const auto present = [&](int32_t x, int32_t y) { parents++; parent = {x, y}; return true; };
    check(missions.descriptorContext(-1, -7, present).has_value() && parent == std::pair{0, 0},
          "containing-cell negative components -1 through-7 truncate to context zero");
    parents = 0;
    check(!missions.descriptorContext(-8, 0, present) && !missions.descriptorContext(65536, 0, present) && parents == 0,
          "invalid divided coordinates reject before parent lookup");
    const auto maximum = missions.descriptorContext(65535, 65535, present);
    check(maximum && maximum->level == 77 && parent == std::pair{1023, 1023},
          "the highest admitted containing-cell pair retains X-major context and parent coordinates");
    check(!missions.descriptorContext(0, 0, [](int32_t, int32_t) { return false; }) &&
          !missions.descriptorContext(8, 0, present), "missing parents/payloads are not allocated by lookup");
    const auto borrowed = missions.descriptorContext(0, 0, present);
    missions.edit(0, 0).mission.level = 65536;
    check(borrowed && borrowed->level == 65536, "cached context references retain later reads rather than freezing field values");
}
void activationAdmission() {
    for (const auto [lower, upper, first, second, admitted, expectedCalls] :
         std::array<std::array<int32_t, 6>, 5>{{
             {10, 10, -999, -999, 1, 0}, {10, 20, 9, 20, 0, 1},
             {10, 20, 10, 20, 1, 2}, {10, 20, 15, 21, 0, 2}, {20, 10, 20, 10, 1, 2}}}) {
        int calls = 0;
        const bool result = activation::inWindow(lower, upper, [&] { return calls++ == 0 ? first : second; });
        check(result == bool(admitted) && calls == expectedCalls,
              "equal bounds bypass clock reads and unequal bounds retain distinct lower/upper queries");
    }
    std::array<int64_t, 3> actor{512 * 65536ll - 1, 0, 0}, descriptor{};
    check(!activation::inRange(actor, descriptor, 512),
          "integer-to-binary32 quantization rejects a point one raw unit inside the exact-radius boundary");
    actor[0]--;
    check(activation::inRange(actor, descriptor, 512) && activation::inRange(actor, descriptor, -512),
          "the next raw point is inside and radius squaring has no invented sign gate");
    actor[0] = std::numeric_limits<int64_t>::min();
    descriptor[0] = std::numeric_limits<int64_t>::max();
    check(activation::inRange(actor, descriptor, 1), "raw position subtraction wraps before conversion and scaling");
    check(!activation::inRange(actor, actor, 0), "zero-radius equality is strictly rejected");
}
void production() {
    for (const int32_t role : {1, 6, 0x106}) {
        auto zone = std::make_shared<worldgen::Zone>(1, 0);
        worldgen::Spawn source;
        source.x = 2300 * 65536ll; source.y = 20 * 65536ll; source.z = 10 * 65536ll;
        source.radius = 5000; // descriptor position is in another context cell
        source.type = 117; source.controller = role; source.level = 17; source.powerTier = 3;
        source.finished = true; source.appearanceFlags = 0x1200; source.bossLoot = 1;
        Appearance looks; looks.flags = source.appearanceFlags; looks.serialize(source.appearance.data());
        source.specials = {55}; source.creationChoice = 2; source.creationKinds = {96};
        for (auto &item : source.equipment) item.level = 1;
        source.inventory.push_back({3, 0, 0, 0, 99, 0, 0, 77});
        zone->spawns.push_back(source);
        worldgen::WorldMap map; map.add(zone);
        worldgen::Random random{123};
        Npcs world{random};
        Missions missions;
        auto &mission = missions.edit(0, 0).mission;
        mission.kind = 5; mission.seed = 123; mission.level = 65536; mission.tier = 255;
        missions.edit(1, 0).mission.level = 7;
        int contexts = 0, observations = 0;
        world.descriptorContext = [&](int32_t x, int32_t y) {
            contexts++;
            return missions.descriptorContext(x, y, [](int32_t, int32_t) { return true; });
        };
        world.onActivate = [&](Npc &actor) {
            observations++;
            check(actor.fighter.level == (role == 6 ? 17 : 65536) && actor.fighter.powerTier == (role == 6 ? 3 : 255) &&
                  actor.looks.flags == 0x3200 && actor.inventory.groups.empty(),
                  "pre-equipment callback observes refreshed descriptor state before the later inventory assignment");
        };
        Combatant player; player.id = 1; player.health = maxHealth(player);
        Body body; body.place({276.5, 150.5, 11.1});
        const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z < 10 ? 1 : 0); };
        const auto step = [&] { world.step(16, map, {{player, body, 0, 0, false}}, 1000, 0, floor); };
        step();
        check(world.all().size() == 1 && contexts == 1 && observations == 1 && random.count() == 8,
              "actual refresh uses one cached containing-cell context and setup/inventory draws before activation");
        if (world.all().size() != 1) continue;
        const auto actions = world.all()[0].fighter.specials;
        check(world.all()[0].inventory.groups.size() == 1 && itemLevel(world.all()[0].inventory.groups[0][0].item) == 0 &&
              actions == (role == 6 ? std::vector<int32_t>{55} : std::vector<int32_t>{55, 92}) &&
              world.all()[0].creationConfig.choice == (role == 6 ? 2 : 0) &&
              itemLevel(world.all()[0].fighter.equipmentRecord(7)) == (role == 6 ? 17 : 0),
              "full descriptor role controls setup and zero low-word hand levels survive actual actor import");
        world.npcs().clear();
        step();
        auto repeated = actions;
        if (role != 6) repeated.push_back(92);
        check(world.all().size() == 1 && world.all()[0].fighter.specials == repeated && random.count() == 16 &&
              zone->spawns[0].specials == source.specials && zone->spawns[0].level == 17 && zone->spawns[0].appearanceFlags == 0x1200,
              "reactivation retains descriptor mutations without altering the shared generated snapshot");
        world.npcs().clear();
        mission.kind = 0;
        step();
        check(world.all().size() == 1 && world.all()[0].fighter.specials == repeated && random.count() == 16,
              "ineligible refresh still activates from retained descriptor state without clearing flags or reseeding");
        world.npcs().clear();
        mission.kind = 5;
        map.add(std::make_shared<worldgen::Zone>(*zone));
        step();
        check(world.all().size() == 1 && world.all()[0].fighter.specials == actions && random.count() == 24,
              "replacing a generated snapshot drops its old overlay before the next eligible refresh");
    }
}
} // namespace
int main() {
    admissionAndFlags(); equipmentAndInventory(); contextLookup(); activationAdmission(); production();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
