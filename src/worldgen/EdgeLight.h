#pragma once
// Rendering §9.2: a zone floods its light alone, so light cannot cross into it from the next
// zone. Alpha floods again when it meshes a chunk at a zone edge, over the chunk and a 16-block
// border that reaches into the neighbours. EdgeLight is that second flood.
#include "worldgen/WorldMap.h"
#include <cstdint>
#include <vector>

namespace worldgen {

class EdgeLight {
  public:
    static constexpr int32_t BORDER = 16;

    // Floods the square of `size` columns at (x0, y0) plus the border. Reads the zones it covers
    // from `map`; if one is missing there is nothing to flood across and ready() is false.
    EdgeLight(const WorldMap &map, int32_t x0, int32_t y0, int32_t size);

    bool ready() const { return !values.empty(); }
    // The light of a clear cell inside the flooded area.
    uint8_t at(int32_t x, int32_t y, int32_t z) const;
    bool covers(int32_t x, int32_t y, int32_t z) const;

  private:
    size_t index(int32_t i, int32_t j, int32_t k) const { return ((size_t)k * side + j) * side + i; }

    int32_t x0, y0, side, zlo = 0, height = 0;
    std::vector<int16_t> values; // light, or SHUT for a cell light cannot enter
};

// Whether a chunk touches the edge of its zone, the chunks Alpha floods again.
bool atZoneEdge(int32_t x0, int32_t y0, int32_t size);

} // namespace worldgen
