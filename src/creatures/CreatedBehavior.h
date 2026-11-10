#pragma once
#include "creatures/Combat.h"
#include "creatures/Physics.h"
#include "creatures/RouteGeometry.h"

namespace creatures::created {

using Position = std::array<int64_t, 3>;
using PathState = route::Endpoints;
struct PathStatus {
    int32_t nodes = 0, coordinates = 0;
    std::array<int32_t, 3> tail{};
};
struct CompanionServices {
    std::function<const Body *(uint64_t)> lookup;
    std::function<float()> maximumHealth;
    std::function<bool(const Body &, const Body &, float)> directRoute;
    std::function<uint8_t(const Position &)> voxel;
    std::function<void()> clearPath;
    std::function<bool(int32_t, int32_t)> cellPresent;
    std::function<void()> initializePath;
    std::function<void()> pathStep; // one expansion, then predecessor-list production
    std::function<PathStatus()> pathStatus;
};

// Held second-child caller. Geometry and path operations are explicit supplied
// dependencies; the Boolean result does not certify a completed route.
bool companion(Combatant &actor, Body &body, uint64_t referenceKey, int32_t step,
               PathState &path, const CompanionServices &services);

// The held third child's complete ordinary update. It always handles the call,
// including a nonzero remaining timer. Reference coordinates are supplied raw
// state; their producers must not be inferred from current/creation position.
void wander(Combatant &actor, Body &body, const std::array<int64_t, 3> &reference,
            int32_t &counter, int32_t step, const std::function<int32_t()> &draw);

} // namespace creatures::created
