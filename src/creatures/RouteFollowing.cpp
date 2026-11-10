#include "creatures/RouteFollowing.h"
#include <bit>
#include <cmath>

namespace creatures::search {
namespace {
int64_t add(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b)); }
int64_t subtract(int64_t a, int64_t b) { return std::bit_cast<int64_t>(uint64_t(a) - uint64_t(b)); }
int64_t fixed(float value) { return int64_t(value * 65536.f); }
int64_t half(float size) { return fixed(size * .5f); }
int32_t distance(const Cell &a, const Cell &b) {
    std::array<uint32_t, 3> square{};
    for (size_t axis = 0; axis < 3; axis++) {
        const uint32_t delta = uint32_t(a[axis]) - uint32_t(b[axis]);
        square[axis] = delta * delta;
    }
    return std::bit_cast<int32_t>((square[2] + square[1]) + square[0]);
}
route::Position center(const Cell &cell, const Body &body) {
    const int64_t vertical = half(body.size.z);
    return {add(int64_t(cell[0]) * 65536, 32768), add(int64_t(cell[1]) * 65536, 32768),
            add(int64_t(cell[2]) * 65536, vertical)};
}
route::Position adjusted(const Body &body, int64_t zOffset) {
    return {body.position[0], body.position[1], add(body.position[2], zOffset)};
}
void blend(float &value, float target, float coefficient, int32_t count) {
    // Each recurrence has three separate binary32 rounding boundaries. A power
    // or scaled coefficient changes the results, even for ordinary durations.
    const auto advance = [&](float current) {
        const float delta = target - current;
        const float change = coefficient * delta;
        return current + change;
    };
    while (count >= 4) {
        const float first = advance(value), second = advance(first);
        const float third = advance(second), fourth = advance(third);
        value = fourth;
        count -= 4;
    }
    while (count > 0) {
        value = advance(value);
        --count;
    }
}
bool interactionOverlap(const Body &body, const route::Record &record) {
    const bool odd = (uint32_t(record.orientation) & 1) != 0;
    for (size_t axis = 0; axis < 2; axis++) {
        const int64_t actorHalf = half(body.size[glm::length_t(axis)]);
        const int64_t recordHalf = half(record.size[odd ? 1 - axis : axis]);
        const int64_t actorLow = subtract(subtract(body.position[axis], actorHalf), 65536);
        const int64_t actorHigh = add(add(body.position[axis], actorHalf), 65536);
        if (subtract(record.position[axis], recordHalf) > actorHigh || actorLow >= add(record.position[axis], recordHalf))
            return false;
    }
    const int64_t actorHalf = half(body.size.z);
    if (record.position[2] > add(body.position[2], actorHalf))
        return false;
    const int64_t height = fixed(record.size[2]);
    return subtract(body.position[2], actorHalf) < add(record.position[2], height);
}
void interact(const Body &body, const FollowingServices &services) {
    if (!services.scene.records)
        return;
    Cell lower{}, upper{};
    for (const bool high : {false, true}) {
        route::Position corner{};
        for (size_t axis = 0; axis < 3; axis++) {
            const int64_t extent = half(body.size[glm::length_t(axis)]);
            corner[axis] = high ? add(body.position[axis], extent) : subtract(body.position[axis], extent);
        }
        (high ? upper : lower) = coordinate(corner);
    }
    for (int32_t x = (lower[0] - 1) / 8; x <= (upper[0] + 1) / 8; x++)
        for (int32_t y = (lower[1] - 1) / 8; y <= (upper[1] + 1) / 8; y++)
            if (x >= 0 && y >= 0 && x < 0x200000 && y < 0x200000)
                services.scene.records(x, y, [&](const route::Record &record) {
                    if ((record.selector == 1 || record.selector == 2) && record.state != 0 &&
                        interactionOverlap(body, record) && services.activate)
                        services.activate(record);
                    return true;
                });
}
}

bool belowTerrain(const route::Position &at, std::span<const float, 3> size,
                  const std::function<uint8_t(int32_t, int32_t, int32_t)> &cell) {
    Cell lower{}, upper{};
    for (const bool high : {false, true}) {
        route::Position corner{};
        for (size_t axis = 0; axis < 3; axis++) {
            const int64_t extent = half(size[axis]);
            corner[axis] = high ? add(at[axis], extent) :
                subtract(subtract(at[axis], extent), axis == 2 ? 13107 : 0);
        }
        (high ? upper : lower) = coordinate(corner);
    }
    for (int32_t x = lower[0]; x <= upper[0]; x++)
        for (int32_t y = lower[1]; y <= upper[1]; y++) {
            const uint8_t type = cell(x, y, lower[2]) & 31;
            if (type != 0 && type != 2)
                return true;
        }
    return false;
}

bool follow(State &state, Body &body, int32_t elapsed, const FollowingServices &services) {
    body.intent = glm::vec3(0);
    const float halfHeight = body.size.z * .5f;
    const float entryOffset = .5f - halfHeight;
    const int64_t offset = fixed(entryOffset);
    const Cell arrival = coordinate(adjusted(body, offset));
    const Cell goal = coordinate(state.goal);
    const float radiusSquared = state.radius * state.radius;
    if (float(distance(arrival, goal)) <= radiusSquared) {
        state.clear();
        return false;
    }
    if (state.path.empty())
        return false;
    if (state.counter > 3000) {
        state.start = adjusted(body, offset);
        services.setup();
        if (state.path.empty())
            return false;
    }
    const int64_t nearestOffset = half(body.size.z);
    size_t nearest = 0;
    int32_t nearestDistance = -1;
    for (size_t i = 0; i < state.path.size(); i++) {
        const Cell feet = coordinate(adjusted(body, -nearestOffset));
        const int32_t candidate = distance(state.path[i], feet);
        if (nearestDistance < 0 || candidate <= nearestDistance) {
            nearestDistance = candidate;
            nearest = i;
        }
    }
    if (nearestDistance == 0)
        state.counter = 0;
    const bool partial = !(body.contact & 0x40) && state.path.size() < 30 &&
        float(distance(state.path.back(), coordinate(state.goal))) > radiusSquared;
    if (!partial && nearestDistance == 0 && state.path[nearest] != state.anchor)
        state.reroot(state.path[nearest]);
    const route::Dimensions dimensions{body.size.x, body.size.y, body.size.z};
    const auto ground = [&](size_t index) {
        return belowTerrain(center(state.path[index], body), dimensions, services.scene.cell);
    };
    const bool firstBlocked = ground(nearest);
    bool loose = false, verticalGate = true, secondBlocked = true;
    size_t target = nearest;
    if (partial || nearest + 1 == state.path.size()) {
        loose = true;
    } else {
        target = nearest + 1;
        secondBlocked = ground(target);
        loose = !secondBlocked;
    }
    if (!partial && (firstBlocked || secondBlocked) && state.path[target][2] != state.path[nearest][2] &&
        target + 1 < state.path.size()) {
        ++target;
        verticalGate = false;
        if (ground(target)) {
            verticalGate = true;
            loose = false;
        }
    }
    const route::Position destination = center(state.path[target], body);
    glm::vec3 direction{};
    for (size_t axis = 0; axis < 3; axis++)
        direction[glm::length_t(axis)] = float(subtract(destination[axis], body.position[axis])) * (1.f / 65536.f);
    if (nearestDistance < 4 && (body.contact & 4) && !firstBlocked)
        body.mode |= 1;
    else
        body.mode &= ~1u;
    verticalGate = verticalGate && firstBlocked && !(body.contact & 0x40) && direction.z <= 1.f;
    const float y2 = direction.y * direction.y, x2 = direction.x * direction.x, z2 = direction.z * direction.z;
    const float lengthSquared = (y2 + x2) + z2;
    if (!(lengthSquared > .001f))
        return true;
    if (lengthSquared > 1.f) {
        const float root = services.squareRoot ? services.squareRoot(lengthSquared) : float(std::sqrt(double(lengthSquared)));
        const float inverse = 1.f / root;
        for (int axis = 0; axis < 3; axis++)
            direction[axis] = direction[axis] * inverse;
    }
    if (direction.z > 0) direction.z = 1.f;
    else if (direction.z < 0 && (body.contact & 2)) direction.z = -2.f;
    if (body.contact & 0x40) {
        direction.z = 3.f;
        interact(body, services);
    }
    if (partial && !(body.mode & 1))
        return false;
    if (nearestDistance >= 4) {
        body.intent = direction * 40.f;
    } else {
        if (partial && direction.z >= 0) {
            body.velocity = direction * 8.f;
        } else {
            if (!partial && direction.z >= 0 && !verticalGate)
                blend(body.velocity.z, direction.z * 4.f, .05f, elapsed);
            const float speed = loose ? 8.f : (body.mode & 0x40) ? 12.f : 6.f;
            const float coefficient = loose ? .01f : .001f;
            blend(body.velocity.x, direction.x * speed, coefficient, elapsed);
            blend(body.velocity.y, direction.y * speed, coefficient, elapsed);
        }
        body.intent = direction * 20.f;
    }
    body.push = glm::vec3(0);
    return true;
}

} // namespace creatures::search
