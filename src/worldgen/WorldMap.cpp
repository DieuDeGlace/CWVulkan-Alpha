#include "worldgen/WorldMap.h"
#include "worldgen/Random.h"

namespace worldgen {

void WorldMap::add(std::shared_ptr<const Zone> zone) {
    const auto key = std::make_pair(zone->zx, zone->zy);
    held[key] = std::move(zone);
}

void WorldMap::remove(int32_t zx, int32_t zy) { held.erase({zx, zy}); }

const Zone *WorldMap::zone(int32_t zx, int32_t zy) const {
    const auto it = held.find({zx, zy});
    return it == held.end() ? nullptr : it->second.get();
}

const Zone *WorldMap::zoneAt(int32_t x, int32_t y) const {
    return zone(floorDiv(x, Zone::SIZE), floorDiv(y, Zone::SIZE));
}

Block WorldMap::read(int32_t x, int32_t y, int32_t z) const {
    const Zone *z0 = zoneAt(x, y);
    return z0 ? z0->read(x, y, z) : UNHELD_CELL;
}

uint8_t WorldMap::light(int32_t x, int32_t y, int32_t z) const {
    const Zone *z0 = zoneAt(x, y);
    return z0 ? z0->light(x, y, z) : 255;
}

WorldMap WorldMap::neighbourhood(int32_t zx, int32_t zy) const {
    WorldMap out;
    for (int32_t i = -1; i <= 1; i++)
        for (int32_t j = -1; j <= 1; j++)
            if (const auto it = held.find({zx + i, zy + j}); it != held.end())
                out.held.emplace(it->first, it->second);
    return out;
}

} // namespace worldgen
