#pragma once
#include "creatures/SearchState.h"

namespace creatures::search {

struct Subject {
    std::span<const float, 3> size;
    std::span<const int32_t, 3> reference;
    std::function<bool(uintptr_t)> excluded;
};
struct Scene {
    // Complete, already resolved cell type byte, including native sentinels.
    std::function<uint8_t(int32_t x, int32_t y, int32_t z)> cell;
    route::RecordLookup records;
};

// Held selected-coordinate classifier. All terrain boundary queries and resident
// entries retain their order; record state is deliberately not an admission gate.
Classification classify(const Cell &selected, const Subject &subject, const Scene &scene);

} // namespace creatures::search
