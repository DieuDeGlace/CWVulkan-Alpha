#include "creatures/TargetSelection.h"
#include "creatures/Npcs.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace creatures;
using namespace creatures::selection;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
struct Entity {
    Combatant fighter;
    Body body;
    Entity(uint64_t id, uint8_t role, double x = 10.5, double y = 10.5) {
        fighter.id = id;
        fighter.controller = role;
        fighter.characterClass = 0;
        fighter.health = 100;
        fighter.actionMs = 100;
        body.place({x, y, 3.5});
    }
    Actor actor() { return {&fighter, &body}; }
};
struct Fixture {
    Entity subject{100, 1};
    ActorMap<Actor> world;
    Services services;
    int32_t special = 50, chase = 70, cooldown = 0;
    float range = 20, maximum = 100;
    int32_t duration = 100;
    std::array<int32_t, 3> timings{5, 10, 20};
    std::vector<std::vector<Stack>> bags;
    std::vector<uint64_t> lookups, seen;
    std::vector<std::string> events;
    std::set<uint64_t> invisible;
    std::vector<int32_t> draws{0, 32767};
    size_t drawIndex = 0;
    int maximumCalls = 0, durationCalls = 0, potionCalls = 0, acquisitionCalls = 0, powerCalls = 0;
    std::function<void()> beforeDraw, maximumMutation;
    Fixture() {
        world[100] = subject.actor();
        subject.body.intent = {7, 8, 9};
        services.lookup = [&](uint64_t key) {
            lookups.push_back(key);
            const auto it = world.find(key);
            return it == world.end() ? Actor{} : it->second;
        };
        services.maximumHealth = [&] {
            maximumCalls++;
            events.push_back("maximum");
            const float value = maximum;
            if (maximumMutation)
                maximumMutation();
            return value;
        };
        services.duration = [&] { durationCalls++; return duration; };
        services.timing = [&](Timing query, int32_t argument) {
            check(argument == -1, "each timing site retains its negative current-action argument");
            events.push_back(query == Timing::Recovery ? "recovery" : query == Timing::Anticipation ? "anticipation" : "active");
            return timings[size_t(query)];
        };
        services.firstPotion = [&] {
            potionCalls++;
            for (auto &bag : bags)
                for (auto &row : bag)
                    if (row.count != 0 && itemType(row.item) == 1 && itemSubtype(row.item) == 1)
                        return Potion{&row.count, &row.item};
            return Potion{};
        };
        services.draw = [&] {
            if (beforeDraw)
                beforeDraw();
            check(drawIndex < draws.size(), "the bounded prefix consumes no extra random value");
            return drawIndex < draws.size() ? draws[drawIndex++] : 0;
        };
        services.power = [&](double exponent) { powerCalls++; events.push_back("power"); return std::pow(1.5, exponent); };
        services.related = [&](const Combatant &a, const Combatant &b) {
            events.push_back("related");
            return hostile(a, b);
        };
        services.classified = [&](const Combatant &a) {
            events.push_back("classified");
            return harmless(a.type, a.appearanceFlags);
        };
        services.visible = [&](Actor, Actor candidate) {
            events.push_back("visible");
            seen.push_back(candidate.fighter->id);
            return !invisible.contains(candidate.fighter->id);
        };
        services.acquired = [&](Actor target) {
            check(subject.fighter.relations.threats.at(target.fighter->id) == 1,
                  "fallback subject map store precedes the publication boundary");
            events.push_back("acquired");
            acquisitionCalls++;
        };
    }
    Result run(int32_t step = 32) {
        TargetSelection selector{subject.actor(), {special, chase, cooldown, range}, step, services};
        selector.begin();
        if (selector.scanning())
            for (auto it = world.begin(); it != world.end(); ++it)
                selector.consider(it->second);
        return selector.finish();
    }
};

ItemRecord payload(int32_t count = 0) {
    ItemRecord item;
    for (size_t i = 0; i < item.size(); i++)
        item[i] = uint8_t(i * 31 + 7);
    item[0] = item[1] = 1;
    setItemLevel(item, 7);
    std::copy_n(reinterpret_cast<const uint8_t *>(&count), 4, item.data() + 0x114);
    return item;
}

void earlyAdmission() {
    {
        Fixture f;
        f.subject.fighter.mode = 0xa644;
        f.subject.fighter.action = 2;
        f.subject.fighter.actionMs = 10001;
        f.subject.fighter.dodgeMs = -7;
        f.cooldown = 40;
        const auto result = f.run();
        check(result.outcome == Outcome::ReturnTrue && f.subject.fighter.action == 0 &&
              f.subject.fighter.actionMs == 10001 && f.subject.fighter.mode == 0xa640 &&
              f.special == 50 && f.cooldown == 40 && f.chase == 70 && f.lookups.empty() && f.maximumCalls == 0,
              "entry clears aim/action before a negative nonzero roll word returns without counter work");
    }
    for (const int action : {79, 82, 83, 84}) {
        Fixture f;
        f.subject.fighter.action = action;
        f.subject.fighter.actionMs = 10001;
        f.subject.fighter.dodgeMs = 1;
        f.run();
        check(f.subject.fighter.action == action, "the four excluded action bytes retain their action beyond 10000");
    }
    {
        Fixture f;
        f.subject.fighter.action = 2;
        f.subject.fighter.actionMs = 10000;
        f.subject.fighter.dodgeMs = 1;
        f.run();
        check(f.subject.fighter.action == 2, "the entry clock boundary is strict");
    }
    {
        Fixture f;
        f.subject.fighter.action = 80;
        f.subject.fighter.actionMs = 2999;
        check(f.run().outcome == Outcome::ReturnTrue && f.maximumCalls == 0 && f.special == 50,
              "drinking returns early strictly before 3000");
        f.subject.fighter.actionMs = 3000;
        check(f.run().outcome == Outcome::ReturnFalse && f.maximumCalls == 2 && f.special == 18,
              "drinking at 3000 proceeds through no-target processing");
    }
    {
        Fixture f;
        f.subject.fighter.health = 25;
        f.subject.fighter.actionMs = 101;
        f.run();
        check(f.durationCalls == 0 && f.potionCalls == 0, "quarter-health equality does not query the inventory duration");
        f.subject.fighter.health = 20;
        f.subject.fighter.actionMs = 100;
        f.run();
        check(f.durationCalls == 1 && f.potionCalls == 0,
              "duration equality does not scan potions, even for action zero");
    }
}

void potions() {
    constexpr std::array<size_t, 5> gaps{2, 3, 15, 18, 19};
    for (const int32_t count : {-7, std::numeric_limits<int32_t>::min(), 2}) {
        Fixture f;
        f.subject.fighter.health = 20;
        f.subject.fighter.actionMs = 101;
        f.subject.fighter.heldSource.fill(0x5a);
        const ItemRecord source = payload(-3);
        f.bags = {{{0, source}}, {{count, source}, {3, source}}};
        ItemRecord expected = source;
        for (const auto gap : gaps)
            expected[gap] = 0x5a;
        const int32_t remaining = count == std::numeric_limits<int32_t>::min() ?
                                  std::numeric_limits<int32_t>::max() : count == 2 ? 1 : 0;
        f.beforeDraw = [&] {
            const Stack &row = f.bags[1][0];
            check(f.subject.fighter.action == 80 && f.subject.fighter.actionMs == 0 &&
                  f.subject.fighter.heldRecord() == expected && row.count == remaining &&
                  row.item[0] == (remaining ? 1 : 0) && row.item[1] == (remaining ? 1 : 0) &&
                  std::equal(source.begin() + 2, source.end(), row.item.begin() + 2) &&
                  f.subject.body.intent == glm::vec3(7, 8, 9),
                  "full held copy and wrapped count mutation precede both draws, without erasing source payload or early intent writes");
        };
        const auto result = f.run();
        check(result.outcome == Outcome::ReturnTrue && f.drawIndex == 2 && f.lookups.empty() &&
              f.subject.body.intent == glm::vec3(-2, 2, 0) &&
              std::bit_cast<uint32_t>(f.subject.body.intent.z) == 0 &&
              f.special == 50 && f.chase == 70 && f.cooldown == 0 && f.bags[1][1].count == 3,
              "first nonzero potion row uses exactly two ordered, unnormalized components and bypasses counters/targets");
    }
    {
        Fixture f;
        f.subject.fighter.health = 20;
        f.subject.fighter.actionMs = 101;
        f.bags = {{{1, payload()}}};
        f.maximumMutation = [&] { f.subject.fighter.health = 100; };
        check(f.run().outcome == Outcome::ReturnTrue && f.drawIndex == 2,
              "the quarter comparison uses the health sampled before its maximum query");
    }
    {
        Fixture f;
        f.subject.fighter.controller = 3;
        f.subject.fighter.health = 30;
        f.subject.fighter.level = 0x1234abcd;
        auto expected = payload();
        f.subject.fighter.setHeld(expected);
        expected[0] = 1;
        expected[1] = 4;
        setItemLevel(expected, std::bit_cast<int16_t>(uint16_t(0xabcd)));
        f.subject.fighter.mode = 0xa644;
        const auto result = f.run();
        check(result.outcome == Outcome::ReturnTrue && f.maximumCalls == 2 && f.drawIndex == 0 &&
              f.lookups == std::vector<uint64_t>{0} && f.subject.fighter.heldRecord() == expected &&
              f.subject.fighter.mode == 0xa640 && f.subject.body.intent == glm::vec3(0) &&
              f.special == 50 && f.cooldown == 0 && f.chase == 70,
              "friendly no-target healing changes only the held selector/level words and returns without draws or terminal work");
    }
}

void preliminary() {
    {
        Fixture f;
        Entity zero{77, 5, 1000};
        zero.fighter.health = 0;
        f.world[0] = zero.actor();
        const auto result = f.run();
        check(result.outcome == Outcome::ContinueWithTarget && result.target.fighter == &zero.fighter &&
              f.lookups == std::vector<uint64_t>{0} && f.maximumCalls == 1 && f.acquisitionCalls == 0 &&
              f.seen.empty() && f.subject.fighter.relations.threats.empty() && f.chase == 70 &&
              (f.subject.fighter.mode & RUN),
              "default key zero is looked up and its dead/distant/role-five pointer bypasses fallback-only gates");
    }
    {
        Fixture f;
        f.subject.fighter.relations.threats = {{0, 100.f}, {UINT64_MAX, 0.f}, {1, 0.f}, {2, 0.f}};
        f.world[2] = f.subject.actor();
        const auto result = f.run();
        check(result.target.fighter == &f.subject.fighter && f.lookups == std::vector<uint64_t>{2} &&
              !f.subject.fighter.relations.threats.contains(100),
              "primary zero-valued ties choose the greatest signed nonzero key and retain a self pointer without stored-ID publication");
    }
    {
        Fixture f;
        Entity lower{41, 0, 15.5};
        f.world[4] = lower.actor();
        f.subject.fighter.relations.threats = {{4, 8.f}, {5, 9.f}};
        f.cooldown = 1;
        const auto first = f.run(1);
        check(first.outcome == Outcome::ReturnFalse && f.lookups == std::vector<uint64_t>{5} && f.cooldown == 0,
              "a missing highest key does not retry a lower threat or reopen a cooldown that expires later");
        const auto second = f.run(1);
        check(second.target.fighter == &lower.fighter && f.subject.fighter.relations.threats.at(41) == 1 &&
              f.acquisitionCalls == 1,
              "the following invocation can scan and publish the target's stored ID rather than its world key");
    }
}

void linkedChoices() {
    {
        Fixture f;
        Entity owner{9, 0}, discarded{11, 0}, selected{12, 5}, tied{13, 0};
        selected.fighter.health = 0;
        f.subject.fighter.controller = 5;
        f.subject.fighter.relations.threats[8] = 3;
        f.world[8] = discarded.actor();
        f.world[0] = owner.actor();
        f.world[uint64_t(-3)] = {};
        f.world[uint64_t(-2)] = selected.actor();
        f.world[uint64_t(-1)] = tied.actor();
        owner.fighter.relations.counts = {{uint64_t(-3), 100.f}, {uint64_t(-2), 2.f}, {UINT64_MAX, 2.f}, {7, 0.f}};
        const auto result = f.run();
        check(result.target.fighter == &selected.fighter &&
              f.lookups == std::vector<uint64_t>{8, 0, uint64_t(-3), uint64_t(-2), uint64_t(-2)} &&
              f.acquisitionCalls == 0,
              "a nonnull zero-key link discards the primary choice; failed entries do not raise the strict best and winning lookup repeats");
        owner.fighter.action = 106;
        f.subject.fighter.mode = 0xa644;
        f.lookups.clear();
        f.special = 50; f.chase = 70; f.cooldown = 40;
        const auto stopped = f.run();
        check(stopped.outcome == Outcome::ReturnFalse && f.lookups == std::vector<uint64_t>{8, 0} &&
              f.special == 50 && f.chase == 70 && f.cooldown == 40 && f.subject.fighter.mode == 0xa640,
              "linked action106 returns false before readiness, counter reduction or chase reset");
    }
    {
        Fixture f;
        Entity candidate{1, 0, 12.5};
        f.subject.fighter.controller = 5;
        f.subject.fighter.ownerId = 99;
        f.world[1] = candidate.actor();
        check(f.run().outcome == Outcome::ReturnFalse && f.seen.empty(),
              "a missing nonzero link still suppresses fallback");
        f.subject.fighter.ownerId = 0;
        f.subject.fighter.type = 25;
        candidate.fighter.action = 82;
        candidate.fighter.equipment[12].type = 20;
        candidate.fighter.equipment[12].subtype = 25;
        check(f.run().target.fighter == &candidate.fighter,
              "an ownerless role-five subject can use the exact food exception despite a false relationship/classifier gate");
        f.subject.fighter.relations.threats.clear();
        f.subject.fighter.type = 281;
        check(f.run().outcome == Outcome::ReturnFalse,
              "the subtype byte compares with the complete kind, without narrowing kind281 to25");
    }
}

void fallbackAndNumerics() {
    {
        Fixture f;
        Entity first{9, 0, 110.5}, closer{10, 0, 110.5};
        first.body.position[1]++;
        f.range = 150;
        f.world[UINT64_MAX] = first.actor();
        f.world[1] = closer.actor();
        check(f.run().target.fighter == &first.fighter,
              "a one-unit fixed transverse difference disappears at the specified float sum, retaining the first tie instead of a double-distance winner");
    }
    {
        Fixture f;
        Entity first{9, 0, 15.5}, equal{10, 0, 15.5}, hidden{11, 0, 11.5};
        f.world[UINT64_MAX] = first.actor();
        f.world[1] = equal.actor();
        f.world[2] = hidden.actor();
        f.world[3] = {};
        f.world[4] = f.subject.actor();
        f.invisible.insert(11);
        const auto result = f.run();
        check(result.target.fighter == &first.fighter && f.seen == std::vector<uint64_t>{9, 11} &&
              f.subject.fighter.relations.threats.at(9) == 1 && f.acquisitionCalls == 1,
              "fallback visits signed keys, keeps the first equal score and does not narrow its limit for an occluded closer candidate");
        const auto publication = std::find(f.events.begin(), f.events.end(), "acquired");
        check(publication != f.events.end() && std::count(f.events.begin(), publication, "classified") == 1,
              "visibility precedes the subject classifier, and fallback publishes only after the scan");
    }
    {
        Fixture f;
        Entity rejected{9, 0, 11.5}, admitted{10, 0, 15.5};
        f.world[1] = rejected.actor();
        f.world[2] = admitted.actor();
        f.services.related = [](const Combatant &, const Combatant &candidate) { return candidate.id != 9; };
        check(f.run().target.fighter == &admitted.fighter && f.seen == std::vector<uint64_t>{10},
              "a relationship rejection cannot hide a farther admissible target");
    }
    {
        Fixture f;
        Entity candidate{9, 0, 12.5};
        f.world[1] = candidate.actor();
        candidate.fighter.stealth = -.5f;
        check(f.run().target.fighter == &candidate.fighter && f.powerCalls == 0,
              "negative stealth bypasses the complete power/factor adjustment");
        f.subject.fighter.relations.threats.clear();
        f.subject.fighter.buffs.push_back({8, 0, -1});
        candidate.fighter.stealth = .5f;
        f.seen.clear();
        check(f.run().outcome == Outcome::ReturnFalse && f.powerCalls == 1 && f.seen.empty(),
              "expired resident type8 still suppresses acquisition after positive stealth has queried power");
    }
    {
        Fixture f;
        Entity hidden{9, 0, 11.5}, ordinary{10, 0, 15.5};
        hidden.fighter.stealth = 2;
        f.world[1] = hidden.actor();
        f.world[2] = ordinary.actor();
        check(f.run().target.fighter == &ordinary.fighter,
              "a negative stealth factor clamps to float0.1 before squaring and dividing the score");
    }
    {
        Fixture f;
        Entity candidate{9, 0, 12.5};
        f.world[1] = candidate.actor();
        candidate.fighter.stealth = .1f;
        f.subject.fighter.level = std::numeric_limits<int32_t>::min();
        candidate.fighter.level = std::numeric_limits<int32_t>::max();
        double exponent = 0;
        f.services.power = [&](double input) { exponent = input; return 1.; };
        f.run();
        check(exponent == -1, "level difference wraps at32 bits before the float exponent conversion");
        f.subject.fighter.relations.threats.clear();
        f.subject.fighter.level = 1;
        candidate.fighter.level = 16777218;
        f.run();
        check(exponent == 16777216., "the signed level difference narrows to float before widening for power");
    }
    {
        Fixture f;
        Entity candidate{9, 0};
        f.subject.body.position[0] = std::numeric_limits<int64_t>::max() - 65535;
        candidate.body.position[0] = std::numeric_limits<int64_t>::min() + 65536;
        f.world[1] = candidate.actor();
        f.range = -3;
        check(f.run().target.fighter == &candidate.fighter,
              "raw64 position subtraction wraps before float scaling, and a negative supplied range is squared");
    }
    {
        Fixture f;
        Entity candidate{9, 0, 12.5}, inserted{10, 0, 11.5};
        f.world[1] = candidate.actor();
        f.services.visible = [&](Actor, Actor actor) {
            if (actor.fighter == &candidate.fighter)
                f.world[2] = inserted.actor();
            return true;
        };
        check(f.run().target.fighter == &inserted.fighter,
              "later live world nodes are considered without a fixed candidate count or copied vector");
    }
    {
        Fixture f;
        Entity candidate{9, 0, 12.5};
        f.subject.fighter.type = 19;
        f.world[1] = candidate.actor();
        check(f.run().outcome == Outcome::ReturnFalse && f.seen == std::vector<uint64_t>{9},
              "the post-visibility classifier inspects the subject, even when the candidate is not classified");
        candidate.fighter.action = 82;
        candidate.fighter.equipment[12].type = 20;
        candidate.fighter.equipment[12].subtype = 19;
        check(f.run().target.fighter == &candidate.fighter, "the exact food fields bypass a true subject classifier");
    }
}

void readinessAndCounters() {
    {
        Fixture f;
        Entity target{9, 0, 15.5};
        f.world[1] = target.actor();
        f.subject.fighter.action = 1;
        f.subject.fighter.actionMs = 33;
        f.run();
        check(std::find(f.events.begin(), f.events.end(), "acquired") <
              std::find(f.events.begin(), f.events.end(), "recovery"),
              "new-target publication completes before the first readiness query");
    }
    for (const auto [recovery, clock, ready] : std::vector<std::tuple<int32_t, int32_t, bool>>{
            {5, 32, false}, {5, 33, true}, {-5, 28, false}, {-5, 29, true}}) {
        Fixture f;
        Entity target{9, 0};
        f.world[0] = target.actor();
        f.subject.fighter.action = 1;
        f.subject.fighter.actionMs = clock;
        f.subject.fighter.mode = 0xa644;
        f.timings[0] = recovery;
        const auto result = f.run();
        check(result.outcome == Outcome::ContinueWithTarget && bool(f.subject.fighter.mode & RUN) == ready &&
              (f.subject.fighter.mode & ~uint32_t(RUN)) == 0xa600 && f.chase == 70 &&
              f.events == std::vector<std::string>{"maximum", "recovery", "anticipation", "active"},
              "readiness queries recovery/anticipation/active in order, halves as an integer and uses a strict clock boundary");
    }
    {
        Fixture f;
        Entity target{9, 0};
        f.world[0] = target.actor();
        f.subject.fighter.action = 1;
        f.subject.fighter.actionMs = -1;
        f.timings = {std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), 10};
        f.special = f.cooldown = std::numeric_limits<int32_t>::min();
        f.run(1);
        check((f.subject.fighter.mode & RUN) && f.special == std::numeric_limits<int32_t>::max() &&
              f.cooldown == std::numeric_limits<int32_t>::max(),
              "readiness additions and counter subtraction have defined32-bit wrap before signed comparisons");
    }
    {
        Fixture f;
        f.special = f.cooldown = 5;
        f.run(-3);
        check(f.special == 8 && f.cooldown == 8 && f.chase == 0,
              "a negative supplied step increases both counters and no target resets chase");
        f.special = f.cooldown = std::numeric_limits<int32_t>::max();
        f.run(-1);
        check(f.special == 0 && f.cooldown == 0, "a wrapped negative result clamps after subtraction");
    }
}

Npc npc(uint64_t id, double x, double y = 10.5) {
    Npc n;
    n.fighter.id = id;
    n.fighter.controller = 1;
    n.fighter.characterClass = 0;
    n.fighter.health = 100;
    n.body.place({x, y, 3.5});
    n.home = n.body.blocks();
    n.thinkMs = 32;
    return n;
}
void production() {
    {
        worldgen::WorldMap origin;
        auto zone = std::make_shared<worldgen::Zone>(0, 0);
        worldgen::Spawn spawn;
        spawn.x = spawn.y = int64_t(10.5 * 65536);
        spawn.z = int64_t(3.5 * 65536);
        spawn.type = 2;
        spawn.controller = 1;
        zone->spawns.push_back(spawn);
        origin.add(zone);
        Npcs world;
        Entity player{1, 0, 15.5};
        NpcFrame frame;
        world.stepInto(32, origin, {{player.fighter, player.body, 0, 0, false}}, 1000, 0,
                       [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); }, frame);
        check(world.all().size() == 1 && world.all()[0].fighter.id == 0 &&
              (world.all()[0].fighter.mode & AIMING) && world.all()[0].fighter.relations.threats.empty() &&
              frame.relationships.empty(),
              "an actual supplied origin spawn at key0 becomes its own preliminary pointer without fallback publication");
    }
    worldgen::WorldMap map;
    map.add(std::make_shared<worldgen::Zone>(0, 0));
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
    {
        Npcs world;
        auto near = npc(uint64_t(-10), 11.5), negative = npc(uint64_t(-9), 12.5), boundary = npc(uint64_t(-8), 18.5);
        near.fighter.hitStun = negative.fighter.hitStun = boundary.fighter.hitStun = 100;
        near.fighter.relations.threats[1] = -0.f;
        negative.fighter.relations.threats[1] = -1;
        world.npcs().push_back(near);
        world.npcs().push_back(negative);
        world.npcs().push_back(boundary);
        auto subject = npc(UINT64_MAX, 10.5);
        subject.fighter.mode = 0xa604;
        world.npcs().push_back(subject);
        Entity player{1, 0, 15.5};
        NpcFrame frame;
        world.stepInto(0, map, {{player.fighter, player.body, 0, 0, false}}, 1000, 0, floor, frame);
        check(world.all()[3].target == 1 && world.all()[3].fighter.relations.threats.at(1) == 1 &&
              world.all()[0].fighter.relations.threats.at(1) == .5f &&
              world.all()[1].fighter.relations.threats.at(1) == -1 &&
              !world.all()[2].fighter.relations.threats.contains(1) && frame.relationships.size() == 2 &&
              frame.relationships[0].target == UINT64_MAX && frame.relationships[0].source == 1 &&
              frame.relationships[1].target == uint64_t(-10) && frame.relationships[1].value == .5f &&
              (world.all()[3].fighter.mode & RUN) && !(world.all()[3].fighter.mode & AIMING) &&
              world.all()[3].body.mode == world.all()[3].fighter.mode,
              "actual NPC acquisition publishes subject then live signed fanout, retains nonzero values, excludes distance64 and commits the complete mode");
    }
    {
        Npcs world;
        auto ally = npc(UINT64_MAX, 10.5), enemy = npc(7, 15.5);
        ally.questRole = 3;
        enemy.fighter.hitStun = 100;
        world.npcs().push_back(ally);
        world.npcs().push_back(enemy);
        world.step(0, map, {}, 1000, 0, floor);
        check(world.all()[0].target == 7 && world.all()[0].fighter.relations.threats.at(7) == 1,
              "required quest allies retain their explicit production admission extension");
    }
    {
        Npcs world;
        auto drinker = npc(7, 10.5);
        drinker.fighter.health = 10;
        drinker.fighter.actionMs = 101;
        drinker.fighter.heldSource.fill(0x5a);
        const ItemRecord source = payload();
        drinker.inventory.appendUnit(source);
        const auto remaining = drinker.inventory.groups[0][0].item;
        world.npcs().push_back(drinker);
        world.step(0, map, {}, 1000, 0, floor);
        const Npc &result = world.all()[0];
        auto expected = source;
        for (const size_t gap : {2, 3, 15, 18, 19})
            expected[gap] = 0x5a;
        check(result.fighter.action == 80 && result.fighter.heldRecord() == expected &&
              result.inventory.groups.size() == 1 && result.inventory.groups[0].size() == 1 && result.inventory.groups[0][0].count == 0 && result.inventory.groups[0][0].item[0] == 0 && result.inventory.groups[0][0].item[1] == 0 &&
              std::equal(remaining.begin() + 2, remaining.end(), result.inventory.groups[0][0].item.begin() + 2),
              "actual grouped inventory consumes one unit in place while retaining complete held and remaining source bytes");
    }
    for (int32_t count : {2, -1, std::numeric_limits<int32_t>::min()}) {
        Npcs world;
        auto drinker = npc(7, 10.5);
        drinker.fighter.health = 10;
        drinker.fighter.actionMs = 101;
        drinker.fighter.heldSource.fill(0x5a);
        const auto source = payload();
        drinker.inventory.groups.resize(3);
        drinker.inventory.groups[0] = {{0, source}};
        drinker.inventory.groups[2] = {{count, source}, {5, source}};
        world.npcs().push_back(std::move(drinker));
        world.step(0, map, {}, 1000, 0, floor);
        const auto &result = world.all()[0];
        const auto &entry = result.inventory.groups[2][0];
        const int32_t expectedCount = count == 2 ? 1 : count == -1 ? 0 : std::numeric_limits<int32_t>::max();
        check(result.fighter.action == 80 && entry.count == expectedCount && result.inventory.groups[0][0].count == 0 &&
              result.inventory.groups[2][1].count == 5 && entry.item[0] == (count == -1 ? 0 : 1) &&
              result.inventory.groups[1].empty() && result.fighter.health == 10,
              "actual world potion consumer uses the first nonzero count across groups, retaining negative/decrement-wrap distinctions without direct healing");
    }
}
} // namespace

int main() {
    earlyAdmission();
    potions();
    preliminary();
    linkedChoices();
    fallbackAndNumerics();
    readinessAndCounters();
    production();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
