#include "BlockEdits.h"
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

void checkOverlappingStrikes() {
    worldgen::WorldMap map;
    auto zone = std::make_shared<worldgen::Zone>(0, 0);
    const worldgen::Block natural{18, 76, 34, worldgen::GRASS | worldgen::SURFACE_COLOURED};
    zone->place(20, 25, 3, natural);
    map.add(zone);
    worldgen::Random random{31};
    auto expected = random;
    const float pitch = float(expected.draw()) * 0.4f / 32767.f + 0.5f;
    Npcs npcs(random);
    const auto makeStriker = [&](uint64_t id, double x) {
        Npc &npc = npcs.npcs().emplace_back();
        npc.fighter.id = id;
        npc.fighter.controller = 3;
        npc.fighter.action = 17;
        const Phases timing = phases(npc.fighter, 17);
        npc.fighter.actionMs = timing.windUp + timing.active / 2;
        npc.body.size = glm::vec3(5.6f);
        npc.body.place({x, 15.5, 3});
        npc.home = npc.body.blocks();
    };
    makeStriker(5, 20.75);
    makeStriker(uint64_t(-2), 20.25);

    std::vector<int64_t> centres;
    std::vector<BlockEdit> edits;
    npcs.breakBlocks = [&](const std::array<int64_t, 3> &centre, int32_t radius, CellRounding rounding,
                           std::vector<Burst> &bursts) {
        centres.push_back(centre[0]);
        const auto cells = breakCells(centre, radius, rounding, 12,
            [&](int32_t x, int32_t y, int32_t z) { return map.read(x, y, z); });
        if (cells.empty())
            return false;
        auto changed = std::make_shared<worldgen::Zone>(*map.zone(0, 0));
        for (const BrokenCell &cell : cells) {
            bursts.push_back(blockBurst(cell.at, glm::vec3(cell.colour)));
            applyLive(*changed, cell.removal, {});
            edits.push_back(cell.removal);
        }
        map.add(std::move(changed));
        return true;
    };
    const BlockType block = [&](int32_t x, int32_t y, int32_t z) { return map.read(x, y, z).type(); };
    const auto frame = npcs.step(1, map, {}, 1000, 12, block);
    check(centres == std::vector<int64_t>{1327104, 1359872}, "overlapping strikes execute in signed actor order");
    check(edits.size() == 1 && edits.front() == BlockEdit{20, 25, 3, 255, 255, 255, 0, 12} &&
              frame.bursts.size() == 1 && frame.bursts.front().style == LIT && map.read(20, 25, 3).type() == worldgen::AIR,
          "the later strike sees the terrain after the first edit and adds no second record or burst");
    check(frame.sounds.size() == 1 && frame.sounds.front().sound == 2 && frame.sounds.front().pitch == pitch &&
              frame.sounds.front().position.x == 20.25 && random.count() == 1,
          "only the first strike spends the removal sound draw");
}

void checkRidingRefillOrder() {
    worldgen::WorldMap world;
    worldgen::Random random{82};
    Npcs npcs(random);
    Combatant rider;
    rider.id = 8;
    rider.linkedId = UINT64_MAX;
    rider.action = 106;
    rider.skills[1] = 1;
    Body body;
    body.place({10.9, 10.9, 2.09});
    body.ridingEnergy = 0.5f;
    body.bob = 0.125f;
    Npc &pet = npcs.npcs().emplace_back();
    pet.fighter.controller = 5;
    pet.fighter.type = 19;
    pet.fighter.id = rider.linkedId;
    pet.fighter.ownerId = rider.id;
    pet.fighter.health = 100.f;
    pet.body.place(body.blocks());
    pet.body.size = pet.looks.box = body.size;
    pet.home = pet.body.blocks();
    const BlockType dryLakeBed = [](int32_t, int32_t y, int32_t z) {
        return uint8_t(z > 0 ? 0 : y < 11 ? 1 : 3);
    };
    const PlayerView player{rider, body, 0, 0, false, false, true};
    NpcFrame frame;
    std::vector<float> sampled;
    bool attachedBeforeRider = true;
    for (const int32_t ms : {13, 7})
        npcs.stepInto(float(ms), world, {player}, 1000, 12, dryLakeBed, frame, {}, {}, [&](int32_t) {
            sampled.push_back(body.ridingEnergy);
            attachedBeforeRider &= pet.body.position[0] == body.position[0] && pet.body.bob == body.bob;
            body.position[0] += 1024;
            body.yaw += 15.f;
            body.bob += 0.125f;
        });
    const float first = 0.5f + 0.05f * (13.f * 0.001f);
    const float second = first + 0.05f * (7.f * 0.001f);
    check(sampled == std::vector<float>{first, second},
          "a lower-key pet finds type 3 later in the same ground column and refills once before its rider");
    check(random.count() == 0, "the riding refill spends no world random draws");
    check(attachedBeforeRider && pet.body.position[0] == body.position[0] && pet.body.follow[0] == body.follow[0] &&
              pet.body.position[2] == body.position[2] + 655 && pet.body.bob == body.bob && pet.yaw == body.yaw,
          "the early attachment sees the prior rider pose and the late attachment sees the updated pose");
    const BlockType unvisitedLakeBed = [](int32_t x, int32_t, int32_t z) {
        return uint8_t(z > 0 ? 0 : x < 11 ? 1 : 3);
    };
    npcs.stepInto(1.f, world, {player}, 1000, 12, unvisitedLakeBed, frame);
    check(body.ridingEnergy == second, "the first solid ground column stops the scan before subsequent X values");
    const BlockType submerged = [](int32_t, int32_t, int32_t z) {
        return uint8_t(z > 0 ? 2 : 1);
    };
    npcs.stepInto(1.f, world, {player}, 1000, 12, submerged, frame);
    check(body.ridingEnergy == second + 0.05f * 0.001f, "water supplies the same one-shot refill mark");
    body.ridingEnergy = 0.99999f;
    npcs.stepInto(1.f, world, {player}, 1000, 12, dryLakeBed, frame);
    check(body.ridingEnergy == 1.f, "the ground refill caps the owner's meter at one");
    const int64_t lastPetX = pet.body.position[0];
    npcs.stepInto(1.f, world, {player}, 1000, 12, dryLakeBed, frame, {}, {}, [&](int32_t) {
        rider.action = 0;
        body.position[0] += 8192;
    });
    check(pet.body.position[0] == lastPetX, "the late attachment rechecks the rider action after a dismount");
}
}

int main() {
    checkOverlappingStrikes();
    checkRidingRefillOrder();
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
        worldgen::WorldMap world;
        world.add(std::make_shared<worldgen::Zone>(0, 0));
        worldgen::Random random{17};
        Npcs npcs{random};
        Npc quest = fixture(4);
        quest.quest = 191;
        const auto swingingNpc = [](uint64_t key) {
            Npc npc = fixture(key);
            npc.fighter.equipment[MAIN_HAND] = {3, GREATSWORD, 1, 0, 1, 0};
            npc.fighter.action = 57;
            return npc;
        };
        npcs.npcs().push_back(swingingNpc(UINT64_MAX));
        npcs.npcs().push_back(std::move(quest));
        npcs.npcs().push_back(swingingNpc(5));

        Combatant first, second;
        first.id = 1;
        first.health = 0;
        first.buffs = {{3, 0, 1000}};
        second.id = 6;
        Body firstBody, secondBody;
        firstBody.place({10, 10, 3});
        secondBody = firstBody;
        firstBody.position[2] = (int64_t(1) << 53) + 1;
        NpcFrame frame;
        std::vector<uint64_t> visited;
        uint64_t bomb = 0;
        npcs.stepInto(20, world, {{second, secondBody, 0, 0, false, true},
                                 {first, firstBody, 0, 0, false, true}},
            1000, 0, floorBlock, frame, {}, {}, {}, [&](PlayerView &player) {
                visited.push_back(player.fighter.id);
                if (player.fighter.id == first.id) {
                    check(first.health == 0 && first.buffs[0].timeMs == 1000 &&
                          npcs.all()[0].fighter.buffs[0].timeMs == 980,
                          "requests precede living admission at the sender key after earlier actors have advanced");
                    check(random.count() == 1, "the preceding NPC's swing uses the first draw");
                    first.health = maxHealth(first) + 1.f;
                    bomb = npcs.placeBombFixed(player.body.position, -19);
                    npcs.removeQuest(191);
                } else {
                    check(npcs.all().size() == 3 && npcs.all()[1].fighter.id == 5 &&
                          npcs.all()[1].fighter.buffs[0].timeMs == 980,
                          "erasing a future quest actor preserves the successor and rebinds moved survivors");
                    check(first.health == maxHealth(first) && first.buffs[0].timeMs == 980,
                          "the preceding sender already clamped and ran its resident countdown");
                    check(random.count() == 7, "the bomb's five draws are between the two NPC swing draws");
                }
            });
        check(visited == std::vector<uint64_t>{first.id, second.id} && npcs.all().back().fighter.id == bomb &&
              bomb == uint64_t(-2) && npcs.all().back().fighter.health == 500.f &&
              npcs.all().back().fighter.level == -19 && maxHealth(npcs.all().back().fighter) == 0.f &&
              npcs.all().back().body.position[2] == firstBody.position[2],
              "reverse player storage still consumes in signed order and defers a newly inserted earlier bomb");
        worldgen::Random expected{17};
        const float firstPitch = (float)expected.draw() * 0.1f / 32767.f + 1.f;
        for (int draw = 0; draw < 5; ++draw)
            (void)expected.draw();
        const float secondPitch = (float)expected.draw() * 0.1f / 32767.f + 1.f;
        check(frame.sounds.size() == 2 && frame.sounds[0].sound == 17 && frame.sounds[0].pitch == firstPitch &&
              frame.sounds[1].sound == 17 && frame.sounds[1].pitch == secondPitch,
              "swing pitches retain their exact positions in the shared random stream");
    }
    {
        worldgen::WorldMap world;
        world.add(std::make_shared<worldgen::Zone>(0, 0));
        Npcs npcs;
        Npc inert = fixture(2);
        inert.fighter.controller = 6;
        inert.fighter.hitStun = 10;
        inert.thinkMs = 16777216;
        npcs.npcs().push_back(std::move(inert));
        npcs.step(1, world, {}, 1000, 0, floorBlock);
        check(npcs.all().size() == 1 && npcs.all().front().thinkMs == 16777217,
              "the stunned world actor keeps integer behavior time past float precision");
        npcs.npcs().front().fighter.hitStun = 0;
        npcs.step(0, world, {}, 1000, 0, floorBlock);
        check(npcs.all().front().thinkMs == 0, "the admitted behavior accumulator resets even for an inert actor");
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
