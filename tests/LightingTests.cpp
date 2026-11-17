// Bounded stored-light contracts from alpha-light-propagation-lifecycle.md.
// Synthetic columns distinguish absent neighbors, dark open neighbors and
// implicit water. No game installation or captured terrain is needed.
#include "CreatureShading.h"
#include "Frame.h"
#include "worldgen/EdgeLight.h"
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
        const auto first = zone.shadedLight;
        terrain.relight(zone);
        expect(zone.shadedLight == first, 1, "relighting resets rather than appending earlier results");
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
    {
        auto zone = std::make_shared<Zone>();
        zone->zx = zone->zy = 0;
        chamber(*zone, 16, 32);
        zone->column(15, 32).blocks[1] = Block{255, 255, 255, AIR};
        zone->shadedLight.emplace_back(lightKey(15, 32, 2), 128);
        WorldMap map;
        map.add(zone);
        EdgeLight light(map, 32, 32, 1);
        expect(light.ready(), 1, "rectangle has its admitted columns");
        expect(light.at(16, 32, 2), 108, "exterior admitted light is sampled beyond the reset rectangle");
    }
    creatureShading();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
