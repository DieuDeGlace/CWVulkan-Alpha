#include "worldgen/WorldMap.h"
#include "worldgen/Random.h"
#include <algorithm>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <tuple>

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
    residents[key].decorations.reset();
    held[key] = std::move(zone);
}

void WorldMap::replaceTerrain(std::shared_ptr<const Zone> zone) {
    held.at({zone->zx, zone->zy}) = std::move(zone);
}

namespace {
size_t decorationIndex(int32_t x, int32_t y) { return size_t(x % 8) * 8 + size_t(y % 8); }
using Position = std::array<int64_t, 3>;
}

// SDK modules can still write the separate typed collections directly. Their unranked
// entries retain collection order after all ranked generation records.
void WorldMap::refreshDecorations(ZoneKey key) {
    const auto &source = *held.at(key);
    auto &resident = residents.at(key);
    const auto old = resident.decorations;
    std::array<DecorationList, 4> authored;
    size_t group = 0;
    const auto append = [&](const Sprite &sprite, int32_t plant = -1) {
        const int64_t cx = decorationChunkOf(sprite.x), cy = decorationChunkOf(sprite.y);
        if (cx < 0 || cy < 0 || cx >= 0x80000 || cy >= 0x80000 || cx / 8 != key.first || cy / 8 != key.second)
            return;
        authored[group].push_back({sprite, 0.f, plant});
    };
    for (const Sprite &sprite : source.sprites)
        append(sprite);
    ++group;
    for (size_t i = 0; i < source.plants.size(); ++i) {
        const auto &p = source.plants[i];
        Sprite sprite{p.kind, int64_t(p.x) * 65536 + 32768, int64_t(p.y) * 65536 + 32768,
                      int64_t(p.z) * 65536 - (p.sink ? 6553 : 0), p.scale, p.rotation, int32_t(p.flags), {1, 1, 1}};
        sprite.encounter = p.encounter;
        append(sprite, int32_t(i));
    }
    ++group;
    for (const auto &leaf : source.leafSources) {
        Sprite sprite{60, leaf.x, leaf.y, leaf.z, 0, 0, 2, {leaf.r, leaf.g, leaf.b}};
        sprite.encounter = leaf.encounter;
        append(sprite);
    }
    // The original std::array position type remains available to existing source modules.
    // Match by position even when a module removed or moved an earlier song post.
    std::map<Position, std::vector<uint32_t>> songOrder;
    for (auto i = source.songEncounters.rbegin(); i != source.songEncounters.rend(); ++i)
        songOrder[i->first].push_back(i->second);
    ++group;
    for (const auto &p : source.songPosts) {
        Sprite sprite{61, p[0], p[1], p[2], 0, 0, 2, {1, 1, 1}};
        if (auto it = songOrder.find(p); it != songOrder.end() && !it->second.empty()) {
            sprite.encounter = it->second.back();
            it->second.pop_back();
        }
        append(sprite);
    }
    // Merge the four ordered streams. A source module may insert or reorder entries in a
    // typed collection directly. Its within-collection order still survives that edit.
    std::array<size_t, 4> next{};
    std::array<DecorationList, 64> lists;
    for (;;) {
        size_t first = authored.size();
        for (size_t i = 0; i < authored.size(); ++i)
            if (next[i] < authored[i].size() &&
                (first == authored.size() || authored[i][next[i]].sprite.encounter < authored[first][next[first]].sprite.encounter))
                first = i;
        if (first == authored.size())
            break;
        auto &record = authored[first][next[first]++];
        const int32_t cx = int32_t(decorationChunkOf(record.sprite.x)), cy = int32_t(decorationChunkOf(record.sprite.y));
        lists[decorationIndex(cx, cy)].push_back(std::move(record));
    }
    auto area = std::make_shared<DecorationArea>();
    for (size_t i = 0; i < lists.size(); ++i) {
        auto &list = lists[i];
        if (list.empty())
            continue;
        if (old && (*old)[i]) {
            // Preserve only surviving occurrences. A module may have supplied duplicate or
            // unranked records at the same point. Newly added occurrences start unsampled.
            const auto binding = [](const PositionedDecoration &r) {
                const auto &s = r.sprite;
                return std::tuple(s.encounter, s.kind, s.x, s.y, s.z, r.plantIndex >= 0);
            };
            std::map<decltype(binding(list.front())), std::pair<float, size_t>> previous;
            for (const auto &entry : *(*old)[i]) {
                auto &value = previous[binding(entry)];
                value.first = entry.light;
                ++value.second;
            }
            for (auto &entry : list)
                if (auto it = previous.find(binding(entry)); it != previous.end() && it->second.second > 0) {
                    entry.light = it->second.first;
                    --it->second.second;
                }
        }
        (*area)[i] = std::make_shared<const DecorationList>(std::move(list));
    }
    resident.decorations = std::move(area);
}

DecorationSnapshot WorldMap::decorations(int32_t cx, int32_t cy) const {
    if (uint32_t(cx) >= 0x80000 || uint32_t(cy) >= 0x80000)
        return {};
    const auto it = residents.find({cx / 8, cy / 8});
    return it != residents.end() && it->second.decorations ?
               (*it->second.decorations)[decorationIndex(cx, cy)] : nullptr;
}

DecorationUpdate WorldMap::sampleDecorations(int32_t cx, int32_t cy) const {
    const auto source = decorations(cx, cy);
    if (!source)
        return {};
    auto sampled = std::make_shared<DecorationList>(*source);
    for (auto &record : *sampled) {
        const auto &s = record.sprite;
        const int64_t x = s.x / 65536, y = s.y / 65536, z = s.z / 65536;
        record.light = 0;
        if (uint64_t(x) >= 0x1000000 || uint64_t(y) >= 0x1000000)
            continue;
        const Zone *area = zoneAt(int32_t(x), int32_t(y));
        if (!area)
            continue;
        const auto &column = area->column(int32_t(x), int32_t(y));
        if (z < column.bottom)
            continue;
        if (z >= int64_t(column.bottom) + int64_t(column.blocks.size())) {
            record.light = 255;
            continue;
        }
        const auto cell = area->read(int32_t(x), int32_t(y), int32_t(z));
        if (cell.type() == GLOWING)
            record.light = 255;
        else if (clear(cell.type()))
            record.light = float(std::max<uint8_t>(5, cell.r));
    }
    return {source, std::move(sampled)};
}

bool WorldMap::publishDecorations(int32_t cx, int32_t cy, const DecorationUpdate &update) {
    if (decorations(cx, cy) != update.source)
        return false; // a genuine replacement or support edit superseded this result
    if (!update.sampled)
        return true;
    auto &area = residents.at({cx / 8, cy / 8}).decorations;
    // Retained and worker maps continue to see their previous record bytes.
    if (area.use_count() != 1)
        area = std::make_shared<DecorationArea>(*area);
    (*area)[decorationIndex(cx, cy)] = update.sampled;
    return true;
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
                const auto &resident = residents.at(it->first);
                out.residents[it->first].recordBase = resident.recordBase;
                out.residents[it->first].decorations = resident.decorations;
            }
    return out;
}

} // namespace worldgen
