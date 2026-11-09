// Generates the zones round a block position and lists what the structure passes placed in
// them: static objects and object-list marks (alpha-worldgen-structures.md).
// Usage: ZoneDump seed x y [radius]
//        ZoneDump seed x y portals   (lists the portal zones, entry kind 4, of that region)
//        ZoneDump seed x y locations (lists the location records of that region; type 1 is the town)
//        ZoneDump seed x y dungeons  (lists the dungeon entrances, entry kind 3, of that region)
#include "worldgen/Terrain.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace worldgen;

int main(int argc, char **argv) {
    if (argc < 4) {
        std::printf("usage: ZoneDump seed x y [radius]\n");
        return 1;
    }
    World world((uint32_t)std::strtoul(argv[1], nullptr, 10));
    const Terrain terrain(world);
    const int32_t x = std::atoi(argv[2]), y = std::atoi(argv[3]);
    if (argc > 4 && std::string(argv[4]) == "portals") {
        const int32_t rx = floorDiv(x, REGION), ry = floorDiv(y, REGION);
        terrain.prepareRegion(rx, ry);
        for (int32_t zx = 64 * rx; zx < 64 * rx + 64; zx++)
            for (int32_t zy = 64 * ry; zy < 64 * ry + 64; zy++)
                if (const ZoneEntry *e = world.zoneEntry(zx, zy); e && e->kind == 4)
                    std::printf("portal zone (%d, %d), centre (%d, %d)\n", zx, zy, 256 * zx + 128, 256 * zy + 128);
        return 0;
    }
    if (argc > 4 && std::string(argv[4]) == "dungeons") {
        const int32_t rx = floorDiv(x, REGION), ry = floorDiv(y, REGION);
        terrain.prepareRegion(rx, ry);
        for (int32_t zx = 64 * rx; zx < 64 * rx + 64; zx++)
            for (int32_t zy = 64 * ry; zy < 64 * ry + 64; zy++)
                if (const ZoneEntry *e = world.zoneEntry(zx, zy); e && e->kind == 3)
                    std::printf("dungeon zone (%d, %d), style %d, level %d, centre (%d, %d)\n", zx, zy, e->variant,
                                e->level, 256 * zx + 128, 256 * zy + 128);
        return 0;
    }
    if (argc > 4 && std::string(argv[4]) == "locations") {
        const int32_t rx = floorDiv(x, REGION), ry = floorDiv(y, REGION);
        terrain.prepareRegion(rx, ry);
        if (const Region *region = world.region(rx, ry))
            for (size_t k = 0; k < region->records.size(); k++)
                if (const Location &L = region->records[k]; L.type)
                    std::printf("location %zu: type %d at (%d, %d), radius %.0f, level %d\n", k, L.type,
                                (int32_t)(L.x >> 16), (int32_t)(L.y >> 16), L.radius, L.level);
        return 0;
    }
    const int32_t radius = argc > 4 ? std::atoi(argv[4]) : 1;
    for (int32_t i = -radius; i <= radius; i++)
        for (int32_t j = -radius; j <= radius; j++) {
            const int32_t zx = floorDiv(x, Zone::SIZE) + i, zy = floorDiv(y, Zone::SIZE) + j;
            const auto zone = terrain.generate(zx, zy);
            const ZoneEntry *e = world.zoneEntry(zx, zy);
            const Location *L = world.location(zone->originX(), zone->originY());
            std::printf("zone (%d, %d): entry kind %d, location type %d, %zu statics, %zu marks, %zu plants\n", zx, zy,
                        e ? e->kind : -1, L ? L->type : 0, zone->statics.size(), zone->objectList.size(),
                        zone->plants.size());
            for (const StaticObject &o : zone->statics)
                std::printf("  static %d at (%.1f, %.1f, %.1f) turn %d\n", o.kind, o.x / 65536.0, o.y / 65536.0,
                            o.z / 65536.0, o.rotation);
            for (const Spawn &s : zone->spawns)
                std::printf("  spawn type %d at (%.1f, %.1f, %.1f)\n", s.type, s.x / 65536.0, s.y / 65536.0,
                            s.z / 65536.0);
        }
    return 0;
}
