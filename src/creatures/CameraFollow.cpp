#include "creatures/CameraFollow.h"
#include <bit>
#include <cmath>

namespace camera_follow {
namespace {

using camera_view::add;
using camera_view::blend;
using camera_view::subtract;
using camera_view::toFixed;
using camera_view::toFloat;
using camera_view::weight;

constexpr int64_t UNIT = 65536;

float constant(uint32_t bits) { return std::bit_cast<float>(bits); }
int64_t wrap(uint64_t value) { return std::bit_cast<int64_t>(value); }
uint64_t raw(int64_t value) { return std::bit_cast<uint64_t>(value); }

float squaredLength(glm::vec3 value) {
    const float x = value.x * value.x, y = value.y * value.y;
    const float sum = x + y;
    const float z = value.z * value.z;
    return sum + z;
}

glm::vec3 scaled(glm::vec3 value, float factor) { return {value.x * factor, value.y * factor, value.z * factor}; }

glm::vec3 normalized(glm::vec3 value) {
    const float root = float(std::sqrt(double(squaredLength(value))));
    const float reciprocal = 1.f / root;
    return scaled(value, reciprocal);
}

// A raw difference times a quantized weight, divided back toward zero.
int64_t quantizedStep(int64_t difference, int64_t quantizedWeight) {
    return wrap(raw(difference) * raw(quantizedWeight)) / UNIT;
}

// The vertical blend passes both raw values through binary32 and has no
// division by the fixed unit.
int64_t rawBlend(int64_t previous, int64_t target, float amount) {
    const float complement = 1.f - amount;
    const float kept = float(previous) * complement;
    const float gained = float(target) * amount;
    return wrap(raw(int64_t(kept)) + raw(int64_t(gained)));
}

void alternativeAnchor(State &state, const Alternative &alternative) {
    state.anchor = toFixed(alternative.anchor);
    const int32_t x = int32_t(uint32_t(raw(state.anchor[0] / UNIT)));
    const int32_t y = int32_t(uint32_t(raw(state.anchor[1] / UNIT)));
    if (const auto column = alternative.column ? alternative.column(x, y) : std::nullopt) {
        const int32_t top = int32_t(uint32_t(column->base) + uint32_t(column->extent));
        state.anchor[2] = int64_t(top) * UNIT;
    }
    state.anchor[2] = wrap(raw(state.anchor[2]) + raw(50 * UNIT));
}

void ordinaryAnchor(State &state, const Subject &subject, int32_t count, bool verticalOverride) {
    const float reached = blend(0.f, 1.f, weight(count, constant(0x3ccccccd)));
    const int64_t quantized = toFixed(reached);
    for (int axis = 0; axis < 2; ++axis) {
        const int64_t difference = wrap(raw(subject.position[axis]) - raw(state.anchor[axis]));
        state.anchor[axis] = wrap(raw(state.anchor[axis]) + raw(quantizedStep(difference, quantized)));
    }
    const int64_t target = verticalTarget(subject, verticalOverride);
    state.anchor[2] = rawBlend(state.anchor[2], target, weight(count, constant(0x3ba3d70a)));
}

Fixed3 eyeBehind(const Fixed3 &anchor, glm::vec3 direction, float distance) {
    return subtract(anchor, toFixed(scaled(direction, distance)));
}

} // namespace

void mouseTargets(State &state, int32_t sensitivity, int32_t verticalDirection, float u, float v) {
    const float rate = float(sensitivity) * constant(0x3ba3d70a);
    const float vertical = rate * v;
    float &first = state.targetAngles[0];
    first = verticalDirection == 0 ? first + vertical : first - vertical;
    if (first > 180.f)
        first = 180.f;
    if (first < 0.f)
        first = 0.f;
    const float horizontal = rate * u;
    state.targetAngles[2] = state.targetAngles[2] - horizontal;
}

int32_t wheelMessage(int16_t highWord) { return highWord < 0 ? -1 : highWord > 0 ? 1 : 0; }

void wheel(State &state, int32_t argument, bool scaleMode) {
    if (!scaleMode) {
        const int32_t doubled = std::bit_cast<int32_t>(std::bit_cast<uint32_t>(argument) * 2u);
        float &target = state.targetDistance;
        target = target - float(doubled);
        if (target < 0.f)
            target = 0.f;
        if (target > 14.f)
            target = 14.f;
        return;
    }
    const float factor = constant(0x3f666666);
    float &target = state.targetScale;
    target = argument < 0 ? target * factor : target / factor;
    if (target > 10.f)
        target = 10.f;
    if (target < constant(0x3c23d70a))
        target = constant(0x3c23d70a);
}

double platformPower(double base, double exponent) { return std::pow(base, exponent); }

void smooth(State &state, int32_t count, int32_t smoothingOption, const Power &power) {
    const float exponent = float(smoothingOption) * constant(0x3c23d70a);
    const float coefficient = float(power(double(constant(0x3ba3d70a)), double(exponent)));
    const float angleWeight = weight(count, coefficient);
    for (int i = 0; i < 3; ++i)
        state.angles[i] = blend(state.angles[i], state.targetAngles[i], angleWeight);
    state.distance = blend(state.distance, state.targetDistance, weight(count, constant(0x3b83126f)));
    state.scale = blend(state.scale, state.targetScale, weight(count, constant(0x3b83126f)));
}

glm::vec3 offsetDirection(const std::array<float, 3> &angles, const Trig &trig) {
    camera_view::Matrix orientation = camera_view::identity();
    camera_view::rotateRows01(orientation, angles[2], trig);
    camera_view::rotateRows02(orientation, angles[1], trig);
    camera_view::rotateRows12(orientation, angles[0], trig);
    return camera_view::transformDirection(orientation, {0.f, 0.f, 1.f});
}

int64_t verticalTarget(const Subject &subject, bool verticalOverride) {
    const int64_t lowered = wrap(raw(subject.position[2]) - raw(toFixed(subject.verticalOffset)));
    if (verticalOverride)
        return lowered;
    const bool lying = subject.action == 0x53 || subject.action == 0x54;
    const float half = (lying ? subject.extent[0] : subject.extent[2]) * .5f;
    int64_t target = wrap(raw(lowered) + raw(toFixed(half)));
    if (!lying) {
        const bool raised = subject.action == 0x6a || subject.action == 0x6b;
        target = wrap(raw(target) + raw(toFixed(raised ? 1.5f : .5f)));
    }
    return target;
}

void followAnchor(State &state, const Subject &subject, int32_t count, const AnchorGates &gates) {
    // Strictly beyond fifty units the whole anchor snaps; later work still runs.
    if (squaredLength(toFloat(subtract(state.anchor, subject.position))) > 2500.f)
        state.anchor = subject.position;
    if (gates.alternative)
        alternativeAnchor(state, *gates.alternative);
    else
        ordinaryAnchor(state, subject, count, gates.verticalOverride);
}

int32_t voxelCoordinate(int64_t value) {
    const int64_t adjusted = value < 0 ? wrap(raw(value) - raw(UNIT)) : value;
    return int32_t(uint32_t(raw(adjusted / UNIT)));
}

bool blocks(uint8_t classification) {
    const uint8_t kind = classification & 0x1f;
    return kind != 0 && kind != 2;
}

void placeEye(State &state, const Fixed3 &subjectPosition, glm::vec3 direction, const Queries &queries) {
    const float distance = state.distance;
    const Fixed3 first = eyeBehind(state.anchor, direction, distance);
    float allowed = distance;
    constexpr int64_t QUARTER = UNIT / 4;
    for (int corner = 0; corner < 8; ++corner) {
        const Fixed3 offset{corner & 4 ? QUARTER : -QUARTER, corner & 2 ? QUARTER : -QUARTER, corner & 1 ? QUARTER : -QUARTER};
        const Fixed3 sample = add(first, offset);
        if (!queries.blocked(sample))
            continue;
        // Every call receives the original distance, not the shortened one.
        const float travelled = float(queries.ray(sample, direction, distance));
        const float candidate = distance - travelled;
        if (allowed > candidate)
            allowed = candidate;
    }
    state.distance = allowed;
    state.eye = eyeBehind(state.anchor, direction, allowed);
    state.retainedDistance = allowed;

    if (!queries.blocked(state.eye))
        return;
    const glm::vec3 outward = toFloat(subtract(state.eye, subjectPosition));
    if (!(squaredLength(outward) > 0.f))
        return;
    const glm::vec3 unit = normalized(outward);
    const float reached = float(queries.ray(subjectPosition, unit, 100.f));
    if (reached >= 0.f) // negative and unordered results keep the eye
        state.eye = add(subjectPosition, toFixed(scaled(unit, reached)));
}

} // namespace camera_follow
