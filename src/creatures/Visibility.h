#pragma once
#include "creatures/Physics.h"
#include <array>
#include <cstdint>
#include <functional>

namespace creatures::visibility {

using Position = std::array<int64_t, 3>;
using Direction = std::array<float, 3>;

// Supplied spatial-record state. Dimensions are blocks; Z is the lower face.
// The query tests only four side faces for kinds 3/4/5/8 with a nonzero state.
struct Record {
    int32_t kind = 0;
    Position position{};
    Direction dimensions{1, 1, 1};
    int32_t selector = 0;
    uint8_t state = 0;
};
using RecordVisitor = std::function<void(const Record &)>;
using RecordLookup = std::function<void(int32_t zoneX, int32_t zoneY, const RecordVisitor &)>;

struct Scene {
    BlockType voxel;
    RecordLookup records;
};

// Held alpha-visibility-ray-geometry.md: finite representable inputs and stable
// callbacks. No direction normalization. Returned parameter may be negative
// when the supplied maximum is negative. Callbacks retain their source order.
float trace(const Position &start, const Direction &direction, float maximum,
            uint8_t requestedCategory, uint8_t additionalGeometry, const Scene &scene);

// The bounded target-selection visibility caller, with a supplied scene.
bool visible(const Position &start, const Position &end, const Scene &scene);
// The same routine (0x19EE90) with the caller's option and maximum distance.
bool visible(const Position &start, const Position &end, uint8_t option, float maximum, const Scene &scene);

} // namespace creatures::visibility
