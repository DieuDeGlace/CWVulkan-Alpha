#include "creatures/Paths.h"
#include <algorithm>
#include <cmath>
#include <bit>

namespace creatures {

namespace {

bool solid(uint8_t type) { return type != 0 && type != 2; }

int32_t cellOf(double v) { return (int32_t)std::floor(v); }

// The cells a box with centre c covers (physics §0).
struct Range {
    int32_t lo[3], hi[3];
};
Range rangeOf(glm::dvec3 c, glm::vec3 size) {
    Range r;
    for (int k = 0; k < 3; k++) {
        r.lo[k] = cellOf(c[k] - size[k] * 0.5);
        r.hi[k] = cellOf(c[k] + size[k] * 0.5);
    }
    return r;
}

bool anySolid(const Range &r, const BlockType &block) {
    for (int32_t x = r.lo[0]; x <= r.hi[0]; x++)
        for (int32_t y = r.lo[1]; y <= r.hi[1]; y++)
            for (int32_t z = r.lo[2]; z <= r.hi[2]; z++)
                if (solid(block(x, y, z)))
                    return true;
    return false;
}


} // namespace

bool clearRoute(glm::vec3 size, bool swimmer, glm::dvec3 P, glm::dvec3 T, float R, const BlockType &block,
                const ObjectTest &objects, const IgnoredStatics *ignored) {
    const auto reached = [&](glm::dvec3 Q, bool strict) {
        const double d = glm::length(glm::dvec2(T - Q));
        return (strict ? d < R : d <= R) && std::abs(Q.z - T.z) < size.z + R;
    };
    if (reached(P, true))
        return true;
    const double D = std::min(glm::length(glm::dvec2(T - P)), 50.0);
    const int32_t n = (int32_t)(std::sqrt(D * D + 0.5625) + 1);
    glm::dvec3 Q = P;
    for (int32_t k = 0; k < n; k++) {
        if (reached(Q, false))
            return true;
        glm::dvec2 u = glm::dvec2(T - Q);
        if (glm::dot(u, u) > 0)
            u = glm::normalize(u);
        const glm::dvec3 step(u.x, u.y, -0.71);
        for (int axis = 0; axis < 3; axis++) {
            Q[axis] += step[axis];
            if (!anySolid(rangeOf(Q, size), block)) {
                // A static in the way stops this axis, with no step-up.
                const glm::dvec3 h = glm::dvec3(size) * 0.5;
                if (objects && objects(Q - h, Q + h, -1, 0, ignored))
                    Q[axis] -= step[axis];
                continue;
            }
            if (axis < 2 && !swimmer) { // a one-block step-up (the bottom ends 2.02 up, as read)
                Range up = rangeOf(Q + glm::dvec3(0, 0, 1.01), size);
                if (!anySolid(up, block)) {
                    const double bottom = std::floor(Q.z - size.z * 0.5) + 2.02;
                    Q.z = bottom + size.z * 0.5;
                    continue;
                }
            }
            Q[axis] -= step[axis];
        }
    }
    return glm::length(Q - P) > 25;
}

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
