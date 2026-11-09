#pragma once
// A zone's map tile (docs/specs/alpha-ui-screens.md §5.1.6): the voxel model of 8-block cells the
// corner map and the world map draw for the zone, and the dotted land borders on it.
#include "worldgen/Terrain.h"
#include <glm/glm.hpp>
#include <vector>

namespace worldgen {

struct MapTile {
    int32_t base = 0;              // B, the bottom layer in map units (8 blocks)
    int32_t sx = 32, sy = 32, sz = 0;
    std::vector<uint8_t> rgb;      // sx * sy * sz voxels, x fastest; (0, 0, 0) is empty
    std::vector<glm::ivec3> markers; // land-border points, blocks
};

// §5.1.6 "Build": the tile of a generated zone.
MapTile buildMapTile(const Zone &zone, const Terrain &terrain);

} // namespace worldgen
