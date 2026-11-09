#include "worldgen/EdgeLight.h"
#include "worldgen/Random.h"
#include <algorithm>
#include <climits>

namespace worldgen {

namespace {

constexpr int16_t SHUT = -1;   // solid: counts 0 for its neighbours
constexpr int OPEN_FLOOR = 5;  // an open neighbour never counts less than this
constexpr int SWEEPS = 16;

} // namespace

EdgeLight::EdgeLight(const WorldMap &map, int32_t chunkX, int32_t chunkY, int32_t size)
    : x0(chunkX - BORDER), y0(chunkY - BORDER), side(size + 2 * BORDER) {
    // The vertical range, one layer past every column, and every zone held.
    int32_t lowest = INT32_MAX, highest = INT32_MIN;
    for (int32_t i = 0; i < side; i++)
        for (int32_t j = 0; j < side; j++) {
            const Zone *zone = map.zoneAt(x0 + i, y0 + j);
            if (!zone)
                return;
            const Column &c = zone->column(x0 + i, y0 + j);
            lowest = std::min(lowest, c.bottom);
            highest = std::max(highest, c.top());
        }
    zlo = lowest - 1;
    height = highest + 1 - zlo;
    values.assign((size_t)side * side * height, SHUT);
    std::vector<uint8_t> implicitWater(values.size(), 0);

    // Step 1: cells above a column's first closed block are sky-lit, those below start dark.
    std::vector<size_t> shaded;
    for (int32_t i = 0; i < side; i++)
        for (int32_t j = 0; j < side; j++) {
            const int32_t x = x0 + i, y = y0 + j;
            const Zone *zone = map.zoneAt(x, y);
            const Column &c = zone->column(x, y);
            int32_t sky = c.bottom;
            for (int32_t z = c.top() - 1; z >= c.bottom; z--)
                if (!clear(zone->read(x, y, z).type())) {
                    sky = z + 1;
                    break;
                }
            for (int32_t k = 0; k < height; k++) {
                const int32_t z = zlo + k;
                const uint8_t type = zone->read(x, y, z).type();
                if (z >= c.bottom && z < c.top()) {
                    const Block stored = c.blocks[(size_t)(z - c.bottom)];
                    implicitWater[index(i, j, k)] = stored.type() == AIR && z <= 0 && !(stored.typeByte & DRY_AIR);
                }
                if (type == GLOWING) {
                    values[index(i, j, k)] = 255;
                } else if (clear(type)) {
                    values[index(i, j, k)] = z >= sky ? 255 : 0;
                    if (z < sky)
                        shaded.push_back(index(i, j, k));
                }
            }
        }

    // Step 2: sixteen sweeps. Each shaded cell takes 85% of its brightest neighbour; the new
    // values take effect together, after the sweep.
    const auto neighbour = [&](int32_t i, int32_t j, int32_t k) -> int {
        if (i < 0 || j < 0 || i >= side || j >= side || k < 0 || k >= height) {
            const int32_t x = x0 + i, y = y0 + j, z = zlo + k;
            if (x < 0 || y < 0 || x > 0xffffff || y > 0xffffff)
                return 0;
            const Zone *zone = map.zoneAt(x, y);
            if (!zone)
                return 0;
            const Column &column = zone->column(x, y);
            if (z < column.bottom)
                return 0;
            if (z >= column.top())
                return 255;
            const Block stored = column.blocks[(size_t)(z - column.bottom)];
            if (stored.type() == GLOWING ||
                (stored.type() == AIR && z <= 0 && !(stored.typeByte & DRY_AIR)))
                return 255;
            // Engine zones are immutable here. Outside this rectangle their
            // generated committed light is the available working-light snapshot.
            return clear(stored.type()) ? std::max((int)zone->light(x, y, z), OPEN_FLOOR) : 0;
        }
        if (implicitWater[index(i, j, k)])
            return 255;
        const int16_t v = values[index(i, j, k)];
        return v == SHUT ? 0 : std::max((int)v, OPEN_FLOOR);
    };
    std::vector<int16_t> next(shaded.size());
    for (int sweep = 0; sweep < SWEEPS; sweep++) {
        for (size_t n = 0; n < shaded.size(); n++) {
            const size_t cell = shaded[n];
            const int32_t i = (int32_t)(cell % side), j = (int32_t)(cell / side % side), k = (int32_t)(cell / side / side);
            int best = 0;
            const int32_t steps[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
            for (const auto &step : steps) {
                best = std::max(best, neighbour(i + step[0], j + step[1], k + step[2]));
                if (best == 255)
                    break;
            }
            next[n] = (int16_t)(best * 85 / 100);
        }
        for (size_t n = 0; n < shaded.size(); n++)
            values[shaded[n]] = next[n];
    }
}

bool EdgeLight::covers(int32_t x, int32_t y, int32_t z) const {
    const int32_t i = x - x0, j = y - y0, k = z - zlo;
    return ready() && i >= 0 && j >= 0 && i < side && j < side && k >= 0 && k < height;
}

uint8_t EdgeLight::at(int32_t x, int32_t y, int32_t z) const {
    const int16_t v = values[index(x - x0, y - y0, z - zlo)];
    return v == SHUT ? (uint8_t)0 : (uint8_t)v;
}

bool atZoneEdge(int32_t x0, int32_t y0, int32_t size) {
    const int32_t i = x0 - floorDiv(x0, Zone::SIZE) * Zone::SIZE, j = y0 - floorDiv(y0, Zone::SIZE) * Zone::SIZE;
    return i == 0 || j == 0 || i + size >= Zone::SIZE || j + size >= Zone::SIZE;
}

} // namespace worldgen
