#include "BlockEdits.h"
#include "SoundRules.h"
#include "StaticStates.h"
#include "WorldItems.h"
#include "creatures/Abilities.h"
#include "creatures/ActorMap.h"
#include "creatures/Npcs.h"
#include "creatures/Strike.h"
#include "net/RemoteCreatures.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

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
    owner.walk = {7.f, 0.5f};
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
            early.live.velocity == glm::vec3(0) && early.live.intent == glm::vec3(0) &&
            early.live.walk.phase == 7.f && early.live.walk.amplitude == 0.5f;
        owner.ridingEnergy -= 0.002f * (20.f * 0.001f);
        owner.position[0] += 65536;
        owner.yaw = 80.f;
        owner.bob = 0.125f;
        owner.walk = {8.f, 0.75f};
    });
    check(earlierVisitVisible && controlledTurns == 1 && owner.ridingEnergy == 1.f - 0.002f * (20.f * 0.001f),
          "a received negative-key pet refills and attaches before the single rider visit");
    check(!remotes.find(dead.id)->attached && remotes.find(dead.id)->live.position[0] == 12 * 65536,
          "the first pass skips the dead received pet");
    remotes.finishStep(actors, &controlled);
    const auto &attached = *remotes.find(pet.id);
    check(attached.live.position[0] == owner.position[0] && attached.live.follow[0] == owner.follow[0] &&
              attached.live.position[2] == owner.position[2] + 655 && attached.live.bob == owner.bob &&
              attached.drawnRotation.z == owner.yaw && attached.smoothing &&
              attached.live.walk.phase == 8.f && attached.live.walk.amplitude == 0.75f,
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
    input.id = uint64_t(-2);
    input.action = 36;
    input.actionMs = -17;
    input.hitStun = 20;
    const auto expiredRecord = receive(input);
    auto &expiredState = actors.at(input.id);
    auto &expired = expiredState.fighter;
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
    player.id = UINT64_MAX;
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

    remotes.updateLive(expired.id, expiredRecord, expiredState);
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

void checkConnectedMovementSounds() {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const auto receive = [&](uint64_t id, const net::CreatureRecord &record) {
        const bool created = !remotes.find(id);
        remotes.receive(net::CreatureReceipt::fromRecord(id, record, created, created));
        remotes.updateLive(id, record, actors[id]);
    };
    const auto recordAt = [](int32_t x, float z, glm::vec3 velocity, int32_t rollMs = 0,
                             glm::vec3 intent = glm::vec3(0)) {
        auto record = net::CreatureRecord::nativeDefaults();
        Combatant fighter;
        fighter.type = 109;
        fighter.dodgeMs = rollMs;
        record.setFighter(fighter);
        Appearance appearance;
        appearance.box = {0.8f, 0.8f, 2.f};
        record.setAppearance(appearance);
        record.setPosition(0, int64_t(x) * 65536);
        record.setPosition(2, int64_t(z * 65536.f));
        record.setMotion(velocity, 0.f, {}, intent);
        return record;
    };
    const BlockType groundAndWater = [](int32_t x, int32_t, int32_t z) -> uint8_t {
        if (x >= 24)
            return z < 10 ? 2 : 0;
        return z <= 0 ? 3 : 0;
    };
    Combatant player;
    player.id = UINT64_MAX;
    Body body;
    const net::RemoteCreatures::Controlled controlled{player, body};
    const auto walkingRecord = recordAt(8, 2.1f, {8, 0, 0});
    for (int piece = 0; piece < 17; piece++) {
        receive(1, walkingRecord);
        remotes.step(actors, 16, groundAndWater, {}, &controlled);
    }
    const auto &walker = *remotes.find(1);
    check(walker.live.walk.phase > 3.f && walker.live.walk.phase < 3.14159265f,
          "the ground phase approaches the crossing over the simulated pieces");
    receive(1, walkingRecord);
    const auto falling = recordAt(2, 2.1f, {0, 0, -10}, 510);
    receive(uint64_t(-2), falling);
    auto swimmer = recordAt(8389632, 3.f, {0, 0, -10});
    swimmer.setPosition(0, swimmer.position(0) + 1);
    receive(2, swimmer);

    constexpr std::array<int, 3> samples{32767, 5, 16384};
    size_t draws = 0;
    const std::function<int()> draw = [&] {
        const size_t next = draws++;
        return next < samples.size() ? samples[next] : 0;
    };
    NpcFrame frame;
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    remotes.step(actors, 16, groundAndWater, {}, &controlled, [&] {
        check(draws == 1 && frame.sounds.size() == 2 && frame.sounds[0].sound == 32 && frame.sounds[1].sound == 26,
              "the negative-key actor's landing draw and roll cue precede the controlled turn");
    }, &effects);
    check(draws == 3 && frame.sounds.size() == 4 && frame.bursts.size() == 1,
          "landing, footfall and splash each draw once, while the roll draws nothing");
    if (!frame.bursts.empty())
        check(frame.bursts.front().position[0] == swimmer.position(0), "splash visuals retain the low fixed-point position bits");
    if (frame.sounds.size() == 4) {
        const float seconds = 16.f * 0.001f;
        const float displacement = (-10.f - 30.f * seconds) * seconds;
        const int64_t blockedZ = falling.position(2) + int64_t(double(displacement) * 65536.0);
        check(frame.sounds[0].position.z == float(blockedZ) * (1.f / 65536.f) &&
                  frame.sounds[0].volume == 0.5f && frame.sounds[0].pitch == sound::randomPitch(samples[0], 0.2f, 1.f),
              "the landing sound retains the blocked z move and exact pitch");
        check(frame.sounds[2].sound == 35 && frame.sounds[2].pitch == 1.f &&
                  frame.sounds[3].sound == 31 && frame.sounds[3].pitch == sound::randomPitch(samples[2], 1.f, 0.9f),
              "the footfall chooses its type-3 sound before the later swimmer's splash");
    }
    check(walker.live.walk.phase > 3.14159265f, "footfalls read the updated resident ground phase");

    receive(3, recordAt(12, 2.1f, {7, 0, 0}, 1, {1, 0, 0}));
    remotes.step(actors, 16, groundAndWater, {});
    const auto &oldRoll = remotes.find(3)->live;
    check(oldRoll.dodgeBeforeMs == 1 && oldRoll.dodgeMs == 0 && oldRoll.yaw == 0.f,
          "the roll's final millisecond still gates this turn's movement facing");
    remotes.step(actors, 16, groundAndWater, {});
    check(oldRoll.yaw < -89.f && oldRoll.yaw > -91.f, "movement can turn on the following piece");

    const int64_t support[3]{-65536, -1, 32768};
    check(footstepCell(support, 2.f) == glm::ivec3(-2, -1, -2), "an aligned negative support sample chooses the previous block");
}

void checkConnectedActionEffects() {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const glm::vec3 aim(0.25f, -0.125f, 0.5f);
    constexpr int64_t origin = 8389632ll * 65536 + 1;
    constexpr uint64_t heavyKey = uint64_t(-2);
    const auto receive = [&](uint64_t id, const Combatant &fighter) -> Combatant & {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(fighter);
        record.setPosition(0, origin + int64_t(id == heavyKey ? 0 : id) * 65536);
        record.setPosition(2, 8 * 65536);
        std::memcpy(record.bytes.data() + 0x150, &aim, sizeof aim);
        remotes.receive(net::CreatureReceipt::fromRecord(id, record, true, true));
        auto &state = actors[id];
        remotes.updateLive(id, record, state);
        return state.fighter;
    };

    Combatant input;
    input.action = 3;
    input.mana = 0.75f;
    input.charge = 0.2f;
    input.hitStun = 21;
    input.equipment[MAIN_HAND].type = 3;
    input.equipment[MAIN_HAND].subtype = LONGSWORD;
    const auto &heavy = receive(heavyKey, input);

    input = {};
    input.action = 5;
    input.characterClass = 4;
    input.specialization = 1;
    input.mana = 0.9f;
    input.charge = 0.4f;
    auto &assassin = receive(1, input);
    assassin.buffs = {{11, 0, 100, 10}};

    input = {};
    input.action = 31;
    input.characterClass = 3;
    input.specialization = 1;
    input.mana = 0.8f;
    input.multipliers[1] = 1.f / attackSpeed(input);
    const int32_t wind = phases(input, 31).windUp;
    check(wind == 1600, "the channel fixture uses a unit timing product");
    input.actionMs = wind - 20;
    const auto &release = receive(2, input);
    input.actionMs = wind;
    auto &lower = receive(3, input);
    lower.buffs = {{9, 0, 25, 99}};
    input.actionMs = wind + 1;
    auto &inside = receive(4, input);
    inside.buffs = {{9, 0, 25, 99}};

    input.action = 88;
    input.actionMs = phases(input, 88).windUp;
    receive(5, input);
    input.action = 30;
    input.actionMs = 149;
    receive(6, input);

    NpcFrame frame;
    int draws = 0;
    const std::function<int()> draw = [&] {
        if (draws == 5)
            check(frame.bursts.size() == 2, "the deterministic explosion visual precedes the shared swing draw");
        ++draws;
        return 32767;
    };
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    Combatant local;
    local.id = UINT64_MAX;
    Body body;
    const net::RemoteCreatures::Controlled controlled{local, body};
    remotes.step(actors, 20, floorBlock, {}, &controlled, [&] {
        check(draws == 1 && frame.sounds.size() == 1 && frame.sounds[0].sound == 17 &&
                  frame.sounds[0].pitch == 1.1f && heavy.spentPower == 0.2f &&
                  heavy.mana == ((0.75f - 0.1f) - 0.2f) - 20.f * 0.00005f && heavy.action == 0,
              "the earlier actor's start sound and both debits precede live stun interruption and the local turn");
    }, &effects);

    std::vector<int32_t> sequence;
    for (const auto &event : frame.sounds)
        sequence.push_back(event.sound);
    const std::vector<int32_t> expected{17, 36, 41, 15, 15, 15, 81, 15};
    if (sequence != expected || draws != 6) {
        std::printf("Action cue sequence with %d draws and anticipation %d:", draws, wind);
        for (const int32_t sound : sequence)
            std::printf(" %d", sound);
        std::printf("\n");
    }
    check(sequence == expected && draws == 6,
          "the 200 ms channel, deterministic releases, shared and repeated swings preserve their draw order");
    check(assassin.action == 5 && assassin.dodgeMs == 580 && assassin.spentPower == 0.2f &&
              assassin.criticalLatched && assassin.buffs.empty(),
          "common preparation consumes type 11 and starts the class-four roll before movement");
    check(release.actionMs == wind && release.mana == 0.8f && release.spentPower == 0.f &&
              lower.actionMs == wind + 20 && lower.mana == 0.8f && lower.spentPower == 0.1f &&
              lower.buffs.size() == 1 && lower.buffs.front().timeMs == 5 && inside.buffs.empty(),
          "projected equality releases without spending and live equality keeps type 9 outside the saved window");
    check(frame.buffs.size() == 2 && frame.buffs[0].source == 1 && frame.buffs[0].target == 1 &&
              frame.buffs[0].record.kind == 11 && frame.buffs[0].record.source == 0 && frame.buffs[0].record.timeMs == 0 &&
              frame.buffs[1].source == 4 && frame.buffs[1].target == 4 && frame.buffs[1].record.kind == 9 &&
              frame.buffs[1].record.source == 0 && frame.buffs[1].record.timeMs == 0,
          "the resident replacements publish self-addressed zero-duration records in signed turn order");
    check(frame.bursts.size() == 2 && frame.bursts[0].position[0] == origin + 2 * 65536 + 16384 &&
              frame.bursts[0].position[1] == -8192 && frame.bursts[0].count == 20 && frame.bursts[0].spread == 8.f &&
              frame.bursts[1].position[0] == origin + 5 * 65536 && frame.bursts[1].count == 80 && frame.bursts[1].spread == 20.f,
          "staff visuals use the live aim offset and explosion visuals use the actor's exact fixed position");
}

struct MeleeFixture {
    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    static constexpr int64_t x = 10 * 65536 + 1;
    static constexpr int64_t y = 20 * 65536;
    static constexpr int64_t z = 8 * 65536;

    Combatant &receive(uint64_t key, const Combatant &input, int64_t atX = x, int64_t atY = y + 65536) {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(input);
        Appearance appearance;
        appearance.box = {1.f, 1.f, 2.f};
        record.setAppearance(appearance);
        record.setPosition(0, atX);
        record.setPosition(1, atY);
        record.setPosition(2, z);
        remotes.receive(net::CreatureReceipt::fromRecord(key, record, true, !remotes.find(key)));
        auto &actor = actors[key];
        remotes.updateLive(key, record, actor);
        return actor.fighter;
    }
};

void checkConnectedMeleeOrder() {
    MeleeFixture world;
    Combatant input;
    input.health = 50;
    input.characterClass = 4;
    input.combo = 7;
    input.comboElapsedMs = 4096;
    input.charge = .4f;
    input.mode = 0x20;
    input.action = 1;
    const auto timing = phases(input, 1);
    input.actionMs = timing.windUp + timing.active / 2;
    auto &striker = world.receive(uint64_t(-5), input, world.x, world.y);

    Combatant local;
    local.id = uint64_t(-4);
    Body localBody;
    localBody.size = {1, 1, 2};
    localBody.place({10, 21, 8});
    const net::RemoteCreatures::Controlled controlled{local, localBody};

    Combatant target;
    target.health = 50;
    target.controller = 1;
    target.mode = 0x80;
    world.receive(uint64_t(-3), target);
    target.mode = 0;
    target.action = 8;
    target.blockGauge = .95f;
    target.mana = .3f;
    auto &guard = world.receive(uint64_t(-2), target);
    target.action = 0;
    target.characterClass = 4;
    target.specialization = 1;
    target.dodgeMs = 530;
    auto &negativeRoll = world.receive(UINT64_MAX, target);
    auto &positiveRoll = world.receive(8, target);
    striker.deferred = {8, 99, UINT64_MAX};
    target.dodgeMs = 0;
    auto &ward = world.receive(2, target);
    ward.buffs.push_back({6, 100.f, 200, 0});
    target.hitStun = -3001;
    target.action = 84;
    target.type = 2;
    auto &ordinary = world.receive(3, target);
    world.receive(6, target, world.x + 2 * 65536);
    target.health = 0;
    world.receive(4, target);

    // Local rejection, protection, guard, ward, then critical ordinary plus voice and stun.
    constexpr std::array samples{32767, 11, 32767, 19, 101, 32767, 29, 102, 32767, 37, 103,
                                 0, 16384, 104, 105, 0};
    size_t draws = 0;
    const std::function<int()> draw = [&] {
        const size_t next = draws++;
        return next < samples.size() ? samples[next] : 32767;
    };
    const BlockType withWall = [](int32_t x, int32_t, int32_t z) { return uint8_t(x == 11 || z <= 0 ? 1 : 0); };
    NpcFrame frame;
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    world.remotes.step(world.actors, 0, withWall, {}, &controlled, [&] {
        check(draws == samples.size() && striker.hitLanded,
              "the signed attacker turn completes its rolls before the controlled actor advances");
    }, &effects);

    std::vector<std::pair<uint64_t, uint8_t>> emitted;
    for (const auto &hit : frame.emittedHits)
        emitted.emplace_back(hit.target, hit.kind);
    const std::vector<std::pair<uint64_t, uint8_t>> expected{{UINT64_MAX, HIT_COUNTER}, {8, HIT_COUNTER},
        {uint64_t(-3), HIT_IMMUNE}, {uint64_t(-2), HIT_BLOCKED}, {2, HIT_ABSORBED}, {3, HIT_DAMAGE}};
    check(emitted == expected, "old roll feedback is signed and precedes protection, guard, ward and ordinary hit");
    check(striker.struck == std::vector<uint64_t>{local.id, uint64_t(-3), uint64_t(-2), 2, 3} &&
              striker.deferred == std::vector<uint64_t>{UINT64_MAX, 8},
          "the local target occupies a struck slot while rolls defer anew and wall and dead targets stay out");
    check(draws == samples.size() && striker.combo == 7 && striker.comboElapsedMs == 4096 && striker.stealth == .8f,
          "local rejection spends its caller draws and occupies the first slot without a reward draw");
    check(frame.buffs.size() == 2 && frame.buffs[0].source == UINT64_MAX && frame.buffs[1].source == 8 &&
              frame.buffs[0].record.source == 0 && frame.buffs[1].record.timeMs == 30000 &&
              negativeRoll.mana == .55f && positiveRoll.mana == .55f,
          "both live rolling targets get the type-11 benefit before later action phases");
    check(guard.mana == 1.f && guard.charge == .95f && guard.blockGauge < 0 && ordinary.health == 50.f &&
              ordinary.hitStun == -3001 && ordinary.hitFlash == 1.f,
          "guard mutates live resources and the ordinary result flashes without applying damage or stun");
    if (emitted == expected) {
        const auto &hit = frame.emittedHits.back();
        check(hit.position == std::array<int64_t, 3>{world.x, world.y + 65536, world.z} && hit.critical &&
                  hit.area && hit.stun == 3000 && hit.knockBack.z == 10.f,
              "the ordinary record keeps raw position, charged critical and stun payload");
    }
    std::vector<int32_t> sounds;
    for (const auto &sound : frame.sounds)
        sounds.push_back(sound.sound);
    check(sounds == std::vector<int32_t>{24, 24, 91, 6, 62, 23}, "defensive, ordinary, voice and stun audio order");
    if (sounds.size() == 6)
        check(frame.sounds[0].pitch == 101.f * .3f / 32767.f + 1.f &&
                  frame.sounds[3].pitch == 104.f * .2f / 32767.f + .9f &&
                  frame.sounds[4].pitch == 105.f * .1f / 32767.f + 1.f,
              "the order-sensitive audio draws use their own samples");

    frame.reset();
    striker.action = 0;
    striker.struck.push_back(8);
    world.remotes.step(world.actors, 0, withWall, {}, &controlled, {}, &effects);
    check(striker.struck.empty() && striker.deferred.empty() && striker.hitLanded && draws == samples.size() &&
              frame.emittedHits.size() == 1 && frame.emittedHits[0].target == UINT64_MAX &&
              negativeRoll.mana == .8f && positiveRoll.mana == .55f,
          "idle cleanup excludes struck IDs, clears both sets and preserves the hit latch without drawing");
}

void checkConnectedMeleeWindow() {
    MeleeFixture world;
    Combatant input;
    input.health = 50;
    input.mode = 0x20;
    input.action = 1;
    input.combo = 400;
    input.multipliers[1] = 8.f;
    const auto timing = phases(input, 1);
    input.actionMs = timing.windUp + timing.active / 2;
    auto &striker = world.receive(1, input, world.x, world.y);
    Combatant receiver;
    receiver.health = 50;
    receiver.controller = 6;
    auto &target = world.receive(2, receiver);
    NpcFrame frame;
    int draws = 0;
    const std::function<int()> draw = [&] { ++draws; return 32767; };
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    const int32_t elapsed = timing.windUp + timing.active - input.actionMs;
    Combatant reset = input;
    reset.combo = 0;
    const auto changed = phases(reset, 1);
    const int32_t scaled = int32_t(float(input.actionMs) * (attackSpeed(input) / attackSpeed(reset)));
    check(changed.windUp < scaled && scaled <= changed.windUp + changed.active / 2,
          "the fixture's post-reset clock would be admitted by fresh bounds");
    world.remotes.step(world.actors, elapsed, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 0 && striker.combo == 0 && striker.actionMs == scaled + elapsed,
          "post-reset clock outside the saved window draws nothing even if admitted by the new timing");

    input.combo = 0;
    input.comboElapsedMs = 700;
    input.multipliers[1] = 1.f;
    input.actionMs = phases(input, 1).windUp + 1;
    world.receive(1, input, world.x, world.y);
    world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 3 && frame.emittedHits.size() == 1 && striker.combo == 1 && striker.comboElapsedMs == 0 &&
              striker.mana == 0 && target.health == 50.f,
          "a first accepted target increments combo without the local-owner mana draws");
    net::CreatureRecord baseline;
    world.remotes.writeLive(1, baseline, world.actors.at(1));
    check(baseline.fighter().combo == 1 && baseline.fighter().comboElapsedMs == 0,
          "locally advanced combo fields reach the receiving baseline");
    world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 3 && frame.emittedHits.size() == 1, "the resident struck set suppresses repeat draws across pieces");
}

void checkConnectedMeleeFamily() {
    for (const int32_t action : {2, 3, 4, 6, 7}) {
        MeleeFixture world;
        Combatant input;
        input.action = action;
        input.health = 50;
        const auto timing = phases(input, action);
        input.actionMs = timing.windUp + timing.active / 2;
        const float impulse = (action == 2 ? 6.f : 3.f) / attackSpeed(input);
        auto &attacker = world.receive(1, input, world.x, world.y);
        Combatant defender;
        defender.controller = 6;
        defender.health = 50;
        defender.multipliers[3] = 0;
        const auto &target = world.receive(2, defender);
        NpcFrame frame;
        int draws = 0;
        const std::function<int()> draw = [&] { draws++; return 32767; };
        const net::RemoteCreatures::StepEffects effects{frame, draw};
        world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
        check(draws == 3 && attacker.combo == 1 && target.health == 50 && frame.emittedHits.size() == 1,
              "each ordinary received melee admits the saved upper bound and avoids local-owner draws");
        if (!frame.emittedHits.empty()) {
            const auto &hit = frame.emittedHits.front();
            check(hit.amount == .75f && hit.knockBack == glm::vec3(0, impulse, .25f * impulse) &&
                      !hit.critical && !hit.area,
                  "ordinary connected melee uses the held damage coefficient and live impulse selection");
        }
        if (action == 3 || action == 4) {
            input.actionMs = 0;
            input.mana = .75f;
            world.receive(1, input, world.x, world.y);
            world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
            check(attacker.spentPower == .1f && attacker.mana == .75f - .1f && draws == 3,
                  "actions three and four spend the fixed nominal resource at clock zero");
        }
    }

    MeleeFixture world;
    Combatant input;
    input.action = 5;
    input.characterClass = 4;
    input.specialization = 1;
    input.health = 50;
    input.combo = 7;
    input.mana = .6f;
    input.charge = .4f;
    auto &attacker = world.receive(1, input, world.x, world.y);
    attacker.buffs.push_back({11, 2.f, 100, 18});
    Combatant defender;
    defender.controller = 6;
    defender.health = 50;
    defender.multipliers[3] = 0;
    world.receive(2, defender);
    NpcFrame frame;
    int draws = 0;
    const std::function<int()> draw = [&] { draws++; return 32767; };
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 0 && attacker.criticalLatched && attacker.spentPower == .2f && attacker.mana == 0.f &&
              attacker.dodgeMs == 600 && frame.buffs.size() == 1 && frame.buffs[0].source == 1 &&
              frame.buffs[0].record.source == 0 && frame.buffs[0].record.timeMs == 0,
          "action five's startup replaces type eleven and records half the positive pending debit");

    const auto timing = phases(attacker, 5);
    const int32_t secondPulse = timing.windUp + timing.active / 3;
    attacker.actionMs = secondPulse - 1;
    attacker.struck = {91};
    attacker.deferred = {2};
    attacker.hitLanded = true;
    world.remotes.step(world.actors, 1, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 0 && frame.emittedHits.empty() && attacker.struck.empty() && attacker.deferred.empty() &&
              attacker.hitLanded && attacker.swingCount == 7 && attacker.actionMs == secondPulse,
          "a projected action-five pulse clears both sets before expiry and keeps the hit latch");
    world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 0 && frame.emittedHits.empty(), "the saved pulse is strict at its lower bound");
    attacker.actionMs++;
    world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
    check(draws == 2 && frame.emittedHits.size() == 1 && attacker.combo == 8,
          "the latched action-five hit uses only jitter and ordinary pitch draws");
    if (!frame.emittedHits.empty()) {
        const auto &hit = frame.emittedHits.front();
        const float amount = .5f * ((.2f * .2f) * 5.f + 1.f) * .75f * 2.f;
        check(hit.amount == amount && hit.critical && hit.area && hit.knockBack.z == 1.f,
              "action five retains the half coefficient, constant impulse and charged critical result");
    }

    int speedQueries = 0;
    auto &speed = itemBonuses().speed;
    auto previous = std::exchange(speed, [&](const Item &) { speedQueries++; return 0.f; });
    Combatant equipped;
    equipped.action = 5;
    equipped.equipment[MAIN_HAND].type = 3;
    Body body, target;
    strikeInput(equipped, body, target, draw);
    check(speedQueries == 0, "constant impulse skips the weapon speed query");
    equipped.action = 4;
    strikeInput(equipped, body, target, draw);
    check(speedQueries == 1, "divided impulse samples the live weapon speed once");
    speed = std::move(previous);
}

void checkConnectedMeleeActionsNineThroughTwentyOne() {
    const float rank = 1.f - 1.f / (1.f + 8.f * .1f);
    const std::array impulses{6.f, 2.f, 10.f, 2.f, 6.f, 6.f, 6.f, 2.f, 6.f, 3.f, 3.f, 6.f, rank * 30.f + 5.f};
    for (int32_t action = 9; action <= 21; ++action) {
        MeleeFixture world;
        Combatant input;
        input.action = action;
        input.health = 50;
        input.skills[6] = 8;
        const auto timing = phases(input, action);
        input.actionMs = action == 11 ? 1 : timing.windUp + timing.active / 2;
        auto &attacker = world.receive(1, input, world.x, world.y);
        float impulse = impulses[action - 9];
        if (action != 11 && action != 17 && action != 20 && action != 21)
            impulse /= attackSpeed(attacker);
        Combatant defender;
        defender.controller = 6;
        defender.health = 50;
        defender.multipliers[3] = 0;
        const auto &target = world.receive(2, defender);
        int draws = 0;
        const std::function<int()> draw = [&] { ++draws; return 32767; };
        NpcFrame frame;
        const net::RemoteCreatures::StepEffects effects{frame, draw};
        world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
        check(draws == 3 && attacker.combo == 1 && target.health == 50 && frame.emittedHits.size() == 1,
              "each action 9 through 21 emits one accepted hit in the connected receiver turn");
        if (!frame.emittedHits.empty()) {
            const auto &hit = frame.emittedHits.front();
            const float amount = action == 11 ? 0.f : action == 21 ? .1f * .75f : .75f;
            const bool matches = hit.amount == amount && hit.knockBack == glm::vec3(0, impulse, .25f * impulse) &&
                                 hit.area == (action == 11);
            if (!matches)
                std::printf("Action %d amount %g / %g, impulse %g / %g\n", action, hit.amount, amount, hit.knockBack.y, impulse);
            check(matches, "physical coefficient, action-eleven zero baseline and all impulse selectors match");
        }
    }

    for (const int32_t action : {17, 20, 21}) {
        MeleeFixture world;
        Combatant input;
        input.action = action;
        input.characterClass = 4;
        input.specialization = 1;
        input.mana = .5f;
        input.charge = action == 21 ? 0.f : .25f;
        auto &attacker = world.receive(1, input, world.x, world.y);
        if (action != 21)
            attacker.deferred = {1}; // the early roll permits live self feedback
        NpcFrame frame;
        int draws = 0;
        const std::function<int()> draw = [&] { ++draws; return 32767; };
        const net::RemoteCreatures::StepEffects effects{frame, draw};
        world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
        if (action == 21) {
            check(attacker.mana == .5f && attacker.spentPower == 1.f && frame.buffs.empty() && draws == 0,
                  "action twenty-one sets the recorded resource to one without a mana debit or draw");
        } else {
            check(attacker.dodgeMs == 600 && attacker.criticalLatched && attacker.mana == 0.f &&
                      attacker.spentPower == .25f && attacker.buffs.empty() && draws == 0 &&
                      frame.buffs.size() == 2 && frame.buffs[0].record.timeMs == 30000 &&
                      frame.buffs[1].record.timeMs == 0 && frame.emittedHits.size() == 1 &&
                      frame.emittedHits[0].kind == HIT_COUNTER,
                  "startup roll precedes feedback, then pending resource overrides the mana snapshot without clearing critical");
        }
    }
}

void checkConnectedMeleePulseAndEffect() {
    MeleeFixture world;
    Combatant input;
    input.action = 11;
    input.characterClass = 4;
    input.health = 50;
    input.mana = .75f;
    input.charge = .5f;
    input.combo = 4;
    const int32_t pulse = phases(input, 11).active / 3;
    input.actionMs = pulse - 1;
    auto &attacker = world.receive(1, input, world.x, world.y);
    attacker.struck = {3};
    attacker.deferred = {3};
    attacker.hitLanded = true;
    Combatant defender;
    defender.controller = 6;
    defender.health = 50;
    defender.multipliers[3] = 0;
    auto &target = world.receive(3, defender);
    NpcFrame frame;
    constexpr std::array samples{127, 32767, 16384, 277};
    size_t draws = 0;
    const std::function<int()> draw = [&] {
        if (draws == 0)
            check(attacker.struck.empty() && attacker.deferred.empty() && attacker.swingCount == 4 && attacker.hitLanded,
                  "action-eleven pulse clears before its sound draw and retains the hit latch");
        const size_t i = draws++;
        return i < samples.size() ? samples[i] : 32767;
    };
    const net::RemoteCreatures::StepEffects effects{frame, draw};
    world.remotes.step(world.actors, 2, floorBlock, {}, nullptr, {}, &effects);
    check(draws == samples.size() && frame.sounds.size() == 2 && frame.emittedHits.size() == 1 &&
              attacker.spentPower == .25f && attacker.charge == 0.f && attacker.combo == 5 &&
              attacker.stealth == .25f - 2.f * .0005f && attacker.actionMs == pulse + 1,
          "earlier saved pulse window consumes full pending, deals one charged hit and avoids counter feedback");
    if (frame.sounds.size() == 2 && frame.emittedHits.size() == 1) {
        check(frame.sounds[0].sound == 15 && frame.sounds[0].pitch == sound::randomPitch(127, .25f, 1.f) &&
                  frame.sounds[1].sound == 5 && frame.sounds[1].pitch == sound::randomPitch(277, .2f, .9f) &&
                  frame.emittedHits[0].amount == (.25f * .25f * 5.f) * (1.25f - 16384.f * .5f / 32767.f),
              "pulse, critical, variation and impact draws keep their exact order");
    }

    input.action = 17;
    input.actionMs = phases(input, 17).windUp + 101;
    input.charge = 0.f;
    world.receive(1, input, world.x, world.y);
    attacker.struck.clear();
    target.buffs.push_back({DAMAGE_OVER_TIME, .125f, 100, 111});
    Combatant local;
    local.id = 2;
    Body localBody;
    localBody.place({0, 0, 8});
    const net::RemoteCreatures::Controlled controlled{local, localBody};
    frame.reset();
    world.remotes.step(world.actors, 20, floorBlock, {}, &controlled, [&] {
        check(target.buffs.size() == 2 && target.buffs.front().source == 111 && target.buffs.back().source == 1 &&
                  target.buffs.back().timeMs == 3000 && frame.buffs.size() == 1 && frame.buffs[0].target == 3,
              "action seventeen publishes a new trailing buff before the next signed actor turn");
    }, &effects);
    check(frame.emittedHits.size() == 1 && target.health == 50 && target.buffs.size() == 2 &&
              target.buffs.back().timeMs == 2980 && target.buffs.back().value == frame.emittedHits[0].amount * .1f,
          "the later target ages the appended damage-over-time effect without applying periodic damage locally");
}

void checkStaffStatSelectors() {
    Combatant actor;
    actor.action = 32;
    actor.controller = 3;
    actor.powerTier = 8;
    actor.appearanceFlags = 0x18;
    actor.multipliers[2] = .625f;
    auto &primary = actor.equipment[MAIN_HAND];
    auto &secondary = actor.equipment[OFF_HAND];
    primary.type = secondary.type = 3;
    primary.subtype = 5;
    secondary.subtype = 12;
    primary.level = secondary.level = 1;
    secondary.rarity = 4;
    check(magicBase(actor) == 12.625f, "staff attack adds the full primary and secondary bracelet in order");
    secondary.subtype = 6;
    check(magicBase(actor) == 16.625f, "a qualifying secondary bow replaces the default equipment list");
    primary.type = secondary.type = 0;
    check(magicBase(actor) == 40.625f, "both unarmed flags add actual-rank terms to the rank-zero base");

    actor.controller = 0;
    actor.appearanceFlags = 0x20;
    actor.multipliers[4] = .25f;
    constexpr std::array slots{CHEST_SLOT, BOOTS_SLOT, GLOVES_SLOT, SHOULDERS_SLOT};
    constexpr std::array types{4, 6, 5, 7}, materials{1, 26, 19, 27};
    for (size_t i = 0; i < slots.size(); ++i) {
        auto &piece = actor.equipment[slots[i]];
        piece.type = uint8_t(types[i]);
        piece.material = uint8_t(materials[i]);
        piece.level = 1;
    }
    check(resistance(actor) == (((4.5f + .85f) + .375f) + .425f) + .375f,
          "staff defense preserves the common actual-rank term and the ordered resistance weights");
}

void checkConnectedStaffHit() {
    for (bool ward : {false, true}) {
        MeleeFixture world;
        Combatant input;
        input.action = 30;
        input.characterClass = 3;
        input.aim = {2, 0, 0};
        input.actionMs = phases(input, 30).windUp + 1;
        auto &attacker = world.receive(1, input, world.x, world.y);
        Combatant defender;
        defender.controller = 6;
        defender.health = 50;
        defender.multipliers[4] = 0;
        defender.multipliers[3] = 100;
        auto &target = world.receive(2, defender, world.x + 4 * 65536, world.y);
        if (ward)
            target.buffs.push_back({ABSORB, 5, 200});
        NpcFrame frame;
        int draws = 0;
        const std::function<int()> draw = [&] { return draws++ >= (ward ? 3 : 2) ? 0 : 32767; };
        const net::RemoteCreatures::StepEffects effects{frame, draw};
        world.remotes.step(world.actors, 0, floorBlock, {}, nullptr, {}, &effects);
        check(draws == (ward ? 4 : 3) && frame.emittedHits.size() == 1 && frame.sounds.size() == 2 &&
                  frame.buffs.size() == 1 && attacker.combo == 1 && target.health == 50,
              "connected action thirty grants its own proc after either ordinary success or a fully absorbed ward");
        if (frame.emittedHits.size() == 1 && frame.sounds.size() == 2 && frame.buffs.size() == 1) {
            const auto &hit = frame.emittedHits.front();
            check(hit.kind == (ward ? HIT_ABSORBED : HIT_DAMAGE) && hit.amount == .4f * .75f &&
                      hit.knockBack == glm::vec3(0) && frame.sounds[0].sound == (ward ? 91 : 39) &&
                      frame.sounds[1].sound == 47 && frame.buffs[0].record.kind == 9 &&
                      frame.buffs[0].record.timeMs == 30000 && frame.buffs[0].source == 1,
                  "staff uses received aim, resistance and fixed magic impact before the single proc draw");
        }
    }
}

void checkConnectedStaffAreaHeal() {
    for (const int32_t action : {32, 33}) {
        MeleeFixture world;
        Combatant input;
        input.action = action;
        input.characterClass = 3;
        input.specialization = 1;
        input.combo = 7;
        input.aim = {2.5f, 0, 0};
        const auto timing = phases(input, action);
        input.actionMs = timing.windUp + 99;
        auto &attacker = world.receive(1, input, world.x, world.y);
        attacker.hitLanded = true;
        attacker.criticalLatched = true;
        attacker.struck = {88};
        Combatant other;
        other.health = 50;
        other.dodgeMs = 200;
        const auto &ally = world.receive(uint64_t(-3), other, world.x, world.y);
        Combatant local;
        local.id = 2;
        local.health = 40;
        Body localBody;
        localBody.size = {1, 1, 2};
        localBody.place({action == 32 ? 15.f : 17.f, 20, 8});
        const net::RemoteCreatures::Controlled controlled{local, localBody};
        constexpr std::array samples{23, 32767, 0, 0, 32767, 16384, 0};
        size_t draws = 0;
        NpcFrame frame;
        const std::function<int()> draw = [&] {
            const size_t next = draws++;
            return next < samples.size() ? samples[next] : 32767;
        };
        const net::RemoteCreatures::StepEffects effects{frame, draw};
        world.remotes.step(world.actors, 1, floorBlock, {}, &controlled, {}, &effects);
        check(draws == samples.size() && frame.emittedHits.size() == 3 && frame.sounds.size() == 4 &&
                  ally.dodgeMs > 0 && ally.health == 50 && local.health == 40 && attacker.combo == 7 &&
                  attacker.struck.empty() && attacker.deferred.empty() && attacker.hitLanded,
              "nonfinal upper equality opens the alternate route for a rolling ally, self and the controlled ally");
        if (frame.emittedHits.size() == 3 && frame.sounds.size() == 4) {
            constexpr std::array targets{uint64_t(-3), uint64_t(1), uint64_t(2)};
            for (size_t i = 0; i < targets.size(); ++i) {
                const auto &hit = frame.emittedHits[i];
                const float variation = 1.25f - float(samples[1 + 2 * i]) * .5f / 32767.f;
                check(hit.target == targets[i] && hit.attacker == 1 && hit.kind == HIT_DAMAGE &&
                          hit.amount == variation * -.1f && hit.critical == (i != 1) &&
                          hit.knockBack == glm::vec3(0) && hit.stun == 0 && frame.sounds[1 + i].sound == 42,
                      "each alternate heal takes variation then critical without scaling the signed amount or latching");
            }
            check(frame.sounds[0].sound == 15 && frame.sounds[0].pitch == sound::randomPitch(23, .25f, 1.f),
                  "the active boundary sound draws before any alternate healing");
        }

        // The final pulse can heal on a strict overrun even though it has no successor.
        attacker.actionMs = timing.windUp + timing.active - 1;
        frame.reset();
        draws = 0;
        world.remotes.step(world.actors, 1, floorBlock, {}, &controlled, {}, &effects);
        check(draws == 1 && frame.emittedHits.empty(), "final pulse equality produces its sound without an alternate heal");
        attacker.actionMs = timing.windUp + timing.active - 1;
        frame.reset();
        draws = 0;
        world.remotes.step(world.actors, 2, floorBlock, {}, &controlled, {}, &effects);
        check(draws == samples.size() && frame.emittedHits.size() == 3, "final pulse overrun independently admits alternate heals");
    }
}

void checkOrdinaryCreatureLink() {
    MeleeFixture world;
    Combatant input;
    input.controller = 6;
    input.health = 50.f;
    auto &earlier = world.receive(uint64_t(-2), input);
    auto &later = world.receive(3, input);
    earlier.linkedId = 123;
    later.ordinaryLink = 94;
    world.receive(3, input);
    check(later.ordinaryLink == 94, "a core receipt leaves the resident ordinary link intact");

    input.controller = 5;
    input.ownerId = 123;
    auto &pet = world.receive(4, input);
    Combatant local;
    local.id = 0;
    Body target;
    target.place({11, 31, 10});
    target.position[0] += 1;
    target.bob = .375f;
    target.swimPitch = -15.f;
    target.bank = 3.f;
    target.yaw = 34.f;
    target.walk = {8.f, 1.6f};
    const net::RemoteCreatures::Controlled controlled{local, target};
    const int64_t earlierX = target.position[0];
    const float amplitude = .8f + (0.f - .8f) * std::bit_cast<float>(0x3BA3D70Au);
    const auto &oldFollower = world.remotes.find(uint64_t(-2))->live;
    const auto &newFollower = world.remotes.find(3)->live;
    world.remotes.step(world.actors, 1, floorBlock, {}, &controlled, [&] {
        check(oldFollower.ordinaryAttached && oldFollower.position[0] == earlierX &&
                  oldFollower.position[2] == target.position[2] + 73728 && oldFollower.swimPitch == 5.f &&
                  oldFollower.bank == 3.f && oldFollower.yaw == 34.f && oldFollower.walk.phase == 8.f &&
                  oldFollower.walk.amplitude == amplitude && oldFollower.velocity == glm::vec3(0) &&
                  oldFollower.intent == glm::vec3(0) && !oldFollower.footfall,
              "the zero-key link copies the live pose before the controlled turn, skips walk and then relaxes amplitude");
        target.position[0] += 65536;
        target.walk.phase = 9.f;
        later.ordinaryLink = 0;
    });
    check(newFollower.ordinaryAttached && newFollower.position[0] == target.position[0] &&
              newFollower.walk.phase == 9.f && oldFollower.position[0] == earlierX && earlier.linkedId == 123,
          "a later zero-key lookup reads the new live pose without changing the separate pet association");
    check(!world.remotes.find(4)->live.ordinaryAttached && pet.ownerId == 123,
          "role five skips the ordinary copy independently of its carried-pet link");

    earlier.ordinaryLink = 94;
    later.health = 0.f;
    pet.ordinaryLink = 3;
    world.remotes.step(world.actors, 0, floorBlock, {}, &controlled);
    check(earlier.ordinaryLink == 94 && !oldFollower.ordinaryAttached && pet.ordinaryLink == 0,
          "a missing key is retained and a dead target clears even on the role-five branch");

    MeleeFixture alias;
    input = {};
    input.controller = 6;
    input.health = std::numeric_limits<float>::quiet_NaN();
    alias.receive(0, input);
    input.health = 50;
    alias.receive(2, input);
    alias.remotes.step(alias.actors, 0, floorBlock, {});
    check(alias.remotes.find(0)->live.position[2] == alias.z + 98304 &&
              alias.remotes.find(2)->live.position[2] == alias.z + 196608,
          "a zero-key self alias is admitted with unordered health and the next actor sees its completed copy");

    Npcs offline;
    offline.enableClientMotion();
    Npc resident = fixture(5);
    resident.fighter.controller = 6;
    offline.npcs().push_back(std::move(resident));
    worldgen::WorldMap map;
    map.add(std::make_shared<worldgen::Zone>(0, 0));
    NpcFrame frame;
    offline.stepInto(1, map, {{local, target, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
    const auto &body = offline.all().front().body;
    check(body.ordinaryAttached && body.position[0] == target.position[0] && body.position[2] == target.position[2] + 73728,
          "the offline client simulation uses the same live zero-key link and fixed lift");
}

void checkConnectedSneakLighting() {
    constexpr int64_t unit = 65536;
    constexpr int64_t x = 510 * unit, y = 300 * unit, z = 8 * unit;
    worldgen::WorldMap world;
    auto centre = std::make_shared<worldgen::Zone>();
    centre->zx = centre->zy = 1;
    centre->place(510, 300, 8, {0, 0, 0, 13});
    world.add(centre);
    auto adjacent = std::make_shared<worldgen::Zone>();
    adjacent->zx = 2;
    adjacent->zy = 1;
    worldgen::Sprite torch;
    torch.flags = 1;
    torch.x = x + 2 * unit;
    torch.y = y;
    torch.z = z;
    adjacent->sprites.push_back(torch);
    worldgen::StaticObject streetlight;
    streetlight.kind = 50;
    streetlight.x = x + 12 * unit;
    streetlight.y = y;
    streetlight.z = z;
    adjacent->statics.push_back(streetlight);
    world.add(adjacent);
    StaticStates statics;
    statics.generated(*adjacent);
    WorldItems items;
    WorldItem candle;
    candle.item[0] = 18;
    candle.position[0] = x + 14 * unit;
    candle.position[1] = y;
    candle.position[2] = z;
    items.replace(2, 1, {candle});

    net::RemoteCreatures remotes;
    ActorMap<net::RemoteFighter> actors;
    const auto receive = [&](uint64_t id, const Combatant &fighter, int64_t offset, glm::vec3 velocity = glm::vec3(0)) {
        auto record = net::CreatureRecord::nativeDefaults();
        record.setFighter(fighter);
        record.setPosition(0, x + offset);
        record.setPosition(1, y);
        record.setPosition(2, z);
        record.setMotion(velocity, 0.f);
        remotes.receive(net::CreatureReceipt::fromRecord(id, record, true, true));
        remotes.updateLive(id, record, actors[id]);
    };
    Combatant input;
    input.controller = 1;
    input.mode = LANTERN;
    receive(UINT64_MAX, input, 0); // its equipment check removes the light first
    input.health = 0;
    receive(2, input, 6 * unit); // death does not remove the received light bit
    input.health = 100;
    input.action = 79;
    input.mana = 0.6f;
    input.skills[7] = 10;
    input.equipment[10].type = 24;
    receive(1, input, 0, {0, 0, 1});
    auto &sneaking = actors.at(1).fighter;
    sneaking.stealth = 0.8f;
    Combatant local;
    local.health = 0; // a zero controller still seeds the zone set
    Body player;
    player.place({510, 300, 8});
    const net::RemoteCreatures::Controlled controlled{local, player};
    const auto night = remotes.makeSneakLight(world, statics, items, 0, actors, &controlled);
    const auto noon = remotes.makeSneakLight(world, statics, items, 43200000, actors, &controlled);
    const Body &subject = remotes.find(1)->live;
    const auto zoneLight = [](float squared) {
        const float weight = 1.f - squared * 0.00390625f;
        return 0.5f * (weight * weight);
    };
    const float nearby = (1.f - 36.f / 100.f) * (1.f - 36.f / 100.f);
    const float before = ((zoneLight(4.f) + zoneLight(144.f) + zoneLight(196.f)) + 1.f) + 1.f;
    check(night(subject) == before + nearby && noon(subject) == 3.f + nearby,
          "the seeded adjacent zone contributes in group order, then creature lights can take the result above one");

    auto snapshot = statics.record(*adjacent, 0);
    const int32_t replacement = 51;
    std::memcpy(snapshot.data() + 0x10, &replacement, sizeof replacement);
    check(statics.apply(snapshot.data(), &world).has_value(), "the streetlight's live section-six replacement is admitted");
    const float zoneBrightness = zoneLight(4.f) + zoneLight(196.f);
    check(night(subject) == ((zoneBrightness + 1.f) + 1.f) + nearby,
          "a light sample reads live streetlight type from the existing collection slot");
    remotes.step(actors, 20, floorBlock, {}, &controlled, [&] { player.position[0] += 1024 * unit; }, nullptr, night);
    const float brightness = (zoneBrightness + 1.f) + nearby;
    const float gain = (1.f - (0.9f - 0.5f * abilityValue(sneaking, 79)) * brightness) -
                       (0.5f - 0.5f * abilityValue(sneaking, 79));
    const float meter = 0.8f + (20.f * 0.0005f) * gain;
    check(sneaking.stealth == meter && sneaking.mana == (0.6f - 20.f * 0.00005f) + (20.f * 0.00025f) * meter,
          "sneaking samples earlier live lantern changes, retained zone membership and full velocity before mana credit");
    const auto next = remotes.makeSneakLight(world, statics, items, 0, actors, &controlled);
    check(next(subject) < night(subject), "a later world piece rebuilds the zone set from the moved seed");
}
}

int main() {
    checkConnectedActionClock();
    checkConnectedActionResources();
    checkConnectedMovementSounds();
    checkConnectedActionEffects();
    checkConnectedMeleeOrder();
    checkConnectedMeleeWindow();
    checkConnectedMeleeFamily();
    checkConnectedMeleeActionsNineThroughTwentyOne();
    checkConnectedMeleePulseAndEffect();
    checkStaffStatSelectors();
    checkConnectedStaffHit();
    checkConnectedStaffAreaHeal();
    checkOrdinaryCreatureLink();
    checkConnectedSneakLighting();
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
