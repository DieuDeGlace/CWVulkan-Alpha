#include "CubModel.h"
#include <cstring>

bool CubModel::load(const uint8_t *data, size_t size, std::string *error) {
    auto fail = [&](const char *why) {
        if (error)
            *error = why;
        return false;
    };
    if (size < 12)
        return fail("file shorter than its header");
    int32_t dims[3];
    std::memcpy(dims, data, 12);
    // The largest Alpha model is a few hundred voxels per side.
    for (int32_t d : dims)
        if (d <= 0 || d > 1024)
            return fail("implausible dimensions");
    const size_t count = (size_t)dims[0] * dims[1] * dims[2];
    if (size != 12 + count * 3)
        return fail("size does not match dimensions");
    sx = dims[0];
    sy = dims[1];
    sz = dims[2];
    rgb.assign(data + 12, data + size);
    return true;
}

bool CubModel::solid(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= sx || y >= sy || z >= sz)
        return false;
    const uint8_t *p = &rgb[((size_t)z * sy * sx + (size_t)y * sx + x) * 3];
    return p[0] | p[1] | p[2];
}

glm::u8vec3 CubModel::colour(int x, int y, int z) const {
    const uint8_t *p = &rgb[((size_t)z * sy * sx + (size_t)y * sx + x) * 3];
    return {p[0], p[1], p[2]};
}

namespace {

// A face direction: the axis its normal runs along, the normal's sign, and the two axes across
// the face, ordered so that u x v points outwards (counter-clockwise seen from outside).
struct Face {
    int axis, sign, u, v;
};
constexpr Face FACES[6] = {
    {0, +1, 1, 2}, {0, -1, 2, 1}, // +X, -X
    {1, +1, 2, 0}, {1, -1, 0, 2}, // +Y, -Y
    {2, +1, 0, 1}, {2, -1, 1, 0}, // +Z, -Z
};
constexpr int CORNERS[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

} // namespace

ModelMesh CubModel::mesh(glm::vec3 shade, bool keyColours) const {
    ModelMesh m;
    const glm::vec3 offset(-sx * .5f, -sy * .5f, 0);

    // Openness of a face corner (§11.1): the share of empty voxels in the layer just outside
    // the face, over offsets -3..+2 from the corner along both axes across the face.
    auto openness = [&](const glm::ivec3 &corner, const Face &f, int outside) {
        int empty = 0;
        glm::ivec3 p;
        p[f.axis] = outside;
        for (int du = -3; du <= 2; du++)
            for (int dv = -3; dv <= 2; dv++) {
                p[f.u] = corner[f.u] + du;
                p[f.v] = corner[f.v] + dv;
                empty += !solid(p.x, p.y, p.z);
            }
        return empty / 36.f;
    };

    for (int z = 0; z < sz; z++)
        for (int y = 0; y < sy; y++)
            for (int x = 0; x < sx; x++) {
                if (!solid(x, y, z))
                    continue;
                // Key colours (creatures spec §5.1): pure red, green or blue voxels draw
                // nothing, yet still close and shade their neighbours' faces.
                if (keyColours) {
                    const glm::u8vec3 c = colour(x, y, z);
                    if (c == glm::u8vec3(255, 0, 0) || c == glm::u8vec3(0, 255, 0) || c == glm::u8vec3(0, 0, 255))
                        continue;
                }
                const glm::ivec3 voxel(x, y, z);
                const glm::vec3 base = glm::vec3(colour(x, y, z)) / 255.f;
                for (const Face &f : FACES) {
                    glm::ivec3 next = voxel;
                    next[f.axis] += f.sign;
                    if (solid(next.x, next.y, next.z))
                        continue;
                    glm::vec3 normal(0);
                    normal[f.axis] = (float)f.sign;
                    const uint32_t first = (uint32_t)m.vertices.size();
                    for (const auto &c : CORNERS) {
                        glm::ivec3 corner = voxel;
                        corner[f.axis] += f.sign > 0 ? 1 : 0;
                        corner[f.u] += c[0];
                        corner[f.v] += c[1];
                        const float w = openness(corner, f, next[f.axis]);
                        m.vertices.push_back({glm::vec3(corner) + offset, normal,
                                              glm::vec4(w * base + (1 - w) * shade, 1)});
                    }
                    for (uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u})
                        m.indices.push_back(first + i);
                }
            }
    return m;
}
