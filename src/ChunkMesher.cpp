#include "ChunkMesher.h"
#include "worldgen/BlockLight.h"
#include <algorithm>
#include <cmath>

using namespace worldgen;

namespace {

bool validChunk(glm::ivec2 chunk) {
    constexpr uint32_t worldChunks = 0x1000000u / CHUNK;
    return uint32_t(chunk.x) < worldChunks && uint32_t(chunk.y) < worldChunks;
}

int32_t seamOffset(int32_t origin) {
    const int32_t cell = origin % Zone::SIZE;
    if (cell == 0)
        return -CHUNK;
    if (cell == Zone::SIZE - CHUNK)
        return CHUNK;
    return 0;
}

bool meshLightAdmitted(const WorldMap &map, int32_t x, int32_t y) {
    if (!validChunk({x / CHUNK, y / CHUNK}))
        return false;
    const int32_t ax = x / Zone::SIZE, ay = y / Zone::SIZE;
    const int32_t acrossX = (x + seamOffset(x)) / Zone::SIZE, acrossY = (y + seamOffset(y)) / Zone::SIZE;
    constexpr uint32_t worldAreas = 0x1000000u / Zone::SIZE;
    return uint32_t(acrossX) < worldAreas && uint32_t(acrossY) < worldAreas &&
           map.zone(ax, ay) && map.zone(acrossX, acrossY) && map.zone(acrossX, ay) && map.zone(ax, acrossY);
}

// A face direction: its axis, the sign of its normal, and the two axes across it, ordered so
// that u x v points outwards (counter-clockwise seen from outside).
struct Face {
    int axis, sign, u, v;
};
constexpr Face FACES[6] = {
    {0, +1, 1, 2}, {0, -1, 2, 1}, {1, +1, 2, 0}, {1, -1, 0, 2}, {2, +1, 0, 1}, {2, -1, 1, 0},
};
constexpr int CORNERS[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

// Rendering §9 neighbour substitution: a stored empty block at z <= 0 without flag 0x40 reads
// as the lit water default, whose light byte is 255 (Zone::read already gives its type).
bool litWaterDefault(const Zone &zone, int32_t x, int32_t y, int32_t z) {
    if (z > 0)
        return false;
    const Column &c = zone.column(x, y);
    if (z < c.bottom || z >= c.top())
        return false;
    return c.blocks[z - c.bottom].implicitWaterAt(z);
}

// The cells round a chunk, read once: type and propagated light, over the chunk plus a border
// wide enough for the corner samples (-3..+2) and the neighbours' faces.
class CellCache {
  public:
    static constexpr int32_t BORDER = 4;

    CellCache(const WorldMap &map, int32_t x0, int32_t y0, const BlockLightUpdate *lightPatch)
        : map(map), lightPatch(lightPatch), x0(x0 - BORDER), y0(y0 - BORDER) {
        zlo = INT32_MAX;
        zhi = 4; // at least past sea level, for the sea's surface
        for (int32_t i = 0; i < SIDE; i++)
            for (int32_t j = 0; j < SIDE; j++)
                if (const Zone *z = map.zoneAt(this->x0 + i, this->y0 + j)) {
                    const Column &c = z->column(this->x0 + i, this->y0 + j);
                    zlo = std::min(zlo, c.bottom - BORDER);
                    zhi = std::max(zhi, c.top() + BORDER);
                }
        if (zlo == INT32_MAX)
            zlo = -BORDER;
        height = zhi - zlo;
        cells.resize((size_t)SIDE * SIDE * height);
        for (int32_t i = 0; i < SIDE; i++)
            for (int32_t j = 0; j < SIDE; j++) {
                const int32_t x = this->x0 + i, y = this->y0 + j;
                const Zone *zone = map.zoneAt(x, y);
                for (int32_t k = 0; k < height; k++)
                    cells[index(i, j, k)] = read(zone, x, y, zlo + k);
            }
    }

    struct Cell {
        uint8_t type, light;
    };

    Cell at(int32_t x, int32_t y, int32_t z) const {
        const int32_t i = x - x0, j = y - y0, k = z - zlo;
        if (i >= 0 && j >= 0 && i < SIDE && j < SIDE && k >= 0 && k < height)
            return cells[index(i, j, k)];
        return read(map.zoneAt(x, y), x, y, z);
    }

  private:
    static constexpr int32_t SIDE = CHUNK + 2 * BORDER;
    size_t index(int32_t i, int32_t j, int32_t k) const { return ((size_t)k * SIDE + j) * SIDE + i; }

    Cell read(const Zone *zone, int32_t x, int32_t y, int32_t z) const {
        const Block block = zone ? zone->read(x, y, z) : IMPLICIT_SOLID;
        const uint8_t type = block.type();
        if (!clear(type))
            return {type, 0};
        if (lightPatch && !litWaterDefault(*zone, x, y, z))
            return {type, lightPatch->committed(x, y, z).value_or(block.r)};
        return {type, block.r};
    }

    const WorldMap &map;
    const BlockLightUpdate *lightPatch;
    int32_t x0, y0, zlo, zhi, height;
    std::vector<Cell> cells;
};

struct CornerShade {
    glm::vec3 rgb;
    float light;
};
// The shared Vulkan vertex carries floats, so give it the values the
// original packed color declaration would deliver to the shader.
float normalizedByte(float channel) {
    const auto packed = uint8_t(int32_t(channel * 255.f));
    return float(packed) / 255.f;
}

// §9.3, §9.4: a face corner's light (its vertex alpha) and its colour pulled towards a darker,
// more saturated shade by how closed in it is, from the 6 x 6 cells just outside the face.
CornerShade cornerShade(const CellCache &cache, glm::ivec3 corner, const Face &f, glm::vec3 colour) {
    float lightSum = 0, weights = 0, open = 0;
    glm::ivec3 sample;
    sample[f.axis] = corner[f.axis] + (f.sign > 0 ? 0 : -1);
    // Accumulation follows X/Y/Z order independently of vertex winding.
    const int outer = f.axis == 0 ? 1 : 0;
    const int inner = f.axis == 2 ? 1 : 2;
    for (int32_t a = -3; a <= 2; a++)
        for (int32_t b = -3; b <= 2; b++) {
            sample[outer] = corner[outer] + a;
            sample[inner] = corner[inner] + b;
            const float m = std::max({std::fabs(a + 0.5f), std::fabs(b + 0.5f), 0.5f});
            const float w = (3.f - m) / 3.f;
            const CellCache::Cell s = cache.at(sample.x, sample.y, sample.z);
            float v = 5;
            bool closed = false;
            if (s.type == GLOWING)
                v = 255;
            else if (clear(s.type))
                v = std::max((float)s.light, 5.f);
            else
                closed = true;
            lightSum += v * w;
            weights += w;
            if (!closed)
                open += w;
        }
    const float light = lightSum / (weights * 255.f);
    open /= weights;
    const glm::vec3 c2 = glm::clamp(colour, 0.f, 1.f);
    const float lum = (0.59f * c2.g * 0.1f + 0.3f * c2.r * 0.1f + 0.11f * c2.b * 0.1f) * -9.f;
    const glm::vec3 shade = glm::clamp(c2 * 0.1f * 10.f + lum, 0.f, 1.f);
    return {colour * open + (1.f - open) * shade, light};
}

// The lowest z of a column that is open (clear), or its top: faces below it are hidden.
int32_t lowestOpen(const WorldMap &map, int32_t x, int32_t y) {
    const Zone *zone = map.zoneAt(x, y);
    if (!zone)
        return INT32_MAX; // missing zones read as solid
    const Column &c = zone->column(x, y);
    for (int32_t z = c.bottom; z < c.top(); z++)
        if (clear(zone->read(x, y, z).type()))
            return z;
    return c.top();
}

} // namespace

ChunkLightUpdates::Slot &ChunkLightUpdates::slot(glm::ivec2 chunk) {
    return slots[(chunk.y % SIDE) * SIDE + chunk.x % SIDE];
}

void ChunkLightUpdates::markChanged(std::span<const glm::ivec2> columns) {
    if (columns.empty())
        return;
    const uint64_t change = ++changes;
    for (const auto column : columns) {
        const glm::ivec2 centre = column / CHUNK;
        for (int32_t dx = -1; dx <= 1; ++dx)
            for (int32_t dy = -1; dy <= 1; ++dy) {
                const glm::ivec2 chunk = centre + glm::ivec2(dx, dy);
                if (!validChunk(chunk))
                    return;
                Slot &entry = slot(chunk);
                if (entry.published == chunk)
                    entry.change = change;
            }
    }
}

uint64_t ChunkLightUpdates::beginBuild(glm::ivec2 chunk) {
    if (!validChunk(chunk))
        return 0;
    Slot &entry = slot(chunk);
    if (entry.published != chunk)
        entry.published = glm::ivec2(-1);
    return entry.change;
}

void ChunkLightUpdates::publish(glm::ivec2 chunk, uint64_t buildChange, bool preparedLighting) {
    if (!validChunk(chunk))
        return;
    Slot &entry = slot(chunk);
    entry.published = chunk;
    // Preserve edits marked after this worker took its terrain snapshot.
    if (preparedLighting && entry.change == buildChange)
        entry.change = 0;
}

void ChunkLightUpdates::invalidate(glm::ivec2 chunk) {
    if (validChunk(chunk) && slot(chunk).published == chunk)
        slot(chunk).published = glm::ivec2(-1);
}

int32_t meshLightPadding(int32_t x0, int32_t y0, bool lightingRequested) {
    if (lightingRequested)
        return 4;
    return seamOffset(x0) || seamOffset(y0) ? 16 : 0;
}

std::shared_ptr<const BlockLightUpdate> prepareChunkLight(const WorldMap &map, int32_t x0, int32_t y0,
                                                         bool lightingRequested) {
    const int32_t pad = meshLightPadding(x0, y0, lightingRequested);
    if (!pad || !meshLightAdmitted(map, x0, y0))
        return {};
    const LightRectangle rectangle{{x0 - pad, y0 - pad}, {x0 + CHUNK + pad, y0 + CHUNK + pad}};
    return std::make_shared<const BlockLightUpdate>(map, rectangle);
}

ChunkMeshes meshChunk(const WorldMap &map, const Terrain &terrain, int32_t x0, int32_t y0, bool lightingRequested) {
    const auto prepared = prepareChunkLight(map, x0, y0, lightingRequested);
    return meshChunk(map, terrain, x0, y0, prepared.get());
}

ChunkMeshes meshChunk(const WorldMap &map, const Terrain &terrain, int32_t x0, int32_t y0, const BlockLightUpdate *lightPatch) {
    ChunkMeshes out;
    out.preparedLighting = lightPatch != nullptr;
    const CellCache cache(map, x0, y0, lightPatch);

    for (int32_t x = x0; x < x0 + CHUNK; x++)
        for (int32_t y = y0; y < y0 + CHUNK; y++) {
            const Zone *zone = map.zoneAt(x, y);
            if (!zone)
                continue;
            const Column &column = zone->column(x, y);
            // The cells to look at: from the lowest a neighbour opens onto, up to the top (and up
            // to sea level, for the sea's surface).
            int32_t zmin = column.bottom;
            for (const auto &n : {glm::ivec2(1, 0), glm::ivec2(-1, 0), glm::ivec2(0, 1), glm::ivec2(0, -1)})
                zmin = std::min(zmin, lowestOpen(map, x + n.x, y + n.y));
            const int32_t zmax = std::max(column.top() - 1, 0);
            // §9.5: an empty column's reference is white type 0 (empty).
            const Block reference = column.blocks.empty() ? Block{255, 255, 255, AIR} : column.blocks.front();

            for (int32_t z = zmin; z <= zmax; z++) {
                const CellCache::Cell cell = cache.at(x, y, z);
                if (cell.type == AIR)
                    continue;
                const bool water = cell.type == WATER;
                // §9.5: the block's colour, 0..1.
                glm::vec3 colour;
                if (water) {
                    const float b = cell.light / 255.f;
                    colour = glm::vec3(0.1f, 0.2f, 1) * (1 - b) + glm::vec3(0.3f, 0.4f, 1) * b;
                } else if (z < column.bottom) {
                    // Under the stored blocks: rock colour blended in with depth, or unblended
                    // when the reference block is empty or water (§9.5, correction 28).
                    const glm::vec3 rock = terrain.rock(x, y, z, zone);
                    if (clear(reference.type()))
                        colour = rock / 255.f;
                    else {
                        const float k = std::min((column.bottom - z) * 0.1f, 1.f);
                        const glm::vec3 ref(reference.r, reference.g, reference.b);
                        colour = (ref * (1 - k) + rock * k) / 255.f;
                    }
                } else {
                    const Block b = zone->read(x, y, z);
                    colour = glm::vec3(b.r, b.g, b.b) / 255.f;
                }
                const glm::ivec3 voxel(x, y, z);
                for (const Face &f : FACES) {
                    glm::ivec3 next = voxel;
                    next[f.axis] += f.sign;
                    const uint8_t neighbour = cache.at(next.x, next.y, next.z).type;
                    // §9.1: faces towards air always; solid faces towards water; never
                    // water-water or solid-solid.
                    if (neighbour != AIR && (water || neighbour != WATER))
                        continue;
                    glm::vec3 normal(0);
                    normal[f.axis] = (float)f.sign;
                    // One face of `rgbIn` into `mesh`: each corner through §9.3 and §9.4.
                    const auto emit = [&](ModelMesh &mesh, glm::vec3 rgbIn, bool sways) {
                        const uint32_t first = (uint32_t)mesh.vertices.size();
                        for (const auto &c : CORNERS) {
                            glm::ivec3 corner = voxel;
                            corner[f.axis] += f.sign > 0 ? 1 : 0;
                            corner[f.u] += c[0];
                            corner[f.v] += c[1];
                            const auto [rgb, light] = cornerShade(cache, corner, f, rgbIn);
                            const glm::vec4 packedColour{
                                normalizedByte(rgb.r), normalizedByte(rgb.g), normalizedByte(rgb.b), normalizedByte(light)};
                            const float sway =
                                sways ? leafSway(corner.x - x0, corner.y - y0, [&](int dx, int dy, int dz) {
                                    return cache.at(corner.x + dx, corner.y + dy, corner.z + dz).type;
                                })
                                      : 0.f;
                            mesh.vertices.push_back(
                                {glm::vec3(corner - glm::ivec3(x0, y0, 0)), normal, packedColour, sway});
                        }
                        for (uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u})
                            mesh.indices.push_back(first + i);
                    };
                    // §9.1: the +Z face of a type-3 block under an empty block first adds a
                    // (0, 0, 1, 1) film quad to the water list, shaded like any face (water
                    // material contract: material 3 below, material 0 above), then its own top.
                    if (cell.type == LAKE_BED && neighbour == AIR && f.axis == 2 && f.sign > 0)
                        emit(out.water, glm::vec3(0, 0, 1), false);
                    emit(water ? out.water : out.solid, colour, cell.type == LEAVES_TYPE);
                }
            }
        }
    return out;
}
