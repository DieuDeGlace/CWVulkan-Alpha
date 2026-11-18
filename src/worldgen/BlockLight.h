#pragma once

#include "worldgen/WorldMap.h"
#include <glm/vec2.hpp>
#include <cstddef>
#include <optional>

namespace worldgen {

struct LightRectangle {
    glm::ivec2 lower, upper; // whole-cell coordinates, upper bound excluded

    bool contains(int32_t x, int32_t y) const {
        return x >= lower.x && x < upper.x && y >= lower.y && y < upper.y;
    }
};

// One native propagation call over stored cells. Values are calculated against
// an immutable terrain snapshot, so meshes can read them and the owning area
// can store them without changing a worker's source columns.
class BlockLightUpdate {
  public:
    BlockLightUpdate(const WorldMap &map, LightRectangle destination, int32_t workingMargin = 0);
    BlockLightUpdate(const Zone &area, LightRectangle destination, int32_t workingMargin = 0);

    std::optional<uint8_t> committed(int32_t x, int32_t y, int32_t z) const;
    // Store into the area used for preparation or its unmodified terrain copy.
    void store(Zone &area) const;

  private:
    using ColumnLookup = std::function<const Column *(int32_t, int32_t)>;
    BlockLightUpdate(LightRectangle destination, int32_t margin, const ColumnLookup &columnAt);
    size_t index(int32_t x, int32_t y) const;

    struct ColumnRange {
        int32_t bottom = 0;
        int32_t count = 0;
        size_t offset = 0;
    };
    LightRectangle destination, working;
    int32_t pitch = 0;
    std::vector<ColumnRange> columns;
    std::vector<uint8_t> values;
};

} // namespace worldgen
