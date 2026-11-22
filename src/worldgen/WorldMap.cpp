#include "worldgen/WorldMap.h"
#include "worldgen/Random.h"
#include <algorithm>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace worldgen {

void WorldMap::add(std::shared_ptr<const Zone> zone) {
    const auto key = std::make_pair(zone->zx, zone->zy);
    if (const auto existing = held.find(key); existing != held.end() && existing->second == zone)
        return;
    // IDs are compared for equality by route and descriptor caches, including across worlds.
    // Reserve a disjoint range once per admission, independent of terrain snapshot storage.
    static std::mutex mutex;
    static uintptr_t last = 0;
    const uintptr_t count = std::max<size_t>(1, zone->statics.size());
    uintptr_t first;
    {
        std::lock_guard lock(mutex);
        if (count > std::numeric_limits<uintptr_t>::max() - last)
            throw std::overflow_error("Resident object identity space exhausted");
        first = last + 1;
        last += count;
    }
    residents[key].recordBase = first;
    held[key] = std::move(zone);
}

void WorldMap::replaceTerrain(std::shared_ptr<const Zone> zone) {
    held.at({zone->zx, zone->zy}) = std::move(zone);
}

uintptr_t WorldMap::recordBase(ZoneKey key) const {
    const auto it = residents.find(key);
    return it == residents.end() ? 0 : it->second.recordBase;
}

void WorldMap::remove(int32_t zx, int32_t zy) {
    held.erase({zx, zy});
    residents.erase({zx, zy});
}

bool WorldMap::containsNetworkZone(ZoneKey key) const {
    return uint32_t(key.first) < 65536u && uint32_t(key.second) < 65536u && held.contains(key);
}

bool WorldMap::needsZoneContents(ZoneKey key) const {
    return containsNetworkZone(key) && !residents.at(key).placed;
}

void WorldMap::acknowledgeZoneContents(ZoneKey key) {
    if (containsNetworkZone(key) && !residents.at(key).placed)
        residents.at(key).placed.emplace();
}

void WorldMap::replacePlacedObjects(ZoneKey key, PlacedObjects records) {
    if (containsNetworkZone(key))
        residents.at(key).placed = std::move(records);
}

void WorldMap::appendPlacedObject(ZoneKey key, const std::array<int32_t, 4> &record) {
    if (containsNetworkZone(key)) {
        auto &placed = residents.at(key).placed;
        if (!placed)
            placed.emplace();
        placed->push_back(record);
    }
}

const WorldMap::PlacedObjects *WorldMap::placedObjects(ZoneKey key) const {
    const auto it = residents.find(key);
    return it == residents.end() || !it->second.placed ? nullptr : &*it->second.placed;
}

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
            if (const auto it = held.find({zx + i, zy + j}); it != held.end()) {
                out.held.emplace(it->first, it->second);
                out.residents[it->first].recordBase = recordBase(it->first);
            }
    return out;
}

} // namespace worldgen
