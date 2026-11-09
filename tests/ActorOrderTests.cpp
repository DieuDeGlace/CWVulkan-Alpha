#include "creatures/ActorMap.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <array>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
constexpr uint64_t minimum = uint64_t(1) << 63;
constexpr uint64_t maximum = minimum - 1;
const BlockType floorBlock = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
Npc fixture(uint64_t key, float health = 100) {
    Npc npc;
    npc.fighter.id = key;
    npc.fighter.controller = 1;
    npc.fighter.health = health;
    npc.fighter.buffs = {{3, 0, 1000}};
    npc.body.place({10, 10, 3});
    npc.home = npc.body.blocks();
    return npc;
}
}

int main() {
    {
        const std::array<uint64_t, 9> expected{
            minimum, 0xffffffff00000000ull, UINT64_MAX, 0, 0x7fffffffull,
            0x80000000ull, 0xffffffffull, 0x100000000ull, maximum};
        Combatant actor;
        actor.id = 777;
        ActorMap<Combatant *> map;
        for (auto key = expected.rbegin(); key != expected.rend(); ++key)
            map.emplace(*key, &actor);
        actor.id = 42;
        std::vector<uint64_t> keys;
        for (const auto &[key, value] : map)
            keys.push_back(key);
        check(std::equal(keys.begin(), keys.end(), expected.begin(), expected.end()),
              "signed high words and unsigned low words determine key order independently of creature own ID");
        const auto *entry = &*map.find(0);
        const auto duplicate = map.try_emplace(0, nullptr);
        check(!duplicate.second && duplicate.first->second == &actor && &*duplicate.first == entry,
              "duplicate insertion preserves the existing mapped pointer and entry identity");
        map[0] = nullptr;
        check(map.contains(0) && map.at(0) == nullptr && !map.contains(2),
              "null mappings retain their exact occupied keys separately from absence");
    }
    {
        ActorMap<int> map{{UINT64_MAX, 1}, {0, 2}, {3, 3}, {5, 5}};
        std::vector<uint64_t> visited;
        const auto *retained = &*map.find(3);
        for (auto entry = map.begin(); entry != map.end(); ++entry) {
            visited.push_back(entry->first);
            if (entry->first == 0) {
                map.emplace(uint64_t(-2), -2);
                map.emplace(2, 2);
                map.erase(5);
            }
        }
        check(visited == std::vector<uint64_t>{UINT64_MAX, 0, 2, 3} &&
              map.contains(uint64_t(-2)) && &*map.find(3) == retained,
              "live successor sees later insertions and future removal without revisiting an earlier inserted key");
    }
    for (const uint64_t localKey : {uint64_t(-2), uint64_t(0), uint64_t(1), uint64_t(4)}) {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap world;
        world.add(zone);
        Npcs npcs;
        npcs.npcs().push_back(fixture(3)); // deliberately reverse the storage order
        npcs.npcs().push_back(fixture(UINT64_MAX));
        npcs.npcs().push_back(fixture(minimum, 0));
        Combatant local;
        local.id = localKey;
        Body body;
        body.place({0, 0, 3});
        NpcFrame frame;
        int calls = 0;
        npcs.stepInto(20, world, {{local, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame, {}, {},
            [&](int32_t elapsed) {
                calls++;
                check(elapsed == 20, "controlled callback receives the shared integer piece");
                for (const Npc &npc : npcs.all()) {
                    const bool advanced = npc.fighter.health > 0 && ActorKeyLess{}(npc.fighter.id, localKey);
                    check(npc.fighter.buffs[0].timeMs == (advanced ? 980 : 1000),
                          "actual NPC common countdown occurs before/after the controlled actor solely by key");
                }
            });
        check(calls == 1 && npcs.all().size() == 3 && npcs.all()[2].fighter.buffs[0].timeMs == 1000,
              "controlled actor runs once; dead NPC skips its body without erasure at the traversal boundary");
    }
    for (const bool wrap : {false, true}) {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap world;
        world.add(zone);
        Npcs npcs;
        npcs.npcs().push_back(fixture(wrap ? minimum : UINT64_MAX));
        const Npc *retained = &npcs.all().front();
        Combatant local;
        local.id = wrap ? 0 : uint64_t(-4);
        Body body;
        body.place({0, 0, 3});
        NpcFrame frame;
        uint64_t made = 0;
        npcs.stepInto(20, world, {{local, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame, {}, {},
            [&](int32_t) {
                made = npcs.placeBomb({5, 5, 3}, 1);
                npcs.npcs().back().fighter.buffs = {{3, 0, 1000}};
            });
        check(made == (wrap ? maximum : uint64_t(-5)) && &npcs.all().front() == retained,
              "in-pass ID selection includes the controlled entry, wraps at signed minimum and preserves existing references");
        check(npcs.all().back().fighter.buffs[0].timeMs == (wrap ? 980 : 1000),
              "actual in-pass append visits a wrapped later key and defers an ordinary earlier key");
        npcs.stepInto(20, world, {{local, body, 0, 0, false, false, true}}, 1200, 0, floorBlock, frame);
        check(npcs.all().back().fighter.buffs[0].timeMs == (wrap ? 960 : 980),
              "the following pass refreshes mappings and visits the surviving inserted actor");
    }
    {
        ActorMap<uint64_t> received{{3, 3}, {UINT64_MAX, UINT64_MAX}, {minimum, minimum}};
        std::vector<uint64_t> visited;
        visitReceivedActors(received, 0, [&] { visited.push_back(0); },
                            [&](uint64_t value) { visited.push_back(value); });
        check(visited == std::vector<uint64_t>{minimum, UINT64_MAX, 0, 3},
              "protected received-state traversal merges the controlled actor by signed key");
        received.clear();
        visited.clear();
        visitReceivedActors(received, 0, [&] { visited.push_back(0); }, [&](uint64_t) {});
        check(visited == std::vector<uint64_t>{0}, "an empty received collection still advances the controlled actor once");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
