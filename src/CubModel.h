#pragma once
// Cube World .cub voxel model (docs/specs/alpha-extractor.md §A5.1): int32 size x, y, z, then
// x*y*z RGB cells with x varying fastest, then y, then z. Black (0,0,0) is treated as empty by
// the engine's mesher (the native empty-cell rule is open question A-2).
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct ModelVertex {
    glm::vec3 position; // voxel units, bottom-centred or measured from the grid corner
    glm::vec3 normal;
    glm::vec4 colour;   // 0..1, occlusion already applied
    float sway = 0;     // how much this corner moves with the leaf sway (0 for all but leaves)
};

struct ModelMesh {
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
};

struct CubModel {
    int sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> rgb; // 3 bytes per voxel
    // §A5.1 marker points: the cells of exactly pure red, green and blue, in cell order.
    enum Marker { RED, GREEN, BLUE };
    std::array<std::vector<glm::ivec3>, 3> markers;
    // False when the value held fewer whole cells than its sizes promise (the rest are zero).
    bool complete = true;

    // §A5.1: reads the three sizes, then every whole 3-byte cell present; missing cells stay
    // zero, bytes past the last cell are ignored. A size field the value is too short to hold
    // takes `fallback`'s value. The model is accepted only when all three sizes are positive
    // (and, as an engine resource guard, at most MAX_CELLS cells in all).
    bool load(const uint8_t *data, size_t size, std::string *error = nullptr, glm::ivec3 fallback = glm::ivec3(0));
    static constexpr size_t MAX_CELLS = size_t(1) << 24;
    bool solid(int x, int y, int z) const;
    glm::u8vec3 colour(int x, int y, int z) const;
    // One quad per exposed face. Each corner's colour fades towards `shade` by how closed in
    // it is (alpha-rendering.md §11.1); Alpha models carry no baked light.
    // With `keyColours` (hair and eye layers, alpha-creatures-models.md §5.1), voxels of
    // exactly (255, 0, 0), (0, 255, 0) or (0, 0, 255) are left out.
    ModelMesh mesh(glm::vec3 shade = glm::vec3(0), bool keyColours = false) const;
    // Indexed chest parts are placed and hinged in the original grid-corner coordinates.
    ModelMesh cornerMesh() const;

  private:
    ModelMesh meshAtOffset(glm::vec3 offset, glm::vec3 shade, bool keyColours) const;
};
