#include "worldgen/BlockLight.h"
#include <algorithm>
#include <bit>

namespace worldgen {

namespace {

constexpr int32_t WORLD_WIDTH = 0x1000000;
constexpr int MINIMUM_OPEN_LIGHT = 5;
constexpr int ROUNDS = 16;

bool inWorld(int32_t x, int32_t y) {
    return uint32_t(x) < WORLD_WIDTH && uint32_t(y) < WORLD_WIDTH;
}

int32_t workingBound(int32_t edge, int64_t margin) {
    return int32_t(std::clamp(int64_t(edge) + margin, int64_t(0), int64_t(WORLD_WIDTH)));
}

} // namespace

LightRectangle destructionLightBounds(const std::array<int64_t, 3> &centre, float radius) {
    const float scaledRadius = radius * 65536.f;
    const int64_t displacement = scaledRadius >= -0x1p63f && scaledRadius < 0x1p63f ? int64_t(scaledRadius) : INT64_MIN;
    const auto whole = [](uint64_t fixed) {
        return std::bit_cast<int32_t>(uint32_t(std::bit_cast<int64_t>(fixed) / 65536));
    };
    const uint64_t distance = uint64_t(displacement);
    const uint64_t x = uint64_t(centre[0]), y = uint64_t(centre[1]);
    return {{whole(x - distance), whole(y - distance)}, {whole(x + distance), whole(y + distance)}};
}

void relightAfterDestruction(WorldMap &map, const std::array<int64_t, 3> &centre, float radius) {
    BlockLightUpdate::AreaCopies copies;
    BlockLightUpdate(map, destructionLightBounds(centre, radius), 8).store(map, copies);
}

BlockLightUpdate::BlockLightUpdate(const WorldMap &map, LightRectangle destination, int32_t workingMargin)
    : BlockLightUpdate(destination, workingMargin, [&](int32_t x, int32_t y) -> const Column * {
          const Zone *area = map.zoneAt(x, y);
          return area ? &area->column(x, y) : nullptr;
      }) {}

BlockLightUpdate::BlockLightUpdate(const Zone &area, LightRectangle destination, int32_t workingMargin)
    : BlockLightUpdate(destination, workingMargin, [&](int32_t x, int32_t y) -> const Column * {
          return area.contains(x, y) ? &area.column(x, y) : nullptr;
      }) {}

size_t BlockLightUpdate::index(int32_t x, int32_t y) const {
    return size_t(x - working.lower.x + 1) * pitch + (y - working.lower.y + 1);
}

BlockLightUpdate::BlockLightUpdate(LightRectangle destination, int32_t margin, const ColumnLookup &columnAt)
    : destination(destination),
      working{{workingBound(destination.lower.x, -int64_t(margin)), workingBound(destination.lower.y, -int64_t(margin))},
              {workingBound(destination.upper.x, margin), workingBound(destination.upper.y, margin)}} {
    const glm::ivec2 extent = working.upper - working.lower;
    if (extent.x <= 0 || extent.y <= 0)
        return;
    pitch = extent.y + 2;
    columns.resize(size_t(extent.x + 2) * pitch);
    std::vector<const Column *> source(columns.size());
    // Resolve the working columns and their one-cell halo once. In the halo
    // propagation reads the existing blue byte of a stored clear cell.
    for (int32_t x = working.lower.x - 1; x <= working.upper.x; ++x)
        for (int32_t y = working.lower.y - 1; y <= working.upper.y; ++y)
            if (inWorld(x, y))
                source[index(x, y)] = columnAt(x, y);

    struct ShadedCell {
        size_t column, offset;
        int32_t z;
        uint8_t next = 0;
    };
    std::vector<ShadedCell> shaded;
    for (int32_t x = working.lower.x; x < working.upper.x; ++x)
        for (int32_t y = working.lower.y; y < working.upper.y; ++y) {
            const size_t i = index(x, y);
            const Column *column = source[i];
            if (!column || column->blocks.empty())
                continue;
            const int32_t count = int32_t(column->blocks.size());
            const size_t offset = values.size();
            columns[i] = {column->bottom, count, offset};
            values.resize(offset + count);
            const size_t first = shaded.size();
            bool open = true;
            for (int32_t k = count - 1; k >= 0; --k) {
                if (!clear(column->blocks[k].type())) {
                    open = false;
                } else if (open) {
                    values[offset + k] = 255;
                } else {
                    shaded.push_back({i, offset + k, column->bottom + k});
                }
            }
            // Seeds are visited top down. The shaded targets run bottom up.
            std::reverse(shaded.begin() + first, shaded.end());
        }

    const auto neighbour = [&](size_t i, int32_t z) -> int {
        const Column *column = source[i];
        if (!column || z < column->bottom)
            return 0;
        if (z >= column->top())
            return 255;
        const size_t k = z - column->bottom;
        const Block &cell = column->blocks[k];
        if (cell.type() == GLOWING || cell.implicitWaterAt(z))
            return 255;
        if (!clear(cell.type()))
            return 0;
        const auto &range = columns[i];
        const int light = range.count ? values[range.offset + k] : cell.b;
        return std::max(light, MINIMUM_OPEN_LIGHT);
    };
    struct Step {
        int32_t column, z;
    };
    const Step steps[] = {{-pitch, 0}, {pitch, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int round = 0; round < ROUNDS; ++round) {
        for (auto &cell : shaded) {
            int brightest = 0;
            for (const auto &step : steps) {
                brightest = std::max(brightest, neighbour(size_t(ptrdiff_t(cell.column) + step.column), cell.z + step.z));
                if (brightest == 255)
                    break;
            }
            cell.next = uint8_t(brightest * 85 / 100);
        }
        for (const auto &cell : shaded)
            values[cell.offset] = cell.next;
    }
}

std::optional<uint8_t> BlockLightUpdate::committed(int32_t x, int32_t y, int32_t z) const {
    if (!pitch || !destination.contains(x, y) || !working.contains(x, y))
        return std::nullopt;
    const auto &range = columns[index(x, y)];
    if (z < range.bottom || z >= range.bottom + range.count)
        return std::nullopt;
    return values[range.offset + (z - range.bottom)];
}

void BlockLightUpdate::store(Zone &area) const {
    if (!pitch)
        return;
    const int32_t left = std::max(working.lower.x, area.originX()), right = std::min(working.upper.x, area.originX() + Zone::SIZE);
    const int32_t front = std::max(working.lower.y, area.originY()), back = std::min(working.upper.y, area.originY() + Zone::SIZE);
    for (int32_t x = left; x < right; ++x)
        for (int32_t y = front; y < back; ++y) {
            const auto &range = columns[index(x, y)];
            if (range.count == 0)
                continue;
            auto &column = area.column(x, y);
            if (column.bottom != range.bottom || column.blocks.size() != size_t(range.count))
                continue;
            const bool commit = destination.contains(x, y);
            for (int32_t k = 0; k < range.count; ++k)
                if (Block &block = column.blocks[k]; clear(block.type())) {
                    const uint8_t light = values[range.offset + k];
                    block.g = block.b = light;
                    if (commit)
                        block.r = light;
                }
        }
}

void BlockLightUpdate::store(WorldMap &map, AreaCopies &copies) const {
    if (!pitch)
        return;
    const glm::ivec2 first = working.lower / Zone::SIZE;
    const glm::ivec2 last = (working.upper - glm::ivec2(1)) / Zone::SIZE;
    for (int32_t ax = first.x; ax <= last.x; ++ax)
        for (int32_t ay = first.y; ay <= last.y; ++ay)
            if (const auto *area = map.zone(ax, ay)) {
                auto &copy = copies[{ax, ay}];
                // The destination map and this cache own a writable copy together.
                // A map snapshot or independently held area must retain its original bytes.
                if (copy.get() != area || copy.use_count() != 2) {
                    copy = std::make_shared<Zone>(*area);
                    map.replaceTerrain(copy);
                }
                store(*copy);
            }
}

} // namespace worldgen
