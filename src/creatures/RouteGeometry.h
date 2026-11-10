#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace creatures::route {

using Position = std::array<int64_t, 3>;
using Dimensions = std::array<float, 3>;
struct Record {
    uintptr_t identity = 0; // spatial-record identity, never a creature ID
    int32_t selector = 0;
    Position position{};
    int32_t orientation = 0;
    Dimensions size{1, 1, 1};
    uint8_t state = 0;
    std::array<int32_t, 3> reference{}; // bucket owner X/Y/ordinal, not record +0x178
};
using RecordVisitor = std::function<bool(const Record &)>; // false stops this bucket
using RecordLookup = std::function<void(int32_t cellX, int32_t cellY, const RecordVisitor &)>;
struct Subject {
    std::span<const int64_t, 3> position;
    std::span<const float, 3> size;
    const uint16_t &flags;
    std::function<bool(uintptr_t)> excluded;
};
struct Column {
    int32_t base = 0, count = 0;
    // Valid resident four-byte cells, RGB followed by the complete type byte.
    // Storage remains readable for the query; no loading or allocation here.
    std::span<const std::byte> cells;
};
struct Scene {
    std::function<std::optional<Column>(int32_t x, int32_t y)> column;
    // Ordered resident spatial pass after a tentative axis movement. The last
    // argument retains earlier terrain blockage for the third-axis traversal.
    std::function<bool(const Position &, int axis, const Dimensions &direction, bool terrainBlocked)> spatial;
    // Binary32 input, promoted square root, then binary32 result. An absent
    // callback uses the bounded platform adapter; native runtime internals open.
    std::function<float(float)> squareRoot;
};

// Held supplied-input record consumers. Setup overlap omits pointer exclusion;
// movement applies it before the state and directional/truncation gates.
bool endpointOverlap(const Subject &subject, const Position &center, const Record &record);
bool recordBlocks(const Subject &subject, const Position &center, int axis,
                  const Dimensions &direction, const Record &record);
int32_t publicationCell(int64_t raw);
bool scanRecords(const Subject &subject, const Position &center, int axis, const Dimensions &direction,
                 bool terrainBlocked, const RecordLookup &lookup);
void endpointExclusions(const Subject &subject, std::span<const int64_t, 3> endpoint,
                        const RecordLookup &lookup, const std::function<void(uintptr_t)> &insert);

// Held alpha-companion-direct-route-geometry.md. Stable valid supplied storage,
// ordinary finite representable conversions and nonoverflowing grid bounds.
// Changes only a private position. True may mean progress beyond 25 units,
// independently of target proximity; it is not proof of target arrival.
bool direct(const Subject &subject, std::span<const int64_t, 3> target, float radius, const Scene &scene);

} // namespace creatures::route
