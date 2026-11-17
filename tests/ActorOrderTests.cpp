#include "BlockEdits.h"
#include "creatures/ActorMap.h"
#include "creatures/Npcs.h"
#include "net/RemoteCreatures.h"
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

void checkConnectedRidingOrder() {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const auto receive = [&](const Combatant &fighter, int32_t x, float yaw, bool snap) {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(fighter);
        Appearance appearance;
        appearance.box = {0.96f, 0.96f, 2.16f};
        record.setAppearance(appearance);
        record.setPosition(0, int64_t(x) * 65536);
        record.setPosition(1, 30 * 65536);
        record.setPosition(2, 8 * 65536);
        record.setMotion({3, 4, 0}, yaw, {}, {1, 0, 0});
        auto &entry = actors[fighter.id];
        const bool created = !remotes.find(fighter.id);
        remotes.receive(net::CreatureReceipt::fromRecord(fighter.id, record, snap, created));
        remotes.updateLive(fighter.id, record, entry);
    };
    Combatant rider;
    rider.id = 2;
    rider.action = 106;
    Body owner;
    owner.place({20.5, 30.5, 8.0});
    std::copy_n(owner.position, 3, owner.follow);
    owner.yaw = 15.f;
    owner.bob = 0.25f;
    owner.ridingEnergy = 0.9999f;
    const net::RemoteCreatures::Controlled controlled{rider, owner};
    Combatant pet;
    pet.id = UINT64_MAX;
    pet.controller = 5;
    pet.type = 19;
    pet.ownerId = rider.id;
    receive(pet, 9, 20.f, true);
    receive(pet, 11, 30.f, false); // the attachment wins over ordinary interpolation
    Combatant dead = pet;
    dead.id = 1;
    dead.health = 0.f;
    receive(dead, 12, 90.f, true);
    const BlockType water = [](int32_t, int32_t, int32_t) { return uint8_t(2); };
    bool earlierVisitVisible = false;
    int controlledTurns = 0;
    remotes.step(actors, 20, water, {}, &controlled, [&] {
        ++controlledTurns;
        const auto &early = *remotes.find(pet.id);
        earlierVisitVisible = owner.ridingEnergy == 1.f && early.attached &&
            early.live.position[0] == owner.position[0] && early.drawnRotation.z == 15.f &&
            early.live.velocity == glm::vec3(0) && early.live.intent == glm::vec3(0);
        owner.ridingEnergy -= 0.002f * (20.f * 0.001f);
        owner.position[0] += 65536;
        owner.yaw = 80.f;
        owner.bob = 0.125f;
    });
    check(earlierVisitVisible && controlledTurns == 1 && owner.ridingEnergy == 1.f - 0.002f * (20.f * 0.001f),
          "a received negative-key pet refills and attaches before the single rider visit");
    check(!remotes.find(dead.id)->attached && remotes.find(dead.id)->live.position[0] == 12 * 65536,
          "the first pass skips the dead received pet");
    remotes.finishStep(actors, &controlled);
    const auto &attached = *remotes.find(pet.id);
    check(attached.live.position[0] == owner.position[0] && attached.live.follow[0] == owner.follow[0] &&
              attached.live.position[2] == owner.position[2] + 655 && attached.live.bob == owner.bob &&
              attached.drawnRotation.z == owner.yaw && attached.smoothing,
          "the late attachment copies the updated pose, including bob, and keeps the receipt's smoothing flag");
    check(remotes.find(dead.id)->attached && remotes.find(dead.id)->live.position[0] == owner.position[0],
          "the late pass reaches the dead received pet");

    const int64_t priorX = owner.position[0];
    remotes.step(actors, 1, water, {}, &controlled, [&] {
        rider.action = 0;
        owner.position[0] += 65536;
    });
    remotes.finishStep(actors, &controlled);
    check(attached.live.position[0] == priorX && !remotes.find(dead.id)->attached,
          "dismounting during the rider visit prevents the later copies");

    Combatant remoteRider;
    remoteRider.id = 0;
    remoteRider.action = 106;
    receive(remoteRider, 20, 0.f, true);
    receive(remoteRider, 24, 45.f, false);
    pet.ownerId = 0;
    receive(pet, 10, 20.f, false);
    remotes.step(actors, 16, floorBlock, {});
    remotes.finishStep(actors);
    const auto &zeroKey = *remotes.find(0);
    check(attached.attached && attached.live.position[0] == zeroKey.live.position[0] &&
              attached.live.follow[0] == zeroKey.live.follow[0] &&
              attached.live.position[0] != attached.live.follow[0] &&
              attached.drawnRotation == zeroKey.drawnRotation && attached.live.yaw == zeroKey.live.yaw,
          "an owner at remote key zero supplies distinct record and smoothed poses");
}

void checkConnectedActionClock() {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const auto receive = [&](const Combatant &fighter) {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(fighter);
        remotes.receive(net::CreatureReceipt::fromRecord(fighter.id, record, true, true));
        remotes.updateLive(fighter.id, record, actors[fighter.id]);
        return record;
    };
    Combatant input;
    input.id = UINT64_MAX;
    input.action = 36;
    input.actionMs = -17;
    input.hitStun = 20;
    const auto expiredRecord = receive(input);
    auto &expired = actors.at(input.id).fighter;
    expired.buffs = {{1, 0, 20, 0}};
    expired.cooldowns[99] = 20;

    input.id = 1;
    input.action = 49;
    input.actionMs = 0;
    input.hitStun = 200;
    receive(input);
    auto &protectedActor = actors.at(1).fighter;
    protectedActor.buffs = {{1, 0, 21, 0}};
    protectedActor.cooldowns[49] = 10;
    const int32_t fullCooldown = cooldownLength(protectedActor, 49);

    input.id = 2;
    input.actionMs = -12;
    input.hitStun = -20;
    input.actionLockMs = 21;
    receive(input);
    auto &locked = actors.at(2).fighter;
    const int32_t lockedClock = duration(locked, 49) + 1 + 20;

    input.id = 3;
    input.controller = 6;
    input.actionMs = 110;
    input.hitStun = 21;
    input.actionLockMs = 0;
    receive(input);
    const auto &inert = actors.at(3).fighter;

    Combatant player;
    Body body;
    const net::RemoteCreatures::Controlled controlled{player, body};
    remotes.step(actors, 20, floorBlock, {}, &controlled, [&] {
        check(expired.action == 36 && expired.actionMs == -17 && expired.hitStun == 0 && expired.buffs.empty() &&
                  expired.cooldowns.at(99) == 0 && protectedActor.actionMs == 0,
              "stun equality and the negative active clock are visible at the earlier signed turn only");
    });
    check(protectedActor.action == 49 && protectedActor.hitStun == -20 && protectedActor.actionMs == 20 &&
              protectedActor.buffs.front().timeMs == 1 && protectedActor.cooldowns.at(49) == fullCooldown,
          "a surviving buff clears stun before interruption and a fresh cooldown follows decay");
    check(locked.actionMs == lockedClock && locked.actionLockMs == 1 && locked.hitStun == -40 &&
              inert.hitStun == 1 && inert.action == 0 && inert.actionMs == 0 && inert.cooldowns.empty(),
          "the positive lock overrides the clock and controller six skips progression after stun interruption");

    remotes.updateLive(UINT64_MAX, expiredRecord, actors.at(UINT64_MAX));
    check(expired.hitStun == 0 && expired.buffs.empty(), "a frame without a receipt retains the advanced state");
    remotes.step(actors, 3, floorBlock, {});
    check(protectedActor.actionMs == 23 && protectedActor.cooldowns.at(49) == fullCooldown - 3 &&
              protectedActor.buffs.empty(), "a second world piece advances the resident state by its own duration");
}

void checkConnectedActionResources() {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const auto receive = [&](uint64_t id, const Combatant &fighter, glm::vec3 intent = {}) -> Combatant & {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(fighter);
        record.setMotion({}, 0.f, {}, intent);
        record.setPosition(0, int64_t(id) * 4 * 65536);
        record.setPosition(2, 8 * 65536);
        remotes.receive(net::CreatureReceipt::fromRecord(id, record, true, true));
        auto &state = actors[id];
        remotes.updateLive(id, record, state);
        return state.fighter;
    };
    Combatant input;
    input.action = 24;
    input.actionMs = 10000;
    input.mana = 0.2f;
    input.mode = LENS;
    auto &charging = receive(1, input);
    charging.buffs = {{3, 0, 21, 0}, {10, 0, 21, 0}};

    input.action = 8;
    input.mode = 0;
    input.blockGauge = 0.5f;
    input.equipment[OFF_HAND].type = 3;
    input.equipment[OFF_HAND].subtype = SHIELD;
    const auto &blocking = receive(2, input);

    input.action = 71;
    input.actionMs = 40;
    input.dodgeMs = 1;
    input.mana = 0.7f;
    input.charge = 0.25f;
    input.characterClass = 4;
    input.specialization = 1;
    const auto &assassin = receive(3, input);
    input.characterClass = 1;
    const auto &ordinary = receive(4, input);

    input.action = 83;
    input.mode = LENS | GLIDER;
    input.dodgeMs = 0;
    const auto &sitting = receive(5, input, {1, 0, 0});
    input.action = 36;
    input.hitStun = 21;
    input.mode = CLIMB | GLIDER;
    const auto &stunned = receive(6, input);

    remotes.step(actors, 20, floorBlock, {});
    check(charging.mana == 0.2f + (20.f * 0.00025f) && charging.charge == charging.mana && charging.stealth == 1.f,
          "buff 3 contributes to mana before surviving buff 10 samples it during charging");
    check(blocking.blockGauge == 0.5f - 20.f / 1200.f, "the shield halves blocking drain in a received actor turn");
    check(assassin.action == 0 && ordinary.action == 0 && assassin.actionMs == 60 && ordinary.actionMs == 60 &&
              assassin.dodgeMs == 0 && ordinary.dodgeMs == 0 && assassin.charge == 0.25f && ordinary.charge == 0.f,
          "roll cancellation uses the old nonzero timer, keeps the clock and keeps only the assassin's charge");
    check(sitting.action == 0 && sitting.actionMs == 60 && sitting.mode == 0 && sitting.stealth == 0.f,
          "intent ends sitting without resetting the clock and clears the lens bit before the meter");
    check(stunned.action == 0 && stunned.actionMs == 20 && stunned.hitStun == 1 &&
              stunned.charge == 0.f && (stunned.mode & (CLIMB | GLIDER)) == 0,
          "late movement stun clears charge and movement toggles without interrupting the clock again");

    input = {};
    input.mana = 1.25f;
    input.charge = 2.f;
    const auto &paused = receive(7, input);
    remotes.step(actors, 0, floorBlock, {});
    check(paused.actionMs == 0 && paused.mana == 1.f && paused.charge == 1.f,
          "a zero-duration connected turn still applies the resource ceilings");
}
}

int main() {
    checkConnectedActionClock();
    checkConnectedActionResources();
    checkOverlappingStrikes();
    checkRidingRefillOrder();
    checkConnectedRidingOrder();
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
                            [&](uint64_t key, uint64_t) { visited.push_back(key); });
        check(visited == std::vector<uint64_t>{minimum, UINT64_MAX, 0, 3},
              "protected received-state traversal merges the controlled actor by signed key");
        received.clear();
        visited.clear();
        visitReceivedActors(received, 0, [&] { visited.push_back(0); }, [&](uint64_t, uint64_t) {});
        check(visited == std::vector<uint64_t>{0}, "an empty received collection still advances the controlled actor once");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
