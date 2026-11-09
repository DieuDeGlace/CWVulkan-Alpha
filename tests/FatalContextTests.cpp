#include "creatures/Npcs.h"
#include "net/Protocol.h"
#include "net/RemoteFighter.h"
#include <algorithm>
#include <bit>
#include <cstdio>

using namespace creatures;
namespace {
int failures = 0;
void check(bool okay, const char *message) {
    if (!okay) { std::printf("FAIL %s\n", message); failures++; }
}
bool defaultBacking(const BackingReference &backing) {
    return backing.x == -1 && backing.y == -1 && backing.index == 0;
}
const BlockType floorBlock = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
}

int main() {
    {
        Combatant original;
        check(defaultBacking(original.backing) && original.linkedId == 0 && original.ownerId == 0,
              "semantic construction retains the written backing/forward defaults and engine reverse-ID initialization");
        net::CreatureRecord record;
        record.bytes.fill(0xcc);
        original.ownerId = 0x80000000fedcba98ull;
        original.linkedId = 123;
        original.backing = {INT32_MIN, 65535, -17};
        record.setFighter(original);
        const auto decoded = record.fighter();
        check(decoded.ownerId == original.ownerId && decoded.linkedId == 0 &&
              decoded.backing.x == INT32_MIN && decoded.backing.y == 65535 && decoded.backing.index == -17 &&
              record.bytes[0x190] == 0xcc && record.bytes[0x1ac] == 0xcc,
              "core codec copies reverse ID and all three signed association words while retaining neighboring bytes and excluding the forward link");
        auto baseline = net::CreatureRecord::nativeDefaults();
        net::CreatureRecord position = baseline;
        position.setPosition(0, 123);
        const auto first = net::applyDelta(net::encodeDelta(77, position, 1), baseline);
        check(first == 77 && defaultBacking(baseline.fighter().backing),
              "a first position-only delta retains native negative association coordinates");
        net::applyDelta(net::encodeDelta(77, record, uint64_t(1) << 39), baseline);
        net::applyDelta(net::encodeDelta(77, record, uint64_t(1) << 35), baseline);
        position.setPosition(0, 456);
        net::applyDelta(net::encodeDelta(77, position, 1), baseline);
        check(baseline.fighter().backing.x == INT32_MIN && baseline.fighter().backing.index == -17 &&
              baseline.fighter().ownerId == original.ownerId,
              "explicit group deltas publish supplied values and later unrelated deltas retain them");
        net::RemoteFighter remote;
        remote.fighter.linkedId = UINT64_MAX;
        remote.refresh(decoded, 20);
        check(remote.fighter.linkedId == UINT64_MAX && remote.fighter.ownerId == original.ownerId &&
              remote.fighter.backing.x == INT32_MIN && remote.fighter.backing.index == -17,
              "core presentation refresh updates association/reverse ID while preserving the independent forward link");
    }
    {
        Combatant owner, child;
        owner.id = 77;
        child.id = UINT64_MAX;
        child.ownerId = 0;
        child.controller = 5;
        child.health = 0;
        ActorMap<Combatant *> table{{0, &owner}, {9, &child}};
        const auto lookup = [&](uint64_t key) -> Combatant * {
            const auto found = table.find(key);
            return found == table.end() ? nullptr : found->second;
        };
        refreshForwardLink(child, lookup);
        check(owner.linkedId == UINT64_MAX,
              "nonzero-mode link refresh resolves reverse key zero and copies the dead child's own ID independently of its entry key");
        child.ownerId = 123;
        child.id = 99;
        refreshForwardLink(child, lookup);
        child.ownerId = 0;
        child.controller = 3;
        refreshForwardLink(child, lookup);
        check(owner.linkedId == UINT64_MAX,
              "missing reverse lookup and non-role-five rows leave existing forward state unchanged");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>(1, 2);
        worldgen::Spawn spawn;
        spawn.x = int64_t(266) * 65536;
        spawn.y = int64_t(522) * 65536;
        spawn.z = 2 * 65536;
        spawn.type = 19;
        spawn.radius = 1000;
        zone->spawns.push_back(spawn);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant local;
        local.id = 1;
        Body body;
        body.place({266, 522, 3});
        Npcs world;
        auto questSpawn = spawn;
        questSpawn.quest = 7;
        world.setQuestCreatures(7, {questSpawn});
        NpcFrame frame;
        const auto step = [&] {
            world.stepInto(20, map, {{local, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        };
        step();
        const auto ordinary = [&]() -> Npc * {
            const auto found = std::find_if(world.npcs().begin(), world.npcs().end(),
                [&](const Npc &npc) { return npc.fighter.id == Npcs::spawnId(1, 2, 0); });
            return found == world.npcs().end() ? nullptr : &*found;
        };
        const auto quest = std::find_if(world.all().begin(), world.all().end(), [](const Npc &npc) { return npc.quest == 7; });
        check(ordinary() && ordinary()->fighter.backing.x == 1 && ordinary()->fighter.backing.y == 2 &&
              ordinary()->fighter.backing.index == 0 && quest != world.all().end() &&
              defaultBacking(quest->fighter.backing) && quest->spawn == Npc::NO_SPAWN,
              "ordinary descriptors publish their cached coordinates/ordinal while engine quests keep independent origin");
        if (Npc *npc = ordinary())
            npc->body.place({513, 522, 3});
        step();
        check(ordinary() && ordinary()->fighter.backing.x == 1 && ordinary()->fighter.backing.y == 2 &&
              ordinary()->fighter.backing.index == 0,
              "movement across a spatial boundary does not reconstruct cached descriptor provenance");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        Combatant owner;
        owner.id = 0;
        owner.backing = {99, 100, 7};
        owner.equipment[12].type = 19;
        owner.equipment[12].subtype = 19;
        Body body;
        body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        NpcFrame frame;
        const auto step = [&] {
            world.stepInto(20, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        };
        step();
        const Npc *pet = world.petOf(owner);
        check(pet && owner.linkedId == pet->fighter.id && pet->fighter.ownerId == 0 && defaultBacking(pet->fighter.backing),
              "a role-zero owner with key zero receives a forward link and its child retains the reverse key");
        if (pet) {
            world.npcs().front().fighter.backing = {1, 2, 3};
            world.npcs().front().fighter.linkedId = 88;
        }
        owner.skills[0]++;
        step();
        pet = world.petOf(owner);
        check(pet && defaultBacking(pet->fighter.backing) && pet->fighter.linkedId == 0 &&
              owner.backing.x == 99 && owner.backing.y == 100 && owner.backing.index == 7,
              "rebuilding the linked actor resets its context and forward ID while preserving the owner's retained context");
    }
    for (const bool named : {false, true}) {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        Npc zero;
        zero.fighter.id = 0;
        zero.fighter.controller = 1;
        zero.fighter.health = 100;
        zero.body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(named);
        world.npcs().push_back(zero);
        Combatant owner;
        owner.id = 1;
        Body body;
        body.place({10, 10, 3});
        NpcFrame frame;
        world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        check(world.all()[0].fighter.health == (named ? 0.f : 100.f) && owner.linkedId == 0 &&
              frame.emittedHits.empty() && frame.playerHits.empty(),
              "only named category maintenance directly zeros a resolved key-zero actor, without producing a hit");
    }
    {
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        Npc zero;
        zero.fighter.controller = 1;
        zero.fighter.health = 100;
        zero.fighter.buffs = {{7, 0, 0, 999}};
        zero.body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        world.npcs().push_back(zero);
        Combatant owner;
        owner.id = 1;
        Body body;
        body.place({10, 10, 3});
        NpcFrame frame;
        const auto step = [&] {
            world.stepInto(20, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        };
        step();
        check(world.all()[0].fighter.health == 100 && !world.all()[0].fighter.hasBuff(7),
              "resident selector-seven presence retains zero-linked health before the later expiration, irrespective of value, time or source");
        step();
        check(world.all()[0].fighter.health == 0,
              "the following category pass observes the expired retention record and directly zeros health");
    }
    {
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        Combatant owner;
        owner.id = 0;
        Body body;
        body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        NpcFrame frame;
        world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        check(owner.health == 0 && frame.emittedHits.empty(),
              "non-pet category maintenance admits key zero resolving to the owner itself, without a self or role exclusion");
    }
    for (const bool recoveredZero : {false, true}) {
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        Npc zero;
        zero.fighter.id = 0;
        zero.fighter.controller = recoveredZero ? 5 : 1;
        zero.fighter.ownerId = 5;
        zero.fighter.health = 100;
        zero.body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        world.npcs().push_back(zero);
        Combatant owner;
        owner.id = 5;
        owner.equipment[12].type = 19;
        owner.equipment[12].subtype = 19;
        Body body;
        body.place({10, 10, 3});
        NpcFrame frame;
        world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        check(owner.linkedId == UINT64_MAX && world.petOf(owner) &&
              (recoveredZero ? world.all().size() == 1 : world.all()[0].fighter.health == 100),
              "pet-category numeric zero admits creation despite a mapped zero actor or a recovered child with own ID zero");
    }
    for (const uint64_t ownerId : {uint64_t(-4), uint64_t(1) << 63}) {
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        Combatant owner;
        owner.id = ownerId;
        owner.equipment[12].type = 19;
        owner.equipment[12].subtype = 19;
        Body body;
        body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        NpcFrame frame;
        world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        check(owner.linkedId == ownerId - 1 && world.petOf(owner),
              "pre-activation pet allocation includes player keys and retains signed-minimum decrement wrap");
    }
    {
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        Npc zero;
        zero.fighter.controller = 3;
        zero.fighter.health = 0;
        zero.fighter.level = 0x18001;
        zero.fighter.experience = -17;
        zero.deadMs = 100;
        Npcs world;
        world.npcs().push_back(zero);
        Combatant owner;
        owner.id = 1;
        owner.equipment[12].type = 19;
        Body body;
        body.place({10, 10, 3});
        NpcFrame frame;
        world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        check(owner.linkedId == 0 && owner.equipment[12].level == -32767 &&
              owner.equipment[12].seed == uint32_t(-17) && frame.petItems.size() == 1,
              "the later designated-actor item copy resolves forward key zero even when the name gate skipped category maintenance, preserving low signed level bits");
    }
    {
        Npcs world;
        Npc target;
        target.fighter.id = 72;
        target.fighter.controller = 1;
        target.fighter.health = 10;
        target.fighter.action = 57;
        target.fighter.actionMs = 16777217;
        target.body.position[0] = (int64_t(1) << 56) + 3 * 65536;
        target.body.position[1] = -(int64_t(1) << 56) - 5 * 65536;
        target.body.position[2] = 3 * 65536;
        world.npcs().push_back(target);
        HitRecord hit;
        hit.attacker = 999;
        hit.target = 72;
        hit.amount = 10;
        NpcFrame frame;
        world.applyWorldHit(hit, {}, frame);
        const auto sounds = [&] { return std::count_if(frame.sounds.begin(), frame.sounds.end(),
            [](const SoundEvent &sound) { return sound.sound == 27; }); };
        check(sounds() == 1 && frame.deaths.empty() && world.all()[0].fighter.health == 0 &&
              world.all()[0].fighter.actionMs == 0 && world.all()[0].fighter.action == 57 &&
              frame.sounds[0].position.x == double(float(target.body.position[0]) * (1.f / 65536.f)) &&
              frame.sounds[0].position.y == double(float(target.body.position[1]) * (1.f / 65536.f)),
              "fatal application immediately emits narrowed-position sound and resets the integer clock while retaining the action byte");
        world.applyWorldHit(hit, {}, frame);
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::WorldMap map;
        map.add(zone);
        world.stepInto(0, map, {}, 1000, 0, floorBlock, frame);
        check(sounds() == 1 && frame.deaths.size() == 1,
              "dead-target rejection and the later corpse adapter do not replay the synchronous death presentation");
    }
    {
        worldgen::WorldMap map;
        map.add(std::make_shared<worldgen::Zone>(0, 0));
        Combatant owner;
        owner.id = 1;
        owner.equipment[12].type = 19;
        owner.equipment[12].subtype = 19;
        Body body;
        body.place({10, 10, 3});
        Npcs world;
        world.setNamedWorld(true);
        NpcFrame frame;
        const auto step = [&] {
            world.stepInto(0, map, {{owner, body, 0, 0, false, false, true}}, 1000, 0, floorBlock, frame);
        };
        step();
        HitRecord hit;
        hit.attacker = 999;
        hit.target = owner.linkedId;
        hit.amount = 1000000;
        std::vector<PlayerView> players{{owner, body, 0, 0, false, false, true}};
        world.applyWorldHit(hit, players, frame);
        step();
        check(world.petOf(owner) && world.petOf(owner)->fighter.health == 0 &&
              std::count_if(frame.sounds.begin(), frame.sounds.end(),
                  [](const SoundEvent &sound) { return sound.sound == 27; }) == 1,
              "the retained pet-corpse adapter does not replay an already emitted immediate death sound");
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
