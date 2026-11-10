#pragma once
#include "creatures/Physics.h"
#include "creatures/SearchGeometry.h"

namespace creatures::search {

struct FollowingServices {
    Scene scene;
    // Reached refresh must perform setup, including endpoint exclusions.
    std::function<void()> setup;
    // Synchronous byte-zero then timer-zero for this still-resident identity.
    std::function<void(const route::Record &)> activate;
    std::function<float(float)> squareRoot;
};

bool belowTerrain(const route::Position &center, std::span<const float, 3> size,
                  const std::function<uint8_t(int32_t, int32_t, int32_t)> &cell);
// Supplied-input consumer; list rebuilding and world fallback belong to caller.
bool follow(State &state, Body &body, int32_t elapsed, const FollowingServices &services);

} // namespace creatures::search
