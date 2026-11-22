#pragma once
// The generated zones the engine holds, looked up by block column. Reading a cell whose zone is
// missing gives solid ground (terrain spec §1.4, type UNHELD), so the mesher closes the world's
// open edges.
// Zones never change once generated, so a map can share them with others: a background job
// takes its own small map of the zones it needs (neighbourhood) and reads it without locks.
#include "worldgen/Zone.h"
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace worldgen {

// The original client's zone clean-up (alpha-extractor.md §B5.2): a pass runs when more than
// ZONE_CLEANUP_INTERVAL_MS have passed since the last one, and a loaded zone survives it while
// its squared zone distance to an interest point is below KEPT_ZONE_DISTANCE2 (a disc: (3, 2)
// stays, (3, 3) and (4, 0) go). A world change unloads every zone.
inline constexpr int64_t ZONE_CLEANUP_INTERVAL_MS = 1000;
inline constexpr int64_t KEPT_ZONE_DISTANCE2 = 16;
inline bool zoneKeptNear(int32_t zx, int32_t zy, int32_t px, int32_t py) {
    const int64_t dx = (int64_t)zx - px, dy = (int64_t)zy - py;
    return dx * dx + dy * dy < KEPT_ZONE_DISTANCE2;
}

// A record owned by a resident area. Its chunk snapshots retain native encounter order.
struct PositionedDecoration {
    Sprite sprite;
    float light = 0; // defined by the first matched mesh build
    int32_t plantIndex = -1; // wind phase for the engine's cell-centred plant adapter
};
using DecorationList = std::vector<PositionedDecoration>;
using DecorationSnapshot = std::shared_ptr<const DecorationList>;
struct DecorationUpdate {
    DecorationSnapshot source, sampled;
};

// The membership conversion first rounds the fixed coordinate to double.
inline int64_t decorationChunkOf(int64_t fixed) { return int64_t(double(fixed) * (1.0 / 32.0)) / 65536; }

class WorldMap {
  public:
    using ZoneKey = std::pair<int32_t, int32_t>;
    using PlacedObjects = std::vector<std::array<int32_t, 4>>;
    void add(std::shared_ptr<const Zone> zone);
    // Publish changed terrain or decorations while retaining the resident descriptor and object identities.
    void replaceTerrain(std::shared_ptr<const Zone> zone);
    // Build fixed-position records for the client on admission or after a change in its source list.
    void refreshDecorations(ZoneKey zone);
    DecorationSnapshot decorations(int32_t cx, int32_t cy) const;
    DecorationUpdate sampleDecorations(int32_t cx, int32_t cy) const;
    bool publishDecorations(int32_t cx, int32_t cy, const DecorationUpdate &update);
    // Nonzero for a held zone. Its indexed static objects have IDs base + ordinal.
    uintptr_t recordBase(ZoneKey key) const;
    void remove(int32_t zx, int32_t zy);
    const Zone *zone(int32_t zx, int32_t zy) const;
    const Zone *zoneAt(int32_t x, int32_t y) const;
    Block read(int32_t x, int32_t y, int32_t z) const;
    // The propagated light of a clear cell (rendering §9.2); 255 where no zone is held.
    uint8_t light(int32_t x, int32_t y, int32_t z) const;
    // A map of zone (zx, zy) and the eight round it, as far as they are held.
    WorldMap neighbourhood(int32_t zx, int32_t zy) const;
    const std::map<std::pair<int32_t, int32_t>, std::shared_ptr<const Zone>> &zones() const { return held; }
    bool containsNetworkZone(ZoneKey key) const;
    // A held zone starts eligible. Any valid section 6, 7 or 8 update satisfies it.
    bool needsZoneContents(ZoneKey key) const;
    void acknowledgeZoneContents(ZoneKey key);
    void replacePlacedObjects(ZoneKey key, PlacedObjects records);
    void appendPlacedObject(ZoneKey key, const std::array<int32_t, 4> &record);
    const PlacedObjects *placedObjects(ZoneKey key) const;

  private:
    std::map<std::pair<int32_t, int32_t>, std::shared_ptr<const Zone>> held;
    using DecorationArea = std::array<DecorationSnapshot, 64>;
    struct ResidentState {
        uintptr_t recordBase = 0;
        std::optional<PlacedObjects> placed;
        std::shared_ptr<DecorationArea> decorations;
    };
    std::map<ZoneKey, ResidentState> residents;
};

} // namespace worldgen
