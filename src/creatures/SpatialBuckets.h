#pragma once
#include "worldgen/Zone.h"
#include <array>
#include <span>
#include <vector>

namespace creatures {

// Read-only index of a held generation snapshot. One admitted record goes into
// one center-selected bucket; each bucket retains source ordinal order.
class SpatialBuckets {
  public:
    void build(const worldgen::Zone &zone);
    std::span<const uint32_t> at(int32_t localX, int32_t localY) const;

  private:
    std::array<uint32_t, 1025> offsets{};
    std::vector<uint32_t> records;
};

} // namespace creatures
