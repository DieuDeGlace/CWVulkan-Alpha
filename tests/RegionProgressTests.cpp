#include "Missions.h"
#include "creatures/Npcs.h"
#include <bit>
#include <cstdio>
#include <limits>

using namespace creatures;
static int failures = 0;
static void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}

struct Fixture {
    Missions missions;
    Combatant target, contributor;
    ActorMap<Combatant *> actors;
    std::vector<uint64_t> lookups;
    std::vector<Missions::Key> regions;
    bool allocated = true;
    Fixture() {
        target.appearanceFlags = 0x2000;
        target.backing = {0, 0, 0};
        target.relations.threats[0] = 1;
        contributor.health = -5; // contributor life is not an admission condition
        actors[0] = &contributor;
    }
    MissionArea &area(int32_t kind = 7) {
        auto &m = missions.edit(target.backing.x / 8, target.backing.y / 8).mission;
        m.kind = kind;
        m.required = 3;
        return m;
    }
    auto progress(uint8_t marker = 0, uint8_t mode = 0) {
        return missions.fatal(target, marker, mode,
            [&](int32_t x, int32_t y) { regions.emplace_back(x, y); return allocated; },
            [&](uint64_t key) -> Combatant * {
                lookups.push_back(key);
                const auto found = actors.find(key);
                return found == actors.end() ? nullptr : found->second;
            });
    }
};

static void spatialAndContributorGates() {
    for (int32_t coordinate : {INT32_MIN, -8, -7, -1, 0, 7, 8, 65535, 65536, INT32_MAX}) {
        Fixture f;
        f.target.backing.x = coordinate;
        f.area();
        const auto event = f.progress();
        const bool admitted = coordinate >= -7 && coordinate < 65536;
        check(bool(event) == admitted, "regional coordinate admission uses truncated division before bounds");
        check(f.regions.size() == size_t(admitted) && f.lookups.size() == (admitted ? 2u : 0u),
              "invalid quotients reject before block lookup and contributor lookup");
        if (event)
            check(event->x == coordinate / 8 && event->y == 0 && event->mission.count == 1 &&
                  f.regions[0] == Missions::Key{coordinate / 64, 0}, "snapshot and region use distinct quotient scales");
    }
    Fixture f;
    f.target.backing = {65535, 65535, -9};
    f.area();
    check(f.progress().has_value() && f.regions[0] == Missions::Key{1023, 1023},
          "both upper coordinate boundaries admit their allocated block, independent of descriptor ordinal");
    f.lookups.clear(); f.regions.clear();
    check(!f.progress(0, 1) && !f.progress(0, 255) && f.lookups.empty() && f.regions.empty(),
          "all nonzero world modes skip regional lookup entirely");
    f.allocated = false;
    check(!f.progress() && f.lookups.empty(), "a missing underlying block prevents contributor search");

    Fixture ordered;
    ordered.area();
    Combatant wrongRole;
    wrongRole.controller = 1;
    ordered.target.relations.threats[uint64_t(INT64_MIN)] = 1;
    ordered.target.relations.threats[UINT64_MAX] = 1; // signed minus one, absent
    ordered.target.relations.threats[uint64_t(-2)] = -0.f;
    ordered.target.relations.threats[uint64_t(-3)] = std::numeric_limits<float>::quiet_NaN();
    ordered.target.relations.threats[9] = 1;
    ordered.actors[uint64_t(INT64_MIN)] = &wrongRole;
    ordered.actors[9] = &ordered.contributor;
    const auto chosen = ordered.progress();
    check(chosen && chosen->contributor == 0 &&
          ordered.lookups == std::vector<uint64_t>{uint64_t(INT64_MIN), uint64_t(INT64_MIN), UINT64_MAX, 0, 0},
          "signed contributor order skips unordered/nonpositive values, repeats successful lookups and admits dead key zero first");
    Fixture flags;
    auto &m = flags.area(5);
    flags.target.appearanceFlags = 0;
    check(!flags.progress() && flags.lookups.empty(), "kind five still needs a nonzero descriptor marker without the flag");
    check(flags.progress(0xff)->completed(), "the raw marker byte is tested nonzero without boolean normalization");
    flags.lookups.clear();
    check(!flags.progress(0xff) && flags.lookups.empty(), "an already complete record does not repeat search or events");
    m.state = 255; m.kind = 7;
    check(!flags.progress(0xff), "a marker alone does not admit other selectors");
    flags.target.appearanceFlags = 0x2800;
    check(flags.progress()->mission.count == 1, "the zero-credit flag does not suppress independent regional progress");
}

static void selectorAndSnapshotRules() {
    for (int32_t kind : {1, 2, 3, 4, 5, 13}) {
        Fixture f;
        auto &m = f.area(kind);
        m.count = -9;
        const auto event = f.progress();
        check(event && event->completed() && event->mission.count == -9 && m.count == -9,
              "completion selectors retain the prior count");
    }
    for (int32_t kind : {7, 8, 9, 10, 11, 12}) {
        Fixture f;
        f.area(kind);
        const auto first = f.progress();
        const auto second = f.progress();
        const auto third = f.progress();
        check(first && second && third && first->mission.count == 1 && first->mission.state == 1 &&
              second->mission.count == 2 && second->mission.state == 1 && third->completed() &&
              third->mission.count == 3 && !f.progress(), "each counting invocation snapshots once, including incomplete state one");
        int32_t x = -1, y = -1;
        const auto bytes = first->record();
        check(MissionArea::fromRecord(bytes.data(), x, y).count == 1 && x == 0 && y == 0 &&
              bytes[8] == 0 && bytes[12] == 0 && bytes[38] == 0 && bytes[39] == 0,
              "early snapshots retain their count and engine-initialized padding after later state changes");
    }
    for (int32_t kind : {INT32_MIN, -1, 0, 6, 14, INT32_MAX}) {
        Fixture f;
        auto &m = f.area(kind);
        m.state = 255; m.count = 7;
        f.target.relations.threats[1] = 1;
        f.actors[1] = &f.contributor;
        check(!f.progress() && m.state == 255 && m.count == 7 && f.missions.takeChanged().empty() &&
              f.lookups == std::vector<uint64_t>{0, 0}, "unsupported selector emits nothing and does not try a second qualified contributor");
    }
    Fixture wrap;
    auto &m = wrap.area();
    m.count = INT32_MAX;
    check(wrap.progress()->mission.count == INT32_MIN && m.state == 1, "count wraps before its signed goal comparison");
    m.count = -4; m.required = -2;
    check(!wrap.progress()->completed() && wrap.progress()->completed(), "negative goals retain signed threshold equality without positivity gates");
}

static void cellTraversalAndPublication() {
    Combatant target;
    target.backing = {-1, -7, 0};
    std::vector<Missions::Key> visits;
    visitMissionCells(target, [&](int32_t x, int32_t y) { visits.emplace_back(x, y); });
    check(visits.size() == 64 && visits[0] == Missions::Key{0, 0} && visits[7] == Missions::Key{0, 7} &&
          visits[8] == Missions::Key{1, 0} && visits.back() == Missions::Key{7, 7}, "unchanged completion cells use X outer and truncated starting coordinates");
    visits.clear(); target.backing = {0, 0, 0};
    visitMissionCells(target, [&](int32_t x, int32_t y) {
        visits.emplace_back(x, y);
        if (visits.size() == 1) target.backing.y = 8;
    });
    check(visits.size() == 72 && visits[15] == Missions::Key{0, 15} && visits[16] == Missions::Key{1, 8} &&
          visits.back() == Missions::Key{7, 15}, "callback coordinate changes affect current upper limits and later row starts");
    visits.clear(); target.backing = {65535, 65535, 0};
    visitMissionCells(target, [&](int32_t x, int32_t y) { visits.emplace_back(x, y); });
    check(visits.size() == 64 && visits.front() == Missions::Key{65528, 65528} && visits.back() == Missions::Key{65535, 65535},
          "cell lookup retains its own inclusive upper world boundary");

    Fixture f; f.area();
    NpcFrame frame;
    NpcFrame::Cursor cursor;
    frame.missions.push_back(*f.progress());
    frame.missions.push_back(*f.progress());
    const auto added = frame.addedSince(cursor);
    check(added.missions.size() == 2 && added.missions[0].mission.count == 1 && added.missions[1].mission.count == 2 &&
          frame.addedSince(cursor).missions.empty(), "retained frame snapshots preserve order and new-output views drain once");
    std::vector<std::vector<uint8_t>> output;
    f.missions.appendUpdates(frame.missions, f.missions.takeChanged(), {}, output);
    check(output.size() == 2 && output[0][40] == 1 && output[1][40] == 2, "publication retains intermediate events and omits an identical final snapshot");
    output.clear();
    f.missions.appendUpdates(frame.missions, {}, {{0, 0}}, output);
    check(output.size() == 3 && output.back()[40] == 2, "an explicit requested cell still receives its full final state");
    f.missions.edit(0, 0).mission.count = 9;
    output.clear();
    f.missions.appendUpdates(frame.missions, {{0, 0}}, {}, output);
    check(output.size() == 3 && output.back()[40] == 9, "a later changed state remains after earlier immediate snapshots");
    f.missions.edit(0, 0).mission.count = 1;
    output.clear();
    f.missions.appendUpdates(frame.missions, {{0, 0}}, {}, output);
    check(output.size() == 3 && output.back()[40] == 1,
          "a final return to an earlier value must follow the last immediate snapshot instead of being omitted");
    const auto capacity = frame.missions.capacity();
    frame.reset();
    check(frame.missions.empty() && frame.missions.capacity() == capacity, "frame reset retains regional buffer capacity");
}

static void actualFatalBoundary() {
    Npcs npcs;
    Npc victim;
    victim.fighter.id = 72;
    victim.fighter.controller = 1;
    victim.fighter.health = 1;
    victim.fighter.level = 5;
    victim.fighter.backing = {8, 16, 0};
    victim.fighter.linkedId = 73;
    victim.spawn = Npc::NO_SPAWN;
    victim.boss = 0xff;
    victim.body.place({10, 10, 3});
    npcs.npcs().push_back(victim);
    victim.fighter.id = 73;
    victim.fighter.controller = 2;
    victim.fighter.health = 17;
    npcs.npcs().push_back(victim);
    Combatant player;
    player.id = 0;
    Body body;
    body.place({11, 10, 3});
    PlayerView view{player, body, 0, 0, false, false, true};
    Missions missions;
    missions.edit(1, 2).mission.kind = 5;
    NpcFrame frame;
    int callbacks = 0;
    npcs.onFatalLoot = [&](const Combatant &, const Body &, std::span<const ItemRecord>) {
        check(npcs.all()[1].fighter.health == 17 && !npcs.all()[0].fatalCredit,
              "fatal loot precedes linked health and credit preparation");
        npcs.npcs()[0].fighter.level = 10;
    };
    npcs.onFatalRegion = [&](Combatant &target, uint8_t marker, const auto &lookup, NpcFrame &events) {
        callbacks++;
        check(marker == 0xff && npcs.all()[1].fighter.health == 0 && events.sounds[0].sound == 27 && target.actionMs == 0,
              "production fatal regional service follows linked health and retains the raw marker byte");
        check(npcs.all()[0].fatalCredit && std::bit_cast<uint32_t>(*npcs.all()[0].fatalCredit) == 0x40c69ee8,
              "credit uses post-loot state and retains its fractional binary32 value before regional callbacks");
        target.level = 100; // later state must not replace the already prepared common amount
        if (const auto progress = missions.fatal(target, marker, 0, [](int32_t, int32_t) { return true; }, lookup))
            events.missions.push_back(*progress);
    };
    HitRecord hit;
    hit.attacker = 0; hit.target = 72; hit.amount = 1;
    npcs.applyWorldHit(hit, {&view, 1}, frame);
    npcs.applyWorldHit(hit, {&view, 1}, frame);
    check(callbacks == 1 && frame.missions.size() == 1 && frame.missions[0].completed() &&
          frame.missions[0].x == 1 && frame.missions[0].y == 2 && frame.missions[0].contributor == 0,
          "the actual world hit completes cached-origin regional state before returning, without dead-target replay");
    worldgen::WorldMap map;
    map.add(std::make_shared<worldgen::Zone>(0, 0));
    const BlockType block = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
    npcs.stepInto(0, map, {view}, 1000, 9, block, frame);
    check(!frame.deaths.empty() && frame.deaths[0].regionApplied && frame.missions.size() == 1 && callbacks == 1,
          "later corpse output marks completed regional admission so callers cannot replay it");
    check(frame.kills.size() == 1 && frame.kills[0].experience == 6 && frame.experience == 6,
          "the retained later adapter converts the prepared amount instead of recomputing changed target state");
    npcs.npcs()[0].fighter.health = 20;
    npcs.npcs()[0].deadMs = 0;
    npcs.stepInto(0, map, {view}, 1000, 9, block, frame);
    check(!npcs.all()[0].fatalCredit, "the next admitted live update clears the engine's prior-fatal amount bridge");
}

static void descriptorDisposal() {
    auto zone = std::make_shared<worldgen::Zone>(1, 2);
    worldgen::Spawn descriptor;
    descriptor.x = int64_t(266) * 65536;
    descriptor.y = int64_t(522) * 65536;
    descriptor.z = 2 * 65536;
    descriptor.type = 19;
    descriptor.radius = 1000;
    zone->spawns.push_back(descriptor);
    worldgen::WorldMap map;
    map.add(zone);
    Combatant local;
    local.id = 1;
    Body body;
    body.place({266, 522, 3});
    worldgen::Random random(123);
    Npcs npcs(random);
    npcs.setExtraSpawns(1, 2, 1, {descriptor});
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
    NpcFrame frame;
    npcs.stepInto(0, map, {{local, body, 0, 0, false, false, true}}, 1000, 0, floor, frame);
    const auto actors = npcs.all().size();
    const auto draws = random.count();
    check(actors == 2, "descriptor disposal fixture activates a retained prefix and removable tail independently");
    Combatant target;
    target.backing = {1, 2, 1};
    npcs.trimMissionSpawns(target, map);
    check(npcs.all().size() == actors && random.count() == draws,
          "fatal descriptor disposal retains instantiated actors and consumes no gameplay draws");
    npcs.setExtraSpawns(1, 2, 1, {});
    check(npcs.all().size() == actors,
          "rebuilding after fatal disposal cannot remove actors through descriptors that no longer exist");
    npcs.setExtraSpawns(1, 2, 1, {descriptor}, false);
    for (auto &npc : npcs.npcs())
        if (npc.fighter.id == Npcs::spawnId(1, 2, 1)) {
            npc.spawn = Npc::NO_SPAWN;
            npc.fighter.controller = 5; // association/role changed, but this descriptor still names its key
        }
    Npc independent;
    independent.fighter.id = 777;
    independent.zone = {1, 2};
    independent.spawn = Npc::NO_SPAWN;
    npcs.npcs().push_back(independent);
    npcs.setExtraSpawns(1, 2, 1, {});
    check(npcs.all().size() == 2 && npcs.all()[0].spawn == 0 && npcs.all()[1].fighter.id == 777,
          "rebuilding matches retained descriptor IDs even after taming, while preserving independent actors and the prefix");
}

int main() {
    spatialAndContributorGates();
    selectorAndSnapshotRules();
    cellTraversalAndPublication();
    actualFatalBoundary();
    descriptorDisposal();
    if (failures) std::printf("%d FAILED\n", failures);
    else std::puts("Regional progress checks passed");
    return failures ? 1 : 0;
}
