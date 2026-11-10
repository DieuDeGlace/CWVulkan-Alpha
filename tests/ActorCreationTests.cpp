#include "creatures/ActorCreation.h"
#include "creatures/CreatedBehavior.h"
#include "creatures/Npcs.h"
#include "net/Protocol.h"
#include "net/RemoteFighter.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
struct Entity {
    Combatant fighter;
    Body body;
    Appearance looks;
    creation::Actor actor() const { return {&fighter, &body, &looks}; }
};
struct Fixture {
    Entity current, other;
    ActorMap<creation::Actor> actors;
    creation::Configuration configuration;
    creation::World world;
    creation::Services services;
    int queries = 0, maximumQueries = 0, draws = 0, visits = 0;
    int32_t first = 400, second = 400;
    std::vector<uint64_t> lookups;
    Fixture() {
        current.fighter.id = 9;
        current.fighter.action = 92;
        current.fighter.actionMs = 390;
        current.fighter.controller = 1;
        current.fighter.level = 200000;
        current.fighter.characterClass = 4;
        current.fighter.specialization = 1;
        current.fighter.experience = 123;
        current.fighter.mode = 0xa644;
        current.fighter.buffs.push_back({8, 3, -7});
        current.looks.flags = 0x62ff;
        current.looks.models[3] = 2107;
        current.body.position[0] = 0x123456789ab;
        current.body.position[1] = 0x23456789abc;
        current.body.position[2] = -65537;
        actors[100] = current.actor();
        world.visit = [&](const creation::Visitor &visit) {
            for (const auto &[key, actor] : actors) {
                visits++;
                visit(key, actor.fighter);
            }
        };
        world.lookup = [&](uint64_t key) {
            lookups.push_back(key);
            const auto it = actors.find(key);
            return it == actors.end() ? creation::Actor{} : it->second;
        };
        services.anticipation = [&](int32_t argument) {
            check(argument == -1, "creation anticipation queries retain the current-action argument");
            return queries++ == 0 ? first : second;
        };
        services.maximumHealth = [&](const Combatant &subject) {
            check(&subject == &current.fighter, "birth health queries the creator, not the provider or new actor");
            maximumQueries++;
            return 731.f;
        };
        services.draw = [&] { draws++; return 32767; };
    }
    creation::Result run(int32_t step = 20) {
        return creation::create(current.actor(), configuration, step, world, services);
    }
};
void admission() {
    {
        Fixture f;
        f.current.fighter.action = 91;
        check(!f.run().actor && f.queries == 0 && f.visits == 0 && f.draws == 0,
              "only action92 reaches any creation query");
    }
    {
        Fixture f;
        f.current.fighter.controller = 0;
        check(!f.run().actor && f.queries == 0, "mode-zero role-zero needs pointer designation");
        f.world.designated = &f.current.fighter;
        f.world.mode = 255;
        check(f.run().actor.has_value(), "exact designated pointer admits creation in a nonzero world mode");
    }
    {
        Fixture f;
        f.world.mode = 1;
        f.other.fighter.id = f.current.fighter.id;
        f.world.designated = &f.other.fighter;
        check(!f.run().actor && f.queries == 0, "equal stored IDs do not satisfy pointer designation");
    }
    for (const auto [clock, step, accepted, queryCount] :
         std::array<std::array<int32_t, 4>, 4>{{{401, 20, 0, 1}, {400, 0, 0, 2}, {390, 10, 0, 2}, {390, 11, 1, 2}}}) {
        Fixture f;
        f.current.fighter.actionMs = clock;
        const auto result = f.run(step);
        check(bool(result.actor) == bool(accepted) && f.queries == queryCount,
              "first anticipation equality is admitted, second equality is rejected, with distinct query counts");
    }
    {
        Fixture f;
        f.second = 350;
        check(f.run(-1).actor.has_value() && f.queries == 2,
              "negative steps are not rejected when the two supplied threshold results admit the native crossing");
    }
    {
        Fixture f;
        f.current.fighter.actionMs = f.first = std::numeric_limits<int32_t>::max();
        f.second = -1;
        check(!f.run(1).actor, "clock-plus-step wraps at32 bits before the signed second comparison");
    }
}
void countAndKeys() {
    {
        Fixture f;
        Entity dead, third;
        f.current.fighter.id = f.current.fighter.ownerId = 0;
        dead.fighter.health = 0;
        third.fighter.health = -1;
        f.actors[uint64_t(-3)] = dead.actor();
        f.actors[uint64_t(-2)] = third.actor();
        f.actors[uint64_t(-99)] = {};
        check(!f.run().actor && f.visits == 4 && f.draws == 0 && f.maximumQueries == 0,
              "three matching links include dead actors, self and zero, but the scan still visits every live map node");
        f.actors.erase(uint64_t(-2));
        f.queries = f.visits = 0;
        const auto result = f.run();
        check(result.actor && result.actor->fighter.id == uint64_t(-4) && result.actor->fighter.ownerId == 0,
              "two links still permit a new actor and null keys do not affect the signed minimum");
    }
    {
        Fixture f;
        f.other.fighter.id = uint64_t(std::numeric_limits<int64_t>::min());
        f.actors[UINT64_MAX] = f.other.actor();
        f.actors[uint64_t(-2)] = {};
        const auto result = f.run();
        check(result.actor && result.actor->fighter.id == uint64_t(-2),
              "allocation uses nonnull map keys, not actor IDs, and may replace an existing null-valued key");
    }
    {
        Fixture f;
        f.actors[uint64_t(std::numeric_limits<int64_t>::min())] = f.other.actor();
        check(f.run().actor->fighter.id == uint64_t(std::numeric_limits<int64_t>::max()),
              "signed-minimum predecessor allocation retains low64-bit wrap");
    }
}
void copiesAndDefaults() {
    Fixture f;
    for (size_t slot = 0; slot < SLOT_COUNT; slot++) {
        ItemRecord item;
        for (size_t byte = 0; byte < item.size(); byte++)
            item[byte] = uint8_t(byte * 17 + slot * 23 + 3);
        const int32_t count = slot % 2 ? -7 : 33;
        std::memcpy(item.data() + 0x114, &count, 4);
        f.current.fighter.setEquipment(slot, item);
    }
    f.current.fighter.linkedItem.retained.word = 99;
    f.configuration.kinds = {6, 117}; // the discarded candidate must still consume a draw
    const auto result = f.run();
    check(result.actor.has_value(), "the current-actor copy route creates an actor");
    if (!result.actor)
        return;
    const auto &actor = *result.actor;
    check(f.draws == 1 && f.maximumQueries == 1 && actor.fighter.type == f.current.fighter.type &&
          actor.fighter.level == 200000 && actor.fighter.health == 731 && actor.fighter.controller == 1 &&
          actor.fighter.ownerId == 9 && actor.fighter.characterClass == 0 && actor.fighter.specialization == 0 &&
          actor.fighter.experience == 0 && actor.fighter.action == 0 && actor.fighter.actionMs == 0 &&
          actor.fighter.hitStun == -3000 && actor.fighter.mode == 0 && actor.fighter.buffs.empty() &&
          actor.fighter.relations.threats.empty() && actor.fighter.linkedItem.retained.word == 0 &&
          actor.fighter.multipliers[0] == 100 && itemLevel(actor.fighter.heldRecord()) == 1,
          "birth copies selected sources while retaining fresh class/action/resident/stat/item defaults, without a new-actor health clamp");
    check(std::equal(std::begin(actor.body.position), std::end(actor.body.position), f.current.body.position) &&
          actor.body.velocity == glm::vec3(0) && actor.body.intent == glm::vec3(0) &&
          actor.looks.flags == 0x68ff && actor.fighter.appearanceFlags == 0x68ff && actor.looks.models[3] == 2107 &&
          actor.setup.children == std::array{creation::Child::Combat, creation::Child::Companion, creation::Child::Wander} &&
          actor.setup.specialMs == 20000 && actor.setup.range == 20 && actor.setup.chaseMs == 0 &&
          actor.setup.cooldownMs == 0 && actor.setup.walkMs == 0 && actor.setup.companionKey == 9 &&
          f.current.fighter.action == 92 && f.current.fighter.actionMs == 390 && f.current.fighter.linkedId == 0,
          "raw position, full appearance flags and ordered setup are retained without source action reset or control/link transfer");
    for (size_t slot = 0; slot < SLOT_COUNT; slot++) {
        auto expected = f.current.fighter.equipmentRecord(slot);
        for (const size_t gap : {2, 3, 15, 18, 19})
            expected[gap] = 0;
        check(actor.fighter.equipmentRecord(slot) == expected,
              "every copied item retains all32 body entries and fresh destination gaps across signed counts");
    }
    {
        Fixture provider;
        provider.configuration.choice = 2;
        provider.other.fighter.type = -77;
        provider.other.fighter.health = 0;
        provider.other.fighter.controller = 6;
        provider.other.looks.flags = 0x0204;
        provider.actors[0] = provider.other.actor();
        const auto copied = provider.run();
        check(copied.actor && copied.actor->fighter.type == -77 && copied.actor->fighter.ownerId == 9 &&
              copied.actor->looks.flags == 0x0804 && copied.actor->fighter.health == 731 &&
              copied.actor->fighter.level == provider.current.fighter.level && provider.lookups == std::vector<uint64_t>{0},
              "choice2 looks up contact key0 and uses its appearance despite health/role, while level/health/link still use the creator");
    }
}
void generation() {
    {
        Fixture f;
        f.configuration.choice = 0;
        f.current.fighter.controller = 0;
        f.world.designated = &f.current.fighter;
        const auto result = f.run();
        check(result.actor && f.draws == 5 && result.actor->fighter.type == 0x25 &&
              result.actor->fighter.controller == 3 && result.actor->looks.flags == 0x0829 &&
              result.actor->looks.models[4] == 0x095f && result.actor->looks.bodyOffset == glm::vec3(0) &&
              result.actor->looks.bodyScale == 1.5f &&
              std::bit_cast<uint32_t>(result.actor->looks.box.x) == 0x3f4ccccd,
              "empty generation uses full kind0x25 with all five common draws and its closed appearance output");
        check(result.actor && result.actor->fighter.equipmentRecord(7)[0] == 0 &&
              itemLevel(result.actor->fighter.equipmentRecord(7)) == 1,
              "appearance generation does not generate equipment");
    }
    {
        Fixture f;
        f.configuration.choice = -1;
        f.configuration.kinds = {-1};
        const auto result = f.run();
        check(result.actor && f.draws == 6 && result.actor->fighter.type == -1 && result.actor->looks.flags == 0x0800 &&
              std::bit_cast<uint32_t>(result.actor->looks.box.x) == 0x3f75c290 &&
              std::bit_cast<uint32_t>(result.actor->looks.box.z) == 0x400a3d71,
              "unsigned out-of-table generation keeps the common fields after the optional candidate draw");
    }
    {
        Fixture f;
        f.configuration.choice = 0;
        f.configuration.kinds = {6};
        const auto result = f.run();
        check(result.outcome == creation::Outcome::AppearancePending && !result.actor && f.draws == 1,
              "an unclosed generation output is an explicit engine capability boundary rather than a claimed native rejection");
    }
}
void descriptorConfiguration() {
    check(worldgen::Spawn{}.creationChoice == 0 && worldgen::Spawn{}.creationKinds.empty() &&
          creation::Configuration{}.choice == 1 && creation::Configuration{}.kinds.empty(),
          "descriptor zero/empty and fresh actor one/empty are distinct defaults");
    const std::array<int, 6> seedSlots{6, 7, 5, 2, 4, 3};
    for (const int32_t kind : {81, 117}) {
        for (const bool generating : {false, true}) {
            Combatant actor;
            actor.type = kind;
            actor.specials = {123};
            Appearance looks;
            creation::Configuration configuration{7, {-1}};
            int calls = 0;
            const auto draw = [&] { return 100 + calls++; };
            equip(actor, looks, draw, generating, 1, configuration);
            const bool candidates = kind == 117;
            const auto actions = candidates ? std::vector<int32_t>{123, 92} :
                generating ? std::vector<int32_t>{123, 87, 92} : std::vector<int32_t>{123, 87};
            check(actor.specials == actions && configuration.choice == (candidates ? 0 : 7) &&
                  configuration.kinds == (candidates ? std::vector<int32_t>{-1, 17, 96} : std::vector<int32_t>{-1}) && calls == 6,
                  "bounded builders preserve old entries, separate action92 from candidate kinds, and spend six tail draws");
            for (size_t i = 0; i < seedSlots.size(); i++)
                check(actor.equipment[seedSlots[i]].seed == 100 + i,
                      "the six seed writes retain descriptor offset order rather than slot order");
            equip(actor, looks, draw, false, 1, configuration);
            auto repeated = actions;
            repeated.push_back(candidates ? 92 : 87);
            check(actor.specials == repeated && configuration.choice == (candidates ? 0 : 7) && calls == 12 &&
                  configuration.kinds == (candidates ? std::vector<int32_t>{-1, 17, 96, 17, 96} : std::vector<int32_t>{-1}),
                  "nonzero setup keeps prior configuration and can append the earlier kind117 pair again");
        }
    }
    for (const int32_t kind : {81, 117}) {
        for (int selected = 0; selected < 5; selected++) {
            Combatant actor;
            actor.type = kind;
            Appearance looks;
            looks.flags = 0x200;
            creation::Configuration configuration{7, {-1}};
            const std::array<int32_t, 5> pool{92, 93, 86, 89, 87};
            int calls = 0;
            const auto draw = [&] {
                const int call = calls++;
                if (call == 1)
                    check(actor.specials.back() == pool[size_t(selected)] && configuration.choice == (kind == 117 ? 0 : 7),
                          "action selection is published before the separate choice draw");
                if (call == 2)
                    check(configuration.choice == 2 && configuration.kinds.back() == 96,
                          "choice and candidate appends precede the common seed tail");
                return call == 0 ? selected : call == 1 ? 5 : 200 + call;
            };
            equip(actor, looks, draw, true, 1, configuration);
            const auto actions = kind == 117 ? std::vector<int32_t>{92, pool[size_t(selected)]} :
                                              std::vector<int32_t>{87, 92, pool[size_t(selected)]};
            const auto kinds = kind == 117 ? std::vector<int32_t>{-1, 17, 96, 17, 96} : std::vector<int32_t>{-1, 17, 96};
            check(actor.specials == actions && configuration.choice == 2 && configuration.kinds == kinds && calls == 8,
                  "boss setup keeps exact five-action selection, separate choice and duplicate candidate order");
            for (size_t i = 0; i < seedSlots.size(); i++)
                check(actor.equipment[seedSlots[i]].seed == 202 + i,
                      "boss selection consumes two draws before all six seeds");
        }
    }
    for (const uint8_t classification : {uint8_t(0), uint8_t(0x80), uint8_t(0x84), uint8_t(0x88), uint8_t(0x89)}) {
        Combatant actor;
        actor.type = 117;
        actor.characterClass = classification;
        actor.specials = {123};
        Appearance looks;
        looks.flags = classification == 0 ? 0x240 : 0x200;
        creation::Configuration configuration{0x12345601, {-7, 96}};
        int calls = 0;
        equip(actor, looks, [&] { calls++; return 0; }, true, 1, configuration);
        check(configuration.choice == 0x12345601 && configuration.kinds == std::vector<int32_t>{-7, 96} &&
              actor.specials == std::vector<int32_t>{123} && calls == (classification == 0x88 ? 1 : 0),
              "early flag and special-class returns leave configuration intact and skip generic draws");
    }
    for (const int32_t role : {6, 0x106}) {
        for (const uint16_t flags : {uint16_t(0), uint16_t(8), uint16_t(16), uint16_t(0x200)}) {
            for (const bool generating : {false, true}) {
                Combatant actor;
                actor.type = 117;
                actor.controller = uint8_t(role);
                Appearance looks;
                looks.flags = flags;
                creation::Configuration configuration{7, {-1}};
                int calls = 0;
                equip(actor, looks, [&] { calls++; return 5; }, generating, role, configuration);
                const bool earlyPair = role != 6 && !(flags & 0x18);
                const bool laterPair = generating && (flags & 0x200);
                std::vector<int32_t> expected{-1};
                for (int pair = 0; pair < int(earlyPair) + int(laterPair); pair++) {
                    expected.push_back(17);
                    expected.push_back(96);
                }
                check(configuration.kinds == expected && configuration.choice == (laterPair ? 2 : earlyPair ? 0 : 7) &&
                      calls == (laterPair ? 8 : 6),
                      "full descriptor role and mask18 gate only the earlier pair; nonzero setup skips the later phase");
            }
        }
    }
    for (const int32_t kind : {81, 117}) {
        worldgen::Spawn spawn;
        spawn.type = kind;
        worldgen::Random random{73};
        finishSpawn(spawn, random, false);
        check(spawn.finished && spawn.creationChoice == 0 &&
              spawn.creationKinds == (kind == 117 ? std::vector<int32_t>{17, 96} : std::vector<int32_t>{}) &&
              spawn.specials == (kind == 117 ? std::vector<int32_t>{92} : std::vector<int32_t>{87, 92}),
              "the actual finishing pass retains independent descriptor configuration and actions");
        finishSpawn(spawn, random, false);
        check(spawn.creationKinds == (kind == 117 ? std::vector<int32_t>{17, 96, 17, 96} : std::vector<int32_t>{}) &&
              spawn.specials == (kind == 117 ? std::vector<int32_t>{92, 92} : std::vector<int32_t>{87, 92, 87, 92}),
              "a second finishing pass appends instead of rebuilding configuration from fresh defaults");
    }
}
void candidateAppearances() {
    for (const int32_t kind : {17, 96}) {
        for (const int32_t randomValue : {0, 32767}) {
            Fixture f;
            f.configuration = {0, {kind}};
            f.services.draw = [&] { f.draws++; return randomValue; };
            const auto result = f.run();
            check(result.actor && result.actor->fighter.type == kind && f.draws == 6 &&
                  f.configuration.kinds == std::vector<int32_t>{kind},
                  "a single candidate still spends its selection draw before five discarded appearance draws without consuming the entry");
            if (!result.actor)
                continue;
            std::array<uint8_t, 172> expected{}, actual{};
            const auto word = [&](size_t offset, uint16_t value) { std::memcpy(expected.data() + offset, &value, 2); };
            const auto bits = [&](size_t offset, uint32_t value) { std::memcpy(expected.data() + offset, &value, 4); };
            expected[2] = expected[3] = expected[4] = 0xff;
            word(6, kind == 17 ? 0x0c28 : 0x0828);
            bits(8, 0x3f75c290);
            bits(12, 0x3f75c290);
            bits(16, 0x400a3d71);
            const std::array<uint16_t, 8> models = kind == 17 ?
                std::array<uint16_t, 8>{0x0148, 0x0149, 0x014a, 0x014c, 0x014b, 0xffff, 0xffff, 0xffff} :
                std::array<uint16_t, 8>{0x00e3, 0xffff, 0x00e5, 0x00e4, 0x00e2, 0xffff, 0xffff, 0xffff};
            for (size_t i = 0; i < models.size(); i++)
                word(0x14 + 2 * i, models[i]);
            // Exact nonzero binary32 fields from the held full-extent table.
            // All unlisted bytes are specified zero, except unknown byte 5.
            for (const auto [offset, value] : std::array<std::pair<size_t, uint32_t>, 23>{{
                    {0x24, kind == 17 ? 0x3f4ccccdu : 0x3f8147aeu},
                    {0x28, 0x3f800000}, {0x2c, 0x3f800000}, {0x30, 0x3f7ae148},
                    {0x34, 0x3f800000}, {0x38, 0x3f733333}, {0x3c, 0x3f4ccccd},
                    {0x40, 0x3f800000}, {0x44, 0x3f800000}, {0x50, 0xc2b40000},
                    {0x6c, kind == 17 ? 0xc0000000u : 0xc0a00000u},
                    {0x74, kind == 17 ? 0x40800000u : 0x3f000000u},
                    {0x78, kind == 17 ? 0x40e00000u : 0x40a00000u},
                    {0x7c, kind == 17 ? 0x40a00000u : 0x40c00000u},
                    {0x80, 0x40400000}, {0x84, kind == 17 ? 0u : 0xc0000000u},
                    {0x88, 0x40400000}, {0x8c, 0x3f800000}, {0x90, 0xc1280000},
                    {0x98, 0xc1000000}, {0x9c, 0x40000000}, {0xa0, 0}, {0xa8, 0}}})
                bits(offset, value);
            result.actor->looks.serialize(actual.data());
            bool matches = true;
            for (size_t i = 0; i < actual.size(); i++)
                if (i != 5 && actual[i] != expected[i])
                    matches = false;
            check(matches, "every specified appearance byte matches the independent fresh-state table after caller flag publication");
        }
        Fixture copied;
        copied.configuration = {1, {kind}};
        const auto result = copied.run();
        check(result.actor && copied.draws == 1 && result.actor->fighter.type == copied.current.fighter.type,
              "copy alternatives retain the candidate draw but skip all appearance-generation draws");
    }
    Fixture fullWidth;
    fullWidth.configuration = {0, {0x10011}};
    const auto result = fullWidth.run();
    check(result.actor && result.actor->fighter.type == 0x10011 && result.actor->looks.models[0] == NO_MODEL &&
          result.actor->looks.flags == 0x0800 && fullWidth.draws == 6,
          "matching a candidate's low byte does not select its full-width appearance case");
}
void production() {
    worldgen::WorldMap map;
    map.add(std::make_shared<worldgen::Zone>(0, 0));
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
    Npcs world;
    Npc creator;
    creator.fighter.id = 9;
    creator.fighter.controller = 1;
    creator.fighter.level = 10;
    creator.fighter.multipliers[0] = 321;
    creator.fighter.health = maxHealth(creator.fighter);
    creator.fighter.action = 92;
    creator.fighter.actionMs = phases(creator.fighter, 92).windUp - 10;
    creator.body.place({10.5, 10.5, 3.5});
    creator.home = creator.body.blocks();
    creator.thinkMs = 0;
    world.npcs().push_back(creator);
    world.step(20, map, {}, 1000, 0, floor);
    check(world.all().size() == 2, "actual NPC action phase publishes a created actor");
    if (world.all().size() != 2)
        return;
    const Npc &born = world.all()[1];
    check(born.fighter.id == UINT64_MAX && born.fighter.ownerId == 9 && born.follows == 9 &&
          born.behaviour == 7 && born.specialMs == 20000 && born.fighter.actionMs == 0 && born.thinkMs == 0 &&
          born.fighter.hitStun == -3000 && born.fighter.health == maxHealth(creator.fighter) &&
          born.fighter.health > maxHealth(born.fighter) && born.creationConfig.choice == 1 &&
          born.creationConfig.kinds.empty() && world.all()[0].fighter.action == 92,
          "live predecessor insertion is not updated or clamped in its birth pass and retains the three-child fresh setup");
}

void referenceAndWander() {
    Combatant actor;
    actor.behaviorReference = {std::numeric_limits<int64_t>::min(), 0x123456789abcdef, -65537};
    net::CreatureRecord wire;
    wire.bytes[0x1af] = 0x5a;
    wire.bytes[0x1c8] = 0xa5;
    wire.setFighter(actor);
    check(wire.fighter().behaviorReference == actor.behaviorReference && wire.bytes[0x1af] == 0x5a && wire.bytes[0x1c8] == 0xa5,
          "all24 core reference bytes round-trip without touching the adjacent gap or property");
    net::CreatureRecord changed = wire;
    actor.behaviorReference[1]++;
    changed.setFighter(actor);
    const uint64_t mask = net::changedFields(changed, &wire);
    check(mask == (uint64_t(1) << 40) && net::applyDelta(net::encodeDelta(19, changed, mask), wire) == 19,
          "a raw reference edit uses only existing delta field40");
    net::RemoteFighter remote;
    remote.refresh(wire.fighter(), 0);
    check(remote.fighter.behaviorReference == actor.behaviorReference,
          "the supplied raw reference survives actual core refresh rather than being replaced with main position");

    Body body;
    body.intent = {7, 8, 9};
    actor.mode = 0xa644;
    actor.action = 73;
    actor.actionMs = 1234;
    int32_t counter = 1;
    int draws = 0;
    const auto unused = [&] { draws++; return 0; };
    created::wander(actor, body, {0, 0, 0}, counter, 0, unused);
    check(actor.mode == 0xa604 && counter == 1 && draws == 0 && body.intent == glm::vec3(7, 8, 9) &&
          actor.action == 73 && actor.actionMs == 1234,
          "a waiting third child still clears only run and retains action/direction without a draw");
    const std::array<int32_t, 3> values{0, 32767, 5001};
    created::wander(actor, body, {10 * 65536, 0, 500 * 65536}, counter, 1, [&] {
        check(actor.action == 73 && actor.actionMs == 1234,
              "direction and final timer draws precede the action-byte clear without resetting its clock");
        if (draws == 2)
            check(body.intent == glm::vec3(-10, 10, 0), "both unnormalized near components are stored before the timer draw");
        return values[size_t(draws++)];
    });
    check(draws == 3 && counter == 3001 && body.intent == glm::vec3(-10, 10, 0) && actor.action == 0 && actor.actionMs == 1234,
          "fixed horizontal equality at100 uses all three draws, no normalization and the retained clock");
    actor.action = 74;
    counter = 0;
    draws = 0;
    created::wander(actor, body, {11 * 65536, 0, 500 * 65536}, counter, 0, [&] { draws++; return 4999; });
    check(draws == 1 && counter == 7999 && body.intent == glm::vec3(10, 0, 0) && actor.action == 0 && actor.actionMs == 1234,
          "the far route uses horizontal reference direction and only the final timer draw");
}

void companionCaller() {
    struct Fixture {
        Combatant actor;
        Body body, reference;
        created::PathState path;
        created::PathStatus status{1, 0, {10, 10, 2}};
        created::CompanionServices services;
        std::vector<created::Position> samples;
        std::vector<float> maxima{100, 100, 100};
        std::vector<std::string> events;
        std::function<void()> afterStep;
        bool haveReference = true, direct = true, present = true;
        int maximumCalls = 0, steps = 0, clears = 0, starts = 0;
        uint64_t queried = 999;
        Fixture() {
            body.place({10.5, 10.5, 3.5});
            body.size.z = 2;
            reference.place({20.5, 10.5, 3.5});
            actor.mode = 0xa604;
            actor.action = 73;
            body.intent = {7, 8, 9};
            services.lookup = [&](uint64_t key) { queried = key; return haveReference ? &reference : nullptr; };
            services.maximumHealth = [&] { return maxima.at(size_t(maximumCalls++)); };
            services.directRoute = [&](const Body &from, const Body &to, float radius) {
                check(&from == &body && &to == &reference && radius == body.size.x + reference.size.x,
                      "the broad direct-route gate receives subject/reference bodies and ordered width sum");
                events.push_back("direct");
                return direct;
            };
            services.voxel = [&](const created::Position &point) {
                samples.push_back(point);
                return uint8_t(point[2] < 65536 ? 1 : 0x82);
            };
            services.clearPath = [&] { clears++; events.push_back("clear"); status.nodes = status.coordinates = 0; };
            services.cellPresent = [&](int32_t x, int32_t y) {
                check(x == 0 && y == 0, "fresh setup uses the corrected coordinate's truncated spatial cell");
                events.push_back("cell");
                return present;
            };
            services.initializePath = [&] {
                starts++;
                status.nodes = 1;
                events.push_back("initialize");
            };
            services.pathStep = [&] {
                steps++;
                status.coordinates = 1;
                events.push_back("step");
                if (afterStep)
                    afterStep();
            };
            services.pathStatus = [&] { return status; };
        }
        bool run(int32_t step = 32) { return created::companion(actor, body, 0, step, path, services); }
    };
    {
        Fixture f;
        f.haveReference = false;
        check(!f.run() && f.queried == 0 && f.actor.mode == 0xa604 && f.body.intent == glm::vec3(7, 8, 9) &&
              f.maximumCalls == 0 && f.events.empty(), "a missing exact key0 returns before any second-child mutations");
    }
    for (const double distance : {3., 4., 6., 8., 10.}) {
        Fixture f;
        f.reference.place(f.body.blocks() + glm::dvec3(distance, 0, 0));
        const bool handled = f.run();
        check(handled == (distance >= 4) && bool(f.actor.mode & RUN) == (distance > 8) &&
              f.body.intent == glm::vec3(distance < 4 ? 0.f : distance <= 6 ? float(distance * 5) : 30.f, 0, 0) &&
              f.actor.action == 73 && f.maximumCalls == 0 && f.clears == (distance >= 4 ? 1 : 0),
              "independent fixed-run, strict float-near and 900-cap boundaries preserve the action and direct-route intent");
    }
    {
        Fixture f;
        f.actor.controller = 5;
        f.actor.health = 95;
        f.maxima = {100, 50, 37};
        f.reference = f.body;
        check(!f.run(1000) && f.actor.health == 37 && f.maximumCalls == 3 && f.body.intent == glm::vec3(0),
              "role-five health uses distinct first/compare/replacement queries before the near return");
        f.maximumCalls = 0;
        f.actor.health = 5;
        f.maxima = {100, 100, 100};
        check(!f.run(-1000) && f.actor.health == -5 && f.maximumCalls == 2,
              "negative updates retain two health queries and no invented lower clamp");
    }
    {
        Fixture f;
        f.direct = false;
        f.services.voxel = [&](const created::Position &point) { f.samples.push_back(point); return uint8_t(0x82); };
        check(!f.run() && f.samples.size() == 21 && f.samples.back()[2] == f.reference.position[2] - 20 * 65536 &&
              f.steps == 0 && f.body.intent == glm::vec3(0),
              "downward exhaustion queries21 locations and stops before the advanced22nd coordinate");
    }
    {
        Fixture f;
        f.direct = false;
        f.services.voxel = [&](const created::Position &point) { f.samples.push_back(point); return uint8_t(1); };
        check(!f.run() && f.samples.size() == 22 && f.samples[0] == f.samples[1] &&
              f.samples.back()[2] == f.reference.position[2] + 20 * 65536 && f.steps == 0,
              "upward exhaustion repeats the turning-point query and admits21 upward samples");
    }
    {
        Fixture f;
        f.direct = false;
        check(f.run() && f.samples.size() == 6 && f.samples[3] == f.samples[4] && f.starts == 1 && f.steps == 11 &&
              f.path.goal == created::Position{20 * 65536 + 32768, 10 * 65536 + 32768, 98304} &&
              f.path.start == created::Position{10 * 65536 + 32768, 10 * 65536 + 32768, 170394} && f.path.radius == 2,
              "fresh terrain setup retains raw fractional coordinates, exact converted offset and one initial plus ten additional path pairs");
        check(f.events[1] == "clear" && f.events[2] == "cell" && f.events[3] == "initialize" && f.events[4] == "step",
              "fresh setup clears, checks the cell, initializes, then runs its first path pair");
    }
    {
        Fixture f;
        f.direct = false;
        f.present = false;
        check(!f.run() && f.clears == 1 && f.starts == 0 && f.steps == 0 && f.path.goal[2] == 98304 &&
              f.path.start[2] == 170394 && f.body.intent == glm::vec3(0),
              "missing fresh cell returns after coordinate copies without rolling back earlier path/vector writes");
    }
    {
        Fixture f;
        f.direct = false;
        f.status.coordinates = 50;
        f.path.start = {1, 2, 3};
        f.path.goal = {30 * 65536, 10 * 65536, 65536};
        f.path.radius = 0;
        const auto retained = f.path.goal;
        f.afterStep = [&] { f.status.nodes = f.status.coordinates = 0; };
        check(f.run() && f.starts == 0 && f.steps == 10 && f.path.goal == retained && f.path.start == created::Position{1, 2, 3},
              "retained setup keeps its old coordinates and permits ten pairs without rechecking the first-tree count");
    }
}

void controlledAndDescriptor() {
    worldgen::WorldMap map;
    auto zone = std::make_shared<worldgen::Zone>(1, 0);
    worldgen::Spawn source;
    source.x = 276 * 65536ll + 7;
    source.y = 20 * 65536ll + 3;
    source.z = 10 * 65536ll + 17;
    source.controller = 3;
    source.type = 2;
    zone->spawns.push_back(source);
    map.add(zone);
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z < 10 ? 1 : 0); };
    Entity player;
    player.fighter.id = 1;
    player.fighter.health = maxHealth(player.fighter);
    player.body.place({276.5, 150.5, 11.1});
    Npcs descriptors;
    descriptors.step(16, map, {{player.fighter, player.body, 0, 0, false}}, 1000, 0, floor);
    check(descriptors.all().size() == 1 && descriptors.all()[0].fighter.behaviorReference ==
          std::array<int64_t, 3>{source.x, source.y, source.z},
          "actual descriptor activation retains all raw reference coordinates independently of the body adapter");
    check(descriptors.all().size() == 1 && descriptors.all()[0].creationConfig.choice == 0 &&
          descriptors.all()[0].creationConfig.kinds.empty(),
          "actual descriptor activation replaces fresh actor choice1 with descriptor choice0 even for an empty collection");
    player.fighter.action = 92;
    player.fighter.actionMs = phases(player.fighter, 92).windUp - 10;
    Npcs controlled;
    controlled.step(20, map, {{player.fighter, player.body, 0, 0, true, false, true, &player.looks}}, 1000, 0, floor);
    const auto born = std::find_if(controlled.all().begin(), controlled.all().end(),
                                  [](const Npc &npc) { return npc.fighter.id == UINT64_MAX; });
    check(born != controlled.all().end() && born->fighter.controller == 3 && born->fighter.ownerId == 1 &&
          born->fighter.actionMs == 0 && born->fighter.behaviorReference == std::array<int64_t, 3>{},
          "actual designated role-zero action dispatch creates before clock advancement with separately defined fresh engine reference storage");
}
void configuredActivation() {
    worldgen::WorldMap map;
    auto zone = std::make_shared<worldgen::Zone>(1, 0);
    worldgen::Spawn source;
    source.x = 276 * 65536ll;
    source.y = 20 * 65536ll;
    source.z = 10 * 65536ll;
    source.type = 81;
    source.finished = true;
    Appearance{}.serialize(source.appearance.data());
    source.specials = {87, 92};
    source.creationChoice = 0x12345601;
    source.creationKinds = {96, -17, 96};
    zone->spawns.push_back(source);
    map.add(zone);
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z < 10 ? 1 : 0); };
    Entity player;
    player.fighter.id = 1;
    player.fighter.health = maxHealth(player.fighter);
    player.body.place({276.5, 150.5, 11.1});
    Npcs world;
    world.step(16, map, {{player.fighter, player.body, 0, 0, false}}, 1000, 0, floor);
    check(world.all().size() == 1, "configured descriptor activates in the real actor world");
    if (world.all().size() != 1)
        return;
    auto &actor = world.npcs()[0];
    check(actor.fighter.specials == source.specials && actor.creationConfig.choice == 0x12345601 &&
          actor.creationConfig.kinds == source.creationKinds,
          "activation copies full configuration and candidate order separately from actions");
    actor.creationConfig.kinds[0] = 17;
    check(zone->spawns[0].creationKinds == source.creationKinds,
          "resident candidates do not alias the descriptor allocation");
    actor.creationConfig = {0, {}};
    actor.thinkMs = 0; // isolate the action phase from the next independent AI decision
    actor.fighter.action = 92;
    actor.fighter.actionMs = phases(actor.fighter, 92).windUp - 10;
    world.step(20, map, {{player.fighter, player.body, 0, 0, false}}, 1000, 0, floor);
    const auto born = std::find_if(world.all().begin(), world.all().end(),
                                  [](const Npc &npc) { return npc.fighter.id == UINT64_MAX; });
    check(born != world.all().end() && born->fighter.type == 37 && born->looks.flags == 0x0829 &&
          born->creationConfig.choice == 1 && born->creationConfig.kinds.empty(),
          "the real activated creator generates kind37 from zero/empty while its newborn retains independent one/empty defaults");
    for (const int32_t kind : {17, 96}) {
        actor.creationConfig = {0, {kind}};
        actor.fighter.action = 92;
        actor.fighter.actionMs = phases(actor.fighter, 92).windUp - 10;
        actor.thinkMs = 0;
        const size_t before = world.all().size();
        world.step(20, map, {{player.fighter, player.body, 0, 0, false}}, 1000, 0, floor);
        check(world.all().size() == before + 1 && world.all().back().fighter.type == kind &&
              world.all().back().looks.flags == (kind == 17 ? 0x0c28 : 0x0828) &&
              world.all().back().creationConfig.choice == 1 && world.all().back().creationConfig.kinds.empty(),
              "real action dispatch also publishes each reachable candidate appearance with fresh independent resident configuration");
    }
}
} // namespace
int main() {
    admission();
    countAndKeys();
    copiesAndDefaults();
    generation();
    descriptorConfiguration();
    candidateAppearances();
    production();
    referenceAndWander();
    companionCaller();
    controlledAndDescriptor();
    configuredActivation();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
