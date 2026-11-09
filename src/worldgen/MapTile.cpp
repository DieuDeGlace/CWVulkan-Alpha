// A zone's map tile (docs/specs/alpha-ui-screens.md §5.1.6, "Building or loading one tile").
#include "worldgen/MapTile.h"
#include <algorithm>

namespace worldgen {
namespace {

const glm::vec3 WATER_COLOUR(80, 100, 255);

} // namespace

MapTile buildMapTile(const Zone &zone, const Terrain &terrain) {
    MapTile tile;
    // The zone's relief in 8-block layers, from its lowest floor to its highest stored block.
    int32_t lo = INT32_MAX, hi = 0;
    for (const Column &c : zone.columns) {
        lo = std::min(lo, c.belowSurface - 1);
        hi = std::max(hi, c.top());
    }
    lo = std::max(0, lo);
    hi = std::max(hi, zone.columns[0].belowSurface);
    tile.base = lo / 8;
    tile.sz = (hi - lo) / 8 + 1;
    tile.rgb.assign((size_t)tile.sx * tile.sy * tile.sz * 3, 0);
    const int32_t B = tile.base;
    auto voxel = [&](int32_t vx, int32_t vy, int32_t k, glm::vec3 c) {
        uint8_t *p = &tile.rgb[3 * (((size_t)k * tile.sy + vy) * tile.sx + vx)];
        p[0] = (uint8_t)(int32_t)c.r, p[1] = (uint8_t)(int32_t)c.g, p[2] = (uint8_t)(int32_t)c.b;
    };
    const World &world = terrain.world();
    for (int32_t vx = 0; vx < 32; vx++)
        for (int32_t vy = 0; vy < 32; vy++) {
            const int32_t x0 = zone.originX() + 8 * vx, y0 = zone.originY() + 8 * vy;
            // (a) The surface: the mean colour and height of the 64 columns' tops.
            glm::vec3 sum(0);
            float heights = 0;
            int32_t m = INT32_MAX;
            for (int32_t sx = 0; sx < 8; sx++)
                for (int32_t sy = 0; sy < 8; sy++) {
                    const int32_t x = x0 + sx, y = y0 + sy;
                    const Column &c = zone.column(x, y);
                    const int32_t F = c.belowSurface;
                    m = std::min(m, F);
                    glm::vec3 colour;
                    int32_t height;
                    if (c.top() <= 0) {
                        colour = WATER_COLOUR, height = 0;
                    } else {
                        colour = terrain.rock(x, y, F, &zone), height = c.bottom - 1;
                        for (int32_t z = c.top() - 1; z >= c.bottom; z--) {
                            const Block &b = c.blocks[z - c.bottom];
                            if (b.type() != AIR) {
                                colour = b.type() == WATER || b.type() == LAKE_BED ? WATER_COLOUR : glm::vec3(b.r, b.g, b.b);
                                height = z;
                                break;
                            }
                            if (z == F - 1) {
                                colour = terrain.rock(x, y, F, &zone), height = F - 1;
                                break;
                            }
                        }
                    }
                    sum += colour;
                    heights += (float)height;
                }
            const glm::vec3 S = sum * (1.f / 64);
            const float mean = heights * (1.f / 64);
            const int32_t T = (int32_t)mean / 8 - B;
            if (T >= 0 && T < tile.sz)
                voxel(vx, vy, T, S);
            // (b) The body under the top: the mean of each cell's non-empty blocks.
            for (int32_t k = std::max(0, m / 8 - B - 1); k < T && k < tile.sz; k++) {
                glm::vec3 total(0);
                int32_t count = 0;
                for (int32_t sx = 0; sx < 8; sx++)
                    for (int32_t sy = 0; sy < 8; sy++)
                        for (int32_t sz = 0; sz < 8; sz++) {
                            const int32_t x = x0 + sx, y = y0 + sy, z = 8 * (B + k) + sz;
                            const Block b = zone.read(x, y, z);
                            if (b.type() == AIR)
                                continue;
                            count++;
                            if (b.type() == SOLID)
                                total += terrain.rock(x, y, z, &zone);
                            else if (b.type() == WATER || b.type() == LAKE_BED)
                                total += WATER_COLOUR;
                            else
                                total += glm::vec3(b.r, b.g, b.b);
                        }
                if (count > 0)
                    voxel(vx, vy, k, total * (1.f / (float)count));
            }
            // (c) Land borders, on a grid 32 blocks apart.
            if (vx % 4 == 0 && vy % 4 == 0) {
                const RegionCell *here = world.owner(x0, y0);
                if (here != world.owner(x0 + 32, y0) || here != world.owner(x0, y0 + 32))
                    tile.markers.push_back({x0, y0, std::max(zone.column(x0, y0).belowSurface, 0)});
            }
        }
    return tile;
}

} // namespace worldgen
