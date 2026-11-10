#include "creatures/Visibility.h"
#include "StaticStates.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <vector>

using namespace creatures;
namespace {
int failures = 0;
void check(bool value, const char *message) {
    if (!value) { std::printf("FAIL %s\n", message); failures++; }
}
visibility::Position position(double x, double y, double z) {
    return {int64_t(x * 65536), int64_t(y * 65536), int64_t(z * 65536)};
}
const BlockType air = [](int32_t, int32_t, int32_t) { return uint8_t(0); };
using Cell = std::array<int32_t, 3>;

void traversal() {
    const auto start = position(.5, .5, 1.5);
    for (const uint8_t type : {uint8_t(0), uint8_t(2), uint8_t(0x82), uint8_t(4), uint8_t(31)}) {
        const visibility::Scene scene{[=](int32_t, int32_t, int32_t) { return type; }, {}};
        const bool clear = (type & 31) == 0 || (type & 31) == 2;
        check(visibility::trace(start, {1, 0, 0}, 2, 0, 0, scene) == (clear ? 2.f : 0.f),
              "voxel category uses only the low five bits, with air/water sharing category zero");
        check(visibility::trace(start, {1, 0, 0}, 2, 1, 0, scene) == (clear ? 0.f : 2.f),
              "requested category one reverses traversal admission without Boolean normalization");
        check(visibility::trace(start, {1, 0, 0}, 2, 2, 0, scene) == 0,
              "requested category two matches neither voxel category");
    }
    std::vector<Cell> queried;
    visibility::Scene wall{[&](int32_t x, int32_t y, int32_t z) {
        queried.push_back({x, y, z});
        return uint8_t(x == 1 ? 1 : 0);
    }, {}};
    check(visibility::trace(start, {1, 0, 0}, .5f, 0, 0, wall) == .5f && queried.size() == 1,
          "entering a solid voxel exactly at maximum does not query that endpoint");
    queried.clear();
    check(visibility::trace(start, {1, 0, 0}, .6f, 0, 0, wall) == .5f && queried.size() == 2,
          "a slightly longer ray queries the crossed solid voxel at the same boundary parameter");
    queried.clear();
    visibility::Scene corner{[&](int32_t x, int32_t y, int32_t z) {
        queried.push_back({x, y, z});
        return uint8_t(x == 1 && y == 0 ? 1 : 0);
    }, {}};
    check(visibility::trace(start, {1, 1, 0}, 2, 0, 0, corner) == .5f && queried.size() == 2 &&
          queried.back() == Cell{1, 0, 1},
          "equal X/Y crossings visit the X neighbor before the diagonal, exposing a thin blocking cell");
    queried.clear();
    visibility::Scene observed{[&](int32_t x, int32_t y, int32_t z) {
        queried.push_back({x, y, z});
        return uint8_t(0);
    }, {}};
    check(visibility::trace(position(1.5, 1.5, -1), {0, 0, 1}, .5f, 0, 0, observed) == .5f &&
          queried.size() == 2 && queried[0][2] == -2 && queried[1][2] == -1,
          "an exact negative fixed multiple receives the original extra decrement and a zero-time crossing");
    queried.clear();
    check(visibility::trace(position(10.5, 1.5, 1.5), {1, 0, 0}, 1000, 0, 0, observed) == 500.5f &&
          queried.size() == 502 && queried.back()[0] == 511,
          "501 advances retain the final 502nd classification before the step cap");
    queried.clear();
    check(visibility::trace(position(10.5, 1.5, 1.5), {0, 0, 0}, 20, 0, 0, observed) == 20 &&
          queried == std::vector<Cell>{{10, 1, 1}, {9, 1, 1}},
          "zero direction retains the default ten-unit increment and decrements X");
    queried.clear();
    visibility::trace(position(10.5, 1.5, 1.5), {0, .05f, 0}, 20, 0, 0, observed);
    check(queried.size() >= 2 && queried[1] == Cell{9, 1, 1},
          "an exactly ten-unit eligible Y candidate does not replace default X");
    queried.clear();
    const visibility::Position quantizedStart{2, 65536, 65536};
    check(visibility::trace(quantizedStart, {3, 0, 0}, 1, 0, 0, wall) == 21844.f / 65536.f,
          "axis travel is integer-quantized before reciprocal scaling rather than continuously divided");
    check(visibility::trace(start, {1, 0, 0}, -3, 0, 0, {air, {}}) == -3,
          "negative maximum is returned through the upper clamp without a lower repair");
    check(std::bit_cast<uint32_t>(visibility::trace(start, {1, 0, 0}, -0.f, 0, 0, {air, {}})) == 0,
          "negative-zero maximum retains the initial positive-zero parameter");
    queried.clear();
    check(visibility::trace(position(-1, 1, 1), {1, 0, 0}, 2, 0, 0, observed) == 0 && queried.empty(),
          "out-of-world horizontal coordinates use the missing category without a scene lookup");
}

void records() {
    visibility::Record object{4, position(2, 1, 0), {2, 2, 2}, 0, 1};
    std::vector<std::array<int32_t, 2>> zones;
    int visits = 0;
    visibility::Scene scene{air, [&](int32_t x, int32_t y, const visibility::RecordVisitor &visit) {
        zones.push_back({x, y});
        visits++;
        visit(object);
    }};
    const auto from = position(.5, 1, 1);
    check(visibility::trace(from, {1, 0, 0}, 10, 0, 1, scene) == .5f,
          "active side geometry shortens the already completed voxel result");
    check(visibility::trace(from, {1, 0, 0}, 10, 0, 0, scene) == 10,
          "zero additional-geometry option omits record faces");
    for (const int kind : {1, 2, 6, 7, 9}) {
        object.kind = kind;
        check(visibility::trace(from, {1, 0, 0}, 10, 0, 1, scene) == 10,
              "other record kinds do not inherit physics-box admission");
    }
    for (const int kind : {3, 4, 5, 8}) {
        object.kind = kind;
        check(visibility::trace(from, {1, 0, 0}, 10, 0, 255, scene) == .5f,
              "each admitted kind accepts any nonzero additional-geometry byte");
    }
    object.state = 0;
    check(visibility::trace(from, {1, 0, 0}, 10, 0, 1, scene) == 10, "zero record state removes its side faces");
    object.state = 2;
    check(visibility::trace(position(.5, 0, 0), {1, 0, 0}, 10, 0, 1, scene) == .5f,
          "lower transverse and height boundaries are included");
    check(visibility::trace(position(.5, 2, 1), {1, 0, 0}, 10, 0, 1, scene) == 10 &&
          visibility::trace(position(.5, 1, 2), {1, 0, 0}, 10, 0, 1, scene) == 10,
          "upper transverse and height boundaries are excluded");
    check(visibility::trace(position(2, 1, -1), {0, 0, 1}, 10, 0, 1, scene) == 10,
          "purely vertical travel tests no top or bottom object face");
    check(visibility::trace(position(2, 1, 1), {1, 0, 0}, 10, 0, 1, scene) == 1,
          "starting inside does not become a closed-volume hit at zero");
    object.dimensions = {2, 4, 2};
    object.selector = -1;
    check(visibility::trace(from, {1, 0, 0}, 10, 0, 1, scene) == 3.5f,
          "a negative odd selector swaps X/Y extents without angle rotation");
    zones.clear();
    visibility::trace(position(255.5, .5, 1), {1, 0, 0}, .1f, 0, 1, scene);
    check(zones == std::vector<std::array<int32_t, 2>>{{0, 0}, {1, 0}},
          "geometry range retains the overshot final voxel even after the parameter is clamped");
    zones.clear();
    visibility::trace(position(255.5, 255.5, 1), {1, 1, 0}, 2, 0, 1, scene);
    check(zones == std::vector<std::array<int32_t, 2>>{{0, 0}, {0, 1}, {1, 0}, {1, 1}},
          "inclusive geometry cells visit X first and Y within each X");
    const int before = visits;
    visibility::trace(from, {1, 0, 0}, -1, 0, 1, scene);
    check(visits > before, "record metadata is still visited when a negative bound prevents face shortening");
}

void production() {
    int queries = 0;
    visibility::Scene observed{[&](int32_t, int32_t, int32_t) { queries++; return uint8_t(1); }, {}};
    check(visibility::visible(position(1, 1, 1), {65537, 65536, 65536}, observed) && queries == 0,
          "near-coincident visibility succeeds before querying even a blocking voxel");
    check(!visibility::visible(position(1, 1, 1), position(202, 1, 1), observed) && queries == 0,
          "the caller rejects distances beyond 200 before the ray");
    check(visibility::visible(position(1, 1, 1), position(201, 1, 1), {air, {}}),
          "distance exactly 200 remains admitted by the caller");

    worldgen::WorldMap map;
    auto zone = std::make_shared<worldgen::Zone>(0, 0);
    worldgen::StaticObject window;
    window.kind = 3;
    window.x = 13 * 65536;
    window.y = int64_t(10.5 * 65536);
    window.z = 2 * 65536;
    window.scale[0] = 1;
    window.scale[1] = window.scale[2] = 3;
    window.state = 1;
    zone->statics.push_back(window);
    map.add(zone);
    StaticStates states;
    states.generated(*zone);
    Npcs world;
    world.sightRecords = [&](int32_t x, int32_t y, const visibility::RecordVisitor &visit) {
        states.visibilityRecords(map, x, y, visit);
    };
    Npc npc;
    npc.fighter.id = 9;
    npc.fighter.controller = 1;
    npc.fighter.health = 100;
    npc.fighter.actionMs = 1000000;
    npc.body.place({10.5, 10.5, 3.5});
    npc.home = npc.body.blocks();
    npc.thinkMs = 32;
    world.npcs().push_back(npc);
    Combatant player;
    player.id = 1;
    Body body;
    body.place({15.5, 10.5, 3.5});
    const BlockType floor = [](int32_t, int32_t, int32_t z) { return uint8_t(z <= 0 ? 1 : 0); };
    NpcFrame frame;
    world.stepInto(0, map, {{player, body, 0, 0, false, false, true}}, 1000, 0, floor, frame);
    check(world.all().size() == 1 && world.all()[0].target == 0,
          "actual NPC acquisition is blocked by the current active spatial-record side");
    states.restore(0, 0, {0});
    world.npcs()[0].thinkMs = 32;
    world.stepInto(0, map, {{player, body, 0, 0, false, false, true}}, 1000, 0, floor, frame);
    check(world.all().size() == 1 && world.all()[0].target == 1,
          "a current state change removes occlusion without changing the baked object or ray callbacks");
}
}

int main() {
    traversal();
    records();
    production();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
