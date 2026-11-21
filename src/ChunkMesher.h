#pragma once
// Turns generated terrain into meshes, one 32 x 32 column chunk at a time, the way Alpha meshes
// its world (docs/specs/alpha-rendering.md §9): which faces are drawn, the light and occlusion
// of each corner, and the colours of water and of the ground below a column's stored blocks.
#include "CubModel.h"
#include "worldgen/Terrain.h"
#include "worldgen/WorldMap.h"
#include <algorithm>
#include <array>
#include <optional>
#include <span>

namespace worldgen { class BlockLightUpdate; }

struct ChunkMeshes {
    ModelMesh solid; // positions relative to the chunk's corner (x, y) and z = 0
    ModelMesh water;
    bool preparedLighting = false;
};

constexpr int32_t CHUNK = 32;

// Region admission for the mesh builder, in 32-column chunk coordinates.
bool chunkMeshSourcesResident(const worldgen::WorldMap &map, glm::ivec2 chunk);

// An arrival supplies its own chunks and the one-chunk perimeter in the adjoining zones.
// Preserve the engine's existing request order within a preparation batch.
template <class Visit> void visitZoneMeshChunks(glm::ivec2 zone, Visit visit) {
    constexpr int32_t width = worldgen::Zone::SIZE / CHUNK;
    const glm::ivec2 first = zone * width;
    for (int32_t x = 0; x < width; ++x)
        for (int32_t y = 0; y < width; ++y)
            visit(first + glm::ivec2(x, y));
    for (int32_t k = 0; k < width; ++k) {
        visit(first + glm::ivec2(-1, k));
        visit(first + glm::ivec2(width, k));
        visit(first + glm::ivec2(k, -1));
        visit(first + glm::ivec2(k, width));
    }
    for (const int32_t dx : {-1, width})
        for (const int32_t dy : {-1, width})
            visit(first + glm::ivec2(dx, dy));
}

// Mesh slots share a light request across reuse. Geometry remains in the renderer's
// independent streaming cache. A nonzero change identifies one edit marking phase.
class ChunkLightUpdates {
  public:
    void markChanged(std::span<const glm::ivec2> columns);
    // Empty means this candidate must wait. Zero is an admitted, clean build.
    std::optional<uint64_t> beginBuild(const worldgen::WorldMap &map, glm::ivec2 chunk);
    void publish(glm::ivec2 chunk, uint64_t buildChange, bool preparedLighting);
    void invalidate(glm::ivec2 chunk);

  private:
    static constexpr int32_t SIDE = 40;
    struct Slot {
        glm::ivec2 published{-1};
        uint64_t change = 0;
    };
    Slot &slot(glm::ivec2 chunk);
    std::array<Slot, SIDE * SIDE> slots;
    uint64_t changes = 0;
};

int32_t meshLightPadding(int32_t x0, int32_t y0, bool lightingRequested);
std::shared_ptr<const worldgen::BlockLightUpdate> prepareChunkLight(const worldgen::WorldMap &map, int32_t x0,
                                                                   int32_t y0, bool lightingRequested);

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

// Meshes the chunk whose corner column is (x0, y0). Missing required sources give empty meshes.
ChunkMeshes meshChunk(const worldgen::WorldMap &map, const worldgen::Terrain &terrain, int32_t x0, int32_t y0,
                      bool lightingRequested = false);
// Geometry worker entry with lighting prepared for its immutable snapshot.
ChunkMeshes meshChunk(const worldgen::WorldMap &map, const worldgen::Terrain &terrain, int32_t x0, int32_t y0,
                      const worldgen::BlockLightUpdate *preparedLighting);
