// Bounded stored-light contracts from alpha-native-light-rectangle-propagation.md.
// Synthetic columns distinguish absent neighbors, dark open neighbors and
// implicit water. No game installation or captured terrain is needed.
#include "CreatureShading.h"
#include "Frame.h"
#include "worldgen/BlockLight.h"
#include "worldgen/Terrain.h"
#include <algorithm>
#include <cstdio>
#include <memory>

using namespace worldgen;

namespace {
int failures = 0;

void expect(int actual, int expected, const char *name) {
    if (actual != expected) {
        std::printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        failures++;
    }
}

void wall(Zone &zone, int x, int y, int bottom = 1) {
    Column &column = zone.column(x, y);
    column.bottom = bottom;
    column.blocks.assign(3, Block{100, 100, 100, ROCK});
}

void chamber(Zone &zone, int x, int y, int bottom = 1) {
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            if (zone.contains(x + dx, y + dy))
                wall(zone, x + dx, y + dy, bottom);
    zone.column(x, y).blocks[1] = Block{255, 255, 255, AIR};
}

bool storedAs(Block actual, Block expected) {
    return actual.r == expected.r && actual.g == expected.g && actual.b == expected.b &&
           actual.typeByte == expected.typeByte;
}

void lightRectangle() {
    {
        auto left = std::make_shared<Zone>(0, 0);
        auto right = std::make_shared<Zone>(1, 0);
        chamber(*left, 255, 32);
        wall(*right, 256, 32);
        auto &exterior = right->column(256, 32).blocks[1];
        exterior = {255, 37, 200, uint8_t(WATER | KEEP)};
        WorldMap map;
        map.add(left);
        map.add(right);
        const LightRectangle destination{{255, 32}, {256, 33}};
        BlockLightUpdate(map, destination).store(*left);
        const auto &inside = left->column(255, 32).blocks[1];
        expect(storedAs(inside, {170, 170, 170, AIR}), 1, "resident halo reads the exterior working byte");
        expect(storedAs(exterior, {255, 37, 200, uint8_t(WATER | KEEP)}), 1, "the exterior bytes are untouched");
        exterior.b = 0;
        BlockLightUpdate(map, destination).store(*left);
        expect(storedAs(inside, {4, 4, 4, AIR}), 1, "the exterior committed byte cannot seed the pass");
        BlockLightUpdate(*left, destination).store(*left);
        expect(storedAs(inside, {0, 0, 0, AIR}), 1, "explicit area access excludes the resident neighbor");
    }
    {
        Zone zone;
        for (int x = 99; x <= 103; ++x)
            for (int y = 31; y <= 33; ++y)
                wall(zone, x, y);
        auto &inside = zone.column(100, 32).blocks[1];
        auto &margin = zone.column(101, 32).blocks[1];
        auto &outside = zone.column(102, 32).blocks[1];
        inside = {80, 40, 20, AIR};
        margin = {19, 42, 64, uint8_t(AIR | DRY_AIR)};
        outside = {255, 90, 200, AIR};
        const LightRectangle destination{{100, 32}, {101, 33}};
        BlockLightUpdate(zone, destination, 1).store(zone);
        expect(storedAs(inside, {144, 144, 144, AIR}), 1, "the destination commits the final round to all three lanes");
        expect(storedAs(margin, {19, 170, 170, uint8_t(AIR | DRY_AIR)}), 1, "margin updates work while preserving committed light");
        expect(storedAs(outside, {255, 90, 200, AIR}), 1, "cells outside the expanded pass are untouched");
        BlockLightUpdate(zone, destination).store(zone);
        expect(storedAs(inside, {144, 144, 144, AIR}), 1, "the next call samples retained margin work");

        outside = {255, 11, 5, AIR};
        const LightRectangle empty{{101, 32}, {101, 33}};
        BlockLightUpdate(zone, empty, 1).store(zone);
        expect(margin.r, 19, "even a nonempty work area preserves committed light with empty destination");
        expect(margin.b != 170 && margin.b == margin.g, 1, "an empty destination still advances both work lanes");
    }
    {
        Zone zone;
        for (int x = 19; x <= 42; ++x)
            for (int y = 31; y <= 33; ++y)
                wall(zone, x, y);
        for (int x = 20; x <= 41; ++x)
            zone.column(x, 32).blocks[1] = {3, 90, 216, AIR};
        zone.column(20, 32).blocks[2] = {3, 9, 18, AIR};
        BlockLightUpdate(zone, {{20, 32}, {42, 33}}).store(zone);
        constexpr uint8_t light[] = {255, 216, 183, 155, 131, 111, 94, 79, 67, 56, 47, 39, 33, 28, 23, 19, 16};
        for (size_t n = 0; n < std::size(light); ++n)
            expect(zone.light(20 + int32_t(n), 32, 2), light[n], "each synchronous corridor distance");
        expect(zone.light(37, 32, 2), 4, "the seventeenth cell receives the floor after sixteen rounds");
        expect(storedAs(zone.column(36, 32).blocks[2], {100, 100, 100, ROCK}), 1, "opaque fields stay untouched");
    }
    {
        Zone zone;
        chamber(zone, 32, 32, -2);
        auto &other = zone.column(31, 32).blocks[1];
        const LightRectangle destination{{32, 32}, {33, 33}};
        other = {255, 255, 0, uint8_t(WATER | KEEP)};
        BlockLightUpdate(zone, destination).store(zone);
        expect(zone.light(32, 32, -1), 4, "stored flagged water reads its own low working byte");
        other = {255, 255, 0, AIR};
        BlockLightUpdate(zone, destination).store(zone);
        expect(zone.light(32, 32, -1), 216, "implicit water contributes full light for the same bytes");
        expect(storedAs(other, {255, 255, 0, AIR}), 1, "neighbor substitution keeps the raw tag and bytes");
    }
}

void residentLight() {
    auto left = std::make_shared<Zone>(0, 0);
    auto right = std::make_shared<Zone>(1, 0);
    chamber(*left, 255, 32);
    chamber(*right, 256, 32);
    left->column(255, 32).blocks[1] = {19, 22, 7, AIR};
    right->column(256, 32).blocks[1] = {35, 95, 15, AIR};
    right->column(257, 32).blocks[1] = {50, 120, 150, GLOWING};
    WorldMap resident;
    resident.add(left);
    resident.add(right);
    resident.acknowledgeZoneContents({0, 0});
    resident.appendPlacedObject({1, 0}, {0, 256, 32, 2});
    const WorldMap earlier = resident.neighbourhood(0, 0);

    BlockLightUpdate::AreaCopies copies;
    BlockLightUpdate(resident, {{256, 32}, {257, 33}}).store(resident, copies);
    const auto *litRight = resident.zone(1, 0);
    expect(litRight->light(256, 32, 2), 216, "the first publication updates a resident area");
    expect(resident.zone(0, 0) == left.get(), 1, "a neighboring read does not copy the halo area");

    BlockLightUpdate(resident, {{255, 32}, {256, 33}}).store(resident, copies);
    const auto *litLeft = resident.zone(0, 0);
    expect(litLeft->light(255, 32, 2), 183, "the next preparation reads the first published working byte");
    expect(storedAs(earlier.read(255, 32, 2), {19, 22, 7, AIR}) &&
               storedAs(earlier.read(256, 32, 2), {35, 95, 15, AIR}),
           1, "workers holding earlier terrain keep both original areas");

    BlockLightUpdate(resident, {{255, 32}, {257, 33}}).store(resident, copies);
    expect(resident.zone(0, 0) == litLeft && resident.zone(1, 0) == litRight, 1,
           "a later rectangle uses the existing writable copies during the same batch");
    const WorldMap earlierFrame = resident;
    const Block previous = earlierFrame.read(255, 32, 2);
    auto relit = std::make_shared<Zone>(*litLeft);
    relit->column(254, 32).blocks[1].typeByte = GLOWING;
    BlockLightUpdate(*relit, {{255, 32}, {256, 33}}).store(resident, copies);
    expect(resident.light(255, 32, 2), 216, "a later frame publishes the new lighting");
    expect(previous.r != 216 && storedAs(earlierFrame.read(255, 32, 2), previous), 1,
           "a snapshot of a previously writable area keeps every stored light lane");
    expect(resident.zone(0, 0) != litLeft && resident.zone(1, 0) == litRight, 1,
           "a shared area is copied only when a new rectangle publishes to it");
    expect(resident.placedObjects({1, 0}) && resident.placedObjects({1, 0})->size() == 1 &&
               !resident.needsZoneContents({0, 0}),
           1, "resident light publication preserves the area's network and placement records");
}

void destructionLighting() {
    constexpr int64_t one = 65536;
    const auto bomb = destructionLightBounds({one / 2, -one / 2, 0}, 8.f);
    expect(bomb.lower == glm::ivec2(-7, -8) && bomb.upper == glm::ivec2(8, 7), 1,
           "Bomb divides each fixed endpoint toward zero");
    const auto strike = destructionLightBounds({3 * one / 4, one / 4, INT64_MIN}, 8.6f);
    expect(strike.lower == glm::ivec2(-7, -8) && strike.upper == glm::ivec2(9, 8), 1,
           "strike lighting keeps the uncapped fractional radius and ignores the vertical input");

    WorldMap resident;
    auto area = std::make_shared<Zone>(0, 0);
    for (const auto x : {92, 108, 116}) {
        auto &column = area->column(x, 100);
        column.bottom = 3;
        column.blocks = {{19, 37, 51, AIR}};
    }
    resident.add(area);
    relightAfterDestruction(resident, {100 * one + one / 2, 100 * one + one / 2, 3 * one}, 8.f);
    expect(storedAs(resident.read(92, 100, 3), {255, 255, 255, AIR}), 1,
           "immediate lighting commits inside the half-open destruction bounds");
    expect(storedAs(resident.read(108, 100, 3), {19, 255, 255, AIR}), 1,
           "the eight-cell working margin leaves its committed byte intact");
    expect(storedAs(resident.read(116, 100, 3), {19, 37, 51, AIR}), 1,
           "the exclusive upper boundary preserves the first exterior column");
    expect(storedAs(area->read(92, 100, 3), {19, 37, 51, AIR}), 1,
           "lighting after destruction keeps earlier terrain snapshots intact");
}

bool same(glm::vec4 a, glm::vec4 b) { return a == b; }

// alpha-rendering.md §11.2 and rulings §16: the creature material, the motion-trail test and
// the draw sequence (ghosts, then the body; a translucent body is deferred).
void creatureShading() {
    using namespace creature_shading;
    expect(standingLight(13, 0), 255, "type 13 counts full light");
    expect(standingLight(0, 2), 5, "empty blocks count at least 5");
    expect(standingLight(2, 200), 200, "water counts its light");
    expect(standingLight(6, 200), 0, "other blocks count 0");

    creatures::Combatant c;
    expect(same(material(255, c, false), glm::vec4(1)), 1, "an untinted creature in full light");
    expect(same(material(51, c, false), glm::vec4(1, 1, 1, 51 / 255.f)), 1, "the light scales the sun term");
    c.slowMs = 1;
    c.buffs = {{1}, {4}, {2}};
    const glm::vec4 opaque = material(255, c, false);
    const glm::vec4 want = glm::vec4(.5f, .5f, 1.5f, 1) * glm::vec4(1, .5f, .5f, 1) * glm::vec4(.5f, 1.5f, .5f, 1) *
                           glm::vec4(1, .5f, .5f, 1);
    expect(same(opaque, want), 1, "slow, then kinds 1, 4 and 2 in list order (correction 17)");
    expect(same(material(255, c, true), glm::vec4(.5f, .5f, 1.5f, 1) * glm::vec4(1, .5f, .5f, 1)), 1,
           "the deferred path tints kind 1 only");

    creatures::Combatant idle;
    expect(hasMotionTrail(idle), 0, "an idle creature has no trail");
    idle.action = 0x30;
    expect(hasMotionTrail(idle), 1, "action 0x30 trails");
    idle.action = 0;
    idle.buffs = {{3}};
    expect(hasMotionTrail(idle), 1, "a kind-3 effect trails");
    creatures::Combatant rogue;
    rogue.characterClass = 4, rogue.specialization = 1, rogue.action = 0x11, rogue.actionMs = 0;
    expect(hasMotionTrail(rogue) == (0 < creatures::duration(rogue, 0x11)), 1,
           "class (4, 1) trails during action 0x11 below its duration");
    rogue.actionMs = creatures::duration(rogue, 0x11);
    expect(hasMotionTrail(rogue), 0, "and not once the clock reaches it");

    const Sequence trail = drawSequence(true, 0);
    expect(int(trail.count), 6, "five ghosts and the body");
    bool ghosts = true;
    for (int k = 0; k < 5; k++)
        ghosts &= trail.draws[k].kind == Draw::Ghost && trail.draws[k].alpha == .5f * (float(k) / 5.f) &&
                  trail.draws[k].offset == -.07f * (1.f - float(k) / 5.f);
    expect(ghosts, 1, "ghost k has alpha k/10 and offset -0.07 (1 - k/5)");
    expect(trail.draws[4].alpha == .4f && trail.draws[0].alpha == 0, 1, "the nearest ghost is the most opaque");
    expect(trail.draws[5].kind == Draw::Body && trail.draws[5].alpha == 1, 1, "the opaque body comes last");
    const Sequence hidden = drawSequence(true, .5f);
    expect(hidden.draws[5].kind == Draw::DeferredBody && hidden.draws[5].alpha == 1 - .75f * .5f &&
               hidden.draws[0].kind == Draw::Ghost, 1,
           "a translucent creature keeps its ghosts in the objects pass and defers only its body");
    expect(int(drawSequence(false, 0).count), 1, "no trail: the body alone");
    expect(postWaterLightRow(std::nullopt) == 0 && postWaterLightRow(7u) == 7, 1,
           "post-water draws keep the last water chunk's lights, else the frame list");
}
} // namespace

int main() {
    World world(1);
    Terrain terrain(world);
    {
        Zone zone;
        zone.zx = zone.zy = 0;
        chamber(zone, 0, 0);
        terrain.relight(zone);
        expect(zone.light(0, 0, 2), 0, "absent neighbors are solid, not dark open cells");
    }
    {
        Zone zone;
        zone.zx = zone.zy = 0;
        chamber(zone, 100, 100);
        wall(zone, 102, 99);
        wall(zone, 102, 100);
        wall(zone, 102, 101);
        zone.column(101, 100).blocks[1] = Block{255, 255, 255, AIR};
        terrain.relight(zone);
        expect(zone.light(100, 100, 2), 4, "a dark open neighbor supplies the five-point floor");
        expect(zone.light(101, 100, 2), 4, "synchronous closed-room result");
        const auto first = zone.column(100, 100).blocks[1];
        terrain.relight(zone);
        expect(storedAs(zone.column(100, 100).blocks[1], first), 1, "relighting resets every stored light lane");
    }
    for (bool dry : {false, true}) {
        Zone zone;
        zone.zx = zone.zy = 0;
        chamber(zone, 100, 100, -2);
        wall(zone, 102, 99, -2);
        wall(zone, 102, 100, -2);
        wall(zone, 102, 101, -2);
        if (dry)
            zone.column(100, 100).blocks[1].typeByte = DRY_AIR;
        zone.column(101, 100).blocks[1] = Block{255, 255, 255, (uint8_t)(dry ? DRY_AIR : AIR)};
        terrain.relight(zone);
        expect(zone.light(100, 100, -1), dry ? 4 : 216,
               dry ? "dry-air flag suppresses implicit water" : "stored underwater air samples full-bright implicit water");
    }
    lightRectangle();
    residentLight();
    destructionLighting();
    creatureShading();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
