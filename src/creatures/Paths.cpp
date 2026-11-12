#include "creatures/Paths.h"
#include <algorithm>
#include <cmath>
#include <bit>

namespace creatures {

void PathSearch::clear() {
    state.clear();
    ignored.clear();
}

void PathSearch::start(glm::dvec3 from, glm::dvec3 to, float radius, glm::vec3 size) {
    // This block-coordinate entry is an engine adapter. The created-companion
    // caller writes the shared raw endpoints directly before setup instead.
    for (int axis = 0; axis < 3; axis++) {
        state.start[size_t(axis)] = int64_t(from[axis] * 65536.0);
        state.goal[size_t(axis)] = int64_t(to[axis] * 65536.0);
    }
    state.radius = radius;
    setup(size);
}

void PathSearch::setup(glm::vec3 size) {
    ignored.clear();
    const route::Dimensions dimensions{size.x, size.y, size.z};
    route::RecordLookup lookup;
    if (records)
        lookup = [&](int32_t x, int32_t y, const route::RecordVisitor &visit) {
            records(x, y, [&](const route::Record &record) {
                const bool keepGoing = visit(record);
                if (state.exclusions.contains(record.identity))
                    ignored.insert(record.reference);
                return keepGoing;
            });
        };
    state.setup(dimensions, lookup);
}

void PathSearch::round(glm::vec3 size, const BlockType &block) {
    const route::Dimensions dimensions{size.x, size.y, size.z};
    const search::Scene scene{
        [&](int32_t x, int32_t y, int32_t z) {
            return columns ? route::residentCell(columns, x, y, z) : block(x, y, z);
        }, records};
    const search::Subject subject{dimensions, state.reference,
        [&](uintptr_t identity) { return state.exclusions.contains(identity); }};
    state.step([&](const Cell &at) { return search::classify(at, subject, scene); });
    state.rebuild();
    if (state.records.empty())
        ignored.clear();
}

void PathSearch::rounds(glm::vec3 size, float stop, const BlockType &block) {
    // Work/arrival policy for older engine callers remains a separate adapter.
    // The held created-companion caller supplies its own strict stopping tests.
    for (int i = 0; i < 10 && searching() && state.path.size() <= 50; i++) {
        if (!empty()) {
            const Cell goal = search::coordinate(state.goal);
            const Cell last = state.path.back();
            const double x = double(last[0]) - goal[0], y = double(last[1]) - goal[1], z = double(last[2]) - goal[2];
            if ((x * x + y * y) + z * z <= double(stop) * stop)
                break;
        }
        round(size, block);
    }
}

bool PathSearch::follow(Body &body, int32_t elapsed, const BlockType &block) {
    search::FollowingServices services;
    services.scene.cell = [&](int32_t x, int32_t y, int32_t z) {
        return columns ? route::residentCell(columns, x, y, z) : block(x, y, z);
    };
    services.scene.records = records;
    services.setup = [&] { setup(body.size); };
    services.activate = activate;
    const bool result = search::follow(state, body, elapsed, services);
    if (state.records.empty())
        ignored.clear();
    return result;
}

glm::vec3 PathSearch::fallback(const Body &body) const {
    const Cell tail = state.path.back();
    route::Position difference{};
    for (size_t axis = 0; axis < 3; axis++) {
        const int64_t target = int64_t(tail[axis]) * 65536 + 32768;
        difference[axis] = std::bit_cast<int64_t>(uint64_t(target) - uint64_t(body.position[axis]));
    }
    glm::vec3 result{};
    for (int axis = 0; axis < 3; axis++)
        result[axis] = float(difference[size_t(axis)]) * (1.f / 65536.f);
    return result;
}

} // namespace creatures
