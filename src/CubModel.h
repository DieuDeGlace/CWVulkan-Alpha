#pragma once
// Cube World .cub voxel model (docs/specs/alpha-data-stores.md §3): int32 size x, y, z, then
// x*y*z RGB triples with x varying fastest, then y, then z. Black (0,0,0) is empty.
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

struct ModelVertex {
    glm::vec3 position; // voxel units, model centred on x/y with its base at z = 0
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

    bool load(const uint8_t *data, size_t size, std::string *error = nullptr);
    bool solid(int x, int y, int z) const;
    glm::u8vec3 colour(int x, int y, int z) const;
    // One quad per exposed face. Each corner's colour fades towards `shade` by how closed in
    // it is (alpha-rendering.md §11.1); Alpha models carry no baked light.
    // With `keyColours` (hair and eye layers, alpha-creatures-models.md §5.1), voxels of
    // exactly (255, 0, 0), (0, 255, 0) or (0, 0, 255) are left out.
    ModelMesh mesh(glm::vec3 shade = glm::vec3(0), bool keyColours = false) const;
};
