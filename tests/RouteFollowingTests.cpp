#include "creatures/RouteFollowing.h"
#include "creatures/Paths.h"
#include "StaticStates.h"
#include <bit>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
route::Position raw(search::Cell cell) {
    return {int64_t(cell[0]) * 65536, int64_t(cell[1]) * 65536, int64_t(cell[2]) * 65536};
}
struct Fixture {
    search::State state;
    Body body;
    search::FollowingServices services;
    std::vector<search::Cell> queries;
    std::vector<float> roots;
    int setups = 0;
    Fixture() {
        body.size = {2, 2, 2};
        body.place({10.5, 20.5, 6});
        body.intent = {9, 8, 7};
        body.velocity = {1, 2, 3};
        body.push = {4, 5, 6};
        state.start = raw({10, 20, 5});
        state.goal = raw({100, 20, 5});
        state.anchor = {10, 20, 5};
        state.path = {{10, 20, 5}, {11, 20, 5}};
        state.radius = .1f;
        services.scene.cell = [&](int32_t x, int32_t y, int32_t z) {
            queries.push_back({x, y, z}); return uint8_t(0);
        };
        services.setup = [&] {
            setups++;
            const route::Dimensions dimensions{body.size.x, body.size.y, body.size.z};
            state.setup(dimensions);
        };
        services.squareRoot = [&](float value) { roots.push_back(value); return float(std::sqrt(double(value))); };
    }
    bool run(int32_t elapsed = 1) { return search::follow(state, body, elapsed, services); }
};
void entryAndTiming() {
    Fixture arrival;
    arrival.state.goal = raw({10, 20, 5});
    arrival.state.counter = 19;
    arrival.state.exclusions.insert(123);
    arrival.state.records[{}] = {};
    arrival.state.pending.insert(search::Cell{});
    check(!arrival.run() && arrival.state.path.empty() && arrival.state.records.empty() && arrival.state.pending.empty() &&
          arrival.state.exclusions.empty() && arrival.state.counter == 19 && arrival.body.intent == glm::vec3(0) &&
          arrival.body.velocity == glm::vec3(1, 2, 3) && arrival.body.push == glm::vec3(4, 5, 6) && arrival.queries.empty(),
          "arrival clears route containers and steering while preserving velocity, auxiliary output and counter");
    Fixture empty;
    empty.state.path.clear();
    empty.state.counter = 3001;
    check(!empty.run() && empty.setups == 0 && empty.body.intent == glm::vec3(0), "empty list returns before timer refresh");
    Fixture expired;
    expired.state.counter = 3001;
    check(!expired.run(-7) && expired.setups == 1 && expired.state.start == route::Position{10 * 65536 + 32768, 20 * 65536 + 32768, 5 * 65536 + 32768} &&
          expired.state.path.empty() && expired.state.records.size() == 1 && expired.state.counter == 0 && expired.queries.empty(),
          "expired counter writes the entry-offset raw start and seeds without expanding or rebuilding, even for negative elapsed");
    Fixture boundary;
    boundary.state.counter = 3000;
    boundary.run(0);
    check(boundary.setups == 0 && boundary.state.counter == 0, "timer equality does not refresh and nearest-zero resets counter");
    Fixture small;
    small.body.contact = 0x40;
    small.state.path.resize(1);
    small.state.goal = raw({100, 20, 5});
    int records = 0;
    small.services.scene.records = [&](int32_t, int32_t, const route::RecordVisitor &) { records++; };
    check(small.run() && small.body.intent == glm::vec3(0) && small.body.velocity == glm::vec3(1, 2, 3) &&
          small.body.push == glm::vec3(4, 5, 6) && records == 0,
          "small-direction true shortcut precedes forced vertical direction, object scan and auxiliary clearing");
}
void partialAndFar() {
    Fixture blocked;
    blocked.body.position[0] -= 16384;
    blocked.body.mode = 0xa5a54040;
    check(!blocked.run() && blocked.body.intent == glm::vec3(0) && blocked.body.push == glm::vec3(4, 5, 6) &&
          blocked.body.velocity == glm::vec3(1, 2, 3) && blocked.body.mode == 0xa5a54040,
          "partial route without the computed climb bit returns false and preserves other mode bits and motion");
    Fixture admitted;
    admitted.body.position[0] -= 16384;
    admitted.body.contact = 4;
    check(admitted.run(-10) && admitted.body.mode == 1 && admitted.body.velocity == glm::vec3(2, 0, 0) &&
          admitted.body.intent == glm::vec3(5, 0, 0) && admitted.body.push == glm::vec3(0) && admitted.roots.empty(),
          "admitted partial route keeps sub-unit direction and directly assigns velocity independently of elapsed");
    Fixture far;
    far.state.path = {{12, 20, 5}};
    far.state.goal = raw({12, 20, 5});
    far.state.counter = 17;
    check(far.run(-1) && far.body.intent == glm::vec3(40, 0, 0) && far.body.velocity == glm::vec3(1, 2, 3) &&
          far.body.push == glm::vec3(0) && far.state.counter == 17 && far.roots == std::vector<float>{4},
          "nearest squared distance4 selects far steering40 without any velocity blending or elapsed counter addition");
    Fixture tie;
    tie.body.position[0] = 11 * 65536 + 32768;
    tie.state.path = {{10, 20, 5}, {12, 20, 5}};
    tie.body.contact = 4;
    check(tie.run(0) && tie.body.intent.x == 20 && tie.body.velocity.x == 8,
          "equidistant nearest nodes choose the later list entry");
}
void recurrenceAndLookahead() {
    const std::array<uint32_t, 6> expected{0x3f800000, 0x3f80a3d7, 0x3f814784, 0x3f81eb07, 0x3f828e60, 0x3f833190};
    for (int elapsed = -1; elapsed <= 5; elapsed++) {
        Fixture f;
        f.state.goal = raw({11, 20, 5});
        f.services.scene.cell = [](int32_t, int32_t, int32_t) { return uint8_t(1); };
        check(f.run(elapsed) && std::bit_cast<uint32_t>(f.body.velocity.x) == expected[size_t(std::max(elapsed, 0))] &&
              f.body.velocity.z == 3 && f.body.intent == glm::vec3(20, 0, 0),
              "grounded horizontal blend retains every binary32 recurrence including zero/negative elapsed");
    }
    Fixture negative;
    negative.state.goal = raw({11, 20, 5});
    negative.services.scene.cell = [](int32_t, int32_t, int32_t) { return uint8_t(1); };
    check(negative.run(std::numeric_limits<int32_t>::min()) && negative.body.velocity == glm::vec3(1, 2, 3),
          "minimum signed elapsed performs no recurrence without decrement overflow");
    Fixture rising;
    rising.state.path = {{10, 20, 5}, {11, 20, 6}};
    rising.state.goal = raw({11, 20, 6});
    rising.services.scene.cell = [](int32_t, int32_t, int32_t z) { return uint8_t(z == 5 ? 1 : 0); };
    check(rising.run() && std::bit_cast<uint32_t>(rising.body.velocity.z) == 0x40433333 && rising.body.intent.z == 20,
          "nonnegative near direction blends z toward4 at0.05 before horizontal work");
    for (bool third : {false, true}) {
        Fixture f;
        f.state.path = {{10, 20, 5}, {20, 20, 6}, {30, 20, 6}};
        f.state.goal = raw({30, 20, 6});
        f.services.scene.cell = [&](int32_t x, int32_t y, int32_t z) {
            f.queries.push_back({x, y, z});
            return uint8_t(z == 4 || (third && x >= 29) ? 1 : 0);
        };
        check(f.run() && f.queries.size() == (third ? 11 : 19) && f.queries[0] == search::Cell{9, 19, 4} &&
              f.queries[1] == search::Cell{19, 19, 5} && f.queries[10] == search::Cell{29, 19, 5} &&
              f.roots == std::vector<float>{401} && f.body.intent.z == 20,
              "height-changing lookahead queries three nodes in order and normalizes only the final direction");
        check(third ? (f.body.velocity.z == 3 && f.body.velocity.x > 1 && f.body.velocity.x < 1.01f) :
                      (f.body.velocity.z > 3 && f.body.velocity.x > 1.06f),
              "third terrain result independently controls vertical admission and horizontal coefficient regime");
    }
    Fixture water;
    water.body.position[2] += 32768;
    water.body.contact = 4 | 2;
    check(water.run(0) && water.body.intent.z == -40 && water.body.velocity.z == 3,
          "negative direction in contact-bit2 becomes minus2 without renormalization or vertical blending");
}
void rerooting() {
    search::State s;
    s.anchor = {9, 0, 0};
    s.path = {{9, 0, 0}, {10, 0, 0}, {11, 0, 0}, {12, 0, 0}};
    const auto path = s.path;
    s.records[{9, 0, 0}] = {90, 50, 140, {9, 0, 0}};
    s.records[{10, 0, 0}] = {100, 40, 140, {9, 0, 0}};
    s.records[{11, 0, 0}] = {110, 30, 140, {10, 0, 0}};
    s.records[{12, 0, 0}] = {120, 20, 140, {11, 0, 0}};
    s.records[{20, 0, 0}] = {1, 0, 1, {20, 0, 0}};
    s.pending = {{10, 0, 0}, {11, 0, 0}, {12, 0, 0}, {20, 0, 0}};
    s.exclusions.insert(321);
    s.reroot({10, 0, 0});
    check(s.anchor == search::Cell{10, 0, 0} && s.start == raw({10, 0, 0}) && s.path == path &&
          s.records.size() == 3 && s.pending.size() == 3 && !s.records.contains({9, 0, 0}) && !s.pending.contains({20, 0, 0}) &&
          s.records.at({10, 0, 0}) == search::Node{100, 40, 140, {10, 0, 0}} && s.remembered == search::Cell{12, 0, 0} &&
          s.exclusions.contains(321),
          "reroot accepts the reference itself, retains reaching chains and scores, rejects self-terminated branches and keeps the list");
    s.clear();
    s.records[{-1, 0, 0}] = {9, 1000, 1009, {-1, 0, 0}};
    for (int i = 0; i < 72; i++) {
        s.records[{i, 0, 0}] = {10, 7, 17, {-1, 0, 0}};
        s.pending.insert({i, 0, 0});
    }
    s.reroot({-1, 0, 0});
    check(s.pending.size() == 70 && s.pending.contains({69, 0, 0}) && !s.pending.contains({70, 0, 0}) &&
          s.records.size() == 71 && s.remembered == search::Cell{0, 0, 0},
          "reroot keeps70 by destination score with first lexicographic ties and recomputes remembered order");
}
void objectEffectsAndFallback() {
    Fixture f;
    f.body.contact = 0x40;
    route::Record record{123, 1, {13 * 65536 + 32768, 20 * 65536 + 32768, 5 * 65536}, 0, {2, 2, 2}, 1, {-1, -1, 0}};
    f.state.exclusions.insert(123);
    int stores = 0;
    f.services.scene.records = [&](int32_t, int32_t, const route::RecordVisitor &visit) { visit(record); visit(record); };
    f.services.activate = [&](const route::Record &r) { check(r.identity == 123, "activation preserves spatial identity"); record.state = 0; stores++; };
    check(f.run(0) && stores == 1 && record.state == 0 && f.body.intent.z == 60 && f.body.velocity.z == 3,
          "forced z3 reaches inclusive inflated overlap, ignores search exclusions/tag and observes immediate state at the duplicate");
    PathSearch path;
    path.state.path = {{20, 30, 40}};
    Body body;
    body.size = {9, 8, 100};
    body.position[0] = 19 * 65536 + 16384;
    body.position[1] = 29 * 65536;
    body.position[2] = 39 * 65536 + 49152;
    check(path.fallback(body) == glm::vec3(1.25f, 1.5f, .75f),
          "false-result fallback uses tail plus half on every axis independently of actor height");
    auto zone = std::make_shared<worldgen::Zone>(0, 0);
    worldgen::StaticObject object;
    object.kind = 1; object.x = 13 * 65536 + 32768; object.y = 20 * 65536 + 32768; object.z = 5 * 65536;
    object.scale[0] = object.scale[1] = object.scale[2] = 2; object.state = 1; object.timer = 123;
    zone->statics.push_back(object);
    worldgen::WorldMap map;
    map.add(zone);
    StaticStates states;
    states.generated(*zone);
    Fixture live;
    live.body.contact = 0x40;
    live.services.scene.records = [&](int32_t x, int32_t y, const route::RecordVisitor &visit) { states.routeRecords(map, x, y, visit); };
    live.services.activate = [&](const route::Record &r) { states.activateRouteRecord(map, r); };
    check(live.run(0) && states.zone(0, 0)->at(0).state == 0 && states.zone(0, 0)->at(0).timerMs == 0 &&
          states.takeChanged() == std::set<StaticStates::Key>{{0, 0}},
          "resident ordered activation reaches actual state/timer and change publication");
}

void worldCaller() {
    auto zone = std::make_shared<worldgen::Zone>(1, 0);
    worldgen::Spawn spawn;
    spawn.x = 276 * 65536; spawn.y = 20 * 65536; spawn.z = 5 * 65536;
    spawn.type = 37; spawn.controller = 3; spawn.finished = true;
    zone->spawns.push_back(spawn);
    worldgen::WorldMap map;
    map.add(zone);
    Combatant player;
    player.id = 1; player.health = maxHealth(player);
    Body playerBody;
    playerBody.place({276.5, 150.5, 6});
    const BlockType flat = [](int32_t, int32_t, int32_t z) { return uint8_t(z < 5 ? 1 : 0); };
    Npcs npcs;
    const auto tick = [&](int32_t elapsed) { npcs.step(float(elapsed), map, {{player, playerBody, 0, 0}}, 43200000, 0, flat); };
    tick(0);
    check(npcs.all().size() == 1, "world follower fixture creates one isolated actor");
    if (npcs.all().empty()) return;
    Npc &npc = const_cast<Npc &>(npcs.all().front()); // underlying test-owned actor is mutable
    npc.thinkMs = -100000;
    npc.body.size = {2, 2, 2};
    npc.body.place({276.25, 20.5, 6});
    npc.body.contact = 0;
    npc.body.mode = npc.fighter.mode = 4 | 0x80;
    npc.path.start({276.5, 20.5, 5}, {286.5, 20.5, 5}, .1f, npc.body.size);
    npc.path.state.path = {{276, 20, 5}, {277, 20, 5}};
    tick(0);
    check((npc.body.mode & 0x84) == 0x84 && npc.body.look == glm::vec3(1.25f, 0, -.5f),
          "actual world false/nonempty caller sets both flags and tail-plus-half fallback");
    npc.fighter.hitStun = 10;
    npc.body.look = {7, 8, 9};
    tick(1);
    check((npc.body.mode & 0x84) == 4 && npc.body.look == glm::vec3(7, 8, 9),
          "actual world stun bypass clears80 before admission while retaining4 and fallback");
    npc.fighter.hitStun = 0;
    npc.body.place({286.5, 20.5, 6});
    npc.body.mode = npc.fighter.mode = 4;
    tick(0);
    check(npc.path.empty() && (npc.body.mode & 0x84) == 0x80 && npc.body.look == glm::vec3(7, 8, 9),
          "actual world false/emptied caller sets80 without writing4 or fallback");
    npc.path.state.counter = std::numeric_limits<int32_t>::max();
    tick(1);
    check(npc.path.state.counter == std::numeric_limits<int32_t>::min(),
          "actual living world advances the signed counter with32-bit wrap after integration even without a path");
}
}

int main() {
    entryAndTiming(); partialAndFar(); recurrenceAndLookahead(); rerooting(); objectEffectsAndFallback();
    worldCaller();
    if (failures) std::printf("%d route-following failures\n", failures);
    else std::puts("RouteFollowingTests PASS");
    return failures ? 1 : 0;
}
