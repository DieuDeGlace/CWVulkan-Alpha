#include "CubModel.h"
#include <algorithm>
#include <cstring>

bool CubModel::load(const uint8_t *data, size_t size, std::string *error, glm::ivec3 fallback) {
    auto fail = [&](const char *why) {
        if (error)
            *error = why;
        return false;
    };
    int32_t dims[3] = {fallback.x, fallback.y, fallback.z};
    for (size_t i = 0; i < 3 && 4 * (i + 1) <= size; i++)
        std::memcpy(&dims[i], data + 4 * i, 4);
    for (int32_t d : dims)
        if (d <= 0)
            return fail("a model size is not positive");
    // Engine guard against absurd sizes in malformed data; Alpha's models are far smaller.
    const size_t count = (size_t)dims[0] * (size_t)dims[1] * (size_t)dims[2];
    if ((size_t)dims[0] * dims[1] > MAX_CELLS || count > MAX_CELLS)
        return fail("model too large");
    sx = dims[0];
    sy = dims[1];
    sz = dims[2];
    const size_t present = size > 12 ? std::min((size - 12) / 3, count) : 0;
    complete = present == count;
    rgb.assign(count * 3, 0);
    if (present)
        std::memcpy(rgb.data(), data + 12, present * 3);

    for (auto &list : markers)
        list.clear();
    for (size_t cell = 0; cell < present; cell++) {
        const uint8_t *p = &rgb[cell * 3];
        int which = -1;
        if (p[0] == 255 && p[1] == 0 && p[2] == 0)
            which = RED;
        else if (p[0] == 0 && p[1] == 255 && p[2] == 0)
            which = GREEN;
        else if (p[0] == 0 && p[1] == 0 && p[2] == 255)
            which = BLUE;
        if (which >= 0)
            markers[which].push_back({(int)(cell % sx), (int)(cell / sx % sy), (int)(cell / ((size_t)sx * sy))});
    }
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
    return meshAtOffset({-sx * .5f, -sy * .5f, 0}, shade, keyColours);
}

ModelMesh CubModel::cornerMesh() const {
    return meshAtOffset({}, {}, false);
}

ModelMesh CubModel::meshAtOffset(glm::vec3 offset, glm::vec3 shade, bool keyColours) const {
    ModelMesh m;

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
