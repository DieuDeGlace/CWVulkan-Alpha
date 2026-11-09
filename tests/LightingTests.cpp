// Bounded stored-light contracts from alpha-light-propagation-lifecycle.md.
// Synthetic columns distinguish absent neighbors, dark open neighbors and
// implicit water. No game installation or captured terrain is needed.
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
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
