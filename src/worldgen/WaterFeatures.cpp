#include "worldgen/WaterFeatures.h"
#include <algorithm>

namespace worldgen {
namespace {
bool naturalBed(uint8_t type) {
    return type == SOLID || type == LAKE_BED || type == GRASS || type == ROCK ||
           type == SAND || type == SNOW || type == FIRE_GROUND;
}

uint32_t scatter(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

bool besideWater(const WorldMap &map, int x, int y, int feet) {
    constexpr int sides[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto &side : sides)
        for (int level = feet - 2; level <= feet; level++)
            if (map.read(x + side[0], y + side[1], level).type() == WATER &&
                map.read(x + side[0], y + side[1], level + 1).type() == AIR)
                return true;
    return false;
}
}

void makeWaterSwimmable(Zone &zone, const BedFill &bed) {
    for (int y = zone.originY(); y < zone.originY() + Zone::SIZE; y++)
        for (int x = zone.originX(); x < zone.originX() + Zone::SIZE; x++) {
            // Freeze the exposed surface heights before carving any depth. This
            // prevents a newly exposed bed from becoming another water surface.
            const Column &column = zone.column(x, y);
            std::vector<int> surfaces;
            for (int z = column.bottom; z < std::max(column.top(), 1); z++) {
                const Block cell = zone.read(x, y, z);
                if ((cell.type() == WATER || cell.type() == LAKE_BED) && zone.read(x, y, z + 1).type() == AIR)
                    surfaces.push_back(z);
            }
            for (int top : surfaces)
                for (int depth = 0; depth < SWIMMING_DEPTH; depth++) {
                    const int z = top - depth;
                    const Block cell = zone.read(x, y, z);
                    if (cell.type() == WATER)
                        continue;
                    // Protected construction and open cavities keep their shape.
                    // This extension lowers natural beds, not every solid nearby.
                    if (!naturalBed(cell.type()) ||
                        (cell.type() != LAKE_BED && (cell.typeByte & (DRY_AIR | KEEP))))
                        break;
                    zone.place(x, y, z, Block{255, 255, 255, (uint8_t)(WATER | (cell.typeByte & KEEP))});
                }
            // Water resting on the implicit cell under the column has no drawn
            // floor (implicit cells have no faces), whether this pass carved it
            // or it was already deep: store that floor cell.
            const Column &after = zone.column(x, y);
            if (!surfaces.empty() && !after.blocks.empty() && after.blocks.front().type() == WATER)
                zone.place(x, y, after.bottom - 1, bed ? bed(x, y, after.bottom - 1) : IMPLICIT_SOLID);
        }
}

std::vector<Plant> bankReeds(const WorldMap &map, const Zone &zone, uint32_t seed) {
    std::vector<Plant> result;
    for (int y = zone.originY(); y < zone.originY() + Zone::SIZE; y++)
        for (int x = zone.originX(); x < zone.originX() + Zone::SIZE; x++) {
            const uint32_t pattern = scatter(seed ^ scatter((uint32_t)x) ^ (scatter((uint32_t)y) * 0x9e3779b9u));
            if (pattern % 6 != 0)
                continue;
            const Column &column = zone.column(x, y);
            for (int z = column.top() - 1; z >= column.bottom; z--) {
                const Block ground = zone.read(x, y, z);
                if (!naturalBed(ground.type()) || (ground.typeByte & (DRY_AIR | KEEP)) ||
                    map.read(x, y, z + 1).type() != AIR || map.read(x, y, z + 2).type() != AIR)
                    continue;
                if (besideWater(map, x, y, z + 1)) {
                    const float rotation = (float)((pattern >> 8) % 360);
                    result.push_back({x, y, z + 1, .09f, rotation, 22, 4});
                    break;
                }
            }
        }
    return result;
}
}
