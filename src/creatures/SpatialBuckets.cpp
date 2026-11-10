#include "creatures/SpatialBuckets.h"
#include "creatures/RouteGeometry.h"

namespace creatures {

void SpatialBuckets::build(const worldgen::Zone &zone) {
    offsets.fill(0);
    records.clear();
    if (zone.zx < 0 || zone.zy < 0 || zone.zx >= 65536 || zone.zy >= 65536)
        return;
    constexpr uint16_t outside = 1024;
    std::vector<uint16_t> membership(zone.statics.size(), outside);
    const int32_t lowerX = zone.zx * 32, lowerY = zone.zy * 32;
    for (size_t index = 0; index < zone.statics.size(); index++) {
        const auto &record = zone.statics[index];
        const int32_t y = route::publicationCell(record.y);
        const int32_t x = route::publicationCell(record.x);
        if (x < 0 || y < 0 || x >= 0x200000 || y >= 0x200000 ||
            x < lowerX || x >= lowerX + 32 || y < lowerY || y >= lowerY + 32)
            continue;
        const auto bucket = uint16_t((y % 32) * 32 + x % 32);
        membership[index] = bucket;
        offsets[size_t(bucket) + 1]++;
    }
    for (size_t i = 1; i < offsets.size(); i++)
        offsets[i] += offsets[i - 1];
    records.resize(offsets.back());
    auto next = offsets;
    for (size_t index = 0; index < membership.size(); index++)
        if (membership[index] != outside)
            records[next[membership[index]]++] = uint32_t(index);
}

std::span<const uint32_t> SpatialBuckets::at(int32_t localX, int32_t localY) const {
    const size_t bucket = size_t(localY * 32 + localX);
    return std::span(records).subspan(offsets[bucket], offsets[bucket + 1] - offsets[bucket]);
}

} // namespace creatures
