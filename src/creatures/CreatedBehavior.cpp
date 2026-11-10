#include "creatures/CreatedBehavior.h"
#include <bit>
#include <cmath>

namespace creatures::created {
namespace {
int64_t difference(int64_t reference, int64_t position) {
    return std::bit_cast<int64_t>(uint64_t(reference) - uint64_t(position));
}
int64_t squaredTerm(int64_t value) {
    const int64_t product = std::bit_cast<int64_t>(uint64_t(value) * uint64_t(value));
    return product / 65536;
}
float component(int32_t draw) {
    const float twice = float(draw) * 2.f;
    const float fraction = twice / 32767.f;
    const float centered = fraction - 1.f;
    return centered * 10.f;
}
int64_t add(int64_t a, int64_t b) {
    return std::bit_cast<int64_t>(uint64_t(a) + uint64_t(b));
}
glm::vec3 scaled(const Position &delta) {
    glm::vec3 value;
    for (int axis = 0; axis < 3; axis++) {
        const float narrowed = float(delta[axis]);
        value[axis] = narrowed * (1.f / 65536.f);
    }
    return value;
}
float squared(glm::vec3 value, bool yFirst) {
    const float x = value.x * value.x, y = value.y * value.y, z = value.z * value.z;
    const float horizontal = yFirst ? y + x : x + y;
    return horizontal + z;
}
std::array<int32_t, 3> grid(const Position &position) {
    std::array<int32_t, 3> result;
    for (int axis = 2; axis >= 0; axis--) {
        uint32_t value = uint32_t(position[axis] / 65536);
        if (position[axis] < 0)
            value--;
        result[axis] = std::bit_cast<int32_t>(value);
    }
    return result;
}
float nodeDistance(const std::array<int32_t, 3> &tail, const Position &goal) {
    const auto target = grid(goal);
    std::array<uint32_t, 3> terms;
    for (size_t axis = 0; axis < 3; axis++) {
        const uint32_t delta = uint32_t(tail[axis]) - uint32_t(target[axis]);
        terms[axis] = delta * delta;
    }
    return float(std::bit_cast<int32_t>((terms[2] + terms[1]) + terms[0]));
}
} // namespace

bool companion(Combatant &actor, Body &body, uint64_t referenceKey, int32_t step,
               PathState &path, const CompanionServices &services) {
    const Body *reference = services.lookup(referenceKey);
    if (!reference)
        return false;
    Position delta;
    int64_t fixedSquared = 0;
    for (size_t axis = 0; axis < 3; axis++) {
        delta[axis] = difference(reference->position[axis], body.position[axis]);
        fixedSquared = add(fixedSquared, squaredTerm(delta[axis]));
    }
    actor.mode = fixedSquared > 0x400000 ? actor.mode | RUN : actor.mode & ~uint32_t(RUN);
    if (actor.controller == 5) {
        const float maximum = services.maximumHealth();
        const float milliseconds = float(step) * std::bit_cast<float>(uint32_t(0x3a83126f));
        const float share = milliseconds * std::bit_cast<float>(uint32_t(0x3dcccccd));
        const float addition = maximum * share;
        actor.health = actor.health + addition;
        const float limit = services.maximumHealth();
        if (actor.health > limit)
            actor.health = services.maximumHealth();
    }
    // Native caller reads the positions again after the conditional stat calls.
    for (size_t axis = 0; axis < 3; axis++)
        delta[axis] = difference(reference->position[axis], body.position[axis]);
    const glm::vec3 direction = scaled(delta);
    body.intent = glm::vec3(0);
    if (squared(direction, true) < 16.f)
        return false;
    const float width = body.size.x + reference->size.x;
    if (services.directRoute(body, *reference, width)) {
        services.clearPath();
        glm::vec3 intent{direction.x * 5.f, direction.y * 5.f, 0.f * 5.f};
        const float measure = squared(intent, true);
        if (measure > 900.f) {
            const float length = float(std::sqrt(double(measure)));
            const float inverse = 1.f / length;
            for (int axis = 0; axis < 3; axis++) {
                const float unit = intent[axis] * inverse;
                intent[axis] = unit * 30.f;
            }
        }
        body.intent = intent;
        return true;
    }

    Position goal{reference->position[0], reference->position[1], reference->position[2]};
    int count = 0;
    for (;;) {
        const uint8_t type = services.voxel(goal) & 31;
        if (type != 0 && type != 2)
            break;
        goal[2] = add(goal[2], -65536);
        if (++count > 20)
            return false;
    }
    count = 0;
    for (;;) {
        const uint8_t type = services.voxel(goal) & 31;
        if (type == 0 || type == 2)
            break;
        goal[2] = add(goal[2], 65536);
        if (++count > 20)
            return false;
    }
    const float halfHeight = body.size.z * .5f;
    const float fixedHalf = halfHeight * 65536.f;
    Position corrected;
    for (size_t axis = 0; axis < 3; axis++) {
        const int64_t offset = axis == 2 ? int64_t(fixedHalf) : 0;
        corrected[axis] = difference(difference(body.position[axis], offset), goal[axis]);
    }
    if (squared(scaled(corrected), false) < 16.f) {
        services.clearPath();
        body.intent = glm::vec3(0);
        return false;
    }
    if (services.pathStatus().coordinates == 0) {
        services.clearPath();
        const float offset = std::bit_cast<float>(uint32_t(0x3dcccccd)) - halfHeight;
        const float fixedOffset = offset * 65536.f;
        path.start = {body.position[0], body.position[1], add(body.position[2], int64_t(fixedOffset))};
        path.goal = goal;
        const int32_t y = std::bit_cast<int32_t>(uint32_t(goal[1] / 65536)) / 256;
        const int32_t x = std::bit_cast<int32_t>(uint32_t(goal[0] / 65536)) / 256;
        if (!services.cellPresent(x, y))
            return false;
        services.initializePath();
        path.radius = 2.f;
        services.pathStep();
        body.intent = glm::vec3(0);
    }
    if (services.pathStatus().nodes == 0)
        return true;
    for (int work = 0; work < 10; work++) {
        const PathStatus status = services.pathStatus();
        if (status.coordinates > 50)
            break;
        if (status.coordinates != 0) {
            const float radiusSquared = path.radius * path.radius;
            if (radiusSquared > nodeDistance(status.tail, path.goal))
                break;
        }
        services.pathStep();
    }
    return true;
}

void wander(Combatant &actor, Body &body, const std::array<int64_t, 3> &reference,
            int32_t &counter, int32_t step, const std::function<int32_t()> &draw) {
    actor.mode &= ~uint32_t(RUN);
    counter = std::bit_cast<int32_t>(uint32_t(counter) - uint32_t(step));
    if (counter < 0)
        counter = 0;
    if (counter != 0)
        return;
    const int64_t dx = difference(reference[0], body.position[0]);
    const int64_t dy = difference(reference[1], body.position[1]);
    const int64_t squared = std::bit_cast<int64_t>(uint64_t(squaredTerm(dx)) + uint64_t(squaredTerm(dy)));
    if (squared <= 0x640000) {
        const float x = component(draw());
        const float y = component(draw());
        body.intent.x = x;
        body.intent.z = 0.f * 10.f;
        body.intent.y = y;
    } else {
        const float x = float(dx) * (1.f / 65536.f);
        const float y = float(dy) * (1.f / 65536.f);
        const float unusedZ = float(difference(reference[2], body.position[2])) * (1.f / 65536.f);
        (void)unusedZ;
        const float x2 = x * x, y2 = y * y;
        const float length = float(std::sqrt(double(x2 + y2)));
        const float inverse = 1.f / length;
        const float unitX = x * inverse, unitY = y * inverse;
        body.intent = {unitX * 10.f, unitY * 10.f, 0};
    }
    counter = draw() % 5000 + 3000;
    actor.action = 0; // the action clock is retained
}

} // namespace creatures::created
