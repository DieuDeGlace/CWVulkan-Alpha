#pragma once
// Turns generated terrain into meshes, one 32 x 32 column chunk at a time, the way Alpha meshes
// its world (docs/specs/alpha-rendering.md §9): which faces are drawn, the light and occlusion
// of each corner, and the colours of water and of the ground below a column's stored blocks.
#include "CubModel.h"
#include "worldgen/Terrain.h"
#include "worldgen/WorldMap.h"
#include <algorithm>

struct ChunkMeshes {
    ModelMesh solid; // positions relative to the chunk's corner (x, y) and z = 0
    ModelMesh water;
};

constexpr int32_t CHUNK = 32;

// How much a corner of a leaf block moves with AlphaVulkan's leaf sway (the rule of the user's
// AlphaCWWorldMotion mod): 0 where the corner touches anything but leaves and air (wood, ground,
// water), so crowns stay on their trunks; on a chunk's edge, so neighbouring chunks never come
// apart; and ramping up over the next three blocks inside the chunk. `type(dx, dy, dz)` reads
// the cells round the corner, d in {-1, 0}; (cx, cy) is the corner within the chunk, 0..32.
template <class Types> float leafSway(int32_t cx, int32_t cy, Types type) {
    if (cx <= 0 || cy <= 0 || cx >= CHUNK || cy >= CHUNK)
        return 0;
    bool leaf = false;
    for (int dx = -1; dx <= 0; dx++)
        for (int dy = -1; dy <= 0; dy++)
            for (int dz = -1; dz <= 0; dz++) {
                const uint8_t t = type(dx, dy, dz);
                if (t == worldgen::LEAVES_TYPE)
                    leaf = true;
                else if (t != worldgen::AIR)
                    return 0;
            }
    return leaf ? std::clamp((float)std::min({cx, cy, CHUNK - cx, CHUNK - cy}) / 3.f, 0.f, 1.f) : 0.f;
}

// Meshes the chunk whose corner column is (x0, y0). Cells of zones the map does not hold read
// as solid, so a chunk at the edge of the held area is closed off there.
ChunkMeshes meshChunk(const worldgen::WorldMap &map, const worldgen::Terrain &terrain, int32_t x0, int32_t y0);
